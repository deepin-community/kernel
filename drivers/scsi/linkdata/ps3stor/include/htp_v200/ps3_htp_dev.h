/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_HTP_DEV_H_
#define _PS3_HTP_DEV_H_

#include "ps3_htp_def.h"

#define PS3_MAX_CHANNEL_NUM 15
#define PS3_MAX_RANDOM_NUM 32
#define PS3_MAX_IV_NUM 16
#define PS3_SECURITY_CIPHER_NUM_MAX 2
#define PS3_STABLE_WRITES_MASK (0x1)


struct PS3IocCtrlProp {
	unsigned int enableSnapshot : 1;
	unsigned int enableSoftReset : 1;
	unsigned int reserved1 : 30;
	unsigned int reserved2;
};


struct PS3IocCtrlCapable {
	unsigned int supportUnevenSpans : 1;
	unsigned int supportJbodSecure : 1;
	unsigned int supportNvmePassthru : 1;
	unsigned int supportDirectCmd : 1;
	unsigned int supportAcceleration : 1;
	unsigned int supportSataDirectCmd : 1;
	unsigned int supportSataNcq : 1;
	unsigned int reserved1 : 25;
	unsigned int reserved2[3];
};

#define PS3_IOC_CLUSTER_SERIAL_NO_SIZE 16

struct PS3ChannelAttr {
	unsigned short channelType : 4;
	unsigned short maxDevNum : 12;
};

struct PS3ChannelInfo {
	unsigned char channelNum;
	unsigned char reserved;
	struct PS3ChannelAttr channels[PS3_MAX_CHANNEL_NUM];
};

struct PS3QosInfo {
	unsigned short tfifoDepth;
	unsigned short sataHddQuota;
	unsigned short sataSsdQuota;
	unsigned short sasHddQuota;
	unsigned short sasSsdQuota;
	unsigned short nvmeVdQuota;
	unsigned short nvmeDirectQuota;
	unsigned short nvmeNormalQuota;
};


struct PS3IocCtrlInfo {

	unsigned short maxVdCount;

	unsigned short maxPdCount;

	unsigned int maxSectors;
	struct PS3IocCtrlProp properties;
	struct PS3IocCtrlCapable capabilities;

	unsigned char scsiTaskAbortTimeout;
	unsigned char scsiTaskResetTimeout;

	unsigned short offsetOfVDID;
	unsigned char reserved1[2];

	unsigned short cancelTimeOut;

	unsigned int vdIOThreshold;

	unsigned char iocPerfMode;

	unsigned char vdQueueNum;

	unsigned char ioTimeOut;
	unsigned char hwVdMaxIOSize : 4;
	unsigned char reserved2 : 4;

	struct PS3ChannelInfo channelInfo;

	struct PS3QosInfo qosInfo;
	unsigned short isotoneTimeOut;
	unsigned char reserved3[2];

	unsigned char reserved4[32];

};

struct PS3Dev {
	union {
		unsigned short phyDiskID;
		unsigned short virtDiskID;
	};
	unsigned short softChan : 4;
	unsigned short devID : 12;
};

union PS3DiskDev {
	unsigned int diskID;
	struct PS3Dev ps3Dev;
};


struct PS3DiskDevPos {
	union {
		struct {
			unsigned char checkSum;
			unsigned char enclId;
			unsigned char phyId;
		};
		unsigned int diskMagicNum;
	};
	union PS3DiskDev diskDev;
};

struct PS3PhyDevice {
	struct PS3DiskDevPos diskPos;
	unsigned char diskState;
	unsigned char configFlag;
	unsigned char driverType : 4;
	unsigned char mediumType : 4;
	unsigned char reserved;
	unsigned char reserved1[4];
};

struct PS3VirtDevice {
	struct PS3DiskDevPos diskPos;
	unsigned char accessPolicy;
	unsigned char isHidden;
	unsigned char diskState;
	unsigned char reserved;
	unsigned char reserved1[4];
};

union PS3Device {
	struct PS3PhyDevice pd;
	struct PS3VirtDevice vd;
};

struct PS3DevList {
	unsigned short count;
	unsigned char reserved[6];
	union PS3Device devs[];
};

struct PS3PDInfo {
	struct PS3DiskDevPos diskPos;
	unsigned char diskState;
	unsigned char configFlag;
	unsigned char driverType : 4;
	unsigned char mediumType : 4;
	unsigned char scsiInterfaceType;
	unsigned char taskAbortTimeout;
	unsigned char taskResetTimeout;
	union {
		struct {
			unsigned char supportNCQ : 1;
			unsigned char protect : 1;
			unsigned char isDirectDisable : 1;
			unsigned char supportNcqPrio:1;
			unsigned char reserved : 4;
		};
		unsigned char pdFlags;
	};
	unsigned char reserved1;
	unsigned short sectorSize;
	unsigned char reserved2[2];
	unsigned char enclId;
	unsigned char phyId;
	unsigned char dmaAddrAlignShift;
	unsigned char dmaLenAlignShift;
	unsigned char reserved3[4];
	unsigned int maxIOSize;
	unsigned int devQueDepth;
	unsigned short normalQuota;
	unsigned short directQuota;
	unsigned char reserved4[20];
};


struct PS3Extent {
	union PS3DiskDev phyDiskID;
	unsigned char state;
	unsigned char reserved[3];
};

struct PS3Span {
	unsigned int spanStripeDataSize;
	unsigned char spanState;
	unsigned char spanPdNum;
	unsigned char reserved[2];
	struct PS3Extent extent[PS3_MAX_PD_COUNT_IN_SPAN];
};

struct PS3VDEntry {
	struct PS3DiskDevPos diskPos;
	unsigned short sectorSize;
	unsigned short stripSize;
	unsigned int stripeDataSize;
	unsigned short physDrvCnt;
	unsigned short diskGrpId;
	unsigned char accessPolicy;

	unsigned char isReadNeedSeq:1;
	unsigned char isWriteNeedSeq:1;
	unsigned char reserved1:6;
	unsigned char dmaAddrAlignShift;
	unsigned char dmaLenAlignShift;
	unsigned char isDirectEnable : 1;
	unsigned char isHidden : 1;
	unsigned char isNvme : 1;
	unsigned char isSsd : 1;
	unsigned char bdev_bdi_cap : 2;
	unsigned char isWriteDirectEnable : 1;
	unsigned char reserved2 : 1;
	unsigned char raidLevel;
	unsigned char spanCount;
	unsigned char diskState;
	unsigned short umapBlkDescCnt : 3;
	unsigned short umapNumblk : 13;
	unsigned short dev_busy_scale;
	unsigned long long startLBA;
	unsigned long long extentSize;
	unsigned long long mapBlock;
	unsigned long long capacity;
	unsigned char isTaskMgmtEnable;
	unsigned char taskAbortTimeout;
	unsigned char taskResetTimeout;
	unsigned char mapBlockVer;
	unsigned int maxIOSize;
	unsigned int devQueDepth;
	unsigned short virtDiskSeq;
	unsigned short normalQuota;
	unsigned short directQuota;
	unsigned short reserved4[21];
	struct PS3Span span[PS3_MAX_SPAN_IN_VD];
};

struct PS3VDInfo {
	unsigned short count;
	unsigned char reserved[6];
	struct PS3VDEntry vds[];
};

struct PS3DrvSysInfo {
	unsigned char version;
	unsigned char systemIDLen;
	unsigned char reserved[6];
	unsigned char systemID[PS3_DRV_SYSTEM_ID_MAX_LEN];
};


struct PS3PhyInfo {
	unsigned long long sasAddr;
	unsigned long long attachedSasAddr;
	unsigned char phyId;
	unsigned char negLinkRate;
	unsigned char slotId;
	unsigned char attachDevType;
	unsigned char initiatorPortProtocol : 4;
	unsigned char targetPortProtocols : 4;
	unsigned char attachInitiatorPortProtocol : 4;
	unsigned char attachTargetPortProtocols : 4;
	unsigned char minLinkRateHw : 4;
	unsigned char maxLinkRateHw : 4;
	unsigned char minLinkRate : 4;
	unsigned char maxLinkRate : 4;
	unsigned char enable : 1;
	unsigned char reserve : 7;
	unsigned char reserved[7];
};


struct PS3ExpanderInfo {
	unsigned long long sasAddr;
	unsigned long long parentSasAddr;
	unsigned char parentId;
	unsigned char enclID;
	unsigned char devType;
	unsigned char phyCount;
	unsigned char reserved[4];
};


struct PS3Expanders {
	unsigned char count;
	unsigned char reserved[7];
	unsigned long long hbaSasAddr[3];
	struct PS3ExpanderInfo expanders[];
};

struct PS3BiosInfo {
	unsigned char biosState;
	unsigned char biosMode;

	unsigned char biosAbs;
	unsigned char devMaxNum;
};

struct PS3BootDriveInfo {
	unsigned char hasBootDrive : 1;
	unsigned char isPD : 1;
	unsigned char reserved_9 : 6;
	unsigned char enclID;
	unsigned short slotID;
	unsigned short vdID;
	unsigned char pad[2];
};

struct PS3RandomInfo {
	unsigned char randomNum[PS3_MAX_RANDOM_NUM];
	unsigned char iv[PS3_MAX_IV_NUM];
};

struct PS3SecurityPwHead {
	unsigned char cipherNum;
	unsigned int cipherLegth[PS3_SECURITY_CIPHER_NUM_MAX];
	unsigned int cipherOffset[PS3_SECURITY_CIPHER_NUM_MAX];
	unsigned char iv[PS3_MAX_IV_NUM];
};

#endif
