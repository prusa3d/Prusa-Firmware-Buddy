#include "gcode_compatibility.hpp"

#include <config_store/store_instance.hpp>
#include <gcode_info.hpp>
#include <filament.hpp>
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <print_utils.hpp>
#include <test_result.hpp>
#include <tool/physical_tool.hpp>
#include <window_msgbox.hpp>
#include <feature/compatibility_checks/filament_compatibility.hpp>

#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include <feature/prusa/MMU2/mmu2_mk4.h>
#endif

#include <option/has_tool_mapping.h>
#if HAS_TOOL_MAPPING()
    #include <tools_mapping.hpp>
#endif

#include <option/has_spool_join.h>
#if HAS_SPOOL_JOIN()
    #include <module/prusa/spool_join.hpp>
#endif

#include <option/has_indx.h>

#include <option/has_anfc.h>
#if HAS_ANFC()
    #include <feature/openprinttag/filament_usage_tracker/filament_usage_tracker.hpp>
#endif

#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

// Needed for ChecksTraits<GeneralCheck>::metadata
using namespace buddy::compatibility_checks;
using namespace buddy::gcode_compatibility;

template <>
constinit const ChecksTraits<GeneralCheck>::Metadata ChecksTraits<GeneralCheck>::metadata {
    {
        GeneralCheck::printer_model,
        CheckMetadata {
            .severity = HWCheckType::model,
            .title = N_("Incompatible printer model"),
            .description = N_("G-Code is sliced for a different printer and is not compatible."),
        },
    },
#if HAS_GCODE_COMPATIBILITY()
        {
            GeneralCheck::gcode_compatibility_mode,
            CheckMetadata {
                .severity = HWCheckType::gcode_compatibility,
                .title = N_("G-Code compatibility mode"),
                .description = N_("G-Code is sliced for a different, but compatible printer model."),
            },
        },
#endif
        {
            GeneralCheck::gcode_level,
            CheckMetadata {
                .severity = HWCheckType::gcode_level,
                .title = N_("G-Code version mismatch"),
                .description = nullptr, // Feel free to fill this in when you figure what this error means
            },
        },
        {
            GeneralCheck::minimum_fw_version,
            CheckMetadata {
                .severity = HWCheckType::firmware,
                .title = N_("Firmware update required"),
                .description = N_("G-Code requires features from a newer firmare version to function properly."),
            },
        },
        {
            GeneralCheck::input_shaper,
            CheckMetadata {
                .severity = HWCheckType::input_shaper,
                .title = N_("Not sliced for Input Shaping"),
                .description = N_("G-Code is not sliced with Input Shaping support. Slicing with IS significantly shortens printing time."),
            },
        },
#if HAS_INDX()
        {
            GeneralCheck::indx_lock,
            CheckMetadata {
                .severity = HWCheckSeverity::Warning,
                .title = N_("Sliced with old INDX profiles"),
                .description = N_("G-Code is not sliced with newest INDX profiles. There will be some issues with the print."),
            },
        },
#endif
#if HAS_MMU2()
        {
            GeneralCheck::mmu,
            CheckMetadata {
                .severity = HWCheckType::firmware,
                .title = N_("Sliced for MMU"),
                .description = N_("G-Code is sliced for MMU, but the MMU is not enabled on the printer. Cannot print."),
            },
        },
#endif
        {
            GeneralCheck::unsupported_features,
            CheckMetadata {
                .severity = HWCheckType::firmware,
                .title = N_("Unsupported features"),
                .description = N_("G-Code requires some features the printer does not have."),
            },
        },
        {
            GeneralCheck::not_enough_tools,
            CheckMetadata {
                .severity = HWCheckSeverity::Abort,
                .title = N_("Not enough tools"),
                .description = N_("G-Code requires more tools than are currently enabled."),
            },
        },
#if HAS_INDX()
        {
            GeneralCheck::nozzle_cleaner_not_calibrated,
            CheckMetadata {
                .severity = HWCheckSeverity::Abort,
                .title = N_("Nozzle cleaner not calibrated"),
                .description = N_("Please calibrate the nozzle cleaner first."),
            },
        },
#endif
};

template <>
constinit const ChecksTraits<VirtualToolCheck>::Metadata ChecksTraits<VirtualToolCheck>::metadata {
    {
        VirtualToolCheck::correct_tool,
        CheckMetadata {
            .severity = HWCheckSeverity::Abort,
            .title = N_("Wrong tool"),
            .description = N_("Tool is disabled or of a different type."),
        },
    },
        {
            VirtualToolCheck::nozzle_diameter,
            CheckMetadata {
                .severity = HWCheckType::nozzle,
                .title = N_("Wrong nozzle diameter"),
                .description = N_("G-Code is sliced for a different tool diameter."),
            },
        },
        {
            VirtualToolCheck::nozzle_hardened,
            CheckMetadata {
                .severity = HWCheckType::nozzle,
                .title = N_("Nozzle not hardened"),
                .description = N_("G-Code is sliced for a hardened nozzle. Non-hardened nozzles can get damaged due to material abrasivity."),
            },
        },
        {
            VirtualToolCheck::nozzle_high_flow,
            CheckMetadata {
                .severity = HWCheckType::nozzle,
                .title = N_("Nozzle not high-flow"),
                .description = N_("G-Code is sliced for a high-flow nozzle. Printing with standard nozzle can lead to underextrusion and extruder skipping."),
            },
        },
        {
            VirtualToolCheck::nozzle_not_high_flow,
            CheckMetadata {
                .severity = HWCheckType::nozzle,
                .title = N_("Nozzle high-flow mismatch"),
                .description = N_("G-Code is sliced for a non-high flow nozzle. High-flow nozzles require more purging, so printing a MMU print with a high-flow nozzle can result in a color creep."),
            },
        },
        {
            VirtualToolCheck::filament_loaded,
            CheckMetadata {
                .severity = HWCheckSeverity::Warning,
                .title = N_("Filament not loaded"),
                .description = N_("One of the used tools does not have a filament loaded. Filament sensors need to be disabled for the print."),
            },
        },
        {
            VirtualToolCheck::filament_type,
            CheckMetadata {
                .severity = HWCheckSeverity::Warning,
                .title = N_("Wrong filament type"),
                .description = N_("G-Code is sliced for a different filament type than what is currently loaded in the assigned tool."),
            },
        },
#if HAS_SPOOL_JOIN()
        {
            VirtualToolCheck::can_spool_join,
            CheckMetadata {
                .severity = HWCheckSeverity::Abort,
                .title = N_("Spool join sensor not ready"),
                .description = N_("Filament sensor required to trigger the spool join is either disabled or not calibrated."),
            },
        },
#endif
#if HAS_INDX()
        {
            VirtualToolCheck::filament_calibrated,
            CheckMetadata {
                .severity = HWCheckSeverity::Abort,
                .title = N_("Filament not calibrated"),
                .description = N_("Filament is missing calibration data. Unload it and load again to run the calibration."),
            },
        },
#endif
#if HAS_TOOLCHANGER()
        {
            VirtualToolCheck::dock_position_calibrated,
            CheckMetadata {
                .severity = HWCheckSeverity::Abort,
                .title = N_("Dock position not calibrated"),
                .description = N_("Dock position calibration needs to be performed before the tool can be used."),
            },
        },
#endif
};

template <>
constinit const ChecksTraits<GCodeToolCheck>::Metadata ChecksTraits<GCodeToolCheck>::metadata {
    {
        GCodeToolCheck::tool_assigned,
        CheckMetadata {
            .severity = HWCheckSeverity::Abort,
            .title = N_("Unmapped tool"),
            .description = N_("G-Code tool is not mapped."),
        },
    },
#if HAS_ANFC()
        {
            GCodeToolCheck::enough_filament,
            CheckMetadata {
                .severity = HWCheckSeverity::Warning, // TODO: Sanitize HWCheckType and use it here as well
                .title = N_("Not enough filament"),
                .description = N_("There is not enough filament remaining on the spools to print the model."),
            },
        },
#endif
};

namespace buddy::gcode_compatibility {

bool CompatibilityReport::visit_failed_checks(const FailedCheckVisitor &visitor, AggregateTools aggregate_tools) const {
    const auto visit_bitset = [&]<typename Check>(const ChecksTraits<Check>::Bitset &bitset, FailedCheck::Tool tool) {
        const auto v = [&](const CheckMetadata &meta) {
            return visitor(FailedCheck {
                .meta = &meta,
                .tool = tool,
                .is_from_filament = false,
            });
        };
        return ChecksTraits<Check>::visit_set_bits(bitset, v);
    };

    if (!visit_bitset.operator()<GeneralCheck>(failed_general_checks, NoTool {})) {
        return false;
    }

    if (aggregate_tools == AggregateTools::yes) {
        {
            ChecksTraits<VirtualToolCheck>::Bitset failed_bits {};
            filament_compatibility::CompatibilityReport filament_report;

            const auto filament_visitor = [&](const filament_compatibility::CompatibilityReport::FailedCheck &check) {
                return visitor(FailedCheck {
                    .meta = check.meta,
                    .tool = NoTool {},
                    .is_from_filament = true,
                });
            };

            for (VirtualToolIndex tool : VirtualToolIndex::all()) {
                failed_bits |= failed_virtual_tool_checks[tool];
                filament_report |= filament_check_reports[tool];
            }

            if (!visit_bitset.operator()<VirtualToolCheck>(failed_bits, NoTool {})) {
                return false;
            }
            if (!filament_report.visit_failed_checks(filament_visitor)) {
                return false;
            }
        }
        {
            ChecksTraits<GCodeToolCheck>::Bitset failed_bits {};
            for (GcodeToolIndex tool : GcodeToolIndex::all()) {
                failed_bits |= failed_gcode_tool_checks[tool];
            }
            if (!visit_bitset.operator()<GCodeToolCheck>(failed_bits, NoTool {})) {
                return false;
            }
        }

    } else {
        for (VirtualToolIndex tool : VirtualToolIndex::all()) {
            if (!visit_bitset.operator()<VirtualToolCheck>(failed_virtual_tool_checks[tool], tool)) {
                return false;
            }

            const auto filament_visitor = [&](const filament_compatibility::CompatibilityReport::FailedCheck &check) {
                return visitor(FailedCheck {
                    .meta = check.meta,
                    .tool = tool,
                    .is_from_filament = true,
                });
            };
            if (!filament_check_reports[tool].visit_failed_checks(filament_visitor)) {
                return false;
            }
        }
        for (GcodeToolIndex tool : GcodeToolIndex::all()) {
            if (!visit_bitset.operator()<GCodeToolCheck>(failed_gcode_tool_checks[tool], tool)) {
                return false;
            }
        }
    }

    return true;
}

const GCodeInfo &CompatibilityReport::default_gcode_info() {
    return GCodeInfo::getInstance();
}

#if HAS_TOOL_MAPPING()
const ToolMapper &CompatibilityReport::default_tool_mapper() {
    return tool_mapper;
}
#endif

#if HAS_SPOOL_JOIN()
const SpoolJoin &CompatibilityReport::default_spool_join() {
    return spool_join;
}
#endif

void CompatibilityReport::generate_without_toolmapping(const GCodeInfo &gcode_info) {
    *this = {};

    failed_general_checks |= gcode_info.info().failed_gcode_checks;

    if (!gcode_info.info().sliced_with_input_shaper_ && !PRINTER_IS_PRUSA_iX()) {
        failed_general_checks.set(GeneralCheck::input_shaper);
    }

#if HAS_INDX()
    if (!gcode_info.info().sliced_with_indx_lock_) {
        failed_general_checks.set(GeneralCheck::indx_lock);
    }
#endif

    if (gcode_info.UsedExtrudersCount() > get_num_of_enabled_tools()) {
        failed_general_checks.set(GeneralCheck::not_enough_tools);
    }

#if HAS_INDX()
    if (config_store().selftest_result_nozzle_cleaner_calibration.get() != TestResult::passed) {
        failed_general_checks.set(GeneralCheck::nozzle_cleaner_not_calibrated);
    }
#endif
}

void CompatibilityReport::generate_full(const ToolMappingArgs &args) {
    generate_without_toolmapping(args.gcode_info);
    generate_toolmapping_only_noclear(args);
}

void CompatibilityReport::generate_toolmapping_only(const ToolMappingArgs &args) {
    *this = {};
    generate_toolmapping_only_noclear(args);
}

void CompatibilityReport::generate_toolmapping_only_noclear([[maybe_unused]] const ToolMappingArgs &args) {
    const auto &gcode_info = GCodeInfo::getInstance();

#if HAS_MMU2()
    const bool mmu_enabled = MMU2::mmu2.Enabled();
#else
    const bool mmu_enabled = false;
#endif

    // Do NOT skip disabled GCodeTools - these need to fail on GCodeToolCheck::tool_assigned if they are used
    for (GcodeToolIndex gcode_tool : GcodeToolIndex::all()) {
        auto &gcode_tool_fails = failed_gcode_tool_checks[gcode_tool];

        const auto &extruder_info = gcode_info.get_extruder_info(gcode_tool);
        if (!extruder_info.used()) {
            continue;
        }

#if HAS_TOOL_MAPPING()
        const auto base_virtual_tool_opt = gcode_tool.to_virtual(args.tool_mapper);
#else
        const auto base_virtual_tool_opt = gcode_tool.to_virtual();
#endif

        if (!std::holds_alternative<VirtualToolIndex>(base_virtual_tool_opt)) {
            gcode_tool_fails.set(GCodeToolCheck::tool_assigned);
            continue;
        }
        const VirtualToolIndex base_virtual_tool = std::get<VirtualToolIndex>(base_virtual_tool_opt);

#if HAS_MMU2()
        // Make sure that MMU gcode is sliced with the correct nozzle.
        // Slicing with a non-HF nozzle while HF nozzle is installed results in unsufficient purging.
        // Slicing for a HF nozzle without having it leads to extruder skipping.
        // Note: Always checking first bit in the config store, since nozzle_is_high_flow is set per toolhead and MMU always uses first one.
        if (extruder_info.requires_high_flow_nozzle == Tristate::yes
            && !config_store().get_nozzle_is_high_flow(0)
            && !gcode_info.is_singletool_gcode()
            && mmu_enabled) {
            failed_virtual_tool_checks[base_virtual_tool].set(VirtualToolCheck::nozzle_not_high_flow);
        }
#endif

#if HAS_ANFC()
        /// Sums remaining filament across the whole spool join chain
        /// Is set to NAN if any of the spooljoined tools does not have an OPT assigned
        float remaining_filament_g = 0;
#endif

        const auto virtual_tool_check = [&](VirtualToolIndex virtual_tool, [[maybe_unused]] std::optional<VirtualToolIndex> previous_virtual_tool) {
            auto &virtual_tool_fails = failed_virtual_tool_checks[virtual_tool];

            const PhysicalToolIndex physical_tool = virtual_tool.to_physical();

            if (!physical_tool.is_enabled()) {
                virtual_tool_fails.set(VirtualToolCheck::correct_tool);
                return;
            }

#if HAS_TOOLCHANGER()
            if (prusa_toolchanger.is_toolchanger_enabled() && !prusa_toolchanger.is_tool_info_valid(physical_tool)) {
                virtual_tool_fails.set(VirtualToolCheck::dock_position_calibrated);
            }
#endif

            const FilamentType loaded_filament_type = config_store().get_filament_type(virtual_tool);
            const FilamentTypeParameters loaded_filament_params = loaded_filament_type.parameters();

            // Check filament type and hotend compatibility
            // Don't report errors if the gcode did not provide the filament type
            if (const auto &fn = extruder_info.filament_name; !fn.empty() && fn != "---") {
                if (fn != loaded_filament_params.name) {
                    virtual_tool_fails.set(VirtualToolCheck::filament_type);
                }

                const auto gcode_filament = FilamentType::from_name(std::string_view(fn.data()));
                if (gcode_filament != FilamentType::none) {
                    const filament_compatibility::CompatibilityReportGenerateArgs args {
                        .filament = gcode_filament.parameters(),
                        .tools = virtual_tool,
                        .assume_filament_already_inserted = true,
                    };
                    filament_check_reports[virtual_tool].generate_noclear(args);
                }
            }

            if (auto dia = extruder_info.nozzle_diameter; dia.has_value() && std::abs(*dia - config_store().get_nozzle_diameter(physical_tool.to_raw())) > 0.001f) {
                virtual_tool_fails.set(VirtualToolCheck::nozzle_diameter);
            }
            if (extruder_info.requires_hardened_nozzle == Tristate::yes && !config_store().get_nozzle_is_hardened(physical_tool)
                // These errors are +- duplicit, prevent showing the both
                && !filament_check_reports[virtual_tool].failed_tool_checks.test(buddy::filament_compatibility::ToolCheck::abrasive)) {
                virtual_tool_fails.set(VirtualToolCheck::nozzle_hardened);
            }
            if (extruder_info.requires_high_flow_nozzle == Tristate::yes && !config_store().get_nozzle_is_high_flow(physical_tool)) {
                virtual_tool_fails.set(VirtualToolCheck::nozzle_high_flow);
            }

            // With MMU, the filaments are intentionally unloaded at the start of the print
            if (!mmu_enabled) {
                if (auto fs = GetExtruderFSensor(physical_tool); fs && fs->get_state() == FilamentSensorState::NoFilament) {
                    virtual_tool_fails.set(VirtualToolCheck::filament_loaded);
                }
                if (auto fs = GetSideFSensor(physical_tool); !mmu_enabled && fs && fs->get_state() == FilamentSensorState::NoFilament) {
                    virtual_tool_fails.set(VirtualToolCheck::filament_loaded);
                }
            }

#if HAS_ANFC()
            if (auto v = buddy::openprinttag::filament_usage_tracker().remaining_filament_g(virtual_tool)) {
                remaining_filament_g += *v;
            } else {
                // At least one of the spools does not have an OPT
                // We cannot rely on the remaining filament calculations
                remaining_filament_g = NAN;
            }
#endif

#if HAS_INDX()
// TODO VirtualToolCheck::filament_calibrated
#endif

#if HAS_SPOOL_JOIN()
            // Spooljoin related checks
            if (previous_virtual_tool.has_value()) {
                does_spool_join = true;

                const auto previous_physical_tool = previous_virtual_tool->to_physical();
                if (!is_fsensor_working_state(GetExtruderFSensor(previous_physical_tool)) && !is_fsensor_working_state(GetSideFSensor(previous_physical_tool))) {
                    virtual_tool_fails.set(VirtualToolCheck::can_spool_join);
                }
            }
#endif
        };

#if HAS_SPOOL_JOIN()
        {
            std::optional<VirtualToolIndex> previous_tool;
            for (std::optional<VirtualToolIndex> tool = base_virtual_tool; tool.has_value();) {
                virtual_tool_check(*tool, previous_tool);
                previous_tool = tool;
                tool = args.spool_join.get_spool_2(*tool);
            }
        }
#else
        virtual_tool_check(base_virtual_tool, std::nullopt);
#endif

#if HAS_ANFC()
        // Note: Comparing NAN to anything is always FALSE, so if either of the values is unknown, we do not fail this check
        // We want to fail this check only when we are certain that we don't have enough filament
        if (extruder_info.filament_used_g.value_or(NAN) > remaining_filament_g) {
            gcode_tool_fails.set(GCodeToolCheck::enough_filament);
        }
#endif
    }
}

[[nodiscard]] bool CompatibilityReport::gui_confirm_incompatibility(const FailedCheck &check, Response abort_response) const {
    if (check.is_from_filament) {
        const filament_compatibility::CompatibilityReport::FailedCheck filament_check {
            .meta = check.meta,
        };
        return filament_compatibility::CompatibilityReport::gui_confirm_incompatibility(filament_check, abort_response);
    }

    // Special case for the filament loaded check - continuing that one requires disabling the filament sensors
    else if (check.meta == &ChecksTraits<VirtualToolCheck>::metadata[VirtualToolCheck::filament_loaded]) {
        if (MsgBoxWarning(_(check.meta->description), { abort_response, Response::FS_disable }) == Response::FS_disable) {
#if HAS_SPOOL_JOIN()
            if (does_spool_join) {
                MsgBoxError(_("Cannot disable filament sensors, because they are required for spool join."), { abort_response });
                return false;
            }
#endif
            FSensors_instance().set_enabled_global(false);
            return true;
        } else {
            return false;
        }
    }

    else {
        return gui_confirm_incompatibility_default(*check.meta, abort_response);
    }
}

} // namespace buddy::gcode_compatibility
