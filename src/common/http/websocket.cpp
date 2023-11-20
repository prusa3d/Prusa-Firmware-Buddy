#include "websocket.hpp"

#include <random.h>

// htons
#include <lwip/def.h>
#include <mbedtls/base64.h>
#include <mbedtls/sha1.h>

using std::array;
using std::monostate;
using std::optional;
using std::variant;

namespace http {

WebSocket::WebSocket(Connection *conn)
    : conn(conn) {}

variant<WebSocket, Error> WebSocket::from_response(const Response &response) {
    assert(response.status == Status::SwitchingProtocols);
    if (response.leftover_size > 0) {
        // We don't support a message sent by the server sooner than us.
        // Do we need to?
        return Error::WebSocket;
    }

    return WebSocket(response.conn);
}

optional<Error> WebSocket::send(Opcode opcode, bool last, uint8_t *data, size_t size) {
    uint8_t header[8];
    header[0] = static_cast<uint8_t>(last) << 7 | static_cast<uint8_t>(opcode);
    header[1] = 0b10000000;

    size_t pos = 2;

    // Variable-length encoding of size.
    if (size >= 126) {
        // Note that the protocol also supports frames larger than this. We currently don't, as we don't need it.
        assert(size <= 0xFFFF);
        // Others not allowed by the protocol to be larger
        assert(opcode == Opcode::Text || opcode == Opcode::Binary || opcode == Opcode::Continuation);
        header[1] |= 126;

        uint16_t len = htons(size);
        memcpy(header + pos, &len, sizeof len);
        pos += sizeof len;
    } else {
        header[1] |= size;
    }

    // Security note:
    //
    // The masking is a feature that prevents some attacks on proxies by
    // running a malicious javascript in a browser (See
    // https://www.rfc-editor.org/rfc/rfc6455#section-10.3). Such thing does
    // not apply to us at all, for two reasons:
    // * We are not running arbitrary untrusted Javascript (or any other untrusted thing).
    // * We are running against known environment with only known proxies (if
    //   any) that hopefully don't get confused by that attack.
    //
    // But the RFC still mandates that the client does masking, so we follow
    // that. We can afford to use the potentially not cryptographically secure
    // RNG for that (it is used only in the theoretical scenario of HW RNG
    // failure anyway).
    uint32_t key_u = rand_u();
    const uint8_t *key = reinterpret_cast<uint8_t *>(&rand_u);

    memcpy(header + pos, key, sizeof key_u);
    pos += sizeof key_u;

    if (auto err = conn->tx_all(header, pos); err.has_value()) {
        return err;
    }

    for (size_t i = 0; i < size; i++) {
        data[i] ^= key[i % 4];
    }

    return conn->tx_all(data, size);
}

variant<monostate, WebSocket::Message, Error> WebSocket::receive(optional<uint32_t> poll) {
    if (poll.has_value()) {
        if (!conn->poll_readable(*poll)) {
            return monostate {};
        }
    }

    uint8_t header[2];
    if (auto err = conn->rx_exact(reinterpret_cast<uint8_t *>(&header), sizeof header); err.has_value()) {
        return *err;
    }

    Message result;
    result.conn = conn;
    result.last = header[0] & 0b10000000;
    result.opcode = Opcode(header[0] & 0b00001111);
    bool has_command_id = header[0] & 0b01000000;
    if (header[0] & 0b00110000) {
        // Not supported / not negotiated extension
        return Error::WebSocket;
    }

    bool mask = header[1] & 0b10000000;
    if (mask) {
        // Masked from the server. Not supported.
        return Error::WebSocket;
    }

    result.len = header[1] & 0b01111111;

    if (result.len == 126) {
        uint16_t len;
        if (auto err = conn->rx_exact(reinterpret_cast<uint8_t *>(&len), sizeof len); err.has_value()) {
            return *err;
        }
        result.len = ntohs(len);
    } else if (result.len == 127) {
        // Not supported / too big
        return Error::WebSocket;
    }

    if (has_command_id) {
        uint32_t cmd_id;

        if (result.len < 4) {
            return Error::WebSocket;
        }

        if (auto err = conn->rx_exact(reinterpret_cast<uint8_t *>(&cmd_id), sizeof cmd_id); err.has_value()) {
            return *err;
        }

        result.command_id = ntohl(cmd_id);
        result.len -= 4;
    }

    return result;
}

void WebSocket::Message::ignore() {
    uint8_t buffer[128];
    while (len > 0) {
        size_t chunk = std::min(len, sizeof buffer);
        if (!conn->rx_exact(buffer, chunk)) {
            return;
        }
        len -= chunk;
    }
}

WebSocketKey::WebSocketKey() {
    // Get 16 bytes of random data
    uint8_t key[16];
    for (size_t i = 0; i < 4; i++) {
        // Good enough even with a fallback to pseudo-random source. This is
        // some kind of protection against accidental replay/accidental
        // caching.
        uint32_t r = rand_u();
        memcpy(&key[4 * i], &r, sizeof r);
    }

    size_t out_pos = 0;
    memset(request, 0, sizeof request);
    int err = mbedtls_base64_encode(reinterpret_cast<uint8_t *>(request), sizeof request, &out_pos, key, sizeof key);
    (void)err;
    assert(err == 0);

    // Fixed string from the RFC (no, this is not a mock)
    constexpr const char *tail = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    mbedtls_sha1_context ctx = {};
    mbedtls_sha1_init(&ctx);
    mbedtls_sha1_starts_ret(&ctx);
    mbedtls_sha1_update_ret(&ctx, reinterpret_cast<const uint8_t *>(request), strlen(request));
    mbedtls_sha1_update_ret(&ctx, reinterpret_cast<const uint8_t *>(tail), strlen(tail));
    uint8_t out[20];
    mbedtls_sha1_finish_ret(&ctx, out);

    mbedtls_sha1_free(&ctx);

    out_pos = 0;
    err = mbedtls_base64_encode(reinterpret_cast<uint8_t *>(response), sizeof response, &out_pos, out, sizeof out);
    assert(err == 0);
}

bool WebSocketAccept::key_matched() const {
    return key_matches && key_pos == strlen(key.response);
}

void WebSocketAccept::character(char c, HeaderName name) {
    switch (name) {
    case HeaderName::WebSocketAccept: {
        if (key_pos >= strlen(key.response)) {
            key_matches = false;
            break;
        }
        if (key.response[key_pos] != c) {
            key_matches = false;
        }
        key_pos++;
    }
    default:
        break;
    }
}

} // namespace http
