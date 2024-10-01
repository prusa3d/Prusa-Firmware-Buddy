#pragma once

#include "status_page.h"

#include <http/types.h>

#include <array>
#include <string_view>

namespace nhttp::printer {

// TODO: Figure a way not to taint the server part with specific implementations

/**
 * \brief The "implementation" of JobCommand/POST to /api/job.
 */
class JobCommand {
private:
    /*
     * Our current json parser expects to get the whole input as one block of
     * memory. Until/unless we replace it with something streaming/incremental,
     * we just gather the data here.
     *
     * This should be enough for most of our needs in practice and we can
     * afford this much, because other handlers also contain ~100B buffer (and
     * we share the space).
     */
    static const constexpr size_t BUFFER_LEN = 100;
    std::array<uint8_t, BUFFER_LEN> buffer;
    size_t buffer_used = 0;
    size_t content_length;
    bool can_keep_alive;
    bool json_errors;

    handler::StatusPage process();

    /*
     * Implementation note: due to the fact these contain talking to
     * marlin, they live in a separate cpp file and are replaced in
     * tests.
     *
     * Do we know of nicer way to do this? Possibly some virtual
     * interface to the "printer" in the future...
     */
    bool stop();
    bool pause();
    bool resume();
    bool pause_toggle();

public:
    JobCommand(size_t content_length, bool can_keep_alive, bool json_errors);
    bool want_read() const { return true; }
    bool want_write() const { return false; }
    void step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size, handler::Step &out);
};

} // namespace nhttp::printer
