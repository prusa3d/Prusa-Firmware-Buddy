// screen_menus.hpp
#pragma once
#include "ScreenFactory.hpp"

ScreenFactory::UniquePtr GetScreenMenuEepromDiagnostics();
ScreenFactory::UniquePtr GetScreenMenuNetwork();
ScreenFactory::UniquePtr GetScreenMenuTest();
