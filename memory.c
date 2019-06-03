
#include "internal.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define GAME_BOY_MEMORY_ADDRESS_SPACE_SIZE (1024*64)

#define ROM_BANK_SIZE 16384 /* 16kB */

u8 robingb_memory[GAME_BOY_MEMORY_ADDRESS_SPACE_SIZE];

typedef enum {
	CART_TYPE_ROM_ONLY = 0x00,
	CART_TYPE_MBC1 = 0x01,
	CART_TYPE_MBC1_RAM = 0x02,
	CART_TYPE_MBC1_RAM_BATTERY = 0x03,
	CART_TYPE_MBC2 = 0x05,
	CART_TYPE_MBC2_BATTERY = 0x06,
	CART_TYPE_RAM = 0x08,
	CART_TYPE_RAM_BATTERY = 0x09,
	CART_TYPE_MMM01 = 0x0b,
	CART_TYPE_MMM01_RAM = 0x0c,
	CART_TYPE_MMM01_RAM_BATTERY = 0x0d,
	CART_TYPE_MBC3_TIMER_BATTERY = 0x0f,
	CART_TYPE_MBC3_TIMER_RAM_BATTERY = 0x10,
	CART_TYPE_MBC3 = 0x11,
	CART_TYPE_MBC3_RAM = 0x12,
	CART_TYPE_MBC3_RAM_BATTERY = 0x13,
	CART_TYPE_MBC4 = 0x15,
	CART_TYPE_MBC4_RAM = 0x16,
	CART_TYPE_MBC4_RAM_BATTERY = 0x17,
	CART_TYPE_MBC5 = 0x19,
	CART_TYPE_MBC5_RAM = 0x1a,
	CART_TYPE_MBC5_RAM_BATTERY = 0x1b,
	CART_TYPE_MBC5_RUMBLE = 0x1c,
	CART_TYPE_MBC5_RUMBLE_RAM = 0x1d,
	CART_TYPE_MBC5_RUMBLE_RAM_BATTERY = 0x1e,
	CART_TYPE_POCKET_CAMERA = 0xfc,
	CART_TYPE_BANDAI_TAMA5 = 0xfd,
	CART_TYPE_HuC3 = 0xfe,
	CART_TYPE_HuC1_RAM_BATTERY = 0xff,
	CART_TYPE_UNDEFINED
} Cart_Type;

typedef enum {
	MBC_NONE,
	MBC_1,
	MBC_2,
	MBC_3
} Mbc_Type;

/* ----------------------------------------------- */
/* cart control code                               */
/* ----------------------------------------------- */

static struct {
	Mbc_Type mbc_type;
	char file_path[256];
	bool has_ram;
	s16 rom_bank_count;
	s16 current_switchable_rom_bank;
} cart_state;

/* After set_cart_state(), cached_rom_banks contains all ROM banks other than banks 0 and 1.
Banks 0 and 1 are stored at the start of robingb_memory.
Bank 2 is at cached_rom_banks[0], bank 3 at cached_rom_banks[1] and so on. */
static struct {
	u8 data[ROM_BANK_SIZE];
} *cached_rom_banks = NULL;
u16 cached_rom_bank_count = 0;

static void perform_rom_bank_control(int address, u8 value) {
	assert(address >= 0x2000 && address < 0x4000);
	
	switch (cart_state.mbc_type) {
		case MBC_NONE:
		/* no-op */
		break;
		case MBC_1: {
			assert(value <= 0x1f);
			u8 new_bank = cart_state.current_switchable_rom_bank & ~0x1f; /* wipe the lower 5 bits */
			new_bank |= value; /* set the lower 5 bits to the new value */

			if (new_bank == 0x00) new_bank++;
			else if (new_bank == 0x20) new_bank++;
			else if (new_bank == 0x40) new_bank++;
			else if (new_bank == 0x60) new_bank++;
			cart_state.current_switchable_rom_bank = new_bank;
		} break;
		case MBC_3: {
			assert(value <= 0x7f);
			if (value == 0x00) value++;
			cart_state.current_switchable_rom_bank = value;
		} break;
		default: assert(false); break;
	}
}

static void perform_cart_control(int address, u8 value) {
	if (address >= 0x0000 && address < 0x2000) {
		/* MBC1: enable/disable external RAM (at 0xa000 to 0xbfff) */
		
		/* ignore the request if the cart has no RAM */
		if (cart_state.has_ram) {
			
		}
		
	} else if (address >= 0x2000 && address < 0x4000) {
		
		perform_rom_bank_control(address, value);
		
	} else if (address >= 0x4000 && address < 0x6000) {
		
		/* Do nothing if cart has no MBC and no RAM. NOTE: Multiple RAM banks may exist even if there is no MBC! */
		if (cart_state.mbc_type == MBC_NONE && !cart_state.has_ram) return;
		
		/* MBC1: RAM bank number, or, upper bits of ROM bank number, depending on ROM/RAM mode */
		assert(false);
		
	} else if (address >= 0x6000 && address < 0x8000) {
		assert(false); /* MBC1: ROM/RAM mode select */
	} else assert(false);
}

static void set_cart_state() {
	char buf[256];
	
	/* Load ROM banks 0 and 1 */
	robingb_log("Loading the first 2 ROM banks...");
	robingb_read_file(cart_state.file_path, 0, ROM_BANK_SIZE*2, robingb_memory);
	robingb_log("Done");
	
	
	Cart_Type cart_type = mem_read(0x0147);
	
	/* MBC type */
	switch (cart_type) {
		case CART_TYPE_ROM_ONLY:
		case CART_TYPE_RAM:
		case CART_TYPE_RAM_BATTERY:
			/* TODO: Not sure if this is a complete list of non-MBC cart types. */
			robingb_log("Cart has no MBC");
			assert(mem_read(0x0148) == 0x00);
			cart_state.mbc_type = MBC_NONE;
		break;
		
		case CART_TYPE_MBC1:
		case CART_TYPE_MBC1_RAM:
		case CART_TYPE_MBC1_RAM_BATTERY:
			robingb_log("Cart has an MBC1");
			cart_state.mbc_type = MBC_1;
		break;
		
		case CART_TYPE_MBC2:
		case CART_TYPE_MBC2_BATTERY:
			robingb_log("Cart has an MBC2");
			cart_state.mbc_type = MBC_2;
		break;
		
		case CART_TYPE_MBC3:
		case CART_TYPE_MBC3_RAM:
		case CART_TYPE_MBC3_RAM_BATTERY:
		case CART_TYPE_MBC3_TIMER_BATTERY:
		case CART_TYPE_MBC3_TIMER_RAM_BATTERY:
			robingb_log("Cart has an MBC3");
			cart_state.mbc_type = MBC_3;
		break;
		
		default: {
			char buf[128];
			sprintf(buf, "Unrecognised cart type: %x", cart_type);
			robingb_log(buf);
			assert(false);
		} break;
	}
	
	/* Additional ROM banks */
	cart_state.current_switchable_rom_bank = 1;
	
	u8 rom_bank_count_identifier = mem_read(0x0148);
	if (rom_bank_count_identifier <= 0x08) {
		cart_state.rom_bank_count = 2 << rom_bank_count_identifier;
	} else {
		switch (rom_bank_count_identifier) {
			case 0x52: cart_state.rom_bank_count = 72; break;
			case 0x53: cart_state.rom_bank_count = 80; break;
			case 0x54: cart_state.rom_bank_count = 96; break;
			default: assert(false); break;
		}
	}
	
	sprintf(buf, "Cart has a total of %i ROM banks", cart_state.rom_bank_count);
	robingb_log(buf);
	
	if (cart_state.rom_bank_count > 2) {
		if (cached_rom_banks) {
			robingb_log("free()ing previous ROM bank cache...");
			free(cached_rom_banks);
			robingb_log("Done");
		}
		
		cached_rom_bank_count = cart_state.rom_bank_count - 2;
		
		sprintf(buf, "Allocating %iKB for the remaining %i ROM banks...", cached_rom_bank_count*16, cached_rom_bank_count);
		robingb_log(buf);
		
		cached_rom_banks = malloc(ROM_BANK_SIZE * cached_rom_bank_count);
		
		assert(cached_rom_banks);
		robingb_log("Done");
		
		sprintf(buf, "Loading the remaining %i ROM banks...", cached_rom_bank_count);
		robingb_log(buf);
		
		u32 file_offset = ROM_BANK_SIZE * 2;
		robingb_read_file(cart_state.file_path, file_offset, ROM_BANK_SIZE*cached_rom_bank_count, (u8*)cached_rom_banks);
		robingb_log("Done");
	} else cached_rom_bank_count = 0;
	
	/* has RAM */
	switch (cart_type) {
		case CART_TYPE_MBC1_RAM:
		case CART_TYPE_MBC1_RAM_BATTERY:
		case CART_TYPE_RAM:
		case CART_TYPE_RAM_BATTERY:
		case CART_TYPE_MMM01_RAM:
		case CART_TYPE_MMM01_RAM_BATTERY:
		case CART_TYPE_MBC3_TIMER_RAM_BATTERY:
		case CART_TYPE_MBC3_RAM:
		case CART_TYPE_MBC3_RAM_BATTERY:
		case CART_TYPE_MBC4_RAM:
		case CART_TYPE_MBC4_RAM_BATTERY:
		case CART_TYPE_MBC5_RAM:
		case CART_TYPE_MBC5_RAM_BATTERY:
		case CART_TYPE_MBC5_RUMBLE_RAM:
		case CART_TYPE_MBC5_RUMBLE_RAM_BATTERY:
		case CART_TYPE_HuC1_RAM_BATTERY:
			cart_state.has_ram = true;
			robingb_log("Cart has RAM");
		break;
		
		default: {
			cart_state.has_ram = false;
			robingb_log("Cart has no RAM");
			assert(mem_read(0x0149) == 0x00);
		} break;
	}
}

/* ----------------------------------------------- */
/* General memory code                             */
/* ----------------------------------------------- */

void mem_init(const char *cart_file_path) {
	strcpy(cart_state.file_path, cart_file_path);
	
	mem_write(0xff10, 0x80);
	mem_write(0xff11, 0xbf);
	mem_write(0xff12, 0xf3);
	mem_write(0xff14, 0xbf);
	mem_write(0xff16, 0x3f);
	mem_write(0xff17, 0x00);
	mem_write(0xff19, 0xbf);
	mem_write(0xff1a, 0x7f);
	mem_write(0xff1b, 0xff);
	mem_write(0xff1c, 0x9f);
	mem_write(0xff1e, 0xbf);
	mem_write(0xff00, 0xff);
	int address;
	for (address = 0xff10; address <= 0xff26; address++) mem_write(address, 0x00); /* zero audio */
	mem_write(0xff20, 0xff);
	mem_write(0xff21, 0x00);
	mem_write(0xff22, 0x00);
	mem_write(0xff23, 0xbf);
	mem_write(0xff24, 0x77);
	mem_write(0xff25, 0xf3);
	mem_write(0xff26, 0xf1); /* NOTE: This is different for Game Boy Color etc. */
	mem_write(LCD_CONTROL_ADDRESS, 0x91);
	mem_write(LCD_STATUS_ADDRESS, 0x85);
	mem_write(0xff42, 0x00);
	mem_write(0xff43, 0x00);
	mem_write(0xff45, 0x00);
	mem_write(0xff47, 0xfc);
	mem_write(0xff48, 0xff);
	mem_write(0xff49, 0xff);
	mem_write(0xff4a, 0x00);
	mem_write(0xff4b, 0x00);
	mem_write(IF_ADDRESS, 0xe1); /* TODO: Might be acceptable for this to be 0xe0 */
	mem_write(IE_ADDRESS, 0x00);
	
	set_cart_state();
}

static u8 read_switchable_rom_bank(u16 address) {
	if (cart_state.current_switchable_rom_bank == 1) {
		return robingb_memory[address];
	} else {
		return cached_rom_banks[cart_state.current_switchable_rom_bank-2].data[address-ROM_BANK_SIZE];
	}
}

u8 mem_read(u16 address) {
	if (address >= 0x4000 && address < 0x8000) {
		return read_switchable_rom_bank(address);
	} else {
		return robingb_memory[address];
	}
}

u16 mem_read_u16(u16 address) {
	u16 out;
	u8 *bytes = (u8*)&out;
	bytes[0] = mem_read(address);
	bytes[1] = mem_read(address+1);
	return out;
}

void mem_write(u16 address, u8 value) {
	if (address < 0x8000) {
		perform_cart_control(address, value);
	} else if (address == 0xff00) {
		robingb_memory[address] = process_written_joypad_register(value);
	} else if (address == 0xff04) {
		robingb_memory[address] = process_written_timer_div_register();
	} else if (address == 0xff46) {
		memcpy(&robingb_memory[0xfe00], &robingb_memory[value * 0x100], 160); /* OAM DMA transfer */
	} else {
		robingb_memory[address] = value;
		
		if (address >= 0xc000 && address < 0xde00) {
			int echo_address = address-0xc000+0xe000;
			robingb_memory[echo_address] = value;
		} else if (address >= 0xe000 && address < 0xfe00) {
			int echo_address = address-0xe000+0xc000;
			robingb_memory[echo_address] = value;
		}
	}
}

void mem_write_u16(u16 address, u16 value) {
	u8 *values = (u8*)&value;
	mem_write(address, values[0]);
	mem_write(address+1, values[1]);
}









