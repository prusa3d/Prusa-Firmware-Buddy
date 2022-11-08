//ScreenPrintingModel.hpp
#pragma once

#include "IScreenPrinting.hpp"
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "png_resources.hpp"

static constexpr BtnResource res_tune = { N_("Tune"), &png::settings_58x58 };
static constexpr BtnResource res_pause = { N_("Pause"), &png::pause_58x58 };
static constexpr BtnResource res_stop = { N_("Stop"), &png::stop_58x58 };

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
    void setIconAndLabel(btn &ref_button, const BtnResource &res);
    void initAndSetIconAndLabel(btn &ref_button, const BtnResource &res);

public:
    ScreenPrintingModel(string_view_utf8 caption);
};
