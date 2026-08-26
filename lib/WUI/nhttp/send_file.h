/**
 * \file
 */
#pragma once

#include "unique_file_ptr.hpp"
#include "step.h"
#include <http/types.h>
#include <buddy/filename_defs.hpp>

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
    char attachment_filename[filename_defs::filename_buffer_size] = "";

public:
    SendFile(FILE *file, const char *path, http::ContentType content_type, bool can_keep_alive, bool json_errors, uint32_t if_none_match, const char *const *extra_hdrs = nullptr);
    void step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size, Step &out);
    bool want_write() const { return bool(file); }
    bool want_read() const { return false; }
    /// Offers the file for download instead of letting the client display it.
    ///
    /// The name offered for "Save as" is the LFN of the path, which may differ
    /// from the one in the request URL (that one has to be the SFN, long names
    /// might not fit the URL length limits). The path is temporarily modified
    /// in place, see get_LFN.
    ///
    /// Implies disable_caching, some browsers reportedly malhandle
    /// Content-Disposition + etags.
    void set_attachment(char *path);
    /// Disables etags & if_none_match handling.
    void disable_caching();
};

} // namespace nhttp::handler
