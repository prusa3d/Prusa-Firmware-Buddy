#pragma once

#include <common/freertos_queue.hpp>
#include <common/marlin_events.h>

namespace marlin_server {

/// marlin_client -> marlin_server request
struct __attribute__((packed)) Request {
    enum class Type : uint8_t {
        EventMask,
        Stop,
        Start,
        Gcode,
        InjectGcode,
        SetVariable,
        Babystep,
        ConfigSave,
        ConfigLoad,
        ConfigReset,
        TestStart,
        TestAbort,
        PrintStart,
        PrintAbort,
        PrintPause,
        PrintResume,
        PrintExit,
        Park,
        KnobMove,
        KnobClick,
        FSM,
        Move,
        PrintReady,
        GuiCantPrint,
        CancelObjectID,
        UncancelObjectID,
        CancelCurrentObject,
        MoveMultiple,
    };

    uint8_t client_id;
    Type type;

    union {
        uint64_t event_mask; // Type::EventMask
        struct {
            float position;
            float feedrate;
            uint8_t axis;
        } move; // Type::Move
        int cancel_object_id; // Type::CancelObjectID
        int uncancel_object_id; // Type::UncancelObjectID
        struct {
            uintptr_t variable;
            union {
                float float_value;
                uint32_t uint32_value;
            };
        } set_variable; // Type::SetVariable
        struct {
            uint64_t test_mask;
            size_t test_data_index;
            uint32_t test_data_data;
        } test_start; // Type::TestStart
        struct {
            float x;
            float y;
            float z;
            float feedrate;
        } move_multiple; // Type::MoveMultiple
        char gcode[MARLIN_MAX_REQUEST + 1]; // Type::Gcode
        char inject_gcode[MARLIN_MAX_REQUEST + 1]; // Type::InjectGcode
        uint32_t fsm; // Type::FSM
        float babystep; // Type::Babystep
        struct {
            marlin_server::PreviewSkipIfAble skip_preview;
            char filename[FILE_PATH_BUFFER_LEN];
        } print_start; // Type::PrintStart
    };
};
static_assert(std::is_trivial_v<Request>);

using ServerQueue = freertos::Queue<Request, 1>;
extern ServerQueue server_queue;

} // namespace marlin_server
