/// @file
#pragma once

#include "WindowMenuInfo.hpp"
#include <basic_screen_menu.hpp>
#include <array>
#include <lang/string_view_utf8.hpp>
#include <otp/types.hpp>

/// Helper base class to fix initialization order fiasco.
class WindowMenuInfoOtpBase {
private:
    std::array<char, 32> buffer;

protected:
    explicit WindowMenuInfoOtpBase(const OTP_v2 &);

    string_view_utf8 get() const;
};

/// Menu item capable of displaying formatted OTP v2.
class WindowMenuInfoOtp : private WindowMenuInfoOtpBase, public IWiInfo {
protected:
    /// Construct invisible menu item, useful to avoid #ifdef hell.
    WindowMenuInfoOtp();

    /// Construct menu item from given label and OTP v2.
    /// Label is intentionally not translated, because it refers to the name
    /// of the electronic component.
    WindowMenuInfoOtp(const char *label, const OTP_v2 &);
};

class MI_OTP_MOTHERBOARD final : public WiInfo<28> {
public:
    MI_OTP_MOTHERBOARD();
};

class MI_OTP_LOVEBOARD final : public WindowMenuInfoOtp {
public:
    MI_OTP_LOVEBOARD();
};

class MI_OTP_XLCD final : public WindowMenuInfoOtp {
public:
    MI_OTP_XLCD();
};

class MI_OTP_XBUDDY_EXTENSION final : public WindowMenuInfoOtp {
public:
    MI_OTP_XBUDDY_EXTENSION();
};

class MI_OTP_INDX_HEAD final : public WindowMenuInfoOtp {
public:
    MI_OTP_INDX_HEAD();
};

class MI_OTP_XL_CAN final : public WindowMenuInfoOtp {
public:
    MI_OTP_XL_CAN();
};

using ScreenMenuBoardInfo__ = BasicScreenMenu<
    MI_OTP_MOTHERBOARD,
    MI_OTP_LOVEBOARD,
    MI_OTP_XLCD,
    MI_OTP_XBUDDY_EXTENSION,
    MI_OTP_INDX_HEAD,
    MI_OTP_XL_CAN>;

class ScreenMenuBoardInfo final : public ScreenMenuBoardInfo__ {
public:
    ScreenMenuBoardInfo();
};
