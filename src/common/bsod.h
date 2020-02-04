/*
 * bsod.h
 *
 *  Created on: 2019-10-01
 *      Author: Radek Vana
 */

#ifndef _BSOD_H
#define _BSOD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//following does not work with macro parameters
//#define QUOTE_X(t)#t
//#define QUOTE(t)QUOTE_X(t)
//#define bsod(format, ...) _bsod(QUOTE(FILE:\n %s\nLINE: %d\n) format, __FILE__ , __LINE__ , ##__VA_ARGS__)

#define bsod(fmt, ...) _bsod(fmt, __FILE__, __LINE__, ##__VA_ARGS__)

//no file name
#define bsod_nofn(fmt, ...) _bsod(fmt, 0, __LINE__, ##__VA_ARGS__)
//no line number
#define bsod_noln(fmt, ...) _bsod(fmt, __FILE__, -1, ##__VA_ARGS__)
//no file name, no line number
#define bsod_nofn_noln(fmt, ...) _bsod(fmt, 0, -1, ##__VA_ARGS__)

void _bsod(const char *fmt, const char *fine_name, int line_number, ...); //with file name and line number

void general_error(const char *error, const char *module);

void temp_error(const char *error, const char *module, float t_noz, float tt_noz, float t_bed, float tt_bed);

void ScreenHardFault();

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_BSOD_H
