/**
 * @file font_flags_c.h
 * @author Radek Vana
 * @brief low level definition of font render flags for C
 * @date 2021-02-02
 */
#pragma once

enum {
    FONT_FLG_SWAP = 0x01, // swap low/high byte
    FONT_FLG_LSBF = 0x02, // LSB first
};
