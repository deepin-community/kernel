/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_HTP_REQFRAME_H_
#define _PS3_HTP_REQFRAME_H_

#include "ps3_htp_def.h"
#include "ps3_htp_dev.h"
#include "ps3_htp_sas.h"
#include "ps3_htp_req_frame_hw.h"

enum {
	PS3_FRAME_SGE_BUFSIZE = 4096,
	PS3_FRAME_SGE_SHIFT = 12,
	PS3_FRAME_REQ_SGE_NUM_FE = 8,
	PS3_FRAME_REQ_PRP_NUM_FE = 2,
	PS3_FRAME_REQ_SGE_NUM_HW = 8,
	PS3_FRAME_REQ_PRP_NUM_HW = 2,
	PS3_FRAME_REQ_SGE_NUM_MGR = 11,
	PS3_FRAME_REQ_EXT_SGE_MIN = 2,
	PS3_FRAME_CDB_BUFLEN = 32,
	PS3_FRAME_LUN_BUFLEN = 8,
	PS3_DEBUG_MEM_ARRAY_MAX_NUM = 16,
	PS3_MAX_DMA_MEM_SIZE = 4096,
	PS3_MAX_DEBUG_MEM_SIZE_PARA = 65536,
	PS3_DRV_NAME_MAX_LEN = 32,
	PS3_DRV_VERSION_MAX_LEN = 24,
};

enum {
	PS3_DATA_DIRECTION_WRITE = 0,
	PS3_DATA_DIRECTION_READ = 1,
};

enum {
	PS3_REQFRAME_FORMAT_FRONTEND = 0,
	PS3_REQFRAME_FORMAT_SAS = 1,
	PS3_REQFRAME_FORMAT_SATA = 2,
	PS3_REQFRAME_FORMAT_NVME = 3,
};

enum {
	PS3_LINUX_FRAME = 0,
	PS3_WINDOWS_FRAME = 1,
};

enum {
	PS3_COMPAT_VER_DEFAULT = 0,
	PS3_COMPAT_VER_1 = 1,
	PS3_COMPAT_VER_MAX = 0xffff,
};

struct PS3Sge {
	unsigned long long addr;
	unsigned int length;
	unsigned int reserved1 : 30;
	unsigned int lastSge : 1;
	unsigned int ext : 1;
};

struct PS3Prp {
	unsigned long long prp1;
	unsigned long long prp2;
};



struct PS3SoftwareZone {
	unsigned long long virtDiskLba;
	unsigned int numBlocks;
	unsigned char opcode;
	unsigned char sglOffset;
	unsigned char sglFormat : 2;
	unsigned char isResendCmd : 1;
	unsigned char reserved1 : 5;
	unsigned char reserved2;
	unsigned short subOpcode;
	unsigned short sgeCount : 9;
	unsigned short reserved3 : 7;
	unsigned char reserved4[4];
};

struct PS3ReqFrameHead {
	unsigned char cmdType;
	unsigned char cmdSubType;
	unsigned short cmdFrameID;
	union{
		struct {
		unsigned int noReplyWord    : 1;
		unsigned int dataFormat     : 1;
		unsigned int reqFrameFormat : 2;
		unsigned int mapBlockVer    : 2;
		unsigned int isWrite	    : 1;
		unsigned int isStream1	    : 1;
		unsigned int reserved	    : 16;
		unsigned int ncq_prio	    : 1;
		unsigned int reserved2	    : 7;
		};
		unsigned int control;
	};
	union PS3DiskDev devID;
	unsigned short timeout;
	unsigned short virtDiskSeq;
	unsigned short reserved1[4];
	unsigned long long traceID;
};

struct PS3HwReqFrame {
	struct PS3ReqFrameHead reqHead;
	struct PS3SoftwareZone softwareZone;
	unsigned char reserved[8];
	union {
		struct IODT_V1 sasReqFrame;
		union PS3NvmeReqFrame nvmeReqFrame;
	};
	struct PS3Sge sgl[PS3_FRAME_REQ_SGE_NUM_FE];
};

struct PS3VDAccAttr {
	unsigned long long firstPdStartLba;
	unsigned char firstSpanNo;
	unsigned char fisrtSeqInSpan;
	unsigned char secondSeqInSapn;
	unsigned char thirdSeqInSapn;
	unsigned char clineCount;
	unsigned char isAccActive : 1;
	unsigned char isStream : 1;
	unsigned char reserved1 : 6;
	unsigned short ioOutStandingCnt;
	unsigned char reserved2[16];
};


struct PS3FrontEndReqFrame {
	struct PS3ReqFrameHead reqHead;
	unsigned char cdb[PS3_FRAME_CDB_BUFLEN];
	struct PS3VDAccAttr vdAccAttr;
	unsigned int dataXferLen;
	unsigned char reserved[25];
	unsigned char sgeOffset;
	unsigned short sgeCount;
	union {
		struct PS3Sge sgl[PS3_FRAME_REQ_SGE_NUM_FE];
		struct PS3Prp prp;
	};
};

struct PS3MgrDev {
	struct PS3DiskDevPos devID;
	unsigned short num;
	unsigned char devType;
	unsigned char reserved[17];
};

struct PS3MgrEvent {
	unsigned int eventTypeMap;
	unsigned int eventTypeMapProcResult;
	unsigned int eventLevel;
	unsigned char reserved[20];
};


struct PS3SasMgr {
	unsigned long long sasAddr;
	unsigned char enclID;
	unsigned char startPhyID;
	unsigned char phyCount;
	unsigned char reserved1;
	unsigned short reqLen;
	unsigned char reserved2[2];
};


struct PS3SasPhySet {
	unsigned long long sasAddr;
	unsigned char phyID;
	unsigned char minLinkRate;
	unsigned char maxLinkRate;
	unsigned char phyCtrl;
	unsigned char reserved[3];
};

union PS3MgrReqDiffValue {
	unsigned char word[32];
	unsigned short originalCmdFrameID;
	unsigned char eventStart;
	struct PS3MgrDev dev;
	struct PS3MgrEvent event;
	struct PS3SasMgr sasMgr;
	struct PS3SasPhySet phySet;
	unsigned char unLoadType;
	int isRetry;
};


struct PS3MgrReqFrame {
	struct PS3ReqFrameHead reqHead;
	unsigned short sgeCount;
	unsigned char sgeOffset;
	unsigned char syncFlag;
	unsigned short timeout;
	unsigned char abortFlag;
	unsigned char pendingFlag;
	union PS3MgrReqDiffValue value;
	unsigned char osType;
	unsigned char suspend_type;
	unsigned char reserved[6];
	struct PS3Sge sgl[PS3_FRAME_REQ_SGE_NUM_MGR];
};


struct PS3MgrTaskReqFrame {
	struct PS3ReqFrameHead reqHead;
	unsigned short taskID;
	unsigned char lun[PS3_FRAME_LUN_BUFLEN];
	unsigned char abortedCmdType;
	unsigned char reserved[5];
};


union PS3ReqFrame {
	struct PS3MgrTaskReqFrame taskReq;
	struct PS3MgrReqFrame mgrReq;
	struct PS3FrontEndReqFrame frontendReq;
	struct PS3HwReqFrame hwReq;
	unsigned char word[256];
};


struct PS3DrvInfo {
	char drvName[PS3_DRV_NAME_MAX_LEN];
	char drvVersion[PS3_DRV_VERSION_MAX_LEN];
	unsigned long long bus;
	unsigned char dev : 5;
	unsigned char func : 3;
	unsigned char domain_support : 1;
	unsigned char reserved : 7;
	unsigned short compatVer;
	unsigned int domain;
	unsigned char reserved1[56];
};


enum {
	PS3_MEM_TYPE_UNKNOWN = 0,
	PS3_MEM_TYPE_SO = 1,
	PS3_MEM_TYPE_RO = 2,
};


struct PS3HostMemInfo {
	unsigned long long startAddr;
	unsigned long long endAddr;
	unsigned char type;
	unsigned char reserved[7];
};

struct PS3InitReqFrame {
	struct PS3ReqFrameHead reqHead;
	unsigned char ver;
	unsigned char reserved0;
	unsigned short length;
	unsigned char operater;
	unsigned char pageSize;
	unsigned char pciIrqType;
	unsigned char osType;
	unsigned char reserved1[6];
	unsigned short msixVector;
	unsigned long long timeStamp;
	unsigned long long reqFrameBufBaseAddr;
	unsigned long long hostMemInfoBaseAddr;
	unsigned int hostMemInfoNum;
	unsigned char reserved2[20];

	unsigned long long replyFifoDescBaseAddr;

	unsigned long long respFrameBaseAddr;
	unsigned int eventTypeMap;
	unsigned short reqFrameMaxNum;
	unsigned short respFrameMaxNum;
	unsigned long long filterTableAddr;
	unsigned int filterTableLen;
	unsigned short bufSizePerRespFrame;
	unsigned char hilMode;
	unsigned char reserved3[33];
	unsigned long long systemInfoBufAddr;
	unsigned long long debugMemArrayAddr;
	unsigned int debugMemArrayNum;
	unsigned int dumpDmaBufLen;
	unsigned long long dumpDmaBufAddr;
	unsigned int dumpIsrSN;
	unsigned short drvInfoBufLen;
	unsigned char reserverd4[2];
	unsigned long long drvInfoBufAddr;
	unsigned char reserved5[36];
	unsigned int respStatus;
};

#endif
