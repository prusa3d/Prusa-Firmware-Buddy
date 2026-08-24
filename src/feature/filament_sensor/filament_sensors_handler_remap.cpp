
#include "filament_sensors_handler_remap.hpp"
#include <config_store/store_instance.hpp>
#include <window_msgbox.hpp>
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <option/has_side_fsensor_remap.h>
#include <module/prusa/toolchanger.h>

static_assert(HAS_SIDE_FSENSOR() && HAS_SIDE_FSENSOR_REMAP(), "You need these for remapping to work");

namespace side_fsensor_remap {

bool has_right_side_sensors() {
    return prusa_toolchanger.is_toolchanger_enabled()
        && (PhysicalToolIndex::from_raw(3).is_enabled() || PhysicalToolIndex::from_raw(4).is_enabled());
}

bool is_remapped() {
    return get_mapping() != preset::no_mapping;
}

void set_mapping(const Mapping &mapping) {
    config_store().side_fs_remap.set(mapping);
}

const Mapping get_mapping() {
    return config_store().side_fs_remap.get();
}

const Mapping &get_simple_mapping(bool enable) {
    if (enable) {
        if (has_right_side_sensors()) {
            return preset::has_right;
        } else {
            return preset::left_only;
        }
    } else {
        return preset::no_mapping;
    }
}

uint32_t ask_to_remap() {
    // If alternative mapping is currently used and requested
    bool current_remap = is_remapped();
    bool new_remap;

    // Ask user to remap
    const char *subject;
    if (!prusa_toolchanger.is_toolchanger_enabled()) {
        subject = N_("Side Filament Sensor");
    } else if (!has_right_side_sensors()) {
        subject = N_("Side Filament Sensors");
    } else {
        subject = N_("Third Side Filament Sensor");
    }

    char subject_buf[100];
    _(subject).copyToRAM(subject_buf);

    StringViewUtf8Parameters<sizeof(subject_buf)> params;
    new_remap = MsgBoxQuestion(_("%s can be remapped to the right side.\n(calibration will follow)\nRemap?").formatted(params, subject_buf),
                    { Response::Left, Response::Right, Response::_none, Response::_none }, current_remap)
        == Response::Right;

    // Change
    if (new_remap != current_remap) {
        // The entire mapping currently and requested
        auto current_mapping = get_mapping();
        auto new_mapping = get_simple_mapping(new_remap);

        // Store mapping
        set_mapping(new_mapping);

        uint32_t remapped = 0; ///< Mask of remapped tools
        for (auto tool : PhysicalToolIndex::all()) {
            if (current_mapping[tool.to_raw()] != new_mapping[tool.to_raw()]) {
#if HAS_SELFTEST()
                if (auto side = GetSideFSensor(tool); side) {
                    // Invalidate the sensor calibration
                    FilamentSensorCalibrator::Storage storage;
                    side->create_calibrator(storage)->finish();
                }
#endif
                remapped |= 1 << tool.to_raw();
            }
        }

        return remapped;
    }

    return 0;
}

static_assert(static_cast<std::result_of<decltype(ask_to_remap) &()>::type>(1 << std::tuple_size<Mapping>()) != 0, "Tool mask won't fit into returned type");

} // namespace side_fsensor_remap
