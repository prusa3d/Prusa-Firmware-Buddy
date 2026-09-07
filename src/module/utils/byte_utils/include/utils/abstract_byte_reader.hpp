/// @file
#pragma once

#include <utils/byte_utils.hpp>

/// Generic interface for reading bytes into buffer.
class AbstractByteReader {
public:
    /// Read bytes into provided buffer, return valid subspan of that buffer.
    virtual WritableBytes read(WritableBytes buffer) = 0;
};
