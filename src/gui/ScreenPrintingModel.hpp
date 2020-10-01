//ScreenPrintingModel.hpp
#pragma once

#include "IScreenPrinting.hpp"
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"
#include "resource.h"

struct btn_resource {
    uint16_t ico;
    const char *txt;
};

static constexpr btn_resource res_tune = { IDR_PNG_menu_icon_settings, N_("Tune") };
static constexpr btn_resource res_pause = { IDR_PNG_menu_icon_pause, (const char *)(N_("Pause")) };
static constexpr btn_resource res_stop = { IDR_PNG_menu_icon_stop, N_("Stop") };

class ScreenPrintingModel : public AddSuperWindow<IScreenPrinting> {
protected:
    struct btn {
        window_icon_button_t ico;
        window_text_t txt;
    };

    btn btn_tune;
    btn btn_pause;
    btn btn_stop;

    void initBtnText(btn &ref_button); // could access just text, but accessing entire button is more general
    void setIconAndLabel(btn &ref_button, const btn_resource &res);
    void initAndsetIconAndLabel(btn &ref_button, const btn_resource &res);

public:
    ScreenPrintingModel(string_view_utf8 caption);
};
