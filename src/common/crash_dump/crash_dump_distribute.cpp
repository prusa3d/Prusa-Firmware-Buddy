#include "crash_dump_distribute.hpp"
#include <http/socket_connection_factory.hpp>
#include <version/version.hpp>
#include <otp.hpp>
#include <printers.h>
#include <cstdio>
#include <logging/log.hpp>
#include <cstring>

LOG_COMPONENT_DEF(CrashDump, logging::Severity::info);

namespace crash_dump {

inline constexpr uint8_t socket_timeout_s { 6 };

bool escape_url_string(std::span<char> escaped_url_string, const std::array<char, url_buff_size> &url_buff) {
    // escaping of special characters, should probably be done elswhere (somewhere that handles the url string)
    size_t cur_written { 0 };
    escaped_url_string[0] = '\0'; // ensure initialization
    for (size_t i = 0; i < strlen(url_buff.data()); ++i) {
        if (cur_written >= escaped_url_string.size() - 4) { // lazy, we're writing max 3 chars in a step (+ need '\0')
            return false;
        }
        switch (url_buff[i]) {
        case ':':
            strcat(escaped_url_string.data(), "%3A");
            cur_written += 3;
            break;
        case '+':
            strcat(escaped_url_string.data(), "%2B");
            cur_written += 3;
            break;
        case '\0': // last char
            break;
        default: {
            auto cur_len = strlen(escaped_url_string.data());
            escaped_url_string[cur_len] = url_buff[i];
            escaped_url_string[cur_len + 1] = '\0';
            cur_written += 1;
            break;
        }
        }
    }
    return true;
}

void create_url_string(std::array<char, url_buff_size> &url_buff, std::array<char, url_buff_size> &escaped_url_string, const char *board) {
    [[maybe_unused]] auto rc = snprintf(url_buff.data(), url_buff.size(), "/?printer_type=%s&version=%s&mac=%s&board=%s", PrinterModelInfo::current().id_str, version::project_version_full, otp_get_mac_address_str().data(), board);
    assert(static_cast<size_t>(rc) < url_buff_size); // either wanted to write too much, or an error (negative, casted to big unsigned)
    if (!escape_url_string(escaped_url_string, url_buff)) {
        assert(false); // doesn't fit
    }
}

bool upload_dump_to_server(http::Request &req) {
    if (server[0] == '\0') {
        // No server set up -> disable crash dumps.
        return false;
    }
    http::SocketConnectionFactory conn_factory(server, port, socket_timeout_s);
    http::HttpClient http(conn_factory);

    if (auto result = http.send(req); std::holds_alternative<http::Error>(result)) {
        log_error(CrashDump, "Error sending dump to server %u",
            static_cast<unsigned>(std::get<http::Error>(result)));
        return false;
    }

    return true;
}
}; // namespace crash_dump
