#pragma once

#include "printers.h"

#include <otp.hpp>
#include <common/shared_buffer.hpp>
#include <inc/MarlinConfigPre.h>

#include <cstdint>
#include <cstddef>
#include <tuple>
#include <optional>

#include <netif_settings.h>
#include "printer_type.hpp"
#include <state/printer_state.hpp>
#include <device/board.h>
#include <connect/hostname.hpp>
#include <filament.hpp>
#include <filament_sensor_states.hpp>

#include <option/has_mmu2.h>
#include <option/has_toolchanger.h>
#if HAS_MMU2()
    #include <Marlin/src/feature/prusa/MMU2/mmu2_mk4.h>
#endif

namespace connect_client {

// NOTE: if you are changing this, change also the one in command.hpp,
//  it is at both places, otherwise it would create circular dependencies
using ToolMapping = std::array<std::array<uint8_t, EXTRUDERS>, EXTRUDERS>;
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
        std::array<char, filament_name_buffer_size> material = { 0 };
        float temp_nozzle = 0;
#if PRINTER_IS_PRUSA_iX()
        float temp_heatbreak = 0;
        std::optional<FilamentSensorState> extruder_fs_state;
        std::optional<FilamentSensorState> remote_fs_state;
#endif
        uint16_t print_fan_rpm = 0;
        uint16_t heatbreak_fan_rpm = 0;
        bool high_flow = false;
        bool hardened = false;
        float nozzle_diameter = 0;
    };

#if XL_ENCLOSURE_SUPPORT()
    struct EnclosureInfo {
        // in INFO
        bool present = false;
        bool enabled = false;
        bool printing_filtration = false;
        bool post_print = false;
        uint16_t post_print_filtration_time = 0;

        // in TELEMETRY
        int temp {};
        uint16_t fan_rpm {};
        int64_t time_in_use {};
    };
#endif

#if PRINTER_IS_PRUSA_COREONE() || defined(UNITTESTS)
    struct ChamberInfo {
        static constexpr int target_temp_unset = 0U; // agreed with the Connect team, that 0 maps to unset values
        uint32_t target_temp = target_temp_unset;
        uint16_t fan_1_rpm = 0;
        uint16_t fan_2_rpm = 0;
        static constexpr int8_t fan_pwm_target_unset = -1; // -1 means auto control, 0-100 is pwm percentage for manual control
        int8_t fan_pwm_target = fan_pwm_target_unset;
        int8_t led_intensity = 0;
    };
#endif

    static constexpr size_t X_AXIS_POS = 0;
    static constexpr size_t Y_AXIS_POS = 1;
    static constexpr size_t Z_AXIS_POS = 2;

#if HAS_MMU2() || HAS_TOOLCHANGER() || defined(UNITTESTS)
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
#if XL_ENCLOSURE_SUPPORT()
        EnclosureInfo enclosure_info;
#endif
#if PRINTER_IS_PRUSA_COREONE() || defined(UNITTESTS)
        ChamberInfo chamber_info;
#endif
#if HAS_MMU2()
        MMU2::Version mmu_version = { 0, 0, 0 };
#endif
        uint32_t progress_code = 0;
        char command_code = 0;
        // Which slots are available/enabled (bitfielt)
        // 0b00000001 for "ordinary" printers
        // 0b00011111 for MMU enabled
        // Something arbitrary for XL, depending on its heads available (note that they don't have to be "continuous")
        uint8_t slot_mask = 1;
        static_assert(8 * sizeof slot_mask >= NUMBER_OF_SLOTS);
        // Note: the 1 is used as default Slot
        // in case neither MMU nor toolchanger is present
        // 0 means "no tool active" (possible with MMU or toolchanger)
        // A 1-based index.
        uint8_t active_slot = 1;
        float temp_bed = 0;
#if PRINTER_IS_PRUSA_iX()
        float temp_psu = 0;
        float temp_ambient = 0;
#endif
        float target_nozzle = 0;
        float target_bed = 0;
        float pos[4] = { 0, 0, 0, 0 };
        float filament_used = 0;
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
        bool can_start_download = false;
        uint64_t usb_space_free = 0;
        PrinterVersion version = { 0, 0, 0 };
        printer_state::StateWithDialog state = printer_state::DeviceState::Unknown;
#if ENABLED(CANCEL_OBJECTS)
        size_t cancel_object_count = 0;
        uint64_t cancel_object_mask = 0;
#endif

        uint32_t telemetry_fingerprint(bool include_xy_axes) const;
        uint32_t state_fingerprint() const;
        uint8_t enabled_tool_cnt() const {
            return std::popcount(slot_mask);
        }
        // Either the active slot, if any, or the first available slot if no slot is active.
        uint8_t preferred_slot() const;
        // Either the active head, if any, or the first available one.
        //
        // This is the same as preferred_slot for XL (where tools and slots are
        // the same thing), but always returns 0 on other printers, including
        // ones with MMU (they have multiple filament slots, but just one head
        // / nozzle / ...).
        uint8_t preferred_head() const;
    };

    struct Config {
        static constexpr size_t CONNECT_URL_LEN = max_host_len;
        static constexpr size_t CONNECT_URL_BUF_LEN = (CONNECT_URL_LEN + 1);
        static constexpr size_t CONNECT_TOKEN_LEN = 20;
        static constexpr size_t CONNECT_TOKEN_BUF_LEN = (CONNECT_TOKEN_LEN + 1);

        char host[CONNECT_URL_BUF_LEN] = "";
        char token[CONNECT_TOKEN_BUF_LEN] = "";
        uint16_t port = 0;
        bool tls = true;
        bool enabled = false;
        bool custom_cert = false;
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
        char hostname[HOSTNAME_LEN + 1];
    };

    enum class JobControl {
        Pause,
        Resume,
        Stop,
    };

    enum class FinishedJobResult {
        FIN_STOPPED,
        FIN_OK,
    };

protected:
    PrinterInfo info;
    virtual Config load_config() = 0;
    bool can_start_download = false;

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
    virtual const char *start_print(const char *path, const std::optional<ToolMapping> &tools_mapping) = 0;
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
    virtual bool set_idle() = 0;
    void set_can_start_download(bool can) {
        can_start_download = can;
    }
    virtual bool is_printing() const = 0;
    // Is the printer in (hard) error?
    //
    // If so, most commands, actions, etc, are blocked.
    virtual bool is_in_error() const = 0;
    virtual bool is_idle() const = 0;
    virtual uint32_t cancelable_fingerprint() const = 0;
#if ENABLED(CANCEL_OBJECTS)
    virtual void cancel_object(uint8_t id) = 0;
    virtual void uncancel_object(uint8_t id) = 0;
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
    virtual std::optional<FinishedJobResult> get_prior_job_result(uint16_t job_id) const = 0;

    // Returns a newly reloaded config and a flag if it changed since last load
    // (unless the reset_fingerprint is set to false, in which case the flag is
    // kept).
    std::tuple<Config, bool> config(bool reset_fingerprint = true);

    uint32_t info_fingerprint() const;

    virtual void set_slot_info(size_t idx, const SlotInfo &slot) = 0;
};

} // namespace connect_client
