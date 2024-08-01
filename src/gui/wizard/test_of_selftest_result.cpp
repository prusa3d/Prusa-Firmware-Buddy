/**
 * @file test_of_selftest_result.cpp
 * @author Radek Vana
 * @date 2022-01-21
 */

#include "test_of_selftest_result.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "img_resources.hpp"

TestResultScreen::TestResultScreen()
    : screen_t()
    , header(this, string_view_utf8::MakeCPUFLASH((uint8_t *)"RESULT TEST"))
    , result(this, PhasesSelftest::Result, somethingToShow()) {
    ClrMenuTimeoutClose(); // don't close on menu timeout
    header.SetIcon(&img::selftest_16x16);

    CaptureNormalWindow(result);
}
