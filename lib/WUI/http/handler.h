#pragma once

#include <stdlib.h>
#include <stdbool.h>

#include <lwip/altcp.h>

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
 *
 * @return Numeric HTTP status code (eg. 200).
 */
typedef uint16_t get_handler(struct HttpHandlers *self, char *buffer, size_t buffer_size);

/**
 * Handlers of the GCODE upload.
 *
 * Few notes:
 *
 * * This is the wrong level of abstraction, but for now, we are following the
 *   current code. Eventually, we'll have to come up with some kind of generic
 *   post handling callback, so the fact we have some GCODE upload endpoint
 *   doesn't leak into the HTTP server. This will hopefully follow from
 *   replacement of the HTTP server implementation.
 * * The API does _not_ currently support multiple parallel uploads. The HTTP
 *   server currently refuses attempts to do so.
 * * Return HTTP error codes for error and 0 for success.
 * * The finish may be called without a filename (final_filename = NULL) as an
 *   indication of an aborted upload. The data did not finish and shall not be
 *   stored.
 */
typedef uint16_t gcode_handler_start(struct HttpHandlers *self, const char *filename);
// FIXME: const char *data is probably wrong, it should be const uint8_t * as arbitrary data; but everything around rigth now uses char :-(
typedef uint16_t gcode_handler_data(struct HttpHandlers *self, const char *data, size_t len);
typedef uint16_t gcode_handler_finish(struct HttpHandlers *self, const char *tmp_filename, const char *final_filename, bool start_print);

/**
 * Function to return current API key.
 *
 * In case it returns NULL, no authentication is possible and all access is denied.
 */
typedef const char *get_api_key();

/**
 * Function to look up a corresponding file/uri/handler page for given error
 * code.
 *
 * This is a hack, as the current http server has very weird handling of error
 * codes.
 *
 * The result needs to point to static memory (not get deallocated).
 */
typedef const char *code_to_uri(struct HttpHandlers *self, uint16_t code);

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

    /**
     * Allocator used for getting the listening socket.
     *
     * This ties us into specific server implementation for now. This hopefully
     * will go away once we have proper socket abstraction and a proper HTTP
     * server.
     */
    altcp_allocator_t listener_alloc;

    gcode_handler_start *gcode_start;
    gcode_handler_data *gcode_data;
    gcode_handler_finish *gcode_finish;

    code_to_uri *code_lookup;
};

// TODO: Eventually turn this thing into C++, so they live "inside" the HttpHandlers.

/**
 * Look up the corresponding GET handler.
 *
 * Returns NULL in case it's not available.
 */
const struct GetDescriptor *http_handlers_find_get(const struct HttpHandlers *self, const char *uri);
