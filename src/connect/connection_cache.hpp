#pragma once

#include "printer.hpp"
#include "tls/tls.hpp"

#include <http/socket.hpp>
#include <http/httpc.hpp>

#include <variant>

namespace connect_client {

using Cache = std::variant<std::monostate, tls, http::socket_con, http::Error>;

// For testing purposes
class RefreshableFactory : public http::ConnectionFactory {
public:
    virtual void refresh(const Printer::Config &config) = 0;
    virtual bool is_valid() const = 0;
};

class CachedFactory final : public RefreshableFactory {
private:
    const char *hostname = nullptr;
    Cache cache;

public:
    virtual std::variant<http::Connection *, http::Error> connection() override;
    virtual const char *host() override;
    virtual void invalidate() override;
    virtual void refresh(const Printer::Config &config) override;
    virtual bool is_valid() const override;
};

} // namespace connect_client
