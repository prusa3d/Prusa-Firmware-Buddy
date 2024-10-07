#include <buddy/fatfs.h>

#include <time.h>

uint8_t retUSBH; /* Return value for USBH */
char USBHPath[4]; /* USBH logical drive path */
FATFS USBHFatFS; /* File system object for USBH logical drive */
FIL USBHFile; /* File object for USBH */

void MX_FATFS_Init(void) {
    retUSBH = FATFS_LinkDriver(&USBH_Driver, USBHPath);
}

/**
 * @brief  Gets Time from RTC
 * @param  None
 * @retval Time in DWORD
 */
DWORD get_fattime(void) {
    time_t timestamp = time(NULL);
    struct tm time;
    localtime_r(&timestamp, &time);

    return ((time.tm_year - 80) << 25) | ((time.tm_mon + 1) << 21) | (time.tm_mday << 16) | (time.tm_hour << 11) | (time.tm_min << 5) | (time.tm_sec >> 1);
}

/// Check if fatfs file is contiguous
/// Source: http://elm-chan.org/fsw/ff/doc/appnote.html#limits
FRESULT fatfs_test_contiguous_file(
    FIL *fp, /* [IN]  Open file object to be checked */
    int *cont /* [OUT] 1:Contiguous, 0:Fragmented or zero-length */
) {
    DWORD clst, clsz, step;
    FSIZE_t fsz;
    FRESULT fr;

    *cont = 0;
    fr = f_rewind(fp); /* Validates and prepares the file */
    if (fr != FR_OK) {
        return fr;
    }

#if FF_MAX_SS == FF_MIN_SS
    clsz = (DWORD)fp->obj.fs->csize * FF_MAX_SS; /* Cluster size */
#else
    clsz = (DWORD)fp->obj.fs->csize * fp->obj.fs->ssize;
#endif
    fsz = f_size(fp);
    if (fsz > 0) {
        clst = fp->obj.sclust - 1; /* A cluster leading the first cluster for first test */
        while (fsz) {
            step = (fsz >= clsz) ? clsz : (DWORD)fsz;
            fr = f_lseek(fp, f_tell(fp) + step); /* Advances file pointer a cluster */
            if (fr != FR_OK) {
                return fr;
            }
            if (clst + 1 != fp->clust) {
                break; /* Is not the cluster next to previous one? */
            }
            clst = fp->clust;
            fsz -= step; /* Get current cluster for next test */
        }
        if (fsz == 0) {
            *cont = 1; /* All done without fail? */
        }
    } else {
        *cont = 1; /* A 0-sized file is continuous by definition. */
    }

    return FR_OK;
}
