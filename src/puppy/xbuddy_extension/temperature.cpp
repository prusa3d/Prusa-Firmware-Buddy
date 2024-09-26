/// @file
#include "temperature.hpp"

// FIXME: This experimental temperature table is good enough for demo,
//        but should eventually be replaced by table based on theoretic values.
//        I suspect the ADC reading isn't configured properly.
static constexpr int16_t temptable[][2] = {
    { 3622, 60 },
    { 3750, 50 },
    { 3865, 40 },
    { 3962, 25 },
};

// FIXME: This is a copy-paste from Marlin and should be replaced by something sane.
//        It basically performs binary search in the temperature table followed by
//        linear interpolation.
float temperature::raw_to_celsius(uint16_t raw) {
    uint8_t l = 0, r = (sizeof(temptable) / sizeof(*temptable)), m;
    for (;;) {
        m = (l + r) >> 1;
        if (!m) {
            return short(({ decltype ( & temptable [ 0 ] [ 1 ] ) _addr = ( & temptable [ 0 ] [ 1 ] ) ; * ( const unsigned short * ) ( _addr ) ; }));
        }
        if (m == l || m == r) {
            return short(({ decltype ( & temptable [ ( sizeof ( temptable ) / sizeof ( * temptable ) ) - 1 ] [ 1 ] ) _addr = ( & temptable [ ( sizeof ( temptable ) / sizeof ( * temptable ) ) - 1 ] [ 1 ] ) ; * ( const unsigned short * ) ( _addr ) ; }));
        }
        short v00 = ({ decltype ( & temptable [ m - 1 ] [ 0 ] ) _addr = ( & temptable [ m - 1 ] [ 0 ] ) ; * ( const unsigned short * ) ( _addr ) ; }), v10 = ({ decltype ( & temptable [ m - 0 ] [ 0 ] ) _addr = ( & temptable [ m - 0 ] [ 0 ] ) ; * ( const unsigned short * ) ( _addr ) ; });
        if (raw < v00) {
            r = m;
        } else if (raw > v10) {
            l = m;
        } else {
            const short v01 = short(({ decltype ( & temptable [ m - 1 ] [ 1 ] ) _addr = ( & temptable [ m - 1 ] [ 1 ] ) ; * ( const unsigned short * ) ( _addr ) ; })), v11 = short(({ decltype ( & temptable [ m - 0 ] [ 1 ] ) _addr = ( & temptable [ m - 0 ] [ 1 ] ) ; * ( const unsigned short * ) ( _addr ) ; }));
            return v01 + (raw - v00) * float(v11 - v01) / float(v10 - v00);
        }
    }
}
