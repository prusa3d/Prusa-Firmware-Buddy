// ScreenFirstLayer.cpp
#include "ScreenFirstLayer.hpp"
#include "config.h"
#include "marlin_client.h"
#include "marlin_server.h"
#include "guitypes.hpp"    //font_meas_text
#include "stm32f4xx_hal.h" //HAL_GetTick
#include "i18n.h"
#include "ScreenHandler.hpp"

ScreenFirstLayer::ScreenFirstLayer(string_view_utf8 caption)
    : IScreenPrinting(caption)
    , header(this)
    , footer(this) {
    header.SetText(caption);
}
