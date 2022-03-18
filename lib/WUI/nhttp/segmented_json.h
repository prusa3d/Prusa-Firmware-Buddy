#pragma once

#include <cstdlib>
#include <cstdint>
#include <tuple>

namespace nhttp {

/// Support for rendering JSON by parts.
///
/// Sometimes, we may not afford to send the whole JSON answer in one packet
/// and we have only a single packet of buffer space. Therefore, we need to be
/// able to store just enough of state to be able to generate a consistent
/// reply and then allow suspending at places.
///
/// This supports suspending between individual JSON fields. We still assume
/// we'll be able to put at least the field inside the buffer (that is, no extra
/// huge strings).
///
/// The user inherits this, adds the state and provides the content function.
///
/// This does not validate the produced JSON in any way (and probably could be
/// used to render something else than JSON, actually).
///
/// Few notes for implementers:
/// * The idea behind the content function is that it tries to call methods on
///   the provided output until it returns ContentResult::Incomplete. Then the
///   content function returns and it'll get called in the future with a new buffer
///   and the resume_point parameter set to whatever was passed to the output that
///   didn't fit. This can be used to jump to the right place and continue from
///   there (retrying the one that didn't fit).
/// * A switch over the resume_point may be used to "jump" to the right place.
///   But the switch is used _without_ breaks, to continue with the next outputs.
/// * The implementation of the content function is a bit awkward. Therefore,
///   we also provide the segmented_json_macros.h header with helpful macros.
///   These take care of generating the right switch with jumping to the
///   resumed position. It is in a separate header that can be included only in
///   the .cpp file, while including this file into another header file without
///   polluting the namespace with macros.
/// * The test cases in tests/unit/lib/WUI/nhttp/segmented_json_rendering.cpp
///   contains an example how to use this.
///
/// Note that state that needs to survive between resumption either needs to be
/// stored in the object or re-computed at the start of the content method (but
/// it gets recomputed on each resume, therefore it should produce the same or at
/// least compatible results).
///
/// Also note that there's no way to "restart" the renderer.
class JsonRenderer {
private:
    size_t resume_point = 0;

public:
    /// Passing state of an attempt to write something.
    enum class ContentResult {
        /// This thing didn't fit.
        ///
        /// But something previous might have.
        Incomplete,
        /// All done.
        Complete,
        /// Propagated request from content to abort the rendering without contituation.
        Abort,
        /// The provided buffer is too small to output even a single token.
        ///
        /// This exists mostly to detect error states with eg. a too large
        /// string. If it was implemented without this detection, it would
        /// repeatedly try to produce empty buffers and create an infinite loop.
        BufferTooSmall,
    };

    /// A proxy for the buffer where the data goes.
    ///
    /// This can be used to produce bits and pieces of the resulting JSON. The
    /// methods either produce what's needed and return ContentResult::Complete,
    /// or produce nothing and return ContentResult::Incomplete.
    ///
    /// Most of the time, it is used through the macros from segmented_json_macros.h
    class Output {
    private:
        bool written_something = false;
        uint8_t *buffer;
        size_t &buffer_size;
        size_t &resume_point;
        friend class JsonRenderer;
        Output(uint8_t *buffer, size_t &buffer_size, size_t &resume_point)
            : buffer(buffer)
            , buffer_size(buffer_size)
            , resume_point(resume_point) {}

    public:
        ContentResult output(size_t resume_point, const char *format, ...);
        // TODO: Add others as needed.
        ContentResult output_field_bool(size_t resume_point, const char *name, bool value);
        ContentResult output_field_str(size_t resume_point, const char *name, const char *value);
        ContentResult output_field_int(size_t resume_point, const char *name, int64_t value);
        // Fixed precision
        ContentResult output_field_float_fixed(size_t resume_point, const char *name, double value, int precision);
        ContentResult output_field_str_format(size_t resume_point, const char *name, const char *format, ...);
        ContentResult output_field_obj(size_t resume_point, const char *name);
        ContentResult output_field_arr(size_t resume_point, const char *name);
    };

    virtual ~JsonRenderer() = default;
    /// The outer entry point.
    ///
    /// When called, this would produce the next part of the resulting JSON into the provided buffer and signal:
    /// * If this is the whole thing or not.
    /// * How much of the buffer was written (even if the Incomplete is
    ///   returned, there might be part of the buffer unused ‒ the data is split
    ///   only at certain points, not at arbitrary byte position).
    ///
    /// Calling it again after Abort or Complete was returned is invalid. It is
    /// possible to call after return of BufferTooSmall with a bigger buffer.
    std::tuple<ContentResult, size_t> render(uint8_t *buffer, size_t buffer_size);

protected:
    /// The inner implementation, to be provided by an author of the renderer.
    virtual ContentResult content(size_t resume_point, Output &output) = 0;
};

}
