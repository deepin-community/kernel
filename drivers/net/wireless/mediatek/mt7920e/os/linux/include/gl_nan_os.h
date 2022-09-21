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
/*
 * Id:
 * //Department/DaVinci/TRUNK/MT6620_5931_WiFi_Driver/os/linux/
 * include/gl_p2p_os.h#28
 */

/*! \file   gl_nan_os.h
 *    \brief  List the external reference to OS for nan GLUE Layer.
 *
 *    In this file we define the data structure - GLUE_INFO_T to store
 *    those objects
 *    we acquired from OS - e.g. TIMER, SPINLOCK, NET DEVICE ... . And all the
 *    external reference (header file, extern func() ..) to OS for GLUE Layer
 *    should also list down here.
 */

#ifndef _GL_NAN_OS_H
#define _GL_NAN_OS_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */
#if CFG_SUPPORT_NAN

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   V A R I A B L E
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

struct _GL_NAN_INFO_T {
	struct net_device *prDevHandler;
	unsigned char fgBMCFilterSet;
	/* struct net_device *prRoleDevHandler; */
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
int mtk_nan_wext_get_priv(IN struct net_device *prDev,
			  IN struct iw_request_info *info,
			  IN OUT union iwreq_data *wrqu, IN OUT char *extra);

unsigned char nanLaunch(struct GLUE_INFO *prGlueInfo);

unsigned char nanRemove(struct GLUE_INFO *prGlueInfo);
void nanSetSuspendMode(struct GLUE_INFO *prGlueInfo, unsigned char fgEnable);

unsigned char glRegisterNAN(struct GLUE_INFO *prGlueInfo,
	const char *prDevName);

int glSetupNAN(struct GLUE_INFO *prGlueInfo, struct wireless_dev *prNanWdev,
	       struct net_device *prNanDev, int u4Idx);

unsigned char glUnregisterNAN(struct GLUE_INFO *prGlueInfo);

unsigned char nanNetRegister(struct GLUE_INFO *prGlueInfo,
		       unsigned char fgIsRtnlLockAcquired);

unsigned char nanNetUnregister(struct GLUE_INFO *prGlueInfo,
			 unsigned char fgIsRtnlLockAcquired);

unsigned char nanAllocInfo(IN struct GLUE_INFO *prGlueInfo, uint8_t ucRoleIdx);
unsigned char nanFreeInfo(struct GLUE_INFO *prGlueInfo, uint8_t ucRoleIdx);

/* VOID p2pSetSuspendMode(P_GLUE_INFO_T prGlueInfo, BOOLEAN fgEnable); */
unsigned char glNanCreateWirelessDevice(struct GLUE_INFO *prGlueInfo);
void nanSetMulticastListWorkQueueWrapper(struct GLUE_INFO *prGlueInfo);

/* VOID p2pUpdateChannelTableByDomain(P_GLUE_INFO_T prGlueInfo); */
#endif
#endif
