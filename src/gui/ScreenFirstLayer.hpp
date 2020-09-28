//ScreenFirstLayer.hpp
#pragma once
#include "IScreenPrinting.hpp"
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"
#include "resource.h"
#include "window_print_progress.hpp"
#include "liveadjust_z.hpp"

class ScreenFirstLayer : public IScreenPrinting {
    static constexpr const char *caption = N_("First Layer Calibration");
    static constexpr const char *text_str = N_("Once the printer   \n"
                                               "starts extruding   \n"
                                               "plastic, adjust    \n"
                                               "the nozzle height  \n"
                                               "by turning the knob\n"
                                               "until the filament \n"
                                               "sticks to the print\n"
                                               "sheet.");
    window_text_t text;
    WindowPrintProgress progress;
    WindowLiveAdjustZ_withText live_z;

    virtual void stopAction() override;
    virtual void pauseAction() override;
    virtual void tuneAction() override;

public:
    ScreenFirstLayer();
};
