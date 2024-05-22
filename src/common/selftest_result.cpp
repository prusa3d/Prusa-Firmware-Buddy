#include "selftest_result.hpp"

bool operator==(SelftestTool_pre_23 lhs, SelftestTool_pre_23 rhs) {
    return lhs.dockoffset == rhs.dockoffset
        && lhs.fsensor == rhs.fsensor
        && lhs.sideFsensor == rhs.sideFsensor
        && lhs.heatBreakFan == rhs.heatBreakFan
        && lhs.printFan == rhs.printFan
        && lhs.loadcell == rhs.loadcell
        && lhs.nozzle == rhs.nozzle
        && lhs.tooloffset == rhs.tooloffset;
}

bool operator==(SelftestResult_pre_23 lhs, SelftestResult_pre_23 rhs) {
    for (size_t i = 0; i < std::size(lhs.tools); ++i) {
        if (lhs.tools[i] != rhs.tools[i]) {
            return false;
        }
    }
    return lhs.bed == rhs.bed
        && lhs.eth == rhs.eth
        && lhs.wifi == rhs.wifi
        && lhs.xaxis == rhs.xaxis
        && lhs.yaxis == rhs.yaxis
        && lhs.zaxis == rhs.zaxis
        && lhs.zalign == rhs.zalign;
}

SelftestResult_pre_gears::SelftestResult_pre_gears(const SelftestResult_pre_23 &sr_pre23)
    : xaxis(sr_pre23.xaxis)
    , yaxis(sr_pre23.yaxis)
    , zaxis(sr_pre23.zaxis)
    , bed(sr_pre23.bed)
    , eth(sr_pre23.eth)
    , wifi(sr_pre23.wifi)
    , zalign(sr_pre23.zalign) {
    for (size_t i = 0; i < std::size(tools); ++i) {
        tools[i].printFan = sr_pre23.tools[i].printFan;
        tools[i].heatBreakFan = sr_pre23.tools[i].heatBreakFan;
        // fansSwitched left defaulted
        tools[i].nozzle = sr_pre23.tools[i].nozzle;
        tools[i].fsensor = sr_pre23.tools[i].fsensor;
        tools[i].loadcell = sr_pre23.tools[i].loadcell;
        tools[i].sideFsensor = sr_pre23.tools[i].sideFsensor;
        tools[i].dockoffset = sr_pre23.tools[i].dockoffset;
        tools[i].tooloffset = sr_pre23.tools[i].tooloffset;
    }
}

bool operator==(SelftestTool lhs, SelftestTool rhs) {
    return lhs.dockoffset == rhs.dockoffset
        && lhs.fsensor == rhs.fsensor
        && lhs.sideFsensor == rhs.sideFsensor
        && lhs.heatBreakFan == rhs.heatBreakFan
        && lhs.fansSwitched == rhs.fansSwitched
        && lhs.printFan == rhs.printFan
        && lhs.loadcell == rhs.loadcell
        && lhs.nozzle == rhs.nozzle
        && lhs.tooloffset == rhs.tooloffset;
}

bool operator==(SelftestResult_pre_gears lhs, SelftestResult_pre_gears rhs) {
    for (size_t i = 0; i < std::size(lhs.tools); ++i) {
        if (lhs.tools[i] != rhs.tools[i]) {
            return false;
        }
    }
    return lhs.bed == rhs.bed
        && lhs.eth == rhs.eth
        && lhs.wifi == rhs.wifi
        && lhs.xaxis == rhs.xaxis
        && lhs.yaxis == rhs.yaxis
        && lhs.zaxis == rhs.zaxis
        && lhs.zalign == rhs.zalign;
}

SelftestResult::SelftestResult(const SelftestResult_pre_gears &sr_pre_gears)
    : xaxis(sr_pre_gears.xaxis)
    , yaxis(sr_pre_gears.yaxis)
    , zaxis(sr_pre_gears.zaxis)
    , bed(sr_pre_gears.bed)
    , eth(sr_pre_gears.eth)
    , wifi(sr_pre_gears.wifi)
    , zalign(sr_pre_gears.zalign)
    , gears(TestResult::TestResult_Unknown) {
    for (size_t i = 0; i < std::size(tools); ++i) {
        tools[i].printFan = sr_pre_gears.tools[i].printFan;
        tools[i].heatBreakFan = sr_pre_gears.tools[i].heatBreakFan;
        tools[i].fansSwitched = sr_pre_gears.tools[i].fansSwitched;
        tools[i].nozzle = sr_pre_gears.tools[i].nozzle;
        tools[i].fsensor = sr_pre_gears.tools[i].fsensor;
        tools[i].loadcell = sr_pre_gears.tools[i].loadcell;
        tools[i].sideFsensor = sr_pre_gears.tools[i].sideFsensor;
        tools[i].dockoffset = sr_pre_gears.tools[i].dockoffset;
        tools[i].tooloffset = sr_pre_gears.tools[i].tooloffset;
    }
}

bool operator==(SelftestResult lhs, SelftestResult rhs) {
    for (size_t i = 0; i < std::size(lhs.tools); ++i) {
        if (lhs.tools[i] != rhs.tools[i]) {
            return false;
        }
    }
    return lhs.bed == rhs.bed
        && lhs.eth == rhs.eth
        && lhs.wifi == rhs.wifi
        && lhs.xaxis == rhs.xaxis
        && lhs.yaxis == rhs.yaxis
        && lhs.zaxis == rhs.zaxis
        && lhs.zalign == rhs.zalign
        && lhs.gears == rhs.gears;
}
