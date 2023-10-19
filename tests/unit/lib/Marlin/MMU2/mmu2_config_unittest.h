#pragma once
// A set of definitions which must be carefully avoided for the unit tests
// because including anything from Marlin is a way to hell.

#define FILAMENT_MMU2_RAMMING_SEQUENCE \
    {                                  \
        { 20, 1500 / 60.F },           \
            { -50, 2700 / 60.F },      \
            { -5, 50 / 60.F },         \
            { -50, 1500 / 60.F },      \
    }

#define ENABLED(x)              1
#define EXTRUDE_MINTEMP         170
#define NOZZLE_PARK_XY_FEEDRATE 100 // (mm/s) X and Y axes feedrate (also used for delta Z axis)
#define NOZZLE_PARK_Z_FEEDRATE  5 // (mm/s) Z axis feedrate (not used for delta printers)
#define pgm_read_float(addr)    ({ \
    decltype(addr) _addr = (addr); \
    *(const float *)(_addr);       \
})
