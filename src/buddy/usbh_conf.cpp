#include "usbh_core.h"

#include <buddy/main.h>
#include <logging/log.hpp>
#include "device/board.h"
#include "usbh_async_diskio.hpp"
#include "usb_host.h"

LOG_COMPONENT_DEF(USBHost, logging::Severity::info);

HCD_HandleTypeDef hhcd_USB_OTG_HS;

void Error_Handler(void);

USBH_StatusTypeDef USBH_Get_USB_Status(HAL_StatusTypeDef hal_status);

void HAL_HCD_MspInit(HCD_HandleTypeDef *hcdHandle) {
    GPIO_InitTypeDef GPIO_InitStruct {};
    if (hcdHandle->Instance == USB_OTG_HS) {
        /**
         * USB_OTG_HS GPIO Configuration
         * PB14     ------> USB_OTG_HS_DM
         * PB15     ------> USB_OTG_HS_DP
         */
        GPIO_InitStruct.Pin = USB_HS_N_Pin | USB_HS_P_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF12_OTG_HS_FS;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

#if (BOARD_IS_XBUDDY())
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_SET);
#elif (BOARD_IS_XLBUDDY())
        GPIO_InitStruct.Pin = GPIO_PIN_13;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_SET);
#else
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
#endif

        /* Peripheral clock enable */
        __HAL_RCC_USB_OTG_HS_CLK_ENABLE();

        /* Peripheral interrupt init */
        HAL_NVIC_SetPriority(OTG_HS_IRQn, ISR_PRIORITY_DEFAULT, 0);
        HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
    }
}

void HAL_HCD_MspDeInit(HCD_HandleTypeDef *hcdHandle) {
    if (hcdHandle->Instance == USB_OTG_HS) {
        /* Peripheral clock disable */
        __HAL_RCC_USB_OTG_HS_CLK_DISABLE();

        /**
         * USB_OTG_HS GPIO Configuration
         * PB14     ------> USB_OTG_HS_DM
         * PB15     ------> USB_OTG_HS_DP
         */
        HAL_GPIO_DeInit(GPIOB, USB_HS_N_Pin | USB_HS_P_Pin);

        /* Peripheral interrupt Deinit*/
        HAL_NVIC_DisableIRQ(OTG_HS_IRQn);
    }
}

void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_IncTimer(static_cast<USBH_HandleTypeDef *>(hhcd->pData));
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_Connect(static_cast<USBH_HandleTypeDef *>(hhcd->pData));
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_Disconnect(static_cast<USBH_HandleTypeDef *>(hhcd->pData));
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, [[maybe_unused]] uint8_t chnum, [[maybe_unused]] HCD_URBStateTypeDef urb_state) {
    /* To be used with OS to sync URB state with the global state machine */
#if (USBH_USE_OS == 1)
    USBH_LL_NotifyURBChange(static_cast<USBH_HandleTypeDef *>(hhcd->pData));
#endif
    if (USBH_MSC_WorkerTaskHandle) {
        BaseType_t pxHigherPriorityTaskWoken;
        vTaskNotifyGiveFromISR(USBH_MSC_WorkerTaskHandle, &pxHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
    }
}

void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_PortEnabled(static_cast<USBH_HandleTypeDef *>(hhcd->pData));
}

void HAL_HCD_PortDisabled_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_PortDisabled(static_cast<USBH_HandleTypeDef *>(hhcd->pData));
    usbh_power_cycle::port_disabled();
}

/*******************************************************************************
                       LL Driver Interface (USB Host Library --> HCD)
*******************************************************************************/

/**
 * @brief  Initialize the low level portion of the host driver.
 * @param  phost: Host handle
 * @retval USBH status
 */
USBH_StatusTypeDef USBH_LL_Init(USBH_HandleTypeDef *phost) {
    /* Init USB_IP */
    if (phost->id == HOST_HS) {
        /* Link the driver to the stack. */
        hhcd_USB_OTG_HS.pData = phost;
        phost->pData = &hhcd_USB_OTG_HS;

        hhcd_USB_OTG_HS.Instance = USB_OTG_HS;
        hhcd_USB_OTG_HS.Init.Host_channels = 4;
        hhcd_USB_OTG_HS.Init.speed = HCD_SPEED_FULL;
        hhcd_USB_OTG_HS.Init.dma_enable = ENABLE;
        hhcd_USB_OTG_HS.Init.phy_itface = USB_OTG_EMBEDDED_PHY;
        hhcd_USB_OTG_HS.Init.Sof_enable = DISABLE;
        hhcd_USB_OTG_HS.Init.low_power_enable = DISABLE;
        hhcd_USB_OTG_HS.Init.vbus_sensing_enable = DISABLE;
        hhcd_USB_OTG_HS.Init.use_external_vbus = DISABLE;
        if (HAL_HCD_Init(&hhcd_USB_OTG_HS) != HAL_OK) {
            Error_Handler();
        }

        USBH_LL_SetTimer(phost, HAL_HCD_GetCurrentFrame(&hhcd_USB_OTG_HS));
    }
    return USBH_OK;
}

/**
 * @brief  De-Initialize the low level portion of the host driver.
 * @param  phost: Host handle
 * @retval USBH status
 */
USBH_StatusTypeDef USBH_LL_DeInit(USBH_HandleTypeDef *phost) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBH_StatusTypeDef usb_status = USBH_OK;

    hal_status = HAL_HCD_DeInit(static_cast<HCD_HandleTypeDef *>(phost->pData));

    usb_status = USBH_Get_USB_Status(hal_status);

    return usb_status;
}

/**
 * @brief  Start the low level portion of the host driver.
 * @param  phost: Host handle
 * @retval USBH status
 */
USBH_StatusTypeDef USBH_LL_Start(USBH_HandleTypeDef *phost) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBH_StatusTypeDef usb_status = USBH_OK;

    hal_status = HAL_HCD_Start(static_cast<HCD_HandleTypeDef *>(phost->pData));

    usb_status = USBH_Get_USB_Status(hal_status);

    return usb_status;
}

/**
 * @brief  Stop the low level portion of the host driver.
 * @param  phost: Host handle
 * @retval USBH status
 */
USBH_StatusTypeDef USBH_LL_Stop(USBH_HandleTypeDef *phost) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBH_StatusTypeDef usb_status = USBH_OK;

    hal_status = HAL_HCD_Stop(static_cast<HCD_HandleTypeDef *>(phost->pData));

    usb_status = USBH_Get_USB_Status(hal_status);

    return usb_status;
}

/**
 * @brief  Return the USB host speed from the low level driver.
 * @param  phost: Host handle
 * @retval USBH speeds
 */
USBH_SpeedTypeDef USBH_LL_GetSpeed(USBH_HandleTypeDef *phost) {
    USBH_SpeedTypeDef speed = USBH_SPEED_FULL;

    switch (HAL_HCD_GetCurrentSpeed(static_cast<HCD_HandleTypeDef *>(phost->pData))) {
    case 0:
        speed = USBH_SPEED_HIGH;
        break;

    case 1:
        speed = USBH_SPEED_FULL;
        break;

    case 2:
        speed = USBH_SPEED_LOW;
        break;

    default:
        speed = USBH_SPEED_FULL;
        break;
    }
    return speed;
}

/**
 * @brief  Reset the Host port of the low level driver.
 * @param  phost: Host handle
 * @retval USBH status
 */
USBH_StatusTypeDef USBH_LL_ResetPort(USBH_HandleTypeDef *phost) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBH_StatusTypeDef usb_status = USBH_OK;

    hal_status = HAL_HCD_ResetPort(static_cast<HCD_HandleTypeDef *>(phost->pData));

    usb_status = USBH_Get_USB_Status(hal_status);

    return usb_status;
}

/**
 * @brief  Return the last transfered packet size.
 * @param  phost: Host handle
 * @param  pipe: Pipe index
 * @retval Packet size
 */
uint32_t USBH_LL_GetLastXferSize(USBH_HandleTypeDef *phost, uint8_t pipe) {
    return HAL_HCD_HC_GetXferCount(static_cast<HCD_HandleTypeDef *>(phost->pData), pipe);
}

/**
 * @brief  Open a pipe of the low level driver.
 * @param  phost: Host handle
 * @param  pipe_num: Pipe index
 * @param  epnum: Endpoint number
 * @param  dev_address: Device USB address
 * @param  speed: Device Speed
 * @param  ep_type: Endpoint type
 * @param  mps: Endpoint max packet size
 * @retval USBH status
 */
USBH_StatusTypeDef USBH_LL_OpenPipe(USBH_HandleTypeDef *phost, uint8_t pipe_num, uint8_t epnum,
    uint8_t dev_address, uint8_t speed, uint8_t ep_type, uint16_t mps) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBH_StatusTypeDef usb_status = USBH_OK;

    hal_status = HAL_HCD_HC_Init(static_cast<HCD_HandleTypeDef *>(phost->pData), pipe_num, epnum,
        dev_address, speed, ep_type, mps);

    usb_status = USBH_Get_USB_Status(hal_status);

    return usb_status;
}

/**
 * @brief  Close a pipe of the low level driver.
 * @param  phost: Host handle
 * @param  pipe: Pipe index
 * @retval USBH status
 */
USBH_StatusTypeDef USBH_LL_ClosePipe(USBH_HandleTypeDef *phost, uint8_t pipe) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBH_StatusTypeDef usb_status = USBH_OK;

    hal_status = HAL_HCD_HC_Halt(static_cast<HCD_HandleTypeDef *>(phost->pData), pipe);

    usb_status = USBH_Get_USB_Status(hal_status);

    return usb_status;
}

/**
 * @brief  Submit a new URB to the low level driver.
 * @param  phost: Host handle
 * @param  pipe: Pipe index
 *         This parameter can be a value from 1 to 15
 * @param  direction : Channel number
 *          This parameter can be one of the these values:
 *           0 : Output
 *           1 : Input
 * @param  ep_type : Endpoint Type
 *          This parameter can be one of the these values:
 *            @arg EP_TYPE_CTRL: Control type
 *            @arg EP_TYPE_ISOC: Isochrounous type
 *            @arg EP_TYPE_BULK: Bulk type
 *            @arg EP_TYPE_INTR: Interrupt type
 * @param  token : Endpoint Type
 *          This parameter can be one of the these values:
 *            @arg 0: PID_SETUP
 *            @arg 1: PID_DATA
 * @param  pbuff : pointer to URB data
 * @param  length : Length of URB data
 * @param  do_ping : activate do ping protocol (for high speed only)
 *          This parameter can be one of the these values:
 *           0 : do ping inactive
 *           1 : do ping active
 * @retval Status
 */
USBH_StatusTypeDef USBH_LL_SubmitURB(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t direction,
    uint8_t ep_type, uint8_t token, uint8_t *pbuff, uint16_t length,
    uint8_t do_ping) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBH_StatusTypeDef usb_status = USBH_OK;

    hal_status = HAL_HCD_HC_SubmitRequest(static_cast<HCD_HandleTypeDef *>(phost->pData), pipe, direction,
        ep_type, token, pbuff, length,
        do_ping);
    usb_status = USBH_Get_USB_Status(hal_status);

    return usb_status;
}

/**
 * @brief  Get a URB state from the low level driver.
 * @param  phost: Host handle
 * @param  pipe: Pipe index
 *         This parameter can be a value from 1 to 15
 * @retval URB state
 *          This parameter can be one of the these values:
 *            @arg URB_IDLE
 *            @arg URB_DONE
 *            @arg URB_NOTREADY
 *            @arg URB_NYET
 *            @arg URB_ERROR
 *            @arg URB_STALL
 */
USBH_URBStateTypeDef USBH_LL_GetURBState(USBH_HandleTypeDef *phost, uint8_t pipe) {
    return (USBH_URBStateTypeDef)HAL_HCD_HC_GetURBState(static_cast<HCD_HandleTypeDef *>(phost->pData), pipe);
}

/**
 * @brief  Drive VBUS.
 * @param  phost: Host handle
 * @param  state : VBUS state
 *          This parameter can be one of the these values:
 *           0 : VBUS Active
 *           1 : VBUS Inactive
 * @retval Status
 */
USBH_StatusTypeDef USBH_LL_DriverVBUS(USBH_HandleTypeDef *phost, uint8_t state) {
    if (phost->id == HOST_HS) {
        if (state == 0) {
            /* Drive high Charge pump */
#if (BOARD_IS_XBUDDY())
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_SET);
#elif (BOARD_IS_XLBUDDY())
            HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_SET);
#else
            HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
#endif
        } else {
            /* Drive low Charge pump */
#if (BOARD_IS_XBUDDY())
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_RESET);
#elif (BOARD_IS_XLBUDDY())
            HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_RESET);
#else
            HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET);
#endif
        }
    }
    return USBH_OK;
}

/**
 * @brief  Set toggle for a pipe.
 * @param  phost: Host handle
 * @param  pipe: Pipe index
 * @param  toggle: toggle (0/1)
 * @retval Status
 */
USBH_StatusTypeDef USBH_LL_SetToggle(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t toggle) {
    HCD_HandleTypeDef *pHandle;
    pHandle = static_cast<HCD_HandleTypeDef *>(phost->pData);

    if (pHandle->hc[pipe].ep_is_in) {
        pHandle->hc[pipe].toggle_in = toggle;
    } else {
        pHandle->hc[pipe].toggle_out = toggle;
    }

    return USBH_OK;
}

/**
 * @brief  Return the current toggle of a pipe.
 * @param  phost: Host handle
 * @param  pipe: Pipe index
 * @retval toggle (0/1)
 */
uint8_t USBH_LL_GetToggle(USBH_HandleTypeDef *phost, uint8_t pipe) {
    uint8_t toggle = 0;
    HCD_HandleTypeDef *pHandle;
    pHandle = static_cast<HCD_HandleTypeDef *>(phost->pData);

    if (pHandle->hc[pipe].ep_is_in) {
        toggle = pHandle->hc[pipe].toggle_in;
    } else {
        toggle = pHandle->hc[pipe].toggle_out;
    }
    return toggle;
}

/**
 * @brief  Delay routine for the USB Host Library
 * @param  Delay: Delay in ms
 * @retval None
 */
void USBH_Delay(uint32_t Delay) {
    osDelay(Delay);
}

/**
 * @brief  Retuns the USB status depending on the HAL status:
 * @param  hal_status: HAL status
 * @retval USB status
 */
USBH_StatusTypeDef USBH_Get_USB_Status(HAL_StatusTypeDef hal_status) {
    USBH_StatusTypeDef usb_status = USBH_OK;

    switch (hal_status) {
    case HAL_OK:
        usb_status = USBH_OK;
        break;
    case HAL_ERROR:
        usb_status = USBH_FAIL;
        break;
    case HAL_BUSY:
        usb_status = USBH_BUSY;
        break;
    case HAL_TIMEOUT:
        usb_status = USBH_FAIL;
        break;
    default:
        usb_status = USBH_FAIL;
        break;
    }
    return usb_status;
}
