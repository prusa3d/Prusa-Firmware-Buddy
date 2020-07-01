#include "ScreenFactory.hpp"
#include "../lang/i18n.h"

ScreenFactory::mem_space ScreenFactory::all_screens;

ScreenFactory::UniquePtr ScreenFactory::ScreenHome() {
    return make_static_unique_ptr<screen_home_data_t>(&all_screens);
}

ScreenFactory::UniquePtr ScreenFactory::ScreenSplash() {
    return make_static_unique_ptr<screen_splash_data_t>(&all_screens);
}
