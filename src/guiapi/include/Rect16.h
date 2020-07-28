#pragma once

#include "guitypes.h"
#include <array>
#include <climits>

////////////////////////////////////////////////////////////////////////////////
/// @enum ShiftDir_t
/// @brief A strongly typed enum class representing the direction where the
///     the next rectangle will be created
enum class ShiftDir_t {
    Left,
    Right,
    Top,
    Bottom
};

////////////////////////////////////////////////////////////////////////////////
///
/// Class prepesents rectagle in graphics system where x rising to the right
/// and y rising down.
///
/// @details Class also support negative value of coordinates. Such a simple perk
/// makes the implemetation more robust and useful.
///
class Rect16 {
    point_i16_t top_left_;
    uint16_t width_;
    uint16_t height_;

public:
    ////////////////////////////////////////////////////////////////////////////
    /// @brief Default constructor
    /// @details set top left corner to {0,0} with width and heigth 0
    Rect16();

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle on specific top-left and bottom-right
    ///        corner
    /// @param[in] x X coordinate of top-left corner
    /// @param[in] y Y coordinate of top-left corner
    /// @param[in] x X coordinate of bottom-right corner
    /// @param[in] y Y coordinate of bottom-right corner
    Rect16(int16_t, int16_t, int16_t, int16_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle on specific top-left corner and width
    ///        and heigth
    /// @param[in] point Top-left corner
    /// @param[in] width Width in pixels
    /// @param[in] heigth Heigth in pixels
    Rect16(point_i16_t, uint16_t, uint16_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Copy contructor
    Rect16(Rect16 const &);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle as a copy of given rectangle with the shift in
    ///        specific direction
    /// @param[in] rect Existing rectangle to copy
    /// @param[in] direction Direction where the created rectangle will be heading to
    /// @param[in] offset Offset in pixels of the new rectangle shift
    Rect16(Rect16 const &, ShiftDir_t, uint16_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle on specific top-left corner and size
    ///
    /// @param[in] point Top-left corner
    /// @param[in] size Size defined by width and heigth
    Rect16(point_i16_t, size_ui16_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Assign operator implementation
    ///
    /// @param[in] rect Existing rectangle to duplicate into curent one
    Rect16 &operator=(Rect16 const &);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the width of current rectangle
    ///
    /// @return Width of the rectangle.
    uint16_t Width() const { return width_; };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the height of current rectangle
    ///
    /// @return Height of the rectangle.
    uint16_t Height() const { return height_; };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the size of current rectangle
    ///
    /// @return Size of the rectangle.
    size_ui16_t Size() const { return size_ui16_t { width_, height_ }; }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the top-left corner of current rectangle
    ///
    /// @return Top-left corner of the rectangle.
    point_i16_t TopLeft() const { return top_left_; };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the bottom-right of current rectangle
    ///
    /// @return Bottom-right of the rectangle.
    point_i16_t BottomRight() const {
        return {
            static_cast<int16_t>(top_left_.x + width_),
            static_cast<int16_t>(top_left_.y + height_)
        };
    };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Determines if the given point is placed inside of the curent rectangle
    ///
    /// @param[in] point Point given to check
    /// @return Return true if the point is in, false otherwise.
    bool Contain(point_i16_t point) const {
        return point.x >= top_left_.x
            && point.x <= (top_left_.x + width_)
            && point.y >= top_left_.y
            && point.y <= (top_left_.y + height_);
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Determines the rectangle structure that represents the intersection
    /// of two rectangles.
    ///
    /// @param[in] rect Given rectangle to check for intersection
    /// @return Return a rectangle that represents the intersection of current
    /// and given.
    Rect16 Intersection(Rect16 const &) const;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Determines the rectangle structure that represents the union of
    /// two rectangles.
    ///
    /// @param[in] rect Given rectangle to united
    /// @return Return a rectangle that represents the union of current
    /// and given.
    Rect16 Union(Rect16 const &) const;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Determines if the given rectangle has intersection with the current
    /// one
    /// @param[in] rect Rectangle given to check
    /// @return Return true if the rectangles has intersection, false otherwise.
    bool HasIntersection(Rect16 const &) const;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Add pixels to given direction
    /// @details Such a method modify the original rectangle. The pixels will be
    /// added to origin coordinations and recent accessor call returns different
    /// value
    /// @param[in] padding Given padding structure that specify additional pixels
    void AddPadding(const padding_ui8_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Subtract pixels from given direction
    /// @details Such a method modify the original rectangle. The pixels will be
    /// deducted from origin coordinations and recent accessor call returns
    /// different value
    /// @param[in] padding Given padding structure that specify deducted pixels
    void CutPadding(const padding_ui8_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Static method determines the rectangle structure that represents
    /// the merge of all given rectangles.
    /// @details The method is static version of Union
    /// @param[in] rectangles Collection of rectangles to merge
    /// @return Return a rectangle that represents the union of all rectangles
    template <size_t SZ>
    static Rect16 Merge(std::array<Rect16, SZ> const &rectangles) {
        int16_t min_x = SHRT_MAX, min_y = SHRT_MAX;
        int16_t max_x = SHRT_MIN, max_y = SHRT_MIN;

        for (size_t i = 0; i < SZ; ++i) {
            if (rectangles[i].Width() > 0) {
                min_x = rectangles[i].TopLeft().x < min_x ? rectangles[i].TopLeft().x : min_x;
                min_y = rectangles[i].TopLeft().y < min_y ? rectangles[i].TopLeft().y : min_y;
                max_x = rectangles[i].BottomRight().x > max_x ? rectangles[i].BottomRight().x : max_x;
                max_y = rectangles[i].BottomRight().y > max_y ? rectangles[i].BottomRight().y : max_y;
            }
        }
        return { min_x, min_y, max_x, max_y };
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Determines the rectangle structure that represents the union of
    /// all given rectangles.
    ///
    /// @param[in] rectangles Collection of rectangles to united
    /// @return Return a rectangle that represents the union of all rectangles
    template <size_t SZ>
    Rect16 Union(std::array<Rect16, SZ> const &rectangles) {
        int16_t min_x = TopLeft().x, min_y = TopLeft().y;
        int16_t max_x = BottomRight().x, max_y = BottomRight().y;

        for (size_t i = 0; i < SZ; ++i) {
            if (rectangles[i].Width() > 0) {
                min_x = rectangles[i].TopLeft().x < min_x ? rectangles[i].TopLeft().x : min_x;
                min_y = rectangles[i].TopLeft().y < min_y ? rectangles[i].TopLeft().y : min_y;
                max_x = rectangles[i].BottomRight().x > max_x ? rectangles[i].BottomRight().x : max_x;
                max_y = rectangles[i].BottomRight().y > max_y ? rectangles[i].BottomRight().y : max_y;
            }
        }
        return { min_x, min_y, max_x, max_y };
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Split the current rectangle by given width and return such a
    /// collection of created rectangles
    /// @details The several situation should be consider
    /// 1) given width is zero => the width will be computed from the number of
    ///     records in the output collection
    /// 2) number of records * width < current rectangle width => algorithm leave
    ///     unused space of the parent rectangle
    /// 3) number of records * width > current rectangle width => algorithm fill
    ///     only records that fully fit into the parent rectangle
    /// @param[out] cuts Collection of contained sub-rectangles
    /// @param[in] width Width of the sub-rectangles
    /// @return Number of sub-rectangles
    template <size_t SZ>
    size_t HorizontalSplit(std::array<Rect16, SZ> &cuts, uint16_t width = 0) const {
        size_t i = 0;
        cuts.fill({ 0, 0, 0, 0 });
        width = (width == 0) ? Height() / SZ : width;
        while ((((i + 1) * width) < Width()) && (i < SZ)) {
            cuts[i] = {
                { static_cast<int16_t>(TopLeft().x + (i * width)), TopLeft().y },
                width,
                Height()
            };
            ++i;
        };
        return i;
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Split the current rectangle by given height and return such a
    /// collection of created rectangles
    /// @details The several situation should be consider
    /// 1) given height is zero => the height will be computed from the number of
    ///     records in the output collection
    /// 2) number of records * height < current rectangle height => algorithm leave
    ///     unused space of the parent rectangle
    /// 3) number of records * height > current rectangle height => algorithm fill
    ///     only records that fully fit into the parent rectangle
    /// @param[out] cuts Collection of contained sub-rectangles
    /// @param[in] height Height of the sub-rectangles
    /// @return Number of sub-rectangles
    template <size_t SZ>
    size_t VerticalSplit(std::array<Rect16, SZ> &cuts, uint16_t height = 0) const {
        size_t i = 0;
        cuts.fill({ 0, 0, 0, 0 });
        height = (height == 0) ? Width() / SZ : height;
        while ((((i + 1) * height) < Height()) && (i < SZ)) {
            cuts[i] = {
                { TopLeft().x, static_cast<int16_t>(TopLeft().y + (i * height)) },
                Width(),
                height
            };
            ++i;
        };
        return i;
    }
};

////////////////////////////////////////////////////////////////////////////
/// @brief Comparison operator of the rectangle class
/// @param[in] lhs Rectangle to compare
/// @param[in] rhs Rectangle to compare
/// @return Return true when the rectangle perfectly match, false otherwise
inline bool operator==(Rect16 const &lhs, Rect16 const &rhs) {
    return lhs.TopLeft().x == rhs.TopLeft().x
        && lhs.Width() == rhs.Width()
        && lhs.Height() == rhs.Height();
}
