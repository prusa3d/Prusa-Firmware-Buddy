#include "upload_state.h"
#include "multipart_parser.h"

#include <file_list_defs.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cstring>
#include <memory>
#include <string_view>

using std::array;
using std::string_view;
using std::unique_ptr;

namespace {

constexpr const char *const CDISP = "content-disposition";
constexpr const char *const HEADER_SEP = "=\"";
constexpr const char *const NAME = "name";
constexpr const char *const FILENAME = "filename";
constexpr const char *const FILE = "file";
constexpr const char *const PRINT = "print";
// FIXME: Have a separate one for parallel uploads
constexpr const char *const DEFAULT_UPLOAD_FILENAME = "tmp.gcode";

const constexpr size_t MAX_SUBTOKEN = 19;

class Accumulator {
private:
    size_t len = 0;
    string_view looking_for;
    bool sensitive = true;
    array<char, MAX_SUBTOKEN> buffer;

public:
    Accumulator() {
        // Initialize to avoid UB due to reading/moving uninit values.
        memset(buffer.begin(), 0, buffer.size());
    }
    enum class Lookup {
        Found,
        NotYet,
        Overflow,
    };
    void start(const char *looking_for, bool sensitive) {
        const size_t len = strlen(looking_for);
        assert(len <= buffer.size());
        this->looking_for = string_view(looking_for, len);
        this->sensitive = sensitive;
        this->len = 0;
    }
    Lookup feed(char c) {
        memmove(buffer.begin(), buffer.begin() + 1, buffer.size() - 1);
        if (!sensitive) {
            c = tolower(c);
        }
        buffer[buffer.size() - 1] = c;
        const bool was_full = (len == buffer.size());
        if (!was_full) {
            len += 1;
        }

        const size_t suffix_len = std::min(len, looking_for.size());
        string_view suffix(buffer.end() - suffix_len, suffix_len);

        if (suffix == looking_for) {
            return Lookup::Found;
        } else if (was_full) {
            return Lookup::Overflow;
        } else {
            return Lookup::NotYet;
        }
    }
    bool empty() const {
        return len == 0;
    }
    string_view prefix() const {
        assert(len >= looking_for.size());
        return string_view(buffer.end() - len, len - looking_for.size());
    }
    array<char, MAX_SUBTOKEN> &borrow_buf() {
        return buffer;
    }
};

class UploadState {
private:
    enum class TokenType {
        NoToken,
        HeaderField,
        HeaderValue,
        Data,
    };

    enum class State {
        NoState,
        CheckHeaderIsCDisp,
        IgnoredHeader,
        CDispHeader,
        ReadString,
        ReadPartName,
        Data,
        Done,
    };

    enum class Part {
        Unknown,
        Print,
        File,
        // Possibly others in the future
    };

    // TODO: Compact them a bit
    TokenType type = TokenType::NoToken;
    State state = State::NoState;
    Part part = Part::Unknown;
    uint16_t error = 0;
    Accumulator accumulator;
    char *string_dst_pos = nullptr;
    const char *string_dst_end = nullptr;
    array<char, FILE_NAME_BUFFER_LEN> filename;
    bool have_valid_filename = false;
    bool init_done = false;
    bool start_print = false;

    HttpHandlers *handlers;

    unique_ptr<multipart_parser, void (*)(multipart_parser *)> multiparter;

    int header_field(const string_view &payload) {
        if (type != TokenType::HeaderField) { // Start this token
            type = TokenType::HeaderField;
            state = State::CheckHeaderIsCDisp;
            accumulator.start(CDISP, false);
        }

        for (const char c : payload) {
            if (state == State::CheckHeaderIsCDisp || state == State::CDispHeader) {
                if (!isspace(c)) {
                    switch (accumulator.feed(c)) {
                    case Accumulator::Lookup::Found:
                        state = State::CDispHeader;
                        break;
                    case Accumulator::Lookup::Overflow:
                        state = State::IgnoredHeader;
                        break;
                    case Accumulator::Lookup::NotYet:
                        break;
                    }
                }
            } else {
                break;
            }
        }

        return 0;
    }

    void handle_disp_field() {
        /*
         * The accumulator contains the name of the field before the separator.
         * Decide what to do with it.
         *
         * The accumulator can overflow (dropping previous characters in the
         * process). That's OK, because we abuse the fact its buffer is longer
         * than whatever we look for here and it would still not match.
         */
        const auto &prefix = accumulator.prefix();
        // Unless it's something special, simply throw it away.
        state = State::ReadString;
        string_dst_end = string_dst_pos = nullptr;

        if (prefix == NAME) {
            state = State::ReadPartName;
            // Let's reuse the accumulator buffer for the name of the part.
            // It's not being used right now.
            auto &buffer = accumulator.borrow_buf();
            string_dst_pos = buffer.begin();
            string_dst_end = buffer.end();
        } else if (prefix == FILENAME && !have_valid_filename) {
            /*
             * This one is a bit tricky. We want the filename of the file part.
             * But we are not guaranteed the name of the part comes first and
             * we don't know there won't be some other part with a file name in
             * it. So we need to accumulate filename even before knowing the
             * name, but only if we don't have one yet. And then when the part
             * ends, we check if it indeed is the right one.
             */
            string_dst_pos = filename.begin();
            // Leave space for \0 in this case - we need to talk with C there.
            string_dst_end = filename.end() - 1;
        }
    }

    int header_value(const string_view &payload) {
        if (type != TokenType::HeaderValue) { // Start this token
            type = TokenType::HeaderValue;
            accumulator.start(HEADER_SEP, true);
        }

        /*
         * In case we just came from the header_field, it may have unfinished
         * bussiness here. So far it didn't find what it was looking for.
         */
        if (state == State::CheckHeaderIsCDisp) {
            state = State::IgnoredHeader;
        }

        if (state == State::IgnoredHeader) {
            return 0;
        }

        for (const char c : payload) {
            switch (state) {
            case State::CDispHeader:
                if (c == ';') {
                    /*
                         * End of the field without = in it, something we don't
                         * care about (or leftover from the previous one). Just
                         * reset the buffer and start fresh.
                         */
                    accumulator.start(HEADER_SEP, true);
                } else if (!isspace(c)) {
                    if (accumulator.feed(c) == Accumulator::Lookup::Found) {
                        handle_disp_field();
                    }
                }
                break;
            case State::ReadString:
            case State::ReadPartName:
                /*
                 * FIXME: In reality, this is a bit more involved and
                 * complex. It can be escaped/otherwise encoded, since the
                 * string also can contain special characters...
                 *
                 * For now we just assume trivial case here.
                 */
                if (c == '"') {
                    if (state == State::ReadPartName) {
                        const char *start = accumulator.borrow_buf().begin();
                        const size_t length = string_dst_pos - start;
                        assert(length <= accumulator.borrow_buf().size());
                        const string_view name(start, length);
                        if (name == PRINT) {
                            part = Part::Print;
                        } else if (name == FILE) {
                            part = Part::File;
                        } else {
                            part = Part::Unknown;
                        }
                    }
                    state = State::CDispHeader;
                    string_dst_end = string_dst_pos = nullptr;
                } else if (string_dst_pos < string_dst_end) {
                    *(string_dst_pos++) = c;
                }
                /*
                 * else:
                 * If both are null, then we just throw the string away.
                 * Otherwise we have filled the string to the brim. What
                 * do we do with it now? Should we use the shorter one or
                 * error?
                 */
                break;
            case State::NoState:
            case State::CheckHeaderIsCDisp:
            case State::IgnoredHeader:
            case State::Data:
            case State::Done:
                assert(0); // Unreachable
                break;
            }
        }

        return 0;
    }

    int headers_complete() {
        type = TokenType::Data;
        if (part == Part::File) {
            if (filename[0] != '\0') {
                // Make sure this is not overwritten next time.
                have_valid_filename = true;
            } else {
                error = 400;
                return 1;
            }
        } else {
            /*
             * In case something else than the file part tries to give us a
             * file name, throw it away. We are not interested.
             *
             * But preserve the one from previous file part, of course.
             */
            if (!have_valid_filename) {
                memset(filename.begin(), 0, filename.size());
            }
        }
        state = State::Data;

        switch (part) {
        case Part::File:
            // Already prepared as part of the constructor.
            break;
        case Part::Print:
            // Prepare a storage for the true/false
            accumulator.start("true", true);
            break;
        case Part::Unknown:
            // Nothing to do with unknown parts. Simply skip.
            break;
        }

        return 0;
    }

    int file_part(const string_view &payload) {
        const auto err = handlers->gcode_data(handlers, payload.begin(), payload.size());
        if (err != 0) {
            error = err;
            init_done = false;
            return 1;
        }
        return 0;
    }

    int print_part(const string_view &payload) {
        for (const char c : payload) {
            if (accumulator.feed(c) == Accumulator::Lookup::Found) {
                start_print = true;
            }
        }

        return 0;
    }

    int part_data(const string_view &payload) {
        switch (part) {
        case Part::File:
            return file_part(payload);
        case Part::Print:
            return print_part(payload);
        case Part::Unknown:
            // Unknown parts are simply ignored.
            return 0;
        }
        assert(0);
        return 0;
    }

    int part_data_end() {
        part = Part::Unknown;
        return 0;
    }

    int body_end() {
        if (init_done) {
            const auto err = handlers->gcode_finish(handlers, DEFAULT_UPLOAD_FILENAME, filename.begin(), start_print);
            init_done = false;
            if (err != 0) {
                error = err;
                return 1;
            }
            state = State::Done;
        }
        return 0;
    }

#define DATA(NAME)                                                              \
    static int NAME##_raw(multipart_parser *p, const char *at, size_t length) { \
        auto me = static_cast<UploadState *>(multipart_parser_get_data(p));     \
        return me->NAME(string_view(at, length));                               \
    }
    DATA(header_field);
    DATA(header_value);
    DATA(part_data);
#undef DATA
#define NOTIF(NAME)                                                         \
    static int NAME##_raw(multipart_parser *p) {                            \
        auto me = static_cast<UploadState *>(multipart_parser_get_data(p)); \
        return me->NAME();                                                  \
    }
    NOTIF(headers_complete);
    NOTIF(part_data_end);
    NOTIF(body_end);
#undef NOTIF

    static const constexpr struct multipart_parser_settings parser_settings = {
        header_field_raw,
        header_value_raw,
        part_data_raw,
        nullptr, // part_data_begin
        headers_complete_raw,
        part_data_end_raw,
        body_end_raw,
    };

public:
    UploadState(const char *boundary, HttpHandlers *handlers)
        : handlers(handlers)
        , multiparter(multipart_parser_init(boundary, &parser_settings), multipart_parser_free) {
        memset(filename.begin(), 0, filename.size());
        if (multiparter) {
            multipart_parser_set_data(multiparter.get(), this);

            const auto err = handlers->gcode_start(handlers, DEFAULT_UPLOAD_FILENAME);
            if (err != 0) {
                error = err;
                return;
            }
            init_done = true;
        } else {
            error = 503;
        }
    }
    ~UploadState() {
        if (init_done) {
            // Something aborted, try to propagate and let the callbacks know they shall free resources.
            handlers->gcode_finish(handlers, DEFAULT_UPLOAD_FILENAME, NULL, false);
        }
    }
    // A good reason why we don't want these is because this is a
    // self-referential type (pointers leading to self) with raw pointers and
    // it's probably not easy to deal with (in case of the multipart parser
    // anyway).
    UploadState(const UploadState &other) = delete;
    UploadState(UploadState &&other) = delete;
    UploadState &operator=(const UploadState &other) = delete;
    UploadState &operator=(UploadState &&other) = delete;

    void feed(const string_view &data) {
        if (error != 0) {
            return;
        }

        if (state == State::Done) {
            return;
        }

        const size_t processed = multipart_parser_execute(multiparter.get(), data.begin(), data.size());

        if (processed < data.size() && state != State::Done && error == 0) {
            // Malformed -> Bad Request
            error = 400;
        }
    }

    uint16_t get_error() const {
        return error;
    }

    bool done() const {
        return state == State::Done;
    }
};

}

extern "C" {

struct Uploader {
    UploadState state;
    Uploader(const char *boundary, HttpHandlers *handlers)
        : state(boundary, handlers) {}
};

struct Uploader *uploader_init(const char *boundary, HttpHandlers *handlers) {
    return new Uploader(boundary, handlers);
}

void uploader_feed(struct Uploader *uploader, const char *data, size_t len) {
    uploader->state.feed(string_view(data, len));
}

bool uploader_finish(struct Uploader *uploader) {
    const bool done = uploader->state.done();
    delete uploader;
    return done;
}

uint16_t uploader_error(const struct Uploader *uploader) {
    return uploader->state.get_error();
}
}

/*
 * TODO: Missing parts:
 * * Make sure there may be multiple instances (create new file name for each?).
 */
