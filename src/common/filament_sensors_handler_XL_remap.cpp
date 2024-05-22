
#include "filament_sensors_handler_XL_remap.hpp"
#include <config_store/store_instance.hpp>
#include <window_msgbox.hpp>
#include <module/prusa/toolchanger.h>
#include <filament_sensors_handler.hpp>

namespace side_fsensor_remap {

bool has_right_side_sensors() {
    return prusa_toolchanger.is_toolchanger_enabled()
        && (prusa_toolchanger.getTool(3).is_enabled() || prusa_toolchanger.getTool(4).is_enabled());
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
    if (!prusa_toolchanger.is_toolchanger_enabled()) {
        new_remap = MsgBoxQuestion(_("Side Filament Sensor can be remapped to the right side.\n(calibration will follow)\nRemap?"),
                        { Response::Left, Response::Right, Response::_none, Response::_none }, current_remap)
            == Response::Right;
    } else if (!has_right_side_sensors()) {
        new_remap = MsgBoxQuestion(_("Side Filament Sensors can be remapped to the right side.\n(calibration will follow)\nRemap?"),
                        { Response::Left, Response::Right, Response::_none, Response::_none }, current_remap)
            == Response::Right;
    } else {
        new_remap = MsgBoxQuestion(_("Third Side Filament Sensor can be remapped to the right side.\n(calibration will follow)\nRemap?"),
                        { Response::Left, Response::Right, Response::_none, Response::_none }, current_remap)
            == Response::Right;
    }

    // Change
    if (new_remap != current_remap) {
        // The entire mapping currently and requested
        auto current_mapping = get_mapping();
        auto new_mapping = get_simple_mapping(new_remap);

        // Store mapping
        set_mapping(new_mapping);

        // Reset selftest state and fsensor calibration for remapped sensors
        uint32_t remapped = 0; ///< Mask of remapped tools
        SelftestResult eeres = config_store().selftest_result.get();
        for (uint8_t e = 0; e < std::size(current_mapping); ++e) {
            if (current_mapping[e] != new_mapping[e]) {
                eeres.tools[e].fsensor = TestResult_Unknown;
                if (auto side = GetSideFSensor(e); side) {
                    side->SetInvalidateCalibrationFlag();
                }
                remapped |= 1 << e;
            }
        }
        config_store().selftest_result.set(eeres);

        return remapped;
    }

    return 0;
}

static_assert(static_cast<std::result_of<decltype(ask_to_remap) &()>::type>(1 << std::tuple_size<Mapping>()) != 0, "Tool mask won't fit into returned type");

} // namespace side_fsensor_remap
