/*
______      _     _       _____ ______
| ___ \    | |   (_)     |  __ \| ___ \
| |_/ /___ | |__  _ _ __ | |  \/| |_/ /
|    // _ \| '_ \| | '_ \| | __ | ___ \
| |\ \ (_) | |_) | | | | | |_\ \| |_/ /
\_| \_\___/|_.__/|_|_| |_|\____/\____/

Welcome! Scroll down for complete usage information.

TODO: Remove assert()s
TODO: Add error logging.
TODO: Fail gracefully if malloc() returns NULL.
TODO: Remove printf().
TODO: Implement simplified init function that doesn't require file access function pointers.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ROBINGB_H
#define ROBINGB_H

#include <stdint.h>
#include <stdbool.h>
    
#define ROBINGB_SCREEN_WIDTH 160
#define ROBINGB_SCREEN_HEIGHT 144

/* This must be called before calling any other functions. You will need to
implement read_file() and write_file() (continue reading for an example) and
pass their pointers in here. This allows RobinGB to load and save games. */
void robingb_init(
    uint32_t audio_sample_rate,
    
    const char *cart_file_path,
    bool (*read_file)(const char *path, uint32_t byte_offset, uint32_t data_size, uint8_t data_out[]),
    bool (*write_file)(const char *path, bool append, uint32_t data_size, uint8_t data_in[])
    );

/* Example implementations of read_file() and write_file():

bool read_file(const char *path, uint32_t byte_offset, uint32_t data_size, uint8_t data_out[]) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, byte_offset, SEEK_SET);
    uint32_t bytes_read = fread(data_out, sizeof(uint8_t), data_size, f);
    if (bytes_read != data_size) return false;
    fclose(f);
    return true;
}

bool write_file(const char *path, bool append, uint32_t data_size, uint8_t data_in[]) {
    FILE *f = fopen(path, append ? "ab" : "wb");
    if (!f) return false;
    uint32_t bytes_written = fwrite(data_in, sizeof(uint8_t), data_size, f);
    if (bytes_written != data_size) return false;
    fclose(f);
    return true;
}
*/

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

/* Call these functions to tell RobinGB about player input. */
void robingb_press_button(RobinGB_Button button);
void robingb_release_button(RobinGB_Button button);

/* These save and load the entire emulator state. They internally call the
write_file() and read_file() functions that were passed to robingb_init(). */
void robingb_save_state();
void robingb_load_state();

/* Run the emulation and update the screen with this function. screen[] must be
an array of ROBINGB_SCREEN_WIDTH*ROBINGB_SCREEN_HEIGHT bytes, one byte per
pixel. Call this 60 times per second to run the game at the correct speed. You
can implement fast-forwarding by calling it more frequently. */
void robingb_update_screen(uint8_t screen[]);
/* NOTE: If your platform can't afford to block for the amount of time that
this function takes, see the lower-level robingb_update_screen_line() function
at the bottom of this file. */

/* Call this to fill your audio output buffer as and when you need to.
samples_out[] must be an array of samples_count elements. Stereo audio isn't
supported for efficiency reasons. */
void robingb_get_audio_samples(int8_t samples_out[], uint16_t samples_count);

/* Call this before quitting, or more frequently if you prefer, otherwise your
saves will be lost. The save file will be automatically loaded when you boot
the game again with robingb_init(). */
void robingb_update_save_file();

/* By default, RobinGB will conveniently render white as 0xFF, black as 0x00 etc.
Set this boolean to true to render using the same data format as the original
hardware. The screen will appear extremely dark and inverted, so you will need
to do some additional processing. */
extern bool robingb_native_pixel_format;

/* Finally, here is the more complicated, per-line alternative to
robingb_update_screen(). */
bool robingb_update_screen_line(uint8_t screen[], uint8_t *updated_screen_line);
/* FULL EXPLANATION:
screen[] must be an array of ROBINGB_SCREEN_WIDTH*ROBINGB_SCREEN_HEIGHT elements.
This function runs the game and potentially updates one horizontal line of the
screen. If the function returns true, one line of the screen has been updated
and the uint8_t pointed by updated_screen_line will be set. Games will usually
update lines in order from 0 to 143, but this is not guaranteed. You can then
read the new line from the screen like so:

for (uint16_t i = updated_screen_line * ROBINGB_SCREEN_WIDTH;
    i < updated_screen_line * ROBINGB_SCREEN_WIDTH + ROBINGB_SCREEN_WIDTH;
    i++) {
    uint8_t pixel_position_x = i;
    uint8_t pixel_position_y = updated_screen_line;
    uint8_t pixel_data = screen[i];
}

NOTE: After a complete screen update, robingb_update_screen_line() will return
false multiple times during the vertical-blanking phase. You must enter this
phase 60 times per second for correct emulation speed.
*/

#endif /* end include guard */

#ifdef __cplusplus
}
#endif /* close extern "C" */



