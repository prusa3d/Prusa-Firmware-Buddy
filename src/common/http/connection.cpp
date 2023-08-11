#include "connection.hpp"

namespace http {

Connection::Connection(uint8_t timeout_s)
    : timeout_s(timeout_s) {}

uint8_t Connection::get_timeout_s() const { return timeout_s; }

} // namespace http
