#pragma once

#include "command.hpp"

#include <common/shared_buffer.hpp>

#include <variant>

namespace connect_client {

class Printer;

struct BackgroundGcode {
    // Stored without \0 at the back.
    SharedBorrow data;
    size_t size;
    size_t position;
};

enum class BackgroundResult {
    Success,
    Failure,
    More,
    Later,
};

using BackgroundCmd = std::variant<BackgroundGcode>;

BackgroundResult background_cmd_step(BackgroundCmd &cmd, Printer &printer);

} // namespace connect_client
