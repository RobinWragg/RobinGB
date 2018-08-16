#include "internal.h"

u8 LY;

#define SCREEN_WIDTH (160)
#define SCREEN_HEIGHT (144)

#define BG_WIDTH (256)
#define BG_HEIGHT (256)
u8 bg[BG_WIDTH*BG_HEIGHT];
// u8 bg[((BG_WIDTH*BG_HEIGHT) * 2) / 8];

#define NUM_TILES_PER_BG_LINE (32)
#define NUM_BYTES_PER_TILE (16)
#define NUM_BYTES_PER_TILE_LINE (2)
#define TILE_WIDTH_IN_PIXELS (8)
#define TILE_HEIGHT_IN_PIXELS (8)

void get_pixel_row_from_tile_line_data(u8 tile_line_data[], u8 row_out[]) {
	for (int r = 0; r < 8; r++) {
		u8 relevant_bit = 0x80 >> r;
		u8 lower_bit = (tile_line_data[0] & relevant_bit) ? 0x01 : 0x00;
		u8 upper_bit = (tile_line_data[1] & relevant_bit) ? 0x02 : 0x00;
		
		u8 shade = lower_bit | upper_bit;
		switch (shade) {
			case 0x00: row_out[r] = 85 * 3; break;
			case 0x01: row_out[r] = 85 * 2; break;
			case 0x02: row_out[r] = 85; break;
			case 0x03: row_out[r] = 0; break;
		}
	}
}

void get_tile_line_data_for_bg_tile_coord(u8 x, u8 y, u8 tile_line_index, u8 tile_line_data_out[]) {
	u8 lcdc = mem_read(LCD_CONTROL_ADDRESS);
	u16 bg_tile_data_address_space_start = (lcdc & 0x10) ? 0x8000 : 0x9000;
	u16 bg_tile_map_address_space_start = (lcdc & 0x08) ? 0x9c00 : 0x9800;
	
	// TODO: data:0x8000 and map:0x9800 for window on the levels of super mario land?
	
	int bg_tile_map_index = x + y*NUM_TILES_PER_BG_LINE;
	int tile_data_index = mem_read(bg_tile_map_address_space_start + bg_tile_map_index);
	int tile_data_address = bg_tile_data_address_space_start + tile_data_index*NUM_BYTES_PER_TILE;
	int tile_line_address = tile_data_address + tile_line_index*NUM_BYTES_PER_TILE_LINE;
	
	tile_line_data_out[0] = mem_read(tile_line_address);
	tile_line_data_out[1] = mem_read(tile_line_address+1);
}

void render_background_line() {
	const u8 bg_y = LY; // TODO: temp. This won't work for vertical scrolling.
	const u8 bg_tile_y = bg_y / TILE_HEIGHT_IN_PIXELS;
	const u8 tile_line_index = LY - bg_tile_y*TILE_HEIGHT_IN_PIXELS;
	
	for (int bg_x = 0; bg_x < BG_WIDTH; bg_x += 8) {
		const u8 bg_tile_x = bg_x / TILE_WIDTH_IN_PIXELS;
		
		u8 tile_line_data[NUM_BYTES_PER_TILE_LINE];
		get_tile_line_data_for_bg_tile_coord(bg_tile_x, bg_tile_y, tile_line_index, tile_line_data);
		
		u8 pixel_row[TILE_WIDTH_IN_PIXELS];
		get_pixel_row_from_tile_line_data(tile_line_data, pixel_row);
		
		memcpy(&bg[bg_x + bg_y*BG_WIDTH], pixel_row, sizeof(u8) * TILE_WIDTH_IN_PIXELS);
	}
}

void get_object_data(int object_data_index, u8 object_data_out[]) {
	for (int b =  0; b < NUM_BYTES_PER_TILE; b++) {
		object_data_out[b] = mem_read(0x8000 + object_data_index*NUM_BYTES_PER_TILE + b);
	}
}

// 5760
// 160 * 144 * 2bits = 46080bits = 5760bytes
u8 screen[SCREEN_WIDTH*SCREEN_HEIGHT];

void render_objects_on_line() {
	assert((mem_read(LCD_CONTROL_ADDRESS) & bit(2)) == false); // not handling 8x16 objects yet
	
	for (int address = 0xfe00; address < 0xfea0; address += 4) {
		u8 y = mem_read(address) - 16; // TODO: +/- 16?
		
		if (LY >= y && LY < y+8 /* TODO: 8 should be 16 in 8x16 mode.*/) {
			u8 x = mem_read(address+1) - 8;
			
			// TODO: ignore the lower bit of this if in 8x16 mode.
			u8 object_data_index = mem_read(address+2);
			
			u8 object_data[NUM_BYTES_PER_TILE];
			get_object_data(object_data_index, object_data);
			
			// get_pixel_row_from_tile_data(object_data, LY - y, &screen[x + LY*160]);
		}
	}
}

void render_screen_line(u8 ly) {
	LY = ly;
	
	u8 lcdc = mem_read(LCD_CONTROL_ADDRESS);
	
	if (lcdc & 0x01) { // NOTE: bit 0 of lcdc has different meanings for Game Boy Color.
		render_background_line();
		
		u8 bg_scroll_x = mem_read(0xff43);
		for (u8 s = 0; s < SCREEN_WIDTH; s++) {
			int bg_x = bg_scroll_x + s;
			while (bg_x >= BG_WIDTH) bg_x -= BG_WIDTH;
			screen[s + LY*SCREEN_WIDTH] = bg[bg_x + LY*BG_WIDTH];
		}
	} else {
		for (u8 s = 0; s < SCREEN_WIDTH; s++) screen[s + LY*SCREEN_WIDTH] = 0xff;
	}
	
	// check if object object drawing is enabled
	// if (lcdc & bit(1)) render_objects_on_line();
}

void robingb_get_background(u8 bg_out[]) {
	memcpy(bg_out, bg, BG_WIDTH*BG_HEIGHT);
}

void robingb_get_screen(u8 screen_out[]) {
	memcpy(screen_out, screen, SCREEN_WIDTH*SCREEN_HEIGHT);
}




