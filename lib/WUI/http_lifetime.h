#pragma once

/**
 * Initial set up of the httpd server.
 *
 * This may be called just once during the lifetime of the application. If the
 * networking is up at that point, it also starts to listen to incoming
 * connections.
 *
 * For further re-binds/re-initializations, see @c httpd_reinit.
 */
void httpd_init(void);

/**
 * Start the http server.
 *
 * If it's already started, it does nothing.
 */
void httpd_start(void);

/**
 * Stop listening with the httpd server.
 *
 * This will shut down the listening socket, but leave the other connections (if any) intact.
 *
 * It can be re-enabled with @c httpd_reinit.
 */
void httpd_close(void);

namespace nhttp {
struct Server;
}

nhttp::Server *httpd_instance();
