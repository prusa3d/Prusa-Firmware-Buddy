/**
 * @file MItem_filament.hpp
 * @brief menu items for filament menu
 */
#pragma once

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
