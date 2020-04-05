//can.c
#include "can.h"
#ifdef HAL_CAN_MODULE_ENABLED
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

    canTxHeader.DLC = 8; //data bytes
    canTxHeader.IDE = CAN_ID_STD;
    canTxHeader.RTR = CAN_RTR_DATA;
    canTxHeader.StdId = TX_ID;

    sFilterConfig.FilterIdHigh = RX_ID << 5;               //11bit ID into 16 bit number
    sFilterConfig.FilterIdLow = 0;                         //unused for 11bit ID
    sFilterConfig.FilterMaskIdHigh = 0;                    //mask not used
    sFilterConfig.FilterMaskIdLow = 0;                     //mask not used
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0; //this filter is for FIFO0
    //sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST; //CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
    //sFilterConfig.SlaveStartFilterBank = 0;// For single CAN instances, this parameter is meaningless.

    HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig);

    return can_inicialized;
}

int CAN2_Start() {
    if (!can_inicialized) {
        if (!CAN2_Init())
            return can_inicialized;
    }
    return HAL_CAN_Start(&hcan2) == HAL_OK;
    //HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);//use for interrupt
}

void CAN2_Stop() {
    HAL_CAN_Stop(&hcan2);
}

int CAN2_is_initialized() { return can_inicialized; }

void CAN2_Tx(uint8_t *data, size_t sz) {
    canTxHeader.DLC = sz;
    HAL_CAN_AddTxMessage(&hcan2, &canTxHeader, data, &canTxMailbox);
}

void CAN2_Tx8(uint8_t *data_8byte) {
    CAN2_Tx(data_8byte, 8);
}

int CAN2_try_Rx(CAN_MSG_t *msg) {
    int ret = HAL_CAN_GetRxFifoFillLevel(&hcan2, CAN_RX_FIFO0);
    if (ret) {
        HAL_CAN_GetRxMessage(&hcan2, CAN_RX_FIFO0, &canRxHeader, msg->data);
        msg->header.is_29_ID = canRxHeader.IDE == CAN_ID_EXT ? 1 : 0;
        msg->header.ID = canRxHeader.IDE == CAN_ID_EXT ? canRxHeader.ExtId : canRxHeader.StdId;
    }

    return ret;
}

/*****************************************************************************/
//TX setting
int CAN2_set_tx_DLC(uint32_t DLC) {
    if (DLC > 8)
        return 0;
    canTxHeader.DLC = DLC;
    return 1;
}

void CAN2_set_tx_RTR_data() {
    canTxHeader.RTR = CAN_RTR_DATA;
}

void CAN2_set_tx_RTR_remote() {
    canTxHeader.RTR = CAN_RTR_REMOTE;
}

int CAN2_set_tx_StdId(uint32_t id) {
    if (id >= (1U << 11))
        return 0; //11bit ID
    canTxHeader.StdId = id;
    canTxHeader.IDE = CAN_ID_STD;
    return 1;
}

int CAN2_set_tx_ExtId(uint32_t id) {
    if (id >= (1U << 29))
        return 0; //29bit ID
    canTxHeader.StdId = id;
    canTxHeader.IDE = CAN_ID_EXT;
    return 1;
}

/*****************************************************************************/
//RX setting
//1 32bit filter list
void _CAN2_set_rx_filter_MASK32(uint32_t mask) {
    sFilterConfig.FilterMaskIdHigh = mask >> 16;
    sFilterConfig.FilterMaskIdLow = (uint16_t)mask;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
}
void CAN2_set_rx_filter_MASK32_STD(uint32_t mask) {
    _CAN2_set_rx_filter_MASK32(mask << (32 - 11));
}
void CAN2_set_rx_filter_MASK32_EXT(uint32_t mask) {
    _CAN2_set_rx_filter_MASK32(mask << (32 - 29));
}

//2 16bit filter list
void CAN2_set_rx_filter_MASK16(uint16_t first, uint16_t next) {
    sFilterConfig.FilterMaskIdHigh = first << (16 - 11);
    sFilterConfig.FilterMaskIdLow = next << (16 - 11);
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;
}

//1 32bit filter mask
void _CAN2_set_rx_filter_LIST32(uint32_t list) {
    sFilterConfig.FilterIdHigh = list >> 16;
    sFilterConfig.FilterIdLow = (uint16_t)list;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
}
void CAN2_set_rx_filter_LIST32_STD(uint32_t mask) {
    _CAN2_set_rx_filter_LIST32(mask << (32 - 11));
}
void CAN2_set_rx_filter_LIST32_EXT(uint32_t mask) {
    _CAN2_set_rx_filter_LIST32(mask << (32 - 29));
}

//2 16bit filter mask
void CAN2_set_rx_filter_LIST16(uint16_t first, uint16_t next) {
    sFilterConfig.FilterIdHigh = first << (16 - 11);
    sFilterConfig.FilterIdLow = next << (16 - 11);
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;
}

void CAN2_set_rx_filter_fifo0() {
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
}

void CAN2_set_rx_filter_fifo1() {
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO1;
}

//single instance max 13
//SlaveStartFilterBank - For single CAN instances, this parameter is meaningless.
int CAN2_set_rx_filter_bank(uint32_t bank) {
    if (bank > 13)
        return 0;
    sFilterConfig.FilterBank = bank;
    return 1;
}

void CAN2_set_rx_filter_activate() {
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
    HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig);
}

void CAN2_set_rx_filter_deactivate() {
    sFilterConfig.FilterActivation = CAN_FILTER_DISABLE;
    HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig);
}

/**
  * @brief This function handles CAN2 TX interrupts.
  */
void CAN2_TX_IRQHandler(void) {
    /* USER CODE BEGIN CAN2_TX_IRQn 0 */

    /* USER CODE END CAN2_TX_IRQn 0 */
    HAL_CAN_IRQHandler(&hcan2);
    /* USER CODE BEGIN CAN2_TX_IRQn 1 */

    /* USER CODE END CAN2_TX_IRQn 1 */
}

/**
  * @brief This function handles CAN2 RX0 interrupts.
  */
void CAN2_RX0_IRQHandler(void) {
    /* USER CODE BEGIN CAN2_RX0_IRQn 0 */

    /* USER CODE END CAN2_RX0_IRQn 0 */
    HAL_CAN_IRQHandler(&hcan2);
    /* USER CODE BEGIN CAN2_RX0_IRQn 1 */

    /* USER CODE END CAN2_RX0_IRQn 1 */
}

/**
  * @brief This function handles CAN2 RX1 interrupt.
  */
void CAN2_RX1_IRQHandler(void) {
    /* USER CODE BEGIN CAN2_RX1_IRQn 0 */

    /* USER CODE END CAN2_RX1_IRQn 0 */
    HAL_CAN_IRQHandler(&hcan2);
    /* USER CODE BEGIN CAN2_RX1_IRQn 1 */

    /* USER CODE END CAN2_RX1_IRQn 1 */
}

/**
  * @brief This function handles CAN2 SCE interrupt.
  */
void CAN2_SCE_IRQHandler(void) {
    /* USER CODE BEGIN CAN2_SCE_IRQn 0 */

    /* USER CODE END CAN2_SCE_IRQn 0 */
    HAL_CAN_IRQHandler(&hcan2);
    /* USER CODE BEGIN CAN2_SCE_IRQn 1 */

    /* USER CODE END CAN2_SCE_IRQn 1 */
}

/**
* @brief CAN MSP Initialization
* This function configures the hardware resources used in this example
* @param hcan: CAN handle pointer
* @retval None
*/
void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan) {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    if (hcan->Instance == CAN2) {
        /* USER CODE BEGIN CAN2_MspInit 0 */

        /* USER CODE END CAN2_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_CAN2_CLK_ENABLE();
        __HAL_RCC_CAN1_CLK_ENABLE();

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**CAN2 GPIO Configuration
    PB5     ------> CAN2_RX
    PB6     ------> CAN2_TX
    */
        GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* CAN2 interrupt Init */
        HAL_NVIC_SetPriority(CAN2_TX_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(CAN2_TX_IRQn);
        HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
        HAL_NVIC_SetPriority(CAN2_RX1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(CAN2_RX1_IRQn);
        HAL_NVIC_SetPriority(CAN2_SCE_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(CAN2_SCE_IRQn);
        /* USER CODE BEGIN CAN2_MspInit 1 */

        /* USER CODE END CAN2_MspInit 1 */
    }
}

/**
* @brief CAN MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hcan: CAN handle pointer
* @retval None
*/
void HAL_CAN_MspDeInit(CAN_HandleTypeDef *hcan) {
    if (hcan->Instance == CAN2) {
        /* USER CODE BEGIN CAN2_MspDeInit 0 */

        /* USER CODE END CAN2_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_CAN2_CLK_DISABLE();
        __HAL_RCC_CAN1_CLK_DISABLE();

        /**CAN2 GPIO Configuration
    PB5     ------> CAN2_RX
    PB6     ------> CAN2_TX
    */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5 | GPIO_PIN_6);

        /* CAN2 interrupt DeInit */
        HAL_NVIC_DisableIRQ(CAN2_TX_IRQn);
        HAL_NVIC_DisableIRQ(CAN2_RX0_IRQn);
        HAL_NVIC_DisableIRQ(CAN2_RX1_IRQn);
        HAL_NVIC_DisableIRQ(CAN2_SCE_IRQn);
        /* USER CODE BEGIN CAN2_MspDeInit 1 */

        /* USER CODE END CAN2_MspDeInit 1 */
    }
}
#endif //HAL_CAN_MODULE_ENABLED
