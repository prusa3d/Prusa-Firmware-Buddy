#include "file_command.h"
#include "wui_api.h"

#include <unistd.h>
#include <cstring>
#include <string_view>

using std::string_view;

namespace nhttp::printer {

namespace {

    bool exists(const char *fname) {
        return access(fname, R_OK) == 0;
    }

} // namespace

FileCommand::StartResult FileCommand::start() {
    if (!exists(filename)) {
        // FIXME: Current prusa-link duplicates the initial usb for no good
        // reason. Temporary workaround.
        const string_view fname(filename);
        if (fname.find("/usb/usb/") == 0) {
            // Eat the first usb.
            const size_t len = strlen(filename);
            // Include copying of the \0 too
            memmove(filename, filename + 4, len - 3);
            // Try again with "fixed" filename.
            return start();
        }
        return StartResult::NotFound;
    }
    if (wui_start_print(filename, true) == StartPrintResult::PrintStarted) {
        return StartResult::Started;
    } else {
        return StartResult::NotReady;
    }
}

} // namespace nhttp::printer
