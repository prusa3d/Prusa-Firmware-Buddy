#pragma once

#include <filament.hpp>

#include <WindowMenuInfo.hpp>
#include <WindowMenuSpin.hpp>
#include <MItem_tools.hpp>
#include <screen_menu.hpp>

namespace screen_filament_detail {

template <typename Child, typename Parent>
class MI_COMMON : public Parent {

public:
    using Parent::Parent;

    void set_filament_type(FilamentType set) {
        filament_type = set;
        static_cast<Child *>(this)->update();
    }

protected:
    FilamentType filament_type;
};

class MI_FILAMENT_NAME final : public MI_COMMON<MI_FILAMENT_NAME, WiInfo<filament_name_buffer_size>> {
public:
    MI_FILAMENT_NAME();
    void update();
};

class MI_FILAMENT_NOZZLE_TEMPERATURE final : public MI_COMMON<MI_FILAMENT_NOZZLE_TEMPERATURE, WiSpin> {
public:
    MI_FILAMENT_NOZZLE_TEMPERATURE();
    void update();
};

class MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE final : public MI_COMMON<MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE, WiSpin> {
public:
    MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE();
    void update();
};

class MI_FILAMENT_BED_TEMPERATURE final : public MI_COMMON<MI_FILAMENT_BED_TEMPERATURE, WiSpin> {
public:
    MI_FILAMENT_BED_TEMPERATURE();
    void update();
};

class MI_FILAMENT_REQUIRES_FILTRATION final : public MI_COMMON<MI_FILAMENT_REQUIRES_FILTRATION, WI_ICON_SWITCH_OFF_ON_t> {
public:
    MI_FILAMENT_REQUIRES_FILTRATION();
    void update();
};

using ScreenFilamentDetail_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_FILAMENT_NAME,
    MI_FILAMENT_NOZZLE_TEMPERATURE,
    MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE,
    MI_FILAMENT_BED_TEMPERATURE,
    MI_FILAMENT_REQUIRES_FILTRATION //
    >;

/// Management of a specified filament type
class ScreenFilamentDetail final : public ScreenFilamentDetail_ {
public:
    ScreenFilamentDetail(FilamentType filament_type);
};

}; // namespace screen_filament_detail

using ScreenFilamentDetail = screen_filament_detail::ScreenFilamentDetail;
