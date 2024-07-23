#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "tusb.h"
#include "main.h"
#include "usb_device.hpp"
#include <logging/log.hpp>
#include "otp.hpp"
#include "buddy/priorities_config.h"
#include <ccm_thread.hpp>
#include <config_store/store_instance.hpp>
#include <tasks.hpp>
#include <timing.h>
#include <atomic>

LOG_COMPONENT_DEF(USBDevice, logging::Severity::info);

#define stringify(a)  _stringify(a)
#define _stringify(a) #a

#define USBD_VID 0x2C99 /// Prusa Research Vendor ID

/// Product ID
#if PRINTER_IS_PRUSA_MINI()
    #define USBD_PID 0x000C
#elif PRINTER_IS_PRUSA_MK4()
// !!! Changes based on extended printer type
#elif PRINTER_IS_PRUSA_MK3_5()
    #define USBD_PID 0x0017
#elif PRINTER_IS_PRUSA_iX()
    #define USBD_PID 0x0010
#elif PRINTER_IS_PRUSA_XL()
    #define USBD_PID 0x0011
#else
    #error "Unknown PRINTER_TYPE!"
#endif

#define USBD_LANGID_STRING          1033
#define USBD_MANUFACTURER_STRING    "Prusa Research (prusa3d.com)"
#define USBD_PRODUCT_STRING_FS      ("Original Prusa " PRINTER_MODEL)
#define USBD_SERIALNUMBER_STRING_FS "00000000001A"
#define USBD_VBUS_CHECK_INTERVAL_MS 1000

#define USB_SIZ_BOS_DESC 0x0C

#define USBD_STACK_SIZE (128 * 5)

static void usb_device_task_run(const void *);

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
        usb_device_log("FUSB302B VBUS state change: %d\n", (int)vbus_status);

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

osThreadCCMDef(usb_device_task, usb_device_task_run, TASK_PRIORITY_USB_DEVICE, 0, USBD_STACK_SIZE);
static osThreadId usb_device_task;

void usb_device_init() {
    usb_device_task = osThreadCreate(osThread(usb_device_task), NULL);
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
#ifdef USBD_PID
    .idProduct = USBD_PID,
#else
    .idProduct = 0, // Will be defined later
#endif
    .bcdDevice = 0x0100,

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01
};

static void usb_device_task_run(const void *) {
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
    static char buffer[128];
    int length = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    length = std::min<int>(length, sizeof(buffer) - 1);
    if (length < 0) {
        log_error(USBDevice, "log encoding issue");
    } else if (length >= 1) {
        buffer[length - 1] = 0; // remove newline (or last character if the print was clipped, but that's acceptable)
        log_info(USBDevice, "%s", buffer);
    }

    return length;
}

// Invoked when received GET DEVICE DESCRIPTOR
// Application returns pointer to the descriptor
uint8_t const *tud_descriptor_device_cb(void) {

#if PRINTER_IS_PRUSA_MK4()
    static constexpr EnumArray<ExtendedPrinterType, uint16_t, extended_printer_type_count> usb_pids {
        { ExtendedPrinterType::mk4, 0x000D },
        { ExtendedPrinterType::mk4s, 0x001a },
        { ExtendedPrinterType::mk3_9, 0x0015 },
    };
    desc_device.idProduct = usb_pids.get_fallback(config_store().extended_printer_type.get(), ExtendedPrinterType::mk4);

#elifndef USBD_PID
    #error USBD_PID must be defined!
#endif

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

// Array of pointer to string descriptors
char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
    USBD_MANUFACTURER_STRING, // 1: Manufacturer
    USBD_PRODUCT_STRING_FS, // 2: Product
    serial_nr.begin(), // 3: Serials, should use chip ID
    "CDC", // 4: CDC Interface
};

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, [[maybe_unused]] uint16_t langid) {
    static uint16_t desc_str[32];
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        // note: Dthe 0xEE index string is a Microsoft OS 1.0 Descriptors.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) {
            return NULL;
        }

        const char *str = string_desc_arr[index];
#if PRINTER_IS_PRUSA_MK4()
        if (index == 2) {
            switch (config_store().extended_printer_type.get()) {

            case ExtendedPrinterType::mk4:
                break;

            case ExtendedPrinterType::mk4s:
                str = "Original Prusa MK4S";
                break;

            case ExtendedPrinterType::mk3_9:
                str = "Original Prusa MK3.9";
                break;
            }
        }
#endif

        // cap at max char
        chr_count = strlen(str);
        if (chr_count > 31) {
            chr_count = 31;
        }

        // convert ASCII string into UTF-16
        for (uint8_t i = 0; i < chr_count; i++) {
            desc_str[1 + i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return desc_str;
}

void tud_suspend_cb(bool /*remote_wakeup_en*/) {
    // Do not wait for timeout on SUSPEND, immediately switch TX to non-blocking mode
    cdcd_set_tx_ovr(TUD_OPT_RHPORT, true);
}
