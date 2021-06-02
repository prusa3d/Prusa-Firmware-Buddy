/**
 * @file mime.h
 * @brief MIME (Multipurpose Internet Mail Extensions)
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2021 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
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

#ifndef _MIME_H
#define _MIME_H

//Dependencies
#include "net.h"

//Custom MIME types
#ifndef MIME_CUSTOM_TYPES
   #define MIME_CUSTOM_TYPES
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief MIME type
 **/

typedef struct
{
   const char_t *extension;
   const char_t *type;
} MimeType;


//MIME related functions
const char_t *mimeGetType(const char_t *filename);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
