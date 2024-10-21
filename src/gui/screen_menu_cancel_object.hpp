/**
 * @file screen_menu_cancel_object.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include <marlin_vars.hpp>

#if ENABLED(CANCEL_OBJECTS)

/**
 * @brief Prototype object that can hide functionality in .cpp.
 */
class MI_CO_OBJECT_N : public MenuItemSwitch {
    char label_buffer[marlin_vars_t::CANCEL_OBJECT_NAME_LEN] = {}; ///< Buffer for object name, start empty

    /// True if object name form G-code is valid, false if backup_label is used or if object name changed
    bool backup_label_used = false; ///< Write backup on first call to UpdateName()

    const int ObjectId; ///< Id of object to cancel with this switch

public:
    MI_CO_OBJECT_N(int ObjectId_);

    /**
     * @brief Update state of switch.
     */
    void UpdateState();

    /**
     * @brief Update visibility and object name.
     */
    void UpdateName();

protected:
    virtual void OnChange(size_t old_index) override;
};

/**
 * @brief Final template to cancel one object.
 */
template <int ObjectIdN>
class MI_CO_OBJECT : public MI_CO_OBJECT_N {
    static_assert(ObjectIdN < marlin_vars_t::CANCEL_OBJECTS_NAME_COUNT, "Too many objects to cancel!");

public:
    MI_CO_OBJECT()
        : MI_CO_OBJECT_N(ObjectIdN) {}
};

/**
 * @brief Button to cancel current object.
 */
class MI_CO_CANCEL_CURRENT : public IWindowMenuItem {
    static constexpr const char *label = N_("Cancel Current");

public:
    MI_CO_CANCEL_CURRENT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

namespace detail {
using ScreenMenuCancelObject = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_CO_CANCEL_CURRENT,
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
} // namespace detail

class ScreenMenuCancelObject : public detail::ScreenMenuCancelObject {
    int loop_index = 0; ///< Refresh names only once every few loops

public:
    constexpr static const char *label = "Canceled Objects";
    ScreenMenuCancelObject();

protected:
    void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};

#else /* ENABLED(CANCEL_OBJECTS) */

class ScreenMenuCancelObject {};

#endif /* ENABLED(CANCEL_OBJECTS) */
