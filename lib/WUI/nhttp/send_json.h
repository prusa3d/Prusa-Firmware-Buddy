#pragma once

#include "step.h"

#include <segmented_json.h>
#include <http/types.h>

#include <string_view>

namespace nhttp::handler {

class Empty {};

/// Renderer with no state attached, will generate json
/// provided a parameter-less function (not a
/// closure/lambda or functor) that generates the content. Meant for simple
/// things where there's no state to track during the generation.
class EmptyRenderer final : public json::JsonRenderer<Empty> {
public:
    typedef json::JsonResult (*Content)(size_t resume_point, json::JsonOutput &output);

private:
    Content generator;

public:
    EmptyRenderer(Content content)
        : JsonRenderer(Empty {})
        , generator(content) {}
    virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, Empty &state) const override;
};

/// Renderer of generic JSON.
///
/// This'll generate a JSON response, provided a class derived from JsonRenderer
/// that may or may not have a state.
template <class Renderer>
class SendJson {
private:
    enum class Progress {
        SendHeaders,
        SendPayload,
        EndChunk,
        Done,
    };

    Renderer renderer;
    Progress progress = Progress::SendHeaders;
    http::ConnectionHandling connection_handling;

public:
    SendJson(Renderer content, bool can_keep_alive)
        : renderer(std::move(content))
        , connection_handling(can_keep_alive ? http::ConnectionHandling::ChunkedKeep : http::ConnectionHandling::Close) {}

    bool want_read() const { return false; }
    bool want_write() const { return progress != Progress::Done; }
    void step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size, Step &out);
};

} // namespace nhttp::handler
