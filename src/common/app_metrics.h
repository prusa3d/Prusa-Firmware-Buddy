#pragma once

namespace Buddy {
namespace Metrics {
    void RecordMarlinVariables();
    void RecordRuntimeStats();
    void RecordPowerStats();
    void RecordPrintFilename();
    void record_dwarf_mcu_temperature();
}
}
