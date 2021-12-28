#pragma once

#include "types.h"

namespace nhttp::handler {

class GenOnce {
public:
    typedef size_t (*Generator)(uint8_t *buffer, size_t buffer_size);

private:
    enum class Progress {
        SendHeaders,
        SendPayload,
        EndChunk,
        Done,
    };

    Generator generator;
    Progress progress = Progress::SendHeaders;
    ContentType content_type;
    ConnectionHandling connection_handling;

public:
    GenOnce(Generator generator, ContentType content_type, bool can_keep_alive)
        : generator(generator)
        , content_type(content_type)
        , connection_handling(can_keep_alive ? ConnectionHandling::ChunkedKeep : ConnectionHandling::Close) {}

    bool want_read() const { return false; }
    bool want_write() const { return progress != Progress::Done; }
    Step step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size);
};

}
