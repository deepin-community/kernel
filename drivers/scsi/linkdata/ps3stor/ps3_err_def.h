/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _PS3_ERR_DEF_H_
#define _PS3_ERR_DEF_H_

#include "ps3_htp_def.h"


enum {
	PS3_SUCCESS = 0x00,
	PS3_FAILED = 0x01,
	PS3_TIMEOUT = 0x02,
	PS3_IN_IRQ_POLLING = 0x03,
	PS3_DEV_UNKNOWN = 0x04,
	PS3_RETRY = 0x05,
	PS3_EBUSY = 0x06,
	PS3_EINVAL = 0x07,
	PS3_ENOMEM = 0x08,
	PS3_ENODEV = 0x09,
	PS3_ERESTARTSYS = 0x0a,
	PS3_ENOTTY = 0x0b,
	PS3_RESP_ERR = 0x0c,
	PS3_RESP_INT = 0x0d,
	PS3_ACTIVE_ERR = 0x0e,
	PS3_CMD_NO_RESP = 0x0f,
	PS3_IO_BLOCK = 0x10,
	PS3_IO_REQUEUE = 0x11,
	PS3_IO_CONFLICT_IN_Q = 0x12,
	PS3_IO_CONFLICT = 0x13,
	PS3_MGR_REC_FORCE = 0x14,
	PS3_RECOVERED = 0x15,
	PS3_IN_UNLOAD = 0x16,
	PS3_NO_RECOVERED = 0x17,
	PS3_IN_QOS_Q = 0x18,
	PS3_IN_PCIE_ERR = 0x19,
};

#define PS3_DRV_TRUE (1)
#define PS3_DRV_FALSE (0)

struct ps3_fault_context {
	unsigned char ioc_busy;
	unsigned char reserved[2];
	unsigned long last_time;
};

#endif
