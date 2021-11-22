#pragma once

#include <stdlib.h>
#include <stdbool.h>

/*
 * The http handlers.
 *
 * These are the functions that provide the dynamic content. They are not
 * supposed to have any knowledge of the actual HTTP or its implementation as
 * far as possible.
 *
 * They are passed to the HTTP server to serve the content. This also allows to
 * plugging some mocks to do the server part.
 *
 * The server is initialized with a pointer to the HttpHandlers structure. The
 * ownership is _not_ passed to the server, and it is required the structure
 * lives at least as long as the server.
 *
 * All the handlers get the structure as their first parameter. This can be
 * used to pass additional data using the "C-style inheritance" pattern (well,
 * C++ inheritance would work  too). Note that the requests share the structure
 * and therefore this is not the right place for per-request scratch pad.
 *
 * The real application can use the `default_http_handlers` for the server.
 */

struct HttpHandlers;

/**
 * Function to handle a get request.
 *
 * For now we assume there are no parameters to the get requests and the
 * response depends only on the state of the printer.
 *
 * Furthermore, we assume the buffer will be large enough to encompass the
 * response; if not, the data shall be truncated. This is wrong, and we need
 * some way to stream large data without putting it all into RAM, but that's
 * going to happen in the future.
 */
typedef void get_handler(struct HttpHandlers *self, char *buffer, size_t buffer_size);

/**
 * Function to return current API key.
 *
 * In case it returns NULL, no authentication is possible and all access is denied.
 */
typedef const char *get_api_key();

/**
 * Descriptor of single GET endpoint.
 */
struct GetDescriptor {
    /// The URI this handles, eg. `/api/whatever`.
    const char *uri;
    /// The function to call.
    get_handler *handler;
    /// If set to true, authentication won't be checked for this request.
    bool anonymous : 1;
    /**
     * Do prefix match on the passed-in uri, not exact match.
     *
     * If set to true, the handler will also match URLs that have some suffix.
     * Eg. the `/api/whatever` handler will also handle `/api/whateverelse` or
     * `/api/whatever/something`.
     */
    bool prefix : 1;
};

struct HttpHandlers {
    /**
     * An array of GET handlers.
     *
     * Terminated by a sentinel ‒ single element with uri & handler set to NULL.
     *
     * The first one matching will be used. Therefore, it is possible to do
     * more exact matches first and than fall back to more generic things.
     */
    const struct GetDescriptor *gets;

    get_api_key *api_key;
};

/**
 * The default handlers that can be used for the real application.
 *
 * Tests may use their own mock set. Also note that the implementation lives in
 * a different file (handler_defaults.c).
 */
extern struct HttpHandlers default_http_handlers;

// TODO: Eventually turn this thing into C++, so they live "inside" the HttpHandlers.

/**
 * Look up the corresponding GET handler.
 *
 * Returns NULL in case it's not available.
 */
const struct GetDescriptor *http_handlers_find_get(const struct HttpHandlers *self, const char *uri);
