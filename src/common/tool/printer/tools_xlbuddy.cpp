/// @file

#include <tool/tool/standard_fff_physical_tool.hpp>
#include <tool/hotend/hotend/dwarf_hotend.hpp>
#include <utils/storage/strong_index_array.hpp>

PhysicalTool &PhysicalTool::for_index(PhysicalToolIndex index) {
    static constexpr DwarfHotend::Config hotend_config {
        .min_nozzle_temp = 5,
        .max_nozzle_temp = 305,
    };
    using Tool = StandardFFFPhysicalTool<DwarfHotend>;
    static StrongIndexArray<Tool, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> tools {
        Tool(PhysicalToolIndex::from_raw(0), &hotend_config),
        Tool(PhysicalToolIndex::from_raw(1), &hotend_config),
        Tool(PhysicalToolIndex::from_raw(2), &hotend_config),
        Tool(PhysicalToolIndex::from_raw(3), &hotend_config),
        Tool(PhysicalToolIndex::from_raw(4), &hotend_config),
    };
    static_assert(PhysicalToolIndex::count == 5);

    return tools[index];
}
