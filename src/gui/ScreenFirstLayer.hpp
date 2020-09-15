//ScreenFirstLayer.hpp
#pragma once
#include "IScreenPrinting.hpp"
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"
#include "resource.h"

class ScreenFirstLayer : public IScreenPrinting {

    static constexpr const char *caption = "First Layer Calibration";

private:
    virtual void stopAction() override;
    virtual void pauseAction() override;
    virtual void tuneAction() override;

protected:
    window_header_t header;
    status_footer_t footer;

public:
    ScreenFirstLayer();
};
