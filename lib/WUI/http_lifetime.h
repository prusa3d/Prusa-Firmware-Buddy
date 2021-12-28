#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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
 * Re-create the httpd listening socket, with current network settings.
 *
 * In case networking is down (netdev_get_active_id() == NETDEV_NODEV_ID), it stops listening.
 *
 * Existing connections are left intact by this (though if the network
 * configuration changed, they would likely die on their own anyway).
 *
 * Thread safe.
 */
void httpd_reinit(void);

/**
 * Stop listening with the httpd server.
 *
 * This will shut down the listening socket, but leave the other connections (if any) intact.
 *
 * It can be re-enabled with @c httpd_reinit.
 */
void httpd_close(void);

#ifdef __cplusplus
}
#endif
