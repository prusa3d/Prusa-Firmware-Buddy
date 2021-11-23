#include "handler.h"

#include "../wui_REST_api.h"
#include "../wui_api.h"
#include "../wui.h"

// The get_* functions we use do exactly what we need, except they take fewer
// params. Generate the wrappers to throw them out without too much
// copy-pasting.
#define GET_WRAPPER(NAME)                                                                       \
    static void handler_##NAME(struct HttpHandlers *unused, char *buffer, size_t buffer_size) { \
        (void)unused;                                                                           \
        NAME(buffer, buffer_size);                                                              \
    }
GET_WRAPPER(get_printer);
GET_WRAPPER(get_version);
GET_WRAPPER(get_job);
GET_WRAPPER(get_files);
#undef GET_WRAPPER

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
    {}, // Sentinel
};

struct HttpHandlers default_http_handlers = {
    .gets = get_handlers,
    .api_key = wui_get_api_key,
    .listener_alloc = {
        .alloc = prusa_alloc,
    }
};
