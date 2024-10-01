#pragma once

#include "step.h"

#include <http/types.h>
#include <segmented_json.h>
#include <unique_dir_ptr.hpp>

#include <string_view>
#include <variant>

// Why does FILE_PATH_BUFFER_LEN lives in *gui*!?
#include "../../src/gui/file_list_defs.h"

namespace nhttp::printer {

// TODO: Figure a way not to taint the server part with specific implementations
/**
 * \brief Handler for serving file info and directory listings.
 */
class FileInfo {
public:
    enum class ReqMethod {
        Get,
        Head
    };

    using APIVersion = http::APIVersion;

private:
    char filepath[FILE_PATH_BUFFER_LEN];
    bool can_keep_alive : 1;
    bool after_upload : 1;
    bool json_errors : 1;
    ReqMethod method;
    APIVersion api;

    /// Marker for the state before we even tried anything.
    class Uninitialized {};

    /// Marker that we want to send the last chunk (if in chunked mode).
    class LastChunk {};

    struct DirState {
        char *filepath = nullptr;
        char *filename = nullptr;
        unique_dir_ptr dir;
        dirent *ent = nullptr;
        time_t base_folder_timestamp {};
        bool first = true;
        bool read_only = false;
        bool partial = false;
        DirState() = default;
        DirState(FileInfo *owner, DIR *dir)
            : filepath(owner->filepath)
            , dir(dir) {
            std::string_view filepath_view { filepath };
            auto pos = filepath_view.find_last_of('/');
            filename = filepath + pos + 1; // + 1 to get past the /
        }
        DirState(DirState &&other) = default;
        DirState &operator=(DirState &&other) = default;
    };

    /// The JSON renderer for the directory listing.
    class DirRenderer final : public json::JsonRenderer<DirState> {
    private:
        APIVersion api;

        json::JsonResult renderStateOctoprint(size_t resume_point, json::JsonOutput &output, DirState &state) const;
        json::JsonResult renderStateV1(size_t resume_point, json::JsonOutput &output, DirState &state) const;

    protected:
        virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, DirState &state) const override;

    public:
        DirRenderer(APIVersion api)
            : JsonRenderer(DirState())
            , api(api) {}
        DirRenderer(FileInfo *owner, DIR *dir, APIVersion api)
            : JsonRenderer(DirState(owner, dir))
            , api(api) {}
    };

    struct FileState {
        FileInfo *owner;
        int64_t size;
        time_t m_timestamp;
        FileState(FileInfo *owner, size_t size, time_t m_timestamp)
            : owner(owner)
            , size(size)
            , m_timestamp(m_timestamp) {}
        FileState(FileState &&other) = default;
        FileState &operator=(FileState &&other) = default;
    };

    /// Renderer for the file info.
    class FileRenderer final : public json::JsonRenderer<FileState> {
    private:
        APIVersion api;
        bool read_only {};

        json::JsonResult renderStateOctoprint(size_t resume_point, json::JsonOutput &output, FileState &state) const;
        json::JsonResult renderStateV1(size_t resume_point, json::JsonOutput &output, FileState &state) const;

    protected:
        virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, FileState &state) const override;

    public:
        FileRenderer(FileInfo *owner, int64_t size, time_t m_timestamp, APIVersion api, bool read_only)
            : JsonRenderer(FileState(owner, size, m_timestamp))
            , api(api)
            , read_only(read_only) {}
    };
    friend class FileRenderer;

    std::variant<Uninitialized, FileRenderer, DirRenderer, LastChunk> renderer;
    std::optional<uint32_t> etag;

public:
    FileInfo(const char *filename, bool can_keep_alive, bool json_error, bool after_upload, ReqMethod method, APIVersion api, std::optional<uint32_t> etag);
    bool want_read() const { return false; }
    bool want_write() const { return true; }
    void step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size, handler::Step &out);
};

} // namespace nhttp::printer
