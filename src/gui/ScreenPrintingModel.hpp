//ScreenPrintingModel.hpp
#pragma once

#include "IScreenPrinting.hpp"
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "resource.h"

struct btn_resource {
    png::Id ico;
    const char *txt;
};

static constexpr btn_resource res_tune = { png::Id({ PNG::settings_58x58 }), N_("Tune") };
static constexpr btn_resource res_pause = { png::Id({ PNG::pause_58x58 }), N_("Pause") };
static constexpr btn_resource res_stop = { png::Id({ PNG::stop_58x58 }), N_("Stop") };

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
    void initAndSetIconAndLabel(btn &ref_button, const btn_resource &res);

public:
    ScreenPrintingModel(string_view_utf8 caption);
};
