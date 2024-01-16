#pragma once
#include "WindowItemFormatableLabel.hpp"
#include "WindowMenuItems.hpp"
#include <utility_extensions.hpp>

class MI_PINDA : public WI_FORMATABLE_LABEL_t<bool> {
    static constexpr const char *const label = N_("P.I.N.D.A.");

public:
    MI_PINDA();
};
