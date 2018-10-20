#include "internal.h"
#include <assert.h>
#include <string.h>

#define LCDC_WINDOW_TILE_MAP_SELECT (0x01 << 6)
#define LCDC_WINDOW_ENABLED (0x01 << 5)
#define LCDC_BG_AND_WINDOW_TILE_DATA_SELECT (0x01 << 4)
#define LCDC_BG_TILE_MAP_SELECT (0x01 << 3)
#define LCDC_DOUBLE_HEIGHT_OBJECTS (0x01 << 2)
#define LCDC_OBJECTS_ENABLED (0x01 << 1)
#define LCDC_BG_AND_WINDOW_ENABLED (0x01)

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
static u8 *bg_palette = &robingb_memory[0xff47];

static u8 *bg_scroll_y = &robingb_memory[0xff42];
static u8 *bg_scroll_x = &robingb_memory[0xff43];

static u8 *window_offset_y = &robingb_memory[0xff4a];
static u8 *window_offset_x_plus_7 = &robingb_memory[0xff4b];

u8 *robingb_screen;

static u8 apply_palette_to_shade(u8 shade, u8 palette) {
	u8 palette_mask = 0x03 << (shade * 2);
	u8 masked_palette_data = palette & palette_mask;
	return masked_palette_data >> (shade * 2);
}

/* IMPORTANT: This function assumes row_out is zero'd. */
/* TODO: Refactor this file so that tile_line_data[] is a u16. */
static void get_pixel_row_from_tile_line_data(u8 tile_line_data[], u8 row_out[]) {
	u16 line_data_as_u16 = *(u16*)tile_line_data;
	
	/* TODO: assert that row_out is zero'd. */
	
	/* TODO: memset() if line_data_as_u16 is a block colour. */
	
	if (!line_data_as_u16) return;
	
	switch (line_data_as_u16 & 0x8080) {
		case 0x0000: row_out[0] = 0x00; break;
		case 0x0080: row_out[0] = 0x01; break;
		case 0x8000: row_out[0] = 0x02; break;
		default: row_out[0] = 0x03; break;
	}
	
	switch (line_data_as_u16 & 0x4040) {
		case 0x0000: row_out[1] = 0x00; break;
		case 0x0040: row_out[1] = 0x01; break;
		case 0x4000: row_out[1] = 0x02; break;
		default: row_out[1] = 0x03; break;
	}
	
	switch (line_data_as_u16 & 0x2020) {
		case 0x0000: row_out[2] = 0x00; break;
		case 0x0020: row_out[2] = 0x01; break;
		case 0x2000: row_out[2] = 0x02; break;
		default: row_out[2] = 0x03; break;
	}
	
	switch (line_data_as_u16 & 0x1010) {
		case 0x0000: row_out[3] = 0x00; break;
		case 0x0010: row_out[3] = 0x01; break;
		case 0x1000: row_out[3] = 0x02; break;
		default: row_out[3] = 0x03; break;
	}
	
	switch (line_data_as_u16 & 0x0808) {
		case 0x0000: row_out[4] = 0x00; break;
		case 0x0008: row_out[4] = 0x01; break;
		case 0x0800: row_out[4] = 0x02; break;
		default: row_out[4] = 0x03; break;
	}
	
	switch (line_data_as_u16 & 0x0404) {
		case 0x0000: row_out[5] = 0x00; break;
		case 0x0004: row_out[5] = 0x01; break;
		case 0x0400: row_out[5] = 0x02; break;
		default: row_out[5] = 0x03; break;
	}
	
	switch (line_data_as_u16 & 0x0202) {
		case 0x0000: row_out[6] = 0x00; break;
		case 0x0002: row_out[6] = 0x01; break;
		case 0x0200: row_out[6] = 0x02; break;
		default: row_out[6] = 0x03; break;
	}
	
	switch (line_data_as_u16 & 0x0101) {
		case 0x0000: row_out[7] = 0x00; break;
		case 0x0100: row_out[7] = 0x02; break;
		case 0x0001: row_out[7] = 0x01; break;
		default: row_out[7] = 0x03; break;
	}
}

static void get_tile_line_data(u16 tile_bank_address, s16 tile_index, u8 tile_line_index, u8 line_data_out[]) {
	u16 tile_address = tile_bank_address + tile_index*NUM_BYTES_PER_TILE;
	u16 tile_line_address = tile_address + tile_line_index*NUM_BYTES_PER_TILE_LINE;
	
	/* each tile line is 2 bytes */
	line_data_out[0] = robingb_memory[tile_line_address];
	line_data_out[1] = robingb_memory[tile_line_address+1];
}

static void get_bg_tile_line_data(u8 coord_x, u8 coord_y, u16 tile_map_address_space, u16 tile_data_bank_address, u8 tile_line_index, u8 tile_line_data_out[]) {
	u16 tile_map_index = coord_x + coord_y*NUM_TILES_PER_BG_LINE;
	s16 tile_data_index = robingb_memory[tile_map_address_space + tile_map_index];
	
	if (tile_data_bank_address == 0x9000) { /* bank 0x9000 uses signed addressing */
		get_tile_line_data(tile_data_bank_address, (s8)tile_data_index, tile_line_index, tile_line_data_out);
	} else {
		get_tile_line_data(tile_data_bank_address, tile_data_index, tile_line_index, tile_line_data_out);
	}
}

static void render_background_line(u8 bg_line[]) {
	s16 bg_y = (*bg_scroll_y) + *ly;
	assert(bg_y < 256); /* TODO: not handling vertical wraparound yet. Could I leverage u8 overflow like bg_scroll_x does? */
	
	u8 tilegrid_y = bg_y / TILE_HEIGHT;
	u8 tile_line_index = bg_y - tilegrid_y*TILE_HEIGHT;
	
	u16 tile_map_address_space = ((*lcdc) & LCDC_BG_TILE_MAP_SELECT) ? 0x9c00 : 0x9800;
	u16 tile_data_address_space = ((*lcdc) & LCDC_BG_AND_WINDOW_TILE_DATA_SELECT) ? 0x8000 : 0x9000;
	
	for (int tilegrid_x = 0; tilegrid_x < NUM_TILES_PER_BG_LINE; tilegrid_x++) {
		u8 tile_line_data[NUM_BYTES_PER_TILE_LINE];
		get_bg_tile_line_data(tilegrid_x, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_line_index, tile_line_data);
		get_pixel_row_from_tile_line_data(tile_line_data, &bg_line[tilegrid_x*TILE_WIDTH]);
	}
}

static void render_window_line() {
	s16 window_line_to_render = (*ly) - (*window_offset_y);
	if (window_line_to_render < 0) return;
	
	u8 tilegrid_y = window_line_to_render / TILE_HEIGHT;
	u8 tile_line_index = window_line_to_render - tilegrid_y*TILE_HEIGHT;
	
	u16 tile_map_address_space = ((*lcdc) & LCDC_WINDOW_TILE_MAP_SELECT) ? 0x9c00 : 0x9800;
	u16 tile_data_address_space = ((*lcdc) & LCDC_BG_AND_WINDOW_TILE_DATA_SELECT) ? 0x8000 : 0x9000;
	
	s16 window_offset_x = (*window_offset_x_plus_7) - 7;
	s16 num_pixels_to_render = SCREEN_WIDTH - window_offset_x;
	s8 num_tiles_to_render = num_pixels_to_render / TILE_WIDTH;
	
	u8 *screen_with_offset = &robingb_screen[window_offset_x + (*ly)*SCREEN_WIDTH];
	memset(screen_with_offset, 0, num_pixels_to_render);
	
	for (u8 tilegrid_x = 0; tilegrid_x < num_tiles_to_render; tilegrid_x++) {
		u8 tile_line_data[NUM_BYTES_PER_TILE_LINE];
		get_bg_tile_line_data(tilegrid_x, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_line_index, tile_line_data);
		get_pixel_row_from_tile_line_data(tile_line_data, &screen_with_offset[tilegrid_x*TILE_WIDTH]);	
	}
	
	/* if a tile is overlapping the edge, copy it to the screen one byte at a time. */
	s16 num_pixels_rendered = num_tiles_to_render*TILE_WIDTH;
	if (num_pixels_rendered < num_pixels_to_render) {
		
		u8 tile_line_data[NUM_BYTES_PER_TILE_LINE];
		get_bg_tile_line_data(num_tiles_to_render, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_line_index, tile_line_data);
		
		u8 tile_pixels[TILE_WIDTH] = {0};
		get_pixel_row_from_tile_line_data(tile_line_data, tile_pixels);
		
		for (s16 screen_x = num_pixels_rendered; screen_x < num_pixels_to_render; screen_x++) {
			screen_with_offset[screen_x] = tile_pixels[screen_x - num_pixels_rendered];
		}
	}
}

static void render_objects() {
	assert(((*lcdc) & LCDC_DOUBLE_HEIGHT_OBJECTS) == false); /* not handling 8x16 objects yet */
	
	for (u16 object_address = 0xfe00; object_address <= 0xfe9f; object_address += 4) {
		u8 translation_y = robingb_memory[object_address] - 16;
		
		if (*ly >= translation_y && *ly < translation_y+8 /* TODO: 8 should be 16 in 8x16 mode.*/) {
			u8 translation_x = robingb_memory[object_address+1] - TILE_WIDTH;
			
			/* TODO: ignore the lower bit of this if in 8x16 mode. */
			u8 tile_data_index = robingb_memory[object_address+2];
			
			u8 object_flags = robingb_memory[object_address+3]; /* TODO: all the other flags */
			bool flip_x = object_flags & bit(5);
			bool flip_y = object_flags & bit(6);
			
			u8 tile_line_index = flip_y ? (translation_y+7 - *ly) : *ly - translation_y;
			u8 tile_line_data[NUM_BYTES_PER_TILE_LINE];
			get_tile_line_data(0x8000, tile_data_index, tile_line_index, tile_line_data);
			
			u8 pixel_row[TILE_WIDTH] = {0};
			get_pixel_row_from_tile_line_data(tile_line_data, pixel_row);
			
			if (flip_x) {
				for (int i = 0; i < TILE_WIDTH; i++) {
					if (pixel_row[i]) robingb_screen[translation_x + 7-i + (*ly)*SCREEN_WIDTH] = pixel_row[i];
				}
			} else {
				for (int i = 0; i < TILE_WIDTH; i++) {
					if (pixel_row[i]) robingb_screen[translation_x + i + (*ly)*SCREEN_WIDTH] = pixel_row[i];
				}
			}
		}
	}
}

void render_screen_line() {
	/* TODO: bg should render to the screen buffer directly. */
	
	if ((*lcdc) & LCDC_BG_AND_WINDOW_ENABLED) {
		u8 bg_buffer[BG_WIDTH] = {0};
		render_background_line(bg_buffer);
		
		for (u8 screen_x = 0; screen_x < SCREEN_WIDTH; screen_x++) {
			u8 bg_x = (*bg_scroll_x) + screen_x;
			robingb_screen[screen_x + (*ly)*SCREEN_WIDTH] = bg_buffer[bg_x];
		}
		
		if ((*lcdc) & LCDC_WINDOW_ENABLED) render_window_line();
	} else {
		/* Background is disabled, so just render white */
		memset(&robingb_screen[(*ly)*SCREEN_WIDTH], 0x00, SCREEN_WIDTH);
	}
	
	/* check if object drawing is enabled */
	if ((*lcdc) & LCDC_OBJECTS_ENABLED) render_objects();
	
	/* convert from game boy 2-bit to target 8-bit */
	int line_start_index = (*ly)*SCREEN_WIDTH;
	for (int x = 0; x < SCREEN_WIDTH; x++) {
		robingb_screen[x + line_start_index] = (robingb_screen[x + line_start_index] - 3) * -85;
	}
}





