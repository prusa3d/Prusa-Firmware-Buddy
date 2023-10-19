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

namespace {

    bool store_segment(variant<FILE *, PartialFile *> file, const uint8_t *data, size_t size) {
        if (FILE **f = get_if<FILE *>(&file); f != nullptr) {
            return transfers::write_block(*f, data, size);
        } else if (PartialFile **f = get_if<PartialFile *>(&file); f != nullptr) {
            return (*f)->write(data, size);
        } else {
            assert(0);
            return false;
        }
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

Write::ProcessResult Write::process(uint8_t *data, size_t size_in, size_t size_out) {
    auto [consumed, produced, need_buff] = transfer->transform(data, size_in, size_out);
    assert(size_out >= produced);
    auto f = transfer->file();

    if (!store_segment(f, data, produced)) {
        return WriteError {};
    }

    assert((consumed == size_in && need_buff == 0) || (consumed < size_in && need_buff >= size_in - consumed));
    if (need_buff > 0) {
        return NeedMore {
            consumed,
            need_buff,
        };
    } else {
        return WriteComplete {};
    }
}

bool Write::io_task() {
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

    // We may need to decrypt the data. We try to do it as much as possible
    // in-place, but in rare cases (the size not being multiple of the cipher
    // block size), we need to handle a "tail".
    //
    // Our TCP settings motivate the sender to prefer packets of 1024B
    // payloads, so that should help in not having many tails.
    uint8_t *data = static_cast<uint8_t *>(current->payload) + skip;
    auto result_1 = process(data, current->len - skip, current->len - skip);

    if (holds_alternative<WriteError>(result_1)) {
        transfer->result = Result::CantWrite;
        return false;
    } else if (auto more_buff = get_if<NeedMore>(&result_1); more_buff != nullptr) {
        size_t leftover = current->len - skip - more_buff->used;
        assert(leftover > 0);
        assert(leftover <= more_buff->buff_needed);
        uint8_t buff[more_buff->buff_needed];
        memcpy(buff, data + more_buff->used, leftover);

        auto result_2 = process(buff, leftover, more_buff->buff_needed);

        if (holds_alternative<WriteError>(result_2)) {
            transfer->result = Result::CantWrite;
            return false;
        }
        // The second attempt must go through, we've given it the buffer it asked for.
        assert(!holds_alternative<NeedMore>(result_2));
    } else {
        // complete OK
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
