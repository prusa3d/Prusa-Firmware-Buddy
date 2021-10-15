/**
 * \page            page_appnote Application note
 * \tableofcontents
 *
 * \section         sect_about About architecture
 *
 * First objective is to make sure you understand this library and its purpose.
 *
 * Library supports ESP32 and ESP8266 wifi devices, manufactured by Espressif Systems.
 * These microcontrollers are generic flashless devices with wifi [and BLE in case of ESP32] connectivity. 
 * In order to operate properly, external flash device is necessary.
 *
 * Espressif [a company behind ESP32 and ESP8266] offers complete `RTOS-based SDK`, written in C language,
 * which you can download from their website and develop complete project for wifi [and BLE] using their microcontrollers `[case nr.1]`.
 *
 * Checking specs of these devices, these are not most powerful and feature-rich available on market,
 * still, they are becoming popular wifi transceivers and could be part of final application where another [more powerful] MCU is main CPU driving more powerful application `[case nr.2]`
 *
 * When application requires architecture `case nr.2`,
 * host MCU normally controls ESP device on demand [to transmit data, start MQTT, etc],
 * that is, ESP is configured in some sort of `MCU <-> Wifi` bridge.
 *
 * In order to speed-up development time, Espressif developed AT-commands firmware based on their SDK. This firmware is executed on Espressif device.
 * Host MCU [main, bigger CPU] needs to communicate with Espressif device hence library `ESP_AT_Lib` has been developed to decrease time-to-market.
 *
 * `ESP_AT_Lib` Library runs on host MCU and controls ESP device via UART/SPI/SDIO interfaces. ESP device executes AT firmware, which was specifically developed by Espressif Systems.
 * To remind you, Espressif offers RTOS-based SDK with C libraries. On top of that,
 * they prepared AT firmware so that you can send command like `AT+CIPSTART="..."` to start TCP connection with your favourite server.
 *
 * \note            If ESP devices runs AT firmware, host MCU needs to know what commands to send and how to parse response
 *
 * To be able to successfully run AT commands to ESP device and parse reply, `ESP_AT_Lib` has been developed and is still in development.
 *
 * Top-view of architecture
 *  
 *  - ESP device runs AT firmware. This can be downloaded from Espressif download section
 *  - Host MCU drives application + it runs `ESP_AT_Lib` firmware
 *  - Host MCU communicates to ESP via UART/SPI/SDIO interface
 *
 * \note            See \ref page_faq for information about supported AT firmware versions running on ESP devices
 *
 * \image html application_note_example_arch.svg Application example. ESP device runs official AT firmware while host MCU runs `ESP_AT_Lib` to interface with target device. ESP acts as Wifi bridge
 *
 * \section         sect_getting_started Getting started
 *
 * Repository <a href="https://github.com/MaJerle/ESP_AT_Lib"><b>ESP_AT_Lib is hosted on Github</b></a>. It combines source code and example projects.
 *
 * \subsection      sect_clone Clone repository
 *
 * \par             First-time clone
 *
 *  - Download and install `git` if not already
 *  - Open console and navigate to path in the system to clone repository to. Use command `cd your_path`
 *  - Run `git clone --recurse-submodules https://github.com/MaJerle/ESP_AT_Lib` command to clone repository including submodules
 *  - Navigate to `examples` directory and run favourite example
 *
 * \par             Already cloned, update to latest version
 *
 *  - Open console and navigate to path in the system where your resources repository is. Use command `cd your_path`
 *  - Run `git pull origin master --recurse-submodules` command to pull latest changes and to fetch latest changes from submodules
 *  - Run `git submodule foreach git pull origin master` to update & merge all submodules
 *
 * \section         sect_project_examples Example projects
 *
 * Several examples are available to show application use cases. These are split and can be tested on different systems.
 *
 * \subsection      sect_project_examples_win32 WIN32 examples
 *
 * Library is developed under WIN32 system. That is, all examples are first developed and tested under WIN32, later ported to embedded application.
 * Examples come with <b>Visual Studio</b> project. You may open project and directly run the example from there.
 *
 * \note            Visual Studio may set different configuration on first project load and this may lead to wrong build and possible errors.
 *                  Active configuration must be set to `Debug` and `Win32 or x86`. Default active build can be set in project settings.
 *
 * \par             NodeMCU development board
 *
 * For development purposes, `NodeMCU v3` board is used with virtual COM port support to translate USB communication to UART required for ESP8266.
 *
 * \warning         Some NodeMCU boards have `CH340 USB->UART` transceiver where I found problems with communication due to data loss between ESP and PC even at `115200` bauds.
 *                  Try to find NodeMCU with something else than CH340.
 *
 * \par             System functions for WIN32
 *
 * Required system functions are based on "windows.h" file, available on windows operating system. Natively, there is support for:
 * 
 *  - Timing functions
 *  - Semaphores
 *  - Mutexes
 *  - Threads
 *
 * The last part are message queues which are not implemented in Windows OS.
 * Message queues were developed with help of semaphores and dynamic memory allocatations.
 * System port for WIN32 is available in `src/system/esp_sys_win32.c` file.
 *
 * \par             Low-level communication between NodeMCU and WIN32
 *
 * Communication with NodeMCU hardware is using virtual files for COM ports.
 * Implementation of low-level part (together with memory allocation for library) is available in `src/system/esp_ll_win32.c` file.
 *
 * \note            In order to start using this port, user must set the appropriate COM port name when opening a virtual file. 
 *                  Please check implementation file for details.
 *
 * \subsection      sect_project_examples_arm_embedded ARM Cortex-M examples
 *
 * Library is independant from CPU architecture, meaning we can also run it on embedded systems. 
 * Different ports for `FreeRTOS` operating system and `STM32` based microcontrollers are available too.
 *
 *  <table>
 *      <caption>STM32 boards and pinouts for tests</caption>
 *      <tr><th>                        <th colspan="7">ESP target settings                         <th colspan="5">Debug settings
 *      <tr><th>Board name              <th>UART    <th>MTX <th>MRX <th>RST <th>GPI0<th>GPI2<th>CHPD<th>UART    <th>MDTX<th>MDRX<th>DBD     <th>Comment
 *      <tr><td>\b STM32F769I-Discovery <td>UART5   <td>PC12<td>PD2 <td>PJ14<td>-   <td>-   <td>-   <td>USART1  <td>PA9 <td>PA10<td>921600  <td>OBSTL
 *      <tr><td>\b STM32F723E-Discovery <td>UART5   <td>PC12<td>PD2 <td>PG14<td>-   <td>PD6 <td>PD3 <td>USART6  <td>PC6 <td>PC7 <td>921600  <td>OBSTL
 *      <tr><td>\b STM32L496G-Discovery <td>USART1  <td>PB6 <td>PG10<td>PB2 <td>PH2 <td>PA0 <td>PA4 <td>USART2  <td>PA2 <td>PD6 <td>921600  <td>OBSTL
 *      <tr><td>\b STM32L432KC-Nucleo   <td>USART1  <td>PA9 <td>PA10<td>PA12<td>PA7 <td>PA6 <td>PB0 <td>USART2  <td>PA2 <td>PA3 <td>921600  <td>OBSTL
 *      <tr><td>\b STM32F429ZI-Nucleo   <td>USART2  <td>PD5 <td>PD6 <td>PD1 <td>PD4 <td>PD7 <td>PD3 <td>USART3  <td>PD8 <td>PD9 <td>921600  <td>OBSTL
 *  </table>
 *
 *  - \b MTX: MCU TX pin, other device RX pin
 *  - \b MRX: MCU RX pin, other device TX pin
 *  - \b RST: Reset pin from ESP device, connected to MCU
 *  - \b GPI0: ESP GPIO0 pin, connected to MCU
 *  - \b GPI2: ESP GPIO0 pin, connected to MCU
 *  - \b CHPD: ESP CH PD pin, connected to MCU
 *  - \b MDTX: MCU Debug TX pin, other device RX pin
 *  - \b MDRX: MCU Debug RX pin, other device TX pin
 *  - \b DBD: Debug UART baudrate
 *  - \b OBSTL: On-Board ST-Link USB virtual COM port
 *
 * \note            All examples for STM32 come with ST's official free development studio STM32CubeIDE, available at st.com
 *
 * \section         sect_porting_guide Porting guide
 *
 * \subsection      sect_sys_arch System structure
 *
 * \image html system_structure.svg System structure organization
 *
 * We can describe library structure in `4` different layers:
 *
 *  - <b>User application</b>: User application is highest layer where entire code is implemented by user
 *      and where ESP AT library API functions are called from
 *
 *  - <b>ESP AT middleware</b>: ESP AT middleware layer consists of API functions,
 *      thread management functions and all utilities necessary for smooth operation.
 *
 *  - <b>System functions</b>: Layer where system dependant functions must be implemented,
 *      such as current time in milliseconds and all OS dependant functions for:
 *      - Managing threads
 *      - Managing semaphores
 *      - Managing mutexes
 *      - Managing message queues
 *
 *      More about this part can be found in \ref ESP_SYS section.
 *
 *  - <b>AT port communication functions</b> or <b>ESP LL</b>: Part of code where user must take care
 *      of sending and receiving data from/to ESP AT lib to properly handle communication between
 *      host device and ESP device.
 *      - User must assign memory for memory manager in this section.
 *          Check \ref ESP_MEM and \ref ESP_LL sections for more information.
 *
 *      More about this part can be found in \ref ESP_LL section.
 *      
 *      Together with this section, user must implement part to input the received data from AT port.
 *
 *  - <b>ESP physical device</b>: Actual ESP8266 or ESP32 device
 *
 * \subsection      sect_port_implementation Implementation specific part
 *
 * Before usage, user must implement all functions in \ref ESP_LL section
 * as well as take care of proper communication with ESP device in \ref ESP_LL section.
 *
 * \note            For more information about how to port, check sections accordingly
 *
 * \section         sect_config Library configuration
 *
 * Different configuration options are available, which increases possible efficiency of the library
 * based on user requirement for final application.
 *
 * A list of all configurations can be found in \ref ESP_CONFIG section.
 *
 * \subsection      sect_conf_file Project configuration file
 *
 * Library comes with `2` configuration files:
 *
 *  - Default configuration file `esp_config_default.h`
 *  - Project template configuration file `esp_config_template.h`
 *
 * When project is started, user has to rename template file to `esp_config.h`
 * and if required, it should override default settings in this file.
 *
 * Default template file comes with something like this:
 *
 * \include         _example_config_template.h
 *
 * If bigger buffer is required, modification must be made like following:
 *
 * \include         _example_config.h
 *
 * \note            Always modify default settings by overriding them in user's custom `esp_config.h` file
 *                      which was previously renamed from `esp_config_template.h`
 *
 * \section         sect_thread_comm Inter-thread communication
 *
 * In order to have very effective library from resources point of view,
 * an inter-thread communication was introduced.
 *
 * \image html thread_communication.svg Inter-Thread communication between user and library.
 *
 * Library consists of 2 threads working in parallel and bunch of different user threads.
 *
 * \subsection      sect_thread_user User thread(s)
 *
 * User thread is a place where user communicates with ESP AT library.
 * When a new command wants to be executed to ESP device, user calls appropriate API function which will do following steps:
 *
 *  - Allocate memory for command message from memory manager
 *  - Assign command type to message
 *  - Set other parameters, related or required to command
 *  - If user wants to wait for response (blocking mode), then create system semaphore `sem` and lock it immediatelly
 *  - Send everything to producing message queue which is later read in producing thread
 *  - If user don't want blocking mode, return from function with status OK
 *      otherwise wait for semaphore `sem` to be released from producing thread
 *      - When `sem` semaphore is locked, user thread may sleep and release resources for other threads this time
 *  - If user selects blocking mode, wait for response, free command memory in memory manager and return command response status
 *
 * User may use different threads to communicate with ESP AT lib at the same time since memory manager
 * is natively protected by mutex and producing queue is protected from multiple accesses by OS natively.
 *
 * \subsection      subsec_thread_producer Producer thread
 *
 * Producer threads reads message queue with user commands and sends initial AT command to AT port.
 * When there is no commands from user, thread can sleep waiting for new command from user.
 *
 * Once there is a command read from message queue, these steps are performed:
 *
 *  - Check if processing function is set and if command is valid
 *  - Locks `sync_sem` semaphore for synchronization between processing and producing threads
 *  - Sends initial AT command to AT port according to command type
 *  - Waits for `sync_sem` to be ready again (released in processing thread)
 *  - If command was blocking, set result and unlock command `sem` semaphore
 *  - If command was not blocking, free command memory from memory manager
 *
 * \subsection      subsec_thread_process Process thread
 *
 * Processing thread reads received data from AT port and processes them.
 *
 * If command is active and received data belongs to command, they are processed according to command.
 * If received data are not related to command (such as received network data `+IPD`),
 * they are also processed and callback function is immediatelly called to notify user about received data.
 *
 * Here is a list of some URC (Unsolicited Result Code) messages:
 *  
 *  - Received network data `+IPD`
 *  - Connection just active `+LINK_CONN`
 *  - Station disconnected from access point `WIFI DISCONNECT`
 *  - Station connected to access point `WIFI CONNECTED`
 *  - ...
 *
 * All these commands must be reported to user. To do this, callback is triggered to notify user.
 *
 * \section         sect_events_and_callbacks Events and callback functions
 *
 * To make library very efficient, events and callback functions are implemented. They are separated in different groups.
 *
 * \subsection      sect_events_and_callbacks_global Global event function
 *
 * This is a callback function for all implemented major events, except events related to \ref ESP_CONN.
 * User may implement all cases from \ref esp_evt_type_t enumeration except those which start with `ESP_CONN_`.
 *
 * This callback is set on first stack init using \ref esp_init function.
 * If later application needs more event functions to receive all events,
 * user may register/unregister new functions using \ref esp_evt_register and \ref esp_evt_unregister respectively.
 *
 * Events which can be implemented:
 *  - WiFi connected/disconnected
 *  - Device reset detected
 *  - New station connected to access point
 *  - ... 
 *
 * \subsection      sect_events_and_callbacks_connection Connection event function
 *
 * To optimize application related to connections and to allow easier implementation of different modules,
 * each connection (started as \em client or \em server) has an option to implement custom callback function for connection related events.
 *
 * User may implement all cases from \ref esp_evt_type_t enumeration which start with `ESP_CONN_`.
 *
 * Callback function is set when connection is started as client using \ref esp_conn_start or
 * set on \ref esp_set_server function when enabling server connections
 *
 * Events which can be implemented:
 *  - Connection active
 *  - Connection data received
 *  - Connection data sent
 *  - Connection closed
 *  - ...
 *
 * \subsection      sect_events_and_callbacks_temporary Temporary event for API functions
 *
 * When API function (ex. \ref esp_hostname_set) directly interacts with device using AT commands,
 * user has an option to set callback function and argument when command finishes.
 *
 * This feature allows application to optimize upper layer implementation when needed
 * or when command is executed as non-blocking API call. Read sect_block_nonblock_commands section for more information
 *
 * \section         sect_block_nonblock_commands Blocking and non-blocking commands
 *
 * When API function needs to interact with device directly before valid data on return,
 * user has an option to execute it in blocking or non-blocking mode.
 *
 * \subsection      sect_blocking Blocking mode
 *
 * In blocking mode, function will block thread execution until response is received
 * and ready for further processing. When the function returns, user has known result from ESP device.
 * 
 *  - Linear programming style may be applied when in thread
 *  - User may need to use multiple threads to execute multiple features in real-time
 *
 * \par 			Example code
 *
 * \include         _example_command_blocking.c
 *
 * \note 			It is not allowed to call API function in blocking mode from other ESP event functions.
 *                  Any attempt to do so will result in function returning \ref espERRBLOCKING.
 *
 * \subsection      sect_nonblocking Non-blocking mode
 *
 * In non-blocking mode, command is created, sent to producing message queue and function returns without waiting for response from device.
 * This mode does not allow linear programming style, because after non-blocking command, callback function is called.
 *
 * Full example for connections API can be found in \ref ESP_CONN section.
 *
 * \par             Example code
 *
 * \include         _example_command_nonblocking.c
 *
 * \note            When calling API functions from any event function, it is not allowed to use \b blocking mode.
 *                  Any attempt to do so will result in function returning \ref espERRBLOCKING.
 */