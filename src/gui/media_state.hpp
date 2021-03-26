/**
 * @file media_state.hpp
 * @author Radek Vana
 * @brief definition of media states
 * @date 2021-03-29
 */

#pragma once

enum class media_state_t {
    unknown,
    inserted,
    removed,
    error
};
