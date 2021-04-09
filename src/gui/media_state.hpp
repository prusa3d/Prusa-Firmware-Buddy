/**
 * @file media_state.hpp
 * @author Radek Vana
 * @brief definition of media states
 * @date 2021-03-29
 */

#pragma once

enum class MediaState_t {
    unknown,
    inserted,
    removed,
    error
};
