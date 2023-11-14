#pragma once

#include "httpc.hpp"

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
//
// We also add our own extension "Commands". It:
// * Allocates one new opcode.
// * Reserves the first reserved bit to mean "has a command ID". If it
//   is set to 1, there's 4 bytes of extension data (preceding the
//   payload, as per the RFC), which is a 32-bit unsigned int in
//   network byte order, meaning the command ID.
// * We do not expect to get a message from the server right away
//   (that is, we don't expect there to be a "body" leftover when we
//   decode the response); we expect to be sending the first message
//   ourselves.
class WebSocket {
private:
    Connection *conn;
    WebSocket(Connection *connection);

public:
    // https://www.rfc-editor.org/rfc/rfc6455#section-1.6
    enum Opcode : uint8_t {
        Continuation = 0,
        Text = 1,
        Binary = 2,
        // Extension Commands
        Gcode = 3,
        Close = 8,
        Ping = 9,
        Pong = 10,
    };

    struct Message {
        Opcode opcode;
        size_t len;
        Connection *conn;
        // Masking key (if present)
        //
        // (Using it is caller's responsibility)
        std::optional<std::array<uint8_t, 4>> key;
        // Already decoded to host byte order.
        //
        // In case it is present, len is already adjusted by the size
        // of the "extension data".
        std::optional<uint32_t> command_id;
        // Last fragment?
        bool last;
    };

    static std::variant<WebSocket, Error> from_response(const Response &response);
    // Send a fragment.
    //
    // Note that fragmenting messages is up to the caller.
    std::optional<Error> send(Opcode opcode, bool last, uint8_t *data, size_t size);

    // Reads the header part of a message.
    //
    // Returns:
    // * monostate: In case of poll = true and no message available.
    // * Message: the message header (the body follows, but it's up to
    //   the caller to read it).
    // * Error: Error happened.
    //
    // poll: Check for availability of the message and return
    // after the given timeout (in ms) if not available.
    //
    // Note that if the server sent only part of the header, this'll
    // block before it returns even if poll is set to true.
    std::variant<std::monostate, Message, Error> receive(std::optional<uint32_t> poll);
};

} // namespace http
