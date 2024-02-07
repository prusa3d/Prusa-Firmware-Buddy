/**
 * \file
 * \brief The HTTP server.
 */
#pragma once

#include "handler.h"

#include <automata/core.h>
#include <common/pbuf_deleter.hpp>

#include <lwip/altcp.h>
#include <lwip/tcpip.h>

#include <array>
#include <variant>
#include <memory>

namespace nhttp {

namespace splice {

    class Transfer;

}

/**
 * \brief Server definitions.
 *
 * This is how one plugs an actual content into the Server. This is an abstract
 * base class, an application would provide a specialization.
 */
class ServerDefs {
public:
    virtual ~ServerDefs() = default;
    /**
     * \brief List of selectors.
     *
     * The selectors are responsible for choosing the right handlers for a
     * parsed request. This is where "routing" of requests happens.
     *
     * This should be a pointer to an array of selectors (each one is an
     * implementation of the Selector abstract base class). The server assumes
     * every request will be handled by one of the provided selectors, it
     * doesn't check for "falling off" the end of the array. For that reason,
     * it is recommended to have some kind of wildcard selector.
     *
     * Also note that the parsed request may contain an invalid request of some form.
     *
     * For both of these reasons, some useful selectors are provided in
     * common_selectors.h.
     */
    virtual const handler::Selector *const *selectors() const = 0;
    /**
     * \brief Looks up a password.
     *
     * Provides the password. If this is null, all access to restricted
     * resources is denied.
     *
     * FIXME: there are (probably) thread synchronization issues:
     *
     * * If the password changes in the middle of parsing of the one provided in
     *   the headers, half of it is checked against the old one and half against
     *   the new one. We probably can live with that (it'll likely result in
     *   refusing the request; if someone can guess where the change happens
     *   and knows both old and new one, then they probably know enough to be
     *   considered authorized).
     * * The key is not copied, not locked, etc. It is just _used_. Therefore,
     *   changing the content of the memory pointed to or (worse) the address and
     *   deleting the old one is a data race/UB. We need to find a solution,
     *   but for now, changing the key is very rare and happens in-place.
     */
    virtual const char *get_apikey() const = 0;
    virtual const char *get_user_password() const = 0;
    /**
     * \brief Allocate the listener socket.
     */
    virtual altcp_pcb *listener_alloc() const = 0;
};

/**
 * \brief The HTTP server itself.
 *
 * This is the entrypoint, it manages the requests, etc. Content is plugged
 * into it by the ServerDefs class.
 *
 * Not much functionality is exposed by this class directly, most of the
 * interaction with it is done by providing it with selectors and handlers.
 *
 * In the big picture, this is responsible mostly for juggling connections and
 * buffers, but (almost) doesn't understand HTTP; it delegates that to the
 * relevant handlers.
 *
 * The server _is not_ thread safe. It must be assured by the user that all the
 * callbacks from the connections are executed in the same thread.
 */
class Server {
private:
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
            LOCK_TCPIP_CORE();
            altcp_close(conn);
            UNLOCK_TCPIP_CORE();
        }
    };

    // Inactivity tracking.
    //
    // We need to close inactive connections. Unfortunately, the LwIP API is
    // not particularly friendly to us to do so reliably. We want to make sure
    // we don't close the connection too soon, we want to err on the side of
    // allowing it to live longer.
    //
    // LwIP has the poll callback. It is called "from time to time", in
    // approximately POLL_TIME intervals (which is in half-second units).
    // Despite what the documentation says, it isn't delivered when the
    // connection is idle, it simply "ticks", with unspecified starting point
    // of the first interval.
    //
    // Furthermore, our event loop sometimes gets held up for longer time
    // (either because one of our callbacks is stuck in IO for extended period
    // of time, or because we are running in lowest-priority task and something
    // else is hogging the CPU). In that case it is possible the first callback
    // to be called for a connection is the poll callback despite the
    // connection having data available. In such case we don't want to deem the
    // connection inactive even though our last recorded "activity" was long
    // time ago.
    //
    // For this reason, we split the inactivity time into "quants". If our last
    // activity or last poll is further in the past than one quant, we decrease
    // a counter. Only once we reach 0, we consider the connection as inactive.
    // This way we extend the timeout a bit, but make sure a single delayed
    // poll which is the first one to get processed doesn't outright kill the
    // connection ‒ it'll only eat one quant and if there are queued data,
    // it'll reset it again.
    class InactivityTimeout {
    private:
        uint32_t last_activity = 0;
        uint32_t quants_left = 0;

    public:
        void schedule(uint32_t after);
        void poll_inactivity();
        bool past() const;
    };

    // TODO: Tune the values.
    static const size_t ACTIVE_CONNS = 3;
    static const size_t BUFF_SIZE = TCP_MSS;
    static const size_t BUFF_CNT = 2;
    // half-seconds... weird units of LwIP
    static const uint8_t POLL_TIME = 1;
    // Idle connections time out after 60 seconds.
    static const uint32_t IDLE_TIMEOUT = 60 * 1000;
    /*
     * Active connections are more expensive to keep around and they are
     * expected to be active, so time them out sooner.
     *
     * Note that this applies to downloads too (eg. Connect downloads).
     */
    static const uint32_t ACTIVE_TIMEOUT = 10 * 1000;
    static const uint32_t INACTIVITY_TIME_QUANT = 400;
    /*
     * Priorities of active vs idle connections. Decides which connections are
     * killed if there's too many of them.
     */
    static const uint8_t IDLE_PRIO = TCP_PRIO_MIN;
    // -1 -> allow other connections to be created, don't hog all of them.
    static const uint8_t ACTIVE_PRIO = TCP_PRIO_NORMAL - 1;

    const ServerDefs &defs;

    struct Buffer;

    // Handling timeouts and such things.
    class BaseSlot {
    public:
        enum class SlotType : uint8_t {
            BaseSlot,
            Slot,
            ConnectionSlot,
            TransferSlot,
        };

        Server *server = nullptr;
        InactivityTimeout timeout;
        virtual ~BaseSlot() = default;

        virtual constexpr SlotType get_slot_type() { return SlotType::BaseSlot; }
    };

    /*
     * We want to be able to keep arbitrary number of idle connections around.
     * That means we don't give each its own slot.
     *
     * This poses a little difficulty with timeouts on them. The altcp_poll
     * doesn't seem to work reliably (it does not set the timeout to that time,
     * it sets the interval but it can trigger early)
     *
     * So we have three slots and alternatingly (based on time) put the
     * connection in either one. That is, one is gathering connections while
     * the other is only timing out. This way the connection might time out a
     * bit later, but eventually it will (because the slot stays inactive for 2
     * iterations).
     *
     * Active connections can afford to have each their own timeout, so they
     * don't do these crazy dances.
     */
    std::array<BaseSlot, 3> idle_slots;

    /*
     * A slot in some way active.
     *
     * We actually have several modes here. One for the normal http handling -
     * parsing, generating responses, morphing from one handler to another (the
     * ConnectionSlot) and another that is mostly for just transfering data
     * from a slot to a file with as high performance as possible, that's
     * separate (needs direct access to pbufs during the handling and all
     * that, but can afford more restrictions on what can and can not happen
     * and doesn't support any kind of connection keep alive and all that).
     */
    class Slot : public BaseSlot {
    protected:
        bool close();

    public:
        altcp_pcb *conn = nullptr;

        virtual void release();
        virtual bool step() = 0;
        // Call step as long as you can, to make all forward progress as possible.
        void forward_progress();
        virtual bool want_read() const = 0;
        virtual bool want_write() const = 0;
        virtual bool take_pbuf(pbuf *data) = 0;
        // Did we send data the other side hasn't acked yet?
        virtual bool has_unacked_data() const = 0;
        // The other side has sent an ack.
        virtual void sent(uint16_t len) = 0;

        virtual constexpr SlotType get_slot_type() override = 0;
    };

    using PbufPtr = std::unique_ptr<pbuf, PbufDeleter>;

    class ConnectionSlot : public Slot {
    private:
        void release_buffer();
        uint16_t send_space() const;
        void step(std::string_view input, uint8_t *output, size_t output_size);

        bool client_closed = false;

        // Close whole connection and release
        void release_partial();

    public:
        handler::ConnectionState state;
        // Owning a buffer?
        Buffer *buffer = nullptr;
        // Do we have a partially processed pbuf, with data left for later?
        PbufPtr partial;
        size_t partial_consumed = 0;
        virtual void release() override;
        bool is_empty() const;
        virtual bool step() override;
        virtual bool want_read() const override;
        virtual bool want_write() const override;
        virtual bool take_pbuf(pbuf *data) override;
        virtual bool has_unacked_data() const override;
        virtual void sent(uint16_t len) override;
        virtual constexpr SlotType get_slot_type() override { return SlotType::ConnectionSlot; }
    };

    std::array<ConnectionSlot, ACTIVE_CONNS> active_slots;
    /*
     * Rotating finger to the last slot that did something. Next time we start
     * with the next one in a row to avoid starving connections.
     */
    uint8_t last_active_slot = 0;

    class TransferSlot : public Slot {
    public:
        static constexpr size_t PbufQueueMax = 8;
        // Queue of pbufs to process/write into the file.
        //
        // The 0 index is the oldest one (being processed right now), we append
        // to the end as needed. When we take the processed one out, we just
        // shift them (there's only few of them, the complexity of a ringbuffer
        // is not worth it).
        //
        // This stores the pbuf heads (pbufs form linked lists).
        std::array<PbufPtr, PbufQueueMax> pbuf_queue;
        // # of pbufs in the queue
        size_t pbuf_queue_size;
        // A shortcut/finger into the pbuf chain/linked list, pointing to the
        // currently processed node of pbuf_queue[0]. Owned by pbuf_queue, not
        // us.
        //
        // May be null, in that case we haven't yet started dealing with
        // pbuf_queue[0] (or, pbuf_queue_size == 0).
        pbuf *current_pbuf;
        // Number of bytes processed from the current_pbuf (relative to the
        // current node, not to the pbuf_queue[0]).
        size_t pbuf_processed;

        // Bytes of data we expect to arrive from the connection.
        size_t expected_data = 0;

        splice::Transfer *transfer = nullptr;
        std::optional<std::tuple<http::Status, const char *>> response;

        virtual void release() override;
        virtual bool step() override;
        virtual bool want_read() const override;
        virtual bool want_write() const override;
        virtual bool take_pbuf(pbuf *data) override;
        virtual bool has_unacked_data() const override;
        virtual void sent(uint16_t len) override;
        bool has_response();
        // Callback from Done Async IO request
        void make_response(ConnectionSlot *slot = nullptr);
        virtual constexpr SlotType get_slot_type() override { return SlotType::TransferSlot; }
        // Notification from PartialFile that something was written and we can try submitting more.
        static void segment_written(void *arg);
        // The same, but not in our thread (this one is wrapper that sends it to tcpip thread).
        static void send_segment_written(void *arg);
    };

    TransferSlot transfer_slot;

    /*
     * There's an activity on the given connection. Reset appropriate timeouts.
     */
    void activity(altcp_pcb *conn, BaseSlot *slot);

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

    void try_send_transfer_response(ConnectionSlot *slot);

    ConnectionSlot *find_empty_slot();
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
    /**
     * \brief Constructor
     *
     * Passes the server definitions to the server.
     *
     * The defs must be alive for the whole life of the server.
     *
     * Note that destruction of the server _is not solved_. Not even stopping
     * closes all active connections (at least not now, we don't seem to need
     * that) and the connection point inside the server. Destroying the server
     * could lead to access of the returned memory by one of those connections.
     *
     * So one either needs to ensure there are no connections alive at that
     * point (tests) or that the server is never destroyed.
     */
    Server(const ServerDefs &defs);
    /*
     * Don't ever think about moving the server between addresses.
     *
     * We put pointers inside ourselves into the LwIP connections. Moving
     * somewhere else would just mess everything.
     */
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;

    /**
     * \brief Start the server.
     *
     * This (attempts to) allocates the listening socket and starts serving
     * requests, according to the defs provided to constructor.
     *
     * It is invalid to start an already started server (one must first stop it).
     *
     * Success is provided by the return value. The reason for failure is not
     * (yet) specified.
     *
     * FIXME: Currently, we call start/stop from a different thread than the
     * callbacks. Is that OK? It actually might be, as this manipulates only
     * the listening socket and that kind of manipulation might be fine, but
     * this should be checked.
     */
    bool start();
    /**
     * \brief Closes the listening socket.
     *
     * This stops the server, it'll no longer accept any more connections. It
     * however does _not_ close the existing connections.
     */
    void stop();

    const handler::Selector *const *selectors() const {
        return defs.selectors();
    }

    const char *get_user_password() const {
        return defs.get_user_password();
    }

    const char *get_apikey() const {
        return defs.get_apikey();
    }

    void inject_transfer(altcp_pcb *conn, pbuf *data, uint16_t data_offset, splice::Transfer *transfer, size_t expected_data);
};

} // namespace nhttp
