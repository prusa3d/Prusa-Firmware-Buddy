/**
  ******************************************************************************
  * @file    usbh_diskio.c (based on usbh_diskio_template.c v2.0.2)
  * @brief   USB Host Disk I/O driver
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
/* USER CODE BEGIN firstSection */
/* can be used to modify / undefine following code or add new definitions */
/* USER CODE END firstSection */

/* Includes ------------------------------------------------------------------*/
#include "ff_gen_drv.h"
#include "usbh_diskio.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

static const uint16_t USB_DEFAULT_BLOCK_SIZE = 512;

/* Private variables ---------------------------------------------------------*/
extern USBH_HandleTypeDef hUSB_Host;

/* Private function prototypes -----------------------------------------------*/
DSTATUS USBH_initialize(BYTE);
DSTATUS USBH_status(BYTE);
DRESULT USBH_read(BYTE, BYTE *, DWORD, UINT);

#if _USE_WRITE == 1
DRESULT USBH_write(BYTE, const BYTE *, DWORD, UINT);
#endif /* _USE_WRITE == 1 */

#if _USE_IOCTL == 1
DRESULT USBH_ioctl(BYTE, BYTE, void *);
#endif /* _USE_IOCTL == 1 */

const Diskio_drvTypeDef USBH_Driver = {
    USBH_initialize,
    USBH_status,
    USBH_read,
#if _USE_WRITE == 1
    USBH_write,
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
    USBH_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* USER CODE BEGIN beforeFunctionSection */
/* can be used to modify / undefine following code or add new code */
/* USER CODE END beforeFunctionSection */

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  lun : lun id
  * @retval DSTATUS: Operation status
  */
DSTATUS USBH_initialize(BYTE lun) {
    /* CAUTION : USB Host library has to be initialized in the application */

    return RES_OK;
}

/**
  * @brief  Gets Disk Status
  * @param  lun : lun id
  * @retval DSTATUS: Operation status
  */
DSTATUS USBH_status(BYTE lun) {
    DRESULT res = RES_ERROR;

    if (USBH_MSC_UnitIsReady(&hUSB_Host, lun)) {
        res = RES_OK;
    } else {
        res = RES_ERROR;
    }

    return res;
}

/* USER CODE BEGIN beforeReadSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END beforeReadSection */

/**
  * @brief  Reads Sector(s)
  * @param  lun : lun id
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT USBH_read(BYTE lun, BYTE *buff, DWORD sector, UINT count) {
    DRESULT res = RES_ERROR;
    MSC_LUNTypeDef info;

    if (USBH_MSC_Read(&hUSB_Host, lun, sector, buff, count) == USBH_OK) {
        res = RES_OK;
    } else {
        USBH_MSC_GetLUNInfo(&hUSB_Host, lun, &info);

        switch (info.sense.asc) {
        case SCSI_ASC_LOGICAL_UNIT_NOT_READY:
        case SCSI_ASC_MEDIUM_NOT_PRESENT:
        case SCSI_ASC_NOT_READY_TO_READY_CHANGE:
            USBH_ErrLog("USB Disk is not ready!"); // not localized, only writes to debug log
            res = RES_NOTRDY;
            break;

        default:
            res = RES_ERROR;
            break;
        }
    }

    return res;
}

/* USER CODE BEGIN beforeWriteSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END beforeWriteSection */

/**
  * @brief  Writes Sector(s)
  * @param  lun : lun id
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT USBH_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count) {
    DRESULT res = RES_ERROR;
    MSC_LUNTypeDef info;

    if (USBH_MSC_Write(&hUSB_Host, lun, sector, (BYTE *)buff, count) == USBH_OK) {
        res = RES_OK;
    } else {
        USBH_MSC_GetLUNInfo(&hUSB_Host, lun, &info);

        switch (info.sense.asc) {
        case SCSI_ASC_WRITE_PROTECTED:
            USBH_ErrLog("USB Disk is Write protected!"); // not localized, only writes to debug log
            res = RES_WRPRT;
            break;

        case SCSI_ASC_LOGICAL_UNIT_NOT_READY:
        case SCSI_ASC_MEDIUM_NOT_PRESENT:
        case SCSI_ASC_NOT_READY_TO_READY_CHANGE:
            USBH_ErrLog("USB Disk is not ready!"); // not localized, only writes to debug log
            res = RES_NOTRDY;
            break;

        default:
            res = RES_ERROR;
            break;
        }
    }

    return res;
}
#endif /* _USE_WRITE == 1 */

/* USER CODE BEGIN beforeIoctlSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END beforeIoctlSection */

/**
  * @brief  I/O control operation
  * @param  lun : lun id
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT USBH_ioctl(BYTE lun, BYTE cmd, void *buff) {
    DRESULT res = RES_ERROR;
    MSC_LUNTypeDef info;

    switch (cmd) {
    /* Make sure that no pending write process */
    case CTRL_SYNC:
        res = RES_OK;
        break;

    /* Get number of sectors on the disk (DWORD) */
    case GET_SECTOR_COUNT:
        if (USBH_MSC_GetLUNInfo(&hUSB_Host, lun, &info) == USBH_OK) {
            *(DWORD *)buff = info.capacity.block_nbr;
            res = RES_OK;
        } else {
            res = RES_ERROR;
        }
        break;

    /* Get R/W sector size (WORD) */
    case GET_SECTOR_SIZE:
        if (USBH_MSC_GetLUNInfo(&hUSB_Host, lun, &info) == USBH_OK) {
            *(DWORD *)buff = info.capacity.block_size;
            res = RES_OK;
        } else {
            res = RES_ERROR;
        }
        break;

        /* Get erase block size in unit of sector (DWORD) */
    case GET_BLOCK_SIZE:

        if (USBH_MSC_GetLUNInfo(&hUSB_Host, lun, &info) == USBH_OK) {
            *(DWORD *)buff = info.capacity.block_size / USB_DEFAULT_BLOCK_SIZE;
            res = RES_OK;
        } else {
            res = RES_ERROR;
        }
        break;

    default:
        res = RES_PARERR;
    }

    return res;
}
#endif /* _USE_IOCTL == 1 */

/* USER CODE BEGIN lastSection */
/* can be used to modify / undefine previous code or add new code */
/* USER CODE END lastSection */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
