/**
 * @file ftp_server_misc.h
 * @brief Helper functions for FTP server
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

#ifndef _FTP_SERVER_MISC_H
#define _FTP_SERVER_MISC_H

//Dependencies
#include "core/net.h"
#include "ftp/ftp_server.h"

//Time constant
#define FTP_SERVER_180_DAYS (180 * 86400)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//FTP server related functions
void ftpServerTick(FtpServerContext *context);

uint16_t ftpServerGetPassivePort(FtpServerContext *context);

error_t ftpServerGetPath(FtpClientConnection *connection,
   const char_t *inputPath, char_t *outputPath, size_t maxLen);

uint_t ftpServerGetFilePermissions(FtpClientConnection *connection,
   const char_t *path);

size_t ftpServerFormatDirEntry(const FsDirEntry *dirEntry, uint_t perm,
   char_t *buffer);

const char_t *ftpServerStripRootDir(FtpServerContext *context,
   const char_t *path);

const char_t *ftpServerStripHomeDir(FtpClientConnection *connection,
   const char_t *path);

void ftpServerCloseConnection(FtpClientConnection *connection);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
