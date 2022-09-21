/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

/*! \file  mt7902.h
*    \brief This file contains the info of MT7902
*/

#ifdef MT7902

#ifndef _MT7902_H
#define _MT7902_H

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
/*TODO: To use correct ID after FPGA uses correct ID @20170927*/

#define CONNAC2X_TOP_HCR 0x70010200
#define CONNAC2X_TOP_HVR 0x70010204
#define CONNAC2X_TOP_FVR 0x88000004
#define CONNAC2x_CONN_CFG_ON_BASE	0x7C060000
#define CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_ADDR \
	(CONNAC2x_CONN_CFG_ON_BASE + 0xF0)
#define CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT         0

#define MT7902_UDMA_CONN_INFRA_STATUS_SEL   0x74000A4C
#define MT7902_UDMA_CONN_INFRA_STATUS_VAL   0xD8000000
#define MT7902_UDMA_CONN_INFRA_STATUS       0x74000A48

#define MT7902_CHIP_ID                 (0x7902)
#define MT7902_SW_SYNC0                CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_ADDR
#define MT7902_SW_SYNC0_RDY_OFFSET \
	CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT
#define MT7902_PATCH_START_ADDR        (0x00900000)
#define MT7902_TOP_CFG_BASE			CONN_CFG_BASE
#define MT7902_TX_DESC_APPEND_LENGTH   32
#define MT7902_RX_DESC_LENGTH   24
#define MT7902_ARB_AC_MODE_ADDR (0x820E315C)

#define MT7902_A_DIE_VER_ADDR 0x70010020
#define MT7902_A_DIE_VER_BIT  BIT(7)

#define MT7902_BT_FW_VER_ADDR 0x7C812004
#define MT7902_BT_FW_VER_MASK 0xFF

/*------------------------------------------------------------------------------
 * MACRO for WTBL INFO GET
 *------------------------------------------------------------------------------
 */

#define MT7902_WIFI_LWTBL_BASE 0x820d4200
#define MT7902_WIFI_UWTBL_BASE 0x820c4094


/*------------------------------------------------------------------------------
 * MACRO for WFDMA INFO GET
 *------------------------------------------------------------------------------
 */
#define MT7902_WFDMA_COUNT 1

/*------------------------------------------------------------------------------
 * MACRO for debug MCU
 *------------------------------------------------------------------------------
 */
#define CURRENT_PC 0x3F
#define PC_LOG_IDX 0x20
#define PC_LOG_NUM 32


/*******************************************************************************
*                         D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void mt7902_show_ple_info(
	struct ADAPTER *prAdapter,
	u_int8_t fgDumpTxd);

void mt7902_show_wfdma_info(
	IN struct ADAPTER *prAdapter);

#if defined(_HIF_SDIO)
u_int8_t mt7902_sdio_show_mcu_debug_info(
	struct ADAPTER *prAdapter,
	IN uint8_t *pucBuf, IN uint32_t u4Max,
	IN uint8_t ucFlag, OUT uint32_t *pu4Length);
#endif

#if (CFG_SUPPORT_DEBUG_SOP == 1)

#if defined(_HIF_PCIE)
void pcie_mt7902_dump_power_debug_cr(struct ADAPTER *prAdapter);
void pcie_mt7902_dump_wfsys_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
void pcie_mt7902_dump_bgfsys_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
void pcie_mt7902_dump_conninfra_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
#endif /* #if defined(_HIF_PCIE) */

#if defined(_HIF_USB)
void usb_mt7902_dump_power_debug_cr(struct ADAPTER *prAdapter);
void usb_mt7902_dump_wfsys_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
void usb_mt7902_dump_bgfsys_debug_cr(struct ADAPTER *prAdapter);
void usb_mt7902_dump_conninfra_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
#endif /* #if defined(_HIF_USB) */

#if defined(_HIF_SDIO)
void sdio_mt7902_dump_power_debug_cr(struct ADAPTER *prAdapter);
void sdio_mt7902_dump_wfsys_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
void sdio_mt7902_dump_conninfra_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
#endif /* #if defined(_HIF_SDIO) */

u_int8_t mt7902_show_debug_sop_info(struct ADAPTER *prAdapter,
	uint8_t ucCase);
#endif


/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _MT7902_H */

#endif  /* MT7902 */
