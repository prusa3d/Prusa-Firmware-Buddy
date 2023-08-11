#pragma once

#include "background.hpp"

namespace transfers {
class Download;
}

namespace connect_client {

class Printer;
class Planner;

// Just make the code a bit more readable by making this distinction.
// Unfortunately, not checked at compile time.
//
// Both are in milliseconds.
using Timestamp = uint32_t;
using Duration = uint32_t;

// Renamed ticks_ms for readability.
Timestamp now();

void sleep_raw(Duration sleep_for);

class Sleep {
private:
    // Non-owning.
    //
    // We would prefer optional<&T>, but that doesn't exist in C++.
    BackgroundCmd *background_cmd;
    transfers::Download *download;
    bool recover_download;

#ifdef UNITTESTS
public:
#else
private:
#endif
    Duration milliseconds;

public:
    Sleep(Duration duration, BackgroundCmd *cmd, transfers::Download *download, bool recover_download)
        : background_cmd(cmd)
        , download(download)
        , recover_download(recover_download)
        , milliseconds(duration) {}
    static Sleep idle();
    /// Sleeps up to the given time, processing any background tasks if possible.
    ///
    /// May terminate sooner if there's a reason to believe some situation has
    /// changed.
    void perform(Printer &printer, Planner &planner);
};

} // namespace connect_client
