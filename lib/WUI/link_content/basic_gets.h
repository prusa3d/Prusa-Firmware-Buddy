#pragma once

#include <segmented_json.h>

#define PL_VERSION_MAJOR    2
#define PL_VERSION_MINOR    0
#define PL_VERSION_REVISION 0

#define PL_VERSTR2(x) #x
#define PL_VERSTR(x)  PL_VERSTR2(x)

#define PL_VERSION_STRING       \
    PL_VERSTR(PL_VERSION_MAJOR) \
    "." PL_VERSTR(PL_VERSION_MINOR) "." PL_VERSTR(PL_VERSION_REVISION)

namespace nhttp::link_content {

// Implementation of the "simple" API endpoints.
//
// Note that this fills in the "stateless json" renderer. This cheats a bit, as
// this gets the flags (and other things) from marlin_vars at each call ‒ even
// after being resumed. This technically may produce inconsistent answer (if
// resumed in the middle of the flags and new ones are different). Doing it
// properly with state would currently be more work than it's worth (the
// inconsistent state is unlikely and not _that_ bad here).

json::JsonResult get_version(size_t resume_point, json::JsonOutput &output);
json::JsonResult get_printer(size_t resume_point, json::JsonOutput &output);
json::JsonResult get_job_octoprint(size_t resume_point, json::JsonOutput &output);
json::JsonResult get_job_v1(size_t resume_point, json::JsonOutput &output);
json::JsonResult get_storage(size_t resume_point, json::JsonOutput &output);
json::JsonResult get_info(size_t resume_point, json::JsonOutput &output);

} // namespace nhttp::link_content
