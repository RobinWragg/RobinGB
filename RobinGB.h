// TODO: Add a print/log function
// TODO: Remove assert()s
// TODO: Add formatting cabability to robingb_log()

#ifndef ROBINGB_H
#define ROBINGB_H

#include <stdint.h>
#include <stdbool.h>

#define ROBINGB_CPU_CYCLES_PER_REFRESH (70224) // Approximately 59.7275Hz
#define ROBINGB_AUDIO_SAMPLE_RATE (44100)

typedef struct {
	bool up, left, right, down;
	bool start, select, a, b;
} RobinGB_Input;

// Set this to your logging function if you want to receive logs from RobinGB.
extern void (*robingb_logging_function_ptr)(const char *text);

void robingb_init(
	const char *cart_file_path,
	void (*read_file_function_ptr)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[])
	);
void robingb_update(RobinGB_Input *input, uint8_t screen_out[], uint8_t *ly_out);
void robingb_read_next_audio_sample(int16_t *l, int16_t *r);

#endif
