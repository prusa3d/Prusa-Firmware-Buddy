/**
 * @file dp83825_driver.h
 * @brief DP83825 Ethernet PHY driver
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

#ifndef _DP83825_DRIVER_H
#define _DP83825_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef DP83825_PHY_ADDR
   #define DP83825_PHY_ADDR 0
#elif (DP83825_PHY_ADDR < 0 || DP83825_PHY_ADDR > 31)
   #error DP83825_PHY_ADDR parameter is not valid
#endif

//DP83825 PHY registers
#define DP83825_BMCR                             0x00
#define DP83825_BMSR                             0x01
#define DP83825_PHYIDR1                          0x02
#define DP83825_PHYIDR2                          0x03
#define DP83825_ANAR                             0x04
#define DP83825_ANLPAR                           0x05
#define DP83825_ANER                             0x06
#define DP83825_ANNPTR                           0x07
#define DP83825_ANLNPTR                          0x08
#define DP83825_CR1                              0x09
#define DP83825_CR2                              0x0A
#define DP83825_CR3                              0x0B
#define DP83825_REGCR                            0x0D
#define DP83825_ADDAR                            0x0E
#define DP83825_FLDS                             0x0F
#define DP83825_PHYSTS                           0x10
#define DP83825_PHYSCR                           0x11
#define DP83825_MISR1                            0x12
#define DP83825_MISR2                            0x13
#define DP83825_FCSCR                            0x14
#define DP83825_RECR                             0x15
#define DP83825_BISCR                            0x16
#define DP83825_RCSR                             0x17
#define DP83825_LEDCR                            0x18
#define DP83825_PHYCR                            0x19
#define DP83825_10BTSCR                          0x1A
#define DP83825_BICSR1                           0x1B
#define DP83825_BICSR2                           0x1C
#define DP83825_CDCR                             0x1E
#define DP83825_PHYRCR                           0x1F

//DP83825 MMD registers
#define DP83825_MMD3_PCS_CTRL_1                  0x03, 0x1000
#define DP83825_MMD3_PCS_STATUS_1                0x03, 0x1001
#define DP83825_MMD3_EEE_CAPABILITY              0x03, 0x1014
#define DP83825_MMD3_WAKE_ERR_CNT                0x03, 0x1016
#define DP83825_MMD7_EEE_ADVERTISEMENT           0x07, 0x203C
#define DP83825_MMD7_EEE_LP_ABILITY_Register     0x07, 0x203D
#define DP83825_MLEDCR                           0x1F, 0x0025
#define DP83825_COMPT                            0x1F, 0x0027
#define DP83825_CDSCR                            0x1F, 0x0170
#define DP83825_CDSCR2                           0x1F, 0x0171
#define DP83825_TDR_172                          0x1F, 0x0172
#define DP83825_CDSCR3                           0x1F, 0x0173
#define DP83825_TDR_174                          0x1F, 0x0174
#define DP83825_TDR_175                          0x1F, 0x0175
#define DP83825_TDR_176                          0x1F, 0x0176
#define DP83825_CDSCR4                           0x1F, 0x0177
#define DP83825_TDR_178                          0x1F, 0x0178
#define DP83825_CDLRR1                           0x1F, 0x0180
#define DP83825_CDLRR2                           0x1F, 0x0181
#define DP83825_CDLRR3                           0x1F, 0x0182
#define DP83825_CDLRR4                           0x1F, 0x0183
#define DP83825_CDLRR5                           0x1F, 0x0184
#define DP83825_CDLAR1                           0x1F, 0x0185
#define DP83825_CDLAR2                           0x1F, 0x0186
#define DP83825_CDLAR3                           0x1F, 0x0187
#define DP83825_CDLAR4                           0x1F, 0x0188
#define DP83825_CDLAR5                           0x1F, 0x0189
#define DP83825_CDLAR6                           0x1F, 0x018A
#define DP83825_IO_CFG                           0x1F, 0x0302
#define DP83825_SPARE_OUT                        0x1F, 0x0308
#define DP83825_DAC_CFG_0                        0x1F, 0x030B
#define DP83825_DAC_CFG_1                        0x1F, 0x030C
#define DP83825_DSP_CFG_0                        0x1F, 0x030F
#define DP83825_DSP_CFG_2                        0x1F, 0x0311
#define DP83825_DSP_CFG_4                        0x1F, 0x0313
#define DP83825_DSP_CFG_13                       0x1F, 0x031C
#define DP83825_DSP_CFG_16                       0x1F, 0x031F
#define DP83825_DSP_CFG_25                       0x1F, 0x033C
#define DP83825_DSP_CFG_27                       0x1F, 0x033E
#define DP83825_ANA_LD_PROG_SL                   0x1F, 0x0404
#define DP83825_ANA_RX10BT_CTRL                  0x1F, 0x040D
#define DP83825_GENCFG                           0x1F, 0x0456
#define DP83825_LEDCFG                           0x1F, 0x0460
#define DP83825_IOCTRL                           0x1F, 0x0461
#define DP83825_SOR1                             0x1F, 0x0467
#define DP83825_SOR2                             0x1F, 0x0468
#define DP83825_RXFCFG                           0x1F, 0x04A0
#define DP83825_RXFS                             0x1F, 0x04A1
#define DP83825_RXFPMD1                          0x1F, 0x04A2
#define DP83825_RXFPMD2                          0x1F, 0x04A3
#define DP83825_RXFPMD3                          0x1F, 0x04A4
#define DP83825_EEECFG2                          0x1F, 0x04D0
#define DP83825_EEECFG3                          0x1F, 0x04D1
#define DP83825_DSP_100M_STEP_2                  0x1F, 0x04D5
#define DP83825_DSP_100M_STEP_3                  0x1F, 0x04D6
#define DP83825_DSP_100M_STEP_4                  0x1F, 0x04D7

//BMCR register
#define DP83825_BMCR_RESET                       0x8000
#define DP83825_BMCR_LOOPBACK                    0x4000
#define DP83825_BMCR_SPEED_SEL                   0x2000
#define DP83825_BMCR_AN_EN                       0x1000
#define DP83825_BMCR_POWER_DOWN                  0x0800
#define DP83825_BMCR_ISOLATE                     0x0400
#define DP83825_BMCR_RESTART_AN                  0x0200
#define DP83825_BMCR_DUPLEX_MODE                 0x0100
#define DP83825_BMCR_COL_TEST                    0x0080

//BMSR register
#define DP83825_BMSR_100BT4                      0x8000
#define DP83825_BMSR_100BTX_FD                   0x4000
#define DP83825_BMSR_100BTX_HD                   0x2000
#define DP83825_BMSR_10BT_FD                     0x1000
#define DP83825_BMSR_10BT_HD                     0x0800
#define DP83825_BMSR_SMI_PREAMBLE_SUPPR          0x0040
#define DP83825_BMSR_AN_COMPLETE                 0x0020
#define DP83825_BMSR_REMOTE_FAULT                0x0010
#define DP83825_BMSR_AN_CAPABLE                  0x0008
#define DP83825_BMSR_LINK_STATUS                 0x0004
#define DP83825_BMSR_JABBER_DETECT               0x0002
#define DP83825_BMSR_EXTENDED_CAPABLE            0x0001

//PHYIDR1 register
#define DP83825_PHYIDR1_OUI_MSB                  0xFFFF
#define DP83825_PHYIDR1_OUI_MSB_DEFAULT          0x2000

//PHYIDR2 register
#define DP83825_PHYIDR2_OUI_LSB                  0xFC00
#define DP83825_PHYIDR2_OUI_LSB_DEFAULT          0xA000
#define DP83825_PHYIDR2_MODEL_NUMBER             0x03F0
#define DP83825_PHYIDR2_MODEL_NUMBER_DEFAULT     0x0240
#define DP83825_PHYIDR2_REV_NUMBER               0x000F

//ANAR register
#define DP83825_ANAR_NEXT_PAGE                   0x8000
#define DP83825_ANAR_REMOTE_FAULT                0x2000
#define DP83825_ANAR_ASYM_DIR                    0x0800
#define DP83825_ANAR_PAUSE                       0x0400
#define DP83825_ANAR_100BT4                      0x0200
#define DP83825_ANAR_100BTX_FD                   0x0100
#define DP83825_ANAR_100BTX_HD                   0x0080
#define DP83825_ANAR_10BT_FD                     0x0040
#define DP83825_ANAR_10BT_HD                     0x0020
#define DP83825_ANAR_SELECTOR                    0x001F
#define DP83825_ANAR_SELECTOR_DEFAULT            0x0001

//ANLPAR register
#define DP83825_ANLPAR_NEXT_PAGE                 0x8000
#define DP83825_ANLPAR_ACK                       0x4000
#define DP83825_ANLPAR_REMOTE_FAULT              0x2000
#define DP83825_ANLPAR_ASYM_DIR                  0x0800
#define DP83825_ANLPAR_PAUSE                     0x0400
#define DP83825_ANLPAR_100BT4                    0x0200
#define DP83825_ANLPAR_100BTX_FD                 0x0100
#define DP83825_ANLPAR_100BTX_HD                 0x0080
#define DP83825_ANLPAR_10BT_FD                   0x0040
#define DP83825_ANLPAR_10BT_HD                   0x0020
#define DP83825_ANLPAR_SELECTOR                  0x001F
#define DP83825_ANLPAR_SELECTOR_DEFAULT          0x0001

//ANER register
#define DP83825_ANER_PAR_DETECT_FAULT            0x0010
#define DP83825_ANER_LP_NEXT_PAGE_ABLE           0x0008
#define DP83825_ANER_NEXT_PAGE_ABLE              0x0004
#define DP83825_ANER_PAGE_RECEIVED               0x0002
#define DP83825_ANER_LP_AN_ABLE                  0x0001

//ANNPTR register
#define DP83825_ANNPTR_NEXT_PAGE                 0x8000
#define DP83825_ANNPTR_MSG_PAGE                  0x2000
#define DP83825_ANNPTR_ACK2                      0x1000
#define DP83825_ANNPTR_TOGGLE                    0x0800
#define DP83825_ANNPTR_CODE                      0x07FF

//ANLNPTR register
#define DP83825_ANLNPTR_NEXT_PAGE                0x8000
#define DP83825_ANLNPTR_ACK                      0x4000
#define DP83825_ANLNPTR_MSG_PAGE                 0x2000
#define DP83825_ANLNPTR_ACK2                     0x1000
#define DP83825_ANLNPTR_TOGGLE                   0x0800
#define DP83825_ANLNPTR_MESSAGE                  0x07FF

//CR1 register
#define DP83825_CR1_RMII_ENHANCED_MODE           0x0200
#define DP83825_CR1_TDR_AUTO_RUN                 0x0100
#define DP83825_CR1_ROBUST_AUTO_MDIX             0x0020
#define DP83825_CR1_FAST_RX_DV_DETECT            0x0002

//CR2 register
#define DP83825_CR2_EXTENDED_FD_ABLE             0x0020
#define DP83825_CR2_RX_ER_DURING_IDLE            0x0004
#define DP83825_CR2_ODD_NIBBLE_DETECT_DIS        0x0002

//CR3 register
#define DP83825_CR3_DESCRAMBLER_FAST_LINK_DOWN   0x0400
#define DP83825_CR3_POLARITY_SWAP                0x0040
#define DP83825_CR3_MDIX_SWAP                    0x0020
#define DP83825_CR3_FAST_LINK_DOWN_MODE          0x000F

//REGCR register
#define DP83825_REGCR_CMD                        0xC000
#define DP83825_REGCR_CMD_ADDR                   0x0000
#define DP83825_REGCR_CMD_DATA_NO_POST_INC       0x4000
#define DP83825_REGCR_CMD_DATA_POST_INC_RW       0x8000
#define DP83825_REGCR_CMD_DATA_POST_INC_W        0xC000
#define DP83825_REGCR_DEVAD                      0x001F

//FLDS register
#define DP83825_FLDS_FAST_LINK_DOWN_STATUS       0x01F0

//PHYSTS register
#define DP83825_PHYSTS_MDIX_MODE                 0x4000
#define DP83825_PHYSTS_RECEIVE_ERROR_LATCH       0x2000
#define DP83825_PHYSTS_POLARITY_STATUS           0x1000
#define DP83825_PHYSTS_FALSE_CARRIER_SENSE_LATCH 0x0800
#define DP83825_PHYSTS_SIGNAL_DETECT             0x0400
#define DP83825_PHYSTS_DESCRAMBLER_LOCK          0x0200
#define DP83825_PHYSTS_PAGE_RECEIVED             0x0100
#define DP83825_PHYSTS_MII_INTERRUPT             0x0080
#define DP83825_PHYSTS_REMOTE_FAULT              0x0040
#define DP83825_PHYSTS_JABBER_DETECT             0x0020
#define DP83825_PHYSTS_AN_COMPLETE               0x0010
#define DP83825_PHYSTS_LOOPBACK_STATUS           0x0008
#define DP83825_PHYSTS_DUPLEX_STATUS             0x0004
#define DP83825_PHYSTS_SPEED_STATUS              0x0002
#define DP83825_PHYSTS_LINK_STATUS               0x0001

//PHYSCR register
#define DP83825_PHYSCR_PLL_DIS                   0x8000
#define DP83825_PHYSCR_POWER_SAVE_MODE_EN        0x4000
#define DP83825_PHYSCR_POWER_SAVE_MODE           0x3000
#define DP83825_PHYSCR_SCRAMBLER_BYPASS          0x0800
#define DP83825_PHYSCR_LOOPBACK_FIFO_DEPTH       0x0300
#define DP83825_PHYSCR_INT_POLARITY              0x0008
#define DP83825_PHYSCR_TEST_INT                  0x0004
#define DP83825_PHYSCR_INT_EN                    0x0002
#define DP83825_PHYSCR_INT_OE                    0x0001

//MISR1 register
#define DP83825_MISR1_LQ_INT                     0x8000
#define DP83825_MISR1_ED_INT                     0x4000
#define DP83825_MISR1_LINK_INT                   0x2000
#define DP83825_MISR1_SPD_INT                    0x1000
#define DP83825_MISR1_DUP_INT                    0x0800
#define DP83825_MISR1_ANC_INT                    0x0400
#define DP83825_MISR1_FHF_INT                    0x0200
#define DP83825_MISR1_RHF_INT                    0x0100
#define DP83825_MISR1_LQ_INT_EN                  0x0080
#define DP83825_MISR1_ED_INT_EN                  0x0040
#define DP83825_MISR1_LINK_INT_EN                0x0020
#define DP83825_MISR1_SPD_INT_EN                 0x0010
#define DP83825_MISR1_DUP_INT_EN                 0x0008
#define DP83825_MISR1_ANC_INT_EN                 0x0004
#define DP83825_MISR1_FHF_INT_EN                 0x0002
#define DP83825_MISR1_RHF_INT_EN                 0x0001

//MISR2 register
#define DP83825_MISR2_EEE_ERROR_INT              0x8000
#define DP83825_MISR2_AN_ERROR_INT               0x4000
#define DP83825_MISR2_PR_INT                     0x2000
#define DP83825_MISR2_FIFO_OF_UF_INT             0x1000
#define DP83825_MISR2_MDI_CHANGE_INT             0x0800
#define DP83825_MISR2_SLEEP_MODE_INT             0x0400
#define DP83825_MISR2_POL_CHANGE_INT             0x0200
#define DP83825_MISR2_JABBER_DETECT_INT          0x0100
#define DP83825_MISR2_EEE_ERROR_INT_EN           0x0080
#define DP83825_MISR2_AN_ERROR_INT_EN            0x0040
#define DP83825_MISR2_PR_INT_EN                  0x0020
#define DP83825_MISR2_FIFO_OF_UF_INT_EN          0x0010
#define DP83825_MISR2_MDI_CHANGE_INT_EN          0x0008
#define DP83825_MISR2_SLEEP_MODE_INT_EN          0x0004
#define DP83825_MISR2_POL_CHANGE_INT_EN          0x0002
#define DP83825_MISR2_JABBER_DETECT_INT_EN       0x0001

//FCSCR register
#define DP83825_FCSCR_FCSCNT                     0x00FF

//RECR register
#define DP83825_RECR_RXERCNT                     0xFFFF

//BISCR register
#define DP83825_BISCR_ERROR_COUNTER_MODE         0x4000
#define DP83825_BISCR_PRBS_CHECKER               0x2000
#define DP83825_BISCR_PACKET_GEN_EN              0x1000
#define DP83825_BISCR_PRBS_CHECKER_LOCK_SYNC     0x0800
#define DP83825_BISCR_PRBS_CHECKER_SYNC_LOSS     0x0400
#define DP83825_BISCR_PACKET_GEN_STATUS          0x0200
#define DP83825_BISCR_POWER_MODE                 0x0100
#define DP83825_BISCR_TX_MII_LOOPBACK            0x0040
#define DP83825_BISCR_LOOPBACK_MODE              0x001F

//RCSR register
#define DP83825_RCSR_RMII_TX_CLOCK_SHIFT         0x0100
#define DP83825_RCSR_RMII_CLK_SEL                0x0080
#define DP83825_RCSR_RMII_REV_SEL                0x0010
#define DP83825_RCSR_RMII_OVF_STATUS             0x0008
#define DP83825_RCSR_RMII_UNF_STATUS             0x0004
#define DP83825_RCSR_RX_ELAST_BUFFER_SIZE        0x0003

//LEDCR register
#define DP83825_LEDCR_BLINK_RATE                 0x0600
#define DP83825_LEDCR_BLINK_RATE_20MHZ           0x0000
#define DP83825_LEDCR_BLINK_RATE_10MHZ           0x0200
#define DP83825_LEDCR_BLINK_RATE_5MHZ            0x0400
#define DP83825_LEDCR_BLINK_RATE_2MHZ            0x0600
#define DP83825_LEDCR_LED_LINK_POLARITY          0x0080
#define DP83825_LEDCR_DRIVE_LINK_LED             0x0010
#define DP83825_LEDCR_LINK_LED_ON_OFF            0x0002

//PHYCR register
#define DP83825_PHYCR_MDIX_EN                    0x8000
#define DP83825_PHYCR_FORCE_MDIX                 0x4000
#define DP83825_PHYCR_PAUSE_RX_STATUS            0x2000
#define DP83825_PHYCR_PAUSE_TX_STATUS            0x1000
#define DP83825_PHYCR_MII_LINK_STATUS            0x0800
#define DP83825_PHYCR_BYPASS_LED_STRETCH         0x0080
#define DP83825_PHYCR_LED_CONFIG                 0x0020
#define DP83825_PHYCR_PHY_ADDR                   0x001F

//10BTSCR register
#define DP83825_10BTSCR_RX_THRESHOLD_EN          0x2000
#define DP83825_10BTSCR_SQUELCH                  0x1E00
#define DP83825_10BTSCR_NLP_DIS                  0x0080
#define DP83825_10BTSCR_POLARITY_STATUS          0x0010
#define DP83825_10BTSCR_JABBER_DIS               0x0001

//BICSR1 register
#define DP83825_BICSR1_BIST_ERROR_COUNT          0xFF00
#define DP83825_BICSR1_BIST_IPG_LENGTH           0x00FF

//BICSR2 register
#define DP83825_BICSR2_BIST_PACKET_LENGTH        0x07FF

//CDCR register
#define DP83825_CDCR_CABLE_DIAG_START            0x8000
#define DP83825_CDCR_CFG_RESCAL_EN               0x4000
#define DP83825_CDCR_CDCR_CABLE_DIAG_STATUS      0x0002
#define DP83825_CDCR_CDCR_CABLE_DIAG_TEST_FAIL   0x0001

//PHYRCR register
#define DP83825_PHYRCR_SOFT_HARD_RESET           0x8000
#define DP83825_PHYRCR_DIGITAL_RESET             0x4000

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//DP83825 Ethernet PHY driver
extern const PhyDriver dp83825PhyDriver;

//DP83825 related functions
error_t dp83825Init(NetInterface *interface);

void dp83825Tick(NetInterface *interface);

void dp83825EnableIrq(NetInterface *interface);
void dp83825DisableIrq(NetInterface *interface);

void dp83825EventHandler(NetInterface *interface);

void dp83825WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t dp83825ReadPhyReg(NetInterface *interface, uint8_t address);

void dp83825DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
