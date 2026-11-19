/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __PS3_HTP_MGR_EVT_H__
#define __PS3_HTP_MGR_EVT_H__

#include "ps3_htp_mgr_evt_raidhba.h"

struct PS3EventFilter {
	unsigned char eventType;
	unsigned char eventCodeCnt;
	unsigned char reserved[6];
	unsigned short eventCodeTable[];
};

#endif
