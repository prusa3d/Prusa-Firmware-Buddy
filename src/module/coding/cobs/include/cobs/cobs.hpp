/// \brief Implements Consistent Overhead Byte Stuffing (COBS) for reliable packet framing in serial communication.
///
/// This header file provides functions for encoding and decoding data using the
/// Consistent Overhead Byte Stuffing (COBS) algorithm. COBS ensures that the
/// data bytes do not contain the packet delimiter (0x00), making packet
/// framing unambiguous.
///
/// The implementation is based on the algorithm described on https://www.wikiwand.com/en/articles/Consistent_Overhead_Byte_Stuffing

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <expected>
#include <array>
#include <cstddef>
#include <cstring>
#include <optional>
#include <inplace_function.hpp>
#include <utils/byte_utils.hpp>
namespace cobs {
/**
 * @brief Calculate the maximal theoretical size needed to encode message of frame_size size + the delimiter
 * @note This is a useful utility function that can be used to correctly size output buffers
 */
static constexpr size_t max_encoded_frame_size(size_t frame_size) {
    return frame_size + (frame_size / 254) + 2;
}

enum class CobsError {
    invalid_input,
    overflow,
};
using OptionalBuffer = std::optional<WritableBytes>;

// Standard delimiter per the COBS specification.
// COBS uses 0x00 as the packet delimiter by default.
//
// While a different delimiter byte can be used (e.g., 0xAA or 0x7E),
// doing so requires special care. The encoding and decoding logic must
// treat the new delimiter as a reserved value.
//
// Example:
// If the delimiter is set to e.g. 0xFF, be aware that e.g. 0xFF is a valid code byte
// in standard COBS. The decoder must be carefully designed to distinguish
// between actual data and the delimiter when parsing.
//
// If a non-standard delimiter is required, the encoder and decoder will need a small rework.
constexpr std::byte DELIMITER { 0x00 };

/**
 * @brief Encodes a complete message using COBS and appends frame delimiter.
 *
 * This function is a one-shot alternative to the CobsStreamEncoder class.
 * It's the recommended choice when the entire message is available in a
 * single buffer. The output is a complete COBS frame ready for transmission.
 *
 * @param input The complete, raw message data to be encoded.
 * @param output The buffer where the encoded message will be written.
 *               Required size: at most input.size() + input.size()/254 + 2
 * @return On success, the number of bytes written (including the 0x00 delimiter).
 * @return On failure, a CobsError::overflow if the output buffer is too small.
 */
std::expected<size_t, CobsError> encode(Bytes input, WritableBytes output);

/**
 * @brief Provides a stateful, streaming COBS encoder.
 *
 * This class is designed for building a COBS message in multiple chunks
 * (e.g., when data arrives incrementally).
 *
 * Typical workflow:
 * 1. Construct the encoder with its buffers.
 * 2. Call add_bytes() one or more times to add data.
 * 3. Call finalize() to perform the encoding.
 * 4. (Optional) Call reset() to encode a new message.
 *
 * @note For one-shot encoding (when the full message is already in a
 * buffer), prefer using the free function cobs::encode() instead.
 */
class CobsStreamEncoder {
public:
    /**
     * @brief Constructs a streaming encoder with the provided buffers.
     *
     * @param input_buffer A writable buffer used to accumulate the raw
     * message data via add_bytes(). Its size determines
     * the maximum raw message size.
     * @param encoded_buffer A writable buffer where the final encoded
     * message will be written by finalize(). Its size
     * must be large enough to hold the encoded output.
     */
    CobsStreamEncoder(WritableBytes input_buffer, WritableBytes encoded_buffer);

    /**
     * @brief Appends a chunk of raw data to the message being built.
     *
     * This copies data into the encoder's internal input buffer.
     *
     * @param data The chunk of raw data to add.
     * @return CobsError::overflow if adding this data would exceed the
     * input buffer's capacity.
     */
    std::expected<void, CobsError> add_bytes(Bytes data);

    using EncodeResult = std::expected<Bytes, CobsError>;
    /**
     * @brief Encodes all data added via add_bytes() into the encoded buffer.
     *
     * This function performs the COBS encoding based on the accumulated data
     * in the input buffer. After this call, the encoder is in a
     * "finished" state and reset() must be called to encode a new message.
     *
     * @note This function performs COBS encoding and appends the
     * final 0x00 frame delimiter.
     *
     * @return On success, an EncodeResult containing a span of the valid
     * encoded data within the encoder's encoded_buffer.
     * @return On failure, a CobsError (e.g., CobsError::overflow if the
     * encoded output does not fit in the encoded_buffer).
     */
    [[nodiscard]] EncodeResult finalize();

    /**
     * @brief Resets the encoder to its initial state for a new message.
     *
     * This clears the internal write position. It can optionally be used
     * to provide new buffers to the encoder.
     *
     * @param new_input_buffer If provided, replaces the input buffer.
     * @param new_encoded_buffer If provided, replaces the encoded buffer.
     */
    void reset(OptionalBuffer new_input_buffer = std::nullopt, OptionalBuffer new_encoded_buffer = std::nullopt);

    [[nodiscard]] Bytes get_used_input_buffer() { return input_buffer.first(state.write_position); }

protected:
    CobsStreamEncoder() = default;

private:
    WritableBytes input_buffer;
    WritableBytes encoded_buffer;

    struct CobsEncoderState {
        size_t write_position = 0;
    } state;
};

/**
 * @brief A convenience wrapper for CobsStreamEncoder that owns its buffers.
 *
 * @tparam MAX_MESSAGE_SIZE The maximum *raw* (decoded) message size
 * this encoder will support.
 */
template <size_t MAX_MESSAGE_SIZE>
class ArrayCobsStreamEncoder
    : public CobsStreamEncoder {
public:
    ArrayCobsStreamEncoder()
        : CobsStreamEncoder() {
        reset(input_buffer_, encoded_buffer_);
    }

private:
    std::array<std::byte, MAX_MESSAGE_SIZE> input_buffer_;
    std::array<std::byte, max_encoded_frame_size(MAX_MESSAGE_SIZE)> encoded_buffer_;
};

/**
 * @brief Provides a stateful, streaming COBS decoder.
 *
 * This class processes an incoming stream of bytes (which may contain
 * partial, full, or multiple messages) and decodes COBS-framed messages
 * as it finds them.
 *
 * It is an inherently stateful parser. The user "feeds" encoded data
 * into the decoder, and the decoder invokes callbacks when a complete
 * message is successfully decoded or an error occurs.
 *
 * ## Mode Management
 *
 * The decoder operates in three modes:
 * @see CobsStreamDecoder::Mode
 *
 * Users control mode transitions via reset(Mode) within callbacks or after add_bytes() returns.
 *
 * ## Error Handling Flow
 *
 * When an error occurs during decoding:
 * 1. The decoder calls error_cb with the error type
 * 2. User can call decoder.reset(Mode::recovering) to enter recovery mode
 * 3. If user doesn't specify mode, decoder goes into error mode.
 *
 * Recovery mode automatically scans through the byte stream, discarding
 * all data until a DELIMITER is found. Once found, the decoder automatically
 * transitions back to decoding mode by calling reset(). Recovery can span multiple add_bytes() calls.
 *
 * ## Typical Workflow
 *
 * 1. Construct the decoder with a buffer to write decoded messages into
 * 2. Call add_bytes() with incoming data chunks
 * 3. Implement the finish_cb callback to handle fully decoded messages
 * 4. Implement the error_cb callback to handle stream errors:
 *    - Call decoder.reset(Mode::recovering) for automatic resync (recommended for lossy streams)
 *    - Call decoder.reset(Mode::error) to stop processing (recommended for critical errors)
 * 5. Call reset() to clear state and prepare for a new stream
 *
 * @example
 * ```cpp
 * ArrayCobsStreamDecoder<256> decoder;
 *
 * auto finish = [](std::span<const uint8_t> msg, CobsStreamDecoder& dec) {
 *     // Process the decoded message
 *     process(msg);
 * };
 *
 * auto error = [](CobsError err, CobsStreamDecoder& dec) {
 *      process_error(err);
 *      dec.reset(Mode::recovering);  // Try to recover
 * };
 *
 * decoder.add_bytes(incoming_data, finish, error);
 */
class CobsStreamDecoder {
public:
    enum class Mode {
        decoding, ///< normal operation, processing incoming COBS data
        recovering, ///< scanning for the next DELIMITER (0x00) to resynchronize after an error
        error ///< processing stopped, decoder will ignore all input until reset
    };

    /**
     * @brief Constructs a streaming decoder.
     *
     * @param decoded_buffer A writable buffer where the *decoded* (raw)
     * message data will be written. Its size determines the
     * maximum decoded message size that can be processed.
     */
    CobsStreamDecoder(WritableBytes decoded_buffer, Mode initial_mode = Mode::decoding)
        : decoded_buffer(decoded_buffer) {
        reset(initial_mode, decoded_buffer);
    }

    using FinishCallback = const stdext::inplace_function<void(Bytes)> &;
    using ErrorCallback = const stdext::inplace_function<void(CobsError)> &;
    /**
     * @brief Feeds multiple bytes to the COBS decoder state machine.
     *
     * Processes bytes sequentially according to the current mode:
     * @see CobsStreamEncoder::Mode
     *
     * Processing stops early if the mode is set to 'error' during a callback.
     *
     * @param data Span of bytes to process
     * @param finish_cb Callback invoked when a complete message is decoded.
     *                  Receives the decoded message.
     *                  Can call decoder.reset(Mode) to change operating mode.
     * @param error_cb Callback invoked on decoding errors.
     *                 Receives the error type and a reference to the decoder.
     *                 Should call decoder.reset(Mode::recovering) to enable automatic
     *                 resynchronization, or decoder.reset(Mode::error) to stop processing.
     */
    void add_bytes(Bytes data, FinishCallback finish_cb, ErrorCallback error_cb);

    /**
     * @brief Feeds a single byte to the COBS decoder state machine.
     *
     * Processes one byte according to the current mode. See add_bytes() for behavior details.
     *
     * @param byte The byte to process
     * @param finish_cb Callback invoked when a complete message is decoded
     * @param error_cb Callback invoked when a decoding error occurs
     */
    void add_byte(std::byte byte, FinishCallback finish_cb, ErrorCallback error_cb);

    /**
     * @brief Resets the decoder state and changes mode and optionally used buffer - std::nullopt will keep using the current buffer
     *
     * This clears all internal decoder state (partial message data, block counters, etc.)
     * and sets the decoder to the specified mode.
     *
     * @param mode The mode to transition to
     * @param new_buffer If provided, replaces the decoded buffer
     */
    void reset(Mode mode = Mode::decoding, OptionalBuffer new_buffer = std::nullopt);

    Mode get_mode() { return state.mode; }

protected:
    CobsStreamDecoder() = default;

    struct State {
        uint8_t remaining_block_bytes = 0;
        uint8_t code = 0xFF;
        uint8_t previous_code = 0xFF;
        size_t buffer_idx = 0;
        Mode mode = Mode::decoding;
    } state;

private:
    WritableBytes decoded_buffer;

    void inline error_handler(CobsError error, ErrorCallback error_cb) {
        state.mode = Mode::error;
        error_cb(error);
    }
};

/**
 * @brief A convenience wrapper for CobsStreamDecoder that owns its buffer.
 *
 * This class manages the *decoded buffer* as a std::array member,
 * simplifying setup.
 *
 * @tparam MAX_MESSAGE_SIZE The maximum *raw* (decoded) message size
 * this decoder will support.
 */
template <size_t MAX_MESSAGE_SIZE>
class ArrayCobsStreamDecoder : public CobsStreamDecoder {
public:
    ArrayCobsStreamDecoder(Mode initial_status = Mode::decoding)
        : CobsStreamDecoder() {
        reset(initial_status, buffer);
    }

private:
    std::array<std::byte, MAX_MESSAGE_SIZE> buffer;
};
} // namespace cobs
