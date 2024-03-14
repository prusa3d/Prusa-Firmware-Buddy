/// @file usb_cdc.h
#pragma once
#include <stdint.h>

namespace modules {

/// The usb namespace provides all necessary facilities related to the USB interface.
namespace usb {

class CDC {
public:
    constexpr inline CDC() = default;

    /// Calls USB_Init from the LUFA library
    void Init();

    /// Calls USB_USBTask from the LUFA library - basically takes care about the runtime of USB CDC operation
    void Step();
};

/// The one and only instance of USB CDC in the FW
extern CDC cdc;

} // namespace usb
} // namespace modules

namespace mu = modules::usb;
