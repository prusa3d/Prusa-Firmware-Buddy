#include "printer.hpp"

#include <common/aggregate_arity.hpp>
#include <crc32.h>
#include <option/has_mmu2.h>
#include <option/has_filament_tracker.h>

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

// The fingerprints pick the members one by one - a new member needs a decision there
static_assert(aggregate_arity<ToConnectToolArgs>() == HAS_MMU2());
static_assert(aggregate_arity<Printer::PhysicalToolInfo>() == 7 + 3 * PRINTER_IS_PRUSA_iX());
static_assert(aggregate_arity<Printer::VirtualToolInfo>() == 2 * HAS_FILAMENT_TRACKER() + 3 * HAS_ANFC());

// A new filament parameter needs a decision whether INFO reports it
static_assert(
    aggregate_arity<FilamentTypeParameters>()
    == 7
        + HAS_FILAMENT_HEATBREAK_PARAM() * 1
        + HAS_CHAMBER_API() * 3
        + HAS_CHAMBER_FILTRATION_API() * 1
        + HAS_FILAMENT_BASE_PRESET_PARAM() * 1
        + HAS_ANFC() * 4
        + HAS_HT_HOTEND() * 1);

namespace {

    void add_material(Crc &crc, const std::optional<FilamentTypeParameters> &material) {
        crc.add(material.has_value());
        if (!material.has_value()) {
            return;
        }

        // Member by member, the padding bytes are indeterminate
        crc
#if HAS_ANFC()
            .add(material->material_uuid.value_or(UUID::nil()))
            .add(material->package_uuid.value_or(UUID::nil()))
            .add(material->instance_uuid.value_or(UUID::nil()))
#endif
            .add_str(material->name.data())
            .add(material->color.has_value() ? material->color->raw : uint32_t(-1))
            .add(material->nozzle_temperature)
            .add(material->nozzle_preheat_temperature)
            .add(material->heatbed_temperature)
#if HAS_FILAMENT_HEATBREAK_PARAM()
            .add(material->heatbreak_temperature)
#endif
#if HAS_CHAMBER_FILTRATION_API()
            .add(material->requires_filtration)
#endif
            .add(material->is_abrasive);
#if HAS_CHAMBER_API()
        // Through the value: the bytes of an empty optional are indeterminate
        for (const std::optional<uint8_t> &temperature : { material->chamber_min_temperature, material->chamber_max_temperature, material->chamber_target_temperature }) {
            crc.add(temperature.has_value() ? int(*temperature) : -1);
        }
#endif
    }

} // namespace

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

    for (PhysicalToolIndex pt : PhysicalToolIndex::all()) {
        // Use the list of enabled at the time of snapshot, not current
        // (that is, the mask, not .skip_all_disabled)
        if (physical_tool_mask & (1 << pt.to_raw())) {
            const auto &physical_tool = physical_tools[pt];
            crc.add(int(physical_tool.temp_nozzle))
                .add(physical_tool.target_nozzle)
#if PRINTER_IS_PRUSA_iX()
                .add(int(physical_tool.temp_heatbreak))
                .add(physical_tool.extruder_fs_state ? std::to_underlying(*physical_tool.extruder_fs_state) : -1)
                .add(physical_tool.remote_fs_state ? std::to_underlying(*physical_tool.remote_fs_state) : -1)
#endif
                // The RPM values are in thousands and fluctuating a bit, we don't want
                // that to trigger the send too often, only when it actually really
                // changes.
                .add(physical_tool.print_fan_rpm / 500)
                .add(physical_tool.heatbreak_fan_rpm / 500);
        }
    }

    for (VirtualToolIndex vt : VirtualToolIndex::all()) {
        // Use the list of enabled at the time of snapshot, not current
        // (that is, the mask, not .skip_all_disabled)
        if (!(virtual_tool_mask & (1 << vt.to_raw()))) {
            continue;
        }

        [[maybe_unused]] const auto &virtual_tool = virtual_tools[vt];
#if HAS_FILAMENT_TRACKER()
        crc.add(virtual_tool.odometer_id)
            // Don't send telemetry on every extruded millimeter
            .add(virtual_tool.odometer_e_mm / 100);
#endif
#if HAS_ANFC()
        // Don't send telemetry on every consumed gram. The full weight is reported in INFO.
        crc.add(uint32_t(virtual_tool.consumed_weight_g.value_or(-1) / 10));
#endif
    }

    return crc
        .add(physical_tool_mask)
        .add(virtual_tool_mask)
        .add(active_physical_tool)
        .add(active_virtual_tool_mask)
#if HAS_MMU2()
        .add(tool_args.mmu_enabled)
#endif
        .add(int(pos[Printer::Z_AXIS_POS]))
        .add(print_speed)
        .add(flow_factor)
        .add(int(target_bed))
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
        .add(chamber_info.target_temp)
        .add(chamber_info.fan_pwm_target)
        .add(chamber_info.led_intensity)
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

    for (PhysicalToolIndex pt : PhysicalToolIndex::all()) {
        // Use the list of enabled at the time of snapshot, not current
        // (that is, the mask, not .skip_all_disabled)
        if (parameters.physical_tool_mask & (1 << pt.to_raw())) {
            const auto &physical_tool = parameters.physical_tools[pt];
            crc
                .add(physical_tool.nozzle_diameter)
                .add(physical_tool.hardened)
                .add(physical_tool.high_flow);
        }
    }

    for (VirtualToolIndex vt : VirtualToolIndex::all()) {
        // Use the list of enabled at the time of snapshot, not current
        if (parameters.virtual_tool_mask & (1 << vt.to_raw())) {
            [[maybe_unused]] auto &vd = parameters.virtual_tools[vt];

            add_material(crc, material(vt));
#if HAS_ANFC()
            crc
                .add(vd.full_weight_g.value_or(-1))
                .add(vd.full_weight_is_nominal);
#endif
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
        .add(parameters.physical_tool_mask)
#if HAS_MMU2()
        // Decides the tool ids
        .add(parameters.tool_args.mmu_enabled)
#endif
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

} // namespace connect_client
