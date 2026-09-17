#include <otp.hpp>

#include <otp/utils.hpp>

#include <optional>

static constexpr uintptr_t OTP_START = 0x1FFF'7000; // OTP memory start address for STM32G0
static const Bytes OTP_MEMORY { reinterpret_cast<std::byte *>(OTP_START), 1024 };

static const auto device_otp = []() -> std::optional<OTP_v5> {
    return otp::helpers::load_otp<OTP_v5>(OTP_MEMORY);
}();

std::optional<uint8_t> otp_get_bom_id() {
    return device_otp.transform([](const auto &o) { return o.bomID; });
}

uint32_t otp_get_timestamp() {
    return device_otp.transform([](const auto &o) { return o.timestamp; }).value_or(0);
}

uint8_t otp_get_serial_nr(serial_nr_t &sn) {
    if (!device_otp.has_value()) {
        return 0;
    }
    return otp::helpers::serial_nr_from_datamatrix(device_otp->datamatrix, sn);
}
