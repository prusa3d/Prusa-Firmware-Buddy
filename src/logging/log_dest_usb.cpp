#include "log_dest_usb.h"
#include "tusb.h"
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

    // do not log USB-related messages to prevent infinite cycle
    if (event->component == &LOG_COMPONENT(USBDevice)) {
        return;
    }

    destination->log_format_fn(event, usb_put_char, NULL);
    tud_cdc_write_str("\r\n");
    tud_cdc_write_flush();
}
