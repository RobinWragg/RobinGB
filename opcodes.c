/* TODO: can we speed stuff up if we increment pc gradually instead of in one go as part of finish_instruction()? */

#include "internal.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

#define INSTRUCTION static

u8 *num_cycles_for_finish = 0;

void finish_instruction(s16 pc_increment, u8 num_cycles_param) {
	assert(num_cycles_for_finish);
	registers.pc += pc_increment;
	*num_cycles_for_finish = num_cycles_param;
}

static bool negate_produces_u8_half_carry(s16 a, s16 b, bool include_carry) {
	int optional_carry = (include_carry && registers.f & FLAG_C) ? 1 : 0;
	
	if ((a & 0x0f) - (b & 0x0f) - optional_carry < 0) return true;
	else return false;
}

static bool addition_produces_u8_half_carry(s16 a, s16 b, bool include_carry) {
	int optional_carry = (include_carry && registers.f & FLAG_C) ? 1 : 0;
	
	if ((a & 0x0f) + (b & 0x0f) + optional_carry > 0x0f) return true;
	else return false;
}

static bool negate_produces_u8_full_carry(s16 a, s16 b) {
	if (a - b < 0) return true;
	else return false;
}

static bool addition_produces_u8_full_carry(s16 a, s16 b) {
	if (a + b > 0xff) return true;
	else return false;
}

static bool addition_produces_u16_half_carry(u16 a, u16 b) {
	u16 a_bit_11_and_under = a & 0x0fff;
	u16 b_bit_11_and_under = b & 0x0fff;
	if (a_bit_11_and_under + b_bit_11_and_under > 0x0fff) return true;
	else return false;
}

static bool addition_produces_u16_full_carry(s32 a, s32 b) {
	if (a + b > 0xffff) return true;
	else return false;
}

INSTRUCTION void instruction_XOR(u8 to_xor, u8 num_cycles) {
	registers.a ^= to_xor;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	registers.f &= ~FLAG_C;
	finish_instruction(1, num_cycles);
}

INSTRUCTION void instruction_RST(u8 address_lower_byte) {
	stack_push(registers.pc+1);
	registers.pc = address_lower_byte;
	finish_instruction(0, 16);
}

INSTRUCTION void instruction_CP(u8 comparator, u16 pc_increment, u8 num_cycles) {
	if (negate_produces_u8_half_carry(registers.a, comparator, false)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	if (negate_produces_u8_full_carry(registers.a, comparator)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	registers.f |= FLAG_N; /* set the add/sub flag high, indicating subtraction */
	
	u8 sub_result = registers.a - comparator;
	
	if (sub_result == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	finish_instruction(pc_increment, num_cycles);
}

INSTRUCTION void instruction_DEC_u8(u8 *value_to_decrement, u8 num_cycles) {
	if (negate_produces_u8_half_carry(*value_to_decrement, 1, false)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	(*value_to_decrement)--;
	
	if (*value_to_decrement == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f |= FLAG_N;
	
	finish_instruction(1, num_cycles);
}

INSTRUCTION void instruction_INC_u8(u8 *value_to_increment, u8 num_cycles) {
	if (addition_produces_u8_half_carry(*value_to_increment, 1, false)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;

	(*value_to_increment)++;

	if (*value_to_increment == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;

	registers.f &= ~FLAG_N;

	finish_instruction(1, num_cycles);
}

INSTRUCTION void instruction_ADC(u8 to_add, u16 pc_increment, int num_cycles) {
	int carry = registers.f & FLAG_C ? 1 : 0;
	
	if (addition_produces_u8_half_carry(registers.a, to_add, true)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	/* TODO: this might have an issue with it similar to the trouble I had with adding the carry for the half-carry calculation above. */
	if (addition_produces_u8_full_carry(registers.a, to_add + carry)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	registers.a += to_add + carry;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	
	finish_instruction(pc_increment, num_cycles);
}

INSTRUCTION void instruction_CALL_cond_xx(bool condition) {
	if (condition) {
		stack_push(registers.pc+3);
		registers.pc = mem_read_u16(registers.pc+1);
		finish_instruction(0, 24);
	} else finish_instruction(3, 12);
}

INSTRUCTION void instruction_SBC(u8 to_subtract, u16 pc_increment, int num_cycles) {
	int carry = registers.f & FLAG_C ? 1 : 0;
	
	if (negate_produces_u8_half_carry(registers.a, to_subtract, true)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	if (negate_produces_u8_full_carry(registers.a, to_subtract + carry)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	registers.a -= to_subtract + carry;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f |= FLAG_N;
	
	finish_instruction(pc_increment, num_cycles);
}

INSTRUCTION void instruction_AND(u8 right_hand_value, u16 pc_increment, int num_cycles) {
	registers.a &= right_hand_value;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f |= FLAG_H;
	registers.f &= ~FLAG_C;
	
	finish_instruction(pc_increment, num_cycles);
}

INSTRUCTION void instruction_OR(u8 right_hand_value, u16 pc_increment, int num_cycles) {
	registers.a |= right_hand_value;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	registers.f &= ~FLAG_C;
	finish_instruction(pc_increment, num_cycles);
}

INSTRUCTION void instruction_ADD_A_u8(u8 to_add, u16 pc_increment, u8 num_cycles) {
	if (addition_produces_u8_half_carry(registers.a, to_add, false)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	if (addition_produces_u8_full_carry(registers.a, to_add)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	registers.a += to_add;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	
	finish_instruction(pc_increment, num_cycles);
}

INSTRUCTION void instruction_ADD_HL_u16(u16 to_add, u16 pc_increment, u8 num_cycles) {
	if (addition_produces_u16_half_carry(registers.hl, to_add)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	if (addition_produces_u16_full_carry(registers.hl, to_add)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	registers.hl += to_add;
	
	registers.f &= ~FLAG_N;
	
	finish_instruction(pc_increment, num_cycles);
}

INSTRUCTION void instruction_SUB_u8(u8 subber, u16 pc_increment, int num_cycles) {
	if (negate_produces_u8_half_carry(registers.a, subber, false)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	if (negate_produces_u8_full_carry(registers.a, subber)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	registers.a -= subber;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f |= FLAG_N;
	
	finish_instruction(pc_increment, num_cycles);
}

#if OPCODE_PROFILING
int64_t opcode_counts[256] = {0};
#endif

void execute_next_opcode(u8 *num_cycles_out) {
	num_cycles_for_finish = num_cycles_out;
	u8 opcode = mem_read(registers.pc);
	
	#if OPCODE_PROFILING
	opcode_counts[opcode]++;
	#endif
	
	/* printf("a:%x op:%x(%x) ", registers.pc, opcode, mem_read(registers.pc+1)); */
	/* printf("%s\n", get_opcode_name(registers.pc)); */
	
	switch (opcode) {
		case 0x00: finish_instruction(1, 4); break;
		case 0x01: registers.bc = mem_read_u16(registers.pc+1); finish_instruction(3, 12); break;
		case 0x02: mem_write(registers.bc, registers.a); finish_instruction(1, 8); break;
		case 0x03: registers.bc++; finish_instruction(1, 8); break;
		case 0x04: instruction_INC_u8(&registers.b, 4); break;
		case 0x05: instruction_DEC_u8(&registers.b, 4); break;
		case 0x06: registers.b = mem_read(registers.pc+1); finish_instruction(2, 8); break;
		case 0x07: { /* RLCA (different flag manipulation to RLC) */
			if (registers.a & bit(7)) {
				registers.f |= FLAG_C;
				registers.a <<= 1;
				registers.a |= bit(0);
			} else {
				registers.f &= ~FLAG_C;
				registers.a <<= 1;
				registers.a &= ~bit(0);
			}
			
			registers.f &= ~FLAG_Z;
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			
			finish_instruction(1, 4);
		} break;
		case 0x08: mem_write_u16(mem_read_u16(registers.pc+1), registers.sp); finish_instruction(3, 20); break;
		case 0x09: instruction_ADD_HL_u16(registers.bc, 1, 8); break;
		case 0x0a: registers.a = mem_read(registers.bc); finish_instruction(1, 8); break;
		case 0x0b: registers.bc--; finish_instruction(1, 8); break;
		case 0x0c: instruction_INC_u8(&registers.c, 4); break;
		case 0x0d: instruction_DEC_u8(&registers.c, 4); break;
		case 0x0e: registers.c = mem_read(registers.pc+1); finish_instruction(2, 8); break;
		case 0x0f: { /* RRCA (different flag manipulation to RRC) */
			if (registers.a & bit(0)) {
				registers.f |= FLAG_C;
				registers.a >>= 1;
				registers.a |= bit(7);
			} else {
				registers.f &= ~FLAG_C;
				registers.a >>= 1;
				registers.a &= ~bit(7);
			}
			
			registers.f &= ~FLAG_Z;
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			
			finish_instruction(1, 4);
		} break;
		/* TODO: Apparently STOP is like HALT except the LCD is inoperational as well, and
		the "stopped" state is only exited when a button is pressed. Look for better documentation
		on it. */
		case 0x11: registers.de = mem_read_u16(registers.pc+1); finish_instruction(3, 12); break;
		case 0x12: mem_write(registers.de, registers.a); finish_instruction(1, 8); break;
		case 0x13: registers.de++; finish_instruction(1, 8); break;
		case 0x14: instruction_INC_u8(&registers.d, 4); break;
		case 0x15: instruction_DEC_u8(&registers.d, 4); break;
		case 0x16: registers.d = mem_read(registers.pc+1); finish_instruction(2, 8); break;
		case 0x17: {
			bool prev_carry = registers.f & FLAG_C;
			
			if (registers.a & bit(7)) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.a = registers.a << 1;
			
			if (prev_carry) registers.a |= bit(0);
			
			registers.f &= ~FLAG_Z;
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			
			finish_instruction(1, 4);
		} break;
		case 0x18: finish_instruction(2 + (s8)mem_read(registers.pc+1), 12); break;
		case 0x19: instruction_ADD_HL_u16(registers.de, 1, 8); break;
		case 0x1a: registers.a = mem_read(registers.de); finish_instruction(1, 8); break;
		case 0x1b: registers.de--; finish_instruction(1, 8); break;
		case 0x1c: instruction_INC_u8(&registers.e, 4); break;
		case 0x1d: instruction_DEC_u8(&registers.e, 4); break;
		case 0x1e: registers.e = mem_read(registers.pc+1); finish_instruction(2, 8); break;
		case 0x1f: {
			bool prev_carry = registers.f & FLAG_C;
			
			if (registers.a & bit(0)) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.a = registers.a >> 1;
			
			if (prev_carry) registers.a |= bit(7);
			
			registers.f &= ~FLAG_Z;
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			
			finish_instruction(1, 4);
		} break;
		case 0x20: { /* JR NZ,r8 */
			if (registers.f & FLAG_Z) finish_instruction(2, 8);
			else finish_instruction(2 + (s8)mem_read(registers.pc+1), 12);
		} break;
		case 0x21: registers.hl = mem_read_u16(registers.pc+1); finish_instruction(3, 12); break;
		case 0x22: mem_write(registers.hl++, registers.a); finish_instruction(1, 8); break;
		case 0x23:  registers.hl++; finish_instruction(1, 8); break;
		case 0x24: instruction_INC_u8(&registers.h, 4); break;
		case 0x25: instruction_DEC_u8(&registers.h, 4); break;
		case 0x26: {
			registers.h = mem_read(registers.pc+1);
			finish_instruction(2, 8);
		} break;
		case 0x27: { /* DAA */
			if ((registers.f & FLAG_N) == 0) {
				u8 a_lower_nybble = registers.a & 0x0f;
				
				if (a_lower_nybble > 0x09) {
					registers.a += 0x06;
					registers.f |= FLAG_H;
				}
				
				u8 a_upper_nybble = (registers.a & 0xf0) >> 4;
				
				if (a_upper_nybble > 0x09 || registers.f & FLAG_C) {
					registers.a += 0x60;
					registers.f |= FLAG_C;
				}
			} else {
				if (registers.f & FLAG_C) registers.a -= 0x60;
				if (registers.f & FLAG_H) registers.a -= 0x6;
			}
			
			/* these flags are always updated */
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_H;
			
			finish_instruction(1, 4);
		} break;
		case 0x28: {
			if (registers.f & FLAG_Z) finish_instruction(2 + (s8)mem_read(registers.pc+1), 12);
			else finish_instruction(2, 8);
		} break;
		case 0x29: instruction_ADD_HL_u16(registers.hl, 1, 8); break;
		case 0x2a: registers.a = mem_read(registers.hl++); finish_instruction(1, 8); break;
		case 0x2b: registers.hl--; finish_instruction(1, 8); break;
		case 0x2c: instruction_INC_u8(&registers.l, 4); break;
		case 0x2d: instruction_DEC_u8(&registers.l, 4); break;
		case 0x2e: registers.l = mem_read(registers.pc+1); finish_instruction(2, 8); break;
		case 0x2f: { /* CPL */
			registers.a ^= 0xff;
			registers.f |= FLAG_N;
			registers.f |= FLAG_H;
			finish_instruction(1, 4);
		} break;
		case 0x30: {
			if ((registers.f & FLAG_C) == 0) finish_instruction(2 + (s8)mem_read(registers.pc+1), 12);
			else finish_instruction(2, 8);
		} break;
		case 0x31: registers.sp = mem_read_u16(registers.pc+1); finish_instruction(3, 12); break;
		case 0x32: mem_write(registers.hl--, registers.a); finish_instruction(1, 8); break;
		case 0x33:  registers.sp++; finish_instruction(1, 8); break;
		case 0x34: {
			u8 hl_value = mem_read(registers.hl);
			instruction_INC_u8(&hl_value, 12);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x35: {
			u8 hl_value = mem_read(registers.hl);
			instruction_DEC_u8(&hl_value, 12);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x36: mem_write(registers.hl, mem_read(registers.pc+1)); finish_instruction(2, 12); break;
		case 0x37: { /* SCF */
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f |= FLAG_C;
			finish_instruction(1, 4);
		} break;
		case 0x38: { /* JR C,x */
			if (registers.f & FLAG_C) finish_instruction(2 + (s8)mem_read(registers.pc+1), 12);
			else finish_instruction(2, 8);
		} break;
		case 0x39: instruction_ADD_HL_u16(registers.sp, 1, 8); break;
		case 0x3a: registers.a = mem_read(registers.hl--); finish_instruction(1, 8); break;
		case 0x3b: registers.sp--; finish_instruction(1, 8); break;
		case 0x3c: instruction_INC_u8(&registers.a, 4); break;
		case 0x3d: instruction_DEC_u8(&registers.a, 4); break;
		case 0x3e: registers.a = mem_read(registers.pc+1); finish_instruction(2, 8); break;
		case 0x3f: { /* CCF */
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f ^= FLAG_C;
			finish_instruction(1, 4);
		} break;
		case 0x40: registers.b = registers.b; finish_instruction(1, 4); break;
		case 0x41: registers.b = registers.c; finish_instruction(1, 4); break;
		case 0x42: registers.b = registers.d; finish_instruction(1, 4); break;
		case 0x43: registers.b = registers.e; finish_instruction(1, 4); break;
		case 0x44: registers.b = registers.h; finish_instruction(1, 4); break;
		case 0x45: registers.b = registers.l; finish_instruction(1, 4); break;
		case 0x46: registers.b = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x47: registers.b = registers.a; finish_instruction(1, 4); break;
		case 0x48: registers.c = registers.b; finish_instruction(1, 4); break;
		case 0x49: registers.c = registers.c; finish_instruction(1, 4); break;
		case 0x4a: registers.c = registers.d; finish_instruction(1, 4); break;
		case 0x4b: registers.c = registers.e; finish_instruction(1, 4); break;
		case 0x4c: registers.c = registers.h; finish_instruction(1, 4); break;
		case 0x4d: registers.c = registers.l; finish_instruction(1, 4); break;
		case 0x4e: registers.c = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x4f: registers.c = registers.a; finish_instruction(1, 4); break;
		case 0x50: registers.d = registers.b; finish_instruction(1, 4); break;
		case 0x51: registers.d = registers.c; finish_instruction(1, 4); break;
		case 0x52: registers.d = registers.d; finish_instruction(1, 4); break;
		case 0x53: registers.d = registers.e; finish_instruction(1, 4); break;
		case 0x54: registers.d = registers.h; finish_instruction(1, 4); break;
		case 0x55: registers.d = registers.l; finish_instruction(1, 4); break;
		case 0x56: registers.d = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x57: registers.d = registers.a; finish_instruction(1, 4); break;
		case 0x58: registers.e = registers.b; finish_instruction(1, 4); break;
		case 0x59: registers.e = registers.c; finish_instruction(1, 4); break;
		case 0x5a: registers.e = registers.d; finish_instruction(1, 4); break;
		case 0x5b: registers.e = registers.e; finish_instruction(1, 4); break;
		case 0x5c: registers.e = registers.h; finish_instruction(1, 4); break;
		case 0x5d: registers.e = registers.l; finish_instruction(1, 4); break;
		case 0x5e: registers.e = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x5f: registers.e = registers.a; finish_instruction(1, 4); break;
		case 0x60: registers.h = registers.b; finish_instruction(1, 4); break;
		case 0x61: registers.h = registers.c; finish_instruction(1, 4); break;
		case 0x62: registers.h = registers.d; finish_instruction(1, 4); break;
		case 0x63: registers.h = registers.e; finish_instruction(1, 4); break;
		case 0x64: registers.h = registers.h; finish_instruction(1, 4); break;
		case 0x65: registers.h = registers.l; finish_instruction(1, 4); break;
		case 0x66: registers.h = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x67: registers.h = registers.a; finish_instruction(1, 4); break;
		case 0x68: registers.l = registers.b; finish_instruction(1, 4); break;
		case 0x69: registers.l = registers.c; finish_instruction(1, 4); break;
		case 0x6a: registers.l = registers.d; finish_instruction(1, 4); break;
		case 0x6b: registers.l = registers.e; finish_instruction(1, 4); break;
		case 0x6c: registers.l = registers.h; finish_instruction(1, 4); break;
		case 0x6d: registers.l = registers.l; finish_instruction(1, 4); break;
		case 0x6e: registers.l = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x6f: registers.l = registers.a; finish_instruction(1, 4); break;
		case 0x70: mem_write(registers.hl, registers.b); finish_instruction(1, 8); break;
		case 0x71: mem_write(registers.hl, registers.c); finish_instruction(1, 8); break;
		case 0x72: mem_write(registers.hl, registers.d); finish_instruction(1, 8); break;
		case 0x73: mem_write(registers.hl, registers.e); finish_instruction(1, 8); break;
		case 0x74: mem_write(registers.hl, registers.h); finish_instruction(1, 8); break;
		case 0x75: mem_write(registers.hl, registers.l); finish_instruction(1, 8); break;
		case 0x76: { /* HALT */
			assert(!halted); /* Instructions shouldn't be getting executed while halted. */
			
			if (registers.ime) {
				halted = true;
				finish_instruction(1, 4);
			} else finish_instruction(1, 4);
		} break;
		case 0x77: mem_write(registers.hl, registers.a); finish_instruction(1, 8); break;
		case 0x78: registers.a = registers.b; finish_instruction(1, 4); break;
		case 0x79: registers.a = registers.c; finish_instruction(1, 4); break;
		case 0x7a: registers.a = registers.d; finish_instruction(1, 4); break;
		case 0x7b: registers.a = registers.e; finish_instruction(1, 4); break;
		case 0x7c: registers.a = registers.h; finish_instruction(1, 4); break;
		case 0x7d: registers.a = registers.l; finish_instruction(1, 4); break;
		case 0x7e: registers.a = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x7f: registers.a = registers.a; finish_instruction(1, 4); break;
		case 0x80: instruction_ADD_A_u8(registers.b, 1, 4); break;
		case 0x81: instruction_ADD_A_u8(registers.c, 1, 4); break;
		case 0x82: instruction_ADD_A_u8(registers.d, 1, 4); break;
		case 0x83: instruction_ADD_A_u8(registers.e, 1, 4); break;
		case 0x84: instruction_ADD_A_u8(registers.h, 1, 4); break;
		case 0x85: instruction_ADD_A_u8(registers.l, 1, 4); break;
		case 0x86: instruction_ADD_A_u8(mem_read(registers.hl), 1, 8); break;
		case 0x87: instruction_ADD_A_u8(registers.a, 1, 4); break;
		case 0x88: instruction_ADC(registers.b, 1, 4); break;
		case 0x89: instruction_ADC(registers.c, 1, 4); break;
		case 0x8a: instruction_ADC(registers.d, 1, 4); break;
		case 0x8b: instruction_ADC(registers.e, 1, 4); break;
		case 0x8c: instruction_ADC(registers.h, 1, 4); break;
		case 0x8d: instruction_ADC(registers.l, 1, 4); break;
		case 0x8e: instruction_ADC(mem_read(registers.hl), 1, 8); break;
		case 0x8f: instruction_ADC(registers.a, 1, 4); break;
		case 0x90: instruction_SUB_u8(registers.b, 1, 4); break;
		case 0x91: instruction_SUB_u8(registers.c, 1, 4); break;
		case 0x92: instruction_SUB_u8(registers.d, 1, 4); break;
		case 0x93: instruction_SUB_u8(registers.e, 1, 4); break;
		case 0x94: instruction_SUB_u8(registers.h, 1, 4); break;
		case 0x95: instruction_SUB_u8(registers.l, 1, 4); break;
		case 0x96: instruction_SUB_u8(mem_read(registers.hl), 1, 8); break;
		case 0x97: instruction_SUB_u8(registers.a, 1, 4); break;
		case 0x98: instruction_SBC(registers.b, 1, 4); break;	
		case 0x99: instruction_SBC(registers.c, 1, 4); break;
		case 0x9a: instruction_SBC(registers.d, 1, 4); break;
		case 0x9b: instruction_SBC(registers.e, 1, 4); break;
		case 0x9c: instruction_SBC(registers.h, 1, 4); break;
		case 0x9d: instruction_SBC(registers.l, 1, 4); break;
		case 0x9e: instruction_SBC(mem_read(registers.hl), 1, 8); break;
		case 0x9f: instruction_SBC(registers.a, 1, 4); break;
		case 0xa0: instruction_AND(registers.b, 1, 4); break;
		case 0xa1: instruction_AND(registers.c, 1, 4); break;
		case 0xa2: instruction_AND(registers.d, 1, 4); break;
		case 0xa3: instruction_AND(registers.e, 1, 4); break;
		case 0xa4: instruction_AND(registers.h, 1, 4); break;
		case 0xa5: instruction_AND(registers.l, 1, 4); break;
		case 0xa6: instruction_AND(mem_read(registers.hl), 1, 8); break;
		case 0xa7: instruction_AND(registers.a, 1, 4); break;
		case 0xa8: instruction_XOR(registers.b, 4); break;
		case 0xa9: instruction_XOR(registers.c, 4); break;
		case 0xaa: instruction_XOR(registers.d, 4); break;
		case 0xab: instruction_XOR(registers.e, 4); break;
		case 0xac: instruction_XOR(registers.h, 4); break;
		case 0xad: instruction_XOR(registers.l, 4); break;
		case 0xae: instruction_XOR(mem_read(registers.hl), 8); break;
		case 0xaf: instruction_XOR(registers.a, 4); break;
		case 0xb0: instruction_OR(registers.b, 1, 4); break;
		case 0xb1: instruction_OR(registers.c, 1, 4); break;
		case 0xb2: instruction_OR(registers.d, 1, 4); break;
		case 0xb3: instruction_OR(registers.e, 1, 4); break;
		case 0xb4: instruction_OR(registers.h, 1, 4); break;
		case 0xb5: instruction_OR(registers.l, 1, 4); break;
		case 0xb6: instruction_OR(mem_read(registers.hl), 1, 8); break;
		case 0xb7: instruction_OR(registers.a, 1, 4); break;
		case 0xb8: instruction_CP(registers.b, 1, 4); break;
		case 0xb9: instruction_CP(registers.c, 1, 4); break;
		case 0xba: instruction_CP(registers.d, 1, 4); break;
		case 0xbb: instruction_CP(registers.e, 1, 4); break;
		case 0xbc: instruction_CP(registers.h, 1, 4); break;
		case 0xbd: instruction_CP(registers.l, 1, 4); break;
		case 0xbe: instruction_CP(mem_read(registers.hl), 1, 8); break;
		case 0xbf: instruction_CP(registers.a, 1, 4); break;
		case 0xc0: {
			if (registers.f & FLAG_Z) finish_instruction(1, 8);
			else {
				registers.pc = stack_pop();
				finish_instruction(0, 20);
			}
		} break;
		case 0xc1: {
			registers.bc = stack_pop();
			finish_instruction(1, 12);
		} break;
		case 0xc2: {
			if ((registers.f & FLAG_Z) == 0) {
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction(0, 16);
			} else {
				finish_instruction(3, 12);
			}
		} break;
		case 0xc3: {
			registers.pc = mem_read_u16(registers.pc+1);
			finish_instruction(0, 16);
		} break;
		case 0xc4: instruction_CALL_cond_xx((registers.f & FLAG_Z) == 0); break;
		case 0xc5: {
			stack_push(registers.bc);
			finish_instruction(1, 16);
		} break;
		case 0xc6: instruction_ADD_A_u8(mem_read(registers.pc+1), 2, 8); break;
		case 0xc7: instruction_RST(0x00); break;
		case 0xc8: {
			if (registers.f & FLAG_Z) {
				registers.pc = stack_pop();
				finish_instruction(0, 20);
			} else finish_instruction(1, 8);
		} break;
		case 0xc9: {
			registers.pc = stack_pop();
			finish_instruction(0, 16);
		} break;
		case 0xca: {
			if (registers.f & FLAG_Z) {
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction(0, 16);
			} else finish_instruction(3, 12);
		} break;
		case 0xcb: {
			execute_cb_opcode();
		} break;
		case 0xcc: instruction_CALL_cond_xx(registers.f & FLAG_Z); break;
		case 0xcd: {
			instruction_CALL_cond_xx(true); break;
		} break;
		case 0xce: instruction_ADC(mem_read(registers.pc+1), 2, 8); break;
		case 0xcf: instruction_RST(0x08); break;
		case 0xd0: {
			if ((registers.f & FLAG_C) == false) {
				registers.pc = stack_pop();
				finish_instruction(0, 20);
			} else finish_instruction(1, 8);
		} break;
		case 0xd1: {
			registers.de = stack_pop();
			finish_instruction(1, 12);
		} break;
		case 0xd2: {
			if ((registers.f & FLAG_C) == 0) {
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction(0, 16);
			} else {
				finish_instruction(3, 12);
			}
		} break;
		/* case 0xd3: Nothing at 0xd3 */
		case 0xd4: instruction_CALL_cond_xx((registers.f & FLAG_C) == 0); break;
		case 0xd5: {
			stack_push(registers.de);
			finish_instruction(1, 16);
		} break;
		case 0xd6: instruction_SUB_u8(mem_read(registers.pc+1), 2, 8); break;
		case 0xd7: instruction_RST(0x10); break;
		case 0xd8: {
			if (registers.f & FLAG_C) {
				registers.pc = stack_pop();
				finish_instruction(0, 20);
			} else finish_instruction(1, 8);
		} break;
		case 0xd9: {
			registers.pc = stack_pop();
			registers.ime = true;
			finish_instruction(0, 16);
		} break;
		case 0xda: {
			if (registers.f & FLAG_C) {
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction(0, 16);
			} else finish_instruction(3, 12);
		} break;
		case 0xdc: instruction_CALL_cond_xx(registers.f & FLAG_C); break;
		/* case 0xdb: Nothing at 0xdb */
		case 0xdf: instruction_RST(0x18); break;
		/* case 0xdd: Nothing at 0xdd */
		case 0xde: instruction_SBC(mem_read(registers.pc+1), 2, 8); break;
		case 0xe0: {
			u8 byte_0 = mem_read(registers.pc+1);
			mem_write(0xff00 + byte_0, registers.a);
			finish_instruction(2, 12);
		} break;
		case 0xe1: {
			registers.hl = stack_pop();
			finish_instruction(1, 12);
		} break;
		case 0xe2: {
			mem_write(0xff00 + registers.c, registers.a);
			finish_instruction(1, 8);
		} break;
		/* case 0xe3: Nothing at 0xe3 */
		/* case 0xe4: Nothing at 0xe4 */
		case 0xe5: {
			stack_push(registers.hl);
			finish_instruction(1, 16);
		} break;
		case 0xe6: instruction_AND(mem_read(registers.pc+1), 2, 8); break;
		case 0xe7: instruction_RST(0x20); break;
		case 0xe8: { /* ADD SP,x */
			s8 signed_byte_to_add = mem_read(registers.pc+1);
			
			if (addition_produces_u16_half_carry(registers.sp, signed_byte_to_add)) registers.f |= FLAG_H;
			else registers.f &= ~FLAG_H;
			
			if (addition_produces_u16_full_carry(registers.sp, signed_byte_to_add)) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.sp += signed_byte_to_add;
			
			registers.f &= ~FLAG_Z;
			registers.f &= ~FLAG_N;
			
			finish_instruction(2, 16);
		} break;
		case 0xe9: {
			registers.pc = registers.hl;
			finish_instruction(0, 4);
		} break;
		case 0xea: {
			mem_write(mem_read_u16(registers.pc+1), registers.a);
			finish_instruction(3, 16);
		} break;
		/* case 0xeb: Nothing at 0xeb */
		/* case 0xec: Nothing at 0xec */
		/* case 0xed: Nothing at 0xed */
		case 0xee: {
			u8 byte_0 = mem_read(registers.pc+1);
			registers.a ^= byte_0;
			
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f &= ~FLAG_C;
			finish_instruction(2, 4);
		} break;
		case 0xef: instruction_RST(0x28); break;
		case 0xf0: { /* LDH A,(0xff00+x) */
			registers.a = mem_read(0xff00 + mem_read(registers.pc+1));
			finish_instruction(2, 12);
		} break;
		case 0xf1: {
			registers.af = stack_pop();
			finish_instruction(1, 12);
		} break;
		case 0xf2: {
			registers.a = mem_read(0xff00 + registers.c);
			finish_instruction(1, 8);
		} break;
		case 0xf3: {
			registers.ime = false;
			finish_instruction(1, 4);
		} break;
		/* case 0xf4: Nothing at 0xf4 */
		case 0xf5: {
			stack_push(registers.af);
			finish_instruction(1, 16);
		} break;
		case 0xf6: instruction_OR(mem_read(registers.pc+1), 2, 8); break;
		case 0xf7: instruction_RST(0x30); break;
		case 0xf8: {
			s8 signed_byte = mem_read(registers.pc+1);
			
			/* TODO: really not sure about the carries for this operation. */
			if (signed_byte >= 0) {
				if (addition_produces_u16_half_carry(registers.sp, signed_byte)) registers.f |= FLAG_H;
				else registers.f &= ~FLAG_H;
				if (addition_produces_u16_full_carry(registers.sp, signed_byte)) registers.f |= FLAG_C;
				else registers.f &= ~FLAG_C;
			} else {
				if (negate_produces_u8_half_carry(registers.sp, -signed_byte, false)) registers.f |= FLAG_H;
				else registers.f &= ~FLAG_H;
				if (negate_produces_u8_full_carry(registers.sp, -signed_byte)) registers.f |= FLAG_C;
				else registers.f &= ~FLAG_C;
			}
			
			registers.hl = registers.sp + signed_byte;
			
			registers.f &= ~FLAG_Z;
			registers.f &= ~FLAG_N;
			
			finish_instruction(2, 12);
		} break;
		case 0xf9: {
			registers.sp = registers.hl;
			finish_instruction(1, 8);
		} break;
		case 0xfa: {
			u16 address = mem_read_u16(registers.pc+1);
			registers.a = mem_read(address);
			finish_instruction(3, 16);
		} break;
		case 0xfb: {
			registers.ime = true;
			finish_instruction(1, 4);
		} break;
		/* case 0xfc: Nothing at 0xfc */
		/* case 0xfd: Nothing at 0xfd */
		case 0xfe: {
			u8 byte_0 = mem_read(registers.pc+1);
			
			if (negate_produces_u8_half_carry(registers.a, byte_0, false)) registers.f |= FLAG_H;
			else registers.f &= ~FLAG_H;
			
			if (negate_produces_u8_full_carry(registers.a, byte_0)) registers.f |= FLAG_C;
			else registers.f &= ~FLAG_C;
			
			registers.f |= FLAG_N; /* set the add/sub flag high, indicating subtraction */
			
			u8 sub_result = registers.a - byte_0;
			if (sub_result == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			finish_instruction(2, 8);
		} break;
		case 0xff: instruction_RST(0x38); break;
		default: {
			char buf[128] = {0};
			sprintf(buf, "Unknown opcode %x at address %x\n", opcode, registers.pc);
			robingb_log(buf);
			assert(false);
		} break;
	}
}







