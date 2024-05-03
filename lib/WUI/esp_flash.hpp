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

    class ProgressHook {
    public:
        virtual ~ProgressHook() = default;

        virtual void update_progress(ESPFlash::State, size_t current, size_t total) = 0;
    };

    // Flash/sync ESP FW
    State flash(ProgressHook &);

private:
    State flash_part(ProgressHook &, esp_fw_entry &fwpart);

    State state;
    size_t total_size;
    size_t total_read;
};
