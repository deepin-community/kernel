/*
 * Copyright (c) [YEAR], MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws.
 * The information contained herein is confidential and proprietary to
 * MediaTek Inc. and/or its licensors.
 * Except as otherwise provided in the applicable licensing terms with
 * MediaTek Inc. and/or its licensors, any reproduction, modification, use or
 * disclosure of MediaTek Software, and information contained herein, in whole
 * or in part, shall be strictly prohibited.
 */

#include "precomp.h"
#include "nan_txm.h"

#if CFG_SUPPORT_NAN

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define TXM_UT_CONTENT_LEN 20

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
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/

uint32_t
nanTxUtTxDone(IN struct ADAPTER *prAdapter, IN struct MSDU_INFO *prMsduInfo,
	      IN enum ENUM_TX_RESULT_CODE rTxDoneStatus) {

	if (!prMsduInfo) {
		DBGLOG(NAN, ERROR, "prMsduInfo error!\n");
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(TX, INFO, "EVENT-TX DONE: Status:%d\n", rTxDoneStatus);

	return WLAN_STATUS_SUCCESS;
}

#endif
