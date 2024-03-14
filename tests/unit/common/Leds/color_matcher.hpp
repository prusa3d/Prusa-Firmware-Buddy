#pragma once
#include <sstream>
#include "gui_leds.hpp"
#include "catch2/catch.hpp"
#include <array>

namespace leds {
std::string colorToStr(const Color &color) {

    std::ostringstream ss;
    ss << "(" << color.r << ", " << color.g << ", " << color.b << ")";
    return ss.str();
}
} // namespace leds

namespace Catch {
template <>
struct StringMaker<leds::Color> {
    static std::string convert(leds::Color const &value) {
        return leds::colorToStr(value);
    }
};

template <size_t COUNT>
struct StringMaker<LedsDummy<COUNT>> {
    static std::string convert(LedsDummy<COUNT> const &value) {
        return Catch::rangeToString(value.leds);
    }
};
} // namespace Catch

template <size_t COUNT>
struct LedsMatcher : Catch::MatcherBase<LedsDummy<COUNT>> {
    LedsMatcher() {
        colors.fill(leds::Color { 0 });
    }

    LedsMatcher(leds::Color color) {
        colors.fill(color);
    }
    LedsMatcher(std::array<leds::Color, COUNT> colors_)
        : colors(colors_) {}

    bool match(const LedsDummy<COUNT> &arg) const override {
        for (size_t i = 0; i < COUNT; i++) {
            if (arg.leds[i] != colors[i]) {
                return false;
            }
        }
        return true;
    }

protected:
    std::string describe() const override {
        std::ostringstream ss;
        ss << "equals target: " << Catch::rangeToString(colors);
        return ss.str();
    }

private:
    std::array<leds::Color, COUNT> colors;
};

struct ColorMatcher : Catch::MatcherBase<leds::Color> {
    ColorMatcher(leds::Color target)
        : m_target(target) {};

    bool match(const leds::Color &arg) const override {
        return m_target == arg;
    }

protected:
    std::string describe() const override {
        std::ostringstream ss;
        ss << "equals target: " << leds::colorToStr(m_target);
        return ss.str();
    }

private:
    leds::Color m_target;
};

using MK4LedsMatcher = LedsMatcher<3>;
