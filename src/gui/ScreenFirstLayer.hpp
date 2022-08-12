//ScreenFirstLayer.hpp
#pragma once
#include "IScreenPrinting.hpp"
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "resource.h"
#include "window_print_progress.hpp"
#include "liveadjust_z.hpp"

class ScreenFirstLayer : public IScreenPrinting {
    static constexpr const char *caption = N_("First Layer Calibration");
    static constexpr const char *text_str = N_(
        "Once the printer starts extruding plastic, adjust the nozzle height by turning the knob until the filament sticks to the print sheet.");
    window_text_t text;
    WindowPrintProgress progress;
    WindowLiveAdjustZ_withText live_z;

    virtual void stopAction() override;
    virtual void pauseAction() override;
    virtual void tuneAction() override;

public:
    ScreenFirstLayer();

private:
    virtual void notifyMarlinStart() override;
    void windowEvent(EventLock lock, window_t *sender, GUI_event_t event, void *param) override;
};
