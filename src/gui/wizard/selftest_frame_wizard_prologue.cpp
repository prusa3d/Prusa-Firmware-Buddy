/**
 * @file selftest_frame_wizard_prologue.cpp
 */

#include "selftest_frame_wizard_prologue.hpp"
#include "i18n.h"
#include <guiconfig/wizard_config.hpp>
#include "img_resources.hpp"
#include "printers.h"

#if PRINTER_IS_PRUSA_MINI()
static constexpr size_t margin_texts = 0;
static constexpr Align_t align_text_icon = Align_t::CenterTop();
static const char *txt_prologue = N_("Welcome to the Original Prusa MINI setup wizard. Would you like to continue?");
#elif PRINTER_IS_PRUSA_MK4()
static constexpr size_t margin_texts = WizardDefaults::MarginLeft;
static constexpr Align_t align_text_icon = Align_t::CenterTop();
static const char *txt_prologue = N_("Hi, this is your\nOriginal Prusa MK4 printer.\n"
                                     "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_iX()
static constexpr size_t margin_texts = WizardDefaults::MarginLeft;
static constexpr Align_t align_text_icon = Align_t::CenterTop();
static const char *txt_prologue = N_("Hi, this is your\nOriginal Prusa iX printer.\n"
                                     "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_MK3_5()
static constexpr size_t margin_texts = WizardDefaults::MarginLeft;
static constexpr Align_t align_text_icon = Align_t::CenterTop();
static const char *txt_prologue = N_("Hi, this is your\nOriginal Prusa MK3.5 printer.\n"
                                     "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_XL()
static constexpr size_t margin_texts = WizardDefaults::MarginLeft;
static constexpr Align_t align_text_icon = Align_t::CenterTop();
static const char *txt_prologue = N_("Hi, this is your\nOriginal Prusa XL printer.\n"
                                     "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_CUBE()
static constexpr size_t margin_texts = WizardDefaults::MarginLeft;
static constexpr Align_t align_text_icon = Align_t::CenterTop();
static const char *txt_prologue = N_("Hi, this is your\nOriginal Prusa CUBE printer.\n"
                                     "I would like to guide you\nthrough the setup process.");
#else
    #error "Unknown printer type"
#endif

static constexpr Rect16 getTextRect() {
    Rect16 ret = GuiDefaults::RectScreenNoHeader;
    Rect16::Top_t btn_top = GuiDefaults::GetButtonRect(ret).Top();
    ret = Rect16::Height_t(btn_top - ret.Top());
    ret = Rect16::Width_t(GuiDefaults::ScreenWidth - 2 * margin_texts);
    ret = Rect16::Left_t(margin_texts);
    return ret;
}

static constexpr Rect16::Height_t icon_h = 150; // 140 + 10 padding

static constexpr Rect16 getIconRect() {
    Rect16 ret = getTextRect();
    ret = icon_h;
    return ret;
}

static constexpr Rect16 getTextIconRect() {
    Rect16 ret = getTextRect();
    ret -= icon_h;
    ret += Rect16::Top_t(icon_h);
    return ret;
}

SelftestFrameWizardPrologue::SelftestFrameWizardPrologue(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : SelftestFrameWithRadio(parent, ph, data)

    , icon(this, getIconRect(), &img::pepa_92x140)
    , text_icon(this, getTextIconRect(), is_multiline::yes)
    , text_full_frame(this, getTextRect(), is_multiline::yes) {

    icon.SetAlignment(Align_t::CenterTop());
    text_icon.SetAlignment(align_text_icon);
    text_full_frame.SetAlignment(Align_t::Center());

    change();
}

void SelftestFrameWizardPrologue::change() {
    const char *txt = nullptr;
    const char *txt_icon = nullptr;
    bool show_icon = false; // hand ok hand with checkmark

    // texts
    switch (phase_current) {
    case PhasesSelftest::WizardPrologue_ask_run:
    case PhasesSelftest::WizardPrologue_ask_run_dev:
        txt_icon = txt_prologue;
        show_icon = true;
        break;
    case PhasesSelftest::WizardPrologue_info:
        txt = N_("At the bottom of screen in status bar you can check some information about your printer."
                 "During self-test, the individual information will alternate."
                 "\n\n"
                 "After the test, you can select the parameters to be displayed in the status bar.");
        break;
    case PhasesSelftest::WizardPrologue_info_detailed:
        txt = N_("Self-test will now begin to check for potential assembly-related issues.");
        break;
    default:
        break;
    }

    if (txt) {
        text_full_frame.Show();
        text_full_frame.SetText(_(txt));
    } else {
        text_full_frame.Hide();
    }

    if (txt_icon) {
        text_icon.Show();
        text_icon.SetText(_(txt_icon));
    } else {
        text_icon.Hide();
    }

    if (show_icon) {
        icon.Show();
    } else {
        icon.Hide();
    }
};
