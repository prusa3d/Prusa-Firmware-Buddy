#pragma once

#include <http/connection.hpp>

#include <cstring>
#include <string>
#include <string_view>

class DummyConnection final : public http::Connection {
public:
    DummyConnection()
        : Connection(5) {}
    std::string sent;
    std::string received;
    virtual std::optional<http::Error> connection(const char *, uint16_t) override {
        return std::nullopt;
    }
    virtual std::variant<size_t, http::Error> rx(uint8_t *buffer, size_t len, bool /*nonblock: we never block here*/) override {
        size_t amnt = std::min(len, received.size());
        memcpy(buffer, received.data(), amnt);
        received.erase(0, amnt);
        return amnt;
    }
    virtual std::variant<size_t, http::Error> tx(const uint8_t *buffer, size_t len) override {
        sent += std::string_view(reinterpret_cast<const char *>(buffer), len);
        return len;
    }
    virtual bool poll_readable(uint32_t /*timeout: no more data will ever arrive*/) override {
        return received.size() > 0;
    }
};
