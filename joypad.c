#include "internal.h"

#include <stdio.h>

/* TODO: What should happen to bits 4 and 5 of the joypad register after the update? */

#define ACTION_BUTTON_REQUEST 0x20
#define DIRECTION_BUTTON_REQUEST 0x10
#define DOWN_OR_START 0x08
#define UP_OR_SELECT 0x04
#define LEFT_OR_B 0x02
#define RIGHT_OR_A 0x01

static u8 action_buttons = 0xff;
static u8 direction_buttons = 0xff;

void robingb_press_button(RobinGB_Button button) {
	switch (button) {
		case ROBINGB_UP: direction_buttons &= ~UP_OR_SELECT; break;
		case ROBINGB_LEFT: direction_buttons &= ~LEFT_OR_B; break;
		case ROBINGB_RIGHT: direction_buttons &= ~RIGHT_OR_A; break;
		case ROBINGB_DOWN: direction_buttons &= ~DOWN_OR_START; break;
		case ROBINGB_A: action_buttons &= ~RIGHT_OR_A; break;
		case ROBINGB_B: action_buttons &= ~LEFT_OR_B; break;
		case ROBINGB_START: action_buttons &= ~DOWN_OR_START; break;
		case ROBINGB_SELECT: action_buttons &= ~UP_OR_SELECT; break;
		default: printf("Invalid joypad button\n"); break;
	}
}

void robingb_release_button(RobinGB_Button button) {
	switch (button) {
		case ROBINGB_UP: direction_buttons |= UP_OR_SELECT; break;
		case ROBINGB_LEFT: direction_buttons |= LEFT_OR_B; break;
		case ROBINGB_RIGHT: direction_buttons |= RIGHT_OR_A; break;
		case ROBINGB_DOWN: direction_buttons |= DOWN_OR_START; break;
		case ROBINGB_A: action_buttons |= RIGHT_OR_A; break;
		case ROBINGB_B: action_buttons |= LEFT_OR_B; break;
		case ROBINGB_START: action_buttons |= DOWN_OR_START; break;
		case ROBINGB_SELECT: action_buttons |= UP_OR_SELECT; break;
		default: printf("Invalid joypad button\n"); break;
	}
}

u8 robingb_respond_to_joypad_register(u8 register_value) {
	register_value |= 0xc0; /* bits 6 and 7 are always 1. */
	register_value |= 0x0f; /* unpressed buttons are 1. */
	
	if ((register_value & ACTION_BUTTON_REQUEST) == false) {
		register_value &= action_buttons;
		robingb_request_interrupt(INTERRUPT_FLAG_JOYPAD);
	}
	
	if ((register_value & DIRECTION_BUTTON_REQUEST) == false) {
		register_value &= direction_buttons;
		robingb_request_interrupt(INTERRUPT_FLAG_JOYPAD);
	}
	
	return register_value;
}




