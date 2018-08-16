#include "internal.h"

#define DIV_ADDRESS (0xff04)
#define TIMA_ADDRESS (0xff05)
#define TMA_ADDRESS (0xff06)
#define TAC_ADDRESS (0xff07)

u16 incrementer_every_cycle;
u8 *div = ((u8*)&incrementer_every_cycle) + 1;

int cycles_since_last_tima_increment = 0;

void init_timer() {
	incrementer_every_cycle = 0xabcc;
	assert(*div == 0xab);
	mem_write(DIV_ADDRESS, *div);
	
	mem_write(TIMA_ADDRESS, 0x00);
	mem_write(TMA_ADDRESS, 0x00);
	mem_write(TAC_ADDRESS, 0x00);
}

void update_timer(u8 num_cycles) {
	
	// if the div register was written to by the game, set it to 0.
	{
		Mem_Log logs[MEM_MAX_NUM_LOGS];
		int num_logs;
		mem_get_logs(logs, &num_logs);
		
		for (int l = 0; l < num_logs; l++) {
			if (logs[l].address == DIV_ADDRESS) {
				mem_write(DIV_ADDRESS, 0x00);
				break;
			}
		}
	}
	
	u8 tac = mem_read(TAC_ADDRESS); // TODO: what do with the upper 5 bytes of this register?
	bool timer_enabled = tac & 0x04; // TODO: don't know if a disabled timer gets set to 0 or just pauses.
	
	int cycles_per_tima_increment;
	switch (tac & 0x03) {
		case 0x00: cycles_per_tima_increment = 1024; break;
		case 0x01: cycles_per_tima_increment = 16; break;
		case 0x02: cycles_per_tima_increment = 64; break;
		case 0x03: cycles_per_tima_increment = 256; break;
		default: assert(false); break;
	}
	
	for (int i = 0; i < num_cycles; i++) {
		// update incrementer and therefore DIV.
		incrementer_every_cycle++;
		
		if (timer_enabled && ++cycles_since_last_tima_increment >= cycles_per_tima_increment) {
			// TODO: can optimise these mem reads and writes out of the loop if necessary.
			
			u8 prev_tima = mem_read(TIMA_ADDRESS);
			u8 new_tima = prev_tima + 1;
			
			// check for overflow
			if (prev_tima > new_tima) {
				new_tima = mem_read(TMA_ADDRESS);
				request_interrupt(INTERRUPT_FLAG_TIMER);
			}
			
			mem_write(TIMA_ADDRESS, new_tima);
			cycles_since_last_tima_increment = 0;
		}
	}
	
	mem_write(DIV_ADDRESS, *div);
}







