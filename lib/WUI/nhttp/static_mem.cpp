#include "static_mem.h"
#include "handler.h"
#include "headers.h"

#include <cstring>

using http::ConnectionHandling;
using http::Status;
using std::string_view;

namespace nhttp::handler {

bool SendStaticMemory::is_done() const {
    return headers_sent && data.empty();
}

void SendStaticMemory::step(string_view, bool, uint8_t *buffer, size_t buff_len, Step &out) {
    if (buffer) {
        size_t sent = 0;
        ConnectionHandling handling = can_keep_alive ? ConnectionHandling::ContentLengthKeep : ConnectionHandling::Close;
        if (!headers_sent) {

            sent = write_headers(buffer, buff_len, Status::Ok, content_type, handling, data.size(), std::nullopt, extra_hdrs);

            headers_sent = true;
        }

        size_t data_to_send = std::min(buff_len - sent, data.size());
        memcpy(buffer + sent, data.begin(), data_to_send);
        data = data.substr(data_to_send);
        if (is_done()) {
            out = Step { 0, sent + data_to_send, Terminating::for_handling(handling) };
        } else {
            out = Step { 0, sent + data_to_send, Continue() };
        }
    }
}

} // namespace nhttp::handler
