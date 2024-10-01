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

// Returns success (or not) of the parsing.
//
// In case of success (true), the filename is set.
// In case of failure (false), the http response is set to out.
bool parse_file_url(const handler::RequestParser &parser, const size_t prefix_len, char *filename, const size_t filename_len, RemapPolicy remapPolicy, handler::Step &out);

handler::StatusPage delete_file(const char *filename, const handler::RequestParser &parser);

handler::StatusPage create_folder(const char *filename, const handler::RequestParser &parser);

handler::StatusPage print_file(char *filename, const handler::RequestParser &parser);

void get_only(handler::ConnectionState state, const handler::RequestParser &parser, handler::Step &out);
} // namespace nhttp::link_content
