#include "status_page.h"
#include "handler.h"
#include "headers.h"
#include "req_parser.h"

#include <json_encode.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cinttypes>

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
}

using http::ConnectionHandling;
using http::ContentType;
using http::Status;

namespace nhttp::handler {

StatusPage::StatusPage(http::Status status, const RequestParser &parser, const char *extra_content)
    : extra_content(extra_content)
    , status(status)
    , json_content(parser.accepts_json) {
    auto default_close_handling = parser.can_keep_alive() ? CloseHandling::KeepAlive : CloseHandling::Close;
    switch (parser.method) {
    case http::Get:
    case http::Head:
    case http::Delete:
        close_handling = default_close_handling;
        break;
    default:
        close_handling = status >= 300 ? CloseHandling::ErrorClose : default_close_handling;
        break;
    }
}
StatusPage::StatusPage(http::Status status, CloseHandling close_handling, bool json_content, const char *extra_content)
    : extra_content(extra_content)
    , status(status)
    , close_handling(close_handling)
    , json_content(json_content) {}
Step StatusPage::step_impl(std::string_view, bool, uint8_t *output, size_t output_size, const char *const *extra_hdrs) {
    /*
     * Note: we assume the buffers has reasonable size and our payload fits. We
     * won't do out-of-range access if not, but the response would be
     * truncated.
     */
    assert(output && output_size > 0);

    const StatusText &text = StatusText::find(status);

    char content_buffer[128];
    ContentType ct = ContentType::ApplicationOctetStream; // "Unknown" content type
    if (status == Status::NoContent || status == Status::NotModified) {
        content_buffer[0] = 0;
        ct = ContentType::TextPlain;
    } else if (json_content) {
        const char *title = text.text;
        JSONIFY_STR(title);
        JSONIFY_STR(extra_content);
        snprintf(content_buffer, sizeof(content_buffer), "{\"title\": \"%u: %s\",\"message\":\"%s\"}", static_cast<unsigned>(status), title_escaped, extra_content_escaped);
        ct = ContentType::ApplicationJson;
    } else {
        snprintf(content_buffer, sizeof(content_buffer), "%u: %s\n\n%s\n", static_cast<unsigned>(status), text.text, extra_content);
        ct = ContentType::TextPlain;
    }

    ConnectionHandling handling = close_handling == CloseHandling::KeepAlive ? ConnectionHandling::ContentLengthKeep : ConnectionHandling::Close;

    /*
     * TODO: We might also want to include the Content-Location header with a
     * link to html version of the error. How to pass the extra text needs to
     * be solved.
     *
     * https://dev.prusa3d.com/browse/BFW-2451
     */
    size_t used_up = write_headers(output, output_size, status, ct, handling, strlen(content_buffer), std::nullopt, extra_hdrs);
    size_t rest = output_size - used_up;
    size_t write = std::min(strlen(content_buffer), rest);
    // Copy without the \0, we don't need it.
    memcpy(output + used_up, content_buffer, write);

    Terminating term = close_handling == CloseHandling::ErrorClose ? Terminating::error_termination() : Terminating::for_handling(handling);
    return Step { 0, used_up + write, term };
}

Step StatusPage::step(std::string_view input, bool terminated_by_client, uint8_t *output, size_t output_size) {
    return step_impl(input, terminated_by_client, output, output_size, nullptr);
}

UnauthenticatedStatusPage::UnauthenticatedStatusPage(const RequestParser &parser, AuthMethod auth_method)
    : StatusPage(Status::Unauthorized, parser, "")
    , auth_method(auth_method) {}

Step UnauthenticatedStatusPage::step(std::string_view input, bool terminated_by_client, uint8_t *output, size_t output_size, DigestAuth digest_auth) {
    const char *stale = digest_auth.nonce_stale ? "true" : "false";
    char digest_header[88];
    snprintf(digest_header, sizeof(digest_header), "WWW-Authenticate: Digest realm=\"" AUTH_REALM "\", nonce=\"%016" PRIx64 "\", stale=\"%s\"\r\n", digest_auth.nonce, stale);

    const char *auth_header[] = {
        digest_header,
        nullptr
    };
    return step_impl(input, terminated_by_client, output, output_size, auth_header);
}

Step UnauthenticatedStatusPage::step(std::string_view input, bool terminated_by_client, uint8_t *output, size_t output_size, ApiKeyAuth api_key_auth) {

    const char *api_key_header = "WWW-Authenticate: ApiKey realm=\"" AUTH_REALM "\"\r\n";
    const char *auth_header[] = {
        api_key_header,
        nullptr
    };
    return step_impl(input, terminated_by_client, output, output_size, auth_header);
}

Step UnauthenticatedStatusPage::step(std::string_view input, bool terminated_by_client, uint8_t *output, size_t output_size) {
    return std::visit([&](auto auth) { return step(input, terminated_by_client, output, output_size, auth); }, auth_method);
}
}
