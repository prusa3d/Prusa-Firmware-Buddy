/*
 * at21csxx.hpp
 *
 *  Created on: Mar 31, 2021
 *      Author: tadeas
 */
#pragma once

#include "hwio_pindef.h" // GPIO_TypeDef

class AT21CSxx {

public:
    AT21CSxx(GPIO_TypeDef *port, uint32_t pin);

    ~AT21CSxx();
    int read(uint8_t data_addr);
    int write(uint8_t data_addr, uint8_t data);
    uint32_t read_mfr_ID();
    int read_block(uint8_t data_addr, uint8_t *buffer, uint8_t len);
    int write_block(uint8_t data_addr, uint8_t *data, uint8_t len);
    int find_device();
    int verified_read(uint8_t data_addr);
    int verified_write(uint8_t data_addr, uint8_t data);
    int cyclic_read(uint8_t data_addr, uint8_t attempts);
    int cyclic_write(uint8_t data_addr, uint8_t data, uint8_t attempts);
    void bus_low();
    void bus_high();
    int reset_discovery();

    uint32_t get_single_read_error() const { return single_read_error_counter; }
    uint32_t get_repeated_read_error() const { return repeated_read_error_counter; }
    uint32_t get_cyclic_read_error() const { return cyclic_read_error_counter; }

private:
    uint32_t single_read_error_counter = 0;
    uint32_t repeated_read_error_counter = 0;
    uint32_t cyclic_read_error_counter = 0;
    int dev_addr = -1;
    GPIO_TypeDef *bus_port;
    uint32_t bus_pin;

    bool bus_state();

    void start_con();
    void stop_con();
    void logic_1();
    void logic_0();

    void ack() { logic_0(); }
    void nack() { logic_1(); }
    bool read_ack() { return !read_bit(); }

    bool read_bit();
    uint8_t read_byte();
    bool write_byte(uint8_t byte);
    uint8_t compose_device_address_byte(uint8_t opcode, uint8_t address, uint8_t read);

    void init(GPIO_TypeDef *port, uint32_t pin);
    int hs_mode(int addr);
    int load_address(uint8_t data_addr);

    static constexpr uint32_t t_LOW1_ns = 1'500;
    static constexpr uint32_t t_RD_ns = 1'000;
    static constexpr uint32_t t_LOW0_ns = 10'000;

    // datasheet says it must be atleast t_LOW0 + t_PUP + t_RCV
    // I think it might be wrong (since t_LOW0 is for input frame) - it should be t_HLD0 + t_PUP + t_RCV
    // but both of those t_HLD0 or t_LOW0 are unknown especially when reading "1", so I will not use this approach
    // it is ensured that 25 us is enough
    static constexpr uint32_t t_BIT_ns = 25'000;

    static constexpr uint8_t CODE_EEPROM_ACCESS = 0b1010'0000;
    static constexpr uint8_t CODE_SECURITY_REGISTER_ACCESS = 0b1011'0000;
    static constexpr uint8_t CODE_LOCK_SECURITY_REGISTER = 0b0010'0000;
    static constexpr uint8_t CODE_ZONE_REGISTER_ACCESS = 0b0111'0000;
    static constexpr uint8_t CODE_FREEZE_ROM_ZONE_REGISTER = 0b0001'0000;
    static constexpr uint8_t CODE_READ_ID = 0b1100'0000;
    static constexpr uint8_t CODE_HIGH_SPEED = 0b1110'0000;

    static constexpr uint8_t cyclic_read_attempts = 3;
};
