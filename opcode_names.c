#include "internal.h"
#include <assert.h>

static const char * get_cb_opcode_name(u16 opcode_address) {
	assert(mem_read(registers.pc) == 0xcb);

	u8 opcode = mem_read(registers.pc+1);

	switch (opcode) {
		case 0x00: return "RLC B";
		case 0x01: return "RLC C";
		case 0x02: return "RLC D";
		case 0x03: return "RLC E";
		case 0x04: return "RLC H";
		case 0x05: return "RLC L";
		case 0x06: return "RLC (HL)";
		case 0x07: return "RLC A";
		case 0x08: return "RRC B";
		case 0x09: return "RRC C";
		case 0x0a: return "RRC D";
		case 0x0b: return "RRC E";
		case 0x0c: return "RRC H";
		case 0x0d: return "RRC L";
		case 0x0e: return "RRC (HL)";
		case 0x0f: return "RRC A";
		case 0x10: return "RR B";
		case 0x11: return "RR C";
		case 0x12: return "RR D";
		case 0x13: return "RR E";
		case 0x14: return "RR H";
		case 0x15: return "RR L";
		case 0x16: return "RR (HL)";
		case 0x17: return "RR A";
		case 0x18: return "RR B";
		case 0x19: return "RR C";
		case 0x1a: return "RR D";
		case 0x1b: return "RR E";
		case 0x1c: return "RR H";
		case 0x1d: return "RR L";
		case 0x1f: return "RR A";
		case 0x20: return "SLA B";
		case 0x21: return "SLA C";
		case 0x22: return "SLA D";
		case 0x23: return "SLA E";
		case 0x24: return "SLA H";
		case 0x25: return "SLA L";
		case 0x27: return "SLA A";
		case 0x30: return "SWAP B";
		case 0x31: return "SWAP C";
		case 0x32: return "SWAP D";
		case 0x33: return "SWAP E";
		case 0x34: return "SWAP H";
		case 0x35: return "SWAP L";
		case 0x36: return "SWAP (HL)";
		case 0x37: return "SWAP A";
		case 0x38: return "SRL B";
		case 0x39: return "SRL C";
		case 0x3a: return "SRL D";
		case 0x3b: return "SRL E";
		case 0x3c: return "SRL H";
		case 0x3d: return "SRL L";
		case 0x3f: return "SRL A";
		case 0x40: return "BIT 0,B";
		case 0x41: return "BIT 0,C";
		case 0x42: return "BIT 0,D";
		case 0x43: return "BIT 0,E";
		case 0x44: return "BIT 0,H";
		case 0x45: return "BIT 0,L";
		case 0x46: return "BIT 0,(HL)";
		case 0x47: return "BIT 0,A";
		case 0x48: return "BIT 1,B";
		case 0x49: return "BIT 1,C";
		case 0x4a: return "BIT 1,D";
		case 0x4b: return "BIT 1,E";
		case 0x4c: return "BIT 1,H";
		case 0x4d: return "BIT 1,L";
		case 0x4e: return "BIT 1,(HL)";
		case 0x4f: return "BIT 1,A";
		case 0x50: return "BIT 2,B";
		case 0x51: return "BIT 2,C";
		case 0x52: return "BIT 2,D";
		case 0x53: return "BIT 2,E";
		case 0x54: return "BIT 2,H";
		case 0x55: return "BIT 2,L";
		case 0x56: return "BIT 2,(HL)";
		case 0x57: return "BIT 2,A";
		case 0x58: return "BIT 3,B";
		case 0x59: return "BIT 3,C";
		case 0x5a: return "BIT 3,D";
		case 0x5b: return "BIT 3,E";
		case 0x5c: return "BIT 3,H";
		case 0x5d: return "BIT 3,L";
		case 0x5f: return "BIT 3,A";
		case 0x60: return "BIT 4,B";
		case 0x61: return "BIT 4,C";
		case 0x62: return "BIT 4,D";
		case 0x63: return "BIT 4,E";
		case 0x64: return "BIT 4,H";
		case 0x65: return "BIT 4,L";
		case 0x66: return "BIT 4,(HL)";
		case 0x67: return "BIT 4,A";
		case 0x68: return "BIT 5,B";
		case 0x69: return "BIT 5,C";
		case 0x6a: return "BIT 5,D";
		case 0x6b: return "BIT 5,E";
		case 0x6c: return "BIT 5,H";
		case 0x6d: return "BIT 5,L";
		case 0x6f: return "BIT 5,A";
		case 0x70: return "BIT 6,B";
		case 0x71: return "BIT 6,C";
		case 0x72: return "BIT 6,D";
		case 0x73: return "BIT 6,E";
		case 0x74: return "BIT 6,H";
		case 0x75: return "BIT 6,L";
		case 0x76: return "BIT 6,(HL)";
		case 0x77: return "BIT 6,A";
		case 0x78: return "BIT 7,B";
		case 0x79: return "BIT 7,C";
		case 0x7a: return "BIT 7,D";
		case 0x7b: return "BIT 7,E";
		case 0x7c: return "BIT 7,H";
		case 0x7d: return "BIT 7,L";
		case 0x7e: return "BIT 7,(HL)";
		case 0x7f: return "BIT 7,A";
		case 0x80: return "RES 0,B";
		case 0x81: return "RES 0,C";
		case 0x82: return "RES 0,D";
		case 0x83: return "RES 0,E";
		case 0x84: return "RES 0,H";
		case 0x85: return "RES 0,L";
		case 0x86: return "RES 0,(HL)";
		case 0x87: return "RES 0,A";
		case 0x88: return "RES 1,B";
		case 0x89: return "RES 1,C";
		case 0x8a: return "RES 1,D";
		case 0x8b: return "RES 1,E";
		case 0x8c: return "RES 1,H";
		case 0x8d: return "RES 1,L";
		case 0x8f: return "RES 1,A";
		case 0x90: return "RES 2,B";
		case 0x91: return "RES 2,C";
		case 0x92: return "RES 2,D";
		case 0x93: return "RES 2,E";
		case 0x94: return "RES 2,H";
		case 0x95: return "RES 2,L";
		case 0x97: return "RES 2,A";
		case 0x98: return "RES 3,B";
		case 0x99: return "RES 3,C";
		case 0x9a: return "RES 3,D";
		case 0x9b: return "RES 3,E";
		case 0x9c: return "RES 3,H";
		case 0x9d: return "RES 3,L";
		case 0x9f: return "RES 3,A";
		case 0xa0: return "RES 4,B";
		case 0xa1: return "RES 4,C";
		case 0xa2: return "RES 4,D";
		case 0xa3: return "RES 4,E";
		case 0xa4: return "RES 4,H";
		case 0xa5: return "RES 4,L";
		case 0xa7: return "RES 4,A";
		case 0xa8: return "RES 5,B";
		case 0xa9: return "RES 5,C";
		case 0xaa: return "RES 5,D";
		case 0xab: return "RES 5,E";
		case 0xac: return "RES 5,H";
		case 0xad: return "RES 5,L";
		case 0xaf: return "RES 5,A";
		case 0xb0: return "RES 6,B";
		case 0xb1: return "RES 6,C";
		case 0xb2: return "RES 6,D";
		case 0xb3: return "RES 6,E";
		case 0xb4: return "RES 6,H";
		case 0xb5: return "RES 6,L";
		case 0xb7: return "RES 6,A";
		case 0xb8: return "RES 7,B";
		case 0xb9: return "RES 7,C";
		case 0xba: return "RES 7,D";
		case 0xbb: return "RES 7,E";
		case 0xbc: return "RES 7,H";
		case 0xbd: return "RES 7,L";
		case 0xbe: return "RES 7,(HL)";
		case 0xbf: return "RES 7,A";
		case 0xc0: return "SET 0,B";
		case 0xc1: return "SET 0,C";
		case 0xc2: return "SET 0,D";
		case 0xc3: return "SET 0,E";
		case 0xc4: return "SET 0,H";
		case 0xc5: return "SET 0,L";
		case 0xc6: return "SET 0,(HL)";
		case 0xc7: return "SET 0,A";
		case 0xc8: return "SET 1,B";
		case 0xc9: return "SET 1,C";
		case 0xca: return "SET 1,D";
		case 0xcb: return "SET 1,E";
		case 0xcc: return "SET 1,H";
		case 0xcd: return "SET 1,L";
		case 0xcf: return "SET 1,A";
		case 0xd0: return "SET 2,B";
		case 0xd1: return "SET 2,C";
		case 0xd2: return "SET 2,D";
		case 0xd3: return "SET 2,E";
		case 0xd4: return "SET 2,H";
		case 0xd5: return "SET 2,L";
		case 0xd7: return "SET 2,A";
		case 0xd8: return "SET 3,B";
		case 0xd9: return "SET 3,C";
		case 0xda: return "SET 3,D";
		case 0xdb: return "SET 3,E";
		case 0xdc: return "SET 3,H";
		case 0xdd: return "SET 3,L";
		case 0xdf: return "SET 3,A";
		case 0xe0: return "SET 4,B";
		case 0xe1: return "SET 4,C";
		case 0xe2: return "SET 4,D";
		case 0xe3: return "SET 4,E";
		case 0xe4: return "SET 4,H";
		case 0xe5: return "SET 4,L";
		case 0xe7: return "SET 4,A";
		case 0xe8: return "SET 5,B";
		case 0xe9: return "SET 5,C";
		case 0xea: return "SET 5,D";
		case 0xeb: return "SET 5,E";
		case 0xec: return "SET 5,H";
		case 0xed: return "SET 5,L";
		case 0xef: return "SET 5,A";
		case 0xf0: return "SET 6,B";
		case 0xf1: return "SET 6,C";
		case 0xf2: return "SET 6,D";
		case 0xf3: return "SET 6,E";
		case 0xf4: return "SET 6,H";
		case 0xf5: return "SET 6,L";
		case 0xf7: return "SET 6,A";
		case 0xf8: return "SET 7,B";
		case 0xf9: return "SET 7,C";
		case 0xfa: return "SET 7,D";
		case 0xfb: return "SET 7,E";
		case 0xfc: return "SET 7,H";
		case 0xfd: return "SET 7,L";
		case 0xfe: return "SET 7,(HL)";
		case 0xff: return "SET 7,A";
		default: assert(false); break; // no name
	}
}

const char * get_opcode_name(u16 opcode_address) {
	u8 opcode = mem_read(opcode_address);
	
	switch (opcode) {
		case 0x00: return "NOP";
		case 0x01: return "LD BC,xx";
		case 0x02: return "LD (BC),A";
		case 0x03: return "INC BC";
		case 0x04: return "INC b";
		case 0x05: return "DEC B";
		case 0x06: return "LD B,x";
		case 0x07: return "RLCA";
		case 0x08: return "LD (xx),SP";
		case 0x09: return "ADD HL,BC";
		case 0x0a: return "LD A,(BC)";
		case 0x0b: return "DEC BC";
		case 0x0c: return "INC C";
		case 0x0d: return "DEC C";
		case 0x0e: return "LD C,x";
		case 0x0f: return "RRCA";
		case 0x11: return "LD DE,xx";
		case 0x12: return "LD (DE),A";
		case 0x13: return "INC DE";
		case 0x14: return "INC D";
		case 0x15: return "DEC D";
		case 0x16: return "LD D,x";
		case 0x17: return "RLA";
		case 0x18: return "JR %i(d)";
		case 0x19: return "ADD HL,DE";
		case 0x1a: return "LD A,(DE)";
		case 0x1b: return "DEC DE";
		case 0x1c: return "INC E";
		case 0x1d: return "DEC E";
		case 0x1e: return "LD E,x";
		case 0x1f: return "RRA";
		case 0x20: return "JR NZ,s";
		case 0x21: return "LD HL,xx";
		case 0x22: return "LD (HL+),A";
		case 0x23: return "INC HL";
		case 0x24: return "INC H";
		case 0x25: return "DEC H";
		case 0x26: return "LD H,x";
		case 0x27: return "DAA";
		case 0x28: return "JR Z,s";
		case 0x29: return "ADD HL,HL";
		case 0x2a: return "LD A,(HL+)";
		case 0x2b: return "DEC HL";
		case 0x2c: return "INC L";
		case 0x2d: return "DEC L";
		case 0x2e: return "LD L,x";
		case 0x2f: return "CPL";
		case 0x30: return "JR NC,s";
		case 0x31: return "LD SP,xx";
		case 0x32: return "LD (HL-),A";
		case 0x33: return "INC SP";
		case 0x34: return "INC (HL)";
		case 0x35: return "DEC (HL)";
		case 0x36: return "LD (HL),x";
		case 0x37: return "SCF";
		case 0x38: return "JR C,s";
		case 0x39: return "ADD HL,SP";
		case 0x3a: return "LD A,(HL-)";
		case 0x3b: return "DEC SP";
		case 0x3c: return "INC A";
		case 0x3d: return "DEC A";
		case 0x3e: return "LD A,x";
		case 0x3f: return "CCF";
		case 0x40: return "LD B,B";
		case 0x41: return "LD B,C";
		case 0x42: return "LD B,D";
		case 0x43: return "LD B,E";
		case 0x44: return "LD B,H";
		case 0x45: return "LD B,L";
		case 0x46: return "LD B,(HL)";
		case 0x47: return "LD B,A";
		case 0x48: return "LD C,B";
		case 0x49: return "LD C,C";
		case 0x4a: return "LD C,D";
		case 0x4b: return "LD C,E";
		case 0x4c: return "LD C,H";
		case 0x4d: return "LD C,L";
		case 0x4e: return "LD C,(HL)";
		case 0x4f: return "LD C,A";
		case 0x50: return "LD D,B";
		case 0x51: return "LD D,C";
		case 0x52: return "LD D,D";
		case 0x53: return "LD D,E";
		case 0x54: return "LD D,H";
		case 0x55: return "LD D,L";
		case 0x56: return "LD D,(HL)";
		case 0x57: return "LD D,A";
		case 0x58: return "LD E,B";
		case 0x59: return "LD E,C";
		case 0x5a: return "LD E,D";
		case 0x5b: return "LD E,E";
		case 0x5c: return "LD E,H";
		case 0x5d: return "LD E,L";
		case 0x5e: return "LD E,(HL)";
		case 0x5f: return "LD E,A";
		case 0x60: return "LD H,B";
		case 0x61: return "LD H,C";
		case 0x62: return "LD H,D";
		case 0x63: return "LD H,E";
		case 0x64: return "LD H,H";
		case 0x65: return "LD H,L";
		case 0x66: return "LD H,(HL)";
		case 0x67: return "LD H,A";
		case 0x68: return "LD L,B";
		case 0x69: return "LD L,C";
		case 0x6a: return "LD L,D";
		case 0x6b: return "LD L,E";
		case 0x6c: return "LD L,H";
		case 0x6d: return "LD L,L";
		case 0x6e: return "LD L,(HL)";
		case 0x6f: return "LD L,A";
		case 0x70: return "LD (HL),B";
		case 0x71: return "LD (HL),C";
		case 0x72: return "LD (HL),D";
		case 0x73: return "LD (HL),E";
		case 0x74: return "LD (HL),H";
		case 0x75: return "LD (HL),L";
		case 0x76: return "HALT";
		case 0x77: return "LD (HL),A";
		case 0x78: return "LD A,B";
		case 0x79: return "LD A,C";
		case 0x7a: return "LD A,D";
		case 0x7b: return "LD A,E";
		case 0x7c: return "LD A,H";
		case 0x7d: return "LD A,L";
		case 0x7e: return "LD A,(HL)";
		case 0x7f: return "LD A,A";
		case 0x80: return "ADD A,B";
		case 0x81: return "ADD A,C";
		case 0x82: return "ADD A,D";
		case 0x83: return "ADD A,E";
		case 0x84: return "ADD A,H";
		case 0x85: return "ADD A,L";
		case 0x86: return "ADD A,(HL)";
		case 0x87: return "ADD A,A";
		case 0x88: return "ADC A,B";
		case 0x89: return "ADC A,C";
		case 0x8a: return "ADC A,D";
		case 0x8b: return "ADC A,E";
		case 0x8c: return "ADC A,H";
		case 0x8d: return "ADC A,L";
		case 0x8e: return "ADC A,(HL)";
		case 0x8f: return "ADC A,A";
		case 0x90: return "SUB B";
		case 0x91: return "SUB C";
		case 0x92: return "SUB D";
		case 0x93: return "SUB E";
		case 0x94: return "SUB H";
		case 0x95: return "SUB L";
		case 0x96: return "SUB (HL)";
		case 0x97: return "SUB A";
		case 0x98: return "SBC A,B";	
		case 0x99: return "SBC A,C";
		case 0x9a: return "SBC A,D";
		case 0x9b: return "SBC A,E";
		case 0x9c: return "SBC A,H";
		case 0x9d: return "SBC A,L";
		case 0x9e: return "SBC A,(HL)";
		case 0x9f: return "SBC A,A";
		case 0xa0: return "AND B";
		case 0xa1: return "AND C";
		case 0xa2: return "AND D";
		case 0xa3: return "AND E";
		case 0xa4: return "AND H";
		case 0xa5: return "AND L";
		case 0xa6: return "AND (HL)";
		case 0xa7: return "AND A";
		case 0xa8: return "XOR B";
		case 0xa9: return "XOR C";
		case 0xad: return "XOR L";
		case 0xae: return "XOR (HL)";
		case 0xaf: return "XOR A";
		case 0xb0: return "OR B";
		case 0xb1: return "OR C";
		case 0xb2: return "OR D";
		case 0xb3: return "OR E";
		case 0xb4: return "OR H";
		case 0xb5: return "OR L";
		case 0xb6: return "OR (HL)";
		case 0xb7: return "OR A";
		case 0xb8: return "CP B";
		case 0xb9: return "CP C";
		case 0xba: return "CP D";
		case 0xbb: return "CP E";
		case 0xbc: return "CP H";
		case 0xbd: return "CP L";
		case 0xbe: return "CP (HL)";
		case 0xc0: return "RET NZ";
		case 0xc1: return "POP BC";
		case 0xc2: return "JP NZ,xx";
		case 0xc3: return "JP xx";
		case 0xc4: return "CALL NZ,xx";
		case 0xc5: return "PUSH BC";
		case 0xc6: return "ADD A,x";
		case 0xc7: return "RST 00h";
		case 0xc8: return "RET Z";
		case 0xc9: return "RET";
		case 0xca: return "JP Z,xx";
		case 0xcb: return get_cb_opcode_name(opcode_address);
		case 0xcc: return "CALL Z,xx";
		case 0xcd: return "CALL xx";
		case 0xce: return "ADC A,x";
		case 0xcf: return "RST 08h";
		case 0xd0: return "RET NC";
		case 0xd1: return "POP DE";
		case 0xd2: return "JP NC,xx";
		case 0xd4: return "CALL NC,xx";
		case 0xd5: return "PUSH DE";
		case 0xd6: return "SUB x";
		case 0xd7: return "RST 10h";
		case 0xd8: return "RET C";
		case 0xd9: return "RETI";
		case 0xda: return "JP C,xx";
		case 0xde: return "SBC A,x";
		case 0xe0: return "LDH (ff00+x),A";
		case 0xe1: return "POP HL";
		case 0xe2: return "LD (ff00+C),A";
		case 0xe5: return "PUSH HL";
		case 0xe6: return "AND x";
		case 0xe9: return "JP (HL)";
		case 0xea: return "LD (x),A";
		case 0xee: return "XOR x";
		case 0xef: return "RST 28H";
		case 0xf0: return "LDH A,(ff00+x)";
		case 0xf1: return "POP AF";
		case 0xf2: return "LD A,(ff00+C)";
		case 0xf3: return "DI";
		case 0xf5: return "PUSH AF";
		case 0xf6: return "OR x";
		case 0xf7: return "RST 30h";
		case 0xf8: return "LDHL SP,s";
		case 0xf9: return "LD SP,HL";
		case 0xfa: return "LD A,(xx)";
		case 0xfb: return "IE";
		case 0xfe: return "CP x";
		case 0xff: return "RST 38H";
		default: assert(false); break; // no name
	}
}




