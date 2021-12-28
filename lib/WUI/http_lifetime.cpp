#include "nhttp/server.h"
#include "nhttp/common_selectors.h"
#include "wui.h"
#include "wui_api.h"

#include <FreeRTOS.h>
#include <semphr.h>

#include <cassert>

using namespace nhttp;
using namespace nhttp::handler;
using namespace nhttp::handler::selectors;

namespace {

SemaphoreHandle_t httpd_mutex = NULL;

const handler::Selector *selectors_array[] = { &validate_request, &unknown_request };

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
