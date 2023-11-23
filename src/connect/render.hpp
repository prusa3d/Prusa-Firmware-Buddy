#pragma once

#include "changes.hpp"
#include "printer.hpp"
#include "planner.hpp"

#include <segmented_json.h>
#include <gcode_thumb_decoder.h>
#include <unique_file_ptr.hpp>
#include <unique_dir_ptr.hpp>
#include <transfers/monitor.hpp>

#include <sys/stat.h>

namespace connect_client {

class PreviewRenderer final : public json::ChunkRenderer {
private:
    AnyGcodeFormatReader *gcode;
    bool started = false;

public:
    PreviewRenderer(AnyGcodeFormatReader *gcode)
        : gcode(gcode) {}
    virtual std::tuple<json::JsonResult, size_t> render(uint8_t *buffer, size_t buffer_size) override;
};

class GcodeMetaRenderer final : public json::ChunkRenderer {
private:
    AnyGcodeFormatReader *gcode;
    GcodeBuffer gcode_line_buffer;
    bool first_run = true;

public:
    GcodeMetaRenderer(AnyGcodeFormatReader *gcode)
        : gcode(gcode) {}

    virtual std::tuple<json::JsonResult, size_t> render(uint8_t *buffer, size_t buffer_size) override;
};

struct DirState {
    unique_dir_ptr dir;
    const char *base_path;
    size_t child_cnt = 0;
    bool first = true;
    bool read_only = false;
    std::optional<size_t> childsize = std::nullopt;
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
    std::unique_ptr<AnyGcodeFormatReader> gcode_reader;
    // Note: The order matters, GcodeMetaRenderer is able to "rewind" the file as
    // needed, the PreviewRenderer doesn't do so.
    using GcodeExtra = json::PairRenderer<PreviewRenderer, GcodeMetaRenderer>;

public:
    json::VariantRenderer<json::EmptyRenderer, GcodeExtra, DirRenderer> renderer;
    FileExtra() = default;
    FileExtra(std::unique_ptr<AnyGcodeFormatReader> gcode_reader_);
    FileExtra(const char *base_path, unique_dir_ptr dir);
};

struct RenderState {
    const Printer &printer;
    const Action &action;
    Tracked &telemetry_changes;
    bool has_stat = false;
    bool read_only = false;
    struct stat st;
    FileExtra file_extra;
    // XXX: Variantize
    std::optional<Printer::NetInfo> lan;
    std::optional<Printer::NetInfo> wifi;

    std::optional<transfers::TransferId> transfer_id = std::nullopt;
    std::optional<CommandId> background_command_id = std::nullopt;

    RenderState(const Printer &printer, const Action &action, Tracked &telemetry_changes, std::optional<CommandId> background_command_id);
};

class Renderer : public json::JsonRenderer<RenderState> {
protected:
    virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, RenderState &state) const override;

public:
    Renderer(RenderState &&state)
        : JsonRenderer(std::move(state)) {}
};

} // namespace connect_client
