#pragma once
#include <inc/MarlinConfigPre.h>

#if ENABLED(PRUSA_TOOLCHANGER)
    #include <inc/MarlinConfig.h>
    #include <puppies/Dwarf.hpp>
    #include <module/tool_change.h>

struct PrusaToolInfo {
    float dock_x;
    float dock_y;
};

class PrusaToolChangerUtils {
public:
    static constexpr uint8_t MARLIN_NO_TOOL_PICKED = EXTRUDERS - 1;
    static constexpr auto PARKING_CURRENT_MA = 950; ///< Higher motor current on the lock and unlock moves
    static constexpr auto PARKING_STALL_SENSITIVITY = 4; ///< Stall sensitivity used when PARKING_CURRENT_MA is used
    static constexpr auto PARKING_FINAL_MAX_SPEED = 300.f; ///< Maximum speed (mm/s) for parking
    static constexpr auto SLOW_ACCELERATION_MM_S2 = 400; ///< Acceleration for parking and picking
    static constexpr auto FORCE_MOVE_MM_S = 30; ///< Not used here, feedrate for locking and unlocking the toolchange clamps
    static constexpr auto SLOW_MOVE_MM_S = 50; ///< Feedrate for tool picking and parking
    static constexpr auto Z_HOP_FEEDRATE_MM_S = 10.0f; ///< Feedrate for z hop
    static constexpr auto TRAVEL_MOVE_MM_S = 400.f; ///< Feedrate for moves around dock
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

    /// Feedrate for moves around dock
    static float limit_stealth_feedrate(float feedrate);

public:
    PrusaToolChangerUtils();

    /**
     * @brief Initialize toolchanger.
     * @param first_run should be true only the first time after boot
     * @return true on success, false on communication error
     */
    bool init(bool first_run);

    /**
     * @brief Request change of active dwarf.
     * This is only to be used in crash recovery, otherwise it could mess up marlin.
     * @param new_dwarf pointer to dwarf that will be selected as active, nullptr to select no dwarf
     * If it fails, it throws redscreen.
     */
    void request_active_switch(buddy::puppies::Dwarf *new_dwarf);

    /**
     * @brief Update dwarf picked/parked and change active tool.
     * Called from puppy task.
     * @return true on success, false on communication error
     */
    bool update();

    /**
     * @brief Ger marlin tool index of a physically picked tool.
     * Can be called from anywhere.
     */
    uint8_t detect_tool_nr();

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

    buddy::puppies::Dwarf &getTool(uint8_t tool_index);
    const PrusaToolInfo &get_tool_info(const buddy::puppies::Dwarf &dwarf, bool check_calibrated = false) const;
    bool is_tool_info_valid(const buddy::puppies::Dwarf &dwarf, const PrusaToolInfo &info) const;
    bool is_tool_info_valid(const buddy::puppies::Dwarf &dwarf) const;
    void set_tool_info(const buddy::puppies::Dwarf &dwarf, const PrusaToolInfo &info);

    /**
     * @brief Get binary mask of all enabled dwarfs.
     * @return high bits for dwarfs that is_enabled()
     */
    uint8_t get_enabled_mask();

    /**
     * @brief Get binary mask of all parked dwarfs.
     * @return high bits for dwarfs that is_enabled() && is_parked() && !is_picked()
     */
    uint8_t get_parked_mask();

    inline bool is_toolchanger_enabled() {
        return toolchanger_enabled;
    }

    ///@return True if at least one dwarf is connected through splitter.
    inline bool is_splitter_enabled() {
        return is_tool_enabled(2) || is_tool_enabled(3) || is_tool_enabled(4) || is_tool_enabled(5);
    }

    inline bool is_tool_enabled(uint8_t tool) {
        assert(tool < buddy::puppies::dwarfs.size());
        return buddy::puppies::dwarfs[tool].is_enabled();
    }

    inline bool has_tool() const {
        return get_active_tool_nr() != MARLIN_NO_TOOL_PICKED;
    }

    [[nodiscard]] uint8_t get_active_tool_nr() const;

    [[nodiscard]] bool is_any_tool_active() const;
    [[nodiscard]] bool is_tool_active(uint8_t idx) const;
    [[nodiscard]] uint8_t get_num_enabled_tools() const;

    void load_tool_info();
    void save_tool_info();
    void load_tool_offsets();
    void save_tool_offsets();

    bool load_tool_info_from_usb();
    bool save_tool_info_to_usb();
    bool load_tool_offsets_from_usb();
    bool save_tool_offsets_to_file(const char *filename = "/usb/tooloffsets.txt");

    void expand_first_dock_position(); // TODO: Is this still needed/wanted ?

protected:
    std::atomic<bool> force_toolchange_gcode = false; ///< after reset force toolchange to init marlin tool variables
    std::atomic<bool> request_toolchange = false; ///< when true, toolchange was requested and will be executed in puppytask
    std::atomic<buddy::puppies::Dwarf *> request_toolchange_dwarf; ///< when request_toolchange=true, this specifies what tool will be changed to

    /**
     * @brief Structure that sets something in its constructor and resets when destroyed.
     * Function set_state should be lambda and set something if state is true and reset if state is false.
     * It is used to clean up something on a return from a function.
     */
    struct ResetOnReturn {
        std::function<void(bool)> set_state;
        [[nodiscard]] ResetOnReturn(std::function<void(bool)> set_state)
            : set_state(set_state) { set_state(true); }
        ~ResetOnReturn() { set_state(false); }
    };

    bool toolchanger_enabled = false;

    std::atomic<buddy::puppies::Dwarf *> picked_dwarf = nullptr; ///< what tool was physically detected as picked
    std::atomic<bool> picked_update = false; ///< Set true each time picked_dwarf is updated
    std::atomic<buddy::puppies::Dwarf *> active_dwarf = nullptr; ///< what tool is active in puppytask

    std::array<PrusaToolInfo, EXTRUDERS> tool_info;

    [[nodiscard]] PrusaToolInfo compute_synthetic_tool_info(const buddy::puppies::Dwarf &dwarf) const;

    bool autodetect_toolchanger_enabled();

    /**
     * @brief Get picked and parked states and detect which tool is active.
     * Modifies parked, picked and picked_dwarf.
     */
    void autodetect_active_tool();

    [[noreturn]] void toolchanger_error(const char *message) const;

    /**
     * @brief Wait until function returns true.
     * @param function wait for this to return true
     * @param timeout_ms maximal time to wait [ms]
     * @return true on success, false if timeout was reached
     */
    [[nodiscard]] bool wait(std::function<bool()> function, uint32_t timeout_ms);

    /**
     * @brief Force a selected tool to marlin.
     * @param dwarf force active_tool to be this dwarf, or nullptr for no tool
     */
    void force_marlin_picked_tool(buddy::puppies::Dwarf *dwarf);

    /**
     * @brief Get maximum difference of MBL height
     * @return float
     */
    float get_mbl_z_lift_height() const;

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

public:
    /**
     * @brief Restore planner feedrate and acceleration.
     * This is for powerpanic to restore original planner config if panic happens during toolchange.
     * @return true if feedrate and acceleration were restored
     */
    bool try_restore() { return conf_restorer.try_restore(); }
};

#endif /*ENABLED(PRUSA_TOOLCHANGER)*/
