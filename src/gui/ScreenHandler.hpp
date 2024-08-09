// ScreenHandler.hpp

#pragma once
#include "screen.hpp"
#include "ScreenFactory.hpp"
#include <inplace_function.hpp>
#include <array>

// stack with screen creator methods
inline constexpr size_t MAX_SCREENS = 16;
struct screen_node {
    ScreenFactory::Creator creator;
    screen_init_variant init_data;

    screen_node(ScreenFactory::Creator creator = {}, screen_init_variant init_data = screen_init_variant())
        : creator(creator)
        , init_data(init_data) {}

    screen_node(ScreenFactory::Creator::Func creator, screen_init_variant init_data = screen_init_variant())
        : creator(creator)
        , init_data(init_data) {}

    void MakeEmpty() {
        creator = {};
        init_data = screen_init_variant();
    }
    bool IsEmpty() {
        return creator.func == nullptr;
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

    uint32_t timeout_tick;

    Screens(screen_node screen_creator);
    Screens(const Screens &) = delete;
    static Screens *instance;

public:
    void Loop(); // call inside guiloop

    void Open(screen_node screen_creator); // remember creator and create later with stored initialization parameter

    template <typename Screen, auto... args>
    void Open() {
        return Open(ScreenFactory::Screen<Screen, args...>);
    }

    bool IsOpenPending() const { return creator_node.creator.func != nullptr; }

    void PushBeforeCurrent(const ScreenFactory::Creator screen_creator);
    void PushBeforeCurrent(screen_node screen_creator);
    void PushBeforeCurrent(const screen_node *begin, const screen_node *end); // push in normal order, skips nullptr

    /// Closes currently open screen on the stack
    void Close();

    /// Closes the specific screen anywhere on the stack
    /// \returns if any screen was closed
    template <typename Screen>
    inline bool Close() {
        return Close(ScreenFactory::Screen<Screen>);
    };

    /// Closes the specific screen anywhere on the stack
    /// \returns if any screen was closed
    bool Close(const ScreenFactory::Creator &creator);

    void CloseAll();

    void ClosePrinting();

    size_t Count() { return stack_iterator - stack.begin(); } // count of closed screens under current one

    void Draw();
    void ResetTimeout();

    void ScreenEvent(window_t *sender, GUI_event_t event, void *param);
    void WindowEvent(GUI_event_t event, void *param);

    screen_t *Get() const;

    template <typename T>
    T *get() const {
        if (IsScreenOpened<T>()) {
            return static_cast<T *>(current.get());
        } else {
            return nullptr;
        }
    }

    void EnableMenuTimeout();
    void DisableMenuTimeout();
    bool GetMenuTimeout();

    void EnableFanCheck();
    void DisableFanCheck();
    bool GetFanChceck();

    static void Init(screen_node screen_creator);
    static void Init(const screen_node *begin, const screen_node *end); // init in normal order, skips nullptr

    static Screens *Access();

    /**
     * @brief check if screen is currently opened
     *
     * @tparam T screen
     * @return true  screen is opened
     * @return false screen is not opened
     */
    template <class T>
    bool IsScreenOpened() const {
        return stack_iterator->creator.is_screen<T>();
    }

    bool IsScreenOpened(ScreenFactory::Creator creator) const {
        return stack_iterator->creator == creator;
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
    bool IsScreenClosed() const {
        return std::any_of(stack.begin(), ScreenArray::const_iterator(stack_iterator + 1), [](const auto &node) { return node.creator.template is_screen<T>(); });
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
    bool IsScreenOnStack() const {
        return IsScreenOpened<T>() || IsScreenClosed<T>();
    }

    // This function is used to keep gui responsive when showing some dialog.
    // TODO: Perhaps it would be better to create the required dialog
    //       on the actual stack of screens.
    void gui_loop_until_dialog_closed(stdext::inplace_function<void()> callback = {});

private:
    void InnerLoop(); // call inside Loop of this class

    bool menu_timeout_enabled = true;
    using r_iter = std::reverse_iterator<const screen_node *>;
    static r_iter rfind_enabled_node(r_iter begin, r_iter end); // reverse find method
    using iter = const screen_node *;
    static iter find_enabled_node(iter begin, iter end); // normal find method
};
