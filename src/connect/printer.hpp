#pragma once

#include "printers.h"

#include <otp.hpp>
#include <common/shared_buffer.hpp>
#include <inc/MarlinConfigPre.h>

#include <cstdint>
#include <cstddef>
#include <tuple>
#include <expected>
#include <optional>
#include <limits>
#include <variant>

#include <netif_settings.h>
#include "printer_type.hpp"
#include <state/printer_state.hpp>
#include <device/board.h>
#include <connect/hostname.hpp>
#include <connect/tools.hpp>
#include <utils/compact_optional.hpp>
#include <feature/filament_sensor/filament_sensor_states.hpp>
#include <filament.hpp>

#include <tool_index.hpp>
#include <utils/storage/strong_index_array.hpp>

#include <option/has_cancel_object.h>
#include <option/xbuddy_extension_variant.h>
#include <option/has_anfc.h>
#include <option/has_filament_tracker.h>
#include <option/has_esp.h>

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

        const char *firmware_version;
        serial_nr_t serial_number;
        char fingerprint[FINGERPRINT_BUFF_LEN];
    };

    struct PhysicalToolInfo {
        float temp_nozzle = 0;

#if PRINTER_IS_PRUSA_iX()
        float temp_heatbreak = 0;
        std::optional<FilamentSensorState> extruder_fs_state;
        std::optional<FilamentSensorState> remote_fs_state;
#endif
        float nozzle_diameter = 0;

        /// 0 when the heating is off
        int16_t target_nozzle = 0;

        uint16_t print_fan_rpm = 0;
        uint16_t heatbreak_fan_rpm = 0;

        bool high_flow : 1 = false;
        bool hardened : 1 = false;
    };

    struct VirtualToolInfo {
#if HAS_FILAMENT_TRACKER()
        /// Changes (not increments) whenever the odometers of the tool are reset
        uint32_t odometer_id = 0;

        /// Length of filament extruded since the last odometer reset
        uint32_t odometer_e_mm = 0;
#endif

#if HAS_ANFC()
        CompactOptional<uint32_t, std::numeric_limits<uint32_t>::max()> consumed_weight_g;

        /// Full weight of the tracked spool, for INFO
        CompactOptional<uint32_t, std::numeric_limits<uint32_t>::max()> full_weight_g;

        /// Whether the spool tag gives the full weight as nominal rather than actual
        bool full_weight_is_nominal = false;
#endif
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

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    struct ChamberInfo {
        static constexpr int target_temp_unset = 0U; // agreed with the Connect team, that 0 maps to unset values
        uint32_t target_temp = target_temp_unset;
        float current_temp = 0;
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

    class Params {
    private:
        // Living in the Printer we come from
        const std::optional<BorrowPaths> &paths;

    public:
        Params(const std::optional<BorrowPaths> &paths);

        StrongIndexArray<PhysicalToolInfo, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> physical_tools;
        StrongIndexArray<VirtualToolInfo, VirtualToolIndex::count, VirtualToolIndex, VirtualToolIndex::to_raw_static> virtual_tools;

#if XL_ENCLOSURE_SUPPORT()
        EnclosureInfo enclosure_info;
#endif
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
        ChamberInfo chamber_info;
        bool addon_power = false;
#endif
        uint32_t progress_code = 0;
        char command_code = 0;

        // Which physical tools (print heads) are enabled (bitfield)
        uint8_t physical_tool_mask = 0;
        static_assert(8 * sizeof physical_tool_mask >= PhysicalToolIndex::count);

        /// Virtual tools loaded into their physical tool, picked or not (bitfield) - one per physical tool at most
        uint8_t active_virtual_tool_mask = 0;
        static_assert(8 * sizeof active_virtual_tool_mask >= VirtualToolIndex::count);
        // Which virtual tools are enabled (bitset)
        uint8_t virtual_tool_mask = 0;
        static_assert(8 * sizeof virtual_tool_mask >= VirtualToolIndex::count);

        ToConnectToolArgs tool_args = {};
        std::variant<PhysicalToolIndex, NoTool> active_physical_tool = NoTool {};
        float temp_bed = 0;
#if PRINTER_IS_PRUSA_iX()
        float temp_psu = 0;
        float temp_ambient = 0;
#endif
        int16_t target_bed = 0;
        float pos[4] = { 0, 0, 0, 0 };
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

        uint32_t telemetry_fingerprint(bool include_xy_axes) const;
        uint32_t state_fingerprint() const;
    };

    struct Config {
        static constexpr size_t CONNECT_URL_LEN = max_host_len;
        static constexpr size_t CONNECT_URL_BUF_LEN = (CONNECT_URL_LEN + 1);
        static constexpr size_t CONNECT_TOKEN_LEN = 20;
        static constexpr size_t CONNECT_TOKEN_BUF_LEN = (CONNECT_TOKEN_LEN + 1);
        static constexpr size_t CONNECT_PROXY_SIZE = 30;
        static constexpr size_t CONNECT_PROXY_BUF_LEN = CONNECT_PROXY_SIZE + 1;

        char host[CONNECT_URL_BUF_LEN] = "";
        char proxy_host[CONNECT_PROXY_BUF_LEN] = "";
        char token[CONNECT_TOKEN_BUF_LEN] = "";
        uint16_t port = 0;
        uint16_t proxy_port = 0;
        bool tls = true;
        bool enabled = false;
        bool custom_cert = false;
        // Used only through loading.
        bool loaded = false;

        uint32_t crc() const;
        bool has_proxy() const {
            return proxy_host[0] != '\0' && proxy_port != 0;
        }
    };

    enum class Iface {
        Ethernet,
#if HAS_ESP()
        Wifi,
#endif
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
    virtual bool is_valid_file_or_transfer(const char *path) const = 0;
    // Result of start_print: job_id on success, error message on failure.
    using StartPrintResult = std::expected<uint16_t, const char *>;
    virtual StartPrintResult start_print(const char *path, const std::optional<ToolMapping> &tools_mapping) = 0;
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
#if HAS_CANCEL_OBJECT()
    virtual void set_object_cancelled(uint16_t id, bool set) = 0;
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

    virtual const char *dialog_action(printer_state::DialogId dialog_id, Response response) = 0;
    virtual std::optional<FinishedJobResult> get_prior_job_result(uint16_t job_id) const = 0;

    // Returns a newly reloaded config and a flag if it changed since last load
    // (unless the reset_fingerprint is set to false, in which case the flag is
    // kept).
    std::tuple<Config, bool> config(bool reset_fingerprint = true);

    uint32_t info_fingerprint() const;

    /// @returns the parameters of the material loaded in the virtual tool, nullopt when there is none
    virtual std::optional<FilamentTypeParameters> material(VirtualToolIndex tool) const = 0;

    virtual void set_physical_tool_info(PhysicalToolIndex tool, const PhysicalToolInfo &info) = 0;
};

} // namespace connect_client
