#include "internal.h"

#include <assert.h>
#include <string.h>

#define CHANNEL_3_WAVE_PATTERN_LENGTH (32)
#define CPU_CLOCK_FREQ (4194304)
#define STEPS_PER_ENVELOPE (16)
#define PHASE_FULL_PERIOD (4294967296)
#define MAX_VOLUME (15)

uint16_t SAMPLE_RATE = 0;

static void get_channel_volume_envelope(
    uint8_t channel, uint8_t *initial_volume, bool *is_increasing, int32_t *step_length_in_cycles) {
    
    int volume_envelope_address;
    
    if (channel == 1 ) {
        volume_envelope_address = 0xff12;
    } else if (channel == 2) {
        volume_envelope_address = 0xff17;
    } else assert(false);
    
    uint8_t envelope_byte = robingb_memory[volume_envelope_address];
    
    *initial_volume = envelope_byte >> 4; /* can be 0 to 15 */
    
    *is_increasing = envelope_byte & 0x08;
    
    uint8_t step_size_specifier = envelope_byte & 0x07;
    *step_length_in_cycles = step_size_specifier * (CPU_CLOCK_FREQ / 64);
}

static void get_channel_freq_and_restart_and_envelope_stop(
    uint8_t channel, uint16_t *freq, bool *should_restart, bool *should_stop_at_envelope_end) {
    
    int lower_freq_bits_address;
    int upper_freq_bits_and_restart_and_stop_address;
    
    if (channel == 1 ) {
        lower_freq_bits_address = 0xff13;
        upper_freq_bits_and_restart_and_stop_address = 0xff14;
    } else if (channel == 2) {
        lower_freq_bits_address = 0xff18;
        upper_freq_bits_and_restart_and_stop_address = 0xff19;
    } else if (channel == 3) {
        lower_freq_bits_address = 0xff1d;
        upper_freq_bits_and_restart_and_stop_address = 0xff1e;
    } else assert(false);
    
    uint16_t freq_specifier = robingb_memory[lower_freq_bits_address]; /* lower 8 bits of frequency */
    uint8_t restart_and_stop_byte = robingb_memory[upper_freq_bits_and_restart_and_stop_address];
    robingb_memory_write(upper_freq_bits_and_restart_and_stop_address, restart_and_stop_byte & ~0x80); /* reset the restart flag */
    
    freq_specifier |= (restart_and_stop_byte & 0x07) << 8; /* upper 3 bits of frequency */
    
    if (channel == 3) *freq = 65536 / (2048 - freq_specifier);
    else *freq = 131072 / (2048 - freq_specifier);
    
    *should_restart = restart_and_stop_byte & 0x80;
    *should_stop_at_envelope_end = restart_and_stop_byte & 0x40; /* TODO: untested */
}

struct {
    uint16_t volume;
    
    /* Where we would normally use a float wraps around at 2*M_PI, I use an unsigned 32-bit int.
    This is more efficient as it auto-wraps, and float calculations are slow without an FPU. */
    uint32_t phase; 
    uint16_t frequency;
    uint64_t num_cycles_since_restart; // TODO: Avoid 64 bit?
} channel_1;

static void handle_channel_1_sweep(uint32_t num_cycles) {
    
    uint32_t step_interval_in_cycles;
    bool is_increasing;
    uint8_t step_amount_divider;
    {
        uint8_t sweep_byte = robingb_memory[0xff10];
        uint8_t time_index = (sweep_byte >> 4) & 0x07;
        
        if (time_index == 0) return; /* sweeping is disabled */
        
        step_interval_in_cycles = time_index * (CPU_CLOCK_FREQ / 128);
        is_increasing = !((sweep_byte >> 3) & 0x01);
        step_amount_divider = 1 << (sweep_byte & 0x07);
    }
    
    /* sweeping is enabled */
    
    uint8_t num_steps = channel_1.num_cycles_since_restart / step_interval_in_cycles;
    
    uint16_t frequency_delta = (channel_1.frequency / step_amount_divider) * num_steps;
    
    if (is_increasing) channel_1.frequency += frequency_delta;
    else channel_1.frequency -= frequency_delta;
}

static void update_channel_1(uint32_t num_cycles) {
    
    uint8_t initial_volume;
    bool volume_is_increasing;
    int32_t envelope_step_length_in_cycles;
    get_channel_volume_envelope(
        1,
        &initial_volume,
        &volume_is_increasing,
        &envelope_step_length_in_cycles);
    
    bool should_restart;
    bool should_stop_at_envelope_end;
    get_channel_freq_and_restart_and_envelope_stop(
        1,
        &channel_1.frequency,
        &should_restart,
        &should_stop_at_envelope_end);
    
    if (should_restart) {
        channel_1.num_cycles_since_restart = 0;
    }
    
    handle_channel_1_sweep(num_cycles);
    
    /* Modify the volume according to the envelope */
    channel_1.volume = initial_volume;
    
    if (envelope_step_length_in_cycles != 0) {
        uint32_t current_step = channel_1.num_cycles_since_restart / envelope_step_length_in_cycles;
        
        if (volume_is_increasing) channel_1.volume += current_step;
        else channel_1.volume -= current_step;
        
        if (channel_1.volume > MAX_VOLUME) channel_1.volume = 0;
    }
    
    channel_1.num_cycles_since_restart += num_cycles;
}

struct {
    uint16_t volume;
    uint32_t phase;
    uint16_t frequency;
    uint64_t num_cycles_since_restart; // TODO: Avoid 64 bit?
} channel_2;

static void update_channel_2(uint32_t num_cycles) {
    
    uint8_t initial_volume;
    bool volume_is_increasing;
    int32_t envelope_step_length_in_cycles;
    get_channel_volume_envelope(
        2,
        &initial_volume,
        &volume_is_increasing,
        &envelope_step_length_in_cycles);
    
    bool should_restart;
    bool should_stop_at_envelope_end;
    get_channel_freq_and_restart_and_envelope_stop(
        2,
        &channel_2.frequency,
        &should_restart,
        &should_stop_at_envelope_end);
    
    if (should_restart) {
        channel_2.num_cycles_since_restart = 0;
    }
    
    channel_2.volume = initial_volume;
    
    /* Set the current volume according to the envelope */
    if (envelope_step_length_in_cycles != 0) {
        uint32_t current_step = channel_2.num_cycles_since_restart / envelope_step_length_in_cycles;
        
        if (volume_is_increasing) channel_2.volume += current_step;
        else channel_2.volume -= current_step;
        
        if (channel_2.volume > MAX_VOLUME) channel_2.volume = 0;
    }
    
    channel_2.num_cycles_since_restart += num_cycles;
}

struct {
    uint16_t frequency;
    uint32_t phase;
    int8_t wave_pattern[CHANNEL_3_WAVE_PATTERN_LENGTH];
} channel_3;

static void get_channel_3_wave_pattern(int8_t pattern_out[]) {
    bool channel_enabled = robingb_memory[0xff1a] & 0x80;
    uint8_t volume_byte = (robingb_memory[0xff1c] & 0x60) >> 5;
    
        
    if (channel_enabled && volume_byte) {
        volume_byte -= 1;
        
        int i;
        for (i = 0; i < CHANNEL_3_WAVE_PATTERN_LENGTH; i += 2) {
            uint8_t value = robingb_memory[0xff30 + i/2];
            
            pattern_out[i] = (value & 0xf0) >> (4 + volume_byte);
            pattern_out[i+1] = (value & 0x0f) >> volume_byte;
        }
    } else memset(pattern_out, 0, CHANNEL_3_WAVE_PATTERN_LENGTH);
}

static void update_channel_3(uint32_t num_cycles) {
    
    get_channel_3_wave_pattern(channel_3.wave_pattern);
    
    bool should_restart;
    bool should_stop_at_envelope_end;
    get_channel_freq_and_restart_and_envelope_stop(
        3,
        &channel_3.frequency,
        &should_restart,
        &should_stop_at_envelope_end);
}

void robingb_audio_init(uint32_t sample_rate) {
    SAMPLE_RATE = sample_rate;
    
    /* Test for unsigned wraparound */
    uint32_t wrapper = 0;
    wrapper += PHASE_FULL_PERIOD + 1;
    
    if (wrapper != 1) {
        /* TODO: Report that the platform doesn't wraparound unsigned ints. */
    }
}

void robingb_audio_update(uint32_t num_cycles) {
    update_channel_1(num_cycles);
    update_channel_2(num_cycles);
    update_channel_3(num_cycles);
    /* update_channel_4(num_cycles); */
}

void robingb_get_audio_samples(int8_t samples_out[], uint16_t samples_count) {
    
    const uint32_t CHANNEL_1_PHASE_INCREMENT = (PHASE_FULL_PERIOD / SAMPLE_RATE) * channel_1.frequency;
    const uint32_t CHANNEL_2_PHASE_INCREMENT = (PHASE_FULL_PERIOD / SAMPLE_RATE) * channel_2.frequency;
    const uint32_t CHANNEL_3_PHASE_INCREMENT = (PHASE_FULL_PERIOD / SAMPLE_RATE) * channel_3.frequency;
    
    int s;
    
    for (s = 0; s < samples_count; s++) {
        samples_out[s] = (channel_1.phase < PHASE_FULL_PERIOD/2) ? 0 : channel_1.volume;
        channel_1.phase += CHANNEL_1_PHASE_INCREMENT;
        
        if (channel_2.phase < PHASE_FULL_PERIOD/2) samples_out[s] += channel_2.volume;
        channel_2.phase += CHANNEL_2_PHASE_INCREMENT;
        
        samples_out[s] += channel_3.wave_pattern[channel_3.phase >> 27];
        channel_3.phase += CHANNEL_3_PHASE_INCREMENT;
    }
}









