// screen_home.hpp
#pragma once
#include "window_header.hpp"
#include "status_footer.hpp"
#include "gui.hpp"
#include "screen.hpp"
#include "gcode_info.hpp"
#include "window_dlg_wait.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_state.h"

class screen_home_data_t : public AddSuperWindow<screen_t> {
public:
    static constexpr size_t button_count = 6;

private:
    static bool usbWasAlreadyInserted; // usb inserted at least once
    static bool ever_been_opened; // set by ctor
    static bool try_esp_flash; // we try this maximum once
    static bool touch_broken_during_run;

    bool usbInserted;
    MMU2::xState mmu_state { MMU2::xState::Stopped };
    bool event_in_progress { false };
    bool first_event { true };
    static bool need_check_wifi_credentials;
    MediaState_t media_event { MediaState_t::unknown };

    window_header_t header;
    StatusFooter footer;

#ifdef USE_ST7789
    window_icon_t logo;
#endif // USE_ST7789
    WindowMultiIconButton w_buttons[button_count];
    window_text_t w_labels[button_count];

public:
    static void SetTouchBrokenDuringRun() { touch_broken_during_run = true; }
    static bool EverBeenOpened() { return ever_been_opened; }
    screen_home_data_t();
    virtual ~screen_home_data_t() override;

    virtual void InitState(screen_init_variant var) override;
    virtual screen_init_variant GetCurrentState() const override;

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    void printBtnEna();
    void printBtnDis();
    void filamentBtnSetState(MMU2::xState mmu);

    static bool find_latest_gcode(char *fpath, int fpath_len, char *fname, int fname_len);

    void on_enter();
    void handle_crash_dump();
    void handle_wifi_credentials();
};
