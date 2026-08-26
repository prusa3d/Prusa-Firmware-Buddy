#include <catch2/catch_test_macros.hpp>
#include <nhttp/headers.h>

#include <string>
#include <string_view>

using http::ConnectionHandling;
using http::ContentType;
using http::Status;
using std::string_view;

namespace {

string_view render(uint8_t *buffer, size_t buffer_len, const char *attachment_filename) {
    const size_t used = nhttp::write_headers(buffer, buffer_len, Status::Ok, ContentType::ApplicationOctetStream, ConnectionHandling::Close, 42, std::nullopt, nullptr, attachment_filename);
    return string_view(reinterpret_cast<char *>(buffer), used);
}

} // namespace

TEST_CASE("Attachment filename is offered percent-encoded") {
    uint8_t buffer[512];
    // "ú" = 0xC3 0xBA in UTF-8; '~' and '.' are attr-chars and stay literal.
    const auto response = render(buffer, sizeof buffer, "My Axolotl~M\xC3\xBA"
                                                        "del.gcode");
    REQUIRE(response.find("Content-Disposition: attachment; filename*=UTF-8''My%20Axolotl~M%C3%BAdel.gcode\r\n") != string_view::npos);
    REQUIRE(response.substr(response.size() - 4) == "\r\n\r\n");
}

TEST_CASE("No attachment filename, no content disposition") {
    uint8_t buffer[512];
    const auto response = render(buffer, sizeof buffer, nullptr);
    REQUIRE(response.find("Content-Disposition") == string_view::npos);
    REQUIRE(response.substr(response.size() - 4) == "\r\n\r\n");
}

TEST_CASE("The worst case headers fit MAX_HEADERS_SIZE") {
    // The longest status text, the longest content type, an etag and a content
    // length that take all the digits they can, and a maximum length filename
    // where every byte percent-encodes into three.
    const std::string filename(filename_defs::max_filename_length, ' ');
    std::string expected = "Content-Disposition: attachment; filename*=UTF-8''";
    for (size_t i = 0; i < filename_defs::max_filename_length; i++) {
        expected += "%20";
    }
    expected += "\r\n\r\n";

    uint8_t buffer[nhttp::MAX_HEADERS_SIZE];
    const size_t used = nhttp::write_headers(buffer, sizeof buffer, Status::ServiceTemporarilyUnavailable, ContentType::ApplicationOctetStream, ConnectionHandling::ChunkedKeep, UINT64_MAX, UINT32_MAX, nullptr, filename.c_str());
    const string_view response(reinterpret_cast<char *>(buffer), used);

    // Nothing was truncated - the whole name made it and so did the empty line
    // terminating the headers.
    REQUIRE(response.ends_with(expected));
}
