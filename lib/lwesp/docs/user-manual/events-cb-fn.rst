.. _um_events_cb_fn:

Events and callback functions
=============================

Library uses events to notify application layer for (possible, but not limited to) unexpected events.
This concept is used aswell for commands with longer executing time, such as *scanning access points* or when application starts new connection as client mode.

There are ``3`` types of events/callbacks available:

* *Global event* callback function, assigned when initializing library
* *Connection specific event* callback function, to process only events related to connection, such as *connection error*, *data send*, *data receive*, *connection closed*
* *API function* call based event callback function

Every callback is always called from protected area of middleware (when exclusing access is granted to single thread only),
and it can be called from one of these ``3`` threads:

- *Producing thread*
- *Processing thread*
- *Input thread*, when :c:macro:`LWESP_CFG_INPUT_USE_PROCESS` is enabled and :cpp:func:`lwesp_input_process` function is called

.. tip::
    Check :ref:`um_inter_thread_comm` for more details about *Producing* and *Processing* thread.

Global event callback
^^^^^^^^^^^^^^^^^^^^^

Global event callback function is assigned at library initialization.
It is used by the application to receive any kind of event, except the one related to connection:

* ESP station successfully connected to access point
* ESP physical device reset has been detected
* Restore operation finished
* New station has connected to access point
* and many more..

.. tip::
    Check :ref:`api_lwesp_evt` section for different kind of events

By default, global event function is single function.
If the application tries to split different events with different callback functions,
it is possible to do so by using :cpp:func:`lwesp_evt_register` function to register a new,
custom, event function.

.. tip::
    Implementation of :ref:`api_app_netconn` leverages :cpp:func:`lwesp_evt_register` to
    receive event when station disconnected from wifi access point.
    Check its source file for actual implementation.

.. literalinclude:: ../../lwesp/src/api/lwesp_netconn.c
    :language: c
    :linenos:
    :caption: Netconn API module actual implementation

Connection specific event
^^^^^^^^^^^^^^^^^^^^^^^^^

This events are subset of global event callback.
They work exactly the same way as global, but only receive events related to connections.

.. tip::
    Connection related events start with ``LWESP_EVT_CONN_*``, such as :c:member:`LWESP_EVT_CONN_RECV`.
    Check :ref:`api_lwesp_evt` for list of all connection events.

Connection events callback function is set for ``2`` cases:

* Each client (when application starts connection) sets event callback function when trying to connect with :cpp:func:`lwesp_conn_start` function
* Application sets global event callback function when enabling server mode with :cpp:func:`lwesp_set_server` function

.. literalinclude:: ../../snippets/client.c
    :language: c
    :linenos:
    :caption: An example of client with its dedicated event callback function

API call event
^^^^^^^^^^^^^^

API function call event function is special type of event and is linked to command execution.
It is especially useful when dealing with non-blocking commands to understand when specific
command execution finished and when next operation could start.

Every API function, which directly operates with AT command on physical device layer,
has optional ``2`` parameters for API call event:

* Callback function, called when command finished
* Custom user parameter for callback function

Below is an example code for DNS resolver.
It uses custom API callback function with custom argument,
used to distinguis domain name (when multiple domains are to be resolved).

.. literalinclude:: ../../snippets/dns.c
    :language: c
    :linenos:
    :caption: Simple example for API call event, using DNS module

.. toctree::
    :maxdepth: 2
    :glob: