#include <cobs/cobs.hpp>
#include <bsod/bsod.h>
#include <cstdint>
namespace cobs {

std::expected<size_t, CobsError> encode(Bytes input, WritableBytes output) {
    // size of encoded empty message
    if (output.size() < 2) {
        return std::unexpected(CobsError::overflow);
    }

    auto code_byte_ptr = output.begin();
    auto out_ptr = output.begin() + 1;
    uint8_t code = 1; // bytes to next encoded delimiter

    for (auto byte : input) {

        if (out_ptr >= output.end()) {
            return std::unexpected(CobsError::overflow);
        }

        // Byte not zero, write it
        if (byte != DELIMITER) {
            *out_ptr = byte;
            out_ptr++;
            code++;
        }

        // Input is zero or block completed, restart
        if (byte == DELIMITER || code == 0xff) {
            *code_byte_ptr = std::byte { code };
            code = 1; // reset code
            code_byte_ptr = out_ptr;
            out_ptr++;
        }
    }
    *code_byte_ptr = std::byte { code }; // Write final code value

    if (out_ptr >= output.end()) {
        return std::unexpected(CobsError::overflow);
    }

    *out_ptr++ = DELIMITER;

    return (out_ptr - output.begin());
}

CobsStreamEncoder::CobsStreamEncoder(WritableBytes input_buffer, WritableBytes encoded_buffer)
    : input_buffer(input_buffer)
    , encoded_buffer(encoded_buffer) {
    reset();
}

std::expected<void, CobsError> CobsStreamEncoder::add_bytes(Bytes data) {

    if (state.write_position + data.size() > input_buffer.size()) {
        return std::unexpected(CobsError::overflow);
    }

    memcpy(input_buffer.data() + state.write_position, data.data(), data.size());
    state.write_position += data.size();
    return {};
}

CobsStreamEncoder::EncodeResult CobsStreamEncoder::finalize() {
    auto result = cobs::encode(
        { input_buffer.data(), state.write_position },
        encoded_buffer);

    if (!result) {
        return std::unexpected(result.error());
    }

    auto encoded_length = result.value();
    return encoded_buffer.first(encoded_length);
}

void CobsStreamEncoder::reset(OptionalBuffer new_input_buffer /* = std::nullopt */, OptionalBuffer new_encoded_buffer /* = std::nullopt */) {
    debug_assert(new_input_buffer.has_value() == new_encoded_buffer.has_value());

    state = {};

    if (new_input_buffer.has_value()) {
        input_buffer = *new_input_buffer;
    }
    if (new_encoded_buffer.has_value()) {
        encoded_buffer = *new_encoded_buffer;
    }
}

void CobsStreamDecoder::add_bytes(Bytes data, FinishCallback finish_cb, ErrorCallback error_cb) {
    for (auto byte : data) {
        add_byte(byte, finish_cb, error_cb);
        if (state.mode == Mode::error) {
            return;
        }
    }
}

void CobsStreamDecoder::add_byte(std::byte byte, FinishCallback finish_cb, ErrorCallback error_cb) {
    switch (state.mode) {
    case Mode::error:
        return;

    case Mode::recovering:
        if (byte == DELIMITER) {
            reset(); // set to valid state
        }
        return;

    case Mode::decoding:

        if (state.remaining_block_bytes == 0) {
            // Waiting for code byte - starting new block
            state.remaining_block_bytes = std::to_integer<uint8_t>(byte);
            state.code = std::to_integer<uint8_t>(byte);

            if (byte == DELIMITER) {
                // A delimiter (0x00) is invalid if it appears where a code
                // byte is expected, AND the previous code was 0xFF.
                // This handles both:
                // 1. Invalid {0x00} at start of stream (prev_code_ is 0xFF)
                // 2. Invalid {0xFF, ...data..., 0x00} (prev_code_ is 0xFF)
                if (state.previous_code == 0xFF) {
                    error_handler(CobsError::invalid_input, error_cb);
                    return;
                }

                // Delimiter found - packet complete
                auto result = decoded_buffer.first(state.buffer_idx);
                reset();
                finish_cb(result);
                return;
            }

            --state.remaining_block_bytes; // Start counting down

            // If we had a previous code that wasn't 0xFF, insert zero
            if (state.previous_code != 0xFF && state.previous_code != to_integer<uint8_t>(DELIMITER)) {
                if (state.buffer_idx >= decoded_buffer.size()) {
                    error_handler(CobsError::overflow, error_cb);
                    return;
                }
                decoded_buffer[state.buffer_idx++] = DELIMITER;
            }
            state.previous_code = state.code;
        } else {
            // incorrectly placed delimiter
            if (byte == DELIMITER) {
                error_handler(CobsError::invalid_input, error_cb);
                return;
            }
            // Reading data bytes in current block
            if (state.buffer_idx >= decoded_buffer.size()) {
                error_handler(CobsError::overflow, error_cb);
                return;
            }

            decoded_buffer[state.buffer_idx++] = byte;
            --state.remaining_block_bytes;
        }
    }
}

void CobsStreamDecoder::reset(Mode mode /* = Mode::decoding*/, OptionalBuffer new_buffer /* = std::nullopt */) {
    state = {};
    state.mode = mode;
    if (new_buffer.has_value()) {
        decoded_buffer = *new_buffer;
    }
}
}; // namespace cobs
