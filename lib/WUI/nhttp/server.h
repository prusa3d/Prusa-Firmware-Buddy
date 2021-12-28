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
    virtual const handler::Selector **selectors() const = 0;
    virtual const char *get_api_key() const = 0;
    // TODO: Have our own abstraction layer for tests vs lwIP? This one is ugly and brings a lot of deps in.
    virtual altcp_allocator_t listener_alloc() const = 0;
    virtual uint16_t port() const = 0;
};

class Server {
private:
    // TODO: Tune the values.
    static const size_t ACTIVE_CONNS = 3;
    static const size_t BUFF_SIZE = TCP_MSS;
    static const size_t BUFF_CNT = 2;
    // 30 half-seconds... weird units of LwIP
    static const uint8_t IDLE_POLL_TIME = 30;
    static const uint8_t ACTIVE_POLL_TIME = 4;

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
        bool want_read() const;
        bool want_write() const;
        void step(std::string_view input, uint8_t *output, size_t output_size);
        // Close whole connection and release
        bool close();

    public:
        Server *server = nullptr;
        altcp_pcb *conn = nullptr;
        handler::ConnectionState state;
        // Owning a buffer?
        Buffer *buffer = nullptr;
        // Do we have a partially processed pbuf, with data left for later?
        std::unique_ptr<pbuf, typeof(&pbuf_free)> partial;
        size_t partial_consumed = 0;
        bool client_closed = false;

        void release();
        bool is_empty() const;
        bool step();
        Slot();
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

    std::unique_ptr<altcp_pcb, typeof(&altcp_close)> listener;

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

public:
    Server(const ServerDefs &defs);
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;

    bool start();
    void stop();

    const handler::Selector **selectors() const {
        return defs.selectors();
    }
};

}
