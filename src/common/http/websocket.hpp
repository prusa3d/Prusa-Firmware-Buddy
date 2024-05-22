#pragma once

#include "httpc.hpp"
#include "resp_parser.h"
#include "buffered_conn.hpp"

namespace http {

// A websocket connection.
//
// This is the "low-level" framing, not any logic on top (eg. doesn't
// automatically handle Close or Ping messages, or continuations of
// frames into messages.
//
// References:
// https://en.wikipedia.org/wiki/WebSocket
// https://www.rfc-editor.org/rfc/rfc6455
//
// Missing parts:
//
// * We assume we are the client (that is, we mask outgoing messages).
// * We don't support payloads of a single fragment larger than 2^16,
//   in either direction.
// * We don't do proper close.
// * We do not expect to get a message from the server right away
//   (that is, we don't expect there to be a "body" leftover when we
//   decode the response); we expect to be sending the first message
//   ourselves.
class WebSocket {
private:
    BufferedConnection connection;
    WebSocket(Connection *connection, const uint8_t *data, size_t len);

public:
    // https://www.rfc-editor.org/rfc/rfc6455#section-1.6
    enum Opcode : uint8_t {
        Continuation = 0,
        Text = 1,
        Binary = 2,
        Close = 8,
        Ping = 9,
        Pong = 10,
    };

    struct FragmentHeader {
        Opcode opcode;
        size_t len;
        Connection *conn;
        // Masking key (if present)
        //
        // (Using it is caller's responsibility)
        std::optional<std::array<uint8_t, 4>> key;
        // Last fragment?
        bool last;

        /// "Ignore" the message by reading the body and throwing it away.
        ///
        /// No errors are handled, they are left for the next use of the
        /// connection.
        void ignore();
    };

    static WebSocket from_response(const Response &response);
    // Send a fragment.
    //
    // Note that fragmenting messages is up to the caller.
    std::optional<Error> send(Opcode opcode, bool last, uint8_t *data, size_t size);

    // Reads the header part of a message.
    //
    // Returns:
    // * monostate: In case of poll = true and no message available.
    // * FragmentHeader: the fragment header (the body follows, but it's up to
    //   the caller to read it).
    // * Error: Error happened.
    //
    // poll: Check for availability of the message and return
    // after the given timeout (in ms) if not available.
    //
    // Note that if the server sent only part of the header, this'll
    // block before it returns even if poll is set to true.
    std::variant<std::monostate, FragmentHeader, Error> receive(std::optional<uint32_t> poll);

    Connection *inner_connection() {
        return &connection;
    }
};

// The Sec-WebSocket-Key request and response
//
// https://www.rfc-editor.org/rfc/rfc6455#section-4.1
// https://www.rfc-editor.org/rfc/rfc6455#section-4.2
// https://www.rfc-editor.org/rfc/rfc6455#section-11.3.1
class WebSocketKey {
    // Allow for unit tests...
protected:
    char request[25];
    char response[30];

    friend class WebSocketAccept;

    void compute_response();

public:
    WebSocketKey();
    const char *req() const {
        return request;
    }
};

class WebSocketAccept final : public ExtraHeader {
private:
    const WebSocketKey &key;
    size_t key_pos = 0;
    bool key_matches = true;
    bool conn_upgrade = false;
    bool upgrade_ws = false;
    bool protocol_connect = false;

public:
    WebSocketAccept(const WebSocketKey &key)
        : key(key) {}
    virtual void character(char c, HeaderName name) override;
    bool key_matched() const;
    bool all_supported() const;
};

} // namespace http
