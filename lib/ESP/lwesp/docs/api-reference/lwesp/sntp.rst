.. _api_lwesp_sntp:

Simple Network Time Protocol
============================

ESP has built-in support for *Simple Network Time Protocol (SNTP)*.
It is support through middleware API calls for configuring servers and reading actual date and time.

.. literalinclude:: ../../../snippets/sntp.c
    :language: c
    :linenos:
    :caption: Minimum SNTP example

.. doxygengroup:: LWESP_SNTP
