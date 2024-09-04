#include "radio_button.hpp"

std::optional<size_t> RadioButton::IndexFromResponse(Response btn) const {
    for (size_t i = 0; i < maxSize(); ++i) {
        if (btn == (responses)[i]) {
            return i;
        }
    }
    return std::nullopt;
}

Response RadioButton::responseFromIndex(size_t index) const {
    if (index >= maxSize()) {
        return Response::_none;
    }
    return (responses)[index];
}

void RadioButton::Change(Responses_t resp, const PhaseTexts *txts) {
    if ((responses == resp) && (txts == texts)) {
        return;
    }
    responses = resp;
    texts = txts;
    SetBtnCount(fixed_width_buttons_count > 0 ? fixed_width_buttons_count : cnt_buttons(texts, responses));

    // in iconned layout index will stay
    if (fixed_width_buttons_count == 0) {
        SetBtnIndex(0);
    }

    validateBtnIndex();

    invalidateWhatIsNeeded();
}

// TODO: REMOVEME completely BFW-6028
#if MAX_RESPONSES != 4
void RadioButton::Change(const PhaseResponses &resp, const PhaseTexts *txts) {
    Change(generateResponses(resp), txts);
}
#endif

RadioButton::RadioButton(window_t *parent, Rect16 rect)
    : RadioButton(parent, rect, Responses_t({ { Response::_none, Response::_none, Response::_none, Response::_none } })) {
}

// TODO: REMOVEME completely BFW-6028
#if MAX_RESPONSES != 4
RadioButton::RadioButton(window_t *parent, Rect16 rect, const PhaseResponses &resp, const PhaseTexts *labels)
    : RadioButton(parent, rect, generateResponses(resp), labels) {
}
#endif

RadioButton::RadioButton(window_t *parent, Rect16 rect, Responses_t resp, const PhaseTexts *labels)
    : IRadioButton(parent, rect, cnt_buttons(labels, resp))
    , responses(resp)
    , texts(labels) {
}
