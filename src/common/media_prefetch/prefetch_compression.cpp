#include "prefetch_compression.hpp"

#include <array>
#include <cctype>
#include <algorithm>
#include <cstring>

#ifdef UNITTESTS
    #include <catch2/catch.hpp>
    #undef assert
    #define assert(x) REQUIRE(x)
#else
    #include <assert.h>
#endif

using namespace media_prefetch;

namespace {

/*
 * General idea of the compression algorithm:
 * The gcode uses only a quite limited character set compared to the full ASCII table. It's mostly numbers, plus some G, X, Y, Z here and there.
 * So, we encode the gcode using 4-bit words instead of 8-bit, meaning that the characters are encoded in 4 bits.
 * That would leave us 16 possible characters, which is not enough, though, so instead we encode the less used characers using 2 words (so 1 byte) using a special continuation character.
 *
 * So, the 4-bit words have this meaning:
 * - If word_value is in [0, primary_page_size), we're encoding a character from character_list[word_value] in 4-bits.
 * - If word_value is in [primary_page_size, 16), that's a two-word character encoding.
 *    - (word_value - primary_page_size) denotes index of the secondary page.
 *    - The following 4-bit word denotes index of the character in that page.
 *    - The formula is character_list[primary_page_size + (first_word - primary_page_size) * secondary_page_size + second_word]
 */

constexpr uint8_t secondary_page_count = 1;

// We're working with 4-bit words
constexpr uint8_t secondary_page_size = (1 << 4);

/// For the first 4 bits, we either encode the ID of the secondary page or the character from the primary page
constexpr uint8_t primary_page_size = secondary_page_size - secondary_page_count;

/// How many different characters we're able to compress, given the selected page configuration
constexpr uint8_t max_supported_compressed_characters = primary_page_size + secondary_page_count * secondary_page_size;

/// Characters we're compressing. The first primary_page_size characters will be encoded in 4 bits, the rest in 8 bits
constexpr std::array character_list {
    // First 15 characters that fit on the primary page
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', '-', 'Y', 'E', 'I',

    // The rest that fits on secondary pages
    'Z', 'X', 'F', 'J', 'P', 'R', 'S', 'M', 'G', ' '
};

/// Common ways a gcode command can start. We can encode 16 possible options in a single 4-bit word
constexpr std::array prefix_dictionary {
    "M486S", // Cancel object
    "M204P", // Set acceleration
    "M73P", // Progress report
    "M73Q", // Progress report
    "G1X1",
    "G0X",
    "G1X",
    "G1Z", // For spiral vase gcode
    "G1F",
    "G1E",
    "G2X",
    "G3X", // Arc
    // Even the following single-character prefixes make sense - this way we encode them using one word, otherwise we would need 3 (0 -> no prefix, 1 -> page, 2 -> index on page)
    "G",
    "M",
};
static_assert(prefix_dictionary.size() < 15); // - 1 for encoding a non-match

// Must be sorted by length so that we hit the longest match first in the iteration
static_assert(std::is_sorted(prefix_dictionary.begin(), prefix_dictionary.end(), [](auto a, auto b) { return strlen(a) > strlen(b); }));

} // namespace

// We're working with 4-bit words, so we cannot encode more than 16 indexes
static_assert(secondary_page_count < (1 << 4));
static_assert(character_list.size() <= max_supported_compressed_characters);

// If we're not utilising a whole secondary page, we can reduce their count to make the algorithm more optimal
static_assert(character_list.size() > max_supported_compressed_characters - secondary_page_size);

/// \returns If \p str starts with \p prefix, returns the length of prefix, otherwise returns 0
static size_t starts_with(const char *str, const char *prefix) {
    const char *prefix_start = prefix;
    while (*prefix && *prefix == *str) {
        prefix++;
        str++;
    }

    return (*prefix == '\0') ? (prefix - prefix_start) : 0;
}

size_t media_prefetch::compact_gcode(char *inplace_buffer) {
    const char *read_ptr = inplace_buffer;
    char *write_ptr = inplace_buffer;

    // Skip whitespaces at the beginning of the gcode, every time
    while (isspace(*read_ptr)) {
        read_ptr++;
    }

    const bool is_gx_gcode = (read_ptr[0] == 'G') && (isdigit(read_ptr[1]));

    // If the command is a G command, strip all whitespaces.
    // Other commands might contain strings and such, so we will rather not do it
    const bool strip_whitespaces = is_gx_gcode;

    // Skip comments for Gx gcodes, or if the line is just a comment
    const bool skip_comments = is_gx_gcode || (read_ptr[0] == ';');

    while (true) {
        const char ch = *read_ptr++;

        if (ch == '\0' || (ch == ';' && skip_comments)) {
            // Null or comment start - write '\0' and finish
            *write_ptr = '\0';
            return write_ptr - inplace_buffer;

        } else if (strip_whitespaces && isspace(ch)) {
            // Whitespace if we're stripping whitespaces -> skip
            continue;

        } else {
            // Otherwise copy to output
            *write_ptr++ = ch;
        }
    }
}

std::optional<size_t> media_prefetch::compress_gcode(const char *input, std::span<uint8_t> output) {
    // Special case for empty strings
    if (*input == '\0') {
        return 0;
    }

    uint8_t *output_ptr = output.data();
    const uint8_t *const output_end = output_ptr + output.size();
    bool second_word_in_byte = false;

    /// Writes a 4-bit word to the output
    const auto write_word = [&](uint8_t word) {
        assert(word <= 15);
        if (output_ptr >= output_end) {
            return false;
        }

        if (!second_word_in_byte) {
            *output_ptr = word;
            second_word_in_byte = true;

        } else {
            *output_ptr |= (word << 4);
            second_word_in_byte = false;
            output_ptr++;
        }

        return true;
    };

    // Check if the gcode starts with any of the common prefixes we have in the dictionary
    {
        size_t prefix_len = 0;

        // Look for the first prefix the input starts with
        const auto prefix_pred = [&](const char *prefix) {
            // Returning true stops the std::find_if iteration, so the matching prefix_len will remain stored
            prefix_len = starts_with(input, prefix);
            return prefix_len != 0;
        };
        const auto prefix_it = std::find_if(prefix_dictionary.begin(), prefix_dictionary.end(), prefix_pred);
        // No match -> writes prefix_dictionary.size()
        write_word(prefix_it - prefix_dictionary.begin());
        input += prefix_len;
    }

    while (true) {
        char ch = *input++;

        // If we've hit end of string, we have a success
        if (ch == '\0') {
            if (second_word_in_byte) {
                // If we're in the middle of writing a byte, fill the last 4 bits with an unfinished escape sequence to make sure we don't emit another character
                write_word(15);
                static_assert(primary_page_size <= 15);
            }

            return output_ptr - output.data();
        }

        const auto encoded_it = std::find(character_list.begin(), character_list.end(), ch);

        // The character is not in the supported list -> fail
        if (encoded_it == character_list.end()) {
            return std::nullopt;
        }

        const int encoded_id = encoded_it - character_list.begin();

        if (encoded_id < primary_page_size) {
            // Is in primary page -> just write  one word
            if (!write_word(encoded_id)) {
                return std::nullopt;
            }

        } else {
            // Is in secondary page -> first word is the page number, second word is position on that page
            const auto page_ix = (encoded_id - primary_page_size) / secondary_page_size;
            if ( //
                !write_word(primary_page_size + page_ix) //
                || !write_word((encoded_id - primary_page_size) % secondary_page_size) //
            ) {
                return std::nullopt;
            }
        }
    }
}

void media_prefetch::decompress_gcode(const uint8_t *input, int compressed_len, std::span<char> output) {
    const uint8_t *input_ptr = input;
    const uint8_t *const input_end = input_ptr + compressed_len;
    char *output_ptr = output.data();
    bool second_word_in_byte = false;

    // Special case for empty strings
    if (compressed_len == 0) {
        output[0] = '\0';
        return;
    }

    /// Reads a 4-bit word from the input
    const auto read_word = [&]() -> uint8_t {
        assert(input_ptr < input_end);

        second_word_in_byte = !second_word_in_byte;

        // We're now checking the value after the toggle, so it's inverse
        if (second_word_in_byte) {
            return (*input_ptr) & 0xf;
        } else {
            return (*input_ptr++) >> 4;
        }
    };

    // First word is always a prefix index (from prefix_dictionary)
    if (const auto prefix_ix = read_word(); prefix_ix < prefix_dictionary.size()) {
        output_ptr += strlcpy(output_ptr, prefix_dictionary[prefix_ix], output.size());
    }

    while (input_ptr < input_end) {
        const auto first_word = read_word();
        char decoded_char;

        if (first_word < primary_page_size) {
            decoded_char = character_list[first_word];

        } else if (input_ptr == input_end) {
            // If by reading the first word, we've got to the end of the buffer, just return.
            // This means that the compressed data was not in multiples of bytes and the compressor just emitted an unfinished control sequence to prevent us from emitting chars.
            break;

        } else {
            const auto page_ix = first_word - primary_page_size;
            const auto second_word = read_word();
            decoded_char = character_list[primary_page_size + page_ix * secondary_page_size + second_word];
        }

        *output_ptr++ = decoded_char;
    }

    // Write terminating null
    *output_ptr = '\0';
}
