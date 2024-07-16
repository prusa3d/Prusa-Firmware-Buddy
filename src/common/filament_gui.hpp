#pragma once

#include <i_window_menu_item.hpp>
#include <img_resources.hpp>
#include <filament.hpp>

/// Collection of utilities for filament-related GUI
struct FilamentTypeGUI {

public:
    /// Sets up menu item to represent the filament type:
    /// * Sets up label
    /// * Possibly changes color scheme
    /// * Possibly sets up icon
    /// \param params must be alive for the whole lifespan of the \p item !
    static void setup_menu_item(FilamentType ft, const FilamentTypeParameters &params, IWindowMenuItem &item);
};
