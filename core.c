#include "internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

Registers registers;
bool halted = false;
bool (*robingb_read_file)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[]);
bool (*robingb_write_file)(const char *path, bool append, uint32_t size, uint8_t buffer[]);
char *robingb_cart_path = NULL;
char *robingb_save_path = NULL;

void robingb_stack_push(u16 value) {
    u8 *bytes = (u8*)&value;
    registers.sp -= 2;
    robingb_memory_write(registers.sp, bytes[0]);
    robingb_memory_write(registers.sp+1, bytes[1]);
}

u16 robingb_stack_pop() {
    u16 value = robingb_memory_read_u16(registers.sp);
    registers.sp += 2;
    return value;
}

u16 make_u16(u8 least_sig, u8 most_sig) {
    union {
        u8 bytes[2];
        u16 out;
    } u16_union;
    
    u16_union.bytes[0] = least_sig;
    u16_union.bytes[1] = most_sig;
    return u16_union.out;
}

void init_registers() {
    registers.af = 0x01b0; /* NOTE: This is different for Game Boy Pocket, Color etc. */
    registers.bc = 0x0013;
    registers.de = 0x00d8;
    registers.hl = 0x014d;
    registers.sp = 0xfffe;
    registers.pc = 0x0100;
    registers.ime = true;
}

void robingb_init(
    uint32_t audio_sample_rate,
    const char *cart_file_path,
    bool (*read_file_function_ptr)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[]),
    bool (*write_file_function_ptr)(const char *path, bool append, uint32_t size, uint8_t buffer[])
    ) {
    
    assert(read_file_function_ptr);
    robingb_read_file = read_file_function_ptr;
    
    assert(write_file_function_ptr);
    robingb_write_file = write_file_function_ptr;
    
    /* Copy the cart path */
    if (robingb_cart_path) free(robingb_cart_path);
    robingb_cart_path = (char*)malloc(strlen(cart_file_path) + 1);
    strcpy(robingb_cart_path, cart_file_path);
    
    /* robingb_save_path shall be robingb_cart_path appended with '.save' */
    /* +1 for null terminator */
    if (robingb_save_path) free(robingb_save_path);
    robingb_save_path = (char*)malloc(strlen(robingb_cart_path) + strlen(".save") + 1);
    strcpy(robingb_save_path, robingb_cart_path);
    int str_end = strlen(robingb_save_path);
    robingb_save_path[str_end++] = '.';
    robingb_save_path[str_end++] = 's';
    robingb_save_path[str_end++] = 'a';
    robingb_save_path[str_end++] = 'v';
    robingb_save_path[str_end++] = 'e';
    robingb_save_path[str_end] = '\0';
    
    robingb_memory_init();
    robingb_audio_init(audio_sample_rate);
    
    /* barebones cart error check */
    {
        int sum = 0;
        int address;
        
        for (address = 0x0134; address <= 0x014D; address++) {
            sum += robingb_memory_read(address);
        }
        
        sum += 25;
        u8 *bytes = (u8*)&sum;
        assert(bytes[0] == 0);
    }
    
    init_registers();
    robingb_timer_init();
}

u8 *lcd_ly = &robingb_memory[LCD_LY_ADDRESS];

bool robingb_update_screen_line(u8 screen_out[], u8 *updated_screen_line) {
    u8 previous_lcd_ly = *lcd_ly;
    
    robingb_screen = screen_out;
    assert(robingb_screen);
    
    u32 num_cycles_this_h_blank = 0;
    
    while (*lcd_ly == previous_lcd_ly) {
        u8 num_cycles_this_opcode;
        robingb_execute_next_opcode(&num_cycles_this_opcode);
        
        robingb_handle_interrupts();
        robingb_lcd_update(num_cycles_this_opcode);
        robingb_timer_update(num_cycles_this_opcode);
        
        num_cycles_this_h_blank += num_cycles_this_opcode;
    }
    
    robingb_audio_update(num_cycles_this_h_blank);
    
    if (previous_lcd_ly < 144) {
        *updated_screen_line = previous_lcd_ly;
        return true;
    } else return false;
}

void robingb_update_screen(uint8_t screen_out[]) {
    u8 updated_screen_line;
    
    /* Call the function until the vblank phase is exited */
    while (robingb_update_screen_line(screen_out, &updated_screen_line) == false) {}
    
    /* Call the function until the vblank phase is entered again */
    while (robingb_update_screen_line(screen_out, &updated_screen_line) == true) {}
    
    /* The screen has now been fully updated */
}






