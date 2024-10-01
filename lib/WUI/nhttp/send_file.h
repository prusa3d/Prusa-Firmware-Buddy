/**
 * \file
 */
#pragma once

#include "unique_file_ptr.hpp"
#include "step.h"
#include <http/types.h>

#include <string_view>
#include <cstdio>
#include <memory>
#include <optional>

namespace nhttp::handler {

/**
 * \brief Handler to send a body from static memory.
 *
 * This sends a complete response where the body is provided as a constant. The
 * data provided is not copied anywhere and it is assumed the data is alive for
 * the whole lifetime of the handler (usually a constant in the program). This
 * allows "embedding" small/static resources directly into the program.
 */
class SendFile {
private:
    unique_file_ptr file;
    http::ContentType content_type;
    http::ConnectionHandling connection_handling = http::ConnectionHandling::Close;
    bool can_keep_alive;
    bool headers_sent = false;
    bool etag_matches = false;
    std::optional<size_t> content_length;
    std::optional<uint32_t> etag;
    const char *const *extra_hdrs;

public:
    SendFile(FILE *file, const char *path, http::ContentType content_type, bool can_keep_alive, bool json_errors, uint32_t if_none_match, const char *const *extra_hdrs = nullptr);
    void step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size, Step &out);
    bool want_write() const { return bool(file); }
    bool want_read() const { return false; }
    /*
     * Disables etags & if_none_match handling.
     *
     * This is due to some browsers reportedly malhandling Content-Disposition: attachement + etags.
     */
    void disable_caching();
};

} // namespace nhttp::handler
