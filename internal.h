#ifndef ROBINGB_INTERNAL_H
#define ROBINGB_INTERNAL_H

/* Declarations for identifiers used across compilation units. */

#include "RobinGB.h"

#define GAME_BOY_MEMORY_ADDRESS_SPACE_SIZE (1024*64)

#define DEBUG_set_opcode_name(x) /* robingb_log(x) */

#define robingb_bit(n) (0x01 << n)

#define FLAG_Z (0x80) /* Zero Flag */
#define FLAG_N (0x40) /* Add/Sub-Flag (BCD) */
#define FLAG_H (0x20) /* Half Carry Flag (BCD) */
#define FLAG_C (0x10) /* Carry Flag */

#define LCD_CONTROL_ADDRESS 0xff40 /* "LCDC" */
#define LCD_STATUS_ADDRESS 0xff41
#define LCD_LY_ADDRESS 0xff44
#define LCD_LYC_ADDRESS 0xff45

#define INTERRUPT_FLAG_ADDRESS (0xff0f)
#define INTERRUPT_ENABLE_ADDRESS (0xffff)

#define INTERRUPT_FLAG_VBLANK (0x01)
#define INTERRUPT_FLAG_LCD_STAT (0x02)
#define INTERRUPT_FLAG_TIMER (0x04)
#define INTERRUPT_FLAG_SERIAL (0x08)
#define INTERRUPT_FLAG_JOYPAD (0x10)

typedef struct {
    struct {
        union {
            struct { uint8_t f; uint8_t a; };
            uint16_t af;
        };
    };
    struct {
        union {
            struct { uint8_t c; uint8_t b; };
            uint16_t bc;
        };
    };
    struct {
        union {
            struct { uint8_t e; uint8_t d; };
            uint16_t de;
        };
    };
    struct {
        union {
            struct { uint8_t l; uint8_t h; };
            uint16_t hl;
        };
    };
    uint16_t sp;
    uint16_t pc;
    bool ime;
} Registers;

typedef enum {
    MBC_NONE,
    MBC_1,
    MBC_2,
    MBC_3
} Mbc_Type;

extern uint8_t *robingb_screen;

extern bool (*robingb_read_file)(const char *path, uint32_t offset, uint32_t size, uint8_t buffer[]);
extern bool (*robingb_write_file)(const char *path, bool append, uint32_t size, uint8_t buffer[]);
extern char *robingb_cart_path;
extern char *robingb_save_path;
extern Registers registers;
extern bool halted;

void robingb_request_interrupt(uint8_t interrupts_to_request);
void robingb_handle_interrupts();
void robingb_stack_push(uint16_t value);
uint16_t robingb_stack_pop();
void robingb_execute_next_opcode(uint8_t *num_cycles_out);
void robingb_execute_cb_opcode();
void robingb_finish_instruction(int16_t pc_increment, uint8_t num_cycles_param);

extern uint8_t robingb_memory[];
void robingb_memory_init();
uint8_t robingb_memory_read(uint16_t address);
uint16_t robingb_memory_read_u16(uint16_t address);
void robingb_memory_write(uint16_t address, uint8_t value);
void robingb_memory_write_u16(uint16_t address, uint16_t value);

void robingb_romb_init_first_banks();
void robingb_romb_init_additional_banks();
void robingb_romb_perform_bank_control(int address, uint8_t value, Mbc_Type mbc_type);
uint8_t robingb_romb_read_switchable_bank(uint16_t address);

void robingb_lcd_update(int num_cycles_passed);
uint8_t robingb_respond_to_joypad_register(uint8_t new_value);
void robingb_timer_init();
uint8_t robingb_respond_to_timer_div_register();
void robingb_timer_update(uint8_t num_cycles_delta);
void robingb_audio_init(uint32_t sample_rate);
void robingb_audio_update(uint32_t num_cycles);
void robingb_render_screen_line();

#endif
