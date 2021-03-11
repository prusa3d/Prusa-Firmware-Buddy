.. _api_lwesp_sta:

Station API
===========

Station API is used to work with *ESP* acting in station mode.
It allows to join other access point, scan for available access points or simply disconnect from it.

An example below is showing how all examples (coming with this library) scan for access point and then
try to connect to AP from list of preferred one.

.. literalinclude:: ../../../snippets/station_manager.c
    :language: c
    :linenos:
    :caption: Station manager used with all examples

.. doxygengroup:: LWESP_STA