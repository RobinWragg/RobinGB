#include "internal.h"

#define ROM_BANK_SIZE 16384 /* 16kB */

void romb_init_first_rom_banks(const char *file_path) {
	/* Load ROM banks 0 and 1 */
	robingb_log("Loading the first 2 ROM banks...");
	robingb_read_file(file_path, 0, ROM_BANK_SIZE*2, robingb_memory);
	robingb_log("Done");
}








