#include "chunked.h"
#include "headers.h"
#include "gcode_preview.h"
#include "handler.h"

#include <sys/stat.h>

namespace nhttp::printer {

using handler::Continue;
using handler::NextInstruction;
using handler::StatusPage;
using handler::Step;
using handler::Terminating;
using std::string_view;

GCodePreview::GCodePreview(FILE *f, const char *path, bool can_keep_alive, bool json_errors, uint16_t width, uint16_t height, uint32_t if_none_match)
    : gcode(f)
    , decoder(f, width, height)
    , can_keep_alive(can_keep_alive)
    , json_errors(json_errors) {
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
        return { 0, 0, StatusPage(Status::NotModified, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, false) };
    }
    ConnectionHandling handling = can_keep_alive ? ConnectionHandling::ChunkedKeep : ConnectionHandling::Close;

    if (!headers_sent) {
        /*
         * We got a file, but we are not entirely sure this file contains any
         * preview (or is a gcode anyway). So we _first_ try to read a little
         * bit even before writing headers.
         */
        const size_t pre_read_size = 10;
        char pre_read[pre_read_size];
        int got = decoder.Read(pre_read, pre_read_size);
        if (got > 0) {
            const size_t reserve = handler::MIN_CHUNK_SIZE + got;
            size_t written = write_headers(buffer, buffer_size - reserve, Status::Ok, ContentType::ImagePng, handling, std::nullopt, etag);
            written += handler::render_chunk(handling, buffer + written, buffer_size - written, [&](uint8_t *buffer, size_t buffer_size) {
                memcpy(buffer, pre_read, got);
                return got;
            });
            headers_sent = true;
            return { 0, written, Continue() };
        } else {
            /*
             * Something is wrong. We don't care about exactly what, we simply
             * don't have the preview -> 404.
             */
            return { 0, 0, StatusPage(Status::NotFound, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, "File doesn't contain preview") };
        }
    } else {
        NextInstruction instruction = Continue();
        size_t written = handler::render_chunk(handling, buffer, buffer_size, [&](uint8_t *buffer, size_t buffer_size) {
            int got = decoder.Read(reinterpret_cast<char *>(buffer), buffer_size);
            if (got > 0) {
                return got;
            } else {
                // The decoder doesn't distinguish between error or end, so we handle both as end.
                instruction = Terminating::for_handling(handling);
                gcode.reset();
                return 0;
            }
        });
        return { 0, written, std::move(instruction) };
    }
}

}
