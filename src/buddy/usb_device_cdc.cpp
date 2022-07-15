#include "tusb.h"
#include "USBSerial.h"
#include "usb_device.h"
#include "log_dest_usb.h"

void usb_cdc_switch_to_marlin() {
    usb_log_disable();
    tud_cdc_write_clear();
    SerialUSB.enable();
}

void usb_cdc_switch_to_logging() {
    SerialUSB.disable();
    tud_cdc_write_clear();
    usb_log_enable();
}

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *p_line_coding) {
    if (p_line_coding->bit_rate == 57600) {
        usb_cdc_switch_to_logging();
    } else {
        usb_cdc_switch_to_marlin();
    }
}
