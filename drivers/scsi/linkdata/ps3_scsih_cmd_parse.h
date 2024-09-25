#ifndef _PS3_SCSIH_CMD_PARSE_H_
#define _PS3_SCSIH_CMD_PARSE_H_

#include "ps3_err_def.h"

#include "ps3_inner_data.h"
#include "ps3_driver_log.h"

#ifndef _WINDOWS
#include <scsi/scsi.h>
#else

#define VARIABLE_LENGTH_CMD   0x7f
#define SYNCHRONIZE_CACHE_16  0x91
#define SYNCHRONIZE_CACHE     0x35

#define FORMAT_UNIT           0x04
#define REASSIGN_BLOCKS       0x07
#define READ_6                0x08
#define WRITE_6               0x0a
#define READ_10               0x28
#define WRITE_10              0x2a
#define WRITE_VERIFY          0x2e
#define VERIFY                0x2f
#define PRE_FETCH             0x34
#define SYNCHRONIZE_CACHE     0x35
#define WRITE_LONG            0x3f
#define WRITE_SAME            0x41
#define UNMAP                 0x42
#define VARIABLE_LENGTH_CMD   0x7f
#define READ_12               0xa8
#define WRITE_12              0xaa
#define WRITE_VERIFY_12       0xae
#define VERIFY_12             0xaf
#define EXTENDED_COPY         0x83
#define READ_16               0x88
#define COMPARE_AND_WRITE     0x89
#define WRITE_16              0x8a
#define WRITE_ATTRIBUTE       0x8d
#define PS3_WRITE_VERIFY_16   0x8e
#define VERIFY_16             0x8f
#define SYNCHRONIZE_CACHE_16  0x91
#define WRITE_SAME_16         0x93
#define SERVICE_ACTION_IN_16  0x9e
#define SERVICE_ACTION_OUT_16 0x9f
#define READ_32               0x09
#define VERIFY_32             0x0a
#define WRITE_32              0x0b
#define PS3_WRITE_VERIFY_32   0x0c
#define WRITE_SAME_32         0x0d

#endif

#define PS3_SCSI_CONFLICT_CHECK	0x80 
#define PS3_SCSI_CONFLICT_CHECK_TEST(type) 	((type) & PS3_SCSI_CONFLICT_CHECK)
#define PS3_SCSI_CMD_TYPE(type)	((type) & 0x7F)

enum ps3_scsi_cmd_type {
	PS3_SCSI_CMD_TYPE_UNKOWN,
	PS3_SCSI_CMD_TYPE_READ,
	PS3_SCSI_CMD_TYPE_WRITE,
	PS3_SCSI_CMD_TYPE_RW,
	PS3_SCSI_CMD_TYPE_UNMAP,
	PS3_SCSI_CMD_TYPE_NORW,
	PS3_SCSI_CMD_TYPE_COUNT
};

enum {
	PS3_ATA_OPC_READ_FPDMA = 0x60,
	PS3_ATA_OPC_WRITE_FPDMA =0x61,
	PS3_SATA_DEV_REG_DEFAULT = 0x40,
	PS3_NCQ_FUA_OFFSET_IN_DEVICE_REG = 7,
};

#define PS3_SERVICE_ACTION32(cdb) (*(cdb + 9) | (*(cdb + 8) << PS3_SHIFT_BYTE))
#define PS3_SERVICE_ACTION16_TO_OPCODE(cdb) (*(cdb + 1) & 0x1f)

static inline void ps3_scsih_cdb_opcode_get(const U8 *cdb,
	U8 *opcode, U16 *sub_opcode)
{
	*opcode = cdb[0];
	switch(cdb[0]) {
	case VARIABLE_LENGTH_CMD:
		*sub_opcode = PS3_SERVICE_ACTION32(cdb);
		break;
	case SERVICE_ACTION_IN_16:
	case SERVICE_ACTION_OUT_16:
		*sub_opcode = (U16)PS3_SERVICE_ACTION16_TO_OPCODE(cdb);
		break;
	default:
		*sub_opcode = 0;
		break;
	}
}

#ifndef U32_HIGH_LOW_TO_U64
#define U32_HIGH_LOW_TO_U64(u64_high, u64_low) \
	(((U64)(u64_high) << 32) | (u64_low))
#endif

Bool ps3_scsih_is_rw_type(U8 type);

U8 ps3_scsih_cdb_rw_type_get(const U8 *cdb);

void ps3_scsih_cdb_parse(const U8 *cdb, U32 *num_blocks,
	U32 *lba_lo, U32 *lba_hi, Bool *is_need_split);

static inline Bool ps3_scsih_cdb_is_rw_cmd(const U8 *cdb)
{
	U8 type = PS3_SCSI_CMD_TYPE(ps3_scsih_cdb_rw_type_get(cdb));
	return ps3_scsih_is_rw_type(type);
}

static inline Bool ps3_scsih_is_read_cmd(U8 type)
{
	return (type == PS3_SCSI_CMD_TYPE_READ);
}

static inline Bool ps3_scsih_is_write_cmd(U8 type)
{
	Bool ret = PS3_FALSE;

	switch (type)
	{
	case PS3_SCSI_CMD_TYPE_WRITE:
	case PS3_SCSI_CMD_TYPE_UNMAP:
	case PS3_SCSI_CMD_TYPE_RW:
		ret = PS3_TRUE;
		goto l_out;
	default:
		ret = PS3_FALSE;
		goto l_out;
	}
l_out:
	return ret;
}
static inline Bool ps3_scsih_is_sync_cache(const U8 *cdb)
{
	return cdb[0] == SYNCHRONIZE_CACHE || cdb[0] == SYNCHRONIZE_CACHE_16;
}

void ps3_scsih_lba_parse(const U8 *cdb, U64 *lba);

void ps3_scsih_len_parse(const U8 *cdb, U32 *len);

S32 ps3_scsih_cdb_opts_parse(struct ps3_cmd *cmd);

void ps3_scsih_cdb_rebuild(U8 *cdb, U16 cdb_len, U32 num_blocks, U32 lba_lo, U32 lba_hi);

static inline Bool ps3_scsih_cdb_is_read_cmd(const U8 *cdb)
{
	U8 type = PS3_SCSI_CMD_TYPE(ps3_scsih_cdb_rw_type_get(cdb));
	return ps3_scsih_is_read_cmd(type);
}

static inline Bool ps3_scsih_cdb_is_write_cmd(const U8 *cdb)
{
	U8 type = PS3_SCSI_CMD_TYPE(ps3_scsih_cdb_rw_type_get(cdb));
	return ps3_scsih_is_write_cmd(type);
}

Bool ps3_scsih_is_protocal_rw(const U8 *cdb);

Bool ps3_scsih_rw_cmd_is_need_split_hba(struct ps3_cmd *cmd);

Bool ps3_scsih_rw_cmd_is_need_split_raid(struct ps3_cmd *cmd);

static inline void ps3_scsih_unmap_desc_parse(const U8 *desc, U32 *num_blocks,
	U32 *lba_lo, U32 *lba_hi)
{
	*lba_lo = ((U32)desc[4] << PS3_SHIFT_3BYTE) |
		((U32)desc[5] << PS3_SHIFT_WORD) |
		((U32) desc[6] << PS3_SHIFT_BYTE) | (U32)desc[7];
	*lba_hi = ((U32)desc[0] << PS3_SHIFT_3BYTE) |
		((U32)desc[1] << PS3_SHIFT_WORD) |
		((U32)desc[2] << PS3_SHIFT_BYTE) | (U32)desc[3];
	*num_blocks = ((U32)desc[8] << PS3_SHIFT_3BYTE) |
		((U32)desc[9] << PS3_SHIFT_WORD) |
		((U32)desc[10] << PS3_SHIFT_BYTE) | (U32)desc[11];
	return;
}

#endif
