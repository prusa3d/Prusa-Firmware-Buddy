///@file
#include "app.hpp"

#include "hal.hpp"
#include "modbus.hpp"
#include "temperature.hpp"
#include <freertos/delay.hpp>

class ModbusCallbacks final : public modbus::Callbacks {
public:
    uint16_t read_register(const uint16_t address) final {
        switch (address) {
        case 0x8000:
            return hal::fan1::get_raw();
        case 0x8001:
            return hal::fan2::get_raw();
        case 0x8002:
            return hal::fan3::get_raw();
        case 0x8003:
            // Note: Mainboard expects this in decidegree Celsius.
            return 10 * temperature::raw_to_celsius(hal::temperature::get_raw());
        }
        return 0;
    }

    void write_register(const uint16_t address, const uint16_t value) final {
        switch (address) {
        case 0x9000:
            hal::fan1::set_pwm(value);
            break;
        case 0x9001:
            hal::fan2::set_pwm(value);
            break;
        case 0x9002:
            hal::fan3::set_pwm(value);
            break;
        case 0x9003:
            hal::w_led::set_pwm(value);
            break;
        case 0x9004:
            hal::rgbw_led::set_r_pwm(value);
            break;
        case 0x9005:
            hal::rgbw_led::set_g_pwm(value);
            break;
        case 0x9006:
            hal::rgbw_led::set_b_pwm(value);
            break;
        case 0x9007:
            hal::rgbw_led::set_w_pwm(value);
            break;
        }
    }
};

void app::run() {
    hal::fan1::set_enabled(true);
    hal::fan2::set_enabled(true);
    hal::fan3::set_enabled(true);

    ModbusCallbacks modbus_callbacks;

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
