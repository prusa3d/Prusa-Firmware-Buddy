#include "dump_parse.hpp"
#include "dump.hpp"

namespace crash_dump {
CrashCatcherDumpParser::CrashCatcherDumpParser() {
    // load size
    dump_size = dump_get_size();

    // load flags
    dump_read_data(FLAGS_OFFSET, sizeof(flags), reinterpret_cast<uint8_t *>(&flags));
}
bool CrashCatcherDumpParser::load_data(uint8_t *to, uintptr_t from, size_t size) {
    memset(to, 0, size); // just to return reasonable data when not found
    uint32_t offset = (flags & CRASH_CATCHER_FLAGS_FLOATING_POINT) ? DATA_BLOCKS_OFFSET_WITH_FP : DATA_BLOCKS_OFFSET_WITHOUT_FP;
    while (offset < dump_size) {
        uint32_t header[2];
        dump_read_data(offset, sizeof(header), reinterpret_cast<uint8_t *>(&header));
        offset += sizeof(header);
        uint32_t block_size = header[1] - header[0];
        if (header[0] <= from && header[1] >= (from + size)) {
            dump_read_data(offset + (from - header[0]), size, to);
            return true;
        }
        offset += block_size;
    }
    return false;
}
void CrashCatcherDumpParser::load_registers(Registers_t &registers) {
    dump_read_data(REGISTERS_OFFSET, sizeof(registers), reinterpret_cast<uint8_t *>(&registers));
}

} // namespace crash_dump
