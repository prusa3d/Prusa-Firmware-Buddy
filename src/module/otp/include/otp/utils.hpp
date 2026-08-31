///@file
#pragma once

#include "types.hpp"

#include <bsod/bsod.h>
#include <utils/byte_utils.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <optional>
#include <variant>

namespace otp {
namespace helpers {
    struct UndefinedType;

    template <typename OTP_Type>
    inline OTP_Type load_otp_unsafe(const Bytes otp) {
        contract_assert(otp.size() >= sizeof(OTP_Type));
        OTP_Type res {};
        WritableBytes res_bytes = trivial_as_writable_bytes(res);

        std::copy_n(otp.begin(), res_bytes.size(), res_bytes.begin());

        return res;
    }

    /**
     * @brief Get OTP structure version.
     * @param scr address of the OTP structure, use (uint8_t*)OTP.data() to read own OTP
     * @return version number or std::nullopt if the version is not defined
     */
    constexpr std::optional<uint8_t> get_structure_version(const Bytes otp) {
        contract_assert(otp.size() > 0); // Trying to parse OTP from nullptr
        if (otp[0] == std::byte { 0xFF }) {
            return std::nullopt;
        }
        return static_cast<uint8_t>(otp[0]);
    }

    using AnyOTP = std::variant<OTP_v0, OTP_v1, OTP_v2, OTP_v3, OTP_v4, OTP_v5>;

    constexpr std::optional<AnyOTP> parse_otp(const Bytes otp) {
        const auto version = get_structure_version(otp);
        if (!version.has_value()) {
            return std::nullopt;
        }
        switch (*version) {
        case 0:
            return { load_otp_unsafe<OTP_v0>(otp) };
        case 1:
            return { load_otp_unsafe<OTP_v1>(otp) };
        case 2:
            return { load_otp_unsafe<OTP_v2>(otp) };
        case 3:
            return { load_otp_unsafe<OTP_v3>(otp) };
        case 4:
            return { load_otp_unsafe<OTP_v4>(otp) };
        case 5:
            return { load_otp_unsafe<OTP_v5>(otp) };
        default:
            break;
        }
        bsod_unreachable();
    }

    inline uint8_t serial_nr_from_datamatrix(DatamatrixRawView datamatrix, serial_nr_t &sn) {
        std::ranges::copy(datamatrix, sn.begin());
        sn[datamatrix.size()] = '\0';
        return datamatrix.size() + 1;
    }

    template <typename OTP_Type>
    inline std::optional<OTP_Type> load_otp(const Bytes &otp) {
        const auto version = get_structure_version(otp);
        if (!version.has_value()) {
            return std::nullopt;
        }
        contract_assert(*version == OTP_Type::version_number);
        return { load_otp_unsafe<OTP_Type>(otp) };
    }

} // namespace helpers

constexpr std::optional<datamatrix_t> decode_datamatrix(DatamatrixRawView datamatrix) {
    debug_assert(datamatrix.data() != nullptr);
    datamatrix_t dm;

    auto *src = datamatrix.data();
    const auto *end = datamatrix.data() + datamatrix.size();

    const auto scan_digit_group = [&src, end](uint8_t digits) {
        uint32_t rv = 0;
        for (; digits && src != end && isdigit(*src); --digits) {
            rv *= 10;
            rv += (*src - '0');
            ++src;
        }
        return rv;
    };

    dm.product_id = scan_digit_group(5);
    debug_assert((src + 1) < end);
    if (*src != '-') {
        return std::nullopt; // couldn't find the '-' separator
    }

    // skip '-'
    ++src;

    // product revision
    dm.revision = scan_digit_group(2);

    // supplier ID (6 digits or letters)
    debug_assert((src + dm.supplier_id.size()) < end);
    std::copy_n(src, dm.supplier_id.size(), dm.supplier_id.begin());
    src += dm.supplier_id.size();
    static_assert(std::size(dm.supplier_id) == 6, "if changed we might not fit, just for verification");

    // year
    dm.production_year = scan_digit_group(1) + 2020;

    // If product ID is 4 digit, or smaller than 10381 read date in format YMMDD
    // else use format YCW (calendar week)
    if (dm.product_id < 10381) {
        // month
        dm.production_month = scan_digit_group(2);

        // day
        dm.production_day = scan_digit_group(2);

        // daily serial
        dm.date_serial_number = scan_digit_group(4);
    } else {
        // Conversion from the ISO calendar week to approximate production day and month i.e. thursday of given week
        auto production_week = static_cast<uint16_t>(scan_digit_group(2));

        /// Thursday of the given ISO-8601 calendar week.
        namespace chrono = std::chrono;

        // ISO week 1 is the week containing 4 January
        const auto jan_4 = chrono::sys_days { chrono::year { dm.production_year } / chrono::January / 4 };
        const auto first_week_thursday = jan_4 - (chrono::weekday { jan_4 } - chrono::Monday) + chrono::days { 3 };

        const auto production_date = chrono::year_month_day { first_week_thursday + chrono::weeks { production_week - 1 } };

        dm.production_month = static_cast<uint8_t>(static_cast<unsigned>(production_date.month()));
        dm.production_day = static_cast<uint8_t>(static_cast<unsigned>(production_date.day()));

        // weekly serial
        dm.date_serial_number = scan_digit_group(5);
    }

    return dm;
}
} // namespace otp
