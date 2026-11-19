/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_HTP_EVENT_H_
#define _PS3_HTP_EVENT_H_

#include "ps3_htp_def.h"
#include "ps3_htp_dev.h"
#include "ps3_htp_mgr_evt.h"
#include "ps3_evtcode_trans.h"
#define PS3_EVENT_DETAIL_BUF_MAX (20)


enum PS3EventLevel {
	PS3_EVENT_LEVEL_INFO,
	PS3_EVENT_LEVEL_WARN,
	PS3_EVENT_LEVEL_CRITICAL,
};


struct PS3EventDetail {
	unsigned int eventCode;
	unsigned int timestamp;
	enum MgrEvtType eventType;
	union {
		struct PS3DiskDevPos devicePos;
		unsigned char EnclId;
	};
};

struct PS3EventInfo {
	unsigned int eventTypeMap;
	unsigned int eventCount;


	struct PS3EventDetail eventDetail[PS3_EVENT_DETAIL_BUF_MAX];
	unsigned char reserved[8];
};
#endif
