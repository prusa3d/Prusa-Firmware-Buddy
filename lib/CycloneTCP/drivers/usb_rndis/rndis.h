/**
 * @file rndis.h
 * @brief RNDIS (Remote Network Driver Interface Specification)
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

#ifndef _RNDIS_H
#define _RNDIS_H

//Dependencies
#include "core/net.h"
#include "error.h"

//MAC address
#ifndef RNDIS_MAC_ADDR
   #define RNDIS_MAC_ADDR "02-00-00-12-34-02"
#endif

//Vendor description
#ifndef RNDIS_VENDOR_DESCRIPTION
   #define RNDIS_VENDOR_DESCRIPTION "Unknown"
#endif

//RNDIS versions
#define RNDIS_MAJOR_VERSION 1
#define RNDIS_MINOR_VERSION 0

//Maximum transfer size
#define RNDIS_MAX_TRANSFER_SIZE 2048

//RNDIS message types
#define RNDIS_PACKET_MSG                   0x00000001
#define RNDIS_INITIALIZE_MSG               0x00000002
#define RNDIS_INITIALIZE_CMPLT             0x80000002
#define RNDIS_HALT_MSG                     0x00000003
#define RNDIS_QUERY_MSG                    0x00000004
#define RNDIS_QUERY_CMPLT                  0x80000004
#define RNDIS_SET_MSG                      0x00000005
#define RNDIS_SET_CMPLT                    0x80000005
#define RNDIS_RESET_MSG                    0x00000006
#define RNDIS_RESET_CMPLT                  0x80000006
#define RNDIS_INDICATE_STATUS_MSG          0x00000007
#define RNDIS_KEEPALIVE_MSG                0x00000008
#define RNDIS_KEEPALIVE_CMPLT              0x80000008

//RNDIS device notifications
#define RNDIS_NOTIFICATION_RESP_AVAILABLE  0x00000001

//RNDIS status values
#define RNDIS_STATUS_SUCCESS               0x00000000
#define RNDIS_STATUS_FAILURE               0xC0000001
#define RNDIS_STATUS_INVALID_DATA          0xC0010015
#define RNDIS_STATUS_NOT_SUPPORTED         0xC00000BB
#define RNDIS_STATUS_MEDIA_CONNECT         0x4001000B
#define RNDIS_STATUS_MEDIA_DISCONNECT      0x4001000C

//Device flags
#define RNDIS_DF_CONNECTIONLESS            0x00000001
#define RNDIS_DF_CONNECTION_ORIENTED       0x00000002

//Support medium
#define RNDIS_MEDIUM_802_3                 0x00000000

//General objects (required)
#define OID_GEN_SUPPORTED_LIST             0x00010101
#define OID_GEN_HARDWARE_STATUS            0x00010102
#define OID_GEN_MEDIA_SUPPORTED            0x00010103
#define OID_GEN_MEDIA_IN_USE               0x00010104
#define OID_GEN_MAXIMUM_LOOKAHEAD          0x00010105
#define OID_GEN_MAXIMUM_FRAME_SIZE         0x00010106
#define OID_GEN_LINK_SPEED                 0x00010107
#define OID_GEN_TRANSMIT_BUFFER_SPACE      0x00010108
#define OID_GEN_RECEIVE_BUFFER_SPACE       0x00010109
#define OID_GEN_TRANSMIT_BLOCK_SIZE        0x0001010A
#define OID_GEN_RECEIVE_BLOCK_SIZE         0x0001010B
#define OID_GEN_VENDOR_ID                  0x0001010C
#define OID_GEN_VENDOR_DESCRIPTION         0x0001010D
#define OID_GEN_CURRENT_PACKET_FILTER      0x0001010E
#define OID_GEN_CURRENT_LOOKAHEAD          0x0001010F
#define OID_GEN_DRIVER_VERSION             0x00010110
#define OID_GEN_MAXIMUM_TOTAL_SIZE         0x00010111
#define OID_GEN_PROTOCOL_OPTIONS           0x00010112
#define OID_GEN_MAC_OPTIONS                0x00010113
#define OID_GEN_MEDIA_CONNECT_STATUS       0x00010114
#define OID_GEN_MAXIMUM_SEND_PACKETS       0x00010115
#define OID_GEN_VENDOR_DRIVER_VERSION      0x00010116
#define OID_GEN_SUPPORTED_GUIDS            0x00010117
#define OID_GEN_NETWORK_LAYER_ADDRESSES    0x00010118
#define OID_GEN_TRANSPORT_HEADER_OFFSET    0x00010119
#define OID_GEN_MACHINE_NAME               0x0001021A
#define OID_GEN_RNDIS_CONFIG_PARAMETER     0x0001021B
#define OID_GEN_VLAN_ID                    0x0001021C

//General objects (optional)
#define OID_GEN_MEDIA_CAPABILITIES         0x00010201
#define OID_GEN_PHYSICAL_MEDIUM            0x00010202

//Statistics objects (required)
#define OID_GEN_XMIT_OK                    0x00020101
#define OID_GEN_RCV_OK                     0x00020102
#define OID_GEN_XMIT_ERROR                 0x00020103
#define OID_GEN_RCV_ERROR                  0x00020104
#define OID_GEN_RCV_NO_BUFFER              0x00020105

//Statistics objects (optional)
#define OID_GEN_DIRECTED_BYTES_XMIT        0x00020201
#define OID_GEN_DIRECTED_FRAMES_XMIT       0x00020202
#define OID_GEN_MULTICAST_BYTES_XMIT       0x00020203
#define OID_GEN_MULTICAST_FRAMES_XMIT      0x00020204
#define OID_GEN_BROADCAST_BYTES_XMIT       0x00020205
#define OID_GEN_BROADCAST_FRAMES_XMIT      0x00020206
#define OID_GEN_DIRECTED_BYTES_RCV         0x00020207
#define OID_GEN_DIRECTED_FRAMES_RCV        0x00020208
#define OID_GEN_MULTICAST_BYTES_RCV        0x00020209
#define OID_GEN_MULTICAST_FRAMES_RCV       0x0002020A
#define OID_GEN_BROADCAST_BYTES_RCV        0x0002020B
#define OID_GEN_BROADCAST_FRAMES_RCV       0x0002020C
#define OID_GEN_RCV_CRC_ERROR              0x0002020D
#define OID_GEN_TRANSMIT_QUEUE_LENGTH      0x0002020E

//Ethernet objects (required)
#define OID_802_3_PERMANENT_ADDRESS        0x01010101
#define OID_802_3_CURRENT_ADDRESS          0x01010102
#define OID_802_3_MULTICAST_LIST           0x01010103
#define OID_802_3_MAXIMUM_LIST_SIZE        0x01010104
#define OID_802_3_MAC_OPTIONS              0x01010105
#define OID_802_3_RCV_ERROR_ALIGNMENT      0x01020101
#define OID_802_3_XMIT_ONE_COLLISION       0x01020102
#define OID_802_3_XMIT_MORE_COLLISIONS     0x01020103
#define OID_802_3_XMIT_DEFERRED            0x01020201
#define OID_802_3_XMIT_MAX_COLLISIONS      0x01020202
#define OID_802_3_RCV_OVERRUN              0x01020203
#define OID_802_3_XMIT_UNDERRUN            0x01020204
#define OID_802_3_XMIT_HEARTBEAT_FAILURE   0x01020205
#define OID_802_3_XMIT_TIMES_CRS_LOST      0x01020206
#define OID_802_3_XMIT_LATE_COLLISIONS     0x01020207

//Hardware status of the underlying NIC
#define RNDIS_HARDWARE_STATUS_READY        0x00000000
#define RNDIS_HARDWARE_STATUS_INITIALIZING 0x00000001
#define RNDIS_HARDWARE_STATUS_RESET        0x00000002
#define RNDIS_HARDWARE_STATUS_CLOSING      0x00000003
#define RNDIS_HARDWARE_STATUS_NOT_READY    0x00000004

//Media types
#define RNDIS_MEDIUM_802_3                 0x00000000

//Media state
#define RNDIS_MEDIA_STATE_CONNECTED        0x00000000
#define RNDIS_MEDIA_STATE_DISCONNECTED     0x00000001

//Packet filter
#define RNDIS_PACKET_TYPE_DIRECTED         0x00000001
#define RNDIS_PACKET_TYPE_MULTICAST        0x00000002
#define RNDIS_PACKET_TYPE_ALL_MULTICAST    0x00000004
#define RNDIS_PACKET_TYPE_BROADCAST        0x00000008
#define RNDIS_PACKET_TYPE_SOURCE_ROUTING   0x00000010
#define RNDIS_PACKET_TYPE_PROMISCUOUS      0x00000020
#define RNDIS_PACKET_TYPE_SMT              0x00000040
#define RNDIS_PACKET_TYPE_ALL_LOCAL        0x00000080
#define RNDIS_PACKET_TYPE_GROUP            0x00000100
#define RNDIS_PACKET_TYPE_ALL_FUNCTIONAL   0x00000200
#define RNDIS_PACKET_TYPE_FUNCTIONAL       0x00000400
#define RNDIS_PACKET_TYPE_MAC_FRAME        0x00000800


/**
 * @brief RNDIS states
 **/

typedef enum
{
   RNDIS_STATE_UNINITIALIZED    = 0,
   RNDIS_STATE_BUS_INITIALIZED  = 1,
   RNDIS_STATE_INITIALIZED      = 2,
   RNDIS_STATE_DATA_INITIALIZED = 3
} RndisState;


/**
 * @brief Generic RNDIS message
 **/

typedef struct
{
   uint32_t messageType;   //0-3
   uint32_t messageLength; //4-7
} RndisMsg;


/**
 * @brief RNDIS Initialize message
 **/

typedef struct
{
   uint32_t messageType;     //0-3
   uint32_t messageLength;   //4-7
   uint32_t requestId;       //8-11
   uint32_t majorVersion;    //12-15
   uint32_t minorVersion;    //16-19
   uint32_t maxTransferSize; //20-23
} RndisInitializeMsg;


/**
 * @brief RNDIS Halt message
 **/

typedef struct
{
   uint32_t messageType;   //0-3
   uint32_t messageLength; //4-7
   uint32_t requestId;     //8-11
} RndisHaltMsg;


/**
 * @brief RNDIS Query message
 **/

typedef struct
{
   uint32_t messageType;      //0-3
   uint32_t messageLength;    //4-7
   uint32_t requestId;        //8-11
   uint32_t oid;              //12-15
   uint32_t infoBufferLength; //16-19
   uint32_t infoBufferOffset; //20-23
   uint32_t reserved;         //24-27
   uint8_t oidInputBuffer[];  //28
} RndisQueryMsg;


/**
 * @brief RNDIS Set message
 **/

typedef struct
{
   uint32_t messageType;      //0-3
   uint32_t messageLength;    //4-7
   uint32_t requestId;        //8-11
   uint32_t oid;              //12-15
   uint32_t infoBufferLength; //16-19
   uint32_t infoBufferOffset; //20-23
   uint32_t reserved;         //24-27
   uint8_t oidInputBuffer[];  //28
} RndisSetMsg;


/**
 * @brief RNDIS Reset message
 **/

typedef struct
{
   uint32_t messageType;   //0-3
   uint32_t messageLength; //4-7
   uint32_t reserved;      //8-11
} RndisResetMsg;


/**
 * @brief RNDIS Indicate Status message
 **/

typedef struct
{
   uint32_t messageType;           //0-3
   uint32_t messageLength;         //4-7
   uint32_t status;                //8-11
   uint32_t statusBufferLength;    //12-15
   uint32_t statusBufferOffset;    //16-19
   uint8_t diagnosticInfoBuffer[]; //20
} RndisIndicateStatusMsg;


/**
 * @brief Diagnostic information
 **/

typedef struct
{
   uint32_t diagStatus;  //0-3
   uint32_t errorOffset; //4-7
} RndisDiagInfo;


/**
 * @brief RNDIS Keep-Alive message
 **/

typedef struct
{
   uint32_t messageType;   //0-3
   uint32_t messageLength; //4-7
   uint32_t requestId;     //8-11
} RndisKeepAliveMsg;


/**
 * @brief Response to a RNDIS Initialize message
 **/

typedef struct
{
   uint32_t messageType;           //0-3
   uint32_t messageLength;         //4-7
   uint32_t requestId;             //8-11
   uint32_t status;                //12-15
   uint32_t majorVersion;          //16-19
   uint32_t minorVersion;          //20-23
   uint32_t deviceFlags;           //24-27
   uint32_t medium;                //28-31
   uint32_t maxPacketsPerTransfer; //32-35
   uint32_t maxTransferSize;       //36-39
   uint32_t packetAlignmentFactor; //40-43
   uint32_t afListOffset;          //44-47
   uint32_t afListSize;            //48-51
} RndisInitializeCmplt;


/**
 * @brief Response to a RNDIS Query message
 **/

typedef struct
{
   uint32_t messageType;      //0-3
   uint32_t messageLength;    //4-7
   uint32_t requestId;        //8-11
   uint32_t status;           //12-15
   uint32_t infoBufferLength; //16-19
   uint32_t infoBufferOffset; //20-23
   uint8_t oidInputBuffer[];  //24
} RndisQueryCmplt;


/**
 * @brief Response to a RNDIS Set message
 **/

typedef struct
{
   uint32_t messageType;   //0-3
   uint32_t messageLength; //4-7
   uint32_t requestId;     //8-11
   uint32_t status;        //12-15
} RndisSetCmplt;


/**
 * @brief Response to a RNDIS Reset message
 **/

typedef struct
{
   uint32_t messageType;     //0-3
   uint32_t messageLength;   //4-7
   uint32_t status;          //8-11
   uint32_t addressingReset; //12-15
} RndisResetCmplt;


/**
 * @brief Response to a RNDIS Keep-Alive message
 **/

typedef struct
{
   uint32_t messageType;   //0-3
   uint32_t messageLength; //4-7
   uint32_t requestId;     //8-11
   uint32_t status;        //12-15
} RndisKeepAliveCmplt;


/**
 * @brief RNDIS Packet message
 **/

typedef struct
{
   uint32_t messageType;         //0-3
   uint32_t messageLength;       //4-7
   uint32_t dataOffset;          //8-11
   uint32_t dataLength;          //12-15
   uint32_t oobDataOffset;       //16-19
   uint32_t oobDataLength;       //20-23
   uint32_t numOobDataElements;  //24-27
   uint32_t perPacketInfoOffset; //28-31
   uint32_t perPacketInfoLength; //32-35
   uint32_t vcHandle;            //36-39
   uint32_t reserved;            //40-43
   uint8_t payload[];            //44
} RndisPacketMsg;


/**
 * @brief Out-of-band data record
 **/

typedef struct
{
   uint32_t size;            //0-3
   uint32_t type;            //4-7
   uint32_t classInfoOffset; //8-11
   uint8_t oobData[];        //12
} RndisOobDataRecord;


/**
 * @brief Per-packet information data record
 **/

typedef struct
{
   uint32_t size;                //0-3
   uint32_t type;                //4-7
   uint32_t perPacketInfoOffset; //8-11
   uint8_t perPacketData[];      //12
} RndisPerPacketInfoDataRecord;


/**
 * @brief Device notification message
 **/

typedef struct
{
   uint32_t notification; //0-3
   uint32_t reserved;     //4-7
} RndisNotificationMsg;


/**
 * @brief RNDIS context
 **/

typedef struct
{
   RndisState state;
   bool_t linkEvent;
   bool_t linkState;
   bool_t txState;
   bool_t rxState;
   uint32_t packetFilter;
   uint8_t rxBuffer[RNDIS_MAX_TRANSFER_SIZE];
   size_t rxBufferLen;
   uint8_t encapsulatedResp[RNDIS_MAX_TRANSFER_SIZE];
   size_t encapsulatedRespLen;

   uint32_t data[512 / 4];
   uint8_t CmdOpCode;
   uint8_t CmdLength;
} RndisContext;

//Global variables
extern RndisContext rndisContext;

//RNDIS related functions
void rndisInit(void);

error_t rndisProcessMsg(const RndisMsg *message, size_t length);
error_t rndisProcessInitializeMsg(const RndisInitializeMsg *message, size_t length);
error_t rndisProcessHaltMsg(const RndisHaltMsg *message, size_t length);
error_t rndisProcessQueryMsg(const RndisQueryMsg *message, size_t length);
error_t rndisProcessSetMsg(const RndisSetMsg *message, size_t length);
error_t rndisProcessResetMsg(const RndisResetMsg *message, size_t length);
error_t rndisProcessKeepAliveMsg(const RndisKeepAliveMsg *message, size_t length);

error_t rndisFormatHaltMsg(void);
error_t rndisFormatIndicateStatusMsg(uint32_t status);

error_t rndisFormatInitializeCmplt(uint32_t requestId);
error_t rndisFormatQueryCmplt(uint32_t requestId, uint32_t status, uint32_t length);
error_t rndisFormatSetCmplt(uint32_t requestId, uint32_t status);
error_t rndisFormatResetCmplt(void);
error_t rndisFormatKeepAliveCmplt(uint32_t requestId);

error_t rndisSendNotification(uint32_t notification);

void rndisChangeState(RndisState newState);

#endif
