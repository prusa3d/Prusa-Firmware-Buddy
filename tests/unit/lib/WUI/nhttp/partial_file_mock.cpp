#include "transfers/partial_file.hpp"

using namespace transfers;

std::variant<const char *, PartialFile::Ptr> PartialFile::create(const char *path, size_t size) {
    return "not implemented";
}

bool PartialFile::write(const uint8_t *data, size_t size) {
    return true;
}

bool PartialFile::sync() {
    return true;
}

bool PartialFile::advance_written(size_t by) {
    return true;
}

PartialFile::BufferPeek PartialFile::get_current_buffer(bool blocking_wait) {
    return WriteError {};
}
