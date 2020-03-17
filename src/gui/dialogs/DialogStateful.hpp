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
/*
class DlgStatemachine {
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

    DlgStatemachine(const char *tit);

};*/

//#define DLG_FRAME_ENA 1
#define DLG_FRAME_ENA 0
//general foe any number of phases
class IDialogStateful : public IDialog {
protected:
    //dialog flags bitshift
    enum {
        DLG_SHI_MOD = 4, // mode shift
        DLG_SHI_CHG = 14 // change flag shift
    };
    enum : uint32_t {
#if DLG_FRAME_ENA == 1
        DLG_DRA_FR = 0x0800, // draw frame
#else
        DLG_DRA_FR = 0x0000, // draw frame
#endif                                          //DLG_FRAME_ENA == 1
        DLG_TXT_CH = 0x2000,                    // text changed
        DLG_PRO_CH = 0x4000,                    // progress changed
        DLG_PPR_CH = 0x8000,                    // part progress changed
        DLG_PRX_CH = (DLG_PRO_CH | DLG_PPR_CH), // some progress changed
        DLG_PHA_CH = (DLG_PRX_CH | DLG_TXT_CH), // phase changed
        //dialog flags bitmasks
        DLG_MSK_MOD = 0x0003,    // mode mask
        DLG_MSK_CHG = DLG_PHA_CH // change flag mask
    };

    /*
//button flags
//combination of enabled and not visible  == do not clear
#define BT_ENABLED ((uint8_t)(1 << 0))
//#define BT_VISIBLE  ((uint8_t)(1 << 1))
#define BT_AUTOEXIT ((uint8_t)(1 << 2))

#define DLG_CH_CMD ((uint8_t)(1 << 1)) //check marlin_command()

//flags for draw_cb function (user callback)
#define DLG_DI_US0 ((uint8_t)(1 << 4)) //user flag 0
#define DLG_DI_US1 ((uint8_t)(1 << 5)) //user flag 1
#define DLG_DI_US2 ((uint8_t)(1 << 6)) //user flag 2
#define DLG_DI_US3 ((uint8_t)(1 << 7)) //user flag 3   */
public:
    using State = std::tuple<const char *, RadioButton>;

protected:
    int16_t WINDOW_CLS;
    int16_t id_capture;

    color_t color_back;
    color_t color_text;
    font_t *font;
    font_t *font_title;
    padding_ui8_t padding;
    uint16_t flags;
    uint8_t last_text_h; //hack todo remove me

    const char *title;
    DlgVars dlg_vars;

    static window_t winCreate(int16_t WINDOW_CLS_) {
        window_t ret;
        window_create_ptr(WINDOW_CLS_, 0, gui_defaults.msg_box_sz, &ret);
        return ret;
    }

    virtual bool can_change(uint8_t phase) = 0;

public:
    IDialogStateful(const char *name, int16_t WINDOW_CLS_);
    bool Change(uint8_t phase, uint8_t progress_tot, uint8_t progress); // = 0; todo should be pure virtual
    virtual ~IDialogStateful();

    static constexpr rect_ui16_t get_radio_button_size() {
        rect_ui16_t rc_btn = { 0, 0, Disp().w, Disp().h };
        rc_btn.y += (rc_btn.h - 40); // 30pixels for button (+ 10 space for grey frame)
        rc_btn.h = 30;
        rc_btn.x += 6;
        rc_btn.w -= 12;
        return rc_btn;
    }

private:
    void _progress_draw(rect_ui16_t win_rect, font_t *font, color_t color_back,
        color_t color_text, padding_ui8_t padding, uint8_t progress);
    void _progress_clr(rect_ui16_t win_rect, font_t *font, color_t color_back);

protected:
    void draw_phase_text(const char *text);
    void draw_frame();
    void draw_progress();
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

protected:
    virtual bool can_change(uint8_t phase) { return phase < SZ; }

public:
    static void draw(DialogStateful<SZ> *window);
    static void event(DialogStateful<SZ> *window, uint8_t event, void *param);
};
#pragma pack(pop)

/*****************************************************************************/
//template definitions
/*
template <int SZ>
void DialogStateful<SZ>::_draw_phase_text(DialogStateful<SZ> *window) {
    rect_ui16_t rc_sta = window->rect;
    size_t nl;                                                   //number of new lines
    const char *s = window->states[window->dlg_vars.phase].text; // window->_ths->p_states[window->vars.phase].text;
    for (nl = 0; s[nl]; s[nl] == '\n' ? nl++ : *s++)
        ; //count '\n' in s
    rc_sta.h = 30 + window->font_title->h * nl;
    rc_sta.y += (30 + 46);
    rc_sta.x += 2;
    rc_sta.w -= 4;

    //erase remains of previous text if it was longer
    //prerelease hack todo text window just should be CENTER_TOP aligned and bigger
    int h_diff = window->last_text_h - rc_sta.h;
    if (h_diff > 0) {
        rect_ui16_t rc = rc_sta;
        rc.h = window->last_text_h - rc_sta.h;
        rc.y += rc_sta.h;
        display->fill_rect(rc, window->color_back);
    }

    window->last_text_h = rc_sta.h;

    render_text_align(rc_sta, window->_ths->p_states[window->vars.phase].text, window->font_title,
        window->color_back, window->color_text, window->padding, ALIGN_CENTER);
}
*/
template <int SZ>
void DialogStateful<SZ>::draw(DialogStateful<SZ> *window) {
    if ((window->f_visible)
        //&& ((size_t)(window->dlg_vars.phase) < window->states.size()) // no need to check
    ) {
        RadioButton &radio = std::get<RadioButton>(window->states[window->dlg_vars.phase]);
        const char *text = std::get<const char *>(window->states[window->dlg_vars.phase]);
        rect_ui16_t rc = window->rect;

        if (window->f_invalid) {
            display->fill_rect(rc, window->color_back);
            rect_ui16_t rc_tit = rc;
            rc_tit.h = 30; // 30pixels for title
            // TODO: - icon
            //			rc_tit.w -= 30;
            //			rc_tit.x += 30;
            //title
            render_text_align(rc_tit, window->title, window->font_title,
                window->color_back, window->color_text, window->padding, ALIGN_CENTER);

            window->f_invalid = 0;
            window->flags |= DLG_DRA_FR | DLG_PHA_CH | DLG_PPR_CH;
        }
        //DLG_PHA_CH == DLG_TXT_CH
        if (window->flags & DLG_TXT_CH) //text changed
        {
            window->draw_phase_text(text);
            window->flags &= ~DLG_TXT_CH;
        }
        //button knows when it needs to be repainted
        radio.Draw();

        if (window->flags & DLG_PRX_CH) //any progress changed
        {
            window->draw_progress();
            window->flags &= ~DLG_PRX_CH;
        }
        if (window->flags & DLG_DRA_FR) { //draw frame
            window->draw_frame();
            window->flags &= ~DLG_DRA_FR;
        }
    }
}

template <int SZ>
void DialogStateful<SZ>::event(DialogStateful<SZ> *window, uint8_t event, void *param) {
    RadioButton &radio = std::get<RadioButton>(window->states[window->dlg_vars.phase]);
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
    //case WINDOW_EVENT_BTN_UP:
    case WINDOW_EVENT_CLICK:
        radio.Click();
        return;
    case WINDOW_EVENT_ENC_UP:
        ++radio;
        return;
    case WINDOW_EVENT_ENC_DN:
        --radio;
        return;
    }
}
