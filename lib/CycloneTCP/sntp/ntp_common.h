/**
 * @file ntp_common.h
 * @brief Definitions common to NTP client and server
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

#ifndef _NTP_COMMON_H
#define _NTP_COMMON_H

//Dependencies
#include "core/net.h"

//NTP port number
#define NTP_PORT 123
//Maximum size of NTP messages
#define NTP_MAX_MSG_SIZE 68
//Difference between NTP and Unix time scales
#define NTP_UNIX_EPOCH 2208988800U

//Kiss code definition
#define NTP_KISS_CODE(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Leap indicator
 **/

typedef enum
{
   NTP_LI_NO_WARNING           = 0,
   NTP_LI_LAST_MIN_HAS_61_SECS = 1,
   NTP_LI_LAST_MIN_HAS_59_SECS = 2,
   NTP_LI_ALARM_CONDITION      = 3
} NtpLeapIndicator;


/**
 * @brief NTP version numbers
 **/

typedef enum
{
   NTP_VERSION_1 = 1,
   NTP_VERSION_2 = 2,
   NTP_VERSION_3 = 3,
   NTP_VERSION_4 = 4
} NtpVersion;


/**
 * @brief Protocol modes
 **/

typedef enum
{
   NTP_MODE_SYMMETRIC_ACTIVE  = 1,
   NTP_MODE_SYMMETRIC_PASSIVE = 2,
   NTP_MODE_CLIENT            = 3,
   NTP_MODE_SERVER            = 4,
   NTP_MODE_BROADCAST         = 5
} NtpMode;


/**
 * @brief Kiss codes
 *
 * The kiss codes can provide useful information for an intelligent client.
 * These codes are encoded in four-character ASCII strings left justified
 * and zero filled
 *
 **/

typedef enum
{
   NTP_KISS_CODE_ACST = NTP_KISS_CODE('A', 'C', 'S', 'T'), ///<The association belongs to a anycast server
   NTP_KISS_CODE_AUTH = NTP_KISS_CODE('A', 'U', 'T', 'H'), ///<Server authentication failed
   NTP_KISS_CODE_AUTO = NTP_KISS_CODE('A', 'U', 'T', 'O'), ///<Autokey sequence failed
   NTP_KISS_CODE_BCST = NTP_KISS_CODE('B', 'C', 'S', 'T'), ///<The association belongs to a broadcast server
   NTP_KISS_CODE_CRYP = NTP_KISS_CODE('C', 'R', 'Y', 'P'), ///<Cryptographic authentication or identification failed
   NTP_KISS_CODE_DENY = NTP_KISS_CODE('D', 'E', 'N', 'Y'), ///<Access denied by remote server
   NTP_KISS_CODE_DROP = NTP_KISS_CODE('D', 'R', 'O', 'P'), ///<Lost peer in symmetric mode
   NTP_KISS_CODE_RSTR = NTP_KISS_CODE('R', 'S', 'T', 'R'), ///<Access denied due to local policy
   NTP_KISS_CODE_INIT = NTP_KISS_CODE('I', 'N', 'I', 'T'), ///<The association has not yet synchronized for the first time
   NTP_KISS_CODE_MCST = NTP_KISS_CODE('M', 'C', 'S', 'T'), ///<The association belongs to a manycast server
   NTP_KISS_CODE_NKEY = NTP_KISS_CODE('N', 'K', 'E', 'Y'), ///<No key found
   NTP_KISS_CODE_RATE = NTP_KISS_CODE('R', 'A', 'T', 'E'), ///<Rate exceeded
   NTP_KISS_CODE_RMOT = NTP_KISS_CODE('R', 'M', 'O', 'T'), ///<Somebody is tinkering with the association from a remote host running ntpdc
   NTP_KISS_CODE_STEP = NTP_KISS_CODE('S', 'T', 'E', 'P')  ///<A step change in system time has occurred
} NtpKissCode;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief NTP timestamp representation
 **/

typedef __start_packed struct
{
   uint32_t seconds;
   uint32_t fraction;
} __end_packed NtpTimestamp;


/**
 * @brief NTP packet header
 **/

typedef __start_packed struct
{
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t li : 2;                  //0
   uint8_t vn : 3;
   uint8_t mode : 3;
#else
   uint8_t mode : 3;                //0
   uint8_t vn : 3;
   uint8_t li : 2;
#endif
   uint8_t stratum;                 //1
   uint8_t poll;                    //2
   int8_t precision;                //3
   uint32_t rootDelay;              //4-7
   uint32_t rootDispersion;         //8-11
   uint32_t referenceId;            //12-15
   NtpTimestamp referenceTimestamp; //16-23
   NtpTimestamp originateTimestamp; //24-31
   NtpTimestamp receiveTimestamp;   //32-39
   NtpTimestamp transmitTimestamp;  //40-47
} __end_packed NtpHeader;


/**
 * @brief NTP authenticator
 **/

typedef __start_packed struct
{
   uint32_t keyId;            //0-3
   uint8_t messageDigest[16]; //4-19
} __end_packed NtpAuthenticator;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
