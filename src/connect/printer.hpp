#pragma once

#include <otp.hpp>
#include <common/shared_buffer.hpp>
#include <Marlin/src/inc/MarlinConfigPre.h>

#include <cstdint>
#include <cstddef>
#include <tuple>
#include <optional>

#include "printer_type.hpp"
#include <state/printer_state.hpp>

#include <option/has_mmu2.h>
#include <option/has_toolchanger.h>
#if HAS_MMU2()
    #include <Marlin/src/feature/prusa/MMU2/mmu2_mk4.h>
#endif

#include "../../lib/Marlin/Marlin/src/core/macros.h"

namespace connect_client {

class Printer {
public:
    struct PrinterInfo {
        static constexpr size_t SER_NUM_STR_LEN = 19;
        static constexpr size_t SER_NUM_BUFR_LEN = (SER_NUM_STR_LEN + 1);

        static constexpr size_t FINGERPRINT_SIZE = 50;
        static constexpr size_t FINGERPRINT_BUFF_LEN = (FINGERPRINT_SIZE + 1);
        static constexpr size_t FINGERPRINT_HDR_SIZE = 16;

        bool appendix;
        const char *firmware_version;
        serial_nr_t serial_number;
        char fingerprint[FINGERPRINT_BUFF_LEN];
    };

    struct SlotInfo {
        const char *material = nullptr;
        float temp_nozzle = 0;
        uint16_t print_fan_rpm = 0;
        uint16_t heatbreak_fan_rpm = 0;
    };

    static constexpr size_t X_AXIS_POS = 0;
    static constexpr size_t Y_AXIS_POS = 1;
    static constexpr size_t Z_AXIS_POS = 2;

#if HAS_MMU2() || HAS_TOOLCHANGER()
    static constexpr size_t NUMBER_OF_SLOTS = 5;
#else
    static constexpr size_t NUMBER_OF_SLOTS = 1;
#endif

#if ENABLED(CANCEL_OBJECTS)
    static constexpr size_t CANCEL_OBJECT_NAME_LEN = 32;
    static constexpr size_t CANCEL_OBJECT_NAME_COUNT = 16;
#endif

    class Params {
    private:
        // Living in the Printer we come from
        const std::optional<BorrowPaths> &paths;

    public:
        Params(const std::optional<BorrowPaths> &paths);
        std::array<SlotInfo, NUMBER_OF_SLOTS> slots;
#if HAS_MMU2()
        MMU2::Version mmu_version = { 0, 0, 0 };
#endif
        uint32_t progress_code = 0;
        char command_code = 0;
        size_t number_of_slots = 0;
        // Note: the 1 is used as default Slot
        // in case neither MMU nor toolchanger is present
        uint8_t active_slot = 1;
        bool mmu_enabled = false;
        float temp_bed = 0;
        float target_nozzle = 0;
        float target_bed = 0;
        float pos[4] = { 0, 0, 0, 0 };
        float filament_used = 0;
        // FIXME: We should handle XL with up to 5 nozzles, but the network protocol
        // does not support it as of now, so for the time being we just send the first one.
        float nozzle_diameter = 0;
        // Note: These strings live in a shared buffer in the real implementation. As a result:
        // * These are NULL unless paths was passed to the constructor.
        // * They get invalidated by calling drop_paths or new renew() on the printer.
        const char *job_path() const;
        const char *job_lfn() const;
        uint16_t flow_factor = 0;
        uint16_t job_id = 0;
        uint16_t print_speed = 0;
        uint32_t print_duration = 0;
        uint32_t time_to_end = 0;
        uint32_t time_to_pause = 0;
        uint8_t progress_percent = 0;
        bool has_usb = false;
        bool has_job = false;
        uint64_t usb_space_free = 0;
        PrinterVersion version = { 0, 0, 0 };
        printer_state::StateWithDialog state = printer_state::DeviceState::Unknown;
#if ENABLED(CANCEL_OBJECTS)
        size_t cancel_object_count = 0;
        uint64_t cancel_object_mask = 0;
#endif

        uint32_t telemetry_fingerprint(bool include_xy_axes) const;
        uint32_t state_fingerprint() const;
    };

    struct Config {
        static constexpr size_t CONNECT_URL_LEN = 35;
        static constexpr size_t CONNECT_URL_BUF_LEN = (CONNECT_URL_LEN + 1);
        static constexpr size_t CONNECT_TOKEN_LEN = 20;
        static constexpr size_t CONNECT_TOKEN_BUF_LEN = (CONNECT_TOKEN_LEN + 1);

        char host[CONNECT_URL_BUF_LEN] = "";
        char token[CONNECT_TOKEN_BUF_LEN] = "";
        uint16_t port = 0;
        bool tls = true;
        bool enabled = false;
        // Used only through loading.
        bool loaded = false;

        uint32_t crc() const;
    };

    enum class Iface {
        Ethernet,
        Wifi,
    };

    struct NetInfo {
        uint8_t ip[4];
        uint8_t mac[6];
    };

    struct NetCreds {
        static constexpr size_t SSID_BUF = 33;
        static constexpr size_t KEY_BUF = 17;
        char ssid[SSID_BUF];
        char pl_password[KEY_BUF];
    };

    enum class JobControl {
        Pause,
        Resume,
        Stop,
    };

protected:
    PrinterInfo info;
    virtual Config load_config() = 0;

private:
    // For checking if config changed. We ignore the 1:2^32 possibility of collision.
    uint32_t cfg_fingerprint = 0;

public:
    virtual void renew(std::optional<SharedBuffer::Borrow> paths) = 0;
    virtual void drop_paths() = 0;
    const PrinterInfo &printer_info() const {
        return info;
    }

    virtual Params params() const = 0;
    virtual std::optional<NetInfo> net_info(Iface iface) const = 0;
    virtual NetCreds net_creds() const = 0;
    virtual bool job_control(JobControl) = 0;
    virtual bool start_print(const char *path) = 0;
    // Deletes a file.
    //
    // returns nullptr on success, message with reason of failure otherwise
    virtual const char *delete_file(const char *path) = 0;
    // Enqueues a gcode command (single one).
    //
    // FIXME: This takes care only of submitting the gcode. If it returns Submitted,
    // it sits in the queue, but it doesn't mean it has been executed.
    enum class GcodeResult {
        Submitted,
        Later,
        Failed,
    };
    virtual GcodeResult submit_gcode(const char *gcode) = 0;
    virtual bool set_ready(bool ready) = 0;
    virtual bool is_printing() const = 0;
    // Is the printer in (hard) error?
    //
    // If so, most commands, actions, etc, are blocked.
    virtual bool is_in_error() const = 0;
    virtual bool is_idle() const = 0;
    virtual uint32_t cancelable_fingerprint() const = 0;
#if ENABLED(CANCEL_OBJECTS)
    virtual const char *get_cancel_object_name(char *buffer, size_t size, size_t index) const = 0;
#endif
    // Turn connect on and set the token.
    //
    // Part of registration.
    //
    // (The other config ‒ hostname, port, … ‒ are left unchanged).
    //
    // (Not const char * for technical reasons).
    virtual void init_connect(const char *token) = 0;

    // Does not return if successful
    virtual void reset_printer() = 0;

    virtual const char *dialog_action(uint32_t dialog_id, Response response) = 0;

    // Returns a newly reloaded config and a flag if it changed since last load
    // (unless the reset_fingerprint is set to false, in which case the flag is
    // kept).
    std::tuple<Config, bool> config(bool reset_fingerprint = true);

    virtual ~Printer() = default;

    uint32_t info_fingerprint() const;
};

} // namespace connect_client
