#include "headers.h"
#include "gcode_preview.h"
#include "handler.h"

#include <http/chunked.h>

#include <sys/stat.h>

using namespace http;

namespace nhttp::printer {

using handler::Continue;
using handler::NextInstruction;
using handler::StatusPage;
using handler::Step;
using handler::Terminating;
using http::ContentType;
using http::Status;
using std::nullopt;
using std::string_view;

GCodePreview::GCodePreview(AnyGcodeFormatReader f, const char *path, bool can_keep_alive, bool json_errors, uint16_t width, uint16_t height, bool allow_larger, uint32_t if_none_match)
    : gcode(std::move(f))
    , can_keep_alive(can_keep_alive)
    , json_errors(json_errors)
    , width(width)
    , height(height)
    , allow_larger(allow_larger) {
    struct stat finfo;

    if (stat(path, &finfo) == 0) {
        etag = compute_etag(finfo);
        if (if_none_match == etag && etag != 0) {
            etag_matches = true;
        }
    }
}

Step GCodePreview::step(string_view, bool, uint8_t *buffer, size_t buffer_size) {
    if (etag_matches) {
        // No need to pass the json_errors, NotModified has no content anyway.
        return { 0, 0, StatusPage(Status::NotModified, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, false, etag) };
    }
    ConnectionHandling handling = can_keep_alive ? ConnectionHandling::ChunkedKeep : ConnectionHandling::Close;

    assert(gcode.is_open());
    size_t written = 0;
    if (!headers_sent) {
        /*
         * We got a file, but we are not entirely sure this file contains any
         * preview (or is a gcode anyway). So we _first_ search for the preview, and only if
         * gcode reader say there is one, we send it.
         */

        bool has_thumbnail = gcode.get()->stream_thumbnail_start(width, height, IGcodeReader::ImgType::PNG, allow_larger);
        if (!has_thumbnail) {
            /*
             * Something is wrong. We don't care about exactly what, we simply
             * don't have the preview -> 404.
             */
            return { 0, 0, StatusPage(Status::NotFound, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, nullopt, "File doesn't contain preview") };
        }

        written += write_headers(buffer, buffer_size, Status::Ok, ContentType::ImagePng, handling, std::nullopt, etag);
        buffer_size -= written;
        buffer += written;
        headers_sent = true;
    }

    NextInstruction instruction = Continue();
    if (buffer_size >= MIN_CHUNK_SIZE) {
        written += http::render_chunk(handling, buffer, buffer_size, [&](uint8_t *buffer_, size_t buffer_size_) {
            int got = 0;
            for (size_t i = 0; i < buffer_size_; i++) {
                if (gcode.get()->stream_getc(*reinterpret_cast<char *>(&buffer_[i])) != IGcodeReader::Result_t::RESULT_OK) {
                    break;
                }
                ++got;
            }
            if (got > 0) {
                return got;
            } else {
                // The decoder doesn't distinguish between error or end, so we handle both as end.
                instruction = Terminating::for_handling(handling);
                gcode.close();
                return 0;
            }
        });
    }
    return { 0, written, std::move(instruction) };
}

} // namespace nhttp::printer
