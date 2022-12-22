#pragma once

#include "screen_home.hpp"
#include "screen_splash.hpp"
#include "screen_menu_info.hpp"
#include "screen_menu_settings.hpp"
#include "screen_menu_tune.hpp"
#include "screen_menu_calibration.hpp"
#include "screen_menu_filament.hpp"
#include "screen_menu_temperature.hpp"

#include "static_alocation_ptr.hpp"
#include <array>

class ScreenFactory {
    // The various menu sizes cannot be accessed from here (as we use them as an argument of
    // aligned_union) so the minimal size is currently hard-coded.
    // The tune menu is usually the biggest: to calculate the size put something like this this:
    //   char (*size_msg_as_error)[sizeof( ScreenMenuTune )] = 1;
    // somewhere in the affected source file (changing the menu class appropriately).
#if _DEBUG
    static constexpr size_t min_union_size = 3628;
#else
    static constexpr size_t min_union_size = 3000;
#endif

    ScreenFactory() = delete;
    ScreenFactory(const ScreenFactory &) = delete;
    using mem_space = std::aligned_union<min_union_size,
        ScreenMenuCalibration,
        ScreenMenuFilament,
        ScreenMenuInfo,
        ScreenMenuSettings,
        ScreenMenuTemperature,
        ScreenMenuTune,
        screen_home_data_t, screen_splash_data_t>::type;

    static mem_space all_screens;

public:
    using UniquePtr = static_unique_ptr<screen_t>;
    using Creator = static_unique_ptr<screen_t> (*)(); //function pointer definition

    template <class T>
    static UniquePtr Screen() {
        static_assert(sizeof(T) <= sizeof(mem_space), "Screen memory space is too small");
        return make_static_unique_ptr<T>(&all_screens);
    }

    template <class T>
    static bool DoesCreatorHoldType(Creator cr) {
        return Screen<T> == cr;
    }
};
