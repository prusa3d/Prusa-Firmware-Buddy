#include "nhttp/server.h"
#include "nhttp/common_selectors.h"
#include "nhttp/headers.h"
#include "nhttp/static_mem.h"
#include "wui.h"
#include "wui_api.h"

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

const handler::Selector *selectors_array[] = { &validate_request, &static_fs_file, &unknown_request };

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
