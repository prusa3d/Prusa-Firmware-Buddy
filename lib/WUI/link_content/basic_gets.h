#pragma once

#include "nhttp/segmented_json.h"

#define PL_VERSION_MAJOR    2
#define PL_VERSION_MINOR    0
#define PL_VERSION_REVISION 0

#define PL_VERSTR2(x) #x
#define PL_VERSTR(x)  PL_VERSTR2(x)

#define PL_VERSION_STRING       \
    PL_VERSTR(PL_VERSION_MAJOR) \
    "." PL_VERSTR(PL_VERSION_MINOR) "." PL_VERSTR(PL_VERSION_REVISION)

namespace nhttp::link_content {

JsonRenderer::ContentResult get_version(size_t resume_point, JsonRenderer::Output &output);
JsonRenderer::ContentResult get_printer(size_t resume_point, JsonRenderer::Output &output);
JsonRenderer::ContentResult get_job(size_t resume_point, JsonRenderer::Output &output);

}
