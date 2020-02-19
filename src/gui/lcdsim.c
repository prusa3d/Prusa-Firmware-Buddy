// lcdsim.c
#include "lcdsim.h"
#include <string.h>
#include "dbg.h"

#define LCD_DDRAM_SIZE 0x80
#define LCD_CGRAM_SIZE 0x40

#define LCD_CMD_CLEARDISPLAY   0x01
#define LCD_CMD_RETURNHOME     0x02
#define LCD_CMD_ENTRYMODESET   0x04
#define LCD_CMD_DISPLAYCONTROL 0x08
#define LCD_CMD_CURSORSHIFT    0x10
#define LCD_CMD_FUNCTIONSET    0x20
#define LCD_CMD_SETCGRAMADDR   0x40
#define LCD_CMD_SETDDRAMADDR   0x80

#define LCD_FLG_FUNC8BIT      0x10
#define LCD_FLG_FUNC2LINE     0x08
#define LCD_FLG_FUNC10DOTS    0x04
#define LCD_FLG_SHIFTDISPLAY  0x08
#define LCD_FLG_SHIFTRIGHT    0x04
#define LCD_FLG_DISPLAYON     0x04
#define LCD_FLG_CURSORON      0x02
#define LCD_FLG_BLINKON       0x01
#define LCD_FLG_ENTRYLEFT     0x01
#define LCD_FLG_ENTRYSHIFTINC 0x02

#define LCD_EXPANDER_RS 0x01
#define LCD_EXPANDER_RW 0x02
#define LCD_EXPANDER_EN 0x04
#define LCD_EXPANDER_BL 0x08

uint8_t lcdsim_expander; // expander output

uint8_t lcdsim_ddram[LCD_DDRAM_SIZE];
uint8_t lcdsim_cgram[LCD_CGRAM_SIZE];
int8_t lcdsim_ddram_addr;
int8_t lcdsim_cgram_addr;
uint32_t lcdsim_inval_mask[LCDSIM_ROWS];

#if (LCDSIM_ROWS == 4)
const uint8_t lcdsim_row_addr[4] = { 0x00, 0x40, 0x14, 0x54 };
#endif

int8_t lcdsim_shift = 0;

int lcdsim_func8bit = 1;
int lcdsim_func2line = 1;
int lcdsim_func10dots = 1;
int lcdsim_displayOn = 1;
int lcdsim_cursorOn = 1;
int lcdsim_blinkOn = 1;
int lcdsim_entryLeft = 0;
int lcdsim_entryShiftInc = 0;

void lcdsim_cmd_clearDisplay(void);

static inline uint8_t lcdsim_addr2row(uint8_t addr) {
    return ((addr & 0x40) >> 6) | (((addr & 0x3f) >= 0x14) ? 2 : 0);
}

static inline uint8_t lcdsim_addr2col(uint8_t addr) {
    return (addr & 0x3f) % 20;
}

static inline uint8_t lcdsim_pos2addr(uint8_t col, uint8_t row) {
    return lcdsim_row_addr[row & 0x03] + (col % 20);
}

void lcdsim_init(void) {
    int i;
    for (i = 0; i < LCD_CGRAM_SIZE; i++)
        lcdsim_cgram[i] = 0x00;
    lcdsim_expander = 0;
    lcdsim_cmd_clearDisplay();
}

void lcdsim_invalidate(void) {
    int i;
    for (i = 0; i < 4; i++)
        lcdsim_inval_mask[i] = 0x000fffff;
}

uint8_t lcdsim_char_at(uint8_t col, uint8_t row) {
    return lcdsim_ddram[lcdsim_pos2addr(col, row)];
}

uint8_t *lcdsim_user_charset_ptr(void) {
    return lcdsim_cgram;
}

uint16_t lcdsim_grab_text(char *text) {
    int row;
    int col;
    int i = 0;
    for (row = 0; row < 4; row++) {
        for (col = 0; col < 20; col++)
            text[i + col] = lcdsim_char_at(col, row);
        for (col = 19; col >= 0; col--)
            if (text[i + col] != ' ')
                break;
        i += col + 1;
        text[i++] = '\n';
    }
    text[i] = 0;
    return i;
}

void lcdsim_cmd_setDDRamAddr(int8_t addr) {
    lcdsim_cgram_addr = -1;   //stop CGRam write
    lcdsim_ddram_addr = addr; //set address
}

void lcdsim_cmd_setCGRamAddr(int8_t addr) {
    lcdsim_cgram_addr = addr; //set address
}

void lcdsim_cmd_functionSet(int func8bit, int func2line, int func10dots) {
    lcdsim_func8bit = func8bit;
    lcdsim_func2line = func2line;
    lcdsim_func10dots = func10dots;
}

void lcdsim_cmd_cursorShift(int shiftDisplay, int shiftRight) {
    lcdsim_cgram_addr = -1;
    if (shiftDisplay) {
        lcdsim_shift += shiftRight ? 1 : -1;
        lcdsim_shift &= 0x3f;
    } else {
        lcdsim_ddram_addr += shiftRight ? 1 : -1;
        lcdsim_ddram_addr &= 0x7f;
    }
}

void lcdsim_cmd_displayControl(int displayOn, int cursorOn, int blinkOn) {
    lcdsim_displayOn = displayOn;
    lcdsim_cursorOn = cursorOn;
    lcdsim_blinkOn = blinkOn;
}

void lcdsim_cmd_entryModeSet(int entryLeft, int entryShiftInc) {
    lcdsim_entryLeft = entryLeft;
    lcdsim_entryShiftInc = entryShiftInc;
}

void lcdsim_cmd_returnHome(void) {
    lcdsim_cgram_addr = -1;
    lcdsim_ddram_addr = 0;
    lcdsim_shift = 0;
}

void lcdsim_cmd_clearDisplay(void) {
    int i;
    //for (i = 0; i < LCD_CGRAM_SIZE; i++)
    //	lcdsim_cgram[i] = 0x00;
    for (i = 0; i < LCD_DDRAM_SIZE; i++)
        lcdsim_ddram[i] = ' ';
    lcdsim_cgram_addr = -1;
    lcdsim_ddram_addr = 0;
    lcdsim_shift = 0;
    lcdsim_invalidate();
}

void lcdsim_wr_cmd(uint8_t cmd) {
    if ((cmd & LCD_CMD_SETDDRAMADDR) != 0)
        lcdsim_cmd_setDDRamAddr((uint8_t)(cmd & 0x7f));
    else if ((cmd & LCD_CMD_SETCGRAMADDR) != 0)
        lcdsim_cmd_setCGRamAddr((uint8_t)(cmd & 0x3f));
    else if ((cmd & LCD_CMD_FUNCTIONSET) != 0)
        lcdsim_cmd_functionSet((cmd & LCD_FLG_FUNC8BIT) != 0, (cmd & LCD_FLG_FUNC2LINE) != 0, (cmd & LCD_FLG_FUNC10DOTS) != 0);
    else if ((cmd & LCD_CMD_CURSORSHIFT) != 0)
        lcdsim_cmd_cursorShift((cmd & LCD_FLG_SHIFTDISPLAY) != 0, (cmd & LCD_FLG_SHIFTRIGHT) != 0);
    else if ((cmd & LCD_CMD_DISPLAYCONTROL) != 0)
        lcdsim_cmd_displayControl((cmd & LCD_FLG_DISPLAYON) != 0, (cmd & LCD_FLG_DISPLAYON) != 0, (cmd & LCD_FLG_BLINKON) != 0);
    else if ((cmd & LCD_CMD_ENTRYMODESET) != 0)
        lcdsim_cmd_entryModeSet((cmd & LCD_FLG_ENTRYLEFT) != 0, (cmd & LCD_FLG_ENTRYSHIFTINC) != 0);
    else if ((cmd & LCD_CMD_RETURNHOME) != 0)
        lcdsim_cmd_returnHome();
    else if ((cmd & LCD_CMD_CLEARDISPLAY) != 0)
        lcdsim_cmd_clearDisplay();
}

void lcdsim_wr_data(uint8_t data) {
    if (lcdsim_cgram_addr >= 0) {
        lcdsim_cgram[lcdsim_cgram_addr++] = data;
        lcdsim_cgram_addr &= 0x3f;
    } else {
        lcdsim_ddram[lcdsim_ddram_addr] = data;
        uint8_t row = lcdsim_addr2row(lcdsim_ddram_addr);
        uint8_t col = lcdsim_addr2col(lcdsim_ddram_addr);
        lcdsim_inval_mask[row] |= (1 << col);
        lcdsim_ddram_addr += lcdsim_entryLeft ? -1 : 1;
        lcdsim_ddram_addr &= 0x7f;
    }
}

//called from Wire.cpp (arduino i2c encapsulation)
// bit 0    - RS, register select
// bit 1    - RW, read/write (unused)
// bit 2    - EN, enable
// bit 3    - BL, backlight control
// bit 4..7 - DB4..DB7 (4bit mode)
void lcdsim_expander_write(uint8_t expander) {
    static uint8_t d4_7 = 0;
    uint8_t d0_7;
    uint8_t change;
    if (lcdsim_expander != expander) {
        change = expander ^ lcdsim_expander;
        if (change & LCD_EXPANDER_RS)
            d4_7 = 0;                                     //reset to first nibble if RS changed
        if (change & lcdsim_expander & LCD_EXPANDER_EN) { // EN falling edge
            if (d4_7) {                                   //second (low) nibble
                d0_7 = (d4_7 & 0xf0) | (expander >> 4);
                if (expander & LCD_EXPANDER_RS)
                    lcdsim_wr_data(d0_7); //data
                else
                    lcdsim_wr_cmd(d0_7); //command
                d4_7 = 0;
            } else //high nibble
                d4_7 = expander | 0x0f;
        }
        lcdsim_expander = expander;
    }
}
