//can.c
#include "can.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_can.h"

uint8_t can_inicialized;

CAN_HandleTypeDef hcan2; //externed in stm32f4xx_it.c
static CAN_TxHeaderTypeDef canHeader;
static uint32_t canTxMailbox;

/**
  * @brief CAN2 Initialization Function
  * @param None
  * @retval None
  */
int CAN2_Init(void) {
    hcan2.Instance = CAN2;
    hcan2.Init.Prescaler = 16;
    hcan2.Init.Mode = CAN_MODE_NORMAL;
    hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan2.Init.TimeSeg1 = CAN_BS1_1TQ;
    hcan2.Init.TimeSeg2 = CAN_BS2_1TQ;
    hcan2.Init.TimeTriggeredMode = DISABLE;
    hcan2.Init.AutoBusOff = DISABLE;
    hcan2.Init.AutoWakeUp = DISABLE;
    hcan2.Init.AutoRetransmission = DISABLE;
    hcan2.Init.ReceiveFifoLocked = DISABLE;
    hcan2.Init.TransmitFifoPriority = DISABLE;
    can_inicialized = HAL_CAN_Init(&hcan2) == HAL_OK;
    return can_inicialized;
}

int CAN2_Start() {
    if (!can_inicialized) {
        if (!CAN2_Init())
            return can_inicialized;
    }
    canHeader.DLC = 8; //data bytes
    canHeader.IDE = CAN_ID_STD;
    canHeader.RTR = CAN_RTR_DATA;

    HAL_CAN_Start(&hcan2);
    HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);

    return can_inicialized;
}

int CAN2_is_initialized() { return can_inicialized; }

void CAN2_Tx(uint8_t *data, size_t sz) {
    canHeader.DLC = sz;
    HAL_CAN_AddTxMessage(&hcan2, &canHeader, data, &canTxMailbox);
}

void CAN2_Tx8(uint8_t *data_8byte) {
    CAN2_Tx(data_8byte, 8);
}
