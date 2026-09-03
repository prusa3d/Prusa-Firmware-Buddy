#include "tool_index.hpp"
#include <printers.h>
#include <module/motion.h>
#include <utils/overloaded_visitor.hpp>

#include <option/has_tool_mapping.h>
#if HAS_TOOL_MAPPING()
    #include <module/prusa/tool_mapper.hpp>
#endif

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include <feature/prusa/MMU2/mmu2_mk4.h>
#endif

#include <option/board_is_master_board.h>
#if BOARD_IS_MASTER_BOARD()
    #include <config_store/store_instance.hpp>
    #include <string_builder.hpp>
#endif

using VirtualExtension = VirtualToolIndexExtension;
using PhysicalExtension = PhysicalToolIndexExtension;
using GcodeExtension = GcodeToolIndexExtension;

static_assert(sizeof(PhysicalToolIndex) == 1);
static_assert(sizeof(VirtualToolIndex) == 1);
static_assert(sizeof(GcodeToolIndex) == 1);

static_assert(std::input_iterator<VirtualToolIndex::Iterator>);
static_assert(std::input_iterator<PhysicalToolIndex::Iterator>);
static_assert(std::input_iterator<GcodeToolIndex::Iterator>);

PhysicalToolIndex VirtualExtension::to_physical() const {
    [[maybe_unused]] const auto &self = static_cast<const VirtualToolIndex &>(*this);

#if HAS_TOOLCHANGER()
    static_assert(PhysicalToolIndex::count == VirtualToolIndex::count);
    return PhysicalToolIndex::from_raw(self.to_raw());
#else
    static_assert(PhysicalToolIndex::count == 1);
    return PhysicalToolIndex::from_raw(0);
#endif
}

#if HAS_TOOL_MAPPING()
std::variant<VirtualToolIndex, ToolNotMapped> GcodeExtension::to_virtual(const ToolMapper &tool_mapper) const {
    [[maybe_unused]] const auto &self = static_cast<const GcodeToolIndex &>(*this);
    return tool_mapper.to_virtual(self);
}

std::variant<VirtualToolIndex, ToolNotMapped> GcodeExtension::to_virtual() const {
    return to_virtual(tool_mapper);
}
#else
std::variant<VirtualToolIndex, ToolNotMapped> GcodeExtension::to_virtual() const {
    const auto &self = static_cast<const GcodeToolIndex &>(*this);
    return VirtualToolIndex::from_raw(self.to_raw());
}
#endif

std::variant<PhysicalToolIndex, ToolNotMapped> GcodeExtension::to_physical() const {
    return to_physical_tool_index<ToolNotMapped>(to_virtual());
}

template <>
bool PhysicalToolIndex::is_enabled() const {
    [[maybe_unused]] const auto &self = static_cast<const PhysicalToolIndex &>(*this);

#if HAS_TOOLCHANGER()
    return prusa_toolchanger.is_tool_enabled(self, {});
#else
    static_assert(PhysicalToolIndex::count == 1);
    return true;
#endif
}

template <>
bool VirtualToolIndex::is_enabled() const {
    [[maybe_unused]] const auto &self = static_cast<const VirtualToolIndex &>(*this);

#if HAS_TOOLCHANGER()
    return self.to_physical().is_enabled();

#elif HAS_MMU2()
    static_assert(PhysicalToolIndex::count == 1);
    if (config_store().mmu2_enabled.get()) {
        // MMU has five slots, the virtual tool count should match
        static_assert(VirtualToolIndex::count == 5);

        // MMU is enabled - we have access to all virtual tools and can toolchange between them
        return true;

    } else {
        // Without the MMU, we only have one virtual tool available (there's nothing that would do the toolchange)
        return self.to_raw() == 0;
    }
#else
    // Single nozzle, single slot - nothing to get disabled
    static_assert(PhysicalToolIndex::count == 1);
    static_assert(VirtualToolIndex::count == 1);
    return true;
#endif
}

template <>
bool GcodeToolIndex::is_enabled() const {
    const auto &self = static_cast<const GcodeToolIndex &>(*this);

    return match(
        self.to_virtual(), //
        [](VirtualToolIndex t) { return t.is_enabled(); }, //
        [](ToolNotMapped) { return false; });
}

std::variant<PhysicalToolIndex, NoTool> PhysicalExtension::currently_selected() {
#if HAS_TOOLCHANGER()
    static_assert(!HAS_MMU2());
    static_assert(PhysicalToolIndex::count == PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
    const uint8_t extruder = active_extruder.load();
    if (extruder == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        return NoTool {};
    } else {
        // Toolchanger uses active_extruder for determining tools
        static_assert(PhysicalToolIndex::count == VirtualToolIndex::count);
        return PhysicalToolIndex::from_raw(extruder);
    }
#else
    // Single tool, no toolchanber, cannot unpick it
    static_assert(PhysicalToolIndex::count == 1);
    return PhysicalToolIndex::from_raw(0);
#endif
}

std::variant<VirtualToolIndex, NoTool> PhysicalExtension::currently_selected_virtual_tool() const {
    const auto &self = static_cast<const PhysicalToolIndex &>(*this);

#if HAS_MMU2()
    static_assert(PhysicalToolIndex::count == 1);
    if (MMU2::mmu2.Enabled()) {
        const auto e = MMU2::mmu2.get_current_tool();
        if (e == MMU2::FILAMENT_UNKNOWN) {
            return NoTool {};
        } else {
            return VirtualToolIndex::from_raw(e);
        }
    }
#else
    static_assert(PhysicalToolIndex::count == VirtualToolIndex::count);
#endif

    return VirtualToolIndex::from_raw(self.to_raw());
}

std::variant<VirtualToolIndex, NoTool> VirtualExtension::currently_selected() {
    using Result = std::variant<VirtualToolIndex, NoTool>;
    return match(
        PhysicalToolIndex::currently_selected(), //
        [](PhysicalToolIndex t) -> Result { return t.currently_selected_virtual_tool(); }, //
        [](NoTool) -> Result { return NoTool {}; } //
    );
}

std::variant<PhysicalToolIndex, NoTool> PhysicalExtension::from_raw_notool(uint8_t index) {
#if HAS_TOOLCHANGER() && BOARD_IS_MASTER_BOARD()
    // count on XL is set to EXTRUDERS - 1 / HOTENDS - 1,
    // because of legacy definition of EXTRUDERS / HOTENDS to 6 instead of 5.
    // Values 0 to 4 are actual tools, 5 represents no tool (using MARLIN_NO_TOOL_PICKED).
    // If we encounter MARLIN_NO_TOOL_PICKED we need to return NoTool and check for it on call site.
    static_assert(PhysicalToolIndex::count == PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
    if (index == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        return NoTool {};
    }
#endif
    // Other non-tool values treat as invalid values -> raise bsod
    return PhysicalToolIndex::from_raw(index);
}

std::variant<VirtualToolIndex, NoTool> VirtualExtension::from_raw_notool(uint8_t index) {
#if HAS_TOOL_MAPPING()
    #if HAS_TOOLCHANGER()
    // count on XL is set to EXTRUDERS - 1 / HOTENDS - 1,
    // because of legacy definition of EXTRUDERS / HOTENDS to 6 instead of 5.
    // Values 0 to 4 are actual tools, 5 represents no tool (using MARLIN_NO_TOOL_PICKED).
    // If we encounter MARLIN_NO_TOOL_PICKED we need to return NoTool and check for it on call site.
    static_assert(VirtualToolIndex::count == PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
    if (index == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        return NoTool {};
    }
    #endif

    // There is 255 (NO_TOOL_MAPPED) used as no tool at some places.
    if (index == ToolMapper::NO_TOOL_MAPPED) {
        return NoTool {};
    }
#endif
    // Other non-tool values treat as invalid values -> raise bsod
    return VirtualToolIndex::from_raw(index);
}

#if BOARD_IS_MASTER_BOARD()

template <>
string_view_utf8 PhysicalToolIndex::display_name(StringViewUtf8ParamBase &params) const {
    return _("Tool %i").formatted(params, display_index());
}

template <>
string_view_utf8 VirtualToolIndex::display_name(StringViewUtf8ParamBase &params) const {
    if constexpr (PhysicalToolIndex::count == VirtualToolIndex::count) {
        // For multi-tool printers with 1:1 mapping between virtual and physica tools, introducing some extra slots/indexing would just confuse the users.
        // So in that case, refer to the virtual tools same as to the physical ones, because they're are 1:1.
        return to_physical().display_name(params);
    } else {
        return _("Filament Slot %i").formatted(params, display_index());
    }
}

template <>
string_view_utf8 GcodeToolIndex::display_name(StringViewUtf8ParamBase &params) const {
    return _("GCode Tool %i").formatted(params, display_index());
}

static void build_physical_details(StringBuilder &sb, const PhysicalToolIndex &self) {
    sb.append_float(
        config_store().get_nozzle_diameter(self),
        {
            .max_decimal_places = 2,
            .skip_zero_before_dot = true,
        });
    sb.append_string(" mm");

    if (config_store().get_nozzle_is_hardened(self)) {
        sb.append_string(" H");
    }
    if (config_store().get_nozzle_is_high_flow(self.to_raw())) {
        sb.append_string(" HF");
    }
}

static void build_virtual_details(StringBuilder &sb, const VirtualToolIndex &self) {
    // Display currently loaded filament
    if (sb.byte_count() > 0) {
        sb.append_char(' ');
    }
    sb.append_string(config_store().get_filament_type(self).parameters().name.data());
}

void PhysicalToolIndexExtension::build_details(StringBuilder &sb) const {
    build_physical_details(sb, static_cast<const PhysicalToolIndex &>(*this));

    // If the Physical-Virtual mapping is 1:1, show also virtual tool properties
    if constexpr (PhysicalToolIndex::count == VirtualToolIndex::count) {
        build_virtual_details(sb, std::get<VirtualToolIndex>(currently_selected_virtual_tool()));
    }
}

void VirtualToolIndexExtension::build_details(StringBuilder &sb) const {
    const auto &self = static_cast<const VirtualToolIndex &>(*this);

    // Discern physical properties if we have multiple physical tools
    if constexpr (PhysicalToolIndex::count > 1) {
        build_physical_details(sb, to_physical());
    }

    build_virtual_details(sb, self);
}

#endif
