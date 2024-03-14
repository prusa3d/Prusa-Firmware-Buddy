/**
 * @file ScreenPrintingModel.hpp
 */
#pragma once

#include "IScreenPrinting.hpp"
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include <utility_extensions.hpp>

class ScreenPrintingModel : public AddSuperWindow<IScreenPrinting> {
protected:
    enum class BtnSocket {
        Left = 0,
        Middle,
        Right,
        _count,
        _last = _count - 1,
    };
    static constexpr size_t socket_count = ftrstd::to_underlying(BtnSocket::_count);

    enum class BtnRes {
        Settings = 0,
        Pause,
        Stop,
        Resume,
        Home,
        Reprint,
        Disconnect,
        _count,
        _last = _count - 1,
    };

    enum class LabelRes {
        Settings = 0,
        Pause,
        Pausing,
        Stop,
        Resume,
        Resuming,
        Reheating,
        Reprint,
        Home,
        Skip,
        Disconnect,
        _count,
        _last = _count - 1,
    };

    WindowMultiIconButton buttons[socket_count];
    window_text_t labels[socket_count];

    Rect16 GetButtonRect(uint8_t idx);
    Rect16 GetButtonLabelRect(uint8_t idx);
    void SetButtonIconAndLabel(BtnSocket btn, BtnRes ico_res, LabelRes txt_res);
    void SetButtonIcon(BtnSocket btn, BtnRes res);
    void SetButtonLabel(BtnSocket btn, LabelRes res);
    void DisableButton(BtnSocket btn);
    void EnableButton(BtnSocket btn);

public:
    ScreenPrintingModel(string_view_utf8 caption);
};
