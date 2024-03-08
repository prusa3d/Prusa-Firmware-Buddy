#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_msc.h"

#include "fatfs.h"
#include "media.hpp"
#include <device/board.h>
#include "common/timing.h"

#include <atomic>
#include "usbh_async_diskio.hpp"
#include "marlin_client.hpp"

LOG_COMPONENT_REF(USBHost);
USBH_HandleTypeDef hUsbHostHS;
ApplicationTypeDef Appli_state = APPLICATION_IDLE;

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

enum class Phase : uint_fast8_t {
    idle,
    power_off,
    power_on
};

std::atomic<Phase> phase = Phase::idle;
std::atomic<bool> printing_paused = false;
std::atomic<bool> trigger_usb_failed_dialog = true;

// Initialize FreeRTOS timer
void init() {
    static StaticTimer_t restart_timer_buffer;
    restart_timer = xTimerCreateStatic("USBHRestart", 10, pdFALSE, 0, restart_timer_callback, &restart_timer_buffer);
}

// callback from USBH_MSC_Worker when an io error occurs => start the restart procedure
void io_error() {
    if (phase == Phase::idle) {
        trigger_usb_failed_dialog = false;
        xTimerChangePeriod(restart_timer, 10, portMAX_DELAY);
    }
}

// callback from isr => start the restart procedure
void port_disabled() {
    if (phase == Phase::idle) {
        trigger_usb_failed_dialog = false;
        xTimerChangePeriodFromISR(restart_timer, 10, nullptr);
    }
}

// called from USBH_Thread
void msc_active() {
    if (phase == Phase::power_on) {
        xTimerStop(restart_timer, portMAX_DELAY);
        phase = Phase::idle;
        block_one_click_print_until_ms = ticks_ms() + 1000;

        // lazy initialization of marlin_client
        static bool marlin_client_initializated = false;
        if (!marlin_client_initializated) {
            marlin_client_initializated = true;
            marlin_client::init();
        }
        switch (media_print_get_state()) {
        case media_print_state_NONE:
            break;
        case media_print_state_PAUSED:
            marlin_client::print_resume();
            break;
        case media_print_state_PRINTING:
            marlin_client::media_print_reopen();
            trigger_usb_failed_dialog = true;
            break;
        }
    }
}

// called from SVC task
void restart_timer_callback(TimerHandle_t) {
    switch (phase) {
    case Phase::idle:
        phase = Phase::power_off;
        xTimerChangePeriod(restart_timer, 150, portMAX_DELAY);
        USBH_Stop(&hUsbHostHS);
        block_one_click_print_until_ms = ticks_ms() + 5000;
        break;
    case Phase::power_off:
        phase = Phase::power_on;
        xTimerChangePeriod(restart_timer, 5000, portMAX_DELAY);
        USBH_Start(&hUsbHostHS);
        break;
    case Phase::power_on:
        phase = Phase::idle;
        trigger_usb_failed_dialog = true;

        switch (media_print_get_state()) {
        case media_print_state_NONE:
        case media_print_state_PRINTING:
            break;
        case media_print_state_PAUSED:
            static bool marlin_client_initializated = false;
            // lazy initialization of marlin_client
            if (!marlin_client_initializated) {
                marlin_client_initializated = true;
                marlin_client::init();
            }
            marlin_client::set_warning(WarningType::USBFlashDiskError);
            break;
        }
        break;
    }
}

bool block_one_click_print() {
    return block_one_click_print_until_ms > ticks_ms();
}

} // namespace usbh_power_cycle

static uint32_t one_click_print_timeout { 0 };
static std::atomic<bool> connected_at_startup { false };

void MX_USB_HOST_Init(void) {
#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_RESET);
#else
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET);
#endif
    // A delay of 3000ms for detecting USB device (flash drive) was present at start
    one_click_print_timeout = ticks_ms() + 3000;

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
    if (one_click_print_timeout > 0 && ticks_ms() >= one_click_print_timeout) {
        one_click_print_timeout = 0;
    }

    switch (id) {
    case HOST_USER_SELECT_CONFIGURATION:
        break;

    case HOST_USER_DISCONNECTION:
        Appli_state = APPLICATION_DISCONNECT;
#ifdef USBH_MSC_READAHEAD
        usbh_msc_readahead.disable();
#endif
        media_set_removed();
        f_mount(0, (TCHAR const *)USBHPath, 1); // umount
        connected_at_startup = false;
        break;

    case HOST_USER_CLASS_ACTIVE: {
        Appli_state = APPLICATION_READY;
        FRESULT result = f_mount(&USBHFatFS, (TCHAR const *)USBHPath, 0);
        if (result == FR_OK) {
            if (one_click_print_timeout > 0 && ticks_ms() < one_click_print_timeout) {
                connected_at_startup = true;
            }
            media_set_inserted();
#ifdef USBH_MSC_READAHEAD
            usbh_msc_readahead.enable(USBHFatFS.pdrv);
#endif
            usbh_power_cycle::msc_active();
        } else {
            media_set_error(media_error_MOUNT);
        }
        break;
    }
    case HOST_USER_CONNECTION:
        Appli_state = APPLICATION_START;
        break;

    default:
        break;
    }
}

bool device_connected_at_startup() {
    return connected_at_startup;
}
