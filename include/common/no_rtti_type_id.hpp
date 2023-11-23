#pragma once

/**
 * @brief Provies unique number for every type
 * @note This is alternative to  std::type_info::hash_code, that can be used without RTTI
 *       Usage
 *           class SomeType {}
 *           uintptr_t some_type_hash_code = no_rtti_hash_code<SomeType>();
 */
template <typename T>
uintptr_t no_rtti_hash_code() noexcept {
    // declare arbitraty character in flash, that we can obtain address of to use as type identifier
    static char const some_value = 0;
    return reinterpret_cast<uintptr_t>(&some_value);
}
