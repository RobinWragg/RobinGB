#include "internal.h"
#include <assert.h>

/* TODO: "Each bit is set to 1 automatically when an internal signal from that subsystem goes from '0' to '1', it doesn't matter if the corresponding bit in IE is set. This is specially important in the case of LCD STAT interrupt, as it will be explained in the video controller chapter." */

/* TODO: "When using a status interrupt in DMG or in CGB in DMG mode, register IF should be set to 0 after the value of the STAT register is set. (In DMG, setting the STAT register value changes the value of the IF register, and an interrupt is generated at the same time as interrupts are enabled.)" */

/*
TODO:
"8.7. STAT Interrupt
This interrupt can be configured with register STAT.
The STAT IRQ is trigged by an internal signal.
This signal is set to 1 if:
    ( (LY = LYC) AND (STAT.ENABLE_LYC_COMPARE = 1) ) OR
    ( (ScreenMode = 0) AND (STAT.ENABLE_HBL = 1) ) OR
    ( (ScreenMode = 2) AND (STAT.ENABLE_OAM = 1) ) OR
    ( (ScreenMode = 1) AND (STAT.ENABLE_VBL || STAT.ENABLE_OAM) ) -> Not only VBL!!??
-If LCD is off, the signal is 0.
-It seems that this interrupt needs less time to execute in DMG than in CGB? -DMG bug?"
*/

/* TODO: can the game change LY? */

#define LCDC_ENABLED_BIT (0x01 << 7)

#define NUM_CYCLES_PER_FULL_SCREEN_REFRESH 70224 /* Approximately 59.7275Hz */
#define NUM_CYCLES_PER_LY_INCREMENT 456
#define LY_VBLANK_ENTRY_VALUE 144
#define LY_MAXIMUM_VALUE 154
#define MODE_0_CYCLE_DURATION 204
#define MODE_1_CYCLE_DURATION 4560
#define MODE_2_CYCLE_DURATION 80
#define MODE_3_CYCLE_DURATION 172

static uint8_t *control = &robingb_memory[LCD_CONTROL_ADDRESS];
static uint8_t *status = &robingb_memory[LCD_STATUS_ADDRESS];
static uint8_t *ly = &robingb_memory[LCD_LY_ADDRESS];
static uint8_t *lyc = &robingb_memory[LCD_LYC_ADDRESS];

void robingb_lcd_update(int num_cycles_delta) {
    if (((*control) & LCDC_ENABLED_BIT) == 0) {
        /* Bit 7 of the LCD control register is 0, so the LCD is switched off. */
        /* LY, the mode, and the LYC=LY flag should all be 0. */
        *ly = 0x00;
        *status &= 0xf8;
        return; 
    }
    
    static int32_t elapsed_cycles = 0;
    elapsed_cycles += num_cycles_delta;
    
    /* set LY */
    if (elapsed_cycles >= NUM_CYCLES_PER_LY_INCREMENT) {
        elapsed_cycles -= NUM_CYCLES_PER_LY_INCREMENT;
        
        if (++(*ly) >= LY_MAXIMUM_VALUE) *ly = 0;
    }
    
    /* handle LYC */
    if (*ly == *lyc) {
        *status |= 0x04;
        if ((*status) & 0x40) robingb_request_interrupt(INTERRUPT_FLAG_LCD_STAT);
    } else {
        *status &= ~0x04;
    }
    
    /* set mode */
    const uint8_t prev_mode = (*status) & 0x03; /* get lower 2 bits only */
    *status &= 0xfc; /* wipe the old mode */
    
    if (*ly < LY_VBLANK_ENTRY_VALUE) {
        /*
        Approx mode graph:
        Mode 2  2_____2_____2_____2_____2_____2___________________2____
        Mode 3  _33____33____33____33____33____33__________________3___
        Mode 0  ___000___000___000___000___000___000________________000
        Mode 1  ____________________________________11111111111111_____
        */
        
        if (elapsed_cycles >= MODE_2_CYCLE_DURATION + MODE_3_CYCLE_DURATION) {
            *status |= 0x00; /* H-blank */
            
            if (prev_mode != 0x00 && ((*status) & 0x08)) {
                robingb_request_interrupt(INTERRUPT_FLAG_LCD_STAT);
            }
        } else if (elapsed_cycles >= MODE_2_CYCLE_DURATION) {
            *status |= 0x03; /* The LCD is reading from both OAM and VRAM */
            
            if (prev_mode != 0x03) robingb_render_screen_line();
        } else {
            *status |= 0x02; /* The LCD is reading from OAM */
            
            if (prev_mode != 0x02 && ((*status) & 0x20)) {
                robingb_request_interrupt(INTERRUPT_FLAG_LCD_STAT);
            }
        }
        
    } else {
        *status |= 0x01; /* V-blank */
        
        if (prev_mode != 0x01) {
            robingb_request_interrupt(INTERRUPT_FLAG_VBLANK);
            if ((*status) & 0x10) robingb_request_interrupt(INTERRUPT_FLAG_LCD_STAT);
        }
    }
}







