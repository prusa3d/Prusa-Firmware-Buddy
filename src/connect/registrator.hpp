#pragma once

#include "printer.hpp"
#include "status.hpp"
#include "connection_cache.hpp"

#include <http/httpc.hpp>
#include <http/resp_parser.h>

#include <optional>

namespace connect_client {

static constexpr size_t CODE_SIZE = 25;
using Code = std::array<char, CODE_SIZE + 1>;

class Registrator {
private:
    Code code = {};
    enum class Status {
        Init,
        GotCode,
        Done,
        Error,
    };
    Status status = Status::Init;
    Printer &printer;
    // Handle error state
    // (returns OnlineStatus::Error for convenience).
    OnlineStatus bail();

    uint32_t last_comm = 0;
    size_t retries_left = 0;

public:
    Registrator(Printer &printer)
        : printer(printer) {}
    std::optional<OnlineStatus> communicate(RefreshableFactory &conn_factory);
    const char *get_code() const;
};

}
