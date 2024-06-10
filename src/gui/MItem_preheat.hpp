/**
 * @file MItem_preheat.hpp
 * @brief menu items for preheat menu
 */
#pragma once
#include "menu_item_event_dispatcher.hpp"
#include "i18n.h"

class MI_PREHEAT_NOZZLE : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Nozzle only (Target temperature)");

public:
    MI_PREHEAT_NOZZLE();
    virtual void Do() override;
};

class MI_PREHEAT_BED : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Bed only");

public:
    MI_PREHEAT_BED();
    virtual void Do() override;
};

class MI_PREHEAT : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Nozzle and Bed (Preheat temperature)");

public:
    MI_PREHEAT();
    virtual void Do() override;
};

class MI_PREHEAT_COOLDOWN : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Cooldown");

public:
    MI_PREHEAT_COOLDOWN();
    virtual void Do() override;
};
