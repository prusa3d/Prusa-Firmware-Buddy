/**
 * \addtogroup      ESP_LL
 * \{
 *
 * Low-level communication part is responsible to make sure
 * all bytes received from ESP device are properly 
 * sent to upper layer stack and that all bytes are sent to ESP
 * when requested by upper layer ESP stack
 *
 * When initializing low-level part, following steps are important and must be done when \ref esp_ll_init function is called:
 *
 *  1. Assign memory for dynamic allocations required by ESP library
 *  2. Configure AT send function to use when we have data to be transmitted
 *  3. Configure AT port to be able to send/receive any data
 *
 * \par             Example code
 * 
 * Example shows basic functionality what user MUST do in order to prepare stack properly.
 *
 * \include         _example_ll.c
 *
 * If host device (usually microcontroller) does not embed enough memory, it is possible to use external memory-mapped RAM.
 *
 * \note            External RAM must be addressable the same way as internal one.
 *                  Usually special hardware must be available in microcontroller to handle communication with memory
 *
 * \par             Example code with external memory
 *
 * \include         _example_ll_ext_mem.c
 *
 * \par             Example code with external memory for STM32F429-Discovery board
 *
 * This is specific example for STM32F429-Discovery board. Board has `64-Mbit` external memory.
 * It is connected via FMC port and is addressable in the memory:
 *
 *  - `64-Mbit` is equal to `8-MBytes` of memory.
 *  - `8-Mbytes` is equal to `8388608` or `0x800000` bytes
 *  - External memory on the board starts at memory `0xD0000000` and is available until `0xD0000000 + 0x800000 - 1`
 *      - Read reference manual of FMC hardware for STM32F429 for more information about `0xD0000000` address
 *      - When application tries to write with `*(volatile uint8_t *)(0xD0000000) = 0x12`, first byte in external memory is written to `0x12`
 *      - When application tries to write with `*(volatile uint8_t *)(0xD0000000 + 0x800000 - 1) = 0x21`, last byte in external memory is written to `0x21`
 *
 * \include         _example_ll_ext_mem_stm32f429_discovery.c
 *
 * \section         sect_input_process Input module
 *
 * Input module is a way how to send received data from AT port
 * to middleware layer where each received byte is processed 
 * to construct valid response from device.
 *
 * It must be solved with caution not to miss any byte or process it wrongly.
 *
 * `2` different input methods are available:
 *  - Writing each byte to receive buffer, which is later read by middleware layer in processing thread
 *  - Direct call of processing buffer (available only when OS system is used and requires separate thread)
 *
 * \note            The best case to implement this part is in \ref ESP_LL module file
 *
 * \section         sect_input_method_1 Receive ring buffer
 *
 * Method 1 is always available (with or without OS) and allows user to write single received byte
 * to input buffer, which is later read and processed in processing thread.
 *
 * The size of received buffer is set with \ref ESP_CFG_RCV_BUFF_SIZE configuration parameter.
 * If you are expecting high data rates at high speeds, make this value as bigger as possible.
 * The memory for buffer will be allocated from memory manager, so make sure you have enough memory
 * in memory manager. Take a look at \ref ESP_MEM section for more information
 *
 * \note            Use this method if you want to use RX interrupt on microcontroller for AT port
 *
 * \par             Example code with RX interrupt writing to receive buffer
 *
 * \include         _example_input_rx_irq.c
 *
 * \section         sect_input_method_2 Process data directly from receive thread
 *
 * When this usage is applied, separate thread must be introduce which only 
 * reads the received data from AT port and calls \ref esp_input_process function directly.
 * 
 * In this mode, receive buffer is not implemented but it is reponsibility of 
 * user to construct linear buffer and send it to upper layer.
 *
 * \note            To use this mode, \ref ESP_CFG_INPUT_USE_PROCESS must be enabled.
 *                  When this mode is enabled, all callbacks are called from this thread to user layer.
 *
 * \note            This mode is perfectly suitable if target host device allows you to use DMA
 *                  (Direct Memory Access) to read data from AT port.
 *
 * \include         _example_input_rx_dma.c
 *
 * \}
 */