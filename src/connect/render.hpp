#pragma once

#include "changes.hpp"
#include "printer.hpp"
#include "planner.hpp"

#include <segmented_json.h>
#include <gcode_reader_any.hpp>
#include <unique_file_ptr.hpp>
#include <directory.hpp>
#include <transfers/monitor.hpp>

#include <bit>
#include <sys/stat.h>

namespace connect_client {

class PreviewRenderer final : public json::ChunkRenderer {
private:
    IGcodeReader *gcode;
    AbstractByteReader *thumbnail_reader;

public:
    PreviewRenderer(IGcodeReader *gcode)
        : gcode(gcode)
        , thumbnail_reader(nullptr) {}
    virtual std::tuple<json::JsonResult, size_t> render(uint8_t *buffer, size_t buffer_size) override;
};

class GcodeMetaRenderer final : public json::ChunkRenderer {
private:
    std::optional<GcodeBuffer::String::parsed_metadata_t> parsed;
    IGcodeReader *gcode;
    GcodeBuffer gcode_line_buffer;
    bool first_run = true;
    bool str_continuation = false;
    void reset_buffer();
    // Will (try to) output part of string "body" (escaped per JSON as needed).
    //
    // Puts a terminating " on the last segment.
    //
    // Will adjust str_continuation based on the buffer's line_complete.
    json::JsonResult out_str_chunk(json::JsonOutput &output, const GcodeBuffer::String &str);

public:
    GcodeMetaRenderer(IGcodeReader *gcode)
        : gcode(gcode) {}

    virtual std::tuple<json::JsonResult, size_t> render(uint8_t *buffer, size_t buffer_size) override;
};

struct DirState {
    Directory dir;
    const char *base_path;
    size_t child_cnt = 0;
    bool first = true;
    bool read_only = false;
    std::optional<size_t> childsize = std::nullopt;
    struct dirent *ent = nullptr;
};

class DirRenderer final : public json::JsonRenderer<DirState> {
public:
    DirRenderer(const char *base_path, Directory dir);
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
    FileExtra(const char *base_path, Directory dir);
};

struct RenderState {
    // variable used to iterate through mmu/xl slots
    size_t iter = 0;
    // Because JSON can't have trailing commas :-(
    bool need_comma = false;

    const Printer &printer;
    const Action &action;
    /// Tools enabled when the response started.
    /// The printer state is re-read for every chunk, but the tool set can change at runtime
    /// (MMU switched, INDX dock calibrated); the renderer reads this instead of each chunk's snapshot.
    decltype(Printer::Params::slot_mask) slot_mask = 0;
    /// Tool active when the response started, kept for the same reason as slot_mask.
    decltype(Printer::Params::active_slot) active_slot = NoTool {};
    /// MMU state when the response started, kept for the same reason as slot_mask - it also decides the tool ids.
    ToConnectToolArgs tool_args = {};
    bool has_stat = false;
    bool read_only = false;
    struct stat st;
    FileExtra file_extra;
    // XXX: Variantize
    std::optional<Printer::NetInfo> lan;
#if HAS_ESP()
    std::optional<Printer::NetInfo> wifi;
#endif

    std::optional<transfers::TransferId> transfer_id = std::nullopt;
    std::optional<CommandId> background_command_id = std::nullopt;
    // Temporary store to keep iteration stable.
    const Response *buttons = nullptr;

    RenderState(const Printer &printer, const Action &action, std::optional<CommandId> background_command_id);

    uint8_t enabled_tool_cnt() const {
        return std::popcount(slot_mask);
    }

    /// The virtual tool whose values fill the single-tool fields of the old protocol (temperature, fans, nozzle,
    /// material): the active one, if any, or the first enabled one.
    /// The choice comes from the captured state, so all chunks of a response describe the same tool. The values are
    /// read from each chunk's snapshot - it always holds all the tools, so the index is valid there.
    VirtualToolIndex preferred_virtual_tool() const;
};

class Renderer : public json::JsonRenderer<RenderState> {
protected:
    virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, RenderState &state) const override;

public:
    Renderer(RenderState &&state)
        : JsonRenderer(std::move(state)) {}
};

} // namespace connect_client
