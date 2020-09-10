// term.h

/// This unit provides write-only terminal.
/// Window in terms of fixed-sized chars is defined.
/// Long lines are wrapped automatically.
/// If a line does not fit to window,
/// first line is deleted and the others are
/// shifted up so the new line can be printed.

#pragma once

#include <inttypes.h>
#include <stdio.h>

// FIXME define meaningful numbers for masks
enum {
    TERM_ATTR_BACK_MASK = 0x00,
    TERM_ATTR_BACK_BLACK = 0x00,
    TERM_ATTR_BACK_WHITE = 0x00,

    TERM_ATTR_TEXT_MASK = 0x00,
    TERM_ATTR_TEXT_BLACK = 0x00,
    TERM_ATTR_TEXT_WHITE = 0x00,

    TERM_ATTR_INVERT = 0x40,
    TERM_ATTR_BLINK = 0x80,
};

enum {
    TERM_COLOR_BLACK,
    TERM_COLOR_RED,
    TERM_COLOR_GREEN,
    TERM_COLOR_YELLOW,
    TERM_COLOR_BLUE,
    TERM_COLOR_MAGENTA,
    TERM_COLOR_CYAN,
    TERM_COLOR_WHITE,
};

static const uint16_t TERM_FLG_CHANGED = 0x40;
static const uint16_t TERM_FLG_ESCAPE = 0x20;
static const uint16_t TERM_FLG_AUTOCR = 0x10;

static const char TERM_DEF_CHAR = ' ';
enum {
    TERM_DEF_ATTR = TERM_ATTR_BACK_BLACK | TERM_ATTR_TEXT_WHITE
};

#define TERM_BUFF_SIZE(c, r) ((r * c * 2) + (r * c + 7) / 8)

#define TERM_PRINTF_MAX 0xff

typedef struct _term_t {
    // 4 bytes
    FILE *file;
    uint8_t *buff;
    // 2 bytes
    uint16_t flg;
    uint16_t size;
    // 1 byte
    uint8_t cols;
    uint8_t rows;
    uint8_t attr;
    uint8_t col;
    uint8_t row;

} term_t;

/// Initializes a terminal
/// \param cols number of columns (fixed-sized chars)
/// \param rows number of rows (fixed-sized chars)
/// \param buff is pre-allocated or nullptr
/// if pre-allocated the size has to be
/// at least  \param cols + \param rows + 1
extern void term_init(term_t *pt, uint8_t cols, uint8_t rows, uint8_t *buff);

extern void term_done(term_t *pt);

extern void term_clear(term_t *pt);

extern uint8_t term_get_char_at(term_t *pt, uint8_t col, uint8_t row);

extern void term_set_char_at(term_t *pt, uint8_t col, uint8_t row, uint8_t ch);

extern uint8_t term_get_attr_at(term_t *pt, uint8_t col, uint8_t row);

extern void term_set_attr_at(term_t *pt, uint8_t col, uint8_t row, uint8_t attr);

extern void term_set_attr(term_t *pt, uint8_t attr);

extern void term_set_pos(term_t *pt, uint8_t col, uint8_t row);

extern void term_write_char(term_t *pt, uint8_t ch);

extern int term_printf(term_t *pt, const char *fmt, ...);
