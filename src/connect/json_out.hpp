#pragma once

#include <segmented_json.h>
#include <http/httpc.hpp>

#include <variant>

namespace connect_client {

class JsonPostRequest : public http::Request {
private:
    enum class Progress {
        Rendering,
        Done,
    };
    Progress progress = Progress::Rendering;

protected:
    virtual json::ChunkRenderer &renderer() = 0;

public:
    using RenderResult = std::variant<size_t, http::Error>;
    virtual http::ContentType content_type() const override;
    virtual http::Method method() const override;
    virtual RenderResult write_body_chunk(char *data, size_t size) override;
};

} // namespace connect_client
