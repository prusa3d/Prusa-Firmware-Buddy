/*****************************************************************************/
//print related menu items
#pragma once
#include "WindowMenuItems.hpp"
#pragma pack(push, 1)

class MI_NOZZLE : public WI_SPIN_U16_t {
    constexpr static const char *label = "Nozzle";

public:
    MI_NOZZLE();
    virtual void OnClick() override;
};

class MI_HEATBED : public WI_SPIN_U08_t {
    constexpr static const char *label = "Heatbed";

public:
    MI_HEATBED();
    virtual void OnClick() override;
};

class MI_PRINTFAN : public WI_SPIN_U08_t {
    constexpr static const char *label = "Print Fan";

public:
    MI_PRINTFAN();
    virtual void OnClick() override;
};

class MI_SPEED : public WI_SPIN_U16_t {
    constexpr static const char *label = "Speed";

public:
    MI_SPEED();
    virtual void OnClick() override;
};

class MI_FLOWFACT : public WI_SPIN_U16_t {
    constexpr static const char *label = "Flow Factor";

public:
    MI_FLOWFACT();
    virtual void OnClick() override;
};

class MI_BABYSTEP : public WI_SPIN_t<float> {
    constexpr static const char *const label = "Live Adjust Z"; // or "Z-offset"?

public:
    MI_BABYSTEP();
    virtual void OnClick() override;
    virtual bool Change(int dif) override;
};

#pragma pack(pop)
