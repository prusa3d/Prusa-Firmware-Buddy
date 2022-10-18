#pragma once
#include <array>
#include "dump.h"
#include <span>
#include <http/httpc.hpp>

namespace crash_dump {

// inline constexpr const char *server { "crashdump.dragomirecky.com" };
// inline constexpr uint16_t port { 80 };
inline constexpr const char *server { "94.142.234.223" }; // temporary server
inline constexpr uint16_t port { 8888 };                  // temporary port

inline constexpr size_t url_buff_size { 128 };

void create_url_string(std::array<char, url_buff_size> &url_buff, std::array<char, url_buff_size> &escaped_url_string, const char *board);

bool upload_dump_to_server(http::Request &req);

}
