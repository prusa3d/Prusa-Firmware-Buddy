#pragma once
#include "window_frame.hpp"
#include "ScreenFactory.hpp"
#include <array>

//stack with screen creator methods
using ScreenArray = std::array<ScreenFactory::Creator, 32>;

class Screens {

    ScreenArray stack;
    ScreenArray::iterator stack_iterator;

    ScreenFactory::UniquePtr current;
    ScreenFactory::Creator creator; // set by Open

    bool close;

    //void stack_push(int16_t screen_id) {}
    //int16_t stack_pop(void) {}

    Screens(ScreenFactory::Creator screen_creator);
    Screens(const Screens &) = delete;
    static Screens *instance;

public:
    void Loop(); //call inside guiloop

    void Open(ScreenFactory::Creator screen_creator); //remember creator and create later

    void Close();

    bool ConsumeClose(); //dialog can erase close signal and close itself

    void Draw();

    void ScreenEvent(window_t *sender, uint8_t event, void *param);

    window_frame_t *Get();

    static void Init(ScreenFactory::Creator screen_creator);
    static Screens *Access();
};
