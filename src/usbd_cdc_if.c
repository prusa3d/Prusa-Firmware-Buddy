#include "usbd_cdc_if.h"
#include "usb_device.h"

/// Whether the CDC device is initialized
static bool usbd_cdc_initialized = false;

static void (*cdc_receive_fn)(uint8_t *buffer, uint32_t length);

/// Received data over USB are stored in this buffer
static uint8_t rx_buffer[USBD_CDC_RX_DATA_SIZE];

/// Data to send over USB CDC are stored in this buffer
static uint8_t tx_buffer[USBD_CDC_TX_DATA_SIZE];

static int8_t cdc_init_fs();
static int8_t cdc_deinit_fs();
static int8_t cdc_control_fs(uint8_t cmd, uint8_t *buffer, uint16_t length);
static int8_t cdc_receive_fs(uint8_t *buffer, uint32_t *length);
static int8_t cdc_transmit_cplt(uint8_t *buffer, uint32_t *length, uint8_t epnum);

/// USB CDC Interface
USBD_CDC_ItfTypeDef usbd_cdc_if = {
    cdc_init_fs,
    cdc_deinit_fs,
    cdc_control_fs,
    cdc_receive_fs,
    cdc_transmit_cplt,
};

/// Initialize the CDC media low layer over the FS USB IP
static int8_t cdc_init_fs() {
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, tx_buffer, 0);
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, rx_buffer);
    usbd_cdc_initialized = 1;
    return USBD_OK;
}

/// DeInitializes the CDC media low layer
static int8_t cdc_deinit_fs() {
    usbd_cdc_initialized = 0;
    return USBD_OK;
}

/// Manage the CDC class requests
static int8_t cdc_control_fs(uint8_t cmd, uint8_t *buffer, uint16_t length) {
    switch (cmd) {
    case CDC_SEND_ENCAPSULATED_COMMAND:
        break;
    case CDC_GET_ENCAPSULATED_RESPONSE:
        break;
    case CDC_SET_COMM_FEATURE:
        break;
    case CDC_GET_COMM_FEATURE:
        break;
    case CDC_CLEAR_COMM_FEATURE:
        break;
        /*******************************************************************************/
        /* Line Coding Structure                                                       */
        /*-----------------------------------------------------------------------------*/
        /* Offset | Field       | Size | Value  | Description                          */
        /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
        /* 4      | bCharFormat |   1  | Number | Stop bits                            */
        /*                                        0 - 1 Stop bit                       */
        /*                                        1 - 1.5 Stop bits                    */
        /*                                        2 - 2 Stop bits                      */
        /* 5      | bParityType |  1   | Number | Parity                               */
        /*                                        0 - None                             */
        /*                                        1 - Odd                              */
        /*                                        2 - Even                             */
        /*                                        3 - Mark                             */
        /*                                        4 - Space                            */
        /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
        /*******************************************************************************/
    case CDC_SET_LINE_CODING:
        break;
    case CDC_GET_LINE_CODING:
        buffer[0] = 0x20; // bits/second 115200
        buffer[1] = 0xc2;
        buffer[2] = 0x01;
        buffer[3] = 0x00;
        buffer[4] = 0x00; // 1 stop bit
        buffer[5] = 0x00; // parity none
        buffer[6] = 0x08; // 8 data bits
        break;
    case CDC_SET_CONTROL_LINE_STATE:
        usbd_cdc_initialized = 1; //hack, we need enable usbd_cdc on reconnect
        break;
    case CDC_SEND_BREAK:
        break;
    default:
        break;
    }

    return USBD_OK;
}

/// Data received over USB OUT endpoint are sent over CDC interface
/// through this function.
static int8_t cdc_receive_fs(uint8_t *buffer, uint32_t *length) {
    // process the data
    if (cdc_receive_fn)
        cdc_receive_fn(buffer, *length);

    // prepare the interface for next packet
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, rx_buffer);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);

    return USBD_OK;
}

/// Data to send over USB IN endpoint are sent over CDC interface
/// through this function.
uint8_t usbd_cdc_transmit(uint8_t *buffer, uint32_t length) {
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;

    // fail if busy
    if (hcdc->TxState != 0)
        return USBD_BUSY;
    // fail if the data to transmit are longer then one packet
    if (length > CDC_DATA_MAX_PACKET_SIZE) {
        return USBD_FAIL;
    }

    // copy the packet to our tx buffer
    memcpy(tx_buffer, buffer, length);

    // send the packet
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, tx_buffer, length);
    return USBD_CDC_TransmitPacket(&hUsbDeviceFS);
}

/// Callback on transmit complete
int8_t cdc_transmit_cplt(uint8_t *Buf, uint32_t *Len, uint8_t epnum) {
    return 0;
}

void usbd_cdc_register_receive_fn(void (*receive_fn)(uint8_t *buffer, uint32_t length)) {
    cdc_receive_fn = receive_fn;
}

bool usbd_cdc_is_ready() {
    return hUsbDeviceFS.pClassData && usbd_cdc_initialized;
}
