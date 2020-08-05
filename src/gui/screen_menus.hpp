// screen_menus.hpp
#pragma once
#include "ScreenFactory.hpp"

ScreenFactory::UniquePtr GetScreenMenuInfo();
ScreenFactory::UniquePtr GetScreenMenuSettings();
ScreenFactory::UniquePtr GetScreenMenuPreheat();
ScreenFactory::UniquePtr GetScreenMenuCalibration();
ScreenFactory::UniquePtr GetScreenMenuFilament();
ScreenFactory::UniquePtr GetScreenMenuTemperature();
ScreenFactory::UniquePtr GetScreenMenuMove();
ScreenFactory::UniquePtr GetScreenMenuVersionInfo();
ScreenFactory::UniquePtr GetScreenMenuTune();
//ScreenFactory::UniquePtr GetScreenMenuService();
ScreenFactory::UniquePtr GetScreenMenuFwUpdate();
ScreenFactory::UniquePtr GetScreenMenuLanguages();
ScreenFactory::UniquePtr GetScreenMenuLanguagesNoRet();
