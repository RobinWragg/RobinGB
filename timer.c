#include "internal.h"
#include <assert.h>

#define DIV_ADDRESS 0xff04
#define TIMA_ADDRESS 0xff05
#define TMA_ADDRESS 0xff06
#define TAC_ADDRESS 0xff07

#define MINIMUM_CYCLES_PER_TIMA_INCREMENT 16

static u8 *tima = &robingb_memory[TIMA_ADDRESS];
static u8 *tma = &robingb_memory[TMA_ADDRESS];
static u8 *tac = &robingb_memory[TAC_ADDRESS]; /* TODO: what do with the upper 5 bytes of this register? */

static u16 incrementer_every_cycle;
static u8 *div = ((u8*)&incrementer_every_cycle) + 1;

static u16 cycles_since_last_tima_increment = 0;

void init_timer() {
	incrementer_every_cycle = 0xabcc;
	assert(*div == 0xab);
	robingb_memory[DIV_ADDRESS] = *div;
	
	*tima = 0x00;
	*tma = 0x00;
	*tac = 0x00;
}

u8 process_written_timer_div_register() {
	return 0x00;
}

void update_timer(u8 num_cycles) {
	
	/* update incrementer and therefore DIV. */
	incrementer_every_cycle += num_cycles;
	robingb_memory[DIV_ADDRESS] = *div;
	
	/* Update TIMA and potentially request an interrupt */
	if ((*tac) & 0x04 /* check if timer is enabled */) {
	
		cycles_since_last_tima_increment += num_cycles;
		
		if (cycles_since_last_tima_increment >= MINIMUM_CYCLES_PER_TIMA_INCREMENT) {
			/* calculate actual cycles per TIMA increment */
			u16 cycles_per_tima_increment;
			switch ((*tac) & 0x03) {
				case 0x00: cycles_per_tima_increment = 1024; break;
				case 0x01: cycles_per_tima_increment = 16; break;
				case 0x02: cycles_per_tima_increment = 64; break;
				case 0x03: cycles_per_tima_increment = 256; break;
				default: assert(false); break;
			}
			
			if (cycles_since_last_tima_increment >= cycles_per_tima_increment) {
				u8 prev_tima = (*tima)++;
				
				/* check for overflow */
				if (prev_tima > *tima) {
					*tima = *tma;
					request_interrupt(INTERRUPT_FLAG_TIMER);
				}
				
				cycles_since_last_tima_increment -= cycles_per_tima_increment;
			}
		}
	}
}







