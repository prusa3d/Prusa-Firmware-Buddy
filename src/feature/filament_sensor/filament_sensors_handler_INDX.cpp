/// @file
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <feature/filament_sensor/filament_sensor_xbuddy_extension.hpp>

FSensorXBuddyExtension *getSideFSensor(PhysicalToolIndex index) {
    static StrongIndexArray<FSensorXBuddyExtension, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> sensors { {
        FSensorXBuddyExtension { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 0 }, FSensorXBuddyExtension::Source::ext },
        FSensorXBuddyExtension { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 1 }, FSensorXBuddyExtension::Source::ext },
        FSensorXBuddyExtension { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 2 }, FSensorXBuddyExtension::Source::ext },
        FSensorXBuddyExtension { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 3 }, FSensorXBuddyExtension::Source::ext },
        FSensorXBuddyExtension { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 4 }, FSensorXBuddyExtension::Source::ext },
        FSensorXBuddyExtension { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 5 }, FSensorXBuddyExtension::Source::ext },
        FSensorXBuddyExtension { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 6 }, FSensorXBuddyExtension::Source::ext },
        FSensorXBuddyExtension { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 7 }, FSensorXBuddyExtension::Source::ext },
    } };
    return &sensors[index];
}

// function returning abstract sensor - used in higher level api
IFSensor *GetExtruderFSensor([[maybe_unused]] PhysicalToolIndex index) {
    /// No extruder sensor on INDX. The sensors are side sensors (on the XBuddy Extension).
    return nullptr;
}

// function returning abstract sensor - used in higher level api
IFSensor *GetSideFSensor(PhysicalToolIndex index) {
    return getSideFSensor(index);
}
