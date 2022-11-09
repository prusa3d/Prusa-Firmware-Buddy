#pragma once

#include "printer.hpp"
#include "planner.hpp"

#include <segmented_json.h>
#include <gcode_thumb_decoder.h>
#include <unique_file_ptr.hpp>
#include <unique_dir_ptr.hpp>

#include <sys/stat.h>

namespace connect_client {

class PreviewRenderer final : public json::ChunkRenderer {
private:
    GCodeThumbDecoder decoder;
    bool started = false;

public:
    PreviewRenderer(FILE *f);
    virtual std::tuple<json::JsonResult, size_t> render(uint8_t *buffer, size_t buffer_size) override;
};

class GcodeMetaRenderer final : public json::ChunkRenderer {
private:
    FILE *f;
    long resume_position = 0;

public:
    GcodeMetaRenderer(FILE *f)
        : f(f) {}
    GcodeMetaRenderer(GcodeMetaRenderer &&other) = default;
    GcodeMetaRenderer &operator=(GcodeMetaRenderer &&other) = default;
    virtual std::tuple<json::JsonResult, size_t> render(uint8_t *buffer, size_t buffer_size) override;
};

struct DirState {
    unique_dir_ptr dir;
    const char *base_path;
    size_t child_cnt = 0;
    bool first = true;
    struct dirent *ent = nullptr;
};

class DirRenderer final : public json::JsonRenderer<DirState> {
public:
    DirRenderer(const char *base_path, unique_dir_ptr dir);
    DirRenderer(DirRenderer &&other) = default;
    DirRenderer &operator=(DirRenderer &&other) = default;
    virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, DirState &state) const override;
};

// An open file + bunch of decoders for it.
struct FileExtra {
private:
    // Just to make sure it's automatically closed when the time comes.
    unique_file_ptr file;
    // Note: The order matters, GcodeMetaRenderer is able to "rewind" the file as
    // needed, the PreviewRenderer doesn't do so.
    using GcodeExtra = json::PairRenderer<PreviewRenderer, GcodeMetaRenderer>;

public:
    json::VariantRenderer<json::EmptyRenderer, GcodeExtra, DirRenderer> renderer;
    FileExtra() = default;
    FileExtra(unique_file_ptr file);
    FileExtra(const char *base_path, unique_dir_ptr dir);
};

struct RenderState {
    const Printer &printer;
    const Action &action;
    bool has_stat = false;
    struct stat st;
    FileExtra file_extra;
    // XXX: Variantize
    std::optional<Printer::NetInfo> lan;
    std::optional<Printer::NetInfo> wifi;
    std::optional<uint32_t> last_telemetry_fingerprint;
    uint32_t &telemetry_fingerprint_out;

    RenderState(const Printer &printer, const Action &action, std::optional<uint32_t> last_telemetry_fingerprint, uint32_t &telemetry_fingerprint_out);
};

class Renderer : public json::JsonRenderer<RenderState> {
protected:
    virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, RenderState &state) const override;

public:
    Renderer(RenderState &&state)
        : JsonRenderer(std::move(state)) {}
};

}
