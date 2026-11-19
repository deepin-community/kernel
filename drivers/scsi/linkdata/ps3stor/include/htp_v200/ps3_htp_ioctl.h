/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_HTP_IOCTL_H_
#define _PS3_HTP_IOCTL_H_

#include "ps3_htp.h"
#include "ps3_htp_reqframe.h"

#define PS3_MAX_IOCTL_SGE_NUM 16
#define PS3_IOCTL_SENSE_SIZE 96
#define PS3_IOCTL_MAX_FRAME_SIZE 128

#ifndef PS3_SCSI_HOST_PROC_NAME
#define PS3_SCSI_HOST_PROC_NAME "ps3stor"
#endif

#define PS3_PRODUCT_MODEL "ps3stor"

#ifndef PS3_SCSI_HOST_PROC_NAME_V100
#define PS3_SCSI_HOST_PROC_NAME_V100 "ps3"
#endif
#define PS3_PRODUCT_MODEL_V100 "ps3"

#define PS3_PSW_PRODUCT_MODEL "psw"


struct PS3CmdIoctlHeader {
	unsigned char cmdType;
	unsigned char version;
	unsigned short deviceId;
	unsigned short cmdSubType;
	unsigned char cmdResult;
	unsigned char sglOffset;
	unsigned short index;
	unsigned short control;
	unsigned int sgeCount;
	unsigned short timeout;
	unsigned char sglChainOffset;
	unsigned char syncFlag;
	unsigned int abortCmdFrameId;
};
union PS3IoctlFrame {
	unsigned char value[PS3_IOCTL_MAX_FRAME_SIZE];
	struct PS3CmdIoctlHeader header;
};
#ifdef _WINDOWS

#define PS3_IOCTL_SIG "ps3stor"
#define PS3_IOCTL_FUNCTION 0x801
#define PS3_DEBUG_CLI_FUNCTION 0x802

#define PS3_CTL_CODE                                                           \
	CTL_CODE(FILE_DEVICE_MASS_STORAGE, PS3_IOCTL_FUNCTION,                 \
		 METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define PS3_DBG_CLI_CODE                                                       \
	CTL_CODE(FILE_DEVICE_MASS_STORAGE, PS3_DEBUG_CLI_FUNCTION,             \
		 METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

struct PS3IoctlSyncCmd {
	unsigned short hostId;
	unsigned short sglOffset;
	unsigned short sgeCount;
	unsigned short reserved;
	unsigned long long traceId;
	struct PS3Sge Sgl[PS3_MAX_IOCTL_SGE_NUM];
	unsigned char data[];
};

struct _PS3_IO_CONTROL {
	SRB_IO_CONTROL SrbHeader;
	unsigned long reserved;
	struct PS3IoctlSyncCmd ps3Ioctl;
};
#else

#define PS3_CMD_IOCTL_SYNC_CMD _IOWR('M', 1, struct PS3IoctlSyncCmd)

#ifndef __WIN32__
#define PS3_EVENT_NOTICE_SIG (SIGRTMIN + 7)
enum {
	PS3_IOCTL_CMD_NORMAL = 0,
	PS3_IOCTL_CMD_WEB_SUBSCRIBE,
};

struct PS3IoctlSyncCmd {
	unsigned short hostId;
	unsigned short sglOffset;
	unsigned short sgeCount;
	unsigned short reserved1;
	unsigned int resultCode;
	unsigned char reserved2[4];
	unsigned char sense[PS3_IOCTL_SENSE_SIZE];
	unsigned long long traceId;
	unsigned char reserved3[120];
	union PS3IoctlFrame msg;
	struct PS3Sge sgl[PS3_MAX_IOCTL_SGE_NUM];
};
#endif

struct PS3IoctlAsynCmd {
	unsigned short hostId;
	unsigned short reserved1;

	unsigned int seqNum;

	unsigned short eventLevel;

	unsigned short eventType;
	unsigned char reserved2[4];
};

#endif
#endif
