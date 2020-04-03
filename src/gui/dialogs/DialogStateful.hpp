#pragma once

#include "IDialog.hpp"
#include <array>
#include "DialogRadioButton.hpp"
#include "marlin_client.hpp"
#include "client_response.hpp"

#pragma pack(push)
#pragma pack(1)

//#define DLG_FRAME_ENA 1
#define DLG_FRAME_ENA 0

//abstract parent containing general code for any number of phases
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

public:
    struct State {
        State(const char *lbl, RadioButton btn)
            : label(lbl)
            , button(btn) {}
        const char *label;
        RadioButton button;
    };

protected:
    int16_t id_capture;

    color_t color_back;
    color_t color_text;
    font_t *font;
    font_t *font_title;
    padding_ui8_t padding;
    uint16_t flags;
    uint8_t last_text_h; //hack todo remove me
    uint8_t phase;
    uint8_t progress;

    const char *title;

    virtual bool can_change(uint8_t phase) = 0;

public:
    IDialogStateful(const char *name, int16_t WINDOW_CLS_);
    bool Change(uint8_t phase, uint8_t progress_tot, uint8_t progress); // = 0; todo should be pure virtual
    virtual ~IDialogStateful();

    static constexpr rect_ui16_t get_radio_button_size() {
        rect_ui16_t rc_btn = { 0, 32, 240, 320 - 96 }; //msg box size
        rc_btn.y += (rc_btn.h - 40);                   // 30pixels for button (+ 10 space for grey frame)
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

/*****************************************************************************/
//parent for stateful dialogs dialog
//use one of enumclass from "client_response.hpp" as T
template <class T>
class DialogStateful : public IDialogStateful {
public:
    enum { SZ = CountPhases<T>() };
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
    void draw();
    void event(uint8_t event, void *param);
};
#pragma pack(pop)

/*****************************************************************************/
//template definitions

template <class T>
void DialogStateful<T>::draw() {
    if ((f_visible)
        //&& ((size_t)(phase) < states.size()) // no need to check
    ) {
        RadioButton &radio = states[phase].button;
        const char *text = states[phase].label;
        rect_ui16_t rc = rect;

        if (f_invalid) {
            display->fill_rect(rc, color_back);
            rect_ui16_t rc_tit = rc;
            rc_tit.h = 30; // 30pixels for title
            // TODO: - icon
            //			rc_tit.w -= 30;
            //			rc_tit.x += 30;
            //title
            render_text_align(rc_tit, title, font_title,
                color_back, color_text, padding, ALIGN_CENTER);

            f_invalid = 0;
            flags |= DLG_DRA_FR | DLG_PHA_CH | DLG_PPR_CH;
        }

        //button knows when it needs to be repainted except when phase changes
        if (flags & DLG_PHA_CH) {
            //do not clear DLG_PHA_CH
            radio.DrawForced();
        } else
            radio.Draw();

        if (flags & DLG_TXT_CH) //text changed
        {
            draw_phase_text(text);
            flags &= ~DLG_TXT_CH;
        }

        if (flags & DLG_PRX_CH) //any progress changed
        {
            draw_progress();
            flags &= ~DLG_PRX_CH;
        }
        if (flags & DLG_DRA_FR) { //draw frame
            draw_frame();
            flags &= ~DLG_DRA_FR;
        }
    }
}

template <class T>
void DialogStateful<T>::event(uint8_t event, void *param) {
    RadioButton &radio = states[phase].button;
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
    //case WINDOW_EVENT_BTN_UP:
    case WINDOW_EVENT_CLICK: {
        Response response = radio.Click();
        marlin_FSM_response(GetEnumFromPhaseIndex<T>(phase), response);
        return;
    }
    case WINDOW_EVENT_ENC_UP:
        ++radio;
        return;
    case WINDOW_EVENT_ENC_DN:
        --radio;
        return;
    }
}
