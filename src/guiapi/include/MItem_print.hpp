/*****************************************************************************/
//print related menu items
#pragma once
#include "WindowMenuItems.hpp"
#pragma pack(push, 1)

class MI_NOZZLE : public WI_SPIN_U16_t {
    constexpr static const char *label = "Nozzle";

public:
    MI_NOZZLE();
    virtual void OnClick();
};

class MI_HEATBED : public WI_SPIN_U08_t {
    constexpr static const char *label = "Heatbed";

public:
    MI_HEATBED();
    virtual void OnClick();
};

class MI_PRINTFAN : public WI_SPIN_U08_t {
    constexpr static const char *label = "Print Fan";

public:
    MI_PRINTFAN();
    virtual void OnClick();
};

#pragma pack(pop)
