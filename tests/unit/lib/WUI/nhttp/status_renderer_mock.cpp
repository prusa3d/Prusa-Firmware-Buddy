#include <status_renderer.h>

// Real impentation calls to marlin_vars, we mock it to not have to
// deal with it
namespace nhttp::handler {
json::JsonResult StatusRenderer::renderState(size_t resume_point, json::JsonOutput &output, StatusState &state) const {
    // empty on purpose
    return json::JsonResult::Complete;
}
} // namespace nhttp::handler
