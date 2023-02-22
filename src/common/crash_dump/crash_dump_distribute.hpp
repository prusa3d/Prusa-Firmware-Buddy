#pragma once
#include <array>
#include "dump.hpp"
#include <span>
#include <http/httpc.hpp>

namespace crash_dump {

inline constexpr const char *server { "" }; // Empty -> disabled
inline constexpr uint16_t port { 8888 }; // temporary port

inline constexpr size_t url_buff_size { 128 };

void create_url_string(std::array<char, url_buff_size> &url_buff, std::array<char, url_buff_size> &escaped_url_string, const char *board);

bool upload_dump_to_server(http::Request &req);

} // namespace crash_dump
