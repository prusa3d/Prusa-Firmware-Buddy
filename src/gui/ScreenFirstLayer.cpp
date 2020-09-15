// ScreenFirstLayer.cpp
#include "ScreenFirstLayer.hpp"
#include "config.h"
#include "marlin_client.h"
#include "marlin_server.h"
#include "guitypes.hpp"    //font_meas_text
#include "stm32f4xx_hal.h" //HAL_GetTick
#include "i18n.h"
#include "ScreenHandler.hpp"

ScreenFirstLayer::ScreenFirstLayer()
    : IScreenPrinting(string_view_utf8::MakeCPUFLASH((const uint8_t *)caption))
    , header(this)
    , footer(this) {
}

void ScreenFirstLayer::stopAction() {}
void ScreenFirstLayer::pauseAction() {}
void ScreenFirstLayer::tuneAction() {}
