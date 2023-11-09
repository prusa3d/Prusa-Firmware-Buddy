/**
 * @file footer_icon.cpp
 * @author Radek Vana
 * @date 2021-03-31
 */

#include "footer_icon.hpp"
#include "guitypes.hpp"

FooterIcon::FooterIcon(window_t *parent, const img::Resource *icon)
    : AddSuperWindow<window_icon_t>(
        parent,
        [parent, icon] {
            if (!parent || !icon) {
                return Rect16(); // does not have parrent, cannot calculate X and Y pos
            }

            point_i16_t pt = { 0, 0 };
            size_ui16_t sz = { icon->w, icon->h };
            size_ui16_t parent_sz = parent->GetRect().Size();

            // limit width
            sz.w = std::min(sz.w, parent_sz.w);

            // limit height and calculate virtual padding
            if (sz.h && sz.h < parent_sz.h) {
                // do not use padding to draw faster
                pt.y += (parent_sz.h - sz.h) / 2;
            } else {
                sz.h = parent_sz.h;
            }

            return Rect16(pt, sz);
        }(),
        icon) {
}
