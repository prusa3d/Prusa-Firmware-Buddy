/**
 * @file ssi.h
 * @brief SSI (Server Side Includes)
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

#ifndef _SSI_H
#define _SSI_H

//Dependencies
#include "http/http_server.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//SSI related functions
error_t ssiExecuteScript(HttpConnection *connection, const char_t *uri, uint_t level);

error_t ssiProcessCommand(HttpConnection *connection,
   const char_t *tag, size_t length, const char_t *uri, uint_t level);

error_t ssiProcessIncludeCommand(HttpConnection *connection,
   const char_t *tag, size_t length, const char_t *uri, uint_t level);

error_t ssiProcessEchoCommand(HttpConnection *connection, const char_t *tag, size_t length);
error_t ssiProcessExecCommand(HttpConnection *connection, const char_t *tag, size_t length);

error_t ssiSearchTag(const char_t *s, size_t sLen, const char_t *tag, size_t tagLen, uint_t *pos);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
