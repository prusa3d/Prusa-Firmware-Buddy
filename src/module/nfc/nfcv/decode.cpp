#include <nfcv/decode.hpp>

#include <iso13239/crc.hpp>

#include <utility>
#include <cstdint>
#include <algorithm>
#include <bsod/bsod.h>
#include <utils/byte_utils.hpp>

nfcv::Result<WritableBytes> nfcv::decode(const Bytes &input, const WritableBytes &output) {
    static constexpr std::byte NFCV_RESPONSE_SOF_MASK { 0x1f };
    static constexpr std::byte NFCV_RESPONSE_SOF_PATTERN { 0x17 };
    static constexpr std::byte NFCV_RESPONSE_EOF { 0x1d };
    static constexpr std::byte NFCV_RESPONSE_BIT_PATTERN_MASK { 0x03 };
    static constexpr std::byte NFCV_RESPONSE_BIT_PATTERN_0 { 0x01 };
    static constexpr std::byte NFCV_RESPONSE_BIT_PATTERN_1 { 0x02 };
    static constexpr size_t NFCV_RESPONSE_DATA_BIT_OFFSET = 5;
    static constexpr uint8_t BITS_IN_BYTE = 8;

    // TODO: More strict check here
    if (input.size() == 0) {
        return std::unexpected(Error::buffer_overflow);
    }

    if ((input[0] & NFCV_RESPONSE_SOF_MASK) != NFCV_RESPONSE_SOF_PATTERN) {
        return std::unexpected(Error::response_format_invalid);
    }

    for (size_t bit_pos = 0, i = NFCV_RESPONSE_DATA_BIT_OFFSET;; i += 2) {
        const size_t byte_index = i / BITS_IN_BYTE;
        const size_t bit_offset = i % BITS_IN_BYTE;
        if (byte_index + 1 >= input.size()) {
            return std::unexpected(Error::buffer_overflow);
        }
        // let's construct the current byte offset by bit_offset
        const auto resp_byte = (input[byte_index] >> bit_offset) | (input[byte_index + 1] << (BITS_IN_BYTE - bit_offset));

        const auto out_byte_pos = bit_pos / BITS_IN_BYTE;
        if (resp_byte == NFCV_RESPONSE_EOF) {
            debug_assert(bit_pos % BITS_IN_BYTE == 0);
            return std::span { output.data(), out_byte_pos };
        }

        const auto bit_pattern = resp_byte & NFCV_RESPONSE_BIT_PATTERN_MASK;

        const auto out_bit_pos = bit_pos % BITS_IN_BYTE;
        // NOTE: we need to explicitly set 0, if we are using the same buffer to store the output
        switch (bit_pattern) {
        case NFCV_RESPONSE_BIT_PATTERN_1:
            output[out_byte_pos] |= std::byte(1 << out_bit_pos);
            break;

        case NFCV_RESPONSE_BIT_PATTERN_0:
            output[out_byte_pos] &= ~(std::byte(1 << out_bit_pos));
            break;

        default:
            return std::unexpected(Error::response_format_invalid);
        }
        ++bit_pos;

        if (bit_pos / BITS_IN_BYTE >= output.size()) {
            return std::unexpected(Error::buffer_overflow);
        }
    }

    std::unreachable();
}

namespace nfcv {
namespace {
    bool validate_response_crc(const Bytes &buffer) {
        static constexpr size_t CRC_SIZE = sizeof(iso13239::CRC::ResultType);
        iso13239::CRC crc {};
        crc.add_bytes(std::span { reinterpret_cast<const uint8_t *>(buffer.data()), buffer.size() - CRC_SIZE });
        const auto ref_crc_bytes = buffer.subspan(buffer.size() - CRC_SIZE);
        static_assert(std::endian::native == std::endian::little);
        const auto transmitted_crc = std::to_integer<iso13239::CRC::ResultType>(ref_crc_bytes[0]) | (std::to_integer<iso13239::CRC::ResultType>(ref_crc_bytes[1]) << 8);
        return crc.get_result() == transmitted_crc;
    }

    template <typename T>
    T read_raw(auto &it) {
        static_assert(std::is_trivial_v<T>);
        static_assert(std::endian::native == std::endian::little);
        T result;
        std::copy_n(it, sizeof(T), reinterpret_cast<std::byte *>(&result));
        return result;
    }

    template <typename Command>
    Result<void> parse_response(const Bytes &data, [[maybe_unused]] const Command &command)
        requires(std::is_empty_v<typename Command::Response>)
    {
        if (data.size() != 3) {
            return std::unexpected(Error::response_invalid_size);
        }

        return {};
    }

    static constexpr std::byte NFCV_ERROR_FLAG { 0x01 };
    Result<void> parse_response(const Bytes &data, const nfcv::command::Inventory &command) {
        if (data.size() != command.response.size() + 4 /*1B flags + 1B mask length (is 0) + 0B mask + 2B CRC*/) {
            return std::unexpected(Error::unknown);
        }

        auto iter = data.begin();
        std::advance(iter, 2);
        std::copy_n(iter, command.response.size(), command.response.begin());

        return {};
    }

    Result<void> parse_response(const Bytes &data, const nfcv::command::SystemInfo &command) {
        // Size validation
        const auto info_flags = data[1];
        static constexpr std::byte INFO_DSFID_SUPPORTED { 0x01 };
        static constexpr std::byte INFO_AFI_SUPPORTED { 0x02 };
        static constexpr std::byte INFO_VICC_MEM_SIZE_SUPPORTED { 0x04 };
        static constexpr std::byte INFO_IC_REF_SUPPORTED { 0x08 };
        size_t size = 12; // 1B flags + 1B info_flags + 8B uid + 2B CRC
        if ((info_flags & INFO_DSFID_SUPPORTED) == INFO_DSFID_SUPPORTED) {
            size += 1;
        }
        if ((info_flags & INFO_AFI_SUPPORTED) == INFO_AFI_SUPPORTED) {
            size += 1;
        }
        if ((info_flags & INFO_VICC_MEM_SIZE_SUPPORTED) == INFO_VICC_MEM_SIZE_SUPPORTED) {
            size += 2;
        }
        if ((info_flags & INFO_IC_REF_SUPPORTED) == INFO_IC_REF_SUPPORTED) {
            size += 1;
        }

        if (data.size() != size) {
            return std::unexpected(Error::response_invalid_size);
        }

        // Data parsing
        auto it = data.begin();
        std::advance(it, 10);

        if ((info_flags & INFO_DSFID_SUPPORTED) == INFO_DSFID_SUPPORTED) {
            command.response.dsfid = std::to_integer<uint8_t>(*it);
            ++it;
        } else {
            command.response.dsfid = std::nullopt;
        }
        if ((info_flags & INFO_AFI_SUPPORTED) == INFO_AFI_SUPPORTED) {
            command.response.afi = std::to_integer<uint8_t>(*it);
            ++it;
        } else {
            command.response.afi = std::nullopt;
        }
        if ((info_flags & INFO_VICC_MEM_SIZE_SUPPORTED) == INFO_VICC_MEM_SIZE_SUPPORTED) {
            const auto first = *it;
            ++it;
            const auto second = *it;
            ++it;

            command.response.mem_size = nfcv::TagInfo::MemorySize {
                .block_size = static_cast<uint8_t>((std::to_integer<uint8_t>(second) & 0x1f) + 1),
                .block_count = static_cast<uint8_t>(std::to_integer<uint8_t>(first) + 1),
            };
        } else {
            command.response.mem_size = std::nullopt;
        }
        if ((info_flags & INFO_IC_REF_SUPPORTED) == INFO_IC_REF_SUPPORTED) {
            command.response.ic_ref = std::to_integer<uint8_t>(*it);
            ++it;
        } else {
            command.response.ic_ref = std::nullopt;
        }

        return {};
    }

    Result<void> parse_response(const Bytes &data, const nfcv::command::ReadSingleBlock &command) {
        if (data.size() != (3 + command.response.size())) {
            return std::unexpected(Error::response_invalid_size);
        }

        std::copy_n(std::next(data.begin()), command.response.size(), command.response.begin());

        return {};
    }

    Result<void> parse_response([[maybe_unused]] const Bytes &data, [[maybe_unused]] const nfcv::command::StayQuiet &command) {
        // according to spec the StayQuiet command has no reponse (or it is not mentioned), so we should never call this method
        std::abort();
    }

    Result<void> parse_response(const Bytes &data, const nfcv::command::GetRandomNumber &command) {
        if (data.size() != 3 + sizeof(uint16_t)) {
            return std::unexpected(Error::response_invalid_size);
        }

        auto it = std::next(data.begin());
        command.response = read_raw<uint16_t>(it);
        return {};
    }
} // namespace
} // namespace nfcv

nfcv::Result<void> nfcv::parse_response(const Bytes &data, const Command &command) {
    if (data.size() <= 2) {
        return std::unexpected(Error::response_invalid_size);
    }

    if (!validate_response_crc(data)) {
        return std::unexpected(Error::invalid_crc);
    }

    if ((data[0] & NFCV_ERROR_FLAG) == NFCV_ERROR_FLAG) {
        return std::unexpected(Error::response_is_error);
    }

    return std::visit([&](auto &cmd) { return parse_response(data, cmd); }, command);
}
