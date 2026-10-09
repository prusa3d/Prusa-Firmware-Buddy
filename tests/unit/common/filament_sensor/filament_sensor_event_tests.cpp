#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <feature/filament_sensor/filament_sensor.hpp>

namespace {

class TestFilamentSensor final : public IFSensor {
public:
    explicit TestFilamentSensor(FilamentSensorID::Position position)
        : IFSensor(FilamentSensorID { .position = position, .index = 0 }) {}

    Event sample(FilamentSensorState next_state) {
        state = next_state;
        check_for_events();
        return last_event();
    }

private:
    void cycle() override {}
};

} // namespace

TEST_CASE("Filament insertion produces a single event for either sensor", "[filament_sensor]") {
    const auto position = GENERATE(FilamentSensorID::Position::extruder, FilamentSensorID::Position::side);
    TestFilamentSensor sensor(position);

    CHECK(sensor.sample(FilamentSensorState::NoFilament) == IFSensor::Event::no_event);
    CHECK(sensor.sample(FilamentSensorState::NoFilament) == IFSensor::Event::no_event);
    CHECK(sensor.sample(FilamentSensorState::HasFilament) == IFSensor::Event::filament_inserted);
    CHECK(sensor.sample(FilamentSensorState::HasFilament) == IFSensor::Event::no_event);
    CHECK(sensor.sample(FilamentSensorState::NoFilament) == IFSensor::Event::filament_removed);
    CHECK(sensor.sample(FilamentSensorState::NoFilament) == IFSensor::Event::no_event);
    CHECK(sensor.sample(FilamentSensorState::HasFilament) == IFSensor::Event::filament_inserted);
}

TEST_CASE("Filament already present at startup is not an insertion", "[filament_sensor]") {
    const auto position = GENERATE(FilamentSensorID::Position::extruder, FilamentSensorID::Position::side);
    TestFilamentSensor sensor(position);

    CHECK(sensor.sample(FilamentSensorState::HasFilament) == IFSensor::Event::no_event);
    CHECK(sensor.sample(FilamentSensorState::HasFilament) == IFSensor::Event::no_event);
}

TEST_CASE("Filament sensor recovery does not generate insertion or removal events", "[filament_sensor]") {
    const auto position = GENERATE(FilamentSensorID::Position::extruder, FilamentSensorID::Position::side);
    const auto unavailable = GENERATE(FilamentSensorState::NotInitialized, FilamentSensorState::NotCalibrated,
        FilamentSensorState::NotConnected, FilamentSensorState::Disabled);
    const auto before = GENERATE(FilamentSensorState::NoFilament, FilamentSensorState::HasFilament);
    const auto after = GENERATE(FilamentSensorState::NoFilament, FilamentSensorState::HasFilament);
    TestFilamentSensor sensor(position);

    CHECK(sensor.sample(before) == IFSensor::Event::no_event);
    CHECK(sensor.sample(unavailable) == IFSensor::Event::no_event);
    CHECK(sensor.sample(after) == IFSensor::Event::no_event);
    CHECK(sensor.sample(after) == IFSensor::Event::no_event);
}
