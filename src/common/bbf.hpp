#pragma once
#include <cstdio>

namespace buddy::bbf {

enum class TLVType : uint8_t {
    RESOURCES_IMAGE = 1,
    RESOURCES_IMAGE_BLOCK_SIZE = 2,
    RESOURCES_IMAGE_BLOCK_COUNT = 3,
    RESOURCES_IMAGE_HASH = 4,
};

bool seek_to_tlv_entry(FILE *bbf, TLVType type, uint32_t &length);

};
