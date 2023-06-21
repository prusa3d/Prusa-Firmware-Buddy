/**
 * @file print_processor.hpp
 * @author Radek Vana
 * @brief api header marlin, meant to be switched with other in unit tests
 * @date 2021-02-11
 */

#pragma once

#include <stdint.h>

class PrintProcessor {
    // called when Serial print screen is opened
    // printer is not in sd printing mode, so filament sensor does not trigger M600
    static void fsm_cb(uint32_t u32, uint16_t u16);

public:
    static void Update();
    static void InjectGcode(const char *str);
    static bool IsPrinting();
    static bool IsAutoloadEnabled();

    static void Init();
};
