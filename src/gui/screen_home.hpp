//screen_home.hpp
#pragma once
#include "window_header.hpp"
#include "status_footer.hpp"
#include "gui.hpp"
#include "screen.hpp"
#include "gcode_info.hpp"
#include "window_dlg_wait.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu_state.h"

class screen_home_data_t : public AddSuperWindow<screen_t> {
public:
    static constexpr size_t button_count = 6;

private:
    static bool usbWasAlreadyInserted; // usb inserted at least once
    static bool ever_been_opened;      //set by ctor
    static bool try_esp_flash;         // we try this maximum once
    static bool touch_broken_during_run;

    bool usbInserted;
    MMU2::State_t mmu_state;
    bool event_in_progress;
    bool first_event { true };

    window_header_t header;
    StatusFooter footer;

    window_icon_t logo;
    WindowMultiIconButton w_buttons[button_count];
    window_text_t w_labels[button_count];

    GCodeInfo &gcode;

    window_dlg_wait_t please_wait_msg; ///< Message shown while a file is being parsed

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
    virtual void draw() override;
    virtual void unconditionalDraw() override;

    void printBtnEna();
    void printBtnDis();
    void filamentBtnSetState(MMU2::State_t mmu);

    static bool find_latest_gcode(char *fpath, int fpath_len, char *fname, int fname_len);

    void on_enter();
    void handle_crash_dump();
};
