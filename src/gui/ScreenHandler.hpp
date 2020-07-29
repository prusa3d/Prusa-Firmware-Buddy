#pragma once
#include "window_frame.hpp"
#include "ScreenFactory.hpp"
#include <array>

//stack with screen creator methods
static constexpr size_t MAX_SCREENS = 32;
using ScreenArray = std::array<ScreenFactory::Creator, MAX_SCREENS>;

struct ScreensInitNode {
    ScreenFactory::Creator screen_creator;
    bool enabled;
};

class Screens {
    ScreenArray stack;
    ScreenArray::iterator stack_iterator;

    ScreenFactory::UniquePtr current;
    ScreenFactory::Creator creator; // set by Open

    bool close;
    bool close_all;

    //void stack_push(int16_t screen_id) {}
    //int16_t stack_pop(void) {}

    Screens(ScreenFactory::Creator screen_creator);
    Screens(const Screens &) = delete;
    static Screens *instance;

public:
    void Loop(); //call inside guiloop

    void Open(ScreenFactory::Creator screen_creator); //remember creator and create later

    void Close();

    void CloseAll();

    bool ConsumeClose(); //dialog can erase close signal and close itself

    void Draw();

    void ScreenEvent(window_t *sender, uint8_t event, void *param);

    window_frame_t *Get();

    static void Init(ScreenFactory::Creator screen_creator);
    static void RInit(ScreensInitNode *begin, ScreensInitNode *end); // init in reversed order
    static void Init(ScreensInitNode *begin, ScreensInitNode *end);  // init in normal
    static Screens *Access();

private:
    using r_iter = std::reverse_iterator<ScreensInitNode *>;
    static r_iter rfind_enabled_node(r_iter begin, r_iter end); // reverse find method
    using iter = ScreensInitNode *;
    static iter find_enabled_node(iter begin, iter end); // normal find method
};
