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
    // Handle error states
    //
    // Passes the error through for convenience.
    //
    // retries:
    // 0: run out of retries, really error out.
    // nullopt: infinity
    CommResult bail(RefreshableFactory &conn_factory, OnlineError error, std::optional<uint8_t> retries);

    uint32_t last_comm = 0;
    // Max number of times we try to get the initial code.
    //
    // (When we have it, we keep trying forever).

    uint8_t retries_left = starting_retries;

public:
    static constexpr uint8_t starting_retries = 3;
    Registrator(Printer &printer)
        : printer(printer) {}
    CommResult communicate(RefreshableFactory &conn_factory);
    const char *get_code() const;
    uint8_t get_retries_left() const;
};

} // namespace connect_client
