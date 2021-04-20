// odometer.hpp

enum {
    ODOMETER_AXES = 4,
};

/// Singleton class that measures
/// distance traveled and filament consumed
class Odometer_s {
private:
    Odometer_s() {}

    /// stores value changes from the last save
    /// extruder trip counts length of filament used (not moved)
    /// new values are stored to RAM (fast, unlimited writes)
    /// it should be stored to EEPROM after a while (slow, limited number of writes)
    float trip_xyze[ODOMETER_AXES];

public:
    /// saves values to EEPROM if they are not zero
    void force_to_eeprom();
    /// save new movement
    void add_value(int axis, float value);
    /// read values from EEPROM
    float get_from_eeprom(int axis);
    /// \returns a value of the specific axis
    float get(int axis);

    /// Mayer's singleton must have part
public:
    static Odometer_s &instance() {
        static Odometer_s s;
        return s;
    }
    Odometer_s(const Odometer_s &) = delete;
    Odometer_s &operator=(const Odometer_s &) = delete;

private:
    ~Odometer_s() {}
};

extern Odometer_s &odometer_s;
