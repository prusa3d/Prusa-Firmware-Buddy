#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "radio_button.hpp"
#include "selftest_dock_type.hpp"
#include "status_footer.hpp"
#include <gui/qr.hpp>

class SelftestFrameDock : public SelftestFrameNamedWithRadio {
    FooterLine footer;
    window_wizard_progress_t progress;
    window_text_t text_info;
    window_text_t text_estimate;
    window_icon_t icon_warning;
    window_text_t text_warning;
    window_icon_t icon_info;
    QRStaticStringWindow qr;
    window_text_t text_link; ///< Web address to manual
    std::array<char, 100> name_buff;
    std::array<char, 50> remaining_buff;

    void set_warning_layout(const string_view_utf8 &txt);
    void set_info_layout(const string_view_utf8 &txt, const img::Resource *res = nullptr);
    void set_name(SelftestDocks_t data);
    void set_prologue();
    void set_remaining();
    constexpr Rect16 get_info_text_rect();
    constexpr Rect16 get_estimate_text_rect();
    constexpr Rect16 get_info_icon_rect();
    constexpr Rect16 get_link_text_rect();
    const char *get_phase_name();
    int get_phase_remaining_minutes();

    // One dock takes approximately 1:45 to calibrate if you know what you are doing
    static constexpr const char *PROLOGUE = N_("We suggest opening the online guide for the first-time calibration.");
    static constexpr const char *LINK = "prusa.io/dock-setup"; // Intentionally without translation
    static constexpr const char *MOVE_AWAY = N_("Do not touch the printer. Be careful around the moving parts.");
    static constexpr const char *PARK1 = N_("1. Please park current tool manually. Move the tool changing mechanism to the rear and align it with pins");
    static constexpr const char *PARK2 = N_("2. Now move the tool changing mechanism to the right, the tool will be locked in the dock");
    static constexpr const char *PARK3 = N_("3. The tool changing mechanism can now move freely.\nMove it a little bit to the front.");
    static constexpr const char *REMOVE_DOCK_PINS = N_("The calibrated dock is illuminated at the bottom and front side is flashing with white color.\n\nLoosen and remove the dock pins.");
    static constexpr const char *LOOSEN_DOCK_SCREW = N_("Loosen the two screws on the right side of the dock pillar (marked in orange) using the uni-wrench.");
    static constexpr const char *LOCK_TOOL = N_("Align the tool changing mechanism with the tool and lock it by sliding both metal bars to the right.");
    static constexpr const char *TIGHTEN_TOP = N_("Tighten the top dock screw at the right side of the pillar\n\nBe careful in next step the printer will be moving");
    static constexpr const char *MEASURING = N_("Do not touch the printer!\nThe printer is moving while measuring dock position.");
    static constexpr const char *TIGHTEN_BOT = N_("Tighten only the bottom screw on the right side of the dock pillar.");
    static constexpr const char *INSTALL_DOCK_PINS = N_("Install and tighten the dock pins.\n\nBe careful in next step the printer will be moving.");
    static constexpr const char *PARKING_TEST = N_("Do not touch the printer!\nThe printer is performing the parking test. Be careful around the moving parts.");
    static constexpr const char *TEST_FAILED = N_("Parking test failed.\n\nPlease try again.");
    static constexpr const char *TEST_SUCCESS = N_("Dock successfully calibrated.");
    static constexpr const char *REMAINING = N_("Approx. %d min");

protected:
    virtual void change() override;

public:
    SelftestFrameDock(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
