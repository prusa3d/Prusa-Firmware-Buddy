#pragma once

#include <window.hpp>

/**
 * @brief Used for drawing a coloured rectangle at a given place. This rectangle is encircled by a border of black/white to make the rect stand out better (this can be disabled by setting border_thickness 0).
 *
 */
class window_colored_rect : public AddSuperWindow<window_t> {
public:
    window_colored_rect(window_t *parent, Rect16 rect);

    void set_border_thickness(uint8_t new_thickness);
    void set_parent_color(color_t new_parent_color);

protected:
    void unconditionalDraw() override;

private:
    uint8_t border_thickness { 1 }; // how thick is the encircling border, 0 for none
    color_t parent_color { COLOR_BLACK }; // Needed for rounded mode to draw the background properly
};
