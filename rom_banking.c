#include "internal.h"
#include <assert.h>

#define BANK_SIZE 16384 /* 16kB */

s16 romb_current_switchable_bank;

void romb_init_first_banks(const char *file_path) {
	/* Load ROM banks 0 and 1 */
	robingb_log("Loading the first 2 ROM banks...");
	robingb_read_file(file_path, 0, BANK_SIZE*2, robingb_memory);
	romb_current_switchable_bank = 1;
	robingb_log("Done");
}

void romb_perform_bank_control(int address, u8 value, Mbc_Type mbc_type) {
	assert(address >= 0x2000 && address < 0x4000);
	
	switch (mbc_type) {
		case MBC_NONE:
		/* no-op */
		break;
		case MBC_1: {
			value &= 0x1f; /* Discard all but the lower 5 bits */
			u8 new_bank = romb_current_switchable_bank & ~0x1f; /* wipe the lower 5 bits */
			new_bank |= value; /* set the lower 5 bits to the new value */
			
			/* TODO: This 0-to-1 conversion should be done when setting the upper bits for the rom bank index instead. */
			if (new_bank == 0x00 || new_bank == 0x20 || new_bank == 0x40 || new_bank == 0x60) new_bank++;
			
			romb_current_switchable_bank = new_bank;
		} break;
		case MBC_3: {
			assert(false); /* Not implemented yet */
		} break;
		default: assert(false); break;
	}
}








