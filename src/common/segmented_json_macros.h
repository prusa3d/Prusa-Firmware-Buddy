#pragma once

// Json streaming rendering
//
// These macros help with rendering JSON in "streaming" fashion. The idea is
// that we have some rendering buffer which is possibly smaller than the whole
// message we want to send. We fill it, send the first part, then continue
// rendering the second part, etc.
//
// The underlying machinery is in segmented_json.h, which contains the buffers,
// etc. The macros help to make the code look more "natural", resembling the
// JSON that is being rendered (see how it is used in other places for
// inspiration).
//
// ============== THIS THING BITES !!! ==============
//
// This is not any kind of "true" coroutine. It is just a function that gets
// called each time for each chunk to be rendered. When it suspends because the
// buffer is full, it stores the "position" in the JSON and when it resumes, it
// skips to that position.
//
// Under the hood, it is implemented by a big `switch` statement with cases
// being the resume points and fallthrough (under ordinary circumstances, it
// just keeps going through the cases).
//
// This has some consequences, specifically:
//
// * Using `switch` inside the "JSON" code is often not possible and needs to
//   be replaced by a sequence of if-else statements.
// * The "intro" part before the JSON macros is executed _each time_ the
//   function is called to render a chunk. As a consequence, if it computes
//   something based on "external" state, it might have a different value the
//   next time. If that's not OK, the value needs to be stored in the renderer
//   state (eg. the acompanied object). This is clearly visible in eg. loop
//   variables (for (i = 0; i < cnt; i ++) ‒ the i needs to survive between the
//   calls).
// * As a very special case, a value of something "external" might change
//   between a checking of conditional and executing the body. Eg. with this
//   code:
//
//   if (something.has_value()) {
//     JSON_FIELD_INT("something", something.value()) JSON_COMMA;
//   }
//
//   it _might_ be subtly incorrect. It can happen that (assuming something is external):
//   1. something has a value, so it proceeds into the body.
//   2. The JSON_FIELD_INT macro figures the output doesn't fit and suspends the rendering.
//   3. It eventually resumes with an empty buffer inside the JSON_FIELD_INT.
//      But if the value could have changed in between, it might be without a
//      value now, which is a big problem (UB).
//
//   While such situation is likely rare (it needs to hit the "full buffer" at
//   the exact place and change the something to empty in between), it needs to
//   be handled in _some_ way. There are two commonly used possibilities:
//
//   if (something.has_value()) {
//     JSON_FIELD_INT("something", something.value_or(42)) JSON_COMMA;
//   }
//
//   In this case, we still keep the conditional to handle the _usual_ case (it
//   didn't have the value to start with, we want nice JSON output that just
//   doesn't have the field). But in the rare case, we use some "bogus" value
//   and hope it won't cause too much trouble or that'll send a new version
//   soon enough.
//
//   The other alternative is:
//
//   if (something.has_value()) {
//     JSON_FIELD_INT_G(something.has_value(), "something", something.value()) JSON_COMMA;
//   }
//
//   We still keep the conditional to handle the case it is empty to start
//   with. But the `_G` macro has _additional_ check that happens at the time
//   of rendering attempt (at each one, so it rechecks the second time). In
//   case the conditional here fails, the rendering of the whole JSON message
//   is aborted with an error (instead of crashing the printer). This usually
//   leads to a new attempt to send the same message at a later time on some
//   higher level, but is in a sense ugly and costly (the other side has
//   already seen the start and needs to discard it).

#define JSON_OUT(RPOINT, GUARD, CALL, ...)                                                                          \
    case RPOINT:                                                                                                    \
        if (!(GUARD)) {                                                                                             \
            return ::json::JsonResult::Abort;                                                                       \
        }                                                                                                           \
        if (::json::JsonResult result = output.CALL(RPOINT, __VA_ARGS__); result != ::json::JsonResult::Complete) { \
            return result;                                                                                          \
        }
#define JSON_CONTROL(STR) JSON_OUT(__COUNTER__, true, output, (STR))

#define JSON_CUSTOM(FORMAT, ...)                           JSON_OUT(__COUNTER__, true, output, (FORMAT), __VA_ARGS__)
#define JSON_FIELD_STR_FORMAT(NAME, FORMAT, ...)           JSON_OUT(__COUNTER__, true, output_field_str_format, (NAME), (FORMAT), __VA_ARGS__)
#define JSON_FIELD_STR_FORMAT_G(GUARD, NAME, FORMAT, ...)  JSON_OUT(__COUNTER__, (GUARD), output_field_str_format, (NAME), (FORMAT), __VA_ARGS__)
#define JSON_FIELD_STR_G(GUARD, NAME, VALUE)               JSON_OUT(__COUNTER__, (GUARD), output_field_str, (NAME), (VALUE))
#define JSON_FIELD_STR(NAME, VALUE)                        JSON_FIELD_STR_G(true, (NAME), (VALUE))
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
