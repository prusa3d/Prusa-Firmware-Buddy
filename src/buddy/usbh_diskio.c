#include "ff_gen_drv.h"
#include "usbh_diskio.h"

static const uint16_t USB_DEFAULT_BLOCK_SIZE = FF_MIN_SS;
static DWORD scratch[FF_MAX_SS / 4];

extern USBH_HandleTypeDef hUsbHostHS;

DSTATUS USBH_initialize(BYTE);
DSTATUS USBH_status(BYTE);
DRESULT USBH_read(BYTE, BYTE *, DWORD, UINT);

#if FF_FS_READONLY == 0
DRESULT USBH_write(BYTE, const BYTE *, DWORD, UINT);
DRESULT USBH_ioctl(BYTE, BYTE, void *);
#endif /* FF_FS_READONLY == 0 */

const Diskio_drvTypeDef USBH_Driver = {
    USBH_initialize,
    USBH_status,
    USBH_read,
#if FF_FS_READONLY == 0
    USBH_write,
    USBH_ioctl,
#endif /* FF_FS_READONLY == 0 */
};

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

    if (USBH_MSC_UnitIsReady(&hUsbHostHS, lun)) {
        res = RES_OK;
    } else {
        res = RES_ERROR;
    }

    return res;
}

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
    USBH_StatusTypeDef status = USBH_OK;

    if ((DWORD)buff & 3) { // DMA Alignment issue, do single up to aligned buffer
        while ((count--) && (status == USBH_OK)) {
            status = USBH_MSC_Read(&hUsbHostHS, lun, sector + count, (uint8_t *)scratch, 1);
            if (status == USBH_OK) {
                memcpy(&buff[count * FF_MAX_SS], scratch, FF_MAX_SS);
            } else {
                break;
            }
        }
    } else {
        status = USBH_MSC_Read(&hUsbHostHS, lun, sector, buff, count);
    }

    if (status == USBH_OK) {
        res = RES_OK;
    } else {
        USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info);

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

/**
 * @brief  Writes Sector(s)
 * @param  lun : lun id
 * @param  *buff: Data to be written
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to write (1..128)
 * @retval DRESULT: Operation result
 */
#if FF_FS_READONLY == 0
DRESULT USBH_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count) {
    DRESULT res = RES_ERROR;
    MSC_LUNTypeDef info;
    USBH_StatusTypeDef status = USBH_OK;

    if ((DWORD)buff & 3) { // DMA Alignment issue, do single up to aligned buffer
        while (count--) {
            memcpy(scratch, &buff[count * FF_MAX_SS], FF_MAX_SS);
            status = USBH_MSC_Write(&hUsbHostHS, lun, sector + count, (BYTE *)scratch, 1);
            if (status == USBH_FAIL) {
                break;
            }
        }
    } else {
        status = USBH_MSC_Write(&hUsbHostHS, lun, sector, (BYTE *)buff, count);
    }

    if (status == USBH_OK) {
        res = RES_OK;
    } else {
        USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info);

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
#endif /* FF_FS_READONLY == 0 */

/**
 * @brief  I/O control operation
 * @param  lun : lun id
 * @param  cmd: Control code
 * @param  *buff: Buffer to send/receive control data
 * @retval DRESULT: Operation result
 */
#if FF_FS_READONLY == 0
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
        if (USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info) == USBH_OK) {
            *(DWORD *)buff = info.capacity.block_nbr;
            res = RES_OK;
        } else {
            res = RES_ERROR;
        }
        break;

    /* Get R/W sector size (WORD) */
    case GET_SECTOR_SIZE:
        if (USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info) == USBH_OK) {
            *(DWORD *)buff = info.capacity.block_size;
            res = RES_OK;
        } else {
            res = RES_ERROR;
        }
        break;

        /* Get erase block size in unit of sector (DWORD) */
    case GET_BLOCK_SIZE:

        if (USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info) == USBH_OK) {
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
#endif /* FF_FS_READONLY == 0 */
