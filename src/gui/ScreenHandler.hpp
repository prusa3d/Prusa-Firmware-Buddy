// ScreenHandler.hpp

#pragma once
#include "screen.hpp"
#include "ScreenFactory.hpp"
#include <array>

// stack with screen creator methods
inline constexpr size_t MAX_SCREENS = 32;
struct screen_node {
    ScreenFactory::Creator creator;
    screen_init_variant init_data;
    screen_node(ScreenFactory::Creator creator = nullptr, screen_init_variant init_data = screen_init_variant())
        : creator(creator)
        , init_data(init_data) {}
    void MakeEmpty() {
        creator = nullptr;
        init_data = screen_init_variant();
    }
    bool IsEmpty() {
        return creator == nullptr;
    }
};
using ScreenArray = std::array<screen_node, MAX_SCREENS>;

class Screens {
    ScreenArray stack;
    ScreenArray::iterator stack_iterator; // points at creator of currently opened screen, init data not valid, because they are set when closed

    ScreenFactory::UniquePtr current; // pointer obtained by screen creation
    screen_node creator_node; // set by Open

    bool close;
    bool close_all;
    bool close_printing;
    bool display_reinitialized;

    uint32_t timeout_tick;

    Screens(screen_node screen_creator);
    Screens(const Screens &) = delete;
    static Screens *instance;

public:
    void Loop(); // call inside guiloop

    void Open(const ScreenFactory::Creator screen_creator); // remember creator and create later with default initialization parameter (like selected item in menu)
    void Open(screen_node screen_creator); // remember creator and create later with stored initialization parameter
    bool IsOpenPending() const { return creator_node.creator != nullptr; }

    void PushBeforeCurrent(const ScreenFactory::Creator screen_creator);
    void PushBeforeCurrent(screen_node screen_creator);
    void PushBeforeCurrent(const screen_node *begin, const screen_node *end); // push in normal order, skips nullptr
    void RPushBeforeCurrent(const screen_node *begin, const screen_node *end); // push in reversed order, skips nullptr

    void Close();

    void CloseAll();

    void ClosePrinting();

    bool ConsumeClose(); // dialog can erase close signal and close itself

    size_t Count() { return stack_iterator - stack.begin(); } // count of closed screens under current one

    void Draw();
    void ResetTimeout();

    void ScreenEvent(window_t *sender, GUI_event_t event, void *param);
    void WindowEvent(GUI_event_t event, void *param);

    screen_t *Get() const;

    void EnableMenuTimeout();
    void DisableMenuTimeout();
    bool GetMenuTimeout();

    void EnableFanCheck();
    void DisableFanCheck();
    bool GetFanChceck();
    void SetDisplayReinitialized();

    static void Init(screen_node screen_creator);
    static void Init(const screen_node *begin, const screen_node *end); // init in normal order, skips nullptr
    static void RInit(const screen_node *begin, const screen_node *end); // init in reversed order, skips nullptr

    static Screens *Access();

    /**
     * @brief check if screen is currently opened
     *
     * @tparam T screen
     * @return true  screen is opened
     * @return false screen is not opened
     */
    template <class T>
    bool IsScreenOpened() {
        return stack_iterator && ScreenFactory::DoesCreatorHoldType<T>(stack_iterator->creator);
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
    bool IsScreenClosed() {
        for (auto it = stack.begin(); it != stack_iterator; ++it) {
            if (it && ScreenFactory::DoesCreatorHoldType<T>(it->creator)) {
                return true;
            }
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
    bool IsScreenOnStack() {
        return IsScreenOpened<T>() || IsScreenClosed<T>();
    }

private:
    void InnerLoop(); // call inside Loop of this class

    bool menu_timeout_enabled = true;
    using r_iter = std::reverse_iterator<const screen_node *>;
    static r_iter rfind_enabled_node(r_iter begin, r_iter end); // reverse find method
    using iter = const screen_node *;
    static iter find_enabled_node(iter begin, iter end); // normal find method
};
