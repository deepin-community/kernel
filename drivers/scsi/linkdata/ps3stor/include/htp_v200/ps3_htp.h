/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_HTP_H_
#define _PS3_HTP_H_

#include "ps3_htp_def.h"
#include "ps3_htp_dev.h"
#include "ps3_htp_req_frame_hw.h"

#define PS3_DUMP_CTRL_COPY_FINISH 0x1
#define PS3_DUMP_CTRL_DUMP_ABORT 0x2
#define PS3_DUMP_CTRL_DUMP_FW_LOG 0x3
#define PS3_DUMP_CTRL_DUMP_BAR_DATA 0x4
#define PS3_DUMP_CTRL_DUMP_CORE_FILE 0x5
#define PS3_DUMP_CTRL_DUMP_END 0x6
#define PS3_DUMP_CTRL_DUMP_INT_READY 0x7
#define PS3_DUMP_DMA_DONE (0x1)
#define PS3_DUMP_DMA_ABORT (0x1 << 6)
#define PS3_DUMP_DATA_UNIT_SIZE (0x400)
#define PS3_DREICT_SENSE_DATA_BUF_SIZE 72

#define PS3_ATU_FLAG_LOW_BITS_MASK (0x0000FFFF)
#define PS3_ATU_FLAG_HIGH_BITS_MASK (0xFFFFFFFFFFFF0000)
#define PS3_ATU_FLAG_DRIVER_SET (0xC0DE)

#define PS3_IOCTL_VERSION (0x2000000)

enum {
	HIL_MODEL_SW = 0,
	HIL_MODEL_HW,
	HIL_MODEL_HW_ENHANCED,
	HIL_MODEL_SW_ASSIST,
};

enum Ps3DumpType {
	PS3_DUMP_TYPE_UNKNOWN = 0,
	PS3_DUMP_TYPE_CRASH = 1,
	PS3_DUMP_TYPE_FW_LOG = 2,
	PS3_DUMP_TYPE_BAR_DATA = 3,
};

enum Ps3DumpState {
	PS3_DUMP_STATE_INVALID = 0,
	PS3_DUMP_STATE_PRE_ABORT,
	PS3_DUMP_STATE_ABORTED,
	PS3_DUMP_STATE_START,
	PS3_DUMP_STATE_COPYING,
	PS3_DUMP_STATE_COPY_DONE,
	PS3_DUMP_STATE_READY = 7,
};
enum Ps3CtrlSecurityState {
	PS3_CTRL_SECURITY_STATE_DECRYPT = 0,
	PS3_CTRL_SECURITY_STATE_ENCRYPT,
};

struct Ps3DumpNotifyInfo {
	int dumpType;
};


struct PS3LinkErrInfo {
	unsigned int invalidDwordCount;
	unsigned int runningDisparityErrCount;
	unsigned int lossOfDwordSyncCount;
	unsigned int phyResetProblemCount;
};

enum PhyCtrl {
	PS3_SAS_CTRL_UNKNOWN = 0,
	PS3_SAS_CTRL_RESET = 1,
	PS3_SAS_CTRL_RESET_HARD = 2,
	PS3_SAS_CTRL_DISABLE = 3
};


enum {
	PS3_UNLOAD_SUB_TYPE_RESERVED = 0,
	PS3_UNLOAD_SUB_TYPE_REMOVE = 1,
	PS3_UNLOAD_SUB_TYPE_SHUTDOWN = 2,
	PS3_UNLOAD_SUB_TYPE_SUSPEND = 3,
};


enum {
	PS3_SUSPEND_TYPE_NONE = 0,
	PS3_SUSPEND_TYPE_SLEEP = 1,
	PS3_SUSPEND_TYPE_HIBERNATE = 2,
};

static inline const char *namePhyCtrl(enum PhyCtrl e)
{
	static const char * const myNames[] = {
		[PS3_SAS_CTRL_UNKNOWN] = "PS3_SAS_CTRL_UNKNOWN",
		[PS3_SAS_CTRL_RESET] = "PS3_SAS_CTRL_RESET",
		[PS3_SAS_CTRL_RESET_HARD] = "PS3_SAS_CTRL_RESET_HARD",
		[PS3_SAS_CTRL_DISABLE] = "PS3_SAS_CTRL_DISABLE"
	};

	return myNames[e];
}


struct PS3InitCmdWord {
	union {
		struct {
			unsigned int type : 2;
			unsigned int reserved1 : 1;
			unsigned int direct : 2;
			unsigned int reserved2 : 27;
		};
		unsigned int lowAddr;
	};

	unsigned int highAddr;
};

struct PS3CmdWord {
	unsigned short type : 2;
	unsigned short reserved1 : 2;
	unsigned short direct : 2;
	unsigned short isrSN : 8;
	unsigned short reserved2 : 2;
	unsigned short cmdFrameID : 13;
	unsigned short reserved3 : 3;
	unsigned short phyDiskID : 12;
	unsigned short reserved4 : 4;
	unsigned short virtDiskID : 8;
	unsigned short reserved5 : 4;
	unsigned short qMask : 4;
};


struct PS3CmdWordSw {
	unsigned int type : 2;
	unsigned int noReplyWord : 1;
	unsigned int cmdFrameID : 13;
	unsigned int isrSN : 8;
	unsigned int cmdIndex : 8;
};


union PS3CmdWordU32 {
	struct PS3CmdWordSw cmdWord;
	unsigned int val;
};


union PS3DefaultCmdWord {
	struct PS3CmdWord cmdWord;
	union {
		struct {
			unsigned int low;
			unsigned int high;
		} u;
		unsigned long long words;
	};
};

enum {
	PS3_ISR_ACC_MODE_LATENCY = 0,
	PS3_ISR_ACC_MODE_SSD_IOPS,
	PS3_ISR_ACC_MODE_HDD_IOPS,
	PS3_ISR_ACC_MODE_IOPS_VER0 = 2,
	PS3_ISR_ACC_MODE_DEV_IOPS,
	PS3_ISR_ACC_MODE_MAX,
};
struct PS3ReplyFifoDesc {
	unsigned long long ReplyFifoBaseAddr;
	unsigned int irqNo;
	unsigned short depthReplyFifo;
	unsigned char isrAccMode;
	unsigned char reserved;
};

struct PS3ReplyWord {
	unsigned short type : 2;
	unsigned short diskType : 1;
	unsigned short reserved1 : 1;
	unsigned short mode : 2;
	unsigned short reserved2 : 10;
	unsigned short cmdFrameID : 13;
	unsigned short reserved3 : 2;
	unsigned short reserved4 : 1;
	unsigned short retStatus : 15;
	unsigned short retType : 1;
	unsigned short reserved5 : 12;
	unsigned short qMask : 4;
};


struct PS3MgrTaskRespInfo {
	unsigned char iocStatus;
	unsigned char reserved1;
	unsigned short iocLogInfo;
	unsigned int terminationCnt;
	unsigned int respInfo;
	unsigned int reserved2;
};


struct PS3MgrCmdReplyRespInfo {
	unsigned char cmdReplyStatus;
	unsigned char reserved[15];
};


union PS3RespDetails {
	unsigned int xfer_cnt;
	unsigned int respData[4];
	struct PS3MgrTaskRespInfo taskMgrRespInfo;
	struct PS3MgrCmdReplyRespInfo replyCmdRespInfo;
};


struct PS3SasDirectRespStatus {
	unsigned int status : 8;
	unsigned int dataPres : 2;
	unsigned int reserved : 22;
};


struct Ps3SasDirectRespFrameIU {
	union {
		unsigned char reserved0[8];
		unsigned long long mediumErrorLba;
	};
	unsigned char reserved1[2];
	unsigned char dataPres;
	unsigned char status;
	union {
		unsigned int reserved2;
		unsigned int xfer_cnt;
	};
	unsigned int senseDataLen;
	unsigned int respDataLen;
	unsigned char data[PS3_SENSE_BUFFER_SIZE];
	unsigned char reserved3[8];
};


struct PS3NormalRespFrame {
	union PS3RespDetails respDetail;
	unsigned char reserved1[8];
	unsigned char sense[PS3_SENSE_BUFFER_SIZE];
	unsigned char type;
	unsigned char reserved2[3];
	unsigned char respStatus;
	unsigned char dataPre;
	unsigned char reserved3[2];
};

union PS3RespFrame {
	struct Ps3SasDirectRespFrameIU sasRespFrame;
	struct PS3NormalRespFrame normalRespFrame;
};


struct Ps3DebugMemEntry {
	unsigned long long debugMemAddr;
	unsigned int debugMemSize;
	unsigned int reserved;
};


struct PS3NvmeCmdStatus {
	union {
		struct {
			unsigned short sc : 8;
			unsigned short sct : 3;
			unsigned short crd : 2;
			unsigned short m : 1;
			unsigned short dnr : 1;
			unsigned short p : 1;
		};
		unsigned short cmdStatus;
	};
};

#endif
