/**
 * \addtogroup      ESP_NETCONN
 * \{
 *
 * Netconn provides sequential API to work with connection 
 * in either server or client mode.
 *
 * Netconn API can handle reading asynchronous network data in synchronous way
 * by using operating system features such as message queues 
 * and by putting thread into blocking mode, it allows zero overhead 
 * from performance point of view.
 * 
 * \section         sect_netconn_client Netconn client
 *
 * \image html netconn_client.svg Netconn API architecture
 *
 * Every netconn structure consists of at least data message queue 
 * to handle received packets before user reads them in user thread.
 *
 * On image we can see blue box showing connection handle data queue.
 * Data queue is filled from connection callback which is dedicated specially for
 * netconn type connections.
 *
 * When user wants to read data from connection,
 * thread will be blocked until something is available in received data message queue.
 *
 * To allow user to handle closed connections while waiting for more data,
 * information about closed connection is also added to received data message queue.
 *
 * \par             Example code
 * 
 * Example code shows how to use netconn API to write and read data in synchronous way,
 * no need to have complex code structure for asynchronous data reception callbacks
 *
 * \include         _example_netconn_client.c
 *
 * \section         sect_netconn_server Netconn server
 *
 * Netconn API allows implementation of server in similar way like client mode.
 *
 * In addition to client, some additional steps must be included:
 *
 *  - Connection must be set to listening mode
 *  - Connection must wait and accept new client
 *
 * \image html netconn_server.svg Server mode netconn architecture
 *
 * If netconn API is used as server mode, accept message queue is introduced.
 * This message queue handles every new connection which is active on
 * dedicated port for server mode we are listening on.
 *
 * When a new client connects, new fresh client structure is created,
 * and put to server's accept message queue. This structure is later used
 * to write received data to it, so when user accepts a connection,
 * it may already have some data to read immediately.
 *
 * Once new client is received with \ref esp_netconn_accept function,
 * control is given to client object which can be later
 * read and written in the same way as client mode.
 * 
 * \par             Example code
 *
 * \include         _example_netconn_server.c
 *
 * \section         sect_netconn_server_thread Netconn server concurrency
 *
 * An improved version for netconn server is to use separate thread for every client.
 * This add new option to process multiple clients concurrent instead of consecutive.
 * Possible drawback is higher RAM usage due to multiple threads (each client own thread),
 * but this makes communication much stable.
 *
 * \par             Example code
 *
 * An example shows single netconn thread which waits for new client.
 * When new client is accepted, it will initiate a new thread with client handle and will immediatelly wait for next client,
 * instead of waiting to process current client.
 *
 * \include         _example_netconn_server_threads.c
 *
 * \section         sect_netconn_nonblocking Non-blocking receive
 *
 * \ref esp_netconn_receive data will block thread by default until connection data received or connection was closed by remote side.
 * In order to allow further processing even if there is no data, you can enable non-blocking support for netconn receive.
 *
 * \note            \ref ESP_CFG_NETCONN_RECEIVE_TIMEOUT must be set to 1 to use this feature
 *
 * Even if feature is enabled, netconn will (by default) still block until data are ready or connection closed.
 * In order to enable non-blocking feature, you have to set maximal timeout value function may block to receive the data, using \ref esp_netconn_set_receive_timeout.
 *
 * This allows you to set different timeout values for different netconns.
 *
 * \note            When the feature is enabled, \ref esp_netconn_receive may return \ref espTIMEOUT to notify user about status
 *
 * \}
 */