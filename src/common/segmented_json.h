#pragma once

#include <cstdlib>
#include <cstdint>
#include <tuple>
#include <optional>
#include <variant>

namespace json {

/// Passing state of an attempt to write something.
///
/// (Helper enum for JsonRenderer).
enum class JsonResult {
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

class ChunkRenderer;

/// A proxy for the buffer where the data goes.
//
/// (Helper class for JsonRenderer).
///
/// This can be used to produce bits and pieces of the resulting JSON. The
/// methods either produce what's needed and return ContentResult::Complete,
/// or produce nothing and return ContentResult::Incomplete.
///
/// Most of the time, it is used through the macros from segmented_json_macros.h
class JsonOutput {
private:
    bool written_something = false;
    uint8_t *buffer;
    size_t &buffer_size;
    size_t &resume_point;

    JsonResult suspend(size_t resume_point);

public:
    JsonOutput(uint8_t *buffer, size_t &buffer_size, size_t &resume_point)
        : buffer(buffer)
        , buffer_size(buffer_size)
        , resume_point(resume_point) {}

    JsonResult output(size_t resume_point, const char *format, ...);
    // Render a bit of string, without any surrounding stuff (eg. no " " around it, no field name, etc).
    // Size-delimited (unlike tho field_str variants).
    //
    // The idea is to support long strings, renderred in multiple chunks.
    JsonResult output_str_chunk(size_t resume_point, const char *value, size_t size);
    // TODO: Add others as needed.
    JsonResult output_field_bool(size_t resume_point, const char *name, bool value);
    JsonResult output_field_str(size_t resume_point, const char *name, const char *value);
    JsonResult output_field_str_esc(size_t resume_point, const char *name, const char *value);
    JsonResult output_field_int(size_t resume_point, const char *name, int64_t value);
    // Fixed precision
    JsonResult output_field_float_fixed(size_t resume_point, const char *name, double value, int precision);
    JsonResult output_field_str_format(size_t resume_point, const char *name, const char *format, ...);
    JsonResult output_field_obj(size_t resume_point, const char *name);
    JsonResult output_field_arr(size_t resume_point, const char *name);
    // The value is rendered by calling the provided sub-renderer.
    //
    // This is not a "field" (it isn't prepended by a name, such part needs to
    // be provided separately for implementation simplicity). If the result
    // shall be eg. a string, the sub-renderer needs to output the quotes and
    // do the escaping.
    JsonResult output_chunk(size_t resume_point, ChunkRenderer &renderer);
};

/// Can render something (not necessarily the whole JSON).
class ChunkRenderer {
public:
    virtual ~ChunkRenderer() = default;
    /// When called, this would produce the next part of the resulting JSON into the provided buffer and signal:
    /// * If this is the whole thing or not.
    /// * How much of the buffer was written (even if the Incomplete is
    ///   returned, there might be part of the buffer unused ‒ the data is split
    ///   only at certain points, not at arbitrary byte position).
    ///
    /// Calling it again after Abort or Complete was returned is invalid. It is
    /// possible to call after return of BufferTooSmall with a bigger buffer.
    virtual std::tuple<JsonResult, size_t> render(uint8_t *buffer, size_t buffer_size) = 0;
};

/// Renders nothing, can act as a placeholder when composing stuff together.
class EmptyRenderer : public ChunkRenderer {
public:
    virtual std::tuple<JsonResult, size_t> render(uint8_t *, size_t) override;
};

template <class... V>
class VariantRenderer : public ChunkRenderer {
private:
    std::variant<V...> data;

public:
    VariantRenderer() = default;
    VariantRenderer(std::variant<V...> &&d)
        : data(std::move(d)) {}
    virtual std::tuple<JsonResult, size_t> render(uint8_t *buffer, size_t buffer_size) override {
        return std::visit([&](auto &d) { return d.render(buffer, buffer_size); }, data);
    }
    template <class T>
    bool holds_alternative() { return std::holds_alternative<T>(data); }
};

// An abitily to join two renderers after each other and compose them into a single renderer.
//
// This in theory should be possible in a variadic form (eg. arbitrary number
// of renderers). But my template-fu is not strong enough and, honestly, I'm
// not sure the code would be remotely readable.
template <class A, class B>
class PairRenderer : public ChunkRenderer {
private:
    A a;
    B b;
    bool consumed_a = false;

public:
    PairRenderer() = default;
    PairRenderer(A &&a, B &&b)
        : a(std::move(a))
        , b(std::move(b)) {}
    PairRenderer(PairRenderer<A, B> &&other) = default;
    PairRenderer<A, B> &operator=(PairRenderer<A, B> &&other) = default;
    virtual std::tuple<JsonResult, size_t> render(uint8_t *buffer, size_t buffer_size) override {
        if (consumed_a) {
            // Yes, the easy thing.
            return b.render(buffer, buffer_size);
        }

        const auto [result_a, used_a] = a.render(buffer, buffer_size);

        switch (result_a) {
        case JsonResult::Complete: {
            consumed_a = true;
            if (buffer_size == used_a) {
                // a is complete, b is not and there's no buffer left to call into b.
                return std::make_tuple(JsonResult::Incomplete, used_a);
            }

            const auto [result_b, used_b] = b.render(buffer + used_a, buffer_size - used_a);
            switch (result_b) {
            case JsonResult::BufferTooSmall:
                // Next time we'll have a bit more of the buffer, so retry.
                return std::make_tuple(JsonResult::Incomplete, used_a);
            default:
                return std::make_tuple(result_b, used_a + used_b);
            }
        }
        default:
            return std::make_tuple(result_a, used_a);
        }
    }
};

/// Support for rendering JSON by parts.
///
/// If possible, prefer the use of JsonRenderer. This one is more low level and
/// can be used when the JsonRenderer is too limiting. But this one is more
/// error prone (it's easier to create inconsistent response).
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
class LowLevelJsonRenderer : public ChunkRenderer {
private:
    size_t resume_point = 0;

public:
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
    virtual std::tuple<JsonResult, size_t> render(uint8_t *buffer, size_t buffer_size) override;

protected:
    /// The inner implementation, to be provided by an author of the renderer.
    virtual JsonResult content(size_t resume_point, JsonOutput &output) = 0;
};

/// The "higher level" JSON renderer.
///
/// This does more or less the same as LowLevelJsonRenderer, but slightly
/// pushes to preparing the rendered state upfront and making sure the response
/// is asured to be consistent.
///
/// The usage:
///
/// * The state to render (the yet unrendered answer) is created and passed to
///   the constructor.
/// * The renderState is used to format the response.
///
/// While it is needed to inherit the class to implement renderState, the
/// actual state should be in the State passed to constructor, not in the child
/// implementation.
template <class State>
class JsonRenderer : public LowLevelJsonRenderer {
private:
    State state;

protected:
    virtual JsonResult content(size_t resume_point, JsonOutput &output) override {
        return renderState(resume_point, output, state);
    }

    virtual JsonResult renderState(size_t resume_point, JsonOutput &output, State &state) const = 0;

public:
    JsonRenderer(State state)
        : state(std::move(state)) {}
};
} // namespace json
