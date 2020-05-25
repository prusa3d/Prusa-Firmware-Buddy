// pngview.c

#include "main.h"
#include "display.h"
#include "ff.h"
#include "jogwheel.h"
#include "dbg.h"

#define PNG_FNAMES_MAX_LENGTH 8
char png_fnames[256][PNG_FNAMES_MAX_LENGTH];
int png_cnt = 0;
FIL fil;

int _f_fread(struct _reent *_r, void *pv, char *pc, int n) {
    UINT br = 0;
    f_read(&fil, pc, n, &br);
    return br;
}

int _f_fwrite(struct _reent *_r, void *pv, const char *pc, int n) {
    return 0;
}

int _f_fclose(struct _reent *_r, void *pv) {
    return 0;
}

_fpos_t _f_fseek(struct _reent *_r, void *pv, _fpos_t fpos, int ipos) {
    return 0;
}

FILE *f_fopen(char *fn) {
    f_open(&fil, fn, FA_READ);
    FILE *pf = malloc(sizeof(FILE));
    memset(pf, 0, sizeof(FILE));
    pf->_read = _f_fread;
    pf->_write = _f_fwrite;
    pf->_close = _f_fclose;
    pf->_seek = _f_fseek;
    pf->_file = -1;
    //	pf->_flags = __SRD | __SNBF;
    pf->_flags = __SRD;
    pf->_lbfsize = 512;
    pf->_bf._base = (uint8_t *)malloc(pf->_lbfsize);
    pf->_bf._size = pf->_lbfsize;

    return pf;
}

void f_fclose(FILE *pf) {
    f_close(&fil);
    if (pf != 0) {
        if (pf->_bf._base != 0)
            free(pf->_bf._base);
        free(pf);
    }
}

void pngview(void) {
    DIR dir;
    FRESULT fres = f_opendir(&dir, "/");
    if (fres == FR_OK) {
        FILINFO info;
        fres = f_findfirst(&dir, &info, "", "*.png");
        while (fres == FR_OK) {
            memset(png_fnames[png_cnt], '\0', sizeof(png_fnames[png_cnt]) * sizeof(char)); // set to zeros to be on the safe side
            strlcpy(png_fnames[png_cnt], info.fname, PNG_FNAMES_MAX_LENGTH);
            png_cnt++;
            fres = f_findnext(&dir, &info);
            if (strncmp(png_fnames[png_cnt - 1], info.fname, PNG_FNAMES_MAX_LENGTH) == 0) {
                png_cnt--;
                break; //hack because f_findnext returns allways FR_OK
            }
        }
    }
    f_closedir(&dir);
    jogwheel_encoder_set(0, 0, png_cnt - 1);
    int old_encoder = -1;
    while (png_cnt > 0) {
        if (jogwheel_encoder != old_encoder) {
            old_encoder = jogwheel_encoder;
            char fn[PNG_FNAMES_MAX_LENGTH + 5] = "/";
            _dbg("%d\n", jogwheel_encoder);
            memset(fn + 1, '\0', (sizeof(fn) - 1) * sizeof(char)); // set to zeros to be on the safe side
            strlcpy(fn + 1, png_fnames[jogwheel_encoder], PNG_FNAMES_MAX_LENGTH);
            strlcat(fn, ".PNG", sizeof(fn));
            _dbg("%s\n", fn);
            FILE *pf = f_fopen(fn);
            display->draw_png(point_ui16(0, 0), pf);
            f_fclose(pf);
        } else
            osDelay(1);
        if (jogwheel_button_down >= 1000)
            break;
    }
}
