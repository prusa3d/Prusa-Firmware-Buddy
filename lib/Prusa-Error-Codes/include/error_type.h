#pragma once

#include <stdint.h>

enum class ErrType : uint8_t {
    ERROR = 0,
    WARNING,
    USER_ACTION,
    CONNECT
};
