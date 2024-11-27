#include <device/board.h>
#include <device/peripherals.h>
#include <buddy/phase_stepping_opts.h>
#include <atomic>
#include "Pin.hpp"
#include "hwio_pindef.h"
#include "safe_state.h"
#include <buddy/main.h>
#include "adc.hpp"
#include "timer_defaults.h"
#include "PCA9557.hpp"
#include "TCA6408A.hpp"
#include <logging/log.hpp>
#include "timing_precise.hpp"
#include <option/has_puppies.h>
#include <option/has_burst_stepping.h>
#include <option/has_i2c_expander.h>
#include <printers.h>

// breakpoint
#include "FreeRTOS.h"
#include "task.h"

#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    #include "hw_configuration.hpp"
#endif

//
// I2C
//

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
I2C_HandleTypeDef hi2c3;

//
// SPI
//

SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;
SPI_HandleTypeDef hspi4;
SPI_HandleTypeDef hspi5;
SPI_HandleTypeDef hspi6;

//
// ADCs
//

ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
ADC_HandleTypeDef hadc3;

//
// Timers
//

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim8;
TIM_HandleTypeDef htim13;
TIM_HandleTypeDef htim14;

//
// Other
//

RTC_HandleTypeDef hrtc;
RNG_HandleTypeDef hrng;

namespace buddy::hw {
#if HAS_I2C_EXPANDER() // HAS_I2C_EXPANDER corresponds to FDM-MK4-GPIO, not io_expander1 which connects DWARFs
TCA6408A io_expander2(I2C_HANDLE_FOR(io_expander2));
#endif // HAS_I2C_EXPANDER()
#if BOARD_IS_XLBUDDY()
PCA9557 io_expander1(I2C_HANDLE_FOR(io_expander1), 0x1);
#endif // BOARD_IS_XLBUDDY()
} // namespace buddy::hw

//
// Initialization
//

#if PRINTER_IS_PRUSA_iX()
// called at earliest possible time after system/core inits to set turbine PWM pin (heatbed PWM pin) high and disable it
void hw_preinit_turbine_disable() {
    uint32_t offset = 0;
    uint32_t pin = BED_HEAT_Pin;
    while (!(pin & 0x1)) {
        pin >>= 1;
        offset++;
    }
    uint32_t temp = BED_HEAT_GPIO_Port->MODER;
    temp &= ~(GPIO_MODER_MODER0 << (offset * 2U));
    temp |= ((GPIO_MODE_OUTPUT_PP) << (offset * 2U));
    BED_HEAT_GPIO_Port->MODER = temp;
    temp = BED_HEAT_GPIO_Port->ODR;
    temp |= BED_HEAT_Pin;
    BED_HEAT_GPIO_Port->ODR = temp;
}
#endif

void hw_rtc_init() {
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Error_Handler();
    }
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_B);
    HAL_RTCEx_DeactivateCalibrationOutPut(&hrtc);
    HAL_RTCEx_DeactivateTamper(&hrtc, RTC_TAFCR_TAMP1E);
    HAL_RTCEx_DeactivateTimeStamp(&hrtc);
}

void hw_rng_init() {
    hrng.Instance = RNG;
    if (HAL_RNG_Init(&hrng) != HAL_OK) {
        Error_Handler();
    }
}

void hw_gpio_init() {
    GPIO_InitTypeDef GPIO_InitStruct {};

    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    // Configure GPIO pins : USB_OVERC_Pin ESP_GPIO0_Pin BED_MON_Pin WP1_Pin
    GPIO_InitStruct.Pin = USB_OVERC_Pin | ESP_GPIO0_Pin | BED_MON_Pin | WP1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    // NOTE: Configuring GPIO causes a short drop of pin output to low. This is
    //       avoided by first setting the pin and then initilizing the GPIO. In case
    //       this does not work we first initilize ESP GPIO0 to avoid reset low
    //       followed by ESP GPIO low as this sequence can switch esp to boot mode */

    // Configure ESP GPIO0 (PROG, High for ESP module boot from Flash)
    GPIO_InitStruct.Pin =
#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
        GPIO_PIN_15
#else
        GPIO_PIN_6
#endif
        ;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_WritePin(GPIOE,
#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
        GPIO_PIN_15
#else
        GPIO_PIN_6
#endif
        ,
        GPIO_PIN_SET);
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    // Configure GPIO pins : ESP_RST_Pin
    GPIO_InitStruct.Pin = ESP_RST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // Configure GPIO pins : WP2_Pin
    GPIO_InitStruct.Pin = WP2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    PIN_TABLE(CONFIGURE_PINS);
#if defined(EXTENDER_PIN_TABLE)
    EXTENDER_PIN_TABLE(CONFIGURE_PINS);
#endif

    buddy::hw::hwio_configure_board_revision_changed_pins();
}

void hw_dma_init() {
    // DMA controller clock enable
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

#if (!PRINTER_IS_PRUSA_MINI())
    // DMA1_Stream3_IRQn interrupt configuration
    #if PRINTER_IS_PRUSA_XL()
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, ISR_PRIORITY_PUPPIES_USART, 0);
    #else
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, ISR_PRIORITY_DEFAULT, 0);
    #endif
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
#endif

#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    // DMA1_Stream0_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
    // DMA1_Stream2_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
    // DMA1_Stream6_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    // DMA2_Stream3_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
    // DMA2_Stream4_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
    // DMA2_Stream5_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
    // DMA2_Stream7_IRQn interrupt configuration
    #if PRINTER_IS_PRUSA_iX() || PRINTER_IS_PRUSA_COREONE()
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, ISR_PRIORITY_PUPPIES_USART, 0);
    #else
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, ISR_PRIORITY_DEFAULT, 0);
    #endif
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
#endif
    // DMA1_Stream0_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

#if BOARD_IS_XLBUDDY()
    // DMA1_Stream1_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, ISR_PRIORITY_PUPPIES_USART, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

    // DMA2_Stream0_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
#endif

    // DMA1_Stream4_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
    // DMA1_Stream5_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    // DMA1_Stream7_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream7_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream7_IRQn);
    // DMA2_Stream1_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
// DMA2_Stream2_IRQn interrupt configuration
#if (PRINTER_IS_PRUSA_iX() || PRINTER_IS_PRUSA_COREONE())
    HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, ISR_PRIORITY_PUPPIES_USART, 0);
#else
    HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, ISR_PRIORITY_DEFAULT, 0);
#endif
    HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
    // DMA2_Stream2_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
    // DMA2_Stream6_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
}

void static config_adc(ADC_HandleTypeDef *hadc, ADC_TypeDef *ADC_NUM, uint32_t NbrOfConversion) {
    hadc->Instance = ADC_NUM;
    hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV8;
    hadc->Init.Resolution = ADC_RESOLUTION_12B;
    hadc->Init.ScanConvMode = ENABLE;
    hadc->Init.ContinuousConvMode = ENABLE;
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc->Init.NbrOfConversion = NbrOfConversion;
    hadc->Init.DMAContinuousRequests = ENABLE;
    hadc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(hadc) != HAL_OK) {
        Error_Handler();
    }
}

static void config_adc_ch(ADC_HandleTypeDef *hadc, uint32_t Channel, uint32_t Rank) {
    Rank++; // Channel rank starts at 1, but for array indexing, we need to start from 0.
    ADC_ChannelConfTypeDef sConfig = { Channel, Rank, ADC_SAMPLETIME_480CYCLES, 0 };
    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

void hw_adc1_init() {
    // Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    config_adc(&hadc1, ADC1, AdcChannel::ADC1_CH_CNT);

    // Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
#if (BOARD_IS_BUDDY())
    config_adc_ch(&hadc1, ADC_CHANNEL_10, AdcChannel::hotend_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_4, AdcChannel::heatbed_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_5, AdcChannel::board_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_6, AdcChannel::pinda_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_3, AdcChannel::heatbed_U);
    config_adc_ch(&hadc1, ADC_CHANNEL_TEMPSENSOR, AdcChannel::mcu_temperature);
    config_adc_ch(&hadc1, ADC_CHANNEL_VREFINT, AdcChannel::vref);
#elif (BOARD_IS_XBUDDY() && PRINTER_IS_PRUSA_MK3_5())
    config_adc_ch(&hadc1, ADC_CHANNEL_10, AdcChannel::hotend_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_4, AdcChannel::heatbed_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_5, AdcChannel::heatbed_U);
    config_adc_ch(&hadc1, ADC_CHANNEL_3, AdcChannel::hotend_U);
    config_adc_ch(&hadc1, ADC_CHANNEL_VREFINT, AdcChannel::vref);
    config_adc_ch(&hadc1, ADC_CHANNEL_TEMPSENSOR, AdcChannel::mcu_temperature);
#elif (BOARD_IS_XBUDDY())
    config_adc_ch(&hadc1, ADC_CHANNEL_10, AdcChannel::hotend_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_4, AdcChannel::heatbed_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_5, AdcChannel::heatbed_U);
    config_adc_ch(&hadc1, ADC_CHANNEL_6, AdcChannel::heatbreak_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_3, AdcChannel::hotend_U);
    config_adc_ch(&hadc1, ADC_CHANNEL_VREFINT, AdcChannel::vref);
    config_adc_ch(&hadc1, ADC_CHANNEL_TEMPSENSOR, AdcChannel::mcu_temperature);
#elif BOARD_IS_XLBUDDY()
    config_adc_ch(&hadc1, ADC_CHANNEL_4, AdcChannel::dwarf_I);
    config_adc_ch(&hadc1, ADC_CHANNEL_5, AdcChannel::mux1_y);
    config_adc_ch(&hadc1, ADC_CHANNEL_8, AdcChannel::mux1_x);
    config_adc_ch(&hadc1, ADC_CHANNEL_VREFINT, AdcChannel::vref);
    config_adc_ch(&hadc1, ADC_CHANNEL_TEMPSENSOR, AdcChannel::mcu_temperature);
#else
    #error Unknown board
#endif

    // Disable ADC DMA IRQ by default. This is enabled on-demand by AdcMultiplexer
    HAL_NVIC_DisableIRQ(DMA2_Stream4_IRQn);
}

#ifdef HAS_ADC3
void hw_adc3_init() {
    // Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    config_adc(&hadc3, ADC3, AdcChannel::ADC3_CH_CNT);

    // Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    #if BOARD_IS_XBUDDY()
    config_adc_ch(&hadc3, ADC_CHANNEL_4, AdcChannel::MMU_I);
    config_adc_ch(&hadc3, ADC_CHANNEL_8, AdcChannel::board_T);
    config_adc_ch(&hadc3, ADC_CHANNEL_9, AdcChannel::hotend_I);
    config_adc_ch(&hadc3, ADC_CHANNEL_14, AdcChannel::board_I);
        #if PRINTER_IS_PRUSA_iX()
    config_adc_ch(&hadc3, ADC_CHANNEL_15, AdcChannel::case_T);
        #elif PRINTER_IS_PRUSA_COREONE()
    config_adc_ch(&hadc3, ADC_CHANNEL_15, AdcChannel::door_sensor);
        #endif
    #elif BOARD_IS_XLBUDDY()
    config_adc_ch(&hadc3, ADC_CHANNEL_8, AdcChannel::board_T);
    config_adc_ch(&hadc3, ADC_CHANNEL_4, AdcChannel::mux2_y);
    config_adc_ch(&hadc3, ADC_CHANNEL_10, AdcChannel::mux2_x);

    #else
        #error Unknown board
    #endif

    // Disable ADC DMA IRQ by default. This is enabled on-demand by AdcMultiplexer
    HAL_NVIC_DisableIRQ(DMA2_Stream0_IRQn);
}
#endif

struct hw_pin {
    GPIO_TypeDef *port;
    uint16_t no;
};

/**
 * @brief Set the pin to open-drain
 */
static void set_pin_od(hw_pin pin) {
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = pin.no;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(pin.port, &GPIO_InitStruct);
}

/**
 * @brief Set the pin to input
 */
static void set_pin_in(hw_pin pin) {
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = pin.no;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(pin.port, &GPIO_InitStruct);
}

/**
 * @brief calculate edge timing
 * edges timing is of half period
 * round up
 *
 * @param clk frequency [Hz]
 * @return constexpr uint32_t half of period [us]
 */
static constexpr uint32_t i2c_get_edge_us(uint32_t clk) {
    // clk + 1 .. round up
    // / 2     .. need half of period
    return (1'000'000 % clk ? (1'000'000 / clk + 1) : (1'000'000 / clk)) / 2;
}

/**
 * @brief unblock i2c data pin
 * apply up to 32 clock pulses and check SDA logical level
 * make sure it is in '1', so master can manipulate with it
 *
 * @param clk   frequency [Hz]
 * @param sda   pin of data
 * @param scl   pin of clock
 */
static void i2c_unblock_sda(uint32_t clk, hw_pin sda, hw_pin scl) {
    delay_us_precise(i2c_get_edge_us(clk)); // half period - ensure first edge is not too short

    // ORIGINAL COMMENT (i < 9): 9 pulses, there is no point to try it more times - 9th bit is ACK (will be NACK)
    // Changed to an arbitrary higher value, because comm with the touchscreen controller is extra sketchy, clock gets lost sometimes and such
    // Cannot be used on multi-master buses
    for (size_t i = 0; i < 32; ++i) {
        HAL_GPIO_WritePin(scl.port, scl.no, GPIO_PIN_SET); // set clock to '1'
        delay_us_precise(i2c_get_edge_us(clk)); // wait half period
        if (HAL_GPIO_ReadPin(sda.port, sda.no) == GPIO_PIN_SET) { // check if slave does not pull SDA to '0' while SCL == 1
            return; // sda is not pulled by a slave, it is done
        }

        HAL_GPIO_WritePin(scl.port, scl.no, GPIO_PIN_RESET); // set clock to '0'
        delay_us_precise(i2c_get_edge_us(clk)); // wait half period
    }

// in case code reaches this, there is some HW issue
// but we cannot log it or rise red screen, it is too early
#ifdef _DEBUG
    buddy_disable_heaters();
    __BKPT(0);
#endif
    HAL_GPIO_WritePin(scl.port, scl.no, GPIO_PIN_SET); // this code should never be reached, just in case it was set clock to '1'
}

/**
 * @brief free I2C in case of slave deadlock
 * in case printer is resetted during I2C transmit, slave can deadlock
 * it has not been resetted and is expecting clock to finish its command
 * problem is that it can hold SDA in '0' - it blocks the bus so master cannot do start / stop condition
 * this code generates a clock until SDA is in '1' and than master sdoes start + stop to end any slave communication
 *
 * @param clk   frequency [Hz]
 * @param sda   pin of data
 * @param scl   pin of clock
 */
static void i2c_free_bus_in_case_of_slave_deadlock(uint32_t clk, hw_pin sda, hw_pin scl) {
    set_pin_in(sda); // configure SDA to input
    if (HAL_GPIO_ReadPin(sda.port, sda.no) == GPIO_PIN_RESET) { // check if slave pulls SDA to '0' while SCL == 1
        set_pin_od(scl); // configure SCL to open-drain
        i2c_unblock_sda(clk, sda, scl); // get SDA pin in state pin can be "moved"
    }

    set_pin_od(sda); // reconfigure SDA to open-drain, to be able to move it
    HAL_GPIO_WritePin(sda.port, sda.no, GPIO_PIN_RESET); // set SDA to '0' while SCL == '1' - start condition
    delay_us_precise(i2c_get_edge_us(clk)); // wait half period
    HAL_GPIO_WritePin(sda.port, sda.no, GPIO_PIN_RESET); // set SDA to '1' while SCL == '1' - stop condition
    delay_us_precise(i2c_get_edge_us(clk)); // wait half period
}

#if HAS_I2CN(1)

static constexpr uint32_t i2c1_speed = 400'000;

void hw_i2c1_pins_init() {
    GPIO_InitTypeDef GPIO_InitStruct {};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    i2c_free_bus_in_case_of_slave_deadlock(i2c1_speed, { i2c1_SDA_PORT, i2c1_SDA_PIN }, { i2c1_SCL_PORT, i2c1_SCL_PIN });

    // GPIO I2C mode
    GPIO_InitStruct.Pin = i2c1_SDA_PIN | i2c1_SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(i2c1_SDA_PORT, &GPIO_InitStruct);

    // Peripheral clock enable
    __HAL_RCC_I2C1_CLK_ENABLE();
}

void hw_i2c1_init() {
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = i2c1_speed;

    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }

    #if defined(I2C_FLTR_ANOFF) && defined(I2C_FLTR_DNF)
    // Configure Analog filter
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    // Configure Digital filter
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
        Error_Handler();
    }
    #endif
}
#endif // HAS_I2CN(1)

#if HAS_I2CN(2)

// speed must be 400k, to speedup PersistentStorage erase (mk3.9/4 switch)
static constexpr uint32_t i2c2_speed = 400'000;

void hw_i2c2_pins_init() {
    GPIO_InitTypeDef GPIO_InitStruct {};

    __HAL_RCC_GPIOF_CLK_ENABLE();

    i2c_free_bus_in_case_of_slave_deadlock(i2c2_speed, { i2c2_SDA_PORT, i2c2_SDA_PIN }, { i2c2_SCL_PORT, i2c2_SCL_PIN });

    // GPIO I2C mode
    GPIO_InitStruct.Pin = i2c2_SDA_PIN | i2c2_SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(i2c2_SDA_PORT, &GPIO_InitStruct);

    // Peripheral clock enable
    __HAL_RCC_I2C2_CLK_ENABLE();
}

void hw_i2c2_init() {
    hi2c2.Instance = I2C2;
    hi2c2.Init.ClockSpeed = i2c2_speed;

    hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c2.Init.OwnAddress1 = 0;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c2) != HAL_OK) {
        Error_Handler();
    }

    #if defined(I2C_FLTR_ANOFF) && defined(I2C_FLTR_DNF)
    // Configure Analog filter
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    // Configure Digital filter to maximum (tHD:STA 0.357us delay)
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0x0F) != HAL_OK) {
        Error_Handler();
    }
    #endif
}
#endif // HAS_I2CN(2)

#if HAS_I2CN(3)

static constexpr uint32_t i2c3_speed = 100'000;

void hw_i2c3_pins_init() {

    GPIO_InitTypeDef GPIO_InitStruct {};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    i2c_free_bus_in_case_of_slave_deadlock(i2c3_speed, { i2c3_SDA_PORT, i2c3_SDA_PIN }, { i2c3_SCL_PORT, i2c3_SCL_PIN });

    // GPIO I2C mode
    GPIO_InitStruct.Pin = i2c3_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(i2c3_SDA_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = i2c3_SCL_PIN;
    HAL_GPIO_Init(i2c3_SCL_PORT, &GPIO_InitStruct);

    // Peripheral clock enable
    __HAL_RCC_I2C3_CLK_ENABLE();
}

void hw_i2c3_init() {
    hi2c3.Instance = I2C3;
    hi2c3.Init.ClockSpeed = i2c3_speed;
    hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c3.Init.OwnAddress1 = 0;
    hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c3.Init.OwnAddress2 = 0;
    hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c3) != HAL_OK) {
        Error_Handler();
    }

    #if defined(I2C_FLTR_ANOFF) && defined(I2C_FLTR_DNF)
    // Configure Analogue filter
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    // Configure Digital filter
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK) {
        Error_Handler();
    }
    #endif
}
#endif // HAS_I2CN(3)

void hw_spi2_init() {
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.NSS = SPI_NSS_SOFT;
#if spi_accelerometer == 2
    hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8
#elif spi_lcd == 2
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2
#endif
        ;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi2) != HAL_OK) {
        Error_Handler();
    }
}

void hw_spi3_init() {
    hspi3.Instance = SPI3;
    hspi3.Init.Mode = SPI_MODE_MASTER;
    hspi3.Init.Direction = SPI_DIRECTION_2LINES;
    hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi3.Init.NSS = SPI_NSS_SOFT;
#if (BOARD_IS_BUDDY())
    hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
#else
    hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
#endif
    hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi3.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi3) != HAL_OK) {
        Error_Handler();
    }
}

#if BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY()
void hw_spi4_init() {
    // SPI 4 is used for side leds, but only on specific HW revisions
    hspi4.Instance = SPI4;
    hspi4.Init.Mode = SPI_MODE_MASTER;
    hspi4.Init.Direction = SPI_DIRECTION_2LINES;
    hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi4.Init.NSS = SPI_NSS_SOFT;
    hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi4.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi4) != HAL_OK) {
        Error_Handler();
    }
}
#endif

#if BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY()
void hw_spi5_init() {
    hspi5.Instance = SPI5;
    hspi5.Init.Mode = SPI_MODE_MASTER;
    hspi5.Init.Direction = SPI_DIRECTION_2LINES;
    hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi5.Init.NSS = SPI_NSS_SOFT;
    hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi5.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi5) != HAL_OK) {
        Error_Handler();
    }
}
#endif

#if BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY()
void hw_spi6_init() {
    hspi6.Instance = SPI6;
    hspi6.Init.Mode = SPI_MODE_MASTER;
    hspi6.Init.Direction = SPI_DIRECTION_2LINES;
    hspi6.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi6.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi6.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi6.Init.NSS = SPI_NSS_SOFT;
    hspi6.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi6.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi6.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi6.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi6.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi6) != HAL_OK) {
        Error_Handler();
    }
}
#endif

void hw_tim1_init() {
    TIM_ClockConfigTypeDef sClockSourceConfig {};
    TIM_MasterConfigTypeDef sMasterConfig {};
    TIM_OC_InitTypeDef sConfigOC {};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig {};

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = TIM1_default_Prescaler; // 0x3fff was 100;
    htim1.Init.CounterMode = TIM_COUNTERMODE_DOWN;
    htim1.Init.Period = TIM1_default_Period; // 0xff was 42000;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) { //_PWM_FAN1
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) { //_PWM_FAN
        Error_Handler();
    }
    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK) {
        Error_Handler();
    }
    __HAL_TIM_ENABLE(&htim1);
}

void hw_tim2_init() {
    TIM_ClockConfigTypeDef sClockSourceConfig {};
    TIM_MasterConfigTypeDef sMasterConfig {};
    TIM_OC_InitTypeDef sConfigOC {};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 100;
    htim2.Init.CounterMode = TIM_COUNTERMODE_DOWN;
    htim2.Init.Period = 42000;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 21000;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim2);
}

void hw_tim3_init() {
    TIM_ClockConfigTypeDef sClockSourceConfig {};
    TIM_MasterConfigTypeDef sMasterConfig {};
    TIM_OC_InitTypeDef sConfigOC {};

    htim3.Instance = TIM3;
#if (PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_iX())
    htim3.Init.Prescaler = 11; // 36us, 33.0kHz
#else
    htim3.Init.Prescaler = TIM3_default_Prescaler; // 49ms, 20.3Hz
#endif
    htim3.Init.CounterMode = TIM_COUNTERMODE_DOWN;
    htim3.Init.Period = TIM3_default_Period; // 0xff was 42000
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 21000;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK) { //_PWM_HEATER_BED
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK) { //_PWM_HEATER_0
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim3);
    __HAL_TIM_ENABLE(&htim3);
}

void hw_tim8_init() {
    TIM_ClockConfigTypeDef sClockSourceConfig {};
    TIM_MasterConfigTypeDef sMasterConfig {};

    using phase_stepping::opts::GPIO_BUFFER_SIZE;
    using phase_stepping::opts::REFRESH_FREQ;

    // Clock the period ever-so-slighly faster than the required number of events to create a gap
    // between two bursts big enough to allow scheduling in the case of two full buffers and a
    // shorter-than-expected interval between the ISRs.
    uint32_t period = 168'000'000 / (REFRESH_FREQ * (GPIO_BUFFER_SIZE + 1)) - 1;

    htim8.Instance = TIM8;
    htim8.Init.Prescaler = 0;
    htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim8.Init.Period = period;
    htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim8) != HAL_OK) {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim8);
}

void hw_tim13_init() {
    htim13.Instance = TIM13;
    htim13.Init.Prescaler = 0;
    htim13.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim13.Init.Period = 84'000'000 / phase_stepping::opts::REFRESH_FREQ - 1;
    htim13.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim13.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim13) != HAL_OK) {
        Error_Handler();
    }
}

void hw_tim14_init() {
    htim14.Instance = TIM14;
    htim14.Init.Prescaler = 84;
    htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim14.Init.Period = 1000;
    htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim14) != HAL_OK) {
        Error_Handler();
    }
    HAL_TIM_Base_Start_IT(&htim14);
}
