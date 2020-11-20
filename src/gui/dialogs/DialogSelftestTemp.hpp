#pragma once

#include "DialogStateful.hpp"
#include "window_icon.hpp"

class DialogSelftestTemp : public IDialogMarlin {
    window_text_t text_checking_temp;
    window_numberless_progress_t progress;
    window_text_t text_noz;
    window_text_t text_noz_cool;
    WindowIcon_OkNg icon_noz_cool;
    window_text_t text_noz_heat;
    WindowIcon_OkNg icon_noz_heat;
    window_text_t text_bed;
    window_text_t text_bed_cool;
    WindowIcon_OkNg icon_bed_cool;
    window_text_t text_bed_heat;
    WindowIcon_OkNg icon_bed_heat;

protected:
    virtual bool change(uint8_t phs, uint8_t progress_tot, uint8_t progress) override;

public:
    DialogSelftestTemp();
};
