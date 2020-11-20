/*****************************************************************************/
//print related menu items
#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"

class MI_NOZZLE : public WI_SPIN_U16_t {
    constexpr static const char *label = N_("Nozzle");

public:
    MI_NOZZLE();
    virtual void OnClick() override;
};

class MI_HEATBED : public WI_SPIN_U08_t {
    constexpr static const char *label = N_("Heatbed");

public:
    MI_HEATBED();
    virtual void OnClick() override;
};

class MI_PRINTFAN : public WI_SPIN_U08_t {
    constexpr static const char *label = N_("Print Fan");

public:
    MI_PRINTFAN();
    virtual void OnClick() override;
};

class MI_SPEED : public WI_SPIN_U16_t {
    constexpr static const char *label = N_("Speed");

public:
    MI_SPEED();
    virtual void OnClick() override;
};

class MI_FLOWFACT : public WI_SPIN_U16_t {
    constexpr static const char *label = N_("Flow Factor");

public:
    MI_FLOWFACT();
    virtual void OnClick() override;
};
