// odometer.hpp

/// Singleton class that measures
/// distance traveled and filament consumed
class Odometer_s {
public:
    enum class axis_t {
        X,
        Y,
        Z,
        E,
        count_
    };
    static constexpr size_t axis_count = size_t(axis_t::count_);

private:
    Odometer_s() {}

    /// stores value changes from the last save
    /// extruder trip counts length of filament used (not moved)
    /// new values are stored to RAM (fast, unlimited writes)
    /// it should be stored to EEPROM after a while (slow, limited number of writes)
    float trip_xyze[axis_count];

public:
    /// saves values to EEPROM if they are not zero
    void force_to_eeprom();
    /// save new movement
    void add_value(int axis, float value);
    /// read values from EEPROM
    float get_from_eeprom(axis_t axis);
    /// \returns a value of the specific axis
    float get(axis_t axis);

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
