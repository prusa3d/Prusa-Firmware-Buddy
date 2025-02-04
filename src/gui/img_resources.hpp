#pragma once
#include "guitypes.hpp" //img::Resource

namespace img {

#include "qoi_resources.gen"

#ifndef UNITTESTS
static constexpr std::array spinner_16x16_stages { &img::spinner0_16x16, &img::spinner1_16x16, &img::spinner2_16x16, &img::spinner3_16x16 };
#endif

/**
 * @brief Enable global resource file to be used.
 */
void enable_resource_file();

/**
 * @brief Get global resource file.
 * Can be used only after enable_resource_file() was called.
 * @return FILE* to resource file.
 */
FILE *get_resource_file();

} // namespace img
