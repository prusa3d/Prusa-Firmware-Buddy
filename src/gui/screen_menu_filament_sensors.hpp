/// \file Filament sensors configuration menu for HAS_SIDE_FSENSOR
/// Introduced in BFW-4973

#pragma once

#include "screen_menu.hpp"
#include <option/has_side_fsensor.h>
#include <option/has_extruder_fsensor.h>
#include "MItem_tools.hpp"

namespace NScreenMenuFilamentSensors {

class MI_RestoreDefaults final : public IWindowMenuItem {
    static constexpr const char *label = N_("Restore Defaults");

public:
    MI_RestoreDefaults();

protected:
    void click(IWindowMenu &window_menu) override;
};

class IMI_AnySensor : public WI_ICON_SWITCH_OFF_ON_t {

protected:
    IMI_AnySensor(uint8_t sensor_index, bool is_side);

    void OnChange(size_t) final;
    void Loop() final;

    const uint8_t sensor_index;
    const bool is_side;

private:
    /// Sync the value and the enable state with the config store
    void update();

    StringViewUtf8Parameters<3> label_params_;
};

template <size_t sensor_index_>
class MI_ExtruderSensor final : public IMI_AnySensor {
public:
    MI_ExtruderSensor()
        : IMI_AnySensor(sensor_index_, false) {}
};

#if HAS_SIDE_FSENSOR()
template <size_t sensor_index_>
class MI_SideSensor final : public IMI_AnySensor {
public:
    MI_SideSensor()
        : IMI_AnySensor(sensor_index_, true) {}
};
#endif

template <typename I>
struct MenuBase_;

template <size_t... i>
struct MenuBase_<std::index_sequence<i...>> {
    using T = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_FILAMENT_SENSOR,
#if HAS_SIDE_FSENSOR()
        MI_SideSensor<i>...,
#endif
#if HAS_EXTRUDER_FSENSOR()
        MI_ExtruderSensor<i>...,
#endif
        MI_RestoreDefaults>;
};

using MenuBase = MenuBase_<std::make_index_sequence<PhysicalToolIndex::count>>::T;

} // namespace NScreenMenuFilamentSensors

class ScreenMenuFilamentSensors final : public NScreenMenuFilamentSensors::MenuBase {
    static constexpr const char *label = N_("FILAMENT SENSORS");

public:
    ScreenMenuFilamentSensors();
};
