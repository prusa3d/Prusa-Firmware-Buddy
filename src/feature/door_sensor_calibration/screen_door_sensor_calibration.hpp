#pragma once
#include <screen_fsm.hpp>
#include <radio_button_fsm.hpp>
#include <option/has_door_sensor_calibration.h>

class ScreenDoorSensorCalibration final : public ScreenFSM {
private:
    RadioButtonFSM radio;

public:
    ScreenDoorSensorCalibration();
    ~ScreenDoorSensorCalibration();

protected:
    void create_frame() final;
    void destroy_frame() final;
    void update_frame() final;
};
