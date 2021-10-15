/**
 * \addtogroup      ESP_CONN
 * \{
 *
 * Connection based functions to manage sending and receiving data
 *
 * \note            Functions are thread safe. If there is an expection, it is mentioned in function's description.
 *
 * In the below example, you can find frequent use case how to use connection API in non-blocking callback mode.
 *
 * \par             Example code
 *
 * In this example, most useful non-blocking approach is used to handle the connection.
 *
 * \include         _example_conn_default.c
 *
 * \section         sect_send_data Send data methods
 *
 * User can choose between `2` different methods for sending the data:
 *
 *  - Temporary connection write buffer
 *  - Send every packet separately
 *
 * \par             Temporary connection write buffer
 *
 * When user calls \ref esp_conn_write function,
 * a temporary buffer is created on connection and data are copied to it,
 * but they might not be send to command queue for sending.
 *
 * ESP can send up to `x` bytes at a time in single AT command,
 * currently limited to `2048` bytes.
 * If we can optimize packets of `2048` bytes,
 * we would have best throughput speed and this is the purpose of write function.
 *
 * \note            If we write bigger amount than max data for packet
 *                  function will automaticaly split data to make sure,
 *                  everything is sent in correct order
 *
 * \include         _example_conn_write.c
 *
 * \par             Send every packet separately
 *
 * If you are not able to use write buffer,
 * due to memory constraints, you may also send data
 * by putting every write command directly to command message queue.
 *
 * Use \ref esp_conn_send or \ref esp_conn_sendto functions,
 * to send data directly to message queue.
 *
 * \}
 */