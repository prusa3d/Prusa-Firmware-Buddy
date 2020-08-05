#pragma once

#include "string_view_utf8.hpp"
#include <utility>

class UnaccentTable {
public:
    struct Item {
        uint16_t key; /// unichar key value (not raw utf8 bytes!)
        uint8_t size; /// how many chars do we have
        char str[3];  /// we support up to two ASCII characters for substitution
    };
    static const Item &Utf8RemoveAccents(unichar c);

private:
    static const Item table[];
};
