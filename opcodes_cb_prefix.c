#include "internal.h"
#include <assert.h>
#include <stdio.h>

#define INSTRUCTION static

INSTRUCTION void instruction_RL(u8 *byte_to_rotate, u8 num_cycles) {
	bool prev_carry = registers.f & FLAG_C;
	
	if ((*byte_to_rotate) & bit(7)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	*byte_to_rotate <<= 1;
	
	if (prev_carry) *byte_to_rotate |= bit(0);
	else *byte_to_rotate &= ~bit(0);
	
	if (*byte_to_rotate == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	
	finish_instruction(1, num_cycles);
}

INSTRUCTION void instruction_RLC(u8 *byte_to_rotate, u8 num_cycles) {
	if ((*byte_to_rotate) & bit(7)) {
		registers.f |= FLAG_C;
		*byte_to_rotate <<= 1;
		*byte_to_rotate |= bit(0);
	} else {
		registers.f &= ~FLAG_C;
		*byte_to_rotate <<= 1;
		*byte_to_rotate &= ~bit(0);
	}
	
	if (*byte_to_rotate == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	
	finish_instruction(1, num_cycles);
}

INSTRUCTION void instruction_RRC(u8 *byte_to_rotate, u8 num_cycles) {
	if ((*byte_to_rotate) & bit(0)) {
		registers.f |= FLAG_C;
		*byte_to_rotate >>= 1;
		*byte_to_rotate |= bit(7);
	} else {
		registers.f &= ~FLAG_C;
		*byte_to_rotate >>= 1;
		*byte_to_rotate &= ~bit(7);
	}
	
	if (*byte_to_rotate == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	
	finish_instruction(1, num_cycles);
}

INSTRUCTION void instruction_RR(u8 *byte_to_rotate, u8 num_cycles) {
	bool prev_carry = registers.f & FLAG_C;
	
	if ((*byte_to_rotate) & bit(0)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	*byte_to_rotate >>= 1;
	
	if (prev_carry) *byte_to_rotate |= bit(7);
	
	if (*byte_to_rotate == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	
	finish_instruction(1, num_cycles);
}

INSTRUCTION void instruction_SLA(u8 *byte_to_shift) {
	if ((*byte_to_shift) & bit(7)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	*byte_to_shift <<= 1;
	*byte_to_shift &= ~bit(0); /* bit 0 should become 0. */
	
	if ((*byte_to_shift) == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	
	finish_instruction(1, 8);
}

INSTRUCTION void instruction_SRA(u8 *byte_to_shift, u8 num_cycles) {
	if ((*byte_to_shift) & bit(0)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	*byte_to_shift >>= 1;
	*byte_to_shift |= ((*byte_to_shift) & bit(6)) << 1; /* bit 7 should stay the same. */
	
	if ((*byte_to_shift) == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	
	finish_instruction(1, num_cycles);
}

INSTRUCTION void instruction_SWAP(u8 *byte_to_swap, u8 num_cycles) {
	u8 upper_4_bits = (*byte_to_swap) & 0xf0;
	u8 lower_4_bits = (*byte_to_swap) & 0x0f;
	*byte_to_swap = upper_4_bits >> 4;
	*byte_to_swap |= lower_4_bits << 4;
	
	if (*byte_to_swap == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	registers.f &= ~FLAG_C;
	finish_instruction(1, num_cycles);
}

INSTRUCTION void instruction_SRL(u8 *byte_to_shift, u8 num_cycles) {
	if ((*byte_to_shift) & bit(0)) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	*byte_to_shift >>= 1; /* bit 7 becomes 0. */
	assert(((*byte_to_shift) & bit(7)) == false);
	
	if ((*byte_to_shift) == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	
	finish_instruction(1, num_cycles);
}

INSTRUCTION void instruction_BIT(u8 bit_index, u8 byte_to_check, int num_cycles) {
	if (byte_to_check & (0x01 << bit_index)) registers.f &= ~FLAG_Z;
	else registers.f |= FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f |= FLAG_H;
	
	finish_instruction(1, num_cycles);
}

INSTRUCTION void instruction_RES(u8 bit_index, u8 *byte_to_reset, u8 num_cycles) {
	*byte_to_reset &= ~(0x01 << bit_index);
	finish_instruction(1, num_cycles);
}

INSTRUCTION void instruction_SET(u8 bit_index, u8 *register_to_set, u8 num_cycles) {
	*register_to_set |= 0x01 << bit_index;
	finish_instruction(1, num_cycles);
}

void execute_cb_opcode() {
	assert(mem_read(registers.pc) == 0xcb);
	u8 opcode = mem_read(++registers.pc);
	
	switch (opcode) {
		case 0x00: DEBUG_set_opcode_name("RLC B"); instruction_RLC(&registers.b, 8); break;
		case 0x01: DEBUG_set_opcode_name("RLC C"); instruction_RLC(&registers.c, 8); break;
		case 0x02: DEBUG_set_opcode_name("RLC D"); instruction_RLC(&registers.d, 8); break;
		case 0x03: DEBUG_set_opcode_name("RLC E"); instruction_RLC(&registers.e, 8); break;
		case 0x04: DEBUG_set_opcode_name("RLC H"); instruction_RLC(&registers.h, 8); break;
		case 0x05: DEBUG_set_opcode_name("RLC L"); instruction_RLC(&registers.l, 8); break;
		case 0x06: {
			u8 hl_value = mem_read(registers.hl);
			instruction_RLC(&hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x07: instruction_RLC(&registers.a, 8); break;
		case 0x08: instruction_RRC(&registers.b, 8); break;
		case 0x09: instruction_RRC(&registers.c, 8); break;
		case 0x0a: instruction_RRC(&registers.d, 8); break;
		case 0x0b: instruction_RRC(&registers.e, 8); break;
		case 0x0c: instruction_RRC(&registers.h, 8); break;
		case 0x0d: instruction_RRC(&registers.l, 8); break;
		case 0x0e: {
					u8 hl_value = mem_read(registers.hl);
					instruction_RRC(&hl_value, 16);
					mem_write(registers.hl, hl_value);
				} break;
		case 0x0f: instruction_RRC(&registers.a, 8); break;
		case 0x10: instruction_RL(&registers.b, 8); break;
		case 0x11: instruction_RL(&registers.c, 8); break;
		case 0x12: instruction_RL(&registers.d, 8); break;
		case 0x13: instruction_RL(&registers.e, 8); break;
		case 0x14: instruction_RL(&registers.h, 8); break;
		case 0x15: instruction_RL(&registers.l, 8); break;
		case 0x16: {
			u8 hl_value = mem_read(registers.hl);
			instruction_RL(&hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x17: instruction_RL(&registers.a, 8); break;
		case 0x18: instruction_RR(&registers.b, 8); break;
		case 0x19: instruction_RR(&registers.c, 8); break;
		case 0x1a: instruction_RR(&registers.d, 8); break;
		case 0x1b: instruction_RR(&registers.e, 8); break;
		case 0x1c: instruction_RR(&registers.h, 8); break;
		case 0x1d: instruction_RR(&registers.l, 8); break;
		case 0x1e: {
			u8 hl_value = mem_read(registers.hl);
			instruction_RR(&hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x1f: instruction_RR(&registers.a, 8); break;
		case 0x20: instruction_SLA(&registers.b); break;
		case 0x21: instruction_SLA(&registers.c); break;
		case 0x22: instruction_SLA(&registers.d); break;
		case 0x23: instruction_SLA(&registers.e); break;
		case 0x24: instruction_SLA(&registers.h); break;
		case 0x25: instruction_SLA(&registers.l); break;
		case 0x26: {
			u8 hl_value = mem_read(registers.hl);
			instruction_SLA(&hl_value);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x27: instruction_SLA(&registers.a); break;
		case 0x28: instruction_SRA(&registers.b, 8); break;
		case 0x29: instruction_SRA(&registers.c, 8); break;
		case 0x2a: instruction_SRA(&registers.d, 8); break;
		case 0x2b: instruction_SRA(&registers.e, 8); break;
		case 0x2c: instruction_SRA(&registers.h, 8); break;
		case 0x2d: instruction_SRA(&registers.l, 8); break;
		case 0x2e: {
			u8 hl_value = mem_read(registers.hl);
			instruction_SRA(&hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x2f: instruction_SRA(&registers.a, 8); break;
		case 0x30: instruction_SWAP(&registers.b, 8); break;
		case 0x31: instruction_SWAP(&registers.c, 8); break;
		case 0x32: instruction_SWAP(&registers.d, 8); break;
		case 0x33: instruction_SWAP(&registers.e, 8); break;
		case 0x34: instruction_SWAP(&registers.h, 8); break;
		case 0x35: instruction_SWAP(&registers.l, 8); break;
		case 0x36: {
			u8 hl_value = mem_read(registers.hl);
			instruction_SWAP(&hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x37: instruction_SWAP(&registers.a, 8); break;
		case 0x38: instruction_SRL(&registers.b, 8); break;
		case 0x39: instruction_SRL(&registers.c, 8); break;
		case 0x3a: instruction_SRL(&registers.d, 8); break;
		case 0x3b: instruction_SRL(&registers.e, 8); break;
		case 0x3c: instruction_SRL(&registers.h, 8); break;
		case 0x3d: instruction_SRL(&registers.l, 8); break;
		case 0x3e: {
			u8 hl_value = mem_read(registers.hl);
			instruction_SRL(&hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x3f: instruction_SRL(&registers.a, 8); break;
		case 0x40: instruction_BIT(0, registers.b, 8); break;
		case 0x41: instruction_BIT(0, registers.c, 8); break;
		case 0x42: instruction_BIT(0, registers.d, 8); break;
		case 0x43: instruction_BIT(0, registers.e, 8); break;
		case 0x44: instruction_BIT(0, registers.h, 8); break;
		case 0x45: instruction_BIT(0, registers.l, 8); break;
		case 0x46: instruction_BIT(0, mem_read(registers.hl), 16); break;
		case 0x47: instruction_BIT(0, registers.a, 8); break;
		case 0x48: instruction_BIT(1, registers.b, 8); break;
		case 0x49: instruction_BIT(1, registers.c, 8); break;
		case 0x4a: instruction_BIT(1, registers.d, 8); break;
		case 0x4b: instruction_BIT(1, registers.e, 8); break;
		case 0x4c: instruction_BIT(1, registers.h, 8); break;
		case 0x4d: instruction_BIT(1, registers.l, 8); break;
		case 0x4e: instruction_BIT(1, mem_read(registers.hl), 16); break;
		case 0x4f: instruction_BIT(1, registers.a, 8); break;
		case 0x50: instruction_BIT(2, registers.b, 8); break;
		case 0x51: instruction_BIT(2, registers.c, 8); break;
		case 0x52: instruction_BIT(2, registers.d, 8); break;
		case 0x53: instruction_BIT(2, registers.e, 8); break;
		case 0x54: instruction_BIT(2, registers.h, 8); break;
		case 0x55: instruction_BIT(2, registers.l, 8); break;
		case 0x56: instruction_BIT(2, mem_read(registers.hl), 16); break;
		case 0x57: instruction_BIT(2, registers.a, 8); break;
		case 0x58: instruction_BIT(3, registers.b, 8); break;
		case 0x59: instruction_BIT(3, registers.c, 8); break;
		case 0x5a: instruction_BIT(3, registers.d, 8); break;
		case 0x5b: instruction_BIT(3, registers.e, 8); break;
		case 0x5c: instruction_BIT(3, registers.h, 8); break;
		case 0x5d: instruction_BIT(3, registers.l, 8); break;
		case 0x5e: {
			u8 hl_value = mem_read(registers.hl);
			instruction_SET(3, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x5f: instruction_BIT(3, registers.a, 8); break;
		case 0x60: instruction_BIT(4, registers.b, 8); break;
		case 0x61: instruction_BIT(4, registers.c, 8); break;
		case 0x62: instruction_BIT(4, registers.d, 8); break;
		case 0x63: instruction_BIT(4, registers.e, 8); break;
		case 0x64: instruction_BIT(4, registers.h, 8); break;
		case 0x65: instruction_BIT(4, registers.l, 8); break;
		case 0x66: instruction_BIT(4, mem_read(registers.hl), 16); break;
		case 0x67: instruction_BIT(4, registers.a, 8); break;
		case 0x68: instruction_BIT(5, registers.b, 8); break;
		case 0x69: instruction_BIT(5, registers.c, 8); break;
		case 0x6a: instruction_BIT(5, registers.d, 8); break;
		case 0x6b: instruction_BIT(5, registers.e, 8); break;
		case 0x6c: instruction_BIT(5, registers.h, 8); break;
		case 0x6d: instruction_BIT(5, registers.l, 8); break;
		case 0x6e: instruction_BIT(5, mem_read(registers.hl), 16); break;
		case 0x6f: instruction_BIT(5, registers.a, 8); break;
		case 0x70: instruction_BIT(6, registers.b, 8); break;
		case 0x71: instruction_BIT(6, registers.c, 8); break;
		case 0x72: instruction_BIT(6, registers.d, 8); break;
		case 0x73: instruction_BIT(6, registers.e, 8); break;
		case 0x74: instruction_BIT(6, registers.h, 8); break;
		case 0x75: instruction_BIT(6, registers.l, 8); break;
		case 0x76: instruction_BIT(6, mem_read(registers.hl), 16); break;
		case 0x77: instruction_BIT(6, registers.a, 8); break;
		case 0x78: instruction_BIT(7, registers.b, 8); break;
		case 0x79: instruction_BIT(7, registers.c, 8); break;
		case 0x7a: instruction_BIT(7, registers.d, 8); break;
		case 0x7b: instruction_BIT(7, registers.e, 8); break;
		case 0x7c: instruction_BIT(7, registers.h, 8); break;
		case 0x7d: instruction_BIT(7, registers.l, 8); break;
		case 0x7e: instruction_BIT(7, mem_read(registers.hl), 16); break;
		case 0x7f: instruction_BIT(7, registers.a, 8); break;
		case 0x80: instruction_RES(0, &registers.b, 8); break;
		case 0x81: instruction_RES(0, &registers.c, 8); break;
		case 0x82: instruction_RES(0, &registers.d, 8); break;
		case 0x83: instruction_RES(0, &registers.e, 8); break;
		case 0x84: instruction_RES(0, &registers.h, 8); break;
		case 0x85: instruction_RES(0, &registers.l, 8); break;
		case 0x86: {
			u8 hl_value = mem_read(registers.hl);
			instruction_RES(0, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x87: instruction_RES(0, &registers.a, 8); break;
		case 0x88: instruction_RES(1, &registers.b, 8); break;
		case 0x89: instruction_RES(1, &registers.c, 8); break;
		case 0x8a: instruction_RES(1, &registers.d, 8); break;
		case 0x8b: instruction_RES(1, &registers.e, 8); break;
		case 0x8c: instruction_RES(1, &registers.h, 8); break;
		case 0x8d: instruction_RES(1, &registers.l, 8); break;
		case 0x8e: {
			u8 hl_value = mem_read(registers.hl);
			instruction_RES(1, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x8f: instruction_RES(1, &registers.a, 8); break;
		case 0x90: instruction_RES(2, &registers.b, 8); break;
		case 0x91: instruction_RES(2, &registers.c, 8); break;
		case 0x92: instruction_RES(2, &registers.d, 8); break;
		case 0x93: instruction_RES(2, &registers.e, 8); break;
		case 0x94: instruction_RES(2, &registers.h, 8); break;
		case 0x95: instruction_RES(2, &registers.l, 8); break;
		case 0x96: {
			u8 hl_value = mem_read(registers.hl);
			instruction_RES(2, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x97: instruction_RES(2, &registers.a, 8); break;
		case 0x98: instruction_RES(3, &registers.b, 8); break;
		case 0x99: instruction_RES(3, &registers.c, 8); break;
		case 0x9a: instruction_RES(3, &registers.d, 8); break;
		case 0x9b: instruction_RES(3, &registers.e, 8); break;
		case 0x9c: instruction_RES(3, &registers.h, 8); break;
		case 0x9d: instruction_RES(3, &registers.l, 8); break;
		case 0x9e: {
			u8 hl_value = mem_read(registers.hl);
			instruction_RES(3, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x9f: instruction_RES(3, &registers.a, 8); break;
		case 0xa0: instruction_RES(4, &registers.b, 8); break;
		case 0xa1: instruction_RES(4, &registers.c, 8); break;
		case 0xa2: instruction_RES(4, &registers.d, 8); break;
		case 0xa3: instruction_RES(4, &registers.e, 8); break;
		case 0xa4: instruction_RES(4, &registers.h, 8); break;
		case 0xa5: instruction_RES(4, &registers.l, 8); break;
		case 0xa6: {
			u8 hl_value = mem_read(registers.hl);
			instruction_RES(4, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xa7: instruction_RES(4, &registers.a, 8); break;
		case 0xa8: instruction_RES(5, &registers.b, 8); break;
		case 0xa9: instruction_RES(5, &registers.c, 8); break;
		case 0xaa: instruction_RES(5, &registers.d, 8); break;
		case 0xab: instruction_RES(5, &registers.e, 8); break;
		case 0xac: instruction_RES(5, &registers.h, 8); break;
		case 0xad: instruction_RES(5, &registers.l, 8); break;
		case 0xae: {
			u8 hl_value = mem_read(registers.hl);
			instruction_RES(5, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xaf: instruction_RES(5, &registers.a, 8); break;
		case 0xb0: instruction_RES(6, &registers.b, 8); break;
		case 0xb1: instruction_RES(6, &registers.c, 8); break;
		case 0xb2: instruction_RES(6, &registers.d, 8); break;
		case 0xb3: instruction_RES(6, &registers.e, 8); break;
		case 0xb4: instruction_RES(6, &registers.h, 8); break;
		case 0xb5: instruction_RES(6, &registers.l, 8); break;
		case 0xb6: {
			u8 hl_value = mem_read(registers.hl);
			instruction_RES(6, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xb7: instruction_RES(6, &registers.a, 8); break;
		case 0xb8: instruction_RES(7, &registers.b, 8); break;
		case 0xb9: instruction_RES(7, &registers.c, 8); break;
		case 0xba: instruction_RES(7, &registers.d, 8); break;
		case 0xbb: instruction_RES(7, &registers.e, 8); break;
		case 0xbc: instruction_RES(7, &registers.h, 8); break;
		case 0xbd: instruction_RES(7, &registers.l, 8); break;
		case 0xbe: {
			u8 hl_value = mem_read(registers.hl);
			instruction_RES(7, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xbf: instruction_RES(7, &registers.a, 8); break;
		case 0xc0: instruction_SET(0, &registers.b, 8); break;
		case 0xc1: instruction_SET(0, &registers.c, 8); break;
		case 0xc2: instruction_SET(0, &registers.d, 8); break;
		case 0xc3: instruction_SET(0, &registers.e, 8); break;
		case 0xc4: instruction_SET(0, &registers.h, 8); break;
		case 0xc5: instruction_SET(0, &registers.l, 8); break;
		case 0xc6: {
			u8 hl_value = mem_read(registers.hl);
			instruction_SET(0, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xc7: instruction_SET(0, &registers.a, 8); break;
		case 0xc8: instruction_SET(1, &registers.b, 8); break;
		case 0xc9: instruction_SET(1, &registers.c, 8); break;
		case 0xca: instruction_SET(1, &registers.d, 8); break;
		case 0xcb: instruction_SET(1, &registers.e, 8); break;
		case 0xcc: instruction_SET(1, &registers.h, 8); break;
		case 0xcd: instruction_SET(1, &registers.l, 8); break;
		case 0xce: {
			u8 hl_value = mem_read(registers.hl);
			instruction_SET(1, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xcf: instruction_SET(1, &registers.a, 8); break;
		case 0xd0: instruction_SET(2, &registers.b, 8); break;
		case 0xd1: instruction_SET(2, &registers.c, 8); break;
		case 0xd2: instruction_SET(2, &registers.d, 8); break;
		case 0xd3: instruction_SET(2, &registers.e, 8); break;
		case 0xd4: instruction_SET(2, &registers.h, 8); break;
		case 0xd5: instruction_SET(2, &registers.l, 8); break;
		case 0xd6: {
			u8 hl_value = mem_read(registers.hl);
			instruction_SET(2, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xd7: instruction_SET(2, &registers.a, 8); break;
		case 0xd8: instruction_SET(3, &registers.b, 8); break;
		case 0xd9: instruction_SET(3, &registers.c, 8); break;
		case 0xda: instruction_SET(3, &registers.d, 8); break;
		case 0xdb: instruction_SET(3, &registers.e, 8); break;
		case 0xdc: instruction_SET(3, &registers.h, 8); break;
		case 0xdd: instruction_SET(3, &registers.l, 8); break;
		case 0xde: {
			u8 hl_value = mem_read(registers.hl);
			instruction_SET(3, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xdf: instruction_SET(3, &registers.a, 8); break;
		case 0xe0: instruction_SET(4, &registers.b, 8); break;
		case 0xe1: instruction_SET(4, &registers.c, 8); break;
		case 0xe2: instruction_SET(4, &registers.d, 8); break;
		case 0xe3: instruction_SET(4, &registers.e, 8); break;
		case 0xe4: instruction_SET(4, &registers.h, 8); break;
		case 0xe5: instruction_SET(4, &registers.l, 8); break;
		case 0xe6: {
			u8 hl_value = mem_read(registers.hl);
			instruction_SET(4, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xe7: instruction_SET(4, &registers.a, 8); break;
		case 0xe8: instruction_SET(5, &registers.b, 8); break;
		case 0xe9: instruction_SET(5, &registers.c, 8); break;
		case 0xea: instruction_SET(5, &registers.d, 8); break;
		case 0xeb: instruction_SET(5, &registers.e, 8); break;
		case 0xec: instruction_SET(5, &registers.h, 8); break;
		case 0xed: instruction_SET(5, &registers.l, 8); break;
		case 0xee: {
			u8 hl_value = mem_read(registers.hl);
			instruction_SET(5, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xef: instruction_SET(5, &registers.a, 8); break;
		case 0xf0: instruction_SET(6, &registers.b, 8); break;
		case 0xf1: instruction_SET(6, &registers.c, 8); break;
		case 0xf2: instruction_SET(6, &registers.d, 8); break;
		case 0xf3: instruction_SET(6, &registers.e, 8); break;
		case 0xf4: instruction_SET(6, &registers.h, 8); break;
		case 0xf5: instruction_SET(6, &registers.l, 8); break;
		case 0xf6: {
			u8 hl_value = mem_read(registers.hl);
			instruction_SET(6, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xf7: instruction_SET(6, &registers.a, 8); break;
		case 0xf8: instruction_SET(7, &registers.b, 8); break;
		case 0xf9: instruction_SET(7, &registers.c, 8); break;
		case 0xfa: instruction_SET(7, &registers.d, 8); break;
		case 0xfb: instruction_SET(7, &registers.e, 8); break;
		case 0xfc: instruction_SET(7, &registers.h, 8); break;
		case 0xfd: instruction_SET(7, &registers.l, 8); break;
		case 0xfe: {
			u8 hl_value = mem_read(registers.hl);
			instruction_SET(7, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xff: instruction_SET(7, &registers.a, 8); break;
		default: {
			char buf[128] = {0};
			sprintf(buf, "Unknown opcode cb %x at address %x\n", opcode, registers.pc-1);
			robingb_log(buf);
			assert(false);
		} break;
	}
}





