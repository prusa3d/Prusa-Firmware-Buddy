// client_response_texts.hpp
// texts for all types of response any Dialog can return

#pragma once

#include "client_response.hpp" //MAX_RESPONSES
#include "guitypes.hpp"
#include <array>
#include <utility>

using PhaseTexts = std::array<const char *, MAX_RESPONSES>;
using PhaseIcons = std::array<const png::Resource *, MAX_RESPONSES>;
using BtnResource = std::pair<const char *, const png::Resource *>;

//todo make some automatic checks names vs enum
//list of all button types
class BtnResponse {
    static const std::array<BtnResource, static_cast<size_t>(Response::_last) + 1> texts_and_icons;

public:
    static constexpr const char *GetText(Response resp) {
        return texts_and_icons[static_cast<size_t>(resp)].first;
    }
    static constexpr const png::Resource *GetIconId(Response resp) {
        return texts_and_icons[static_cast<size_t>(resp)].second;
    }
};

extern const PhaseTexts ph_txt_stop;
extern const PhaseTexts ph_txt_continue;
extern const PhaseTexts ph_txt_continue_stop;
extern const PhaseTexts ph_txt_none;
extern const PhaseTexts ph_txt_yesno;
