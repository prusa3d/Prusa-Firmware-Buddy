/**
 * @file screen_menu_move.hpp
 */
#pragma once

#include <meta_utils.hpp>
#include "screen_menu.hpp"
#include "WindowItemFormatableLabel.hpp"
#include "MItem_filament.hpp"
#include <window_menu_callback_item.hpp>

namespace screen_menu_move {

class I_MI_AXIS : public WiSpin {
public:
    I_MI_AXIS(size_t index);
};

class DUMMY_AXIS_E : public WI_FORMATABLE_LABEL_t<int> {

public:
    DUMMY_AXIS_E();

protected:
    void click(IWindowMenu &window_menu) override;
};

using MI_AXIS_X = WithConstructorArgs<I_MI_AXIS, X_AXIS>;
using MI_AXIS_Y = WithConstructorArgs<I_MI_AXIS, Y_AXIS>;
using MI_AXIS_Z = WithConstructorArgs<I_MI_AXIS, Z_AXIS>;
using MI_AXIS_E = WithConstructorArgs<I_MI_AXIS, E_AXIS>;

using MI_COOLDOWN = WithConstructorArgs<WindowMenuCallbackItem, N_("Cooldown"), nullptr>;

using ScreenMenuMove_ = ScreenMenu<EFooter::On, MI_RETURN, MI_AXIS_X, MI_AXIS_Y, MI_AXIS_Z, MI_AXIS_E, DUMMY_AXIS_E, MI_COOLDOWN>;

} // namespace screen_menu_move

class ScreenMenuMove : public screen_menu_move::ScreenMenuMove_ {

public:
    ScreenMenuMove();

protected:
    void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    void loop();
    void plan_moves();

private:
    xyze_float_t queued_pos { { NAN, NAN, NAN, NAN } };
    float e_axis_offset;
};
