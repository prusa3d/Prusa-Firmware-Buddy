#include "nhttp/server.h"
#include "nhttp/common_selectors.h"
#include "link_content/static_file.h"
#include "link_content/prusa_link_api.h"
#include "link_content/usb_files.h"
#include "link_content/previews.h"
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
    static const constexpr handler::Selector *const selectors_array[] = { &validate_request, &static_file, &prusa_link_api, &usb_files, &previews, &unknown_request };

public:
    virtual const Selector *const *selectors() const override { return selectors_array; }
    virtual const char *get_api_key() const override { return wui_get_api_key(); }
    virtual altcp_pcb *listener_alloc() const override {
        /*
         * We know we are in the part where ALTCP is turned off. That means the
         * tcp_pcb and altcp_pcb are the same thing.
         *
         * We use the altcp_pcb in the interface only to allow tests to mock it
         * (and have it turned on).
         */
        altcp_pcb *l = tcp_new_ip_type(IPADDR_TYPE_ANY);

        if (l == nullptr) {
            return nullptr;
        }

        /*
         * set SOF_REUSEADDR to explicitly bind httpd to multiple
         * interfaces and to allow re-binding after enabling & disabling
         * ethernet.
         */
        ip_set_option((struct tcp_pcb *)l, SOF_REUSEADDR);
        const auto err = tcp_bind(l, IP_ANY_TYPE, 80);
        if (err != ERR_OK) {
            /*
             * Note: According to docs, the altcp_close _can fail_. Nevertheless:
             *
             * * Using altcp_abort on listening connection doesn't work.
             * * It is assumed to be able to fail due to eg. inability to send a
             *   FIN packet. This is not the case for a listening socket.
             */
            altcp_close(l);
            return nullptr;
        }

        return altcp_listen(l);
    }
};

const DefaultServerDefs server_defs;

Server server(server_defs);

}

void httpd_init(void) {
    assert(httpd_mutex == nullptr);
    httpd_mutex = xSemaphoreCreateMutex();
    assert(httpd_mutex != nullptr);
}

void httpd_start(void) {
    assert(httpd_mutex != nullptr);
    xSemaphoreTake(httpd_mutex, portMAX_DELAY);

    server.start();

    xSemaphoreGive(httpd_mutex);
}

void httpd_close(void) {
    assert(httpd_mutex != nullptr);
    xSemaphoreTake(httpd_mutex, portMAX_DELAY);

    server.stop();

    xSemaphoreGive(httpd_mutex);
}
