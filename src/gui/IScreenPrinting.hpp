// IScreenPrinting.hpp
#pragma once
#include "gui.hpp"
#include "screen.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"

class IScreenPrinting : public screen_t {
protected:
    window_header_t header;
    StatusFooter footer;

    static IScreenPrinting *ths;
    static void StopAction();
    static void PauseAction();
    static void TuneAction();
    virtual void stopAction() = 0;
    virtual void pauseAction() = 0;
    virtual void tuneAction() = 0;

public:
    IScreenPrinting(const string_view_utf8 &caption);
    ~IScreenPrinting();
    static IScreenPrinting *GetInstance();
    static void NotifyMarlinStart();
    virtual void on_message(const char *) {}

private:
    virtual void notifyMarlinStart() {};
};
