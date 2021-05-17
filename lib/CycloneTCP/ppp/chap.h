/**
 * @file chap.h
 * @brief CHAP (Challenge Handshake Authentication Protocol)
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

#ifndef _CHAP_H
#define _CHAP_H

//Dependencies
#include "core/net.h"
#include "ppp/ppp.h"

//CHAP authentication support
#ifndef CHAP_SUPPORT
   #define CHAP_SUPPORT DISABLED
#elif (CHAP_SUPPORT != ENABLED && CHAP_SUPPORT != DISABLED)
   #error CHAP_SUPPORT parameter is not valid
#endif

//Restart timer
#ifndef CHAP_RESTART_TIMER
   #define CHAP_RESTART_TIMER 3000
#elif (CHAP_RESTART_TIMER < 1000)
   #error CHAP_RESTART_TIMER parameter is not valid
#endif

//Maximum number of retransmissions for Challenge packets
#ifndef CHAP_MAX_CHALLENGES
   #define CHAP_MAX_CHALLENGES 5
#elif (CHAP_MAX_CHALLENGES < 1)
   #error CHAP_MAX_CHALLENGES parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief CHAP states
 **/

typedef enum
{
   CHAP_STATE_0_INITIAL        = 0,
   CHAP_STATE_1_STARTED        = 1,
   CHAP_STATE_2_CHALLENGE_SENT = 2,
   CHAP_STATE_3_CHALLENGE_RCVD = 3,
   CHAP_STATE_4_RESPONSE_SENT  = 4,
   CHAP_STATE_5_RESPONSE_RCVD  = 5,
   CHAP_STATE_6_SUCCESS_SENT   = 6,
   CHAP_STATE_7_SUCCESS_RCVD   = 7,
   CHAP_STATE_8_FAILURE_SENT   = 8,
   CHAP_STATE_9_FAILURE_RCVD   = 9
} ChapState;


/**
 * @brief Code field values
 **/

typedef enum
{
   CHAP_CODE_CHALLENGE = 1, ///<Challenge
   CHAP_CODE_RESPONSE  = 2, ///<Response
   CHAP_CODE_SUCCESS   = 3, ///<Success
   CHAP_CODE_FAILURE   = 4  ///<Failure
} ChapCode;


/**
 * @brief CHAP algorithm identifiers
 **/

typedef enum
{
   CHAP_ALGO_ID_CHAP_MD5   = 5,   //CHAP with MD5
   CHAP_ALGO_ID_MS_CHAP    = 128, //MS-CHAP
   CHAP_ALGO_ID_MS_CHAP_V2 = 129  //MS-CHAP-2
} ChapAlgoId;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief Challenge packet
 **/

typedef __start_packed struct
{
   uint8_t code;       //0
   uint8_t identifier; //1
   uint16_t length;    //2-3
   uint8_t valueSize;  //4
   uint8_t value[];    //5
} __end_packed ChapChallengePacket;


/**
 * @brief Response packet
 **/

typedef __start_packed struct
{
   uint8_t code;       //0
   uint8_t identifier; //1
   uint16_t length;    //2-3
   uint8_t valueSize;  //4
   uint8_t value[];    //5
} __end_packed ChapResponsePacket;


/**
 * @brief Success packet
 **/

typedef __start_packed struct
{
   uint8_t code;       //0
   uint8_t identifier; //1
   uint16_t length;    //2-3
   uint8_t message[];  //4
} __end_packed ChapSuccessPacket;


/**
 * @brief Failure packet
 **/

typedef __start_packed struct
{
   uint8_t code;       //0
   uint8_t identifier; //1
   uint16_t length;    //2-3
   uint8_t message[];  //4
} __end_packed ChapFailurePacket;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


/**
 * @brief CHAP finite state machine
 **/

typedef struct
{
   uint_t localState;       ///<Local state
   uint8_t localIdentifier; ///<Identifier used to match requests and replies
   uint_t peerState;        ///<Peer state
   uint8_t peerIdentifier;  ///<Identifier used to match requests and replies
   uint_t restartCounter;   ///<Restart counter
   systime_t timestamp;     ///<Timestamp to manage retransmissions
   uint8_t challenge[16];   ///<Challenge value sent to the peer
   const uint8_t *response; ///<Response value from the peer
} ChapFsm;


//CHAP related functions
error_t chapStartAuth(PppContext *context);
error_t chapAbortAuth(PppContext *context);

void chapTick(PppContext *context);

void chapProcessPacket(PppContext *context,
   const PppPacket *packet, size_t length);

error_t chapProcessChallenge(PppContext *context,
   const ChapChallengePacket *challengePacket, size_t length);

error_t chapProcessResponse(PppContext *context,
   const ChapResponsePacket *responsePacket, size_t length);

error_t chapProcessSuccess(PppContext *context,
   const ChapSuccessPacket *successPacket, size_t length);

error_t chapProcessFailure(PppContext *context,
   const ChapFailurePacket *failurePacket, size_t length);

error_t chapSendChallenge(PppContext *context);
error_t chapSendResponse(PppContext *context, const uint8_t *value);
error_t chapSendSuccess(PppContext *context);
error_t chapSendFailure(PppContext *context);

bool_t chapCheckPassword(PppContext *context, const char_t *password);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
