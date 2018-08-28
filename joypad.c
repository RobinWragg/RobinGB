#include "internal.h"

void joypad_update(RobinGB_Input *input) {
	const int joypad_address = 0xff00;
	
	const u8 OTHER_BUTTON_REQUEST = 0x20;
	const u8 DIRECTION_BUTTON_REQUEST = 0x10;
	const u8 DOWN_OR_START = 0x08;
	const u8 UP_OR_SELECT = 0x04;
	const u8 LEFT_OR_B = 0x02;
	const u8 RIGHT_OR_A = 0x01;
	
	u8 bits = mem_read(joypad_address);
	bits |= 0xc0; /* bits 6 and 7 are always 1. */
	bits |= 0x0f; /* unpressed buttons are 1. */
	
	if ((bits & OTHER_BUTTON_REQUEST) == false) {
		if (input->start) {
			bits &= ~DOWN_OR_START;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
		if (input->select) {
			bits &= ~UP_OR_SELECT;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
		if (input->b) {
			bits &= ~LEFT_OR_B;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
		if (input->a) {
			bits &= ~RIGHT_OR_A;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
	}
	
	if ((bits & DIRECTION_BUTTON_REQUEST) == false) {
		if (input->down) {
			bits &= ~DOWN_OR_START;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
		if (input->up) {
			bits &= ~UP_OR_SELECT;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
		if (input->left) {
			bits &= ~LEFT_OR_B;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
		if (input->right) {
			bits &= ~RIGHT_OR_A;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
	}
	
	mem_write(joypad_address, bits);
}




