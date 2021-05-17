/**
 * @file tftp_common.h
 * @brief Definitions common to TFTP client and server
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

#ifndef _TFTP_COMMON_H
#define _TFTP_COMMON_H

//Dependencies
#include "core/net.h"

//TFTP port number
#define TFTP_PORT 69

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief TFTP opcodes
 **/

typedef enum
{
   TFTP_OPCODE_RRQ   = 1, ///<Read request
   TFTP_OPCODE_WRQ   = 2, ///<Write request
   TFTP_OPCODE_DATA  = 3, ///<Data
   TFTP_OPCODE_ACK   = 4, ///<Acknowledgment
   TFTP_OPCODE_ERROR = 5, ///<Error
   TFTP_OPCODE_OACK  = 6  ///<Option acknowledgment
} TftpOpcode;


/**
 * @brief TFTP error codes
 **/

typedef enum
{
   TFTP_ERROR_NOT_DEFINED         = 0,
   TFTP_ERROR_FILE_NOT_FOUND      = 1,
   TFTP_ERROR_ACCESS_VIOLATION    = 2,
   TFTP_ERROR_DISK_FULL           = 3,
   TFTP_ERROR_ILLEGAL_OPERATION   = 4,
   TFTP_ERROR_UNKNOWN_TID         = 5,
   TFTP_ERROR_FILE_ALREADY_EXISTS = 6,
   TFTP_ERROR_NO_SUCH_USER        = 7
} TftpErrorCode;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief Read request packet (RRQ)
 **/

typedef __start_packed struct
{
   uint16_t opcode;   //0-1
   char_t filename[]; //2
} __end_packed TftpRrqPacket;


/**
 * @brief Write request packet (WRQ)
 **/

typedef __start_packed struct
{
   uint16_t opcode;   //0-1
   char_t filename[]; //2
} __end_packed TftpWrqPacket;


/**
 * @brief Data packet (DATA)
 **/

typedef __start_packed struct
{
   uint16_t opcode; //0-1
   uint16_t block;  //2-3
   uint8_t data[];  //4
} __end_packed TftpDataPacket;


/**
 * @brief Acknowledgment packet (ACK)
 **/

typedef __start_packed struct
{
   uint16_t opcode; //0-1
   uint16_t block;  //2-3
} __end_packed TftpAckPacket;


/**
 * @brief Error packet (ERROR)
 **/

typedef __start_packed struct
{
   uint16_t opcode;    //0-1
   uint16_t errorCode; //2-3
   char_t errorMsg[];  //4
} __end_packed TftpErrorPacket;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
