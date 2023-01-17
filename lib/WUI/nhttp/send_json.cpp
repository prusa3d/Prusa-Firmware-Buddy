#include "send_json.h"
#include "handler.h"
#include "headers.h"

#include <timing.h>
#include <segmented_json_macros.h>
#include <http/chunked.h>
#include <transfers/monitor.hpp>

using transfers::Monitor;
using namespace json;
using namespace http;

namespace nhttp::handler {

json::JsonResult EmptyRenderer::renderState(size_t resume_point, json::JsonOutput &output, Empty &state) const {
    return generator(resume_point, output);
}

JsonResult TransferRenderer::renderState(size_t resume_point, json::JsonOutput &output, TransferState &state) const {
    // Note: We allow stale, because we already checked there was a trasnfer running
    // in prusa_link_api, so even if it ended, we want to report it.
    auto transfer_status = Monitor::instance.status(true);
    if (transfer_status.has_value() && transfer_status->id != state.transfer_id) {
        // if transfer changes mid report, bail out
        transfer_status.reset();
    }

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
        JSON_OBJ_START
            JSON_FIELD_STR("target", "usb") JSON_COMMA;
            JSON_FIELD_STR_FORMAT_G(transfer_status.has_value(), "type", "%s", to_str(transfer_status->type)) JSON_COMMA;
            JSON_FIELD_STR_FORMAT_G(transfer_status.has_value(), "size", "%d", transfer_status->expected) JSON_COMMA;
            //FIXME: Right now should be seconds from epoch start, would be much nicer
            // for us to make it seconds from print start and make the client compute the rest,
            // also it would solve some potential problems with NTP and time zones etc.
            JSON_FIELD_STR_FORMAT_G(transfer_status.has_value(), "start_time", "%d", time(nullptr) - (ticks_s() - transfer_status->start)) JSON_COMMA;
            JSON_FIELD_BOOL_G(transfer_status.has_value(), "to_print", transfer_status->print_after_upload) JSON_COMMA;
            // Note: This works, because destination cannot go from non null to null
            // (if one transfer ends and another starts mid report, we bail out)
            if (transfer_status->destination) {
                JSON_FIELD_STR_G(transfer_status.has_value(), "destination", transfer_status->destination) JSON_COMMA;
            }
            // Note: To prevent division by zero, this is in fact secure,
            // because once the check passes once, it can never stop being true
            // since neither transferred nor ticks_s can ever decrease
            if (transfer_status->transferred !=0 && ticks_s() > transfer_status->start) {
                JSON_FIELD_STR_FORMAT_G(transfer_status.has_value(), "remaining_time", "%.2f", static_cast<double>(transfer_status->expected - transfer_status->transferred) / (static_cast<double>(transfer_status->transferred) / (ticks_s() - transfer_status->start))) JSON_COMMA;
                JSON_FIELD_STR_FORMAT_G(transfer_status.has_value(), "progress", "%.2f", static_cast<double>(transfer_status->transferred) / transfer_status->expected);
            }
        JSON_OBJ_END
    JSON_END;
    // clang-format on
}

template <class Renderer>
Step SendJson<Renderer>::step(std::string_view, bool, uint8_t *buffer, size_t buffer_size) {
    const size_t last_chunk_len = strlen(LAST_CHUNK);
    size_t written = 0;
    bool first_packet = false;
    switch (progress) {
    case Progress::SendHeaders:
        written = write_headers(buffer, buffer_size, Status::Ok, ContentType::ApplicationJson, connection_handling);
        progress = Progress::SendPayload;
        first_packet = true;

        if (buffer_size - written <= MIN_CHUNK_SIZE) {
            return { 0, written, Continue() };
        }
        // Fall through, see if something more fits.
    case Progress::SendPayload:
        JsonResult render_result;

        written += render_chunk(connection_handling, buffer + written, buffer_size - written, [&](uint8_t *buffer, size_t buffer_size) {
            const auto [result, written_json] = renderer.render(buffer, buffer_size);
            render_result = result;
            return written_json;
        });

        switch (render_result) {
        case JsonResult::Complete:
            progress = Progress::EndChunk;

            break;
        case JsonResult::Incomplete:
            // Send this packet out, but ask for another one.
            return { 0, written, Continue() };
        case JsonResult::BufferTooSmall:
            // It is small, but we've alreaty taken part of it by headers. Try
            // again with the next packet.
            if (first_packet) {
                return { 0, written, Continue() };
            }
            // Fall through to the error state.
        case JsonResult::Abort:
            // Something unexpected got screwed up. We don't have a way to
            // return a 500 error, we have sent the headers out already
            // (possibly), so the best we can do is to abort the
            // connection.
            return { 0, 0, Terminating { 0, Done::CloseFast } };
        }

        // Fall through: the last chunk may fit
    case Progress::EndChunk:
        if (connection_handling == ConnectionHandling::ChunkedKeep) {
            if (written + last_chunk_len > buffer_size) {
                // Need to leave the last chunk for next packet
                return { 0, written, Continue() };
            } else {
                memcpy(buffer + written, LAST_CHUNK, last_chunk_len);
                written += last_chunk_len;
            }
        }

        progress = Progress::Done;

        return Step { 0, written, Terminating::for_handling(connection_handling) };
    case Progress::Done:
    default:
        assert(false);
        return Step { 0, 0, Continue() };
    }
}

template class SendJson<EmptyRenderer>;
template class SendJson<TransferRenderer>;

}
