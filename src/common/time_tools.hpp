#pragma once
/**
 * @file time_tools.hpp
 * @author Michal Rudolf
 * @brief Contains useful tools and enums for working with time
 * @date 2021-05-17
 */

namespace time_format {

enum class TF_t {
    TF_12H,
    TF_24H,
};

bool HasChanged();

void ClearChanged();

void Change(TF_t new_format);

TF_t Get();

}
