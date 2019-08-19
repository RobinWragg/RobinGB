#include "internal.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* Allow the user to set the sample rate. */

/* DIV and AudioPU */
/* - The APU uses the DIV to update sweep (channel 1), fade in/out and time out, the same way the timer uses it to update itself. */
/* - In normal speed mode the APU updates when bit 5 of DIV goes from 1 to 0 (256 Hz). In double speed mode, bit 6. */
/* - Writing to DIV every instruction, for example, will make the APU produce the same frequency with the same volume even if sweep and fade out are enabled. */
/* - Writing to DIV doesn't affect the frequency itself. The waveform generation is driven by another timer. */

#define CHAN3_WAVE_PATTERN_LENGTH (32)
#define CPU_CLOCK_FREQ (4194304)
#define STEPS_PER_ENVELOPE (16)
#define PHASE_FULL_PERIOD (65536)
#define MAX_VOLUME (15)

u16 SAMPLE_RATE = 0;

static void get_channel_volume_envelope(
	u8 channel, u8 *initial_volume, bool *direction_is_increase, s32 *step_length_in_cycles) {
	
	int volume_envelope_address;
	
	if (channel == 1 ) {
		volume_envelope_address = 0xff12;
	} else if (channel == 2) {
		volume_envelope_address = 0xff17;
	} else assert(false);
	
	u8 envelope_byte = robingb_memory_read(volume_envelope_address);
	
	*initial_volume = envelope_byte >> 4; /* can be 0 to 15 */
	
	*direction_is_increase = envelope_byte & 0x08;
	
	u8 step_size_specifier = envelope_byte & 0x07;
	*step_length_in_cycles = step_size_specifier * (CPU_CLOCK_FREQ / 64);
}

static void get_channel_freq_and_restart_and_envelope_stop(
	u8 channel, u16 *freq, bool *should_restart, bool *should_stop_at_envelope_end) {
	
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
		float volume;
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

typedef struct {
	u16 volume;
	
	/* Where we would normally use a float wraps around at 2*M_PI, I use an unsigned 16-bit int.
	This is more efficient as it auto-wraps, and float calculations are slow without an FPU. */
	u16 phase; 
	u16 frequency;
	u64 num_cycles_since_restart;
} robingb_Channel;

robingb_Channel channel_1;
robingb_Channel channel_2;

static void update_channel_1(u32 num_cycles) {
	
	u8 initial_volume;
	bool direction_is_increase;
	s32 envelope_step_length_in_cycles;
	get_channel_volume_envelope(
		1,
		&initial_volume,
		&direction_is_increase,
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
		channel_1.phase = 0;
	}
	
	channel_1.volume = initial_volume;
	
	/* Set the current volume according to the envelope */
	if (envelope_step_length_in_cycles != 0) {
		u32 current_step = channel_1.num_cycles_since_restart / envelope_step_length_in_cycles;
		
		if (direction_is_increase) channel_1.volume += current_step;
		else channel_1.volume -= current_step;
		
		if (channel_1.volume > MAX_VOLUME) channel_1.volume = 0;
	}
	
	channel_1.num_cycles_since_restart += num_cycles;
}

static void update_channel_2(u32 num_cycles) {
	
	u8 initial_volume;
	bool direction_is_increase;
	s32 envelope_step_length_in_cycles;
	get_channel_volume_envelope(
		2,
		&initial_volume,
		&direction_is_increase,
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
		channel_2.phase = 0;
	}
	
	channel_2.volume = initial_volume;
	
	/* Set the current volume according to the envelope */
	if (envelope_step_length_in_cycles != 0) {
		u32 current_step = channel_2.num_cycles_since_restart / envelope_step_length_in_cycles;
		
		if (direction_is_increase) channel_2.volume += current_step;
		else channel_2.volume -= current_step;
		
		if (channel_2.volume > MAX_VOLUME) channel_2.volume = 0;
	}
	
	channel_2.num_cycles_since_restart += num_cycles;
}

void robingb_audio_init(u32 sample_rate) {
	SAMPLE_RATE = sample_rate;
	
	/* Test for unsigned wraparound */
	u8 wrapper = 0;
	wrapper += PHASE_FULL_PERIOD + 1;
	
	if (wrapper != 1) {
		/* TODO: Report that the platform doesn't wraparound unsigned bytes. */
	}
}

void robingb_audio_update(u32 num_cycles) {
	update_channel_1(num_cycles);
	update_channel_2(num_cycles);
	/* update_channel_3(num_cycles); */
	/* update_channel_4(num_cycles); */
}

void robingb_get_audio_samples(s8 samples_out[], uint16_t samples_count) {
	
	u16 channel_1_phase_increment = (PHASE_FULL_PERIOD * channel_1.frequency) / SAMPLE_RATE;
	u16 channel_2_phase_increment = (PHASE_FULL_PERIOD * channel_2.frequency) / SAMPLE_RATE;
	
	int s;
	for (s = 0; s < samples_count; s++) {
		s8 channel_1_sample = channel_1.volume;
		if (channel_1.phase < (PHASE_FULL_PERIOD/2)) channel_1_sample *= -1;
		channel_1.phase += channel_1_phase_increment;
		
		s8 channel_2_sample = channel_2.volume;
		if (channel_2.phase < (PHASE_FULL_PERIOD/2)) channel_2_sample *= -1;
		channel_2.phase += channel_2_phase_increment;
		
		s8 master_sample = 0;
		master_sample += channel_1_sample;
		master_sample += channel_2_sample;
		
		samples_out[s*2] = master_sample;
		samples_out[s*2+1] = master_sample;
	}
}









