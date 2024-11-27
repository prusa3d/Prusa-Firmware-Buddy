/// \file Filament sensors configuration menu for HAS_SIDE_FSENSOR
/// Introduced in BFW-4973

#pragma once

#include "screen_menu.hpp"
#include <option/has_side_fsensor.h>
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
    // We need a buffer to store the formatted string
    // because the label_base needs to go through snprintf
    std::array<char, 32> label;

public:
    /// Update enable value/enable state
    void update();

protected:
    IMI_AnySensor(uint8_t sensor_index, bool is_side, const char *label_base);

    void OnChange(size_t) final;

    const uint8_t sensor_index;
    const bool is_side;
};

class IMI_ExtruderSensor : public IMI_AnySensor {
    static constexpr const char *label_base = N_("Toolhead Filament Sensor");

protected:
    inline IMI_ExtruderSensor(uint8_t sensor_index)
        : IMI_AnySensor(sensor_index, false, label_base) {}
};

template <size_t sensor_index_>
class MI_ExtruderSensor final : public IMI_ExtruderSensor {
public:
    MI_ExtruderSensor()
        : IMI_ExtruderSensor(sensor_index_) {}
};

#if HAS_SIDE_FSENSOR()
class IMI_SideSensor : public IMI_AnySensor {
    static constexpr const char *label_base = N_("Side Filament Sensor");

protected:
    inline IMI_SideSensor(uint8_t sensor_index)
        : IMI_AnySensor(sensor_index, true, label_base) {}
};

template <size_t sensor_index_>
class MI_SideSensor final : public IMI_SideSensor {
public:
    MI_SideSensor()
        : IMI_SideSensor(sensor_index_) {}
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
        MI_ExtruderSensor<i>..., MI_RestoreDefaults>;
};

using MenuBase = MenuBase_<std::make_index_sequence<EXTRUDERS>>::T;

} // namespace NScreenMenuFilamentSensors

class ScreenMenuFilamentSensors final : public NScreenMenuFilamentSensors::MenuBase {
    static constexpr const char *label = N_("FILAMENT SENSORS");

public:
    ScreenMenuFilamentSensors();

protected:
    void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
