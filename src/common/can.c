//can.c
#include "can.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_can.h"

#define TX_ID 0x100
#define RX_ID 0x200

uint8_t can_inicialized;

CAN_HandleTypeDef hcan2; //externed in stm32f4xx_it.c
static CAN_TxHeaderTypeDef canTxHeader;
static CAN_RxHeaderTypeDef canRxHeader;
static uint32_t canTxMailbox;
static CAN_FilterTypeDef sFilterConfig;
/**
  * @brief CAN2 Initialization Function
  * @param None
  * @retval None
  */
int CAN2_Init(void) {
    hcan2.Instance = CAN2;
    hcan2.Init.Prescaler = 21;
    hcan2.Init.Mode = CAN_MODE_NORMAL;
    hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan2.Init.TimeSeg1 = CAN_BS1_12TQ;
    hcan2.Init.TimeSeg2 = CAN_BS2_4TQ;
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
    canTxHeader.DLC = 8; //data bytes
    canTxHeader.IDE = CAN_ID_STD;
    canTxHeader.RTR = CAN_RTR_DATA;
    canTxHeader.StdId = TX_ID;

    sFilterConfig.FilterIdHigh = RX_ID << 5; //11bit ID?
    sFilterConfig.FilterIdLow = 0;
    sFilterConfig.FilterMaskIdHigh = 0; //mask not used
    sFilterConfig.FilterMaskIdLow = 0;  //mask not used
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    //sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST; //CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE; //todo use CAN_FILTER_DISABLE
    //sFilterConfig.SlaveStartFilterBank = 0;// For single CAN instances, this parameter is meaningless.

    HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig);

    HAL_CAN_Start(&hcan2);
    //HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);//use for interrupt

    return can_inicialized;
}

int CAN2_is_initialized() { return can_inicialized; }

void CAN2_Tx(uint8_t *data, size_t sz) {
    canTxHeader.DLC = sz;
    HAL_CAN_AddTxMessage(&hcan2, &canTxHeader, data, &canTxMailbox);
}

void CAN2_Tx8(uint8_t *data_8byte) {
    CAN2_Tx(data_8byte, 8);
}

int CAN2_try_Rx(uint8_t *data) {
    int ret = HAL_CAN_GetRxFifoFillLevel(&hcan2, CAN_RX_FIFO0);
    if (ret)
        HAL_CAN_GetRxMessage(&hcan2, CAN_RX_FIFO0, &canRxHeader, data);
    return ret;
}
