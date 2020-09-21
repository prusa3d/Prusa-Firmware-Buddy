// G26.hpp
namespace PrusaGcodeSuite {
void G26();
}

/// Path of Manhattan snake
/// Alternate X and Y coordinates.
/// It cannot print diagonals etc.
/// First X and Y are a starting point.
static const constexpr float snake1[]
    = {
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
          9.5,
          ///
          /// frame around
          17,
          31.5,
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
      };
