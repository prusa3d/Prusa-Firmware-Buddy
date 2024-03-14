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

using http::Status;
using std::array;
using std::make_tuple;
using std::string_view;
using std::unique_ptr;

namespace {

constexpr const char *const CDISP = "content-disposition";
constexpr const char *const HEADER_SEP = "=\"";
constexpr const char *const NAME = "name";
constexpr const char *const FILENAME = "filename";
constexpr const char *const FILE_TOKEN = "file";
constexpr const char *const PRINT_TOKEN = "print";

} // namespace

namespace nhttp::printer {

class UploadState::Adaptor {
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

public:
    static const constexpr struct multipart_parser_settings parser_settings = {
        header_field_raw,
        header_value_raw,
        part_data_raw,
        nullptr, // part_data_begin
        headers_complete_raw,
        part_data_end_raw,
        body_end_raw,
    };
};

UploadState::Accumulator::Accumulator()
    : len(0)
    , sensitive(true) {
    // Initialize to avoid UB due to reading/moving uninit values.
    memset(buffer.begin(), 0, buffer.size());
}

void UploadState::Accumulator::start(const char *looking_for, bool sensitive) {
    const size_t len = strlen(looking_for);
    assert(len <= buffer.size());
    this->looking_for = string_view(looking_for, len);
    this->sensitive = sensitive;
    this->len = 0;
}

UploadState::Accumulator::Lookup UploadState::Accumulator::feed(char c) {
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

std::string_view UploadState::Accumulator::prefix() const {
    assert(len >= looking_for.size());
    return string_view(buffer.end() - len, len - looking_for.size());
}

void UploadState::MultiparserDeleter::operator()(multipart_parser *parser) {
    multipart_parser_free(parser);
}

int UploadState::header_field(const string_view &payload) {
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

void UploadState::handle_disp_field() {
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

int UploadState::header_value(const string_view &payload) {
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
                    if (name == PRINT_TOKEN) {
                        part = Part::Print;
                    } else if (name == FILE_TOKEN) {
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

int UploadState::headers_complete() {
    type = TokenType::DataToken;
    if (part == Part::File) {
        if (filename[0] != '\0') {
            // Make sure this is not overwritten next time.
            have_valid_filename = true;
            error = hooks->check_filename(filename.begin());
            if (!err_ok()) {
                return 1;
            }
        } else {
            error = make_tuple(Status::BadRequest, "Missing/empty file name");
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

int UploadState::file_part(const string_view &payload) {
    error = hooks->data(payload);
    if (!err_ok()) {
        init_done = false;
        return 1;
    }
    return 0;
}

int UploadState::print_part(const string_view &payload) {
    for (const char c : payload) {
        if (accumulator.feed(c) == Accumulator::Lookup::Found) {
            start_print = true;
        }
    }

    return 0;
}

int UploadState::part_data(const string_view &payload) {
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

int UploadState::part_data_end() {
    part = Part::Unknown;
    return 0;
}

int UploadState::body_end() {
    if (init_done) {
        error = hooks->finish(filename.begin(), start_print);
        init_done = false;
        if (!err_ok()) {
            return 1;
        }
        state = State::Done;
    }
    return 0;
}

bool UploadState::err_ok() const {
    return std::get<0>(error) == Status::Ok;
}

UploadState::UploadState(const char *boundary)
    : multiparter(multipart_parser_init(boundary, &Adaptor::parser_settings))
    , type(TokenType::NoToken)
    , state(State::NoState)
    , part(Part::Unknown)
    , have_valid_filename(false)
    , init_done(false)
    , start_print(false) {
    memset(filename.begin(), 0, filename.size());
    if (multiparter) {
        init_done = true;
    } else {
        error = make_tuple(Status::ServiceTemporarilyUnavailable, "Not enough memory");
    }
}

void UploadState::feed(const string_view &data) {
    if (!err_ok()) {
        return;
    }

    if (state == State::Done) {
        return;
    }

    // Set every time, in case we got moved.
    multipart_parser_set_data(multiparter.get(), this);
    const size_t processed = multipart_parser_execute(multiparter.get(), data.begin(), data.size());

    if (processed < data.size() && state != State::Done && err_ok()) {
        // Malformed -> Bad Request
        error = make_tuple(Status::BadRequest, "Malformed multipart form");
    }
}

void UploadState::setup(UploadHooks *hooks) {
    this->hooks = hooks;
}

} // namespace nhttp::printer
