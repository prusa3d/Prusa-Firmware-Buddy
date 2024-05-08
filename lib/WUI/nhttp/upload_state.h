#pragma once

/**
 * \file
 * \brief Tracking the state of gcode upload and parsing whatever needed.
 *
 * Each new upload creates an instance, feeds it with data and checks if
 * everything goes fine (the errors can happen anytime and it is up to the
 * caller to either check after each feeding or at the end).
 *
 * Internally, it calls the hooks from the http handlers. The finish is called
 * if it succeeds; otherwise it simply terminates without calling finish.
 *
 * The data fed to it may be split arbitrarily.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string_view>
#include <memory>
#include <tuple>

#include "../../src/gui/file_list_defs.h"
#include <http/types.h>

struct multipart_parser;

namespace nhttp::printer {

class UploadHooks {
public:
    using Result = std::tuple<http::Status, const char *>;
    virtual ~UploadHooks() = default;
    virtual Result data(std::string_view data) = 0;
    virtual Result finish(const char *final_filename, bool start_print) = 0;
    // Early check of a filename validity.
    //
    // Used once the file name is known, possibly before all the data is
    // uploaded.
    virtual Result check_filename(const char *final_filename) const = 0;
};

} // namespace nhttp::printer
