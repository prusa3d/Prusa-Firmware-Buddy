#include "http/handler.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The default handlers that can be used for the real application.
 *
 * Tests may use their own mock set.
 */
extern struct HttpHandlers default_http_handlers;

#ifdef __cplusplus
}
#endif
