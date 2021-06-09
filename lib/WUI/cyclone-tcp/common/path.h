/**
 * @file path.h
 * @brief Path manipulation helper functions
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

#ifndef _PATH_H
#define _PATH_H

//Dependencies
#include "os_port.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Path manipulation helper functions
bool_t pathIsAbsolute(const char_t *path);
bool_t pathIsRelative(const char_t *path);

const char_t *pathGetFilename(const char_t *path);
void pathRemoveFilename(char_t *path);

void pathCopy(char_t *dest, const char_t *src, size_t maxLen);

void pathCanonicalize(char_t *path);

void pathAddSlash(char_t *path, size_t maxLen);
void pathRemoveSlash(char_t *path);

void pathCombine(char_t *path, const char_t *more, size_t maxLen);

bool_t pathMatch(const char_t *path, const char_t *pattern);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
