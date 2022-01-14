/**
 * \file
 *
 * Support for uploading gcode.
 */
#pragma once

#include "req_parser.h"
#include "status_page.h"
#include "types.h"

#include <memory>

struct Uploader;

namespace nhttp::printer {

/**
 * \brief A handler to accept incoming gcodes.
 *
 * This will take a POST request with valid form containing gcode and store it
 * onto the USB drive.
 *
 * Note that this adapts the old gcode upload hack to the new server. There are
 * many things that should be improved or refactored, this was just the fastest
 * way to glue the old code in. Specifically, we want to:
 * * Support multiple simultaneous uploads.
 * * Figure a way to not inject printer-specific types into the generic server (even though it's just by listing the handler somewhere).
 * * Generalize uploading files for whatever reason (may go hand in hand with the above?).
 * * Not go C->C++->C->C++…
 * * Not having several dynamic allocation layers on the way and somehow flatten it.
 * * Propagate error _reasons_ out of the upload state.
 *
 * Flattening the Uploader in here might go part of the way.
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
