// odometer.hpp

class odometer_c {
private:
public:
    /// stores value changes from the last save
    /// extruder trip counts length of filament used (not retractions)
    /// new values are not added to the total values immediately
    /// to improve precision (1e10 + 1 = 1e10)
    float trip_xyze[4];
    /// saves new values to EEPROM if the change is significant
    void lazy_add_to_eeprom(int axis = -1);
    /// saves values to EEPROM if they are not zero
    void force_to_eeprom();
    /// save new movement
    void add_new_value(int axis, float value);
};

extern odometer_c odometer;
