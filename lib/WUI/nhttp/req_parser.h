/**
 * \file
 * \brief HTTP request parser.
 */
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
/**
 * \brief HTTP request parser.
 *
 * This is the handler used by the server on a start of connection. It is
 * responsible to parse the request headers (any optional body is left
 * untouched).
 *
 * It'll use the server-provided selectors to transition into the next handler.
 *
 * The selector gets a read-only access to the parser, which contains the
 * already parsed state. It is up to the selector to extract whatever needed
 * and pass it on onto the produced handler. Most of the parsed state is
 * available as simple variables, but there are also few methods to interpret
 * the parsed values.
 */
class RequestParser final : public automata::Execution {
private:
    const Server *server;
    const char *api_key = nullptr;

public:
    /*************   These are the parsed variables that can be used by the selector *******/
    std::optional<size_t> content_length;
    /**
     * \brief Decoded etag.
     *
     * We want the browser to cache things ‒ static files, gcode downloads,
     * gcode thumbnails, etc. For that reason, we give the client an etag based
     * on the file metadata. To save space, we use a numeric representation
     * (part of a hash). The chance of the file having the same name, same FS
     * metadata and a different content is pretty low, same as the chance of
     * collision on that 32-bit value and the damage would be small.
     *
     * As we produce the etag in the first place, we can decode it when
     * receiving. A client can give us a wrong one, which would produce garbage
     * ‒ but that can lead only to either false hit or false miss and that
     * hurts mostly the client, not us ‒ so they deserve it for the
     * misbehaviour.
     */
    uint32_t if_none_match = 0;
    Method method : 4;
    Status error_code : 10;

private:
    bool done : 1;
    /*
     * HTTP version.
     */
    uint8_t version_major : 2;
    uint8_t version_minor : 4;
    // FIXME: Parse these!
    enum Connection {
        Unknown,
        KeepAlive,
        Close,
    };
    Connection connection : 2;

public:
    bool accepts_json : 1;

private:
    std::variant<std::monostate, uint8_t, bool> auth_status;

    // TODO: Eventually get rid of stringy URLs and replace by enums/tokens as much as possible
    // Note: The same buffer is also reused for boundary, that lives just behind it.
    Url url = {};
    uint8_t url_size = 0;
    uint8_t boundary_size = 0;

public:
    /*************   This part makes it a valid Handler and is used by the Server ***********/
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
    /**
     * \brief Is the request authenticated by a valid api key?
     */
    bool authenticated() const;

    bool uri_filename(char *buffer, size_t buffer_len) const;
    std::string_view uri() const { return std::string_view(url.begin(), url_size); }

    /// Multipart boundary (or empty string if not present)
    std::string_view boundary() const { return std::string_view(url.begin() + url_size, boundary_size); }
};

}
