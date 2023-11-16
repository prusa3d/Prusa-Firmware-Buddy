#include "log_dest_usb.h"
#include "tusb.h"
#include "FreeRTOS.h"
#include "stm32f4xx.h"
#include "usb_device.hpp"

static bool usb_logging_enabled = false;

void usb_log_enable() {
    usb_logging_enabled = true;
}

void usb_log_disable() {
    usb_logging_enabled = false;
}

static void usb_put_char(char character, [[maybe_unused]] void *arg) {
    tud_cdc_write_char(character);
}

void usb_log_event(log_destination_t *destination, log_event_t *event) {
    // check we are attached to a CDC interface
    if (!usb_logging_enabled) {
        return;
    }

    // check the CDC interface is being used by some terminal
    if (!tud_cdc_connected()) {
        return;
    }

    // do not log from interrupts (the usb stack requires sync primitives of freertos)
    if (xPortIsInsideInterrupt()) {
        return;
    }

    // do not log USB-related messages to prevent infinite cycle
    if (event->component == &LOG_COMPONENT(USBDevice)) {
        return;
    }

    // prevent (infinite) recursion within the usb
    // handler (for example, the usb stack might try to log something)
    if ((intptr_t)pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_USB_LOGGING_IDX) == 1) {
        return;
    }
    vTaskSetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_USB_LOGGING_IDX, (void *)1);

    destination->log_format_fn(event, usb_put_char, NULL);
    tud_cdc_write_str("\r\n");
    tud_cdc_write_flush();

    vTaskSetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_USB_LOGGING_IDX, (void *)0);
}
