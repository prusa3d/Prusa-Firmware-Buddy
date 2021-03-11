.. _api_lwesp_conn:

Connections
===========

Connections are essential feature of WiFi device and middleware.
It is developed with strong focus on its performance and since it may interact with huge amount of data,
it tries to use zero-copy (when available) feature, to decrease processing time.

*ESP AT Firmware* by default supports up to ``5`` connections being active at the same time and supports:

* Up to ``5`` TCP connections active at the same time
* Up to ``5`` UDP connections active at the same time
* Up to ``1`` SSL connection active at a time

.. note::
    Client or server connections are available.
    Same API function call are used to send/receive data or close connection.

Architecture of the connection API is using callback event functions.
This allows maximal optimization in terms of responsiveness on different kind of events.

Example below shows *bare minimum* implementation to:

* Start a new connection to remote host
* Send *HTTP GET* request to remote host
* Process received data in event and print number of received bytes

.. literalinclude:: ../../../snippets/client.c
    :language: c
    :linenos:
    :caption: Client connection minimum example

Sending data
^^^^^^^^^^^^

Receiving data flow is always the same. Whenever new data packet arrives, corresponding event is called to notify application layer.
When it comes to sending data, application may decide between ``2`` options (*this is valid only for non-UDP connections):

* Write data to temporary transmit buffer
* Execute *send command* for every API function call

Temporary transmit buffer
*************************

By calling :cpp:func:`lwesp_conn_write` on active connection, temporary buffer is allocated and input data are copied to it.
There is always up to ``1`` internal buffer active. When it is full (or if input data length is longer than maximal size),
data are immediately send out and are not written to buffer.

*ESP AT Firmware* allows (current revision) to transmit up to ``2048`` bytes at a time with single command.
When trying to send more than this, application would need to issue multiple *send commands* on *AT commands level*.

Write option is used mostly when application needs to write many different small chunks of data.
Temporary buffer hence prevents many *send command* instructions as it is faster to send single command with big buffer,
than many of them with smaller chunks of bytes.

.. literalinclude:: ../../examples_src/conn_write.c
    :language: c
    :linenos:
    :caption: Write data to connection output buffer

Transmit packet manually
************************

In some cases it is not possible to use temporary buffers,
mostly because of memory constraints.
Application can directly start *send data* instructions on *AT* level by using :cpp:func:`lwesp_conn_send` or :cpp:func:`lwesp_conn_sendto` functions.

.. doxygengroup:: LWESP_CONN