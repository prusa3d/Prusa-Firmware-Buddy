#pragma once

#include "nhttp/status_page.h"
#include "nhttp/handler.h"

#include <string_view>
#include <optional>

namespace nhttp::link_content {

enum class RemapPolicy {
    Octoprint,
    NoRemap
};

std::optional<std::string_view> remove_prefix(std::string_view input, std::string_view prefix);

std::optional<handler::ConnectionState> parse_file_url(const handler::RequestParser &parser, const size_t prefix_len, char *filename, const size_t filename_len, RemapPolicy remapPolicy);

handler::StatusPage delete_file(const char *filename, const handler::RequestParser &parser);

handler::StatusPage create_folder(const char *filename, const handler::RequestParser &parser);

handler::StatusPage print_file(char *filename, const handler::RequestParser &parser);

handler::ConnectionState get_only(handler::ConnectionState state, const handler::RequestParser &parser);
} // namespace nhttp::link_content
