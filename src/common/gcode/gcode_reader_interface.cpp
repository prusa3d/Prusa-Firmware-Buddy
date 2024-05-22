#include "gcode_reader_interface.hpp"

#include <type_traits>
#include <errno.h>
#include <cassert>
#include "core/core.hpp"
#include "transfers/transfer.hpp"
#include "lang/i18n.h"

IGcodeReader::Result_t IGcodeReader::stream_get_line(GcodeBuffer &b) {
    b.line.begin = begin(b.buffer);
    b.line.end = begin(b.buffer);

    if (line_continuations == IGcodeReader::Continuations::Discard) {
        // Even incomplete lines "eat up" the data in the Discard mode, so at
        // this point they _act_ as complete (for the purposes of EOF and similar).
        b.line_complete = true;
    }

    while (b.line.end < end(b.buffer)) {
        char c;
        Result_t result = stream_getc(c);

        switch (result) {
        case Result_t::RESULT_EOF:
            c = '\n';
            break;
        case Result_t::RESULT_ERROR:
        case Result_t::RESULT_OUT_OF_RANGE:
        case Result_t::RESULT_TIMEOUT:
            return result;
        case Result_t::RESULT_OK:
            break;
        }

        if (c == '\r' || c == '\n') {
            // null terminate => safe atof+atol
            // (we do not advance the end, it's not part of the string)
            *b.line.end = '\0';
            if (b.line.is_empty() && b.line_complete) {
                if (result == Result_t::RESULT_EOF) {
                    return Result_t::RESULT_EOF;
                } else {
                    // Skip blank lines
                    continue;
                }
            } else {
                // We either have some actual data in the line or we have a
                // last empty chunk of line that continues from previous calls.
                b.line_complete = true;
                return Result_t::RESULT_OK;
            }
        }

        *b.line.end++ = c;
    }

    // At this point, the buffer is full.
    b.line_complete = false;

    switch (line_continuations) {
    case IGcodeReader::Continuations::Discard:
        // In this mode, we need to really have the final \0, so kill the final char instead.
        *--b.line.end = '\0';

        for (;;) {
            char c;
            Result_t result = stream_getc(c);

            switch (result) {
            case Result_t::RESULT_EOF:
                // The below ones are errors. Nevertheless, we run into the
                // error at the part of the line we are discarding, so the
                // part we have is fine.
            case Result_t::RESULT_ERROR:
            case Result_t::RESULT_OUT_OF_RANGE:
            case Result_t::RESULT_TIMEOUT:
                return Result_t::RESULT_OK;
            case Result_t::RESULT_OK:
                if (c == '\r' || c == '\n') {
                    return Result_t::RESULT_OK;
                }
                break;
            }
        }
    case IGcodeReader::Continuations::Split:
        return Result_t::RESULT_OK;
    }

    // Unreachable, but C++ and its non-exhaustive enums :-|
    assert(0);
    return Result_t::RESULT_ERROR;
}

bool IGcodeReader::range_valid(size_t start, size_t end) const {
    assert(start <= end);
    if (start == end) {
        // 0-sized range.
        return true;
    }

    if (!validity.has_value()) {
        // Whole file valid
        return true;
    }

    // Cap them into the range of the file to the range of the total file
    end = std::min(validity->total_size, end);

    auto inside = [&](const auto &part) -> bool {
        if (part.has_value()) {
            return start >= part->start && end <= part->end;
        } else {
            // The part doesn't exist at all, it can't be inside it
            return false;
        }
    };

    return inside(validity->valid_head) || inside(validity->valid_tail);
}

void IGcodeReader::update_validity(transfers::Transfer::Path &filename) {
#if !defined(UNITTESTS) // validity update is disabled for unit tests, because it drags in lots of dependencies
    using transfers::PartialFile;
    using transfers::Transfer;

    const auto transfer_state = Transfer::load_state(filename.as_destination());
    const auto new_validity = std::visit(
        [this](const auto &arg) -> std::optional<PartialFile::State> {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, PartialFile::State>) {
                return arg;

            } else if constexpr (std::is_same_v<T, Transfer::Error>) {
                set_error(arg.msg ?: N_("File read error"));
                // State saying "nothing available"
                return PartialFile::State();

            } else if constexpr (std::is_same_v<T, Transfer::Complete>) {
                // Whole file available -> no restrictions on what ranges of files it can access.
                return std::nullopt;
            }
        },
        transfer_state);

    set_validity(new_validity);
#endif
}

bool IGcodeReader::check_file_starts_with_BGCODE_magic() const {
    auto file = this->file.get();

    // Todo respect file availability?
    rewind(file);

    static constexpr int magicSize = bgcode::core::MAGIC.size();
    char check_buffer[magicSize];
    if (!fread(check_buffer, magicSize, 1, file)) {
        return false;
    }

    if (memcmp(check_buffer, bgcode::core::MAGIC.data(), magicSize)) {
        return false;
    }

    return true;
}
