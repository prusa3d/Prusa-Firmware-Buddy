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

#include <stdbool.h>
#include <stdlib.h>

// TODO: Is the WUI the right place for this?

#ifdef __cplusplus
extern "C" {
#endif

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
 * \param input The input string, null-terminated.
 * \return
 *   - 0 in case no escaping needs to happen (and the string may be used unmodified).
 *   - Number of bytes in the output buffer of jsonify_str, including space for the final \0.
 */
size_t jsonify_str_buffer(const char *input);

/**
 * \brief Just like jsonify_str_buffer, but with size instead of null-terminated.
 *
 * The difference here is that the input doesn't have to be null-terminated and
 * is specified by its length. If any null-bytes are included in the range,
 * they are considered part of the string and would be counted for escaping
 * (that is, this returns either 0 or at least len even if there's a null-byte
 * in the string.
 */
size_t jsonify_str_buffer_len(const char *input, size_t len);

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
void jsonify_str(const char *input, char *output);

/**
 * \brief Variant of jsonify_str for length-delimited strings.
 *
 * Acts in the same way as jsonify_str_buffer, except that the string is not
 * null-terminated, but of given length. Any null-bytes encountered in that
 * range are encoded. The output is null-terminated (even if the input doesn't
 * have to be).
 */
void jsonify_str_len(const char *input, size_t input_len, char *output);

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
 * * Have a `const char *` input variable with a certain name.
 * * Call the macro with that variable name.
 * * A variable `name_escaped` is created. Eg:
 *
 * ```
 * const char *whatever = "hello\nworld";
 * JSONIFY_STR(whatever);
 * printf("%s", whatever_escaped);
 * ```
 */
#define JSONIFY_STR(NAME)                               \
    const size_t NAME##_len = jsonify_str_buffer(NAME); \
    char NAME##_buffer[NAME##_len];                     \
    const char *NAME##_escaped = NAME##_len ? (jsonify_str(NAME, NAME##_buffer), NAME##_buffer) : NAME

#ifdef __cplusplus
}
#endif
