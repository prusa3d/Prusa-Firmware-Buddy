#include "otp.hpp"

#include <bsod/bsod.h>
#include <otp/utils.hpp>
#include <utils/overloaded_visitor.hpp>

#ifdef STM32F4
    #include <stm32f4xx.h>
#elif defined(UNITTEST)

#else /*MCU*/
    #error "Unknown MCU"
#endif /*MCU*/

#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <ranges>
#include <span>

// Private absolute addresses for items already provided by the public otp functions.
// These absolute addresses/constants shall not be used outside of otp.cpp anymore!
#if defined(UNITTEST) && !defined(STM32F4)
static constexpr size_t UID_BASE = 0;
#endif

static constexpr size_t OTP_DATAMATRIX_TEXT_SIZE = sizeof(OTP_v2::datamatrix); // size of textual representation of Datamatrix (as burned in OTP)

static_assert(sizeof(serial_nr_t) == OTP_DATAMATRIX_TEXT_SIZE + 1, "serial number size inconsistent");
static_assert(sizeof(board_revision_t) == 2, "board_revision_t size inconsistent");
static_assert(sizeof(OTP_v0) == 32, "OTP_v0 size inconsistent");
static_assert(sizeof(OTP_v1) == 32, "OTP_v1 size inconsistent");
static_assert(sizeof(OTP_v2) == 32, "OTP_v2 size inconsistent");
static_assert(sizeof(OTP_v3) == 76, "OTP_v3 size inconsistent");
static_assert(sizeof(OTP_v4) == 38, "OTP_v4 size inconsistent");
static_assert(sizeof(OTP_v5) == 32, "OTP_v5 size inconsistent");

namespace {

Bytes otp_memory() {
#ifdef STM32F4
    return { reinterpret_cast<std::byte *>(FLASH_OTP_BASE), FLASH_OTP_END - FLASH_OTP_BASE };
#elif defined(UNITTEST) /*MCU*/
    static constexpr std::array<std::byte, 1024> memory {};
    return memory;
#else
    #error "Unknown MCU"
#endif /*MCU*/
}

// Parsed on first use, because the first read happens before the static initializers run.
const std::optional<otp::helpers::AnyOTP> &board_otp() {
    static const auto parsed = otp::helpers::parse_otp(otp_memory());
    return parsed;
}

inline std::optional<datamatrix_t> extract_datamatrix(const otp::helpers::AnyOTP &otp_str) {
    return match(otp_str, []<typename T>([[maybe_unused]] const T &otp) -> std::optional<datamatrix_t> {
        if constexpr (requires { T::datamatrix; }) {
            return otp::decode_datamatrix(otp.datamatrix);
        } else {
            return std::nullopt;
        }
    });
}

inline std::optional<board_revision_t> extract_board_revision(const otp::helpers::AnyOTP &otp_str) {
    return match(otp_str, []<typename T>([[maybe_unused]] const T &otp) -> std::optional<board_revision_t> {
        if constexpr (requires { T::datamatrix; }) {
            return otp::decode_datamatrix(otp.datamatrix).transform([](const datamatrix_t &dm) {
                return static_cast<board_revision_t>(dm.revision);
            });
        } else if constexpr (requires { T::board_revision; }) {
            // MAJOR.MINOR.PATCH, each a byte, packed into one decimal number
            return static_cast<board_revision_t>(otp.board_revision[0] * 100 + otp.board_revision[1] * 10 + otp.board_revision[2]);
        } else {
            return std::nullopt;
        }
    });
}

/// The returned pointer refers into `otp_str`.
inline const MAC_addr *extract_mac_address(const otp::helpers::AnyOTP &otp_str) {
    return match(otp_str, []<typename T>([[maybe_unused]] const T &otp) -> const MAC_addr * {
        if constexpr (requires { T::mac_address; }) {
            return &otp.mac_address;
        } else {
            return nullptr;
        }
    });
}

inline std::optional<uint32_t> extract_timestamp(const otp::helpers::AnyOTP &otp_str) {
    return match(otp_str, []<typename T>([[maybe_unused]] const T &otp) -> std::optional<uint32_t> {
        if constexpr (requires { T::timestamp; }) {
            return { otp.timestamp };
        } else {
            return std::nullopt;
        }
    });
}

static uint8_t parse_czpx_serial_number(serial_nr_t &sn, std::span<const uint8_t, std::tuple_size_v<decltype(OTP_v0::serialnumber)>> serial_number) {
    static constexpr std::string_view prefix = "CZPX";
    auto out = sn.begin();
    out = std::copy_n(prefix.begin(), prefix.size(), out);
    debug_assert(serial_number.size() < sn.size() - prefix.size());
    out = std::ranges::copy(serial_number | std::views::take_while(isprint), out).out;
    std::fill(out, sn.end(), '\0');
    return strlen(sn.data());
}

uint8_t copy_or_construct_serial_nr_from_otp(serial_nr_t &sn, const otp::helpers::AnyOTP &otp_str) {
    return match(otp_str, [&sn]<typename T>([[maybe_unused]] const T &otp) -> uint8_t {
        if constexpr (requires { T::datamatrix; }) {
            return otp::helpers::serial_nr_from_datamatrix(otp.datamatrix, sn);
        } else if constexpr (requires { T::serialnumber; }) { // old boards
            return parse_czpx_serial_number(sn, std::span { otp.serialnumber });
        }
        return 0;
    });
}

std::optional<uint8_t> extract_bom_id(const otp::helpers::AnyOTP &otp_str) {
    return match(otp_str, []<typename T>([[maybe_unused]] const T &otp) -> std::optional<uint8_t> {
        if constexpr (requires { T::bomID; }) {
            return { otp.bomID };
        } else {
            return std::nullopt;
        }
    });
}

} // namespace

std::optional<board_revision_t> otp_get_board_revision() {
    return board_otp().and_then(extract_board_revision);
}

std::optional<board_revision_t> otp_parse_board_revision(const Bytes memory) {
    return otp::helpers::parse_otp(memory).and_then(extract_board_revision);
}

std::optional<datamatrix_t> otp_get_datamatrix() {
    return board_otp().and_then(extract_datamatrix);
}

std::optional<datamatrix_t> otp_parse_datamatrix(const Bytes memory) {
    return otp::helpers::parse_otp(memory).and_then(extract_datamatrix);
}

std::optional<datamatrix_t> otp_serial_nr_to_datamatrix(const serial_nr_t &sn) {
    return otp::decode_datamatrix(std::span<const uint8_t, std::tuple_size_v<DatamatrixRaw>> { reinterpret_cast<const uint8_t *>(sn.begin()), sn.size() - 1 });
}

const MAC_addr *otp_get_mac_address() {
    const auto &otp = board_otp();
    if (!otp.has_value()) {
        return nullptr;
    }
    return extract_mac_address(*otp);
}

std::optional<MAC_addr> otp_parse_mac_address(const Bytes memory) {
    return otp::helpers::parse_otp(memory).and_then([](const otp::helpers::AnyOTP &otp) -> std::optional<MAC_addr> {
        const MAC_addr *mac = extract_mac_address(otp);
        if (mac == nullptr) {
            return std::nullopt;
        }
        return *mac;
    });
}

MAC_str otp_get_mac_address_str() {
    MAC_str mac_address_str;
    mac_address_str.fill(0);

    const MAC_addr *mac = otp_get_mac_address();
    if (mac != nullptr) {
        snprintf(mac_address_str.begin(), mac_address_str.size(),
            "%02x:%02x:%02x:%02x:%02x:%02x",
            mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
    }
    return mac_address_str;
}

uint32_t otp_get_timestamp() {
    return board_otp().and_then(extract_timestamp).value_or(0);
}

std::optional<uint32_t> otp_parse_timestamp(const Bytes memory) {
    return otp::helpers::parse_otp(memory).and_then(extract_timestamp);
}

const STM32_UUID *otp_get_STM32_UUID() {
    return reinterpret_cast<const STM32_UUID *>(UID_BASE);
}

uint8_t otp_get_serial_nr(serial_nr_t &sn) {
    if (!board_otp().has_value()) {
        return 0;
    }
    return copy_or_construct_serial_nr_from_otp(sn, board_otp().value());
}

uint8_t otp_parse_serial_nr(serial_nr_t &sn, const Bytes memory) {
    const auto otp_str = otp::helpers::parse_otp(memory);
    if (!otp_str.has_value()) {
        return 0;
    }
    return copy_or_construct_serial_nr_from_otp(sn, otp_str.value());
}

std::optional<uint8_t> otp_get_bom_id() {
    return board_otp().and_then(extract_bom_id);
}

std::optional<uint8_t> otp_parse_bom_id(const Bytes memory) {
    return otp::helpers::parse_otp(memory).and_then(extract_bom_id);
}
