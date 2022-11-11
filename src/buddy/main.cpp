#include "main.h"
#include "../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include "usb_device.h"
#include "usb_host.h"
#include "buffered_serial.hpp"
#include "bsod.h"
#ifdef BUDDY_ENABLE_CONNECT
    #include "connect/run.hpp"
#endif

#include "sys.h"
#include "app.h"
#include "wdt.h"
#include <crash_dump/dump.h>
#include "timer_defaults.h"
#include "tick_timer_api.h"
#include "thread_measurement.h"
#include "metric_handlers.h"
#include "hwio_pindef.h"
#include "gui.hpp"
#include "config_buddy_2209_02.h"
#include "eeprom.h"
#include "crc32.h"
#include "w25x.h"
#include "timing.h"
#include "filesystem.h"
#include "adc.hpp"
#include "logging.h"
#include "common/disable_interrupts.h"
#include <option/filament_sensor.h>
#include <option/has_gui.h>
#include "tasks.h"

#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
#endif
#ifdef BUDDY_ENABLE_WUI
    #include "wui.h"
#endif

LOG_COMPONENT_REF(Buddy);

osThreadId defaultTaskHandle;
osThreadId displayTaskHandle;
osThreadId connectTaskHandle;
uint HAL_RCC_CSR = 0;
int HAL_GPIO_Initialized = 0;
int HAL_ADC_Initialized = 0;
int HAL_PWM_Initialized = 0;
int HAL_SPI_Initialized = 0;

void SystemClock_Config(void);
void StartDefaultTask(void const *argument);
void StartDisplayTask(void const *argument);
void StartConnectTask(void const *argument);
void StartESPTask(void const *argument);
void iwdg_warning_cb(void);

uartrxbuff_t uart1rxbuff;
static uint8_t uart1rx_data[200];
#ifndef USE_ESP01_WITH_UART6
uartrxbuff_t uart6rxbuff;
uint8_t uart6rx_data[128];
uartslave_t uart6slave;
char uart6slave_line[32];
#endif

extern "C" void main_cpp(void) {
    // save and clear reset flags
    HAL_RCC_CSR = RCC->CSR;
    __HAL_RCC_CLEAR_RESET_FLAGS();

    logging_init();
    components_init();

    hw_gpio_init();
    hw_dma_init();

    hw_adc1_init();

    hw_uart1_init();

    hw_tim1_init();
    hw_tim3_init();

    SPI_INIT(flash);

#if HAS_GUI()
    SPI_INIT(lcd);
#endif

    UART_INIT(tmc);

#ifdef BUDDY_ENABLE_WUI
    UART_INIT(esp);
#endif

#if HAS_GUI()
    hw_tim2_init(); // TIM2 is used to generate buzzer PWM. Not needed without display.
#endif

    hw_tim14_init();
    hw_rtc_init();
    hw_rng_init();

    // initialize SPI flash
    w25x_spi_assign(&SPI_HANDLE_FOR(flash));
    if (!w25x_init())
        bsod("failed to initialize ext flash");

    MX_USB_HOST_Init();

    MX_FATFS_Init();

    usb_device_init();

    HAL_GPIO_Initialized = 1;
    HAL_ADC_Initialized = 1;
    HAL_PWM_Initialized = 1;
    HAL_SPI_Initialized = 1;

    bool block_networking = false;
    /*
     * Checking this first, before starting the GUI thread. The GUI thread
     * resets/consumes the dump as a side effect.
     */
    if (dump_in_xflash_is_valid() && !dump_in_xflash_is_displayed()) {
        int dump_type = dump_in_xflash_get_type();
        if (dump_type == DUMP_HARDFAULT || dump_type == DUMP_FATALERROR) {
            /*
             * This corresponds to booting into a bluescreen or serious
             * redscreen. In such case, the GUI is blocked. Similar logic
             * should apply to any network communication ‒ one probably shall
             * not eg. start a print from there.
             */
            block_networking = true;
        }
    }

    eeprom_init_status_t status = eeprom_init();
    if (status == EEPROM_INIT_Defaults || status == EEPROM_INIT_Upgraded) {
        // this means we are either starting from defaults or after a FW upgrade -> invalidate the XFLASH dump, since it is not relevant anymore
        dump_in_xflash_reset();
    }

    wdt_iwdg_warning_cb = iwdg_warning_cb;

    buddy::hw::BufferedSerial::uart2.Open();

    uartrxbuff_init(&uart1rxbuff, &huart1, &hdma_usart1_rx, sizeof(uart1rx_data), uart1rx_data);
    HAL_UART_Receive_DMA(&huart1, uart1rxbuff.buffer, uart1rxbuff.buffer_size);
    uartrxbuff_reset(&uart1rxbuff);

    filesystem_init();

    adcDma1.init();

    static metric_handler_t *handlers[] = {
        &metric_handler_syslog,
        NULL
    };
    metric_system_init(handlers);

    osThreadDef(defaultTask, StartDefaultTask, osPriorityHigh, 0, 1024);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    if (option::has_gui) {
        osThreadDef(displayTask, StartDisplayTask, osPriorityNormal, 0, 1024 + 256);
        displayTaskHandle = osThreadCreate(osThread(displayTask), NULL);
    }

#ifdef BUDDY_ENABLE_WUI
    if (!block_networking) {
        start_network_task();
    }
#else
    // Avoid unused warning.
    (void)block_networking;
#endif

#ifdef BUDDY_ENABLE_CONNECT
    if (!block_networking) {
        /* definition and creation of connectTask */
        osThreadDef(connectTask, StartConnectTask, osPriorityBelowNormal, 0, 2048);
        connectTaskHandle = osThreadCreate(osThread(connectTask), NULL);
    }
#endif

#if ENABLED(POWER_PANIC)
    /* definition and creation of acFaultTask */
    osThreadDef(acFaultTask, power_panic::ac_fault_main, osPriorityRealtime, 0, 80);
    power_panic::ac_fault_task = osThreadCreate(osThread(acFaultTask), NULL);
#endif

    if constexpr (option::filament_sensor != option::FilamentSensor::no) {
        /* definition and creation of measurementTask */
        osThreadDef(measurementTask, StartMeasurementTask, osPriorityNormal, 0, 512);
        osThreadCreate(osThread(measurementTask), NULL);
    }
}

extern void st7789v_spi_tx_complete(void);
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi == st7789v_config.phspi) {
        st7789v_spi_tx_complete();
    }

    if (hspi == &SPI_HANDLE_FOR(flash)) {
        w25x_spi_transfer_complete_callback();
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi == &SPI_HANDLE_FOR(flash)) {
        w25x_spi_receive_complete_callback();
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *haurt) {
    if (haurt == &huart2)
        buddy::hw::BufferedSerial::uart2.WriteFinishedISR();
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &huart2)
        buddy::hw::BufferedSerial::uart2.FirstHalfReachedISR();
#if 0
    else if (huart == &huart6)
        uartrxbuff_rxhalf_cb(&uart6rxbuff);
#endif
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &huart1)
        uartrxbuff_rxcplt_cb(&uart1rxbuff);
    else if (huart == &huart2)
        buddy::hw::BufferedSerial::uart2.SecondHalfReachedISR();
#ifndef USE_ESP01_WITH_UART6
    else if (huart == &huart6)
        uartrxbuff_rxcplt_cb(&uart6rxbuff);
#endif
}

void StartDefaultTask(void const *argument) {
    log_info(Buddy, "marlin task waiting for dependecies");
    wait_for_dependecies(DEFAULT_TASK_DEPS);
    log_info(Buddy, "marlin task is starting");

    app_run();
    for (;;) {
        osDelay(1);
    }
}

void StartDisplayTask(void const *argument) {
    gui_run();
    for (;;) {
        osDelay(1);
    }
}

#ifdef BUDDY_ENABLE_CONNECT
void StartConnectTask(void const *argument) {
    connect_client::run();
}
#endif

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM6 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM14) {
        app_tim14_tick();
    } else if (htim->Instance == TICK_TIMER) {
        app_tick_timer_overflow();
    } else {
        HardwareTimer::updateCallback(htim);
    }
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
    /* User can add his own implementation to report the HAL error return state */
    app_error();
}

void system_core_error_handler() {
    app_error();
}

void iwdg_warning_cb(void) {
    DUMP_IWDGW_TO_CCRAM(0x10);
    wdt_iwdg_refresh();
    dump_to_xflash();
    while (1)
        ;
    //	sys_reset();
}

static uint32_t _spi_prescaler(int prescaler_num) {
    switch (prescaler_num) {
    case 0:
        return SPI_BAUDRATEPRESCALER_2; // 0x00000000U
    case 1:
        return SPI_BAUDRATEPRESCALER_4; // 0x00000008U
    case 2:
        return SPI_BAUDRATEPRESCALER_8; // 0x00000010U
    case 3:
        return SPI_BAUDRATEPRESCALER_16; // 0x00000018U
    case 4:
        return SPI_BAUDRATEPRESCALER_32; // 0x00000020U
    case 5:
        return SPI_BAUDRATEPRESCALER_64; // 0x00000028U
    case 6:
        return SPI_BAUDRATEPRESCALER_128; // 0x00000030U
    case 7:
        return SPI_BAUDRATEPRESCALER_256; // 0x00000038U
    }
    return SPI_BAUDRATEPRESCALER_2;
}

void spi_set_prescaler(SPI_HandleTypeDef *hspi, int prescaler_num) {
    buddy::DisableInterrupts disable_interrupts;
    HAL_SPI_DeInit(hspi);
    hspi->Init.BaudRatePrescaler = _spi_prescaler(prescaler_num);
    HAL_SPI_Init(hspi);
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
    /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    app_assert(file, line);
}
#endif /* USE_FULL_ASSERT */
