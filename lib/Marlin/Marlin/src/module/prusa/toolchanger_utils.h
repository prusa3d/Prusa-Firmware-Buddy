#pragma once
#include <inc/MarlinConfigPre.h>

#include <option/has_indx.h>
#include <option/has_dwarf.h>
#if HAS_INDX()
    #include <module/prusa/indx_dock_position_defaults.hpp>
#endif
#include <utils/storage/strong_index_array.hpp>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <inc/MarlinConfig.h>
    #include <puppies/PuppyModbus.hpp>

    #if HAS_DWARF()
        #include <puppies/Dwarf.hpp>
    #endif
    #if HAS_INDX()
        #include <puppies/INDX.hpp>
    #endif
    #include <module/tool_change.h>
    #include <tool_index.hpp>
    #include <utils/badge.hpp>

    #include <inplace_function.hpp>

struct PrusaToolInfo {
    float dock_x;
    float dock_y;
};

enum class WaitMode {
    default_mode, ///< Bail on planner.draining() (includes user Stop, power panic, and the other quick_stop callers).
    bail_on_power_panic, ///< Keep polling through any planner.draining() cause; only bail when power panic is active.
};

class PrusaToolChangerUtils {
public:
    #if HAS_INDX()
    static constexpr uint8_t MARLIN_NO_TOOL_PICKED = EXTRUDERS - 1;
    static constexpr auto PARKING_FINAL_MAX_SPEED = 300.f; ///< Maximum speed (mm/s) for parking
    static constexpr auto SLOW_MOVE_MM_S = 100; ///< General slow feedrate [mm/s]
    static constexpr auto Z_HOP_FEEDRATE_MM_S = 10.0f; ///< Feedrate for z hop
    static constexpr auto TRAVEL_MOVE_MM_S = 300.f; ///< Feedrate for moves around dock
    static constexpr auto OPEN_HEAD_TRAVEL_MOVE_MM_S = 250.f; ///< Feedrate for open head travel move (skipping on 300 during G28 Z -> tool_change -> open_head)

    // Dock geometry
    static constexpr StrongIndexArray<float, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::no> DOCK_DEFAULT_X_MM { indx_dock_position_defaults::x_mm };
    static constexpr auto DOCK_DEFAULT_Y_MM = indx_dock_position_defaults::y_mm;
    static constexpr auto DOCK_INVALID_OFFSET_X_MM = 1.0f;
    static constexpr auto DOCK_INVALID_OFFSET_Y_MM = 1.0f;
    static constexpr auto EPSILON_MM = 0.05f; ///< Tolerance slack so values displayed within limits (rounded to 0.1 mm) don't fail validation [mm]

    // Y offsets from dock_y (dock_y is the deepest/full-dock position)
    static constexpr float DOCK_SAFE_Y_OFFSET = 28.6f; ///< Collision-free distance in front of dock [mm]
    // Absolute safe Y positions (computed from default dock Y, used by external code for boundary checks)
    static constexpr auto SAFE_Y_WITH_TOOL = DOCK_DEFAULT_Y_MM + DOCK_SAFE_Y_OFFSET;
        #if PRINTER_IS_PRUSA_COREONEL()
    static constexpr float DOCK_UNLOCK_Y_OFFSET = 6.6f; ///< Y offset for the unlock position [mm]
        #else
    static constexpr float DOCK_UNLOCK_Y_OFFSET = 10.6f; ///< Y offset for the unlock position [mm]
        #endif
    static constexpr float DOCK_BACKOFF_Y_OFFSET = 50.0f; ///< Y back-off in front of the dock after parking, to clear it for manual access (tool_return_t::dock_backoff) [mm]

    // E-axis lock/unlock mechanism
    static constexpr float E_WIGGLE_DISTANCE = 0.27f; ///< Tooth alignment wiggle distance [mm]
    static constexpr float E_PARTIAL_UNLOCK_DISTANCE = 1.3f; ///< Partial unlock, head still holds nozzle [mm]
    static constexpr float E_FULL_OPEN_DISTANCE = 11.2f; ///< Full open distance [mm]
    static constexpr float E_FULL_CLOSE_DISTANCE = 12.3f; ///< Full lock distance [mm]

    // E-axis motor currents for lock/unlock
    static constexpr uint16_t E_WIGGLE_CURRENT_MA = 200; ///< Low current for safe tooth engagement [mA]
    static constexpr uint16_t E_UNLOCK_CURRENT_MA = 650; ///< Higher current for actual unlock [mA]

    // Park/pickup feedrates [mm/s]
    static constexpr float PARK_APPROACH_FEEDRATE = 250.0f; ///< Approach to unlock position
    static constexpr float DOCK_ENGAGE_FEEDRATE = 100.0f; ///< Deeper dock move during park
    static constexpr float PICKUP_APPROACH_FEEDRATE = 100.0f; ///< Pickup approach into dock
    static constexpr float E_WIGGLE_FEEDRATE = 13.3f; ///< Tooth alignment E moves
    static constexpr float E_UNLOCK_FEEDRATE = 35.0f; ///< Partial unlock E move
    static constexpr float E_FULL_OPEN_FEEDRATE = 40.0f; ///< Full open E move
    static constexpr float E_LOCK_FEEDRATE = 40.0f; ///< Lock E move
    static constexpr float FAST_EXIT_FEEDRATE = 250.0f; ///< Fast exit from dock
    static constexpr float SLOW_EXIT_FEEDRATE = 100.0f; ///< slow exit from dock
    static constexpr uint32_t DOCK_DWELL_MS = 150; ///< Dwell time after lock/unlock [ms]
    static constexpr uint32_t NOZZLE_VERIFY_TIMEOUT_MS = 5000; ///< Max wait for nozzle presence data after pickup/park [ms]
    #else
    static constexpr uint8_t MARLIN_NO_TOOL_PICKED = EXTRUDERS - 1;
    static constexpr auto PARKING_CURRENT_MA = 950; ///< Higher motor current on the lock and unlock moves
    static constexpr auto PARKING_STALL_SENSITIVITY = 4; ///< Stall sensitivity used when PARKING_CURRENT_MA is used
    static constexpr auto PARKING_FINAL_MAX_SPEED = 300.f; ///< Maximum speed (mm/s) for parking
    static constexpr auto SLOW_ACCELERATION_MM_S2 = 400; ///< Acceleration for parking and picking
    static constexpr auto FORCE_MOVE_MM_S = 30; ///< Not used here, feedrate for locking and unlocking the toolchange clamps
    static constexpr auto SLOW_MOVE_MM_S = 50; ///< Feedrate for tool picking and parking
    static constexpr auto Z_HOP_FEEDRATE_MM_S = 10.0f; ///< Feedrate for z hop
    static constexpr auto TRAVEL_MOVE_MM_S =
        #if defined(_DEBUG)
        300.f; // stepping computations are too slow for full speed in debug (BFW-8259)
        #else
        400.f;
        #endif ///< Feedrate for moves around dock
    static constexpr uint32_t WAIT_TIME_TOOL_SELECT = 3000; ///< Max wait for puppytask tool switch [ms], needs a lot of time if there is a hiccup in puppy communication
    static constexpr uint32_t WAIT_TIME_TOOL_PARKED_PICKED = 200; ///< Max wait for cheese to detect magnet [ms]
    static constexpr auto SAFE_Y_WITH_TOOL = 360.0f;
    static constexpr auto SAFE_Y_WITHOUT_TOOL = 425.0f;
    static constexpr auto DOCK_OFFSET_X_MM = 82.0f;
    static constexpr auto DOCK_DEFAULT_FIRST_X_MM = 25.0f;
    static constexpr auto DOCK_DEFAULT_Y_MM = 455.0f;
    static constexpr auto DOCK_INVALID_OFFSET_MM = 6; // TODO: Tighten this once 5mm smaller XLs are phased out
    static constexpr auto PURGE_Y_POSITION = 380.0f;
    static constexpr auto DOCK_WIGGLE_OFFSET = 0.5f; ///< Relative offset from intended position when wiggling the tool to detect magnet [mm]
    static constexpr auto PARK_X_OFFSET_1 = -10.0f; ///< Offset from dock_x when tool can be moved into the dock [mm]
    static constexpr auto PARK_X_OFFSET_2 = -9.0f; ///< Offset from dock_x when tool is being unlocked [mm]
    static constexpr auto PARK_X_OFFSET_3 = +0.5f; ///< Offset from dock_x when tool is fully unlocked [mm]
    static constexpr auto PICK_Y_OFFSET = -5.0f; ///< Offset from dock_y before head touches the parked dwarf [mm]
    static constexpr auto PICK_X_OFFSET_1 = -11.8f; ///< Offset from dock_x when tool is being locked [mm]
    static constexpr auto PICK_X_OFFSET_2 = -12.8f; ///< Offset from dock_x when tool is fully locked [mm]
    static constexpr auto PICK_X_OFFSET_3 = -9.9f; ///< Offset from dock_x when tool can be pulled from the dock area [mm]
    static constexpr auto X_UNLOCK_DISTANCE_MM = PICK_X_OFFSET_3; ///< Unlock move length while dock calibrating [mm]
    #endif
    /// Feedrate for moves around dock
    static float limit_stealth_feedrate(float feedrate);

    static bool is_pos_in_toolchange_area(const xy_pos_t &pos);

public:
    PrusaToolChangerUtils();

    #if HAS_DWARF()
    /**
     * @brief Initialize toolchanger.
     * @param first_run should be true only the first time after boot
     * @return true on success, false on communication error
     */
    bool init(buddy::puppies::PuppyModbus &, bool first_run);
    #endif

    #if HAS_INDX()

    void set_active_extruder(std::variant<PhysicalToolIndex, NoTool> tool);

    /// Restore active tool from EEPROM at boot. Must run on marlin thread, exactly once.
    void restore_last_picked_tool();

    inline bool is_toolchanger_enabled() {
        return true;
    }

    #else // !HAS_INDX()

    /**
     * @brief Request change of active dwarf and wait for the puppy task to apply it.
     * Outside of tool_change()'s own sequencing this belongs to crash recovery only - on its own
     * it leaves marlin's active_extruder disagreeing with the selected dwarf.
     * Throws a redscreen if the puppy task does not pick the request up in time.
     * @param new_dwarf pointer to dwarf that will be selected as active, nullptr to select no dwarf
     */
    void request_active_switch(buddy::puppies::Dwarf *new_dwarf);

    /**
     * @brief Update dwarf picked/parked and change active tool.
     * Called from puppy task.
     * @return true on success, false on communication error
     */
    bool update(buddy::puppies::PuppyModbus &);

    /**
     * @brief Ger marlin tool index of a physically picked tool.
     * Can be called from anywhere.
     */
    uint16_t detect_tool_nr();

    inline bool is_toolchanger_enabled() {
        return toolchanger_enabled;
    }

    /**
     * @brief Get currently selected dwarf or dwarfs[0] if none is selected
     * @return Dwarf instance
     */
    buddy::puppies::Dwarf &getActiveToolOrFirst();

    /**
     * @brief Get dwarf that is currently selected in marlin.
     * @return pointer to active dwarf or nullptr
     */
    buddy::puppies::Dwarf *get_marlin_picked_tool();

    buddy::puppies::Dwarf &getTool(PhysicalToolIndex tool);

    /**
     * @brief Get binary mask of all enabled dwarfs.
     * @return high bits for dwarfs that is_enabled()
     */
    uint16_t get_enabled_mask();

    /**
     * @brief Get binary mask of all parked dwarfs.
     * @return high bits for dwarfs that is_enabled() && is_parked() && !is_picked()
     */
    uint16_t get_parked_mask();

    ///@return True if at least one dwarf is connected through splitter.
    inline bool is_splitter_enabled() {
        for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
            if (tool.to_raw() >= 2) {
                return true;
            }
        }
        return false;
    }

    bool autodetect_toolchanger_enabled();
    /**
     * @brief Get picked and parked states and detect which tool is active.
     * Modifies picked_dwarf and picked_update.
     */
    void autodetect_active_tool(buddy::puppies::PuppyModbus &);

    class StepperConfigGuard final {
        uint32_t x_stall_sensitivity; ///< Sensitivity to restore [driver specific]
        uint32_t x_current_ma; ///< Current to restore [mA]
        uint32_t y_stall_sensitivity; ///< Sensitivity to restore [driver specific]
        uint32_t y_current_ma; ///< Current to restore [mA]
    public:
        /**
         * @brief Configure stepper current and stall sensitivity for toolchange.
         * Use constants defined above.
         * Configure only X and Y (A and B) steppers.
         * Use constants defined above, on top of PrusaToolChangerUtils.
         * Remember previous values and restore them in destructor.
         */
        StepperConfigGuard();
        StepperConfigGuard(const StepperConfigGuard &) = delete; ///< No copy constructor
        StepperConfigGuard &operator=(const StepperConfigGuard &) = delete; ///< No copy assignment
        StepperConfigGuard(StepperConfigGuard &&) = delete; ///< No move constructor
        StepperConfigGuard &operator=(StepperConfigGuard &&) = delete; ///< No move assignment
        ~StepperConfigGuard(); ///< Restore stepper current and stall sensitivity
    };

    #endif // HAS_INDX()

    /// Should be used only by PhysicalToolIndex class.
    /// Use tool.is_enabled() instead.
    bool is_tool_enabled(PhysicalToolIndex tool, Badge<PhysicalToolIndex>);

    const PrusaToolInfo &get_tool_info(PhysicalToolIndex tool, bool check_calibrated = false) const;
    [[nodiscard]] PrusaToolInfo create_default_tool_info(PhysicalToolIndex tool_index) const;

    [[nodiscard]] bool is_tool_info_valid(PhysicalToolIndex tool, const PrusaToolInfo &info) const;
    [[nodiscard]] bool is_tool_info_valid(PhysicalToolIndex tool) const {
        return is_tool_info_valid(tool, get_tool_info(tool, false));
    }

    void set_tool_info(PhysicalToolIndex tool, const PrusaToolInfo &info);

    [[nodiscard]] uint8_t get_num_enabled_tools() const;

    void load_tool_info();
    void save_tool_info();
    void load_tool_offsets();
    void save_tool_offset(PhysicalToolIndex tool);
    void save_tool_offsets();

protected:
    #if !HAS_INDX()
    std::atomic<bool> force_toolchange_gcode = false; ///< after reset force toolchange to init marlin tool variables
    std::atomic<bool> request_toolchange = false; ///< when true, toolchange was requested and will be executed in puppytask
    std::atomic<buddy::puppies::Dwarf *> request_toolchange_dwarf; ///< when request_toolchange=true, this specifies what tool will be changed to
    bool toolchanger_enabled = false;
    std::atomic<buddy::puppies::Dwarf *> picked_dwarf = nullptr; ///< what tool was physically detected as picked
    std::atomic<buddy::puppies::Dwarf *> active_dwarf = nullptr; ///< what tool is active in puppytask
    std::atomic<bool> picked_update = false; ///< Set true each time picked_dwarf is updated

    /**
     * @brief Force a selected tool to marlin.
     * @param dwarf force active_tool to be this dwarf, or nullptr for no tool
     */
    void force_marlin_picked_tool(buddy::puppies::Dwarf *dwarf);
    #endif

    StrongIndexArray<PrusaToolInfo, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> tool_info;

    [[noreturn]] void toolchanger_error(const char *message) const;

    /**
     * @brief Wait until function returns true.
     * @param function wait for this to return true
     * @param timeout_ms maximal time to wait [ms]
     * @param mode controls the bail-out condition during the wait (see WaitMode)
     * @return true on success, false if the condition was not met (timeout or early-exit per @p mode)
     */
    [[nodiscard]] bool wait(const stdext::inplace_function<bool()> &function, uint32_t timeout_ms, WaitMode mode = WaitMode::default_mode);

    /**
     * @brief Get maximum difference of MBL height
     * @return float
     */
    float get_mbl_z_lift_height() const;

    #if !HAS_INDX()
    /**
     * @brief Structure to sample and restore planner feedrate and acceleration.
     * This is important for powerpanic which stores the original values and not temporary values used while changing tools.
     */
    class ConfRestorer {
        xyze_pos_t sampled_jerk; ///< Copy of planner.planner.max_jerk
        float sampled_travel_acceleration; ///< Copy of planner.settings.travel_acceleration
        feedRate_t sampled_feedrate_mm_s; ///< Copy of feedrate_mm_s
        int16_t sampled_feedrate_percentage; ///< Copy of feedrate_percentage
        std::atomic<bool> sampled; ///< True if configuration is stored

    public:
        ConfRestorer()
            : sampled(false) {}

        /**
         * @brief Sample planner feedrate and acceleration.
         */
        void sample();

        /**
         * @brief Restore planner feedrate and acceleration.
         */
        void restore() {
            restore_jerk();
            restore_acceleration();
            restore_feedrate();
        }

        /**
         * @brief Try to restore planner feedrate and acceleration.
         * @return true if feedrate and acceleration were restored
         */
        bool try_restore() {
            if (sampled.load()) {
                restore();
                return true;
            }
            return false;
        }

        /**
         * @brief Restore and clear planner feedrate and acceleration.
         */
        void restore_clear();

        /**
         * @brief Restore planner jerk.
         */
        void restore_jerk();

        /**
         * @brief Restore planner acceleration.
         */
        void restore_acceleration();

        /**
         * @brief Restore planner feedrate.
         */
        void restore_feedrate();
    } conf_restorer;
    #endif

public:
    #if HAS_INDX()
    /**
     * @brief Noop on INDX
     */
    bool try_restore() { return true; }
    #else
    /**
     * @brief Restore planner feedrate and acceleration.
     * This is for powerpanic to restore original planner config if panic happens during toolchange.
     * @return true if feedrate and acceleration were restored
     */
    bool try_restore() { return conf_restorer.try_restore(); }
    #endif
};

#endif
