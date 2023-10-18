#include "splice.h"
#include "server.h"

#include <transfers/files.hpp>
#include <transfers/partial_file.hpp>

using nhttp::splice::Result;
using std::array;
using std::get_if;
using std::holds_alternative;
using std::variant;
using std::visit;
using transfers::PartialFile;

namespace nhttp::splice {

bool Done::io_task() {
    // Empty on purpose
    return false;
}

void Done::callback() {
    // Empty on purpose
}

void Done::final_callback() {
    assert(transfer);
    assert(transfer->server);
    auto res = transfer->done();
    transfer->server->transfer_done(res);
    transfer = nullptr;
}

void Done::init(Transfer *transfer) {
    assert(this->transfer == nullptr);
    assert(transfer != nullptr);
    this->transfer = transfer;
}

Done Done::instance;

bool Write::io_task() {
    if (transfer->result != Result::Ok) {
        return false;
    }

    // Init (current == nullptr at the start)
    if (current == nullptr) {
        if (!transfer->progress(data->tot_len - offset)) {
            transfer->result = Result::Stopped;
            return false;
        }

        current = data;
    }

    // A pbuf is a linked-list with disjoint blocks.
    // Find the right pbuf chain node
    while (offset >= current->len) {
        offset -= current->len;
        current = current->next;
        if (current == nullptr) {
            return false;
        }
    }

    // Get in-buffer
    assert(current != nullptr);
    assert(offset < current->len);
    const uint8_t *in = static_cast<uint8_t *>(current->payload) + offset;
    const size_t in_size = current->len - offset;

    // Get out buffer
    auto f = transfer->file();
    auto buff = f->get_current_buffer(true);
    if (holds_alternative<PartialFile::WouldBlock>(buff)) {
        // We ask for blocking mode
        assert(0);
        transfer->result = Result::CantWrite;
        return false;
    } else if (holds_alternative<PartialFile::WriteError>(buff) || holds_alternative<PartialFile::OutOfRange>(buff)) {
        transfer->result = Result::CantWrite;
        return false;
    }
    assert(std::holds_alternative<PartialFile::BufferAndOffset>(buff));
    auto [buff_ptr, buff_off] = get<PartialFile::BufferAndOffset>(buff);
    uint8_t *out = buff_ptr + buff_off;
    const size_t out_size = PartialFile::SECTOR_SIZE - buff_off;

    // Perform a write/transformation of the data into the out buffer
    const auto [in_used, out_used] = transfer->write(in, in_size, out, out_size);
    assert(in_used <= in_size);
    assert(out_used <= out_size);

    if (!f->advance_written(out_used)) {
        transfer->result = Result::CantWrite;
        return false;
    }

    to_ack.fetch_add(in_used);
    offset += in_used;

    // Do we have more data to process? Do we want to get called again?
    return (current->len > offset || current->next != nullptr);
}

void Write::callback() {
    assert(transfer);
    assert(transfer->server);
    transfer->server->write_done(to_ack.exchange(0), false);
}

void Write::final_callback() {
    assert(transfer);
    assert(transfer->server);
    transfer->server->write_done(to_ack.exchange(0), true);
    pbuf_free(data);
    data = nullptr;
    transfer = nullptr;
}

array<Write, Write::REQ_CNT> Write::instances;

Write *Write::find_empty() {
    for (auto &w : instances) {
        if (w.transfer == nullptr) {
            return &w;
        }
    }
    return nullptr;
}

void Write::init(Transfer *transfer, pbuf *data, uint16_t offset) {
    assert(this->transfer == nullptr);
    assert(transfer != nullptr);
    this->transfer = transfer;
    this->data = data;
    this->offset = offset;
    current = nullptr;
    to_ack = 0;
}

void Transfer::release() {
    // we release on second call, because both TransferSlot::release and
    // Transfer::done needs to call it before we actually can safely release
    // monitor_slot and reset result
    released_counter++;
    if (released_counter == 2) {
        assert(monitor_slot.has_value());
        monitor_slot.reset();
        result = Result::Ok;
        released_counter = 0;
    }
}

const char *Transfer::filepath() {
    assert(monitor_slot.has_value());
    return monitor_slot->filepath();
}

void Transfer::set_monitor_slot(transfers::Monitor::Slot &&slot) {
    monitor_slot = std::move(slot);
}

}; // namespace nhttp::splice
