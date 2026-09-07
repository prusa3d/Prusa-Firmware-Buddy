#pragma once

#include "commands.hpp"
#include "error.hpp"

#include <expected>
#include <span>
#include <utils/byte_utils.hpp>

namespace nfcv {
/**
 * Decodes incoming nfc-v message responses and stores them in specified buffer according to ISO-15693.
 *
 * Decoded data still needs to be deserialized by @ref parse_response
 *
 * @attention It is safe to call with the same buffer to save RAM.
 *
 * @param input std::span of NFC-V reponse encoded data
 * @param output buffer to store decoded data in (can be same buffer as input)
 * @return std::span of decoded data based on @p output, but with actual decoded size
 */
Result<WritableBytes> decode(const Bytes &input, const WritableBytes &output);

Result<void> parse_response(const Bytes &data, const Command &command);
} // namespace nfcv
