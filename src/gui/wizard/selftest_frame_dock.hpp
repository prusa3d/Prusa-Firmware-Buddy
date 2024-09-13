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
    StringViewUtf8Parameters<4> remaining_params;

    void set_warning_layout(const string_view_utf8 &txt);
    void set_info_layout(const string_view_utf8 &txt, const img::Resource *res = nullptr);
    void set_name(SelftestDocks_t data);
    void set_prologue();
    constexpr Rect16 get_info_text_rect();
    constexpr Rect16 get_estimate_text_rect();
    constexpr Rect16 get_info_icon_rect();
    constexpr Rect16 get_link_text_rect();
    const char *get_phase_name();
    int get_phase_remaining_minutes();

protected:
    virtual void change() override;

public:
    SelftestFrameDock(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
