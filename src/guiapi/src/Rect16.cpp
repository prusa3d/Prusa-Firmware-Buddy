#include "Rect16.h"

Rect16::Rect16(point_i16_t p0, point_i16_t p1)
    : top_left_(p0) {
    if (p1.x < top_left_.x)
        std::swap(p1.x, top_left_.x);
    if (p1.y < top_left_.y)
        std::swap(p1.y, top_left_.y);
    width_ = p1.x - top_left_.x + 1;
    height_ = p1.y - top_left_.y + 1;
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
    point_i16_t top_left;
    point_i16_t bot_right;

    // If one Rect16 is on left side of other
    if (TopLeft().x > r.BottomRight().x
        || r.TopLeft().x > BottomRight().x)
        return Rect16();
    else {
        top_left.x = TopLeft().x > r.TopLeft().x
            ? TopLeft().x
            : r.TopLeft().x;
        bot_right.x = BottomRight().x < r.BottomRight().x
            ? BottomRight().x
            : r.BottomRight().x;
    }

    // If one Rect16 is above other
    if (TopLeft().y > r.BottomRight().y
        || r.TopLeft().y > BottomRight().y)
        return Rect16();
    else {
        top_left.y = TopLeft().y > r.TopLeft().y
            ? TopLeft().y
            : r.TopLeft().y;
        bot_right.y = BottomRight().y < r.BottomRight().y
            ? BottomRight().y
            : r.BottomRight().y;
    }
    return Rect16 { top_left, bot_right };
}

Rect16 Rect16::Union(Rect16 const &r) const {
    point_i16_t top_left;
    point_i16_t bot_right;

    top_left.x = TopLeft().x < r.TopLeft().x
        ? TopLeft().x
        : r.TopLeft().x;
    bot_right.x = BottomRight().x > r.BottomRight().x
        ? BottomRight().x
        : r.BottomRight().x;
    top_left.y = TopLeft().y < r.TopLeft().y
        ? TopLeft().y
        : r.TopLeft().y;
    bot_right.y = BottomRight().y > r.BottomRight().y
        ? BottomRight().y
        : r.BottomRight().y;

    return Rect16 { top_left, bot_right };
}

bool Rect16::HasIntersection(Rect16 const &r) const {
    return TopLeft().x < r.EndPoint().x
        && EndPoint().x > r.TopLeft().x
        && TopLeft().y < r.EndPoint().y
        && EndPoint().y > r.TopLeft().y;
}

bool Rect16::Contain(Rect16 const &r) const {
    return Contain(r.TopLeft()) && Contain(r.BottomRight());
}

void Rect16::Align(Rect16 rc, uint8_t align) {
    switch (align & ALIGN_HMASK) {
    case ALIGN_LEFT:
        top_left_.x = rc.Left();
        break;
    case ALIGN_RIGHT:
        top_left_.x = ((rc.Left() + rc.Width()) > width_) ? ((rc.Left() + rc.Width()) - width_) : 0;
        break;
    case ALIGN_HCENTER:
        if (rc.Width() >= width_)
            top_left_.x = rc.Left() + (rc.Width() - width_) / 2;
        else
            top_left_.x = std::max(0, rc.Left() - (width_ - rc.Width()) / 2);
        break;
    }

    switch (align & ALIGN_VMASK) {
    case ALIGN_TOP:
        top_left_.y = rc.Top();
        break;
    case ALIGN_BOTTOM:
        top_left_.y = ((rc.Top() + rc.Height()) > height_) ? ((rc.Top() + rc.Height()) - height_) : 0;
        top_left_.y = std::max(0, (rc.Top() + rc.Height()) - height_);
        break;
    case ALIGN_VCENTER:
        if (rc.Height() >= height_)
            top_left_.y = rc.Top() + ((rc.Height() - height_) / 2);
        else
            top_left_.y = (rc.Top() > ((height_ - rc.Height()) / 2)) ? rc.Top() - ((height_ - rc.Height()) / 2) : 0;
        break;
    }
}

void Rect16::VerticalSplit(Rect16 splits[], Rect16 spaces[], size_t count, uint16_t spacing) const {
    if (count == 0)
        return;
    if (count == 1) {
        splits[0] = *this;
        return;
    }

    uint16_t width = Width() / count - spacing * (count - 1);

    Rect16 rc({ 0, Top() }, width, Height());
    Rect16 rc_space({ 0, Top() }, spacing, Height());
    size_t index;
    size_t right = EndPoint().x - width;

    for (index = 0; index < count / 2; ++index) {
        splits[index] = rc + Rect16::Left_t(Left() + index * (width + spacing));            // 1 from begin
        splits[count - 1 - index] = rc + Rect16::Left_t(right - index * (width + spacing)); // 1 from end
    }

    //even count
    //middle rect can be bit smaller, so spacing remains the same
    if (count & 0x01) {
        point_i16_t p0 = splits[index - 1].EndPoint();
        point_i16_t p1 = splits[index + 1].TopLeft();
        p0.x += spacing;
        p1.x -= spacing;
        splits[index] = Rect16(p0, p1);
    }

    for (index = 0; index < count - 1; ++index) {
        spaces[index] = Rect16(splits[index].EndPoint(), splits[index + 1].TopLeft());
    }
}
