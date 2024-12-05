#pragma once

#include <common/encoded_fsm_response.hpp>
#include <freertos/queue.hpp>
#include <common/marlin_events.h>
#include <gcode/inject_queue_actions.hpp>

namespace marlin_server {

/// marlin_client -> marlin_server request
struct Request {
    enum class Type : uint8_t {
        EventMask,
        Gcode,
        Inject,
        SetVariable,
        Babystep,
        TestStart,
        TestAbort,
        PrintStart,
        PrintAbort,
        PrintPause,
        PrintResume,
        TryRecoverFromMediaError,
        PrintExit,
        KnobMove,
        KnobClick,
        FSM,
        PrintReady,
        GuiCantPrint,
        CancelObjectID,
        UncancelObjectID,
        CancelCurrentObject,
        SetWarning,
    };

    union {
        uint64_t event_mask = 0; // Type::EventMask
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
        char gcode[MARLIN_MAX_REQUEST + 1]; // Type::Gcode
        InjectQueueRecord inject; // Type::Inject
        EncodedFSMResponse encoded_fsm_response; // Type::FSM
        float babystep; // Type::Babystep
        struct {
            marlin_server::PreviewSkipIfAble skip_preview;
            char filename[FILE_PATH_BUFFER_LEN];
        } print_start; // Type::PrintStart
        WarningType warning_type;
    };

    /// if it is set to 1, then the marlin server sends an acknowledge (default)
    /// in some cases (sending a request from svc task) waiting is prohibited and it is necessary not to request an acknowledgment
    unsigned response_required : 1;
    unsigned client_id : 7;
    Type type;
};

using ServerQueue = freertos::Queue<Request, 1>;
extern ServerQueue server_queue;

} // namespace marlin_server
