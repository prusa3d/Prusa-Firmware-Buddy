.. _api_lwesp_input:

Input module
============

Input module is used to input received data from *ESP* device to *LwESP* middleware part.
``2`` processing options are possible:

* Indirect processing with :cpp:func:`lwesp_input` (default mode)
* Direct processing with :cpp:func:`lwesp_input_process`

.. tip::
    Direct or indirect processing mode is select by setting :c:macro:`LWESP_CFG_INPUT_USE_PROCESS` configuration value.

Indirect processing
^^^^^^^^^^^^^^^^^^^

With indirect processing mode, every received character from *ESP* physical device is written to
intermediate buffer between low-level driver and *processing* thread.

Function :cpp:func:`lwesp_input` is used to write data to buffer, which is later processed
by *processing* thread.

Indirect processing mode allows embedded systems to write received data to buffer from interrupt context (outside threads).
As a drawback, its performance is decreased as it involves copying every receive character to intermediate buffer,
and may also introduce RAM memory footprint increase.

Direct processing
^^^^^^^^^^^^^^^^^

Direct processing is targeting more advanced host controllers, like STM32 or WIN32 implementation use.
It is developed with DMA support in mind, allowing low-level drivers to skip intermediate data buffer
and process input bytes directly.

.. note::
	When using this mode, function :cpp:func:`lwesp_input_process` must be used and it may
	only be called from thread context. Processing of input bytes is done in low-level
	input thread, started by application.

.. tip::
	Check :ref:`um_porting_guide` for implementation examples.

.. doxygengroup:: LWESP_INPUT