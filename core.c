#include "internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

Registers registers;
bool halted = false;

void (*robingb_logging_function_ptr)(const char *text) = 0;

void robingb_log_with_prefix(const char *prefix, const char *main_body) {
	if (robingb_logging_function_ptr) {
		char *buf = malloc(strlen(prefix) + strlen(main_body) + 5);
		buf[0] = '\0';
		strcat(buf, prefix);
		strcat(buf, "(): ");
		strcat(buf, main_body);
		
		robingb_logging_function_ptr(buf);
		
		free(buf);
	}
}

void stack_push(u16 value) {
	u8 *bytes = (u8*)&value;
	registers.sp -= 2;
	mem_write(registers.sp, bytes[0]);
	mem_write(registers.sp+1, bytes[1]);
}

u16 stack_pop() {
	u16 value = mem_read_u16(registers.sp);
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

void zero_unused_f_register_bits() {
	registers.f &= 0xf0;
}

void (*robingb_read_file)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[]) = 0;

void robingb_init(
	const char *cart_file_path,
	void (*read_file_function_ptr)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[])
	) {
	
	robingb_read_file = read_file_function_ptr;
	assert(robingb_read_file);
	
	mem_init(cart_file_path);
	
	{
		char buf[128] = {0};
		strcpy(buf, "Name: ");
		for (int i = 0x0134; i < 0x0143; i++) {
			s8 b = mem_read(i);
			if (b == '\0') break;
			sprintf(buf, "%s%c", buf, b);
		}
		robingb_log(buf);
	}
	
	/* barebones cart error check */
	{
		int sum = 0;
		for (int addr = 0x0134; addr <= 0x014D; addr++) {
			sum += mem_read(addr);
		}
		sum += 25;
		u8 *bytes = (u8*)&sum;
		assert(bytes[0] == 0);
	}
	
	init_registers();
	init_timer();
}

u8 *lcd_ly = &robingb_memory[LCD_LY_ADDRESS];

void robingb_update(RobinGB_Input *input, u8 screen_out[], u8 *ly_out) {
	u8 prev_lcd_ly = *lcd_ly;
	
	robingb_screen = screen_out;
	assert(robingb_screen);
	
	while (*lcd_ly == prev_lcd_ly) {
		u8 num_cycles = 4;
		
		if (!halted) {
			execute_next_opcode(&num_cycles);
			zero_unused_f_register_bits();
		}
		
		handle_interrupts();
		lcd_update(num_cycles);
		joypad_update(input);
		update_audio(num_cycles);
		update_timer(num_cycles);
	}
	
	*ly_out = *lcd_ly;
}






