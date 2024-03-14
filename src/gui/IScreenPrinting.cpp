#include "IScreenPrinting.hpp"
#include "config.h"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "img_resources.hpp"

IScreenPrinting::IScreenPrinting(string_view_utf8 caption)
    : AddSuperWindow<screen_t>()
    , header(this)
    , footer(this) {
    IScreenPrinting::ClrMenuTimeoutClose(); // don't close on menu timeout
    header.SetText(caption);
    header.SetIcon(&img::print_16x16);
    ths = this;
}

IScreenPrinting::~IScreenPrinting() {
    ths = nullptr;
}

IScreenPrinting *IScreenPrinting::ths = nullptr;

/******************************************************************************/
// static methods to be pointed by fnc pointers
void IScreenPrinting::StopAction() {
    if (IScreenPrinting::ths) {
        IScreenPrinting::ths->stopAction();
    }
}
void IScreenPrinting::PauseAction() {
    if (IScreenPrinting::ths) {
        IScreenPrinting::ths->pauseAction();
    }
}
void IScreenPrinting::TuneAction() {
    if (IScreenPrinting::ths) {
        IScreenPrinting::ths->tuneAction();
    }
}

IScreenPrinting *IScreenPrinting::GetInstance() {
    return IScreenPrinting::ths;
}

void IScreenPrinting::NotifyMarlinStart() {
    if (ths) {
        ths->notifyMarlinStart();
    }
}
