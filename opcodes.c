#include "internal.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

// TODO: 8bit ADC func
// TODO: 8bit CP func

char *asm_log_for_finish;
u8 *num_cycles_for_finish = 0;

void finish_instruction(const char *desc, u16 pc_increment, u8 num_cycles_param) {
	assert(num_cycles_for_finish);
	registers.pc += pc_increment;
	*num_cycles_for_finish = num_cycles_param;
	
	strcpy(asm_log_for_finish, desc);
}

bool negate_produces_u8_half_carry(s16 a, s16 b) {
	if ((a & 0x0f) - (b & 0x0f) < 0) return true;
	else return false;
}

bool addition_produces_u8_half_carry(s16 a, s16 b) {
	if (a + b > 0x0f) return true;
	else return false;
}

bool negate_produces_u8_full_carry(s16 a, s16 b) {
	if (a - b < 0) return true;
	else return false;
}

bool addition_produces_u8_full_carry(s16 a, s16 b) {
	if (a + b > 0xff) return true;
	else return false;
}

bool addition_produces_u16_half_carry(u16 a, u16 b) {
	u16 a_bit_11_and_under = a & 0xfff;
	u16 b_bit_11_and_under = b & 0xfff;
	if (a_bit_11_and_under + b_bit_11_and_under > 0x0fff) return true;
	else return false;
}

bool addition_produces_u16_full_carry(s32 a, s32 b) {
	if (a + b > 0xffff) return true;
	else return false;
}

void execute_instruction_DEC_u8(u8 *value_to_decrement, const char *asm_name, u8 num_cycles) {
	if (negate_produces_u8_half_carry(*value_to_decrement, 1)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	(*value_to_decrement)--;
	
	if (*value_to_decrement == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f |= FLAG_N;
	
	finish_instruction(asm_name, 1, num_cycles);
}

void execute_instruction_INC_u8(u8 *value_to_increment, const char *asm_name, u8 num_cycles) {
	if (addition_produces_u8_half_carry(*value_to_increment, 1)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;

	(*value_to_increment)++;

	if (*value_to_increment == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;

	registers.f &= ~FLAG_N;

	finish_instruction(asm_name, 1, num_cycles);
}

void execute_instruction_ADC(u8 to_add, const char *asm_name, u16 pc_increment, int num_cycles) {
	int carry = registers.f & FLAG_C ? 1 : 0;
	
	if (addition_produces_u8_half_carry(registers.a, to_add + carry)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	if (addition_produces_u8_full_carry(registers.a, to_add + carry)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	registers.a += to_add + carry;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	
	finish_instruction(asm_name, pc_increment, num_cycles);
}

void execute_instruction_SBC(u8 to_subtract, const char *asm_name, u16 pc_increment, int num_cycles) {
	int carry = registers.f & FLAG_C ? 1 : 0;
	
	if (negate_produces_u8_half_carry(registers.a, to_subtract + carry)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	if (negate_produces_u8_full_carry(registers.a, to_subtract + carry)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	registers.a -= to_subtract + carry;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f |= FLAG_N;
	
	finish_instruction(asm_name, pc_increment, num_cycles);
}

void execute_instruction_OR(u8 right_hand_value, const char *asm_name, u16 pc_increment, int num_cycles) {
	registers.a |= right_hand_value;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	registers.f &= ~FLAG_C;
	finish_instruction(asm_name, pc_increment, num_cycles);
}

void execute_instruction_ADD_A_u8(u8 to_add, const char *asm_name, u16 pc_increment, u8 num_cycles) {
	if (addition_produces_u8_half_carry(registers.a, to_add)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	if (addition_produces_u8_full_carry(registers.a, to_add)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	registers.a += to_add;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	
	finish_instruction(asm_name, pc_increment, num_cycles);
}

void execute_instruction_ADD_HL_u16(u16 to_add, const char *asm_name, u16 pc_increment, u8 num_cycles) {
	if (addition_produces_u16_half_carry(registers.hl, to_add)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	if (addition_produces_u16_full_carry(registers.hl, to_add)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	registers.hl += to_add;
	
	registers.f &= ~FLAG_N;
	
	finish_instruction(asm_name, pc_increment, num_cycles);
}

void execute_instruction_SUB_u8(u8 subber, const char *asm_name, u16 pc_increment, int num_cycles) {
	if (negate_produces_u8_half_carry(registers.a, subber)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	if (negate_produces_u8_full_carry(registers.a, subber)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	registers.a -= subber;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f |= FLAG_N;
	
	finish_instruction(asm_name, pc_increment, num_cycles);
}

void execute_opcode(u8 opcode, u8 *num_cycles_out, char *asm_log_out) {
	num_cycles_for_finish = num_cycles_out;
	asm_log_for_finish = asm_log_out;
	
	switch (opcode) {
		case 0x00: finish_instruction("NOP", 1, 4); break;
		case 0x01: registers.bc = mem_read_u16(registers.pc+1); finish_instruction("LD BC,xx", 3, 12); break;
		case 0x02: mem_write(registers.bc, registers.a); finish_instruction("LD (BC),A", 1, 8); break;
		case 0x03: registers.bc++; finish_instruction("INC BC", 1, 8); break;
		case 0x04: execute_instruction_INC_u8(&registers.b, "INC b", 4); break;
		case 0x05: execute_instruction_DEC_u8(&registers.b, "DEC B", 4); break;
		case 0x06: registers.b = mem_read(registers.pc+1); finish_instruction("LD B,x", 2, 8); break;
		case 0x07: {
			if (registers.a & 0x80) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.a = registers.a << 1;
			
			registers.f &= ~FLAG_Z;
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			
			finish_instruction("RLCA", 1, 4);
		} break;
		case 0x08: mem_write_u16(mem_read_u16(registers.pc+1), registers.sp); finish_instruction("LD (xx),SP", 3, 20); break;
		case 0x09: execute_instruction_ADD_HL_u16(registers.bc, "ADD HL,BC", 1, 8); break;
		case 0x0a: registers.a = mem_read(registers.bc); finish_instruction("LD A,(BC)", 1, 8); break;
		case 0x0b: registers.bc--; finish_instruction("DEC BC", 1, 8); break;
		case 0x0c: execute_instruction_INC_u8(&registers.c, "INC C", 4); break;
		case 0x0d: execute_instruction_DEC_u8(&registers.c, "DEC C", 4); break;
		case 0x0e: registers.c = mem_read(registers.pc+1); finish_instruction("LD C,x", 2, 8); break;
		case 0x0f: {
			if (registers.a & 0x01) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.a = registers.a >> 1;
			
			registers.f &= ~FLAG_Z;
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			
			finish_instruction("RRCA", 1, 4);
		} break;
		case 0x11: registers.de = mem_read_u16(registers.pc+1); finish_instruction("LD DE,xx", 3, 12); break;
		case 0x12: mem_write(registers.de, registers.a); finish_instruction("LD (DE),A", 1, 8); break;
		case 0x13: registers.de++; finish_instruction("INC DE", 1, 8); break;
		case 0x14: execute_instruction_INC_u8(&registers.d, "INC D", 4); break;
		case 0x15: execute_instruction_DEC_u8(&registers.d, "DEC D", 4); break;
		case 0x16: registers.d = mem_read(registers.pc+1); finish_instruction("LD D,x", 2, 8); break;
		case 0x17: {
			bool prev_carry = registers.f & FLAG_C;
			
			if (registers.a & 0x80) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.a = registers.a << 1;
			
			if (prev_carry) registers.a |= 0x01;
			
			registers.f &= ~FLAG_Z;
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			
			finish_instruction("RLA", 1, 4);
		} break;
		case 0x18: finish_instruction("JR %i(d)", 2 + (s8)mem_read(registers.pc+1), 12); break;
		case 0x19: execute_instruction_ADD_HL_u16(registers.de, "ADD HL,DE", 1, 8); break;
		case 0x1a: registers.a = mem_read(registers.de); finish_instruction("LD A,(DE)", 1, 8); break;
		case 0x1b: registers.de--; finish_instruction("DEC DE", 1, 8); break;
		case 0x1c: execute_instruction_INC_u8(&registers.e, "INC E", 4); break;
		case 0x1d: execute_instruction_DEC_u8(&registers.e, "DEC E", 4); break;
		case 0x1e: registers.e = mem_read(registers.pc+1); finish_instruction("LD E,x", 2, 8); break;
		case 0x1f: {
			bool prev_carry = registers.f & FLAG_C;
			
			if (registers.a & 0x01) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.a = registers.a >> 1;
			
			if (prev_carry) registers.a |= 0x80;
			
			registers.f &= ~FLAG_Z;
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			
			finish_instruction("RRA", 1, 4);
		} break;
		case 0x20: {
			u8 byte_0 = mem_read(registers.pc+1);
			if ((registers.f & FLAG_Z) == 0) finish_instruction("JR NZ,s", 2 + (s8)byte_0, 12);
			else finish_instruction("JR NZ,s", 2, 8);
		} break;
		case 0x21: registers.hl = mem_read_u16(registers.pc+1); finish_instruction("LD HL,xx", 3, 12); break;
		case 0x22: mem_write(registers.hl++, registers.a); finish_instruction("LD (HL+),A", 1, 8); break;
		case 0x23:  registers.hl++; finish_instruction("INC HL", 1, 8); break;
		case 0x24: execute_instruction_INC_u8(&registers.h, "INC H", 4); break;
		case 0x25: execute_instruction_DEC_u8(&registers.h, "DEC H", 4); break;
		case 0x26: {
			registers.h = mem_read(registers.pc+1);
			finish_instruction("LD H,x", 2, 8);
		} break;
		case 0x27: {
			if ((registers.f & FLAG_N) == 0) { // after an addition, adjust if (half-)carry occurred or if result is out of bounds
				if ((registers.f & FLAG_C) || registers.a > 0x99) {
					registers.a += 0x60;
					registers.f |= FLAG_C;
				}
				if ((registers.f & FLAG_H) || (registers.a & 0x0f) > 0x09) registers.a += 0x6;
			} else { // after a subtraction, only adjust if (half-)carry occurred
				if (registers.f & FLAG_C) registers.a -= 0x60;
				if (registers.f & FLAG_H) registers.a -= 0x6;
			}
			
			// these flags are always updated
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_H;
			
			finish_instruction("DAA", 1, 4);
		} break;
		case 0x28: {
			u8 byte_0 = mem_read(registers.pc+1);
			if (registers.f & FLAG_Z) finish_instruction("JR Z,s", 2 + (s8)byte_0, 12);
			else finish_instruction("JR Z,s", 2, 8);
		} break;
		case 0x29: execute_instruction_ADD_HL_u16(registers.hl, "ADD HL,HL", 1, 8); break;
		case 0x2a: registers.a = mem_read(registers.hl++); finish_instruction("LD A,(HL+)", 1, 8); break;
		case 0x2b: registers.hl--; finish_instruction("DEC HL", 1, 8); break;
		case 0x2c: execute_instruction_INC_u8(&registers.l, "INC L", 4); break;
		case 0x2d: execute_instruction_DEC_u8(&registers.l, "DEC L", 4); break;
		case 0x2e: registers.l = mem_read(registers.pc+1); finish_instruction("LD L,x", 2, 8); break;
		case 0x2f: {
			registers.a ^= 0xff;
			registers.f |= FLAG_N;
			registers.f |= FLAG_H;
			finish_instruction("CPL", 1, 4);
		} break;
		case 0x30: {
			u8 byte_0 = mem_read(registers.pc+1);
			if ((registers.f & FLAG_C) == 0) finish_instruction("JR NC,s", 2 + (s8)byte_0, 12);
			else finish_instruction("JR NC,s", 2, 8);
		} break;
		case 0x31: registers.sp = mem_read_u16(registers.pc+1); finish_instruction("LD SP,xx", 3, 12); break;
		case 0x32: mem_write(registers.hl--, registers.a); finish_instruction("LD (HL-),A", 1, 8); break;
		case 0x33:  registers.sp++; finish_instruction("INC SP", 1, 8); break;
		case 0x34: {
			u8 hl_value = mem_read(registers.hl);
			execute_instruction_INC_u8(&hl_value, "INC (HL)", 12);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x35: {
			u8 hl_value = mem_read(registers.hl);
			execute_instruction_DEC_u8(&hl_value, "DEC (HL)", 12);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x36: mem_write(registers.hl, mem_read(registers.pc+1)); finish_instruction("LD (HL),x", 2, 12); break;
		case 0x37: {
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f |= FLAG_C;
			finish_instruction("SCF", 1, 4);
		} break;
		case 0x38: {
			u8 byte_0 = mem_read(registers.pc+1);
			if (registers.f & FLAG_C) finish_instruction("JR C,s", 2 + (s8)byte_0, 12);
			else finish_instruction("JR C,s", 2, 8);
		} break;
		case 0x39: execute_instruction_ADD_HL_u16(registers.sp, "ADD HL,SP", 1, 8); break;
		case 0x3a: registers.a = mem_read(registers.hl--); finish_instruction("LD A,(HL-)", 1, 8); break;
		case 0x3b: registers.sp--; finish_instruction("DEC SP", 1, 8); break;
		case 0x3c: execute_instruction_INC_u8(&registers.a, "INC A", 4); break;
		case 0x3d: execute_instruction_DEC_u8(&registers.a, "DEC A", 4); break;
		case 0x3e: registers.a = mem_read(registers.pc+1); finish_instruction("LD A,x", 2, 8); break;
		case 0x3f: {
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f ^= FLAG_C;
			finish_instruction("CCF", 1, 4);
		} break;
		case 0x40: registers.b = registers.b; finish_instruction("LD B,B", 1, 4); break;
		case 0x41: registers.b = registers.c; finish_instruction("LD B,C", 1, 4); break;
		case 0x42: registers.b = registers.d; finish_instruction("LD B,D", 1, 4); break;
		case 0x43: registers.b = registers.e; finish_instruction("LD B,E", 1, 4); break;
		case 0x44: registers.b = registers.h; finish_instruction("LD B,H", 1, 4); break;
		case 0x45: registers.b = registers.l; finish_instruction("LD B,L", 1, 4); break;
		case 0x46: registers.b = mem_read(registers.hl); finish_instruction("LD B,(HL)", 1, 8); break;
		case 0x47: registers.b = registers.a; finish_instruction("LD B,A", 1, 4); break;
		case 0x48: registers.c = registers.b; finish_instruction("LD C,B", 1, 4); break;
		case 0x49: registers.c = registers.c; finish_instruction("LD C,C", 1, 4); break;
		case 0x4a: registers.c = registers.d; finish_instruction("LD C,D", 1, 4); break;
		case 0x4b: registers.c = registers.e; finish_instruction("LD C,E", 1, 4); break;
		case 0x4c: registers.c = registers.h; finish_instruction("LD C,H", 1, 4); break;
		case 0x4d: registers.c = registers.l; finish_instruction("LD C,L", 1, 4); break;
		case 0x4e: registers.c = mem_read(registers.hl); finish_instruction("LD C,(HL)", 1, 8); break;
		case 0x4f: registers.c = registers.a; finish_instruction("LD C,A", 1, 4); break;
		case 0x50: registers.d = registers.b; finish_instruction("LD D,B", 1, 4); break;
		case 0x51: registers.d = registers.c; finish_instruction("LD D,C", 1, 4); break;
		case 0x52: registers.d = registers.d; finish_instruction("LD D,D", 1, 4); break;
		case 0x53: registers.d = registers.e; finish_instruction("LD D,E", 1, 4); break;
		case 0x54: registers.d = registers.h; finish_instruction("LD D,H", 1, 4); break;
		case 0x55: registers.d = registers.l; finish_instruction("LD D,L", 1, 4); break;
		case 0x56: registers.d = mem_read(registers.hl); finish_instruction("LD D,(HL)", 1, 8); break;
		case 0x57: registers.d = registers.a; finish_instruction("LD D,A", 1, 4); break;
		case 0x58: registers.e = registers.b; finish_instruction("LD E,B", 1, 4); break;
		case 0x59: registers.e = registers.c; finish_instruction("LD E,C", 1, 4); break;
		case 0x5a: registers.e = registers.d; finish_instruction("LD E,D", 1, 4); break;
		case 0x5b: registers.e = registers.e; finish_instruction("LD E,E", 1, 4); break;
		case 0x5c: registers.e = registers.h; finish_instruction("LD E,H", 1, 4); break;
		case 0x5d: registers.e = registers.l; finish_instruction("LD E,L", 1, 4); break;
		case 0x5e: registers.e = mem_read(registers.hl); finish_instruction("LD E,(HL)", 1, 8); break;
		case 0x5f: registers.e = registers.a; finish_instruction("LD E,A", 1, 4); break;
		case 0x60: registers.h = registers.b; finish_instruction("LD H,B", 1, 4); break;
		case 0x61: registers.h = registers.c; finish_instruction("LD H,C", 1, 4); break;
		case 0x62: registers.h = registers.d; finish_instruction("LD H,D", 1, 4); break;
		case 0x63: registers.h = registers.e; finish_instruction("LD H,E", 1, 4); break;
		case 0x64: registers.h = registers.h; finish_instruction("LD H,H", 1, 4); break;
		case 0x65: registers.h = registers.l; finish_instruction("LD H,L", 1, 4); break;
		case 0x66: registers.h = mem_read(registers.hl); finish_instruction("LD H,(HL)", 1, 8); break;
		case 0x67: registers.h = registers.a; finish_instruction("LD H,A", 1, 4); break;
		case 0x68: registers.l = registers.b; finish_instruction("LD L,B", 1, 4); break;
		case 0x69: registers.l = registers.c; finish_instruction("LD L,C", 1, 4); break;
		case 0x6a: registers.l = registers.d; finish_instruction("LD L,D", 1, 4); break;
		case 0x6b: registers.l = registers.e; finish_instruction("LD L,E", 1, 4); break;
		case 0x6c: registers.l = registers.h; finish_instruction("LD L,H", 1, 4); break;
		case 0x6d: registers.l = registers.l; finish_instruction("LD L,L", 1, 4); break;
		case 0x6e: registers.l = mem_read(registers.hl); finish_instruction("LD L,(HL)", 1, 8); break;
		case 0x6f: registers.l = registers.a; finish_instruction("LD L,A", 1, 4); break;
		case 0x70: mem_write(registers.hl, registers.b); finish_instruction("LD (HL),B", 1, 8); break;
		case 0x71: mem_write(registers.hl, registers.c); finish_instruction("LD (HL),C", 1, 8); break;
		case 0x72: mem_write(registers.hl, registers.d); finish_instruction("LD (HL),D", 1, 8); break;
		case 0x73: mem_write(registers.hl, registers.e); finish_instruction("LD (HL),E", 1, 8); break;
		case 0x74: mem_write(registers.hl, registers.h); finish_instruction("LD (HL),H", 1, 8); break;
		case 0x75: mem_write(registers.hl, registers.l); finish_instruction("LD (HL),L", 1, 8); break;
		case 0x76: {
			if (halted) finish_instruction("(still halted)", 0, 4);
			else {
				if (registers.ime) {
					halted = true;
					finish_instruction("HALT", 0, 4);
				} else finish_instruction("HALT", 1, 4);
			}
		} break;
		case 0x77: mem_write(registers.hl, registers.a); finish_instruction("LD (HL),A", 1, 8); break;
		case 0x78: registers.a = registers.b; finish_instruction("LD A,B", 1, 4); break;
		case 0x79: registers.a = registers.c; finish_instruction("LD A,C", 1, 4); break;
		case 0x7a: registers.a = registers.d; finish_instruction("LD A,D", 1, 4); break;
		case 0x7b: registers.a = registers.e; finish_instruction("LD A,E", 1, 4); break;
		case 0x7c: registers.a = registers.h; finish_instruction("LD A,H", 1, 4); break;
		case 0x7d: registers.a = registers.l; finish_instruction("LD A,L", 1, 4); break;
		case 0x7e: registers.a = mem_read(registers.hl); finish_instruction("LD A,(HL)", 1, 8); break;
		case 0x7f: registers.a = registers.a; finish_instruction("LD A,A", 1, 4); break;
		case 0x80: execute_instruction_ADD_A_u8(registers.b, "ADD A,B", 1, 4); break;
		case 0x81: execute_instruction_ADD_A_u8(registers.c, "ADD A,C", 1, 4); break;
		case 0x82: execute_instruction_ADD_A_u8(registers.d, "ADD A,D", 1, 4); break;
		case 0x83: execute_instruction_ADD_A_u8(registers.e, "ADD A,E", 1, 4); break;
		case 0x84: execute_instruction_ADD_A_u8(registers.h, "ADD A,H", 1, 4); break;
		case 0x85: execute_instruction_ADD_A_u8(registers.l, "ADD A,L", 1, 4); break;
		case 0x86: execute_instruction_ADD_A_u8(mem_read(registers.hl), "ADD A,(HL)", 1, 8); break;
		case 0x87: execute_instruction_ADD_A_u8(registers.a, "ADD A,A", 1, 4); break;
		case 0x88: execute_instruction_ADC(registers.b, "ADC A,B", 1, 4); break;
		case 0x89: execute_instruction_ADC(registers.c, "ADC A,C", 1, 4); break;
		case 0x8a: execute_instruction_ADC(registers.d, "ADC A,D", 1, 4); break;
		case 0x8b: execute_instruction_ADC(registers.e, "ADC A,E", 1, 4); break;
		case 0x8c: execute_instruction_ADC(registers.h, "ADC A,H", 1, 4); break;
		case 0x8d: execute_instruction_ADC(registers.l, "ADC A,L", 1, 4); break;
		case 0x8e: execute_instruction_ADC(mem_read(registers.hl), "ADC A,(HL)", 1, 8); break;
		case 0x8f: execute_instruction_ADC(registers.a, "ADC A,A", 1, 4); break;
		case 0x90: execute_instruction_SUB_u8(registers.b, "SUB B", 1, 4); break;
		case 0x91: execute_instruction_SUB_u8(registers.c, "SUB C", 1, 4); break;
		case 0x92: execute_instruction_SUB_u8(registers.d, "SUB D", 1, 4); break;
		case 0x93: execute_instruction_SUB_u8(registers.e, "SUB E", 1, 4); break;
		case 0x94: execute_instruction_SUB_u8(registers.h, "SUB H", 1, 4); break;
		case 0x95: execute_instruction_SUB_u8(registers.l, "SUB L", 1, 4); break;
		case 0x96: execute_instruction_SUB_u8(mem_read(registers.hl), "SUB (HL)", 1, 8); break;
		case 0x97: execute_instruction_SUB_u8(registers.a, "SUB A", 1, 4); break;
		case 0x98: execute_instruction_SBC(registers.b, "SBC A,B", 1, 4); break;	
		case 0x99: execute_instruction_SBC(registers.c, "SBC A,C", 1, 4); break;
		case 0x9a: execute_instruction_SBC(registers.d, "SBC A,D", 1, 4); break;
		case 0x9b: execute_instruction_SBC(registers.e, "SBC A,E", 1, 4); break;
		case 0x9c: execute_instruction_SBC(registers.h, "SBC A,H", 1, 4); break;
		case 0x9d: execute_instruction_SBC(registers.l, "SBC A,L", 1, 4); break;
		case 0x9e: execute_instruction_SBC(mem_read(registers.hl), "SBC A,(HL)", 1, 8); break;
		case 0x9f: execute_instruction_SBC(registers.a, "SBC A,A", 1, 4); break;
		case 0xa0: {
			registers.a &= registers.b;
			
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_N;
			registers.f |= FLAG_H;
			registers.f &= ~FLAG_C;
			
			finish_instruction("AND B", 1, 4);
		} break;
		case 0xa1: {
			registers.a &= registers.c;
			
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_N;
			registers.f |= FLAG_H;
			registers.f &= ~FLAG_C;
			
			finish_instruction("AND C", 1, 4);
		} break;
		case 0xa7: {
			registers.a &= registers.a;
			
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_N;
			registers.f |= FLAG_H;
			registers.f &= ~FLAG_C;
			finish_instruction("AND A", 1, 4);
		} break;
		case 0xa8: {
			registers.a ^= registers.b;
			
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f &= ~FLAG_C;
			finish_instruction("XOR B", 1, 4);
		} break;
		case 0xa9: {
			registers.a ^= registers.c;
			
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f &= ~FLAG_C;
			finish_instruction("XOR C", 1, 4);
		} break;
		case 0xad: {
			registers.a ^= registers.l;
			
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f &= ~FLAG_C;
			finish_instruction("XOR L", 1, 4);
		} break;
		case 0xae: {
			registers.a ^= mem_read(registers.hl);
			
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f &= ~FLAG_C;
			finish_instruction("XOR (HL)", 1, 8);
		} break;
		case 0xaf: {
			registers.a ^= registers.a;
			
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f &= ~FLAG_C;
			finish_instruction("XOR A", 1, 4);
		} break;
		case 0xb0: execute_instruction_OR(registers.b, "OR B", 1, 4); break;
		case 0xb1: execute_instruction_OR(registers.c, "OR C", 1, 4); break;
		case 0xb2: execute_instruction_OR(registers.d, "OR D", 1, 4); break;
		case 0xb3: execute_instruction_OR(registers.e, "OR E", 1, 4); break;
		case 0xb4: execute_instruction_OR(registers.h, "OR H", 1, 4); break;
		case 0xb5: execute_instruction_OR(registers.l, "OR L", 1, 4); break;
		case 0xb6: execute_instruction_OR(mem_read(registers.hl), "OR (HL)", 1, 8); break;
		case 0xb7: execute_instruction_OR(registers.a, "OR A", 1, 4); break;
		case 0xb8: {
			if (negate_produces_u8_half_carry(registers.a, registers.b)) registers.f |= FLAG_H;
			else registers.f &= ~FLAG_H;
			
			if (negate_produces_u8_full_carry(registers.a, registers.b)) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.f |= FLAG_N; // set the add/sub flag high, indicating subtraction
			
			u8 sub_result = registers.a - registers.b;
			
			if (sub_result == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			finish_instruction("CP B", 1, 4);
		} break;
		case 0xb9: {
			if (negate_produces_u8_half_carry(registers.a, registers.c)) registers.f |= FLAG_H;
			else registers.f &= ~FLAG_H;
			
			if (negate_produces_u8_full_carry(registers.a, registers.c)) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.f |= FLAG_N; // set the add/sub flag high, indicating subtraction
			
			u8 sub_result = registers.a - registers.c;
			
			if (sub_result == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			finish_instruction("CP C", 1, 4);
		} break;
		case 0xba: {
			if (negate_produces_u8_half_carry(registers.a, registers.d)) registers.f |= FLAG_H;
			else registers.f &= ~FLAG_H;
			
			if (negate_produces_u8_full_carry(registers.a, registers.d)) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.f |= FLAG_N; // set the add/sub flag high, indicating subtraction
			
			u8 sub_result = registers.a - registers.d;
			
			if (sub_result == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			finish_instruction("CP D", 1, 4);
		} break;
		case 0xbb: {
			if (negate_produces_u8_half_carry(registers.a, registers.e)) registers.f |= FLAG_H;
			else registers.f &= ~FLAG_H;
			
			if (negate_produces_u8_full_carry(registers.a, registers.e)) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.f |= FLAG_N; // set the add/sub flag high, indicating subtraction
			
			u8 sub_result = registers.a - registers.e;
			
			if (sub_result == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			finish_instruction("CP E", 1, 4);
		} break;
		case 0xbc: {
			if (negate_produces_u8_half_carry(registers.a, registers.h)) registers.f |= FLAG_H;
			else registers.f &= ~FLAG_H;
			
			if (negate_produces_u8_full_carry(registers.a, registers.h)) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.f |= FLAG_N; // set the add/sub flag high, indicating subtraction
			
			u8 sub_result = registers.a - registers.h;
			
			if (sub_result == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			finish_instruction("CP H", 1, 4);
		} break;
		case 0xbd: {
			if (negate_produces_u8_half_carry(registers.a, registers.l)) registers.f |= FLAG_H;
			else registers.f &= ~FLAG_H;
			
			if (negate_produces_u8_full_carry(registers.a, registers.l)) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.f |= FLAG_N; // set the add/sub flag high, indicating subtraction
			
			u8 sub_result = registers.a - registers.l;
			
			if (sub_result == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			finish_instruction("CP L", 1, 4);
		} break;
		case 0xbe: {
			u8 value = mem_read(registers.hl);
			
			if (negate_produces_u8_half_carry(registers.a, value)) registers.f |= FLAG_H;
			else registers.f &= ~FLAG_H;
			
			if (negate_produces_u8_full_carry(registers.a, value)) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.f |= FLAG_N; // set the add/sub flag high, indicating subtraction
			
			u8 sub_result = registers.a - value;
			
			if (sub_result == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			finish_instruction("CP (HL)", 1, 8);
		} break;
		case 0xc0: {
			if (registers.f & FLAG_Z) finish_instruction("RET NZ", 1, 8);
			else {
				registers.pc = stack_pop();
				finish_instruction("RET NZ", 0, 20);
			}
		} break;
		case 0xc1: {
			registers.bc = stack_pop();
			finish_instruction("POP BC", 1, 12);
		} break;
		case 0xc2: {
			if ((registers.f & FLAG_Z) == 0) {
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction("JP NZ,xx", 0, 16);
			} else {
				finish_instruction("JP NZ,xx", 3, 12);
			}
		} break;
		case 0xc3: {
			registers.pc = mem_read_u16(registers.pc+1);
			finish_instruction("JP xx", 0, 16);
		} break;
		case 0xc4: {
			if (registers.f & FLAG_Z) finish_instruction("CALL NZ,xx", 3, 12);
			else {
				stack_push(registers.pc + 3);
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction("CALL NZ,xx", 0, 24);
			}
		} break;
		case 0xc5: {
			stack_push(registers.bc);
			finish_instruction("PUSH BC", 1, 16);
		} break;
		case 0xc6: execute_instruction_ADD_A_u8(mem_read(registers.pc+1), "ADD A,x", 2, 8); break;
		case 0xc8: {
			if (registers.f & FLAG_Z) {
				registers.pc = stack_pop();
				finish_instruction("RET Z", 0, 20);
			} else finish_instruction("RET Z", 1, 8);
		} break;
		case 0xc9: {
			registers.pc = stack_pop();
			finish_instruction("RET", 0, 16);
		} break;
		case 0xca: {
			if (registers.f & FLAG_Z) {
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction("JP Z,xx", 0, 16);
			} else finish_instruction("JP Z,xx", 3, 12);
		} break;
		case 0xcb: {
			execute_cb_opcode();
		} break;
		case 0xcc: {
			if (registers.f & FLAG_Z) {
				stack_push(registers.pc + 3);
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction("CALL Z,xx", 0, 24);
			} else finish_instruction("CALL Z,xx", 3, 12);
		} break;
		case 0xcd: {
			stack_push(registers.pc + 3);
			registers.pc = mem_read_u16(registers.pc+1);
			finish_instruction("CALL xx", 0, 24);
		} break;
		case 0xce: execute_instruction_ADC(mem_read(registers.pc+1), "ADC A,x", 2, 8); break;
		case 0xd0: {
			if ((registers.f & FLAG_C) == false) {
				registers.pc = stack_pop();
				finish_instruction("RET NC", 0, 20);
			} else finish_instruction("RET NC", 1, 8);
		} break;
		case 0xd1: {
			registers.de = stack_pop();
			finish_instruction("POP DE", 1, 12);
		} break;
		case 0xd2: {
			if ((registers.f & FLAG_C) == 0) {
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction("JP NC,xx", 0, 16);
			} else {
				finish_instruction("JP NC,xx", 3, 12);
			}
		} break;
		case 0xd4: {
			if ((registers.f & FLAG_C) == 0) {
				stack_push(registers.pc + 3);
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction("CALL NC,xx", 0, 24);
			} else finish_instruction("CALL NC,xx", 3, 12);
		} break;
		case 0xd5: {
			stack_push(registers.de);
			finish_instruction("PUSH DE", 1, 16);
		} break;
		case 0xd6: execute_instruction_SUB_u8(mem_read(registers.pc+1), "SUB x", 2, 8); break;
		case 0xd8: {
			if (registers.f & FLAG_C) {
				registers.pc = stack_pop();
				finish_instruction("RET C", 0, 20);
			} else finish_instruction("RET C", 1, 8);
		} break;
		case 0xd9: {
			registers.pc = stack_pop();
			registers.ime = true;
			finish_instruction("RETI", 0, 16);
		} break;
		case 0xda: {
			if (registers.f & FLAG_C) {
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction("JP C,xx", 0, 16);
			} else finish_instruction("JP C,xx", 3, 12);
		} break;
		case 0xde: execute_instruction_SBC(mem_read(registers.pc+1), "SBC A,x", 2, 8); break;
		case 0xe0: {
			u8 byte_0 = mem_read(registers.pc+1);
			mem_write(0xff00 + byte_0, registers.a);
			finish_instruction("LDH (ff00+x),A", 2, 12);
		} break;
		case 0xe1: {
			registers.hl = stack_pop();
			finish_instruction("POP HL", 1, 12);
		} break;
		case 0xe2: {
			mem_write(0xff00 + registers.c, registers.a);
			finish_instruction("LD (ff00+C),A", 1, 8);
		} break;
		case 0xe5: {
			stack_push(registers.hl);
			finish_instruction("PUSH HL", 1, 16);
		} break;
		case 0xe6: {
			u8 byte_0 = mem_read(registers.pc+1);
			registers.a &= byte_0;
			
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_N;
			registers.f |= FLAG_H;
			registers.f &= ~FLAG_C;
			finish_instruction("AND x", 2, 8);
		} break;
		case 0xe9: {
			registers.pc = registers.hl;
			finish_instruction("JP (HL)", 0, 4);
		} break;
		case 0xea: {
			mem_write(mem_read_u16(registers.pc+1), registers.a);
			finish_instruction("LD (x),A", 3, 16);
		} break;
		case 0xee: {
			u8 byte_0 = mem_read(registers.pc+1);
			registers.a ^= byte_0;
			
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f &= ~FLAG_C;
			finish_instruction("XOR x", 2, 4);
		} break;
		case 0xef: {
			stack_push(registers.pc + 1);
			registers.pc = 0x0028;
			finish_instruction("RST 28H", 0, 16);
		} break;
		case 0xf0: {
			u8 byte_0 = mem_read(registers.pc+1);
			registers.a = mem_read(0xff00 + byte_0);
			finish_instruction("LDH A,(ff00+x)", 2, 12);
		} break;
		case 0xf1: {
			registers.af = stack_pop();
			finish_instruction("POP AF", 1, 12);
		} break;
		case 0xf2: {
			registers.a = mem_read(0xff00 + registers.c);
			finish_instruction("LD A,(ff00+C)", 1, 8);
		} break;
		case 0xf3: {
			registers.ime = false;
			finish_instruction("DI", 1, 4);
		} break;
		case 0xf5: {
			stack_push(registers.af);
			finish_instruction("PUSH AF", 1, 16);
		} break;
		case 0xf6: execute_instruction_OR(mem_read(registers.pc+1), "OR x", 2, 8); break;
		case 0xf8: {
			s8 signed_byte = mem_read(registers.pc+1);
			
			// TODO: really not sure about the carries for this operation.
			if (signed_byte >= 0) {
				if (addition_produces_u8_half_carry(registers.sp, signed_byte)) registers.f |= FLAG_H;
				else registers.f &= ~FLAG_H;
				if (addition_produces_u8_full_carry(registers.sp, signed_byte)) registers.f |= FLAG_C;
				else registers.f &= ~FLAG_C;
			} else {
				if (negate_produces_u8_half_carry(registers.sp, -signed_byte)) registers.f |= FLAG_H;
				else registers.f &= ~FLAG_H;
				if (negate_produces_u8_full_carry(registers.sp, -signed_byte)) registers.f |= FLAG_C;
				else registers.f &= ~FLAG_C;
			}
			
			registers.hl = registers.sp + signed_byte;
			
			registers.f &= ~FLAG_Z;
			registers.f &= ~FLAG_N;
			
			finish_instruction("LDHL SP,s", 2, 12);
		} break;
		case 0xf9: {
			registers.sp = registers.hl;
			finish_instruction("LD SP,HL", 1, 8);
		} break;
		case 0xfa: {
			u16 address = mem_read_u16(registers.pc+1);
			registers.a = mem_read(address);
			finish_instruction("LD A,(xx)", 3, 16);
		} break;
		case 0xfb: {
			registers.ime = true;
			finish_instruction("IE", 1, 4);
		} break;
		case 0xfe: {
			u8 byte_0 = mem_read(registers.pc+1);
			
			if (negate_produces_u8_half_carry(registers.a, byte_0)) registers.f |= FLAG_H;
			else registers.f &= ~FLAG_H;
			
			if (negate_produces_u8_full_carry(registers.a, byte_0)) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.f |= FLAG_N; // set the add/sub flag high, indicating subtraction
			
			u8 sub_result = registers.a - byte_0;
			if (sub_result == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			finish_instruction("CP x", 2, 8);
		} break;
		case 0xff: {
			stack_push(registers.pc + 1);
			registers.pc = 0x0038;
			finish_instruction("RST 38H", 0, 16);
		} break;
		default: {
			char buf[128] = {0};
			sprintf(buf, "Unknown opcode %x at address %x\n", opcode, registers.pc);
			robingb_log(buf);
			assert(false);
		} break;
	}
}







