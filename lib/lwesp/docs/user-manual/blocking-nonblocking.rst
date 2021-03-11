.. _um_blocking_nonblocking:

Blocking or non-blocking API calls
==================================

API functions often allow application to set ``blocking`` parameter indicating if function shall be blocking or non-blocking.

Blocking mode
^^^^^^^^^^^^^

When the function is called in blocking mode ``blocking = 1``, application thread gets suspended until response from *ESP* device is received.
If there is a queue of multiple commands, thread may wait a while before receiving data.

When API function returns, application has valid response data and can react immediately.

* Linear programming model may be used
* Application may use multiple threads for real-time execution to prevent system stalling when running function call

.. warning::
	Due to internal architecture, it is not allowed to call API functions in *blocking mode* from events or callbacks.
	Any attempt to do so will result in function returning error.

Example code:

.. literalinclude:: ../examples_src/command_blocking.c
    :language: c
    :linenos:
    :caption: Blocking command example

Non-blocking mode
^^^^^^^^^^^^^^^^^

If the API function is called in non-blocking mode, function will return immediately with status indicating if command request has been successfully sent to internal command queue.
Response has to be processed in event callback function.

.. warning::
	Due to internal architecture, it is only allowed to call API functions in *non-blocking mode* from events or callbacks.
	Any attempt to do so will result in function returning error.

Example code:

.. literalinclude:: ../examples_src/command_nonblocking.c
    :language: c
    :linenos:
    :caption: Non-blocking command example

.. warning::
	When using non-blocking API calls, do not use local variables as parameter. 
	This may introduce *undefined behavior* and *memory corruption* if application function returns before command is executed.

Example of a bad code:

.. literalinclude:: ../examples_src/command_nonblocking_bad.c
    :language: c
    :linenos:
    :caption: Example of bad usage of non-blocking command

.. toctree::
    :maxdepth: 2
    :glob: