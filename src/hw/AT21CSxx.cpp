/*
 * AT21CSxx.c
 *
 *  Created on: Mar 31, 2021
 *      Author: tadeas
 *
 * @file
 */
#include "config_buddy_2209_02.h"
#if (BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_EEPROM)

    #include "AT21CSxx.hpp"
    #include <string.h>
    #include "FreeRTOS.h"
    #include "task.h"
    #include "wdt.h"

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

    /* */

    /** @addtogroup Exported_macro
  * @{
  */
    #define SET_BIT(REG, BIT) ((REG) |= (BIT))

    #define CLEAR_BIT(REG, BIT) ((REG) &= ~(BIT))

    #define READ_BIT(REG, BIT) ((REG) & (BIT))

    #define CLEAR_REG(REG) ((REG) = (0x0))

    #define WRITE_REG(REG, VAL) ((REG) = (VAL))

    #define READ_REG(REG) ((REG))

    #define MODIFY_REG(REG, CLEARMASK, SETMASK) WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

    #define POSITION_VAL(VAL) (__CLZ(__RBIT(VAL)))

void AT21CSxx::LL_GPIO_SetPinMode(GPIO_TypeDef *GPIOx, uint32_t Pin, uint32_t Mode) {
    MODIFY_REG(GPIOx->MODER, (GPIO_MODER_MODER0 << (POSITION_VAL(Pin) * 2U)), (Mode << (POSITION_VAL(Pin) * 2U)));
}

void AT21CSxx::LL_GPIO_ResetOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask) {
    WRITE_REG(GPIOx->BSRR, (PinMask << 16));
}

void AT21CSxx::LL_GPIO_SetPinPull(GPIO_TypeDef *GPIOx, uint32_t Pin, uint32_t Pull) {
    MODIFY_REG(GPIOx->PUPDR, (GPIO_PUPDR_PUPDR0 << (POSITION_VAL(Pin) * 2U)), (Pull << (POSITION_VAL(Pin) * 2U)));
}

uint32_t AT21CSxx::LL_GPIO_ReadInputPort(GPIO_TypeDef *GPIOx) {
    return (uint32_t)(READ_REG(GPIOx->IDR));
}

uint32_t AT21CSxx::micros() {
    return (DWT->CYCCNT); ///micros_divider;
                          //return HAL_GetTick()*1000 + TIM6->CNT;
}

void AT21CSxx::delay(__IO uint32_t delay_tacts) {

    uint32_t start = DWT->CYCCNT;

    while ((DWT->CYCCNT - start) < delay_tacts)
        ;
}

void AT21CSxx::bus_low() {
    LL_GPIO_SetPinMode(bus_port, bus_pin, LL_GPIO_MODE_OUTPUT);
    //LL_GPIO_SetPinPull(bus_port, bus_pin, LL_GPIO_PULL_UP);
    LL_GPIO_ResetOutputPin(bus_port, bus_pin);
}

void AT21CSxx::bus_high() {
    LL_GPIO_SetPinMode(bus_port, bus_pin, LL_GPIO_MODE_INPUT);
}

int AT21CSxx::bus_state() {
    return (LL_GPIO_ReadInputPort(bus_port) & bus_pin) != 0;
}

void AT21CSxx::start_con() {
    bus_high();
    delay(84000);
}

void AT21CSxx::stop_con() {
    start_con();
}

void AT21CSxx::logic_1() {
    uint32_t start = DWT->CYCCNT;
    bus_low();
    uint32_t finish = start + 252; //keep bus low for 1.5 us = t_LOW1
    while (DWT->CYCCNT < finish)
        ;
    bus_high();
    finish = start + 3360; //Wait until bit frame end 20 us = t_BIT
    while (DWT->CYCCNT < finish)
        ;
}

void AT21CSxx::logic_0() {
    uint32_t start = DWT->CYCCNT;
    bus_low();
    uint32_t finish = start + 1680; //keep bus low for 10 us = t_LOW0
    while (DWT->CYCCNT < finish)
        ;
    bus_high();
    finish = start + 3360; //Wait until bit frame end 20 us = t_BIT
    while (DWT->CYCCNT < finish)
        ;
}

void AT21CSxx::write_bit(int bit) {
    if (bit) {
        logic_1();
    } else {
        logic_0();
    }
}

void AT21CSxx::ack() {
    write_bit(0);
}

void AT21CSxx::nack() {
    write_bit(1);
}

int AT21CSxx::read_bit() {
    uint32_t start = DWT->CYCCNT;
    bus_low();
    uint32_t finish = start + 168; //keep bus low for 1 us = t_RD
    while (DWT->CYCCNT < finish)
        ;
    bus_high();
    //finish = start + 302;	//
    //while(DWT->CYCCNT < finish);
    while (bus_state() == 0)
        ;
    uint32_t stop = DWT->CYCCNT;
    uint32_t time = stop - start;
    int state; // = bus_state();
    if (time < 336) {
        state = 1;
    } else {
        state = 0;
    }

    finish = start + 3360; //Wait until bit frame end 20 us = t_BIT
    while (DWT->CYCCNT < finish)
        ;
    return state;
}

int AT21CSxx::read_ack() {
    return !read_bit();
}

uint8_t AT21CSxx::read_byte() {
    uint8_t res = 0;
    for (int i = 7; i >= 0; i--) {
        int bit = read_bit();
        res = res | (bit << i);
    }
    return res;
}

/**
 * Return 1 if write successful
 */
int AT21CSxx::write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        write_bit((byte << i) & 128);
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
    bus_low();    //reset device
    delay(25200); //t_DSCHG
    bus_high();
    delay(16800); //t_RRT

    bus_low();  //request ack
    delay(168); //t_DDR
    bus_high();
    delay(504);                 //t_MSDR-t_DDR
    int ack = bus_state() == 0; //check ACK from device
    delay(25200);               //t_HTTS
    return ack;                 //device is now ready to receive commands
}

uint32_t AT21CSxx::DWT_Delay_Init(void) {
    /* Disable TRC */
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk; // ~0x01000000;
    /* Enable TRC */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // 0x01000000;
    /* Disable clock cycle counter */
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; //~0x00000001;
    /* Enable  clock cycle counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; //0x00000001;
    /* Reset the clock cycle counter value */
    DWT->CYCCNT = 0;
    /* 3 NO OPERATION instructions */
    __ASM volatile("NOP");
    __ASM volatile("NOP");
    __ASM volatile("NOP");
    /* Check if clock cycle counter has started */
    if (DWT->CYCCNT) {
        return 0; /*clock cycle counter started*/
    } else {
        return 1; /*clock cycle counter not started*/
    }
}

AT21CSxx::AT21CSxx(GPIO_TypeDef *port, uint32_t pin) {
    bus_pin = pin;
    bus_port = port;
    DWT_Delay_Init();
    reset_discovery();
    dev_addr = 0; //find_device();
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
    if (data_addr > 128) { //Check address is in range
        return -1;
    }
    uint8_t device_addr_byte = compose_device_address_byte(CODE_EEPROM_ACCESS, dev_addr, 0);
    if (!write_byte(device_addr_byte)) { //Address device, return if device didn't ack
        return -2;
    }
    if (!write_byte(data_addr)) { //Select write address in device. Return if device didn't ack.
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
    if (!reset_discovery()) { //make sure there is a device on bus
        return -1;
    }
    for (int i = 0; i < 8; i++) { //loop through all addresses
        uint8_t addr = compose_device_address_byte(CODE_READ_ID, i, 1);

        if (write_byte(addr)) { //return current address if device acknowledged read attempt
            return i;
        }
        reset_discovery(); //Reset device before going to nex attempt
    }
    return -2; //This should be never reached
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
    int res = load_address(data_addr); //Load data address into Address Pointer.
    if (res < 0) {
        return res - 5;
    }
    stop_con();
    uint8_t device_addr_byte = compose_device_address_byte(CODE_EEPROM_ACCESS, dev_addr, 1);
    if (!write_byte(device_addr_byte)) { //Address device, return if device didn't ack
        return -5;
    }
    int data = (int)read_byte(); //Ready byte from bus
    nack();
    stop_con();
    delay(168000);
    delay(1680000); //Give EEPROM some extra time. Reduces errors.
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

    int res = load_address(data_addr); //Load data address into Address Pointer.
    if (res < 0) {
        return res;
    }

    if (!write_byte(data)) { //Write data. Return if device didn't ack.
        return -4;
    }
    stop_con();
    delay(10122000); //Wait for data to be saved to EEPROM. t_WR = 5ms
    int test_data = (uint8_t)read(data_addr);
    if (test_data != data) {
        return -10;
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
    if (!write_byte(device_addr_byte)) { //Address device, return if device didn't ack
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
    taskENTER_CRITICAL();
    if (len + data_addr - 1 > 127) {
        taskEXIT_CRITICAL();
        return -1;
    }

    for (int i = 0; i < len; i++) {
        if (cyclic_write(data_addr + i, data[i], 5) != 1) {
            taskEXIT_CRITICAL();
            return -1;
        }
    }
    taskEXIT_CRITICAL();
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
    taskENTER_CRITICAL();
    for (int i = 0; i < len; i++) {
        int res = cyclic_read(data_addr + i, 5);
        if (res < 0) {
            wdt_iwdg_refresh();
            taskEXIT_CRITICAL();
            return -1;
        }
        buffer[i] = (uint8_t)res;
    }
    wdt_iwdg_refresh();
    taskEXIT_CRITICAL();
    return 1;
}

int AT21CSxx::verified_read(uint8_t data_addr) {
    //return read(data_addr);
    int data[3];

    for (int i = 0; i < 3; i++) {
        data[i] = read(data_addr);
    }
    if (data[0] == data[1] && data[0] == data[2]) {
        return data[0];
    } else {
        error_counter++;
        if (data[0] == data[1]) {
            return data[0];
        } else if (data[1] == data[2]) {
            return data[1];
        } else if (data[2] == data[0]) {
            return data[2];
        } else {
            return -1;
        }
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
        error_counter++;
    }
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

    #pragma GCC pop_options //Restore optimizations. See begining of file for explanation.

#endif //(BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_EEPROM && defined LOVEBOARD_HAS_PT100)
