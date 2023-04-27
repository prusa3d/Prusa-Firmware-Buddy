#include "main.h"
#include "platform.h"
#include <device/board.h>
#include "config_features.h"
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
#include <crash_dump/dump.hpp>
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
#include <option/has_puppies_bootloader.h>
#include <option/filament_sensor.h>
#include <option/has_gui.h>
#include <option/has_side_leds.h>
#include <option/has_embedded_esp32.h>
#include "tasks.hpp"
#include <appmain.hpp>
#include "safe_state.h"
#include <espif.h>

#if BOARD_IS_XLBUDDY
    #include "puppies/PuppyBus.hpp"
    #include "puppies/puppy_task.hpp"
#endif

#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
#endif

#if HAS_SIDE_LEDS()
    #include <leds/task.hpp>
#endif

#ifdef BUDDY_ENABLE_WUI
    #include "wui.h"
#endif

using namespace crash_dump;

LOG_COMPONENT_REF(Buddy);

osThreadId defaultTaskHandle;
osThreadId displayTaskHandle;
osThreadId connectTaskHandle;

unsigned HAL_RCC_CSR = 0;
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

#if (BOARD_IS_BUDDY)
uartrxbuff_t uart1rxbuff;
static uint8_t uart1rx_data[32];
#endif

extern "C" void main_cpp(void) {
    // save and clear reset flags
    HAL_RCC_CSR = RCC->CSR;
    __HAL_RCC_CLEAR_RESET_FLAGS();

    hw_gpio_init();
    hw_dma_init();

#if BOARD_IS_BUDDY || BOARD_IS_XBUDDY
    hw_tim1_init();
#endif
    hw_tim14_init();

    SPI_INIT(flash);
    // initialize SPI flash
    w25x_spi_assign(&SPI_HANDLE_FOR(flash));
    if (!w25x_init())
        bsod("failed to initialize ext flash");

    /*
     * If we have BSOD or red screen we want to have as small boot process as we can.
     * We want to init just xflash, display and start gui task to display the bsod or redscreen
     */
    if ((dump_in_xflash_is_valid() && !dump_in_xflash_is_displayed()) || (dump_err_in_xflash_is_valid() && !dump_err_in_xflash_is_displayed())) {
        hwio_safe_state();
        init_error_screen();
        return;
    }

    logging_init();
    TaskDeps::components_init();
    hw_adc1_init();

#if (BOARD_IS_BUDDY)
    hw_uart1_init();
#endif

#if BOARD_IS_BUDDY || BOARD_IS_XBUDDY
    hw_tim3_init();
#endif

#ifdef HAS_ADC3
    hw_adc3_init();
#endif

#if HAS_GUI()
    SPI_INIT(lcd);
#endif

#if BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY
    I2C_INIT(usbc);
#endif

#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
    I2C_INIT(touch);
#endif

#if (BOARD_IS_XBUDDY)
    SPI_INIT(extconn);
    SPI_INIT(accelerometer);
#endif

#if defined(spi_tmc)
    SPI_INIT(tmc);
#elif defined(uart_tmc)
    UART_INIT(tmc);
#else
    #error Do not know how to init TMC communication channel
#endif

#ifdef BUDDY_ENABLE_WUI
    UART_INIT(esp);
#endif

#if HAS_MMU2
    UART_INIT(mmu);
#endif

#if HAS_GUI() && !(BOARD_IS_XLBUDDY)
    hw_tim2_init(); // TIM2 is used to generate buzzer PWM. Not needed without display.
#endif

#if BOARD_IS_XLBUDDY
    UART_INIT(puppies);
    SPI_INIT(led);
    buddy::puppies::PuppyBus::Open();
#endif

    hw_rtc_init();
    hw_rng_init();

    MX_USB_HOST_Init();

    MX_FATFS_Init();

    usb_device_init();

    HAL_GPIO_Initialized = 1;
    HAL_ADC_Initialized = 1;
    HAL_PWM_Initialized = 1;
    HAL_SPI_Initialized = 1;

    eeprom_init_status_t status = eeprom_init();
    if (status == EEPROM_INIT_Defaults || status == EEPROM_INIT_Upgraded) {
        // this means we are either starting from defaults or after a FW upgrade -> invalidate the XFLASH dump, since it is not relevant anymore
        dump_in_xflash_reset();
    }

    wdt_iwdg_warning_cb = iwdg_warning_cb;

#if (BOARD_IS_BUDDY)
    buddy::hw::BufferedSerial::uart2.Open();
#endif

#if (BOARD_IS_BUDDY)
    uartrxbuff_init(&uart1rxbuff, &hdma_usart1_rx, sizeof(uart1rx_data), uart1rx_data);
    HAL_UART_Receive_DMA(&huart1, uart1rxbuff.buffer, uart1rxbuff.buffer_size);
    uartrxbuff_reset(&uart1rxbuff);
#endif

#if (BOARD_IS_XBUDDY)
    buddy::hw::BufferedSerial::uart6.Open();
#endif

    filesystem_init();

    adcDma1.init();

#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
    adcDma3.init();
#endif

    static metric_handler_t *handlers[] = {
        &metric_handler_syslog,
        &metric_handler_info_screen,
        NULL
    };
    metric_system_init(handlers);

#ifdef BUDDY_ENABLE_WUI
    espif_init_hw();
#endif

    osThreadDef(defaultTask, StartDefaultTask, osPriorityHigh, 0, 1024);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    if (option::has_gui) {
        osThreadDef(displayTask, StartDisplayTask, osPriorityNormal, 0, 1024 + 256 + 128);
        displayTaskHandle = osThreadCreate(osThread(displayTask), NULL);
    }

#if BOARD_IS_XLBUDDY
    buddy::puppies::start_puppy_task();
#endif

#ifdef BUDDY_ENABLE_WUI
    #if HAS_EMBEDDED_ESP32()
        #if BOARD_VER_EQUAL_TO(0, 5, 0)
    // This is temporary, remove once everyone has compatible hardware.
    // Requires new sandwich rev. 06 or rev. 05 with R83 removed.

    TaskDeps::wait(TaskDeps::Tasks::network);
        #endif
    #endif
    start_network_task();
#endif

#ifdef BUDDY_ENABLE_CONNECT
    /* definition and creation of connectTask */
    osThreadDef(connectTask, StartConnectTask, osPriorityBelowNormal, 0, 2304);
    connectTaskHandle = osThreadCreate(osThread(connectTask), NULL);
#endif

#if ENABLED(POWER_PANIC)
    /* definition and creation of acFaultTask */
    osThreadDef(acFaultTask, power_panic::ac_fault_main, osPriorityRealtime, 0, 80);
    power_panic::ac_fault_task = osThreadCreate(osThread(acFaultTask), NULL);
#endif

#if HAS_SIDE_LEDS()
    osThreadDef(ledsTask, leds::run_task, osPriorityNormal, 0, 256);
    osThreadCreate(osThread(ledsTask), NULL);
#endif

    if constexpr (option::filament_sensor != option::FilamentSensor::no) {
        /* definition and creation of measurementTask */
        osThreadDef(measurementTask, StartMeasurementTask, osPriorityNormal, 0, 512);
        osThreadCreate(osThread(measurementTask), NULL);
    }
}

#ifdef USE_ST7789
extern void st7789v_spi_tx_complete(void);
#endif

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {

#if HAS_GUI() && defined(USE_ST7789)
    if (hspi == st7789v_config.phspi) {
        st7789v_spi_tx_complete();
    }
#endif

#if HAS_GUI() && defined(USE_ILI9488)
    if (hspi == ili9488_config.phspi) {
        ili9488_spi_tx_complete();
    }
#endif

    if (hspi == &SPI_HANDLE_FOR(flash)) {
        w25x_spi_transfer_complete_callback();
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {

#if HAS_GUI() && defined(USE_ILI9488)
    if (hspi == ili9488_config.phspi) {
        ili9488_spi_rx_complete();
    }
#endif

    if (hspi == &SPI_HANDLE_FOR(flash)) {
        w25x_spi_receive_complete_callback();
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *haurt) {
#if (BOARD_IS_BUDDY)
    if (haurt == &huart2) {
        buddy::hw::BufferedSerial::uart2.WriteFinishedISR();
    }
#endif

#if BOARD_IS_XLBUDDY
    if (haurt == &huart3) {
        buddy::puppies::PuppyBus::bufferedSerial.WriteFinishedISR();
    }
#endif

#if (BOARD_IS_XBUDDY)
    if (haurt == &huart6) {
        //        log_debug(Buddy, "HAL_UART6_TxCpltCallback");
        buddy::hw::BufferedSerial::uart6.WriteFinishedISR();
    #if HAS_MMU2
            // instruct the RS485 converter, that we have finished sending data and from now on we are expecting a response from the MMU
            // set to high in hwio_pindef.h
            // buddy::hw::RS485FlowControl.write(buddy::hw::Pin::State::high);
    #endif
    }
#endif
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {

#if (BOARD_IS_BUDDY)
    if (huart == &huart2) {
        buddy::hw::BufferedSerial::uart2.FirstHalfReachedISR();
    }
#endif

#if BOARD_IS_XLBUDDY
    if (huart == &huart3) {
        buddy::puppies::PuppyBus::bufferedSerial.FirstHalfReachedISR();
    }
#endif

#if (BOARD_IS_XBUDDY)

    if (huart == &huart6) {
        buddy::hw::BufferedSerial::uart6.FirstHalfReachedISR();
    }
#endif
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
#if (BOARD_IS_BUDDY)
    if (huart == &huart2) {
        buddy::hw::BufferedSerial::uart2.SecondHalfReachedISR();
    }
#endif

#if (BOARD_IS_XLBUDDY)
    if (huart == &huart3) {
        buddy::puppies::PuppyBus::bufferedSerial.SecondHalfReachedISR();
    }
#endif

#if (BOARD_IS_XBUDDY)
    if (huart == &huart6) {
        buddy::hw::BufferedSerial::uart6.SecondHalfReachedISR();
    }
#endif
}

void StartDefaultTask([[maybe_unused]] void const *argument) {
    app_startup();

    app_run();
    for (;;) {
        osDelay(1);
    }
}

void StartDisplayTask([[maybe_unused]] void const *argument) {
    gui_run();
    for (;;) {
        osDelay(1);
    }
}

void StartErrorDisplayTask([[maybe_unused]] void const *argument) {
    gui_error_run();
    for (;;) {
        osDelay(1);
    }
}

#ifdef BUDDY_ENABLE_CONNECT
void StartConnectTask([[maybe_unused]] void const *argument) {
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
#ifdef _DEBUG
    ///@note Watchdog is disabled in debug but,
    /// so this will be helpful only if it is manually enabled or ifdef commented out

    // Breakpoint if debugger is connected
    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
        __BKPT(0);
    }
#endif /*_DEBUG*/
    DUMP_IWDGW_TO_CCRAM(0x10);
    wdt_iwdg_refresh();
    dump_to_xflash();
#ifndef _DEBUG
    while (true)
        ;
#else
    sys_reset();
#endif
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

void init_error_screen() {
    if constexpr (option::has_gui) {
        // init lcd spi and timer for buzzer
        SPI_INIT(lcd);
#if !(BOARD_IS_XLBUDDY && _DEBUG)
        hw_tim2_init(); // TIM2 is used to generate buzzer PWM. Not needed without display.
#endif

        init_only_littlefs();

        osThreadDef(displayTask, StartErrorDisplayTask, osPriorityNormal, 0, 1024 + 256);
        displayTaskHandle = osThreadCreate(osThread(displayTask), NULL);
    }
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
