//IScreenPrinting.hpp
#pragma once
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"
#include "resource.h"

class IScreenPrinting : public window_frame_t {
protected:
    struct btn {
        window_icon_button_t ico;
        window_text_t txt;
    };

    struct btn_resource {
        uint16_t ico;
        const char *txt;
    };

    static constexpr btn_resource res_tune = { IDR_PNG_menu_icon_settings, N_("Tune") };
    static constexpr btn_resource res_pause = { IDR_PNG_menu_icon_pause, N_("Pause") };
    static constexpr btn_resource res_stop = { IDR_PNG_menu_icon_stop, N_("Stop") };

    window_header_t header;
    status_footer_t footer;

    btn btn_tune;
    btn btn_pause;
    btn btn_stop;

public:
    IScreenPrinting(string_view_utf8 caption);

protected:
    //void enableButton(btn &ref_button);  // could access just icon, but accessing entire button is more general
    //void disableButton(btn &ref_button); // could access just icon, but accessing entire button is more general
    void initBtnText(btn &ref_button); // could access just text, but accessing entire button is more general
    void setIconAndLabel(btn &ref_button, btn_resource res);
    void initAndsetIconAndLabel(btn &ref_button, btn_resource res);
};
