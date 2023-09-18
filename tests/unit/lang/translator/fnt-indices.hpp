/// @file
/// Wrapper around generated fnt-indices.ipp for checking nonASCII characters (if we have all of them in the generated font bitmaps)
#pragma once
#include <cstdint>
#include "string_view_utf8.hpp"

bool NonASCIICharKnown(unichar c);
