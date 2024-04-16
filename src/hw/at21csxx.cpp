/*
 * at21csxx.cpp
 *
 *  Created on: Mar 31, 2021
 *      Author: tadeas
 *
 * @file
 */

#include "at21csxx.hpp"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <wdt.hpp>
#include "rtos_api.hpp"
#include "timing_precise.hpp"

/*
 * This code is time critical and does not work with optimizations turned off.
 * Force at least -Og optimization level.
 */
#pragma GCC push_options
#ifdef _DEBUG
    #pragma GCC optimize("Og")
#else
    #pragma GCC optimize("Os")
#endif

void AT21CSxx::bus_low() {
    HAL_GPIO_WritePin(bus_port, bus_pin, GPIO_PIN_RESET);
}

void AT21CSxx::bus_high() {
    HAL_GPIO_WritePin(bus_port, bus_pin, GPIO_PIN_SET);
}

bool AT21CSxx::bus_state() {
    return HAL_GPIO_ReadPin(bus_port, bus_pin) == GPIO_PIN_SET;
}

void AT21CSxx::start_con() {
    bus_high();
    delay_us_precise(500);
}

void AT21CSxx::stop_con() {
    start_con();
}

void AT21CSxx::logic_1() {
    bus_low();
    delay_ns_precise<t_LOW1_ns>(); // keep bus low for t_LOW1
    bus_high();
    delay_ns_precise<t_BIT_ns - t_LOW1_ns>(); // Wait until bit frame end
}

void AT21CSxx::logic_0() {
    bus_low();
    delay_ns_precise<t_LOW0_ns>(); // keep bus low for t_LOW0
    bus_high();
    delay_ns_precise<t_BIT_ns - t_LOW0_ns>(); // Wait until bit frame end
}

bool AT21CSxx::read_bit() {
    // hold bus low for a t_RD
    bus_low();
    delay_ns_precise<t_RD_ns>(); // keep bus low for t_RD

    // stop holding the bus
    bus_high();

    // now we need to sample inside sampling window
    // we need to wait at least t_PUP, but no longer than t_MRS
    // t_MRS = t_RD + t_PUP + sampling window
    // t_MRS = 2 us for high speed, so we need to wait less than 1us
    delay_ns_precise<800>(); // waiting 0,8us seems OK
    bool state = bus_state();

    // now we need to wait t_BIT
    delay_ns_precise<t_BIT_ns - t_RD_ns - 800>();
    return state;
}

uint8_t AT21CSxx::read_byte() {
    uint8_t res = 0;
    for (int i = 7; i >= 0; i--) {
        uint8_t bit = read_bit();
        res |= (bit << i);
    }
    return res;
}

/**
 * Return true if write successful
 */
bool AT21CSxx::write_byte(uint8_t byte) {
    for (uint8_t mask = 0b1000'0000; mask > 0; mask = (mask >> 1)) {
        if (byte & mask) {
            logic_1();
        } else {
            logic_0();
        }
    }
    return read_ack();
}

uint8_t AT21CSxx::compose_device_address_byte(uint8_t opcode, uint8_t address, uint8_t read) {
    uint8_t address_byte = opcode;
    address_byte = address_byte | (address << 1);
    address_byte = address_byte | (read);
    return address_byte;
}

int AT21CSxx::reset_discovery() {
    bus_low(); // reset device
    delay_us_precise(150); // t_DSCHG
    bus_high();
    delay_us_precise(100); // t_RRT

    bus_low(); // request ack
    delay_us_precise(1); // t_DDR
    bus_high();
    delay_us_precise(3); // t_MSDR-t_DDR
    bool ack = !bus_state(); // check ACK from device
    delay_us_precise(150); // t_HTTS
    return ack; // device is now ready to receive commands
}

AT21CSxx::AT21CSxx(GPIO_TypeDef *port, uint32_t pin)
    : bus_port(port)
    , bus_pin(pin) {

    // configure to open drain
    // we can read it normally when bus is high
    // so no reconfiguration is needed
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = bus_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_WritePin(bus_port, bus_pin, GPIO_PIN_SET); // OD - pullup
    HAL_GPIO_Init(bus_port, &GPIO_InitStruct);

    reset_discovery();
    dev_addr = 0; // find_device();
}

AT21CSxx::~AT21CSxx() {
    bus_low(); // reset pin to output mode so it can be used by other stuff once vacated
}

int AT21CSxx::hs_mode(int addr) {
    uint8_t foo = compose_device_address_byte(CODE_HIGH_SPEED, addr, 0);
    return write_byte(foo);
}

int AT21CSxx::load_address(uint8_t data_addr) {
    start_con();
    if (data_addr > 128) { // Check address is in range
        return -1;
    }
    uint8_t device_addr_byte = compose_device_address_byte(CODE_EEPROM_ACCESS, dev_addr, 0);
    if (!write_byte(device_addr_byte)) { // Address device, return if device didn't ack
        return -2;
    }
    if (!write_byte(data_addr)) { // Select write address in device. Return if device didn't ack.
        return -3;
    }
    return 1;
}

/**
 * Find device on bus
 *
 * @return -1 if no device found, device address otherwise(0-7)
 */
int AT21CSxx::find_device() {
    if (!reset_discovery()) { // make sure there is a device on bus
        return -1;
    }
    for (int i = 0; i < 8; i++) { // loop through all addresses
        uint8_t addr = compose_device_address_byte(CODE_READ_ID, i, 1);

        if (write_byte(addr)) { // return current address if device acknowledged read attempt
            return i;
        }
        reset_discovery(); // Reset device before going to nex attempt
    }
    return -2; // This should be never reached
}

/**
 * Read byte from EEPROM
 * This function return int to allow for error states to be returned.
 * Should be cast to uint8_t for further procesing.
 *
 * @param data_addr Address to read(0-127)
 * @return negative number if error occurred, data on address otherwise
 */
int AT21CSxx::read(uint8_t data_addr) {
    int data;
    {
        CriticalSection cs;
        int res = load_address(data_addr); // Load data address into Address Pointer.
        if (res < 0) {
            return res - 5;
        }
        stop_con();
        uint8_t device_addr_byte = compose_device_address_byte(CODE_EEPROM_ACCESS, dev_addr, 1);
        if (!write_byte(device_addr_byte)) { // Address device, return if device didn't ack
            return -5;
        }
        data = (int)read_byte(); // Ready byte from bus
        nack();
        stop_con();
    }
    osDelay(1); // Give EEPROM some extra time. Reduces errors.
    return data;
}

/**
 * Write byte
 * @param data_addr Address to write data to(0-127)
 * @param data Data to write
 *
 * @return 1 if write successful, negative number otherwise.
 */
int AT21CSxx::write(uint8_t data_addr, uint8_t data) {

    {
        CriticalSection cs;
        int res = load_address(data_addr); // Load data address into Address Pointer.
        if (res < 0) {
            return res;
        }

        if (!write_byte(data)) { // Write data. Return if device didn't ack.
            return -4;
        }
        stop_con();
    }
    osDelay(5); // Wait for data to be saved to EEPROM. t_WR = 5ms
    {
        CriticalSection cs;
        int test_data = read(data_addr);
        if (test_data != int(data)) {
            return -10;
        }
    }
    return 1;
}

/**
 * Return manufacturer device ID
 * 0x00D200 for AT21CS01
 * 0x00D380 for AT21CS11
 *
 * @return Manufacturer ID
 */
uint32_t AT21CSxx::read_mfr_ID() {
    uint32_t ID = 0;
    uint8_t device_addr_byte = compose_device_address_byte(CODE_READ_ID, dev_addr, 1);
    if (!write_byte(device_addr_byte)) { // Address device, return if device didn't ack
        return 0;
    }
    ID = ID | (read_byte() << 16);
    ack();
    ID = ID | (read_byte() << 8);
    ack();
    ID = ID | (read_byte() << 0);
    nack();
    return ID;
}

/**
 * Write multiple bytes of data
 *
 * @param data_addr Address in EEPROM (<127)
 * @param data Pointer to data buffer
 * @param len Length of data(in bytes)
 *
 * @return 1 if write successful, -1 otherwise
 */
int AT21CSxx::write_block(uint8_t data_addr, uint8_t *data, uint8_t len) {
    if (len + data_addr - 1 > 127) {
        return -1;
    }

    for (int i = 0; i < len; i++) {
        if (cyclic_write(data_addr + i, data[i], 5) != 1) {
            return -1;
        }
    }
    return 1;
}

/**
 * Read multiple bytes of data
 *
 * @param data_addr Address in EEPROM (<127)
 * @param pointer to buffer
 * @param length of data to read
 *
 */
int AT21CSxx::read_block(uint8_t data_addr, uint8_t *buffer, uint8_t len) {
    if (len + data_addr - 1 > 128) {
        return -1;
    }
    for (int i = 0; i < len; i++) {
        int res = cyclic_read(data_addr + i, cyclic_read_attempts);
        if (res < 0) {
            wdt_iwdg_refresh();
            return -1;
        }
        buffer[i] = (uint8_t)res;
    }
    wdt_iwdg_refresh();
    return 1;
}

int AT21CSxx::verified_read(uint8_t data_addr) {
    int data[3];

    data[0] = read(data_addr);
    data[1] = read(data_addr);

    if (data[0] == data[1]) {
        return data[0];
    }

    ++single_read_error_counter;

    // data mismatch, we need 3rd data to find out which one is correct
    data[2] = read(data_addr);

    if (data[1] == data[2]) {
        return data[1];
    } else if (data[2] == data[0]) {
        return data[2];
    } else {
        ++repeated_read_error_counter;
        return -1;
    }
}

int AT21CSxx::verified_write(uint8_t data_addr, uint8_t data) {
    write(data_addr, data);
    if (verified_read(data_addr) == (int)data) {
        return 1;
    }
    return -1;
}

int AT21CSxx::cyclic_read(uint8_t data_addr, uint8_t attempts) {
    for (int i = 0; i < attempts; i++) {
        int data = verified_read(data_addr);
        if (data >= 0) {
            return data;
        }
    }
    ++cyclic_read_error_counter;
    return -1;
}

int AT21CSxx::cyclic_write(uint8_t data_addr, uint8_t data, uint8_t attempts) {
    for (int i = 0; i < attempts; i++) {
        int res = verified_write(data_addr, data);
        if (res == 1) {
            return 1;
        }
    }
    return -1;
}

#pragma GCC pop_options // Restore optimizations. See beginning of file for explanation.
