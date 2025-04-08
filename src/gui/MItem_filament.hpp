/**
 * @file MItem_filament.hpp
 * @brief menu items for filament menu
 */
#pragma once
#include "WindowMenuItems.hpp"

#include <i_window_menu_item.hpp>
#include <i18n.h>

class MI_LOAD : public IWindowMenuItem {
    constexpr static const char *const label = N_("Load Filament");
    constexpr static const char *const warning_loaded = N_("Filament appears to be already loaded, are you sure you want to load it anyway?");

public:
    MI_LOAD();
    virtual void click(IWindowMenu &) override;
};

class MI_UNLOAD : public IWindowMenuItem {
    constexpr static const char *const label = N_("Unload Filament");

public:
    MI_UNLOAD();
    virtual void click(IWindowMenu &) override;
};

class MI_CHANGE : public IWindowMenuItem {
    constexpr static const char *const label = N_("Change Filament");

    static bool AvailableForTool(uint8_t tool);
    static bool AvailableForAnyTool();

public:
    MI_CHANGE();
    virtual void click(IWindowMenu &) override;

    void UpdateEnableState();
};

class MI_CHANGEALL : public IWindowMenuItem {
    constexpr static const char *const label = N_("Change Filament in All Tools");

public:
    MI_CHANGEALL();

protected:
    virtual void click(IWindowMenu &) override;
};

class MI_PURGE : public IWindowMenuItem {
    constexpr static const char *const label = N_("Purge Filament");

    static bool AvailableForTool(uint8_t tool);
    static bool AvailableForAnyTool();

public:
    MI_PURGE();
    virtual void click(IWindowMenu &) override;

    void UpdateEnableState();
};

class MI_COOLDOWN : public IWindowMenuItem {
    static constexpr const char *const label = N_("Cooldown");

public:
    MI_COOLDOWN();

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override;
};

class MI_AUTO_COOLDOWN : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Cooldown after unload");
    bool init_index() const;

public:
    MI_AUTO_COOLDOWN()
        : WI_ICON_SWITCH_OFF_ON_t(init_index(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void OnChange(size_t old_index) override;
};
