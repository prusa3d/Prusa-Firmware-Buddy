#include "splice.h"
#include "server.h"

#include <transfers/files.hpp>

using nhttp::splice::Result;
using std::array;

namespace nhttp::splice {

namespace {

    bool store_segment(FILE *f, pbuf *data, size_t offset) {
        return transfers::write_block(f, static_cast<const uint8_t *>(data->payload) + offset, data->len - offset);
    }

} // namespace

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

Done Done::instance;

bool Write::io_task() {
    FILE *f = transfer->file();
    if (transfer->result != Result::Ok) {
        return false;
    }

    uint16_t skip = 0;
    if (current == nullptr) {
        skip = offset;
        if (!transfer->progress(data->tot_len - offset)) {
            transfer->result = Result::Stopped;
            return false;
        }

        // A pbuf is a linked-list with disjoint blocks.
        current = data;
        while (skip >= current->len) {
            skip -= current->len;
            current = current->next;
            if (current == nullptr) {
                return false;
            }
        }
    }

    if (!store_segment(f, current, skip)) {
        transfer->result = Result::CantWrite;
        return false;
    }

    to_ack.fetch_add(current->len - skip);
    current = current->next;
    if (current == nullptr) {
        return false;
    } else {
        return true;
    }
}

void Done::init(Transfer *transfer) {
    assert(this->transfer == nullptr);
    assert(transfer != nullptr);
    this->transfer = transfer;
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
