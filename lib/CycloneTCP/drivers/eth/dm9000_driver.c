/**
 * @file dm9000_driver.c
 * @brief DM9000A/B Ethernet controller
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

//Switch to the appropriate trace level
#define TRACE_LEVEL NIC_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "core/ethernet.h"
#include "drivers/eth/dm9000_driver.h"
#include "debug.h"


/**
 * @brief DM9000 driver
 **/

const NicDriver dm9000Driver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   dm9000Init,
   dm9000Tick,
   dm9000EnableIrq,
   dm9000DisableIrq,
   dm9000EventHandler,
   dm9000SendPacket,
   dm9000UpdateMacAddrFilter,
   NULL,
   NULL,
   NULL,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief DM9000 controller initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t dm9000Init(NetInterface *interface)
{
   uint_t i;
   uint16_t vendorId;
   uint16_t productId;
   uint8_t chipRev;
   Dm9000Context *context;

   //Debug message
   TRACE_INFO("Initializing DM9000 Ethernet controller...\r\n");

   //Initialize external interrupt line
   interface->extIntDriver->init();

   //Point to the driver context
   context = (Dm9000Context *) interface->nicContext;

   //Initialize driver specific variables
   context->queuedPackets = 0;

   //Retrieve vendorID, product ID and chip revision
   vendorId = (dm9000ReadReg(DM9000_VIDH) << 8) | dm9000ReadReg(DM9000_VIDL);
   productId = (dm9000ReadReg(DM9000_PIDH) << 8) | dm9000ReadReg(DM9000_PIDL);
   chipRev = dm9000ReadReg(DM9000_CHIPR);

   //Check vendor ID and product ID
   if(vendorId != DM9000_VID || productId != DM9000_PID)
   {
      return ERROR_WRONG_IDENTIFIER;
   }

   //Check chip revision
   if(chipRev != DM9000_CHIPR_REV_A && chipRev != DM9000_CHIPR_REV_B)
   {
      return ERROR_WRONG_IDENTIFIER;
   }

   //Power up the internal PHY by clearing PHYPD
   dm9000WriteReg(DM9000_GPR, 0x00);
   //Wait for the PHY to be ready
   sleep(10);

   //Software reset
   dm9000WriteReg(DM9000_NCR, DM9000_NCR_RST);
   //Wait for the reset to complete
   while((dm9000ReadReg(DM9000_NCR) & DM9000_NCR_RST) != 0)
   {
   }

   //PHY software reset
   dm9000WritePhyReg(DM9000_BMCR, DM9000_BMCR_RST);
   //Wait for the PHY reset to complete
   while((dm9000ReadPhyReg(DM9000_BMCR) & DM9000_BMCR_RST) != 0)
   {
   }

   //Debug message
   TRACE_INFO("  VID = 0x%04" PRIX16 "\r\n", vendorId);
   TRACE_INFO("  PID = 0x%04" PRIX16 "\r\n", productId);
   TRACE_INFO("  CHIPR = 0x%02" PRIX8 "\r\n", chipRev);
   TRACE_INFO("  PHYIDR1 = 0x%04" PRIX16 "\r\n", dm9000ReadPhyReg(DM9000_PHYIDR1));
   TRACE_INFO("  PHYIDR2 = 0x%04" PRIX16 "\r\n", dm9000ReadPhyReg(DM9000_PHYIDR2));

   //Enable loopback mode?
#if (DM9000_LOOPBACK_MODE == ENABLED)
   //Enable loopback mode
   dm9000WriteReg(DM9000_NCR, DM9000_NCR_LBK_PHY);

   //Set operation mode
   dm9000WritePhyReg(DM9000_BMCR, DM9000_BMCR_LOOPBACK | DM9000_BMCR_SPEED_SEL |
      DM9000_BMCR_AN_EN | DM9000_BMCR_DUPLEX_MODE);
#endif

   //Set host MAC address
   for(i = 0; i < 6; i++)
   {
      dm9000WriteReg(DM9000_PAR0 + i, interface->macAddr.b[i]);
   }

   //Initialize hash table
   for(i = 0; i < 8; i++)
   {
      dm9000WriteReg(DM9000_MAR0 + i, 0x00);
   }

   //Always accept broadcast packets
   dm9000WriteReg(DM9000_MAR7, 0x80);

   //Enable the Pointer Auto Return function
   dm9000WriteReg(DM9000_IMR, DM9000_IMR_PAR);

   //Clear NSR status bits
   dm9000WriteReg(DM9000_NSR, DM9000_NSR_WAKEST | DM9000_NSR_TX2END |
      DM9000_NSR_TX1END);

   //Clear interrupt flags
   dm9000WriteReg(DM9000_ISR, DM9000_ISR_LNKCHG | DM9000_ISR_UDRUN |
      DM9000_ISR_ROO | DM9000_ISR_ROS | DM9000_ISR_PT | DM9000_ISR_PR);

   //Enable interrupts
   dm9000WriteReg(DM9000_IMR, DM9000_IMR_PAR | DM9000_IMR_LNKCHGI |
      DM9000_IMR_PTI | DM9000_IMR_PRI);

   //Enable the receiver by setting RXEN
   dm9000WriteReg(DM9000_RCR, DM9000_RCR_DIS_LONG | DM9000_RCR_DIS_CRC |
      DM9000_RCR_RXEN);

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Force the TCP/IP stack to poll the link state at startup
   interface->nicEvent = TRUE;
   //Notify the TCP/IP stack of the event
   osSetEvent(&netEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief DM9000 timer handler
 * @param[in] interface Underlying network interface
 **/

void dm9000Tick(NetInterface *interface)
{
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void dm9000EnableIrq(NetInterface *interface)
{
   //Enable interrupts
   interface->extIntDriver->enableIrq();
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void dm9000DisableIrq(NetInterface *interface)
{
   //Disable interrupts
   interface->extIntDriver->disableIrq();
}


/**
 * @brief DM9000 interrupt service routine
 * @param[in] interface Underlying network interface
 * @return TRUE if a higher priority task must be woken. Else FALSE is returned
 **/

bool_t dm9000IrqHandler(NetInterface *interface)
{
   bool_t flag;
   uint8_t status;
   uint8_t mask;
   Dm9000Context *context;

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Point to the driver context
   context = (Dm9000Context *) interface->nicContext;

   //Read interrupt status register
   status = dm9000ReadReg(DM9000_ISR);

   //Link status change?
   if((status & DM9000_ISR_LNKCHG) != 0)
   {
      //Read interrupt mask register
      mask = dm9000ReadReg(DM9000_IMR);
      //Disable LNKCHGI interrupt
      dm9000WriteReg(DM9000_IMR, mask & ~DM9000_IMR_LNKCHGI);

      //Set event flag
      interface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Packet transmission complete?
   if((status & DM9000_ISR_PT) != 0)
   {
      //Check TX complete status bits
      if((dm9000ReadReg(DM9000_NSR) & (DM9000_NSR_TX2END | DM9000_NSR_TX1END)) != 0)
      {
         //The transmission of the current packet is complete
         if(context->queuedPackets > 0)
         {
            context->queuedPackets--;
         }

         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&interface->nicTxEvent);
      }

      //Clear interrupt flag
      dm9000WriteReg(DM9000_ISR, DM9000_ISR_PT);
   }

   //Packet received?
   if((status & DM9000_ISR_PR) != 0)
   {
      //Read interrupt mask register
      mask = dm9000ReadReg(DM9000_IMR);
      //Disable PRI interrupt
      dm9000WriteReg(DM9000_IMR, mask & ~DM9000_IMR_PRI);

      //Set event flag
      interface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //A higher priority task must be woken?
   return flag;
}


/**
 * @brief DM9000 event handler
 * @param[in] interface Underlying network interface
 **/

void dm9000EventHandler(NetInterface *interface)
{
   error_t error;
   uint8_t status;

   //Read interrupt status register
   status = dm9000ReadReg(DM9000_ISR);

   //Check whether the link status has changed?
   if((status & DM9000_ISR_LNKCHG) != 0)
   {
      //Clear interrupt flag
      dm9000WriteReg(DM9000_ISR, DM9000_ISR_LNKCHG);
      //Read network status register
      status = dm9000ReadReg(DM9000_NSR);

      //Check link state
      if((status & DM9000_NSR_LINKST) != 0)
      {
         //Get current speed
         if((status & DM9000_NSR_SPEED) != 0)
         {
            interface->linkSpeed = NIC_LINK_SPEED_10MBPS;
         }
         else
         {
            interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
         }

         //Read network control register
         status = dm9000ReadReg(DM9000_NCR);

         //Determine the new duplex mode
         if((status & DM9000_NCR_FDX) != 0)
         {
            interface->duplexMode = NIC_FULL_DUPLEX_MODE;
         }
         else
         {
            interface->duplexMode = NIC_HALF_DUPLEX_MODE;
         }

         //Link is up
         interface->linkState = TRUE;
      }
      else
      {
         //Link is down
         interface->linkState = FALSE;
      }

      //Process link state change event
      nicNotifyLinkChange(interface);
   }

   //Check whether a packet has been received?
   if((status & DM9000_ISR_PR) != 0)
   {
      //Clear interrupt flag
      dm9000WriteReg(DM9000_ISR, DM9000_ISR_PR);

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = dm9000ReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //Re-enable LNKCHGI and PRI interrupts
   dm9000WriteReg(DM9000_IMR, DM9000_IMR_PAR | DM9000_IMR_LNKCHGI |
      DM9000_IMR_PTI | DM9000_IMR_PRI);
}


/**
 * @brief Send a packet
 * @param[in] interface Underlying network interface
 * @param[in] buffer Multi-part buffer containing the data to send
 * @param[in] offset Offset to the first data byte
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t dm9000SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   static uint8_t temp[DM9000_ETH_TX_BUFFER_SIZE];
   size_t i;
   size_t length;
   uint16_t *p;
   Dm9000Context *context;

   //Point to the driver context
   context = (Dm9000Context *) interface->nicContext;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > DM9000_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Copy user data
   netBufferRead(temp, buffer, offset, length);

   //A dummy write is required before accessing FIFO
   dm9000WriteReg(DM9000_MWCMDX, 0);
   //Select MWCMD register
   DM9000_INDEX_REG = DM9000_MWCMD;

   //Point to the beginning of the buffer
   p = (uint16_t *) temp;

   //Write data to the FIFO using 16-bit mode
   for(i = length; i > 1; i -= 2)
   {
      DM9000_DATA_REG = *(p++);
   }

   //Odd number of bytes?
   if(i > 0)
   {
      DM9000_DATA_REG = *((uint8_t *) p);
   }

   //Write the number of bytes to send
   dm9000WriteReg(DM9000_TXPLL, LSB(length));
   dm9000WriteReg(DM9000_TXPLH, MSB(length));

   //Clear interrupt flag
   dm9000WriteReg(DM9000_ISR, DM9000_ISR_PT);
   //Start data transfer
   dm9000WriteReg(DM9000_TCR, DM9000_TCR_TXREQ);

   //The packet was successfully written to FIFO
   context->queuedPackets++;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Receive a packet
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t dm9000ReceivePacket(NetInterface *interface)
{
   static uint8_t temp[DM9000_ETH_RX_BUFFER_SIZE];
   error_t error;
   size_t i;
   size_t n;
   size_t length;
   volatile uint8_t status;
   volatile uint16_t data;

   //A dummy read is required before accessing the 4-byte header
   data = dm9000ReadReg(DM9000_MRCMDX);

   //Select MRCMDX1 register
   DM9000_INDEX_REG = DM9000_MRCMDX1;
   //Read the first byte of the header
   status = LSB(DM9000_DATA_REG);

   //The first byte indicates if a packet has been received
   if(status == 0x01)
   {
      //Select MRCMD register
      DM9000_INDEX_REG = DM9000_MRCMD;
      //The second byte is the RX status byte
      status = MSB(DM9000_DATA_REG);

      //Retrieve packet length
      length = DM9000_DATA_REG;
      //Limit the number of data to read
      n = MIN(length, DM9000_ETH_RX_BUFFER_SIZE);

      //Point to the beginning of the buffer
      i = 0;

      //Make sure no error occurred
      if((status & (DM9000_RSR_LCS | DM9000_RSR_RWTO | DM9000_RSR_PLE |
         DM9000_RSR_AE | DM9000_RSR_CE | DM9000_RSR_FOE)) == 0)
      {
         //Read data from FIFO using 16-bit mode
         while((i + 1) < n)
         {
            data = DM9000_DATA_REG;
            temp[i++] = LSB(data);
            temp[i++] = MSB(data);
         }

         //Odd number of bytes to read?
         if((i + 1) == n)
         {
            data = DM9000_DATA_REG;
            temp[i] = LSB(data);
            i += 2;
         }

         //Valid packet received
         error = NO_ERROR;
      }
      else
      {
         //The received packet contains an error
         error = ERROR_INVALID_PACKET;
      }

      //Flush remaining bytes
      while(i < length)
      {
         data = DM9000_DATA_REG;
         i += 2;
      }
   }
   else
   {
      //No more data in the receive buffer
      error = ERROR_BUFFER_EMPTY;
   }

   //Check whether a valid packet has been received
   if(!error)
   {
      NetRxAncillary ancillary;

      //Additional options can be passed to the stack along with the packet
      ancillary = NET_DEFAULT_RX_ANCILLARY;

      //Pass the packet to the upper layer
      nicProcessPacket(interface, temp, n, &ancillary);
   }

   //Return status code
   return error;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t dm9000UpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t k;
   uint32_t crc;
   uint8_t hashTable[8];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Clear hash table
   osMemset(hashTable, 0, sizeof(hashTable));
   //Always accept broadcast packets regardless of the MAC filter table
   hashTable[7] = 0x80;

   //The MAC address filter contains the list of MAC addresses to accept
   //when receiving an Ethernet frame
   for(i = 0; i < MAC_ADDR_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->macAddrFilter[i];

      //Valid entry?
      if(entry->refCount > 0)
      {
         //Compute CRC over the current MAC address
         crc = dm9000CalcCrc(&entry->addr, sizeof(MacAddr));
         //Calculate the corresponding index in the table
         k = crc & 0x3F;
         //Update hash table contents
         hashTable[k / 8] |= (1 << (k % 8));
      }
   }

   //Write the hash table to the DM9000 controller
   for(i = 0; i < 8; i++)
   {
      dm9000WriteReg(DM9000_MAR0 + i, hashTable[i]);
   }

   //Debug message
   TRACE_DEBUG("  MAR = %02" PRIX8 " %02" PRIX8 " %02" PRIX8 " %02" PRIX8 " "
      "%02" PRIX8 " %02" PRIX8 " %02" PRIX8 " %02" PRIX8 "\r\n",
      dm9000ReadReg(DM9000_MAR0), dm9000ReadReg(DM9000_MAR1),
      dm9000ReadReg(DM9000_MAR2), dm9000ReadReg(DM9000_MAR3),
      dm9000ReadReg(DM9000_MAR4), dm9000ReadReg(DM9000_MAR5),
      dm9000ReadReg(DM9000_MAR6), dm9000ReadReg(DM9000_MAR7));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Write DM9000 register
 * @param[in] address Register address
 * @param[in] data Register value
 **/

void dm9000WriteReg(uint8_t address, uint8_t data)
{
   //Write register address to INDEX register
   DM9000_INDEX_REG = address;
   //Write register value to DATA register
   DM9000_DATA_REG = data;
}


/**
 * @brief Read DM9000 register
 * @param[in] address Register address
 * @return Register value
 **/

uint8_t dm9000ReadReg(uint8_t address)
{
   //Write register address to INDEX register
   DM9000_INDEX_REG = address;
   //Read register value from DATA register
   return DM9000_DATA_REG;
}


/**
 * @brief Write DM9000 PHY register
 * @param[in] address PHY register address
 * @param[in] data Register value
 **/

void dm9000WritePhyReg(uint8_t address, uint16_t data)
{
   //Write PHY register address
   dm9000WriteReg(DM9000_EPAR, 0x40 | address);
   //Write register value
   dm9000WriteReg(DM9000_EPDRL, LSB(data));
   dm9000WriteReg(DM9000_EPDRH, MSB(data));

   //Start the write operation
   dm9000WriteReg(DM9000_EPCR, DM9000_EPCR_EPOS | DM9000_EPCR_ERPRW);

   //PHY access is still in progress?
   while((dm9000ReadReg(DM9000_EPCR) & DM9000_EPCR_ERRE) != 0)
   {
   }

   //Wait 5us minimum
   usleep(5);
   //Clear command register
   dm9000WriteReg(DM9000_EPCR, DM9000_EPCR_EPOS);
}


/**
 * @brief Read DM9000 PHY register
 * @param[in] address PHY register address
 * @return Register value
 **/

uint16_t dm9000ReadPhyReg(uint8_t address)
{
   //Write PHY register address
   dm9000WriteReg(DM9000_EPAR, 0x40 | address);

   //Start the read operation
   dm9000WriteReg(DM9000_EPCR, DM9000_EPCR_EPOS | DM9000_EPCR_ERPRR);

   //PHY access is still in progress?
   while((dm9000ReadReg(DM9000_EPCR) & DM9000_EPCR_ERRE) != 0)
   {
   }

   //Clear command register
   dm9000WriteReg(DM9000_EPCR, DM9000_EPCR_EPOS);
   //Wait 5us minimum
   usleep(5);

   //Return register value
   return (dm9000ReadReg(DM9000_EPDRH) << 8) | dm9000ReadReg(DM9000_EPDRL);
}


/**
 * @brief CRC calculation
 * @param[in] data Pointer to the data over which to calculate the CRC
 * @param[in] length Number of bytes to process
 * @return Resulting CRC value
 **/

uint32_t dm9000CalcCrc(const void *data, size_t length)
{
   uint_t i;
   uint_t j;
   uint32_t crc;
   const uint8_t *p;

   //Point to the data over which to calculate the CRC
   p = (uint8_t *) data;
   //CRC preset value
   crc = 0xFFFFFFFF;

   //Loop through data
   for(i = 0; i < length; i++)
   {
      //Update CRC value
      crc ^= p[i];
      //The message is processed bit by bit
      for(j = 0; j < 8; j++)
      {
         if((crc & 0x01) != 0)
         {
            crc = (crc >> 1) ^ 0xEDB88320;
         }
         else
         {
            crc = crc >> 1;
         }
      }
   }

   //Return CRC value
   return crc;
}
