/**
 * @file ppp_debug.c
 * @brief Data logging functions for debugging purpose (PPP)
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

//Dependencies
#include "core/net.h"
#include "ppp/ppp_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (PPP_SUPPORT == ENABLED && PPP_TRACE_LEVEL >= TRACE_LEVEL_DEBUG)

//LCP codes
static const char_t *lcpCodeLabel[] =
{
   "",                  //0
   "Configure-Request", //1
   "Configure-Ack",     //2
   "Configure-Nak",     //3
   "Configure-Reject",  //4
   "Terminate-Request", //5
   "Terminate-Ack",     //6
   "Code-Reject",       //7
   "Protocol-Reject",   //8
   "Echo-Request",      //9
   "Echo-Reply",        //10
   "Discard-Request"    //11
};

//NCP codes
static const char_t *ncpCodeLabel[] =
{
   "",                  //0
   "Configure-Request", //1
   "Configure-Ack",     //2
   "Configure-Nak",     //3
   "Configure-Reject",  //4
   "Terminate-Request", //5
   "Terminate-Ack",     //6
   "Code-Reject"        //7
};

//PAP codes
static const char_t *papCodeLabel[] =
{
   "",                     //0
   "Authenticate-Request", //1
   "Authenticate-Ack",     //2
   "Authenticate-Nak"      //3
};

//CHAP codes
static const char_t *chapCodeLabel[] =
{
   "",          //0
   "Challenge", //1
   "Response",  //2
   "Success",   //3
   "Failure"    //4
};

//LCP options
static const char_t *lcpOptionLabel[] =
{
   "",                                      //0
   "Maximum-Receive-Unit",                  //1
   "Async-Control-Character-Map",           //2
   "Authentication-Protocol",               //3
   "Quality-Protocol",                      //4
   "Magic-Number",                          //5
   "",                                      //6
   "Protocol-Field-Compression",            //7
   "Address-and-Control-Field-Compression", //8
   "FCS-Alternatives",                      //9
   "Self-Describing-Pad",                   //10
   "Numbered-Mode",                         //11
   "",                                      //12
   "Callback"                               //13
};

//IPCP options
static const char_t *ipcpOptionLabel[] =
{
   "",                        //0
   "IP-Addresses",            //1
   "IP-Compression-Protocol", //2
   "IP-Address",              //3
   "Mobile-IPv4",             //4
};

static const char_t *ipcpOptionLabel2[] =
{
   "",                             //128
   "Primary-DNS-Server-Address",   //129
   "Primary-NBNS-Server-Address",  //130
   "Secondary-DNS-Server-Address", //131
   "Secondary-NBNS-Server-Address" //132
};

//IPV6CP options
static const char_t *ipv6cpOptionLabel[] =
{
   "",                         //0
   "Interface-Identifier",     //1
   "IPv6-Compression-Protocol" //2
};


/**
 * @brief Dump LCP/NCP packet for debugging purpose
 * @param[in] packet Pointer to the LCP packet
 * @param[in] length Length of the packet, in bytes
 * @param[in] protocol Protocol field
 * @return Error code
 **/

error_t pppDumpPacket(const PppPacket *packet, size_t length, PppProtocol protocol)
{
   error_t error;

   //Check protocol field
   switch(protocol)
   {
   //LCP packet?
   case PPP_PROTOCOL_LCP:
      error = lcpDumpPacket(packet, length);
      break;
   //NCP packet?
   case PPP_PROTOCOL_IPCP:
   case PPP_PROTOCOL_IPV6CP:
      error = ncpDumpPacket(packet, length, protocol);
      break;
   //PAP packet?
   case PPP_PROTOCOL_PAP:
      error = papDumpPacket(packet, length);
      break;
   //CHAP packet?
   case PPP_PROTOCOL_CHAP:
      error = chapDumpPacket(packet, length);
      break;
   //Unknown protocol?
   default:
      error = ERROR_FAILURE;
      break;
   }

   //Return status code
   return error;
}


/**
 * @brief Dump LCP packet for debugging purpose
 * @param[in] packet Pointer to the LCP packet
 * @param[in] length Length of the packet, in bytes
 * @return Error code
 **/

error_t lcpDumpPacket(const PppPacket *packet, size_t length)
{
   error_t error;
   const char_t *label;

   //Make sure the LCP packet is valid
   if(length < sizeof(PppPacket))
      return ERROR_INVALID_LENGTH;

   //Check the length field
   if(ntohs(packet->length) > length)
      return ERROR_INVALID_LENGTH;
   if(ntohs(packet->length) < sizeof(PppPacket))
      return ERROR_INVALID_LENGTH;

   //Save the length of the LCP packet
   length = ntohs(packet->length);

   //Retrieve the name of the LCP packet
   if(packet->code < arraysize(lcpCodeLabel))
      label = lcpCodeLabel[packet->code];
   else
      label = "Unknown";

   //Dump LCP packet header
   TRACE_DEBUG("  Code = %" PRIu8 " (%s)\r\n", packet->code, label);
   TRACE_DEBUG("  Identifier = %" PRIu8  "\r\n", packet->identifier);
   TRACE_DEBUG("  Length = %" PRIu16 "\r\n", ntohs(packet->length));

   //Configure-Request, Configure-Ack, Configure-Nak or Configure-Reject packet?
   if(packet->code == PPP_CODE_CONFIGURE_REQ ||
      packet->code == PPP_CODE_CONFIGURE_ACK ||
      packet->code == PPP_CODE_CONFIGURE_NAK ||
      packet->code == PPP_CODE_CONFIGURE_REJ)
   {
      //Cast LCP packet
      PppConfigurePacket *p = (PppConfigurePacket *) packet;

      //Valid packet length?
      if(length < sizeof(PppConfigurePacket))
         return ERROR_INVALID_LENGTH;

      //Retrieve the length of the option list
      length -= sizeof(PppConfigurePacket);

      //Dump options
      error = lcpDumpOptions((PppOption *) p->options, length);
      //Any error to report?
      if(error)
         return error;
   }
   //Terminate-Request or Terminate-Ack packet?
   else if(packet->code == PPP_CODE_TERMINATE_REQ ||
      packet->code == PPP_CODE_TERMINATE_ACK)
   {
      //Cast LCP packet
      PppTerminatePacket *p = (PppTerminatePacket *) packet;

      //Valid packet length?
      if(length < sizeof(PppTerminatePacket))
         return ERROR_INVALID_LENGTH;

      //Retrieve the length of data
      length -= sizeof(PppTerminatePacket);

      //Any data?
      if(length > 0)
      {
         //Dump data
         TRACE_DEBUG("  Data (%" PRIuSIZE " bytes)\r\n", length);
         TRACE_DEBUG_ARRAY("    ", p->data, length);
      }
   }
   //Code-Reject packet?
   else if(packet->code == PPP_CODE_CODE_REJ)
   {
      //Cast LCP packet
      PppCodeRejPacket *p = (PppCodeRejPacket *) packet;

      //Valid packet length?
      if(length < sizeof(PppCodeRejPacket))
         return ERROR_INVALID_LENGTH;

      //Retrieve the length of Rejected-Packet field
      length -= sizeof(PppCodeRejPacket);

      //Rejected-Packet
      TRACE_DEBUG("  Rejected-Packet (%" PRIuSIZE " bytes)\r\n", length);
      TRACE_DEBUG_ARRAY("    ", p->rejectedPacket, length);
   }
   //Protocol-Reject packet?
   else if(packet->code == PPP_CODE_PROTOCOL_REJ)
   {
      //Cast LCP packet
      PppProtocolRejPacket *p = (PppProtocolRejPacket *) packet;

      //Valid packet length?
      if(length < sizeof(PppProtocolRejPacket))
         return ERROR_INVALID_LENGTH;

      //Retrieve the length of Rejected-Information field
      length -= sizeof(PppProtocolRejPacket);

      //Rejected-Protocol
      TRACE_DEBUG("  Rejected-Protocol = %" PRIu16 "\r\n", htons(p->rejectedProtocol));
      //Rejected-Information
      TRACE_DEBUG("  Rejected-Information (%" PRIuSIZE " bytes)\r\n", length);
      TRACE_DEBUG_ARRAY("    ", p->rejectedInfo, length);
   }
   //Echo-Request, Echo-Reply or Discard-Request packet?
   else if(packet->code == PPP_CODE_ECHO_REQ ||
      packet->code == PPP_CODE_ECHO_REP ||
      packet->code == PPP_CODE_DISCARD_REQ)
   {
      //Cast LCP packet
      PppEchoPacket *p = (PppEchoPacket *) packet;

      //Valid packet length?
      if(length < sizeof(PppEchoPacket))
         return ERROR_INVALID_LENGTH;

      //Retrieve the length of data
      length -= sizeof(PppEchoPacket);

      //Magic-Number
      TRACE_DEBUG("  Magic-Number = %" PRIu32 "\r\n", htonl(p->magicNumber));
      //Data
      TRACE_DEBUG("  Data (%" PRIuSIZE " bytes)\r\n", length);
      TRACE_DEBUG_ARRAY("    ", p->data, length);
   }
   //Unknown packet?
   else
   {
      //Retrieve the length of data
      length -= sizeof(PppPacket);

      //Any data?
      if(length > 0)
      {
         //Dump data
         TRACE_DEBUG("  Data (%" PRIuSIZE " bytes)\r\n", length);
         TRACE_DEBUG_ARRAY("    ", packet->data, length);
      }
   }

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump NCP packet for debugging purpose
 * @param[in] packet Pointer to the NCP packet
 * @param[in] length Length of the packet, in bytes
 * @param[in] protocol Protocol field
 * @return Error code
 **/

error_t ncpDumpPacket(const PppPacket *packet, size_t length, PppProtocol protocol)
{
   error_t error;
   const char_t *label;

   //Make sure the NDP packet is valid
   if(length < sizeof(PppPacket))
      return ERROR_INVALID_LENGTH;

   //Check the length field
   if(ntohs(packet->length) > length)
      return ERROR_INVALID_LENGTH;
   if(ntohs(packet->length) < sizeof(PppPacket))
      return ERROR_INVALID_LENGTH;

   //Save the length of the NDP packet
   length = ntohs(packet->length);

   //Retrieve the name of the NDP packet
   if(packet->code < arraysize(ncpCodeLabel))
      label = ncpCodeLabel[packet->code];
   else
      label = "Unknown";

   //Dump NDP packet header
   TRACE_DEBUG("  Code = %" PRIu8 " (%s)\r\n", packet->code, label);
   TRACE_DEBUG("  Identifier = %" PRIu8  "\r\n", packet->identifier);
   TRACE_DEBUG("  Length = %" PRIu16 "\r\n", ntohs(packet->length));

   //Configure-Request, Configure-Ack, Configure-Nak or Configure-Reject packet?
   if(packet->code == PPP_CODE_CONFIGURE_REQ ||
      packet->code == PPP_CODE_CONFIGURE_ACK ||
      packet->code == PPP_CODE_CONFIGURE_NAK ||
      packet->code == PPP_CODE_CONFIGURE_REJ)
   {
      //Cast NDP packet
      PppConfigurePacket *p = (PppConfigurePacket *) packet;

      //Valid packet length?
      if(length < sizeof(PppConfigurePacket))
         return ERROR_INVALID_LENGTH;

      //Retrieve the length of the option list
      length -= sizeof(PppConfigurePacket);

      //IPCP protocol?
      if(protocol == PPP_PROTOCOL_IPCP)
      {
         //Dump options
         error = ipcpDumpOptions((PppOption *) p->options, length);
         //Any error to report?
         if(error)
            return error;
      }
      //IPV6CP protocol?
      else if(protocol == PPP_PROTOCOL_IPV6CP)
      {
         //Dump options
         error = ipv6cpDumpOptions((PppOption *) p->options, length);
         //Any error to report?
         if(error)
            return error;
      }
   }
   //Terminate-Request or Terminate-Ack packet?
   else if(packet->code == PPP_CODE_TERMINATE_REQ ||
      packet->code == PPP_CODE_TERMINATE_ACK)
   {
      //Cast NDP packet
      PppTerminatePacket *p = (PppTerminatePacket *) packet;

      //Valid packet length?
      if(length < sizeof(PppTerminatePacket))
         return ERROR_INVALID_LENGTH;

      //Retrieve the length of data
      length -= sizeof(PppTerminatePacket);

      //Any data?
      if(length > 0)
      {
         //Dump data
         TRACE_DEBUG("  Data (%" PRIuSIZE " bytes)\r\n", length);
         TRACE_DEBUG_ARRAY("    ", p->data, length);
      }
   }
   //Code-Reject packet?
   else if(packet->code == PPP_CODE_CODE_REJ)
   {
      //Cast NDP packet
      PppCodeRejPacket *p = (PppCodeRejPacket *) packet;

      //Valid packet length?
      if(length < sizeof(PppCodeRejPacket))
         return ERROR_INVALID_LENGTH;

      //Retrieve the length of Rejected-Packet field
      length -= sizeof(PppCodeRejPacket);

      //Rejected-Packet
      TRACE_DEBUG("  Rejected-Packet (%" PRIuSIZE " bytes)\r\n", length);
      TRACE_DEBUG_ARRAY("    ", p->rejectedPacket, length);
   }
   //Unknown packet?
   else
   {
      //Retrieve the length of data
      length -= sizeof(PppPacket);

      //Any data?
      if(length > 0)
      {
         //Dump data
         TRACE_DEBUG("  Data (%" PRIuSIZE " bytes)\r\n", length);
         TRACE_DEBUG_ARRAY("    ", packet->data, length);
      }
   }

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump PAP packet for debugging purpose
 * @param[in] packet Pointer to the PAP packet
 * @param[in] length Length of the packet, in bytes
 * @return Error code
 **/

error_t papDumpPacket(const PppPacket *packet, size_t length)
{
   const char_t *label;

   //Make sure the PAP packet is valid
   if(length < sizeof(PppPacket))
      return ERROR_INVALID_LENGTH;

   //Check the length field
   if(ntohs(packet->length) > length)
      return ERROR_INVALID_LENGTH;
   if(ntohs(packet->length) < sizeof(PppPacket))
      return ERROR_INVALID_LENGTH;

   //Save the length of the PAP packet
   length = ntohs(packet->length);

   //Retrieve the name of the PAP packet
   if(packet->code < arraysize(papCodeLabel))
      label = papCodeLabel[packet->code];
   else
      label = "Unknown";

   //Dump PAP packet header
   TRACE_DEBUG("  Code = %" PRIu8 " (%s)\r\n", packet->code, label);
   TRACE_DEBUG("  Identifier = %" PRIu8  "\r\n", packet->identifier);
   TRACE_DEBUG("  Length = %" PRIu16 "\r\n", ntohs(packet->length));

   //Authenticate-Request packet?
   if(packet->code == PAP_CODE_AUTH_REQ)
   {
      uint8_t *q;
      PapAuthReqPacket *p;

      //Cast PAP packet
      p = (PapAuthReqPacket *) packet;

      //Valid packet length?
      if(length < sizeof(PapAuthReqPacket))
         return ERROR_INVALID_LENGTH;

      //Peer-ID-Length field
      TRACE_DEBUG("  Peer-ID-Length = %" PRIu8 "\r\n", p->peerIdLength);

      //Check the length of the Peer-ID field
      if(length < (sizeof(PapAuthAckPacket) + 1 + p->peerIdLength))
         return ERROR_INVALID_LENGTH;

      //Peer-ID field
      TRACE_DEBUG("  Peer-ID\r\n");
      TRACE_DEBUG_ARRAY("    ", p->peerId, p->peerIdLength);

      //Point to the Passwd-Length field
      q = p->peerId + p->peerIdLength;

      //Passwd-Length field
      TRACE_DEBUG("  Passwd-Length = %" PRIu8 "\r\n", q[0]);

      //Check the length of the Password field
      if(length < (sizeof(PapAuthAckPacket) + 1 + p->peerIdLength + q[0]))
         return ERROR_INVALID_LENGTH;

      //Password field
      TRACE_DEBUG("  Password\r\n");
      TRACE_DEBUG_ARRAY("    ", q + 1, q[0]);
   }
   //Authenticate-Ack or Authenticate-Nak packet?
   else if(packet->code == PAP_CODE_AUTH_ACK ||
      packet->code == PAP_CODE_AUTH_NAK)
   {
      PapAuthAckPacket *p;

      //Cast PAP packet
      p = (PapAuthAckPacket *) packet;

      //Valid packet length?
      if(length < sizeof(PapAuthAckPacket))
         return ERROR_INVALID_LENGTH;

      //Msg-Length field
      TRACE_DEBUG("  Msg-Length = %" PRIu8 "\r\n", p->msgLength);

      //Check the length of the Message field
      if(length < (sizeof(PapAuthAckPacket) + p->msgLength))
         return ERROR_INVALID_LENGTH;

      if(length > 0)
      {
         //Message field
         TRACE_DEBUG("  Message\r\n");
         TRACE_DEBUG_ARRAY("    ", p->message, p->msgLength);
      }
   }
   //Unknown packet?
   else
   {
      //Retrieve the length of data
      length -= sizeof(PppPacket);

      //Any data?
      if(length > 0)
      {
         //Dump data
         TRACE_DEBUG("  Data (%" PRIuSIZE " bytes)\r\n", length);
         TRACE_DEBUG_ARRAY("    ", packet->data, length);
      }
   }

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump CHAP packet for debugging purpose
 * @param[in] packet Pointer to the PAP packet
 * @param[in] length Length of the packet, in bytes
 * @return Error code
 **/

error_t chapDumpPacket(const PppPacket *packet, size_t length)
{
   const char_t *label;

   //Make sure the CHAP packet is valid
   if(length < sizeof(PppPacket))
      return ERROR_INVALID_LENGTH;

   //Check the length field
   if(ntohs(packet->length) > length)
      return ERROR_INVALID_LENGTH;
   if(ntohs(packet->length) < sizeof(PppPacket))
      return ERROR_INVALID_LENGTH;

   //Save the length of the CHAP packet
   length = ntohs(packet->length);

   //Retrieve the name of the CHAP packet
   if(packet->code < arraysize(chapCodeLabel))
      label = chapCodeLabel[packet->code];
   else
      label = "Unknown";

   //Dump CHAP packet header
   TRACE_DEBUG("  Code = %" PRIu8 " (%s)\r\n", packet->code, label);
   TRACE_DEBUG("  Identifier = %" PRIu8  "\r\n", packet->identifier);
   TRACE_DEBUG("  Length = %" PRIu16 "\r\n", ntohs(packet->length));

   //Challenge or Response packet?
   if(packet->code == CHAP_CODE_CHALLENGE ||
      packet->code == CHAP_CODE_RESPONSE)
   {
      uint8_t *q;
      ChapChallengePacket *p;

      //Cast CHAP packet
      p = (ChapChallengePacket *) packet;

      //Valid packet length?
      if(length < sizeof(ChapChallengePacket))
         return ERROR_INVALID_LENGTH;

      //Value-Size field
      TRACE_DEBUG("  Value-Size = %" PRIu8 "\r\n", p->valueSize);

      //Check the length of the Value field
      if(length < (sizeof(ChapChallengePacket) + p->valueSize))
         return ERROR_INVALID_LENGTH;

      //Value field
      TRACE_DEBUG("  Value\r\n");
      TRACE_DEBUG_ARRAY("    ", p->value, p->valueSize);

      //Point to the Name field
      q = p->value + p->valueSize;
      //Retrieve the length of the Name field
      length -= sizeof(ChapChallengePacket) + p->valueSize;

      //Name field
      TRACE_DEBUG("  Name (%" PRIuSIZE " bytes)\r\n", length);
      TRACE_DEBUG_ARRAY("    ", q, length);
   }
   //Success or Failure packet?
   else if(packet->code == CHAP_CODE_SUCCESS ||
      packet->code == CHAP_CODE_FAILURE)
   {
      ChapSuccessPacket *p;

      //Cast CHAP packet
      p = (ChapSuccessPacket *) packet;

      //Valid packet length?
      if(length < sizeof(ChapSuccessPacket))
         return ERROR_INVALID_LENGTH;

      //Retrieve the length of Message field
      length -= sizeof(ChapSuccessPacket);

      //Message field
      TRACE_DEBUG("  Message (%" PRIuSIZE " bytes)\r\n", length);
      TRACE_DEBUG_ARRAY("    ", p->message, length);
   }
   //Unknown packet?
   else
   {
      //Retrieve the length of data
      length -= sizeof(PppPacket);

      //Any data?
      if(length > 0)
      {
         //Dump data
         TRACE_DEBUG("  Data (%" PRIuSIZE " bytes)\r\n", length);
         TRACE_DEBUG_ARRAY("    ", packet->data, length);
      }
   }

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump LCP options for debugging purpose
 * @param[in] option Pointer to the option list
 * @param[in] length Length of the option list, in bytes
 * @return Error code
 **/

error_t lcpDumpOptions(const PppOption *option, size_t length)
{
   uint32_t value;

   //Parse options
   while(length > 0)
   {
      //Malformed LCP packet?
      if(length < sizeof(PppOption))
         return ERROR_INVALID_LENGTH;

      //Check option length
      if(option->length < sizeof(PppOption))
         return ERROR_INVALID_LENGTH;
      if(option->length > length)
         return ERROR_INVALID_LENGTH;

      //Display the name of the current option
      if(option->type < arraysize(lcpOptionLabel))
         TRACE_DEBUG("  %s option (%" PRIu8 " bytes)\r\n", lcpOptionLabel[option->type], option->length);
      else
         TRACE_DEBUG("  Option %" PRIu8 " (%" PRIu8 " bytes)\r\n", option->type, option->length);

      //Check option code
      switch(option->type)
      {
      //16-bit unsigned value?
      case LCP_OPTION_MRU:
         //Check length field
         if(option->length != (sizeof(PppOption) + sizeof(uint16_t)))
            return ERROR_INVALID_OPTION;
         //Retrieve 16-bit value
         value = LOAD16BE(option->data);
         //Dump option contents
         TRACE_DEBUG("    %" PRIu32 "\r\n", value);
         break;

      //32-bit unsigned value?
      case LCP_OPTION_ACCM:
      case LCP_OPTION_MAGIC_NUMBER:
         //Check length field
         if(option->length != (sizeof(PppOption) + sizeof(uint32_t)))
            return ERROR_INVALID_OPTION;
         //Retrieve 32-bit value
         value = LOAD32BE(option->data);
         //Dump option contents
         TRACE_DEBUG("    0x%08" PRIX32 "\r\n", value);
         break;

      //Raw data?
      default:
         //Dump option contents
         TRACE_DEBUG_ARRAY("    ", option->data, option->length - sizeof(PppOption));
         break;
      }

      //Remaining bytes to process
      length -= option->length;
      //Jump to the next option
      option = (PppOption *) ((uint8_t *) option + option->length);
   }

   //No error to report
   return NO_ERROR;
}

/**
 * @brief Dump IPCP options for debugging purpose
 * @param[in] option Pointer to the option list
 * @param[in] length Length of the option list, in bytes
 * @return Error code
 **/

error_t ipcpDumpOptions(const PppOption *option, size_t length)
{
#if (IPV4_SUPPORT == ENABLED)
   Ipv4Addr ipAddr;
#endif

   //Parse options
   while(length > 0)
   {
      //Malformed IPCP packet?
      if(length < sizeof(PppOption))
         return ERROR_INVALID_LENGTH;

      //Check option length
      if(option->length < sizeof(PppOption))
         return ERROR_INVALID_LENGTH;
      if(option->length > length)
         return ERROR_INVALID_LENGTH;

      //Display the name of the current option
      if(option->type < arraysize(ipcpOptionLabel))
         TRACE_DEBUG("  %s option (%" PRIu8 " bytes)\r\n", ipcpOptionLabel[option->type], option->length);
      else if(option->type >= 128 && option->type < (128 + arraysize(ipcpOptionLabel2)))
         TRACE_DEBUG("  %s option (%" PRIu8 " bytes)\r\n", ipcpOptionLabel2[option->type - 128], option->length);
      else
         TRACE_DEBUG("  Option %" PRIu8 " (%" PRIu8 " bytes)\r\n", option->type, option->length);

      //Check option code
      switch(option->type)
      {
#if (IPV4_SUPPORT == ENABLED)
      //IP address?
      case IPCP_OPTION_IP_ADDRESS:
      case IPCP_OPTION_PRIMARY_DNS:
      case IPCP_OPTION_PRIMARY_NBNS:
      case IPCP_OPTION_SECONDARY_DNS:
      case IPCP_OPTION_SECONDARY_NBNS:
         //Check length field
         if(option->length != (sizeof(PppOption) + sizeof(Ipv4Addr)))
            return ERROR_INVALID_OPTION;
         //Retrieve IPv4 address
         ipv4CopyAddr(&ipAddr, option->data);
         //Dump option contents
         TRACE_DEBUG("    %s\r\n", ipv4AddrToString(ipAddr, NULL));
         break;
#endif
      //Raw data?
      default:
         //Dump option contents
         TRACE_DEBUG_ARRAY("    ", option->data, option->length - sizeof(PppOption));
         break;
      }

      //Remaining bytes to process
      length -= option->length;
      //Jump to the next option
      option = (PppOption *) ((uint8_t *) option + option->length);
   }

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump IPV6CP options for debugging purpose
 * @param[in] option Pointer to the option list
 * @param[in] length Length of the option list, in bytes
 * @return Error code
 **/

error_t ipv6cpDumpOptions(const PppOption *option, size_t length)
{
   //Parse options
   while(length > 0)
   {
      //Malformed IPV6CP packet?
      if(length < sizeof(PppOption))
         return ERROR_INVALID_LENGTH;

      //Check option length
      if(option->length < sizeof(PppOption))
         return ERROR_INVALID_LENGTH;
      if(option->length > length)
         return ERROR_INVALID_LENGTH;

      //Display the name of the current option
      if(option->type < arraysize(ipv6cpOptionLabel))
         TRACE_DEBUG("  %s option (%" PRIu8 " bytes)\r\n", ipv6cpOptionLabel[option->type], option->length);
      else
         TRACE_DEBUG("  Option %" PRIu8 " (%" PRIu8 " bytes)\r\n", option->type, option->length);

      //Check option code
      switch(option->type)
      {
      //Raw data?
      default:
         //Dump option contents
         TRACE_DEBUG_ARRAY("    ", option->data, option->length - sizeof(PppOption));
         break;
      }

      //Remaining bytes to process
      length -= option->length;
      //Jump to the next option
      option = (PppOption *) ((uint8_t *) option + option->length);
   }

   //No error to report
   return NO_ERROR;
}

#endif
