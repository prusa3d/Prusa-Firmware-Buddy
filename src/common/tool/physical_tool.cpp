/// @file
#include "physical_tool.hpp"

#include <option/has_toolchanger.h>
#include <bsod/bsod.h>
#if HAS_TOOLCHANGER()
    #include <tool/tool/dummy_physical_tool.hpp>
#endif

PhysicalTool::PhysicalTool(ToolType type, Hotend &hotend)
    : type_(type)
    , hotend_ref_(hotend) {
}

PhysicalTool &PhysicalTool::for_index(std::variant<PhysicalToolIndex, NoTool> tool_index) {
    return match(
        tool_index, //
        [](PhysicalToolIndex t) -> PhysicalTool & { return for_index(t); }, //
        [](NoTool) -> PhysicalTool & {
#if HAS_TOOLCHANGER()
            static DummyPhysicalTool dummy;
            return dummy;
#else
            bsod("notool");
#endif
        } //
    );
}
