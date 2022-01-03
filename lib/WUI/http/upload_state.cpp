/*
 * Little high-level overview of what happens in here.
 *
 * # Quirks of the interface
 *
 * We expose a C-style API for the functionality here (see the header), but
 * internally use C++ to have access to more functionality. Therefore, we have
 * certain amount of boilerplate here to translate between the idioms. We might
 * switch the interface to C++ eventually, once the consumers migrate too.
 *
 * Furthermore, we keep the classes here in anonymous namespace. This is in
 * hope the compiler will use the information to prove the data and types never
 * escape the current compilation unit and allow it to do much more aggressive
 * optimizations - specifically, reordering or eliminating member variables in
 * the classes, or making them smaller.
 *
 * # Motivation
 *
 * We use the "multipart_parser.h" thing (that one is also in C, so more
 * boilerplate). That thing mostly handles the multipart/ * content types, with
 * some missing flexibility and calls callbacks on "tokens" in there, without
 * keeping anything around. It can be fed by multiple chunks (as they come
 * split into packets). As a result, it can also report a single token by
 * multiple calls.
 *
 * This file is responsible for tracking the tokens, parsing them further (eg.
 * finding and parsing the Content-Disposition header) and making sense of them
 * to implement the actual storing of the files.
 *
 * # Design
 *
 * Due to the splitting at arbitrary place, this just splits it further to
 * individual chars and handles them as an automaton with states.
 *
 * If any kind of longer string needs to be compared, it accumulates it in a
 * (short, we know max lengths of the interesting ones) buffer.
 *
 * The only exception to the dribbling by characters is the "bulk" of the data
 * payload, which is forwarded in whole chunks for performance reasons.
 */
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
constexpr const char *const DEFAULT_UPLOAD_FILENAME = "upload.tmp";

const constexpr size_t MAX_SUBTOKEN = 19;

/*
 * A helper class that acts like a sliding window buffer. It is used to find
 * the right parts in stream of incoming characters.
 *
 * This is done by moving the whole buffer by a single character each time. It
 * is not exactly _optimal_ solution, but:
 * * This happens very seldom.
 * * The buffer is rather short.
 *
 * Therefore, we prefer simplicity over CPU performance. The alternative would
 * be something like a Aho-Corasick automaton built at compile time and
 * tracking only the ID of the state (which would make it faster and smaller in
 * RAM), but it is also a complex thing.
 */
class Accumulator {
private:
    string_view looking_for;
    uint8_t len : 7;
    bool sensitive : 1;
    array<char, MAX_SUBTOKEN> buffer;

public:
    Accumulator()
        : len(0)
        , sensitive(true) {
        // Initialize to avoid UB due to reading/moving uninit values.
        memset(buffer.begin(), 0, buffer.size());
    }
    enum class Lookup {
        Found,
        NotYet,
        Overflow,
    };
    /*
     * Starts looking for specific substring.
     *
     * * Reset the buffer to be "empty".
     * * Start looking for the given string.
     * * Sensitive means if it should be case sensitive or not.
     */
    void start(const char *looking_for, bool sensitive) {
        const size_t len = strlen(looking_for);
        assert(len <= buffer.size());
        this->looking_for = string_view(looking_for, len);
        this->sensitive = sensitive;
        this->len = 0;
    }
    /*
     * Puts another character into the buffer.
     *
     * This potentially pushes the oldest one out in case the buffer is full.
     *
     * Returns if the substring has been found at this position or not. It may
     * return Overflow, which means the string already fed in here doesn't
     * match from the beginning. Depending on if you're looking for an
     * exact/full match or substring anywhere, you may take this as a "No
     * match" or "Keep looking".
     */
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

        const size_t suffix_len = std::min(static_cast<size_t>(len), looking_for.size());
        string_view suffix(buffer.end() - suffix_len, suffix_len);

        if (suffix == looking_for) {
            return Lookup::Found;
        } else if (was_full) {
            return Lookup::Overflow;
        } else {
            return Lookup::NotYet;
        }
    }
    // Is the buffer empty?
    bool empty() const {
        return len == 0;
    }
    /*
     * Returns the part of buffer before the looking-for string (as much of it as fits).
     *
     * If the looking-for string is considered a separator, the thing before it
     * is left in the buffer and can be accessed with this.
     *
     * The returned prefix can be shorter than whatever was fed into it before
     * the looking-for separator (in case oldest chars no longer fit). This is
     * OK in our use, as the buffer - separator is longer (due to other
     * reasons) than the constant strings we look for before the separator, so
     * in case something fell off the left side of the buffer, it would no
     * longer match anyway.
     */
    string_view prefix() const {
        assert(len >= looking_for.size());
        return string_view(buffer.end() - len, len - looking_for.size());
    }
    /*
     * Access the internal buffer.
     *
     * This can be used to recycle the buffer for different purposes (to save
     * memory) while not being in used. The buffer or the functionality of
     * looking for something are mutually exclusive (use one or the other, not
     * both at the same time).
     */
    array<char, MAX_SUBTOKEN> &borrow_buf() {
        return buffer;
    }
};

class UploadState {
private:
    /*
     * This helps tracking if we start a new token or if this is a continuation
     * of the same one.
     */
    // Note: Use of 'enum class' generates a warning at the place of the
    // bitfield usage below, even though it is big enough.
    enum TokenType {
        NoToken,
        HeaderField,
        HeaderValue,
        DataToken,
        // Warning: Check the bitfield sizes if adding values.
    };

    /*
     * The states of the automaton.
     */
    enum State {
        NoState,
        CheckHeaderIsCDisp,
        IgnoredHeader,
        CDispHeader,
        ReadString,
        ReadPartName,
        Data,
        Done,
        // Warning: Check the bitfield sizes if adding values.
    };

    /*
     * Whenever we found a part of a known name, we record it by this.
     *
     * Anything unknown or before finding the name, it is Unknown and just
     * ignored.
     */
    enum Part {
        Unknown,
        Print,
        File,
        // Possibly others in the future
        // Warning: Check the bitfield sizes if adding values.
    };

    /**
     * The handlers.
     *
     * Not owned by us.
     */
    HttpHandlers *handlers;

    unique_ptr<multipart_parser, void (*)(multipart_parser *)> multiparter;

    /*
     * When reading a string separated by quotes, this points to the current
     * poisition and an end of a buffer to store it. This makes it possible to
     * point the reading of characters to different buffers depending on what
     * the string means.
     *
     * Relevant for the ReadString and ReadPartName states.
     *
     * When both are pointing to the same location, no more characters are read
     * and they are simply thrown away. The special-case of both pointing to
     * NULL is a /dev/null buffer for skipping until the end of a string that's
     * of no interest to us.
     */
    char *string_dst_pos = nullptr;
    const char *string_dst_end = nullptr;

    /*
     * A buffer for accumulating strings. See above.
     *
     * Warning/TODO: From time to time, its internal buffer is abused for other
     * purposes. This is a hack to save some memory and we are careful not to
     * use both at the same time (and to call a new .start after being molested
     * in this way). We hope the tests would catch anything if we mess up. If
     * you have a more elegant solution that isn't heavy-weight, proposals are
     * welcome.
     *
     * So far we tried to use std::variant and that turned a bit heavy. Using a
     * RAII object to „lock“ would mean having to store it somewhere, which is
     * also heavy-weight.
     */
    Accumulator accumulator;

    TokenType type : 2;
    State state : 3;
    Part part : 2;
    bool have_valid_filename : 1;
    /*
     * The gcode_start hook has been called and we need to eventually call the finish hook too.
     *
     * This may get reset back to false in case the finish hook is called or
     * when there's an error from hook (the hooks should know they erorred out
     * and clean up their state as needed).
     */
    bool init_done : 1;

    // The data ask us to start the print.
    bool start_print : 1;

    // The values are >512, but <1024.
    uint16_t error : 10;

    array<char, FILE_NAME_BUFFER_LEN> filename;
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

    /*
     * The prefix of the accumulator contains the name of a field in the
     * content-disposition header. Decide what to do with it.
     */
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
        type = TokenType::DataToken;
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

    // This part is just boilerplate/adaptor code for callbacks from the multipart parser.
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
        , multiparter(multipart_parser_init(boundary, &parser_settings), multipart_parser_free)
        , type(TokenType::NoToken)
        , state(State::NoState)
        , part(Part::Unknown)
        , have_valid_filename(false)
        , init_done(false)
        , start_print(false)
        , error(0) {
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
