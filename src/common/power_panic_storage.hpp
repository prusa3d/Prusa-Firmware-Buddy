
#include <option/has_modular_bed.h>
#include <option/has_puppies.h>
#include <option/has_toolchanger.h>
#include <option/has_chamber_api.h>
#include <option/has_motor_current_profiles.h>
#include <option/has_indx.h>
#include <option/has_tool_crash_recovery.h>

#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

#include "marlin_server.hpp"

#include "../lib/Marlin/Marlin/src/feature/prusa/crash_recovery.hpp"

#include "../lib/Marlin/Marlin/src/feature/bedlevel/bedlevel.h"
#if ENABLED(AUTO_BED_LEVELING_UBL)
    #include "../lib/Marlin/Marlin/src/feature/bedlevel/ubl/ubl.h"
#else
    #error "powerpanic currently supports only UBL"
#endif

#include <option/has_cancel_object.h>
#if HAS_CANCEL_OBJECT()
    #include <feature/cancel_object/cancel_object.hpp>
#endif

#if HAS_TOOL_MAPPING()
    #include "module/prusa/tool_mapper.hpp"
#endif

#include <option/has_spool_join.h>
#if HAS_SPOOL_JOIN()
    #include <module/prusa/spool_join.hpp>
#endif

#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
#endif

#include "../lib/Marlin/Marlin/src/feature/input_shaper/input_shaper_config.hpp"
#include "../lib/Marlin/Marlin/src/feature/pressure_advance/pressure_advance_config.hpp"

#include <logging/log.hpp>

#include <common/sys.hpp>
#include <buddy/filename_defs.hpp>

#include <option/has_gui.h>

#include <option/has_leds.h>
#if HAS_LEDS()
    #include <leds/led_manager.hpp>
#endif

#include <option/has_side_leds.h>

#include <usb_host/usbh_async_diskio.hpp>
#include <gcode/gcode_reader_restore_info.hpp>
#include <tool_index.hpp>
#include <utils/storage/strong_index_array.hpp>

#include "feature/print_area.h"

// WARNING: mind the alignment, the following structures are not automatically packed
// as they're shared also for the on-memory copy. The on-memory copy can be avoided
// if we decide to use two flash blocks (keeping one as a working set)
// main reason for this warnings is we want to keep the structure small
// so if you see this warning, just reorder the fields, eventually add some minimal padding
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wpadded"

namespace power_panic {

// planner state (TODO: a _lot_ of essential state is missing here and Crash_s also due to
// the partial Motion_Parameters implementation)
struct state_planner_t {
    user_planner_settings_t settings;

    // IS/PA
    input_shaper::AxisConfig axis_config[3]; // XYZ
    input_shaper::AxisConfig original_y;
    input_shaper::WeightAdjustConfig axis_y_weight_adjust;
    pressure_advance::Config axis_e_config;

    float z_position;
    float max_printed_z;
#if DISABLED(CLASSIC_JERK)
    float junction_deviation_mm;
#endif

    int16_t target_bed;
    int16_t extrude_min_temp;
#if HAS_MODULAR_BED()
    uint16_t enabled_bedlets_mask;
    uint8_t _padding_heat[2]; // padding to 4 bytes
#endif

    uint16_t print_speed;
    uint8_t was_paused;
    uint8_t was_crashed;
    uint8_t fan_speed;
    uint8_t axis_relative;
    uint8_t allow_cold_extrude;

    PrinterGCodeCompatibilityReport compatibility;

    StrongIndexArray<int16_t, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> target_nozzle;
    StrongIndexArray<int16_t, VirtualToolIndex::count, VirtualToolIndex, VirtualToolIndex::to_raw_static> flow_percentage;

    uint8_t marlin_debug_flags;

#if HAS_MOTOR_CURRENT_PROFILES()
    uint8_t current_profile { 0 };
#endif

#if HAS_INDX()
    uint8_t active_tool { PrusaToolChanger::MARLIN_NO_TOOL_PICKED };
#endif

    // everything to target_nozzle is aligned - padding the remaining fields
    static constexpr int _size_to_pad = sizeof(target_nozzle) + sizeof(flow_percentage) + sizeof(marlin_debug_flags)
#if HAS_MOTOR_CURRENT_PROFILES()
        + sizeof(current_profile)
#endif
#if HAS_INDX()
        + sizeof(active_tool) // uint8_t
#endif
        ;
    uint8_t _padding[(4 - _size_to_pad % 4) % 4];
};

// fully independent state that persist across panics until the end of the print
struct state_print_t {
    float odometer_e_start; /// E odometer value at the start of the print
};

// crash recovery data
struct state_crash_t {
    uint32_t sdpos; /// sdpos of the gcode instruction being aborted

    /// Value of current_position at the start of the interrupted GCode
    xyze_pos_t start_current_position;

    /// Basically just crash_machine_position as native coordinates instead of machine
    xyze_pos_t crash_native_position;

    /// Exact stepper positions at the time of the crash
    MachinePosXYZE crash_machine_position;

    uint16_t segments_finished = 0;
    uint8_t leveling_active; /// state of MBL before crashing
    AxesHomeLevel axes_home_level; /// axis state before crashing
    Crash_s::RecoverFlags recover_flags; /// instruction replay flags
    uint8_t _padding[1]; // Silence the compiler
    feedRate_t fr_mm_s; /// current move feedrate
    Crash_s_Counters::Data counters;
    uint8_t _padding2[2]; // Silence the compiler
};

// print progress data
struct state_progress_t {
    struct ModeSpecificData {
        uint32_t percent_done;
        uint32_t time_to_end;
        uint32_t time_to_pause;
    };

    millis_t print_duration;
    ModeSpecificData standard_mode, stealth_mode;
};

// toolchanger recovery info
//   can't use PrusaToolChanger::ToolchangeReturnData as it doesn't have to be packed
struct state_toolchanger_t {
#if HAS_TOOLCHANGER() && (HAS_TOOL_CRASH_RECOVERY() || HAS_INDX())
    XYZval<float, LogicalPosTag> return_pos; ///< Position wanted after toolchange
    uint8_t tool_nr; ///< Tool wanted to be picked before panic (MARLIN_NO_TOOL_PICKED if none)
    tool_return_t return_type : 8; ///< Where to return after recovery
    #if HAS_INDX()
    PrusaToolChanger::ToolchangePhase phase : 8; ///< Toolchange phase if PP hit mid-toolchange
    uint32_t : 8; ///< Padding to keep the structure size aligned to 32 bit
    #else
    uint32_t : 16; ///< Padding to keep the structure size aligned to 32 bit
    #endif
#endif
};

#pragma GCC diagnostic pop

// Data storage layout
struct fixed_t {
    xy_pos_t bounding_rect_a;
    xy_pos_t bounding_rect_b;
    bed_mesh_t z_values;
    char media_SFN_path[filename_defs::max_path_length];

    static void load();
    static void save();
};

// varying parameters
struct state_t {
    state_crash_t crash;
    state_planner_t planner;
    state_progress_t progress;
    state_print_t print;
#if HAS_TOOL_CRASH_RECOVERY() || HAS_INDX()
    state_toolchanger_t toolchanger;
#endif

#if HAS_CANCEL_OBJECT()
    buddy::CancelObject::State cancel_object;
#endif
#if HAS_TOOL_MAPPING()
    ToolMapper::serialized_state_t tool_mapping;
#endif
#if HAS_SPOOL_JOIN()
    SpoolJoin::serialized_state_t spool_join;
#endif
#if HAS_CHAMBER_API()
    // The chamber API has it as optional, we map nullopt (disabled) to
    // chamber_temp_off (0xffff), as optional is not guaranteed to be POD.
    uint16_t chamber_target_temp;
#endif
#if HAS_TEMP_HEATBREAK_CONTROL
    // we keep old array size instead of PhysicalToolIndex::count because of weak indexing (see definition of PhysicalToolIndex::count)
    StrongIndexArray<uint8_t, HOTENDS, PhysicalToolIndex, PhysicalToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> heatbreak_temperatures;
#endif
    GCodeReaderStreamRestoreInfo gcode_stream_restore_info;

    static void load();
    static void save();
};

enum class PPState : uint8_t {
    // note: order is important, there is check that PPState >= Triggered
    Inactive,
    Prepared,
    Triggered,
    Retracting,
    SaveState,
    WaitingToDie,
};

/// State power panic data is stored here
extern state_t &state_buf;

struct runtime_state_t {
    bool nested_fault;
    PPState orig_state; // state that was active when power panic was triggered
    char media_SFN_path[filename_defs::max_path_length]; // temporary buffer
    AxesHomeLevel orig_axes_home_level;
    uint32_t fault_stamp; // time since acFault trigger
};
/// Runtime state of the power panic, runtime means that is not persisted on restart, just on PP save/resume
extern runtime_state_t runtime_state;

// Will erase persistent data, note that it doesn't reset state_buf!
void erase();

} // namespace power_panic
