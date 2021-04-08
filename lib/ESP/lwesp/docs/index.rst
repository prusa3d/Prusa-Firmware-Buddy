LwESP |version| documentation
=============================

Welcome to the documentation for version |version|.

LwESP is generic, platform independent, ESP-AT parser library to communicate with *ESP8266* or *ESP32* WiFi-based microcontrollers from *Espressif systems* using official AT Commands set running on ESP device.
Its objective is to run on master system, while Espressif device runs official AT commands firmware developed and maintained by *Espressif systems*.

.. image:: static/images/logo.svg
    :align: center

.. rst-class:: center
.. rst-class:: index_links

	:ref:`download_library` :ref:`getting_started` `Open Github <https://github.com/MaJerle/lwesp>`_ `Donate <https://paypal.me/tilz0R>`_

Features
^^^^^^^^

* Supports latest ESP8266 and ESP32 RTOS-SDK AT commands firmware
* Platform independent and easy to port, written in C99

  * Library is developed under Win32 platform
  * Provided examples for ARM Cortex-M or Win32 platforms

* Allows different configurations to optimize user requirements
* Optimized for systems with operating systems (or RTOS)

  * Currently only OS mode is supported
  * ``2`` different threads to process user inputs and received data

    * Producer thread to collect user commands from application threads and to start command execution
    * Process thread to process received data from *ESP* device

* Allows sequential API for connections in client and server mode
* Includes several applications built on top of library

  * HTTP server with dynamic files (file system) support
  * MQTT client for MQTT connection
  * MQTT client Cayenne API for Cayenne MQTT server

* Embeds other AT features, such as WPS
* User friendly MIT license

Requirements
^^^^^^^^^^^^

* C compiler
* *ESP8266* or *ESP32* device with running AT-Commands firmware

Contribute
^^^^^^^^^^

Fresh contributions are always welcome. Simple instructions to proceed:

#. Fork Github repository
#. Respect `C style & coding rules <https://github.com/MaJerle/c-code-style>`_ used by the library
#. Create a pull request to ``develop`` branch with new features or bug fixes

Alternatively you may:

#. Report a bug
#. Ask for a feature request

License
^^^^^^^

.. literalinclude:: ../LICENSE

Table of contents
^^^^^^^^^^^^^^^^^

.. toctree::
    :maxdepth: 2

    self
    get-started/index
    user-manual/index
    api-reference/index
    examples/index
