/**
 * @file
 * @date Jun 23, 2020
 * @author Marek Bel
 */

#pragma once

/**
 * @brief Concept of configure_all implementation
 */
class Pin {
public:
    Pin();
    void configure() {}
    static void configure_all();

private:
    Pin *const previous;
    static Pin *last;
};
