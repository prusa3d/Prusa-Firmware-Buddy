#pragma once

#include "types.h"

#include <automata/core.h>

#include <cstdlib>

namespace nhttp {
class ServerDefs;
}

namespace nhttp::handler {

/*
 * TODO: This one is somewhat large state. We probably don't want to keep many
 * of them alive at all times. What can we do?
 *
 * * Have one/two somewhere and assign them similarly as the output buffers.
 * * Share the space with the output buffers (we are not going to parse and
 *   send at the same time anyway).
 * * Try to have it on stack and only relocate to dynamically allocated memory?
 */
class RequestParser final : public automata::Execution {
private:
    const Server *server;
    const char *api_key = nullptr;
    bool done = false;
    /*
     * HTTP version.
     */
    uint8_t version_major = 0;
    uint8_t version_minor = 0;
    // FIXME: Parse these!
    enum class Connection {
        Unknown,
        KeepAlive,
        Close,
    };
    Connection connection = Connection::Unknown;
    std::variant<std::monostate, uint8_t, bool> auth_status;

    // TODO: Eventually get rid of stringy URLs and replace by enums/tokens as much as possible
    Url url = {};
    size_t url_size = 0;
    // TODO: Could we not have the boundary always?
    Boundary boundary_buff = {};
    size_t boundary_size = 0;

public:
    Method method = Method::Get;
    std::optional<size_t> content_length;
    Status error_code = Status::Unknown;

    virtual automata::ExecutionControl event(automata::Event event) override;

    bool want_write() const { return false; }
    bool want_read() const { return !done; }

    Step step(std::string_view input, bool terminated_by_client, uint8_t *output, size_t output_size);

    RequestParser(const Server &server);

    /*
     * Are we allowed to keep the connection?
     *
     * For now, this is a stub and assume the safe "no". Check:
     * * Protocol version.
     * * Connection header, if present.
     */
    bool can_keep_alive() const;
    bool authenticated() const;

    bool uri_filename(char *buffer, size_t buffer_len) const;
    std::string_view uri() const { return std::string_view(url.begin(), url_size); }

    std::string_view boundary() const { return std::string_view(boundary_buff.begin(), boundary_size); }
};

}
