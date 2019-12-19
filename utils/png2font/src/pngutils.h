//pngutils.h
#ifndef _PNGUTILS_H
#define _PNGUTILS_H

extern png_structp _png_open_read(FILE *fp, png_infop *pppi);

extern png_structp _png_open_write(FILE *fp, png_infop *pppi, int w, int h, int colortype);

#endif //_PNGUTILS_H
