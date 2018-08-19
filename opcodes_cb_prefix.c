#include "internal.h"
#include <assert.h>
#include <stdio.h>

void execute_instruction_RL(u8 *byte_to_rotate, u8 num_cycles) {
	bool prev_carry = registers.f & FLAG_C;
	
	if ((*byte_to_rotate) & 0x80) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	*byte_to_rotate <<= 1;
	
	if (prev_carry) *byte_to_rotate |= 0x0f;
	
	if (*byte_to_rotate == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	
	finish_instruction(1, num_cycles);
}

void execute_instruction_RR(u8 *byte_to_rotate, u8 num_cycles) {
	bool prev_carry = registers.f & FLAG_C;
	
	if ((*byte_to_rotate) & 0x01) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	*byte_to_rotate >>= 1;
	
	if (prev_carry) *byte_to_rotate |= 0x80;
	
	if (*byte_to_rotate == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	
	finish_instruction(1, num_cycles);
}

void execute_instruction_SLA(u8 *byte_to_shift) {
	if ((*byte_to_shift) & 0x80) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	*byte_to_shift <<= 1; // bit 0 becomes 0.
	assert(((*byte_to_shift) & 0x01) == false);
	
	if ((*byte_to_shift) == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	
	finish_instruction(1, 8);
}

void execute_instruction_SWAP(u8 *byte_to_swap, u8 num_cycles) {
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

void execute_instruction_SRL(u8 *byte_to_shift) {
	if ((*byte_to_shift) & 0x01) registers.f |= FLAG_C;
	else registers.f &= ~FLAG_C;
	
	*byte_to_shift >>= 1; // bit 7 becomes 0.
	assert(((*byte_to_shift) & 0x80) == false);
	
	if ((*byte_to_shift) == 0) registers.f |= FLAG_Z;
	else registers.f &= ~FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f &= ~FLAG_H;
	
	finish_instruction(1, 8);
}

void execute_instruction_BIT(u8 bit_index, u8 byte_to_check, int num_cycles) {
	if (byte_to_check & (0x01 << bit_index)) registers.f &= ~FLAG_Z;
	else registers.f |= FLAG_Z;
	
	registers.f &= ~FLAG_N;
	registers.f |= FLAG_H;
	
	finish_instruction(1, num_cycles);
}

void execute_instruction_RES(u8 bit_index, u8 *byte_to_reset, u8 num_cycles) {
	*byte_to_reset &= ~(0x01 << bit_index);
	finish_instruction(1, num_cycles);
}

void execute_instruction_SET(u8 bit_index, u8 *register_to_set, u8 num_cycles) {
	*register_to_set |= 0x01 << bit_index;
	finish_instruction(1, num_cycles);
}

void execute_cb_opcode() {
	assert(mem_read(registers.pc) == 0xcb);
	
	registers.pc++;
	u8 opcode = mem_read(registers.pc);
	
	switch (opcode) {
		case 0x10: execute_instruction_RL(&registers.b, 8); break;
		case 0x11: execute_instruction_RL(&registers.c, 8); break;
		case 0x12: execute_instruction_RL(&registers.d, 8); break;
		case 0x13: execute_instruction_RL(&registers.e, 8); break;
		case 0x14: execute_instruction_RL(&registers.h, 8); break;
		case 0x15: execute_instruction_RL(&registers.l, 8); break;
		case 0x16: {
			u8 hl_value = mem_read(registers.hl);
			execute_instruction_RL(&hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x17: execute_instruction_RL(&registers.a, 8); break;
		case 0x18: execute_instruction_RR(&registers.b, 8); break;
		case 0x19: execute_instruction_RR(&registers.c, 8); break;
		case 0x1a: execute_instruction_RR(&registers.d, 8); break;
		case 0x1b: execute_instruction_RR(&registers.e, 8); break;
		case 0x1c: execute_instruction_RR(&registers.h, 8); break;
		case 0x1d: execute_instruction_RR(&registers.l, 8); break;
		case 0x1f: execute_instruction_RR(&registers.a, 8); break;
		case 0x20: execute_instruction_SLA(&registers.b); break;
		case 0x21: execute_instruction_SLA(&registers.c); break;
		case 0x22: execute_instruction_SLA(&registers.d); break;
		case 0x23: execute_instruction_SLA(&registers.e); break;
		case 0x24: execute_instruction_SLA(&registers.h); break;
		case 0x25: execute_instruction_SLA(&registers.l); break;
		case 0x27: execute_instruction_SLA(&registers.a); break;
		case 0x30: execute_instruction_SWAP(&registers.b, 8); break;
		case 0x31: execute_instruction_SWAP(&registers.c, 8); break;
		case 0x32: execute_instruction_SWAP(&registers.d, 8); break;
		case 0x33: execute_instruction_SWAP(&registers.e, 8); break;
		case 0x34: execute_instruction_SWAP(&registers.h, 8); break;
		case 0x35: execute_instruction_SWAP(&registers.l, 8); break;
		case 0x36: {
			u8 hl_value = mem_read(registers.hl);
			execute_instruction_SWAP(&hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x37: execute_instruction_SWAP(&registers.a, 8); break;
		case 0x38: execute_instruction_SRL(&registers.b); break;
		case 0x39: execute_instruction_SRL(&registers.c); break;
		case 0x3a: execute_instruction_SRL(&registers.d); break;
		case 0x3b: execute_instruction_SRL(&registers.e); break;
		case 0x3c: execute_instruction_SRL(&registers.h); break;
		case 0x3d: execute_instruction_SRL(&registers.l); break;
		case 0x3f: execute_instruction_SRL(&registers.a); break;
		case 0x40: execute_instruction_BIT(0, registers.b, 8); break;
		case 0x41: execute_instruction_BIT(0, registers.c, 8); break;
		case 0x42: execute_instruction_BIT(0, registers.d, 8); break;
		case 0x43: execute_instruction_BIT(0, registers.e, 8); break;
		case 0x44: execute_instruction_BIT(0, registers.h, 8); break;
		case 0x45: execute_instruction_BIT(0, registers.l, 8); break;
		case 0x46: execute_instruction_BIT(0, mem_read(registers.hl), 16); break;
		case 0x47: execute_instruction_BIT(0, registers.a, 8); break;
		case 0x48: execute_instruction_BIT(1, registers.b, 8); break;
		case 0x49: execute_instruction_BIT(1, registers.c, 8); break;
		case 0x4a: execute_instruction_BIT(1, registers.d, 8); break;
		case 0x4b: execute_instruction_BIT(1, registers.e, 8); break;
		case 0x4c: execute_instruction_BIT(1, registers.h, 8); break;
		case 0x4d: execute_instruction_BIT(1, registers.l, 8); break;
		case 0x4e: execute_instruction_BIT(1, mem_read(registers.hl), 16); break;
		case 0x4f: execute_instruction_BIT(1, registers.a, 8); break;
		case 0x50: execute_instruction_BIT(2, registers.b, 8); break;
		case 0x51: execute_instruction_BIT(2, registers.c, 8); break;
		case 0x52: execute_instruction_BIT(2, registers.d, 8); break;
		case 0x53: execute_instruction_BIT(2, registers.e, 8); break;
		case 0x54: execute_instruction_BIT(2, registers.h, 8); break;
		case 0x55: execute_instruction_BIT(2, registers.l, 8); break;
		case 0x56: execute_instruction_BIT(2, mem_read(registers.hl), 16); break;
		case 0x57: execute_instruction_BIT(2, registers.a, 8); break;
		case 0x58: execute_instruction_BIT(3, registers.b, 8); break;
		case 0x59: execute_instruction_BIT(3, registers.c, 8); break;
		case 0x5a: execute_instruction_BIT(3, registers.d, 8); break;
		case 0x5b: execute_instruction_BIT(3, registers.e, 8); break;
		case 0x5c: execute_instruction_BIT(3, registers.h, 8); break;
		case 0x5d: execute_instruction_BIT(3, registers.l, 8); break;
		case 0x5f: execute_instruction_BIT(3, registers.a, 8); break;
		case 0x60: execute_instruction_BIT(4, registers.b, 8); break;
		case 0x61: execute_instruction_BIT(4, registers.c, 8); break;
		case 0x62: execute_instruction_BIT(4, registers.d, 8); break;
		case 0x63: execute_instruction_BIT(4, registers.e, 8); break;
		case 0x64: execute_instruction_BIT(4, registers.h, 8); break;
		case 0x65: execute_instruction_BIT(4, registers.l, 8); break;
		case 0x66: execute_instruction_BIT(4, mem_read(registers.hl), 16); break;
		case 0x67: execute_instruction_BIT(4, registers.a, 8); break;
		case 0x68: execute_instruction_BIT(5, registers.b, 8); break;
		case 0x69: execute_instruction_BIT(5, registers.c, 8); break;
		case 0x6a: execute_instruction_BIT(5, registers.d, 8); break;
		case 0x6b: execute_instruction_BIT(5, registers.e, 8); break;
		case 0x6c: execute_instruction_BIT(5, registers.h, 8); break;
		case 0x6d: execute_instruction_BIT(5, registers.l, 8); break;
		case 0x6f: execute_instruction_BIT(5, registers.a, 8); break;
		case 0x70: execute_instruction_BIT(6, registers.b, 8); break;
		case 0x71: execute_instruction_BIT(6, registers.c, 8); break;
		case 0x72: execute_instruction_BIT(6, registers.d, 8); break;
		case 0x73: execute_instruction_BIT(6, registers.e, 8); break;
		case 0x74: execute_instruction_BIT(6, registers.h, 8); break;
		case 0x75: execute_instruction_BIT(6, registers.l, 8); break;
		case 0x76: execute_instruction_BIT(6, mem_read(registers.hl), 16); break;
		case 0x77: execute_instruction_BIT(6, registers.a, 8); break;
		case 0x78: execute_instruction_BIT(7, registers.b, 8); break;
		case 0x79: execute_instruction_BIT(7, registers.c, 8); break;
		case 0x7a: execute_instruction_BIT(7, registers.d, 8); break;
		case 0x7b: execute_instruction_BIT(7, registers.e, 8); break;
		case 0x7c: execute_instruction_BIT(7, registers.h, 8); break;
		case 0x7d: execute_instruction_BIT(7, registers.l, 8); break;
		case 0x7e: execute_instruction_BIT(7, mem_read(registers.hl), 16); break;
		case 0x7f: execute_instruction_BIT(7, registers.a, 8); break;
		case 0x80: execute_instruction_RES(0, &registers.b, 8); break;
		case 0x81: execute_instruction_RES(0, &registers.c, 8); break;
		case 0x82: execute_instruction_RES(0, &registers.d, 8); break;
		case 0x83: execute_instruction_RES(0, &registers.e, 8); break;
		case 0x84: execute_instruction_RES(0, &registers.h, 8); break;
		case 0x85: execute_instruction_RES(0, &registers.l, 8); break;
		case 0x86: {
			u8 hl_value = mem_read(registers.hl);
			execute_instruction_RES(0, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0x87: execute_instruction_RES(0, &registers.a, 8); break;
		case 0x88: execute_instruction_RES(1, &registers.b, 8); break;
		case 0x89: execute_instruction_RES(1, &registers.c, 8); break;
		case 0x8a: execute_instruction_RES(1, &registers.d, 8); break;
		case 0x8b: execute_instruction_RES(1, &registers.e, 8); break;
		case 0x8c: execute_instruction_RES(1, &registers.h, 8); break;
		case 0x8d: execute_instruction_RES(1, &registers.l, 8); break;
		case 0x8f: execute_instruction_RES(1, &registers.a, 8); break;
		case 0x90: execute_instruction_RES(2, &registers.b, 8); break;
		case 0x91: execute_instruction_RES(2, &registers.c, 8); break;
		case 0x92: execute_instruction_RES(2, &registers.d, 8); break;
		case 0x93: execute_instruction_RES(2, &registers.e, 8); break;
		case 0x94: execute_instruction_RES(2, &registers.h, 8); break;
		case 0x95: execute_instruction_RES(2, &registers.l, 8); break;
		case 0x97: execute_instruction_RES(2, &registers.a, 8); break;
		case 0x98: execute_instruction_RES(3, &registers.b, 8); break;
		case 0x99: execute_instruction_RES(3, &registers.c, 8); break;
		case 0x9a: execute_instruction_RES(3, &registers.d, 8); break;
		case 0x9b: execute_instruction_RES(3, &registers.e, 8); break;
		case 0x9c: execute_instruction_RES(3, &registers.h, 8); break;
		case 0x9d: execute_instruction_RES(3, &registers.l, 8); break;
		case 0x9f: execute_instruction_RES(3, &registers.a, 8); break;
		case 0xa0: execute_instruction_RES(4, &registers.b, 8); break;
		case 0xa1: execute_instruction_RES(4, &registers.c, 8); break;
		case 0xa2: execute_instruction_RES(4, &registers.d, 8); break;
		case 0xa3: execute_instruction_RES(4, &registers.e, 8); break;
		case 0xa4: execute_instruction_RES(4, &registers.h, 8); break;
		case 0xa5: execute_instruction_RES(4, &registers.l, 8); break;
		case 0xa7: execute_instruction_RES(4, &registers.a, 8); break;
		case 0xa8: execute_instruction_RES(5, &registers.b, 8); break;
		case 0xa9: execute_instruction_RES(5, &registers.c, 8); break;
		case 0xaa: execute_instruction_RES(5, &registers.d, 8); break;
		case 0xab: execute_instruction_RES(5, &registers.e, 8); break;
		case 0xac: execute_instruction_RES(5, &registers.h, 8); break;
		case 0xad: execute_instruction_RES(5, &registers.l, 8); break;
		case 0xaf: execute_instruction_RES(5, &registers.a, 8); break;
		case 0xb0: execute_instruction_RES(6, &registers.b, 8); break;
		case 0xb1: execute_instruction_RES(6, &registers.c, 8); break;
		case 0xb2: execute_instruction_RES(6, &registers.d, 8); break;
		case 0xb3: execute_instruction_RES(6, &registers.e, 8); break;
		case 0xb4: execute_instruction_RES(6, &registers.h, 8); break;
		case 0xb5: execute_instruction_RES(6, &registers.l, 8); break;
		case 0xb7: execute_instruction_RES(6, &registers.a, 8); break;
		case 0xb8: execute_instruction_RES(7, &registers.b, 8); break;
		case 0xb9: execute_instruction_RES(7, &registers.c, 8); break;
		case 0xba: execute_instruction_RES(7, &registers.d, 8); break;
		case 0xbb: execute_instruction_RES(7, &registers.e, 8); break;
		case 0xbc: execute_instruction_RES(7, &registers.h, 8); break;
		case 0xbd: execute_instruction_RES(7, &registers.l, 8); break;
		case 0xbe: {
			u8 hl_value = mem_read(registers.hl);
			execute_instruction_RES(7, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xbf: execute_instruction_RES(7, &registers.a, 8); break;
		case 0xc0: execute_instruction_SET(0, &registers.b, 8); break;
		case 0xc1: execute_instruction_SET(0, &registers.c, 8); break;
		case 0xc2: execute_instruction_SET(0, &registers.d, 8); break;
		case 0xc3: execute_instruction_SET(0, &registers.e, 8); break;
		case 0xc4: execute_instruction_SET(0, &registers.h, 8); break;
		case 0xc5: execute_instruction_SET(0, &registers.l, 8); break;
		case 0xc6: {
			u8 hl_value = mem_read(registers.hl);
			execute_instruction_SET(0, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xc7: execute_instruction_SET(0, &registers.a, 8); break;
		case 0xc8: execute_instruction_SET(1, &registers.b, 8); break;
		case 0xc9: execute_instruction_SET(1, &registers.c, 8); break;
		case 0xca: execute_instruction_SET(1, &registers.d, 8); break;
		case 0xcb: execute_instruction_SET(1, &registers.e, 8); break;
		case 0xcc: execute_instruction_SET(1, &registers.h, 8); break;
		case 0xcd: execute_instruction_SET(1, &registers.l, 8); break;
		case 0xcf: execute_instruction_SET(1, &registers.a, 8); break;
		case 0xd0: execute_instruction_SET(2, &registers.b, 8); break;
		case 0xd1: execute_instruction_SET(2, &registers.c, 8); break;
		case 0xd2: execute_instruction_SET(2, &registers.d, 8); break;
		case 0xd3: execute_instruction_SET(2, &registers.e, 8); break;
		case 0xd4: execute_instruction_SET(2, &registers.h, 8); break;
		case 0xd5: execute_instruction_SET(2, &registers.l, 8); break;
		case 0xd7: execute_instruction_SET(2, &registers.a, 8); break;
		case 0xd8: execute_instruction_SET(3, &registers.b, 8); break;
		case 0xd9: execute_instruction_SET(3, &registers.c, 8); break;
		case 0xda: execute_instruction_SET(3, &registers.d, 8); break;
		case 0xdb: execute_instruction_SET(3, &registers.e, 8); break;
		case 0xdc: execute_instruction_SET(3, &registers.h, 8); break;
		case 0xdd: execute_instruction_SET(3, &registers.l, 8); break;
		case 0xdf: execute_instruction_SET(3, &registers.a, 8); break;
		case 0xe0: execute_instruction_SET(4, &registers.b, 8); break;
		case 0xe1: execute_instruction_SET(4, &registers.c, 8); break;
		case 0xe2: execute_instruction_SET(4, &registers.d, 8); break;
		case 0xe3: execute_instruction_SET(4, &registers.e, 8); break;
		case 0xe4: execute_instruction_SET(4, &registers.h, 8); break;
		case 0xe5: execute_instruction_SET(4, &registers.l, 8); break;
		case 0xe7: execute_instruction_SET(4, &registers.a, 8); break;
		case 0xe8: execute_instruction_SET(5, &registers.b, 8); break;
		case 0xe9: execute_instruction_SET(5, &registers.c, 8); break;
		case 0xea: execute_instruction_SET(5, &registers.d, 8); break;
		case 0xeb: execute_instruction_SET(5, &registers.e, 8); break;
		case 0xec: execute_instruction_SET(5, &registers.h, 8); break;
		case 0xed: execute_instruction_SET(5, &registers.l, 8); break;
		case 0xef: execute_instruction_SET(5, &registers.a, 8); break;
		case 0xf0: execute_instruction_SET(6, &registers.b, 8); break;
		case 0xf1: execute_instruction_SET(6, &registers.c, 8); break;
		case 0xf2: execute_instruction_SET(6, &registers.d, 8); break;
		case 0xf3: execute_instruction_SET(6, &registers.e, 8); break;
		case 0xf4: execute_instruction_SET(6, &registers.h, 8); break;
		case 0xf5: execute_instruction_SET(6, &registers.l, 8); break;
		case 0xf7: execute_instruction_SET(6, &registers.a, 8); break;
		case 0xf8: execute_instruction_SET(7, &registers.b, 8); break;
		case 0xf9: execute_instruction_SET(7, &registers.c, 8); break;
		case 0xfa: execute_instruction_SET(7, &registers.d, 8); break;
		case 0xfb: execute_instruction_SET(7, &registers.e, 8); break;
		case 0xfc: execute_instruction_SET(7, &registers.h, 8); break;
		case 0xfd: execute_instruction_SET(7, &registers.l, 8); break;
		case 0xfe: {
			u8 hl_value = mem_read(registers.hl);
			execute_instruction_SET(7, &hl_value, 16);
			mem_write(registers.hl, hl_value);
		} break;
		case 0xff: execute_instruction_SET(7, &registers.a, 8); break;
		default: {
			char buf[128] = {0};
			sprintf(buf, "Unknown opcode cb%x at address %x\n", opcode, registers.pc-1);
			robingb_log(buf);
			assert(false);
		} break;
	}
}





