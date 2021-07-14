/**
 * @file changed.hpp
 * @author Radek Vana
 * @brief safer, more specific boolean type to be used as return value or function parameter
 *        to increase readability and code safety
 *        changed_t means something changed
 * @date 2021-05-24
 */

#pragma once

enum class changed_t : bool { no,
    yes };
