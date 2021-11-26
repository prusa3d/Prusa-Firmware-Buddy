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
    AutoRestore(T &ref, T set_to)
        : ref(ref)
        , val(ref) {
        ref = set_to;
    }
    /**
     * @brief Construct a new Auto Restore object
     *
     * @param ref reference to variable to be restored
     */
    AutoRestore(T &ref)
        : ref(ref)
        , val(ref) {}
    /**
     * @brief Destroy the Auto Restore object
     * and return it to original state
     */
    ~AutoRestore() { ref = val; }
};
