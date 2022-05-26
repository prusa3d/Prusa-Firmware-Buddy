#pragma once

#include "types.h"

#include <optional>
#include <string_view>

namespace nhttp::handler {

class StatusPage {
public:
    enum class CloseHandling {
        Close,
        /*
         * Close for error states.
         *
         * First closes the outward side, then eats all the data.
         */
        ErrorClose,
        KeepAlive,
    };

private:
    const char *extra_content;
    Status status;
    CloseHandling close_handling;
    bool json_content;

public:
    StatusPage(Status status, CloseHandling close_handling, bool json_content, const char *extra_content = "")
        : extra_content(extra_content)
        , status(status)
        , close_handling(close_handling)
        , json_content(json_content) {}
    bool want_read() const { return false; }
    bool want_write() const { return true; }
    Step step(std::string_view input, bool terminated_by_client, uint8_t *output, size_t output_size);
};

}
