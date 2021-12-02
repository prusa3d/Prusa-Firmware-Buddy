#include "http_handler_default.h"
#include "wui_REST_api.h"
#include "wui_api.h"
#include "wui.h"

#define GET_WRAPPER(NAME)                                                                           \
    static uint16_t handler_##NAME(struct HttpHandlers *unused, char *buffer, size_t buffer_size) { \
        (void)unused;                                                                               \
        NAME(buffer, buffer_size);                                                                  \
        return 200;                                                                                 \
    }
GET_WRAPPER(get_printer);
GET_WRAPPER(get_version);
GET_WRAPPER(get_job);
GET_WRAPPER(get_files);
#undef GET_WRAPPER

static uint16_t no_content(struct HttpHandlers *unused, char *buffer, size_t buffer_size) {
    if (buffer_size > 0) {
        *buffer = '\0';
    }
    return 204;
}

static uint16_t settings_stub(struct HttpHandlers *unused, char *buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size, "{\"printer\": {}}");
    return 200;
}

static uint32_t post_start(struct HttpHandlers *unused, const char *filename) {
    return wui_upload_begin(filename);
}

static uint32_t post_data(struct HttpHandlers *unused, const char *data, size_t len) {
    return wui_upload_data(data, len);
}

static uint32_t post_finish(struct HttpHandlers *unused, const char *tmp_filename, const char *final_filename, bool start) {
    return wui_upload_finish(tmp_filename, final_filename, start);
}

static const struct GetDescriptor get_handlers[] = {
    {
        .uri = "/api/printer",
        .handler = handler_get_printer,
    },
    {
        .uri = "/api/version",
        .handler = handler_get_version,
    },
    {
        .uri = "/api/job",
        .handler = handler_get_job,
    },
    {
        .uri = "/api/files",
        .handler = handler_get_files,
        .prefix = true, // Any uri starting with /api/files is fine (for now).
    },
    {
        /*
         * We don't support downloading GCODE from an url (at least not yet).
         * But the PrusaLink page is insistent on asking about it. Stub it out,
         * this basically means "we are not downloading anything right now".
         *
         * Which is correct.
         */
        .uri = "/api/download",
        .handler = no_content,
        .prefix = true,
    },
    {
        /*
         * Settings are stubbed out too, for now.
         */
        .uri = "/api/settings",
        .handler = settings_stub,
    },
    {}, // Sentinel
};

struct HttpHandlers default_http_handlers = {
    .gets = get_handlers,
    .api_key = wui_get_api_key,
    .listener_alloc = {
        .alloc = prusa_alloc,
    },
    .gcode_start = post_start,
    .gcode_data = post_data,
    .gcode_finish = post_finish,
};
