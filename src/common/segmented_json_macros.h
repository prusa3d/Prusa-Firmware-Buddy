#pragma once

#define JSON_OUT(RPOINT, GUARD, CALL, ...)                                                                          \
    case RPOINT:                                                                                                    \
        if (!(GUARD)) {                                                                                             \
            return ::json::JsonResult::Abort;                                                                       \
        }                                                                                                           \
        if (::json::JsonResult result = output.CALL(RPOINT, __VA_ARGS__); result != ::json::JsonResult::Complete) { \
            return result;                                                                                          \
        }
#define JSON_CONTROL(STR) JSON_OUT(__COUNTER__, true, output, (STR))

#define JSON_CUSTOM(FORMAT, ...)                          JSON_OUT(__COUNTER__, true, output, (FORMAT), __VA_ARGS__)
#define JSON_FIELD_STR_FORMAT(NAME, FORMAT, ...)          JSON_OUT(__COUNTER__, true, output_field_str_format, (NAME), (FORMAT), __VA_ARGS__)
#define JSON_FIELD_STR_FORMAT_G(GUARD, NAME, FORMAT, ...) JSON_OUT(__COUNTER__, (GUARD), output_field_str_format, (NAME), (FORMAT), __VA_ARGS__)
#define JSON_FIELD_STR_G(GUARD, NAME, VALUE)              JSON_OUT(__COUNTER__, (GUARD), output_field_str, (NAME), (VALUE))
#define JSON_FIELD_STR(NAME, VALUE)                       JSON_FIELD_STR_G(true, (NAME), (VALUE))
// For SFNs
// Note: no json forbidden characters allowed in value - SFNs can't contain them anyway.
#define JSON_FIELD_STR_437_G(GUARD, NAME, VALUE)           JSON_OUT(__COUNTER__, (GUARD), output_field_str_437, (NAME), (VALUE))
#define JSON_FIELD_STR_437(NAME, VALUE)                    JSON_FIELD_STR_437_G(true, (NAME), (VALUE))
#define JSON_FIELD_BOOL(NAME, VALUE)                       JSON_OUT(__COUNTER__, true, output_field_bool, (NAME), (VALUE))
#define JSON_FIELD_BOOL_G(GUARD, NAME, VALUE)              JSON_OUT(__COUNTER__, (GUARD), output_field_bool, (NAME), (VALUE))
#define JSON_FIELD_INT_G(GUARD, NAME, VALUE)               JSON_OUT(__COUNTER__, (GUARD), output_field_int, (NAME), (VALUE))
#define JSON_FIELD_INT(NAME, VALUE)                        JSON_FIELD_INT_G(true, (NAME), (VALUE))
#define JSON_FIELD_FFIXED_G(GUARD, NAME, VALUE, PRECISION) JSON_OUT(__COUNTER__, (GUARD), output_field_float_fixed, (NAME), (VALUE), (PRECISION))
#define JSON_FIELD_FFIXED(NAME, VALUE, PRECISION)          JSON_FIELD_FFIXED_G(true, (NAME), (VALUE), (PRECISION))
#define JSON_FIELD_OBJ(NAME)                               JSON_OUT(__COUNTER__, true, output_field_obj, (NAME))
#define JSON_FIELD_ARR(NAME)                               JSON_OUT(__COUNTER__, true, output_field_arr, (NAME))
#define JSON_CHUNK(VALUE)                                  JSON_OUT(__COUNTER__, true, output_chunk, (VALUE))
#define JSON_FIELD_CHUNK(NAME, VALUE) \
    JSON_CUSTOM("\"%s\":", (NAME))    \
    JSON_CHUNK((VALUE))
#define JSON_OBJ_START JSON_CONTROL("{")
#define JSON_OBJ_END   JSON_CONTROL("}")
#define JSON_ARR_END   JSON_CONTROL("]")
#define JSON_COMMA     JSON_CONTROL(",")

#define JSON_START                                                \
    _Pragma("GCC diagnostic push");                               \
    _Pragma("GCC diagnostic ignored \"-Wimplicit-fallthrough\""); \
    (void)__COUNTER__;                                            \
    switch (resume_point) {                                       \
    case 0:;
#define JSON_END                         \
    }                                    \
    return ::json::JsonResult::Complete; \
    _Pragma("GCC diagnostic pop");
