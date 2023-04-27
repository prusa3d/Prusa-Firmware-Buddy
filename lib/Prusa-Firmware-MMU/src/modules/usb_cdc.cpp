/// @file usb_cdc.cpp
#include "usb_cdc.h"
#include "../hal/cpu.h"
#include "../hal/watchdog.h"
#include "debug.h"

extern "C" {
#include "lufa_config.h"
#include "Descriptors.h"
#include "lufa/LUFA/Drivers/USB/USB.h"

/** Standard file stream for the CDC interface when set up, so that the virtual CDC COM port can be
 *  used like any regular character stream in the C APIs.
 */
static FILE USBSerialStream;

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface = {
    .Config = {
        .ControlInterfaceNumber = INTERFACE_ID_CDC_CCI,
        .DataINEndpoint = {
            .Address = CDC_TX_EPADDR,
            .Size = CDC_TXRX_EPSIZE,
            .Type = EP_TYPE_BULK,
            .Banks = 2,
        },
        .DataOUTEndpoint = {
            .Address = CDC_RX_EPADDR,
            .Size = CDC_TXRX_EPSIZE,
            .Type = EP_TYPE_BULK,
            .Banks = 2,
        },
        .NotificationEndpoint = {
            .Address = CDC_NOTIFICATION_EPADDR,
            .Size = CDC_NOTIFICATION_EPSIZE,
            .Type = EP_TYPE_INTERRUPT,
            .Banks = 1,
        },
    },
};

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void) {
    // dbg_modules_P(PSTR("EVENT_USB_Device_Connect"));
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void) {
    // dbg_modules_P(PSTR("EVENT_USB_Device_Disconnect"));
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void) {
    bool ConfigSuccess = true;
    ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

    // dbg_modules_fP(PSTR("EVENT_USB_Device_ConfigurationChanged:%S"), ConfigSuccess ? PSTR("ready") : PSTR("error"));
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void) {
    // dbg_modules_P(PSTR("EVENT_USB_Device_ControlRequest"));
    CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

/** CDC class driver callback function the processing of changes to the virtual
 *  control lines sent from the host..
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo) {
}

void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo) {
    if (CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 1200) {
        // *(uint16_t *)0x0800U = 0x7777; //old bootloader?
        *(uint16_t *)(RAMEND - 1) = 0x7777;
        hal::cpu::resetPending = true;
        hal::watchdog::Enable(hal::watchdog::configuration::compute(250));
    }
}
} // extern "C"

namespace modules {
namespace usb {

CDC cdc;

void CDC::Init() {
    USB_Init();

    /* Create a regular character stream for the interface so that it can be used with the stdio.h functions */
    CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);
    stdout = &USBSerialStream;
}

void CDC::Step() {
    /* Must throw away unused bytes from the host, or it will lock up while waiting for the device */
    CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
    USB_USBTask();
}

} // namespace usb

} // namespace modules
