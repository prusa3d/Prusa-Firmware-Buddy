#include "bbf.hpp"
#include <logging/log.hpp>

static const long bbf_firmware_size_offset = 96;
static const long bbf_firmware_code_offset = 576;

bool buddy::bbf::seek_to_tlv_entry(FILE *bbf, TLVType type, uint32_t &length) {
    // read firmware size
    fseek(bbf, bbf_firmware_size_offset, SEEK_SET);
    uint32_t fw_size;
    if (fread(&fw_size, 1, sizeof(fw_size), bbf) != sizeof(fw_size)) {
        return false;
    }

    // seek to first TLV entry (header 512B, then firmware, then TLV)
    uint32_t tlv_offset = bbf_firmware_code_offset + fw_size;
    fseek(bbf, tlv_offset, SEEK_SET);

    // search for the entry
    while (!feof(bbf) && !ferror(bbf)) {
        TLVType current_type;
        if (fread(&current_type, 1, sizeof(current_type), bbf) != sizeof(current_type)) {
            return false;
        }
        if (fread(&length, 1, sizeof(length), bbf) != sizeof(length) || length == 0) {
            return false;
        }
        if (current_type == type) {
            return true;
        } else {
            fseek(bbf, length, SEEK_CUR);
        }
    }
    return false;
}
