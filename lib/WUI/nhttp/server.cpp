#include "server.h"

#include <algorithm>
#include <cassert>

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

ServerDefs::~ServerDefs() {};

Server::Slot::Slot()
    : partial(nullptr, &pbuf_free) {}

void Server::Slot::release_buffer() {
    if (buffer) {
        buffer->reset();
        buffer = nullptr;
    }
}

void Server::Slot::release_partial() {
    altcp_recved(conn, partial->tot_len);
    partial.reset();
    partial_consumed = 0;
}

uint16_t Server::Slot::send_space() const {
    if (conn) {
        return std::min(altcp_mss(conn), altcp_sndbuf(conn));
    } else {
        return 0;
    }
}

bool Server::Slot::want_read() const {
    return std::visit([](const auto &phase) -> bool { return phase.want_read(); }, state);
}

bool Server::Slot::want_write() const {
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
    release_buffer();
    release_partial();
    state = Idle();
    conn = nullptr;
    client_closed = false;
}

bool Server::Slot::is_empty() const {
    return holds_alternative<Idle>(state);
}

void Server::Slot::step(string_view input, uint8_t *output, size_t out_buff) {
    Step s = std::visit([this, input, output, out_buff](auto &phase) -> Step {
        return phase.step(input, client_closed && input.empty() && output == nullptr, output, out_buff);
    },
        state);

    assert(s.read <= input.size());
    assert(s.written <= out_buff);
    assert(s.written == 0 || buffer);
    partial_consumed += s.read;
    if (s.written > 0) {
        assert(buffer->write_len == 0);
        assert(buffer->write_pos == 0);
        assert(buffer->acked == 0);
        buffer->write_len = s.written;
    }

    if (holds_alternative<ConnectionState>(s.next)) {
        state = get<ConnectionState>(std::move(s.next));
    }
}

bool Server::Slot::step() {
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
        const auto flags = (to_send < (buffer->write_len - buffer->write_pos) || want_write()) ? TCP_WRITE_FLAG_MORE : 0;
        if (to_send > 0 && altcp_write(conn, buffer->data.begin() + buffer->write_pos, to_send, flags) == ERR_OK) {
            buffer->write_pos += to_send;
            altcp_output(conn);
            seen_activity = true;
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
        assert(current);
        input = string_view(static_cast<const char *>(current->payload) + skip, current->len - skip);
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
        seen_activity = true;
        step(input, output, output_size);
        return true;
    }

    if (!re && !wr) {
        /*
         * The current phase doesn't want to do anything. Check for special
         * cases.
         */
        if (const Terminating *t = get_if<Terminating>(&state)) {
            altcp_poll(conn, Server::idle_conn_wrap, Server::IDLE_POLL_TIME);
            altcp_setprio(conn, Server::IDLE_PRIO);
            altcp_arg(conn, &server->idle_slot);
            switch (t->how) {
            case Done::KeepAlive:
                if (partial) {
                    // We still have some data to process. Pipelining?
                    state.emplace<handler::RequestParser>(*server);
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
                assert(false);
            }
        }
    }

    /*
     * Nothing to do here, waiting for something to happen:
     *
     * * Data arriving over the network.
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
    : defs(defs)
    ,
    /*
     * Note: According to docs, the altcp_close _can fail_. Nevertheless:
     *
     * * Using altcp_abort on listening connection doesn't work.
     * * It is assumed to be able to fail due to eg. inability to send a
     *   FIN packet. This is not the case for a listening socket.
     */
    listener(nullptr, &altcp_close) {
    idle_slot.server = this;
    for (auto &slot : active_slots) {
        slot.server = this;
    }
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
    set_callbacks(new_conn, &idle_slot);

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
    altcp_poll(conn, idle_conn_wrap, IDLE_POLL_TIME);
    altcp_recv(conn, received_wrap);
    altcp_sent(conn, sent_wrap);
    altcp_arg(conn, slot);
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
    if (is_active_slot(slot)) {
        /*
         * LwIP seems to be invoking the poll timer from time to time, not when
         * inactive as documented. So track if we are or are not active.
         */
        Slot *s = static_cast<Slot *>(slot);
        if (s->seen_activity) {
            s->seen_activity = false;
            return ERR_OK;
        }
    }

    bool send_goodbye = (!is_active_slot(slot) || static_cast<Slot *>(slot)->want_read());
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
        if (altcp_close(conn) != ERR_OK) {
            altcp_abort(conn);
            return ERR_ABRT;
        }
    }

    return ERR_OK;
}

err_t Server::received_wrap(void *raw_slot, struct altcp_pcb *conn, pbuf *data, err_t err) {
    assert(raw_slot != nullptr);
    assert(conn != nullptr);

    BaseSlot *base_slot = static_cast<BaseSlot *>(raw_slot);
    if (base_slot == &base_slot->server->idle_slot) {
        if (data == nullptr) {
            // Closing an idle connection.
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
        if (Slot *active_slot = base_slot->server->find_empty_slot(); active_slot != nullptr) {
            assert(!active_slot->partial);
            assert(active_slot->partial_consumed == 0);
            assert(!active_slot->buffer);
            assert(holds_alternative<Idle>(active_slot->state));

            active_slot->state.emplace<handler::RequestParser>(*active_slot->server);
            active_slot->conn = conn;
            altcp_arg(conn, active_slot);
            altcp_poll(conn, idle_conn_wrap, ACTIVE_POLL_TIME);
            base_slot = active_slot;

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
    slot->seen_activity = true;

    if (slot->partial) {
        /*
         * We are still in the middle of the previous buffer. We don't want
         * more yet. Come back to as with it later.
         *
         * Yes, we do _not_ free the pbuf.
         */
        return ERR_MEM;
    }

    assert(slot->partial_consumed == 0);
    // data = null - other side did shutdown.
    if (data) {
        slot->partial.reset(data);
    } else {
        slot->client_closed = true;
    }

    /*
     * Will send data, consume data, free pbufs, release buffers and close
     * connections as needed.
     *
     * Any freeing of the pbuf happens in there.
     */
    while (slot->step()) {
    }

    return ERR_OK;
}

err_t Server::sent_wrap(void *raw_slot, altcp_pcb *conn, uint16_t len) {
    if (is_active_slot(raw_slot)) {
        Slot *slot = static_cast<Slot *>(raw_slot);

        assert(!holds_alternative<Idle>(slot->state));
        slot->server->sent(slot, len);
        slot->seen_activity = true;
    }

    return ERR_OK;
}

void Server::sent(Slot *slot, uint16_t len) {
    Buffer *buffer = slot->buffer;
    assert(buffer->write_pos >= buffer->acked);
    const uint16_t unacked = buffer->write_pos - buffer->acked;
    assert(len <= unacked);
    buffer->acked += len;

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
    return s != &s->server->idle_slot;
}

Server::Slot *Server::find_empty_slot() {
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
    assert(!listener);

    auto listener_alloc = defs.listener_alloc();

    altcp_pcb *l = altcp_new_ip_type(&listener_alloc, IPADDR_TYPE_ANY);

    if (l == nullptr) {
        return false;
    }
    listener.reset(l);

    /* set SOF_REUSEADDR to explicitly bind httpd to multiple
     * interfaces and to allow re-binding after enabling & disabling
     * ethernet. This is set in the prusa_alloc. */
    const auto err = altcp_bind(listener.get(), IP_ANY_TYPE, defs.port());
    if (err != ERR_OK) {
        listener.reset();
        return false;
    }

    listener.reset(altcp_listen(listener.release()));
    altcp_arg(listener.get(), this);
    altcp_accept(listener.get(), Server::accept_wrap);

    return true;
}

void Server::stop() {
    listener.reset();
    // Note: Letting the rest of the connections to live on!
}

}
