/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_IODT_H_
#define _PS3_IODT_H_

#include "ps3_htp_def.h"

enum IodtProtocolType {
	PROTOCOL_SMP = 0b000,
	PROTOCOL_SSP = 0b001,
	PROTOCOL_STP = 0b010,
	PROTOCOL_DIRT = 0b111,
};

enum IodtFrameType {
	FRAMETYPE_SSP_CMD = 0b001,
	FRAMETYPE_SSP_SMP = 0b001,
	FRAMETYPE_SATA_NONDATA = 0b001,

	FRAMETYPE_SSP_TASK = 0b010,
	FRAMETYPE_SATA_PIO = 0b010,

	FRAMETYPE_SATA_DMA = 0b011,

	FRAMETYPE_SATA_FPDMA = 0b100,

	FRAMETYPE_SATA_ATAPI = 0b101,
	FRAMETYPE_DIRECT = 0b111,
};

enum IodtIuSrc {
	IU_SRC_MEM = 0b00,
	IU_SRC_IODT = 0b01,
	IU_SRC_TUPLE = 0b10,
	IU_SRC_SATA = 0b11,
};

enum IodtSgeMode {
	IODT_SGEMODE_DIRECT = 0x0,
	IODT_SGEMODE_SGL = 0x1,
};

enum IodtEedpMode {
	EEDP_MODE_CHECK = 0x0,
	EEDP_MODE_INSERT = 0x1,
	EEDP_MODE_REPLACE = 0x2,
	EEDP_MODE_RMV = 0x3,
};

enum AbortCtrl {

	ABT_SASSATA_TASK = 0b00,
	ABT_SAS_TASKSET = 0b01,
	ABT_LOCAL_BY_DISK = 0b10,
	ABT_LOCAL_BY_PORT = 0b11,

	ABT_MGT_IO = 0b00,
	ABT_SOFTRESET = 0b01,
	ABT_READ_NCQ_ERR_LOG = 0b10,
	ABT_SMP = 0b11,
};

enum DirectFlag {
	DIRECT_FLAG_NORMAL = 0b00,
	DIRECT_FLAG_DIRECT = 0b10,
};

enum CmdWordType {
	CMD_WORD_TYPE_ABORT = 0b00,
	CMD_WORD_TYPE_MGT = 0b01,
	CMD_WORD_TYPE_READ = 0b10,
	CMD_WORD_TYPE_WRITE = 0b11,
};

enum SmpFrameType {
	SMP_REQ = 0x40,
	SMP_RESP = 0x41,
};

enum eedpMode {
	EEDP_NONE = 0b0000,
	EEDP_CHK = 0b0001,
	EEDP_INST = 0b0010,
	EEDP_REPL = 0b0100,
	EEDP_RMV = 0b1000,
};

union IoDmaCfg {
	struct {
		unsigned char eedpEn : 1;
		unsigned char eedpSgMod : 1;
		unsigned char twoSglMod : 1;
		unsigned char sgMode : 1;
		unsigned char eedpMode : 4;
	};
	unsigned char byte;
};


struct __packed SspTaskFrameIu {
	unsigned long long LUN;
	unsigned short reserved0;
	unsigned char function;
	unsigned char reserved1;
	unsigned short manageTag;
	unsigned char reserved2[14];
};


union __packed SmpFrameIu {
	struct {
		unsigned char frameType;
		unsigned char reqestBytes[31];
	};
	unsigned char smpIURaw[32];
};


struct __packed DfifoWordCommon {

	union {
		struct {
			unsigned short type : 2;
			unsigned short rsv1 : 2;
			unsigned short direct : 2;
			unsigned short rFifoID : 4;
			unsigned short rsv2 : 6;
		};
		unsigned short WD0;
	};


	union {

		struct {
			union {

				struct {
					unsigned short darID : 13;
					unsigned short rsv3 : 2;

					unsigned short function : 1;
				};

				struct {
					unsigned short manageIptt : 13;
				};
			};
		};

		struct {
			union {

				struct {
					unsigned short reqFrameID : 13;
				};

				struct {
					unsigned short manageReqFrameID : 13;
				};
			};
		};
		unsigned short WD1;
	};

	union {
		struct {
			unsigned short phyDiskID : 12;
			unsigned short rsv4 : 2;
			unsigned short abortCtrl : 2;
		};
		unsigned short WD2;
	};
};

struct __packed IODT_V1 {
	union {

		struct __packed {
			union {
				struct {
					union {
						struct {
							unsigned char
								protocolType : 3;
							unsigned char
								frameType : 3;
							unsigned char iuSrc : 2;
						};
						unsigned char byte0;
					};

					union IoDmaCfg dmaCfg;
				};
				unsigned short config;
			};



			unsigned short cmdLen : 9;
			unsigned short rsv0 : 7;

			union {
				struct {
					unsigned int taskDarID : 13;
					unsigned int resv0 : 19;
				};
				struct {
					unsigned int dataBufLenDWAlign : 24;

					unsigned int rsvd0 : 1;
					unsigned int cmdDir : 1;
					unsigned int refTagEn : 1;
					unsigned int appTagEn : 1;
					unsigned int guardTagEn : 1;
					unsigned int refTagInc : 1;
					unsigned int rsv1 : 1;
					unsigned int aborted : 1;
				};
			};
		};
		unsigned long long QW0;
	};


	union {
		struct __packed {
			struct DfifoWordCommon commonWord;
			unsigned short rsv2 : 1;
			unsigned short sataCtl : 1;
			unsigned short rsv3 : 2;
			unsigned short sasCtl : 1;
			unsigned short sataByteBlock0 : 1;
			unsigned short sataByteBlock1 : 1;
			unsigned short rsv4 : 9;
		};
		unsigned long long QW1;
	};


	union {
		unsigned long long dataBaseAddr;
		unsigned long long QW2;
	};


	union {
		unsigned long long eedpBaseAddr;
		unsigned long long QW3;
	};


	union {

		struct {
			unsigned long long cmdIUAddr;
			unsigned long long rsv9;

			unsigned long long refTag : 32;
			unsigned long long appTag : 16;
			unsigned long long rsv10 : 16;

			unsigned long long rsv11;
		} A;

		union {
			unsigned char cdb[32];
			struct SspTaskFrameIu taskIU;
			union SmpFrameIu smpIU;
		} B;


		struct {
			unsigned long long opCode : 8;
			unsigned long long rsv12 : 56;

			unsigned long long lba : 48;
			unsigned long long rsv13 : 16;

			unsigned long long refTag : 32;
			unsigned long long appTag : 16;
			unsigned long long rsv14 : 16;

			unsigned long long rsv15;
		} C;

		struct {
			unsigned int ataCmd : 8;
			unsigned int ataDev : 8;
			unsigned int ataCtl : 8;
			unsigned int ataIcc : 8;
			unsigned int ataSecCnt : 16;
			unsigned int ataFeature : 16;

			union {
				struct {
					unsigned long long ataLba : 48;
					unsigned long long rsv16 : 16;
				};
				struct {
					unsigned char lba0;
					unsigned char lba1;
					unsigned char lba2;
					unsigned char lba3;
					unsigned char lba4;
					unsigned char lba5;
				};
				unsigned char lba[6];
			};

			unsigned int ataAuxiliary;
			unsigned int rsv17;

			unsigned long long rsv18;
		} D;
	};
};

enum {
	CMD_LEN_THR = 32,
	CMD_LEN_S = 7,
	CMD_LEN_L = 11,
};

#endif
