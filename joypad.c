#include "internal.h"

static u8 *joypad_register = &robingb_memory[0xff00];

void joypad_update(RobinGB_Input *input) {
	const u8 OTHER_BUTTON_REQUEST = 0x20;
	const u8 DIRECTION_BUTTON_REQUEST = 0x10;
	const u8 DOWN_OR_START = 0x08;
	const u8 UP_OR_SELECT = 0x04;
	const u8 LEFT_OR_B = 0x02;
	const u8 RIGHT_OR_A = 0x01;
	
	*joypad_register |= 0xc0; /* bits 6 and 7 are always 1. */
	*joypad_register |= 0x0f; /* unpressed buttons are 1. */
	
	if (((*joypad_register) & OTHER_BUTTON_REQUEST) == false) {
		if (input->start) {
			*joypad_register &= ~DOWN_OR_START;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
		if (input->select) {
			*joypad_register &= ~UP_OR_SELECT;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
		if (input->b) {
			*joypad_register &= ~LEFT_OR_B;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
		if (input->a) {
			*joypad_register &= ~RIGHT_OR_A;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
	}
	
	if (((*joypad_register) & DIRECTION_BUTTON_REQUEST) == false) {
		if (input->down) {
			*joypad_register &= ~DOWN_OR_START;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
		if (input->up) {
			*joypad_register &= ~UP_OR_SELECT;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
		if (input->left) {
			*joypad_register &= ~LEFT_OR_B;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
		if (input->right) {
			*joypad_register &= ~RIGHT_OR_A;
			request_interrupt(INTERRUPT_FLAG_JOYPAD);
		}
	}
}




