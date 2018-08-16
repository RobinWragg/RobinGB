#include "internal.h"

// DIV and AudioPU
// - The APU uses the DIV to update sweep (channel 1), fade in/out and time out, the same way the timer uses it to update itself.
// - In normal speed mode the APU updates when bit 5 of DIV goes from 1 to 0 (256 Hz). In double speed mode, bit 6.
// - Writing to DIV every instruction, for example, will make the APU produce the same frequency with the same volume even if sweep and fade out are enabled.
// - Writing to DIV doesn't affect the frequency itself. The waveform generation is driven by another timer.

#define RING_SIZE (2048) // TODO: set this low to improve latency
#define CHAN3_WAVE_PATTERN_LENGTH (32)

struct {
	s16 l, r;
} ring[RING_SIZE];

int ring_read_index = 0;
int ring_write_index = 0;

void get_chan_volume_envelope(int channel, f32 *initial_volume, bool *direction_is_increase, u32 *envelope_length_in_samples) {
	int volume_envelope_address;
	
	if (channel == 1 ) {
		volume_envelope_address = 0xff12;
	} else if (channel == 2) {
		volume_envelope_address = 0xff17;
	} else assert(false);
	
	u8 envelope_byte = mem_read(volume_envelope_address);
	
	*initial_volume = (envelope_byte >> 4) * (1.0/15);
	*direction_is_increase = envelope_byte & 0x08;
	f32 envelope_step_in_seconds = (envelope_byte & 0x07) * (1.0/64);
	f32 envelope_length_in_seconds = envelope_step_in_seconds * 16; // 16 steps across an envelope
	*envelope_length_in_samples = envelope_length_in_seconds * ROBINGB_AUDIO_SAMPLE_RATE;
}

void get_chan_freq_and_restart_and_envelope_stop(int channel, s16 *freq, bool *should_restart, bool *should_stop_at_envelope_end) {
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
	
	u16 freq_specifier = mem_read(lower_freq_bits_address); // lower 8 bits of frequency
	u8 restart_and_stop_byte = mem_read(upper_freq_bits_and_restart_and_stop_address);
	mem_write(upper_freq_bits_and_restart_and_stop_address, restart_and_stop_byte & ~0x80); // reset the restart flag
	
	freq_specifier |= (restart_and_stop_byte & 0x07) << 8; // upper 3 bits of frequency
	
	if (channel == 3) *freq = 65536.0 / (2048 - freq_specifier);
	else *freq = 131072.0 / (2048 - freq_specifier);
	
	*should_restart = restart_and_stop_byte & 0x80;
	*should_stop_at_envelope_end = restart_and_stop_byte & 0x40; // TODO: untested
}

bool chan3_is_enabled() {
	return mem_read(0xff1a) & bit(7);
}

void get_chan3_wave_pattern(s8 pattern_out[]) {
	u8 volume_byte = (mem_read(0xff1c) & 0x60) >> 5;
	
	if (volume_byte) {
		f32 volume;
		switch (volume_byte) {
			case 1: volume = 1; break;
			case 2: volume = 0.5; break;
			case 3: volume = 0.25; break;
			default: assert(false); break;
		}
		
		for (int i = 0; i < CHAN3_WAVE_PATTERN_LENGTH; i += 2) {
			u8 value = mem_read(0xff30 + i/2);
			pattern_out[i] = (((value & 0xf0) >> 4) * 16 - 128) * volume;
			pattern_out[i+1] = ((value & 0x0f) * 16 - 128) * volume;
		}
	} else {
		for (int i = 0; i < CHAN3_WAVE_PATTERN_LENGTH; i++) {
			pattern_out[i] = 0;
		}
	}
}

void robingb_read_next_audio_sample(s16 *l, s16 *r) {
	*l = ring[ring_read_index].l;
	*r = ring[ring_read_index].r;
	if (++ring_read_index >= RING_SIZE) ring_read_index = 0;
}

// TODO: use a step-based envelope instead of lerping
f32 lerp(f32 a, f32 b, f32 t) {
	if (t > 1) t = 1;
	if (t < 0) t = 0;
	return a + (b-a)*t;
}

void write_next_sample() {
	ring[ring_write_index].l = 0;
	ring[ring_write_index].r = 0;
	
	// channel 1
	if (true) {
		static f32 period_position = 0;
		static f32 current_volume = 1;
		static u32 num_samples_since_restart = 0;
		
		f32 initial_volume;
		bool direction_is_increase;
		u32 envelope_length_in_samples;
		get_chan_volume_envelope(1, &initial_volume, &direction_is_increase, &envelope_length_in_samples);
		
		s16 freq;
		bool should_restart;
		bool should_stop_at_envelope_end;
		get_chan_freq_and_restart_and_envelope_stop(1, &freq, &should_restart, &should_stop_at_envelope_end);
		
		if (should_restart) {
			current_volume = initial_volume;
			num_samples_since_restart = 0;
			period_position = 0;
		}
		
		if (direction_is_increase) {
			current_volume = lerp(initial_volume, 1, num_samples_since_restart / (f32)envelope_length_in_samples);
		} else {
			current_volume = lerp(initial_volume, 0, num_samples_since_restart / (f32)envelope_length_in_samples);
		}
		
		num_samples_since_restart++;
		
		ring[ring_write_index].l += (period_position > 0.5 ? 127 : -128) * current_volume;
		ring[ring_write_index].r += (period_position > 0.5 ? 127 : -128) * current_volume;
		
		period_position += (1.0/ROBINGB_AUDIO_SAMPLE_RATE) * freq;
		while (period_position > 1.0) period_position -= 1.0;
	}
	
	// channel 2
	if (true) {
		static f32 period_position = 0;
		static f32 current_volume = 1;
		static u32 num_samples_since_restart = 0;
		
		f32 initial_volume;
		bool direction_is_increase;
		u32 envelope_length_in_samples;
		get_chan_volume_envelope(2, &initial_volume, &direction_is_increase, &envelope_length_in_samples);
		
		s16 freq;
		bool should_restart;
		bool should_stop_at_envelope_end;
		get_chan_freq_and_restart_and_envelope_stop(2, &freq, &should_restart, &should_stop_at_envelope_end);
		
		if (should_restart) {
			current_volume = initial_volume;
			num_samples_since_restart = 0;
			period_position = 0;
		}
		
		if (direction_is_increase) {
			current_volume = lerp(initial_volume, 1, num_samples_since_restart / (f32)envelope_length_in_samples);
		} else {
			current_volume = lerp(initial_volume, 0, num_samples_since_restart / (f32)envelope_length_in_samples);
		}
		
		num_samples_since_restart++;
		
		ring[ring_write_index].l += (period_position > 0.5 ? 127 : -128) * current_volume;
		ring[ring_write_index].r += (period_position > 0.5 ? 127 : -128) * current_volume;
		
		period_position += (1.0/ROBINGB_AUDIO_SAMPLE_RATE) * freq;
		while (period_position > 1.0) period_position -= 1.0;
	}
	
	// channel 3
	if (true) {
		static f32 period_position = 0;
		static u32 num_samples_since_restart = 0;
		
		if (chan3_is_enabled()) {
			s16 freq;
			bool should_restart;
			bool should_stop_at_envelope_end;
			get_chan_freq_and_restart_and_envelope_stop(3, &freq, &should_restart, &should_stop_at_envelope_end);
			
			s8 wave_pattern[CHAN3_WAVE_PATTERN_LENGTH];
			get_chan3_wave_pattern(wave_pattern);
			
			u32 pattern_sample_index = period_position * CHAN3_WAVE_PATTERN_LENGTH;
			pattern_sample_index %= CHAN3_WAVE_PATTERN_LENGTH;
			
			ring[ring_write_index].l += wave_pattern[pattern_sample_index];
			ring[ring_write_index].r += wave_pattern[pattern_sample_index];
			
			num_samples_since_restart++;
			
			period_position += (1.0/ROBINGB_AUDIO_SAMPLE_RATE) * freq;
			while (period_position > 1.0) period_position -= 1.0;
		}
	}
	
	// make sure we don't clip. 16 bit max / (8 bit max * number of channels)
	ring[ring_write_index].l *= 32768.0 / (128*4);
	ring[ring_write_index].r *= 32768.0 / (128*4);
	
	if (++ring_write_index >= RING_SIZE) ring_write_index = 0;
}

void update_audio(int num_cycles) {
	for (int i = 0; i < RING_SIZE; i++) {
		if (ring_write_index != ring_read_index) write_next_sample();
		else break;
	}
}









