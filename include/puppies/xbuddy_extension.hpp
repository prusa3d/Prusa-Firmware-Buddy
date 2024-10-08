#pragma once
#include "PuppyModbus.hpp"
#include "PuppyBus.hpp"

#include <freertos/mutex.hpp>

namespace buddy::puppies {

class XBuddyExtension final : public ModbusDevice {
public:
    XBuddyExtension(PuppyModbus &bus, const uint8_t modbus_address);

    // These are called from whatever task that needs them.
    void set_fan_pwm(size_t fan_idx, uint8_t pwm);
    void set_white_led(uint8_t intensity);
    void set_rgbw_led(std::array<uint8_t, 4> rgbw);
    std::optional<uint16_t> get_fan_rpm(size_t fan_idx);
    std::optional<float> get_chamber_temp();

    // These are called from the puppy task.
    CommunicationStatus refresh();
    CommunicationStatus initial_scan();
    CommunicationStatus ping();

private:
    static constexpr size_t FAN_CNT = 3;

    // The registers cached here are accessed from different tasks.
    freertos::Mutex mutex;

    // If reading/refresh failed, this'll be in invalid state and we'll return
    // nullopt for queries.
    bool valid = false;

    // TODO: More registers?

    MODBUS_REGISTER Requiremnt {
        // 0-255
        std::array<uint16_t, FAN_CNT> fan_pwm = { 0, 0, 0 };

        // 0-255
        uint16_t white_led = 0;
        // Split into components, each 0-255, for convenience.
        std::array<uint16_t, 4> rgbw_led = { 0, 0, 0, 0 };
    };
    ModbusHoldingRegisterBlock<0x9000, Requiremnt> requirement;

    MODBUS_REGISTER Status {
        uint16_t fan_rpm[FAN_CNT] = { 0, 0, 0 };
        // In degrees * 10 (eg. 23.5°C = 235 in the register)
        uint16_t chamber_temp = 0;
    };
    ModbusInputRegisterBlock<0x8000, Status> status;

    CommunicationStatus refresh_holding();
    CommunicationStatus refresh_input(uint32_t max_age);
};

extern XBuddyExtension xbuddy_extension;

} // namespace buddy::puppies
