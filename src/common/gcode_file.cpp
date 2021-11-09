#include "gcode_file.h"
#include "dbg.h"
#include "gcode_thumb_decoder.h"

#define DBG _dbg0

static FILE *gcode_thumb_fp = nullptr;

static int read(struct _reent *_r, void *pv, char *pc, int n) {
    int count = GCodeThumbDecoder::Instance().Read(gcode_thumb_fp, pc, n);
    if (count < 0) {
        return 0;
    }
    return count;
}

static int write(struct _reent *_r, void *pv, const char *pc, int n) {
    return 0;
}

static int close(struct _reent *_r, void *pv) {
    return 0;
}

static _fpos_t seek(struct _reent *_r, void *pv, _fpos_t fpos, int ipos) {
    return 0;
}

extern "C" int f_gcode_thumb_open(FILE *fp, FILE *gcode_fp) {
    if (gcode_thumb_fp) {
        DBG("a gcode png file is already open");
        return 1;
    }
    gcode_thumb_fp = gcode_fp;

    GCodeThumbDecoder::Instance().Reset();

    memset(fp, 0, sizeof(FILE));
    fp->_read = read;
    fp->_write = write;
    fp->_close = close;
    fp->_seek = seek;
    fp->_file = -1;
    fp->_flags = __SRD;
    fp->_lbfsize = 512;
    fp->_bf._base = (uint8_t *)malloc(fp->_lbfsize);
    fp->_bf._size = fp->_lbfsize;

    return 0;
}

extern "C" int f_gcode_thumb_close(FILE *fp) {
    if (fp && fp->_bf._base) {
        free(fp->_bf._base);
    }
    gcode_thumb_fp = nullptr;
    return 0;
}

static bool read_line(FILE *fp, SLine &line) {
    uint8_t byte;
    line.Reset();
    for (;;) {
        if (feof(fp))
            return line.size > 0;
        if (fread(&byte, 1, 1, fp) == 0)
            return false;
        if (byte == '\n')
            break;
        line.AppendByte(byte);
    }
    return true;
}

static char *str_trim(char *str) {

    // trim leading space
    while (*str == ' ')
        str++;

    if (*str == 0)
        return str;

    // trim trailing space
    char *end = str + strlen(str) - 1;
    while (end > str && *end == ' ')
        end--;
    end[1] = '\0';

    return str;
}

bool f_gcode_get_next_comment_assignment(FILE *fp, char *name_buffer,
    int name_buffer_len,
    char *value_buffer,
    int value_buffer_len) {
    SLine line;
    while (true) {
        if (!read_line(fp, line))
            return false;

        // is it a comment line?
        if (strncmp(line, ";", 1) != 0)
            continue;

        // find equal sign or discard this line
        int equal_sign_pos = -1;
        for (int i = 1; i < (int)strlen(line); i++) {
            if (line[i] == '=') {
                equal_sign_pos = i;
                break;
            }
        }
        if (equal_sign_pos == -1)
            continue;

        // trim leading spaces
        int name_start = 1;
        while (name_start < equal_sign_pos && line[name_start] == ' ')
            name_start++;
        int value_start = equal_sign_pos + 1;
        while (line[value_start] == ' ')
            value_start++;

        // copy name and value to given buffers
        snprintf(name_buffer, name_buffer_len, "%.*s",
            equal_sign_pos - name_start,
            (const char *)line.l + name_start);
        snprintf(value_buffer, value_buffer_len, "%s",
            (const char *)line.l + value_start);

        // trim trailing spaces
        str_trim(name_buffer);
        str_trim(value_buffer);

        return true;
    }
}
