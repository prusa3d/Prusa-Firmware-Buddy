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

    for (size_t i = 0; i < number_of_slots; i++) {
        if (slots[i].material != nullptr) {
            crc.add(slots[i].material);
        }
        crc.add(int(slots[i].temp_nozzle))
            // The RPM values are in thousands and fluctuating a bit, we don't want
            // that to trigger the send too often, only when it actually really
            // changes.
            .add(slots[i].print_fan_rpm / 500)
            .add(slots[i].heatbreak_fan_rpm / 500);
    }

    return crc
        .add(int(pos[Printer::Z_AXIS_POS]))
        .add(print_speed)
        .add(flow_factor)
        // Report only about once every 10mm of filament
        .add(int(filament_used / 10))
        .add(int(target_nozzle))
        .add(int(temp_bed))
#if XL_ENCLOSURE_SUPPORT()
        .add(int(enclosure_info.temp))
        .add(enclosure_info.fan_rpm / 500)
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
    update_net(Iface::Wifi);

    const auto creds = net_creds();
    const auto &parameters = params();

    return crc
        .add_str(creds.ssid)
        .add_str(creds.pl_password)
        .add(parameters.has_usb)
        .add(parameters.nozzle_diameter)
        .add(parameters.version.type)
        .add(parameters.version.version)
        .add(parameters.version.subversion)
#if XL_ENCLOSURE_SUPPORT()
        .add(parameters.enclosure_info.present)
        .add(parameters.enclosure_info.enabled)
        .add(parameters.enclosure_info.printing_filtration)
        .add(parameters.enclosure_info.post_print)
        .add(parameters.enclosure_info.post_print_filtration_time)
#endif
        .done();
}

uint32_t Printer::Params::state_fingerprint() const {
    Crc crc;

    const uint32_t dialog_id = state.dialog.has_value() ? state.dialog->dialog_id : 0xFFFFFFFF;
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
