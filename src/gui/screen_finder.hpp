/**
 * @file screen_finder.hpp
 * could be part of ScreenHandler, but it would make id dependent on ScreenFactory
 */

#pragma once
#include "ScreenHandler.hpp"
#include "ScreenFactory.hpp"

class ScreenFinder {
    ScreenFinder() = delete;
    ScreenFinder(const ScreenFinder &) = delete;

public:
    /**
     * @brief check if screen is currently opened
     *
     * @tparam T screen
     * @return true  screen is opened
     * @return false screen is not opened
     */
    template <class T>
    static bool IsScreenOpened() {
        return Screens::Access() && Screens::Access()->GetStackIterator() && ScreenFactory::DoesCreatorHoldType<T>(Screens::Access()->GetStackIterator()->creator);
    }

    /**
     * @brief check if screen is closed
     * == it is on stack, but is not opened
     *
     * @tparam T screen
     * @return true  screen is closed
     * @return false screen is not closed
     */
    template <class T>
    static bool IsScreenClosed() {
        if (Screens::Access())
            return false;
        for (auto it = Screens::Access()->GetStack().begin(); it != Screens::Access()->GetStackIterator(); ++it) {
            if (it && ScreenFactory::DoesCreatorHoldType<T>(it->creator))
                return true;
        }
        return false;
    }

    /**
     * @brief check if screen is on stack
     * == ot is opened or closed
     *
     * @tparam T screen
     * @return true  screen is on stack
     * @return false screen is not on stack
     */
    template <class T>
    static bool IsScreenOnStack() {
        return IsScreenOpened<T>() || IsScreenClosed<T>();
    }
};
