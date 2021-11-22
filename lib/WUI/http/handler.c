#include "handler.h"

#include <string.h>

const struct GetDescriptor *http_handlers_find_get(const struct HttpHandlers *self, const char *uri) {
    for (const struct GetDescriptor *h = self->gets; h->uri != NULL; h++) {
        if (h->prefix) {
            if (strncmp(uri, h->uri, strlen(h->uri)) == 0) {
                return h;
            }
        } else {
            if (strcmp(uri, h->uri) == 0) {
                return h;
            }
        }
    }

    return NULL;
}
