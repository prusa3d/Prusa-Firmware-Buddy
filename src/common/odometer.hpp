// odometer.hpp

enum {
    ODOMETER_AXES = 4,
};

class odometer_c {
public:
    /// stores value changes from the last save
    /// extruder trip counts length of filament used (not moved)
    /// new values are not added to the total values immediately
    /// to improve precision (1e20 + 1 = 1e20)
    float trip_xyze[ODOMETER_AXES];
    /// saves new values to EEPROM if the change is significant
    void lazy_add_to_eeprom();
    /// saves values to EEPROM if they are not zero
    void force_to_eeprom();
    /// save new movement
    void add_new_value(int axis, float value);
    /// read values from EEPROM
    float get_from_eeprom(int axis);
    /// read current status
    float get(int axis);
};

extern odometer_c odometer;
