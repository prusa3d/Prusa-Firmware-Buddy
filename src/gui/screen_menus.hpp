// screen_menus.hpp
#pragma once
#include "ScreenFactory.hpp"

ScreenFactory::UniquePtr GetScreenMenuHwSetup();
ScreenFactory::UniquePtr GetScreenMenuSteelSheets();
ScreenFactory::UniquePtr GetScreenEeprom();
ScreenFactory::UniquePtr GetScreenMenuFooterSettings();
ScreenFactory::UniquePtr GetScreenMenuFooterSettingsAdv();
ScreenFactory::UniquePtr GetScreenPrusaLink();
ScreenFactory::UniquePtr GetScreenMenuConnect();
ScreenFactory::UniquePtr GetScreenMenuExperimentalSettings();
ScreenFactory::UniquePtr GetScreenMenuEepromDiagnostics();
ScreenFactory::UniquePtr GetScreenMenuNetwork();
ScreenFactory::UniquePtr GetScreenMenuTest();
