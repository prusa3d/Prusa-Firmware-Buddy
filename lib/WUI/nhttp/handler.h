#pragma once

#include "types.h"
#include "req_parser.h"
#include "status_page.h"

#include <optional>
#include <string_view>
#include <variant>

namespace nhttp::handler {

/*
 * The request has been fully handled.
 *
 * No more actions are to be performed on this request.
 */
enum class Done {
    /// Go to the Idle state and wait for more requests.
    KeepAlive,
    /// Close after sending all the data provided now.
    Close,
    /// Close without finishing sending/receiving.
    CloseFast,
};

/*
 * Keep going using this state.
 */
class Continue {};

// Marker for an idle connection state. Does nothing.
class Idle {
public:
    bool want_read() const { return false; }
    bool want_write() const { return false; }
    Step step(std::string_view input, uint8_t *output, size_t output_size);
};

// Marker for last state of connection, just closing a request and then transitioning to idle.
struct Terminating {
    Done how;
    bool want_read() const { return false; }
    bool want_write() const { return false; }
    Step step(std::string_view input, uint8_t *output, size_t output_size);
    static Terminating for_handling(ConnectionHandling handling) {
        return Terminating { handling == ConnectionHandling::Close ? Done::Close : Done::KeepAlive };
    }
};

using ConnectionState = std::variant<
    Idle,
    RequestParser,
    StatusPage,
    // TODO: Some other generators/consumers
    // TODO: Some generic generators/consumers
    Terminating>;

using NextInstruction = std::variant<
    ConnectionState,
    Continue>;

struct Step {
    size_t read;
    size_t written;
    NextInstruction next;
};

class Selector {
public:
    virtual ~Selector();
    virtual std::optional<ConnectionState> accept(const RequestParser &request) const = 0;
};

}
