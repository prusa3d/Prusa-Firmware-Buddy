// term.cpp
#include <stdarg.h>
#include <algorithm>

#include "term.h"
#include "gui.hpp"
#include "bsod.h"

void term_init(term_t *pt, uint8_t cols, uint8_t rows, uint8_t *buff) {
    if (!pt /*|| pt->buff*/) {
        return;
    }
    pt->cols = cols;
    pt->rows = rows;
    pt->size = cols * rows * 2;
    pt->flg = TERM_FLG_AUTOCR;
    if (buff) {
        pt->buff = buff;
    } else {
        bsod("Terminal buffer invalid");
    }
    pt->attr = TERM_DEF_ATTR;
    pt->col = 0;
    pt->row = 0;
    pt->file = 0;
    term_clear(pt);
}

void term_done(term_t *pt) {
    if (!pt || !(pt->buff)) {
        return;
    }
}

void term_clear(term_t *pt) {
    if (pt == nullptr || pt->buff == nullptr) {
        return;
    }
    uint8_t *p = pt->buff;
    int c;
    for (int r = 0; r < pt->rows; r++) {
        for (c = 0; c < pt->cols; c++) {
            *(p++) = TERM_DEF_CHAR;
            *(p++) = TERM_DEF_ATTR;
        }
    }
    memset(pt->buff + pt->size, 0xff, 40 /*(pt->size + 15) >> 4*/);
    pt->col = 0;
    pt->row = 0;
    pt->flg |= TERM_FLG_CHANGED;
}

uint8_t term_get_char_at(term_t *pt, uint8_t col, uint8_t row) {
    if (!pt || !(pt->buff)) {
        return 0;
    }
    if ((col >= pt->cols) || (row >= pt->rows)) {
        return 0;
    }
    int i = col + row * pt->cols;
    return pt->buff[2 * i + 0];
}

void term_set_char_at(term_t *pt, uint8_t col, uint8_t row, uint8_t ch) {
    if (!pt || !(pt->buff)) {
        return;
    }
    if ((col >= pt->cols) || (row >= pt->rows)) {
        return;
    }
    int i = col + row * pt->cols;
    pt->buff[2 * i + 0] = ch;
    pt->buff[pt->size + (i >> 3)] |= (1 << (i % 8));
    pt->flg |= TERM_FLG_CHANGED;
}

uint8_t term_get_attr_at(term_t *pt, uint8_t col, uint8_t row) {
    if (!pt || !(pt->buff)) {
        return 0;
    }
    if ((col >= pt->cols) || (row >= pt->rows)) {
        return 0;
    }
    int i = col + row * pt->cols;
    return pt->buff[2 * i + 1];
}

void term_set_attr_at(term_t *pt, uint8_t col, uint8_t row, uint8_t attr) {
    if (!pt || !(pt->buff)) {
        return;
    }
    if ((col >= pt->cols) || (row >= pt->rows)) {
        return;
    }
    int i = col + row * pt->cols;
    pt->buff[2 * i + 1] = attr;
    pt->buff[pt->size + (i >> 3)] |= (1 << (i % 8));
    pt->flg |= TERM_FLG_CHANGED;
}

uint8_t term_get_attr(term_t *pt) {
    if (!pt || !(pt->buff)) {
        return 0;
    }
    return pt->attr;
}

void term_set_attr(term_t *pt, uint8_t attr) {
    if (!pt || !(pt->buff)) {
        return;
    }
    pt->attr = attr;
}

void term_set_pos(term_t *pt, uint8_t col, uint8_t row) {
    if (!pt || !(pt->buff)) {
        return;
    }
    if (col >= pt->cols) {
        col = pt->cols - 1;
    }
    if (row >= pt->rows) {
        row = pt->rows - 1;
    }
    pt->col = col;
    pt->row = row;
}

void term_scroll_up(term_t *pt) {
    if (!pt || !(pt->buff)) {
        return;
    }
    memcpy(pt->buff, pt->buff + 2 * pt->cols, pt->cols * (pt->rows - 1) * 2);
    pt->row--;
    uint8_t *p = pt->buff + pt->size - 2 * pt->cols;
    for (int c = 0; c < pt->cols; c++) {
        *(p++) = TERM_DEF_CHAR;
        *(p++) = TERM_DEF_ATTR;
    }
    memset(pt->buff + pt->size, 0xff, (pt->size + 15) >> 4);
}

void term_write_escape_char(term_t *pt, uint8_t ch) {
    if (ch == 0x1b) {
        pt->flg |= TERM_FLG_ESCAPE;
    }
}

void term_write_CR(term_t *pt) {
    pt->col = 0;
}

void term_write_LF(term_t *pt) {
    if (++(pt->row) >= pt->rows) {
        term_scroll_up(pt);
    }
    if (pt->flg & TERM_FLG_AUTOCR) {
        term_write_CR(pt);
    }
}

void term_write_control_char(term_t *pt, uint8_t ch) {
    switch (ch) {
    case '\n':
        term_write_LF(pt);
        break; // 0x0a 10
    case '\r':
        term_write_CR(pt);
        break; // 0x0d 13
    }
}

void term_write_char(term_t *pt, uint8_t ch) {
    if (!pt || !(pt->buff)) {
        return;
    }

    /// Add new line if needed
    /// if it's not a new line char.
    if (ch != '\n') {
        if (pt->col >= pt->cols) {
            pt->col = 0;
            if (++(pt->row) >= pt->rows) {
                term_scroll_up(pt);
            }
        }
    }

    if ((ch == 0x1b) || (pt->flg & TERM_FLG_ESCAPE)) {
        term_write_escape_char(pt, ch);
    } else if (ch < 32) {
        term_write_control_char(pt, ch);
    } else {
        const uint16_t i = pt->col + pt->row * pt->cols;
        pt->buff[2 * i + 0] = ch;
        pt->buff[2 * i + 1] = pt->attr;
        pt->buff[pt->size + (i >> 3)] |= (1 << (i % 8));
        pt->flg |= TERM_FLG_CHANGED;
        /// leave the cursor even behind the end of the line
        /// this allows merging auto-new-line with '\n'
        ++(pt->col);
    }
}
// duplicit function, todo erase
// to be replaced by window_term_t::Printf, but it does not work in bsod
int term_printf(term_t *pt, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    char text[TERM_PRINTF_MAX];

    int ret = vsnprintf(text, sizeof(text), fmt, va);

    const size_t range = std::min(ret, TERM_PRINTF_MAX);
    for (size_t i = 0; i < range; i++) {
        term_write_char(pt, text[i]);
    }

    va_end(va);

    return ret;
}

// cannot use this
// passing va_list into multiple nested functions does not work
// undefined behavior
/*
int term_printf(term_t *pt, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    int ret = term_vprintf(pt, fmt, va);
    va_end(va);
    return ret;
}


int term_vprintf(term_t *pt, const char *fmt, va_list va) {

    char text[TERM_PRINTF_MAX];

    int ret = vsnprintf(text, sizeof(text), fmt, va);

    const size_t range = std::min(ret, TERM_PRINTF_MAX);
    for (size_t i = 0; i < range; i++)
        term_write_char(pt, text[i]);

    return ret;
}
*/
