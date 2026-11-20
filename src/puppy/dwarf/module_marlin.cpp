#include "module_marlin.hpp"

#include <device/peripherals.hpp>
#include <device/hal.h>
#include "Marlin.h"
#include "modbus/ModbusTask.hpp"
#include <PuppyConfig.hpp>
#include "wiring_analog.h"
#include "wiring_digital.h"
#include <logging/log.hpp>
#include "gpio.h"
#include "modbus/ModbusInit.hpp" //for modbus::modbus_control
#include "loadcell.hpp"
#include "led.h"
#include "adc.hpp"
#include "safe_state.h"
#include "Cheese.hpp"
#include "fanctl.hpp"
#include "task_startup.h"
#include <hal/HAL_MultiWatchdog.hpp>
#include <advanced_power.hpp>
#include <bsod.h>
#include "Marlin/src/module/temperature.h"

#include <option/has_planner.h>
#if HAS_PLANNER()
    // #error dead code found by automatic analyses (see BFW-5461)
    #include <module/planner.h>
#endif

LOG_COMPONENT_REF(Marlin);

[[noreturn]] void stop_marlin();

/**
 * @brief Marlin's watchdog init.
 * Empty as IWDG is initialized here.
 */
void watchdog_init() {
}

static hal::MultiWatchdog marlin_timer_wdg; // Add one instance of watchdog

/**
 * @brief Watchdog refresh called from Marlin from timer interrupt.
 */
void HAL_watchdog_refresh() {
    marlin_timer_wdg.kick(false);
}

// Check dwarf is operated under safe conditions
void check_operation_safety() {
    if constexpr (ENABLE_HEATER_CURRENT_CHECKS) {
        // Filter nozzle current using moving average
        static float avg_nozzle_current = 0;
        avg_nozzle_current = (avg_nozzle_current * 9 + advancedpower.GetDwarfNozzleCurrent()) / 10;

        // Fail if nozzle current too high
        if (avg_nozzle_current > MAX_HEATER_CURRENT_A) {
            // TODO: Would be nice to pass nozzle current and configured maximum to the error message
            kill(GET_TEXT(MSG_ERR_NOZZLE_OVERCURRENT));
        }
    }
}

static auto &watchdog() {
    static hal::MultiWatchdog wdg; // Add one instance of watchdog
    return wdg;
}

void dwarf::modules::marlin::start() {
    int too_hot;
    auto &wdg = watchdog();
    setup();

    modbus::ModbusTask::EnableModbus();

    while (true) {
        switch (ModbusControl::status_led.mode) {
        case dwarf_shared::StatusLed::Mode::solid_color:
            led::set_rgb(ModbusControl::status_led.r, ModbusControl::status_led.g, ModbusControl::status_led.b);
            break;
        case dwarf_shared::StatusLed::Mode::blink:
            led::blinking(ModbusControl::status_led.r, ModbusControl::status_led.g, ModbusControl::status_led.b, 500, 500);
            break;
        case dwarf_shared::StatusLed::Mode::pulse:
            led::pulsing(ModbusControl::status_led.r, ModbusControl::status_led.g, ModbusControl::status_led.b,
                ModbusControl::status_led.r >> 3, ModbusControl::status_led.g >> 3, ModbusControl::status_led.b >> 3, // Pulse to 1/8 of the color
                1000);
            break;
        case dwarf_shared::StatusLed::Mode::pulse_w:
            led::pulsing(ModbusControl::status_led.r, ModbusControl::status_led.g, ModbusControl::status_led.b,
                0xff, 0xff, 0xff, // Pulse to white
                1000);
            break;
        default: // Follow internal status
            too_hot = (Temperature::degHotend(0) > 50);

            if (dwarf::ModbusControl::isDwarfSelected()) {
                if (too_hot) {
                    led::pulsing(0x0f, 0x07, 0, 0x0f, 0x0f, 0x0f, 1000);
                } else {
                    led::blinking(0x0f, 0x0f, 0x0f, 500, 500);
                }
            } else if (!Cheese::is_parked()) {
                if (too_hot) {
                    led::pulsing(0x0f, 0x00, 0, 0x0f, 0x0c, 0x00, 250);
                } else {
                    led::blinking(0x0f, 0x0c, 0, 200, 50);
                }
            } else {
                if (too_hot) {
                    led::pulsing(0x0f, 0x07, 0, 0x00, 0x0f, 0x00, 1000);
                } else {
                    led::blinking(0, 0x0f, 0, 1000, 75);
                }
            }
            break;
        }

        idle(false);

        dwarf::loadcell::loadcell_loop();

        Cheese::update();

        // process any modbus request that are pending
        dwarf::ModbusControl::ProcessModbusMessages();
        dwarf::ModbusControl::UpdateRegisters();

        check_operation_safety();

        // Reload this instance of watchdog
        wdg.kick();

        // Needs to be delay and not osThreadYield(),
        // with yield it would not let idle task to refresh another watchdog instance
        osDelay(1);
    }
}

[[noreturn]] void stop_marlin() {
#if HAS_PLANNER()
    // #error dead code found by automatic analyses (see BFW-5461)
    planner.quick_stop();
#endif

    dwarf::ModbusControl::ProcessModbusMessages();
    dwarf::ModbusControl::UpdateRegisters();

    // Safe state AFTER processing modbus messages
    // The modbus message could try to set the nozzle temp and whatnot
    DISABLE_TEMPERATURE_INTERRUPT();
    dwarf_init_done = false;
    hwio_safe_state();

    while (1) {
        watchdog().kick(); // Reload this instance of watchdog
        led::blinking(0xFF, 0x00, 0x00, 500, 500);
        osDelay(100);
    }
}

void analogWrite(uint32_t ulPin, uint32_t ulValue) {
    switch (ulPin) {
    case MARLIN_PIN(FAN): // print fan
        Fans::print(PhysicalToolIndex::from_raw(0)).set_pwm(ulValue);
        return;
    default:
        bsod("Write undefined pin");
    }
}

uint32_t analogRead(uint32_t marlin_pin) {
    switch (marlin_pin) {
    case MARLIN_PIN(TEMP_HEATBREAK): {
        return AdcGet::heatbreakTemp();
    }
    case MARLIN_PIN(TEMP_0): {
        return AdcGet::nozzle();
    }
    case MARLIN_PIN(TEMP_BOARD): {
        return AdcGet::boardTemp();
    }
    default:
        bsod("Read undefined pin");
    }
}

void pinMode([[maybe_unused]] uint32_t marlin_pin, [[maybe_unused]] uint32_t value) {
    // not used
}

void digitalWrite(uint32_t marlin_pin, uint32_t value) {
    gpio_set(marlin_pin, value ? 1 : 0);
}

int digitalRead(uint32_t marlinPin) {
    return gpio_get(marlinPin);
}

void kill(PGM_P const lcd_error /*=nullptr*/, PGM_P const lcd_component /*=nullptr*/, [[maybe_unused]] const bool steppers_off /*=false*/) {
    log_error(Marlin, "Printer killed: %s: %s", lcd_component, lcd_error);
    dwarf::ModbusControl::TriggerMarlinKillFault(dwarf_shared::errors::FaultStatusMask::MARLIN_KILLED, lcd_component, lcd_error);
    stop_marlin();
}

[[noreturn]] void fatal_error(ErrCode error_code, ...) {
    log_error(Marlin, "Fatal error: %i", (int)error_code);
    std::array<char, 32> err;
    snprintf(err.data(), err.size(), "ERRC%i", (int)error_code);
    dwarf::ModbusControl::TriggerMarlinKillFault(dwarf_shared::errors::FaultStatusMask::MARLIN_KILLED, "", err.data());
    stop_marlin();
}

#include "SPI.h"

#define TMC_WRITE 0x80;

#define writeCSN_H  HAL_GPIO_WritePin(STEPPER_CSN_GPIO_Port, STEPPER_CSN_Pin, GPIO_PIN_SET)
#define writeSCK_L  HAL_GPIO_WritePin(STEPPER_SCK_GPIO_Port, STEPPER_SCK_Pin, GPIO_PIN_RESET)
#define writeSCK_H  HAL_GPIO_WritePin(STEPPER_SCK_GPIO_Port, STEPPER_SCK_Pin, GPIO_PIN_SET)
#define writeMOSI_L HAL_GPIO_WritePin(STEPPER_MOSI_GPIO_Port, STEPPER_MOSI_Pin, GPIO_PIN_RESET)
#define writeMOSI_H HAL_GPIO_WritePin(STEPPER_MOSI_GPIO_Port, STEPPER_MOSI_Pin, GPIO_PIN_SET)
#define readMISO    HAL_GPIO_ReadPin(STEPPER_MISO_GPIO_Port, STEPPER_MISO_Pin)

SPISettings::SPISettings() {}
SPISettings::SPISettings([[maybe_unused]] uint32_t clock, [[maybe_unused]] BitOrder bitOrder, [[maybe_unused]] uint8_t dataMode) {}
SPIClass::SPIClass() {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // MISO
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = STEPPER_MISO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(STEPPER_MISO_GPIO_Port, &GPIO_InitStruct);

    // MOSI
    GPIO_InitStruct.Pin = STEPPER_MOSI_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STEPPER_MOSI_GPIO_Port, &GPIO_InitStruct);

    // SCK CSN
    GPIO_InitStruct.Pin = STEPPER_SCK_Pin | STEPPER_CSN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STEPPER_SCK_GPIO_Port, &GPIO_InitStruct);
    writeCSN_H;
    writeSCK_H;
}
void SPIClass::begin([[maybe_unused]] uint8_t _pin) {}
void SPIClass::end(void) {}
void SPIClass::beginTransaction([[maybe_unused]] SPISettings settings) {}
void SPIClass::endTransaction(void) {}

byte SPIClass::transfer(uint8_t _data, [[maybe_unused]] SPITransferMode _mode) {
    uint8_t value = 0;
    writeSCK_L;

    for (uint8_t i = 7; i >= 1; i--) {
        // Write bit
        !!(_data & (1 << i)) ? writeMOSI_H : writeMOSI_L;
        // Start clock pulse
        writeSCK_H;
        // Read bit
        value |= (readMISO ? 1 : 0) << i;
        // Stop clock pulse
        writeSCK_L;
    }

    !!(_data & (1 << 0)) ? writeMOSI_H : writeMOSI_L;
    writeSCK_H;
    value |= (readMISO ? 1 : 0) << 0;

    return value;
}

uint16_t SPIClass::transfer16(uint16_t _data, [[maybe_unused]] SPITransferMode _mode) {
    uint16_t returnVal = 0x0000;
    returnVal |= transfer((_data >> 8) & 0xFF) << 8;
    returnVal |= transfer(_data & 0xFF) & 0xFF;
    return returnVal;
}

SPIClass SPI;

#include "HardwareSerial.h"

HardwareSerial::HardwareSerial(void *) {
}

size_t HardwareSerial::write(uint8_t ch) {
    static uint8_t line_buffer_len = 0;
    static char line_buffer[128] = { 0 };

    if (line_buffer_len < sizeof(line_buffer)) {
        line_buffer[line_buffer_len++] = ch;
    }

    if (line_buffer[line_buffer_len - 1] == '\n' || line_buffer[line_buffer_len - 1] == '\r' || line_buffer_len == sizeof(line_buffer)) {
        line_buffer[line_buffer_len - 1] = 0;
        log_debug(Marlin, "%s", line_buffer);
        line_buffer_len = 0;
    }
    return 1;
}

size_t HardwareSerial::write(unsigned char const *buffer, unsigned int length) {
    for (unsigned int i = 0; i < length; i++) {
        write(buffer[i]);
    }
    return length;
}

void HardwareSerial::begin([[maybe_unused]] unsigned long baud) {
}

int HardwareSerial::read() {
    return 0;
}

int HardwareSerial::available() {
    return 0;
}

void HardwareSerial::flush() {
}

void HardwareSerial::close() {
}

int HardwareSerial::peek() {
    return 0;
}

HardwareSerial::operator bool() {
    return false;
}

HardwareSerial SerialUART3(nullptr);
HardwareSerial Serial3(nullptr);

#include "metric.h"

void metric_record_custom_at_time(metric_t *, uint32_t, const char *, ...) {}
void metric_record_integer(metric_t *, int) {}
void metric_record_integer_at_time(metric_t *, uint32_t, int) {}

// Marlin's HAL

unsigned HAL_RCC_CSR = 0;

#include "Marlin/src/gcode/queue.h"

void GCodeQueue::get_available_commands() {
}

uint8_t GCodeQueue::length = 0;

#include "Marlin/src/lcd/extensible_ui/ui_api.h"

void ExtUI::onIdle() {}

#include "Marlin/src/feature/pause.h"

uint8_t did_pause_print;

// configuration_store

StrongIndexArray<fil_change_settings_t, EXTRUDERS, VirtualToolIndex, VirtualToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> fc_settings;
