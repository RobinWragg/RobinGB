/* TODO: can we speed stuff up if we increment pc gradually instead of in one go as part of finish_instruction()? */

#include "internal.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

#define INSTRUCTION

u8 *num_cycles_for_finish = 0;

void finish_instruction(s16 pc_increment, u8 num_cycles_param) {
	assert(num_cycles_for_finish);
	registers.pc += pc_increment;
	*num_cycles_for_finish = num_cycles_param;
}

static bool negate_produces_u8_half_carry(s16 a, s16 b, bool include_carry) {
	u8 optional_carry = (include_carry && registers.f & FLAG_C) ? 1 : 0;
	
	if ((a & 0x0f) - (b & 0x0f) - optional_carry < 0) return true;
	else return false;
}

static bool addition_produces_u8_half_carry(s16 a, s16 b, bool include_carry) {
	u8 optional_carry = (include_carry && registers.f & FLAG_C) ? 1 : 0;
	
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

INSTRUCTION static void instruction_XOR(u8 to_xor, u8 num_cycles) {
	registers.a ^= to_xor;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	registers.f &= ~FLAG_C;
	finish_instruction(1, num_cycles);
}

INSTRUCTION static void instruction_RST(u8 address_lower_byte) {
	stack_push(registers.pc+1);
	registers.pc = address_lower_byte;
	finish_instruction(0, 16);
}

INSTRUCTION static void instruction_CP(u8 comparator, u16 pc_increment, u8 num_cycles) {
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

INSTRUCTION static void instruction_DEC_u8(u8 *value_to_decrement, u8 num_cycles) {
	if (negate_produces_u8_half_carry(*value_to_decrement, 1, false)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	if (--(*value_to_decrement)) registers.f &= ~FLAG_Z;
	else registers.f |= FLAG_Z;
	
	registers.f |= FLAG_N;
	
	finish_instruction(1, num_cycles);
}

INSTRUCTION static void instruction_INC_u8(u8 *value_to_increment, u8 num_cycles) {
	if (addition_produces_u8_half_carry(*value_to_increment, 1, false)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	if (++(*value_to_increment)) registers.f &= ~FLAG_Z;
	else registers.f |= FLAG_Z;

	registers.f &= ~FLAG_N;

	finish_instruction(1, num_cycles);
}

INSTRUCTION static void instruction_ADC(u8 to_add, u16 pc_increment, int num_cycles) {
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

INSTRUCTION static void instruction_CALL_cond_xx(bool condition) {
	if (condition) {
		stack_push(registers.pc+3);
		registers.pc = mem_read_u16(registers.pc+1);
		finish_instruction(0, 24);
	} else finish_instruction(3, 12);
}

INSTRUCTION static void instruction_SBC(u8 to_subtract, u16 pc_increment, int num_cycles) {
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

INSTRUCTION static void instruction_AND(u8 right_hand_value, u16 pc_increment, int num_cycles) {
	registers.a &= right_hand_value;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f |= FLAG_H;
	registers.f &= ~FLAG_C;
	
	finish_instruction(pc_increment, num_cycles);
}

INSTRUCTION static void instruction_OR(u8 right_hand_value, u16 pc_increment, int num_cycles) {
	registers.a |= right_hand_value;
	
	if (registers.a == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	registers.f &= ~FLAG_C;
	finish_instruction(pc_increment, num_cycles);
}

INSTRUCTION static void instruction_ADD_A_u8(u8 to_add, u16 pc_increment, u8 num_cycles) {
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

INSTRUCTION static void instruction_ADD_HL_u16(u16 to_add, u16 pc_increment, u8 num_cycles) {
	if (addition_produces_u16_half_carry(registers.hl, to_add)) registers.f |= FLAG_H;
	else registers.f &= ~FLAG_H;
	
	if (addition_produces_u16_full_carry(registers.hl, to_add)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	registers.hl += to_add;
	
	registers.f &= ~FLAG_N;
	
	finish_instruction(pc_increment, num_cycles);
}

INSTRUCTION static void instruction_SUB_u8(u8 subber, u16 pc_increment, int num_cycles) {
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

void execute_next_opcode(u8 *num_cycles_out) {
	num_cycles_for_finish = num_cycles_out;
	u8 opcode = mem_read(registers.pc);
	
	switch (opcode) {
		case 0x00: DEBUG_set_opcode_name("NOP"); finish_instruction(1, 4); break;
		case 0x01: DEBUG_set_opcode_name("LD BC,xx"); registers.bc = mem_read_u16(registers.pc+1); finish_instruction(3, 12); break;
		case 0x02: DEBUG_set_opcode_name("LD (BC),A"); mem_write(registers.bc, registers.a); finish_instruction(1, 8); break;
		case 0x03: DEBUG_set_opcode_name("INC BC"); registers.bc++; finish_instruction(1, 8); break;
		case 0x04: DEBUG_set_opcode_name("INC b"); instruction_INC_u8(&registers.b, 4); break;
		case 0x05: DEBUG_set_opcode_name("DEC B"); instruction_DEC_u8(&registers.b, 4); break;
		case 0x06: DEBUG_set_opcode_name("LD B,x"); registers.b = mem_read(registers.pc+1); finish_instruction(2, 8); break;
		case 0x07: { DEBUG_set_opcode_name("RLCA");
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
		case 0x08: DEBUG_set_opcode_name("LD (xx),SP"); mem_write_u16(mem_read_u16(registers.pc+1), registers.sp); finish_instruction(3, 20); break;
		case 0x09: DEBUG_set_opcode_name("ADD HL,BC"); instruction_ADD_HL_u16(registers.bc, 1, 8); break;
		case 0x0a: DEBUG_set_opcode_name("LD A,(BC)"); registers.a = mem_read(registers.bc); finish_instruction(1, 8); break;
		case 0x0b: DEBUG_set_opcode_name("DEC BC"); registers.bc--; finish_instruction(1, 8); break;
		case 0x0c: DEBUG_set_opcode_name("INC C"); instruction_INC_u8(&registers.c, 4); break;
		case 0x0d: DEBUG_set_opcode_name("DEC C"); instruction_DEC_u8(&registers.c, 4); break;
		case 0x0e: DEBUG_set_opcode_name("LD C,x"); registers.c = mem_read(registers.pc+1); finish_instruction(2, 8); break;
		case 0x0f: { /* RRCA (different flag manipulation to RRC) */
			DEBUG_set_opcode_name("RRCA");
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
		case 0x11: DEBUG_set_opcode_name("LD DE,xx"); registers.de = mem_read_u16(registers.pc+1); finish_instruction(3, 12); break;
		case 0x12: DEBUG_set_opcode_name("LD (DE),A"); mem_write(registers.de, registers.a); finish_instruction(1, 8); break;
		case 0x13: DEBUG_set_opcode_name("INC DE"); registers.de++; finish_instruction(1, 8); break;
		case 0x14: DEBUG_set_opcode_name("INC D"); instruction_INC_u8(&registers.d, 4); break;
		case 0x15: DEBUG_set_opcode_name("DEC D"); instruction_DEC_u8(&registers.d, 4); break;
		case 0x16: DEBUG_set_opcode_name("LD D,x"); registers.d = mem_read(registers.pc+1); finish_instruction(2, 8); break;
		case 0x17: { DEBUG_set_opcode_name("RLA");
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
		case 0x18: DEBUG_set_opcode_name("JR %i(d)"); finish_instruction(2 + (s8)mem_read(registers.pc+1), 12); break;
		case 0x19: DEBUG_set_opcode_name("ADD HL,DE"); instruction_ADD_HL_u16(registers.de, 1, 8); break;
		case 0x1a: DEBUG_set_opcode_name("LD A,(DE)"); registers.a = mem_read(registers.de); finish_instruction(1, 8); break;
		case 0x1b: DEBUG_set_opcode_name("DEC DE"); registers.de--; finish_instruction(1, 8); break;
		case 0x1c: DEBUG_set_opcode_name("INC E"); instruction_INC_u8(&registers.e, 4); break;
		case 0x1d: DEBUG_set_opcode_name("DEC E"); instruction_DEC_u8(&registers.e, 4); break;
		case 0x1e: DEBUG_set_opcode_name("LD E,x"); registers.e = mem_read(registers.pc+1); finish_instruction(2, 8); break;
		case 0x1f: { DEBUG_set_opcode_name("RRA");
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
			DEBUG_set_opcode_name("JR NZ,s");
			if (registers.f & FLAG_Z) finish_instruction(2, 8);
			else finish_instruction(2 + (s8)mem_read(registers.pc+1), 12);
		} break;
		case 0x21: DEBUG_set_opcode_name("LD HL,xx"); registers.hl = mem_read_u16(registers.pc+1); finish_instruction(3, 12); break;
		case 0x22: DEBUG_set_opcode_name("LD (HL+),A"); mem_write(registers.hl++, registers.a); finish_instruction(1, 8); break;
		case 0x23: DEBUG_set_opcode_name("INC HL");  registers.hl++; finish_instruction(1, 8); break;
		case 0x24: DEBUG_set_opcode_name("INC H"); instruction_INC_u8(&registers.h, 4); break;
		case 0x25: DEBUG_set_opcode_name("DEC H"); instruction_DEC_u8(&registers.h, 4); break;
		case 0x26: { DEBUG_set_opcode_name("LD H,x");
			registers.h = mem_read(registers.pc+1);
			finish_instruction(2, 8);
		} break;
		case 0x27: { /* DAA */
			DEBUG_set_opcode_name("DAA");
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
		case 0x28: { DEBUG_set_opcode_name("JR Z,s");
			if (registers.f & FLAG_Z) finish_instruction(2 + (s8)mem_read(registers.pc+1), 12);
			else finish_instruction(2, 8);
		} break;
		case 0x29: DEBUG_set_opcode_name("ADD HL,HL"); instruction_ADD_HL_u16(registers.hl, 1, 8); break;
		case 0x2a: DEBUG_set_opcode_name("LD A,(HL+)"); registers.a = mem_read(registers.hl++); finish_instruction(1, 8); break;
		case 0x2b: DEBUG_set_opcode_name("DEC HL"); registers.hl--; finish_instruction(1, 8); break;
		case 0x2c: DEBUG_set_opcode_name("INC L"); instruction_INC_u8(&registers.l, 4); break;
		case 0x2d: DEBUG_set_opcode_name("DEC L"); instruction_DEC_u8(&registers.l, 4); break;
		case 0x2e: DEBUG_set_opcode_name("LD L,x"); registers.l = mem_read(registers.pc+1); finish_instruction(2, 8); break;
		case 0x2f: { DEBUG_set_opcode_name("CPL");
			registers.a ^= 0xff;
			registers.f |= FLAG_N;
			registers.f |= FLAG_H;
			finish_instruction(1, 4);
		} break;
		case 0x30: { DEBUG_set_opcode_name("JR NC,s");
			if ((registers.f & FLAG_C) == 0) finish_instruction(2 + (s8)mem_read(registers.pc+1), 12);
			else finish_instruction(2, 8);
		} break;
		case 0x31: DEBUG_set_opcode_name("LD SP,xx"); registers.sp = mem_read_u16(registers.pc+1); finish_instruction(3, 12); break;
		case 0x32: DEBUG_set_opcode_name("LD (HL-),A"); mem_write(registers.hl--, registers.a); finish_instruction(1, 8); break;
		case 0x33: DEBUG_set_opcode_name("INC SP"); registers.sp++; finish_instruction(1, 8); break;
		case 0x34: { DEBUG_set_opcode_name("INC (HL)");
			u8 hl_value = mem_read(registers.hl);
			instruction_INC_u8(&hl_value, 12);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x35: { DEBUG_set_opcode_name("DEC (HL)");
			u8 hl_value = mem_read(registers.hl);
			instruction_DEC_u8(&hl_value, 12);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x36: DEBUG_set_opcode_name("LD (HL),x"); mem_write(registers.hl, mem_read(registers.pc+1)); finish_instruction(2, 12); break;
		case 0x37: { /* SCF */
			DEBUG_set_opcode_name("SCF");
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f |= FLAG_C;
			finish_instruction(1, 4);
		} break;
		case 0x38: { /* JR C,x */
			DEBUG_set_opcode_name("JR C,s");
			if (registers.f & FLAG_C) finish_instruction(2 + (s8)mem_read(registers.pc+1), 12);
			else finish_instruction(2, 8);
		} break;
		case 0x39: DEBUG_set_opcode_name("ADD HL,SP"); instruction_ADD_HL_u16(registers.sp, 1, 8); break;
		case 0x3a: DEBUG_set_opcode_name("LD A,(HL-)"); registers.a = mem_read(registers.hl--); finish_instruction(1, 8); break;
		case 0x3b: DEBUG_set_opcode_name("DEC SP"); registers.sp--; finish_instruction(1, 8); break;
		case 0x3c: DEBUG_set_opcode_name("INC A"); instruction_INC_u8(&registers.a, 4); break;
		case 0x3d: DEBUG_set_opcode_name("DEC A"); instruction_DEC_u8(&registers.a, 4); break;
		case 0x3e: DEBUG_set_opcode_name("LD A,x"); registers.a = mem_read(registers.pc+1); finish_instruction(2, 8); break;
		case 0x3f: { /* CCF */
			DEBUG_set_opcode_name("CCF");
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f ^= FLAG_C;
			finish_instruction(1, 4);
		} break;
		case 0x40: DEBUG_set_opcode_name("LD B,B"); registers.b = registers.b; finish_instruction(1, 4); break;
		case 0x41: DEBUG_set_opcode_name("LD B,C"); registers.b = registers.c; finish_instruction(1, 4); break;
		case 0x42: DEBUG_set_opcode_name("LD B,D"); registers.b = registers.d; finish_instruction(1, 4); break;
		case 0x43: DEBUG_set_opcode_name("LD B,E"); registers.b = registers.e; finish_instruction(1, 4); break;
		case 0x44: DEBUG_set_opcode_name("LD B,H"); registers.b = registers.h; finish_instruction(1, 4); break;
		case 0x45: DEBUG_set_opcode_name("LD B,L"); registers.b = registers.l; finish_instruction(1, 4); break;
		case 0x46: DEBUG_set_opcode_name("LD B,(HL)"); registers.b = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x47: DEBUG_set_opcode_name("LD B,A"); registers.b = registers.a; finish_instruction(1, 4); break;
		case 0x48: DEBUG_set_opcode_name("LD C,B"); registers.c = registers.b; finish_instruction(1, 4); break;
		case 0x49: DEBUG_set_opcode_name("LD C,C"); registers.c = registers.c; finish_instruction(1, 4); break;
		case 0x4a: DEBUG_set_opcode_name("LD C,D"); registers.c = registers.d; finish_instruction(1, 4); break;
		case 0x4b: DEBUG_set_opcode_name("LD C,E"); registers.c = registers.e; finish_instruction(1, 4); break;
		case 0x4c: DEBUG_set_opcode_name("LD C,H"); registers.c = registers.h; finish_instruction(1, 4); break;
		case 0x4d: DEBUG_set_opcode_name("LD C,L"); registers.c = registers.l; finish_instruction(1, 4); break;
		case 0x4e: DEBUG_set_opcode_name("LD C,(HL)"); registers.c = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x4f: DEBUG_set_opcode_name("LD C,A"); registers.c = registers.a; finish_instruction(1, 4); break;
		case 0x50: DEBUG_set_opcode_name("LD D,B"); registers.d = registers.b; finish_instruction(1, 4); break;
		case 0x51: DEBUG_set_opcode_name("LD D,C"); registers.d = registers.c; finish_instruction(1, 4); break;
		case 0x52: DEBUG_set_opcode_name("LD D,D"); registers.d = registers.d; finish_instruction(1, 4); break;
		case 0x53: DEBUG_set_opcode_name("LD D,E"); registers.d = registers.e; finish_instruction(1, 4); break;
		case 0x54: DEBUG_set_opcode_name("LD D,H"); registers.d = registers.h; finish_instruction(1, 4); break;
		case 0x55: DEBUG_set_opcode_name("LD D,L"); registers.d = registers.l; finish_instruction(1, 4); break;
		case 0x56: DEBUG_set_opcode_name("LD D,(HL)"); registers.d = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x57: DEBUG_set_opcode_name("LD D,A"); registers.d = registers.a; finish_instruction(1, 4); break;
		case 0x58: DEBUG_set_opcode_name("LD E,B"); registers.e = registers.b; finish_instruction(1, 4); break;
		case 0x59: DEBUG_set_opcode_name("LD E,C"); registers.e = registers.c; finish_instruction(1, 4); break;
		case 0x5a: DEBUG_set_opcode_name("LD E,D"); registers.e = registers.d; finish_instruction(1, 4); break;
		case 0x5b: DEBUG_set_opcode_name("LD E,E"); registers.e = registers.e; finish_instruction(1, 4); break;
		case 0x5c: DEBUG_set_opcode_name("LD E,H"); registers.e = registers.h; finish_instruction(1, 4); break;
		case 0x5d: DEBUG_set_opcode_name("LD E,L"); registers.e = registers.l; finish_instruction(1, 4); break;
		case 0x5e: DEBUG_set_opcode_name("LD E,(HL)"); registers.e = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x5f: DEBUG_set_opcode_name("LD E,A"); registers.e = registers.a; finish_instruction(1, 4); break;
		case 0x60: DEBUG_set_opcode_name("LD H,B"); registers.h = registers.b; finish_instruction(1, 4); break;
		case 0x61: DEBUG_set_opcode_name("LD H,C"); registers.h = registers.c; finish_instruction(1, 4); break;
		case 0x62: DEBUG_set_opcode_name("LD H,D"); registers.h = registers.d; finish_instruction(1, 4); break;
		case 0x63: DEBUG_set_opcode_name("LD H,E"); registers.h = registers.e; finish_instruction(1, 4); break;
		case 0x64: DEBUG_set_opcode_name("LD H,H"); registers.h = registers.h; finish_instruction(1, 4); break;
		case 0x65: DEBUG_set_opcode_name("LD H,L"); registers.h = registers.l; finish_instruction(1, 4); break;
		case 0x66: DEBUG_set_opcode_name("LD H,(HL)"); registers.h = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x67: DEBUG_set_opcode_name("LD H,A"); registers.h = registers.a; finish_instruction(1, 4); break;
		case 0x68: DEBUG_set_opcode_name("LD L,B"); registers.l = registers.b; finish_instruction(1, 4); break;
		case 0x69: DEBUG_set_opcode_name("LD L,C"); registers.l = registers.c; finish_instruction(1, 4); break;
		case 0x6a: DEBUG_set_opcode_name("LD L,D"); registers.l = registers.d; finish_instruction(1, 4); break;
		case 0x6b: DEBUG_set_opcode_name("LD L,E"); registers.l = registers.e; finish_instruction(1, 4); break;
		case 0x6c: DEBUG_set_opcode_name("LD L,H"); registers.l = registers.h; finish_instruction(1, 4); break;
		case 0x6d: DEBUG_set_opcode_name("LD L,L"); registers.l = registers.l; finish_instruction(1, 4); break;
		case 0x6e: DEBUG_set_opcode_name("LD L,(HL)"); registers.l = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x6f: DEBUG_set_opcode_name("LD L,A"); registers.l = registers.a; finish_instruction(1, 4); break;
		case 0x70: DEBUG_set_opcode_name("LD (HL),B"); mem_write(registers.hl, registers.b); finish_instruction(1, 8); break;
		case 0x71: DEBUG_set_opcode_name("LD (HL),C"); mem_write(registers.hl, registers.c); finish_instruction(1, 8); break;
		case 0x72: DEBUG_set_opcode_name("LD (HL),D"); mem_write(registers.hl, registers.d); finish_instruction(1, 8); break;
		case 0x73: DEBUG_set_opcode_name("LD (HL),E"); mem_write(registers.hl, registers.e); finish_instruction(1, 8); break;
		case 0x74: DEBUG_set_opcode_name("LD (HL),H"); mem_write(registers.hl, registers.h); finish_instruction(1, 8); break;
		case 0x75: DEBUG_set_opcode_name("LD (HL),L"); mem_write(registers.hl, registers.l); finish_instruction(1, 8); break;
		case 0x76: { /* HALT */
			DEBUG_set_opcode_name("HALT");
			assert(!halted); /* Instructions shouldn't be getting executed while halted. */
			
			if (registers.ime) {
				halted = true;
				finish_instruction(1, 4);
			} else finish_instruction(1, 4);
		} break;
		case 0x77: DEBUG_set_opcode_name("LD (HL),A"); mem_write(registers.hl, registers.a); finish_instruction(1, 8); break;
		case 0x78: DEBUG_set_opcode_name("LD A,B"); registers.a = registers.b; finish_instruction(1, 4); break;
		case 0x79: DEBUG_set_opcode_name("LD A,C"); registers.a = registers.c; finish_instruction(1, 4); break;
		case 0x7a: DEBUG_set_opcode_name("LD A,D"); registers.a = registers.d; finish_instruction(1, 4); break;
		case 0x7b: DEBUG_set_opcode_name("LD A,E"); registers.a = registers.e; finish_instruction(1, 4); break;
		case 0x7c: DEBUG_set_opcode_name("LD A,H"); registers.a = registers.h; finish_instruction(1, 4); break;
		case 0x7d: DEBUG_set_opcode_name("LD A,L"); registers.a = registers.l; finish_instruction(1, 4); break;
		case 0x7e: DEBUG_set_opcode_name("LD A,(HL)"); registers.a = mem_read(registers.hl); finish_instruction(1, 8); break;
		case 0x7f: DEBUG_set_opcode_name("LD A,A"); registers.a = registers.a; finish_instruction(1, 4); break;
		case 0x80: DEBUG_set_opcode_name("ADD A,B"); instruction_ADD_A_u8(registers.b, 1, 4); break;
		case 0x81: DEBUG_set_opcode_name("ADD A,C"); instruction_ADD_A_u8(registers.c, 1, 4); break;
		case 0x82: DEBUG_set_opcode_name("ADD A,D"); instruction_ADD_A_u8(registers.d, 1, 4); break;
		case 0x83: DEBUG_set_opcode_name("ADD A,E"); instruction_ADD_A_u8(registers.e, 1, 4); break;
		case 0x84: DEBUG_set_opcode_name("ADD A,H"); instruction_ADD_A_u8(registers.h, 1, 4); break;
		case 0x85: DEBUG_set_opcode_name("ADD A,L"); instruction_ADD_A_u8(registers.l, 1, 4); break;
		case 0x86: DEBUG_set_opcode_name("ADD A,(HL)"); instruction_ADD_A_u8(mem_read(registers.hl), 1, 8); break;
		case 0x87: DEBUG_set_opcode_name("ADD A,A"); instruction_ADD_A_u8(registers.a, 1, 4); break;
		case 0x88: DEBUG_set_opcode_name("ADC A,B"); instruction_ADC(registers.b, 1, 4); break;
		case 0x89: DEBUG_set_opcode_name("ADC A,C"); instruction_ADC(registers.c, 1, 4); break;
		case 0x8a: DEBUG_set_opcode_name("ADC A,D"); instruction_ADC(registers.d, 1, 4); break;
		case 0x8b: DEBUG_set_opcode_name("ADC A,E"); instruction_ADC(registers.e, 1, 4); break;
		case 0x8c: DEBUG_set_opcode_name("ADC A,H"); instruction_ADC(registers.h, 1, 4); break;
		case 0x8d: DEBUG_set_opcode_name("ADC A,L"); instruction_ADC(registers.l, 1, 4); break;
		case 0x8e: DEBUG_set_opcode_name("ADC A,(HL)"); instruction_ADC(mem_read(registers.hl), 1, 8); break;
		case 0x8f: DEBUG_set_opcode_name("ADC A,A"); instruction_ADC(registers.a, 1, 4); break;
		case 0x90: DEBUG_set_opcode_name("SUB B"); instruction_SUB_u8(registers.b, 1, 4); break;
		case 0x91: DEBUG_set_opcode_name("SUB C"); instruction_SUB_u8(registers.c, 1, 4); break;
		case 0x92: DEBUG_set_opcode_name("SUB D"); instruction_SUB_u8(registers.d, 1, 4); break;
		case 0x93: DEBUG_set_opcode_name("SUB E"); instruction_SUB_u8(registers.e, 1, 4); break;
		case 0x94: DEBUG_set_opcode_name("SUB H"); instruction_SUB_u8(registers.h, 1, 4); break;
		case 0x95: DEBUG_set_opcode_name("SUB L"); instruction_SUB_u8(registers.l, 1, 4); break;
		case 0x96: DEBUG_set_opcode_name("SUB (HL)"); instruction_SUB_u8(mem_read(registers.hl), 1, 8); break;
		case 0x97: DEBUG_set_opcode_name("SUB A"); instruction_SUB_u8(registers.a, 1, 4); break;
		case 0x98: DEBUG_set_opcode_name("SBC A,B"); instruction_SBC(registers.b, 1, 4); break;	
		case 0x99: DEBUG_set_opcode_name("SBC A,C"); instruction_SBC(registers.c, 1, 4); break;
		case 0x9a: DEBUG_set_opcode_name("SBC A,D"); instruction_SBC(registers.d, 1, 4); break;
		case 0x9b: DEBUG_set_opcode_name("SBC A,E"); instruction_SBC(registers.e, 1, 4); break;
		case 0x9c: DEBUG_set_opcode_name("SBC A,H"); instruction_SBC(registers.h, 1, 4); break;
		case 0x9d: DEBUG_set_opcode_name("SBC A,L"); instruction_SBC(registers.l, 1, 4); break;
		case 0x9e: DEBUG_set_opcode_name("SBC A,(HL)"); instruction_SBC(mem_read(registers.hl), 1, 8); break;
		case 0x9f: DEBUG_set_opcode_name("SBC A,A"); instruction_SBC(registers.a, 1, 4); break;
		case 0xa0: DEBUG_set_opcode_name("AND B"); instruction_AND(registers.b, 1, 4); break;
		case 0xa1: DEBUG_set_opcode_name("AND C"); instruction_AND(registers.c, 1, 4); break;
		case 0xa2: DEBUG_set_opcode_name("AND D"); instruction_AND(registers.d, 1, 4); break;
		case 0xa3: DEBUG_set_opcode_name("AND E"); instruction_AND(registers.e, 1, 4); break;
		case 0xa4: DEBUG_set_opcode_name("AND H"); instruction_AND(registers.h, 1, 4); break;
		case 0xa5: DEBUG_set_opcode_name("AND L"); instruction_AND(registers.l, 1, 4); break;
		case 0xa6: DEBUG_set_opcode_name("AND (HL)"); instruction_AND(mem_read(registers.hl), 1, 8); break;
		case 0xa7: DEBUG_set_opcode_name("AND A"); instruction_AND(registers.a, 1, 4); break;
		case 0xa8: DEBUG_set_opcode_name("XOR B"); instruction_XOR(registers.b, 4); break;
		case 0xa9: DEBUG_set_opcode_name("XOR C"); instruction_XOR(registers.c, 4); break;
		case 0xaa: DEBUG_set_opcode_name("XOR D"); instruction_XOR(registers.d, 4); break;
		case 0xab: DEBUG_set_opcode_name("XOR E"); instruction_XOR(registers.e, 4); break;
		case 0xac: DEBUG_set_opcode_name("XOR H"); instruction_XOR(registers.h, 4); break;
		case 0xad: DEBUG_set_opcode_name("XOR L"); instruction_XOR(registers.l, 4); break;
		case 0xae: DEBUG_set_opcode_name("XOR (HL)"); instruction_XOR(mem_read(registers.hl), 8); break;
		case 0xaf: DEBUG_set_opcode_name("XOR A"); instruction_XOR(registers.a, 4); break;
		case 0xb0: DEBUG_set_opcode_name("OR B"); instruction_OR(registers.b, 1, 4); break;
		case 0xb1: DEBUG_set_opcode_name("OR C"); instruction_OR(registers.c, 1, 4); break;
		case 0xb2: DEBUG_set_opcode_name("OR D"); instruction_OR(registers.d, 1, 4); break;
		case 0xb3: DEBUG_set_opcode_name("OR E"); instruction_OR(registers.e, 1, 4); break;
		case 0xb4: DEBUG_set_opcode_name("OR H"); instruction_OR(registers.h, 1, 4); break;
		case 0xb5: DEBUG_set_opcode_name("OR L"); instruction_OR(registers.l, 1, 4); break;
		case 0xb6: DEBUG_set_opcode_name("OR (HL)"); instruction_OR(mem_read(registers.hl), 1, 8); break;
		case 0xb7: DEBUG_set_opcode_name("OR A"); instruction_OR(registers.a, 1, 4); break;
		case 0xb8: DEBUG_set_opcode_name("CP B"); instruction_CP(registers.b, 1, 4); break;
		case 0xb9: DEBUG_set_opcode_name("CP C"); instruction_CP(registers.c, 1, 4); break;
		case 0xba: DEBUG_set_opcode_name("CP D"); instruction_CP(registers.d, 1, 4); break;
		case 0xbb: DEBUG_set_opcode_name("CP E"); instruction_CP(registers.e, 1, 4); break;
		case 0xbc: DEBUG_set_opcode_name("CP H"); instruction_CP(registers.h, 1, 4); break;
		case 0xbd: DEBUG_set_opcode_name("CP L"); instruction_CP(registers.l, 1, 4); break;
		case 0xbe: DEBUG_set_opcode_name("CP (HL)"); instruction_CP(mem_read(registers.hl), 1, 8); break;
		case 0xbf: DEBUG_set_opcode_name("RES 7,A"); instruction_CP(registers.a, 1, 4); break;
		case 0xc0: { DEBUG_set_opcode_name("RET NZ");
			if (registers.f & FLAG_Z) finish_instruction(1, 8);
			else {
				registers.pc = stack_pop();
				finish_instruction(0, 20);
			}
		} break;
		case 0xc1: { DEBUG_set_opcode_name("POP BC");
			registers.bc = stack_pop();
			finish_instruction(1, 12);
		} break;
		case 0xc2: { DEBUG_set_opcode_name("JP NZ,xx");
			if ((registers.f & FLAG_Z) == 0) {
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction(0, 16);
			} else {
				finish_instruction(3, 12);
			}
		} break;
		case 0xc3: { DEBUG_set_opcode_name("JP xx");
			registers.pc = mem_read_u16(registers.pc+1);
			finish_instruction(0, 16);
		} break;
		case 0xc4: DEBUG_set_opcode_name("CALL NZ,xx"); instruction_CALL_cond_xx((registers.f & FLAG_Z) == 0); break;
		case 0xc5: { DEBUG_set_opcode_name("PUSH BC");
			stack_push(registers.bc);
			finish_instruction(1, 16);
		} break;
		case 0xc6: DEBUG_set_opcode_name("ADD A,x"); instruction_ADD_A_u8(mem_read(registers.pc+1), 2, 8); break;
		case 0xc7: DEBUG_set_opcode_name("RST 00h"); instruction_RST(0x00); break;
		case 0xc8: { DEBUG_set_opcode_name("RET Z");
			if (registers.f & FLAG_Z) {
				registers.pc = stack_pop();
				finish_instruction(0, 20);
			} else finish_instruction(1, 8);
		} break;
		case 0xc9: { DEBUG_set_opcode_name("RET");
			registers.pc = stack_pop();
			finish_instruction(0, 16);
		} break;
		case 0xca: { DEBUG_set_opcode_name("JP Z,xx");
			if (registers.f & FLAG_Z) {
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction(0, 16);
			} else finish_instruction(3, 12);
		} break;
		case 0xcb: execute_cb_opcode(); break;
		case 0xcc: DEBUG_set_opcode_name("CALL Z,xx"); instruction_CALL_cond_xx(registers.f & FLAG_Z); break;
		case 0xcd: { DEBUG_set_opcode_name("CALL xx");
			instruction_CALL_cond_xx(true); break;
		} break;
		case 0xce: DEBUG_set_opcode_name("ADC A,x"); instruction_ADC(mem_read(registers.pc+1), 2, 8); break;
		case 0xcf: DEBUG_set_opcode_name("RST 08h"); instruction_RST(0x08); break;
		case 0xd0: { DEBUG_set_opcode_name("RET NC");
			if ((registers.f & FLAG_C) == false) {
				registers.pc = stack_pop();
				finish_instruction(0, 20);
			} else finish_instruction(1, 8);
		} break;
		case 0xd1: { DEBUG_set_opcode_name("POP DE");
			registers.de = stack_pop();
			finish_instruction(1, 12);
		} break;
		case 0xd2: { DEBUG_set_opcode_name("JP NC,xx");
			if ((registers.f & FLAG_C) == 0) {
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction(0, 16);
			} else {
				finish_instruction(3, 12);
			}
		} break;
		case 0xd3: DEBUG_set_opcode_name("(invalid)"); break;
		case 0xd4: DEBUG_set_opcode_name("CALL NC,xx"); instruction_CALL_cond_xx((registers.f & FLAG_C) == 0); break;
		case 0xd5: { DEBUG_set_opcode_name("PUSH DE");
			stack_push(registers.de);
			finish_instruction(1, 16);
		} break;
		case 0xd6: instruction_SUB_u8(mem_read(registers.pc+1), 2, 8); break;
		case 0xd7: DEBUG_set_opcode_name("RST 10h"); instruction_RST(0x10); break;
		case 0xd8: { DEBUG_set_opcode_name("RET C");
			if (registers.f & FLAG_C) {
				registers.pc = stack_pop();
				finish_instruction(0, 20);
			} else finish_instruction(1, 8);
		} break;
		case 0xd9: { DEBUG_set_opcode_name("RETI");
			registers.pc = stack_pop();
			registers.ime = true;
			finish_instruction(0, 16);
		} break;
		case 0xda: { DEBUG_set_opcode_name("JP C,xx");
			if (registers.f & FLAG_C) {
				registers.pc = mem_read_u16(registers.pc+1);
				finish_instruction(0, 16);
			} else finish_instruction(3, 12);
		} break;
		case 0xdb: DEBUG_set_opcode_name("(invalid)"); assert(false); break;
		case 0xdc: DEBUG_set_opcode_name("CALL C,xx"); instruction_CALL_cond_xx(registers.f & FLAG_C); break;
		case 0xdd: DEBUG_set_opcode_name("(invalid)"); assert(false); break;
		case 0xde: DEBUG_set_opcode_name("SBC A,x"); instruction_SBC(mem_read(registers.pc+1), 2, 8); break;
		case 0xdf: DEBUG_set_opcode_name("RST 18H"); instruction_RST(0x18); break;
		case 0xe0: { DEBUG_set_opcode_name("LDH (ff00+x),A");
			u8 byte_0 = mem_read(registers.pc+1);
			mem_write(0xff00 + byte_0, registers.a);
			finish_instruction(2, 12);
		} break;
		case 0xe1: { DEBUG_set_opcode_name("POP HL");
			registers.hl = stack_pop();
			finish_instruction(1, 12);
		} break;
		case 0xe2: { DEBUG_set_opcode_name("LD (ff00+C),A");
			mem_write(0xff00 + registers.c, registers.a);
			finish_instruction(1, 8);
		} break;
		case 0xe3: DEBUG_set_opcode_name("(invalid)"); assert(false); break;
		case 0xe4: DEBUG_set_opcode_name("(invalid)"); assert(false); break;
		case 0xe5: { DEBUG_set_opcode_name("PUSH HL");
			stack_push(registers.hl);
			finish_instruction(1, 16);
		} break;
		case 0xe6: DEBUG_set_opcode_name("AND x"); instruction_AND(mem_read(registers.pc+1), 2, 8); break;
		case 0xe7: DEBUG_set_opcode_name("RST 20H"); instruction_RST(0x20); break;
		case 0xe8: { DEBUG_set_opcode_name("ADD SP,s");
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
		case 0xe9: { DEBUG_set_opcode_name("JP (HL)");
			registers.pc = registers.hl;
			finish_instruction(0, 4);
		} break;
		case 0xea: { DEBUG_set_opcode_name("LD (x),A");
			mem_write(mem_read_u16(registers.pc+1), registers.a);
			finish_instruction(3, 16);
		} break;
		case 0xeb: DEBUG_set_opcode_name("(invalid)"); break;
		case 0xec: DEBUG_set_opcode_name("(invalid)"); break;
		case 0xed: DEBUG_set_opcode_name("(invalid)"); break;
		case 0xee: { DEBUG_set_opcode_name("XOR x");
			u8 byte_0 = mem_read(registers.pc+1);
			registers.a ^= byte_0;
			
			if (registers.a == 0) registers.f |= FLAG_Z;
			else registers.f &= ~FLAG_Z;
			
			registers.f &= ~FLAG_N;
			registers.f &= ~FLAG_H;
			registers.f &= ~FLAG_C;
			finish_instruction(2, 4);
		} break;
		case 0xef: DEBUG_set_opcode_name("RST 28H"); instruction_RST(0x28); break;
		case 0xf0: { DEBUG_set_opcode_name("LDH A,(0xff00+x)");
			registers.a = mem_read(0xff00 + mem_read(registers.pc+1));
			finish_instruction(2, 12);
		} break;
		case 0xf1: { DEBUG_set_opcode_name("POP AF");
			registers.af = stack_pop() & 0xfff0; /* lower nybble of F must stay 0 */
			finish_instruction(1, 12);
		} break;
		case 0xf2: { DEBUG_set_opcode_name("LD A,(ff00+C)");
			registers.a = mem_read(0xff00 + registers.c);
			finish_instruction(1, 8);
		} break;
		case 0xf3: { DEBUG_set_opcode_name("DI");
			registers.ime = false;
			finish_instruction(1, 4);
		} break;
		case 0xf4: DEBUG_set_opcode_name("(invalid)"); break;
		case 0xf5: { DEBUG_set_opcode_name("PUSH AF");
			stack_push(registers.af);
			finish_instruction(1, 16);
		} break;
		case 0xf6: DEBUG_set_opcode_name("OR x"); instruction_OR(mem_read(registers.pc+1), 2, 8); break;
		case 0xf7: DEBUG_set_opcode_name("RST 30H"); instruction_RST(0x30); break;
		case 0xf8: { DEBUG_set_opcode_name("LDHL SP,s");
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
		case 0xf9: { DEBUG_set_opcode_name("LD SP,HL");
			registers.sp = registers.hl;
			finish_instruction(1, 8);
		} break;
		case 0xfa: { DEBUG_set_opcode_name("LD A,(xx)");
			u16 address = mem_read_u16(registers.pc+1);
			registers.a = mem_read(address);
			finish_instruction(3, 16);
		} break;
		case 0xfb: { DEBUG_set_opcode_name("IE");
			registers.ime = true;
			finish_instruction(1, 4);
		} break;
		case 0xfc: DEBUG_set_opcode_name("(invalid)"); break;
		case 0xfd: DEBUG_set_opcode_name("(invalid)"); break;
		case 0xfe: { DEBUG_set_opcode_name("CP x");
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
		case 0xff: DEBUG_set_opcode_name("RST 38H"); instruction_RST(0x38); break;
		default: {
			char buf[128] = {0};
			sprintf(buf, "Unknown opcode %x at address %x\n", opcode, registers.pc);
			robingb_log(buf);
			assert(false);
		} break;
	}
	
	assert((registers.f & 0x0f) == 0);
}







