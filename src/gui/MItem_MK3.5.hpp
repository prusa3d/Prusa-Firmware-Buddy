#pragma once
#include "WindowMenuItems.hpp"
#include <utility_extensions.hpp>

class MI_PINDA : public WI_SWITCH_0_1_NA_t {
    static constexpr const char *const label = N_("P.I.N.D.A.");
    static state_t get_state();

public:
    MI_PINDA();
    virtual void Loop() override;
    virtual void OnChange([[maybe_unused]] size_t old_index) override {}
};
