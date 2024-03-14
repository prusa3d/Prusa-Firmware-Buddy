#pragma once
#include <cstdio>

namespace buddy::bbf {

enum class TLVType : uint8_t {
    RESOURCES_IMAGE = 1,
    RESOURCES_IMAGE_BLOCK_SIZE = 2,
    RESOURCES_IMAGE_BLOCK_COUNT = 3,
    RESOURCES_IMAGE_HASH = 4,
    RESOURCES_BOOTLOADER_IMAGE = 5,
    RESOURCES_BOOTLOADER_IMAGE_BLOCK_SIZE = 6,
    RESOURCES_BOOTLOADER_IMAGE_BLOCK_COUNT = 7,
    RESOURCES_BOOTLOADER_IMAGE_HASH = 8,
};

bool seek_to_tlv_entry(FILE *bbf, TLVType type, uint32_t &length);

inline TLVType block_size_for_image(TLVType tlv_entry) {
    return static_cast<TLVType>(static_cast<uint8_t>(tlv_entry) + 1);
}

inline TLVType block_count_for_image(TLVType tlv_entry) {
    return static_cast<TLVType>(static_cast<uint8_t>(tlv_entry) + 2);
}

inline TLVType hash_for_image(TLVType tlv_entry) {
    return static_cast<TLVType>(static_cast<uint8_t>(tlv_entry) + 3);
}

}; // namespace buddy::bbf
