///@file
#include "app.hpp"

#include "hal.hpp"
#include "modbus.hpp"
#include "temperature.hpp"
#include <freertos/delay.hpp>

namespace {

constexpr uint16_t MY_MODBUS_ADDR = 7;
constexpr uint16_t MMU_MODBUS_ADDR = 220;

class Logic final : public modbus::Callbacks {
public:
    Status read_register(uint8_t, const uint16_t address, uint16_t &out) final {
        switch (address) {
        case 0x8000:
            out = hal::fan1::get_raw();
            return Status::Ok;
        case 0x8001:
            out = hal::fan2::get_raw();
            return Status::Ok;
        case 0x8002:
            out = hal::fan3::get_raw();
            return Status::Ok;
        case 0x8003:
            // Note: Mainboard expects this in decidegree Celsius.
            out = 10 * temperature::raw_to_celsius(hal::temperature::get_raw());
            return Status::Ok;
        }
        return Status::IllegalAddress;
    }

    Status write_register(uint8_t, const uint16_t address, const uint16_t value) final {
        switch (address) {
        case 0x9000:
            hal::fan1::set_pwm(value);
            return Status::Ok;
        case 0x9001:
            hal::fan2::set_pwm(value);
            return Status::Ok;
        case 0x9002:
            hal::fan3::set_pwm(value);
            return Status::Ok;
        case 0x9003:
            hal::w_led::set_pwm(value);
            return Status::Ok;
        case 0x9004:
            hal::rgbw_led::set_r_pwm(value);
            return Status::Ok;
        case 0x9005:
            hal::rgbw_led::set_g_pwm(value);
            return Status::Ok;
        case 0x9006:
            hal::rgbw_led::set_b_pwm(value);
            return Status::Ok;
        case 0x9007:
            hal::rgbw_led::set_w_pwm(value);
            return Status::Ok;
        }
        return Status::IllegalAddress;
    }
};

class MMU final : public modbus::Callbacks {
public:
    Status read_register(uint8_t, uint16_t, uint16_t &) {
        // TODO: MMU not implemented yet, stubbed out for now.
        return Status::GatewayPathUnavailable;
    }
    Status write_register(uint8_t, uint16_t, uint16_t) {
        // TODO: MMU not implemented yet, stubbed out for now.
        return Status::GatewayPathUnavailable;
    }
};

class Dispatch final : public modbus::Callbacks {
public:
    // Do we want these to be some kind of number-value array, or is this hardcoded thing good enough?
    Logic logic;
    MMU mmu;
    Status read_register(uint8_t device, uint16_t address, uint16_t &out) {
        switch (device) {
        case MY_MODBUS_ADDR:
            return logic.read_register(device, address, out);
        case MMU_MODBUS_ADDR:
            return mmu.read_register(device, address, out);
        default:
            return Status::Ignore;
        }
    }

    Status write_register(uint8_t device, uint16_t address, uint16_t value) {
        switch (device) {
        case MY_MODBUS_ADDR:
            return logic.write_register(device, address, value);
        case MMU_MODBUS_ADDR:
            return mmu.write_register(device, address, value);
        default:
            return Status::Ignore;
        }
    }
};

} // namespace

void app::run() {
    hal::fan1::set_enabled(true);
    hal::fan2::set_enabled(true);
    hal::fan3::set_enabled(true);

    Dispatch modbus_callbacks;

    std::byte response_buffer[32]; // is enough for now
    hal::rs485::start_receiving();
    for (;;) {
        const auto request = hal::rs485::receive();
        const auto response = modbus::handle_transaction(modbus_callbacks, request, response_buffer);
        if (response.size()) {
            freertos::delay(1);
            hal::rs485::transmit_and_then_start_receiving(response);
        } else {
            hal::rs485::start_receiving();
        }
    }
}
