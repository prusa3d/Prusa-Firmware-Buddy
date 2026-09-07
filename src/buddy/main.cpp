#include <buddy/main.h>
#include "platform.h"
#include <device/board.h>
#include <device/peripherals.hpp>
#include <device/peripherals_uart.hpp>
#include <freertos/critical_section.hpp>
#include <guiconfig/guiconfig.h>
#include "config_features.h"
#include "cmsis_os.h"
#include <buddy/fatfs.h>
#include <buddy/usb_device.hpp>
#include <bsod/bsod.h>
#include <common/st25dv64k.h>
#include "usb_host.h"
#include "buffered_serial.hpp"
#include "bsod_gui.hpp"
#include <config_store/store_instance.hpp>
#include <Marlin/src/feature/prusa/e-stall_detector.h>
#include <wdt.hpp>
#include <crash_dump/dump.hpp>
#include "error_codes.hpp"
#include <find_error.hpp>
#include "timer_defaults.h"
#include "tick_timer_api.h"
#include <logging/log_dest_syslog.hpp>
#include "metric_handlers.h"
#include "hwio_pindef.h"
#include "gui.hpp"
#include "display.hpp"
#include <span>
#include <stdint.h>
#include "printers.h"
#include "MarlinPin.h"
#include "crc32.h"
#include <common/sys.hpp>
#include <common/spi_flash_bus.hpp>
#include <common/w25x.hpp>
#include "timing.h"
#include <buddy/filesystem.h>
#include "adc.hpp"
#include <buddy/logging.h>
#include <i2c.hpp>
#include <option/buddy_enable_connect.h>
#include <option/has_power_panic.h>
#include <option/has_puppies.h>
#include <option/has_puppies_bootloader.h>
#include <option/filament_sensor.h>
#include <option/has_gui.h>
#include <option/has_mmu2_over_uart.h>
#include <option/resources.h>
#include <option/bootloader_update.h>
#include <option/has_side_leds.h>
#include <option/has_advanced_power.h>
#include <option/has_phase_stepping.h>
#include <option/has_burst_stepping.h>
#include <option/has_indx.h>
#include <option/has_indx_head.h>
#include <option/has_internal_storage_flash.h>
#include <option/has_xbuddy_extension.h>
#include <option/buddy_enable_wui.h>
#include <option/has_touch.h>
#include <option/has_nfc.h>
#include <option/has_i2c_expander.h>
#include <option/has_local_accelerometer.h>
#include <option/has_cpu_fan.h>
#include "tasks.hpp"
#include <appmain.hpp>
#include "safe_state.h"
#include "sound.hpp"
#include <version/version.hpp>
#include "data_exchange.hpp"
#include "bootloader/bootloader.hpp"
#include "resources/revision.hpp"
#include <buddy/filesystem_semihosting.h>
#include <freertos/timing.hpp>
#include <heap.h>
#include <heap.hpp>
#include <fanctl.hpp>
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <sensor_data.hpp>

#include <option/rtt_metrics_enabled.h>
#if RTT_METRICS_ENABLED()
    #include <rtt_metrics_task/rtt_metrics_task.hpp>
#endif

#if BUDDY_ENABLE_CONNECT()
    #include "connect/run.hpp"
#endif
#if HAS_PUPPIES()
    #include "puppies/PuppyBus.hpp"
    #include "puppies/puppy_task.hpp"
#endif
#if ENABLED(RESOURCES())
    #include "resources/bootstrap.hpp"
    #include "resources/revision.hpp"
#endif

#if HAS_POWER_PANIC()
    #include "power_panic.hpp"
#endif

#if BUDDY_ENABLE_WUI()
    #include "wui.h"
#endif

#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    #include "hw_configuration.hpp"
#endif

#if HAS_PHASE_STEPPING()
    #include <feature/phase_stepping/phase_stepping.hpp>
#endif

#if HAS_LOCAL_ACCELEROMETER()
    #include <module/prusa/accelerometer_local.hpp>
#endif

#if HAS_NFC()
    #include <nfc.hpp>
#endif

#if HAS_ADVANCED_POWER()
    #include <advanced_power.hpp>
#endif

#if HAS_INTERNAL_STORAGE_FLASH()
    #include <common/mt29f_flash.hpp>
#endif

#include <option/has_esp.h>
#if HAS_ESP()
    #include "buddy/esp_flash_task.hpp"
#endif

#if BUDDY_ENABLE_CONNECT() && !BUDDY_ENABLE_WUI()
    // #error dead code found by automatic analyses (see BFW-5461)
    // FIXME: We should be able to split networking to the lower-level network part and the Link part. Currently, both are done through WUI.
    #error "Can't have connect without WUI"
#endif

using namespace crash_dump;

LOG_COMPONENT_REF(Buddy);

osThreadId defaultTaskHandle;
osThreadId displayTaskHandle;

unsigned HAL_RCC_CSR = 0;
int HAL_GPIO_Initialized = 0;
int HAL_ADC_Initialized = 0;
int HAL_PWM_Initialized = 0;
int HAL_SPI_Initialized = 0;

void SystemClock_Config(void);
void iwdg_warning_cb(void);

/// Keep task control blocks together and away from stacks.
struct TaskControlBlock {
    StaticTask_t marlin;
#if HAS_POWER_PANIC()
    StaticTask_t acfault;
#endif
#if HAS_GUI()
    StaticTask_t display;
#endif
#if HAS_PUPPIES()
    StaticTask_t puppies;
#endif
#if BUDDY_ENABLE_WUI()
    StaticTask_t network;
#endif
#if BUDDY_ENABLE_CONNECT()
    StaticTask_t connect;
#endif
#if RTT_METRICS_ENABLED()
    StaticTask_t rtt_metrics;
#endif
};
static TaskControlBlock task_control_block;

/// Keep task stacks together and in the CCMRAM.
struct TaskStack {
    uint32_t marlin[1360];
#if HAS_POWER_PANIC()
    uint32_t acfault[80];
#endif
#if HAS_GUI()
    uint32_t display[1536];
#endif
#if HAS_PUPPIES()
    uint32_t puppies[896];
#endif
#if BUDDY_ENABLE_WUI()
    uint32_t network[1024];
#endif
#if BUDDY_ENABLE_CONNECT()
    uint32_t connect[2336];
#endif
#if RTT_METRICS_ENABLED()
    uint32_t rtt_metrics[100];
#endif
};
static TaskStack __attribute__((section(".ccmram"))) task_stack;

/// Helper to run freertos task.
static TaskHandle_t create_task(const char *name, void (*func)(), osPriority priority, std::span<uint32_t> stack, StaticTask_t &tcb) {
    return xTaskCreateStatic(
        [](void *arg) { reinterpret_cast<void (*)()>(arg)(); },
        name,
        stack.size(),
        reinterpret_cast<void *>(func),
        tskIDLE_PRIORITY + priority - osPriorityIdle,
        stack.data(),
        &tcb);
}

/**
 * Report bootstrap finished and firmware version.
 * This needs to be called after resources were successfully updated
 * in xFlash. This needs to be output to ESP UART at 115200 bauds.
 * Format of the messages can not be changed as test station
 * expect those as step in manufacturing process.
 * The board needs to be able to report this with no additional
 * dependencies to connected peripherals.
 */
static void manufacture_report_endless_loop() {
#if HAS_ESP()
    // ESP reset (needed for XL, since it has embedded ESP)
    HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_RESET);
    UART_HandleTypeDef uart_for_tester = uart_handle_for_esp;
#else
    UART_HandleTypeDef uart_for_tester = uart_handle_for_puppies;
#endif

    constexpr const uint8_t endl = '\n';
    constexpr const char *str_fw = "FW:";
    while (true) {
        HAL_UART_Transmit(&uart_for_tester, reinterpret_cast<const uint8_t *>(str_fw), strlen(str_fw), 1000);
        HAL_UART_Transmit(&uart_for_tester, reinterpret_cast<const uint8_t *>(version::project_version_full), strlen(version::project_version_full), 1000);
        HAL_UART_Transmit(&uart_for_tester, &endl, sizeof(endl), 1000);
        osDelay(500); // tester needs 500ms, do not change this value!
    }
}

#if ENABLED(RESOURCES()) && ENABLED(BOOTLOADER_UPDATE())
// Return TRUE if bootloader was updated -> in this case we have to reset the system, because important data addresses could be moved
static bool bootloader_update() {
    if (buddy::bootloader::needs_update()) {
        buddy::bootloader::update();
        return true;
    }
    return false;
}
#endif

static void resources_update() {
    if (!buddy::resources::has_resources(buddy::resources::revision::standard)) {
        buddy::resources::bootstrap(buddy::resources::revision::standard);
    }
    TaskDeps::provide(TaskDeps::Dependency::resources_ready);
}

// Initializes static variables of singletons which are accessed from ISRs (requires locking a mutex)
// Note: Hotend/Tool initialization is NOT done here — it is deferred to after ADC init
// to allow hotend model detection from ADC readings.
static void init_isr_statics() {
    EMotorStallDetector::Instance();
    Fans::print(PhysicalToolIndex::from_raw(0));
    Fans::heat_break(PhysicalToolIndex::from_raw(0));
#if XL_ENCLOSURE_SUPPORT()
    Fans::enclosure();
#endif
#if HAS_INDX()
    Fans::dock_fan();
#endif
#if HAS_CPU_FAN()
    Fans::cpu();
#endif
    sensor_data();
    GetExtruderFSensor(0);
    GetSideFSensor(0);
    Sound::getInstance();
}

extern "C" void main_cpp(void) {
    // save and clear reset flags
    HAL_RCC_CSR = RCC->CSR;
    __HAL_RCC_CLEAR_RESET_FLAGS();

    hw_gpio_init();
    Fans::init_hw();
    hw_dma_init();

    if (!W25xFlash::instance().init()) {
        // Actually, there is no point in calling bsod() here since it writes the message
        // to the external FLASH to show after the reboot
        bsod("failed to initialize ext flash");
    }
#if HAS_INTERNAL_STORAGE_FLASH()
    (void)Mt29fFlash::instance().init();
#endif

    // ADC/DMA
    hw_adc1_init();
    adcDma1.init();

#ifdef HAS_ADC3
    hw_adc3_init();
    adcDma3.init();
#endif
    hw_adc_irq_init();

    // After initializing the adc we need to wait some time before the internal MCU temp channel is stable.
    // Required time is at least 6us. We use 2ms to force pause of at least 1ms
    freertos::delay(2);

    // Initialize tool/hotend AFTER the ADC is ready — on HT-capable builds the
    // LocalHotend ctor reads the ADC to detect the NTC vs PT1000 hotend.
    // ISR ordering (BFW-8126): Hotend is accessed from the Marlin temperature ISR, so
    // it must be constructed before that ISR is armed — it is, the temperature timer is
    // started in Temperature::init() on the marlin task (created below). The ADC IRQ
    // enabled just above never touches Hotend (it only fills the DMA buffer the ISR reads).
    Hotend::for_tool(PhysicalToolIndex::from_raw(0));

#if PRINTER_IS_PRUSA_XL()
    // Read Sandwich hw revision
    SandwichConfiguration::Instance();
#endif

#if BOARD_IS_BUDDY() || BOARD_IS_XBUDDY()
    hw_tim1_init();
    #if HAS_LOCAL_ACCELEROMETER()
    hw_tim9_init();
    #endif
#endif

#if HAS_PHASE_STEPPING()
    hw_tim13_init();
#endif

#if HAS_BURST_STEPPING()
    // #error dead code found by automatic analyses (see BFW-5461)
    hw_tim8_init();
#endif

    hw_tim14_init();

    const bool want_error_screen = (dump_is_valid() && !dump_is_displayed()) || (message_is_valid() && message_get_type() != MsgType::EMPTY && !message_is_displayed());

#if HAS_NFC()
    nfc::turn_off();
#endif

#if HAS_GUI()
    spi_init_lcd();
#endif

#if HAS_GUI() && !BOARD_IS_XLBUDDY()
    hw_tim2_init(); // TIM2 is used to generate buzzer PWM, except on older versions of XL. Not needed without display.
#endif

#if BOARD_IS_XLBUDDY()
    hw_init_spi_side_leds();
#endif

#if PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
    /*
     * MK3.5 HW detected on MK4 firmware or vice versa
     * MK4 HW detected on CORE ONE firmware or vice versa
     *
     * Ignore the check in production (tester_mode), the xBuddy's connected peripherals are safe in this mode.
     */
    if (!buddy::hw::Configuration::Instance().is_fw_compatible_with_hw() && !running_in_tester_mode()) {
        const auto &error = find_error(ErrCode::WARNING_DIFFERENT_FW_REQUIRED);
        crash_dump::force_save_message_without_dump(crash_dump::MsgType::FATAL_WARNING, static_cast<uint16_t>(error.err_code), error.err_text, error.err_title);
        hwio_safe_state();
        init_error_screen();
        return;
    }
#endif

#if !BOARD_IS_BUDDY()
    // No Configuration class for BUDDY
    if (!buddy::hw::Configuration::Instance().check_bom_compatible()) {
        bsod("BOM ID not compatible");
    }
#endif

#if HAS_XBUDDY_EXTENSION()
    buddy::hw::Configuration::Instance().setup_ext_reset();
    #if XBUDDY_EXTENSION_VARIANT_IS_iX()
    buddy::hw::ext_shutdown.writeb(want_error_screen);
    buddy::hw::ext_pwr_enable.writeb(!want_error_screen);
    #else
    buddy::hw::ext_pwr_enable.set();
    #endif
#endif

#if HAS_INDX_HEAD()
    // The indx_head_reset pin table init state holds the head in reset only on BOM >= 37, re-assert so older boards do too
    buddy::hw::Configuration::Instance().write_indx_head_reset(buddy::hw::Pin::State::high);
#endif

    /*
     * If we have BSOD or red screen we want to have as small boot process as we can.
     * We want to init just xflash, display and start gui task to display the bsod or redscreen
     */
    if (want_error_screen) {
        hwio_safe_state();
        init_error_screen();

#if BUDDY_ENABLE_WUI() && BUDDY_ENABLE_CONNECT()
        // We want to send the error screen to Connect to show there.
        //
        // For that we need networking (and some other peripherals). We do not
        // init the rest - including the USB stack.
        //
        // We do not start link and we run Connect in special mode that allows
        // mostly nothing.
        //
        // block esp in tester mode (redscreen probably shouldn't happen on tester, but better safe than sorry)
        if (!running_in_tester_mode() && config_store().connect_enabled.get()) {
            TaskDeps::components_init();
            // Needed for certificate verification
            hw_rtc_init();
            // Needed for SSL random data
            hw_rng_init();

    #if HAS_ESP()
            uart_init_esp();
            // We can't flash ESP while showing error screen as there is no bootstrap progressbar.
            // Let's pretend that flashing was successful in order to enable Wi-Fi.
            skip_esp_flashing();
    #endif

            create_task("network", network_run_minimal, TASK_PRIORITY_WUI, task_stack.network, task_control_block.network);
            create_task("connect", connect_client::run_error, TASK_PRIORITY_CONNECT, task_stack.connect, task_control_block.connect);
        }
#endif
        return;
    }
    bsod_mark_shown(); // BSOD would be shown, allow new BSOD dump

    logging_init();
    TaskDeps::components_init();

#if BOARD_IS_BUDDY() || BOARD_IS_XBUDDY()
    hw_tim3_init();
#endif

#if BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY()
    i2c_init_usbc();
#endif

#if HAS_TOUCH()
    i2c_init_touch();
#endif

#if (BOARD_IS_XBUDDY())
    spi_init_accelerometer();
#endif

#if HAS_LOCAL_ACCELEROMETER()
    prusa_accelerometer_local_init();
#endif

#if defined(spi_init_tmc)
    spi_init_tmc();
#elif HAS_TMC_UART()
    uart_init_tmc();
#else
    #error Do not know how to init TMC communication channel
#endif

#if HAS_ESP() && BUDDY_ENABLE_WUI()
    uart_init_esp();
#endif

#if HAS_MMU2_OVER_UART()
    uart_init_mmu();
#endif

#if HAS_PUPPIES()
    hw_tim4_init();
    uart_init_puppies(running_in_tester_mode());
    if (!running_in_tester_mode()) {
        buddy::puppies::PuppyBus::Open();
    }
#endif

    hw_rtc_init();
    hw_rng_init();

#if HAS_ESP()
    // ESP flashing can start fairly early in the boot process.
    // On printers without embedded ESP32 we need to upload stub to enable verification.
    // This would take some seconds, which we can hide here.
    // Only after we find out that we actually need to flash the firmware we wait
    // for the bootstrap resources and take over the progress bar.
    // And as always, we need to prevent interactions with the UART in tester mode.
    if (!running_in_tester_mode()) {
        start_flash_esp_task();
    }
#endif

#if HAS_ADVANCED_POWER()
    advancedpower.ResetOvercurrentFault();
#endif

    MX_USB_HOST_Init();

    MX_FATFS_Init();

    HAL_GPIO_Initialized = 1;
    HAL_ADC_Initialized = 1;
    HAL_PWM_Initialized = 1;
    HAL_SPI_Initialized = 1;

    config_store_ns::InitResult status = config_store_init_result();
    if (status == config_store_ns::InitResult::cold_start) {
        // this means we are either starting from defaults or after a FW upgrade -> invalidate the
        // XFLASH dump and power-panic data, since it is not relevant anymore
        dump_reset();
#if HAS_POWER_PANIC()
        power_panic::reset();
#endif
    }

    // Restore sound settings from eeprom
    Sound::getInstance().restore_from_eeprom();

    wdt_iwdg_warning_cb = iwdg_warning_cb;

    filesystem_init();

#if HAS_GUI()
    displayTaskHandle = create_task("display", gui_run, TASK_PRIORITY_DISPLAY_TASK, task_stack.display, task_control_block.display);
#endif
    // wait for gui to init and render loading screen before starting flashing. We need to init bootstrap screen so we can send process percentage to it. Also it would look laggy without it.
    TaskDeps::wait(TaskDeps::Tasks::bootstrap_start);

#if ENABLED(RESOURCES()) && ENABLED(BOOTLOADER_UPDATE())
    if (bootloader_update()) {
        // Wait a while, before restart (this prevents some older board without appendix to enter internal bootloader on reset)
        osDelay(300);
        sys_reset();
    }
#endif

    usb_device_init();

#if ENABLED(RESOURCES())
    resources_update();
#endif
    filesystem_semihosting_deinit();

    metric_system_init();
    if (running_in_tester_mode()) {
        manufacture_report_endless_loop();
        return;
    }

#if HAS_TMC_UART()
    uart_for_tmc.Open();
#endif

#if HAS_MMU2_OVER_UART()
    uart_for_mmu.Open();
#endif

#if HAS_I2C_EXPANDER()
    // I2C IO Expander have to be initialized after Configuration Store
    buddy::hw::io_expander2.initialize();
#endif

    defaultTaskHandle = create_task("marlin", app_run, TASK_PRIORITY_DEFAULT_TASK, task_stack.marlin, task_control_block.marlin);

#if HAS_POWER_PANIC()
    power_panic::check_ac_fault_at_startup();
    power_panic::ac_fault_task = create_task("acfault", power_panic::ac_fault_task_main, TASK_PRIORITY_AC_FAULT, task_stack.acfault, task_control_block.acfault);
#endif

#if HAS_PUPPIES()
    create_task("puppies", buddy::puppies::run, TASK_PRIORITY_PUPPY_TASK, task_stack.puppies, task_control_block.puppies);
#endif

#if BUDDY_ENABLE_WUI()
    create_task("network", network_run, TASK_PRIORITY_WUI, task_stack.network, task_control_block.network);
#endif

#if BUDDY_ENABLE_CONNECT()
    create_task("connect", connect_client::run, TASK_PRIORITY_CONNECT, task_stack.connect, task_control_block.connect);
#endif

    // There is no point in initializing syslog before networking is up
    TaskDeps::wait(TaskDeps::Tasks::syslog);
    logging::syslog_reconfigure();
    metrics_reconfigure();

#if RTT_METRICS_ENABLED()
    create_task("rtt_metrics", rtt_metrics_task, TASK_PRIORITY_RTT_METRICS_TASK, task_stack.rtt_metrics, task_control_block.rtt_metrics);
#endif
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {

#if HAS_GUI()
    if (hspi == spi_handle_lcd) {
        display::spi_tx_complete();
    }
#endif

    if (hspi == spi_handle_flash) {
        SpiFlashBus::instance().on_tx_complete();
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi == spi_handle_flash) {
        SpiFlashBus::instance().on_rx_complete();
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
    if (hspi == spi_handle_flash) {
        SpiFlashBus::instance().on_error();
    }
}

void HAL_SPI_TxRxCpltCallback([[maybe_unused]] SPI_HandleTypeDef *hspi) {
#if HAS_LOCAL_ACCELEROMETER()
    if (hspi == spi_handle_accelerometer) {
        prusa_accelerometer_handle_spi_finish();
        return;
    }
#endif
    bsod_unreachable();
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM6 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
#if HAS_LOCAL_ACCELEROMETER()
    if (htim->Instance == TIM9) {
        prusa_accelerometer_handle_polling();
    }
#endif
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
    bsod("Error_Handler");
}

void iwdg_warning_cb(void) {
    const auto &e = find_error(ErrCode::ERR_SYSTEM_INTERNAL_ERROR);
    crash_dump::save_message(crash_dump::MsgType::IWDGW, static_cast<uint16_t>(e.err_code), nullptr, nullptr);
    trigger_crash_dump();
}

extern "C" void idle_callback() {
    if (isr_stack_overflow_checker().has_overflowed()) {
        bsod("ISR stack overflow");
    }
}

void init_error_screen() {
#if HAS_TOUCH
    // #error dead code found by automatic analyses (see BFW-5461)
    touchscreen.disable_till_reset();
#endif

#if HAS_GUI()
    init_only_littlefs();

    displayTaskHandle = create_task("display", gui_error_run, TASK_PRIORITY_DISPLAY_TASK, task_stack.display, task_control_block.display);
#endif
}

static void enable_trap_on_division_by_zero() {
    SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
}

static void enable_backup_domain() {
    // this allows us to use the RTC->BKPXX registers
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
}

static void enable_segger_sysview() {
    // enable the cycle counter for correct time reporting
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    SEGGER_SYSVIEW_Conf();
}

extern "C" void __libc_init_array(void);

namespace {
/// The entrypoint of the startup task
///
/// WARNING
/// The C++ runtime isn't initialized at the beginning of this function
/// and initializing it is the main priority here.
/// So first, we have to get the EEPROM ready, then we call libc_init_array
/// and that is the time everything is ready for us to switch to C++ context.
extern "C" void startup_task(void const *) {
    // init crc32 module. We need crc in eeprom_init
    crc32_init();

    i2c::ChannelMutex::static_init();

    // init communication with eeprom
    i2c_init_eeprom();

    // init eeprom module itself
    {
        freertos::CriticalSection critical_section;
        st25dv64k_init();
#if HAS_NFC()
        nfc::init();
#endif

        init_config_store();
        config_store().perform_config_check();
    }

// must do this before timer 1, timer 1 interrupt calls Configuration
// also must be before initializing global variables
#if BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY()
    buddy::hw::Configuration::Instance();
#endif

    // init global variables and call constructors
    __libc_init_array();

    init_isr_statics();

    // call the main main() function
    main_cpp();

    // terminate this thread (release its resources), we are done
    osThreadTerminate(osThreadGetId());
}
} // namespace

/// The entrypoint of our firmware
///
/// Do not do anything here that isn't essential to starting the RTOS
/// That is our one and only priority.
///
/// WARNING
/// The C++ runtime hasn't been initialized yet (together with C's constructors).
/// So make sure you don't do anything that is dependent on it.
int main() {
    // initialize FPU, vector table & external memory
    SystemInit();

    // initialize HAL
    HAL_Init();

    // configure system clock and timing
    system_core_init();
    tick_timer_init();

    // Instantiate isr_stack_overflow_checker now
    (void)isr_stack_overflow_checker();

    // other MCU setup
    enable_trap_on_division_by_zero();
    enable_backup_domain();
    enable_segger_sysview();

    // init the RAM area that serves for exchanging data with bootloader in
    // case this is a noboot build
    data_exchange_init();

    // define the startup task
    osThreadDef(startup, startup_task, TASK_PRIORITY_STARTUP, 0, 1024 + 512 + 256);
    osThreadCreate(osThread(startup), NULL);

    // start the RTOS with the single startup task
    osKernelStart();
}

#ifdef USE_FULL_ASSERT
// #error dead code found by automatic analyses (see BFW-5461)
/// Used by stm32 HAL"
void assert_failed(uint8_t *file, uint32_t line) {
    _bsod("STM32 assert fail", file, line);
}
#endif /* USE_FULL_ASSERT */
