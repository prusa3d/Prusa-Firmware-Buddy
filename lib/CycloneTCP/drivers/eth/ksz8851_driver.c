/**
 * @file ksz8851_driver.c
 * @brief KSZ8851 Ethernet controller
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
#include "drivers/eth/ksz8851_driver.h"
#include "debug.h"


/**
 * @brief KSZ8851 driver
 **/

const NicDriver ksz8851Driver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   ksz8851Init,
   ksz8851Tick,
   ksz8851EnableIrq,
   ksz8851DisableIrq,
   ksz8851EventHandler,
   ksz8851SendPacket,
   ksz8851UpdateMacAddrFilter,
   NULL,
   NULL,
   NULL,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief KSZ8851 controller initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ksz8851Init(NetInterface *interface)
{
   Ksz8851Context *context;

   //Point to the driver context
   context = (Ksz8851Context *) interface->nicContext;

   //Debug message
   TRACE_INFO("Initializing KSZ8851 Ethernet controller...\r\n");

#if (KSZ8851_SPI_SUPPORT == ENABLED)
   //Initialize SPI
   interface->spiDriver->init();
#endif

   //Initialize external interrupt line
   interface->extIntDriver->init();

   //Debug message
   TRACE_DEBUG("CIDER=0x%04" PRIX16 "\r\n", ksz8851ReadReg(interface, KSZ8851_CIDER));
   TRACE_DEBUG("PHY1ILR=0x%04" PRIX16 "\r\n", ksz8851ReadReg(interface, KSZ8851_PHY1ILR));
   TRACE_DEBUG("PHY1IHR=0x%04" PRIX16 "\r\n", ksz8851ReadReg(interface, KSZ8851_PHY1IHR));

   //Check device ID and revision ID
   if(ksz8851ReadReg(interface, KSZ8851_CIDER) != (KSZ8851_CIDER_FAMILY_ID_DEFAULT |
      KSZ8851_CIDER_CHIP_ID_DEFAULT | KSZ8851_CIDER_REV_ID_A3))
   {
      return ERROR_WRONG_IDENTIFIER;
   }

   //Dump registers for debugging purpose
   ksz8851DumpReg(interface);

   //Initialize driver specific variables
   context->frameId = 0;

   //Initialize MAC address
   ksz8851WriteReg(interface, KSZ8851_MARH, htons(interface->macAddr.w[0]));
   ksz8851WriteReg(interface, KSZ8851_MARM, htons(interface->macAddr.w[1]));
   ksz8851WriteReg(interface, KSZ8851_MARL, htons(interface->macAddr.w[2]));

   //Packets shorter than 64 bytes are padded and the CRC is automatically
   //generated
   ksz8851WriteReg(interface, KSZ8851_TXCR, KSZ8851_TXCR_TXFCE |
      KSZ8851_TXCR_TXPE | KSZ8851_TXCR_TXCE);

   //Automatically increment TX data pointer
   ksz8851WriteReg(interface, KSZ8851_TXFDPR, KSZ8851_TXFDPR_TXFPAI);

   //Configure address filtering
   ksz8851WriteReg(interface, KSZ8851_RXCR1, KSZ8851_RXCR1_RXPAFMA |
      KSZ8851_RXCR1_RXFCE | KSZ8851_RXCR1_RXBE | KSZ8851_RXCR1_RXME |
      KSZ8851_RXCR1_RXUE);

   //No checksum verification
   ksz8851WriteReg(interface, KSZ8851_RXCR2, KSZ8851_RXCR2_SRDBL_SINGLE_FRAME |
      KSZ8851_RXCR2_IUFFP | KSZ8851_RXCR2_RXIUFCEZ);

   //Enable automatic RXQ frame buffer dequeue
   ksz8851WriteReg(interface, KSZ8851_RXQCR, KSZ8851_RXQCR_RXFCTE |
      KSZ8851_RXQCR_ADRFE);

   //Automatically increment RX data pointer
   ksz8851WriteReg(interface, KSZ8851_RXFDPR, KSZ8851_RXFDPR_RXFPAI);
   //Configure receive frame count threshold
   ksz8851WriteReg(interface, KSZ8851_RXFCTR, 1);

   //Force link in half-duplex if auto-negotiation failed
   ksz8851ClearBit(interface, KSZ8851_P1CR, KSZ8851_P1CR_FORCE_DUPLEX);
   //Restart auto-negotiation
   ksz8851SetBit(interface, KSZ8851_P1CR, KSZ8851_P1CR_RESTART_AN);

   //Clear interrupt flags
   ksz8851SetBit(interface, KSZ8851_ISR, KSZ8851_ISR_LCIS |
      KSZ8851_ISR_TXIS | KSZ8851_ISR_RXIS | KSZ8851_ISR_RXOIS |
      KSZ8851_ISR_TXPSIS | KSZ8851_ISR_RXPSIS | KSZ8851_ISR_TXSAIS |
      KSZ8851_ISR_RXWFDIS | KSZ8851_ISR_RXMPDIS | KSZ8851_ISR_LDIS |
      KSZ8851_ISR_EDIS | KSZ8851_ISR_SPIBEIS);

   //Configure interrupts as desired
   ksz8851SetBit(interface, KSZ8851_IER, KSZ8851_IER_LCIE |
      KSZ8851_IER_TXIE | KSZ8851_IER_RXIE);

   //Enable TX operation
   ksz8851SetBit(interface, KSZ8851_TXCR, KSZ8851_TXCR_TXE);
   //Enable RX operation
   ksz8851SetBit(interface, KSZ8851_RXCR1, KSZ8851_RXCR1_RXE);

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
 * @brief KSZ8851 timer handler
 * @param[in] interface Underlying network interface
 **/

void ksz8851Tick(NetInterface *interface)
{
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void ksz8851EnableIrq(NetInterface *interface)
{
   //Enable interrupts
   interface->extIntDriver->enableIrq();
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void ksz8851DisableIrq(NetInterface *interface)
{
   //Disable interrupts
   interface->extIntDriver->disableIrq();
}


/**
 * @brief KSZ8851 interrupt service routine
 * @param[in] interface Underlying network interface
 * @return TRUE if a higher priority task must be woken. Else FALSE is returned
 **/

bool_t ksz8851IrqHandler(NetInterface *interface)
{
   bool_t flag;
   size_t n;
   uint16_t ier;
   uint16_t isr;

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Save IER register value
   ier = ksz8851ReadReg(interface, KSZ8851_IER);
   //Disable interrupts to release the interrupt line
   ksz8851WriteReg(interface, KSZ8851_IER, 0);

   //Read interrupt status register
   isr = ksz8851ReadReg(interface, KSZ8851_ISR);

   //Link status change?
   if((isr & KSZ8851_ISR_LCIS) != 0)
   {
      //Disable LCIE interrupt
      ier &= ~KSZ8851_IER_LCIE;

      //Set event flag
      interface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Packet transmission complete?
   if((isr & KSZ8851_ISR_TXIS) != 0)
   {
      //Clear interrupt flag
      ksz8851WriteReg(interface, KSZ8851_ISR, KSZ8851_ISR_TXIS);

      //Get the amount of free memory available in the TX FIFO
      n = ksz8851ReadReg(interface, KSZ8851_TXMIR) & KSZ8851_TXMIR_TXMA;

      //Check whether the TX FIFO is available for writing
      if(n >= (KSZ8851_ETH_TX_BUFFER_SIZE + 8))
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&interface->nicTxEvent);
      }
   }

   //Packet received?
   if((isr & KSZ8851_ISR_RXIS) != 0)
   {
      //Disable RXIE interrupt
      ier &= ~KSZ8851_IER_RXIE;

      //Set event flag
      interface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Re-enable interrupts once the interrupt has been serviced
   ksz8851WriteReg(interface, KSZ8851_IER, ier);

   //A higher priority task must be woken?
   return flag;
}


/**
 * @brief KSZ8851 event handler
 * @param[in] interface Underlying network interface
 **/

void ksz8851EventHandler(NetInterface *interface)
{
   uint16_t status;
   uint_t frameCount;

   //Read interrupt status register
   status = ksz8851ReadReg(interface, KSZ8851_ISR);

   //Check whether the link status has changed?
   if((status & KSZ8851_ISR_LCIS) != 0)
   {
      //Clear interrupt flag
      ksz8851WriteReg(interface, KSZ8851_ISR, KSZ8851_ISR_LCIS);
      //Read PHY status register
      status = ksz8851ReadReg(interface, KSZ8851_P1SR);

      //Check link state
      if((status & KSZ8851_P1SR_LINK_GOOD) != 0)
      {
         //Get current speed
         if((status & KSZ8851_P1SR_OPERATION_SPEED) != 0)
         {
            interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
         }
         else
         {
            interface->linkSpeed = NIC_LINK_SPEED_10MBPS;
         }

         //Determine the new duplex mode
         if((status & KSZ8851_P1SR_OPERATION_DUPLEX) != 0)
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
   if((status & KSZ8851_ISR_RXIS) != 0)
   {
      //Clear interrupt flag
      ksz8851WriteReg(interface, KSZ8851_ISR, KSZ8851_ISR_RXIS);
      //Get the total number of frames that are pending in the buffer
      frameCount = MSB(ksz8851ReadReg(interface, KSZ8851_RXFCTR));

      //Process all pending packets
      while(frameCount > 0)
      {
         //Read incoming packet
         ksz8851ReceivePacket(interface);
         //Decrement frame counter
         frameCount--;
      }
   }

   //Re-enable LCIE and RXIE interrupts
   ksz8851SetBit(interface, KSZ8851_IER, KSZ8851_IER_LCIE | KSZ8851_IER_RXIE);
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

error_t ksz8851SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   static uint8_t temp[KSZ8851_ETH_TX_BUFFER_SIZE];
   size_t n;
   size_t length;
   Ksz8851TxHeader header;
   Ksz8851Context *context;

   //Point to the driver context
   context = (Ksz8851Context *) interface->nicContext;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > KSZ8851_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Get the amount of free memory available in the TX FIFO
   n = ksz8851ReadReg(interface, KSZ8851_TXMIR) & KSZ8851_TXMIR_TXMA;

   //Make sure the TX FIFO is available for writing
   if(n < (length + 8))
   {
      return ERROR_FAILURE;
   }

   //Copy user data
   netBufferRead(temp, buffer, offset, length);

   //Format control word
   header.controlWord = htole16(KSZ8851_TX_CTRL_TXIC |
      (context->frameId++ & KSZ8851_TX_CTRL_TXFID));

   //Total number of bytes to be transmitted
   header.byteCount = htole16(length);

   //Enable TXQ write access
   ksz8851SetBit(interface, KSZ8851_RXQCR, KSZ8851_RXQCR_SDA);
   //Write TX packet header
   ksz8851WriteFifo(interface, (uint8_t *) &header, sizeof(Ksz8851TxHeader));
   //Write data
   ksz8851WriteFifo(interface, temp, length);
   //End TXQ write access
   ksz8851ClearBit(interface, KSZ8851_RXQCR, KSZ8851_RXQCR_SDA);

   //Start transmission
   ksz8851SetBit(interface, KSZ8851_TXQCR, KSZ8851_TXQCR_METFE);

   //Get the amount of free memory available in the TX FIFO
   n = ksz8851ReadReg(interface, KSZ8851_TXMIR) & KSZ8851_TXMIR_TXMA;

   //Check whether the TX FIFO is available for writing
   if(n >= (KSZ8851_ETH_TX_BUFFER_SIZE + 8))
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Receive a packet
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ksz8851ReceivePacket(NetInterface *interface)
{
   static uint8_t temp[KSZ8851_ETH_RX_BUFFER_SIZE];
   size_t n;
   uint16_t status;
   NetRxAncillary ancillary;

   //Read received frame status from RXFHSR
   status = ksz8851ReadReg(interface, KSZ8851_RXFHSR);

   //Make sure the frame is valid
   if((status & KSZ8851_RXFHSR_RXFV) != 0)
   {
      //Check error flags
      if((status & (KSZ8851_RXFHSR_RXMR | KSZ8851_RXFHSR_RXFTL |
         KSZ8851_RXFHSR_RXRF | KSZ8851_RXFHSR_RXCE)) == 0)
      {
         //Read received frame byte size from RXFHBCR
         n = ksz8851ReadReg(interface, KSZ8851_RXFHBCR) & KSZ8851_RXFHBCR_RXBC;

         //Ensure the frame size is acceptable
         if(n > 0 && n <= KSZ8851_ETH_RX_BUFFER_SIZE)
         {
            //Reset QMU RXQ frame pointer to zero
            ksz8851WriteReg(interface, KSZ8851_RXFDPR, KSZ8851_RXFDPR_RXFPAI);
            //Enable RXQ read access
            ksz8851SetBit(interface, KSZ8851_RXQCR, KSZ8851_RXQCR_SDA);
            //Read data
            ksz8851ReadFifo(interface, temp, n);
            //End RXQ read access
            ksz8851ClearBit(interface, KSZ8851_RXQCR, KSZ8851_RXQCR_SDA);

            //Additional options can be passed to the stack along with the packet
            ancillary = NET_DEFAULT_RX_ANCILLARY;

            //Pass the packet to the upper layer
            nicProcessPacket(interface, temp, n, &ancillary);
            //Valid packet received
            return NO_ERROR;
         }
      }
   }

   //Release the current error frame from RXQ
   ksz8851SetBit(interface, KSZ8851_RXQCR, KSZ8851_RXQCR_RRXEF);
   //Report an error
   return ERROR_INVALID_PACKET;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ksz8851UpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t k;
   uint32_t crc;
   uint16_t hashTable[4];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Clear hash table
   osMemset(hashTable, 0, sizeof(hashTable));

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
         crc = ksz8851CalcCrc(&entry->addr, sizeof(MacAddr));
         //Calculate the corresponding index in the table
         k = (crc >> 26) & 0x3F;
         //Update hash table contents
         hashTable[k / 16] |= (1 << (k % 16));
      }
   }

   //Write the hash table to the KSZ8851 controller
   ksz8851WriteReg(interface, KSZ8851_MAHTR0, hashTable[0]);
   ksz8851WriteReg(interface, KSZ8851_MAHTR1, hashTable[1]);
   ksz8851WriteReg(interface, KSZ8851_MAHTR2, hashTable[2]);
   ksz8851WriteReg(interface, KSZ8851_MAHTR3, hashTable[3]);

   //Debug message
   TRACE_DEBUG("  MAHTR0 = %04" PRIX16 "\r\n", ksz8851ReadReg(interface, KSZ8851_MAHTR0));
   TRACE_DEBUG("  MAHTR1 = %04" PRIX16 "\r\n", ksz8851ReadReg(interface, KSZ8851_MAHTR1));
   TRACE_DEBUG("  MAHTR2 = %04" PRIX16 "\r\n", ksz8851ReadReg(interface, KSZ8851_MAHTR2));
   TRACE_DEBUG("  MAHTR3 = %04" PRIX16 "\r\n", ksz8851ReadReg(interface, KSZ8851_MAHTR3));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Write KSZ8851 register
 * @param[in] interface Underlying network interface
 * @param[in] address Register address
 * @param[in] data Register value
 **/

void ksz8851WriteReg(NetInterface *interface, uint8_t address, uint16_t data)
{
#if (KSZ8851_SPI_SUPPORT == ENABLED)
   uint8_t command;

   //Form the write command
   if((address & 0x02) != 0)
   {
      command = KSZ8851_CMD_WR_REG | KSZ8851_CMD_B3 | KSZ8851_CMD_B2;
   }
   else
   {
      command = KSZ8851_CMD_WR_REG | KSZ8851_CMD_B1 | KSZ8851_CMD_B0;
   }

   //Pull the CS pin low
   interface->spiDriver->assertCs();

   //Command phase
   interface->spiDriver->transfer(command | (address >> 6));
   interface->spiDriver->transfer(address << 2);

   //Data phase
   interface->spiDriver->transfer(LSB(data));
   interface->spiDriver->transfer(MSB(data));

   //Terminate the operation by raising the CS pin
   interface->spiDriver->deassertCs();
#else
   //Set register address
   if((address & 0x02) != 0)
   {
      KSZ8851_CMD_REG = KSZ8851_CMD_B3 | KSZ8851_CMD_B2 | address;
   }
   else
   {
      KSZ8851_CMD_REG = KSZ8851_CMD_B1 | KSZ8851_CMD_B0 | address;
   }

   //Write register value
   KSZ8851_DATA_REG = data;
#endif
}


/**
 * @brief Read KSZ8851 register
 * @param[in] interface Underlying network interface
 * @param[in] address Register address
 * @return Register value
 **/

uint16_t ksz8851ReadReg(NetInterface *interface, uint8_t address)
{
#if (KSZ8851_SPI_SUPPORT == ENABLED)
   uint8_t command;
   uint16_t data;

   //Form the read command
   if((address & 0x02) != 0)
   {
      command = KSZ8851_CMD_RD_REG | KSZ8851_CMD_B3 | KSZ8851_CMD_B2;
   }
   else
   {
      command = KSZ8851_CMD_RD_REG | KSZ8851_CMD_B1 | KSZ8851_CMD_B0;
   }

   //Pull the CS pin low
   interface->spiDriver->assertCs();

   //Command phase
   interface->spiDriver->transfer(command | (address >> 6));
   interface->spiDriver->transfer(address << 2);

   //Data phase (lower 8 bits)
   data = interface->spiDriver->transfer(0x00);
   //Data phase (upper 8 bits)
   data |= interface->spiDriver->transfer(0x00) << 8;

   //Terminate the operation by raising the CS pin
   interface->spiDriver->deassertCs();

   //Return register value
   return data;
#else
   //Set register address
   if((address & 0x02) != 0)
   {
      KSZ8851_CMD_REG = KSZ8851_CMD_B3 | KSZ8851_CMD_B2 | address;
   }
   else
   {
      KSZ8851_CMD_REG = KSZ8851_CMD_B1 | KSZ8851_CMD_B0 | address;
   }

   //Return register value
   return KSZ8851_DATA_REG;
#endif
}


/**
 * @brief Write TX FIFO
 * @param[in] interface Underlying network interface
 * @param[in] data Pointer to the data being written
 * @param[in] length Number of data to write
 **/

void ksz8851WriteFifo(NetInterface *interface, const uint8_t *data,
   size_t length)
{
#if (KSZ8851_SPI_SUPPORT == ENABLED)
   uint_t i;

   //Pull the CS pin low
   interface->spiDriver->assertCs();

   //Command phase
   interface->spiDriver->transfer(KSZ8851_CMD_WR_FIFO);

   //Data phase
   for(i = 0; i < length; i++)
   {
      interface->spiDriver->transfer(data[i]);
   }

   //Maintain alignment to 4-byte boundaries
   for(; (i % 4) != 0; i++)
   {
      interface->spiDriver->transfer(0x00);
   }

   //Terminate the operation by raising the CS pin
   interface->spiDriver->deassertCs();
#else
   uint_t i;

   //Data phase
   for(i = 0; i < length; i += 2)
   {
      KSZ8851_DATA_REG = data[i] | data[i + 1] << 8;
   }

   //Maintain alignment to 4-byte boundaries
   for(; (i % 4) != 0; i += 2)
   {
      KSZ8851_DATA_REG = 0x0000;
   }
#endif
}


/**
 * @brief Read RX FIFO
 * @param[in] interface Underlying network interface
 * @param[in] data Buffer where to store the incoming data
 * @param[in] length Number of data to read
 **/

void ksz8851ReadFifo(NetInterface *interface, uint8_t *data, size_t length)
{
#if (KSZ8851_SPI_SUPPORT == ENABLED)
   uint_t i;

   //Pull the CS pin low
   interface->spiDriver->assertCs();

   //Command phase
   interface->spiDriver->transfer(KSZ8851_CMD_RD_FIFO);

   //The first 4 bytes are dummy data and must be discarded
   for(i = 0; i < 4; i++)
   {
      interface->spiDriver->transfer(0x00);
   }

   //Ignore RX packet header
   for(i = 0; i < 4; i++)
   {
      interface->spiDriver->transfer(0x00);
   }

   //Data phase
   for(i = 0; i < length; i++)
   {
      data[i] = interface->spiDriver->transfer(0x00);
   }

   //Maintain alignment to 4-byte boundaries
   for(; (i % 4) != 0; i++)
   {
      interface->spiDriver->transfer(0x00);
   }

   //Terminate the operation by raising the CS pin
   interface->spiDriver->deassertCs();
#else
   uint_t i;
   uint16_t temp;

   //The first 2 bytes are dummy data and must be discarded
   temp = KSZ8851_DATA_REG;

   //Ignore RX packet header
   temp = KSZ8851_DATA_REG;
   temp = KSZ8851_DATA_REG;

   //Data phase
   for(i = 0; i < length; i += 2)
   {
      temp = KSZ8851_DATA_REG;
      data [i] = temp & 0xFF;
      data [i + 1] = (temp >> 8) & 0xFF;
   }

   //Maintain alignment to 4-byte boundaries
   for(; (i % 4) != 0; i += 2)
   {
      temp = KSZ8851_DATA_REG;
   }
#endif
}


/**
 * @brief Set bit field
 * @param[in] interface Underlying network interface
 * @param[in] address Register address
 * @param[in] mask Bits to set in the target register
 **/

void ksz8851SetBit(NetInterface *interface, uint8_t address, uint16_t mask)
{
   uint16_t value;

   //Read current register value
   value = ksz8851ReadReg(interface, address);
   //Set specified bits
   ksz8851WriteReg(interface, address, value | mask);
}


/**
 * @brief Clear bit field
 * @param[in] interface Underlying network interface
 * @param[in] address Register address
 * @param[in] mask Bits to clear in the target register
 **/

void ksz8851ClearBit(NetInterface *interface, uint8_t address, uint16_t mask)
{
   uint16_t value;

   //Read current register value
   value = ksz8851ReadReg(interface, address);
   //Clear specified bits
   ksz8851WriteReg(interface, address, value & ~mask);
}


/**
 * @brief CRC calculation
 * @param[in] data Pointer to the data over which to calculate the CRC
 * @param[in] length Number of bytes to process
 * @return Resulting CRC value
 **/

uint32_t ksz8851CalcCrc(const void *data, size_t length)
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
      //The message is processed bit by bit
      for(j = 0; j < 8; j++)
      {
         //Update CRC value
         if((((crc >> 31) ^ (p[i] >> j)) & 0x01) != 0)
         {
            crc = (crc << 1) ^ 0x04C11DB7;
         }
         else
         {
            crc = crc << 1;
         }
      }
   }

   //Return CRC value
   return crc;
}


/**
 * @brief Dump registers for debugging purpose
 * @param[in] interface Underlying network interface
 **/

void ksz8851DumpReg(NetInterface *interface)
{
#if (TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   uint_t i;
   uint_t j;
   uint_t address;

   //Loop through register addresses
   for(i = 0; i < 256; i += 16)
   {
      //Display register address
      TRACE_DEBUG("%02" PRIu8 ": ", i);

      //Display 8 registers at a time
      for(j = 0; j < 16; j += 2)
      {
         //Format register address
         address = i + j;
         //Display register contents
         TRACE_DEBUG("0x%04" PRIX16 "  ", ksz8851ReadReg(interface, address));
      }

      //Jump to the following line
      TRACE_DEBUG("\r\n");
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
#endif
}
