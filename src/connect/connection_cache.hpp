#pragma once

#include "printer.hpp"
#include "tls/tls.hpp"

#include <http/socket.hpp>
#include <http/httpc.hpp>

#include <variant>

namespace connect_client {

using Cache = std::variant<std::monostate, tls, http::socket_con, http::Error>;

class CachedFactory final : public http::ConnectionFactory {
private:
    const char *hostname = nullptr;
    Cache cache;

public:
    virtual std::variant<http::Connection *, http::Error> connection() override;
    virtual const char *host() override;
    virtual void invalidate() override;
    void refresh(const Printer::Config &config);
};

}
