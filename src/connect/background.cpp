#include "background.hpp"
#include "printer.hpp"

#include <logging/log.hpp>
#include <bsod/bsod.h>

LOG_COMPONENT_REF(connect);

using std::visit;

namespace connect_client {

namespace {

    BackgroundResult step(BackgroundGcode &gcode, Printer &printer) {
        if (auto content = get_if<BackgroundGcodeContent>(&gcode); content != nullptr) {
            if (buddy::cork::tracker.clear_cnt() != content->start_tracker_clears) {
                log_debug(connect, "Queue got cleared while submitting a gcode command");
                return BackgroundResult::Failure;
            }

            if (content->size <= content->position) {
                // All lines from the server are submitted. We need to "cork" it and wait for the execution.
                auto cork = buddy::cork::tracker.new_cork();
                if (cork.has_value()) {
                    gcode = BackgroundGcodeWait {
                        .cork = std::move(*cork),
                        .start_tracker_clears = content->start_tracker_clears,
                        .submitted = false,
                    };

                    return BackgroundResult::More;
                } else {
                    // We are low on the corks?? Well, wait for the cork to become available.
                    // (we'll get into this same position again next loop).
                    return BackgroundResult::Later;
                }
            }

            // In C++, it's a lot of work to convert void * -> char * or uint8_t * ->
            // char *, although it's both legal conversion (at least in this case). In
            // C, that works out of the box without casts.
            const char *start = reinterpret_cast<const char *>(content->data->data()) + content->position;
            const size_t tail_size = content->size - content->position;

            const char *newline = reinterpret_cast<const char *>(memchr(start, '\n', tail_size));
            // If there's no newline at all, pretend that there's one just behind the end.
            const size_t end_pos = newline != nullptr ? newline - start : tail_size;

            // We'll replace the \n with \0
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla" // TODO: person who knows a reasonable buffer size should refactor this code to not use variable length array
            char gcode_buf[end_pos + 1];
#pragma GCC diagnostic pop
            memcpy(gcode_buf, start, end_pos);
            gcode_buf[end_pos] = '\0';

            // Skip whitespace at the start and the end
            // (\0 isn't a space, so it works as a stop implicitly)
            char *g_start = gcode_buf;
            while (isspace(*g_start)) {
                g_start++;
            }

            char *g_end = g_start + strlen(g_start) - 1;
            while (g_end >= g_start && isspace(*g_end)) {
                *g_end = '\0';
                g_end--;
            }

            // Skip over empty ones to not hog the queue
            if (strlen(g_start)) {
                switch (printer.submit_gcode(g_start)) {
                case Printer::GcodeResult::Submitted:
                    log_debug(connect, "Gcode submitted to marlin: %s", g_start);
                    break;
                case Printer::GcodeResult::Later:
                    log_debug(connect, "Gcode doesn't fit into queue yet: %s", g_start);
                    // In case this gcode doesn't fit, retry again without moving
                    // the position - we'll reparse it next time.
                    return BackgroundResult::More;
                case Printer::GcodeResult::Failed:
                    log_warning(connect, "Gcode refused: %s", g_start);
                    return BackgroundResult::Failure;
                }
            }

            content->position += end_pos + 1;

            return BackgroundResult::More;
        } else if (auto wait = get_if<BackgroundGcodeWait>(&gcode); wait != nullptr) {
            if (buddy::cork::tracker.clear_cnt() != wait->start_tracker_clears) {
                log_debug(connect, "Queue got cleared while corking a gcode command");
                return BackgroundResult::Failure;
            }
            if (wait->submitted) {
                if (wait->cork.is_done_and_consumed()) {
                    log_debug(connect, "Cork done!");
                    return BackgroundResult::Success;
                } else {
                    log_debug(connect, "Cork still waiting");
                    return BackgroundResult::Later;
                }
            } else {
                const auto gcode = wait->cork.get_gcode();
                switch (printer.submit_gcode(gcode.data())) {
                case Printer::GcodeResult::Submitted:
                    log_debug(connect, "Wait for cork submitted: %s", gcode.data());
                    wait->submitted = true;
                    return BackgroundResult::More;

                case Printer::GcodeResult::Later:
                    // No place for the "cork" gcode. Give up now, try again later (do *not* mark as submitted).
                    log_debug(connect, "Cork doesn't fit yet");
                    return BackgroundResult::Later;

                case Printer::GcodeResult::Failed:
                    log_warning(connect, "Cork refused");
                    return BackgroundResult::Failure;
                }
            }
        }
        debug_assert(0);
        return BackgroundResult::Failure;
    }

} // namespace

BackgroundGcodeContent::BackgroundGcodeContent(SharedBorrow data, size_t size)
    : data(data)
    , size(size)
    , position(0)
    , start_tracker_clears(buddy::cork::tracker.clear_cnt()) {}

BackgroundResult background_cmd_step(BackgroundCmd &cmd, Printer &printer) {
    return visit([&](auto &cmd) { return step(cmd, printer); }, cmd);
}

} // namespace connect_client
