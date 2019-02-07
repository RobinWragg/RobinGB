/* TODO: Remove assert()s */
/* TODO: Add formatting cabability to robingb_log() */

#ifndef ROBINGB_H
#define ROBINGB_H

#include <stdint.h>
#include <stdbool.h>

#define ROBINGB_AUDIO_SAMPLE_RATE (44100)

#define ROBINGB_DPAD_NONE 0
#define ROBINGB_DPAD_RIGHT 1
#define ROBINGB_DPAD_UPRIGHT 2
#define ROBINGB_DPAD_UP 3
#define ROBINGB_DPAD_UPLEFT 4
#define ROBINGB_DPAD_LEFT 5
#define ROBINGB_DPAD_DOWNLEFT 6
#define ROBINGB_DPAD_DOWN 7
#define ROBINGB_DPAD_DOWNRIGHT 8

typedef enum {
	ROBINGB_UP,
	ROBINGB_LEFT,
	ROBINGB_RIGHT,
	ROBINGB_DOWN,
	ROBINGB_A,
	ROBINGB_B,
	ROBINGB_START,
	ROBINGB_SELECT
} RobinGB_Button;

void robingb_press_button(RobinGB_Button button);
void robingb_release_button(RobinGB_Button button);

/* Set this to your logging function if you want to receive logs from RobinGB. */
extern void (*robingb_logging_function)(const char *text);

void robingb_init(
	const char *cart_file_path,
	void (*read_file_function_ptr)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[])
	);
void robingb_update(uint8_t screen_out[], uint8_t *ly_out);
void robingb_read_next_audio_sample(int16_t *l, int16_t *r);

#endif
