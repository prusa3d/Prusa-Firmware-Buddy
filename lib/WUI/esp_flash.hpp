#include <cstdint>

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

    struct esp_fw_entry {
        uintptr_t address;
        const char *filename;
        size_t size;
    };

    ESPFlash();

    // Flash/sync ESP FW
    State flash();

private:
    State flash_part(esp_fw_entry &fwpart);
    void update_progress();

    State state;
    size_t total_size;
    size_t total_read;
};
