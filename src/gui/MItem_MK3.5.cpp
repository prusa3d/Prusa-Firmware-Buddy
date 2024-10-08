#include "MItem_MK3.5.hpp"

MI_PINDA::MI_PINDA()
    : WI_FORMATABLE_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](const std::span<char> &buffer) {
        snprintf(buffer.data(), buffer.size(), "%i", value);
    }) {
}
