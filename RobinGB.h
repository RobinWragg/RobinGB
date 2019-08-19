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
	
	/* TODO: The emulator will only call the user-given read-file function with the user-given cart file path, so just remove the paths from all of that. */
	const char *cart_file_path,
	void (*read_file_function_ptr)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[])
	);
void robingb_update(uint8_t screen_out[], uint8_t *ly_out);

/* Stereo samples are stored in 8-bit interleaved format (LRLRLRLR...). Note that if you want
to get 256 samples, you'll need to give this function an array of 512 int8_t elements. This
is double because of the two channels (left and right). */
void robingb_get_audio_samples(int8_t samples_out[], uint16_t samples_count);

#endif



