#pragma once

#include <http/httpc.hpp>
#include <version/version.hpp>

namespace connect_client {

constexpr http::HeaderOut user_agent_printer = { "User-Agent-Printer", version::project_firmware_name, std::nullopt };
constexpr http::HeaderOut user_agent_version = { "User-Agent-Version", version::project_version_full, std::nullopt };

} // namespace connect_client
