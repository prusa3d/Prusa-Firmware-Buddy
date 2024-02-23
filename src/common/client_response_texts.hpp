// client_response_texts.hpp
// texts for all types of response any Dialog can return

#pragma once

#include "client_response.hpp" //MAX_RESPONSES
#include <array>
#include <utility_extensions.hpp>

using PhaseTexts = std::array<const char *, MAX_RESPONSES>;

// todo make some automatic checks names vs enum
// list of all button types
class BtnResponse {
    static const std::array<const char *, ftrstd::to_underlying(Response::_count)> texts;

public:
    static constexpr const char *GetText(Response resp) {
        return texts[ftrstd::to_underlying(resp)];
    }
};

extern const PhaseTexts ph_txt_stop;
extern const PhaseTexts ph_txt_continue;
extern const PhaseTexts ph_txt_continue_stop;
extern const PhaseTexts ph_txt_none;
extern const PhaseTexts ph_txt_yesno;
