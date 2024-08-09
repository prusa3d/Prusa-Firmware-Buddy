#pragma once

#include <meta_utils.hpp>

#include <i_window_menu_item.hpp>
#include <WindowMenuItems.hpp>
#include <screen_menu.hpp>

class MI_LOADED_FILAMENT : public IWindowMenuItem {

public:
    enum class DisplayFormat {
        /// On multi-slot printer: "Loaded filaments"
        /// On single-slot printer: "Loaded filament (PLA)"
        auto_submenu,

        /// "Slot N (PLA)"
        slot_number,
    };

public:
    MI_LOADED_FILAMENT(DisplayFormat display_format = DisplayFormat::auto_submenu, uint8_t tool = 0);

protected:
    virtual void click(IWindowMenu &) override;

private:
    DisplayFormat display_format_;
    uint8_t tool_;
    bool should_open_submenu_;
    FilamentType filament_type_;

    std::array<char, 32> label_buffer_;
};

template <typename>
struct ScreenLoadedFilaments_ {};

template <size_t... i>
struct ScreenLoadedFilaments_<std::index_sequence<i...>> {
    using T = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, WithConstructorArgs<MI_LOADED_FILAMENT, MI_LOADED_FILAMENT::DisplayFormat::slot_number, i>...>;
};

class ScreenLoadedFilaments : public ScreenLoadedFilaments_<std::make_index_sequence<EXTRUDERS>>::T {
public:
    ScreenLoadedFilaments();
};
