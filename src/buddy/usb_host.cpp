#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_msc.h"

#include "fatfs.h"
#include "media.hpp"
#include <device/board.h>
#include "common/timing.h"

#include <atomic>
#include "usbh_async_diskio.hpp"

LOG_COMPONENT_REF(USBHost);
USBH_HandleTypeDef hUsbHostHS;
ApplicationTypeDef Appli_state = APPLICATION_IDLE;

static uint32_t one_click_print_timeout { 0 };
static std::atomic<bool> connected_at_startup { false };

TimerHandle_t USBH_restart_timer;
enum class USBHRestartPhase : uint_fast8_t {
    stop,
    start
};

void USBH_restart_timer_callback(TimerHandle_t) {
    static USBHRestartPhase USBH_restart_phase = USBHRestartPhase::stop;
    switch (USBH_restart_phase) {
    case USBHRestartPhase::stop:
        log_info(USBHost, "USBH power cycle");
        USBH_restart_phase = USBHRestartPhase::start;
        xTimerChangePeriod(USBH_restart_timer, 150, portMAX_DELAY);
        USBH_Stop(&hUsbHostHS);
        break;
    case USBHRestartPhase::start:
        log_info(USBHost, "USBH power cycle complete");
        USBH_restart_phase = USBHRestartPhase::stop;
        USBH_Start(&hUsbHostHS);
    }
}

void USBH_restart_timer_init() {
    static StaticTimer_t USBH_restart_timer_buffer;
    USBH_restart_timer = xTimerCreateStatic("USBHRestart", 10, pdFALSE, 0, USBH_restart_timer_callback, &USBH_restart_timer_buffer);
}

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

    USBH_restart_timer_init();
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
