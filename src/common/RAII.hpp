/**
 * @file RAII.hpp
 * @author Radek Vana
 * @brief RAII pattern template
 * @date 2021-11-26
 */
#pragma once

/**
 * @brief bind to a variable or an object
 * store its state to be able to restore it at the end of scope
 *
 * @tparam T
 */
template <class T>
class AutoRestore {
    T &ref;
    T val;

public:
    /**
     * @brief Construct a new Auto Restore object
     *
     * @param ref reference to variable to be restored
     * @param set_to change value after construction
     */
    [[nodiscard]] AutoRestore(T &ref, T set_to, bool do_set = true)
        : ref(ref)
        , val(ref) {
        if (do_set) {
            ref = set_to;
        }
    }
    /**
     * @brief Construct a new Auto Restore object
     *
     * @param ref reference to variable to be restored
     */
    [[nodiscard]] AutoRestore(T &ref)
        : ref(ref)
        , val(ref) {}
    /**
     * @brief Destroy the Auto Restore object
     * and return it to original state
     */
    ~AutoRestore() { ref = val; }
};
