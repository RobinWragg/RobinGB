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

static uint8_t *lcdc = &robingb_memory[LCD_CONTROL_ADDRESS];
static uint8_t *ly = &robingb_memory[LCD_LY_ADDRESS];
static uint8_t *bg_palette = &robingb_memory[0xff47];
static uint8_t *object_palette_0 = &robingb_memory[0xff48];
static uint8_t *object_palette_1 = &robingb_memory[0xff49];

static uint8_t *bg_scroll_y = &robingb_memory[0xff42];
static uint8_t *bg_scroll_x = &robingb_memory[0xff43];

static uint8_t *window_offset_y = &robingb_memory[0xff4a];
static uint8_t *window_offset_x_plus_7 = &robingb_memory[0xff4b];

#define SHADE_0_FLAG 0x04

static uint8_t shade_0;
static uint8_t shade_1;
static uint8_t shade_2;
static uint8_t shade_3;

uint8_t *robingb_screen;

static void set_palette(uint8_t palette) {
    /* SHADE_0_FLAG ensures shade_0 is unique, which streamlines the process of shade-0-dependent
    blitting. The flag is discarded in the final step of the render. */
    shade_0 = (palette & 0x03) | SHADE_0_FLAG;
    shade_1 = (palette & 0x0c) >> 2;
    shade_2 = (palette & 0x30) >> 4;
    shade_3 = (palette & 0xc0) >> 6;
}

static void get_tile_line(uint16_t tile_bank_address, int16_t tile_index, uint8_t tile_line_index, uint8_t line_out[]) {
    
    uint16_t tile_address = tile_bank_address + tile_index*NUM_BYTES_PER_TILE;
    uint16_t line_data = *(uint16_t*)&robingb_memory[tile_address + tile_line_index*NUM_BYTES_PER_TILE_LINE];
    
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

static void get_bg_tile_line(uint8_t coord_x, uint8_t coord_y, uint16_t tile_map_address_space, uint16_t tile_data_bank_address, uint8_t tile_line_index, uint8_t line_out[]) {
    uint16_t tile_map_index = coord_x + coord_y*NUM_TILES_PER_BG_LINE;
    int16_t tile_data_index = robingb_memory[tile_map_address_space + tile_map_index];
    
    if (tile_data_bank_address == 0x9000) { /* bank 0x9000 uses signed addressing */
        get_tile_line(tile_data_bank_address, (int8_t)tile_data_index, tile_line_index, line_out);
    } else {
        get_tile_line(tile_data_bank_address, tile_data_index, tile_line_index, line_out);
    }
}

static void render_background_line() {
    uint8_t bg_y = *ly + *bg_scroll_y;
    
    uint8_t tilegrid_y = bg_y / TILE_HEIGHT;
    uint8_t tile_line_index = bg_y - tilegrid_y*TILE_HEIGHT;
    
    uint16_t tile_map_address_space = ((*lcdc) & LCDC_BG_TILE_MAP_SELECT) ? 0x9c00 : 0x9800;
    uint16_t tile_data_address_space = ((*lcdc) & LCDC_BG_AND_WINDOW_TILE_DATA_SELECT) ? 0x8000 : 0x9000;
    
    uint8_t *screen_line = &robingb_screen[(*ly) * SCREEN_WIDTH];
    
    int8_t tilegrid_x;
    for (tilegrid_x = 0; tilegrid_x < NUM_TILES_PER_BG_LINE; tilegrid_x++) {
        uint8_t screen_x = tilegrid_x*TILE_WIDTH - (*bg_scroll_x);
        
        if (screen_x <= SCREEN_WIDTH - TILE_WIDTH) {
            
            get_bg_tile_line(tilegrid_x, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_line_index, &screen_line[screen_x]);
            
        } else if (screen_x <= SCREEN_WIDTH) {
            
            uint8_t tile_line[TILE_WIDTH];
            get_bg_tile_line(tilegrid_x, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_line_index, tile_line);
            
            uint8_t tile_x;
            for (tile_x = 0; tile_x < SCREEN_WIDTH-screen_x; tile_x++) {
                screen_line[screen_x + tile_x] = tile_line[tile_x];
            }
        } else if (screen_x-SCREEN_WIDTH < TILE_WIDTH) {
            uint8_t pixel_count_to_render = screen_x - SCREEN_WIDTH;
            screen_x = 0;
            
            uint8_t tile_line[TILE_WIDTH];
            get_bg_tile_line((*bg_scroll_x)/TILE_WIDTH, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_line_index, tile_line);
            
            uint8_t tile_x;
            for (tile_x = TILE_WIDTH - pixel_count_to_render; tile_x < TILE_WIDTH; tile_x++) {
                screen_line[screen_x++] = tile_line[tile_x];
            }
        }
    }
}

static void render_window_line() {
    int16_t window_line_to_render = (*ly) - (*window_offset_y);
    if (window_line_to_render < 0) return;
    
    uint8_t tilegrid_y = window_line_to_render / TILE_HEIGHT;
    uint8_t tile_line_index = window_line_to_render - tilegrid_y*TILE_HEIGHT;
    
    uint16_t tile_map_address_space = ((*lcdc) & LCDC_WINDOW_TILE_MAP_SELECT) ? 0x9c00 : 0x9800;
    uint16_t tile_data_address_space = ((*lcdc) & LCDC_BG_AND_WINDOW_TILE_DATA_SELECT) ? 0x8000 : 0x9000;
    
    int16_t window_offset_x = (*window_offset_x_plus_7) - 7;
    int16_t num_pixels_to_render = SCREEN_WIDTH - window_offset_x;
    int8_t num_tiles_to_render = num_pixels_to_render / TILE_WIDTH;
    
    uint8_t *screen_with_offset = &robingb_screen[window_offset_x + (*ly)*SCREEN_WIDTH];
    
    uint8_t tilegrid_x;
    for (tilegrid_x = 0; tilegrid_x < num_tiles_to_render; tilegrid_x++) {
        get_bg_tile_line(tilegrid_x, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_line_index, &screen_with_offset[tilegrid_x*TILE_WIDTH]);
    }
    
    /* if a tile is overlapping the edge, copy it to the screen one byte at a time. */
    int16_t num_pixels_rendered = num_tiles_to_render*TILE_WIDTH;
    if (num_pixels_rendered < num_pixels_to_render) {
        
        uint8_t tile_pixels[TILE_WIDTH];
        get_bg_tile_line(num_tiles_to_render, tilegrid_y, tile_map_address_space, tile_data_address_space, tile_line_index, tile_pixels);
        
        int16_t screen_x;
        for (screen_x = num_pixels_rendered; screen_x < num_pixels_to_render; screen_x++) {
            screen_with_offset[screen_x] = tile_pixels[screen_x - num_pixels_rendered];
        }
    }
}

static void render_objects() {
    uint8_t object_height = 8;
    if ((*lcdc) & LCDC_DOUBLE_HEIGHT_OBJECTS) object_height = 16;
    
    uint8_t *screen_line = &robingb_screen[(*ly) * SCREEN_WIDTH];
    
    uint16_t object_address;
    for (object_address = 0xfe9c; object_address >= 0xfe00; object_address -= 4) {
        int16_t translate_y = robingb_memory[object_address] - TILE_HEIGHT*2;
        
        if (*ly >= translate_y && *ly < translate_y+object_height) {
            int16_t translate_x = robingb_memory[object_address+1] - TILE_WIDTH;
            
            uint8_t tile_data_index = robingb_memory[object_address+2];
            
            /* ignore the lowest bit of the index if in double-height mode */
            if (object_height > 8) tile_data_index &= 0xfe;
            
            uint8_t object_flags = robingb_memory[object_address+3];
            bool choose_palette_1 = object_flags & robingb_bit(4);
            bool flip_x = object_flags & robingb_bit(5);
            bool flip_y = object_flags & robingb_bit(6);
            bool behind_background = object_flags & robingb_bit(7);
            
            if (choose_palette_1) set_palette(*object_palette_1);
            else set_palette(*object_palette_0);
            
            uint8_t tile_line[TILE_WIDTH];
            {
                int8_t tile_line_index = flip_y ? (translate_y+7 - *ly) : *ly - translate_y;
                get_tile_line(0x8000, tile_data_index, tile_line_index, tile_line);
            }
            
            uint8_t screen_x_start = translate_x < 0 ? 0 : translate_x;
            uint8_t screen_x_end = translate_x + TILE_WIDTH;
            
            if (flip_x) {
                uint8_t tile_pixel_index = translate_x < 0 ? (TILE_WIDTH-1)+translate_x : (TILE_WIDTH-1);
                
                if (behind_background) {
                    uint8_t screen_x;
                    for (screen_x = screen_x_start; screen_x < screen_x_end; screen_x++) {
                        uint8_t tile_pixel = tile_line[tile_pixel_index--];
                        
                        if (!(tile_pixel & SHADE_0_FLAG) && screen_line[screen_x] & SHADE_0_FLAG) {
                            screen_line[screen_x] = tile_pixel;
                        }
                    }
                } else {
                    uint8_t screen_x;
                    for (screen_x = screen_x_start; screen_x < screen_x_end; screen_x++) {
                        uint8_t tile_pixel = tile_line[tile_pixel_index--];
                        
                        if (!(tile_pixel & SHADE_0_FLAG)) {
                            screen_line[screen_x] = tile_pixel;
                        }
                    }
                }
            } else {
                uint8_t tile_pixel_index = translate_x < 0 ? -translate_x : 0;
                
                if (behind_background) {
                    uint8_t screen_x;
                    for (screen_x = screen_x_start; screen_x < screen_x_end; screen_x++) {
                        uint8_t tile_pixel = tile_line[tile_pixel_index++];
                        
                        if (!(tile_pixel & SHADE_0_FLAG) && screen_line[screen_x] & SHADE_0_FLAG) {
                            screen_line[screen_x] = tile_pixel;
                        }
                    }
                } else {
                    uint8_t screen_x;
                    for (screen_x = screen_x_start; screen_x < screen_x_end; screen_x++) {
                        uint8_t tile_pixel = tile_line[tile_pixel_index++];
                        
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
        uint16_t pixel_index;
        uint16_t screen_line_start = (*ly)*SCREEN_WIDTH;
        uint16_t screen_line_end = screen_line_start + SCREEN_WIDTH;
        
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







