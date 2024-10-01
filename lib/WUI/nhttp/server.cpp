#include "server.h"
#include "splice.h"

#include <algorithm>
#include <cassert>
#include <lwip/sys.h>
#include <lwip/tcpip.h>

namespace nhttp {

using handler::ConnectionState;
using handler::Done;
using handler::Idle;
using handler::RequestParser;
using handler::Step;
using handler::Terminating;
using std::get;
using std::get_if;
using std::holds_alternative;
using std::string_view;

void Server::InactivityTimeout::schedule(uint32_t after) {
    last_activity = sys_now();
    // Number of quants in after rounded up.
    quants_left = (after + INACTIVITY_TIME_QUANT - 1) / INACTIVITY_TIME_QUANT;
    assert(!past());
}

void Server::InactivityTimeout::poll_inactivity() {
    uint32_t now = sys_now();
    uint32_t since_activity = now - last_activity;
    if (quants_left > 0 && since_activity >= INACTIVITY_TIME_QUANT) {
        quants_left--;
    }
}

bool Server::InactivityTimeout::past() const {
    return quants_left == 0;
}

void Server::ConnectionSlot::release_buffer() {
    if (buffer) {
        buffer->reset();
        buffer = nullptr;
    }
}

void Server::ConnectionSlot::release_partial() {
    if (partial) {
        if (conn != nullptr) {
            altcp_recved(conn, partial->tot_len);
        }
        partial.reset();
    }
    partial_consumed = 0;
}

uint16_t Server::ConnectionSlot::send_space() const {
    if (conn) {
        return std::min(altcp_mss(conn), altcp_sndbuf(conn));
    } else {
        return 0;
    }
}

bool Server::ConnectionSlot::has_unacked_data() const {
    return buffer != nullptr;
}

bool Server::ConnectionSlot::want_read() const {
    return std::visit([](const auto &phase) -> bool { return phase.want_read(); }, state);
}

bool Server::ConnectionSlot::want_write() const {
    return std::visit([](const auto &phase) -> bool { return phase.want_write(); }, state);
}

bool Server::Slot::close() {
    assert(conn);
    /*
     * The connection can be in a closing state for a while. It's unclear from
     * docs if it still can generate events at that point, so make sure to
     * remove the argument from it.
     *
     * On the other hand, close can also fail. So return it to previous state
     * if that happens.
     */
    Server::remove_callbacks(conn);
    if (altcp_close(conn) == ERR_OK) {
        release();
        return true;
    } else {
        Server::set_callbacks(conn, this);
        return false;
    }
}

void Server::Slot::release() {
    conn = nullptr;
}

void Server::Slot::forward_progress() {
    while (step()) {
    }
}

void Server::ConnectionSlot::release() {
    state = Idle();
    release_partial();
    release_buffer();
    client_closed = false;
    Slot::release();
    server->try_send_transfer_response(this);
}

bool Server::ConnectionSlot::is_empty() const {
    return holds_alternative<Idle>(state);
}

bool Server::ConnectionSlot::take_pbuf(pbuf *data) {
    // data = null - other side did shutdown.
    if (!data) {
        client_closed = true;
    }

    // TODO: Is this really supposed to be checked _after_ the above client closed thing?
    if (partial) {
        /*
         * We are still in the middle of the previous buffer. We don't want
         * more yet. Come back to as with it later.
         *
         * Yes, we do _not_ free the pbuf.
         */
        return false;
    }

    assert(partial_consumed == 0);
    if (data) {
        partial.reset(data);
    }

    return true;
}

void Server::inject_transfer(altcp_pcb *conn, pbuf *data, uint16_t data_offset, splice::Transfer *transfer, size_t expected_data) {
    TransferSlot *dest = &transfer_slot;
    // We are asked to perform a transfer from socket -> file. For that we:
    // * Assume the transfer slot is free (must be ensured by the caller).
    // * Migrate an existing connection into it.
    assert(dest->transfer == nullptr);
    // We are not allowed to go to the transfer at the same time as writing
    // data (too complex and not needed).
    dest->transfer = transfer;
    dest->transfer->server = this;
    dest->transfer->file()->set_written_callback(TransferSlot::send_segment_written, dest);
    dest->conn = conn;
    dest->expected_data = expected_data;
    dest->current_pbuf = nullptr;
    dest->pbuf_processed = 0;
    dest->pbuf_queue_size = 0;
    set_callbacks(conn, dest);

    // Still some part of data to deal with.
    if (data != nullptr) {
        dest->pbuf_queue_size = 1;
        dest->pbuf_queue[0].reset(data);
        dest->pbuf_processed = data_offset;
    }

    transfer_slot.forward_progress();
}

void Server::ConnectionSlot::step(string_view input, uint8_t *output, size_t out_size) {
    Step s = { 0, 0, handler::Continue() };
    std::visit([&](auto &phase) {
        phase.step(input, client_closed && input.empty() && output == nullptr, output, out_size, s);
    },
        state);

    assert(s.read <= input.size());
    assert(s.written <= out_size);
    assert(s.written == 0 || buffer);
    partial_consumed += s.read;
    if (s.written > 0) {
        assert(buffer->write_len == 0);
        assert(buffer->write_pos == 0);
        assert(buffer->acked == 0);
        // We do the check by the above assert and it is wrong if the returned
        // written is larger. Nevertheless, we have seen it slip, in case of
        // snprintf.
        //
        // Snprintf writes only the allowed amount of bytes but still can
        // return more bytes as its return in case it truncates. We don't want
        // to send some more random data than what was written.
        buffer->write_len = std::min(s.written, out_size);
    }

    if (holds_alternative<ConnectionState>(s.next)) {
        state = get<ConnectionState>(std::move(s.next));
    } else if (holds_alternative<handler::TransferExpected>(s.next)) {
        if (partial && partial_consumed > 0) {
            // This part is already taken care of, make sure accounting books sum up.
            altcp_recved(conn, partial_consumed);
        }

        if (partial && partial->tot_len == partial_consumed) {
            release_partial();
        }

        pbuf *data = partial.release(); // Steal the ownership of that pbuf and pass it on
        auto [transfer, expected] = get<handler::TransferExpected>(s.next);
        auto *inner_conn = conn;
        conn = nullptr;
        server->inject_transfer(inner_conn, data, partial_consumed, transfer, expected);

        release();
    }
}

bool Server::ConnectionSlot::step() {
    if (is_empty()) {
        /*
         * The rest would still work correctly for an empty slot, but it is
         * subtle at places.
         */
        return false;
    }

    /*
     * First try to reclaim some resources.
     */
    if (const Terminating *t = get_if<Terminating>(&state); t && t->how == Done::CloseFast) {
        /*
         * We can't use close with yet unacked data, that would allow
         * retransmits to send buffer now belonging to someone else. Abort in
         * that case.
         */
        if (buffer) {
            release();
            remove_callbacks(conn);
            altcp_abort(conn);
            return true;
        }
        return close();
    }

    if (partial && partial->tot_len == partial_consumed) {
        release_partial();
        return true;
    }

    if (buffer) {
        if (buffer->acked == buffer->write_len) {
            release_buffer();
            return true;
        }

        /*
         * Try to advance in/out buffers as much as possible. This'll get us closer
         * to releasing resources.
         */
        assert(buffer->write_pos <= buffer->write_len);
        const auto to_send = std::min(static_cast<uint16_t>(buffer->write_len - buffer->write_pos), send_space());
        if (to_send > 0 && altcp_write(conn, buffer->data.begin() + buffer->write_pos, to_send, 0) == ERR_OK) {
            buffer->write_pos += to_send;
            altcp_output(conn);
            server->activity(conn, this);
            return true;
        } else {
            /*
             * Couldn't send more (no space? Nothing to send?), but we are
             * still waiting for some acks. Keep waiting, don't do anything
             * else until we get rid of that data.
             *
             * (We could probably still try consuming more input, but being
             * able to and having it would be rare and would complicate things.)
             */
            return false;
        }
    }

    /*
     * No more sending or receiving, do some processing.
     */
    const bool re = want_read();
    const bool wr = want_write();
    string_view input = "";
    uint8_t *output = nullptr;
    size_t output_size = 0;
    const bool invoke_client_close = re && client_closed;

    if (re && partial) {
        // The input pbuf is a linked list with disjoint blocks. Find the right
        // block.
        pbuf *current = partial.get();
        uint16_t skip = partial_consumed;
        // If it's fully consumed, we would have gotten rid of it above.
        assert(skip < current->tot_len);
        while (current && skip >= current->len) {
            skip -= current->len;
            current = current->next;
        }
        if (current) {
            input = string_view(static_cast<const char *>(current->payload) + skip, current->len - skip);
        } else {
            // This is in theory impossible, it would mean LwIP gave us
            // inconsistent pbuf list that ends prematurely.
            assert(false);
        }
    }

    if (wr) {
        const uint16_t queue_size = send_space();
        Buffer *empty = server->find_empty_buffer();

        if (queue_size > 0 && empty) {
            buffer = empty;
            buffer->owned = true;
            output = buffer->data.begin();
            output_size = buffer->data.size();
        }
    }

    if (!input.empty() || invoke_client_close || output) {
        server->activity(conn, this);
        step(input, output, output_size);
        // Note: The step can take extensive amount of time - specifically, it
        // can take a long time to write data to USB (several seconds!). We
        // therefore want to reset the timer after that too, because it's not
        // inactivity of the other side.
        server->activity(conn, this);
        return true;
    }

    if (!wr) {
        if (Terminating *t = get_if<Terminating>(&state); t && t->shutdown_send) {
            t->shutdown_send = false;
            // Ignoring errors, no way to handle anyway and we'll close it soon.
            altcp_shutdown(conn, 0, 1);
            return true;
        }
    }
    if (!re && !wr) {
        /*
         * The current phase doesn't want to do anything. Check for special
         * cases.
         */
        if (const Terminating *t = get_if<Terminating>(&state)) {
            altcp_setprio(conn, Server::IDLE_PRIO);
            altcp_arg(conn, server->idle_slots.begin());
            server->activity(conn, server->idle_slots.begin());
            switch (t->how) {
            case Done::KeepAlive:
                if (partial) {
                    // We still have some data to process. Pipelining?
                    state.emplace<handler::RequestParser>(*server);
                    altcp_setprio(conn, ACTIVE_PRIO);
                    altcp_arg(conn, this);
                    server->activity(conn, this);
                } else {
                    release();
                }
                return true;
            case Done::Close:
                if (close()) {
                    return true;
                }
                break;
            default:
                // CloseFast is handled above
                assert(false);
            }
        }
    }

    /*
     * Nothing to do here, waiting for something to happen:
     *
     * * Data arriving over the network.
     * * Something timing out.
     * * Freeing some resources.
     */
    return false;
}

void Server::Buffer::reset() {
    owned = false;
    acked = 0;
    write_len = 0;
    write_pos = 0;
}

Server::Server(const ServerDefs &defs)
    : defs(defs) {
    for (auto &slot : idle_slots) {
        slot.server = this;
    }
    for (auto &slot : active_slots) {
        slot.server = this;
    }
    transfer_slot.server = this;
}

err_t Server::accept_wrap(void *me, struct altcp_pcb *new_conn, err_t err) {
    return static_cast<Server *>(me)->accept(new_conn, err);
}

err_t Server::accept(altcp_pcb *new_conn, err_t err) {
    if ((err != ERR_OK) || (new_conn == nullptr)) {
        return ERR_VAL;
    }

    /*
     * The connection starts in an idle state - until it starts doing
     * something, we won't give it anything out of our memory.
     *
     * Let LwIP handle the memory for the connection itself, though.
     */
    set_callbacks(new_conn, idle_slots.begin());

    /*
     * We are willing to throw idle connections overboard to have more active ones.
     */
    altcp_setprio(new_conn, IDLE_PRIO);
    /*
     * Don't wait with sending more data. We are going to keep sending small
     * chunks anyway.
     */
    altcp_nagle_disable(new_conn);

    return ERR_OK;
}

void Server::set_callbacks(altcp_pcb *conn, BaseSlot *slot) {
    altcp_err(conn, lost_conn_wrap);
    altcp_poll(conn, idle_conn_wrap, POLL_TIME);
    altcp_recv(conn, received_wrap);
    altcp_sent(conn, sent_wrap);
    altcp_arg(conn, slot);
    slot->server->activity(conn, slot);
}

void Server::remove_callbacks(altcp_pcb *conn) {
    altcp_err(conn, nullptr);
    altcp_poll(conn, nullptr, 0);
    altcp_recv(conn, nullptr);
    altcp_sent(conn, nullptr);
    altcp_arg(conn, nullptr);
}

void Server::lost_conn_wrap(void *slot, err_t) {
    if (is_active_slot(slot)) {
        static_cast<Slot *>(slot)->release();
    }
}

err_t Server::idle_conn_wrap(void *slot, altcp_pcb *conn) {
    /*
     * LwIP seems to be invoking the poll timer from time to time, not when
     * inactive as documented. So track if we are or are not active on our side.
     */
    BaseSlot *s = static_cast<BaseSlot *>(slot);
    if (s->get_slot_type() == BaseSlot::SlotType::TransferSlot) {
        // Transfer slot may lose callbacks from the USB thread in case the
        // tcpip msgbox is full. This is not expected to happen often, but we
        // still need a last-resort "resurrect" for such case, which we do
        // here.
        TransferSlot *ts = static_cast<TransferSlot *>(slot);
        ts->forward_progress();

        if (ts->pbuf_queue_size > 0) {
            // We want to prevent the connection from timing out just because
            // we are writing data into USB.
            //
            // Either we just submitted a bit of data (in which case the
            // activity was marked) or we have something still sitting in the
            // queue without a space in the USB.
            return ERR_OK;
        }
    }
    s->timeout.poll_inactivity();
    if (!s->timeout.past()) {
        return ERR_OK;
    }

    bool send_goodbye = false;
    bool has_unacked_data = false;
    Slot *active_slot = nullptr;
    if (is_active_slot(slot)) {
        active_slot = static_cast<Slot *>(slot);
        send_goodbye = active_slot->want_read();
        has_unacked_data = active_slot->has_unacked_data();
    }
    lost_conn_wrap(slot, ERR_OK);
    if (conn != nullptr) {
        if (send_goodbye) {
            /*
             * This is a bit best-effort. We don't stress over this failing or whatever.
             */
            static const char goodbye[] = "HTTP/1.1 408 Request Timeout\r\n\r\n";
            altcp_write(conn, goodbye, sizeof(goodbye), 0);
            altcp_output(conn);
        }
        /*
         * We don't know if connection-in-close can still generate callbacks
         * but we sure don't want them.
         */
        remove_callbacks(conn);
        if (active_slot != nullptr) {
            active_slot->release();
        }
        /*
         * If there are unacked data, they sit in the buffer we just released.
         * We need to abort the connection (it's idle while not getting ACKs,
         * so broken anyway) so we wont try to retransmit from memory assigned
         * to something else.
         */
        if (has_unacked_data || altcp_close(conn) != ERR_OK) {
            altcp_abort(conn);
            return ERR_ABRT;
        }
    }

    return ERR_OK;
}

err_t Server::received_wrap(void *raw_slot, struct altcp_pcb *conn, pbuf *data, [[maybe_unused]] err_t err) {
    assert(raw_slot != nullptr);
    assert(conn != nullptr);

    BaseSlot *base_slot = static_cast<BaseSlot *>(raw_slot);
    if (!is_active_slot(base_slot)) {
        if (data == nullptr) {
            // Closing an idle connection (no buffers & similar tied to it -> nothing to release)
            remove_callbacks(conn);
            if (altcp_close(conn) == ERR_OK) {
                return ERR_OK;
            } else {
                altcp_abort(conn);
                return ERR_ABRT;
            }
        }
        /*
         * The connection was not yet active. Find a slot for it and activate it.
         */
        if (ConnectionSlot *active_slot = base_slot->server->find_empty_slot(); active_slot != nullptr) {
            assert(!active_slot->partial);
            assert(active_slot->partial_consumed == 0);
            assert(!active_slot->buffer);
            assert(holds_alternative<Idle>(active_slot->state));

            active_slot->state.emplace<handler::RequestParser>(*active_slot->server);
            active_slot->conn = conn;
            altcp_arg(conn, active_slot);
            base_slot = active_slot;
            // Activity set below

            altcp_setprio(conn, ACTIVE_PRIO);
        } else {
            /*
             * No slot available. Refuse the pbuf now (it'll be given to us
             * later on, at which point we might have some resources
             * available).
             *
             * Yes, we do _not_ free the pbuf.
             */
            return ERR_MEM;
        }
    }

    assert(is_active_slot(base_slot));
    Slot *slot = static_cast<Slot *>(base_slot);

    slot->server->activity(conn, slot);

    if (slot->take_pbuf(data)) {
        /*
         * Will send data, consume data, free pbufs, release buffers and close
         * connections as needed.
         *
         * Any freeing of the pbuf happens in there (or maybe even later on in
         * case of async processing), as does the altcp_recved
         */
        slot->forward_progress();

        return ERR_OK;
    } else {
        // No freeing of pbuf!
        return ERR_MEM;
    }
}

err_t Server::sent_wrap(void *raw_slot, altcp_pcb *conn, uint16_t len) {
    if (is_active_slot(raw_slot)) {
        Slot *slot = static_cast<Slot *>(raw_slot);

        slot->server->sent(slot, len);
        slot->server->activity(conn, slot);
    }

    return ERR_OK;
}

void Server::ConnectionSlot::sent(uint16_t len) {
    assert(buffer != nullptr);
    assert(buffer->write_pos >= buffer->acked);
    const uint16_t unacked = buffer->write_pos - buffer->acked;
    assert(len <= unacked);
    (void)unacked; // No warnings on release
    buffer->acked += len;
}

void Server::sent(Slot *slot, uint16_t len) {
    slot->sent(len);

    step();
}

void Server::step() {
    const uint8_t cnt = active_slots.size();

    /*
     * Do a round-robin through the slots until none of them can make progress.
     * Start past the last one that made progress previously, to avoid
     * starvation.
     */
    for (uint8_t sleeping_slots = 0, idx = (last_active_slot + 1) % cnt; sleeping_slots < cnt; idx = (idx + 1) % cnt) {
        if (active_slots[idx].step()) {
            last_active_slot = idx;
            sleeping_slots = 0;
        } else {
            sleeping_slots += 1;
        }
    }

    transfer_slot.forward_progress();
}

bool Server::is_active_slot(void *slot) {
    if (!slot) {
        return false;
    }

    BaseSlot *s = static_cast<BaseSlot *>(slot);

    return s->get_slot_type() == BaseSlot::SlotType::ConnectionSlot || s->get_slot_type() == BaseSlot::SlotType::TransferSlot;
}

void Server::activity(altcp_pcb *conn, BaseSlot *slot) {
    if (is_active_slot(slot)) {
        slot->timeout.schedule(ACTIVE_TIMEOUT);
    } else {
        /*
         * Trick to share timeouts for arbitrary number of connections. See the
         * comment in the header file for details how (and why) it works.
         */
        const uint32_t now = sys_now();
        const size_t slot_id = (now / IDLE_TIMEOUT) % slot->server->idle_slots.size();
        slot = &slot->server->idle_slots[slot_id];
        slot->timeout.schedule(IDLE_TIMEOUT);
        altcp_arg(conn, slot);
    }
}

Server::ConnectionSlot *Server::find_empty_slot() {
    for (auto &slot : active_slots) {
        if (slot.is_empty()) {
            return &slot;
        }
    }
    return nullptr;
}

Server::Buffer *Server::find_empty_buffer() {
    for (auto &buffer : buffers) {
        if (!buffer.owned) {
            return &buffer;
        }
    }
    return nullptr;
}

bool Server::start() {
    if (listener) {
        // We are already started, so NOP.
        return true;
    }

    LOCK_TCPIP_CORE();
    listener.reset(defs.listener_alloc());

    if (!listener) {
        UNLOCK_TCPIP_CORE();
        return false;
    }

    altcp_arg(listener.get(), this);
    altcp_accept(listener.get(), Server::accept_wrap);

    UNLOCK_TCPIP_CORE();
    return true;
}

void Server::stop() {
    listener.reset();
    // Note: Letting the rest of the connections to live on!
}

void Server::TransferSlot::release() {
    if (transfer != nullptr) {
        // Just make sure each file gets closed and transfer gets done.
        transfer->result = splice::Result::Timeout;
        transfer->file()->set_written_callback(nullptr, nullptr);
        if (expected_data != 0 && transfer->result == splice::Result::Ok) {
            transfer->result = splice::Result::ClosedByClient;
        }
        transfer->done();
        transfer->release();
    }
    expected_data = 0;
    transfer = nullptr;
    for (size_t i = 0; i < PbufQueueMax; i++) {
        pbuf_queue[i].reset();
    }
    pbuf_queue_size = 0;
    current_pbuf = 0;
    pbuf_processed = 0;
    response.reset();
    Slot::release();
}

bool Server::TransferSlot::want_write() const {
    return false;
}

bool Server::TransferSlot::want_read() const {
    return expected_data > 0;
}

bool Server::TransferSlot::step() {
    if (transfer == nullptr) {
        // Called on empty transfer slot
        return false;
    }

    if (expected_data == 0 && response.has_value()) {
        // We've already closed stuff, we are just waiting for response to get picked up by a connection slot.
        return false;
    }

    if (expected_data == 0) {
        // Peer closed or we received all the data
        transfer->file()->set_written_callback(nullptr, nullptr);
        auto res = transfer->done();

        if (res.has_value()) {
            response = res;
            // This closes the connections / releases the slot (eventually)
            make_response();
            return false;
        }

        transfer->release();
        transfer = nullptr;

        // Close can fail. Unlike the "usual" slots, we don't get called again in
        // such case and it's going to be very rare and all that, so we just clean
        // up the best way we can to avoid the complexity.
        //
        // (conn may be null by make_response right now, in which case we
        // already transferred the ownership)
        if (!res.has_value() && conn != nullptr && !close()) {
            altcp_abort(conn);
            release();
        }
        return false;
    }

    if (pbuf_queue_size == 0) {
        // Nothing to process right now, waiting for more data.
        return false;
    }

    // Start working on another chain.
    if (current_pbuf == nullptr) {
        current_pbuf = pbuf_queue[0].get();
        if (current_pbuf == nullptr) {
            // Client closed, no more data coming. Handle the close in next
            // call.
            if (expected_data != 0 && transfer->result == splice::Result::Ok) {
                transfer->result = splice::Result::ClosedByClient;
            }
            expected_data = 0;
            return true;
        }

        // Deal with the peer sending more data than expected.
        size_t available = current_pbuf->tot_len - pbuf_processed;
        if (available > expected_data) {
            pbuf_realloc(current_pbuf, expected_data + pbuf_processed);
            available = expected_data;
        }

        if (!transfer->progress(available)) {
            transfer->result = splice::Result::Stopped;
            expected_data = 0;
            // Do a "close"
            return true;
        }
    }

    // Handle partially processed or previously processed parts of pbuf chains
    while (pbuf_processed >= current_pbuf->len) {
        pbuf_processed -= current_pbuf->len;
        current_pbuf = current_pbuf->next;
        if (current_pbuf == nullptr) {
            // Run out of this particular pbuf chain, drop it and shift the queue
            pbuf_processed = 0;
            pbuf_queue[0].reset();
            for (size_t i = 0; i < pbuf_queue_size - 1; i++) {
                pbuf_queue[i] = move(pbuf_queue[i + 1]);
            }
            pbuf_queue_size--;
            // Next call of step (that shall happen now) will try with the next one, if any.
            return true;
        }
    }

    assert(current_pbuf != nullptr);
    assert(current_pbuf->len > pbuf_processed); // At least little bit of data is left
    auto [read, result] = transfer->write(static_cast<const uint8_t *>(current_pbuf->payload) + pbuf_processed, current_pbuf->len - pbuf_processed);
    if (read != 0) {
        // We move the TCP window at the point of submitting to the USB.
        // Doing it after it was really written would not particularly help
        // anything (we still need buffers for the whole window anyway) and it
        // would be more complex.
        altcp_recved(conn, read);
        server->activity(conn, this);

        // Will deal with processing the pbuf next time
        pbuf_processed += read;
        expected_data -= read;
    }

    if (result != splice::Result::Ok) {
        // Something has gone wrong, expect no more data.
        expected_data = 0;
        return true;
    }

    // If we submitted something right now, try to submit more.
    return read != 0;
}

bool Server::TransferSlot::take_pbuf(pbuf *data) {
    if (expected_data == 0 && data != nullptr) {
        // pbuf past end handling
        pbuf_free(data);
        return true;
    }

    if (data == nullptr && pbuf_queue_size >= 2) {
        // When we get an EOF, we look into the queue and estimate if we could
        // have enough data to finish the whole transfer. If not, there's no
        // chance of success, so we drop most of the queue to give up faster
        // (so the transfer isn't stuck there for few seconds after the client
        // closes).
        //
        // Note that we do not touch the head of the queue - that one might be
        // currently in process of being handled, so we don't complicate
        // things.
        //
        // The in_queue is potentially including already handled data
        // (therefore higher than expected_data), but that's good enough
        // estimate for us.
        size_t in_queue = 0;
        for (size_t i = 0; i < pbuf_queue_size; i++) {
            if (pbuf_queue[i]) {
                in_queue += pbuf_queue[i]->tot_len;
            }
        }

        if (in_queue < expected_data) {
            for (size_t i = 1 /* Leave the [0] alone */; i < pbuf_queue_size; i++) {
                pbuf_queue[i].reset();
            }
            pbuf_queue_size = 1;
        }
    }

    if (pbuf_queue_size >= PbufQueueMax) {
        // Try to make room for the incoming pbuf in the queue.
        // (Further attempts will be done in the step just after this, to submit this pbuf)
        forward_progress();
    }

    if (pbuf_queue_size >= PbufQueueMax) {
        // Not enough room in the queue (unlikely, but possible with tiny
        // packets). Refuse the pbuf now and let lwip call us later again with it.
        return false;
    }

    // Note: We store the nullptr "peer closed" pbuf there too.
    pbuf_queue[pbuf_queue_size++].reset(data);

    // Our step will get called right after this and it'll deal with possibly
    // submitting it to USB.
    return true;
}

// TransferSlot never sends anything, so no unacked data
bool Server::TransferSlot::has_unacked_data() const {
    return false;
}

// TransferSlot never sends anything...
void Server::TransferSlot::sent(uint16_t) {
    // Intentionally empty
}

void Server::try_send_transfer_response(ConnectionSlot *slot) {
    if (transfer_slot.has_response()) {
        transfer_slot.make_response(slot);
    }
}

bool Server::TransferSlot::has_response() {
    return response.has_value();
}

void Server::TransferSlot::make_response(ConnectionSlot *slot) {
    auto &[status, message] = *response;
    ConnectionSlot *active_slot = slot != nullptr ? slot : server->find_empty_slot();
    if (active_slot != nullptr) {
        active_slot->conn = conn;
        if (status == http::Status::Ok) {
            active_slot->state.emplace<printer::FileInfo>(transfer->filepath(), false, /*TODO:: is this correct?*/ false, true, printer::FileInfo::ReqMethod::Get, printer::FileInfo::APIVersion::v1, std::nullopt);
        } else {
            active_slot->state.emplace<handler::StatusPage>(status, handler::StatusPage::CloseHandling::Close, /*TODO: is this correct?*/ false, std::nullopt, message);
        }
        altcp_arg(conn, active_slot);
        server->activity(conn, active_slot);
        altcp_setprio(conn, ACTIVE_PRIO);
        conn = nullptr;
        // Do this before release, we've already called done.
        transfer->file()->set_written_callback(nullptr, nullptr);
        transfer->release();
        transfer = nullptr;
        release();
        server->step();
    }
    // if not this will be called once more, from the ConnectionSlot::release
}

void Server::TransferSlot::segment_written(void *arg) {
    // This might get called with a failure too. The step is supposed to handle that.
    //
    // Also, as the thing is sent through the queue to the tcpip thread, it
    // might get delayed, potentially past the point when the transfer is
    // aborted. That is OK, because a) the slot is long-livig, b) step checks
    // for validity.
    assert(arg != nullptr);
    TransferSlot *slot = static_cast<TransferSlot *>(arg);
    assert(slot->get_slot_type() == SlotType::TransferSlot);
    slot->forward_progress();
}

void Server::TransferSlot::send_segment_written(void *arg) {
    // Get the notification to our thread.
    //
    // This might fail in case the mbox is full. We absolutely can not afford
    // to block in here (this is the USB thread and we would block all USB
    // processing and it could potentially even deadlock). If _all_ such
    // notifications are lost, there's a last-resort resurrect in the poll
    // callback. But it's not expected to be happening.
    tcpip_try_callback(TransferSlot::segment_written, arg);
}

} // namespace nhttp
