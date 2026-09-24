/// @file
#include <buddy/usb_device.hpp>

#include "stm32f4xx_hal.h"
#include "tusb.h"
#include <device/peripherals.hpp>
#include <logging/log.hpp>
#include "otp.hpp"
#include "buddy/priorities_config.h"
#include <tasks.hpp>
#include <timing.h>
#include <atomic>
#include <utils/string_builder.hpp>

LOG_COMPONENT_DEF(USBDevice, logging::Severity::info);

#define USBD_VID 0x2C99 /// Prusa Research Vendor ID

#define USBD_MANUFACTURER_STRING    "Prusa Research (prusa3d.com)"
#define USBD_VBUS_CHECK_INTERVAL_MS 1000

static serial_nr_t serial_nr;

#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    #define FUSB302B_INTERPOSER
    #include "FUSB302B.hpp"
    #include "hwio_pindef.h"

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    #include <device/dcd.h>
    #pragma GCC diagnostic pop

static std::atomic<bool> usb_vbus_state = false;
#endif

bool usb_device_attached() {
#ifdef FUSB302B_INTERPOSER
    return usb_vbus_state.load();
#else
    return tud_connected() && !tud_suspended();
#endif
}

static std::atomic<bool> usb_device_seen_v = false;

bool usb_device_seen() {
#ifdef FUSB302B_INTERPOSER
    // we can't query FUSB302B from random threads, rely on the USB thread for updates
    return usb_device_seen_v.load();
#else
    // check and update with the current connection status
    usb_device_seen_v.store(usb_device_seen_v.load() || usb_device_attached());
    return usb_device_seen_v.load();
#endif
}

void usb_device_clear() {
    usb_device_seen_v.store(false);
}

static void check_usb_connection() {
#ifdef FUSB302B_INTERPOSER
    if (buddy::hw::fsUSBCInt.read() == buddy::hw::Pin::State::low) {
        buddy::hw::FUSB302B::ClearVBUSIntFlag();

        bool vbus_status = buddy::hw::FUSB302B::ReadVBUSState();
        usb_vbus_state.store(vbus_status);
        log_info(USBDevice, "FUSB302B VBUS state change: %d", (int)vbus_status);

        if (!vbus_status) {
            if (dcd_connected(TUD_OPT_RHPORT)) {
                // VBUS off: trigger a disconnect
                tud_disconnect();
            }
        } else {
            usb_device_seen_v.store(true);
            if (!dcd_connected(TUD_OPT_RHPORT)) {
                // VBUS on: trigger connect
                tud_connect();
            }
        }
    }
#else
    usb_device_seen_v.store(usb_device_seen_v.load() || usb_device_attached());
#endif
}

// The descriptor of this USB device
static tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,

    // Use Interface Association Descriptor (IAD) for CDC
    // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = USBD_VID,
    .idProduct = 0, // Will be defined later
    .bcdDevice = 0x0100,

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01
};

void usb_device_task_run() {
#ifdef FUSB302B_INTERPOSER
    buddy::hw::FUSB302B::InitChip();
#endif

    GPIO_InitTypeDef GPIO_InitStruct;

    // configure data pins
    GPIO_InitStruct.Pin = USB_FS_N_Pin | USB_FS_P_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // enable USB peripheral clock
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

    // setup interrupt priority
    HAL_NVIC_SetPriority(OTG_FS_IRQn, ISR_PRIORITY_DEFAULT, 0);

    // disable vbus sensing
    USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;
    USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBUSBSEN;

    // init serial number
    // this function correctly initializes the data and null terminates them
    otp_get_serial_nr(serial_nr);

    // initialize tinyusb stack
    tusb_init();

    // wait for bus reset to complete if possible
    for (uint32_t start_ts = ticks_ms(); ticks_diff(ticks_ms(), start_ts) < 250;) {
        tud_task_ext(100, false);
        if (tud_speed_get() != TUSB_SPEED_INVALID) {
            break;
        }
    }

    // initialize the connection state
#ifdef FUSB302B_INTERPOSER
    usb_vbus_state.store(buddy::hw::FUSB302B::ReadVBUSState());
    usb_device_seen_v.store(usb_vbus_state.load());
#else
    // If bus RESET was seen without FUSB302B the link just came up on it's own
    usb_device_seen_v.store(tud_speed_get() != TUSB_SPEED_INVALID);
#endif
    TaskDeps::provide(TaskDeps::Dependency::usb_device_ready);

    // periodically check for disconnection
    while (true) {
        tud_task_ext(USBD_VBUS_CHECK_INTERVAL_MS, false);
        check_usb_connection();
    }
}

// This function is referenced from tusb_config.h file; do not change its signature
int __attribute__((format(__printf__, 1, 2)))
usb_device_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logging::_log_event_valist(logging::Severity::info, &LOG_COMPONENT(USBDevice), fmt, &args);
    va_end(args);
    return 0; // tinyusb ignores the result
}

// Invoked when received GET DEVICE DESCRIPTOR
// Application returns pointer to the descriptor
uint8_t const *tud_descriptor_device_cb(void) {
    desc_device.idProduct = PrinterModelInfo::current().usb_pid;
    return (uint8_t const *)&desc_device;
}

enum {
    INTERFACE_CDC = 0,
    INTERFACE_CDC_DATA,
    INTERFACE_COUNT
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT   0x02
#define EPNUM_CDC_IN    0x82

uint8_t const desc_fs_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, INTERFACE_COUNT, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(INTERFACE_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb([[maybe_unused]] uint8_t index) {
    return desc_fs_configuration;
}

/// Updated dynamically
static char product_str[32];

// Array of pointer to string descriptors
char const *string_desc_arr[] = {
    "\x09\x04", // 0: is supported language is English (0x0409)
    USBD_MANUFACTURER_STRING, // 1: Manufacturer
    product_str, // 2: Product
    serial_nr.begin(), // 3: Serials, should use chip ID
    "CDC", // 4: CDC Interface
};

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, [[maybe_unused]] uint16_t langid) {
    // note: Dthe 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if (index >= std::size(string_desc_arr)) {
        return NULL;
    }

    if (index == 2) {
        // Product string - needs to be dynamically updated
        StringBuilder sb(product_str);
        sb.append_string("Original Prusa ");
        sb.append_string(PrinterModelInfo::current().id_str);
    }

    const char *str = string_desc_arr[index];
    const uint8_t chr_count = strlen(str);

    static uint16_t desc_str[32];

    // first byte is length (including header), second byte is string type
    desc_str[0] = (2 * chr_count + 2) | (TUSB_DESC_STRING << 8);

    // convert ASCII string into UTF-16
    for (uint8_t i = 0; i < chr_count; i++) {
        desc_str[i + 1] = str[i];
    }

    return desc_str;
}

void tud_suspend_cb(bool /*remote_wakeup_en*/) {
    // Do not wait for timeout on SUSPEND, immediately switch TX to non-blocking mode
    cdcd_set_tx_ovr(TUD_OPT_RHPORT, true);
}
