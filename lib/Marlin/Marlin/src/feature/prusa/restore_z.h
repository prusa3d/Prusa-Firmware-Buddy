/**
 * @file
 */
#pragma once
#include "restore_z_storage.h"
#include <config_store/store_instance.hpp>

/**
 * @brief Implements restore Z coordinate after boot
 */
namespace restore_z {

void restore();
void store();

/**
 * @brief Clear stored Z coordinate
 */
inline void clear() {
    config_store().restore_z_after_boot.set(default_position);
}

} // namespace restore_z
