#include "internal.h"
#include <assert.h>

#define DIVIDER_ADDRESS 0xff04 /* "DIV" */
#define COUNTER_ADDRESS 0xff05 /* "TIMA" */ 
#define MODULO_ADDRESS 0xff06 /* "TMA" */
#define CONTROL_ADDRESS 0xff07 /* "TAC" */

#define MINIMUM_CYCLES_PER_COUNTER_INCREMENT 16

static uint8_t *counter = &robingb_memory[COUNTER_ADDRESS];
static uint8_t *modulo  = &robingb_memory[MODULO_ADDRESS];
static uint8_t *control = &robingb_memory[CONTROL_ADDRESS]; /* Note, the upper 5 bits are undefined. */

static uint16_t incrementer_every_cycle;
static uint8_t *div_byte = ((uint8_t*)&incrementer_every_cycle) + 1;

static uint16_t cycles_since_last_tima_increment = 0;

void robingb_timer_init() {
	incrementer_every_cycle = 0xabcc;
	assert(*div_byte == 0xab);
	robingb_memory[DIVIDER_ADDRESS] = *div_byte;
	
	*counter = 0x00;
	*modulo = 0x00;
	*control = 0x00;
}

uint8_t robingb_respond_to_timer_div_register() {
	return 0x00;
}

void robingb_timer_update(uint8_t num_cycles) {
	
	/* update incrementer and therefore DIV. */
	incrementer_every_cycle += num_cycles;
	robingb_memory[DIVIDER_ADDRESS] = *div_byte;
	
	/* Update TIMA and potentially request an interrupt */
	if ((*control) & 0x04 /* check if timer is enabled */) {
	
		cycles_since_last_tima_increment += num_cycles;
		
		if (cycles_since_last_tima_increment >= MINIMUM_CYCLES_PER_COUNTER_INCREMENT) {
			/* calculate actual cycles per TIMA increment from the lowest 2 bits */
			uint16_t cycles_per_tima_increment;
			switch ((*control) & 0x03) {
				case 0x00: cycles_per_tima_increment = 1024; break;
				case 0x01: cycles_per_tima_increment = 16; break;
				case 0x02: cycles_per_tima_increment = 64; break;
				case 0x03: cycles_per_tima_increment = 256; break;
				default: assert(false); break;
			}
			
			if (cycles_since_last_tima_increment >= cycles_per_tima_increment) {
				uint8_t prev_tima = (*counter)++;
				
				/* check for overflow */
				if (prev_tima > *counter) {
					*counter = *modulo;
					robingb_request_interrupt(INTERRUPT_FLAG_TIMER);
				}
				
				cycles_since_last_tima_increment -= cycles_per_tima_increment;
			}
		}
	}
}







