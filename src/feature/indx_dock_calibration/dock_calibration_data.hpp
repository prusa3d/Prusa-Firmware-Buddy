#pragma once

#include <algorithm>
#include <cstdint>
#include <common/fsm_base_types.hpp>
#include <module/prusa/toolchanger.h>
#include <tool_index.hpp>

/// Serialized dock calibration failure data, transferred via fsm::PhaseData.
///
/// Layout (4 bytes):
///   [0]   dock_index
///   [1:2] measured X in tenths of mm (uint16_t LE)
///   [3]   measured Y offset from DOCK_DEFAULT_Y_MM in tenths of mm (int8_t)
struct DockCalibrationFailedData {
    PhysicalToolIndex dock_index;
    float measured_x;
    float measured_y;

    fsm::PhaseData serialize() const {
        fsm::PhaseData data {};
        data[0] = dock_index.to_raw();

        const auto x_tenths = static_cast<uint16_t>(std::clamp(measured_x * 10.0f, 0.0f, 65535.0f));
        data[1] = x_tenths & 0xFF;
        data[2] = (x_tenths >> 8) & 0xFF;

        const auto y_offset = measured_y - PrusaToolChanger::DOCK_DEFAULT_Y_MM;
        data[3] = static_cast<uint8_t>(static_cast<int8_t>(std::clamp(y_offset * 10.0f, -127.0f, 127.0f)));

        return data;
    }

    static DockCalibrationFailedData deserialize(fsm::PhaseData data) {
        return {
            .dock_index = PhysicalToolIndex::from_raw(data[0]),
            .measured_x = static_cast<float>(static_cast<uint16_t>(data[1] | (data[2] << 8))) / 10.0f,
            .measured_y = PrusaToolChanger::DOCK_DEFAULT_Y_MM
                + static_cast<float>(static_cast<int8_t>(data[3])) / 10.0f,
        };
    }
};

/// Per-dock actions chosen on the dock-selection screen, transferred via FSMResponseVariant.
/// One bit per dock; a dock is in at most one mask (in neither = keep unchanged).
struct DockSelection {
    uint8_t calibrate = 0; ///< Docks to (re)measure and store
    uint8_t invalidate = 0; ///< Docks whose stored calibration to clear
};
static_assert(PhysicalToolIndex::count <= 8, "DockSelection packs one bit per dock into uint8_t masks");
