#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_msc.h"

#include "fatfs.h"
#include "media.h"
#include <device/board.h>
#include "common/timing.h"

#include <atomic>
#include "usbh_async_diskio.hpp"

LOG_COMPONENT_REF(USBHost);
USBH_HandleTypeDef hUsbHostHS;
ApplicationTypeDef Appli_state = APPLICATION_IDLE;

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
        if (!phost->stealth_reset) {
            Appli_state = APPLICATION_DISCONNECT;
#ifdef USBH_MSC_READAHEAD
            usbh_msc_readahead.disable();
#endif
            media_set_removed();
            f_mount(0, (TCHAR const *)USBHPath, 1); // umount
            connected_at_startup = false;
        }
        break;

    case HOST_USER_CLASS_ACTIVE: {
        if (!phost->stealth_reset) {

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
        } else {
            phost->stealth_reset = false;
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

// performs a usb reset (including disconnecting the power supply for the USB device)
// and waits for its reconnection, the operation is masked for higher application layers
void USBH_MSC_StealthReset(USBH_HandleTypeDef *phost, uint8_t lun) {
    log_info(USBHost, "USBH stealth reset");
    phost->stealth_reset = true;
    USBH_Stop(phost);
    osDelay(150);
    USBH_Start(phost);
    for (auto i = 100; phost->stealth_reset && i; --i) { // total delay 10s
        osDelay(100);
    }
    phost->stealth_reset = false;
    auto success = USBH_MSC_UnitIsReady(phost, lun);
    if (!success) {
        USBH_UserProcess(&hUsbHostHS, HOST_USER_DISCONNECTION);
    }
    log_info(USBHost, "USBH stealth reset finished: %s", success ? "SUCCESS" : "FAIL");
}

bool device_connected_at_startup() {
    return connected_at_startup;
}
