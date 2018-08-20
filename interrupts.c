#include "internal.h"
#include <assert.h>

void handle_interrupts() {
	if (registers.ime) {
		u8 *requested_interrupts = mem_get_pointer(IF_ADDRESS);
		u8 *enabled_interrupts = mem_get_pointer(IE_ADDRESS);
		
		u8 interrupts_to_handle = (*requested_interrupts) & (*enabled_interrupts);
		
		if (interrupts_to_handle) {
			registers.ime = false;
			
			if (halted) {
				registers.pc++; // Advance past the HALT instruction
				halted = false;
			}
			
			stack_push(registers.pc);
			
			if (interrupts_to_handle & INTERRUPT_FLAG_VBLANK) {
				*requested_interrupts &= ~INTERRUPT_FLAG_VBLANK;
				registers.pc = 0x0040;
			} else if (interrupts_to_handle & INTERRUPT_FLAG_LCD_STAT) {
				*requested_interrupts &= ~INTERRUPT_FLAG_LCD_STAT;
				registers.pc = 0x0048;
			} else if (interrupts_to_handle & INTERRUPT_FLAG_TIMER) {
				*requested_interrupts &= ~INTERRUPT_FLAG_TIMER;
				registers.pc = 0x0050;
			} else if (interrupts_to_handle & INTERRUPT_FLAG_SERIAL) {
				*requested_interrupts &= ~INTERRUPT_FLAG_SERIAL;
				registers.pc = 0x0058;
			} else if (interrupts_to_handle & INTERRUPT_FLAG_JOYPAD) {
				*requested_interrupts &= ~INTERRUPT_FLAG_JOYPAD;
				registers.pc = 0x0060;
			} else assert(false); // unexpected interrupts_to_handle value.
		}
	}
}

void request_interrupt(u8 interrupt_to_request) {
	interrupt_to_request |= mem_read(IF_ADDRESS); // combine with the existing request flags.
	interrupt_to_request |= 0xe0; // top 3 bits are always 1.
	mem_write(IF_ADDRESS, interrupt_to_request);
}






