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

#if CFG_SUPPORT_NAN

enum ENUM_MODULE {
	ENUM_NAN_DATA_MODULE,
	ENUM_NAN_RANGE_MODULE,
	ENUM_NAN_MODULE_NUM
};

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

uint8_t nanDevInit(IN struct ADAPTER *prAdapter, uint8_t ucIdx);

void nanDevFsmUninit(IN struct ADAPTER *prAdapter, uint8_t ucIdx);
struct _NAN_SPECIFIC_BSS_INFO_T *
nanGetSpecificBssInfo(IN struct ADAPTER *prAdapter,
		      enum NAN_BSS_ROLE_INDEX eIndex);
uint8_t
nanGetBssIdxbyBand(IN struct ADAPTER *prAdapter,
		      enum ENUM_BAND eBand);

void nanDevSetMasterPreference(IN struct ADAPTER *prAdapter,
			       uint8_t ucMasterPreference);

enum NanStatusType nanDevEnableRequest(IN struct ADAPTER *prAdapter,
				       struct NanEnableRequest *prEnableReq);
enum NanStatusType nanDevDisableRequest(IN struct ADAPTER *prAdapter);
void nanDevMasterIndEvtHandler(IN struct ADAPTER *prAdapter,
			       IN uint8_t *pcuEvtBuf);
uint32_t nanDevGetMasterIndAttr(IN struct ADAPTER *prAdapter,
				uint8_t *pucMasterIndAttrBuf,
				uint32_t *pu4MasterIndAttrLength);
void nanDevClusterIdEvtHandler(IN struct ADAPTER *prAdapter,
		IN uint8_t *pcuEvtBuf);
uint32_t nanDevGetClusterId(IN struct ADAPTER *prAdapter,
		uint8_t *pucClusterId);
uint32_t nanDevSendEnableRequestToCnm(IN struct ADAPTER *prAdapter);
uint32_t nanDevSendAbortRequestToCnm(IN struct ADAPTER *prAdapter);
void nanDevSendEnableRequest(struct ADAPTER *prAdapter,
				struct MSG_HDR *prMsgHdr);
/*========================= FUNCTIONs ============================*/
#endif
