.. _api_lwesp_timeout:

Timeout manager
===============

Timeout manager allows application to call specific function at desired time.
It is used in middleware (and can be used by application too) to poll active connections.

.. note::
    Callback function is called from *processing* thread.
    It is not allowed to call any blocking API function from it.

When application registers timeout, it needs to set timeout, callback function and optional user argument.
When timeout elapses, ESP middleware will call timeout callback.

This feature can be considered as single-shot software timer.

.. doxygengroup:: LWESP_TIMEOUT