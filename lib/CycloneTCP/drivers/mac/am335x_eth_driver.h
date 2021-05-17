/**
 * @file am335x_eth_driver.h
 * @brief Sitara AM335x Gigabit Ethernet MAC driver
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

#ifndef _AM335X_ETH_DRIVER_H
#define _AM335X_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef AM335X_ETH_TX_BUFFER_COUNT
   #define AM335X_ETH_TX_BUFFER_COUNT 16
#elif (AM335X_ETH_TX_BUFFER_COUNT < 1)
   #error AM335X_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef AM335X_ETH_TX_BUFFER_SIZE
   #define AM335X_ETH_TX_BUFFER_SIZE 1536
#elif (AM335X_ETH_TX_BUFFER_SIZE != 1536)
   #error AM335X_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef AM335X_ETH_RX_BUFFER_COUNT
   #define AM335X_ETH_RX_BUFFER_COUNT 16
#elif (AM335X_ETH_RX_BUFFER_COUNT < 1)
   #error AM335X_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef AM335X_ETH_RX_BUFFER_SIZE
   #define AM335X_ETH_RX_BUFFER_SIZE 1536
#elif (AM335X_ETH_RX_BUFFER_SIZE != 1536)
   #error AM335X_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Ethernet interrupt priority
#ifndef AM335X_ETH_IRQ_PRIORITY
   #define AM335X_ETH_IRQ_PRIORITY 1
#elif (AM335X_ETH_IRQ_PRIORITY < 0)
   #error AM335X_ETH_IRQ_PRIORITY parameter is not valid
#endif

//Name of the section where to place DMA buffers
#ifndef AM335X_ETH_RAM_SECTION
   #define AM335X_ETH_RAM_SECTION ".ram_no_cache"
#endif

//Name of the section where to place DMA descriptors
#ifndef AM335X_ETH_RAM_CPPI_SECTION
   #define AM335X_ETH_RAM_CPPI_SECTION ".ram_cppi"
#endif

//CPSW interrupts
#ifndef SYS_INT_3PGSWRXINT0
  #define SYS_INT_3PGSWRXINT0 41
#endif

#ifndef SYS_INT_3PGSWTXINT0
  #define SYS_INT_3PGSWTXINT0 42
#endif

//CPSW cores
#define CPSW_CORE0 0
#define CPSW_CORE1 1
#define CPSW_CORE2 2

//CPSW ports
#define CPSW_PORT0 0
#define CPSW_PORT1 1
#define CPSW_PORT2 2

//CPSW channels
#define CPSW_CH0 0
#define CPSW_CH1 1
#define CPSW_CH2 2
#define CPSW_CH3 3
#define CPSW_CH4 4
#define CPSW_CH5 5
#define CPSW_CH6 6
#define CPSW_CH7 7

//PRCM registers
#define CM_PER_CPGMAC0_CLKCTRL_R        HWREG(SOC_PRCM_REGS + CM_PER_CPGMAC0_CLKCTRL)
#define CM_PER_CPSW_CLKSTCTRL_R         HWREG(SOC_PRCM_REGS + CM_PER_CPSW_CLKSTCTRL)

//CONTROL registers
#define CONTROL_MAC_ID_LO_R(n)          HWREG(SOC_CONTROL_REGS + CONTROL_MAC_ID_LO(n))
#define CONTROL_MAC_ID_HI_R(n)          HWREG(SOC_CONTROL_REGS + CONTROL_MAC_ID_HI(n))
#define CONTROL_GMII_SEL_R              HWREG(SOC_CONTROL_REGS + CONTROL_GMII_SEL)
#define CONTROL_CONF_GPMC_A_R(n)        HWREG(SOC_CONTROL_REGS + CONTROL_CONF_GPMC_A(n))
#define CONTROL_CONF_MII1_COL_R         HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_COL)
#define CONTROL_CONF_MII1_CRS_R         HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_CRS)
#define CONTROL_CONF_MII1_RXERR_R       HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXERR)
#define CONTROL_CONF_MII1_TXEN_R        HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_TXEN)
#define CONTROL_CONF_MII1_RXDV_R        HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXDV)
#define CONTROL_CONF_MII1_TXD3_R        HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_TXD3)
#define CONTROL_CONF_MII1_TXD2_R        HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_TXD2)
#define CONTROL_CONF_MII1_TXD1_R        HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_TXD1)
#define CONTROL_CONF_MII1_TXD0_R        HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_TXD0)
#define CONTROL_CONF_MII1_TXCLK_R       HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_TXCLK)
#define CONTROL_CONF_MII1_RXCLK_R       HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXCLK)
#define CONTROL_CONF_MII1_RXD3_R        HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXD3)
#define CONTROL_CONF_MII1_RXD2_R        HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXD2)
#define CONTROL_CONF_MII1_RXD1_R        HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXD1)
#define CONTROL_CONF_MII1_RXD0_R        HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXD0)
#define CONTROL_CONF_RMII1_REFCLK_R     HWREG(SOC_CONTROL_REGS + CONTROL_CONF_RMII1_REFCLK)
#define CONTROL_CONF_MDIO_DATA_R        HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MDIO_DATA)
#define CONTROL_CONF_MDIO_CLK_R         HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MDIO_CLK)

//CPSW_ALE registers
#define CPSW_ALE_IDVER_R                HWREG(SOC_CPSW_ALE_REGS + CPSW_ALE_IDVER)
#define CPSW_ALE_CTRL_R                 HWREG(SOC_CPSW_ALE_REGS + CPSW_ALE_CTRL)
#define CPSW_ALE_PRESCALE_R             HWREG(SOC_CPSW_ALE_REGS + CPSW_ALE_PRESCALE)
#define CPSW_ALE_UNKNOWN_VLAN_R         HWREG(SOC_CPSW_ALE_REGS + CPSW_ALE_UNKNOWN_VLAN)
#define CPSW_ALE_TBLCTL_R               HWREG(SOC_CPSW_ALE_REGS + CPSW_ALE_TBLCTL)
#define CPSW_ALE_TBLW_R(n)              HWREG(SOC_CPSW_ALE_REGS + CPSW_ALE_TBLW(n))
#define CPSW_ALE_PORTCTL_R(n)           HWREG(SOC_CPSW_ALE_REGS + CPSW_ALE_PORTCTL(n))

//CPSW_CPDMA registers
#define CPSW_CPDMA_TX_IDVER_R           HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_TX_IDVER)
#define CPSW_CPDMA_TX_CTRL_R            HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_TX_CTRL)
#define CPSW_CPDMA_TX_TEARDOWN_R        HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_TX_TEARDOWN)
#define CPSW_CPDMA_RX_IDVER_R           HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_RX_IDVER)
#define CPSW_CPDMA_RX_CTRL_R            HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_RX_CTRL)
#define CPSW_CPDMA_RX_TEARDOWN_R        HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_RX_TEARDOWN)
#define CPSW_CPDMA_SOFT_RESET_R         HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_SOFT_RESET)
#define CPSW_CPDMA_DMACONTROL_R         HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_DMACONTROL)
#define CPSW_CPDMA_DMASTATUS_R          HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_DMASTATUS)
#define CPSW_CPDMA_RX_BUFFER_OFFSET_R   HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_RX_BUFFER_OFFSET)
#define CPSW_CPDMA_EMCONTROL_R          HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_EMCONTROL)
#define CPSW_CPDMA_TX_PRI_RATE_R(n)     HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_TX_PRI_RATE(n))
#define CPSW_CPDMA_TX_INTSTAT_RAW_R     HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_TX_INTSTAT_RAW)
#define CPSW_CPDMA_TX_INTSTAT_MASKED_R  HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_TX_INTSTAT_MASKED)
#define CPSW_CPDMA_TX_INTMASK_SET_R     HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_TX_INTMASK_SET)
#define CPSW_CPDMA_TX_INTMASK_CLEAR_R   HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_TX_INTMASK_CLEAR)
#define CPSW_CPDMA_IN_VECTOR_R          HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_IN_VECTOR)
#define CPSW_CPDMA_EOI_VECTOR_R         HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_EOI_VECTOR)
#define CPSW_CPDMA_RX_INTSTAT_RAW_R     HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_RX_INTSTAT_RAW)
#define CPSW_CPDMA_RX_INTSTAT_MASKED_R  HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_RX_INTSTAT_MASKED)
#define CPSW_CPDMA_RX_INTMASK_SET_R     HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_RX_INTMASK_SET)
#define CPSW_CPDMA_RX_INTMASK_CLEAR_R   HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_RX_INTMASK_CLEAR)
#define CPSW_CPDMA_DMA_INTSTAT_RAW_R    HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_DMA_INTSTAT_RAW)
#define CPSW_CPDMA_DMA_INTSTAT_MASKED_R HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_DMA_INTSTAT_MASKED)
#define CPSW_CPDMA_DMA_INTMASK_SET_R    HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_DMA_INTMASK_SET)
#define CPSW_CPDMA_DMA_INTMASK_CLEAR_R  HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_DMA_INTMASK_CLEAR)
#define CPSW_CPDMA_RX_PENDTHRESH_R(n)   HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_RX_PENDTHRESH(n))
#define CPSW_CPDMA_RX_FREEBUFFER_R(n)   HWREG(SOC_CPSW_CPDMA_REGS + CPSW_CPDMA_RX_FREEBUFFER(n))
#define CPSW_CPDMA_STATERAM_TX_HDP_R(n) HWREG(SOC_CPSW_CPDMA_REGS + 0x200 + CPSW_CPDMA_STATERAM_TX_HDP(n))
#define CPSW_CPDMA_STATERAM_RX_HDP_R(n) HWREG(SOC_CPSW_CPDMA_REGS + 0x200 + CPSW_CPDMA_STATERAM_RX_HDP(n))
#define CPSW_CPDMA_STATERAM_TX_CP_R(n)  HWREG(SOC_CPSW_CPDMA_REGS + 0x200 + CPSW_CPDMA_STATERAM_TX_CP(n))
#define CPSW_CPDMA_STATERAM_RX_CP_R(n)  HWREG(SOC_CPSW_CPDMA_REGS + 0x200 + CPSW_CPDMA_STATERAM_RX_CP(n))

//CPSW_PORT registers
#define CPSW_PORT0_CTRL_R               HWREG(SOC_CPSW_PORT_0_REGS + CPSW_PORT_P_CTRL(0))
#define CPSW_PORT0_MAX_BLKS_R           HWREG(SOC_CPSW_PORT_0_REGS + CPSW_PORT_P_MAX_BLKS(0))
#define CPSW_PORT0_BLK_CNT_R            HWREG(SOC_CPSW_PORT_0_REGS + CPSW_PORT_P_BLK_CNT(0))
#define CPSW_PORT0_TX_IN_CTL_R          HWREG(SOC_CPSW_PORT_0_REGS + CPSW_PORT_P_TX_IN_CTL(0))
#define CPSW_PORT0_VLAN_R               HWREG(SOC_CPSW_PORT_0_REGS + CPSW_PORT_P_VLAN(0))
#define CPSW_PORT0_TX_PRI_MAP_R         HWREG(SOC_CPSW_PORT_0_REGS + CPSW_PORT_P_TX_PRI_MAP(0))
#define CPSW_PORT0_CPDMA_TX_PRI_MAP_R   HWREG(SOC_CPSW_PORT_0_REGS + CPSW_PORT_P0_CPDMA_TX_PRI_MAP)
#define CPSW_PORT0_CPDMA_RX_CH_MAP_R    HWREG(SOC_CPSW_PORT_0_REGS + CPSW_PORT_P0_CPDMA_RX_CH_MAP)
#define CPSW_PORT0_RX_DSCP_PRI_MAP_R(n) HWREG(SOC_CPSW_PORT_0_REGS + CPSW_PORT_P_RX_DSCP_PRI_MAP(0, n))
#define CPSW_PORT0_TS_SEQ_MTYPE_R       HWREG(SOC_CPSW_PORT_0_REGS + CPSW_PORT_P_TS_SEQ_MTYPE(0))
#define CPSW_PORT0_SA_LO_R              HWREG(SOC_CPSW_PORT_0_REGS + CPSW_PORT_P_SA_LO(0))
#define CPSW_PORT0_SA_HI_R              HWREG(SOC_CPSW_PORT_0_REGS + CPSW_PORT_P_SA_HI(0))
#define CPSW_PORT0_SEND_PERCENT_R       HWREG(SOC_CPSW_PORT_0_REGS + CPSW_PORT_P_SEND_PERCENT(0))

#define CPSW_PORT1_CTRL_R               HWREG(SOC_CPSW_PORT_1_REGS + CPSW_PORT_P_CTRL(0))
#define CPSW_PORT1_MAX_BLKS_R           HWREG(SOC_CPSW_PORT_1_REGS + CPSW_PORT_P_MAX_BLKS(0))
#define CPSW_PORT1_BLK_CNT_R            HWREG(SOC_CPSW_PORT_1_REGS + CPSW_PORT_P_BLK_CNT(0))
#define CPSW_PORT1_TX_IN_CTL_R          HWREG(SOC_CPSW_PORT_1_REGS + CPSW_PORT_P_TX_IN_CTL(0))
#define CPSW_PORT1_VLAN_R               HWREG(SOC_CPSW_PORT_1_REGS + CPSW_PORT_P_VLAN(0))
#define CPSW_PORT1_TX_PRI_MAP_R         HWREG(SOC_CPSW_PORT_1_REGS + CPSW_PORT_P_TX_PRI_MAP(0))
#define CPSW_PORT1_RX_DSCP_PRI_MAP_R(n) HWREG(SOC_CPSW_PORT_1_REGS + CPSW_PORT_P_RX_DSCP_PRI_MAP(0, n))
#define CPSW_PORT1_TS_SEQ_MTYPE_R       HWREG(SOC_CPSW_PORT_1_REGS + CPSW_PORT_P_TS_SEQ_MTYPE(0))
#define CPSW_PORT1_SA_LO_R              HWREG(SOC_CPSW_PORT_1_REGS + CPSW_PORT_P_SA_LO(0))
#define CPSW_PORT1_SA_HI_R              HWREG(SOC_CPSW_PORT_1_REGS + CPSW_PORT_P_SA_HI(0))
#define CPSW_PORT1_SEND_PERCENT_R       HWREG(SOC_CPSW_PORT_1_REGS + CPSW_PORT_P_SEND_PERCENT(0))

#define CPSW_PORT2_CTRL_R               HWREG(SOC_CPSW_PORT_2_REGS + CPSW_PORT_P_CTRL(0))
#define CPSW_PORT2_MAX_BLKS_R           HWREG(SOC_CPSW_PORT_2_REGS + CPSW_PORT_P_MAX_BLKS(0))
#define CPSW_PORT2_BLK_CNT_R            HWREG(SOC_CPSW_PORT_2_REGS + CPSW_PORT_P_BLK_CNT(0))
#define CPSW_PORT2_TX_IN_CTL_R          HWREG(SOC_CPSW_PORT_2_REGS + CPSW_PORT_P_TX_IN_CTL(0))
#define CPSW_PORT2_VLAN_R               HWREG(SOC_CPSW_PORT_2_REGS + CPSW_PORT_P_VLAN(0))
#define CPSW_PORT2_TX_PRI_MAP_R         HWREG(SOC_CPSW_PORT_2_REGS + CPSW_PORT_P_TX_PRI_MAP(0))
#define CPSW_PORT2_RX_DSCP_PRI_MAP_R(n) HWREG(SOC_CPSW_PORT_2_REGS + CPSW_PORT_P_RX_DSCP_PRI_MAP(0, n))
#define CPSW_PORT2_TS_SEQ_MTYPE_R       HWREG(SOC_CPSW_PORT_2_REGS + CPSW_PORT_P_TS_SEQ_MTYPE(0))
#define CPSW_PORT2_SA_LO_R              HWREG(SOC_CPSW_PORT_2_REGS + CPSW_PORT_P_SA_LO(0))
#define CPSW_PORT2_SA_HI_R              HWREG(SOC_CPSW_PORT_2_REGS + CPSW_PORT_P_SA_HI(0))
#define CPSW_PORT2_SEND_PERCENT_R       HWREG(SOC_CPSW_PORT_2_REGS + CPSW_PORT_P_SEND_PERCENT(0))

//CPSW_SL registers
#define CPSW_SL1_IDVER_R                HWREG(SOC_CPSW_SLIVER_1_REGS + CPSW_SL_IDVER)
#define CPSW_SL1_MACCTRL_R              HWREG(SOC_CPSW_SLIVER_1_REGS + CPSW_SL_MACCTRL)
#define CPSW_SL1_MACSTS_R               HWREG(SOC_CPSW_SLIVER_1_REGS + CPSW_SL_MACSTS)
#define CPSW_SL1_SOFT_RESET_R           HWREG(SOC_CPSW_SLIVER_1_REGS + CPSW_SL_SOFT_RESET)
#define CPSW_SL1_RX_MAXLEN_R            HWREG(SOC_CPSW_SLIVER_1_REGS + CPSW_SL_RX_MAXLEN)
#define CPSW_SL1_BOFFTEST_R             HWREG(SOC_CPSW_SLIVER_1_REGS + CPSW_SL_BOFFTEST)
#define CPSW_SL1_RX_PAUSE_R             HWREG(SOC_CPSW_SLIVER_1_REGS + CPSW_SL_RX_PAUSE)
#define CPSW_SL1_TX_PAUSE_R             HWREG(SOC_CPSW_SLIVER_1_REGS + CPSW_SL_TX_PAUSE)
#define CPSW_SL1_EMCTRL_R               HWREG(SOC_CPSW_SLIVER_1_REGS + CPSW_SL_EMCTRL)
#define CPSW_SL1_RX_PRI_MAP_R           HWREG(SOC_CPSW_SLIVER_1_REGS + CPSW_SL_RX_PRI_MAP)
#define CPSW_SL1_TX_GAP_R               HWREG(SOC_CPSW_SLIVER_1_REGS + CPSW_SL_TX_GAP)

#define CPSW_SL2_IDVER_R                HWREG(SOC_CPSW_SLIVER_2_REGS + CPSW_SL_IDVER)
#define CPSW_SL2_MACCTRL_R              HWREG(SOC_CPSW_SLIVER_2_REGS + CPSW_SL_MACCTRL)
#define CPSW_SL2_MACSTS_R               HWREG(SOC_CPSW_SLIVER_2_REGS + CPSW_SL_MACSTS)
#define CPSW_SL2_SOFT_RESET_R           HWREG(SOC_CPSW_SLIVER_2_REGS + CPSW_SL_SOFT_RESET)
#define CPSW_SL2_RX_MAXLEN_R            HWREG(SOC_CPSW_SLIVER_2_REGS + CPSW_SL_RX_MAXLEN)
#define CPSW_SL2_BOFFTEST_R             HWREG(SOC_CPSW_SLIVER_2_REGS + CPSW_SL_BOFFTEST)
#define CPSW_SL2_RX_PAUSE_R             HWREG(SOC_CPSW_SLIVER_2_REGS + CPSW_SL_RX_PAUSE)
#define CPSW_SL2_TX_PAUSE_R             HWREG(SOC_CPSW_SLIVER_2_REGS + CPSW_SL_TX_PAUSE)
#define CPSW_SL2_EMCTRL_R               HWREG(SOC_CPSW_SLIVER_2_REGS + CPSW_SL_EMCTRL)
#define CPSW_SL2_RX_PRI_MAP_R           HWREG(SOC_CPSW_SLIVER_2_REGS + CPSW_SL_RX_PRI_MAP)
#define CPSW_SL2_TX_GAP_R               HWREG(SOC_CPSW_SLIVER_2_REGS + CPSW_SL_TX_GAP)

//CPSW_SS registers
#define CPSW_SS_ID_VER_R                HWREG(SOC_CPSW_SS_REGS + CPSW_SS_ID_VER)
#define CPSW_SS_CTRL_R                  HWREG(SOC_CPSW_SS_REGS + CPSW_SS_CTRL)
#define CPSW_SS_SOFT_RESET_R            HWREG(SOC_CPSW_SS_REGS + CPSW_SS_SOFT_RESET)
#define CPSW_SS_STAT_PORT_EN_R          HWREG(SOC_CPSW_SS_REGS + CPSW_SS_STAT_PORT_EN)
#define CPSW_SS_PTYPE_R                 HWREG(SOC_CPSW_SS_REGS + CPSW_SS_PTYPE)
#define CPSW_SS_SOFT_IDLE_R             HWREG(SOC_CPSW_SS_REGS + CPSW_SS_SOFT_IDLE)
#define CPSW_SS_THRU_RATE_R             HWREG(SOC_CPSW_SS_REGS + CPSW_SS_THRU_RATE)
#define CPSW_SS_GAP_THRESH_R            HWREG(SOC_CPSW_SS_REGS + CPSW_SS_GAP_THRESH)
#define CPSW_SS_TX_START_WDS_R          HWREG(SOC_CPSW_SS_REGS + CPSW_SS_TX_START_WDS)
#define CPSW_SS_FLOW_CTRL_R             HWREG(SOC_CPSW_SS_REGS + CPSW_SS_FLOW_CTRL)
#define CPSW_SS_VLAN_LTYPE_R            HWREG(SOC_CPSW_SS_REGS + CPSW_SS_VLAN_LTYPE)
#define CPSW_SS_TS_LTYPE_R              HWREG(SOC_CPSW_SS_REGS + CPSW_SS_TS_LTYPE)
#define CPSW_SS_DLR_LTYPE_R             HWREG(SOC_CPSW_SS_REGS + CPSW_SS_DLR_LTYPE)

//CPSW_WR registers
#define CPSW_WR_IDVER_R                 HWREG(SOC_CPSW_WR_REGS + CPSW_WR_IDVER)
#define CPSW_WR_SOFT_RESET_R            HWREG(SOC_CPSW_WR_REGS + CPSW_WR_SOFT_RESET)
#define CPSW_WR_CTRL_R                  HWREG(SOC_CPSW_WR_REGS + CPSW_WR_CTRL)
#define CPSW_WR_INT_CTRL_R              HWREG(SOC_CPSW_WR_REGS + CPSW_WR_INT_CTRL)
#define CPSW_WR_C_RX_THRESH_EN_R(n)     HWREG(SOC_CPSW_WR_REGS + CPSW_WR_C_RX_THRESH_EN(n))
#define CPSW_WR_C_RX_EN_R(n)            HWREG(SOC_CPSW_WR_REGS + CPSW_WR_C_RX_EN(n))
#define CPSW_WR_C_TX_EN_R(n)            HWREG(SOC_CPSW_WR_REGS + CPSW_WR_C_TX_EN(n))
#define CPSW_WR_C_MISC_EN_R(n)          HWREG(SOC_CPSW_WR_REGS + CPSW_WR_C_MISC_EN(n))
#define CPSW_WR_C_RX_THRESH_STAT_R(n)   HWREG(SOC_CPSW_WR_REGS + CPSW_WR_C_RX_THRESH_STAT(n))
#define CPSW_WR_C_RX_STAT_R(n)          HWREG(SOC_CPSW_WR_REGS + CPSW_WR_C_RX_STAT(n))
#define CPSW_WR_C_TX_STAT_R(n)          HWREG(SOC_CPSW_WR_REGS + CPSW_WR_C_TX_STAT(n))
#define CPSW_WR_C_MISC_STAT_R(n)        HWREG(SOC_CPSW_WR_REGS + CPSW_WR_C_MISC_STAT(n))
#define CPSW_WR_C_RX_IMAX_R(n)          HWREG(SOC_CPSW_WR_REGS + CPSW_WR_C_RX_IMAX(n))
#define CPSW_WR_C_TX_IMAX_R(n)          HWREG(SOC_CPSW_WR_REGS + CPSW_WR_C_TX_IMAX(n))
#define CPSW_WR_RGMII_CTL_R             HWREG(SOC_CPSW_WR_REGS + CPSW_WR_RGMII_CTL)

//MDIO registers
#define MDIO_REVID_R                    HWREG(SOC_CPSW_MDIO_REGS + MDIO_REVID)
#define MDIO_CTRL_R                     HWREG(SOC_CPSW_MDIO_REGS + MDIO_CTRL)
#define MDIO_ALIVE_R                    HWREG(SOC_CPSW_MDIO_REGS + MDIO_ALIVE)
#define MDIO_LINK_R                     HWREG(SOC_CPSW_MDIO_REGS + MDIO_LINK)
#define MDIO_LINKINTRAW_R               HWREG(SOC_CPSW_MDIO_REGS + MDIO_LINKINTRAW)
#define MDIO_LINKINTMASKED_R            HWREG(SOC_CPSW_MDIO_REGS + MDIO_LINKINTMASKED)
#define MDIO_USERINTRAW_R               HWREG(SOC_CPSW_MDIO_REGS + MDIO_USERINTRAW)
#define MDIO_USERINTMASKED_R            HWREG(SOC_CPSW_MDIO_REGS + MDIO_USERINTMASKED)
#define MDIO_USERINTMASKSET_R           HWREG(SOC_CPSW_MDIO_REGS + MDIO_USERINTMASKSET)
#define MDIO_USERINTMASKCLEAR_R         HWREG(SOC_CPSW_MDIO_REGS + MDIO_USERINTMASKCLEAR)
#define MDIO_USERACCESS_R(n)            HWREG(SOC_CPSW_MDIO_REGS + MDIO_USERACCESS(n))
#define MDIO_USERPHYSEL_R(n)            HWREG(SOC_CPSW_MDIO_REGS + MDIO_USERPHYSEL(n))

//GMII_SEL register
#define CONTROL_GMII_SEL_GMII2_SEL_MII        0x00000000
#define CONTROL_GMII_SEL_GMII2_SEL_RMII       0x00000004
#define CONTROL_GMII_SEL_GMII2_SEL_RGMII      0x00000008
#define CONTROL_GMII_SEL_GMII1_SEL_MII        0x00000000
#define CONTROL_GMII_SEL_GMII1_SEL_RMII       0x00000001
#define CONTROL_GMII_SEL_GMII1_SEL_RGMII      0x00000002

//CPDMA_EOI_VECTOR register
#define CPSW_CPDMA_EOI_VECTOR_RX_THRESH_PULSE 0x00000000
#define CPSW_CPDMA_EOI_VECTOR_RX_PULSE        0x00000001
#define CPSW_CPDMA_EOI_VECTOR_TX_PULSE        0x00000002
#define CPSW_CPDMA_EOI_VECTOR_MISC_PULSE      0x00000003

//CPSW_PORT_P_TX_IN_CTL register
#define CPSW_PORT_P_TX_IN_CTL_SEL_DUAL_MAC    0x00010000

//TX buffer descriptor flags
#define CPSW_TX_WORD0_NEXT_DESC_POINTER       0xFFFFFFFF
#define CPSW_TX_WORD1_BUFFER_POINTER          0xFFFFFFFF
#define CPSW_TX_WORD2_BUFFER_OFFSET           0xFFFF0000
#define CPSW_TX_WORD2_BUFFER_LENGTH           0x0000FFFF
#define CPSW_TX_WORD3_SOP                     0x80000000
#define CPSW_TX_WORD3_EOP                     0x40000000
#define CPSW_TX_WORD3_OWNER                   0x20000000
#define CPSW_TX_WORD3_EOQ                     0x10000000
#define CPSW_TX_WORD3_TDOWN_CMPLT             0x08000000
#define CPSW_TX_WORD3_PASS_CRC                0x04000000
#define CPSW_TX_WORD3_TO_PORT_EN              0x00100000
#define CPSW_TX_WORD3_TO_PORT                 0x00030000
#define CPSW_TX_WORD3_TO_PORT_1               0x00010000
#define CPSW_TX_WORD3_TO_PORT_2               0x00020000
#define CPSW_TX_WORD3_PACKET_LENGTH           0x000007FF

//RX buffer descriptor flags
#define CPSW_RX_WORD0_NEXT_DESC_POINTER       0xFFFFFFFF
#define CPSW_RX_WORD1_BUFFER_POINTER          0xFFFFFFFF
#define CPSW_RX_WORD2_BUFFER_OFFSET           0x07FF0000
#define CPSW_RX_WORD2_BUFFER_LENGTH           0x000007FF
#define CPSW_RX_WORD3_SOP                     0x80000000
#define CPSW_RX_WORD3_EOP                     0x40000000
#define CPSW_RX_WORD3_OWNER                   0x20000000
#define CPSW_RX_WORD3_EOQ                     0x10000000
#define CPSW_RX_WORD3_TDOWN_CMPLT             0x08000000
#define CPSW_RX_WORD3_PASS_CRC                0x04000000
#define CPSW_RX_WORD3_LONG                    0x02000000
#define CPSW_RX_WORD3_SHORT                   0x01000000
#define CPSW_RX_WORD3_CONTROL                 0x00800000
#define CPSW_RX_WORD3_OVERRUN                 0x00400000
#define CPSW_RX_WORD3_PKT_ERROR               0x00300000
#define CPSW_RX_WORD3_RX_VLAN_ENCAP           0x000C0000
#define CPSW_RX_WORD3_FROM_PORT               0x00030000
#define CPSW_RX_WORD3_FROM_PORT_1             0x00010000
#define CPSW_RX_WORD3_FROM_PORT_2             0x00020000
#define CPSW_RX_WORD3_PACKET_LENGTH           0x000007FF

//Number of entries in the ALE table
#define CPSW_ALE_MAX_ENTRIES                   1024

//ALE table entry
#define CPSW_ALE_WORD1_ENTRY_TYPE_MASK         (3 << 28)
#define CPSW_ALE_WORD1_ENTRY_TYPE_FREE         (0 << 28)
#define CPSW_ALE_WORD1_ENTRY_TYPE_ADDR         (1 << 28)
#define CPSW_ALE_WORD1_ENTRY_TYPE_VLAN         (2 << 28)
#define CPSW_ALE_WORD1_ENTRY_TYPE_VLAN_ADDR    (3 << 28)
#define CPSW_ALE_WORD1_MULTICAST               (1 << 8)

//Unicast address table entry
#define CPSW_ALE_WORD2_DLR_UNICAST             (1 << 5)
#define CPSW_ALE_WORD2_PORT_NUMBER_MASK        (3 << 2)
#define CPSW_ALE_WORD2_PORT_NUMBER(n)          ((n) << 2)
#define CPSW_ALE_WORD2_BLOCK                   (1 << 1)
#define CPSW_ALE_WORD2_SECURE                  (1 << 0)
#define CPSW_ALE_WORD1_UNICAST_TYPE_MASK       (3 << 30)
#define CPSW_ALE_WORD1_UNICAST_TYPE(n)         ((n) << 30)

//Multicast address table entry
#define CPSW_ALE_WORD2_PORT_LIST_MASK          (3 << 2)
#define CPSW_ALE_WORD2_PORT_LIST(n)            ((n) << 2)
#define CPSW_ALE_WORD2_SUPER                   (1 << 1)
#define CPSW_ALE_WORD1_MCAST_FWD_STATE_MASK    (3 << 30)
#define CPSW_ALE_WORD1_MCAST_FWD_STATE(n)      ((n) << 30)

//VLAN table entry
#define CPSW_ALE_WORD1_VLAN_ID_MASK            (4095 << 16)
#define CPSW_ALE_WORD1_VLAN_ID(n)              ((n) << 16)
#define CPSW_ALE_WORD0_FORCE_UNTAG_EGRESS_MASK (7 << 24)
#define CPSW_ALE_WORD0_FORCE_UNTAG_EGRESS(n)   ((n) << 24)
#define CPSW_ALE_WORD0_REG_MCAST_FLOOD_MASK    (7 << 16)
#define CPSW_ALE_WORD0_REG_MCAST_FLOOD(n)      ((n) << 16)
#define CPSW_ALE_WORD0_UNREG_MCAST_FLOOD_MASK  (7 << 8)
#define CPSW_ALE_WORD0_UNREG_MCAST_FLOOD(n)    ((n) << 8)
#define CPSW_ALE_WORD0_VLAN_MEMBER_LIST_MASK   (7 << 0)
#define CPSW_ALE_WORD0_VLAN_MEMBER_LIST(n)     ((n) << 0)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief ALE table entry
 **/

typedef struct
{
   uint32_t word2;
   uint32_t word1;
   uint32_t word0;
} Am335xAleEntry;


/**
 * @brief TX buffer descriptor
 **/

typedef struct _Am335xTxBufferDesc
{
   uint32_t word0;
   uint32_t word1;
   uint32_t word2;
   uint32_t word3;
   struct _Am335xTxBufferDesc *next;
   struct _Am335xTxBufferDesc *prev;
} Am335xTxBufferDesc;


/**
 * @brief RX buffer descriptor
 **/

typedef struct _Am335xRxBufferDesc
{
   uint32_t word0;
   uint32_t word1;
   uint32_t word2;
   uint32_t word3;
   struct _Am335xRxBufferDesc *next;
   struct _Am335xRxBufferDesc *prev;
} Am335xRxBufferDesc;


//AM335x Ethernet MAC driver
extern const NicDriver am335xEthPort1Driver;
extern const NicDriver am335xEthPort2Driver;

//AM335x Ethernet MAC related functions
error_t am335xEthInitPort1(NetInterface *interface);
error_t am335xEthInitPort2(NetInterface *interface);
void am335xEthInitInstance(NetInterface *interface);
void am335xEthInitGpio(NetInterface *interface);
void am335xEthInitBufferDesc(NetInterface *interface);

void am335xEthTick(NetInterface *interface);

void am335xEthEnableIrq(NetInterface *interface);
void am335xEthDisableIrq(NetInterface *interface);
void am335xEthTxIrqHandler(void);
void am335xEthRxIrqHandler(void);
void am335xEthEventHandler(NetInterface *interface);

error_t am335xEthSendPacketPort1(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t am335xEthSendPacketPort2(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t am335xEthUpdateMacAddrFilter(NetInterface *interface);
error_t am335xEthUpdateMacConfig(NetInterface *interface);

void am335xEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t am335xEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

void am335xEthWriteEntry(uint_t index, const Am335xAleEntry *entry);
void am335xEthReadEntry(uint_t index, Am335xAleEntry *entry);

uint_t am335xEthFindFreeEntry(void);
uint_t am335xEthFindVlanEntry(uint_t vlanId);
uint_t am335xEthFindVlanAddrEntry(uint_t vlanId, MacAddr *macAddr);

error_t am335xEthAddVlanEntry(uint_t port, uint_t vlanId);
error_t am335xEthAddVlanAddrEntry(uint_t port, uint_t vlanId, MacAddr *macAddr);
error_t am335xEthDeleteVlanAddrEntry(uint_t port, uint_t vlanId, MacAddr *macAddr);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
