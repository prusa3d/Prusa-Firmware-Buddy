#pragma once

#include "IDialog.hpp"
#include <array>
#include <tuple>
#include "DialogRadioButton.hpp"
#include "display.hpp"

#pragma pack(push)
#pragma pack(1)

//universal dialog vars
struct DlgVars {
    int8_t phase;
    int8_t prev_phase;
    uint8_t progress;
    uint8_t prev_progress;
    DlgVars();
};

class DlgStatemachine : public window_t {
public:
    color_t color_back;
    color_t color_text;
    font_t *font;
    font_t *font_title;
    padding_ui8_t padding;
    uint16_t flags;
    uint8_t last_text_h; //hack todo remove me

    const char *title;
    DlgVars dlg_vars;

    DlgStatemachine(window_t win, const char *tit);
    void DrawProgress() {}
};
/*
template<size_t SZ>
class DlgStatemachine:public _DlgStatemachine{
public:

    using States = std::array<State ,SZ>;
    States states;//phase text and radiobutton

    DlgStatemachine(window_t win, States st):window_t(win), _DlgStatemachine(st){}
};*/
#pragma pack(pop)

class IDialogStateful : public IDialog {
public:
    using State = std::tuple<const char *, RadioButton>;

protected:
    int16_t WINDOW_CLS;
    int16_t id_capture;
    DlgStatemachine data;
    //int16_t id;  DlgStatemachine::window_t.id

    static window_t winCreate(int16_t WINDOW_CLS_) {
        window_t ret;
        window_create_ptr(WINDOW_CLS_, 0, gui_defaults.msg_box_sz, &ret);
        return ret;
    }

public:
    IDialogStateful(const char *name, int16_t WINDOW_CLS_);
    virtual void Change(uint8_t phase, uint8_t progress_tot, uint8_t progress); // = 0; todo should be pure virtual
    virtual ~IDialogStateful();

    static constexpr rect_ui16_t get_radio_button_size() {
        rect_ui16_t rc_btn = { 0, 0, Disp().w, Disp().h };
        rc_btn.y += (rc_btn.h - 40); // 30pixels for button (+ 10 space for grey frame)
        rc_btn.h = 30;
        rc_btn.x += 6;
        rc_btn.w -= 12;
        return rc_btn;
    }
};

//parent for stateful dialogs dialog
template <int SZ>
class DialogStateful : public IDialogStateful {
public:
    using States = std::array<State, SZ>;

protected:
    States states; //phase text and radiobutton
public:
    DialogStateful(const char *name, int16_t WINDOW_CLS_, States st)
        : IDialogStateful(name, WINDOW_CLS_)
        , states(st) {};
};
