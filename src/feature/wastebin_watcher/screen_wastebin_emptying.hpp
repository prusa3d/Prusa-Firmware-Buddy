/// @file
#pragma once

#include <screen_fsm.hpp>

class ScreenWastebinEmptying final : public ScreenFSM {
public:
    ScreenWastebinEmptying();
    ~ScreenWastebinEmptying();

protected:
    void create_frame() final;
    void destroy_frame() final;
    void update_frame() final;
};
