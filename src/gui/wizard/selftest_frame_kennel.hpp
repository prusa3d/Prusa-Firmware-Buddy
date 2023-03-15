#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "radio_button.hpp"
#include "selftest_kennel_type.hpp"
#include "status_footer.hpp"

class SelftestFrameKennel : public AddSuperWindow<SelftestFrameNamedWithRadio> {
    FooterLine footer;
    window_wizard_progress_t progress;
    window_text_t text_info;
    window_icon_t icon_warning;
    window_text_t text_warning;
    window_icon_t icon_info;
    window_qr_t qr;
    std::array<char, 100> name_buff;

    void set_warning_layout(string_view_utf8 txt);
    void set_info_layout(string_view_utf8 txt, const png::Resource *res = nullptr);
    void set_name(SelftestKennels_t data);
    void set_prologue();
    constexpr Rect16 get_info_text_rect();
    constexpr Rect16 get_info_icon_rect();
    const char *get_phase_name();

    static constexpr const char *PROLOGUE = N_("Multitool calibration is quite a complex process. For the first time please read tutorial.\n\nMore detail at help.prusa3d.com/12201 ");
    static constexpr const char *PARK1 = N_("1. Please park current tool manually. Move the head to the rear and align it with pins");
    static constexpr const char *PARK2 = N_("2. Now move the head to the right, the tool will be locked in the kennel");
    static constexpr const char *PARK3 = N_("3. The head can now move freely.\nMove it a little bit to the front.");
    static constexpr const char *REMOVE_KENNEL_PINS = N_("Loosen and remove the kennel pins");
    static constexpr const char *LOOSEN_KENNEL_SCREW = N_("Loosen screws on left side of the kennel pillar");
    static constexpr const char *LOCK_TOOL = N_("Lock the head to the tool");
    static constexpr const char *TIGHTEN_TOP = N_("Tighten the top kennel screw at the left side of the pillar\n\nBe careful in next step the printer will be moving");
    static constexpr const char *MEASURING = N_("Be careful\nThe printer is moving while measuring kennel position.");
    static constexpr const char *INSTALL_KENNEL_PINS = N_("Install and tighten the kennel pins.");
    static constexpr const char *TIGHTEN_BOT = N_("Tighten the bottom kennel screw at the left side of the pillar\n\nBe careful in next step the printer will be moving");
    static constexpr const char *PARKING_TEST = N_("Be careful!\nPrinter is moving during parking test.");

protected:
    virtual void change() override;

public:
    SelftestFrameKennel(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
