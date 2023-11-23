#include "gcode_thumb_decoder.h"

namespace {
int gcode_thumb_read([[maybe_unused]] struct _reent *_r, [[maybe_unused]] void *pv, [[maybe_unused]] char *pc, [[maybe_unused]] int n) {
    IGcodeReader *gd = reinterpret_cast<IGcodeReader *>(pv);

    int pos = 0;
    while (n != pos) {
        char c;
        auto res = gd->stream_getc(c);
        if (res != IGcodeReader::Result_t::RESULT_OK) {
            return pos; //  return whatever was read till error or EOF
        }
        pc[pos++] = c;
    }

    return pos;
}

int gcode_thumb_write([[maybe_unused]] struct _reent *_r, [[maybe_unused]] void *pv, [[maybe_unused]] const char *pc, [[maybe_unused]] int n) {
    return 0;
}

int gcode_thumb_close([[maybe_unused]] struct _reent *_r, [[maybe_unused]] void *pv) {
    return 0;
}

_fpos_t gcode_thumb_seek([[maybe_unused]] struct _reent *_r, [[maybe_unused]] void *pv, [[maybe_unused]] _fpos_t fpos, [[maybe_unused]] int ipos) {
    return 0;
}
} // namespace

int f_gcode_thumb_open(IGcodeReader &reader, FILE *fp) {
    memset(fp, 0, sizeof(FILE));
    fp->_read = gcode_thumb_read;
    fp->_write = gcode_thumb_write;
    fp->_close = gcode_thumb_close;
    fp->_seek = gcode_thumb_seek;
    // we can use the cookie to pass any user-defined pointer/context to all of the I/O routines
    fp->_cookie = reinterpret_cast<void *>(&reader);
    fp->_file = -1;
    fp->_flags = __SRD;
    fp->_lbfsize = 512;
    fp->_bf._base = (uint8_t *)malloc(fp->_lbfsize);
    fp->_bf._size = fp->_lbfsize;

    return 0;
}

int f_gcode_thumb_close(FILE *fp) {
    if (fp && fp->_bf._base) {
        free(fp->_bf._base);
    }
    return 0;
}
