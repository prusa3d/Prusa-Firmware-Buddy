#pragma once

#include "status_page.h"

// Why does FILE_PATH_BUFFER_LEN lives in *gui*!?
#include "../../src/gui/file_list_defs.h"

#include <http/types.h>

#include <array>
#include <string_view>

namespace nhttp::printer {

// TODO: Figure a way not to taint the server part with specific implementations

/**
 * \brief The "implementation" of file-path/POST.
 */
class FileCommand {
private:
    /*
     * Our current json parser expects to get the whole input as one block of
     * memory. Until/unless we replace it with something streaming/incremental,
     * we just gather the data here.
     */
    static const constexpr size_t BUFFER_LEN = 25;
    std::array<uint8_t, BUFFER_LEN> buffer;
    char filename[FILE_PATH_BUFFER_LEN];
    size_t buffer_used = 0;
    size_t content_length;
    bool can_keep_alive;
    bool json_errors;

    handler::StatusPage process();

    enum class StartResult {
        Started,
        NotReady,
        NotFound,
    };

    /*
     * Implementation note: due to the fact these contain talking to
     * marlin, they live in a separate cpp file and are replaced in
     * tests.
     *
     * Do we know of nicer way to do this? Possibly some virtual
     * interface to the "printer" in the future...
     */
    StartResult start();

public:
    FileCommand(const char *filename, size_t content_length, bool can_keep_alive, bool json_errors);
    bool want_read() const { return true; }
    bool want_write() const { return false; }
    void step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size, handler::Step &step);
};

} // namespace nhttp::printer
