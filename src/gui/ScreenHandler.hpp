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

    bool is_dialog_open = false;

    bool close_dialog_requested = false;
    bool close = false;
    bool close_all = false;
    bool close_printing = false;

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

    /// @returns whether the currently open screen has a pending close
    bool is_close_pending() const { return close; }

    void PushBeforeCurrent(screen_node screen_creator);

    /// Closes the currently open screen on the stack.
    /// The screen is not destroyed immediately - it happens on the next \p Loop.
    /// Consequently, if a blocking dialog is open on top of the screen, the close only takes
    /// effect after that dialog's loop finishes - use \p close_dialog to close the dialog itself.
    void close_screen();

    /// Closes the specific screen anywhere on the stack
    /// \returns if any screen was closed
    template <typename Screen>
    inline bool close_screen() {
        return close_screen(ScreenFactory::Screen<Screen>);
    };

    /// Closes the specific screen anywhere on the stack
    /// \returns if any screen was closed
    bool close_screen(const ScreenFactory::Creator &creator);

    /// Closes the blocking dialog we're currently waiting on in \p gui_loop_until_dialog_closed.
    /// Must not be called when no such dialog is open.
    void close_dialog();

    /// Closes the blocking dialog if we're waiting on one, the current screen otherwise.
    /// Only for generic widgets that can be placed both directly on a screen and inside a dialog
    /// (MI_RETURN, close-on-click windows, swipe-back menus) - everywhere else, the context is
    /// known and the explicit \p close_screen / \p close_dialog should be used instead.
    void close_screen_or_dialog();

    /// Closes the current screen, together with any dialogs on top of it (if they allow it)
    void close_screen_and_dialog();

    void CloseAll();

    void ClosePrinting();

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

    static void Init(screen_node screen_creator);

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

    /// @returns whether any screen on stack (including the currently open one) matches the specified predicate
    template <typename F>
    bool any_screen_on_stack(F &&f) const {
        // Note: stack_iterator point to the currently opened screen
        return f(creator_node) || std::any_of(std::make_const_iterator(stack.begin()), ScreenArray::const_iterator(stack_iterator + 1), f);
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
        return any_screen_on_stack([](const auto &node) { return node.creator.template is_screen<T>(); });
    }

    // This function is used to keep gui responsive when showing some dialog.
    // TODO: Perhaps it would be better to create the required dialog
    //       on the actual stack of screens.
    void gui_loop_until_dialog_closed(stdext::inplace_function<void()> callback = {});

private:
    void InnerLoop(); // call inside Loop of this class

    bool menu_timeout_enabled = true;
};
