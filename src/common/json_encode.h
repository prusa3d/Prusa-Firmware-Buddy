#pragma once

/**
 * \file
 *
 * \brief Encoding JSON.
 *
 * The networking needs to handle JSON data, in both directions.
 *
 * For decoding, we have jsmn (not sure if it's a good option ‒ it needs the
 * whole input at once, can't parse in streaming fashion. Furthermore, it
 * doesn't de-escape strings).
 *
 * This file provides very minimal set of utils for encoding JSON. Most things
 * can be just encoded with printf or similar (numbers, names of fields, the
 * general structure) ‒ while that's not exactly convenient and a bit of error
 * prone (nothing checks for syntax errors), it's doable.
 *
 * But we need booleans and we need strings (as these might need some escaping).
 *
 * Note that this also supports somewhat minimal way to do chunked JSON
 * rendering ‒ with printf-rendering, one can just try adding more things until
 * it no longer fits and leave the rest for later.
 */

#include <cstdbool>
#include <cstdlib>
#include <string_view>

/**
 * \brief de-escapes json string in place
 *
 * De-escaping never makes the string longer, so doing it in place
 * is ok. If used on part of the string, it will shift only size chars
 * and the rest will be unchanged.
 * e.g. unescape_json_i("string\\\"bla\\t1234", 17) -> "string\"bla\t123434"
 *
 * \param in the input string to de-escape
 * \param size number of chars to consoder for de-escaping
 *
 * \return the new size after de-escaping
 */

size_t unescape_json_i(char *in, size_t size);

/**
 * \brief Returns a string representing a boolean value
 *
 * This returns a constant string representation of the boolean value, suitable
 * for embedding into a JSON object. Eg:
 *
 * printf("{\"mybool\": %s}", jsonify_bool(true));
 */
const char *jsonify_bool(bool value);

/**
 * \brief Estimates how large buffer needs to be used for jsonify_str.
 *
 * Examines the input string and returns how large the output will be in case
 * some escaping needs to happen.
 *
 * \param input The input string
 * \return
 *   - 0 in case no escaping needs to happen (and the string may be used unmodified).
 *   - Number of bytes in the output buffer of jsonify_str, including space for the final \0.
 */
size_t jsonify_str_buffer(std::string_view input);

/**
 * \brief Escapes a string for JSON.
 *
 * Copies the input into output, escaping any characters as needed.
 *
 * The output buffer needs to be at least jsonify_str_buffer(input) large. In
 * case that one returns 0, this function must not be called. Exactly that many
 * bytes are written.
 *
 * The output is terminated by null-byte (which is already accounted for in the
 * size of the buffer).
 */
void jsonify_str(std::string_view, char *output);

/**
 * \brief Maximum on-stack buffer used by JSONIFY_STR.
 *
 * jsonify_str_buffer() returns up to `6 * input_len + 1` for worst-case
 * input (every byte becomes \uXXXX). With JSONIFY_STR's VLA growing
 * linearly with that estimate, a sufficiently long input — e.g. a
 * 255-byte file path passed verbatim, or a 200-byte gcode line — can
 * overflow a small task stack. The tcpip thread (TCPIP_THREAD_STACKSIZE
 * = 1248 B) is the most exposed: JSONIFY_STR is called from several
 * status renderer paths that run on it.
 *
 * Cap the VLA at this many bytes. If a caller's input would need more,
 * the escaped result is the empty string "" and a warning is logged
 * via DEBUG_ERROR_MSG (when EEPROM_CHITCHAT-style debug is enabled).
 * This is a strict safety improvement: previous behaviour for inputs
 * that fit is unchanged; inputs that previously could crash the stack
 * now degrade to an empty JSON string field instead.
 */
#define JSONIFY_STR_MAX_BUFFER 384

/**
 * \brief Macro to put the jsonification of strings together conveniently.
 *
 * Usually, one wants to first check how large a buffer for the rendered JSON
 * shall be, then create the buffer on stack and render into that. This is not
 * possible to wrap into a function (because the buffer needs to be on the
 * caller's stack), therefore it is a macro.
 *
 * Note that the macro is not "hygienic" and it expands into multiple
 * statements (wrapping it into do {} while (0) would also destroy the buffer)
 * and creating several local variables. Be conservative in its use.
 *
 * The buffer size is bounded by `JSONIFY_STR_MAX_BUFFER` for stack safety;
 * inputs whose escaped form would exceed that limit yield an empty
 * `name_escaped` (rather than corrupting the stack).
 *
 * * Have a `const char *` or `std::string_view` input variable with a certain name.
 * * Call the macro with that variable name.
 * * A variable `std::string_view name_escaped` is created. Eg:
 *
 * ```
 * const char *whatever = "hello\nworld";
 * JSONIFY_STR(whatever);
 * printf("%.*s", (int)whatever_escaped.size(), whatever_escaped.data());
 * ```
 */
#define JSONIFY_STR(NAME)                                                                                                            \
    _Pragma("GCC diagnostic push");                                                                                                  \
    _Pragma("GCC diagnostic ignored \"-Wvla\"");                                                                                     \
    const std::string_view NAME##_view { NAME }; /*Prevent double-counting the length in case const char* is provided*/              \
    const size_t NAME##_len = jsonify_str_buffer(NAME##_view);                                                                       \
    /* VLA size is bounded — if NAME##_len is too big the buffer is a tiny placeholder. */                                           \
    char NAME##_buffer[NAME##_len < JSONIFY_STR_MAX_BUFFER ? (NAME##_len ? NAME##_len : 1) : 1];                                     \
    const std::string_view NAME##_escaped =                                                                                          \
        (NAME##_len == 0)                        ? NAME##_view                                                                       \
        : (NAME##_len < JSONIFY_STR_MAX_BUFFER)  ? (jsonify_str(NAME##_view, NAME##_buffer), std::string_view { NAME##_buffer })     \
                                                 : std::string_view {};                                                              \
    _Pragma("GCC diagnostic pop");
