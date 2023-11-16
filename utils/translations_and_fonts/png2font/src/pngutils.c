// pngutils.c

#include <png.h>

png_structp _png_open_read(FILE *fp, png_infop *pppi) {
    if ((fp == NULL) || (pppi == NULL)) {
        return NULL;
    }
    *pppi = NULL;
    png_structp pp = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (pp == NULL) {
        goto _e_0;
    }
    *pppi = png_create_info_struct(pp);
    if (*pppi == NULL) {
        goto _e_1;
    }
    // if (setjmp (png_jmpbuf (pp))) goto _e_1;
    png_init_io(pp, fp);
    png_read_info(pp, *pppi);
    return pp;
_e_1:
    png_destroy_read_struct(&pp, pppi, 0);
_e_0:
    return NULL;
}

png_structp _png_open_write(FILE *fp, png_infop *pppi, int w, int h, int colortype) {
    if ((fp == NULL) || (pppi == NULL)) {
        return NULL;
    }
    *pppi = NULL;
    png_structp pp = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (pp == NULL) {
        goto _e_0;
    }
    *pppi = png_create_info_struct(pp);
    if (*pppi == NULL) {
        goto _e_1;
    }
    // if (setjmp (png_jmpbuf (pp))) goto _e_1;
    png_set_IHDR(pp, *pppi, w, h, 8,
        colortype,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);
    png_init_io(pp, fp);
    png_write_info(pp, *pppi);
    return pp;
_e_1:
    png_destroy_write_struct(&pp, pppi);
_e_0:
    return NULL;
}
