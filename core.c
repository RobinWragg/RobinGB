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
	registers.af = 0x01b0; // NOTE: This is different for Game Boy Pocket, Color etc.
	registers.bc = 0x0013;
	registers.de = 0x00d8;
	registers.hl = 0x014d;
	registers.sp = 0xfffe;
	registers.pc = 0x0100;
	registers.ime = true;
}

void report(const char *asm_log) {
	char buf[128] = {0};
	
	// sprintf(buf, "%s\n", asm_log);
	// robingb_log(buf);
	
	// sprintf(buf, "AF:%2x%2x   BC:%2x%2x   DE:%2x%2x   HL:%2x%2x   SP:%4x   PC:%4x\n", registers.a, registers.f, registers.b, registers.c, registers.d, registers.e, registers.h, registers.l, registers.sp, registers.pc);
	// robingb_log(buf);
	
	// sprintf(buf, "IE:%2x   IF:%2x   IME:%i\n", mem_read(IE_ADDRESS), mem_read(IF_ADDRESS), registers.ime);
	// robingb_log(buf);
	
	// sprintf(buf, "Flags: Z:%i  N:%i  H:%i  C:%i\n", (registers.f & FLAG_Z) > 0, (registers.f & FLAG_N) > 0, (registers.f & FLAG_H) > 0, (registers.f & FLAG_C) > 0);
	// robingb_log(buf);
	
	// sprintf(buf, "LY: %x\n", mem_read(0xff44));
	// robingb_log(buf);
	
	strcpy(buf, "\n");
	robingb_log(buf);
}

void zero_unused_f_register_bits() {
	registers.f &= 0xf0;
}

void (*robingb_read_file)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[]) = 0;

void robingb_init(const char *rom_file_path, void (*read_file_function_ptr)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[])) {
	
	robingb_read_file = read_file_function_ptr;
	assert(robingb_read_file);
	
	mem_init(rom_file_path);
	
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
	
	// barebones cart error check
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

// TODO: robingb_update() should execute until LY increments.
int robingb_update(RobinGB_Input *input) {
	
	mem_remove_all_logs();
	
	// char buf[128] = {0};
	// sprintf(buf, "Address %4x: %2x %2x %2x %2x\n", registers.pc, mem_read(registers.pc), mem_read(registers.pc+1), mem_read(registers.pc+2), mem_read(registers.pc+3));
	// robingb_log(buf);
	
	u8 num_cycles;
	
	mem_logging_enabled = true;
	execute_next_opcode(&num_cycles);
	mem_logging_enabled = false;
	
	zero_unused_f_register_bits();
	handle_interrupts();
	lcd_update(num_cycles);
	joypad_update(input);
	update_audio(num_cycles);
	update_timer(num_cycles);
	
	// show_write_progression();
	// report(asm_log);
	
	return num_cycles;
}






