#include "internal.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define GAME_BOY_MEMORY_ADDRESS_SPACE_SIZE (1024*64)

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

/* ----------------------------------------------- */
/* cart control code                               */
/* ----------------------------------------------- */
typedef enum {
	BM_ROM,
	BM_RAM
} Banking_Mode;

static struct {
	Mbc_Type mbc_type;
	char file_path[256];
	bool has_ram;
	Banking_Mode banking_mode;
} cart_state;

static void ramb_perform_bank_control(int address, u8 value, Mbc_Type mbc_type) {
	printf("perform_ram_bank_control: %x %x\n", address, value);
	
	/* Temporary no-op */
}

static void perform_cart_control(int address, u8 value) {
	/* printf("perform_cart_control: %x %x\n", address, value); */
	
	if (address >= 0x0000 && address < 0x2000) {
		/* MBC1: enable/disable external RAM (at 0xa000 to 0xbfff) */
		
		/* Ignore the request if the cart has no RAM */
		if (cart_state.has_ram) {
			if ((value & 0x0f) == 0x0a) {
				printf("TODO: Enable RAM\n");
			} else {
				printf("TODO: Disable RAM\n");
			}
		}
		
	} else if (address >= 0x2000 && address < 0x4000) {
		
		robingb_romb_perform_bank_control(address, value, cart_state.mbc_type);
		
	} else if (address >= 0x4000 && address < 0x6000) {
		
		/* Do nothing if cart has no MBC and no RAM. NOTE: Multiple RAM banks may exist even if there is no MBC! */
		if (cart_state.mbc_type == MBC_NONE && !cart_state.has_ram) return;
		
		assert(cart_state.mbc_type == MBC_1); /* Code below assumes mbc1. */
		
		if (cart_state.banking_mode == BM_ROM) {
			robingb_romb_perform_bank_control(address, value, cart_state.mbc_type);
		} else {
			ramb_perform_bank_control(address, value, cart_state.mbc_type);
		}
		
	} else if (address >= 0x6000 && address < 0x8000) {
		/* MBC1: ROM/RAM banking mode select */
		assert(cart_state.mbc_type == MBC_1);
		
		if (value & 0x01) {
			cart_state.banking_mode = BM_RAM;
			printf("Switched to RAM banking mode\n");
		} else {
			cart_state.banking_mode = BM_ROM;
			printf("Switched to RAM banking mode\n");
		}
		
	} else assert(false);
}

static Mbc_Type calculate_mbc_type(Cart_Type cart_type) {
	switch (cart_type) {
		case CART_TYPE_ROM_ONLY:
		case CART_TYPE_RAM:
		case CART_TYPE_RAM_BATTERY:
			/* TODO: Not sure if this is a complete list of non-MBC cart types. */
			printf("Cart has no MBC\n");
			assert(robingb_memory_read(0x0148) == 0x00);
			return MBC_NONE;
		break;
		
		case CART_TYPE_MBC1:
		case CART_TYPE_MBC1_RAM:
		case CART_TYPE_MBC1_RAM_BATTERY:
			printf("Cart has an MBC1\n");
			return MBC_1;
		break;
		
		case CART_TYPE_MBC2:
		case CART_TYPE_MBC2_BATTERY:
			printf("Cart has an MBC2\n");
			return MBC_2;
		break;
		
		case CART_TYPE_MBC3:
		case CART_TYPE_MBC3_RAM:
		case CART_TYPE_MBC3_RAM_BATTERY:
		case CART_TYPE_MBC3_TIMER_BATTERY:
		case CART_TYPE_MBC3_TIMER_RAM_BATTERY:
			printf("Cart has an MBC3\n");
			return MBC_3;
		break;
		
		default: {
			printf("Unrecognised cart type: %x", cart_type);
			assert(false);
		} break;
	}
}

static int calculate_ram_bank_count(Cart_Type cart_type) {
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
			printf("Cart has RAM\n");
			return 9999; /* TODO: Calculate the correct value */
			
			/*
			Byte at address 0x0149:
			00h - None
			01h - 2 KBytes
			02h - 8 Kbytes
			03h - 32 KBytes (4 banks of 8KBytes each)
			04h - 128 KBytes (16 banks of 8KBytes each)
			05h - 64 KBytes (8 banks of 8KBytes each)
			*/
		break;
		
		default: {
			printf("Cart has no RAM\n");
			assert(robingb_memory_read(0x0149) == 0x00);
			return 0;
		} break;
	}
}

static void init_cart_state(
	const char *file_path,
	void (*read_file_function_ptr)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[])
	) {
	
	strcpy(cart_state.file_path, file_path);
	robingb_romb_init_first_banks(cart_state.file_path, read_file_function_ptr);
	
	Cart_Type cart_type = (Cart_Type)robingb_memory_read(0x0147);
	
	cart_state.mbc_type = calculate_mbc_type(cart_type);
	
	/* Supported MBC types are currently MBC1, MBC3, or none (ROM only). */
	assert(cart_state.mbc_type == MBC_NONE
		|| cart_state.mbc_type == MBC_1
		|| cart_state.mbc_type == MBC_3);
	
	robingb_romb_init_additional_banks();
	
	int ram_bank_count = calculate_ram_bank_count(cart_type);
	cart_state.has_ram = ram_bank_count > 0;
	
	cart_state.banking_mode = BM_ROM; /* Default banking mode */
}

/* ----------------------------------------------- */
/* General memory code                             */
/* ----------------------------------------------- */

void robingb_memory_init(
	const char *cart_file_path,
	void (*read_file_function_ptr)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[])
	) {
	robingb_memory_write(0xff10, 0x80);
	robingb_memory_write(0xff11, 0xbf);
	robingb_memory_write(0xff12, 0xf3);
	robingb_memory_write(0xff14, 0xbf);
	robingb_memory_write(0xff16, 0x3f);
	robingb_memory_write(0xff17, 0x00);
	robingb_memory_write(0xff19, 0xbf);
	robingb_memory_write(0xff1a, 0x7f);
	robingb_memory_write(0xff1b, 0xff);
	robingb_memory_write(0xff1c, 0x9f);
	robingb_memory_write(0xff1e, 0xbf);
	robingb_memory_write(0xff00, 0xff);
	int address;
	for (address = 0xff10; address <= 0xff26; address++) robingb_memory_write(address, 0x00); /* zero audio */
	robingb_memory_write(0xff20, 0xff);
	robingb_memory_write(0xff21, 0x00);
	robingb_memory_write(0xff22, 0x00);
	robingb_memory_write(0xff23, 0xbf);
	robingb_memory_write(0xff24, 0x77);
	robingb_memory_write(0xff25, 0xf3);
	robingb_memory_write(0xff26, 0xf1); /* NOTE: This is different for Game Boy Color etc. */
	robingb_memory_write(LCD_CONTROL_ADDRESS, 0x91);
	robingb_memory_write(LCD_STATUS_ADDRESS, 0x85);
	robingb_memory_write(0xff42, 0x00);
	robingb_memory_write(0xff43, 0x00);
	robingb_memory_write(0xff45, 0x00);
	robingb_memory_write(0xff47, 0xfc);
	robingb_memory_write(0xff48, 0xff);
	robingb_memory_write(0xff49, 0xff);
	robingb_memory_write(0xff4a, 0x00);
	robingb_memory_write(0xff4b, 0x00);
	robingb_memory_write(IF_ADDRESS, 0xe1); /* TODO: Might be acceptable for this to be 0xe0 */
	robingb_memory_write(IE_ADDRESS, 0x00);
	
	init_cart_state(cart_file_path, read_file_function_ptr);
}

u8 robingb_memory_read(u16 address) {
	if (address >= 0x4000 && address < 0x8000) {
		return robingb_romb_read_switchable_bank(address);
	} else {
		return robingb_memory[address];
	}
}

u16 robingb_memory_read_u16(u16 address) {
	u16 out;
	u8 *bytes = (u8*)&out;
	bytes[0] = robingb_memory_read(address);
	bytes[1] = robingb_memory_read(address+1);
	return out;
}

void robingb_memory_write(u16 address, u8 value) {
	if (address < 0x8000) {
		perform_cart_control(address, value);
	} else if (address == 0xff00) {
		robingb_memory[address] = robingb_respond_to_joypad_register(value);
	} else if (address == 0xff04) {
		robingb_memory[address] = robingb_respond_to_timer_div_register();
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

void robingb_memory_write_u16(u16 address, u16 value) {
	u8 *values = (u8*)&value;
	robingb_memory_write(address, values[0]);
	robingb_memory_write(address+1, values[1]);
}









