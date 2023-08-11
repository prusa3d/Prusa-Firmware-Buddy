#include "server.h"
#include "splice.h"

#include <algorithm>
#include <cassert>
#include <lwip/sys.h>

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

void Server::ConnectionSlot::step(string_view input, uint8_t *output, size_t out_size) {
    Step s = std::visit([this, input, output, out_size](auto &phase) -> Step {
        return phase.step(input, client_closed && input.empty() && output == nullptr, output, out_size);
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
        TransferSlot *dest = &server->transfer_slot;
        // We are asked to perform a transfer from socket -> file. For that we:
        // * Assume the transfer slot is free (must be ensured by the caller).
        // * Rip the data out of this slot and move everything there.
        assert(dest->transfer == nullptr);
        assert(dest->reqs_pending == 0);
        assert(dest->done_called == false);
        // We are not allowed to go to the transfer at the same time as writing
        // data (too complex and not needed).
        assert(buffer == nullptr);
        auto [transfer, expected] = get<handler::TransferExpected>(s.next);
        dest->transfer = transfer;
        dest->transfer->server = server;
        dest->conn = conn;
        dest->expected_data = expected;
        altcp_arg(conn, dest);
        server->activity(conn, dest);

        conn = nullptr;
        if (partial && partial->tot_len == partial_consumed) {
            release_partial();
        }

        // Still some part of data to deal with.
        if (partial) {
            pbuf *data = partial.release(); // We take ownership and pass it to the Write request.
            auto len = data->tot_len - partial_consumed;
            if (len > dest->expected_data) {
                pbuf_realloc(data, dest->expected_data + partial_consumed);
                len = data->tot_len - partial_consumed;
            }
            dest->expected_data -= len;
            auto req = splice::Write::find_empty();
            // Because we have an empty transfer slot, it means no transfer is
            // running and therefore all write requests shall be ready to be
            // used.
            assert(req != nullptr);
            req->init(dest->transfer, data, partial_consumed);
            dest->reqs_pending++;
            async_io::enqueue(req);
            // This part of data is already processed, we can confirm it (the Write confirms only its own part).
            altcp_recved(dest->conn, partial_consumed);
        }

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
        while (slot->step()) {
        }

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
}

bool Server::is_active_slot(void *slot) {
    if (!slot) {
        return false;
    }

    BaseSlot *s = static_cast<BaseSlot *>(slot);
    return (dynamic_cast<ConnectionSlot *>(s) != nullptr) || (dynamic_cast<TransferSlot *>(s) != nullptr);
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

    listener.reset(defs.listener_alloc());

    if (!listener) {
        return false;
    }

    altcp_arg(listener.get(), this);
    altcp_accept(listener.get(), Server::accept_wrap);

    return true;
}

void Server::stop() {
    listener.reset();
    // Note: Letting the rest of the connections to live on!
}

void Server::TransferSlot::release() {
    expected_data = 0;
    reqs_pending = 0;
    if (!done_called && transfer != nullptr) {
        // Just make sure each file gets closed and transfer gets done.
        //
        // The TransferSlot now "looks empty" (and connection is closed and
        // everything that), the Done instances is taken and a request is
        // pending there. This looks like it could cause trouble if another
        // transfer comes because this solves itself (by finishing the Done).
        //
        // The trick here is that the transfer / monitor slot is released
        // inside the callback of Done and we won't accept a new transfer until
        // then.
        auto req = &splice::Done::instance;
        reqs_pending++;
        transfer->result = splice::Result::Timeout;
        req->init(transfer);
        async_io::enqueue(req);
    }
    transfer->release();
    transfer = nullptr;
    done_called = false;
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
    if (want_read() || reqs_pending > 0) {
        // We don't do any processing here at all, we just say we want to wait
        // for more data.
        return false;
    }

    // We don't want to read, we don't want to write and all the data we have
    // sent are acked -> we are done.
    //
    // Close can fail. Unlike the "usual" slots, we don't get called again in
    // such case and it's going to be very rare and all that, so we just clean
    // up the best way we can to avoid the complexity.
    if (!close()) {
        altcp_abort(conn);
        release();
    }
    return false;
}

bool Server::TransferSlot::take_pbuf(pbuf *data) {
    // Closed by client
    if (data == nullptr && expected_data > 0 && !done_called) {
        transfer->result = splice::Result::ClosedByClient;
        auto done = &splice::Done::instance;
        done->init(transfer);
        reqs_pending++;
        done_called = true;
        async_io::enqueue(done);
    }

    auto request = splice::Write::find_empty();

    if (request == nullptr) {
        // Too many requests are pending right now, refuse the pbuf now and let
        // LwIP give it to us later on.
        return false;
    }

    // In case there's more data than we want, throw the rest out of the
    // window (shouldn't happen, but don't just crash on other side's bad
    // behaviour). Also never enqueue Write after Done. Can happen if we stop the transfer, but another
    //  packet comes before we close the connection, in that case just ignore it and
    //  throw away.
    if (expected_data == 0 or done_called) {
        pbuf_free(data);
        return true;
    }

    if (data->tot_len > expected_data) {
        // Shrink it to only the data we actually want
        pbuf_realloc(data, expected_data);
    }

    assert(data->tot_len <= expected_data);
    expected_data -= data->tot_len;

    request->init(transfer, data);
    reqs_pending++;
    async_io::enqueue(request);

    if (expected_data == 0 && !done_called) {
        auto done = &splice::Done::instance;
        done->init(transfer);
        reqs_pending++;
        done_called = true;
        async_io::enqueue(done);
    }

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

void Server::TransferSlot::write_done(uint16_t len, bool complete) {
    if (complete) {
        reqs_pending--;
    }
    // Confirm processing of our data and ask to get more
    altcp_recved(conn, len);
    server->activity(conn, this);

    if (transfer->result != splice::Result::Ok && !done_called) {
        auto done = &splice::Done::instance;
        done->init(transfer);
        reqs_pending++;
        done_called = true;
        async_io::enqueue(done);
    } else {
        step();
    }
}

void Server::write_done(uint16_t len, bool complete) {
    transfer_slot.write_done(len, complete);
}

void Server::TransferSlot::done(std::optional<std::tuple<http::Status, const char *>> res) {
    reqs_pending--;
    assert(reqs_pending == 0);

    if (res.has_value()) {
        response = res;
        make_response();
    }
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
        release();
        server->step();
    }
    // if not this will be called once more, from the ConnectionSlot::release
}

void Server::transfer_done(std::optional<std::tuple<http::Status, const char *>> res) {
    transfer_slot.done(res);
}

} // namespace nhttp
