/**
  ******************************************************************************
  * @file           : usbh_conf.c
  * @version        : v1.0_Cube
  * @brief          : This file implements the board support package for the USB host library
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2019 STMicroelectronics International N.V.
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbh_core.h"

/* USER CODE BEGIN Includes */
#include "main.h"
#include "log.h"

LOG_COMPONENT_DEF(USBHost, LOG_SEVERITY_INFO);

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

HCD_HandleTypeDef hhcd_USB_OTG_HS;
void Error_Handler(void);

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
USBH_StatusTypeDef USBH_Get_USB_Status(HAL_StatusTypeDef hal_status);

/* USER CODE END PFP */

/* Private functions ---------------------------------------------------------*/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/*******************************************************************************
                       LL Driver Callbacks (HCD -> USB Host Library)
*******************************************************************************/
/* MSP Init */

void HAL_HCD_MspInit(HCD_HandleTypeDef *hcdHandle) {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    if (hcdHandle->Instance == USB_OTG_HS) {
        /* USER CODE BEGIN USB_OTG_HS_MspInit 0 */

        /* USER CODE END USB_OTG_HS_MspInit 0 */

        /**USB_OTG_HS GPIO Configuration
    PB14     ------> USB_OTG_HS_DM
    PB15     ------> USB_OTG_HS_DP
    */
        GPIO_InitStruct.Pin = USB_HS_N_Pin | USB_HS_P_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF12_OTG_HS_FS;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* Peripheral clock enable */
        __HAL_RCC_USB_OTG_HS_CLK_ENABLE();

        /* Peripheral interrupt init */
        HAL_NVIC_SetPriority(OTG_HS_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
        /* USER CODE BEGIN USB_OTG_HS_MspInit 1 */

        /* USER CODE END USB_OTG_HS_MspInit 1 */
    }
}

void HAL_HCD_MspDeInit(HCD_HandleTypeDef *hcdHandle) {
    if (hcdHandle->Instance == USB_OTG_HS) {
        /* USER CODE BEGIN USB_OTG_HS_MspDeInit 0 */

        /* USER CODE END USB_OTG_HS_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_USB_OTG_HS_CLK_DISABLE();

        /**USB_OTG_HS GPIO Configuration
    PB14     ------> USB_OTG_HS_DM
    PB15     ------> USB_OTG_HS_DP
    */
        HAL_GPIO_DeInit(GPIOB, USB_HS_N_Pin | USB_HS_P_Pin);

        /* Peripheral interrupt Deinit*/
        HAL_NVIC_DisableIRQ(OTG_HS_IRQn);

        /* USER CODE BEGIN USB_OTG_HS_MspDeInit 1 */

        /* USER CODE END USB_OTG_HS_MspDeInit 1 */
    }
}

/**
  * @brief  SOF callback.
  * @param  hhcd: HCD handle
  * @retval None
  */
void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_IncTimer(hhcd->pData);
}

/**
  * @brief  SOF callback.
  * @param  hhcd: HCD handle
  * @retval None
  */
void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_Connect(hhcd->pData);
}

/**
  * @brief  SOF callback.
  * @param  hhcd: HCD handle
  * @retval None
  */
void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_Disconnect(hhcd->pData);
}

/**
  * @brief  Notify URB state change callback.
  * @param  hhcd: HCD handle
  * @param  chnum: channel number
  * @param  urb_state: state
  * @retval None
  */
void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum, HCD_URBStateTypeDef urb_state) {
    /* To be used with OS to sync URB state with the global state machine */
#if (USBH_USE_OS == 1)
    USBH_LL_NotifyURBChange(hhcd->pData);
#endif
}
/**
* @brief  Port Port Enabled callback.
  * @param  hhcd: HCD handle
  * @retval None
  */
void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_PortEnabled(hhcd->pData);
}

/**
  * @brief  Port Port Disabled callback.
  * @param  hhcd: HCD handle
  * @retval None
  */
void HAL_HCD_PortDisabled_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_PortDisabled(hhcd->pData);
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
        hhcd_USB_OTG_HS.Init.Host_channels = 12;
        hhcd_USB_OTG_HS.Init.speed = HCD_SPEED_FULL;
        hhcd_USB_OTG_HS.Init.dma_enable = DISABLE;
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

    hal_status = HAL_HCD_DeInit(phost->pData);

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

    hal_status = HAL_HCD_Start(phost->pData);

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

    hal_status = HAL_HCD_Stop(phost->pData);

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

    switch (HAL_HCD_GetCurrentSpeed(phost->pData)) {
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

    hal_status = HAL_HCD_ResetPort(phost->pData);

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
    return HAL_HCD_HC_GetXferCount(phost->pData, pipe);
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

    hal_status = HAL_HCD_HC_Init(phost->pData, pipe_num, epnum,
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

    hal_status = HAL_HCD_HC_Halt(phost->pData, pipe);

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

    hal_status = HAL_HCD_HC_SubmitRequest(phost->pData, pipe, direction,
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
    return (USBH_URBStateTypeDef)HAL_HCD_HC_GetURBState(phost->pData, pipe);
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

    /* USER CODE BEGIN 0 */

    /* USER CODE END 0*/

    if (phost->id == HOST_HS) {
        if (state == 0) {
            /* Drive high Charge pump */
            /* ToDo: Add IOE driver control */
            /* USER CODE BEGIN DRIVE_HIGH_CHARGE_FOR_HS */

            /* USER CODE END DRIVE_HIGH_CHARGE_FOR_HS */
        } else {
            /* Drive low Charge pump */
            /* ToDo: Add IOE driver control */
            /* USER CODE BEGIN DRIVE_LOW_CHARGE_FOR_HS */

            /* USER CODE END DRIVE_LOW_CHARGE_FOR_HS */
        }
    }
    HAL_Delay(200);
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
    pHandle = phost->pData;

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
    pHandle = phost->pData;

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
    HAL_Delay(Delay);
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
