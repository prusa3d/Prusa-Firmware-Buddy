.. _um_porting_guide:

Porting guide
=============

High level of *ESP-AT* library is platform independent, written in ANSI C99,
however there is an important part where middleware needs to communicate with target *ESP* device
and it must work under different optional operating systems selected by final customer.

Porting consists of:

* Implementation of *low-level* part, for actual communication between host device and *ESP* device
* Implementation of system functions, link between target operating system and middleware functions
* Assignment of memory for allocation manager

Implement low-level driver
^^^^^^^^^^^^^^^^^^^^^^^^^^

To successfully prepare all parts of *low-level* driver, application must take care of:

* Implementing :cpp:func:`lwesp_ll_init` and :cpp:func:`lwesp_ll_deinit` callback functions
* Implement and assign *send data* and optional *hardware reset* function callbacks
* Assign memory for allocation manager when using default allocator or use custom allocator
* Process received data from *ESP* device and send it to input module for further processing

.. tip::
    Port examples are available for STM32 and WIN32 architectures.
    Both actual working and up-to-date implementations are available within the library.

.. note::
    Check :ref:`api_lwesp_input` for more information about direct & indirect input processing.

Implement system functions
^^^^^^^^^^^^^^^^^^^^^^^^^^

System functions are bridge between operating system calls and *ESP* middleware.
*ESP* library relies on stable operating system features and its implementation and
does not require any special features which do not normally come with operating systems.

Operating system must support:

* Thread management functions
* Mutex management functions
* Binary semaphores only, no need for counting semaphores
* Message queue management functions

.. warning::
    If any of the features are not available within targeted operating system,
    customer needs to resolve it with care. As an example, message queue is not available
    in WIN32 OS API therefore custom message queue has been implemented using binary semaphores

Application needs to implement all system call functions, starting with ``lwesp_sys_``.
It must also prepare header file for standard types in order to support OS types within *ESP* middleware.

An example code is provided latter section of this page for WIN32 and STM32.

Steps to follow
***************

* Copy ``lwesp/src/system/lwesp_sys_template.c`` to the same folder and rename it to application port, eg. ``lwesp_sys_win32.c``
* Open newly created file and implement all system functions
* Copy folder ``lwesp/src/include/system/port/template/*`` to the same folder and rename *folder name* to application port, eg. ``cmsis_os``
* Open ``lwesp_sys_port.h`` file from newly created folder and implement all *typedefs* and *macros* for specific target
* Add source file to compiler sources and add path to header file to include paths in compiler options

.. note::
    Check :ref:`api_lwesp_sys` for function prototypes.

Example: Low-level driver for WIN32
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Example code for low-level porting on `WIN32` platform.
It uses native *Windows* features to open *COM* port and read/write from/to it.

Notes:

* It uses separate thread for received data processing.
  It uses :cpp:func:`lwesp_input_process` or :cpp:func:`lwesp_input` functions, based on application configuration of :c:macro:`LWESP_CFG_INPUT_USE_PROCESS` parameter.

  * When :c:macro:`LWESP_CFG_INPUT_USE_PROCESS` is disabled, dedicated receive buffer is created by *ESP-AT* library
    and :cpp:func:`lwesp_input` function just writes data to it and does not process received characters immediately.
    This is handled by *Processing* thread at later stage instead.
  * When :c:macro:`LWESP_CFG_INPUT_USE_PROCESS` is enabled, :cpp:func:`lwesp_input_process` is used,
    which directly processes input data and sends potential callback/event functions to application layer.

* Memory manager has been assigned to ``1`` region of ``LWESP_MEM_SIZE`` size
* It sets *send* and *reset* callback functions for *ESP-AT* library

.. literalinclude:: ../../lwesp/src/system/lwesp_ll_win32.c
    :language: c
    :linenos:
    :caption: Actual implementation of low-level driver for WIN32

Example: Low-level driver for STM32
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Example code for low-level porting on `STM32` platform.
It uses `CMSIS-OS` based application layer functions for implementing threads & other OS dependent features.

Notes:

* It uses separate thread for received data processing.
  It uses :cpp:func:`lwesp_input_process` function to directly process received data without using intermediate receive buffer
* Memory manager has been assigned to ``1`` region of ``LWESP_MEM_SIZE`` size
* It sets *send* and *reset* callback functions for *ESP-AT* library

.. literalinclude:: ../../lwesp/src/system/lwesp_ll_stm32.c
    :language: c
    :linenos:
    :caption: Actual implementation of low-level driver for STM32

Example: System functions for WIN32
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../lwesp/src/include/system/port/win32/lwesp_sys_port.h
    :language: c
    :linenos:
    :caption: Actual header implementation of system functions for WIN32

.. literalinclude:: ../../lwesp/src/system/lwesp_sys_win32.c
    :language: c
    :linenos:
    :caption: Actual implementation of system functions for WIN32

Example: System functions for CMSIS-OS
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../lwesp/src/include/system/port/cmsis_os/lwesp_sys_port.h
    :language: c
    :linenos:
    :caption: Actual header implementation of system functions for CMSIS-OS based operating systems

.. literalinclude:: ../../lwesp/src/system/lwesp_sys_cmsis_os.c
    :language: c
    :linenos:
    :caption: Actual implementation of system functions for CMSIS-OS based operating systems

.. toctree::
    :maxdepth: 2
    :glob: