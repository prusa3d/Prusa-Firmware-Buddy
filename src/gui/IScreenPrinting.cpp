#include "IScreenPrinting.hpp"
#include "config.h"
#include "marlin_client.h"
#include "marlin_server.h"
#include "guitypes.hpp"    //font_meas_text
#include "stm32f4xx_hal.h" //HAL_GetTick
#include "i18n.h"
#include "ScreenHandler.hpp"

IScreenPrinting::IScreenPrinting(string_view_utf8 caption)
    : window_frame_t()
    , header(this)
    , footer(this) {
    header.SetText(caption);
    ths = this;
}

IScreenPrinting::~IScreenPrinting() {
    ths = nullptr;
}

IScreenPrinting *IScreenPrinting::ths = nullptr;

/******************************************************************************/
//static methods to be pointed by fnc pointers
void IScreenPrinting::StopAction() {
    if (IScreenPrinting::ths)
        IScreenPrinting::ths->stopAction();
}
void IScreenPrinting::PauseAction() {
    if (IScreenPrinting::ths)
        IScreenPrinting::ths->pauseAction();
}
void IScreenPrinting::TuneAction() {
    if (IScreenPrinting::ths)
        IScreenPrinting::ths->tuneAction();
}

bool IScreenPrinting::CanOpen() {
    return IScreenPrinting::ths == nullptr;
}
