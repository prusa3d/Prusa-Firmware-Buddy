#include "main.h"
#include "buddy/esp_flash_task.hpp"
#include "platform.h"
#include <device/board.h>
#include <device/peripherals.h>
#include <freertos/critical_section.hpp>
#include <guiconfig/guiconfig.h>
#include "config_features.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include "usb_device.hpp"
#include "usb_host.h"
#include "buffered_serial.hpp"
#include "bsod_gui.hpp"
#include <config_store/store_instance.hpp>
#include "sys.h"
#include <wdt.hpp>
#include <crash_dump/dump.hpp>
#include "error_codes.hpp"
#include <find_error.hpp>
#include "timer_defaults.h"
#include "tick_timer_api.h"
#include "thread_measurement.h"
#include <logging/log_dest_syslog.hpp>
#include "metric_handlers.h"
#include "hwio_pindef.h"
#include "gui.hpp"
#include "display.hpp"
#include <stdint.h>
#include "printers.h"
#include "MarlinPin.h"
#include "crc32.h"
#include "w25x.h"
#include "timing.h"
#include "filesystem.h"
#include "adc.hpp"
#include "logging.h"
#include <i2c.hpp>
#include <option/buddy_enable_connect.h>
#include <option/has_puppies.h>
#include <option/has_puppies_bootloader.h>
#include <option/filament_sensor.h>
#include <option/has_gui.h>
#include <option/has_mmu2.h>
#include <option/resources.h>
#include <option/bootloader_update.h>
#include <option/has_side_leds.h>
#include <option/has_phase_stepping.h>
#include <option/has_burst_stepping.h>
#include <option/buddy_enable_wui.h>
#include <option/has_touch.h>
#include <option/has_nfc.h>
#include <option/has_i2c_expander.h>
#include "tasks.hpp"
#include <appmain.hpp>
#include "safe_state.h"
#include <espif.h>
#include "sound.hpp"
#include <ccm_thread.hpp>
#include "version.h"
#include "str_utils.hpp"
#include "data_exchange.hpp"
#include "bootloader/bootloader.hpp"
#include "gui_bootstrap_screen.hpp"
#include "resources/revision.hpp"
#include "filesystem_semihosting.h"

#if BUDDY_ENABLE_CONNECT()
    #include "connect/run.hpp"
#endif
#if HAS_PUPPIES()
    #include "puppies/PuppyBus.hpp"
    #include "puppies/puppy_task.hpp"
#endif
#if ENABLED(RESOURCES())
    #include "resources/bootstrap.hpp"
    #include "resources/revision_standard.hpp"
#endif

#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
#endif

#if BUDDY_ENABLE_WUI()
    #include "wui.h"
#endif

#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    #include "hw_configuration.hpp"
#endif

#if HAS_MMU2()
    #include "feature/prusa/MMU2/mmu2_mk4.h"
#endif

#if HAS_PHASE_STEPPING()
    #include <feature/phase_stepping/phase_stepping.hpp>
#endif

#if HAS_NFC()
    #include <nfc.hpp>
#endif

#include <option/has_advanced_power.h>
#if HAS_ADVANCED_POWER()
    #include <advanced_power.hpp>
#endif

using namespace crash_dump;

LOG_COMPONENT_REF(Buddy);

osThreadId defaultTaskHandle;
osThreadId displayTaskHandle;
osThreadId connectTaskHandle;

#if HAS_GUI()
static constexpr size_t displayTask_stacksz = 1024 + 512; // in words
static uint32_t __attribute__((section(".ccmram"))) displayTask_buffer[displayTask_stacksz];
static StaticTask_t __attribute__((section(".ccmram"))) displayTask_control;
#endif

unsigned HAL_RCC_CSR = 0;
int HAL_GPIO_Initialized = 0;
int HAL_ADC_Initialized = 0;
int HAL_PWM_Initialized = 0;
int HAL_SPI_Initialized = 0;

void SystemClock_Config(void);
void StartDefaultTask(void const *argument);
void StartDisplayTask(void const *argument);
void StartConnectTask(void const *argument);
void StartConnectTaskError(void const *argument); // Version for redscreen
void StartESPTask(void const *argument);
void iwdg_warning_cb(void);

extern const metric_handler_t *const metric_system_handlers[] = {
    &metric_handler_syslog,
    nullptr
};

extern buddy::hw::BufferedSerial uart2;
extern buddy::hw::BufferedSerial uart6;

/**
 * @brief Bootstrap finished
 *
 * Report bootstrap finished and firmware version.
 * This needs to be called after resources were successfully updated
 * in xFlash. This also needs to be called even if xFlash / resources
 * are unused. This needs to be output to standard USB CDC destination.
 * Format of the messages can not be changed as test station
 * expect those as step in manufacturing process.
 * The board needs to be able to report this with no additional
 * dependencies to connected peripherals.
 *
 * It is expected, that the testing station opens printer's serial port at 115200 bauds to obtain these messages.
 * Beware: previous attempts to writing these messages onto USB CDC log destination (baudrate 57600) resulted
 * in cross-linked messages because the logging subsystem intentionally has no prevention (locks/mutexes) against such a situation.
 * Therefore the only reliable output is the "Marlin's" serial output (before Marlin is actually started)
 * as nothing else is actually using this serial line (therefore no cross-linked messages can appear at this spot),
 * and Marlin itself is guaranteed to not have been started by order of startup task initialization
 */
static void manufacture_report() {
    // The first '\n' is just a precaution - terminate any partially printed message from Marlin if any
    static const uint8_t intro[] = "\nbootstrap finished\nfirmware version: ";

    static_assert(sizeof(intro) > 1); // prevent accidental buffer underrun below
    SerialUSB.write(intro, sizeof(intro) - 1); // -1 prevents from writing the terminating \0 onto the serial line
    SerialUSB.write(reinterpret_cast<const uint8_t *>(project_version_full), strlen(project_version_full));
    SerialUSB.write('\n');
}

static void manufacture_report_endless_loop() {
    // ESP reset (needed for XL, since it has embedded ESP)
    HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_RESET);

    constexpr const uint8_t endl = '\n';
    constexpr const char *str_fw = "FW:";
    while (true) {
        HAL_UART_Transmit(&UART_HANDLE_FOR(esp), reinterpret_cast<const uint8_t *>(str_fw), strlen(str_fw), 1000);
        HAL_UART_Transmit(&UART_HANDLE_FOR(esp), reinterpret_cast<const uint8_t *>(project_version_full), strlen(project_version_full), 1000);
        HAL_UART_Transmit(&UART_HANDLE_FOR(esp), &endl, sizeof(endl), 1000);
        osDelay(500); // tester needs 500ms, do not change this value!
    }
}

#if ENABLED(RESOURCES()) && ENABLED(BOOTLOADER_UPDATE())
// Return TRUE if bootloader was updated -> in this case we have to reset the system, because important data addresses could be moved
static bool bootloader_update() {
    if (buddy::bootloader::needs_update()) {
        buddy::bootloader::update(
            [](int percent_done, buddy::bootloader::UpdateStage stage) {
                const char *stage_description = nullptr;
                switch (stage) {
                case buddy::bootloader::UpdateStage::LookingForBbf:
                    stage_description = "Looking for BBF...";
                    break;
                case buddy::bootloader::UpdateStage::PreparingUpdate:
                case buddy::bootloader::UpdateStage::Updating:
                    stage_description = "Updating bootloader";
                    break;
                default:
                    bsod("unreachable");
                }

                if (gui_bootstrap_screen_set_state(percent_done, stage_description)) {
                    log_info(Buddy, "Bootloader update progress %s (%i %%)", stage_description, percent_done);
                }
            });
        return true;
    }
    return false;
}
#endif

static void resources_update() {
    if (!buddy::resources::has_resources(buddy::resources::revision::standard)) {
        buddy::resources::bootstrap(
            buddy::resources::revision::standard, [](int percent_done, buddy::resources::BootstrapStage stage) {
                const char *stage_description = nullptr;
                switch (stage) {
                case buddy::resources::BootstrapStage::LookingForBbf:
                    stage_description = "Looking for BBF...";
                    break;
                case buddy::resources::BootstrapStage::PreparingBootstrap:
                    stage_description = "Preparing";
                    break;
                case buddy::resources::BootstrapStage::CopyingFiles:
                    stage_description = "Installing";
                    break;
                default:
                    bsod("unreachable");
                }

                if (gui_bootstrap_screen_set_state(percent_done, stage_description)) {
                    log_info(Buddy, "Bootstrap progress %s (%i %%)", stage_description, percent_done);
                }
            });
    }
    TaskDeps::provide(TaskDeps::Dependency::resources_ready);
}

extern "C" void main_cpp(void) {
    // save and clear reset flags
    HAL_RCC_CSR = RCC->CSR;
    __HAL_RCC_CLEAR_RESET_FLAGS();

    // Initialize sound instance now with its default settings
    // to be able to service sound related calls from interrupts.
    Sound::getInstance();

    hw_gpio_init();
    hw_dma_init();

    // ADC/DMA
    hw_adc1_init();
    adcDma1.init();

#if PRINTER_IS_PRUSA_XL()
    // Read Sandwich hw revision
    SandwichConfiguration::Instance();
#endif

#ifdef HAS_ADC3
    hw_adc3_init();
    adcDma3.init();
#endif

#if BOARD_IS_BUDDY() || BOARD_IS_XBUDDY()
    hw_tim1_init();
#endif

#if HAS_PHASE_STEPPING()
    hw_tim13_init();
#endif

#if HAS_BURST_STEPPING()
    hw_tim8_init();
#endif

    hw_tim14_init();

    SPI_INIT(flash);
    // initialize SPI flash
    w25x_spi_assign(&SPI_HANDLE_FOR(flash));
    if (!w25x_init()) {
        bsod("failed to initialize ext flash");
    }

    const bool want_error_screen = (dump_is_valid() && !dump_is_displayed()) || (message_is_valid() && message_get_type() != MsgType::EMPTY && !message_is_displayed());

#if BUDDY_ENABLE_CONNECT()
    // On a place shared for both code branches, so we have just one connectTask buffer.
    osThreadCCMDef(connectTask, want_error_screen ? StartConnectTaskError : StartConnectTask, TASK_PRIORITY_CONNECT, 0, 2336);
#endif

#if HAS_NFC()
    nfc::turn_off();
#endif

#if PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_MK3_5()
    /*
     * MK3.5 HW detected on MK4 firmware or vice versa
       Ignore the check in production (tester_mode), the xBuddy's connected peripherals are safe in this mode.
     */
    if (buddy::hw::Configuration::Instance().is_fw_incompatible_with_hw() && !running_in_tester_mode()) {
        const auto &error = find_error(ErrCode::WARNING_DIFFERENT_FW_REQUIRED);
        crash_dump::force_save_message_without_dump(crash_dump::MsgType::FATAL_WARNING, static_cast<uint16_t>(error.err_code), error.err_text, error.err_title);
        hwio_safe_state();
        init_error_screen();
        return;
    }
#endif

    /*
     * If we have BSOD or red screen we want to have as small boot process as we can.
     * We want to init just xflash, display and start gui task to display the bsod or redscreen
     */
    if (want_error_screen) {
        hwio_safe_state();
        init_error_screen();

#if BUDDY_ENABLE_WUI() && BUDDY_ENABLE_CONNECT()
        // We want to send the redscreen/bluescreen/error to Connect to show there.
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
            UART_INIT(esp);
            // Needed for certificate verification
            hw_rtc_init();
            // Needed for SSL random data
            hw_rng_init();

            // We can't flash ESP while showing error screen as there is no bootstrap progressbar.
            // Let's pretend that flashing was successful in order to enable Wi-Fi.
            skip_esp_flashing();

            TaskDeps::wait(TaskDeps::Tasks::network);
            start_network_task(/*allow_full=*/false);
            // definition and creation of connectTask
            TaskDeps::wait(TaskDeps::Tasks::connect);
            connectTaskHandle = osThreadCreate(osThread(connectTask), NULL);
        }
#endif
        return;
    }
    bsod_mark_shown(); // BSOD would be shown, allow new BSOD dump

    logging_init();
    TaskDeps::components_init();

#if (BOARD_IS_BUDDY())
    hw_uart1_init();
#endif

#if BOARD_IS_BUDDY() || BOARD_IS_XBUDDY()
    hw_tim3_init();
#endif

#if HAS_GUI()
    SPI_INIT(lcd);
#endif

#if BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY()
    I2C_INIT(usbc);
#endif

#if HAS_TOUCH()
    I2C_INIT(touch);
#endif

#if (BOARD_IS_XBUDDY())
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

#if BUDDY_ENABLE_WUI()
    UART_INIT(esp);
#endif

#if HAS_MMU2()
    UART_INIT(mmu);
#endif

#if HAS_GUI() && !(BOARD_IS_XLBUDDY())
    hw_tim2_init(); // TIM2 is used to generate buzzer PWM, except on XL. Not needed without display.
#endif

#if HAS_SIDE_LEDS()
    hw_init_spi_side_leds();
#endif

#if HAS_PUPPIES()
    UART_INIT(puppies);
    buddy::puppies::PuppyBus::Open();
#endif

    hw_rtc_init();
    hw_rng_init();

    // ESP flashing can start fairly early in the boot process.
    // On printers without embedded ESP32 we need to upload stub to enable verification.
    // This would take some seconds, which we can hide here.
    // Only after we find out that we actually need to flash the firmware we wait
    // for the bootstrap resources and take over the progress bar.
    // And as always, we need to prevent interactions with the UART in tester mode.
    if (!running_in_tester_mode()) {
        start_flash_esp_task();
    }

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
    if (status == config_store_ns::InitResult::cold_start || status == config_store_ns::InitResult::migrated_from_old) {
        // this means we are either starting from defaults or after a FW upgrade -> invalidate the XFLASH dump, since it is not relevant anymore
        dump_reset();
    }

    // Restore sound settings from eeprom
    Sound::getInstance().restore_from_eeprom();

    wdt_iwdg_warning_cb = iwdg_warning_cb;

    filesystem_init();

    if (option::has_gui) {
        osThreadStaticDef(displayTask, StartDisplayTask, TASK_PRIORITY_DISPLAY_TASK, 0, displayTask_stacksz, displayTask_buffer, &displayTask_control);
        displayTaskHandle = osThreadCreate(osThread(displayTask), NULL);
    }
    // wait for gui to init and render loading screen before starting flashing. We need to init bootstrap screen so we can send process percentage to it. Also it would look laggy without it.
    TaskDeps::wait(TaskDeps::Tasks::bootstrap_start);

#if ENABLED(RESOURCES()) && ENABLED(BOOTLOADER_UPDATE())
    if (bootloader_update()) {
        // Wait a while, before restart (this prevents some older board without appendix to enter internal bootloader on reset)
        osDelay(300);
        __disable_irq();
        HAL_NVIC_SystemReset();
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
    } else {
        manufacture_report(); // TODO erase this after all printers use manufacture_report_endless_loop (== ESP UART)
    }

#if (BOARD_IS_BUDDY())
    uart2.Open();
#endif

#if (BOARD_IS_XBUDDY())
    #if !HAS_PUPPIES()
    uart6.Open();
    #endif
#endif

#if HAS_MMU2()
    // mmu2 is normally serviced from the marlin thread
    // so execute it before the defaultTask is created to prevent race conditions
    if (config_store().mmu2_enabled.get()) {
        MMU2::mmu2.Start();
    }
#endif

#if HAS_I2C_EXPANDER()
    // I2C IO Expander have to be initialized after Configuration Store
    buddy::hw::io_expander2.initialize();
#endif

    osThreadCCMDef(defaultTask, StartDefaultTask, TASK_PRIORITY_DEFAULT_TASK, 0, 1152);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

#if ENABLED(POWER_PANIC)
    power_panic::check_ac_fault_at_startup();
    /* definition and creation of acFaultTask */
    osThreadCCMDef(acFaultTask, power_panic::ac_fault_task_main, TASK_PRIORITY_AC_FAULT, 0, 80);
    power_panic::ac_fault_task = osThreadCreate(osThread(acFaultTask), NULL);
#endif

#if HAS_PUPPIES()
    buddy::puppies::start_puppy_task();
#endif

#if BUDDY_ENABLE_WUI()
    // In tester mode ESP UART is being used to talk to the testing station,
    // thus it must not be used for the ESP -> no networking tasks shall be started.
    if (!running_in_tester_mode()) {
        TaskDeps::wait(TaskDeps::Tasks::network);
        start_network_task(/*allow_full=*/true);
    }
#endif

#if BUDDY_ENABLE_CONNECT()
    #if !BUDDY_ENABLE_WUI()
        // FIXME: We should be able to split networking to the lower-level network part and the Link part. Currently, both are done through WUI.
        #error "Can't have connect without WUI"
    #endif
    // In tester mode ESP UART is being used to talk to the testing station,
    // thus it must not be used for the ESP -> no networking tasks shall be started.
    if (!running_in_tester_mode()) {
        // definition and creation of connectTask
        TaskDeps::wait(TaskDeps::Tasks::connect);
        connectTaskHandle = osThreadCreate(osThread(connectTask), NULL);
    }
#endif

    // There is no point in initializing syslog before networking is up
    TaskDeps::wait(TaskDeps::Tasks::syslog);
    logging::syslog_reconfigure();
    metrics_reconfigure();

    if constexpr (option::filament_sensor != option::FilamentSensor::no) {
        /* definition and creation of measurementTask */
        osThreadCCMDef(measurementTask, StartMeasurementTask, TASK_PRIORITY_MEASUREMENT_TASK, 0, 620);
        osThreadCreate(osThread(measurementTask), NULL);
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {

#if HAS_GUI()
    if (hspi == &SPI_HANDLE_FOR(lcd)) {
        display::spi_tx_complete();
    }
#endif

    if (hspi == &SPI_HANDLE_FOR(flash)) {
        w25x_spi_transfer_complete_callback();
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {

#if HAS_GUI()
    if (hspi == &SPI_HANDLE_FOR(lcd)) {
        display::spi_rx_complete();
    }
#endif

    if (hspi == &SPI_HANDLE_FOR(flash)) {
        w25x_spi_receive_complete_callback();
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
#if (BOARD_IS_BUDDY())
    if (huart == &huart2) {
        uart2.WriteFinishedISR();
    }
#endif

#if HAS_PUPPIES()
    if (huart == &UART_HANDLE_FOR(puppies)) {
        buddy::puppies::PuppyBus::bufferedSerial.WriteFinishedISR();
    }
#endif

#if (BOARD_IS_XBUDDY())
    #if !HAS_PUPPIES()
    if (huart == &huart6) {
        //        log_debug(Buddy, "HAL_UART6_TxCpltCallback");
        uart6.WriteFinishedISR();
        #if HAS_MMU2()
                // instruct the RS485 converter, that we have finished sending data and from now on we are expecting a response from the MMU
                // set to high in hwio_pindef.h
                // buddy::hw::RS485FlowControl.write(buddy::hw::Pin::State::high);
        #endif
    }
    #endif
#endif
    if (huart == &UART_HANDLE_FOR(esp)) {
        return espif_tx_callback();
    }
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {

#if (BOARD_IS_BUDDY())
    if (huart == &huart2) {
        uart2.FirstHalfReachedISR();
    }
#endif

#if HAS_PUPPIES()
    if (huart == &UART_HANDLE_FOR(puppies)) {
        buddy::puppies::PuppyBus::bufferedSerial.FirstHalfReachedISR();
    }
#endif

#if (BOARD_IS_XBUDDY())
    #if !HAS_PUPPIES()
    if (huart == &huart6) {
        uart6.FirstHalfReachedISR();
    }
    #endif
#endif
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
#if (BOARD_IS_BUDDY())
    if (huart == &huart2) {
        uart2.SecondHalfReachedISR();
    }
#endif

#if HAS_PUPPIES()
    if (huart == &UART_HANDLE_FOR(puppies)) {
        buddy::puppies::PuppyBus::bufferedSerial.SecondHalfReachedISR();
    }
#endif

#if (BOARD_IS_XBUDDY())
    #if !HAS_PUPPIES()
    if (huart == &huart6) {
        uart6.SecondHalfReachedISR();
    }
    #endif
#endif
}

void StartDefaultTask([[maybe_unused]] void const *argument) {
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

#if BUDDY_ENABLE_CONNECT()
void StartConnectTask([[maybe_unused]] void const *argument) {
    connect_client::run();
}

void StartConnectTaskError([[maybe_unused]] void const *argument) {
    connect_client::run_error();
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
    crash_dump::save_message(crash_dump::MsgType::IWDGW, 0, nullptr, nullptr);
    trigger_crash_dump();
}

void init_error_screen() {
#if HAS_TOUCH
    touchscreen.disable_till_reset();
#endif

    if constexpr (option::has_gui) {
        // init lcd spi and timer for buzzer
        SPI_INIT(lcd);

#if !(_DEBUG)
    #if HAS_GUI() && !(BOARD_IS_XLBUDDY())
        hw_tim2_init(); // TIM2 is used to generate buzzer PWM, except on XL. Not needed without display.
    #endif
#endif

        init_only_littlefs();

        osThreadStaticDef(displayTask, StartErrorDisplayTask, TASK_PRIORITY_DISPLAY_TASK, 0, displayTask_stacksz, displayTask_buffer, &displayTask_control);
        displayTaskHandle = osThreadCreate(osThread(displayTask), NULL);
    }
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

static void enable_dfu_entry() {
#ifdef BUDDY_ENABLE_DFU_ENTRY
    // check whether user requested to enter the DFU mode
    // this has to be checked after having
    //  1) initialized access to the backup domain
    //  2) having initialized related clocks (SystemClock_Config)
    if (sys_dfu_requested()) {
        sys_dfu_boot_enter();
    }
#endif
}

static void eeprom_init_i2c() {
    I2C_INIT(eeprom);
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
    eeprom_init_i2c();

    // init eeprom module itself
    {
        freertos::CriticalSection critical_section;
        st25dv64k_init(); // init NFC+eeprom chip

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

    // other MCU setup
    enable_trap_on_division_by_zero();
    enable_backup_domain();
    enable_segger_sysview();
    enable_dfu_entry();

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
