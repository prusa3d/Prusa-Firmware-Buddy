#ifndef FPM_MATH_HPP
#define FPM_MATH_HPP

#include "fixed.hpp"
#include <cmath>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace fpm
{

//
// Helper functions
//
namespace detail
{

// Returns the index of the most-signifcant set bit
constexpr unsigned int find_highest_bit(unsigned long long value) noexcept
{
    assert(value != 0);
#if defined(_MSC_VER)
    unsigned long index;
#if defined(_WIN64)
    _BitScanReverse64(&index, value);
#else
    if (_BitScanReverse(&index, static_cast<unsigned long>(value >> 32)) != 0) {
        index += 32;
    } else {
        _BitScanReverse(&index, static_cast<unsigned long>(value & 0xfffffffflu));
    }
#endif
    return static_cast<unsigned int>(index);
#elif defined(__GNUC__) || defined(__clang__)
    return sizeof(value) * 8 - 1 - __builtin_clzll(value);
#else
#   error "your platform does not support find_highest_bit()"
#endif
}

}

//
// Classification methods
//

template <typename B, typename I, unsigned int F, bool R>
constexpr inline int fpclassify(fixed<B, I, F, R> x) noexcept
{
    return (x.raw_value() == 0) ? FP_ZERO : FP_NORMAL;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline bool isfinite(fixed<B, I, F, R>) noexcept
{
    return true;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline bool isinf(fixed<B, I, F, R>) noexcept
{
    return false;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline bool isnan(fixed<B, I, F, R>) noexcept
{
    return false;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline bool isnormal(fixed<B, I, F, R> x) noexcept
{
    return x.raw_value() != 0;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline bool signbit(fixed<B, I, F, R> x) noexcept
{
    return x.raw_value() < 0;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline bool isgreater(fixed<B, I, F, R> x, fixed<B, I, F, R> y) noexcept
{
    return x > y;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline bool isgreaterequal(fixed<B, I, F, R> x, fixed<B, I, F, R> y) noexcept
{
    return x >= y;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline bool isless(fixed<B, I, F, R> x, fixed<B, I, F, R> y) noexcept
{
    return x < y;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline bool islessequal(fixed<B, I, F, R> x, fixed<B, I, F, R> y) noexcept
{
    return x <= y;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline bool islessgreater(fixed<B, I, F, R> x, fixed<B, I, F, R> y) noexcept
{
    return x != y;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline bool isunordered(fixed<B, I, F, R> x, fixed<B, I, F, R> y) noexcept
{
    return false;
}

//
// Nearest integer operations
//
template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> ceil(fixed<B, I, F, R> x) noexcept
{
    constexpr auto FRAC = B(1) << F;
    auto value = x.raw_value();
    if (value > 0) value += FRAC - 1;
    return fixed<B, I, F, R>::from_raw_value(value / FRAC * FRAC);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> floor(fixed<B, I, F, R> x) noexcept
{
    constexpr auto FRAC = B(1) << F;
    auto value = x.raw_value();
    if (value < 0) value -= FRAC - 1;
    return fixed<B, I, F, R>::from_raw_value(value / FRAC * FRAC);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> trunc(fixed<B, I, F, R> x) noexcept
{
    constexpr auto FRAC = B(1) << F;
    return fixed<B, I, F, R>::from_raw_value(x.raw_value() / FRAC * FRAC);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> round(fixed<B, I, F, R> x) noexcept
{
    constexpr auto FRAC = B(1) << F;
    auto value = x.raw_value() / (FRAC / 2);
    return fixed<B, I, F, R>::from_raw_value(((value / 2) + (value % 2)) * FRAC);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> nearbyint(fixed<B, I, F, R> x) noexcept
{
    // Rounding mode is assumed to be FE_TONEAREST
    constexpr auto FRAC = B(1) << F;
    auto value = x.raw_value();
    const bool is_half = std::abs(value % FRAC) == FRAC / 2;
    value /= FRAC / 2;
    value = (value / 2) + (value % 2);
    value -= (value % 2) * is_half;
    return fixed<B, I, F, R>::from_raw_value(value * FRAC);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline fixed<B, I, F, R> rint(fixed<B, I, F, R> x) noexcept
{
    // Rounding mode is assumed to be FE_TONEAREST
    return nearbyint(x);
}

//
// Mathematical functions
//
template <typename B, typename I, unsigned int F, bool R>
constexpr inline fixed<B, I, F, R> abs(fixed<B, I, F, R> x) noexcept
{
    return (x >= fixed<B, I, F, R>{0}) ? x : -x;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline fixed<B, I, F, R> fmod(fixed<B, I, F, R> x, fixed<B, I, F, R> y) noexcept
{
    return
        assert(y.raw_value() != 0),
        fixed<B, I, F, R>::from_raw_value(x.raw_value() % y.raw_value());
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline fixed<B, I, F, R> remainder(fixed<B, I, F, R> x, fixed<B, I, F, R> y) noexcept
{
    return
        assert(y.raw_value() != 0),
        x - nearbyint(x / y) * y;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> remquo(fixed<B, I, F, R> x, fixed<B, I, F, R> y, int* quo) noexcept
{
    assert(y.raw_value() != 0);
    assert(quo != nullptr);
    *quo = x.raw_value() / y.raw_value();
    return fixed<B, I, F, R>::from_raw_value(x.raw_value() % y.raw_value());
}

//
// Manipulation functions
//

template <typename B, typename I, unsigned int F, bool R, typename C, typename J, unsigned int G, bool S>
constexpr inline fixed<B, I, F, R> copysign(fixed<B, I, F, R> x, fixed<C, J, G, S> y) noexcept
{
    return
        x = abs(x),
        (y >= fixed<C, J, G, S>{0}) ? x : -x;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline fixed<B, I, F, R> nextafter(fixed<B, I, F, R> from, fixed<B, I, F, R> to) noexcept
{
    return from == to ? to :
           to > from ? fixed<B, I, F, R>::from_raw_value(from.raw_value() + 1)
                     : fixed<B, I, F, R>::from_raw_value(from.raw_value() - 1);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr inline fixed<B, I, F, R> nexttoward(fixed<B, I, F, R> from, fixed<B, I, F, R> to) noexcept
{
    return nextafter(from, to);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> modf(fixed<B, I, F, R> x, fixed<B, I, F, R>* iptr) noexcept
{
    const auto raw = x.raw_value();
    constexpr auto FRAC = B{1} << F;
    *iptr = fixed<B, I, F, R>::from_raw_value(raw / FRAC * FRAC);
    return fixed<B, I, F, R>::from_raw_value(raw % FRAC);
}


//
// Power functions
//

template <typename B, typename I, unsigned int F, bool R, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
constexpr fixed<B, I, F, R> pow(fixed<B, I, F, R> base, T exp) noexcept
{
    using Fixed = fixed<B, I, F, R>;

    if (base == Fixed(0)) {
        assert(exp > 0);
        return Fixed(0);
    }

    Fixed result {1};
    if (exp < 0)
    {
        for (Fixed intermediate = base; exp != 0; exp /= 2, intermediate *= intermediate)
        {
            if ((exp % 2) != 0)
            {
                result /= intermediate;
            }
        }
    }
    else
    {
        for (Fixed intermediate = base; exp != 0; exp /= 2, intermediate *= intermediate)
        {
            if ((exp % 2) != 0)
            {
                result *= intermediate;
            }
        }
    }
    return result;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> pow(fixed<B, I, F, R> base, fixed<B, I, F, R> exp) noexcept
{
    using Fixed = fixed<B, I, F, R>;

    if (base == Fixed(0)) {
        assert(exp > Fixed(0));
        return Fixed(0);
    }

    if (exp < Fixed(0))
    {
        return 1 / pow(base, -exp);
    }

    constexpr auto FRAC = B(1) << F;
    if (exp.raw_value() % FRAC == 0)
    {
        // Non-fractional exponents are easier to calculate
        return pow(base, exp.raw_value() / FRAC);
    }

    // For negative bases we do not support fractional exponents.
    // Technically fractions with odd denominators could work,
    // but that's too much work to figure out.
    assert(base > Fixed(0));
    return exp2(log2(base) * exp);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> exp(fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;
    if (x < Fixed(0)) {
        return 1 / exp(-x);
    }
    constexpr auto FRAC = B(1) << F;
    const B x_int = x.raw_value() / FRAC;
    x -= x_int;
    assert(x >= Fixed(0) && x < Fixed(1));

    constexpr auto fA = Fixed::template from_fixed_point<63>( 128239257017632854ll); // 1.3903728105644451e-2
    constexpr auto fB = Fixed::template from_fixed_point<63>( 320978614890280666ll); // 3.4800571158543038e-2
    constexpr auto fC = Fixed::template from_fixed_point<63>(1571680799599592947ll); // 1.7040197373796334e-1
    constexpr auto fD = Fixed::template from_fixed_point<63>(4603349000587966862ll); // 4.9909609871464493e-1
    constexpr auto fE = Fixed::template from_fixed_point<62>(4612052447974689712ll); // 1.0000794567422495
    constexpr auto fF = Fixed::template from_fixed_point<63>(9223361618412247875ll); // 9.9999887043019773e-1
    return pow(Fixed::e(), x_int) * (((((fA * x + fB) * x + fC) * x + fD) * x + fE) * x + fF);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> exp2(fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;
    if (x < Fixed(0)) {
        return 1 / exp2(-x);
    }
    constexpr auto FRAC = B(1) << F;
    const B x_int = x.raw_value() / FRAC;
    x -= x_int;
    assert(x >= Fixed(0) && x < Fixed(1));

    constexpr auto fA = Fixed::template from_fixed_point<63>(  17491766697771214ll); // 1.8964611454333148e-3
    constexpr auto fB = Fixed::template from_fixed_point<63>(  82483038782406547ll); // 8.9428289841091295e-3
    constexpr auto fC = Fixed::template from_fixed_point<63>( 515275173969157690ll); // 5.5866246304520701e-2
    constexpr auto fD = Fixed::template from_fixed_point<63>(2214897896212987987ll); // 2.4013971109076949e-1
    constexpr auto fE = Fixed::template from_fixed_point<63>(6393224161192452326ll); // 6.9315475247516736e-1
    constexpr auto fF = Fixed::template from_fixed_point<63>(9223371050976163566ll); // 9.9999989311082668e-1
    return Fixed(1 << x_int) * (((((fA * x + fB) * x + fC) * x + fD) * x + fE) * x + fF);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> expm1(fixed<B, I, F, R> x) noexcept
{
    return exp(x) - 1;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> log2(fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;
    assert(x > Fixed(0));

    // Normalize input to the [1:2] domain
    B value = x.raw_value();
    const unsigned int highest = detail::find_highest_bit(value);
    if (highest >= F) {
        value >>= (highest - F);
    } else {
        value <<= (F - highest);
    }
    x = Fixed::from_raw_value(value);
    assert(x >= Fixed(1) && x < Fixed(2));

    constexpr auto fA = Fixed::template from_fixed_point<63>(  413886001457275979ll); //  4.4873610194131727e-2
    constexpr auto fB = Fixed::template from_fixed_point<63>(-3842121857793256941ll); // -4.1656368651734915e-1
    constexpr auto fC = Fixed::template from_fixed_point<62>( 7522345947206307744ll); //  1.6311487636297217
    constexpr auto fD = Fixed::template from_fixed_point<61>(-8187571043052183818ll); // -3.5507929249026341
    constexpr auto fE = Fixed::template from_fixed_point<60>( 5870342889289496598ll); //  5.0917108110420042
    constexpr auto fF = Fixed::template from_fixed_point<61>(-6457199832668582866ll); // -2.8003640347009253
    return Fixed(highest - F) + (((((fA * x + fB) * x + fC) * x + fD) * x + fE) * x + fF);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> log(fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;
    return log2(x) / log2(Fixed::e());
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> log10(fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;
    return log2(x) / log2(Fixed(10));
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> log1p(fixed<B, I, F, R> x) noexcept
{
    return log(1 + x);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> cbrt(fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;

    if (x == Fixed(0))
    {
        return x;
    }
    if (x < Fixed(0))
    {
        return -cbrt(-x);
    }
    assert(x >= Fixed(0));

    // Finding the cube root of an integer, taken from Hacker's Delight,
    // based on the square root algorithm.

    // We start at the greatest power of eight that's less than the argument.
    int ofs = ((detail::find_highest_bit(x.raw_value()) + 2*F) / 3 * 3);
    I num = I{x.raw_value()};
    I res = 0;

    const auto do_round = [&]
    {
        for (; ofs >= 0; ofs -= 3)
        {
            res += res;
            const I val = (3*res*(res + 1) + 1) << ofs;
            if (num >= val)
            {
                num -= val;
                res++;
            }
        }
    };

    // We should shift by 2*F (since there are two multiplications), but that
    // could overflow even the intermediate type, so we have to split the
    // algorithm up in two rounds of F bits each. Each round will deplete
    // 'num' digit by digit, so after a round we can shift it again.
    num <<= F;
    ofs -= F;
    do_round();

    num <<= F;
    ofs += F;
    do_round();

    return Fixed::from_raw_value(static_cast<B>(res));
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> sqrt(fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;

    assert(x >= Fixed(0));
    if (x == Fixed(0))
    {
        return x;
    }

    // Finding the square root of an integer in base-2, from:
    // https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Binary_numeral_system_.28base_2.29

    // Shift by F first because it's fixed-point.
    I num = I{x.raw_value()} << F;
    I res = 0;

    // "bit" starts at the greatest power of four that's less than the argument.
    for (I bit = I{1} << ((detail::find_highest_bit(x.raw_value()) + F) / 2 * 2); bit != 0; bit >>= 2)
    {
        const I val = res + bit;
        res >>= 1;
        if (num >= val)
        {
            num -= val;
            res += bit;
        }
    }

    // Round the last digit up if necessary
    if (num > res)
    {
        res++;
    }

    return Fixed::from_raw_value(static_cast<B>(res));
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> hypot(fixed<B, I, F, R> x, fixed<B, I, F, R> y) noexcept
{
    assert(x != 0 || y != 0);
    return sqrt(x*x + y*y);
}

//
// Trigonometry functions
//

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> sin(fixed<B, I, F, R> x) noexcept
{
    // This sine uses a fifth-order curve-fitting approximation originally
    // described by Jasper Vijn on coranac.com which has a worst-case
    // relative error of 0.07% (over [-pi:pi]).
    using Fixed = fixed<B, I, F, R>;

    // Turn x from [0..2*PI] domain into [0..4] domain
    x = fmod(x, Fixed::two_pi());
    x = x / Fixed::half_pi();

    // Take x modulo one rotation, so [-4..+4].
    if (x < Fixed(0)) {
        x += Fixed(4);
    }

    int sign = +1;
    if (x > Fixed(2)) {
        // Reduce domain to [0..2].
        sign = -1;
        x -= Fixed(2);
    }

    if (x > Fixed(1)) {
        // Reduce domain to [0..1].
        x = Fixed(2) - x;
    }

    const Fixed x2 = x*x;
    return sign * x * (Fixed::pi() - x2*(Fixed::two_pi() - 5 - x2*(Fixed::pi() - 3)))/2;
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> cos(fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;
    if (x > Fixed(0)) {  // Prevent an overflow due to the addition of π/2
        return sin(x - (Fixed::two_pi() - Fixed::half_pi()));
    } else {
        return sin(Fixed::half_pi() + x);
    }    
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> tan(fixed<B, I, F, R> x) noexcept
{
    auto cx = cos(x);

    // Tangent goes to infinity at 90 and -90 degrees.
    // We can't represent that with fixed-point maths.
    assert(abs(cx).raw_value() > 1);

    return sin(x) / cx;
}

namespace detail {

// Calculates atan(x) assuming that x is in the range [0,1]
template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> atan_sanitized(fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;
    assert(x >= Fixed(0) && x <= Fixed(1));

    constexpr auto fA = Fixed::template from_fixed_point<63>(  716203666280654660ll); //  0.0776509570923569
    constexpr auto fB = Fixed::template from_fixed_point<63>(-2651115102768076601ll); // -0.287434475393028
    constexpr auto fC = Fixed::template from_fixed_point<63>( 9178930894564541004ll); //  0.995181681698119  (PI/4 - A - B)

    const auto xx = x * x;
    return ((fA*xx + fB)*xx + fC)*x;
}

// Calculate atan(y / x), assuming x != 0.
//
// If x is very, very small, y/x can easily overflow the fixed-point range.
// If q = y/x and q > 1, atan(q) would calculate atan(1/q) as intermediate step
// anyway. We can shortcut that here and avoid the loss of information, thus
// improving the accuracy of atan(y/x) for very small x.
template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> atan_div(fixed<B, I, F, R> y, fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;
    assert(x != Fixed(0));

    // Make sure y and x are positive.
    // If y / x is negative (when y or x, but not both, are negative), negate the result to
    // keep the correct outcome.
    if (y < Fixed(0)) {
        if (x < Fixed(0)) {
            return atan_div(-y, -x);
        }
        return -atan_div(-y, x);
    }
    if (x < Fixed(0)) {
        return -atan_div(y, -x);
    }
    assert(y >= Fixed(0));
    assert(x >  Fixed(0));

    if (y > x) {
        return Fixed::half_pi() - detail::atan_sanitized(x / y);
    }
    return detail::atan_sanitized(y / x);
}

}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> atan(fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;
    if (x < Fixed(0))
    {
        return -atan(-x);
    }

    if (x > Fixed(1))
    {
        return Fixed::half_pi() - detail::atan_sanitized(Fixed(1) / x);
    }

    return detail::atan_sanitized(x);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> asin(fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;
    assert(x >= Fixed(-1) && x <= Fixed(+1));

    const auto yy = Fixed(1) - x * x;
    if (yy == Fixed(0))
    {
        return copysign(Fixed::half_pi(), x);
    }
    return detail::atan_div(x, sqrt(yy));
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> acos(fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;
    assert(x >= Fixed(-1) && x <= Fixed(+1));

    if (x == Fixed(-1))
    {
        return Fixed::pi();
    }
    const auto yy = Fixed(1) - x * x;
    return Fixed(2)*detail::atan_div(sqrt(yy), Fixed(1) + x);
}

template <typename B, typename I, unsigned int F, bool R>
constexpr fixed<B, I, F, R> atan2(fixed<B, I, F, R> y, fixed<B, I, F, R> x) noexcept
{
    using Fixed = fixed<B, I, F, R>;
    if (x == Fixed(0))
    {
        assert(y != Fixed(0));
        return (y > Fixed(0)) ? Fixed::half_pi() : -Fixed::half_pi();
    }

    auto ret = detail::atan_div(y, x);

    if (x < Fixed(0))
    {
        return (y >= Fixed(0)) ? ret + Fixed::pi() : ret - Fixed::pi();
    }
    return ret;
}

}

#endif
