/**
 * @file MItem_filament.hpp
 * @brief menu items for filament menu
 */
#pragma once
#include "MItem_event_dispatcher.hpp"
#include "i18n.h"

class MI_LOAD : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Load Filament");
    constexpr static const char *const warning_loaded = N_("Filament appears to be already loaded, are you sure you want to load it anyway?");

public:
    MI_LOAD();
    virtual void Do() override;
};

class MI_UNLOAD : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Unload Filament");
    constexpr static const char *const header_label = N_("UNLOAD FILAMENT");

public:
    MI_UNLOAD();
    virtual void Do() override;
};

class MI_CHANGE : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Change Filament");
    constexpr static const char *const header_label = N_("CHANGE FILAMENT");

public:
    MI_CHANGE();
    virtual void Do() override;
};

class MI_PURGE : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Purge Filament");
    constexpr static const char *const header_label = N_("PURGE FILAMENT");

public:
    MI_PURGE();
    virtual void Do() override;
};
