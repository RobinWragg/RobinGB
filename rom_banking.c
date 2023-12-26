#include "internal.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define BANK_SIZE 16384 /* 16kB */
#define BANK_COUNT_ADDRESS 0x0148

static int16_t robingb_romb_current_switchable_bank;

/* After init_cart_state(), cached_banks contains all ROM banks other than banks 0 and 1.
Banks 0 and 1 are stored at the start of robingb_memory.
Bank 2 is at cached_banks[0], bank 3 at cached_banks[1] and so on. */
typedef struct {
	uint8_t data[BANK_SIZE];
} Cached_Bank;

Cached_Bank *cached_banks = NULL;
static uint16_t cached_bank_count = 0;

uint8_t robingb_romb_read_switchable_bank(uint16_t address) {
	if (robingb_romb_current_switchable_bank == 1) {
		return robingb_memory[address];
	} else {
		return cached_banks[robingb_romb_current_switchable_bank-2].data[address-BANK_SIZE];
	}
}

void robingb_romb_init_first_banks() {
	/* Load ROM banks 0 and 1 */
	printf("Loading the first 2 ROM banks...\n");
	bool success = robingb_read_file(robingb_cart_path, 0, BANK_SIZE*2, robingb_memory);
	assert(success);
	robingb_romb_current_switchable_bank = 1;
	printf("Done\n");
}

void robingb_romb_init_additional_banks() {
	int16_t total_bank_count;
	
	uint8_t bank_count_identifier = robingb_memory[BANK_COUNT_ADDRESS];
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
	
	printf("Cart has a total of %i ROM banks\n", total_bank_count);
	
	if (total_bank_count > 2) {
		if (cached_banks) {
			printf("free()ing previous ROM bank cache...\n");
			free(cached_banks);
			printf("Done\n");
		}
		
		cached_bank_count = total_bank_count - 2;
		
		printf("Allocating %iKB for the remaining %i ROM banks...\n", cached_bank_count*BANK_SIZE/1024, cached_bank_count);
		
		cached_banks = (Cached_Bank*)malloc(sizeof(uint8_t) * BANK_SIZE * cached_bank_count);
		
		assert(cached_banks);
		printf("Done\n");
		
		printf("Loading the remaining %i ROM banks...\n", cached_bank_count);
		
		uint32_t file_offset = BANK_SIZE * 2; /* Offset of 2, as the first 2 banks are already loaded */
		
		bool success = robingb_read_file(robingb_cart_path, file_offset, BANK_SIZE*cached_bank_count, (uint8_t*)cached_banks);
		assert(success);
		
		printf("Done\n");
	} else cached_bank_count = 0;
}

void robingb_romb_perform_bank_control(int address, uint8_t value, Mbc_Type mbc_type) {
	switch (mbc_type) {
		case MBC_NONE:
		/* no-op */
		break;
		case MBC_1: {
			if (address >= 0x2000 && address < 0x4000) {
				/* Select lower 5 bits of ROM bank number */
				
				value &= 0x1f; /* Discard all but the lower 5 bits */
				uint8_t new_bank = robingb_romb_current_switchable_bank & ~0x1f; /* wipe the lower 5 bits */
				new_bank |= value; /* set the lower 5 bits to the new value */
				
				if (new_bank == 0x00 || new_bank == 0x20 || new_bank == 0x40 || new_bank == 0x60) new_bank++;
				
				robingb_romb_current_switchable_bank = new_bank;
			} else if (address >= 0x4000 && address < 0x6000) {
				/* Select bits 5+6 of ROM bank number */
				
				uint8_t bits_5_6_mask = robingb_bit(5) & robingb_bit(6);
				
				value <<= 5; /* Put bits 0+1 in slots 5+6 */
				value &= bits_5_6_mask; /* Discard all but bits 5+6 */
				
				robingb_romb_current_switchable_bank &= ~bits_5_6_mask; /* wipe bits 5+6 */
				
				robingb_romb_current_switchable_bank |= value; /* set bits 5+6 to the new value */
			} else assert(false);
		} break;
		case MBC_3: {
			assert(false); /* Not implemented yet */
		} break;
		default: assert(false); break;
	}
}








