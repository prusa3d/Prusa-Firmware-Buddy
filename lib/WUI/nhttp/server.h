#pragma once

#include "handler.h"

#include <automata/core.h>

#include <lwip/altcp.h>

#include <array>
#include <variant>
#include <memory>

namespace nhttp {

class ServerDefs {
public:
    virtual ~ServerDefs();
    virtual const handler::Selector *const *selectors() const = 0;
    virtual const char *get_api_key() const = 0;
    // TODO: Have our own abstraction layer for tests vs lwIP? This one is ugly and brings a lot of deps in.
    virtual altcp_allocator_t listener_alloc() const = 0;
    virtual uint16_t port() const = 0;
};

class Server {
private:
    class PbufDeleter {
    public:
        void operator()(pbuf *buff) {
            pbuf_free(buff);
        }
    };

    class ListenerDeleter {
    public:
        void operator()(altcp_pcb *conn) {
            /*
             * Note: According to docs, the altcp_close _can fail_. Nevertheless:
             *
             * * Using altcp_abort on listening connection doesn't work.
             * * It is assumed to be able to fail due to eg. inability to send a
             *   FIN packet. This is not the case for a listening socket.
             */
            altcp_close(conn);
        }
    };

    // TODO: Tune the values.
    static const size_t ACTIVE_CONNS = 3;
    static const size_t BUFF_SIZE = TCP_MSS;
    static const size_t BUFF_CNT = 2;
    // 10 half-seconds... weird units of LwIP
    static const uint8_t IDLE_POLL_TIME = 10;
    static const uint8_t ACTIVE_POLL_TIME = 4;
    /*
     * Priorities of active vs idle connections. Decides which connections are
     * killed if there's too many of them.
     */
    static const uint8_t IDLE_PRIO = TCP_PRIO_MIN;
    // -1 -> allow other connections to be created, don't hog all of them.
    static const uint8_t ACTIVE_PRIO = TCP_PRIO_NORMAL - 1;

    const ServerDefs &defs;
    struct IdleConn {};

    struct Buffer;

    class BaseSlot {
    public:
        Server *server = nullptr;
    };

    class Slot : public BaseSlot {
    private:
        void release_buffer();
        void release_partial();
        uint16_t send_space() const;
        void step(std::string_view input, uint8_t *output, size_t output_size);
        // Close whole connection and release
        bool close();

    public:
        Server *server = nullptr;
        altcp_pcb *conn = nullptr;
        handler::ConnectionState state;
        // Owning a buffer?
        Buffer *buffer = nullptr;
        bool seen_activity = false;
        // Do we have a partially processed pbuf, with data left for later?
        std::unique_ptr<pbuf, PbufDeleter> partial;
        size_t partial_consumed = 0;
        bool client_closed = false;

        void release();
        bool is_empty() const;
        bool step();
        bool want_read() const;
        bool want_write() const;
    };

    BaseSlot idle_slot;
    std::array<Slot, ACTIVE_CONNS> active_slots;
    /*
     * Rotating finger to the last slot that did something. Next time we start
     * with the next one in a row to avoid starving connections.
     */
    uint8_t last_active_slot = 0;

    struct Buffer {
        // Yet unwritten data.
        uint16_t write_pos = 0;
        uint16_t write_len = 0;
        uint16_t acked = 0;
        std::array<uint8_t, BUFF_SIZE> data;
        bool owned = false;
        void reset();
    };

    // TODO: Alternative: allocate the buffers from somewhere inside LwIP
    // buffers (there are separate ones) and handle sending according to what's
    // available?
    // TODO: Alternative: some small and some big buffers?
    std::array<Buffer, BUFF_CNT> buffers;

    std::unique_ptr<altcp_pcb, ListenerDeleter> listener;

    Slot *find_empty_slot();
    Buffer *find_empty_buffer();
    void step();

    static err_t accept_wrap(void *me, altcp_pcb *new_conn, err_t err);
    err_t accept(altcp_pcb *new_conn, err_t err);
    static void lost_conn_wrap(void *slot, err_t);
    static err_t idle_conn_wrap(void *slot, altcp_pcb *conn);
    static err_t received_wrap(void *slot, altcp_pcb *conn, pbuf *data, err_t err);
    static err_t sent_wrap(void *slot, altcp_pcb *conn, uint16_t len);
    void sent(Slot *slot, uint16_t len);
    static bool is_active_slot(void *slot);
    static void remove_callbacks(altcp_pcb *conn);
    static void set_callbacks(altcp_pcb *conn, BaseSlot *slot);

public:
    Server(const ServerDefs &defs);
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;

    bool start();
    void stop();

    const handler::Selector *const *selectors() const {
        return defs.selectors();
    }

    const char *get_api_key() const {
        return defs.get_api_key();
    }
};

}
