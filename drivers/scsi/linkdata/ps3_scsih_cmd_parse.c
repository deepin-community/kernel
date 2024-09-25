
#ifndef _WINDOWS
#include <scsi/scsi_cmnd.h>
#include "ps3_scsih.h"
#include "ps3_module_para.h"
#endif

#include "ps3_scsih_cmd_parse.h"
#include "ps3_err_def.h"
#include "ps3_inner_data.h"
#include "ps3_instance_manager.h"
#include "ps3_driver_log.h"

#define PS3_WRITE_VERIFY_16 (0x8e)
#define PS3_WRITE_VERIFY_32 (0x0C)
#define ORWRITE_16 (0x8B)
#define ORWRITE_32 (0x0E)
#define PRE_FETCH_16 (0x90)
#define READ_BUFFER_16 (0X9B)
#define REPORT_REFERRALS (0x9E)
#define SANITIZE (0x48)
#define WRITE_ATOMIC_16 (0x9C)
#define WRITE_ATOMIC_32 (0x0F)
#define WRITE_SCATTERED_16 (0x12)
#define WRITE_SCATTERED_32 (0x0011)
#define WRITE_STREAM_16 (0x9A)
#define WRITE_STREAM_32 (0x10)
#define WRITE_LONG_16 (0x11)

Bool ps3_scsih_is_rw_type(U8 type)
{
	Bool is_rw_type = PS3_FALSE;

	switch (type) {
	case PS3_SCSI_CMD_TYPE_READ:
	case PS3_SCSI_CMD_TYPE_WRITE:
	case PS3_SCSI_CMD_TYPE_UNMAP:
	case PS3_SCSI_CMD_TYPE_RW:
		is_rw_type = PS3_TRUE;
		break;
	default:
		is_rw_type = PS3_FALSE;
		break;
	}

	return is_rw_type;
}

Bool ps3_scsih_rw_cmd_is_need_split_hba(struct ps3_cmd *cmd)
{
	Bool ret = PS3_FALSE;
	(void)cmd;

	return ret;
}

Bool ps3_scsih_rw_cmd_is_need_split_raid(struct ps3_cmd *cmd)
{
	Bool is_need_split = PS3_FALSE;
	U32 num_blocks = 0;
	U32 lba_lo = 0;
	U32 lba_hi = 0;


	ps3_scsih_cdb_parse(cmd->scmd->cmnd, &num_blocks, &lba_lo, &lba_hi, &is_need_split);

	return is_need_split;
}

static U8 ps3_scsih_service_action32_rw_type_get(const U8 *cdb)
{
	enum ps3_scsi_cmd_type rw_type = PS3_SCSI_CMD_TYPE_UNKOWN;

	U16 cmd_type = PS3_SERVICE_ACTION32(cdb);
	switch (cmd_type) {
	case READ_32:
		rw_type = PS3_SCSI_CMD_TYPE_READ;
		break;

	case WRITE_32:
	case PS3_WRITE_VERIFY_32:
	case ORWRITE_32:
	case WRITE_ATOMIC_32:
		rw_type = (enum ps3_scsi_cmd_type)((U8)PS3_SCSI_CMD_TYPE_WRITE | PS3_SCSI_CONFLICT_CHECK);
		break;
	case VERIFY_32:
	case WRITE_SAME_32:
	case WRITE_STREAM_32:
	case WRITE_SCATTERED_32:
		rw_type = PS3_SCSI_CMD_TYPE_WRITE;
		break;

	default:
		rw_type = PS3_SCSI_CMD_TYPE_NORW;
		break;
	}

	return (U8)rw_type;
}

static inline U8 ps3_service_action16_rw_type_get(const U8 *cdb)
{
	enum ps3_scsi_cmd_type rw_type = PS3_SCSI_CMD_TYPE_UNKOWN;

	U8 cmd_type = cdb[1] & 0x1f;
	switch (cmd_type) {
	case WRITE_LONG_16:
	case WRITE_SCATTERED_16:
		rw_type = PS3_SCSI_CMD_TYPE_WRITE;
		break;
	default:
		rw_type = PS3_SCSI_CMD_TYPE_NORW;
		break;
	}

	return (U8)rw_type;
}

static inline void ps3_scsih_cdb_options_get(const U8 *cdb,
	ps3_scsi_cdb_opts_u *cdb_opts)
{
	ps3_scsi_cdb_opts_u *pRead = (ps3_scsi_cdb_opts_u *)(cdb);

	cdb_opts->fua = pRead->fua;
	cdb_opts->protect = pRead->protect;
	cdb_opts->dpo = pRead->dpo;
}

S32 ps3_scsih_cdb_opts_parse(struct ps3_cmd *cmd)
{
	ps3_scsi_cdb_opts_u *cdb_opts = &cmd->io_attr.cdb_opts;
#ifndef _WINDOWS
	const U8 *cdb = cmd->scmd->cmnd;
#else
	const U8 *cdb = scsi_cmnd_cdb(cmd->scmd);
#endif

	U16 sub_cmd_type = 0;
	S32 ret = PS3_SUCCESS;

	switch (cdb[0]) {
	case READ_10:
	case READ_12:
	case READ_16:
	case WRITE_10:
	case WRITE_12:
	case WRITE_16:
		ps3_scsih_cdb_options_get(&cdb[1], cdb_opts);
		break;
	case VARIABLE_LENGTH_CMD:
		sub_cmd_type = PS3_SERVICE_ACTION32(cdb);
		switch (sub_cmd_type) {
		case READ_32:
		case WRITE_32:
			ps3_scsih_cdb_options_get(&cdb[10], cdb_opts);
			break;
		default:
			ret = -PS3_FAILED;
			break;
		}
		break;
	case WRITE_6:
		cdb_opts->option = 0;
		break;
	case READ_6:
		cdb_opts->option = 0;
		break;
	default:
		ret = -PS3_FAILED;
		break;
	}

	return ret;
}

Bool ps3_scsih_is_protocal_rw(const U8 *cdb)
{
	Bool ret = PS3_DRV_FALSE;
	U16 sub_cmd_type = 0;

	switch (cdb[0]) {
	case READ_6:
	case READ_10:
	case READ_12:
	case READ_16:
	case WRITE_6:
	case WRITE_10:
	case WRITE_12:
	case WRITE_16:
		ret = PS3_DRV_TRUE;
		break;
	case VARIABLE_LENGTH_CMD:
		sub_cmd_type = PS3_SERVICE_ACTION32(cdb);
		switch (sub_cmd_type) {
		case READ_32:
		case WRITE_32:
			ret = PS3_DRV_TRUE;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return ret;
}

U8 ps3_scsih_cdb_rw_type_get(const U8 *cdb)
{
	U8 rw_type = (U8)PS3_SCSI_CMD_TYPE_UNKOWN;

	switch (cdb[0]) {
	case READ_6:
	case READ_10:
	case READ_12:
	case READ_16:
	case PRE_FETCH:
	case PRE_FETCH_16:
		rw_type = (U8)PS3_SCSI_CMD_TYPE_READ;
		break;

	case WRITE_6:
	case WRITE_10:
	case WRITE_12:
	case WRITE_16:
	case WRITE_VERIFY:
	case WRITE_VERIFY_12:
	case ORWRITE_16:
	case WRITE_ATOMIC_16:
	case PS3_WRITE_VERIFY_16:
		rw_type = (U8)PS3_SCSI_CMD_TYPE_WRITE | PS3_SCSI_CONFLICT_CHECK;
		break;
	case VERIFY:
	case WRITE_SAME:
	case VERIFY_12:
	case VERIFY_16:
	case WRITE_SAME_16:
	case SYNCHRONIZE_CACHE:
	case SYNCHRONIZE_CACHE_16:
	case WRITE_STREAM_16:
	case WRITE_LONG:
		rw_type = (U8)PS3_SCSI_CMD_TYPE_WRITE;
		break;

	case VARIABLE_LENGTH_CMD:
		rw_type = ps3_scsih_service_action32_rw_type_get(cdb);
		break;
	case UNMAP:
		rw_type = (U8)PS3_SCSI_CMD_TYPE_UNMAP | PS3_SCSI_CONFLICT_CHECK;
		break;

	case COMPARE_AND_WRITE:
		rw_type = (U8)PS3_SCSI_CMD_TYPE_RW | PS3_SCSI_CONFLICT_CHECK;
		break;

	case SERVICE_ACTION_OUT_16:
		rw_type = ps3_service_action16_rw_type_get(cdb);
		break;

	default:
		rw_type = (U8)PS3_SCSI_CMD_TYPE_NORW;
		break;
	}

	return rw_type;
}

static inline void ps3_scsih_cdb_rw6_rebuild(U8 *cdb, U32 num_blocks,
	U32 lba_lo)
{
	cdb[1] &= ~(0x1f);
	cdb[1] |= (U8) (lba_lo >> PS3_SHIFT_WORD) & 0x1f;
	cdb[2] = (U8) (lba_lo >> PS3_SHIFT_BYTE) & 0xff;
	cdb[3] = (U8) lba_lo & 0xff;

	cdb[4] = (256 == num_blocks ) ? 0 : ((U8) num_blocks & 0xff);
}

static inline void ps3_scsih_cdb_rw10_rebuild(U8 *cdb, U32 num_blocks,
	U32 lba_lo)
{
	cdb[2] = (U8) (lba_lo >> PS3_SHIFT_3BYTE) & 0xff;
	cdb[3] = (U8) (lba_lo >> PS3_SHIFT_WORD) & 0xff;
	cdb[4] = (U8) (lba_lo >> PS3_SHIFT_BYTE) & 0xff;
	cdb[5] = (U8) lba_lo & 0xff;

	cdb[7] = (U8) (num_blocks >> PS3_SHIFT_BYTE) & 0xff;
	cdb[8] = (U8) num_blocks & 0xff;
}

static inline void ps3_scsih_cdb_rw12_rebuild(U8 *cdb, U32 num_blocks,
	U32 lba_lo)
{
	cdb[2] = (U8) (lba_lo >> PS3_SHIFT_3BYTE) & 0xff;
	cdb[3] = (U8) (lba_lo >> PS3_SHIFT_WORD) & 0xff;
	cdb[4] = (U8) (lba_lo >> PS3_SHIFT_BYTE) & 0xff;
	cdb[5] = (U8) lba_lo & 0xff;

	cdb[6] = (U8) (num_blocks >> PS3_SHIFT_3BYTE) & 0xff;
	cdb[7] = (U8) (num_blocks >> PS3_SHIFT_WORD) & 0xff;
	cdb[8] = (U8) (num_blocks >> PS3_SHIFT_BYTE) & 0xff;
	cdb[9] = (U8) num_blocks & 0xff;
}

static inline void ps3_scsih_cdb_rw16_rebuild(U8 *cdb, U32 num_blocks,
	U32 lba_lo, U32 lba_hi)
{
	cdb[2] = (U8) (lba_hi >> PS3_SHIFT_3BYTE) & 0xff;
	cdb[3] = (U8) (lba_hi >> PS3_SHIFT_WORD) & 0xff;
	cdb[4] = (U8) (lba_hi >> PS3_SHIFT_BYTE) & 0xff;
	cdb[5] = (U8) lba_hi & 0xff;

	cdb[6] = (U8) (lba_lo >> PS3_SHIFT_3BYTE) & 0xff;
	cdb[7] = (U8) (lba_lo >> PS3_SHIFT_WORD) & 0xff;
	cdb[8] = (U8) (lba_lo >> PS3_SHIFT_BYTE) & 0xff;
	cdb[9] = (U8) lba_lo & 0xff;

	cdb[10] = (U8) (num_blocks >> PS3_SHIFT_3BYTE) & 0xff;
	cdb[11] = (U8) (num_blocks >> PS3_SHIFT_WORD) & 0xff;
	cdb[12] = (U8) (num_blocks >> PS3_SHIFT_BYTE) & 0xff;
	cdb[13] = (U8) num_blocks & 0xff;
}

static inline void ps3_scsih_cdb_rw32_rebuild(U8 *cdb, U32 num_blocks,
	U32 lba_lo, U32 lba_hi)
{
	U16 cmd_type = PS3_SERVICE_ACTION32(cdb);
	LOG_DEBUG("[ps3]VARIABLE_LENGTH_CMD :0x%x!\n", cmd_type);

	switch (cmd_type) {
	case READ_32:
	case VERIFY_32:
	case WRITE_32:
	case PS3_WRITE_VERIFY_32:
	case WRITE_SAME_32:
	case ORWRITE_32:
	case WRITE_ATOMIC_32:
	case WRITE_STREAM_32:
		cdb[12] = (U8) (lba_hi >> PS3_SHIFT_3BYTE) & 0xff;
		cdb[13] = (U8) (lba_hi >> PS3_SHIFT_WORD) & 0xff;
		cdb[14] = (U8) (lba_hi >> PS3_SHIFT_BYTE) & 0xff;
		cdb[15] = (U8) lba_hi & 0xff;

		cdb[16] = (U8) (lba_lo >> PS3_SHIFT_3BYTE) & 0xff;
		cdb[17] = (U8) (lba_lo >> PS3_SHIFT_WORD) & 0xff;
		cdb[18] = (U8) (lba_lo >> PS3_SHIFT_BYTE) & 0xff;
		cdb[19] = (U8) lba_lo & 0xff;

		cdb[20] = (U8) (lba_lo >> PS3_SHIFT_3BYTE) & 0xff;
		cdb[21] = (U8) (lba_lo >> PS3_SHIFT_WORD) & 0xff;
		cdb[22] = (U8) (lba_lo >> PS3_SHIFT_BYTE) & 0xff;
		cdb[23] = (U8) lba_lo & 0xff;

		cdb[28] = (U8) (num_blocks >> PS3_SHIFT_3BYTE) & 0xff;
		cdb[29] = (U8) (num_blocks >> PS3_SHIFT_WORD) & 0xff;
		cdb[30] = (U8) (num_blocks >> PS3_SHIFT_BYTE) & 0xff;
		cdb[31] = (U8) num_blocks & 0xff;
		break;
	default :
		break;
	}
}
static inline void ps3_scsih_cdb_write_long10_rebuild(U8 *cdb,U32 lba_lo)
{
	cdb[2] = (U8) (lba_lo >> PS3_SHIFT_3BYTE) & 0xff;
	cdb[3] = (U8) (lba_lo >> PS3_SHIFT_WORD) & 0xff;
	cdb[4] = (U8) (lba_lo >> PS3_SHIFT_BYTE) & 0xff;
	cdb[5] = (U8) lba_lo & 0xff;
}

static inline void ps3_cdb_comp_and_write_rebuild(U8 *cdb,
	U32 num_blocks, U32 lba_lo, U32 lba_hi)
{
	cdb[2] = (U8) (lba_hi >> PS3_SHIFT_3BYTE) & 0xff;
	cdb[3] = (U8) (lba_hi >> PS3_SHIFT_WORD) & 0xff;
	cdb[4] = (U8) (lba_hi >> PS3_SHIFT_BYTE) & 0xff;
	cdb[5] = (U8) lba_hi & 0xff;

	cdb[6] = (U8) (lba_lo >> PS3_SHIFT_3BYTE) & 0xff;
	cdb[7] = (U8) (lba_lo >> PS3_SHIFT_WORD) & 0xff;
	cdb[8] = (U8) (lba_lo >> PS3_SHIFT_BYTE) & 0xff;
	cdb[9] = (U8) lba_lo & 0xff;

	cdb[13] = (U8) num_blocks & 0xff;
}

static inline void ps3_scsih_service_16_rebuild(U8 *cdb,
	U32 lba_lo, U32 lba_hi)
{
	U8 cmd_type = cdb[1] & 0x1f;
	LOG_DEBUG("[ps3] CMD :0x%x!\n", cmd_type);

	if (WRITE_LONG_16 == cmd_type) {
		cdb[2] = (U8) (lba_hi >> PS3_SHIFT_3BYTE) & 0xff;
		cdb[3] = (U8) (lba_hi >> PS3_SHIFT_WORD) & 0xff;
		cdb[4] = (U8) (lba_hi >> PS3_SHIFT_BYTE) & 0xff;
		cdb[5] = (U8) lba_hi & 0xff;

		cdb[6] = (U8) (lba_lo >> PS3_SHIFT_3BYTE) & 0xff;
		cdb[7] = (U8) (lba_lo >> PS3_SHIFT_WORD) & 0xff;
		cdb[8] = (U8) (lba_lo >> PS3_SHIFT_BYTE) & 0xff;
		cdb[9] = (U8) lba_lo & 0xff;
	}
}

static inline void ps3_scsih_atomic_stream_16_rebuild(U8 *cdb,
	U32 num_blocks, U32 lba_lo, U32 lba_hi)
{
	cdb[2] = (lba_hi >> PS3_SHIFT_3BYTE) & 0xff;
	cdb[3] = (lba_hi >> PS3_SHIFT_WORD) & 0xff;
	cdb[4] = (lba_hi >> PS3_SHIFT_BYTE) & 0xff;
	cdb[5] = lba_hi & 0xff;

	cdb[6] = (lba_lo >> PS3_SHIFT_3BYTE) & 0xff;
	cdb[7] = (lba_lo >> PS3_SHIFT_WORD) & 0xff;
	cdb[8] = (lba_lo >> PS3_SHIFT_BYTE) & 0xff;
	cdb[9] = lba_lo & 0xff;

	cdb[12] = (num_blocks >> PS3_SHIFT_BYTE) & 0xff;
	cdb[13] = num_blocks & 0xff;
}

static inline void ps3_scsih_cdb_rw6_parse(const U8 *cdb, U32 *num_blocks,
	U32 *lba_lo, U32 *lba_hi)
{
	const U32 default_num_blocks = 256;
	(void)lba_hi;
	*lba_lo = (U32)(((cdb[1] & 0x1f) << PS3_SHIFT_WORD) |
		(cdb[2] << PS3_SHIFT_BYTE) | cdb[3]);
	*num_blocks = ((U32)cdb[4] == 0) ? default_num_blocks : (U32)cdb[4];
	return;
}

static inline void ps3_scsih_cdb_rw10_parse(const U8 *cdb, U32 *num_blocks,
	U32 *lba_lo, U32 *lba_hi)
{
	(void)lba_hi;
	*lba_lo = ((U32)cdb[2] << PS3_SHIFT_3BYTE) |
		((U32)cdb[3] << PS3_SHIFT_WORD) |
		((U32)cdb[4] << PS3_SHIFT_BYTE) | (U32)cdb[5];
	*num_blocks = ((U32)cdb[8] | ((U32)cdb[7] << PS3_SHIFT_BYTE));

	return;
}

static inline void ps3_scsih_cdb_rw12_parse(const U8 *cdb, U32 *num_blocks,
	U32 *lba_lo, U32 *lba_hi)
{
	(void)lba_hi;
	*lba_lo = ((U32)cdb[2] << PS3_SHIFT_3BYTE) |
		((U32)cdb[3] << PS3_SHIFT_WORD) |
		((U32)cdb[4] << PS3_SHIFT_BYTE) | (U32)cdb[5];
	*num_blocks = ((U32)cdb[6] << PS3_SHIFT_3BYTE) |
		((U32)cdb[7] << PS3_SHIFT_WORD) |
		((U32)cdb[8] << PS3_SHIFT_BYTE) | (U32)cdb[9];
	return;
}

static inline void ps3_scsih_cdb_rw16_parse(const U8 *cdb, U32 *num_blocks,
	U32 *lba_lo, U32 *lba_hi)
{
	*lba_lo = ((U32)cdb[6] << PS3_SHIFT_3BYTE) |
		((U32)cdb[7] << PS3_SHIFT_WORD) |
		((U32) cdb[8] << PS3_SHIFT_BYTE) | (U32)cdb[9];
	*lba_hi = ((U32)cdb[2] << PS3_SHIFT_3BYTE) |
		((U32)cdb[3] << PS3_SHIFT_WORD) |
		((U32)cdb[4] << PS3_SHIFT_BYTE) | (U32)cdb[5];
	*num_blocks = ((U32)cdb[10] << PS3_SHIFT_3BYTE) |
		((U32)cdb[11] << PS3_SHIFT_WORD) |
		((U32)cdb[12] << PS3_SHIFT_BYTE) | (U32)cdb[13];
	return;
}

static inline void ps3_scsih_cdb_rw32_parse(const U8 *cdb,
	U32 *num_blocks, U32 *lba_lo, U32 *lba_hi, Bool *is_need_split)
{
	U16 cmd_type = PS3_SERVICE_ACTION32(cdb);
	LOG_DEBUG("[ps3]VARIABLE_LENGTH_CMD :0x%x!\n", cmd_type);
	*is_need_split = PS3_FALSE;

	switch (cmd_type) {
	case READ_32:
	case WRITE_32:
	case PS3_WRITE_VERIFY_32:
		*is_need_split = PS3_TRUE;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
		fallthrough;
#endif
	case VERIFY_32:
	case WRITE_SAME_32:
	case ORWRITE_32:
	case WRITE_ATOMIC_32:
	case WRITE_STREAM_32:
		*lba_lo = ((U32)cdb[16] << PS3_SHIFT_3BYTE) |
			((U32)cdb[17] << PS3_SHIFT_WORD) |
			((U32) cdb[18] << PS3_SHIFT_BYTE) | (U32)cdb[19];
		*lba_hi = ((U32)cdb[12] << PS3_SHIFT_3BYTE) |
			((U32)cdb[13] << PS3_SHIFT_WORD) |
			((U32)cdb[14] << PS3_SHIFT_BYTE) | (U32)cdb[15];
		*num_blocks = ((U32)cdb[28] << PS3_SHIFT_3BYTE) |
			((U32)cdb[29] << PS3_SHIFT_WORD) |
			((U32)cdb[30] << PS3_SHIFT_BYTE) | (U32)cdb[31];
		break;

	case WRITE_SCATTERED_32:
		*lba_lo = 0;
		*num_blocks = 0;
		break;

	default:
		break;
	}

	return;
}

static inline void ps3_cdb_write_long10_parse(const U8 *cdb,
	U32 *num_blocks, U32 *lba_lo, U32 *lba_hi)
{
	(void)lba_hi;
	*lba_lo = ((U32)cdb[2] << PS3_SHIFT_3BYTE) |
		((U32)cdb[3] << PS3_SHIFT_WORD) |
		((U32)cdb[4] << PS3_SHIFT_BYTE) | (U32)cdb[5];
	*num_blocks =1;
}

static inline void ps3_cdb_comp_and_write_parse(const U8 *cdb,
	U32 *num_blocks, U32 *lba_lo, U32 *lba_hi)
{
	*lba_lo = ((U32)cdb[6] << PS3_SHIFT_3BYTE) |
		((U32)cdb[7] << PS3_SHIFT_WORD) |
		((U32) cdb[8] << PS3_SHIFT_BYTE) | (U32)cdb[9];
	*lba_hi = ((U32)cdb[2] << PS3_SHIFT_3BYTE) |
		((U32)cdb[3] << PS3_SHIFT_WORD) |
		((U32)cdb[4] << PS3_SHIFT_BYTE) | (U32)cdb[5];
	*num_blocks = (U32)cdb[13];
}

static inline void ps3_scsih_service_16_parse(const U8 *cdb,
	U32 *num_blocks, U32 *lba_lo, U32 *lba_hi)
{
	U8 cmd_type = cdb[1] & 0x1f;
	LOG_DEBUG("[ps3] CMD :0x%x!\n", cmd_type);

	switch (cmd_type) {
	case WRITE_LONG_16:
		*lba_lo = ((U32)cdb[6] << PS3_SHIFT_3BYTE) |
			((U32)cdb[7] << PS3_SHIFT_WORD) |
			((U32) cdb[8] << PS3_SHIFT_BYTE) | (U32)cdb[9];
		*lba_hi = ((U32)cdb[2] << PS3_SHIFT_3BYTE) |
			((U32)cdb[3] << PS3_SHIFT_WORD) |
			((U32)cdb[4] << PS3_SHIFT_BYTE) | (U32)cdb[5];
		*num_blocks = 1;
		break;

	case WRITE_SCATTERED_16:
		*lba_lo = 0;
		*num_blocks = 0;
		break;

	default:
		break;
	}

	return;
}

void ps3_scsih_cdb_parse(const U8 *cdb, U32 *num_blocks,
	U32 *lba_lo, U32 *lba_hi, Bool *is_need_split)
{
	*num_blocks = 0;
	*lba_lo = 0;
	*lba_hi = 0;
	*is_need_split = PS3_FALSE;

	switch (cdb[0]) {
	case READ_6:
	case WRITE_6:
		ps3_scsih_cdb_rw6_parse(cdb, num_blocks, lba_lo, lba_hi);
		*is_need_split = PS3_TRUE;
		break;

	case READ_10:
	case WRITE_10:
	case WRITE_VERIFY:
		*is_need_split = PS3_TRUE;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
		fallthrough;
#endif
	case VERIFY:
	case WRITE_SAME:
	case PRE_FETCH:
	case SYNCHRONIZE_CACHE:
		ps3_scsih_cdb_rw10_parse(cdb, num_blocks, lba_lo, lba_hi);
		break;

	case READ_12:
	case WRITE_12:
	case WRITE_VERIFY_12:
		*is_need_split = PS3_TRUE;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
		fallthrough;
#endif
	case VERIFY_12:
		ps3_scsih_cdb_rw12_parse(cdb, num_blocks, lba_lo, lba_hi);
		break;

	case READ_16:
	case WRITE_16:
	case PS3_WRITE_VERIFY_16:
		*is_need_split = PS3_TRUE;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
		fallthrough;
#endif
	case VERIFY_16:
	case WRITE_SAME_16:
	case ORWRITE_16:
	case PRE_FETCH_16:
	case SYNCHRONIZE_CACHE_16:
		ps3_scsih_cdb_rw16_parse(cdb, num_blocks, lba_lo, lba_hi);
		break;

	case FORMAT_UNIT:
	case UNMAP:
	case SANITIZE:
		break;

	case VARIABLE_LENGTH_CMD:
		ps3_scsih_cdb_rw32_parse(cdb, num_blocks, lba_lo, lba_hi, is_need_split);
		break;

	case COMPARE_AND_WRITE:
		ps3_cdb_comp_and_write_parse(cdb, num_blocks, lba_lo, lba_hi);
		break;

	case WRITE_ATOMIC_16:
	case WRITE_STREAM_16:
		*lba_lo = ((U32)cdb[6] << PS3_SHIFT_3BYTE) |
			((U32)cdb[7] << PS3_SHIFT_WORD) |
			((U32) cdb[8] << PS3_SHIFT_BYTE) | (U32)cdb[9];
		*lba_hi = ((U32)cdb[2] << PS3_SHIFT_3BYTE) |
			((U32)cdb[3] << PS3_SHIFT_WORD) |
			((U32)cdb[4] << PS3_SHIFT_BYTE) | (U32)cdb[5];
		*num_blocks = ((U32)cdb[12] << PS3_SHIFT_BYTE) | (U32)cdb[13];
		break;

	case WRITE_LONG:
		ps3_cdb_write_long10_parse(cdb, num_blocks, lba_lo, lba_hi);
		break;

	case SERVICE_ACTION_OUT_16:
		ps3_scsih_service_16_parse(cdb, num_blocks, lba_lo, lba_hi);
		break;

	default:
		break;
	}

	return;
}
static inline void ps3_convert_to_cdb16(U8 *cdb, U16 cdb_len, U32 num_blocks, U32 lba_lo, U32 lba_hi)
{
	U8 opcode = 0;
	U8 flagvals = 0;
	U8 groupnum = 0;
	U8 control = 0;

	switch (cdb_len) {
	case 6:
		opcode = cdb[0] == READ_6 ? READ_16 : WRITE_16;
		control = cdb[5];
		break;
	case 10:
		opcode =
			cdb[0] == READ_10 ? READ_16 : WRITE_16;
		flagvals = cdb[1];
		groupnum = cdb[6];
		control = cdb[9];
		break;
	case 12:
		opcode =
			cdb[0] == READ_12 ? READ_16 : WRITE_16;
		flagvals = cdb[1];
		groupnum = cdb[10];
		control = cdb[11];
		break;
	default:
		break;
	}

	cdb[0] = opcode;
	cdb[1] = flagvals;
	cdb[14] = groupnum;
	cdb[15] = control;
	
	cdb[9]	  = (U8)(lba_lo & 0xff);
	cdb[8]	  = (U8)((lba_lo >> PS3_SHIFT_BYTE) & 0xff);
	cdb[7]	  = (U8)((lba_lo >> PS3_SHIFT_WORD) & 0xff);
	cdb[6]	  = (U8)((lba_lo >> PS3_SHIFT_3BYTE) & 0xff);
	cdb[5]	  = (U8)(lba_hi & 0xff);
	cdb[4]	  = (U8)((lba_hi >> PS3_SHIFT_BYTE) & 0xff);
	cdb[3]	  = (U8)((lba_hi >> PS3_SHIFT_WORD) & 0xff);
	cdb[2]	  = (U8)((lba_hi >> PS3_SHIFT_3BYTE) & 0xff);

	cdb[13] = (U8)(num_blocks & 0xff);
	cdb[12] = (U8)((num_blocks >> PS3_SHIFT_BYTE) & 0xff);
	cdb[11] = (U8)((num_blocks >> PS3_SHIFT_WORD) & 0xff);
	cdb[10] = (U8)((num_blocks >> PS3_SHIFT_3BYTE) & 0xff);
}

void ps3_scsih_cdb_rebuild(U8 *cdb, U16 cdb_len, U32 num_blocks,
	U32 lba_lo, U32 lba_hi)
{
	if (unlikely((cdb_len < 16) && (((U64)lba_hi << PS3_SHIFT_DWORD | lba_lo) > 0xffffffff))) {
		ps3_convert_to_cdb16(cdb, cdb_len, num_blocks, lba_lo, lba_hi);
		goto l_out;
	}

	switch (cdb[0]) {
	case READ_6:
	case WRITE_6:
		ps3_scsih_cdb_rw6_rebuild( cdb, num_blocks, lba_lo);
		break;

	case READ_10:
	case WRITE_10:
	case VERIFY:
	case WRITE_VERIFY:
	case WRITE_SAME:
	case PRE_FETCH:
	case SYNCHRONIZE_CACHE:
		ps3_scsih_cdb_rw10_rebuild( cdb, num_blocks, lba_lo);
		break;

	case READ_12:
	case WRITE_12:
	case VERIFY_12:
	case WRITE_VERIFY_12:
		ps3_scsih_cdb_rw12_rebuild( cdb, num_blocks, lba_lo);
		break;

	case READ_16:
	case WRITE_16:
	case VERIFY_16:
	case PS3_WRITE_VERIFY_16:
	case WRITE_SAME_16:
	case ORWRITE_16:
	case PRE_FETCH_16:
	case SYNCHRONIZE_CACHE_16:
		ps3_scsih_cdb_rw16_rebuild( cdb, num_blocks, lba_lo, lba_hi);
		break;

	case VARIABLE_LENGTH_CMD:
		ps3_scsih_cdb_rw32_rebuild(cdb, num_blocks, lba_lo, lba_hi);
		break;

	case COMPARE_AND_WRITE:
		ps3_cdb_comp_and_write_rebuild(cdb, num_blocks, lba_lo, lba_hi);
		break;

	case WRITE_ATOMIC_16:
	case WRITE_STREAM_16:
		ps3_scsih_atomic_stream_16_rebuild(cdb, num_blocks, lba_lo, lba_hi);
		break;

	case WRITE_LONG:
		ps3_scsih_cdb_write_long10_rebuild(cdb, lba_lo);
		break;

	case SERVICE_ACTION_OUT_16:
		ps3_scsih_service_16_rebuild(cdb, lba_lo, lba_hi);
		break;

	default:
		break;
	}
l_out:
	return;
}

void ps3_scsih_lba_parse(const U8 *cdb, U64 *lba)
{
	U32 num_blocks = 0;
	U32 lba_lo = 0;
	U32 lba_hi = 0;
	Bool is_need_split = PS3_FALSE;

	ps3_scsih_cdb_parse(cdb, &num_blocks, &lba_lo, &lba_hi, &is_need_split);
	*lba = ((U64)lba_hi << PS3_SHIFT_DWORD) | lba_lo;
}

void ps3_scsih_len_parse(const U8 *cdb, U32 *len)
{
	U32 num_blocks = 0;
	U32 lba_lo = 0;
	U32 lba_hi = 0;
	Bool is_need_split = PS3_FALSE;

	ps3_scsih_cdb_parse(cdb, &num_blocks, &lba_lo, &lba_hi, &is_need_split);
	*len = num_blocks;
}

