#pragma once

#include <optional>

#include "changes.hpp"
#include "printer.hpp"
#include "planner.hpp"

#include <segmented_json.h>
#include <gcode_reader_any.hpp>
#include <unique_file_ptr.hpp>
#include <directory.hpp>
#include <transfers/monitor.hpp>

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
    /// Physical tools enabled when the response started.
    /// The printer state is re-read for every chunk, but the tool set can change at runtime
    /// (MMU switched, INDX dock calibrated); the renderer reads this instead of each chunk's snapshot.
    decltype(Printer::Params::physical_tool_mask) physical_tool_mask = 0;
    decltype(Printer::Params::virtual_tool_mask) virtual_tool_mask = 0;
    /// Physical tool picked when the response started, kept for the same reason as physical_tool_mask.
    decltype(Printer::Params::active_physical_tool) active_physical_tool = NoTool {};
    /// Virtual tools loaded when the response started, kept for the same reason as physical_tool_mask.
    decltype(Printer::Params::active_virtual_tool_mask) active_virtual_tool_mask = 0;
    /// MMU state when the response started, kept for the same reason as physical_tool_mask - it also decides the
    /// tool ids.
    ToConnectToolArgs tool_args = {};
    /// Material of the INFO tool being rendered, sampled once per tool so that its fields come from one material
    std::optional<FilamentTypeParameters> material;
    /// Spool data of the INFO tool being rendered, sampled with its material
    Printer::VirtualToolInfo virtual_tool_info;
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

#if PRINTER_IS_PRUSA_iX()
    /// The physical tool whose values fill the single-tool telemetry fields (heatbreak and filament sensors): the
    /// picked one, if any, or the first enabled one. The choice is sampled once per response, the values come from
    /// each chunk's snapshot - zero defaults for a tool it does not fill (error printer, or disabled mid-response).
    PhysicalToolIndex preferred_physical_tool() const;
#endif

    bool is_enabled(PhysicalToolIndex tool) const {
        return physical_tool_mask & (1 << tool.to_raw());
    }

    bool is_enabled(VirtualToolIndex tool) const {
        return virtual_tool_mask & (1 << tool.to_raw());
    }

    /// Whether the physical tool is picked.
    bool is_active(PhysicalToolIndex tool) const {
        return stdext::get_optional<PhysicalToolIndex>(active_physical_tool) == tool;
    }

    /// Whether the virtual tool is loaded into its physical tool, picked or not.
    bool is_active(VirtualToolIndex tool) const {
        return active_virtual_tool_mask & (1 << tool.to_raw());
    }

    [[nodiscard]] size_t iter_tool_count() const;

    /// @returns iter to a physical tool index
    [[nodiscard]] std::optional<PhysicalToolIndex> iter_physical_tool() const;

    /// @returns iter to a virtual tool index
    [[nodiscard]] std::optional<VirtualToolIndex> iter_virtual_tool() const;
};

class Renderer : public json::JsonRenderer<RenderState> {
protected:
    virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, RenderState &state) const override;

public:
    Renderer(RenderState &&state)
        : JsonRenderer(std::move(state)) {}
};

} // namespace connect_client
