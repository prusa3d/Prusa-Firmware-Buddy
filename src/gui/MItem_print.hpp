/*****************************************************************************/
// print related menu items
#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "option/has_toolchanger.h"
#include <option/has_mmu2.h>
#include <guiconfig/guiconfig.h>
#include <WindowItemFormatableLabel.hpp>

class MI_NOZZLE_ABSTRACT : public WiSpin {
    static constexpr const char *const generic_label =
#if HAS_MINI_DISPLAY()
        N_("Nozzle"); // Generic string for no toolchanger
#else
        N_("Nozzle Temperature");
#endif

    uint8_t tool_nr;
    is_hidden_t is_hidden(uint8_t tool_nr);

public:
    MI_NOZZLE_ABSTRACT(uint8_t tool_nr, const char *label);
    virtual void OnClick() override;
};

/// Nozzle target temperature (adjustable spin)
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
    #if HAS_MINI_DISPLAY()
        return N_("Nozzle");
    #else
        return N_("Nozzle Temperature");
    #endif
#endif
    }

    MI_NOZZLE()
        : MI_NOZZLE_ABSTRACT(N, get_label()) {}
};

/// Nozzle current temperature (read only, auto updateing)
class MI_INFO_NOZZLE_TEMP : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_NOZZLE_TEMP(uint8_t tool = 0);

private:
    std::array<char, 32> label_;
    const uint8_t tool_;
};

/// Nozzle current temperature (read only, auto updating)
class MI_INFO_HEATBREAK_TEMP : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_HEATBREAK_TEMP(uint8_t tool = 0);

private:
    std::array<char, 32> label_;
    const uint8_t tool_;
};

class MI_HEATBED : public WiSpin {
    constexpr static const char *label =
#if HAS_MINI_DISPLAY()
        N_("Heatbed");
#else
        N_("Heatbed Temperature");
#endif

public:
    MI_HEATBED();
    virtual void OnClick() override;
};

class MI_PRINTFAN : public WiSpin {
    constexpr static const char *label =
#if HAS_MINI_DISPLAY()
        N_("Print Fan");
#else
        N_("Print Fan Speed");
#endif

public:
    MI_PRINTFAN();
    virtual void OnClick() override;
};

class MI_SPEED : public WiSpin {
    constexpr static const char *label = N_("Print Speed");

public:
    MI_SPEED();
    virtual void OnClick() override;
};

class MI_FLOWFACT_ABSTRACT : public WiSpin {
    static constexpr const char *const generic_label = N_("Flow Factor"); // Generic string for no toolchanger

    uint8_t tool_nr;
    is_hidden_t is_hidden(uint8_t tool_nr);

public:
    MI_FLOWFACT_ABSTRACT(uint8_t tool_nr, const char *label);
    virtual void OnClick() override;
};

template <uint8_t N>
class MI_FLOWFACT : public MI_FLOWFACT_ABSTRACT {
public:
    static constexpr const char *get_label() {
#if HAS_TOOLCHANGER() || HAS_MMU2()
        static_assert(N >= 0 && N <= 4, "bad input");
        switch (N) {
        case 0:
            return N_("Tool 1 Flow Factor");
        case 1:
            return N_("Tool 2 Flow Factor");
        case 2:
            return N_("Tool 3 Flow Factor");
        case 3:
            return N_("Tool 4 Flow Factor");
        case 4:
            return N_("Tool 5 Flow Factor");
        }
#else
        static_assert(N == 0, "Only tool 0 allowed");
        return N_("Flow Factor");
#endif
    }

    MI_FLOWFACT()
        : MI_FLOWFACT_ABSTRACT(N, get_label()) {}
};
