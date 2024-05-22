#pragma once

#include <window.hpp>

/**
 * @brief Draws connectors between two lines of coordinates (columns), so that if the line isn't a single perpendicular line, there is a middle 'joint' added to it so that it ends up being 3 perpendicular ones.
 * Example in vertical mode (1 is connected to A B C and 4 is connected to D E):

 *   ------------------------------------------
 *  |         Parent         Child             |
 *  |           1  ----------  A               |
 *  |                   |                      |
 *  |           2       |----  B               |
 *  |                   |                      |
 *  |           3        ----  C               |
 *  |                                          |
 *  |           4  ----------  D               |
 *  |                   |                      |
 *  |           5        ----  E               |
 *  |                                          |
 *   ------------------------------------------

 * It is left to the user to make sure the resulting 'diagram' looks nice, ie adding 3 -> E would make the example very ugly. Parent side can have 1 - N connections to the child side, whilst child side can have only 0 - 1 connections with the parent side
 */
class window_line_connector : public AddSuperWindow<window_aligned_t> {
public:
    enum class ParentSide : uint8_t {
        top,
        bot,
        left,
        right
    };

    window_line_connector(window_t *parent, Rect16 rect);

    static constexpr size_t max_one_side_points { 8 };
    using PointsT = std::array<uint16_t, max_one_side_points>;
    using ParentsT = std::array<uint16_t, max_one_side_points>;
    static constexpr size_t unassigned_value { std::numeric_limits<uint16_t>::max() };

    void set_parent_side(ParentSide new_side);
    void set_points(PointsT new_points);
    void set_parents(ParentsT new_parents);
    void set_line_thickness(uint16_t new_value);

protected:
    void unconditionalDraw() override;

private:
    ParentSide parent_side { ParentSide::left }; // whether the middle connection line is vertical or horizontal (this has an effect on everything), is dependant on the parent side

    PointsT points {}; //  points need to be on a perpendicular line (same row in vertical mode), so only row coords are needed to be stored
    ParentsT parents {}; // holds connections between child side (index) and parent side (value)

    uint16_t line_thickness { 5 };
};
