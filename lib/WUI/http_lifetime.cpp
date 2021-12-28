#include "nhttp/server.h"
#include "nhttp/common_selectors.h"
#include "nhttp/headers.h"
#include "nhttp/static_mem.h"
#include "wui.h"
#include "wui_api.h"
#include "wui_REST_api.h"

#include <FreeRTOS.h>
#include <semphr.h>
#include <lwip/apps/fs.h>

#include <cassert>
#include <cstring>

using std::nullopt;
using std::optional;
using std::string_view;
using namespace nhttp;
using namespace nhttp::handler;
using namespace nhttp::handler::selectors;

namespace {

SemaphoreHandle_t httpd_mutex = NULL;

// TODO: Move to http_handler_default.cpp?
// TODO: Eventually remove in favor of files in XFlash.
class StaticFsFile final : public Selector {
public:
    virtual optional<ConnectionState> accept(const RequestParser &parser) const override {
        char filename[MAX_URL_LEN + 1];

        if (!parser.uri_filename(filename, sizeof(filename))) {
            return nullopt;
        }

        if (strcmp(filename, "/") == 0) {
            strlcpy(filename, "/index.html", sizeof filename);
        }

        fs_file file;
        if (fs_open(&file, filename) == ERR_OK) {
#if LWIP_HTTPD_CUSTOM_FILES
            // These are not static chunk of memory, not supported.
            if (file.is_custom_file) {
                fs_close(&file);
                return nullopt;
            }
#endif
            /*
             * The data of non-custom fs files are embedded inside the program
             * as C arrays. Therefore, we can get the data and close the file
             * right away.
             */
            /*
             * We guess the content type by a file extension. Looking inside
             * and doing "file magic" would be better/more reliable, but don't
             * want the code complexity of that.
             */
            static const char *extra_hdrs[] = { "Content-Encoding: gzip\r\n", nullptr };
            SendStaticMemory send(string_view(file.data, file.len), guess_content_by_ext(filename), parser.can_keep_alive(), extra_hdrs);
            fs_close(&file);
            return send;
        }

        return nullopt;
    }
};

const StaticFsFile static_fs_file;

#define GET_WRAPPER(NAME)                                               \
    static size_t handler_##NAME(uint8_t *buffer, size_t buffer_size) { \
        char *b = reinterpret_cast<char *>(buffer);                     \
        NAME(b, buffer_size);                                           \
        return strlen(b);                                               \
    }
GET_WRAPPER(get_printer);
GET_WRAPPER(get_version);
GET_WRAPPER(get_job);
#undef GET_WRAPPER

class PrusaLinkApi final : public Selector {
    virtual optional<ConnectionState> accept(const RequestParser &parser) const override {
        const string_view uri = parser.uri();

        // Claim the whole /api prefix.
        const string_view prefix("/api/");
        if (uri.size() < prefix.size() || uri.substr(0, prefix.size()) != prefix) {
            return nullopt;
        }

        if (!parser.authenticated()) {
            return StatusPage(Status::Unauthorized, parser.can_keep_alive());
        }

        const string_view suffix = uri.substr(prefix.size());

        const auto get_only = [parser](ConnectionState state) -> ConnectionState {
            if (parser.method == Method::Get) {
                return state;
            } else {
                return StatusPage(Status::MethodNotAllowerd, parser.can_keep_alive());
            }
        };

        // Some stubs for now, to make more clients (including the web page) happier.
        if (suffix == "download") {
            return get_only(StatusPage(Status::NoContent, parser.can_keep_alive()));
        } else if (suffix == "settings") {
            return get_only(SendStaticMemory("{\"printer\": {}}", ContentType::ApplicationJson, parser.can_keep_alive()));
        } else if (suffix == "version") {
            return get_only(GenOnce(handler_get_version, ContentType::ApplicationJson, parser.can_keep_alive()));
        } else if (suffix == "job") {
            return get_only(GenOnce(handler_get_job, ContentType::ApplicationJson, parser.can_keep_alive()));
        } else if (suffix == "printer") {
            return get_only(GenOnce(handler_get_printer, ContentType::ApplicationJson, parser.can_keep_alive()));
        } else {
            return StatusPage(Status::NotFound, parser.can_keep_alive());
        }
    }
};

const PrusaLinkApi prusa_link_api;

const handler::Selector *selectors_array[] = { &validate_request, &static_fs_file, &prusa_link_api, &unknown_request };

const altcp_allocator_t altcp_alloc = { prusa_alloc };

class DefaultServerDefs final : public ServerDefs {
    virtual const Selector **selectors() const { return selectors_array; }
    virtual const char *get_api_key() const { return wui_get_api_key(); }
    virtual altcp_allocator_t listener_alloc() const { return altcp_alloc; }
    virtual uint16_t port() const { return 80; }
};

const DefaultServerDefs server_defs;

Server server(server_defs);

}

extern "C" {

void httpd_init(void) {
    assert(httpd_mutex == nullptr);
    httpd_mutex = xSemaphoreCreateMutex();
    assert(httpd_mutex != nullptr);

    server.start();
}

void httpd_close(void) {
    assert(httpd_mutex != nullptr);
    xSemaphoreTake(httpd_mutex, portMAX_DELAY);

    server.stop();

    xSemaphoreGive(httpd_mutex);
}

void httpd_reinit(void) {
    assert(httpd_mutex != nullptr);
    xSemaphoreTake(httpd_mutex, portMAX_DELAY);

    server.stop();
    server.start();

    xSemaphoreGive(httpd_mutex);
}
}
