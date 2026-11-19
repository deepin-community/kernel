/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_HTP_REQ_FRAME_HW_H_
#define _PS3_HTP_REQ_FRAME_HW_H_

#include "ps3_htp_def.h"

#define ENCODE_CCS_XFERLEN(x) (((unsigned int)(x) >> 2))
#define DECODE_CCS_XFERLEN(x) (((unsigned int)(x) << 2))

#ifdef _WINDOWS
#define __attribute__(x)
#pragma pack(push, 1)
#endif

struct PS3NvmeSglDesc {
	unsigned long long addr;
	union {
		struct {
			unsigned char reserved[7];
			unsigned char subtype : 4;
			unsigned char type : 4;
		} generic;

		struct {
			unsigned int length;
			unsigned char reserved[3];
			unsigned char subtype : 4;
			unsigned char type : 4;
		} unkeyed;

		struct {
			unsigned long long length : 24;
			unsigned long long key : 32;
			unsigned long long subtype : 4;
			unsigned long long type : 4;
		} keyed;
	};
};

struct PS3NvmeCmdDw0_9 {
	unsigned short opcode : 8;
	unsigned short fuse : 2;
	unsigned short reserved1 : 4;
	unsigned short psdt : 2;
	unsigned short cID;

	unsigned int nsID;

	unsigned int reserved2;
	unsigned int reserved3;

	unsigned long long mPtr;


	union {
		struct {
			unsigned long long prp1;
			unsigned long long prp2;
		} prp;
		struct PS3NvmeSglDesc sgl1;

	} dPtr;
};


struct PS3NvmeCommonCmd {
	struct PS3NvmeCmdDw0_9 cDW0_9;

	unsigned int cDW10;
	unsigned int cDW11;
	unsigned int cDW12;
	unsigned int cDW13;
	unsigned int cDW14;
	unsigned int cDW15;
};

struct PS3NvmeRWCmd {
	struct PS3NvmeCmdDw0_9 cDW0_9;

	unsigned int sLbaLo;
	unsigned int sLbaHi;
	unsigned int numLba;
	unsigned int cDW13;
	unsigned int cDW14;
	unsigned int cDW15;
};


union PS3NvmeReqFrame {
	struct PS3NvmeCommonCmd commonReqFrame;
	struct PS3NvmeRWCmd rwReqFrame;
};

#ifdef _WINDOWS
#pragma pack(pop)
#endif

#endif
