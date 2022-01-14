#include "status_page.h"
#include "handler.h"
#include "headers.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>

namespace nhttp::handler {

Step StatusPage::step(std::string_view, bool, uint8_t *output, size_t output_size) {
    /*
     * Note: we assume the buffers has reasonable size and our payload fits. We
     * won't do out-of-range access if not, but the response would be
     * truncated.
     */
    assert(output && output_size > 0);

    const StatusText &text = StatusText::find(status);

    char content_buffer[128];
    if (status == Status::NoContent) {
        content_buffer[0] = 0;
    } else {
        snprintf(content_buffer, sizeof(content_buffer), "<html><body><h1>%u: %s</h1><p>%s", static_cast<unsigned>(status), text.text, extra_content);
    }

    ConnectionHandling handling = can_keep_alive ? ConnectionHandling::ContentLengthKeep : ConnectionHandling::Close;

    size_t used_up = write_headers(output, output_size, status, ContentType::TextHtml, handling, strlen(content_buffer), text.extra_hdrs);
    size_t rest = output_size - used_up;
    size_t write = std::min(strlen(content_buffer), rest);
    // If we use up the whole buffer, there's no \0 at the end. We are fine
    // with that, we work with byte-arrays with lenghts here.
    strncpy(reinterpret_cast<char *>(output + used_up), content_buffer, write);

    return Step { 0, used_up + write, Terminating::for_handling(handling) };
}

}
