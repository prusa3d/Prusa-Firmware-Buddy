#include "Rect16.h"

Rect16::Rect16() {
    top_left_ = { 0, 0 };
    width_ = 0;
    height_ = 0;
}

Rect16::Rect16(
    int16_t left,
    int16_t top,
    int16_t right,
    int16_t bottom) {
    width_ = right < left ? 0 : right - left;
    height_ = top > bottom ? 0 : bottom - top;

    top_left_ = (width_ > 0 && height_ > 0)
        ? point_i16_t { left, top }
        : point_i16_t { 0, 0 };
}

Rect16::Rect16(point_i16_t top_left, uint16_t width, uint16_t height) {
    top_left_ = top_left;
    width_ = width;
    height_ = height;
}

Rect16::Rect16(Rect16 const &rect, ShiftDir_t direction, uint16_t distance) {
    switch (direction) {
    case ShiftDir_t::Left:
        top_left_ = {
            static_cast<int16_t>(rect.TopLeft().x - distance),
            rect.TopLeft().y
        };
        break;
    case ShiftDir_t::Right:
        top_left_ = {
            static_cast<int16_t>(rect.TopLeft().x + distance),
            rect.TopLeft().y
        };
        break;
    case ShiftDir_t::Top:
        top_left_ = {
            rect.TopLeft().x,
            static_cast<int16_t>(rect.TopLeft().y - distance)
        };
        break;
    case ShiftDir_t::Bottom:
        top_left_ = {
            rect.TopLeft().x,
            static_cast<int16_t>(rect.TopLeft().y + distance)
        };
        break;
    default:
        top_left_ = rect.TopLeft();
        break;
    }
    width_ = rect.Width();
    height_ = rect.Height();
}

Rect16::Rect16(point_i16_t top_left, size_ui16_t s) {
    top_left_ = top_left;
    width_ = s.w;
    height_ = s.h;
}

Rect16 Rect16::Intersection(Rect16 const &r) const {
    int16_t min_x, max_x;
    int16_t min_y, max_y;

    // If one Rect16 is on left side of other
    if (TopLeft().x >= r.BottomRight().x
        || r.TopLeft().x >= BottomRight().x)
        return Rect16();
    else {
        min_x = TopLeft().x > r.TopLeft().x
            ? TopLeft().x
            : r.TopLeft().x;
        max_x = BottomRight().x < r.BottomRight().x
            ? BottomRight().x
            : r.BottomRight().x;
    }

    // If one Rect16 is above other
    if (TopLeft().y >= r.BottomRight().y
        || r.TopLeft().y >= BottomRight().y)
        return Rect16();
    else {
        min_y = TopLeft().y > r.TopLeft().y
            ? TopLeft().y
            : r.TopLeft().y;
        max_y = BottomRight().y < r.BottomRight().y
            ? BottomRight().y
            : r.BottomRight().y;
    }
    return Rect16 { min_x, min_y, max_x, max_y };
}

Rect16 Rect16::Union(Rect16 const &r) const {
    int16_t min_x, max_x;
    int16_t min_y, max_y;

    min_x = TopLeft().x < r.TopLeft().x
        ? TopLeft().x
        : r.TopLeft().x;
    max_x = BottomRight().x > r.BottomRight().x
        ? BottomRight().x
        : r.BottomRight().x;
    min_y = TopLeft().y < r.TopLeft().y
        ? TopLeft().y
        : r.TopLeft().y;
    max_y = BottomRight().y > r.BottomRight().y
        ? BottomRight().y
        : r.BottomRight().y;

    return Rect16 { min_x, min_y, max_x, max_y };
}

bool Rect16::HasIntersection(Rect16 const &r) const {
    return TopLeft().x < r.BottomRight().x
        && BottomRight().x > r.TopLeft().x
        && TopLeft().y < r.BottomRight().y
        && BottomRight().y > r.TopLeft().y;
}

bool Rect16::IsSubrectangle(Rect16 const &r) const {
    return Contain(r.TopLeft()) && Contain(r.BottomRight());
}

void Rect16::AddPadding(const padding_ui8_t p) {
    top_left_.x = top_left_.x - p.left;
    top_left_.y = top_left_.y - p.top;
    width_ += (p.left + p.right);
    height_ += (p.top + p.bottom);
}

void Rect16::CutPadding(const padding_ui8_t p) {
    if ((p.left + p.right) >= width_
        || (p.top + p.bottom) >= height_) {
        width_ = height_ = 0;
        top_left_.x = top_left_.y = 0;
    } else {
        top_left_.x = top_left_.x + p.left;
        top_left_.y = top_left_.y + p.top;
        width_ -= (p.left + p.right);
        height_ -= (p.top + p.bottom);
    }
}
