//ScreenHandler.hpp

#pragma once
#include "screen.hpp"
#include "ScreenFactory.hpp"
#include <array>

//stack with screen creator methods
static constexpr size_t MAX_SCREENS = 32;
struct screen_node {
    ScreenFactory::Creator creator;
    screen_init_variant init_data;
    screen_node(ScreenFactory::Creator creator = nullptr, screen_init_variant init_data = screen_init_variant())
        : creator(creator)
        , init_data(init_data) {}
};
using ScreenArray = std::array<screen_node, MAX_SCREENS>;

class Screens {
    ScreenArray stack;
    ScreenArray::iterator stack_iterator;

    ScreenFactory::UniquePtr current;
    screen_node creator_node; // set by Open

    bool close;
    bool close_all;
    bool close_serial;

    uint32_t timeout_tick;

    //void stack_push(int16_t screen_id) {}
    //int16_t stack_pop(void) {}

    Screens(screen_node screen_creator);
    Screens(const Screens &) = delete;
    static Screens *instance;

public:
    void Loop(); //call inside guiloop

    void Open(const ScreenFactory::Creator screen_creator); //remember creator and create later with default initialization parameter (like selected item in menu)
    void Open(screen_node screen_creator);                  //remember creator and create later with stored initialization parameter
    bool IsOpenPending() const { return creator_node.creator != nullptr; }

    void PushBeforeCurrent(const ScreenFactory::Creator screen_creator);
    void PushBeforeCurrent(screen_node screen_creator);
    void PushBeforeCurrent(const screen_node *begin, const screen_node *end);  // push in normal order, skips nullptr
    void RPushBeforeCurrent(const screen_node *begin, const screen_node *end); // push in reversed order, skips nullptr

    void Close();

    void CloseAll();

    void CloseSerial();

    bool ConsumeClose(); //dialog can erase close signal and close itself

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

    static void Init(screen_node screen_creator);
    static void Init(const screen_node *begin, const screen_node *end);  // init in normal order, skips nullptr
    static void RInit(const screen_node *begin, const screen_node *end); // init in reversed order, skips nullptr

    static Screens *Access();

private:
    void InnerLoop(); //call inside Loop of this class

    bool menu_timeout_enabled = true;
    using r_iter = std::reverse_iterator<const screen_node *>;
    static r_iter rfind_enabled_node(r_iter begin, r_iter end); // reverse find method
    using iter = const screen_node *;
    static iter find_enabled_node(iter begin, iter end); // normal find method
};
