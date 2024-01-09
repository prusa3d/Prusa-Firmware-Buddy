#pragma once
#include <string>
#include <deque>
#include <vector>
#include <array>
#include "mmu2_fsensor.h"

std::string AppendCRCWrite(const std::string_view src);
std::string AppendCRC(const std::string_view src);

#define mockLog_RecordFn() mockLog.Record(mockLog.MethodName(__PRETTY_FUNCTION__))

/// mock log is a sequence of strings which are being recorded during the execution of various stubbed methods.
struct MockLog {
    // vector is not the best option, but due to some QtCreator bug displaying real content of this container is broken unless it's a plain array
    std::vector<std::string> log, expected;

    // log must match the msgs exactly
    bool Matches(std::initializer_list<std::string_view> msgs) const;
    bool MatchesExpected() const;
    void Record(std::string_view s);
    void Clear();

    void DeduplicateLog();
    void DeduplicateExpected();

    std::string InfoText();

    consteval std::string_view MethodName(const char *s) {
        std::string_view prettyFunction(s);
        // terminate the string_view with the first occurence of '('
        size_t bracket = prettyFunction.rfind("(");
        // find a space separating the function name from return type
        size_t space = prettyFunction.rfind(" ", bracket) + 1;
        // if there is a "MMU2::" namespace prefix, drop it as well
        if (prettyFunction.substr(space, bracket - space).starts_with("MMU2::")) {
            return prettyFunction.substr(space + 6, bracket - space - 6);
        } else {
            return prettyFunction.substr(space, bracket - space);
        }
    }
};
extern MockLog mockLog;

namespace MMU2 {
class MMU2Serial;
}

struct IOSimRec {
    /// Mock data that are "sent" to the MMU
    std::string tx;

    /// MMU logic functions log debugging data to the mock log.
    /// This field containts what mock data is expected to be added to the mock log by the logic while execution this sim rec.
    /// The check is not performed automatically, one has to call CHECK(mockLog.MatchesExpected());
    std::vector<std::string> mock;

    /// This is used for checking against expected marlin log, but the checks are currently commented out (?)
    std::vector<std::string> marlin;

    /// Mock data that are "received" from the MMU
    /// This data is appended to the mmu2SerialSim.rxbuff mmu rx buffer emulator
    std::string rx;

    uint32_t incMs;

    using WorkFunc = void (*)();
    WorkFunc work = nullptr;
};

using IOSim = std::vector<IOSimRec>;
extern IOSim::const_iterator ioSimI;
extern IOSim ioSim;
extern size_t ioSimMockCheckIndex;
void IOSimCheck();
void IOSimStart(std::initializer_list<IOSimRec> init);

struct MMU2SerialSim {
    std::string rxbuff;
    std::deque<std::string> txbuffQ;
    bool automagic = false; // true = automagically step and check rx/tx
    bool runIOSimOnNextRead = false;

    void SetRxBuffCRC(std::string_view s);
    bool TxBuffMatchesCRC(std::initializer_list<std::string_view> msgs) const {
        return std::equal(txbuffQ.begin(), txbuffQ.end(), msgs.begin(), [](auto a, auto b) { return a == AppendCRC(b); });
    }
    bool TxBuffMatchesCRC(std::string_view msg) const {
        if (txbuffQ.size() != 1) {
            return false;
        }
        return txbuffQ.back() == AppendCRC(msg);
    }
    void SetAutomagic() {
        automagic = true;
    }
};
extern MMU2SerialSim mmu2SerialSim;

struct MarlinLogSim {
    std::vector<std::string> log;

    void AppendLine(std::string s) {
        log.emplace_back(s);
    }
};
extern MarlinLogSim marlinLogSim;

namespace MMU2 {
extern FilamentState fs;
}

// stubbed external C-interfaces to play with
extern "C" {
// Beware - this call is intentionally not const as it is used to clock timeouts while retaining the non-blocking interface
// That's purely a simulation hook (hack) to trick the MMU classes
uint32_t millis(void);
void SetTimeoutCountdown(uint32_t tc);
// a simple way of playing with timeouts
void SetMillis(uint32_t m);
void IncMillis(uint32_t diff = 1);
} // extern "C"

void InitEnvironment(MMU2::FilamentState fsensor);

void SetMarlinIsPrinting(bool p);

void SetHotendTargetTemp(uint16_t t);
void SetHotendCurrentTemp(uint16_t t);
