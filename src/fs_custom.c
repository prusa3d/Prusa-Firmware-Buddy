/*
 * fs_custom.c
 *
 *  Created on: Aug 7, 2019
 *      Author: joshy
 */
#include "ff.h"
#include "lwip/apps/fs.h"

static FIL webFile;
//static DIR dir;

int fs_open_custom(struct fs_file *file, const char *name) {
    //FRESULT res;

    //	res = f_opendir(&dir, "/");
    //	if (res != FR_OK) {
    //		return 0;
    //	}

    if (f_open(&webFile, (const TCHAR *)name, FA_READ) != FR_OK) {
        return 0;
    }
    file->data = NULL;
    FSIZE_t fileSize = f_size(&webFile);
    file->len = fileSize;
    file->index = 0;
    file->pextension = NULL;
    return 1;
}

void fs_close_custom(struct fs_file *file) {
    f_close(&webFile);
    //	f_closedir(&dir);
}

int fs_read_custom(struct fs_file *file, char *buffer, int count) {

    FRESULT res;
    uint32_t bytesRead = 0;

    if (f_eof(&webFile)) {
        return FS_READ_EOF;
    }
    //char testbuff[1000];
    res = f_read(&webFile, buffer, count, (void *)&bytesRead);
    //memcpy(buffer, testbuff, bytesRead);
    file->index += bytesRead;
    if (res != FR_OK) {
        return 0;
    }

    return bytesRead;
}
