#include "window_line_connector.hpp"

window_line_connector::window_line_connector(window_t *parent, Rect16 rect)
    : AddSuperWindow<window_aligned_t>(parent, rect) {
    points.fill(unassigned_value);
    parents.fill(unassigned_value);
}

void window_line_connector::set_parent_side(ParentSide new_side) {
    parent_side = new_side;
}

void window_line_connector::set_points(PointsT new_points) {
    points = new_points;
}

void window_line_connector::set_parents(ParentsT new_parents) {
    parents = new_parents;
}

void window_line_connector::set_line_thickness(uint16_t new_value) {
    line_thickness = new_value;
}

void window_line_connector::unconditionalDraw() {
    auto this_rect = GetRect();
    display::FillRect(this_rect, COLOR_BLACK);

    // horizontal mode not yet implemented
    assert(parent_side == ParentSide::left || parent_side == ParentSide::right);

    // ps -> parent side, cs -> child side
    const auto [ps_shared_coord, cs_shared_coord] = [&]() -> std::pair<uint16_t, uint16_t> {
        switch (parent_side) {
        case ParentSide::top:
            return std::make_pair(this_rect.Top(), this_rect.Bottom());
        case ParentSide::bot:
            return std::make_pair(this_rect.Bottom(), this_rect.Top());
        case ParentSide::left:
            return std::make_pair(this_rect.Left(), this_rect.Right());
        case ParentSide::right:
            return std::make_pair(this_rect.Right(), this_rect.Left());
        }
        assert(false);
        return std::make_pair(0, 0);
    }();

    const uint16_t middle_point = (ps_shared_coord + cs_shared_coord) / 2;
    const uint16_t middle_point_len = std::max(ps_shared_coord, cs_shared_coord) - middle_point;

    // for each child: draw a line from it to the parent
    for (size_t i = 0; i < std::size(parents); ++i) {
        const auto &parent = parents[i];
        if (parent == unassigned_value) {
            continue;
        }

        // drawing a line has 3 parts - draw from child to child-middle, from child-middle to parent-middle, from parent-middle to parent
        Rect16 child_to_middle_rect {
            static_cast<int16_t>(std::min(middle_point, cs_shared_coord)),
            static_cast<int16_t>(points[i]),
            middle_point_len,
            line_thickness
        };
        Rect16 middle_to_middle_rect {
            static_cast<int16_t>(middle_point),
            static_cast<int16_t>(std::min(points[i], points[parent])),
            line_thickness,
            static_cast<uint16_t>(std::max(points[i], points[parent]) - std::min(points[i], points[parent]))
        };
        Rect16 parent_to_middle_rect {
            static_cast<int16_t>(std::min(middle_point, ps_shared_coord)),
            static_cast<int16_t>(points[parent]),
            middle_point_len,
            line_thickness
        };

        display::FillRect(child_to_middle_rect, COLOR_GRAY);
        display::FillRect(middle_to_middle_rect, COLOR_GRAY);
        display::FillRect(parent_to_middle_rect, COLOR_GRAY);
    }
}
