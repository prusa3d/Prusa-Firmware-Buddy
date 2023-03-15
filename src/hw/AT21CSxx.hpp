/*
 * AT21CSxx.h
 *
 *  Created on: Mar 31, 2021
 *      Author: tadeas
 */

#include "printers.h"
#include <device/board.h>

#ifndef INC_AT21CSXX_H_
    #define INC_AT21CSXX_H_

    #if (BOARD_IS_XBUDDY && BOARD_VER_EQUAL_TO(0, 1, 8))
        #include "stm32f407xx.h"
    #elif (BOARD_IS_XBUDDY && BOARD_VER_HIGHER_OR_EQUAL_TO(0, 2, 0))
        #include "stm32f427xx.h"
    #else
        #error PT100 not supported on Buddy board
    #endif
    #include "stm32f4xx_hal.h"

class AT21CSxx {

public:
    AT21CSxx(GPIO_TypeDef *port, uint32_t pin);

    virtual ~AT21CSxx();
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
    int error_counter = 0;
    void bus_low();
    void bus_high();
    int reset_discovery();

private:
    int dev_addr = -1;
    GPIO_TypeDef *bus_port;
    uint32_t bus_pin;

    __STATIC_INLINE void LL_GPIO_SetPinMode(GPIO_TypeDef *GPIOx, uint32_t Pin, uint32_t Mode);
    __STATIC_INLINE void LL_GPIO_ResetOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask);
    __STATIC_INLINE uint32_t LL_GPIO_ReadInputPort(GPIO_TypeDef *GPIOx);

    __STATIC_INLINE void LL_GPIO_SetPinPull(GPIO_TypeDef *GPIOx, uint32_t Pin, uint32_t Pull);
    __STATIC_INLINE uint32_t micros();
    __STATIC_INLINE void delay(__IO uint32_t delay_tacts);

    int bus_state();
    uint32_t DWT_Delay_Init(void);

    void start_con();
    void stop_con();
    void logic_1();
    void logic_0();
    void write_bit(int bit);

    void ack();
    void nack();
    int read_ack();

    int read_bit();
    uint8_t read_byte();
    int write_byte(uint8_t byte);
    uint8_t compose_device_address_byte(uint8_t opcode, uint8_t address, uint8_t read);

    void init(GPIO_TypeDef *port, uint32_t pin);
    int hs_mode(int addr);
    int load_address(uint8_t data_addr);

    #define CODE_EEPROM_ACCESS            0b10100000
    #define CODE_SECURITY_REGISTER_ACCESS 0b10110000
    #define CODE_LOCK_SECURITY_REGISTER   0b00100000
    #define CODE_ZONE_REGISTER_ACCESS     0b01110000
    #define CODE_FREEZE_ROM_ZONE_REGISTER 0b00010000
    #define CODE_READ_ID                  0b11000000
    #define CODE_HIGH_SPEED               0b11100000

    /** @defgroup GPIO_LL_EC_PIN PIN
  * @{
  */
    #define LL_GPIO_PIN_0   GPIO_BSRR_BS_0                                                                                                                                                                                                                                                                        /*!< Select pin 0 */
    #define LL_GPIO_PIN_1   GPIO_BSRR_BS_1                                                                                                                                                                                                                                                                        /*!< Select pin 1 */
    #define LL_GPIO_PIN_2   GPIO_BSRR_BS_2                                                                                                                                                                                                                                                                        /*!< Select pin 2 */
    #define LL_GPIO_PIN_3   GPIO_BSRR_BS_3                                                                                                                                                                                                                                                                        /*!< Select pin 3 */
    #define LL_GPIO_PIN_4   GPIO_BSRR_BS_4                                                                                                                                                                                                                                                                        /*!< Select pin 4 */
    #define LL_GPIO_PIN_5   GPIO_BSRR_BS_5                                                                                                                                                                                                                                                                        /*!< Select pin 5 */
    #define LL_GPIO_PIN_6   GPIO_BSRR_BS_6                                                                                                                                                                                                                                                                        /*!< Select pin 6 */
    #define LL_GPIO_PIN_7   GPIO_BSRR_BS_7                                                                                                                                                                                                                                                                        /*!< Select pin 7 */
    #define LL_GPIO_PIN_8   GPIO_BSRR_BS_8                                                                                                                                                                                                                                                                        /*!< Select pin 8 */
    #define LL_GPIO_PIN_9   GPIO_BSRR_BS_9                                                                                                                                                                                                                                                                        /*!< Select pin 9 */
    #define LL_GPIO_PIN_10  GPIO_BSRR_BS_10                                                                                                                                                                                                                                                                       /*!< Select pin 10 */
    #define LL_GPIO_PIN_11  GPIO_BSRR_BS_11                                                                                                                                                                                                                                                                       /*!< Select pin 11 */
    #define LL_GPIO_PIN_12  GPIO_BSRR_BS_12                                                                                                                                                                                                                                                                       /*!< Select pin 12 */
    #define LL_GPIO_PIN_13  GPIO_BSRR_BS_13                                                                                                                                                                                                                                                                       /*!< Select pin 13 */
    #define LL_GPIO_PIN_14  GPIO_BSRR_BS_14                                                                                                                                                                                                                                                                       /*!< Select pin 14 */
    #define LL_GPIO_PIN_15  GPIO_BSRR_BS_15                                                                                                                                                                                                                                                                       /*!< Select pin 15 */
    #define LL_GPIO_PIN_ALL (GPIO_BSRR_BS_0 | GPIO_BSRR_BS_1 | GPIO_BSRR_BS_2 | GPIO_BSRR_BS_3 | GPIO_BSRR_BS_4 | GPIO_BSRR_BS_5 | GPIO_BSRR_BS_6 | GPIO_BSRR_BS_7 | GPIO_BSRR_BS_8 | GPIO_BSRR_BS_9 | GPIO_BSRR_BS_10 | GPIO_BSRR_BS_11 | GPIO_BSRR_BS_12 | GPIO_BSRR_BS_13 | GPIO_BSRR_BS_14 | GPIO_BSRR_BS_15) /*!< Select all pins */
    /**
  * @}
  */

    /** @defgroup GPIO_LL_EC_MODE Mode
  * @{
  */
    #define LL_GPIO_MODE_INPUT     (0x00000000U)       /*!< Select input mode */
    #define LL_GPIO_MODE_OUTPUT    GPIO_MODER_MODER0_0 /*!< Select output mode */
    #define LL_GPIO_MODE_ALTERNATE GPIO_MODER_MODER0_1 /*!< Select alternate function mode */
    #define LL_GPIO_MODE_ANALOG    GPIO_MODER_MODER0   /*!< Select analog mode */
};
#endif /* INC_AT21CSXX_H_ */

/** @defgroup GPIO_LL_EC_PULL Pull Up Pull Down
  * @{
  */
#define LL_GPIO_PULL_NO   (0x00000000U)       /*!< Select I/O no pull */
#define LL_GPIO_PULL_UP   GPIO_PUPDR_PUPDR0_0 /*!< Select I/O pull up */
#define LL_GPIO_PULL_DOWN GPIO_PUPDR_PUPDR0_1 /*!< Select I/O pull down */
