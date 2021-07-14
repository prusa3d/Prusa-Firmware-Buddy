/**
 * @file resized.hpp
 * @author Radek Vana
 * @brief safer, more specific boolean type to be used as return value or function parameter
 *        to increase readability and code safety
 *        resized_t means something was resized
 * @date 2021-05-24
 */

#pragma once

enum class resized_t : bool { no,
    yes };
