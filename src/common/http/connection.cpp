#include "connection.hpp"

using std::get;
using std::holds_alternative;
using std::nullopt;
using std::optional;

namespace http {

Connection::Connection(uint8_t timeout_s)
    : timeout_s(timeout_s) {}

uint8_t Connection::get_timeout_s() const { return timeout_s; }

optional<Error> Connection::tx_all(const uint8_t *data, size_t size) {
    while (size > 0) {
        auto res = tx(data, size);

        if (holds_alternative<Error>(res)) {
            return get<Error>(res);
        }

        size_t sent = get<size_t>(res);
        size -= sent;
        data += sent;
    }

    return nullopt;
}

optional<Error> Connection::rx_exact(uint8_t *data, size_t size) {
    while (size > 0) {
        auto res = rx(data, size, false);

        if (holds_alternative<Error>(res)) {
            return get<Error>(res);
        }

        size_t got = get<size_t>(res);
        if (got == 0) {
            return Error::UnexpectedEOF;
        }

        size -= got;
        data += got;
    }

    return nullopt;
}

} // namespace http
