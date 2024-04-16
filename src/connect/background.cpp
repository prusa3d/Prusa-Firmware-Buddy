#include "background.hpp"
#include "printer.hpp"

using std::visit;

namespace connect_client {

namespace {

    BackgroundResult step(BackgroundGcode &gcode, Printer &printer) {
        if (gcode.size <= gcode.position) {
            // FIXME: We need a way to know if the commands are not just submitted,
            // but also processed. Some kind of additional "cork" command that'll
            // report back to us and we can check it, maybe?

            // All gcode submitted.
            return BackgroundResult::Success;
        }

        // In C++, it's a lot of work to convert void * -> char * or uint8_t * ->
        // char *, although it's both legal conversion (at least in this case). In
        // C, that works out of the box without casts.
        const char *start = reinterpret_cast<const char *>(gcode.data->data()) + gcode.position;
        const size_t tail_size = gcode.size - gcode.position;

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
                break;
            case Printer::GcodeResult::Later:
                // In case this gcode doesn't fit, retry again without moving
                // the position - we'll reparse it next time.
                return BackgroundResult::More;
            case Printer::GcodeResult::Failed:
                return BackgroundResult::Failure;
            }
        }

        gcode.position += end_pos + 1;

        return BackgroundResult::More;
    }

} // namespace

BackgroundResult background_cmd_step(BackgroundCmd &cmd, Printer &printer) {
    return visit([&](auto &cmd) { return step(cmd, printer); }, cmd);
}

} // namespace connect_client
