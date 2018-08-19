#include "internal.h"
#include <assert.h>

// TODO: "Each bit is set to 1 automatically when an internal signal from that subsystem goes from '0' to '1', it doesn't matter if the corresponding bit in IE is set. This is specially important in the case of LCD STAT interrupt, as it will be explained in the video controller chapter."

// TODO: "When using a status interrupt in DMG or in CGB in DMG mode, register IF should be set to 0 after the value of the STAT register is set. (In DMG, setting the STAT register value changes the value of the IF register, and an interrupt is generated at the same time as interrupts are enabled.)"

/*
TODO:
"8.7. STAT Interrupt
This interrupt can be configured with register STAT.
The STAT IRQ is trigged by an internal signal.
This signal is set to 1 if:
	( (LY = LYC) AND (STAT.ENABLE_LYC_COMPARE = 1) ) OR
	( (ScreenMode = 0) AND (STAT.ENABLE_HBL = 1) ) OR
	( (ScreenMode = 2) AND (STAT.ENABLE_OAM = 1) ) OR
	( (ScreenMode = 1) AND (STAT.ENABLE_VBL || STAT.ENABLE_OAM) ) -> Not only VBL!!??
-If LCD is off, the signal is 0.
-It seems that this interrupt needs less time to execute in DMG than in CGB? -DMG bug?"
*/

#define NUM_CYCLES_PER_LY_INCREMENT 456
#define LY_ADDRESS 0xff44
#define LYC_ADDRESS 0xff45
#define MODE_2_CYCLE_DURATION 77
#define MODE_3_CYCLE_DURATION 169

void update_mode_and_write_status(int elapsed_cycles, u8 status) {
	u8 current_mode;
	
	if (elapsed_cycles < 65664) {
		int row_draw_phase = elapsed_cycles % NUM_CYCLES_PER_LY_INCREMENT;
		
		if (row_draw_phase < MODE_2_CYCLE_DURATION) {
			current_mode = 0x02; // The LCD is reading from OAM
		} else if (row_draw_phase < MODE_2_CYCLE_DURATION+MODE_3_CYCLE_DURATION) {
			current_mode = 0x03; // The LCD is reading from both OAM and VRAM
		} else {
			current_mode = 0x00; // H-blank
		}
	} else {
		current_mode = 0x01; // V-blank
	}
	
	const u8 prev_mode = status & 0x03; // get lower 2 bits only
	status &= 0xfc; // wipe the old mode
	
	bool should_render_screen_line = false;
	if (prev_mode != current_mode) {
		switch (current_mode) {
			case 0x00: {
				if (status & 0x08) request_interrupt(INTERRUPT_FLAG_LCD_STAT);
			} break;
			case 0x01: {
				request_interrupt(INTERRUPT_FLAG_VBLANK);
				if (status & 0x10) request_interrupt(INTERRUPT_FLAG_LCD_STAT);
			} break;
			case 0x02: {
				if (status & 0x20) request_interrupt(INTERRUPT_FLAG_LCD_STAT);
			} break;
			case 0x03: {
				should_render_screen_line = true;
			} break;
		}
	}
	
	status &= 0xfc;
	status |= current_mode;
	mem_write(LCD_STATUS_ADDRESS, status);
	
	if (should_render_screen_line) render_screen_line(mem_read(LY_ADDRESS));
}

void lcd_update(int num_cycles_delta) {
	u8 status = mem_read(LCD_STATUS_ADDRESS);
	
	if ((mem_read(LCD_CONTROL_ADDRESS) & 0x80) == 0) {
		// Bit 7 of the LCD control register is 0, so the LCD is switched off.
		// LY, the mode, and the LYC=LY flag should all be 0.
		mem_write(LY_ADDRESS, 0x00);
		mem_write(LCD_STATUS_ADDRESS, status & 0xf8);
		return; 
	}
	
	static int elapsed_cycles = 0;
	elapsed_cycles += num_cycles_delta;
	if (elapsed_cycles >= ROBINGB_CPU_CYCLES_PER_REFRESH) elapsed_cycles -= ROBINGB_CPU_CYCLES_PER_REFRESH;
	
	// set LY
	u8 ly = elapsed_cycles / NUM_CYCLES_PER_LY_INCREMENT;
	mem_write(LY_ADDRESS, ly);
	
	// handle LYC
	if (ly == mem_read(LYC_ADDRESS)) {
		status |= 0x04;
		if (status & 0x40) request_interrupt(INTERRUPT_FLAG_LCD_STAT);
	} else {
		status &= ~0x04;
	}
	
	update_mode_and_write_status(elapsed_cycles, status);
	
	// assertions
	// {
	// 	u8 ly = mem_read(LY_ADDRESS);
	// 	u8 mode = mem_read(LCD_STATUS_ADDRESS) & 0x03; // get lower 2 bits only
	// 	assert((ly < 144 && (mode == 0x02 || mode == 0x03 || mode == 0x00))
	// 		|| (ly >= 144 && mode == 0x01));
	// 	assert(ly < 154);
	// }
}







