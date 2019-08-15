/* TODO: Remove assert()s */
/* TODO: Remove mallocs and instead obtain pre-allocated memory from the user. */
/* TODO: Add error logging. */
/* TODO: Fail gracefully if malloc() returns NULL. */
/* TODO: Remove printf(). */

#ifndef ROBINGB_H
#define ROBINGB_H

#include <stdint.h>
#include <stdbool.h>

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

void robingb_init(
	uint32_t audio_sample_rate,
	uint16_t audio_buffer_size,
	
	/* TODO: The emulator will only call the user-given read-file function with the user-given cart file path, so just remove the paths from all of that. */
	const char *cart_file_path,
	void (*read_file_function_ptr)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[])
	);
void robingb_update(uint8_t screen_out[], uint8_t *ly_out);
void robingb_read_next_audio_sample(int16_t *l, int16_t *r);

#endif
