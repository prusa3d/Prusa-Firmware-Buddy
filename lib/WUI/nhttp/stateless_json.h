#pragma once

#include "types.h"
#include "segmented_json.h"

#include <string_view>

namespace nhttp::handler {

/// Renderer of generic JSON.
///
/// This'll generate a JSON response, provided a parameter-less function (not a
/// closure/lambda or functor) that generates the content. Meant for simple
/// things where there's no state to track during the generation.
class StatelessJson {
public:
    typedef JsonResult (*Content)(size_t resume_point, JsonOutput &output);

private:
    enum class Progress {
        SendHeaders,
        SendPayload,
        EndChunk,
        Done,
    };

    class Empty {};

    class Renderer final : public JsonRenderer<Empty> {
    private:
        Content generator;

    public:
        Renderer(Content content)
            : JsonRenderer(Empty {})
            , generator(content) {}
        virtual JsonResult renderState(size_t resume_point, JsonOutput &output, Empty &state) const override;
    };

    Renderer renderer;
    Progress progress = Progress::SendHeaders;
    ConnectionHandling connection_handling;

public:
    StatelessJson(Content content, bool can_keep_alive)
        : renderer(content)
        , connection_handling(can_keep_alive ? ConnectionHandling::ChunkedKeep : ConnectionHandling::Close) {}

    bool want_read() const { return false; }
    bool want_write() const { return progress != Progress::Done; }
    Step step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size);
};

}
