#include <array>
#include <cstdint>

#include <printers.h>

class ESPFlash {
public:
    enum class State {
        Done,
        Init,
        Connected,
        Flashing,
        Checking,
        FlashError,
        NotConnected,
        WriteError,
        ReadError,
        WriteData,
        DataWritten,
    };

    struct Progress {
        State state;
        size_t bytes_total;
        size_t bytes_flashed;
    };

    static constexpr size_t files_to_upload = 3;
    static constexpr size_t buffer_length = 512;
    static constexpr auto retries = 3;

    struct esp_fw_entry {
        uintptr_t address;
        const char *filename;
        size_t size;
    };

    using firmware_set_t = std::array<esp_fw_entry, files_to_upload>;

#if PRINTER_IS_PRUSA_XL
    static constexpr firmware_set_t FIRMWARE_SET { { { .address = 0x08000, .filename = "/internal/res/esp32/partition-table.bin", .size = 0 },
        { .address = 0x01000ul, .filename = "/internal/res/esp32/bootloader.bin", .size = 0 },
        { .address = 0x10000ul, .filename = "/internal/res/esp32/uart_wifi.bin", .size = 0 } } };
#else
    static constexpr firmware_set_t FIRMWARE_SET { { { .address = 0x08000ul, .filename = "/internal/res/esp/partition-table.bin", .size = 0 },
        { .address = 0x00000ul, .filename = "/internal/res/esp/bootloader.bin", .size = 0 },
        { .address = 0x10000ul, .filename = "/internal/res/esp/uart_wifi.bin", .size = 0 } } };
#endif

    ESPFlash();

    // Flash/sync ESP FW
    State flash();
    // Get flash progress
    static Progress get_progress();
    // Raise error according to flash state
    static void fatal_err(const State state);

private:
    State flash_part(esp_fw_entry &fwpart);
    void update_progress();

    static Progress progress;
    firmware_set_t firmware_set;
    State state;
    size_t total_size;
    size_t total_read;
};
