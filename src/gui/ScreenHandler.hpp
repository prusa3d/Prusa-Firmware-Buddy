//ScreenHandler.hpp

#pragma once
#include "screen.hpp"
#include "ScreenFactory.hpp"
#include <array>

//stack with screen creator methods
static constexpr size_t MAX_SCREENS = 32;
using ScreenArray = std::array<ScreenFactory::Creator, MAX_SCREENS>;

class Screens {
    ScreenArray stack;
    ScreenArray::iterator stack_iterator;

    ScreenFactory::UniquePtr current;
    ScreenFactory::Creator creator; // set by Open

    bool close;
    bool close_all;
    bool close_serial;

    uint32_t timeout_tick;

    //void stack_push(int16_t screen_id) {}
    //int16_t stack_pop(void) {}

    Screens(const ScreenFactory::Creator screen_creator);
    Screens(const Screens &) = delete;
    static Screens *instance;

public:
    void Loop(); //call inside guiloop

    void Open(const ScreenFactory::Creator screen_creator); //remember creator and create later

    void PushBeforeCurrent(const ScreenFactory::Creator screen_creator);
    void PushBeforeCurrent(const ScreenFactory::Creator *begin, const ScreenFactory::Creator *end);  // push in normal order, skips nullptr
    void RPushBeforeCurrent(const ScreenFactory::Creator *begin, const ScreenFactory::Creator *end); // push in reversed order, skips nullptr

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

    static void Init(const ScreenFactory::Creator screen_creator);
    static void Init(const ScreenFactory::Creator *begin, const ScreenFactory::Creator *end);  // init in normal order, skips nullptr
    static void RInit(const ScreenFactory::Creator *begin, const ScreenFactory::Creator *end); // init in reversed order, skips nullptr

    static Screens *Access();

private:
    void InnerLoop(); //call inside Loop of this class

    bool menu_timeout_enabled = true;
    using r_iter = std::reverse_iterator<const ScreenFactory::Creator *>;
    static r_iter rfind_enabled_node(r_iter begin, r_iter end); // reverse find method
    using iter = const ScreenFactory::Creator *;
    static iter find_enabled_node(iter begin, iter end); // normal find method
};
