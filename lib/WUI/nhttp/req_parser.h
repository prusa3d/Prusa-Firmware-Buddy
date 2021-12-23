#pragma once

#include "types.h"

#include <automata/core.h>

#include <cstdlib>

namespace nhttp {
class ServerDefs;
}

namespace nhttp::handler {

struct RequestParser final : public automata::Execution {
    const Server *server;
    Method method = Method::Get;
    // TODO: Eventually get rid of stringy URLs and replace by enums/tokens as much as possible
    Url url = {};
    size_t url_size = 0;
    size_t content_length = 0;
    bool done = false;
    Status error_code = Status::Unknown;

    virtual automata::ExecutionControl event(automata::Event event) override;

    bool want_write() const { return false; }
    bool want_read() const { return !done; }

    Step step(std::string_view input, uint8_t *output, size_t output_size);

    RequestParser(const Server &server);

    /*
     * Are we allowed to keep the connection?
     *
     * For now, this is a stub and assume the safe "no". Check:
     * * Protocol version.
     * * Connection header, if present.
     */
    bool can_keep_alive() const { return false; }
};

}
