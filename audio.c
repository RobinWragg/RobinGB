#include "internal.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/* Allow the user to set the sample rate. */

/* DIV and AudioPU */
/* - The APU uses the DIV to update sweep (channel 1), fade in/out and time out, the same way the timer uses it to update itself. */
/* - In normal speed mode the APU updates when bit 5 of DIV goes from 1 to 0 (256 Hz). In double speed mode, bit 6. */
/* - Writing to DIV every instruction, for example, will make the APU produce the same frequency with the same volume even if sweep and fade out are enabled. */
/* - Writing to DIV doesn't affect the frequency itself. The waveform generation is driven by another timer. */

#define CHAN3_WAVE_PATTERN_LENGTH (32)
#define CPU_CLOCK_FREQ (4194304)
#define STEPS_PER_ENVELOPE (16)

typedef struct {
	s16 l, r;
} RobinGB_Sample;

static RobinGB_Sample *sample_buffer = NULL;
static u16 sample_buffer_size = 0;

static void get_channel_volume_envelope(s8 channel, f32 *initial_volume, bool *direction_is_increase, s32 *step_length_in_cycles) {
	int volume_envelope_address;
	
	if (channel == 1 ) {
		volume_envelope_address = 0xff12;
	} else if (channel == 2) {
		volume_envelope_address = 0xff17;
	} else assert(false);
	
	u8 envelope_byte = robingb_memory_read(volume_envelope_address);
	
	u8 initial_volume_specifier = envelope_byte >> 4; /* can be 0 to 15 */
	*initial_volume = initial_volume_specifier / 15.0f;
	
	*direction_is_increase = envelope_byte & 0x08;
	
	u8 step_size_specifier = envelope_byte & 0x07;
	*step_length_in_cycles = step_size_specifier * (CPU_CLOCK_FREQ / 64);
}

static void get_channel_freq_and_restart_and_envelope_stop(int channel, s16 *freq, bool *should_restart, bool *should_stop_at_envelope_end) {
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
	
	u16 freq_specifier = robingb_memory_read(lower_freq_bits_address); /* lower 8 bits of frequency */
	u8 restart_and_stop_byte = robingb_memory_read(upper_freq_bits_and_restart_and_stop_address);
	robingb_memory_write(upper_freq_bits_and_restart_and_stop_address, restart_and_stop_byte & ~0x80); /* reset the restart flag */
	
	freq_specifier |= (restart_and_stop_byte & 0x07) << 8; /* upper 3 bits of frequency */
	
	if (channel == 3) *freq = 65536.0 / (2048 - freq_specifier);
	else *freq = 131072.0 / (2048 - freq_specifier);
	
	*should_restart = restart_and_stop_byte & 0x80;
	*should_stop_at_envelope_end = restart_and_stop_byte & 0x40; /* TODO: untested */
}

static bool chan3_is_enabled() {
	return robingb_memory_read(0xff1a) & bit(7);
}

static void get_chan3_wave_pattern(s8 pattern_out[]) {
	u8 volume_byte = (robingb_memory_read(0xff1c) & 0x60) >> 5;
	
	if (volume_byte) {
		f32 volume;
		switch (volume_byte) {
			case 1: volume = 1; break;
			case 2: volume = 0.5; break;
			case 3: volume = 0.25; break;
			default: assert(false); break;
		}
		
		int i;
		for (i = 0; i < CHAN3_WAVE_PATTERN_LENGTH; i += 2) {
			u8 value = robingb_memory_read(0xff30 + i/2);
			pattern_out[i] = (((value & 0xf0) >> 4) * 16 - 128) * volume;
			pattern_out[i+1] = ((value & 0x0f) * 16 - 128) * volume;
		}
	} else {
		int i;
		for (i = 0; i < CHAN3_WAVE_PATTERN_LENGTH; i++) {
			pattern_out[i] = 0;
		}
	}
}

static void update_channel_2(int num_cycles) {
	
	// static f32 period_position = 0;
	// static f32 current_volume = 1;
	// static u32 num_samples_since_restart = 0;
	
	f32 initial_volume;
	bool direction_is_increase;
	s32 envelope_step_length_in_cycles;
	get_channel_volume_envelope(2, &initial_volume, &direction_is_increase, &envelope_step_length_in_cycles);
	
	s16 freq;
	bool should_restart;
	bool should_stop_at_envelope_end;
	get_channel_freq_and_restart_and_envelope_stop(2, &freq, &should_restart, &should_stop_at_envelope_end);
	
	// if (should_restart) {
	// 	current_volume = initial_volume;
	// 	num_samples_since_restart = 0;
	// 	period_position = 0;
	// }
	
	// if (direction_is_increase) {
	// 	current_volume = lerp(initial_volume, 1, num_samples_since_restart / (f32)envelope_length_in_samples);
	// } else {
	// 	current_volume = lerp(initial_volume, 0, num_samples_since_restart / (f32)envelope_length_in_samples);
	// }
	
	// num_samples_since_restart++;
	
	// ring[ring_write_index].l += (period_position > 0.5 ? 127 : -128) * current_volume;
	// ring[ring_write_index].r += (period_position > 0.5 ? 127 : -128) * current_volume;
	
	// period_position += (1.0/ROBINGB_AUDIO_SAMPLE_RATE) * freq;
	// while (period_position > 1.0) period_position -= 1.0;
}

void robingb_audio_init(uint32_t sample_rate, uint16_t buffer_size_param) {
	sample_buffer_size = buffer_size_param;
	assert(sample_buffer_size > 0);
	
	sample_buffer = (RobinGB_Sample*)malloc(sizeof(RobinGB_Sample) * sample_buffer_size);
	assert(sample_buffer);
}

void robingb_audio_update(int num_cycles_this_update) {
	static s16 accumulated_cycles = 0; /* This carries over to the next update */
	
	/* If we produce a sample every 87 cycles, we produce a sample rate of just over 48000Hz,
	because CPU_CLOCK_FREQ / 48000Hz = 87.381 cycles. The speed of video and audio will diverge; that's just that nature of real hardware. So instead */
	const int NUM_CYCLES_PER_SAMPLE = 87;
	
	accumulated_cycles += num_cycles_this_update;
	
	if (accumulated_cycles >= NUM_CYCLES_PER_SAMPLE) {
		// update_channel_1(NUM_CYCLES_PER_SAMPLE);
		update_channel_2(NUM_CYCLES_PER_SAMPLE);
		// update_channel_3(NUM_CYCLES_PER_SAMPLE);
		// update_channel_4(NUM_CYCLES_PER_SAMPLE);
		
		accumulated_cycles -= NUM_CYCLES_PER_SAMPLE;
	}
}

void robingb_read_next_audio_sample(s16 *l, s16 *r) {
	*l = 0;
	*r = 0;
}









