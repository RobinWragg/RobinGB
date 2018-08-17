// TODO: Prevent name clashes with user code (RobinGB prefixes, static definitions in .c files)

#ifndef ROBINGB_INTERNAL_H
#define ROBINGB_INTERNAL_H

#include "RobinGB.h"

typedef float f32;
typedef double f64;
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;

#define robingb_log(x) robingb_log_with_prefix(__func__, x)

#define bit(n) (0x01 << n)

#define CPU_CLOCK_FREQ (4194304)

#define MEM_MAX_NUM_LOGS (1024)

#define FLAG_Z (0x80) // Zero Flag
#define FLAG_N (0x40) // Add/Sub-Flag (BCD)
#define FLAG_H (0x20) // Half Carry Flag (BCD)
#define FLAG_C (0x10) // Carry Flag

#define LCD_CONTROL_ADDRESS (0xff40)
#define LCD_STATUS_ADDRESS (0xff41)

#define IF_ADDRESS (0xff0f)
#define IE_ADDRESS (0xffff)

#define INTERRUPT_FLAG_VBLANK (0x01)
#define INTERRUPT_FLAG_LCD_STAT (0x02)
#define INTERRUPT_FLAG_TIMER (0x04)
#define INTERRUPT_FLAG_SERIAL (0x08)
#define INTERRUPT_FLAG_JOYPAD (0x10)

typedef struct {
	struct {
		union {
			struct { uint8_t f; uint8_t a; };
			u16 af;
		};
	};
	struct {
		union {
			struct { uint8_t c; uint8_t b; };
			u16 bc;
		};
	};
	struct {
		union {
			struct { uint8_t e; uint8_t d; };
			u16 de;
		};
	};
	struct {
		union {
			struct { uint8_t l; uint8_t h; };
			u16 hl;
		};
	};
	u16 sp;
	u16 pc;
	bool ime;
} Registers;

void print_binary(uint8_t value);

typedef struct {
	u16 address;
	uint8_t value;
	bool is_write, is_echo;
} Mem_Log;

typedef struct {
	char region[1024];
	char byte[1024];
} Mem_Address_Description;

extern void (*robingb_read_file)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[]);

extern Registers registers;
extern bool halted;
extern bool mem_logging_enabled;
void request_interrupt(uint8_t interrupts_to_request);
void handle_interrupts();
void stack_push(u16 value);
u16 stack_pop();
void execute_opcode(uint8_t opcode, uint8_t *num_cycles_out, char *asm_log_out);
void execute_cb_opcode();
void finish_instruction(const char *desc, u16 pc_increment, u8 num_cycles_param);
void mem_get_logs(Mem_Log logs_out[], int *num_logs_out);
void mem_remove_all_logs();
Mem_Address_Description mem_get_address_description(int address);
void mem_init(const char *rom_file_path);
uint8_t mem_read(int address);
u16 mem_read_u16(int address);
void mem_write(int address, uint8_t value);
void mem_write_u16(int address, u16 value);
void lcd_update(int num_cycles_passed);
void joypad_update(RobinGB_Input *input);
void init_timer();
void update_timer(uint8_t num_cycles_delta);
void update_audio(int num_cycles);
void render_screen_line(uint8_t ly);

#endif
