/**
 * \file
 *
 * The handler framework for the HTTP server.
 *
 * A handler is part of handling of a HTTP request. It can read some data,
 * write some data and then pass the bucket to some other handler. Eventually
 * it reaches a termination handler.
 *
 * We put all the handlers into a variant. Note that they do _not_ inherit from
 * some common interface/ancestor, they only have to provide the correct
 * methods to be usable.
 *
 * * `want_read` - the handler wishes to receive some data from the client.
 *   Note that the wish may change during the lifetime of the handler.
 * * `want_write` - the handler wishes to send some data to the client. Note
 *   that the wish may change during the lifetime of the handler.
 * * `step` - performs one step of computation. Depending on the wishes
 *   (signaled by the above methods) and what is available (data from client,
 *   send buffers), it is called with at least one of data from client and a send
 *   buffer (of at least the size specified in constant in the Server class),
 *   possibly with both. It shall do some progress and specify how much it did
 *   and what shall be done next.
 *
 * It is allowed (and preferred) to write some data and signal the end right
 * away (if the data fits), the server will handle the delivery while letting
 * the next handler perform what it needs doing.
 */
#pragma once

#include "types.h"
#include "file_info.h"
#include "gcode_upload.h"
#include "gcode_preview.h"
#include "job_command.h"
#include "req_parser.h"
#include "send_file.h"
#include "stateless_json.h"
#include "static_mem.h"
#include "status_page.h"

#include <optional>
#include <string_view>
#include <variant>

namespace nhttp::handler {

/**
 * \brief The request has been fully handled.
 *
 * No more actions are to be performed on this request. This specifies how to
 * terminate request and if the connection should be terminated too.
 */
enum class Done {
    /// Go to the Idle state and wait for more requests.
    KeepAlive,
    /// Close after sending all the data provided now.
    Close,
    /// Close without finishing sending/receiving.
    CloseFast,
};

/**
 * \brief Keep going using this state.
 *
 * Marker type to signal the server that the current handler withes to be
 * called again next time.
 */
class Continue {};

/**
 * \brief Marker for an idle connection state. Does nothing.
 *
 * This is basically a plug for connections that are not doing anything at the
 * moment, but the type system/variant still needs something to reside there.
 * It provides all the necessary methods, but does nothing.
 *
 * Mostly for internal Server use.
 */
class Idle {
public:
    bool want_read() const { return false; }
    bool want_write() const { return false; }
    Step step(std::string_view input, bool terminated_by_client, uint8_t *output, size_t output_size);
};

/**
 * \brief Marker for last state of connection.
 *
 * A handler might return this as the next handler to signal the request is
 * fully done. The server will finish writing all data and either close the
 * connection or transition to Idle on its own.
 */
struct Terminating {
    Done how;
    bool want_read() const { return false; }
    bool want_write() const { return false; }
    Step step(std::string_view input, bool terminated_by_client, uint8_t *output, size_t output_size);
    static Terminating for_handling(ConnectionHandling handling) {
        return Terminating { handling == ConnectionHandling::Close ? Done::Close : Done::KeepAlive };
    }
};

/**
 * \brief All the different handlers currently supported by the server.
 *
 * For now, we list all the possible handlers the server may contain here. This
 * allows the server to know the sizes at compile time. But it also introduces
 * dependencies between types and headers, which is slightly uncomfortable and
 * annoying. Better solution (one where we still reserve the space, with
 * compiler-computed size, but don't list it upfront) would be nice. However,
 * we don't want to template the whole server either (because we would have to
 * put everything into its header file).
 */
using ConnectionState = std::variant<
    Idle,
    RequestParser,
    StatusPage,
    SendStaticMemory,
    SendFile,
    StatelessJson,
    printer::GcodeUpload,
    printer::GCodePreview,
    printer::JobCommand,
    printer::FileInfo,
    // TODO: Some other generators/consumers
    // TODO: Some generic generators/consumers
    Terminating>;

/**
 * \brief Instruction on what to do next, coming from a handler.
 *
 * A handler may either provide the next handler here or signal it wants to continue being in use.
 */
using NextInstruction = std::variant<
    ConnectionState,
    Continue>;

/**
 * \brief The full response of the handler's step method.
 */
struct Step {
    /**
     * \brief How many bytes of the input was consumed.
     *
     * The consumed bytes won't be passed to any future step invocations and
     * may be acked/thrown away. The handler may but doesn't have to consume
     * all the provided data, but consuming nothing (in case a wish to read was
     * specified) may lead to livelocks/infinite loops.
     */
    size_t read;
    /**
     * \brief How many bytes were written into the send buffer.
     *
     * Given number of bytes will be sent to the client.
     */
    size_t written;
    /**
     * \brief Instruction what to do next.
     *
     * Either Continue using this handler (call it again once there are data to
     * be read or buffers to be filled) or another handler to pass the control to.
     */
    NextInstruction next;
};

/**
 * \brief Selector base class.
 *
 * The server contains a chain of selectors. A selector can take a parsed
 * request and decide to handle it, by returning a handler. It may decide to
 * not deal with the request, in which case the request is passed to the next
 * selector, until one matches and returns something.
 *
 * It is important that the chain always selects some action ‒ the server has
 * no handling for requests "falling off" the end of the chain. A 404 catch-all
 * handler is a good thing to have at the end (see the common_selectors.h).
 */
class Selector {
public:
    virtual ~Selector();
    virtual std::optional<ConnectionState> accept(const RequestParser &request) const = 0;
};

}
