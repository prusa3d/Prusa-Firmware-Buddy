// screen_menus.hpp
#pragma once
#include "ScreenFactory.hpp"

ScreenFactory::UniquePtr GetScreenMenuInfo();
ScreenFactory::UniquePtr GetScreenMenuSettings();
ScreenFactory::UniquePtr GetScreenMenuPreheat();
ScreenFactory::UniquePtr GetScreenMenuCalibration();
ScreenFactory::UniquePtr GetScreenMenuFilament();
