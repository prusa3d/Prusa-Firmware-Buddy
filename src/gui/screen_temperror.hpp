//screen_temperror.hpp
#pragma once
#include "gui.hpp"
#include "screen_reset_error.hpp"

class screen_temperror_data_t : public screen_reset_error_data_t {

public:
    screen_temperror_data_t();

protected:
    virtual void draw() override;
};
