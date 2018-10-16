#include "internal.h"
#include <assert.h>
#include <string.h>

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144

#define BG_WIDTH 256

#define NUM_TILES_PER_BG_LINE 32
#define NUM_BYTES_PER_TILE 16
#define NUM_BYTES_PER_TILE_LINE 2
#define TILE_WIDTH 8
#define TILE_HEIGHT 8

static u8 *lcdc = &robingb_memory[LCD_CONTROL_ADDRESS];
static u8 *ly = &robingb_memory[LCD_LY_ADDRESS];
static u8 *bg_scroll_y = &robingb_memory[0xff42];
static u8 *bg_scroll_x = &robingb_memory[0xff43];

u8 *robingb_screen;

static void get_pixel_row_from_tile_line_data(u8 tile_line_data[], u8 row_out[]) {
	for (int r = 0; r < TILE_WIDTH; r++) {
		u8 relevant_bit = 0x80 >> r;
		row_out[r] = (tile_line_data[0] & relevant_bit) ? 0x01 : 0x00;
		row_out[r] |= (tile_line_data[1] & relevant_bit) ? 0x02 : 0x00;
	}
}

static void get_tile_line_data_for_bg_tilegrid_coord(u8 x, u8 y, u16 tile_map_address_space, u16 tile_data_address_space, u8 tile_pixel_y, u8 tile_line_data_out[]) {
	int bg_tile_map_index = x + y*NUM_TILES_PER_BG_LINE;
	
	int tile_data_index = robingb_memory[tile_map_address_space + bg_tile_map_index];
	if (tile_data_address_space == 0x9000) tile_data_index = (s8)tile_data_index;
	
	int tile_data_address = tile_data_address_space + tile_data_index*NUM_BYTES_PER_TILE;
	int tile_line_address = tile_data_address + tile_pixel_y*NUM_BYTES_PER_TILE_LINE;
	
	tile_line_data_out[0] = robingb_memory[tile_line_address];
	tile_line_data_out[1] = robingb_memory[tile_line_address+1];
}

static void render_background_line(u8 bg_line[], bool is_window) {
	const u8 bg_y = *ly; /* TODO: temp. This won't work for vertical scrolling. */
	const u8 tilegrid_y = bg_y / TILE_HEIGHT;
	const u8 tile_pixel_y = bg_y - tilegrid_y*TILE_HEIGHT;
	
	u8 tile_map_control_bit = is_window ? bit(6) : bit(3);
	u16 tile_map_address_space = ((*lcdc) & tile_map_control_bit) ? 0x9c00 : 0x9800;
	u16 tile_data_address_space = ((*lcdc) & bit(4)) ? 0x8000 : 0x9000;
	
	for (int tilegrid_x = 0; tilegrid_x < NUM_TILES_PER_BG_LINE; tilegrid_x++) {
		u8 tile_line_data[NUM_BYTES_PER_TILE_LINE];
		get_tile_line_data_for_bg_tilegrid_coord(tilegrid_x, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_pixel_y, tile_line_data);
		get_pixel_row_from_tile_line_data(tile_line_data, &bg_line[tilegrid_x*TILE_WIDTH]);
	}
}

static void get_object_data(int object_data_index, u8 object_data_out[]) {
	for (int b =  0; b < NUM_BYTES_PER_TILE; b++) {
		object_data_out[b] = robingb_memory[0x8000 + object_data_index*NUM_BYTES_PER_TILE + b];
	}
}

static void render_objects_on_line() {
	assert(((*lcdc) & bit(2)) == false); /* not handling 8x16 objects yet */
	
	for (u16 object_address = 0xfe00; object_address <= 0xfe9f; object_address += 4) {
		u8 obj_y = robingb_memory[object_address] - 16;
		
		if (*ly >= obj_y && *ly < obj_y+8 /* TODO: 8 should be 16 in 8x16 mode.*/) {
			u8 obj_x = robingb_memory[object_address+1] - TILE_WIDTH;
			
			/* TODO: ignore the lower bit of this if in 8x16 mode. */
			u8 object_data_index = robingb_memory[object_address+2];
			
			u8 object_flags = robingb_memory[object_address+3]; /* TODO: all the other flags */
			bool flip_x = object_flags & bit(5);
			bool flip_y = object_flags & bit(6);
			
			u8 object_data[NUM_BYTES_PER_TILE];
			get_object_data(object_data_index, object_data); /* TODO: don't need to get the full object */
			
			u8 pixel_row[TILE_WIDTH];
			if (flip_y) get_pixel_row_from_tile_line_data(&object_data[(obj_y+7 - *ly)*2], pixel_row);
			else get_pixel_row_from_tile_line_data(&object_data[(*ly - obj_y)*2], pixel_row);
			
			if (flip_x) {
				for (int i = 0; i < TILE_WIDTH; i++) {
					if (pixel_row[i]) robingb_screen[obj_x + 7-i + (*ly)*SCREEN_WIDTH] = pixel_row[i];
				}
			} else {
				for (int i = 0; i < TILE_WIDTH; i++) {
					if (pixel_row[i]) robingb_screen[obj_x + i + (*ly)*SCREEN_WIDTH] = pixel_row[i];
				}
			}
		}
	}
}

void render_screen_line() {
	
	/* Check if the background is enabled */
	if ((*lcdc) & 0x01) {
		
		if ((*lcdc) & bit(5)) {
			robingb_log("window enabled");
		}
		
		u8 bg_line[BG_WIDTH] = {0};
		render_background_line(bg_line, false);
		
		for (u8 screen_x = 0; screen_x < SCREEN_WIDTH; screen_x++) {
			u16 bg_x = (*bg_scroll_x) + screen_x;
			
			while (bg_x >= BG_WIDTH) bg_x -= BG_WIDTH;
			
			robingb_screen[screen_x + (*ly)*SCREEN_WIDTH] = bg_line[bg_x];
		}
	} else {
		/* Background is disabled, so just render white */
		memset(&robingb_screen[(*ly)*SCREEN_WIDTH], 0x00, SCREEN_WIDTH);
	}
	
	/* check if object drawing is enabled */
	if ((*lcdc) & bit(1)) render_objects_on_line();
	
	/* convert from game boy 2-bit to target 8-bit */
	int line_index_start = (*ly)*SCREEN_WIDTH;
	for (int x = 0; x < SCREEN_WIDTH; x++) {
		robingb_screen[x + line_index_start] = (robingb_screen[x + line_index_start] - 3) * -85;
	}
}





