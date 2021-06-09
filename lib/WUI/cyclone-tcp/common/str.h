/**
 * @file str.h
 * @brief String manipulation helper functions
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

#ifndef _STR_H
#define _STR_H

//Dependencies
#include "os_port.h"
#include "error.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//String manipulation helper functions
char_t *strDuplicate(const char_t *s);
char_t *strTrimWhitespace(char_t *s);
void strRemoveTrailingSpace(char_t *s);
void strReplaceChar(char_t *s, char_t oldChar, char_t newChar);

error_t strSafeCopy(char_t *dest, const char_t *src, size_t destSize);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
