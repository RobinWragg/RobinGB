#include "internal.h"
#include <assert.h>
#include <string.h>

#define SCREEN_WIDTH_IN_PIXELS 160
#define SCREEN_WIDTH_IN_BYTES 40
#define SCREEN_HEIGHT 144

#define BG_WIDTH_IN_BYTES 64
#define BG_WIDTH_IN_PIXELS 256

#define NUM_TILES_PER_BG_LINE 32
#define NUM_BYTES_PER_TILE 16
#define NUM_BYTES_PER_TILE_LINE 2
#define TILE_WIDTH_IN_PIXELS 8
#define TILE_HEIGHT_IN_PIXELS 8

static u8 *lcdc = &robingb_memory[LCD_CONTROL_ADDRESS];
static u8 *ly = &robingb_memory[LCD_LY_ADDRESS];
static u8 *bg_scroll_x = &robingb_memory[0xff43];

u8 *robingb_screen;

static void get_pixel_row_from_tile_line_data(u8 tile_line_data[], u8 row_out[]) {
	for (int r = 0; r < 8; r++) {
		u8 relevant_bit = 0x80 >> r;
		u8 lower_bit = (tile_line_data[0] & relevant_bit) ? 0x01 : 0x00;
		u8 upper_bit = (tile_line_data[1] & relevant_bit) ? 0x02 : 0x00;
		
		row_out[r] = lower_bit | upper_bit;
	}
}

static void get_tile_line_data_for_bg_tile_coord(u8 x, u8 y, u8 tile_line_index, u8 tile_line_data_out[]) {
	u16 bg_tile_data_address_space_start = ((*lcdc) & 0x10) ? 0x8000 : 0x9000;
	u16 bg_tile_map_address_space_start = ((*lcdc) & 0x08) ? 0x9c00 : 0x9800;
	
	// TODO: data:0x8000 and map:0x9800 for window on the levels of super mario land?
	
	int bg_tile_map_index = x + y*NUM_TILES_PER_BG_LINE;
	int tile_data_index = robingb_memory[bg_tile_map_address_space_start + bg_tile_map_index];
	int tile_data_address = bg_tile_data_address_space_start + tile_data_index*NUM_BYTES_PER_TILE;
	int tile_line_address = tile_data_address + tile_line_index*NUM_BYTES_PER_TILE_LINE;
	
	tile_line_data_out[0] = robingb_memory[tile_line_address];
	tile_line_data_out[1] = robingb_memory[tile_line_address+1];
}

static u8 get_bg_line_pixel(u8 bg_line[], u8 x) {
	u8 byte_index = x / 4; // 4 pixels per byte
	u8 bit_index = (x % 4) * 2; // 2 bits per pixel
	
	u8 bg_byte = bg_line[byte_index];
	u8 bg_pixel = bg_byte & (0x03 << bit_index);
	return (bg_pixel >> bit_index) * 85;
}

static void render_background_line(u8 bg_line[]) {
	const u8 bg_y = *ly; // TODO: temp. This won't work for vertical scrolling.
	const u8 bg_tile_y = bg_y / TILE_HEIGHT_IN_PIXELS;
	const u8 tile_line_index = (*ly) - bg_tile_y*TILE_HEIGHT_IN_PIXELS;
	
	for (int bg_x = 0; bg_x < BG_WIDTH_IN_PIXELS; bg_x += 8) {
		const u8 bg_tile_x = bg_x / TILE_WIDTH_IN_PIXELS;
		
		u8 tile_line_data[NUM_BYTES_PER_TILE_LINE];
		get_tile_line_data_for_bg_tile_coord(bg_tile_x, bg_tile_y, tile_line_index, tile_line_data);
		
		for (int pixel_x = 0; pixel_x < 8; pixel_x++) {
			u8 pixel_bit_index = 0x80 >> pixel_x;
			u8 lower_shade_bit = (tile_line_data[0] & pixel_bit_index) ? 0x01 : 0x00;
			u8 upper_shade_bit = (tile_line_data[1] & pixel_bit_index) ? 0x02 : 0x00;
			
			u8 shade_2bit = lower_shade_bit | upper_shade_bit;
			
			u16 bg_pixel_x = bg_x + pixel_x;
			
			u8 byte_index = bg_pixel_x / 4; // 4 pixels per byte
			u8 bit_index = (pixel_x % 4) * 2; // 2 bits per pixel
			bg_line[byte_index] |= shade_2bit << bit_index;
		}
	}
}

// static void get_object_data(int object_data_index, u8 object_data_out[]) {
// 	for (int b =  0; b < NUM_BYTES_PER_TILE; b++) {
// 		object_data_out[b] = robingb_memory[0x8000 + object_data_index*NUM_BYTES_PER_TILE + b];
// 	}
// }

// static void render_objects_on_line() {
// 	assert(((*lcdc) & bit(2)) == false); // not handling 8x16 objects yet
	
// 	for (int address = 0xfe00; address < 0xfea0; address += 4) {
// 		u8 y = robingb_memory[address] - 16; // TODO: +/- 16?
		
// 		if (*ly >= y && *ly < y+8 /* TODO: 8 should be 16 in 8x16 mode.*/) {
// 			u8 x = robingb_memory[address+1] - 8;
			
// 			// TODO: ignore the lower bit of this if in 8x16 mode.
// 			u8 object_data_index = robingb_memory[address+2];
			
// 			u8 object_data[NUM_BYTES_PER_TILE];
// 			get_object_data(object_data_index, object_data);
			
// 			// get_pixel_row_from_tile_data(object_data, (*ly) - y, &robingb_screen[x + (*ly)*160]);
// 		}
// 	}
// }

static void set_screen_pixel(u8 x, u8 y, u8 doublebit_shade) {
	u16 byte_index = x/4 + y*SCREEN_WIDTH_IN_BYTES; // 4 pixels per byte
	u8 bit_index = (x % 4) * 2; // 2 bits per pixel
	robingb_screen[byte_index] &= ~(0x03 << bit_index); // wipe the pixel
	robingb_screen[byte_index] |= doublebit_shade << bit_index;
}

void render_screen_line() {
	if ((*lcdc) & 0x01) { // Check if the background is enabled. NOTE: bit 0 of lcdc has different meanings for Game Boy Color.
		
		u8 bg_line[BG_WIDTH_IN_BYTES] = {0};
		render_background_line(bg_line);
		
		for (u8 s = 0; s < SCREEN_WIDTH_IN_PIXELS; s++) {
			int bg_x = (*bg_scroll_x) + s;
			
			while (bg_x >= BG_WIDTH_IN_PIXELS) bg_x -= BG_WIDTH_IN_PIXELS;
			
			u8 pixel = get_bg_line_pixel(bg_line, bg_x);
			set_screen_pixel(s, *ly, pixel);
		}
	} else {
		// Background is disabled, so just render white
		for (int x = 0; x < SCREEN_WIDTH_IN_BYTES; x++) {
			robingb_screen[x + (*ly)*SCREEN_WIDTH_IN_BYTES] = 0x00;
		}
	}
	
	// check if object object drawing is enabled
	// if ((*lcdc) & bit(1)) render_objects_on_line();
	
}





