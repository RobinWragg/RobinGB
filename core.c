#include "internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

Registers registers;
bool halted = false;

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
	uint16_t audio_buffer_size,
	const char *cart_file_path,
	void (*read_file_function_ptr)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[])
	) {
	
	assert(read_file_function_ptr);
	
	robingb_memory_init(cart_file_path, read_file_function_ptr);
	robingb_audio_init(audio_sample_rate, audio_buffer_size);
	
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

void robingb_update(u8 screen_out[], u8 *ly_out) {
	u8 previous_lcd_ly = *lcd_ly;
	
	robingb_screen = screen_out;
	assert(robingb_screen);
	
	while (*lcd_ly == previous_lcd_ly) {
		u8 num_cycles;
		robingb_execute_next_opcode(&num_cycles);
		
		robingb_handle_interrupts();
		robingb_lcd_update(num_cycles);
		robingb_audio_update(num_cycles);
		robingb_timer_update(num_cycles);
	}
	
	*ly_out = *lcd_ly;
}






