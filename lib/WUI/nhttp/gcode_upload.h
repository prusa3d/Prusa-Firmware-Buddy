#pragma once

#include "req_parser.h"
#include "status_page.h"
#include "types.h"

#include <memory>

struct Uploader;

namespace nhttp::printer {

/*
 * FIXME:
 *
 * This adapts the old gcode upload hack to the new server. This is not exactly right way to do it, though, we want to:
 * * Support multiple simultaneous uploads.
 * * Figure a way to not inject printer-specific types into the generic server (even though it's just by listing the handler somewhere).
 * * Generalize uploading files for whatever reason.
 * * Not go C->C++->C->C++…
 * * Not having several dynamic allocation layers on the way and somehow flatten it.
 * * Propagate error _reasons_ out of the upload state.
 *
 * Flattening this one and the Uploader together might be a good idea.
 */
class GcodeUpload {
private:
    class UploaderDeleter {
    public:
        void operator()(Uploader *uploader);
    };
    /*
     * Currently, we are limited to just one instance of gcode upload,
     * otherwise Bad Things would happen. This is a temporary "lock" for them.
     */
    static bool have_instance;
    bool holds_instance = true;
    using UploaderPtr = std::unique_ptr<Uploader, UploaderDeleter>;
    UploaderPtr uploader;
    size_t size_rest;
    GcodeUpload(UploaderPtr uploader, size_t size_rest);

public:
    bool want_read() const { return size_rest > 0; }
    bool want_write() const { return false; }
    handler::Step step(std::string_view input, bool terminated_by_client, uint8_t *output, size_t output_size);
    using UploadResult = std::variant<handler::StatusPage, GcodeUpload>;
    static UploadResult start(const handler::RequestParser &parser);
    GcodeUpload(const GcodeUpload &other) = delete;
    GcodeUpload(GcodeUpload &&other);
    GcodeUpload &operator=(const GcodeUpload &other) = delete;
    GcodeUpload &operator=(GcodeUpload &&other);
    ~GcodeUpload();
};

}
