/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __S1861_HIL_REG0_PS3_REQUEST_QUEUE_REG_H__
#define __S1861_HIL_REG0_PS3_REQUEST_QUEUE_REG_H__
#include "s1861_global_baseaddr.h"
#ifndef __S1861_HIL_REG0_PS3_REQUEST_QUEUE_REG_MACRO__
#define HIL_REG0_PS3_REQUEST_QUEUE_PS3_REQUEST_QUEUE_ADDR                      \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x0)
#define HIL_REG0_PS3_REQUEST_QUEUE_PS3_REQUEST_QUEUE_RST (0xFFFFFFFFFFFFFFFF)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOERRCNT_ADDR                             \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOERRCNT_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_ADDR                             \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x10)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_RST (0x0000000C1FFF0000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELCONFIG_ADDR                        \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x18)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELCONFIG_RST (0x0000000300000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFORST_ADDR                                \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x20)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFORST_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOIOCNT_ADDR                              \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x28)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOIOCNT_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOFLOWCNT_ADDR                            \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x30)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOFLOWCNT_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_STATUS_ADDR                        \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x38)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_STATUS_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_SET_ADDR                           \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x40)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_SET_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_CLR_ADDR                           \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x48)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_CLR_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_MASK_ADDR                          \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x50)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_MASK_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_CNT_CLR_ADDR                           \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x58)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_CNT_CLR_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOORDERERROR_ADDR                         \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x60)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOORDERERROR_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFODINSHIFT_ADDR(_n)                       \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x68 + (_n) * 0x8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFODINSHIFT_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFODOUTSHIFT_ADDR(_n)                      \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x88 + (_n) * 0x8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFODOUTSHIFT_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_MAXLEVEL_ADDR                    \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xa8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_MAXLEVEL_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOINIT_ADDR                               \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xb0)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOINIT_RST (0x0000000000000002)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOINIT_EN_ADDR                            \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xb8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOINIT_EN_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOINIT_MAX_ADDR                           \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xc0)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOINIT_MAX_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_ECC_CNT_ADDR                     \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xc8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_ECC_CNT_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_ECC_ADDR_ADDR                    \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xd0)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_ECC_ADDR_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_DECODER_OVERFLOW_ADDR                  \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xd8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_DECODER_OVERFLOW_RST                   \
	(0x000000000000003F)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_ECC_BAD_PROJECT_ADDR                   \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xe0)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_ECC_BAD_PROJECT_RST (0x0000000000000001)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOOVERFLOW_WORD_ADDR                      \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xe8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOOVERFLOW_WORD_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORCTL_ADDR                    \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xf0)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORCTL_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORCNTCLR_ADDR                 \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xf8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORCNTCLR_RST                  \
	(0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORLOW_ADDR                    \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x100)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORLOW_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORMID_ADDR                    \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x108)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORMID_RST (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORHIGH_ADDR                   \
	(HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x110)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORHIGH_RST (0x0000000000000000)
#endif

#ifndef __S1861_HIL_REG0_PS3_REQUEST_QUEUE_REG_STRUCT__
union HilReg0Ps3RequestQueuePs3RequestQueue {
	unsigned long long val;
	struct {

		unsigned long long port : 64;
	} reg;
};

union HilReg0Ps3RequestQueueFifoErrCnt {
	unsigned long long val;
	struct {

		unsigned long long waddrerr : 32;
		unsigned long long reserved1 : 32;
	} reg;
};

union HilReg0Ps3RequestQueueFifoStatus {
	unsigned long long val;
	struct {

		unsigned long long filled : 16;
		unsigned long long fifoDepth : 16;
		unsigned long long almostfull : 1;
		unsigned long long full : 1;
		unsigned long long almostempty : 1;
		unsigned long long empty : 1;
		unsigned long long reserved6 : 28;
	} reg;
};

union HilReg0Ps3RequestQueueFifoLevelConfig {
	unsigned long long val;
	struct {

		unsigned long long cfgAempty : 16;
		unsigned long long cfgAfull : 16;
		unsigned long long emptyProtect : 1;
		unsigned long long fullProtect : 1;
		unsigned long long reserved4 : 30;
	} reg;
};

union HilReg0Ps3RequestQueueFifoRst {
	unsigned long long val;
	struct {

		unsigned long long resetPls : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RequestQueueFifoIOCnt {
	unsigned long long val;
	struct {

		unsigned long long wr : 32;
		unsigned long long rd : 32;
	} reg;
};

union HilReg0Ps3RequestQueueFifoFlowCnt {
	unsigned long long val;
	struct {

		unsigned long long overflow : 32;
		unsigned long long underflow : 32;
	} reg;
};

union HilReg0Ps3RequestQueueFifoIntStatus {
	unsigned long long val;
	struct {

		unsigned long long overflowStatus : 1;
		unsigned long long underflowStatus : 1;
		unsigned long long nemptyStatus : 1;
		unsigned long long eccBadStatus : 1;
		unsigned long long reserved4 : 60;
	} reg;
};

union HilReg0Ps3RequestQueueFifoIntSet {
	unsigned long long val;
	struct {

		unsigned long long overflowSet : 1;
		unsigned long long underflowSet : 1;
		unsigned long long nemptySet : 1;
		unsigned long long eccBadSet : 1;
		unsigned long long reserved4 : 60;
	} reg;
};

union HilReg0Ps3RequestQueueFifoIntClr {
	unsigned long long val;
	struct {

		unsigned long long overflowClr : 1;
		unsigned long long underflowClr : 1;
		unsigned long long nemptyClr : 1;
		unsigned long long eccBadClr : 1;
		unsigned long long reserved4 : 60;
	} reg;
};

union HilReg0Ps3RequestQueueFifoIntMask {
	unsigned long long val;
	struct {

		unsigned long long overflowMask : 1;
		unsigned long long underflowMask : 1;
		unsigned long long nemptyMask : 1;
		unsigned long long eccBadMask : 1;
		unsigned long long reserved4 : 60;
	} reg;
};

union HilReg0Ps3RequestQueueFifoCntClr {
	unsigned long long val;
	struct {

		unsigned long long fifowrcntClr : 1;
		unsigned long long fifordcntClr : 1;
		unsigned long long fifoerrcntClr : 1;
		unsigned long long fifoordererrwrcntClr : 1;
		unsigned long long fifoordererrrdcntClr : 1;
		unsigned long long fifobit1errcntClr : 1;
		unsigned long long fifobit2errcntClr : 1;
		unsigned long long reserved7 : 57;
	} reg;
};

union HilReg0Ps3RequestQueueFifoOrderError {
	unsigned long long val;
	struct {

		unsigned long long wrcnt : 32;
		unsigned long long rdcnt : 32;
	} reg;
};

union HilReg0Ps3RequestQueueFifoDinShift {
	unsigned long long val;
	struct {

		unsigned long long val : 64;
	} reg;
};

union HilReg0Ps3RequestQueueFifoDoutShift {
	unsigned long long val;
	struct {

		unsigned long long val : 64;
	} reg;
};

union HilReg0Ps3RequestQueueFifostatusMaxlevel {
	unsigned long long val;
	struct {

		unsigned long long val : 16;
		unsigned long long reserved1 : 48;
	} reg;
};

union HilReg0Ps3RequestQueueFifoInit {
	unsigned long long val;
	struct {

		unsigned long long stat : 2;
		unsigned long long reserved1 : 62;
	} reg;
};

union HilReg0Ps3RequestQueueFifoinitEn {
	unsigned long long val;
	struct {

		unsigned long long start : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RequestQueueFifoinitMax {
	unsigned long long val;
	struct {

		unsigned long long num : 16;
		unsigned long long reserved1 : 48;
	} reg;
};

union HilReg0Ps3RequestQueueFifostatusEccCnt {
	unsigned long long val;
	struct {

		unsigned long long bit1Err : 32;
		unsigned long long bit2Err : 32;
	} reg;
};

union HilReg0Ps3RequestQueueFifostatusEccAddr {
	unsigned long long val;
	struct {

		unsigned long long errPoint : 64;
	} reg;
};

union HilReg0Ps3RequestQueueFifoDecoderOverflow {
	unsigned long long val;
	struct {

		unsigned long long rCmdwordEmpty : 1;
		unsigned long long rPortindexEmpty : 1;
		unsigned long long rCmdbackEmpty : 1;
		unsigned long long wCmdwordEmpty : 1;
		unsigned long long wPortindexEmpty : 1;
		unsigned long long wCmdbackEmpty : 1;
		unsigned long long rCmdwordFull : 1;
		unsigned long long rPortindexFull : 1;
		unsigned long long rCmdbackFull : 1;
		unsigned long long wCmdwordFull : 1;
		unsigned long long wPortindexFull : 1;
		unsigned long long wCmdbackFull : 1;
		unsigned long long reserved12 : 52;
	} reg;
};

union HilReg0Ps3RequestQueueFifoEccBadProject {
	unsigned long long val;
	struct {

		unsigned long long en : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RequestQueueFifooverflowWord {
	unsigned long long val;
	struct {

		unsigned long long record : 64;
	} reg;
};

union HilReg0Ps3RequestQueueFifoLevelMonitorCtl {
	unsigned long long val;
	struct {

		unsigned long long low : 16;
		unsigned long long high : 16;
		unsigned long long en : 1;
		unsigned long long reserved3 : 31;
	} reg;
};

union HilReg0Ps3RequestQueueFifoLevelMonitorCntClr {
	unsigned long long val;
	struct {

		unsigned long long en : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RequestQueueFifoLevelMonitorLow {
	unsigned long long val;
	struct {

		unsigned long long cnt : 64;
	} reg;
};

union HilReg0Ps3RequestQueueFifoLevelMonitorMid {
	unsigned long long val;
	struct {

		unsigned long long cnt : 64;
	} reg;
};

union HilReg0Ps3RequestQueueFifoLevelMonitorHigh {
	unsigned long long val;
	struct {

		unsigned long long cnt : 64;
	} reg;
};

struct HilReg0Ps3RequestQueue {

	union HilReg0Ps3RequestQueuePs3RequestQueue ps3RequestQueue;
	union HilReg0Ps3RequestQueueFifoErrCnt fifoErrCnt;
	union HilReg0Ps3RequestQueueFifoStatus fifoStatus;
	union HilReg0Ps3RequestQueueFifoLevelConfig fifoLevelConfig;
	union HilReg0Ps3RequestQueueFifoRst fifoRst;
	union HilReg0Ps3RequestQueueFifoIOCnt fifoIOCnt;
	union HilReg0Ps3RequestQueueFifoFlowCnt fifoFlowCnt;
	union HilReg0Ps3RequestQueueFifoIntStatus fifoIntStatus;
	union HilReg0Ps3RequestQueueFifoIntSet fifoIntSet;
	union HilReg0Ps3RequestQueueFifoIntClr fifoIntClr;
	union HilReg0Ps3RequestQueueFifoIntMask fifoIntMask;
	union HilReg0Ps3RequestQueueFifoCntClr fifoCntClr;
	union HilReg0Ps3RequestQueueFifoOrderError fifoOrderError;
	union HilReg0Ps3RequestQueueFifoDinShift fifoDinShift[4];
	union HilReg0Ps3RequestQueueFifoDoutShift fifoDoutShift[4];
	union HilReg0Ps3RequestQueueFifostatusMaxlevel fifoStatusMaxLevel;
	union HilReg0Ps3RequestQueueFifoInit fifoInit;
	union HilReg0Ps3RequestQueueFifoinitEn fifoinitEn;
	union HilReg0Ps3RequestQueueFifoinitMax fifoinitMax;
	union HilReg0Ps3RequestQueueFifostatusEccCnt fifoStatusEccCnt;
	union HilReg0Ps3RequestQueueFifostatusEccAddr fifoStatusEccAddr;
	union HilReg0Ps3RequestQueueFifoDecoderOverflow fifoDecoderOverflow;
	union HilReg0Ps3RequestQueueFifoEccBadProject fifoEccBadProject;
	union HilReg0Ps3RequestQueueFifooverflowWord fifoOverFlowWord;
	union HilReg0Ps3RequestQueueFifoLevelMonitorCtl fifoLevelMonitorCtl;
	union HilReg0Ps3RequestQueueFifoLevelMonitorCntClr
		fifoLevelMonitorCntClr;
	union HilReg0Ps3RequestQueueFifoLevelMonitorLow fifoLevelMonitorLow;
	union HilReg0Ps3RequestQueueFifoLevelMonitorMid fifoLevelMonitorMid;
	union HilReg0Ps3RequestQueueFifoLevelMonitorHigh fifoLevelMonitorHigh;
};
#endif
#endif
