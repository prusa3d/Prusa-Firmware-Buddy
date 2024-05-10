#pragma once

#include <http/httpc.hpp>
#include <common/version.h>

namespace connect_client {

constexpr http::HeaderOut user_agent_printer = { "User-Agent-Printer", project_firmware_name, std::nullopt };
constexpr http::HeaderOut user_agent_version = { "User-Agent-Version", project_version_full, std::nullopt };

} // namespace connect_client
