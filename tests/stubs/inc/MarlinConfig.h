#pragma once

#define HOTENDS 1

#define _CAT(a, ...)    a##__VA_ARGS__
#define SWITCH_ENABLED_ 1
#define ENABLED(b)      _CAT(SWITCH_ENABLED_, b)

#define SINGLENOZZLE
