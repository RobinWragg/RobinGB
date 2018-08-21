#include "internal.h"
#include <assert.h>

#define TIMA_ADDRESS 0xff05
#define TMA_ADDRESS 0xff06
#define TAC_ADDRESS 0xff07

static u8 *tima = &robingb_memory[TIMA_ADDRESS];
static u8 *tma = &robingb_memory[TMA_ADDRESS];
static u8 *tac = &robingb_memory[TAC_ADDRESS]; // TODO: what do with the upper 5 bytes of this register?

static u16 incrementer_every_cycle;
static u8 *div = ((u8*)&incrementer_every_cycle) + 1;

int cycles_since_last_tima_increment = 0;

void init_timer() {
	incrementer_every_cycle = 0xabcc;
	assert(*div == 0xab);
	mem_write(TIMER_DIV_ADDRESS, *div);
	
	*tima = 0x00;
	*tma = 0x00;
	*tac = 0x00;
}

u8 get_new_timer_div_value_on_write() {
	return 0x00;
}

void update_timer(u8 num_cycles) {
	bool timer_enabled = (*tac) & 0x04; // TODO: don't know if a disabled timer gets set to 0 or just pauses.
	
	int cycles_per_tima_increment;
	switch ((*tac) & 0x03) {
		case 0x00: cycles_per_tima_increment = 1024; break;
		case 0x01: cycles_per_tima_increment = 16; break;
		case 0x02: cycles_per_tima_increment = 64; break;
		case 0x03: cycles_per_tima_increment = 256; break;
		default: assert(false); break;
	}
	
	// Can we do this without having to increment one at a time?
	for (int i = 0; i < num_cycles; i++) {
		// update incrementer and therefore DIV.
		incrementer_every_cycle++;
		
		if (timer_enabled && ++cycles_since_last_tima_increment >= cycles_per_tima_increment) {
			u8 prev_tima = *tima;
			(*tima)++;
			
			// check for overflow
			if (prev_tima > *tima) {
				*tima = *tma;
				request_interrupt(INTERRUPT_FLAG_TIMER);
			}
			
			cycles_since_last_tima_increment = 0;
		}
	}
	
	mem_write(TIMER_DIV_ADDRESS, *div);
}







