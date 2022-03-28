#pragma once

#define JSON_OUT(RPOINT, CALL, ...)                                                                                   \
    case RPOINT:                                                                                                      \
        if (::nhttp::JsonResult result = output.CALL(RPOINT, __VA_ARGS__); result != ::nhttp::JsonResult::Complete) { \
            return result;                                                                                            \
        }
#define JSON_CONTROL(STR) JSON_OUT(__COUNTER__, output, (STR))

#define JSON_CUSTOM(FORMAT, ...)                  JSON_OUT(__COUNTER__, output, (FORMAT), __VA_ARGS__)
#define JSON_FIELD_STR_FORMAT(NAME, FORMAT, ...)  JSON_OUT(__COUNTER__, output_field_str_format, (NAME), (FORMAT), __VA_ARGS__)
#define JSON_FIELD_STR(NAME, VALUE)               JSON_OUT(__COUNTER__, output_field_str, (NAME), (VALUE))
#define JSON_FIELD_BOOL(NAME, VALUE)              JSON_OUT(__COUNTER__, output_field_bool, (NAME), (VALUE))
#define JSON_FIELD_INT(NAME, VALUE)               JSON_OUT(__COUNTER__, output_field_int, (NAME), (VALUE))
#define JSON_FIELD_FFIXED(NAME, VALUE, PRECISION) JSON_OUT(__COUNTER__, output_field_float_fixed, (NAME), (VALUE), (PRECISION))
#define JSON_FIELD_OBJ(NAME)                      JSON_OUT(__COUNTER__, output_field_obj, (NAME))
#define JSON_FIELD_ARR(NAME)                      JSON_OUT(__COUNTER__, output_field_arr, (NAME))
#define JSON_OBJ_START                            JSON_CONTROL("{")
#define JSON_OBJ_END                              JSON_CONTROL("}")
#define JSON_ARR_END                              JSON_CONTROL("]")
#define JSON_COMMA                                JSON_CONTROL(",")

#define JSON_START          \
    (void)__COUNTER__;      \
    switch (resume_point) { \
    case 0:;
#define JSON_END \
    }            \
    return nhttp::JsonResult::Complete;
