// G26.hpp

#pragma once

/// Path of Manhattan snake
/// Alternate X and Y coordinates.
/// It cannot print diagonals etc.
/// First X and Y are a starting point.
static const constexpr float snake1[] = {
    /// use 0.5 extrusion width
    10,  /// start X
    150, /// start Y
    170, /// X
    130, /// Y
    10,  /// X
    110, /// ...
    170,
    90,
    10,
    70,
    170,
    50,
    ///
    /// frame around
    9.5,
    17,
    30.5,
    30.5,
    10,
    30,
    ///
    /// infill
    30,
    29.5,
    10,
    29,
    30,
    28.5,
    10,
    28,
    30,
    27.5,
    10,
    27,
    30,
    26.5,
    10,
    26,
    30,
    25.5,
    10,
    25,
    30,
    24.5,
    10,
    24,
    30,
    23.5,
    10,
    23,
    30,
    22.5,
    10,
    22,
    30,
    21.5,
    10,
    21,
    30,
    20.5,
    10,
    20,
    30,
    19.5,
    10,
    19,
    30,
    18.5,
    10,
    18,
    30,
    17.5,
    10,
};

/// Path of Manhattan snake
/// Alternate X and Y coordinates.
/// It cannot print diagonals etc.
/// First X and Y are a starting point.
static const constexpr float snake2[] = {
    /// use 0.5 extrusion width
    10,        /// start X
    180 - 150, /// start Y
    170,       /// X
    180 - 130, /// Y
    10,        /// X
    180 - 110, /// ...
    170,
    180 - 90,
    10,
    180 - 70,
    170,
    180 - 50,
    ///
    /// frame around
    9.5,
    180 - 17,
    30.5,
    180 - 30.5,
    10,
    180 - 30,
    ///
    /// infill
    30,
    180 - 29.5,
    10,
    180 - 29,
    30,
    180 - 28.5,
    10,
    180 - 28,
    30,
    180 - 27.5,
    10,
    180 - 27,
    30,
    180 - 26.5,
    10,
    180 - 26,
    30,
    180 - 25.5,
    10,
    180 - 25,
    30,
    180 - 24.5,
    10,
    180 - 24,
    30,
    180 - 23.5,
    10,
    180 - 23,
    30,
    180 - 22.5,
    10,
    180 - 22,
    30,
    180 - 21.5,
    10,
    180 - 21,
    30,
    180 - 20.5,
    10,
    180 - 20,
    30,
    180 - 19.5,
    10,
    180 - 19,
    30,
    180 - 18.5,
    10,
    180 - 18,
    30,
    180 - 17.5,
    10,
};
