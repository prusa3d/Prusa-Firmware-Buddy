#include "segmented_json.h"
#include "../json_encode.h"

#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <cinttypes>

namespace nhttp {

JsonRenderer::ContentResult JsonRenderer::Output::output(size_t resume_point, const char *format, ...) {
    va_list params;
    va_start(params, format);
    const size_t written = vsnprintf(reinterpret_cast<char *>(buffer), buffer_size, format, params);
    va_end(params);

    if (written < buffer_size) {
        // Note: If the size is equal, the last byte of output was overwritten
        // by \0. We don't _need_ that \0 there, so we would be fine without
        // it, but there seems to be no way to tell vsnprintf to not put it there.
        //
        // Therefore, if the written is equal, then we _didn't_ fit.
        buffer += written;
        buffer_size -= written;
        written_something = true;
        return ContentResult::Complete;
    } else {
        this->resume_point = resume_point;
        return written_something ? ContentResult::Incomplete : ContentResult::BufferTooSmall;
    }
}

JsonRenderer::ContentResult JsonRenderer::Output::output_field_str(size_t resume_point, const char *name, const char *value) {
    JSONIFY_STR(value);
    return output(resume_point, "\"%s\":\"%s\"", name, value_escaped);
}

JsonRenderer::ContentResult JsonRenderer::Output::output_field_bool(size_t resume_point, const char *name, bool value) {
    return output(resume_point, "\"%s\":%s", name, jsonify_bool(value));
}

JsonRenderer::ContentResult JsonRenderer::Output::output_field_int(size_t resume_point, const char *name, int64_t value) {
    return output(resume_point, "\"%s\":%" PRIi64, name, value);
}

JsonRenderer::ContentResult JsonRenderer::Output::output_field_float_fixed(size_t resume_point, const char *name, double value, int precision) {
    return output(resume_point, "\"%s\":%.*f", name, precision, value);
}

JsonRenderer::ContentResult JsonRenderer::Output::output_field_obj(size_t resume_point, const char *name) {
    return output(resume_point, "\"%s\":{", name);
}

JsonRenderer::ContentResult JsonRenderer::Output::output_field_arr(size_t resume_point, const char *name) {
    return output(resume_point, "\"%s\":[", name);
}

JsonRenderer::ContentResult JsonRenderer::Output::output_iterator(size_t resume_point, Iterator &iterator) {
    JsonRenderer *sub_renderer;
    while ((sub_renderer = iterator.get())) {
        const auto [result, written] = sub_renderer->render(buffer, buffer_size);
        switch (result) {
        case ContentResult::Complete:
            // Completed this iterator item. Confirm the written data and
            // advance to the next item in the iterator, continuing until
            // we either fill in the buffer or run out of items.
            assert(written <= buffer_size);
            buffer += written;
            buffer_size -= written;
            iterator.advance();
            written_something = true;
            break;
        case ContentResult::Incomplete:
            // The whole current item didn't fit in. But maybe part of it
            // did. Confirm the part that did fit and tell the caller we
            // are not done yet and to call us again. We'll re-get the same
            // item from the iterator and continue that.
            assert(written <= buffer_size);
            buffer += written;
            buffer_size -= written;
            written_something = true;
            this->resume_point = resume_point;
            return ContentResult::Incomplete;
        case ContentResult::Abort:
            return ContentResult::Abort;
        case ContentResult::BufferTooSmall:
            if (written_something) {
                // The sub-renderer can't fit anything. But we have already
                // used part of the buffer, so it _might_ fit next time
                // with empty buffer.
                return ContentResult::Incomplete;
            } else {
                return ContentResult::BufferTooSmall;
            }
        }
    }

    return ContentResult::Complete;
}

std::tuple<JsonRenderer::ContentResult, size_t> JsonRenderer::render(uint8_t *buffer, size_t buffer_size) {
    size_t buffer_size_rest = buffer_size;
    Output output(buffer, buffer_size_rest, resume_point);
    const auto result = content(resume_point, output);
    assert(buffer_size_rest <= buffer_size);
    size_t written = (result == ContentResult::Abort) ? 0 : buffer_size - buffer_size_rest;
    return std::make_tuple(result, written);
}

}
