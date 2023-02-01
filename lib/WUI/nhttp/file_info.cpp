#include "file_info.h"
#include "handler.h"
#include "headers.h"
#include "status_page.h"
#include "../../src/common/lfn.h"
#include "../../src/common/filename_type.hpp"
#include "../wui_api.h"

#include <http/chunked.h>
#include <json_encode.h>
#include <segmented_json_macros.h>
#include <basename.h>

#include <cstring>
#include <sys/stat.h>

using http::ConnectionHandling;
using http::ContentType;
using http::LAST_CHUNK;
using http::MIN_CHUNK_SIZE;
using http::Status;
using namespace json;
using std::get_if;
using std::holds_alternative;

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
}

namespace nhttp::printer {

using namespace handler;

JsonResult FileInfo::DirRenderer::renderState(size_t resume_point, JsonOutput &output, FileInfo::DirState &state) const {
    if (api == APIVersion::Octoprint) {
        return renderStateOctoprint(resume_point, output, state);
    } else {
        return renderStateV1(resume_point, output, state);
    }
}

JsonResult FileInfo::DirRenderer::renderStateV1(size_t resume_point, JsonOutput &output, FileInfo::DirState &state) const {
    struct stat st {};

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START;

        JSON_CONTROL("\"type\":\"FOLDER\"") JSON_COMMA;
        JSON_FIELD_BOOL("ro", false) JSON_COMMA;
        // Note: we can only get a timestamp for folders that are not
        // /usb/ because that is the root of FATFS and stat() don't work on it
        if (stat(state.filepath, &st) == 0) {
            // Note: We need the timestamp to be persistent betweeen resumes,
            // so we save it to state.
            state.base_folder_timestamp = st.st_mtime;
            JSON_FIELD_INT("m_timestamp", state.base_folder_timestamp) JSON_COMMA;
        }
        JSON_FIELD_STR("name", state.filename) JSON_COMMA;
        JSON_FIELD_ARR("children");
        while (state.dir.get() && (state.ent = readdir(state.dir.get()))) {
            if (state.ent->d_type != DT_DIR and !filename_is_gcode(state.ent->d_name)) {
                continue;
            }

            if (!state.first) {
                JSON_COMMA;
            } else {
                state.first = false;
            }
            JSON_OBJ_START;
                JSON_FIELD_STR("name", state.ent->d_name) JSON_COMMA;
                JSON_FIELD_BOOL("ro", false) JSON_COMMA;
                JSON_FIELD_STR("type", file_type(state.ent)) JSON_COMMA;
                JSON_FIELD_INT("m_timestamp", state.ent->time) JSON_COMMA;
                if (state.ent->d_type != DT_DIR) {
                    JSON_FIELD_OBJ("refs");
                        if (filename_is_gcode(state.ent->d_name)) {
                            JSON_FIELD_STR_FORMAT("icon", "/thumb/s%s/%s", state.filepath, state.ent->d_name) JSON_COMMA;
                            JSON_FIELD_STR_FORMAT("thumbnail", "/thumb/l%s/%s", state.filepath, state.ent->d_name) JSON_COMMA;
                        }
                        JSON_FIELD_STR_FORMAT("download", "%s/%s", state.filepath, state.ent->d_name);
                    JSON_OBJ_END JSON_COMMA;
                }
                JSON_FIELD_STR("display_name", state.ent->lfn);
            JSON_OBJ_END;
        }
        JSON_ARR_END;

    JSON_OBJ_END;
    JSON_END;
    // clang-format on
}
JsonResult FileInfo::DirRenderer::renderStateOctoprint(size_t resume_point, JsonOutput &output, FileInfo::DirState &state) const {
    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START
        JSON_FIELD_ARR("files");
            JSON_OBJ_START;
                JSON_CONTROL("\"name\":\"USB\",\"path\":\"/usb\",\"display\":\"USB\",\"type\":\"folder\",\"origin\":\"usb\",");
                JSON_FIELD_ARR("children");

                // Note: ent, as the control variable, needs to be preserved inside the
                // object, so it survives resumes.
                while (state.dir.get() && (state.ent = readdir(state.dir.get()))) {
                    if (!filename_is_gcode(state.ent->d_name)) {
                        continue;
                    }

                    if (!state.first) {
                        JSON_COMMA;
                    } else {
                        state.first = false;
                    }

                    JSON_OBJ_START;
                        JSON_FIELD_STR("name", state.ent->d_name) JSON_COMMA;
#ifdef UNITTESTS
                        JSON_FIELD_STR("display", state.ent->d_name) JSON_COMMA;
#else
                        JSON_FIELD_STR("display", state.ent->lfn) JSON_COMMA;
#endif
                        JSON_FIELD_STR_FORMAT("path", "%s/%s", state.filename, state.ent->d_name) JSON_COMMA;
                        JSON_CONTROL("\"origin\":\"usb\",");
                        JSON_FIELD_OBJ("refs");
                            JSON_FIELD_STR_FORMAT("resource", "/api/files%s/%s", state.filepath, state.ent->d_name) JSON_COMMA;
                            JSON_FIELD_STR_FORMAT("thumbnailSmall", "/thumb/s%s/%s", state.filepath, state.ent->d_name) JSON_COMMA;
                            JSON_FIELD_STR_FORMAT("thumbnailBig", "/thumb/l%s/%s", state.filepath, state.ent->d_name) JSON_COMMA;
                            JSON_FIELD_STR_FORMAT("download", "%s/%s", state.filename, state.ent->d_name);
                        JSON_OBJ_END;
                    JSON_OBJ_END;
                }

                JSON_ARR_END;
            JSON_OBJ_END;
        JSON_ARR_END;
    JSON_OBJ_END
    JSON_END;
    // clang-format on
}

JsonResult FileInfo::FileRenderer::renderState(size_t resume_point, JsonOutput &output, FileInfo::FileState &state) const {
    if (api == APIVersion::Octoprint) {
        return renderStateOctoprint(resume_point, output, state);
    } else {
        return renderStateV1(resume_point, output, state);
    }
}

JsonResult FileInfo::FileRenderer::renderStateV1(size_t resume_point, JsonOutput &output, FileInfo::FileState &state) const {
    char *filename = state.owner->filepath;
    get_SFN_path(filename);
    JSONIFY_STR(filename);
    char long_name[FILE_NAME_BUFFER_LEN];
    get_LFN(long_name, sizeof long_name, filename);
    const char *type = file_type_by_ext(long_name);
    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START;

        JSON_FIELD_STR("name", basename_b(filename)) JSON_COMMA;
        JSON_FIELD_BOOL("ro", false) JSON_COMMA;
        JSON_FIELD_STR("type", type) JSON_COMMA;
        JSON_FIELD_INT("m_timestamp", state.m_timestamp) JSON_COMMA;
        JSON_FIELD_INT("size", state.size) JSON_COMMA;
        if (strcmp(type, "FOLDER") != 0) {
            JSON_FIELD_OBJ("refs");
                if (filename_is_gcode(filename)) {
                    JSON_CUSTOM("\"icon\":\"/thumb/s%s\",", filename_escaped);
                    JSON_CUSTOM("\"thumbnail\":\"/thumb/l%s\",", filename_escaped);
                }
                JSON_FIELD_STR("download", filename);
            JSON_OBJ_END JSON_COMMA;
        }
        JSON_FIELD_STR("display_name", long_name);
        // metadata

    JSON_OBJ_END;
    JSON_END;
    // clang-format on
}

JsonResult FileInfo::FileRenderer::renderStateOctoprint(size_t resume_point, JsonOutput &output, FileInfo::FileState &state) const {
    char *filename = state.owner->filepath;
    JSONIFY_STR(filename);
    char long_name[FILE_NAME_BUFFER_LEN];
    get_LFN(long_name, sizeof long_name, filename);
    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START;
        JSON_FIELD_STR("name", long_name) JSON_COMMA;
        JSON_FIELD_STR("origin", "local") JSON_COMMA;
        JSON_FIELD_INT("size", state.size) JSON_COMMA;
        JSON_FIELD_OBJ("refs");
            JSON_CUSTOM("\"resource\":\"/api/files%s\",", filename_escaped);
            JSON_CUSTOM("\"thumbnailSmall\":\"/thumb/s%s\",", filename_escaped);
            JSON_CUSTOM("\"thumbnailBig\":\"/thumb/l%s\",", filename_escaped);
            JSON_FIELD_STR("download", filename);
        JSON_OBJ_END;
    JSON_OBJ_END;
    JSON_END;
    // clang-format on
}

FileInfo::FileInfo(const char *filepath, bool can_keep_alive, bool json_errors, bool after_upload, ReqMethod method, APIVersion api)
    : can_keep_alive(can_keep_alive)
    , after_upload(after_upload)
    , json_errors(json_errors)
    , method(method)
    , api(api) {
    strlcpy(this->filepath, filepath, sizeof this->filepath);
    /*
     * Eat the last slash on directories.
     *
     * This is mostly aestetic only... otherwise we get double slashs in the
     * results/URLs. That works, but, meh...
     */
    const size_t len = strlen(this->filepath);
    if (len > 0 && this->filepath[len - 1] == '/') {
        this->filepath[len - 1] = '\0';
    }
}

Step FileInfo::step(std::string_view, bool, uint8_t *output, size_t output_size) {
    const size_t last_chunk_len = strlen(LAST_CHUNK);
    size_t written = 0;
    const ConnectionHandling handling = can_keep_alive ? (method == ReqMethod::Head ? ConnectionHandling::ContentLengthKeep : ConnectionHandling::ChunkedKeep) : ConnectionHandling::Close;
    bool first_packet = false;

    if (holds_alternative<Uninitialized>(renderer)) {
        // We have not been initialized yet. Try deciding if we are listing a
        // directory or a file (or if there's an error).
        //
        // At this point we not only create the right renderer, we also produce
        // the right headers.

        struct stat finfo;
        first_packet = true;

        if (DIR *dir_attempt = opendir(filepath); dir_attempt) {
            renderer = DirRenderer(this, dir_attempt, api);
        } else if (stat(filepath, &finfo) == 0) {
            renderer = FileRenderer(this, finfo.st_size, finfo.st_mtime, api);
        } else if (strcmp(filepath, "/usb") == 0) {
            // We are trying to list files in the root and it's not there -> USB is missing.
            // Special case it, we return empty list of files.
            renderer = DirRenderer(api); // Produces empty file list
        } else {
            return Step { 0, 0, StatusPage(Status::NotFound, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors) };
        }

        const char *extra_hdrs[] = {
            "Read-Only: false\r\n",
            wui_is_file_being_printed(filepath) ? "Currently-Printing: true\r\n" : "Currently-Printing: false\r\n",
            nullptr
        };
        written = write_headers(output, output_size, after_upload ? Status::Created : Status::Ok, ContentType::ApplicationJson, handling, std::nullopt, std::nullopt, extra_hdrs);

        if (method == ReqMethod::Head) {
            return Step { 0, written, Terminating::for_handling(handling) };
        }

        if (output_size - written <= MIN_CHUNK_SIZE) {
            return { 0, written, Continue() };
        }
    }
    // Note: The above falls through and tries to put at least part of the answer into the rest of the current packet with headers.

    // Process both the renderers in uniform way.
    LowLevelJsonRenderer *rend = get_if<DirRenderer>(&renderer);
    rend = rend ?: get_if<FileRenderer>(&renderer);
    if (rend != nullptr) {
        JsonResult render_result;
        written += render_chunk(handling, output + written, output_size - written, [&](uint8_t *output, size_t output_size) {
            const auto [result, written_json] = rend->render(output, output_size);
            render_result = result;
            return written_json;
        });

        switch (render_result) {
        case JsonResult::Complete:
            renderer = LastChunk();
            break;
        case JsonResult::Incomplete:
            // Send this packet out, but ask for another one.
            return { 0, written, Continue() };
        case JsonResult::BufferTooSmall:
            if (first_packet) {
                // Too small, but possibly because we've taken up a part by the headers.
                return { 0, written, Continue() };
            }
        case JsonResult::Abort:
            // Something unexpected got screwed up. We don't have a way to
            // return a 500 error, we have sent the headers out already
            // (possibly), so the best we can do is to abort the
            // connection.
            return { 0, 0, Terminating { 0, Done::CloseFast } };
        }
    }

    if (holds_alternative<LastChunk>(renderer)) {
        if (can_keep_alive) {
            if (written + last_chunk_len > output_size) {
                // Need to leave the last chunk for next packet
                return { 0, written, Continue() };
            } else {
                memcpy(output + written, LAST_CHUNK, last_chunk_len);
                written += last_chunk_len;
            }
        }

        return Step { 0, written, Terminating::for_handling(handling) };
    }

    // This place should be unreachable!
    assert(0);
    return { 0, 0, Terminating { 0, Done::CloseFast } };
}

}
