#include "internal.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define BANK_SIZE 16384 /* 16kB */
#define BANK_COUNT_ADDRESS 0x0148

s16 romb_current_switchable_bank;

/* After init_cart_state(), cached_banks contains all ROM banks other than banks 0 and 1.
Banks 0 and 1 are stored at the start of robingb_memory.
Bank 2 is at cached_banks[0], bank 3 at cached_banks[1] and so on. */
static struct {
	u8 data[BANK_SIZE];
} *cached_banks = NULL;
static u16 cached_bank_count = 0;

u8 romb_read_switchable_bank(u16 address) {
	if (romb_current_switchable_bank == 1) {
		return robingb_memory[address];
	} else {
		return cached_banks[romb_current_switchable_bank-2].data[address-BANK_SIZE];
	}
}

void romb_init_first_banks(const char *file_path) {
	/* Load ROM banks 0 and 1 */
	robingb_log("Loading the first 2 ROM banks...");
	robingb_read_file(file_path, 0, BANK_SIZE*2, robingb_memory);
	romb_current_switchable_bank = 1;
	robingb_log("Done");
}

void romb_init_additional_banks(const char *file_path) {
	s16 total_bank_count;
	
	u8 bank_count_identifier = robingb_memory[BANK_COUNT_ADDRESS];
	if (bank_count_identifier <= 0x08) {
		total_bank_count = 2 << bank_count_identifier;
	} else {
		switch (bank_count_identifier) {
			case 0x52: total_bank_count = 72; break;
			case 0x53: total_bank_count = 80; break;
			case 0x54: total_bank_count = 96; break;
			default: assert(false); break;
		}
	}
	
	char buf[256];
	sprintf(buf, "Cart has a total of %i ROM banks", total_bank_count);
	robingb_log(buf);
	
	if (total_bank_count > 2) {
		if (cached_banks) {
			robingb_log("free()ing previous ROM bank cache...");
			free(cached_banks);
			robingb_log("Done");
		}
		
		cached_bank_count = total_bank_count - 2;
		
		sprintf(buf, "Allocating %iKB for the remaining %i ROM banks...", cached_bank_count*BANK_SIZE/1024, cached_bank_count);
		robingb_log(buf);
		
		cached_banks = malloc(BANK_SIZE * cached_bank_count);
		
		assert(cached_banks);
		robingb_log("Done");
		
		sprintf(buf, "Loading the remaining %i ROM banks...", cached_bank_count);
		robingb_log(buf);
		
		u32 file_offset = BANK_SIZE * 2; /* Offset of 2, as the first 2 banks are already loaded */
		robingb_read_file(file_path, file_offset, BANK_SIZE*cached_bank_count, (u8*)cached_banks);
		robingb_log("Done");
	} else cached_bank_count = 0;
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








