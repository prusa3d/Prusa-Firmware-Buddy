#include "splice.h"
#include "server.h"

#include <transfers/files.hpp>
#include <transfers/partial_file.hpp>

using std::get;
using std::holds_alternative;
using std::make_tuple;
using std::tuple;
using transfers::PartialFile;

namespace nhttp::splice {

tuple<size_t, Result> Transfer::write(const uint8_t *in, size_t in_size) {
    auto res = result;
    if (result != Result::Ok) {
        return make_tuple(0, res);
    }

    // Get out buffer
    auto f = file();
    auto buff = f->get_current_buffer(false);
    if (holds_alternative<PartialFile::WouldBlock>(buff)) {
        return make_tuple(0, res);
    } else if (holds_alternative<PartialFile::WriteError>(buff) || holds_alternative<PartialFile::OutOfRange>(buff)) {
        result = Result::CantWrite;
        return make_tuple(0, Result::CantWrite);
    }
    assert(std::holds_alternative<PartialFile::BufferAndOffset>(buff));
    auto [buff_ptr, buff_off] = get<PartialFile::BufferAndOffset>(buff);
    uint8_t *out = buff_ptr + buff_off;
    const size_t out_size = PartialFile::SECTOR_SIZE - buff_off;

    // Perform a write/transformation of the data into the out buffer
    const auto [in_used, out_used] = write(in, in_size, out, out_size);
    assert(in_used <= in_size);
    assert(out_used <= out_size);

    if (!f->advance_written(out_used)) {
        result = Result::CantWrite;
        return make_tuple(0, Result::CantWrite);
    }

    return make_tuple(in_used, res);
}

void Transfer::release() {
    result = Result::Ok;
    monitor_slot.reset();
}

const char *Transfer::filepath() {
    assert(monitor_slot.has_value());
    return monitor_slot->filepath();
}

void Transfer::set_monitor_slot(transfers::Monitor::Slot &&slot) {
    monitor_slot = std::move(slot);
}

}; // namespace nhttp::splice
