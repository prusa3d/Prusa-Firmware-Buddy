/**
 * @file os_port.h
 * @brief RTOS abstraction layer
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2021 Oryx Embedded SARL. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

#ifndef _OS_PORT_H
#define _OS_PORT_H

//Dependencies
#include "os_port_config.h"
#include "compiler_port.h"

//Compilation flags used to enable/disable features
#define ENABLED  1
#define DISABLED 0

#define PTR_OFFSET(addr, offset) ((void *) ((uint8_t *) (addr) + (offset)))

#define timeCompare(t1, t2) ((int32_t) ((t1) - (t2)))

//Miscellaneous macros
#ifndef FALSE
   #define FALSE 0
#endif

#ifndef TRUE
   #define TRUE 1
#endif

#ifndef LSB
   #define LSB(x) ((x) & 0xFF)
#endif

#ifndef MSB
   #define MSB(x) (((x) >> 8) & 0xFF)
#endif

#ifndef MIN
   #define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
   #define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef arraysize
   #define arraysize(a) (sizeof(a) / sizeof(a[0]))
#endif

//Infinite delay
#define INFINITE_DELAY ((uint_t) -1)
//Maximum delay
#define MAX_DELAY (INFINITE_DELAY / 2)

//Invalid handle value
#define OS_INVALID_HANDLE NULL

//No RTOS?
#if defined(USE_NO_RTOS)
   #include "os_port_none.h"
//ChibiOS/RT port?
#elif defined(USE_CHIBIOS)
   #include "os_port_chibios.h"
//CMSIS-RTOS port?
#elif defined(USE_CMSIS_RTOS)
   #include "os_port_cmsis_rtos.h"
//CMSIS-RTOS2 port?
#elif defined(USE_CMSIS_RTOS2)
   #include "os_port_cmsis_rtos2.h"
//FreeRTOS port?
#elif defined(USE_FREERTOS)
   #include "os_port_freertos.h"
//SafeRTOS port?
#elif defined(USE_SAFERTOS)
   #include "os_port_safertos.h"
//Keil RTX port?
#elif defined(USE_RTX)
   #include "os_port_rtx.h"
//Micrium uC/OS-II port?
#elif defined(USE_UCOS2)
   #include "os_port_ucos2.h"
//Micrium uC/OS-III port?
#elif defined(USE_UCOS3)
   #include "os_port_ucos3.h"
//Nut/OS port?
#elif defined(USE_NUTOS)
   #include "os_port_nutos.h"
//Segger embOS port?
#elif defined(USE_EMBOS)
   #include "os_port_embos.h"
//TI SYS/BIOS port?
#elif defined(USE_SYS_BIOS)
   #include "os_port_sys_bios.h"
//Windows port?
#elif defined(_WIN32)
   #include "os_port_windows.h"
//POSIX Threads port?
#elif defined(__linux__) || defined(__FreeBSD__)
   #include "os_port_posix.h"
#endif

//Fill block of memory
#ifndef osMemset
   #include <string.h>
   #define osMemset(p, value, length) (void) memset(p, value, length)
#endif

//Copy block of memory
#ifndef osMemcpy
   #include <string.h>
   #define osMemcpy(dest, src, length) (void) memcpy(dest, src, length)
#endif

//Move block of memory
#ifndef osMemmove
   #include <string.h>
   #define osMemmove(dest, src, length) (void) memmove(dest, src, length)
#endif

//Compare two blocks of memory
#ifndef osMemcmp
   #include <string.h>
   #define osMemcmp(p1, p2, length) memcmp(p1, p2, length)
#endif

//Get string length
#ifndef osStrlen
   #include <string.h>
   #define osStrlen(s) strlen(s)
#endif

//Compare strings
#ifndef osStrcmp
   #include <string.h>
   #define osStrcmp(s1, s2) strcmp(s1, s2)
#endif

//Compare substrings
#ifndef osStrncmp
   #include <string.h>
   #define osStrncmp(s1, s2, length) strncmp(s1, s2, length)
#endif

//Compare strings without case
#ifndef osStrcasecmp
   #include <string.h>
   #define osStrcasecmp(s1, s2) strcasecmp(s1, s2)
#endif

//Compare substrings without case
#ifndef osStrncasecmp
   #include <string.h>
   #define osStrncasecmp(s1, s2, length) strncasecmp(s1, s2, length)
#endif

//Search for the first occurrence of a given character
#ifndef osStrchr
   #include <string.h>
   #define osStrchr(s, c) strchr(s, c)
#endif

//Search for the first occurrence of a substring
#ifndef osStrstr
   #include <string.h>
   #define osStrstr(s1, s2) strstr(s1, s2)
#endif

//Copy string
#ifndef osStrcpy
   #include <string.h>
   #define osStrcpy(s1, s2) (void) strcpy(s1, s2)
#endif

//Copy characters from string
#ifndef osStrncpy
   #include <string.h>
   #define osStrncpy(s1, s2, length) (void) strncpy(s1, s2, length)
#endif

//Concatenate strings
#ifndef osStrcat
   #include <string.h>
   #define osStrcat(s1, s2) (void) strcat(s1, s2)
#endif

//Extract tokens from string
#ifndef osStrtok_r
   #include <string.h>
   #define osStrtok_r(s, delim, last) strtok_r(s, delim, last)
#endif

//Format string
#ifndef osSprintf
   #include <stdio.h>
   #define osSprintf(dest, ...) sprintf(dest, __VA_ARGS__)
#endif

//Format string
#ifndef osVsnprintf
   #include <stdio.h>
   #define osVsnprintf(dest, size, format, ap) vsnprintf(dest, size, format, ap)
#endif

//Convert string to unsigned long integer
#ifndef osStrtoul
   #include <stdlib.h>
   #define osStrtoul(s, endptr, base) strtoul(s, endptr, base)
#endif

//Convert string to unsigned long long integer
#ifndef osStrtoull
   #include <stdlib.h>
   #define osStrtoull(s, endptr, base) strtoull(s, endptr, base)
#endif

//Convert a character to lowercase
#ifndef osTolower
   #include <ctype.h>
   #define osTolower(c) tolower((uint8_t) (c))
#endif

//Convert a character to uppercase
#ifndef osToupper
   #include <ctype.h>
   #define osToupper(c) toupper((uint8_t) (c))
#endif

//Check if a character is an uppercase letter
#ifndef osIsupper
   #include <ctype.h>
   #define osIsupper(c) isupper((c))
#endif

//Check if a character is a decimal digit
#ifndef osIsdigit
   #include <ctype.h>
   #define osIsdigit(c) isdigit((uint8_t) (c))
#endif

//Check if a character is a whitespace character
#ifndef osIsspace
   #include <ctype.h>
   #define osIsspace(c) isspace((uint8_t) (c))
#endif

//Check if a character is a blank character
#ifndef osIsblank
   #define osIsblank(c) ((c) == ' ' || (c) == '\t')
#endif

#if !defined(__linux__) && !defined(__FreeBSD__)

//Delay routines
#ifndef usleep
   #define usleep(delay) {volatile uint32_t n = delay * 4; while(n > 0) n--;}
#endif

#ifndef sleep
   #define sleep(delay) {volatile uint32_t n = delay * 4000; while(n > 0) n--;}
#endif

#endif
#endif
