#ifndef ROBINGB_INTERNAL_H
#define ROBINGB_INTERNAL_H

#include "RobinGB.h"

typedef float f32;
typedef double f64;
typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;

#define DEBUG_set_opcode_name(x) /* robingb_log(x) */

#define bit(n) (0x01 << n)

#define FLAG_Z (0x80) /* Zero Flag */
#define FLAG_N (0x40) /* Add/Sub-Flag (BCD) */
#define FLAG_H (0x20) /* Half Carry Flag (BCD) */
#define FLAG_C (0x10) /* Carry Flag */

#define LCD_CONTROL_ADDRESS 0xff40 /* "LCDC" */
#define LCD_STATUS_ADDRESS 0xff41
#define LCD_LY_ADDRESS 0xff44
#define LCD_LYC_ADDRESS 0xff45

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
			struct { u8 f; u8 a; };
			u16 af;
		};
	};
	struct {
		union {
			struct { u8 c; u8 b; };
			u16 bc;
		};
	};
	struct {
		union {
			struct { u8 e; u8 d; };
			u16 de;
		};
	};
	struct {
		union {
			struct { u8 l; u8 h; };
			u16 hl;
		};
	};
	u16 sp;
	u16 pc;
	bool ime;
} Registers;

typedef enum {
	MBC_NONE,
	MBC_1,
	MBC_2,
	MBC_3
} Mbc_Type;

extern u8 *robingb_screen;

extern Registers registers;
extern bool halted;

void robingb_request_interrupt(u8 interrupts_to_request);
void robingb_handle_interrupts();
void robingb_stack_push(u16 value);
u16 robingb_stack_pop();
void robingb_execute_next_opcode(u8 *num_cycles_out);
void robingb_execute_cb_opcode();
void robingb_finish_instruction(s16 pc_increment, u8 num_cycles_param);

extern u8 robingb_memory[];
void robingb_memory_init();
u8 robingb_memory_read(u16 address);
u16 robingb_memory_read_u16(u16 address);
void robingb_memory_write(u16 address, u8 value);
void robingb_memory_write_u16(u16 address, u16 value);

void robingb_romb_init_first_banks(
	const char *file_path,
	void (*read_file_function_ptr)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[])
	);
void robingb_romb_init_additional_banks();
void robingb_romb_perform_bank_control(int address, u8 value, Mbc_Type mbc_type);
u8 robingb_romb_read_switchable_bank(u16 address);

void robingb_lcd_update(int num_cycles_passed);
u8 robingb_respond_to_joypad_register(u8 new_value);
void robingb_timer_init();
u8 robingb_respond_to_timer_div_register();
void robingb_timer_update(u8 num_cycles_delta);
void robingb_audio_init(u32 sample_rate);
void robingb_audio_update(u32 num_cycles);
void robingb_render_screen_line();

#endif
