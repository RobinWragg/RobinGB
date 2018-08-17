#include "internal.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

u8 memory[1024 * 64];

typedef enum {
	CART_TYPE_ROM_ONLY = 0x00,
	CART_TYPE_ROM_MBC1 = 0x01,
	CART_TYPE_ROM_MBC1_RAM = 0x02,
	CART_TYPE_ROM_MBC1_RAM_BATTERY = 0x03,
	CART_TYPE_ROM_MBC2 = 0x05,
	CART_TYPE_ROM_MBC2_BATTERY = 0x06,
	CART_TYPE_ROM_RAM = 0x08,
	CART_TYPE_ROM_RAM_BATTERY = 0x09,
	CART_TYPE_ROM_MMM01 = 0x0b,
	CART_TYPE_ROM_MMM01_RAM = 0x0c,
	CART_TYPE_ROM_MMM01_RAM_BATTERY = 0x0d,
	CART_TYPE_ROM_MBC3_TIMER_BATTERY = 0x0f,
	CART_TYPE_ROM_MBC3_TIMER_RAM_BATTERY = 0x10,
	CART_TYPE_ROM_MBC3 = 0x11,
	CART_TYPE_ROM_MBC3_RAM = 0x12,
	CART_TYPE_ROM_MBC3_RAM_BATTERY = 0x13,
	CART_TYPE_ROM_MBC4 = 0x15,
	CART_TYPE_ROM_MBC4_RAM = 0x16,
	CART_TYPE_ROM_MBC4_RAM_BATTERY = 0x17,
	CART_TYPE_ROM_MBC5 = 0x19,
	CART_TYPE_ROM_MBC5_RAM = 0x1a,
	CART_TYPE_ROM_MBC5_RAM_BATTERY = 0x1b,
	CART_TYPE_ROM_MBC5_RUMBLE = 0x1c,
	CART_TYPE_ROM_MBC5_RUMBLE_RAM = 0x1d,
	CART_TYPE_ROM_MBC5_RUMBLE_RAM_BATTERY = 0x1e,
	CART_TYPE_ROM_POCKET_CAMERA = 0xfc,
	CART_TYPE_ROM_BANDAI_TAMA5 = 0xfd,
	CART_TYPE_ROM_HuC3 = 0xfe,
	CART_TYPE_ROM_HuC1_RAM_BATTERY = 0xff,
	CART_TYPE_ROM_UNDEFINED
} Cart_Type;

typedef enum {
	MBC_NONE,
	MBC_1,
	MBC_3
} Mbc_Type;

int current_rom_bank;
Mbc_Type mbc_type = MBC_NONE;

char current_rom_file_path[256];

void switch_rom_bank(int new_bank_index) {
	assert(new_bank_index > 0 && new_bank_index <= 125);
	
	const int single_bank_size = 0x4000; // 16kB
	const int rom_bank_source = new_bank_index * 0x4000; // Multiples of 16kB
	const int rom_bank_destination = 0x4000;
	
	char buf[64] = {0};
	sprintf(buf, "Switching to ROM bank %i, loading %iKB from file", new_bank_index, single_bank_size/1024);
	robingb_log(buf);
	
	robingb_read_file(current_rom_file_path, rom_bank_source, single_bank_size, &memory[rom_bank_destination]);
	
	current_rom_bank = new_bank_index;
}

void calculate_and_switch_rom_bank(int address, u8 value) {
	assert(address >= 0x2000 && address < 0x4000);
	
	switch (mbc_type) {
		case MBC_1: {
			assert(value <= 0x1f);
			u8 new_bank = current_rom_bank & ~0x1f; // wipe the lower 5 bits
			new_bank |= value; // set the lower 5 bits to the new value

			if (new_bank == 0x00) new_bank++;
			else if (new_bank == 0x20) new_bank++;
			else if (new_bank == 0x40) new_bank++;
			else if (new_bank == 0x60) new_bank++;
			switch_rom_bank(new_bank);
		} break;
		case MBC_3: {
			assert(value <= 0x7f);
			if (value == 0x00) value++;
			switch_rom_bank(value);
		} break;
		default: assert(false); break;
	}
}

void mem_init(const char *rom_file_path) {
	strcpy(current_rom_file_path, rom_file_path);
	
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
	for (int i = 0xff10; i <= 0xff26; i++) mem_write(i, 0x00); // zero audio
	mem_write(0xff20, 0xff);
	mem_write(0xff21, 0x00);
	mem_write(0xff22, 0x00);
	mem_write(0xff23, 0xbf);
	mem_write(0xff24, 0x77);
	mem_write(0xff25, 0xf3);
	mem_write(0xff26, 0xf1); // NOTE: This is different for Game Boy Color etc.
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
	mem_write(IF_ADDRESS, 0xe1); // TODO: Might be acceptable for this to be 0xe0
	mem_write(IE_ADDRESS, 0x00);
	
	const int single_bank_size = 1024 * 16; // 16kB
	const int bank_0_destination = 0x0000;
	
	char buf[64] = {0};
	sprintf(buf, "Loading initial ROM data, loading %iKB from file", single_bank_size/1024);
	robingb_log(buf);
	robingb_read_file(current_rom_file_path, 0, single_bank_size, &memory[bank_0_destination]);
	
	current_rom_bank = 0;
	int cart_type = mem_read(0x0147);
	
	switch (cart_type) {
		case CART_TYPE_ROM_MBC1:
		case CART_TYPE_ROM_MBC1_RAM:
		case CART_TYPE_ROM_MBC1_RAM_BATTERY:
			mbc_type = MBC_1;
		break;
		case CART_TYPE_ROM_MBC3:
		case CART_TYPE_ROM_MBC3_RAM:
		case CART_TYPE_ROM_MBC3_RAM_BATTERY:
		case CART_TYPE_ROM_MBC3_TIMER_BATTERY:
		case CART_TYPE_ROM_MBC3_TIMER_RAM_BATTERY:
			mbc_type = MBC_3;
		break;
		default: assert(false); break;
	}
	
	switch_rom_bank(1);
}

Mem_Address_Description mem_get_address_description(int address) {
	Mem_Address_Description desc;
	desc.region[0] = '\0';
	desc.byte[0] = '\0';
	
	if (address >= 0x0000 && address <= 0x3fff) strcpy(desc.region, "Cart ROM, Bank 0");
	else if (address >= 0x4000 && address <= 0x7fff) strcpy(desc.region, "Cart ROM, Switchable Bank");
	else if (address >= 0x8000 && address <= 0x9fff) strcpy(desc.region, "VRAM");
	else if (address >= 0xa000 && address <= 0xbfff) strcpy(desc.region, "External RAM");
	else if (address >= 0xc000 && address <= 0xfdff) strcpy(desc.region, "WRAM"); // all WRAM including echo.
	else if (address >= 0xfe00 && address <= 0xfe9f) strcpy(desc.region, "OAM");
	else if (address >= 0xfea0 && address <= 0xfeff) strcpy(desc.region, "Not Usable");
	else if (address >= 0xff00 && address <= 0xff7f) {
		if (address >= 0xff10 && address <= 0xff26) {
			
			// if (address == 0xff24) sprintf(buf, "%s, Sound L/R channel control (NR50)", full_prefix);
			// else if (address == 0xff25) sprintf(buf, "%s, Sound output terminal selection (NR51)", full_prefix);
			// else if (address == 0xff26) sprintf(buf, "%s, Sound on/off (NR52)", full_prefix);
			strcpy(desc.region, "Sound");
			
		} else if (address >= 0xff40 && address <= 0xff49) {
			strcpy(desc.region, "LCD");
			
			if (address == 0xff40) strcpy(desc.byte, "Control");
			else if (address == 0xff41) strcpy(desc.byte, "Status");
			else if (address == 0xff42) strcpy(desc.byte, "Scroll Y");
			else if (address == 0xff43) strcpy(desc.byte, "Scroll X");
			else if (address == 0xff44) strcpy(desc.byte, "LY");
			else if (address == 0xff45) strcpy(desc.byte, "LYC");
			else if (address == 0xff46) strcpy(desc.byte, "DMA Transfer and Start Address");
			else if (address == 0xff47) strcpy(desc.byte, "BGP");
			else if (address == 0xff48) strcpy(desc.byte, "OBP0");
			else if (address == 0xff49) strcpy(desc.byte, "OBP1");
			else if (address == 0xff4a) strcpy(desc.byte, "WY");
			else if (address == 0xff4b) strcpy(desc.byte, "WX");
			
		} else strcpy(desc.region, "I/O");
		
		if (address == 0xff00) strcpy(desc.byte, "Joypad");
	}
	else if (address >= 0xff80 && address <= 0xfffe) {
		strcpy(desc.region, "HRAM");
	// } else if (address == 0xffff) {
	// 	sprintf(buf, "%s, Interrupts Enable Register (IE)", full_prefix);
	// } else {
	// 	sprintf(buf, "%s", full_prefix);
	}
	
	return desc;
}

void perform_cart_control(int address, u8 value) {
	if (address >= 0x0000 && address < 0x2000) {
		// MBC1: external RAM control (at 0xa000 to 0xbfff)
		assert(false); 
	} else if (address >= 0x2000 && address < 0x4000) {
		
		calculate_and_switch_rom_bank(address, value);
		
	} else if (address >= 0x4000 && address < 0x6000) {
		assert(false); // MBC1: RAM bank control, or, upper bits of ROM bank number, depending on ROM/RAM mode
	} else if (address >= 0x6000 && address < 0x8000) {
		assert(false); // MBC1: ROM/RAM mode select
	} else assert(false);
}

Mem_Log mem_logs[MEM_MAX_NUM_LOGS];
int mem_num_logs = 0;
bool mem_logging_enabled = true;

void mem_get_logs(Mem_Log logs_out[], int *num_logs_out) {
	memcpy(logs_out, mem_logs, sizeof(Mem_Log) * mem_num_logs);
	*num_logs_out = mem_num_logs;
}

void mem_remove_all_logs() {
	mem_num_logs = 0;
}

void mem_add_log(u16 address, u8 value, bool is_write, bool is_echo) {
	assert(mem_num_logs < MEM_MAX_NUM_LOGS);
	
	mem_logs[mem_num_logs].address = address;
	mem_logs[mem_num_logs].value = value;
	mem_logs[mem_num_logs].is_write = is_write;
	mem_logs[mem_num_logs].is_echo = is_echo;
	mem_num_logs++;
}

u8 mem_read(int address) {
	u8 value = memory[address];
	if (mem_logging_enabled) mem_add_log(address, value, false, false);
	return value;
}

u16 mem_read_u16(int address) {
	u16 out;
	u8 *bytes = (u8*)&out;
	bytes[0] = mem_read(address);
	bytes[1] = mem_read(address+1);
	return out;
}

void mem_write(int address, u8 value) {
	if (mem_logging_enabled) mem_add_log(address, value, true, false);
	
	if (address < 0x8000) {
		perform_cart_control(address, value);
	} else if (address == 0xff46) {
		memcpy(&memory[0xfe00], &memory[value * 0x100], 160); // OAM DMA transfer
	} else {
		memory[address] = value;
		
		if (address >= 0xc000 && address < 0xde00) {
			int echo_address = address-0xc000+0xe000;
			memory[echo_address] = value;
			if (mem_logging_enabled) mem_add_log(echo_address, value, true, true);
		} else if (address >= 0xe000 && address < 0xfe00) {
			int echo_address = address-0xe000+0xc000;
			memory[echo_address] = value;
			if (mem_logging_enabled) mem_add_log(echo_address, value, true, true);
		}
	}
}

void mem_write_u16(int address, u16 value) {
	u8 *values = (u8*)&value;
	mem_write(address, values[0]);
	mem_write(address+1, values[1]);
}









