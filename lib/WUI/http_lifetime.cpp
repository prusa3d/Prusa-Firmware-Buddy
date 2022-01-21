#include "nhttp/server.h"
#include "nhttp/common_selectors.h"
#include "link_content/static_fs_file.h"
#include "link_content/static_file.h"
#include "link_content/prusa_link_api.h"
#include "wui.h"
#include "wui_api.h"

#include <FreeRTOS.h>
#include <semphr.h>

#include <cassert>
#include <cstring>

using std::nullopt;
using std::optional;
using std::string_view;
using namespace nhttp;
using namespace nhttp::link_content;
using namespace nhttp::handler;
using namespace nhttp::handler::selectors;
using nhttp::printer::GcodeUpload;

namespace {

SemaphoreHandle_t httpd_mutex = NULL;

class DefaultServerDefs final : public ServerDefs {
private:
    static const constexpr handler::Selector *const selectors_array[] = { &validate_request, &static_file, &static_fs_file, &prusa_link_api, &unknown_request };
    static const constexpr altcp_allocator_t altcp_alloc = { prusa_alloc };

public:
    virtual const Selector *const *selectors() const override { return selectors_array; }
    virtual const char *get_api_key() const override { return wui_get_api_key(); }
    virtual altcp_allocator_t listener_alloc() const override { return altcp_alloc; }
    virtual uint16_t port() const override { return 80; }
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
