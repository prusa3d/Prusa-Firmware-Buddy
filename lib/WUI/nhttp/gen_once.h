/**
 * \file
 *
 * \brief Handler to generate a small response that fits into a single packet.
 */
#pragma once

#include "types.h"

#include <string_view>

namespace nhttp::handler {

/**
 * \brief Handler to generate a response into a single buffer.
 *
 * This calls the provided generator function and poppulates a response with it. This assumes:
 * * There's no state to be passed to the generator. That is, the response
 *   generated might be dynamic (taking data out of the printer), but doesn't
 *   depend on the request.
 * * The content of the response fits into single buffer and it is generated in
 *   one go (eg. not listing all the files in the SD card).
 * * The request was already validated and the generation of response can't fail.
 */
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
