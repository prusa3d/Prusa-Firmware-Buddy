#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_msc.h"

#include <buddy/fatfs.h>
#include <device/board.h>
#include "common/timing.h"

#include <atomic>
#include <state/printer_state.hpp>
#include "usbh_async_diskio.hpp"
#include "marlin_client.hpp"
#include <logging/log.hpp>

LOG_COMPONENT_REF(USBHost);
USBH_HandleTypeDef hUsbHostHS;

static std::atomic<bool> media_inserted = false;
static std::atomic<bool> media_inserted_since_startup = false;
static uint32_t media_on_startup_detection_timeout = 0;

namespace usb_host {

bool is_media_inserted() {
    return media_inserted;
}

bool is_media_inserted_since_startup() {
    return media_inserted_since_startup;
}

void disable_media() {
    media_inserted = false;
}

} // namespace usb_host

namespace usbh_power_cycle {
// USB communication problems may occur at the physical layer. (emc interference, etc.)
// In the CPU, this will cause the HAL_HCD_Port Disabled callback or timeout in the io operation in the USBH_MSC_WorkerTask

// Functions in this namespace try to work around this by reinitializing the usbh stack (including turning off power on the bus)
// The procedure is 10ms pause => deinit usb => 150ms pause => init usb

// if printing was in progress:
//   if the usb is successfully initialized within 5s, it will connect automatically,
//   otherwise the "USB drive or file error" warning will appear.
// network transfers should continue (it's actually a new flash insertion)
// other operations may fail (highly unlikely situation)

// timer to handle the restart procedure
TimerHandle_t restart_timer;
void restart_timer_callback(TimerHandle_t);

uint32_t block_one_click_print_until_ms = 0;

enum class RecoveryPhase : uint_fast8_t {
    idle,

    /// USB has been turned off and will be turned back on at the end of this phase.
    restarting_usb,

};

std::atomic<RecoveryPhase> recovery_phase = RecoveryPhase::idle;

// Initialize FreeRTOS timer
void init() {
    static StaticTimer_t restart_timer_buffer;
    restart_timer = xTimerCreateStatic("USBHRestart", 10, pdFALSE, 0, restart_timer_callback, &restart_timer_buffer);
}

// callback from USBH_MSC_Worker when an io error occurs => start the restart procedure
void io_error() {
    if (recovery_phase == RecoveryPhase::idle) {
        xTimerChangePeriod(restart_timer, 10, portMAX_DELAY);
    }
}

// callback from isr => start the restart procedure
void port_disabled() {
    if (recovery_phase == RecoveryPhase::idle) {
        xTimerChangePeriodFromISR(restart_timer, 10, nullptr);
    }
}

// called from SVC task
void restart_timer_callback(TimerHandle_t) {
    switch (recovery_phase) {

    case RecoveryPhase::idle: {
        // If the phase is idle and the timer was called -> problem occured, start recovery process
        // This can either mean that the USB was disconnected,
        // or the communication had a problem (but the flash is still inserted and we need to recover)

        // Turn the USB off
        recovery_phase = RecoveryPhase::restarting_usb;
        USBH_Stop(&hUsbHostHS);

        // Call this timer again in 150 ms for the next phase
        xTimerChangePeriod(restart_timer, 150, portMAX_DELAY);
        break;
    }

    case RecoveryPhase::restarting_usb:
        // Prevent one click print from popping up when the drive initializes.
        // Most USBs should initialize within a few hundreds of ms.
        // Some drives take ~3 s to reinitialize, but we don't want to block the OCP for that long.
        // If user disconnects and reconnects the drive, he can do it faster than that and we want the OCP to trigger.
        block_one_click_print_until_ms = ticks_ms() + 800;

        // Reinitialize the USB host low-level driver
        // This seems to fix BFW-5333
        USBH_LL_DeInit(&hUsbHostHS);
        USBH_LL_Init(&hUsbHostHS);

        USBH_Start(&hUsbHostHS);

        // We're done here
        recovery_phase = RecoveryPhase::idle;
        xTimerStop(restart_timer, portMAX_DELAY);

        break;
    }
}

bool block_one_click_print() {
    return block_one_click_print_until_ms > ticks_ms();
}

} // namespace usbh_power_cycle

void MX_USB_HOST_Init(void) {
#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_SET);
    osDelay(200);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_RESET);
#else
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
    osDelay(200);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET);
#endif
    // A delay of 3000ms for detecting USB device (flash drive) was present at start
    media_on_startup_detection_timeout = ticks_ms() + 3000;

    usbh_power_cycle::init();
    if (USBH_Init(&hUsbHostHS, USBH_UserProcess, HOST_HS) != USBH_OK) {
        Error_Handler();
    }
    if (USBH_RegisterClass(&hUsbHostHS, USBH_MSC_CLASS) != USBH_OK) {
        Error_Handler();
    }
    if (USBH_Start(&hUsbHostHS) != USBH_OK) {
        Error_Handler();
    }
}

void USBH_UserProcess([[maybe_unused]] USBH_HandleTypeDef *phost, uint8_t id) {
    // don't detect device at startup when ticks_ms() overflows (every ~50 hours)
    if (media_on_startup_detection_timeout != 0 && ticks_ms() >= media_on_startup_detection_timeout) {
        media_on_startup_detection_timeout = 0;
    }

    switch (id) {

    case HOST_USER_DISCONNECTION:
        media_inserted = false;
        media_inserted_since_startup = false;
        f_mount(0, (TCHAR const *)USBHPath, 1); // umount
        break;

    case HOST_USER_CLASS_ACTIVE: {
        FRESULT result = f_mount(&USBHFatFS, (TCHAR const *)USBHPath, 0);
        if (result != FR_OK) {
            break;
        }

        media_inserted_since_startup = (media_on_startup_detection_timeout != 0);
        media_inserted = true;

        marlin_client::init_maybe();
        marlin_client::try_recover_from_media_error();
        break;
    }

    default:
        break;
    }
}
