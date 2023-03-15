/**
 * @file screen_menu_cancel_object.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"

#include "../Marlin/src/feature/cancel_object.h"

template <int ObjectId>
class MI_CO_OBJECT : public WI_SWITCH_t<2> {
    char label_buffer[10];
    static constexpr const char *const label = N_("Object X");

    constexpr static const char *str_printing = N_("Printing");
    constexpr static const char *str_canceled = N_("Canceled");

public:
    MI_CO_OBJECT()
        : WI_SWITCH_t(
            0, _(label), nullptr, is_enabled_t::yes,
            []() {
                return cancelable.object_count > ObjectId ? is_hidden_t::no : is_hidden_t::yes;
            }(),
            _(str_printing), _(str_canceled)) {
        SetIndex(cancelable.is_canceled(ObjectId) ? 1 : 0);

        const char *object_name = cancelable.get_object_name(ObjectId);
        if (object_name) {
            SetLabel(string_view_utf8::MakeRAM((uint8_t *)object_name));
        } else {
            snprintf(label_buffer, sizeof(label_buffer), "Object %i", ObjectId);
            SetLabel(string_view_utf8::MakeRAM((uint8_t *)label_buffer));
        }
    }

protected:
    virtual void OnChange(size_t old_index) override {
        if (old_index == 0) { // is printing
            cancelable.cancel_object(ObjectId);
            SetIndex(1);
        } else {
            cancelable.uncancel_object(ObjectId);
            SetIndex(0);
        }
    }
};

using ScreenMenuCancelObject__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_CO_OBJECT<0>,
    MI_CO_OBJECT<1>,
    MI_CO_OBJECT<2>,
    MI_CO_OBJECT<3>,
    MI_CO_OBJECT<4>,
    MI_CO_OBJECT<5>,
    MI_CO_OBJECT<6>,
    MI_CO_OBJECT<7>,
    MI_CO_OBJECT<8>,
    MI_CO_OBJECT<9>,
    MI_CO_OBJECT<10>,
    MI_CO_OBJECT<11>,
    MI_CO_OBJECT<12>,
    MI_CO_OBJECT<13>,
    MI_CO_OBJECT<14>,
    MI_CO_OBJECT<15>>;

class ScreenMenuCancelObject : public ScreenMenuCancelObject__ {
public:
    constexpr static const char *label = "Canceled Objects";
    ScreenMenuCancelObject();
};
