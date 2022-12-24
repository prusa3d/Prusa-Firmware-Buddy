// screen_menus.hpp
#pragma once
#include "ScreenFactory.hpp"

ScreenFactory::UniquePtr GetScreenMenuConnect();
ScreenFactory::UniquePtr GetScreenMenuExperimentalSettings();
ScreenFactory::UniquePtr GetScreenMenuEepromDiagnostics();
ScreenFactory::UniquePtr GetScreenMenuNetwork();
ScreenFactory::UniquePtr GetScreenMenuTest();
