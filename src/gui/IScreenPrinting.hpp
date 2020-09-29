//IScreenPrinting.hpp
#pragma once
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"
#include "resource.h"

class IScreenPrinting : public AddSuperWindow<window_frame_t> {
protected:
    window_header_t header;
    status_footer_t footer;

    static IScreenPrinting *ths;
    static void StopAction();
    static void PauseAction();
    static void TuneAction();
    virtual void stopAction() = 0;
    virtual void pauseAction() = 0;
    virtual void tuneAction() = 0;

public:
    IScreenPrinting(string_view_utf8 caption);
    ~IScreenPrinting();
    static bool CanOpen();
};
