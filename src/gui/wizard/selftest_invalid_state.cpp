/**
 * @file selftest_invalid_state.cpp
 */
#include "selftest_invalid_state.hpp"
#include "i18n.h"
#include <guiconfig/wizard_config.hpp>

static constexpr size_t col_0 = WizardDefaults::MarginLeft;
static constexpr size_t row_0 = WizardDefaults::row_0;
static constexpr size_t txt_h = WizardDefaults::txt_h;

static constexpr const char *en_text = N_("Error invalid selftest state");

ScreenSelftestInvalidState::ScreenSelftestInvalidState(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrame>(parent, ph, data)
    , text(this, Rect16(col_0, row_0, WizardDefaults::X_space, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text)) {
}
