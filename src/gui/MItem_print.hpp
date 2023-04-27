/*****************************************************************************/
//print related menu items
#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "option/has_toolchanger.h"

class MI_NOZZLE_ABSTRACT : public WiSpinInt {
    static constexpr const char *const generic_label = N_("Nozzle Temperature"); // Generic string for no toolchanger

    uint8_t tool_nr;
    is_hidden_t is_hidden(uint8_t tool_nr);

public:
    MI_NOZZLE_ABSTRACT(uint8_t tool_nr, const char *label);
    virtual void OnClick() override;
};

template <uint8_t N>
class MI_NOZZLE : public MI_NOZZLE_ABSTRACT {
public:
    static constexpr const char *get_label() {
#if HAS_TOOLCHANGER()
        static_assert(N >= 0 && N <= 4, "bad input");
        switch (N) {
        case 0:
            return N_("Nozzle 1 Temperature");
        case 1:
            return N_("Nozzle 2 Temperature");
        case 2:
            return N_("Nozzle 3 Temperature");
        case 3:
            return N_("Nozzle 4 Temperature");
        case 4:
            return N_("Nozzle 5 Temperature");
        }
#else
        static_assert(N == 0, "For single tool printer, only 0 nozzle is allowed ");
        return N_("Nozzle Temperature");
#endif
    }

    MI_NOZZLE()
        : MI_NOZZLE_ABSTRACT(N, get_label()) {}
};

class MI_HEATBED : public WiSpinInt {
    constexpr static const char *label = N_("Heatbed Temperature");

public:
    MI_HEATBED();
    virtual void OnClick() override;
};

class MI_PRINTFAN : public WiSpinInt {
    constexpr static const char *label = N_("Print Fan Speed");
    uint8_t val_mapping(const bool rounding_floor, const uint8_t val, const uint8_t max_val, const uint8_t new_max_val);

public:
    MI_PRINTFAN();
    virtual void OnClick() override;
};

class MI_SPEED : public WiSpinInt {
    constexpr static const char *label = N_("Print Speed");

public:
    MI_SPEED();
    virtual void OnClick() override;
};

class MI_FLOWFACT : public WiSpinInt {
    constexpr static const char *label = N_("Flow Factor");

public:
    MI_FLOWFACT();
    virtual void OnClick() override;
};
