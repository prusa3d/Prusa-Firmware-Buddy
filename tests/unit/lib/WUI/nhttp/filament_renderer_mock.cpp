#include <filament_renderer.h>

// The real implementation reads the filament configuration and trackers,
// which the unit tests don't provide
namespace nhttp::handler {
json::JsonResult FilamentRenderer::renderState(size_t resume_point, json::JsonOutput &output, FilamentState &state) const {
    // empty on purpose
    return json::JsonResult::Complete;
}
} // namespace nhttp::handler
