#pragma once

#include "printer.hpp"
#include "planner.hpp"

#include <segmented_json.h>
#include <gcode_thumb_decoder.h>
#include <unique_file_ptr.hpp>

#include <sys/stat.h>

namespace connect_client {

class PreviewRenderer final : public json::ChunkRenderer {
private:
    GCodeThumbDecoder decoder;
    bool started = false;

protected:
    virtual std::tuple<json::JsonResult, size_t> render(uint8_t *buffer, size_t buffer_size);

public:
    PreviewRenderer(FILE *f);
};

// An open file + bunch of decoders for it.
struct FileExtra {
private:
    // Just to make sure it's automatically closed when the time comes.
    unique_file_ptr file;

public:
    PreviewRenderer preview_renderer;
    FileExtra(unique_file_ptr file);
};

struct RenderState {
    const Printer &printer;
    const Action &action;
    bool has_stat = false;
    struct stat st;
    std::optional<FileExtra> file_extra;
    std::optional<Printer::NetInfo> lan;
    std::optional<Printer::NetInfo> wifi;

    RenderState(const Printer &printer, const Action &action);
};

class Renderer : public json::JsonRenderer<RenderState> {
protected:
    virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, RenderState &state) const override;

public:
    Renderer(RenderState &&state)
        : JsonRenderer(std::move(state)) {}
};

}
