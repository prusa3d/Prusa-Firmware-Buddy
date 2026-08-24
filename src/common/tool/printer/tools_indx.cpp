/// @file

#include <tool/hotend/hotend/indx_hotend.hpp>
#include <utils/storage/strong_index_array.hpp>

using INDXTool = StandardFFFPhysicalTool<IndxHotend>;

INDXTool &IndxHotend::indx_tool(PhysicalToolIndex tool) {
    static const IndxHotend::Config hotend_config {
        .min_nozzle_temp = 5,
        .max_nozzle_temp = 305,
    };
    static StrongIndexArray<INDXTool, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> tools {
        INDXTool(PhysicalToolIndex::from_raw(0), &hotend_config),
        INDXTool(PhysicalToolIndex::from_raw(1), &hotend_config),
        INDXTool(PhysicalToolIndex::from_raw(2), &hotend_config),
        INDXTool(PhysicalToolIndex::from_raw(3), &hotend_config),
        INDXTool(PhysicalToolIndex::from_raw(4), &hotend_config),
        INDXTool(PhysicalToolIndex::from_raw(5), &hotend_config),
        INDXTool(PhysicalToolIndex::from_raw(6), &hotend_config),
        INDXTool(PhysicalToolIndex::from_raw(7), &hotend_config),
    };
    static_assert(PhysicalToolIndex::count == 8);
    return tools[tool];
}

PhysicalTool &PhysicalTool::for_index(PhysicalToolIndex tool) {
    return IndxHotend::indx_tool(tool);
}
