#include "segmented_json.h"
#include "../json_encode.h"

#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <cinttypes>

namespace nhttp {

JsonResult JsonOutput::output(size_t resume_point, const char *format, ...) {
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
        return JsonResult::Complete;
    } else {
        this->resume_point = resume_point;
        return written_something ? JsonResult::Incomplete : JsonResult::BufferTooSmall;
    }
}

JsonResult JsonOutput::output_field_str(size_t resume_point, const char *name, const char *value) {
    JSONIFY_STR(value);
    return output(resume_point, "\"%s\":\"%s\"", name, value_escaped);
}

JsonResult JsonOutput::output_field_str_format(size_t resume_point, const char *name, const char *format, ...) {
    va_list params1, params2;
    va_start(params1, format);
    va_copy(params2, params1);
    // First, discover how much space we need for the formatted string.
    char first_buffer[1];
    // +1 for \0
    const size_t needed = vsnprintf(first_buffer, 1, format, params1) + 1;
    va_end(params1);

    // Now, get the buffer of the right size and format it.
    char buffer[needed];
    vsnprintf(buffer, needed, format, params2);
    va_end(params2);

    return output_field_str(resume_point, name, buffer);
}

JsonResult JsonOutput::output_field_bool(size_t resume_point, const char *name, bool value) {
    return output(resume_point, "\"%s\":%s", name, jsonify_bool(value));
}

JsonResult JsonOutput::output_field_int(size_t resume_point, const char *name, int64_t value) {
    return output(resume_point, "\"%s\":%" PRIi64, name, value);
}

JsonResult JsonOutput::output_field_float_fixed(size_t resume_point, const char *name, double value, int precision) {
    return output(resume_point, "\"%s\":%.*f", name, precision, value);
}

JsonResult JsonOutput::output_field_obj(size_t resume_point, const char *name) {
    return output(resume_point, "\"%s\":{", name);
}

JsonResult JsonOutput::output_field_arr(size_t resume_point, const char *name) {
    return output(resume_point, "\"%s\":[", name);
}

std::tuple<JsonResult, size_t> LowLevelJsonRenderer::render(uint8_t *buffer, size_t buffer_size) {
    size_t buffer_size_rest = buffer_size;
    JsonOutput output(buffer, buffer_size_rest, resume_point);
    const auto result = content(resume_point, output);
    assert(buffer_size_rest <= buffer_size);
    size_t written = (result == JsonResult::Abort) ? 0 : buffer_size - buffer_size_rest;
    return std::make_tuple(result, written);
}

}
