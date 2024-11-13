// screen_home.hpp
#pragma once
#include "window_header.hpp"
#include "status_footer.hpp"
#include "gui.hpp"
#include "screen.hpp"
#include "gcode_info.hpp"
#include "window_dlg_wait.hpp"
#include <guiconfig/guiconfig.h>
#include <option/has_mmu2.h>
#include <option/has_nfc.h>

#if HAS_MMU2()
    #include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_state.h"
#endif

#if HAS_NFC()
    #include <nfc.hpp>
    #include <optional>
#endif

class screen_home_data_t : public screen_t {
public:
    static constexpr size_t button_count = 6;

private:
    static bool usbWasAlreadyInserted; // usb inserted at least once
    static bool ever_been_opened; // set by ctor
    static bool try_esp_flash; // we try this maximum once

    std::array<char, 32> header_text;

    bool usbInserted;
    bool event_in_progress { false };
    bool first_event { true };
    static bool need_check_wifi_credentials;
    MediaState_t media_event { MediaState_t::unknown };

#if HAS_MMU2()
    MMU2::xState mmu_state { MMU2::xState::Stopped };
#endif

    window_header_t header;
    StatusFooter footer;

#if HAS_MINI_DISPLAY()
    window_icon_t logo;
#endif
    WindowMultiIconButton w_buttons[button_count];
    window_text_t w_labels[button_count];

#if HAS_NFC()
    std::optional<nfc::SharedEnabler> nfc_enable { std::in_place };
    void update_nfc_state();
#endif

public:
    static bool EverBeenOpened() { return ever_been_opened; }
    screen_home_data_t();
    ~screen_home_data_t();

    virtual void InitState(screen_init_variant var) override;
    virtual screen_init_variant GetCurrentState() const override;

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    void printBtnEna();
    void printBtnDis();
    void filamentBtnSetState();

    void on_enter();
    void handle_crash_dump();
    void handle_wifi_credentials();
};
