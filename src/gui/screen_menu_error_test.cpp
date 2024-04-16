#include "screen_menu_error_test.hpp"
#include <error_codes.hpp>
#include <bsod_gui.hpp>
#include <bsod.h>

static const IWindowMenuItem::ColorScheme blue_scheme = {
    .text = {
        .focused = COLOR_BLACK,
        .unfocused = COLOR_AZURE,
    },
    .back = {
        .focused = COLOR_AZURE,
        .unfocused = COLOR_BLACK,
    },
    .rop = {
        .focused = ropfn(),
        .unfocused = ropfn(),
    }
};

static const IWindowMenuItem::ColorScheme red_scheme = {
    .text = {
        .focused = COLOR_BLACK,
        .unfocused = COLOR_RED_ALERT,
    },
    .back = {
        .focused = COLOR_RED_ALERT,
        .unfocused = COLOR_BLACK,
    },
    .rop = {
        .focused = ropfn(),
        .unfocused = ropfn(),
    }
};

MI_TRIGGER_BSOD::MI_TRIGGER_BSOD()
    : IWindowMenuItem(_("Test bsod() Function"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
    set_color_scheme(&blue_scheme);
}

void MI_TRIGGER_BSOD::click([[maybe_unused]] IWindowMenu &window_menu) {
    bsod("Triggered from menu");
}

// generate stack overflow
static volatile int _recursive = 1;
static void recursive(uint64_t i) {
    volatile uint64_t x = i + (uint64_t)_recursive;
    osDelay(1);
    if (_recursive) {
        recursive(x);
    }
    (void)x;
}

MI_STACK_OVERFLOW::MI_STACK_OVERFLOW()
    : IWindowMenuItem(_("Stack Overflow"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
    set_color_scheme(&blue_scheme);
}

void MI_STACK_OVERFLOW::click([[maybe_unused]] IWindowMenu &window_menu) {
    recursive(0);
}

MI_DIV0::MI_DIV0()
    : IWindowMenuItem(_("Divide by Zero"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
    set_color_scheme(&blue_scheme);
}

void MI_DIV0::click([[maybe_unused]] IWindowMenu &window_menu) {
    static volatile int i = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiv-by-zero"
    i = i / 0;
#pragma GCC diagnostic pop
}

MI_WATCHDOG::MI_WATCHDOG()
    : IWindowMenuItem(_("Emulate Watchdog"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
    set_color_scheme(&blue_scheme);
}

void MI_WATCHDOG::click([[maybe_unused]] IWindowMenu &window_menu) {
    // This has to block marlin thread which refreshes watchdog,
    // but not 1 ms interrupt that dumps memory before hardware watchdog acts
    osThreadSuspendAll();
    while (1)
        ;
}

MI_PREHEAT_ERROR::MI_PREHEAT_ERROR()
    : IWindowMenuItem(_("Preheat Error"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
    set_color_scheme(&red_scheme);
}

void MI_PREHEAT_ERROR::click([[maybe_unused]] IWindowMenu &window_menu) {
    fatal_error(ErrCode::ERR_TEMPERATURE_HOTEND_PREHEAT_ERROR);
}

MI_TRIGGER_REDSCREEN::MI_TRIGGER_REDSCREEN()
    : IWindowMenuItem(_("Trigger Redscreen"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
    set_color_scheme(&red_scheme);
}

void MI_TRIGGER_REDSCREEN::click(IWindowMenu &) {
    fatal_error("Triggered from menu", "GUI");
}

ScreenMenuErrorTest::ScreenMenuErrorTest()
    : detail::ScreenMenuErrorTest(_(label)) {}
