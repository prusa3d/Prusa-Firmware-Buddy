#include "printer.hpp"

#include <crc32.h>

#include <cstring>

using std::make_tuple;
using std::optional;
using std::tuple;

namespace {

struct Crc {
    uint32_t crc = 0;

    template <class T>
    Crc &add(const T &value) {
        crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(&value), sizeof value);
        return *this;
    }

    Crc &add_str(const char *s) {
        if (s != nullptr) {
            crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(s), strlen(s));
        }
        return *this;
    }

    Crc &add_str(std::string_view s) {
        crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(s.data()), s.size());
        return *this;
    }

    uint32_t done() const {
        return crc;
    }
};

} // namespace

namespace connect_client {

uint32_t Printer::Params::telemetry_fingerprint(bool include_xy_axes) const {
    // Note: keep in sync with the rendering of telemetry in render.cpp
    // Note: There are some guessed "precision" constants - making sure we
    //   don't resend the telemetry too often because something changes only a
    //   little bit.

    Crc crc;

    if (include_xy_axes) {
        // Add the axes, but with only whole-int precision.
        crc
            .add(int(pos[Printer::X_AXIS_POS]))
            .add(int(pos[Printer::Y_AXIS_POS]));
    }

    for (VirtualToolIndex vt : VirtualToolIndex::all()) {
        // Use the list of enabled at the time of snapshot, not current
        // (that is, the mask, not .skip_all_disabled)
        if (slot_mask & (1 << vt.to_raw())) {
            crc.add_str(slots[vt].material.data())
                .add(int(slots[vt].temp_nozzle))
#if PRINTER_IS_PRUSA_iX()
                .add(int(slots[vt].temp_heatbreak))
                .add(slots[vt].extruder_fs_state ? std::to_underlying(*slots[vt].extruder_fs_state) : -1)
                .add(slots[vt].remote_fs_state ? std::to_underlying(*slots[vt].remote_fs_state) : -1)
#endif
                // The RPM values are in thousands and fluctuating a bit, we don't want
                // that to trigger the send too often, only when it actually really
                // changes.
                .add(slots[vt].print_fan_rpm / 500)
                .add(slots[vt].heatbreak_fan_rpm / 500);
        }
    }

    return crc
        .add(active_slot)
        .add(int(pos[Printer::Z_AXIS_POS]))
        .add(print_speed)
        .add(flow_factor)
        // Report only about once every 10mm of filament
        .add(int(filament_used / 10))
        .add(int(target_nozzle))
        .add(int(temp_bed))
#if PRINTER_IS_PRUSA_iX()
        .add(int(temp_psu))
        .add(int(temp_ambient))
#endif
#if XL_ENCLOSURE_SUPPORT()
        .add(int(enclosure_info.temp))
        .add(enclosure_info.fan_rpm / 500)
#endif
#if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
        .add(int(chamber_info.fan_1_rpm / 500))
        .add(int(chamber_info.fan_2_rpm / 500))
#endif
        .done();
}

uint32_t Printer::Config::crc() const {
    return Crc()
        .add_str(host)
        .add_str(token)
        .add(port)
        .add(tls)
        .add(enabled)
        .done();
}

tuple<Printer::Config, bool> Printer::config(bool reset_fingerprint) {
    Config result = load_config();

    const uint32_t new_fingerprint = result.crc();
    const bool changed = new_fingerprint != cfg_fingerprint;
    if (reset_fingerprint) {
        cfg_fingerprint = new_fingerprint;
    }
    return make_tuple(result, changed);
}

uint32_t Printer::info_fingerprint() const {
    // The actual INFO message contains more info. But most of it doesn't
    // actually change (eg. our own firmware version) - at least not at
    // runtime.
    Crc crc;

    auto update_net = [&](Iface iface) {
        if (const auto info = net_info(iface); info.has_value()) {
            crc.add(info->ip).add(info->mac);
        }
    };

    update_net(Iface::Ethernet);
#if HAS_ESP()
    update_net(Iface::Wifi);
#endif

    const auto creds = net_creds();
    const auto &parameters = params();

    for (VirtualToolIndex vt : VirtualToolIndex::all()) {
        // Use the list of enabled at the time of snapshot, not current
        // (that is, the mask, not .skip_all_disabled)
        if (parameters.slot_mask & (1 << vt.to_raw())) {
            const auto &slot = parameters.slots[vt];
            crc
                .add(slot.nozzle_diameter)
                .add(slot.hardened)
                .add(slot.high_flow)
                .add_str(slot.material.data());
        }
    }

    return crc
        .add_str(creds.ssid)
        .add_str(creds.pl_password)
        .add_str(creds.hostname)
        .add(parameters.has_usb)
        .add(parameters.can_start_download)
        .add(parameters.version.type)
        .add(parameters.version.version)
        .add(parameters.version.subversion)
        .add(parameters.enabled_tool_cnt())
#if XL_ENCLOSURE_SUPPORT()
        .add(parameters.enclosure_info.present)
        .add(parameters.enclosure_info.enabled)
        .add(parameters.enclosure_info.printing_filtration)
        .add(parameters.enclosure_info.post_print)
        .add(parameters.enclosure_info.post_print_filtration_time)
#endif
#if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
        .add(parameters.addon_power)
#endif
        .done();
}

uint32_t Printer::Params::state_fingerprint() const {
    Crc crc;

    // internal variable used to calculate fingerprint and may exceed 31bits reserved for DialogId's
    // value 0xFFFFFFFF reserved for missing Id
    const uint32_t dialog_id = state.dialog.has_value() ? state.dialog->dialog_id.to_uint32_t() : 0xFFFFFFFF;

    return crc
        .add(state.device_state)
        .add(dialog_id)
        .add(state.code_num())
        .add_str(state.title())
        .add_str(state.text())
        .done();
}

Printer::Params::Params(const optional<BorrowPaths> &paths)
    : paths(paths) {}

const char *Printer::Params::job_path() const {
    if (paths.has_value()) {
        return paths->path();
    } else {
        return nullptr;
    }
}

const char *Printer::Params::job_lfn() const {
    if (paths.has_value()) {
        return paths->name();
    } else {
        return nullptr;
    }
}

VirtualToolIndex Printer::Params::preferred_slot() const {
    return match(
        active_slot,
        [](VirtualToolIndex vt) { return vt; },
        // We need to give Connect _some_ temperature. Pick the first enabled
        // tool (or the first tool completely, in case of Error printer).
        [this](NoTool) { return VirtualToolIndex::from_raw(slot_mask == 0 ? 0 : std::countr_zero(slot_mask)); });
}

} // namespace connect_client
