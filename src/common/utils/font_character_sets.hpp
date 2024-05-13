#pragma once

/* We support different character sets for different fonts
 * Various character sets are used for various fonts to optimize the memory
 */
enum class FontCharacterSet : uint8_t {
    full = 0, /// standard ASCII (32 - 127) + all required non-ascii latin + all required Katakana characters + japanese ',' + japanese '.'
    standard = 1, /// standard ASCII (32 - 127) + all required non-ascii latin
    digits = 2, /// digits (0 - 9) + '.' + '?' + '%' + '-'
};
