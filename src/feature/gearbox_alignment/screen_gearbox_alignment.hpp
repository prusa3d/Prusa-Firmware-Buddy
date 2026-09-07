/// @file
#pragma once

#include "screen_fsm.hpp"
#include <option/has_gearbox_alignment.h>

static_assert(HAS_GEARBOX_ALIGNMENT());

class ScreenGearboxAlignment final : public ScreenFSM {

public:
    ScreenGearboxAlignment();
    ~ScreenGearboxAlignment();
    static ScreenGearboxAlignment *GetInstance();

protected:
    void create_frame() final;
    void destroy_frame() final;
    void update_frame() final;
};
