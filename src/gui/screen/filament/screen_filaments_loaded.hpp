#pragma once

#include <i_window_menu_item.hpp>
#include <tool_index.hpp>
#include <gui_utils.hpp>
#include <filament.hpp>

class MI_LOADED_FILAMENT final : public IWindowMenuItem {

public:
    enum class DisplayFormat {
        /// On multi-slot printer: "Loaded filaments"
        /// On single-slot printer: "Loaded filament (PLA)"
        auto_submenu,

        /// "Slot N (PLA)"
        slot_number,
    };

public:
    MI_LOADED_FILAMENT(DisplayFormat display_format = DisplayFormat::auto_submenu, VirtualToolIndex tool = VirtualToolIndex::from_raw(0));

protected:
    virtual void click(IWindowMenu &) override;
    virtual void Loop() override;
    void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const override;

private:
    std::array<char, 32> info_buffer_;
    StringViewUtf8Parameters<4> label_params_;
    MenuItemColorSwatchExtension extension_;
    VirtualToolIndex tool_;
    FilamentType filament_type_;
    DisplayFormat display_format_ : 2;
    bool should_open_submenu_ : 1;
};
