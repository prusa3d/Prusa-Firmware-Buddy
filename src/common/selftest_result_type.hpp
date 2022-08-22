/**
 * @file selftest_result_type.hpp
 * @author Radek Vana
 * @brief defines how is result selftest passed between threads
 * @date 2022-01-23
 */

#pragma once
#include "selftest_eeprom.hpp"
#include "fsm_base_types.hpp"
#include "selftest_log.hpp"

LOG_COMPONENT_REF(Selftest);

struct SelftestResult_t {
    TestResult_t printFan;
    TestResult_t heatBreakFan;
    TestResult_t xaxis;
    TestResult_t yaxis;
    TestResult_t zaxis;
    TestResult_t nozzle;
    TestResult_t bed;
    TestResultNet_t eth;
    TestResultNet_t wifi;

    constexpr SelftestResult_t(

        TestResult_t printFan = TestResult_t::Unknown,
        TestResult_t heatBreakFan = TestResult_t::Unknown,
        TestResult_t xaxis = TestResult_t::Unknown,
        TestResult_t yaxis = TestResult_t::Unknown,
        TestResult_t zaxis = TestResult_t::Unknown,
        TestResult_t nozzle = TestResult_t::Unknown,
        TestResult_t bed = TestResult_t::Unknown,
        TestResultNet_t eth = TestResultNet_t::Unknown,
        TestResultNet_t wifi = TestResultNet_t::Unknown)
        : printFan(printFan)
        , heatBreakFan(heatBreakFan)
        , xaxis(xaxis)
        , yaxis(yaxis)
        , zaxis(zaxis)
        , nozzle(nozzle)
        , bed(bed)
        , eth(eth)
        , wifi(wifi) {}

    constexpr SelftestResult_t(fsm::PhaseData new_data)
        : SelftestResult_t() {
        Deserialize(new_data);
    }

    constexpr SelftestResult_t(SelftestResultEEprom_t res)
        : SelftestResult_t() {
        Deserialize(res);
    }

    constexpr fsm::PhaseData Serialize() const {
        SelftestResultEEprom_t res = { 0 };
        res.printFan = uint8_t(printFan);
        res.heatBreakFan = uint8_t(heatBreakFan);
        res.xaxis = uint8_t(xaxis);
        res.yaxis = uint8_t(yaxis);
        res.zaxis = uint8_t(zaxis);
        res.nozzle = uint8_t(nozzle);
        res.bed = uint8_t(bed);
        res.eth = uint8_t(eth);
        res.wifi = uint8_t(wifi);

        fsm::PhaseData ret = { { res.arr[0], res.arr[1], res.arr[2], res.arr[3] } };
        return ret;
    }

    void Log() const {
        log_info(Selftest, "Print fan result is %s", ToString(printFan));
        log_info(Selftest, "Heatbreak fan result is %s", ToString(heatBreakFan));
        log_info(Selftest, "X axis result is %s", ToString(xaxis));
        log_info(Selftest, "Y axis result is %s", ToString(yaxis));
        log_info(Selftest, "Z axis result is %s", ToString(zaxis));
        log_info(Selftest, "Nozzle heater result is %s", ToString(nozzle));
        log_info(Selftest, "Bed heater result is %s", ToString(bed));
        log_info(Selftest, "Ethernet result is %s", ToString(eth));
        log_info(Selftest, "Wifi result is %s", ToString(wifi));
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        SelftestResultEEprom_t res = { 0 };
        res.arr[0] = new_data[0];
        res.arr[1] = new_data[1];
        res.arr[2] = new_data[2];
        res.arr[3] = new_data[3];
        Deserialize(res);
    }

    constexpr void Deserialize(SelftestResultEEprom_t res) {
        printFan = TestResult_t(res.printFan);
        heatBreakFan = TestResult_t(res.heatBreakFan);
        xaxis = TestResult_t(res.xaxis);
        yaxis = TestResult_t(res.yaxis);
        zaxis = TestResult_t(res.zaxis);
        nozzle = TestResult_t(res.nozzle);
        bed = TestResult_t(res.bed);
        eth = TestResultNet_t(res.eth);
        wifi = TestResultNet_t(res.wifi);
    }

    constexpr bool operator==(const SelftestResult_t &other) const {
        // operator == is not constexpr over std::array
        // must comapre each element
        return Serialize()[0] == other.Serialize()[0] && Serialize()[1] == other.Serialize()[1] && Serialize()[2] == other.Serialize()[2] && Serialize()[3] == other.Serialize()[3];
    }

    constexpr bool operator!=(const SelftestResult_t &other) const {
        return !((*this) == other);
    }

    // currently not affected by eth and wifi
    constexpr bool Passed() {
        if (printFan != TestResult_t::Passed)
            return false;
        if (heatBreakFan != TestResult_t::Passed)
            return false;
        if (xaxis != TestResult_t::Passed)
            return false;
        if (yaxis != TestResult_t::Passed)
            return false;
        if (zaxis != TestResult_t::Passed)
            return false;
        if (nozzle != TestResult_t::Passed)
            return false;
        if (bed != TestResult_t::Passed)
            return false;
        return true;
    }

    constexpr bool Failed() {
        if (printFan == TestResult_t::Failed)
            return true;
        if (heatBreakFan == TestResult_t::Failed)
            return true;
        if (xaxis == TestResult_t::Failed)
            return true;
        if (yaxis == TestResult_t::Failed)
            return true;
        if (zaxis == TestResult_t::Failed)
            return true;
        if (nozzle == TestResult_t::Failed)
            return true;
        if (bed == TestResult_t::Failed)
            return true;
        return false;
    }
};
