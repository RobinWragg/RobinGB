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

bool robingb_native_pixel_format = false;

static u8 *lcdc = &robingb_memory[LCD_CONTROL_ADDRESS];
static u8 *ly = &robingb_memory[LCD_LY_ADDRESS];
static u8 *bg_palette = &robingb_memory[0xff47];
static u8 *object_palette_0 = &robingb_memory[0xff48];
static u8 *object_palette_1 = &robingb_memory[0xff49];

static u8 *bg_scroll_y = &robingb_memory[0xff42];
static u8 *bg_scroll_x = &robingb_memory[0xff43];

static u8 *window_offset_y = &robingb_memory[0xff4a];
static u8 *window_offset_x_plus_7 = &robingb_memory[0xff4b];

#define SHADE_0_FLAG 0x04

static u8 shade_0;
static u8 shade_1;
static u8 shade_2;
static u8 shade_3;

u8 *robingb_screen;

static void set_palette(u8 palette) {
	/* SHADE_0_FLAG ensures shade_0 is unique, which streamlines the process of shade-0-dependent
	blitting. The flag is discarded in the final step of the render. */
	shade_0 = (palette & 0x03) | SHADE_0_FLAG;
	shade_1 = (palette & 0x0c) >> 2;
	shade_2 = (palette & 0x30) >> 4;
	shade_3 = (palette & 0xc0) >> 6;
}

static void get_tile_line(u16 tile_bank_address, s16 tile_index, u8 tile_line_index, u8 line_out[]) {
	
	u16 tile_address = tile_bank_address + tile_index*NUM_BYTES_PER_TILE;
	u16 line_data = *(u16*)&robingb_memory[tile_address + tile_line_index*NUM_BYTES_PER_TILE_LINE];
	
	switch (line_data) {
		case 0x0000: memset(line_out, shade_0, TILE_WIDTH); return; break;
		case 0x00ff: memset(line_out, shade_1, TILE_WIDTH); return; break;
		case 0xff00: memset(line_out, shade_2, TILE_WIDTH); return; break;
		case 0xffff: memset(line_out, shade_3, TILE_WIDTH); return; break;
	}
	
	switch (line_data & 0x8080) {
		case 0x0000: line_out[0] = shade_0; break;
		case 0x0080: line_out[0] = shade_1; break;
		case 0x8000: line_out[0] = shade_2; break;
		default: line_out[0] = shade_3; break;
	}
	
	switch (line_data & 0x4040) {
		case 0x0000: line_out[1] = shade_0; break;
		case 0x0040: line_out[1] = shade_1; break;
		case 0x4000: line_out[1] = shade_2; break;
		default: line_out[1] = shade_3; break;
	}
	
	switch (line_data & 0x2020) {
		case 0x0000: line_out[2] = shade_0; break;
		case 0x0020: line_out[2] = shade_1; break;
		case 0x2000: line_out[2] = shade_2; break;
		default: line_out[2] = shade_3; break;
	}
	
	switch (line_data & 0x1010) {
		case 0x0000: line_out[3] = shade_0; break;
		case 0x0010: line_out[3] = shade_1; break;
		case 0x1000: line_out[3] = shade_2; break;
		default: line_out[3] = shade_3; break;
	}
	
	switch (line_data & 0x0808) {
		case 0x0000: line_out[4] = shade_0; break;
		case 0x0008: line_out[4] = shade_1; break;
		case 0x0800: line_out[4] = shade_2; break;
		default: line_out[4] = shade_3; break;
	}
	
	switch (line_data & 0x0404) {
		case 0x0000: line_out[5] = shade_0; break;
		case 0x0004: line_out[5] = shade_1; break;
		case 0x0400: line_out[5] = shade_2; break;
		default: line_out[5] = shade_3; break;
	}
	
	switch (line_data & 0x0202) {
		case 0x0000: line_out[6] = shade_0; break;
		case 0x0002: line_out[6] = shade_1; break;
		case 0x0200: line_out[6] = shade_2; break;
		default: line_out[6] = shade_3; break;
	}
	
	switch (line_data & 0x0101) {
		case 0x0000: line_out[7] = shade_0; break;
		case 0x0001: line_out[7] = shade_1; break;
		case 0x0100: line_out[7] = shade_2; break;
		default: line_out[7] = shade_3; break;
	}
}

static void get_bg_tile_line(u8 coord_x, u8 coord_y, u16 tile_map_address_space, u16 tile_data_bank_address, u8 tile_line_index, u8 line_out[]) {
	u16 tile_map_index = coord_x + coord_y*NUM_TILES_PER_BG_LINE;
	s16 tile_data_index = robingb_memory[tile_map_address_space + tile_map_index];
	
	if (tile_data_bank_address == 0x9000) { /* bank 0x9000 uses signed addressing */
		get_tile_line(tile_data_bank_address, (s8)tile_data_index, tile_line_index, line_out);
	} else {
		get_tile_line(tile_data_bank_address, tile_data_index, tile_line_index, line_out);
	}
}

static void render_background_line() {
	u8 bg_y = *ly + *bg_scroll_y;
	
	u8 tilegrid_y = bg_y / TILE_HEIGHT;
	u8 tile_line_index = bg_y - tilegrid_y*TILE_HEIGHT;
	
	u16 tile_map_address_space = ((*lcdc) & LCDC_BG_TILE_MAP_SELECT) ? 0x9c00 : 0x9800;
	u16 tile_data_address_space = ((*lcdc) & LCDC_BG_AND_WINDOW_TILE_DATA_SELECT) ? 0x8000 : 0x9000;
	
	u8 *screen_line = &robingb_screen[(*ly) * SCREEN_WIDTH];
	
	s8 tilegrid_x;
	for (tilegrid_x = 0; tilegrid_x < NUM_TILES_PER_BG_LINE; tilegrid_x++) {
		u8 screen_x = tilegrid_x*TILE_WIDTH - (*bg_scroll_x);
		
		if (screen_x <= SCREEN_WIDTH - TILE_WIDTH) {
			
			get_bg_tile_line(tilegrid_x, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_line_index, &screen_line[screen_x]);
			
		} else if (screen_x <= SCREEN_WIDTH) {
			
			u8 tile_line[TILE_WIDTH];
			get_bg_tile_line(tilegrid_x, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_line_index, tile_line);
			
			u8 tile_x;
			for (tile_x = 0; tile_x < SCREEN_WIDTH-screen_x; tile_x++) {
				screen_line[screen_x + tile_x] = tile_line[tile_x];
			}
		} else if (screen_x-SCREEN_WIDTH < TILE_WIDTH) {
			u8 pixel_count_to_render = screen_x - SCREEN_WIDTH;
			screen_x = 0;
			
			u8 tile_line[TILE_WIDTH];
			get_bg_tile_line((*bg_scroll_x)/TILE_WIDTH, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_line_index, tile_line);
			
			u8 tile_x;
			for (tile_x = TILE_WIDTH - pixel_count_to_render; tile_x < TILE_WIDTH; tile_x++) {
				screen_line[screen_x++] = tile_line[tile_x];
			}
		}
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
	
	u8 tilegrid_x;
	for (tilegrid_x = 0; tilegrid_x < num_tiles_to_render; tilegrid_x++) {
		get_bg_tile_line(tilegrid_x, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_line_index, &screen_with_offset[tilegrid_x*TILE_WIDTH]);
	}
	
	/* if a tile is overlapping the edge, copy it to the screen one byte at a time. */
	s16 num_pixels_rendered = num_tiles_to_render*TILE_WIDTH;
	if (num_pixels_rendered < num_pixels_to_render) {
		
		u8 tile_pixels[TILE_WIDTH];
		get_bg_tile_line(num_tiles_to_render, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_line_index, tile_pixels);
		
		s16 screen_x;
		for (screen_x = num_pixels_rendered; screen_x < num_pixels_to_render; screen_x++) {
			screen_with_offset[screen_x] = tile_pixels[screen_x - num_pixels_rendered];
		}
	}
}

static void render_objects() {
	u8 object_height = 8;
	if ((*lcdc) & LCDC_DOUBLE_HEIGHT_OBJECTS) object_height = 16;
	
	u8 *screen_line = &robingb_screen[(*ly) * SCREEN_WIDTH];
	
	u16 object_address;
	for (object_address = 0xfe9c; object_address >= 0xfe00; object_address -= 4) {
		s16 translate_y = robingb_memory[object_address] - TILE_HEIGHT*2;
		
		if (*ly >= translate_y && *ly < translate_y+object_height) {
			s16 translate_x = robingb_memory[object_address+1] - TILE_WIDTH;
			
			u8 tile_data_index = robingb_memory[object_address+2];
			
			/* ignore the lowest bit of the index if in double-height mode */
			if (object_height > 8) tile_data_index &= 0xfe;
			
			u8 object_flags = robingb_memory[object_address+3];
			bool choose_palette_1 = object_flags & robingb_bit(4);
			bool flip_x = object_flags & robingb_bit(5);
			bool flip_y = object_flags & robingb_bit(6);
			bool behind_background = object_flags & robingb_bit(7);
			
			if (choose_palette_1) set_palette(*object_palette_1);
			else set_palette(*object_palette_0);
			
			u8 tile_line[TILE_WIDTH];
			{
				s8 tile_line_index = flip_y ? (translate_y+7 - *ly) : *ly - translate_y;
				get_tile_line(0x8000, tile_data_index, tile_line_index, tile_line);
			}
			
			u8 screen_x_start = translate_x < 0 ? 0 : translate_x;
			u8 screen_x_end = translate_x + TILE_WIDTH;
			
			if (flip_x) {
				u8 tile_pixel_index = translate_x < 0 ? (TILE_WIDTH-1)+translate_x : (TILE_WIDTH-1);
				
				if (behind_background) {
					u8 screen_x;
					for (screen_x = screen_x_start; screen_x < screen_x_end; screen_x++) {
						u8 tile_pixel = tile_line[tile_pixel_index--];
						
						if (!(tile_pixel & SHADE_0_FLAG) && screen_line[screen_x] & SHADE_0_FLAG) {
							screen_line[screen_x] = tile_pixel;
						}
					}
				} else {
					u8 screen_x;
					for (screen_x = screen_x_start; screen_x < screen_x_end; screen_x++) {
						u8 tile_pixel = tile_line[tile_pixel_index--];
						
						if (!(tile_pixel & SHADE_0_FLAG)) {
							screen_line[screen_x] = tile_pixel;
						}
					}
				}
			} else {
				u8 tile_pixel_index = translate_x < 0 ? -translate_x : 0;
				
				if (behind_background) {
					u8 screen_x;
					for (screen_x = screen_x_start; screen_x < screen_x_end; screen_x++) {
						u8 tile_pixel = tile_line[tile_pixel_index++];
						
						if (!(tile_pixel & SHADE_0_FLAG) && screen_line[screen_x] & SHADE_0_FLAG) {
							screen_line[screen_x] = tile_pixel;
						}
					}
				} else {
					u8 screen_x;
					for (screen_x = screen_x_start; screen_x < screen_x_end; screen_x++) {
						u8 tile_pixel = tile_line[tile_pixel_index++];
						
						if (!(tile_pixel & SHADE_0_FLAG)) {
							screen_line[screen_x] = tile_pixel;
						}
					}
				}
			}
		}
	}
}

void robingb_render_screen_line() {
	
	if ((*lcdc) & LCDC_BG_AND_WINDOW_ENABLED) {
		set_palette(*bg_palette);
		
		render_background_line();
		
		if ((*lcdc) & LCDC_WINDOW_ENABLED) render_window_line();
	} else {
		/* Background is disabled, so just render white */
		memset(&robingb_screen[(*ly)*SCREEN_WIDTH], 0x00, SCREEN_WIDTH);
	}
	
	/* check if object drawing is enabled */
	if ((*lcdc) & LCDC_OBJECTS_ENABLED) render_objects();
	
	/* convert from game boy 2-bit to target 8-bit */
	{
		u16 pixel_index;
		u16 screen_line_start = (*ly)*SCREEN_WIDTH;
		u16 screen_line_end = screen_line_start + SCREEN_WIDTH;
		
		if (robingb_native_pixel_format) {
			/* The '& 0x03' below is to discard the SHADE_0_FLAG bit,
			which has already served its purpose in render_objects(). */
			
			for (pixel_index = screen_line_start; pixel_index < screen_line_end; pixel_index++) {
				robingb_screen[pixel_index] &= 0x03;
			}
		} else {
			for (pixel_index = screen_line_start; pixel_index < screen_line_end; pixel_index++) {
				robingb_screen[pixel_index] = ((robingb_screen[pixel_index] & 0x03) - 3) * -85;
			}
		}
	}
}







