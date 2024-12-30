
#ifndef  _WINDOWS
#include <linux/compiler.h>
#include <scsi/scsi_cmnd.h>
#else
#include "ps3_def.h"
#endif

#include "ps3_scsih_raid_engine.h"
#include "ps3_instance_manager.h"
#include "ps3_meta.h"
#include "ps3_scsih_cmd_parse.h"
#include "ps3_driver_log.h"
#include "ps3_util.h"
#include "ps3_module_para.h"
#include "ps3_scsih.h"

#define LBA_TO_STRIPE_INDEX(u64_lba, u32_stripe_data_size, u64_stripe_idx) \
	(u64_stripe_idx) = (u64_lba) / (u32_stripe_data_size)

#define LBA_TO_STRIPE_OFFSET(u64_lba, u32_stripe_data_size, u32_stripe_offset) \
	(u32_stripe_offset) = (u64_lba) % (u32_stripe_data_size)

#define SPANNO_DISKIDX_TO_PHYDISKID(vd_entry, span_idx, span_disk_idx) \
	(vd_entry)->span[(span_idx)].extent[(span_disk_idx)].phyDiskID.ps3Dev.phyDiskID

#define VD_SPAN_PD_NUM(vd_entry, span_idx) \
	(vd_entry)->span[(span_idx)].spanPdNum

#define STRIP_SIZE_MASK(strip_size) ((strip_size) - 1)

#define RAID660_GET_PQ_SPINDLENO(stripe_idx, phy_disk_count, q_disk_idx)  \
do {                                                                      \
	if (ps3_is_power_of_2(phy_disk_count)){                               \
		(q_disk_idx) = (stripe_idx) & ((phy_disk_count) - 1);             \
	} else {                                                              \
		(q_disk_idx) = (stripe_idx) % (phy_disk_count);                   \
	}                                                                     \
	(q_disk_idx) = (phy_disk_count) - (q_disk_idx) - 1;                   \
} while (0);

#define PS3_R1X_HDD_MAX_SWAP_CNT_1 (64)
#define PS3_R1X_HDD_MAX_SWAP_CNT_2 (32)
#define PS3_R1X_SSD_MAX_SWAP_CNT_1 (32)
#define PS3_R1X_SSD_MAX_SWAP_CNT_2 (16)
#define PS3_R1X_SWAP_VD_MEMBER_CNT (8)
#define PS3_R1X_RB_DIFF_CMDS_DEFAULT (4)
#define ABS_DIFF(a, b)   (((a) > (b)) ? ((a) - (b)) : ((b) - (a)))
#define PS3_R1X_RB_INFO_INDEX(span, span_disk_idx) \
		((span) * PS3_MAX_PD_COUNT_IN_SPAN + (span_disk_idx) + 1)
U32 g_ps3_r1x_rb_diff_cmds = PS3_R1X_RB_DIFF_CMDS_DEFAULT;

static inline Bool ps3_is_power_of_2(U32 n)
{
    return (Bool)(n != 0 && ((n & (n - 1)) == 0));
}

static inline Bool ps3_vd_entry_valid_check(const struct PS3VDEntry *vd_entry)
{
	Bool ret = PS3_DRV_TRUE;

	if (unlikely(vd_entry == NULL)) {
		ret = PS3_DRV_FALSE;
		goto l_out;
	}

	if (unlikely(!ps3_is_power_of_2(vd_entry->stripSize))) {
		ret = PS3_DRV_FALSE;
		goto l_out;
	}

	if (unlikely(!vd_entry->stripeDataSize)) {
		ret = PS3_DRV_FALSE;
		goto l_out;
	}
l_out:
	return ret;
}

Bool ps3_scsih_is_same_strip(const struct PS3VDEntry *vd_entry, U32 vlba_lo, U32 lba_length)
{
	if (unlikely(!ps3_vd_entry_valid_check(vd_entry))) {
		return PS3_DRV_FALSE;
	}

	if ( (vd_entry->stripSize - (vlba_lo & STRIP_SIZE_MASK(vd_entry->stripSize)) )
		>= lba_length ) {
		return PS3_DRV_TRUE;
	} else	{
		return PS3_DRV_FALSE;
	}
}

static void ps3_r0_convert_vlba_to_pd_attr(struct ps3_cmd *cmd)
{
	U32 stripe_offset = 0;	
	U64 stripe_idx = 0;	
	U32 strip_size_shift = 0; 
	U32 span_data_disk_idx = 0;	
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;

	LBA_TO_STRIPE_INDEX(vlba, vd_entry->stripeDataSize, stripe_idx);
	LBA_TO_STRIPE_OFFSET(vlba, vd_entry->stripeDataSize, stripe_offset);

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	span_data_disk_idx = stripe_offset >> strip_size_shift;

	cmd->io_attr.plba = vd_entry->startLBA +
		(stripe_idx << strip_size_shift) +
		(stripe_offset & STRIP_SIZE_MASK(vd_entry->stripSize));

	cmd->io_attr.span_idx = 0;
	cmd->io_attr.span_pd_idx = (U8)span_data_disk_idx;
}

static void ps3_r1x_rand_read_target_pd_calc(struct ps3_cmd *cmd, U32 span_idx, 
			U32 span_pd_idx, U32 span_pd_back_idx)
{
	struct ps3_r1x_read_balance_info *rb_info = NULL;
	struct ps3_scsi_priv_data *priv_data = NULL;
	U64 vlba = 0;
	U32 outstanding0 = 0;
	U32 outstanding1 = 0;
	U64 diff0 = 0;
	U64 diff1 = 0;
	U32 pd0 = 0;
	U32 pd1 = 0;
	U32 target_pd = 0;

	if (!cmd->r1x_read_pd) {
		priv_data = scsi_device_private_data(cmd->scmd);
		rb_info = priv_data->r1x_rb_info;
		vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
		pd0 = PS3_R1X_RB_INFO_INDEX(span_idx, span_pd_idx);
		pd1 = PS3_R1X_RB_INFO_INDEX(span_idx, span_pd_back_idx);
		outstanding0 = ps3_atomic_read(&rb_info->scsi_outstanding_cmds[pd0]);
		outstanding1 = ps3_atomic_read(&rb_info->scsi_outstanding_cmds[pd1]);

		if (outstanding0 > outstanding1 + g_ps3_r1x_rb_diff_cmds) {
			target_pd = pd1;
		} else if (outstanding1 > outstanding0 + g_ps3_r1x_rb_diff_cmds) {
			target_pd = pd0;
		} else {
			diff0 = ABS_DIFF(vlba, rb_info->last_accessed_block[pd0]);
			diff1 = ABS_DIFF(vlba, rb_info->last_accessed_block[pd1]);
			target_pd = (diff0 <= diff1 ? pd0 : pd1);
		}

		if (target_pd == pd0) {
			cmd->io_attr.span_pd_idx = (U8)span_pd_idx;
			cmd->io_attr.span_pd_idx_p = (U8)span_pd_back_idx;
		} else {
			cmd->io_attr.span_pd_idx = (U8)span_pd_back_idx;
			cmd->io_attr.span_pd_idx_p = (U8)span_pd_idx;
		}	

		cmd->r1x_read_pd = target_pd;
		rb_info->last_accessed_block[target_pd] = vlba + cmd->io_attr.num_blocks;
		ps3_atomic_inc(&rb_info->scsi_outstanding_cmds[target_pd]);
	}
}

static void ps3_r1x_seq_read_target_pd_calc(struct ps3_cmd *cmd, U32 span_idx, 
			U32 span_pd_idx, U32 span_pd_back_idx)
{
	struct ps3_r1x_read_balance_info *rb_info = NULL;
	struct ps3_scsi_priv_data *priv_data = NULL;
	U64 vlba = 0;
	U32 outstanding0 = 0;
	U32 outstanding1 = 0;
	U64 diff0 = 0;
	U64 diff1 = 0;
	U32 pd0 = 0;
	U32 pd1 = 0;
	U32 target_pd = 0;

	if (!cmd->r1x_read_pd) {
		priv_data = scsi_device_private_data(cmd->scmd);
		rb_info = priv_data->r1x_rb_info;
		vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
		pd0 = PS3_R1X_RB_INFO_INDEX(span_idx, span_pd_idx);
		pd1 = PS3_R1X_RB_INFO_INDEX(span_idx, span_pd_back_idx);
		outstanding0 = ps3_atomic_read(&rb_info->scsi_outstanding_cmds[pd0]);
		outstanding1 = ps3_atomic_read(&rb_info->scsi_outstanding_cmds[pd1]);

		if (outstanding0 == 0 &&  outstanding1 != 0) {
			target_pd = pd0;
		} else if (outstanding0 != 0 && outstanding1 == 0) {
			target_pd = pd1;
		} else {
			diff0 = ABS_DIFF(vlba, rb_info->last_accessed_block[pd0]);
			diff1 = ABS_DIFF(vlba, rb_info->last_accessed_block[pd1]);
			target_pd = (diff0 <= diff1 ? pd0 : pd1);
		}

		if (target_pd == pd0) {
			cmd->io_attr.span_pd_idx = (U8)span_pd_idx;
			cmd->io_attr.span_pd_idx_p = (U8)span_pd_back_idx;
		} else {
			cmd->io_attr.span_pd_idx = (U8)span_pd_back_idx;
			cmd->io_attr.span_pd_idx_p = (U8)span_pd_idx;
		}	

		cmd->r1x_read_pd = target_pd;
		rb_info->last_accessed_block[target_pd] = vlba + cmd->io_attr.num_blocks;
		ps3_atomic_inc(&rb_info->scsi_outstanding_cmds[target_pd]);
	}
}

static void ps3_r1_convert_vlba_to_pd_attr(struct ps3_cmd *cmd)
{
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
	cmd->io_attr.plba = vd_entry->startLBA + vlba;
	cmd->io_attr.span_idx = 0;
	cmd->io_attr.plba_back = cmd->io_attr.plba;

	if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
		if (cmd->io_attr.seq_flag == SCSI_RW_RANDOM_CMD) { 
 			ps3_r1x_rand_read_target_pd_calc(cmd, 0, 0, 1);
		} else {
			ps3_r1x_seq_read_target_pd_calc(cmd, 0, 0, 1);
		}
	} else {
		cmd->io_attr.span_pd_idx = 0;
		cmd->io_attr.span_pd_idx_p = 1;
	}

	LOG_DEBUG("hno:%u chl:%u id:%u rw=%u, span_pd_idx=%u, span_pd_idx_p=%u\n",
			PS3_HOST(cmd->instance), cmd->scmd->device->channel, cmd->scmd->device->id,
			cmd->io_attr.rw_flag, cmd->io_attr.span_pd_idx, cmd->io_attr.span_pd_idx_p);
}

static void ps3_r5_convert_vlba_to_pd_attr(struct ps3_cmd *cmd)
{
	U32 stripe_offset = 0;	
	U64 stripe_idx = 0;	
	U32 strip_size_shift = 0; 
	U32 span_data_disk_idx = 0;	
	U32 span_parity_disk_idx = 0;	
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);

	LBA_TO_STRIPE_INDEX(vlba, vd_entry->stripeDataSize, stripe_idx);
	LBA_TO_STRIPE_OFFSET(vlba, vd_entry->stripeDataSize, stripe_offset);

	if (ps3_is_power_of_2(vd_entry->physDrvCnt)) {
		span_parity_disk_idx = stripe_idx & (vd_entry->physDrvCnt - 1);
	} else {
		span_parity_disk_idx = stripe_idx % vd_entry->physDrvCnt;
	}
	span_parity_disk_idx = vd_entry->physDrvCnt - span_parity_disk_idx - 1;

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	span_data_disk_idx = stripe_offset >> strip_size_shift;
	span_data_disk_idx = span_parity_disk_idx + span_data_disk_idx + 1;
	if ( span_data_disk_idx >= vd_entry->physDrvCnt) {
		span_data_disk_idx -= vd_entry->physDrvCnt;
	}

	cmd->io_attr.plba = vd_entry->startLBA +
		(stripe_idx << strip_size_shift) +
		(stripe_offset & STRIP_SIZE_MASK(vd_entry->stripSize));

	cmd->io_attr.span_idx = 0;
	cmd->io_attr.span_pd_idx = (U8)span_data_disk_idx;
	cmd->io_attr.span_pd_idx_p = (U8)span_parity_disk_idx;
	LOG_DEBUG("vlba:0x%llx plba:0x%llx startLBA:0x%llx stripe_idx:%llu"
		"strip_size_shift:%u stripe_offset:%u span_pd_indx:%u\n",
		vlba, cmd->io_attr.plba, vd_entry->startLBA, stripe_idx,
		strip_size_shift, stripe_offset, cmd->io_attr.span_pd_idx);
}

static void ps3_r6_convert_vlba_to_pd_attr(struct ps3_cmd *cmd)
{
	U32 stripe_offset = 0;	
	U64 stripe_idx = 0;	
	U32 strip_size_shift = 0; 
	U32 span_data_disk_idx = 0;	
	U32 span_q_disk_idx = 0;	
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);

	LBA_TO_STRIPE_INDEX(vlba, vd_entry->stripeDataSize, stripe_idx);
	LBA_TO_STRIPE_OFFSET(vlba, vd_entry->stripeDataSize, stripe_offset);

	RAID660_GET_PQ_SPINDLENO(stripe_idx, vd_entry->physDrvCnt, span_q_disk_idx);

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	span_data_disk_idx = stripe_offset >> strip_size_shift;
	span_data_disk_idx = span_data_disk_idx + span_q_disk_idx + 1;
	if ( span_data_disk_idx >= vd_entry->physDrvCnt) {
		span_data_disk_idx -= vd_entry->physDrvCnt;
	}

	cmd->io_attr.plba = vd_entry->startLBA +
		(stripe_idx << strip_size_shift) +
		(stripe_offset & STRIP_SIZE_MASK(vd_entry->stripSize));

	cmd->io_attr.span_idx = 0;
	cmd->io_attr.span_pd_idx = (U8)span_data_disk_idx;
	cmd->io_attr.span_pd_idx_q = (U8)span_q_disk_idx;
	if(0 != span_q_disk_idx){
		cmd->io_attr.span_pd_idx_p = span_q_disk_idx - 1;
	} else{
		cmd->io_attr.span_pd_idx_p = vd_entry->physDrvCnt - 1;
	}
}

static void ps3_r10_convert_vlba_to_pd_attr(struct ps3_cmd *cmd)
{
	U32 stripe_offset = 0;	
	U64 stripe_idx = 0;	
	U32 strip_size_shift = 0; 
	U32 span_data_disk_idx = 0;	
	U32 span_data_disk_back_idx = 0;	
	U32 span_idx = 0;
	U8  span_pd_num = 0;
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);

	LBA_TO_STRIPE_INDEX(vlba, vd_entry->stripeDataSize, stripe_idx);
	LBA_TO_STRIPE_OFFSET(vlba, vd_entry->stripeDataSize, stripe_offset);

	while(stripe_offset >= vd_entry->span[span_idx].spanStripeDataSize) {
		stripe_offset -= vd_entry->span[span_idx].spanStripeDataSize;
		span_idx++;
	}

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	span_data_disk_idx = stripe_offset >> strip_size_shift;
	span_data_disk_idx = span_data_disk_idx << 1;
	span_pd_num = VD_SPAN_PD_NUM(vd_entry, span_idx);
	if ( span_data_disk_idx > span_pd_num ) { 
		span_data_disk_idx -= span_pd_num;
		span_data_disk_back_idx =
			((span_data_disk_idx+1) == span_pd_num)?
			0 :(span_data_disk_idx+1);
		stripe_idx = (stripe_idx << 1) + 1;
	} else {
		if (span_pd_num & 1) { 
			stripe_idx = stripe_idx << 1;
			span_data_disk_back_idx =
				((span_data_disk_idx+1) == span_pd_num)?
				0 :(span_data_disk_idx+1);
		} else {
			span_data_disk_back_idx = span_data_disk_idx + 1;
		}
	}

	cmd->io_attr.plba = vd_entry->startLBA +
		(stripe_idx << strip_size_shift) +
		(stripe_offset & STRIP_SIZE_MASK(vd_entry->stripSize));
	cmd->io_attr.plba_back = cmd->io_attr.plba;
	if (span_data_disk_back_idx == 0) {
		cmd->io_attr.plba_back += vd_entry->stripSize;
	}
	cmd->io_attr.span_idx = (U8)span_idx;

	if (!(span_pd_num & 1) && ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
		if (cmd->io_attr.seq_flag == SCSI_RW_RANDOM_CMD) { 
			ps3_r1x_rand_read_target_pd_calc(cmd, span_idx, 
					span_data_disk_idx, span_data_disk_back_idx);
		} else {
			ps3_r1x_seq_read_target_pd_calc(cmd, span_idx, 
					span_data_disk_idx, span_data_disk_back_idx);
		}
	} else {
		cmd->io_attr.span_pd_idx = (U8)span_data_disk_idx;
		cmd->io_attr.span_pd_idx_p = (U8)span_data_disk_back_idx;
	}

	LOG_DEBUG("hno:%u tid:0x%llx chl:%u id:%u "
		"odd physDrvCnt:%d rw=%u span_pd_idx=%u-plba:0x%llx span_pd_idx_p=%u-bplba:0x%llx stripe_offset=0x%x\n",
		PS3_HOST(cmd->instance), cmd->trace_id,
		cmd->scmd->device->channel, cmd->scmd->device->id,
		vd_entry->physDrvCnt & 1,
		cmd->io_attr.rw_flag, cmd->io_attr.span_pd_idx,
		cmd->io_attr.plba, cmd->io_attr.span_pd_idx_p,
		cmd->io_attr.plba_back, stripe_offset);
}

static void ps3_r1e_convert_vlba_to_pd_attr(struct ps3_cmd *cmd)
{
	U32 stripe_offset = 0;	
	U64 stripe_idx = 0;	
	U32 strip_size_shift = 0; 
	U32 span_data_disk_idx = 0;	
	U32 span_data_disk_back_idx = 0;	
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);

	LBA_TO_STRIPE_INDEX(vlba, vd_entry->stripeDataSize, stripe_idx);
	LBA_TO_STRIPE_OFFSET(vlba, vd_entry->stripeDataSize, stripe_offset);

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);

	span_data_disk_idx = stripe_offset >> strip_size_shift;
	span_data_disk_idx = span_data_disk_idx << 1;
	if ( span_data_disk_idx > vd_entry->physDrvCnt ) {
		span_data_disk_idx -= vd_entry->physDrvCnt;
		span_data_disk_back_idx =
			((span_data_disk_idx+1) == vd_entry->physDrvCnt)?
			0 :(span_data_disk_idx+1) ;
		stripe_idx = (stripe_idx << 1) + 1;
	} else {
		if (vd_entry->physDrvCnt & 1) {
			stripe_idx = stripe_idx << 1;
			span_data_disk_back_idx =
				((span_data_disk_idx+1) == vd_entry->physDrvCnt)?
				0 :(span_data_disk_idx+1) ;
		} else {
			span_data_disk_back_idx = span_data_disk_idx + 1;
		}
	}

	cmd->io_attr.plba = vd_entry->startLBA +
		(stripe_idx << strip_size_shift) +
		(stripe_offset & STRIP_SIZE_MASK(vd_entry->stripSize));
	cmd->io_attr.plba_back = cmd->io_attr.plba;
	if (span_data_disk_back_idx == 0) {
		cmd->io_attr.plba_back += vd_entry->stripSize;
	}
	cmd->io_attr.span_idx = 0;

	if (!(vd_entry->physDrvCnt & 1) && ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
		if (cmd->io_attr.seq_flag == SCSI_RW_RANDOM_CMD) { 
			ps3_r1x_rand_read_target_pd_calc(cmd, 0, span_data_disk_idx, span_data_disk_back_idx);
		} else {
			ps3_r1x_seq_read_target_pd_calc(cmd, 0, span_data_disk_idx, span_data_disk_back_idx);
		}
	} else {
		cmd->io_attr.span_pd_idx = (U8)span_data_disk_idx;
		cmd->io_attr.span_pd_idx_p = (U8)span_data_disk_back_idx;
	}

	LOG_DEBUG("hno:%u chl:%u id:%u "
		"odd physDrvCnt:%d rw=%u, span_pd_idx=%u, span_pd_idx_p=%u\n",
		PS3_HOST(cmd->instance), cmd->scmd->device->channel, cmd->scmd->device->id,
		vd_entry->physDrvCnt & 1, cmd->io_attr.rw_flag,
		cmd->io_attr.span_pd_idx, cmd->io_attr.span_pd_idx_p);
}

static void ps3_r00_convert_vlba_to_pd_attr(struct ps3_cmd *cmd)
{
	U32 stripe_offset = 0;	
	U64 stripe_idx = 0;	
	U32 strip_size_shift = 0; 
	U32 span_data_disk_idx = 0;	
	U32 span_idx = 0;
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);

	LBA_TO_STRIPE_INDEX(vlba, vd_entry->stripeDataSize, stripe_idx);
	LBA_TO_STRIPE_OFFSET(vlba, vd_entry->stripeDataSize, stripe_offset);

	while (stripe_offset >= vd_entry->span[span_idx].spanStripeDataSize) {
		stripe_offset -= vd_entry->span[span_idx].spanStripeDataSize;
		span_idx++;
	}

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	span_data_disk_idx = stripe_offset >> strip_size_shift;

	cmd->io_attr.plba = vd_entry->startLBA +
		(stripe_idx << strip_size_shift) +
		(stripe_offset & STRIP_SIZE_MASK(vd_entry->stripSize));
	cmd->io_attr.span_idx = (U8)span_idx;
	cmd->io_attr.span_pd_idx = (U8)span_data_disk_idx;
}

static void ps3_r50_convert_vlba_to_pd_attr(struct ps3_cmd *cmd)
{
	U32 stripe_offset = 0;	
	U64 stripe_idx = 0;	
	U32 strip_size_shift = 0; 
	U32 span_data_disk_idx = 0;	
	U32 span_parity_disk_idx = 0;	
	U32 span_idx = 0;
	U8  span_pd_num = 0;
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);

	LBA_TO_STRIPE_INDEX(vlba, vd_entry->stripeDataSize, stripe_idx);
	LBA_TO_STRIPE_OFFSET(vlba, vd_entry->stripeDataSize, stripe_offset);

	while (stripe_offset >= vd_entry->span[span_idx].spanStripeDataSize) {
		stripe_offset -= vd_entry->span[span_idx].spanStripeDataSize;
		span_idx++;
	}

	span_pd_num = VD_SPAN_PD_NUM(vd_entry, span_idx);
	if (ps3_is_power_of_2(span_pd_num)) {
		span_parity_disk_idx = stripe_idx & (span_pd_num - 1);
	} else {
		span_parity_disk_idx = stripe_idx % span_pd_num;
	}
	span_parity_disk_idx = span_pd_num - span_parity_disk_idx - 1;

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	span_data_disk_idx = stripe_offset >> strip_size_shift;
	span_data_disk_idx = span_parity_disk_idx + span_data_disk_idx + 1;
	if ( span_data_disk_idx >= span_pd_num) {
		span_data_disk_idx -= span_pd_num;
	}

	cmd->io_attr.plba = vd_entry->startLBA +
		(stripe_idx << strip_size_shift) +
		(stripe_offset & STRIP_SIZE_MASK(vd_entry->stripSize));
	cmd->io_attr.span_idx = (U8)span_idx;
	cmd->io_attr.span_pd_idx = (U8)span_data_disk_idx;
	cmd->io_attr.span_pd_idx_p = (U8)span_parity_disk_idx;
}

static void ps3_r60_convert_vlba_to_pd_attr(struct ps3_cmd *cmd)
{
	U32 stripe_offset = 0;	
	U64 stripe_idx = 0;	
	U32 strip_size_shift = 0; 
	U32 span_data_disk_idx = 0;	
	U32 span_q_disk_idx = 0;	
	U32 span_idx = 0;
	U8  span_pd_num = 0;
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);

	LBA_TO_STRIPE_INDEX(vlba, vd_entry->stripeDataSize, stripe_idx);
	LBA_TO_STRIPE_OFFSET(vlba, vd_entry->stripeDataSize, stripe_offset);

	while (stripe_offset >= vd_entry->span[span_idx].spanStripeDataSize) {
		stripe_offset -= vd_entry->span[span_idx].spanStripeDataSize;
		span_idx++;
	}

	span_pd_num = VD_SPAN_PD_NUM(vd_entry, span_idx);
	RAID660_GET_PQ_SPINDLENO(stripe_idx, span_pd_num, span_q_disk_idx);

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	span_data_disk_idx = stripe_offset >> strip_size_shift;
	span_data_disk_idx = span_data_disk_idx + span_q_disk_idx + 1;
	if ( span_data_disk_idx >= span_pd_num) {
		span_data_disk_idx -= span_pd_num;
	}

	cmd->io_attr.plba = vd_entry->startLBA +
		(stripe_idx << strip_size_shift) +
		(stripe_offset & STRIP_SIZE_MASK(vd_entry->stripSize));
	cmd->io_attr.span_idx = (U8)span_idx;
	cmd->io_attr.span_pd_idx = (U8)span_data_disk_idx;
	cmd->io_attr.span_pd_idx_q = (U8)span_q_disk_idx;
	if(0 != span_q_disk_idx){
		cmd->io_attr.span_pd_idx_p = span_q_disk_idx - 1;
	} else{
		cmd->io_attr.span_pd_idx_p = span_pd_num - 1;
	}
}

static S32 ps3_convert_to_pd_info(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	const struct PS3Extent *extent = &(vd_entry->span[cmd->io_attr.span_idx].
		extent[cmd->io_attr.span_pd_idx]);

	if (extent->state != MIC_PD_STATE_ONLINE) {
		LOG_DEBUG("cmd :%u direct check pd:%u state:%s != ONLINE\n",
			cmd->index, extent->phyDiskID.ps3Dev.phyDiskID,
			getPdStateName((MicPdState_e)extent->state, cmd->instance->is_raid));
		ret = -PS3_ENODEV;
		goto l_out;
	}

	cmd->io_attr.pd_entry = ps3_dev_mgr_lookup_pd_info_by_id(cmd->instance,
		extent->phyDiskID.ps3Dev.phyDiskID);
	if (unlikely(cmd->io_attr.pd_entry == NULL)) {
		LOG_FILE_ERROR("host_no:%u trace_id:0x%llx idspan_id:%d "
			"span_pd_idx:%d pd[%u:%u:%u], pd_entry == NULL\n",
			PS3_HOST(cmd->instance),
			cmd->trace_id,
			cmd->io_attr.span_idx,
			cmd->io_attr.span_pd_idx,
			extent->phyDiskID.ps3Dev.softChan,
			extent->phyDiskID.ps3Dev.devID,
			extent->phyDiskID.ps3Dev.phyDiskID);
		ret = -PS3_ENODEV;
		goto l_out;
	}

	if (!ps3_is_r1x_write_cmd(cmd)) {
		goto l_out;
	}

	extent = &(vd_entry->span[cmd->io_attr.span_idx].
		extent[cmd->io_attr.span_pd_idx_p]);

	if (extent->state != MIC_PD_STATE_ONLINE) {
		LOG_DEBUG("cmd :%u direct check pd:%u state:%s != ONLINE\n",
			cmd->index, extent->phyDiskID.ps3Dev.phyDiskID,
			getPdStateName((MicPdState_e)extent->state, cmd->instance->is_raid));
		ret = -PS3_ENODEV;
		goto l_out;
	}

	cmd->io_attr.peer_pd_entry = ps3_dev_mgr_lookup_pd_info_by_id(
		cmd->instance, extent->phyDiskID.ps3Dev.phyDiskID);
	if (unlikely(cmd->io_attr.peer_pd_entry == NULL)) {
		LOG_ERROR_LIM("host_no:%u trace_id:0x%llx idspan_id:%d "
			"span_pd_idx:%d peer_pd[%u:%u:%u], pd_entry == NULL\n",
			PS3_HOST(cmd->instance),
			cmd->trace_id,
			cmd->io_attr.span_idx,
			cmd->io_attr.span_pd_idx,
			extent->phyDiskID.ps3Dev.softChan,
			extent->phyDiskID.ps3Dev.devID,
			extent->phyDiskID.ps3Dev.phyDiskID);
		ret = -PS3_ENODEV;
		goto l_out;
	}

l_out:
	return ret;
}

S32 ps3_scsih_vlba_to_pd_calc(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;

	if ( unlikely(!ps3_vd_entry_valid_check(cmd->io_attr.vd_entry)) ) {
		ret = -PS3_FAILED;
		LOG_ERROR_LIM("trace_id:0x%llx host_no:%u vd entry is invalid\n",
			 cmd->trace_id, PS3_HOST(cmd->instance));
		goto l_out;
	}

	switch(cmd->io_attr.vd_entry->raidLevel) {
	case RAID0:
		ps3_r0_convert_vlba_to_pd_attr(cmd);
		break;
	case RAID1:
		ps3_r1_convert_vlba_to_pd_attr(cmd);
		break;
	case RAID5:
		ps3_r5_convert_vlba_to_pd_attr(cmd);
		break;
	case RAID6:
		ps3_r6_convert_vlba_to_pd_attr(cmd);
		break;
	case RAID10:
		ps3_r10_convert_vlba_to_pd_attr(cmd);
		break;
	case RAID1E:
		ps3_r1e_convert_vlba_to_pd_attr(cmd);
		break;
	case RAID00:
		ps3_r00_convert_vlba_to_pd_attr(cmd);
		break;
	case RAID50:
		ps3_r50_convert_vlba_to_pd_attr(cmd);
		break;
	case RAID60:
		ps3_r60_convert_vlba_to_pd_attr(cmd);
		break;
	default:
		ret = -PS3_FAILED;
		LOG_ERROR_LIM("trace_id:0x%llx host_no:%u vd level:%d is illegal\n",
			 cmd->trace_id, PS3_HOST(cmd->instance),
			cmd->io_attr.vd_entry->raidLevel);
		goto l_out;
	}

l_out:
	return ret;
}

Bool ps3_scsih_vd_acc_att_build(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	PS3VDAccAttr_s *acc_attr = &cmd->req_frame->frontendReq.vdAccAttr;
	static unsigned long j;
	ret = ps3_scsih_vlba_to_pd_calc(cmd);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "trace_id:0x%llx host_no:%u vlba calc NOK\n",
			 cmd->trace_id, PS3_HOST(cmd->instance));
		acc_attr->isAccActive = 0;
		goto l_out;
	}

	acc_attr->isAccActive = 1;
	acc_attr->firstPdStartLba = cmd->io_attr.plba;
	acc_attr->firstSpanNo = cmd->io_attr.span_idx;
	acc_attr->fisrtSeqInSpan = cmd->io_attr.span_pd_idx;
	acc_attr->secondSeqInSapn = cmd->io_attr.span_pd_idx_p;
	acc_attr->thirdSeqInSapn = cmd->io_attr.span_pd_idx_q;
l_out:
	return (acc_attr->isAccActive == 1);
}

S32 ps3_scsih_vd_rw_io_to_pd_calc(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;

	ret = ps3_scsih_vlba_to_pd_calc(cmd);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR_LIM("trace_id:0x%llx host_no:%u vlba calc NOK\n",
			 cmd->trace_id, PS3_HOST(cmd->instance));
		goto l_out;
	}

	ret = ps3_convert_to_pd_info(cmd);

l_out:
	return ret;
}

static void ps3_update_cmd_target_pd(struct ps3_cmd *cmd, U16 disk_id)
{
	U16 i = 0;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	struct ps3_instance *instance = cmd->instance;
	if (unlikely(cmd->target_pd_count >= PS3_QOS_MAX_PD_IN_VD)) {
		LOG_INFO("host_no:%u CFID:%u did:%u target pd count %u check NOK\n",
			 PS3_HOST(instance), cmd->index, disk_id, cmd->target_pd_count);
		goto l_out;
	}
	if (disk_id > instance->ctrl_info.maxPdCount ) {
		LOG_DEBUG("disk_id is error.host_no:%u CFID:%u did:%u\n",
			 PS3_HOST(instance), cmd->index, disk_id);
		goto l_out;
	}

	qos_pd_mgr = &instance->qos_context.pd_ctx.qos_pd_mgrs[disk_id];
	if (ps3_atomic_read(&qos_pd_mgr->valid) != 1) {
		LOG_DEBUG("qos pd is invalid. host_no:%u CFID:%u did:%u\n",
			 PS3_HOST(instance), cmd->index, disk_id);
		goto l_out;
	}

	for (i = 0; i < cmd->target_pd_count; i++) {
		if (cmd->target_pd[i].flat_disk_id == disk_id) {
			cmd->target_pd[i].strip_count++;
			goto l_out;
		}
	}

	cmd->target_pd[cmd->target_pd_count].flat_disk_id = disk_id;
	cmd->target_pd[cmd->target_pd_count].strip_count = 1;
	cmd->target_pd_count++;
l_out:
	return;
}

static void ps3_r0_vlba_to_pd(struct ps3_cmd *cmd, U64 lba)
{
	U32 stripe_offset = 0;
	U32 strip_size_shift = 0;
	U8 span_idx = 0;
	U32 span_disk_idx = 0;
	U16 disk_id= 0;
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	LBA_TO_STRIPE_OFFSET(lba, vd_entry->stripeDataSize, stripe_offset);
	span_disk_idx = stripe_offset >> strip_size_shift;
	if (vd_entry->span[span_idx].extent[span_disk_idx].state == MIC_PD_STATE_ONLINE) {
		disk_id = vd_entry->span[span_idx].extent[span_disk_idx].phyDiskID.ps3Dev.phyDiskID;
		ps3_update_cmd_target_pd(cmd, disk_id);
	}
}

static void ps3_r0_target_pd_get(struct ps3_cmd *cmd)
{
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U32 left_len = cmd->io_attr.num_blocks;
	U32 strip_len = 0;
	Bool first_strip = PS3_TRUE;

	while (left_len > 0) {
		ps3_r0_vlba_to_pd(cmd, vlba);
		if (first_strip) {
			strip_len = vd_entry->stripSize - (vlba & STRIP_SIZE_MASK(vd_entry->stripSize));
			strip_len = PS3_MIN(strip_len, left_len);
			first_strip = PS3_FALSE;
		} else {
			strip_len = PS3_MIN(left_len, vd_entry->stripSize);
		}
		left_len -= strip_len;
		vlba += strip_len;
	}
}

static void ps3_r1_target_pd_get(struct ps3_cmd *cmd)
{
	U8 span_idx = 0;
	U16 span_disk_idx = 0;
	U16 disk_id= 0;
	const struct PS3Extent *extent = NULL;
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	Bool primary_valid = PS3_FALSE;

	ps3_scsih_vlba_to_pd_calc(cmd);
	span_disk_idx = cmd->io_attr.span_pd_idx;
	extent = &vd_entry->span[span_idx].extent[span_disk_idx];
	if (extent->state == MIC_PD_STATE_ONLINE) {
		disk_id = extent->phyDiskID.ps3Dev.phyDiskID;
		ps3_update_cmd_target_pd(cmd, disk_id);
		primary_valid = PS3_TRUE;
	}

	if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag) || !primary_valid) {
		span_disk_idx = cmd->io_attr.span_pd_idx_p;
		extent = &vd_entry->span[span_idx].extent[span_disk_idx];
		if (extent->state == MIC_PD_STATE_ONLINE) {
			disk_id = extent->phyDiskID.ps3Dev.phyDiskID;
			ps3_update_cmd_target_pd(cmd, disk_id);
		}
	}
}

static void ps3_r00_vlba_to_pd(struct ps3_cmd *cmd, U64 lba)
{
	U32 stripe_offset = 0;
	U32 strip_size_shift = 0;
	U8 span_idx = 0;
	U32 span_disk_idx = 0;
	U16 disk_id= 0;
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	LBA_TO_STRIPE_OFFSET(lba, vd_entry->stripeDataSize, stripe_offset);
	while (stripe_offset >= vd_entry->span[span_idx].spanStripeDataSize) {
		stripe_offset -= vd_entry->span[span_idx].spanStripeDataSize;
		span_idx++;
	}
	span_disk_idx = stripe_offset >> strip_size_shift;
	if (vd_entry->span[span_idx].extent[span_disk_idx].state == MIC_PD_STATE_ONLINE) {
		disk_id = vd_entry->span[span_idx].extent[span_disk_idx].phyDiskID.ps3Dev.phyDiskID;
		ps3_update_cmd_target_pd(cmd, disk_id);
	}
}

static void ps3_r00_target_pd_get(struct ps3_cmd *cmd)
{
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U32 left_len = cmd->io_attr.num_blocks;
	U32 strip_len = 0;
	Bool first_strip = PS3_TRUE;

	while (left_len > 0) {
		ps3_r00_vlba_to_pd(cmd, vlba);
		if (first_strip) {
			strip_len = vd_entry->stripSize - (vlba & STRIP_SIZE_MASK(vd_entry->stripSize));
			strip_len = PS3_MIN(strip_len, left_len);
			first_strip = PS3_FALSE;
		} else {
			strip_len = PS3_MIN(left_len, vd_entry->stripSize);
		}
		left_len -= strip_len;
		vlba += strip_len;
	}
}

static void ps3_r5_vlba_to_pd(struct ps3_cmd *cmd, U64 vlba)
{
	U32 stripe_offset = 0;	
	U64 stripe_idx = 0;	
	U32 strip_size_shift = 0; 
	U32 span_data_disk_idx = 0;	
	U32 span_parity_disk_idx = 0;	
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U16 disk_id = 0;

	LBA_TO_STRIPE_INDEX(vlba, vd_entry->stripeDataSize, stripe_idx);
	LBA_TO_STRIPE_OFFSET(vlba, vd_entry->stripeDataSize, stripe_offset);

	if (ps3_is_power_of_2(vd_entry->physDrvCnt)) {
		span_parity_disk_idx = stripe_idx & (vd_entry->physDrvCnt - 1);
	} else {
		span_parity_disk_idx = stripe_idx % vd_entry->physDrvCnt;
	}
	span_parity_disk_idx = vd_entry->physDrvCnt - span_parity_disk_idx - 1;

	strip_size_shift = ps3_blocksize_to_shift(vd_entry->stripSize);
	span_data_disk_idx = stripe_offset >> strip_size_shift;
	span_data_disk_idx = span_parity_disk_idx + span_data_disk_idx + 1;
	if (span_data_disk_idx >= vd_entry->physDrvCnt) {
		span_data_disk_idx -= vd_entry->physDrvCnt;
	}

	if (vd_entry->span[0].extent[span_data_disk_idx].state == MIC_PD_STATE_ONLINE) {
		disk_id = vd_entry->span[0].extent[span_data_disk_idx].phyDiskID.ps3Dev.phyDiskID;
		ps3_update_cmd_target_pd(cmd, disk_id);
	}
}

static void ps3_r5_target_pd_get(struct ps3_cmd *cmd)
{
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U32 left_len = cmd->io_attr.num_blocks;
	U32 strip_len = 0;
	Bool first_strip = PS3_TRUE;

	while (left_len > 0) {
		ps3_r5_vlba_to_pd(cmd, vlba);
		if (first_strip) {
			strip_len = vd_entry->stripSize - (vlba & STRIP_SIZE_MASK(vd_entry->stripSize));
			strip_len = PS3_MIN(strip_len, left_len);
			first_strip = PS3_FALSE;
		} else {
			strip_len = PS3_MIN(left_len, vd_entry->stripSize);
		}
		left_len -= strip_len;
		vlba += strip_len;
	}
}

static void ps3_r6_vlba_to_pd(struct ps3_cmd *cmd, U64 vlba)
{
   U32 stripe_offset = 0;	
	U64 stripe_idx = 0;	
	U32 strip_size_shift = 0; 
	U32 span_data_disk_idx = 0;	
	U32 span_q_disk_idx = 0;	
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U16 disk_id = 0;

	LBA_TO_STRIPE_INDEX(vlba, vd_entry->stripeDataSize, stripe_idx);
	LBA_TO_STRIPE_OFFSET(vlba, vd_entry->stripeDataSize, stripe_offset);

	if (ps3_is_power_of_2(vd_entry->physDrvCnt)){
		span_q_disk_idx = stripe_idx & (vd_entry->physDrvCnt - 1);
	} else {
		span_q_disk_idx = stripe_idx % vd_entry->physDrvCnt;
	}
	span_q_disk_idx = (vd_entry->physDrvCnt) - span_q_disk_idx - 1;

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	span_data_disk_idx = stripe_offset >> strip_size_shift;
	span_data_disk_idx = span_data_disk_idx + span_q_disk_idx + 1;
	if ( span_data_disk_idx >= vd_entry->physDrvCnt) {
		span_data_disk_idx -= vd_entry->physDrvCnt;
	}

	if (vd_entry->span[0].extent[span_data_disk_idx].state == MIC_PD_STATE_ONLINE) {
		disk_id = vd_entry->span[0].extent[span_data_disk_idx].phyDiskID.ps3Dev.phyDiskID;
		ps3_update_cmd_target_pd(cmd, disk_id);
	}
}

static void ps3_r6_target_pd_get(struct ps3_cmd *cmd)
{
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U32 left_len = cmd->io_attr.num_blocks;
	U32 strip_len = 0;

	ps3_r6_vlba_to_pd(cmd, vlba);

	strip_len = vd_entry->stripSize - (vlba & STRIP_SIZE_MASK(vd_entry->stripSize));
	strip_len = PS3_MIN(strip_len, left_len);
	left_len -=  strip_len;
	vlba += strip_len;
	while (left_len > 0) {
		strip_len = PS3_MIN(left_len, vd_entry->stripSize);
		ps3_r6_vlba_to_pd(cmd, vlba);
		left_len -= strip_len;
		vlba += strip_len;
	}
}

static void ps3_r50_vlba_to_pd(struct ps3_cmd *cmd, U64 vlba)
{
	U32 stripe_offset = 0;	
	U64 stripe_idx = 0;	
	U32 strip_size_shift = 0; 
	U32 span_data_disk_idx = 0;	
	U32 span_parity_disk_idx = 0;	
	U32 span_idx = 0;
	U8	span_pd_num = 0;
	U16 disk_id = 0;
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;

	LBA_TO_STRIPE_INDEX(vlba, vd_entry->stripeDataSize, stripe_idx);
	LBA_TO_STRIPE_OFFSET(vlba, vd_entry->stripeDataSize, stripe_offset);

	while (stripe_offset >= vd_entry->span[span_idx].spanStripeDataSize) {
		stripe_offset -= vd_entry->span[span_idx].spanStripeDataSize;
		span_idx++;
	}

	span_pd_num = VD_SPAN_PD_NUM(vd_entry, span_idx);
	if (ps3_is_power_of_2(span_pd_num)) {
		span_parity_disk_idx = stripe_idx & (span_pd_num - 1);
	} else {
		span_parity_disk_idx = stripe_idx % span_pd_num;
	}
	span_parity_disk_idx = span_pd_num - span_parity_disk_idx - 1;

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	span_data_disk_idx = stripe_offset >> strip_size_shift;
	span_data_disk_idx = span_parity_disk_idx + span_data_disk_idx + 1;
	if ( span_data_disk_idx >= span_pd_num) {
		span_data_disk_idx -= span_pd_num;
	}

	if (vd_entry->span[span_idx].extent[span_data_disk_idx].state == MIC_PD_STATE_ONLINE) {
		disk_id = vd_entry->span[span_idx].extent[span_data_disk_idx].phyDiskID.ps3Dev.phyDiskID;
		ps3_update_cmd_target_pd(cmd, disk_id);
	}
}

static void ps3_r50_target_pd_get(struct ps3_cmd *cmd)
{
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U32 left_len = cmd->io_attr.num_blocks;
	U32 strip_len = 0;

	ps3_r50_vlba_to_pd(cmd, vlba);

	strip_len = vd_entry->stripSize - (vlba & STRIP_SIZE_MASK(vd_entry->stripSize));
	strip_len = PS3_MIN(strip_len, left_len);
	left_len -=  strip_len;
	vlba += strip_len;
	while (left_len > 0) {
		strip_len = PS3_MIN(left_len, vd_entry->stripSize);
		ps3_r50_vlba_to_pd(cmd, vlba);
		left_len -= strip_len;
		vlba += strip_len;
	}
}

static void ps3_r60_vlba_to_pd(struct ps3_cmd *cmd, U64 vlba)
{
	U32 stripe_offset = 0;	
	U64 stripe_idx = 0;	
	U32 strip_size_shift = 0; 
	U32 span_data_disk_idx = 0;	
	U32 span_q_disk_idx = 0;	
	U32 span_idx = 0;
	U8	span_pd_num = 0;
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U16 disk_id = 0;

	LBA_TO_STRIPE_INDEX(vlba, vd_entry->stripeDataSize, stripe_idx);
	LBA_TO_STRIPE_OFFSET(vlba, vd_entry->stripeDataSize, stripe_offset);

	while (stripe_offset >= vd_entry->span[span_idx].spanStripeDataSize) {
		stripe_offset -= vd_entry->span[span_idx].spanStripeDataSize;
		span_idx++;
	}

	span_pd_num = VD_SPAN_PD_NUM(vd_entry, span_idx);
	RAID660_GET_PQ_SPINDLENO(stripe_idx, span_pd_num, span_q_disk_idx);

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	span_data_disk_idx = stripe_offset >> strip_size_shift;
	span_data_disk_idx = span_data_disk_idx + span_q_disk_idx + 1;
	if ( span_data_disk_idx >= span_pd_num) {
		span_data_disk_idx -= span_pd_num;
	}

	if (vd_entry->span[span_idx].extent[span_data_disk_idx].state == MIC_PD_STATE_ONLINE) {
		disk_id = vd_entry->span[span_idx].extent[span_data_disk_idx].phyDiskID.ps3Dev.phyDiskID;
		ps3_update_cmd_target_pd(cmd, disk_id);
	}
}

static void ps3_r60_target_pd_get(struct ps3_cmd *cmd)
{
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	U32 left_len = cmd->io_attr.num_blocks;
	U32 strip_len = 0;

	ps3_r60_vlba_to_pd(cmd, vlba);

	strip_len = vd_entry->stripSize - (vlba & STRIP_SIZE_MASK(vd_entry->stripSize));
	strip_len = PS3_MIN(strip_len, left_len);
	left_len -=  strip_len;
	vlba += strip_len;
	while (left_len > 0) {
		strip_len = PS3_MIN(left_len, vd_entry->stripSize);
		ps3_r60_vlba_to_pd(cmd, vlba);
		left_len -= strip_len;
		vlba += strip_len;
	}
}

static void ps3_r10_vlba_to_pd(struct ps3_cmd *cmd, U64 lba)
{
	U32 stripe_offset = 0;
	U32 strip_size_shift = 0;
	U8 span_idx = 0;
	U32 span_disk_idx = 0;
	U32 span_disk_back_idx = 0;
	U16 span_pd_num = 0;
	U16 disk_id= 0;
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	Bool primary_valid = PS3_FALSE;

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	LBA_TO_STRIPE_OFFSET(lba, vd_entry->stripeDataSize, stripe_offset);
	while(stripe_offset >= vd_entry->span[span_idx].spanStripeDataSize) {
		stripe_offset -= vd_entry->span[span_idx].spanStripeDataSize;
		span_idx++;
	}
	span_disk_idx = stripe_offset >> strip_size_shift;
	span_disk_idx = span_disk_idx << 1;
	span_pd_num = VD_SPAN_PD_NUM(vd_entry, span_idx);
	if (span_pd_num & 1) {
		if (span_disk_idx > span_pd_num ) {
			span_disk_idx -= span_pd_num;
		}

		if (span_disk_idx + 1 == span_pd_num) {
			span_disk_back_idx = 0;
		} else {
			span_disk_back_idx = span_disk_idx + 1;
		}
	} else {
		if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
			if (cmd->io_attr.span_pd_idx & 1) {
				span_disk_back_idx = span_disk_idx;
				span_disk_idx++;
			}

		} else {
			span_disk_back_idx = span_disk_idx + 1;
		}
	}

	if (vd_entry->span[span_idx].extent[span_disk_idx].state == MIC_PD_STATE_ONLINE) {
		disk_id = vd_entry->span[span_idx].extent[span_disk_idx].phyDiskID.ps3Dev.phyDiskID;
		ps3_update_cmd_target_pd(cmd, disk_id);
		primary_valid = PS3_TRUE;
	}

	if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag) || !primary_valid) {
		if (vd_entry->span[span_idx].extent[span_disk_back_idx].state == MIC_PD_STATE_ONLINE) {
			disk_id = vd_entry->span[span_idx].extent[span_disk_back_idx].phyDiskID.ps3Dev.phyDiskID;
			ps3_update_cmd_target_pd(cmd, disk_id);
		}
	}
}

static void ps3_r10_target_pd_get(struct ps3_cmd *cmd)
{
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	const struct PS3Extent *extent = NULL;
	U32 left_len = cmd->io_attr.num_blocks;
	U32 strip_len = 0;
	U16 flat_pd_id = 0;
	Bool primary_valid = PS3_FALSE;
	U8 span_idx = 0;
	U8 span_disk_idx = 0;

	ps3_scsih_vlba_to_pd_calc(cmd);
	span_idx = cmd->io_attr.span_idx;
	span_disk_idx = cmd->io_attr.span_pd_idx;
	extent = &vd_entry->span[span_idx].extent[span_disk_idx];
	if (extent->state == MIC_PD_STATE_ONLINE) {
		flat_pd_id = extent->phyDiskID.ps3Dev.phyDiskID;
		ps3_update_cmd_target_pd(cmd, flat_pd_id);
		primary_valid = PS3_TRUE;
	}
	if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag) || !primary_valid) {
		span_disk_idx = cmd->io_attr.span_pd_idx_p;
		extent = &vd_entry->span[span_idx].extent[span_disk_idx];
		if (extent->state == MIC_PD_STATE_ONLINE) {
			flat_pd_id =  extent->phyDiskID.ps3Dev.phyDiskID;
			ps3_update_cmd_target_pd(cmd, flat_pd_id);
		}
	}

	strip_len = vd_entry->stripSize - (vlba & STRIP_SIZE_MASK(vd_entry->stripSize));
	strip_len = PS3_MIN(strip_len, left_len);
	left_len -=  strip_len;
	vlba += strip_len;
	while (left_len > 0) {
		strip_len = PS3_MIN(left_len, vd_entry->stripSize);
		ps3_r10_vlba_to_pd(cmd, vlba);
		left_len -= strip_len;
		vlba += strip_len;
	}
}

static void ps3_r1e_vlba_to_pd(struct ps3_cmd *cmd, U64 lba)
{
	U32 stripe_offset = 0;
	U32 strip_size_shift = 0;
	U8 span_idx = 0;
	U32 span_disk_idx = 0;
	U32 span_disk_back_idx = 0;
	U16 disk_id= 0;
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	Bool primary_valid = PS3_FALSE;

	strip_size_shift = ps3_blocksize_to_shift (vd_entry->stripSize);
	LBA_TO_STRIPE_OFFSET(lba, vd_entry->stripeDataSize, stripe_offset);
	span_disk_idx = stripe_offset >> strip_size_shift;
	span_disk_idx = span_disk_idx << 1;
	if (vd_entry->physDrvCnt & 1) {
		if (span_disk_idx > vd_entry->physDrvCnt) {
			span_disk_idx -= vd_entry->physDrvCnt;
		}
		if (span_disk_idx + 1 == vd_entry->physDrvCnt) {
			span_disk_back_idx = 0;
		} else {
			span_disk_back_idx = span_disk_idx + 1;
		}
	} else {
		if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
			if (cmd->io_attr.span_pd_idx & 1) {
				span_disk_back_idx = span_disk_idx;
				span_disk_idx++;
			}

		} else {
			span_disk_back_idx = span_disk_idx + 1;
		}
	}

	if (vd_entry->span[span_idx].extent[span_disk_idx].state == MIC_PD_STATE_ONLINE) {
		disk_id = vd_entry->span[span_idx].extent[span_disk_idx].phyDiskID.ps3Dev.phyDiskID;
		ps3_update_cmd_target_pd(cmd, disk_id);
		primary_valid = PS3_TRUE;
	}
	if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag) || !primary_valid) {
		if (vd_entry->span[span_idx].extent[span_disk_back_idx].state == MIC_PD_STATE_ONLINE) {
			disk_id = vd_entry->span[span_idx].extent[span_disk_back_idx].phyDiskID.ps3Dev.phyDiskID;
			ps3_update_cmd_target_pd(cmd, disk_id);
		}
	}
}

static void ps3_r1e_target_pd_get(struct ps3_cmd *cmd)
{
	U64 vlba = U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
	const struct PS3VDEntry *vd_entry = cmd->io_attr.vd_entry;
	const struct PS3Extent *extent = NULL;
	U32 left_len = cmd->io_attr.num_blocks;
	U32 strip_len = 0;
	U16 flat_pd_id = 0;
	Bool primary_valid = PS3_FALSE;
	U8 span_idx = 0;
	U8 span_disk_idx = 0;

	ps3_scsih_vlba_to_pd_calc(cmd);
	span_idx = cmd->io_attr.span_idx;
	span_disk_idx = cmd->io_attr.span_pd_idx;
	extent = &vd_entry->span[span_idx].extent[span_disk_idx];
	if (extent->state == MIC_PD_STATE_ONLINE) {
		flat_pd_id = extent->phyDiskID.ps3Dev.phyDiskID;
		ps3_update_cmd_target_pd(cmd, flat_pd_id);
		primary_valid = PS3_TRUE;
	}
	if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag) || !primary_valid) {
		span_disk_idx = cmd->io_attr.span_pd_idx_p;
		extent = &vd_entry->span[span_idx].extent[span_disk_idx];
		if (extent->state == MIC_PD_STATE_ONLINE) {
			flat_pd_id =  extent->phyDiskID.ps3Dev.phyDiskID;
			ps3_update_cmd_target_pd(cmd, flat_pd_id);
		}
	}

	strip_len = vd_entry->stripSize - (vlba & STRIP_SIZE_MASK(vd_entry->stripSize));
	strip_len = PS3_MIN(strip_len, left_len);
	left_len -=  strip_len;
	vlba += strip_len;
	while (left_len > 0) {
		strip_len = PS3_MIN(left_len, vd_entry->stripSize);
		ps3_r1e_vlba_to_pd(cmd, vlba);
		left_len -= strip_len;
		vlba += strip_len;
	}
}

static inline void ps3_swap_in_array(struct ps3_qos_member_pd_info *arr, U16 i, U16 j)
{
	struct ps3_qos_member_pd_info tmp;
	tmp = arr[i];
	arr[i] = arr[j];
	arr[j] = tmp;
}

static void ps3_qos_target_pd_adjust(struct ps3_cmd *cmd)
{
	U16 i = 0;
	U16 j = 0;

	for (i = 0; i < cmd->target_pd_count; i++) {
		for (j = i + 1; j < cmd->target_pd_count; j++) {
			if (cmd->target_pd[i].flat_disk_id >
					cmd->target_pd[j].flat_disk_id) {
				ps3_swap_in_array(cmd->target_pd, i, j);
			}
		}
	}
}

void ps3_qos_cmd_member_pd_calc(struct ps3_cmd *cmd)
{
	const struct PS3VDEntry *vd_entry = NULL;
	U16 disk_id = 0;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	struct ps3_qos_pd_mgr *qos_peer_pd_mgr = NULL;
	static unsigned long j;
	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		vd_entry = cmd->io_attr.vd_entry;

		if (cmd->cmd_word.direct == PS3_CMDWORD_DIRECT_ADVICE) {
			disk_id = PS3_PDID(&cmd->io_attr.pd_entry->disk_pos);
			if (ps3_is_r1x_write_cmd(cmd)) {
				qos_pd_mgr = &cmd->instance->qos_context.pd_ctx.qos_pd_mgrs[disk_id];
				disk_id = PS3_PDID(&cmd->io_attr.peer_pd_entry->disk_pos);
				qos_peer_pd_mgr = &cmd->instance->qos_context.pd_ctx.qos_pd_mgrs[disk_id];
				disk_id = (qos_pd_mgr->pd_quota <= qos_peer_pd_mgr->pd_quota) ? 
							qos_pd_mgr->disk_id : qos_peer_pd_mgr->disk_id;
			}
			ps3_update_cmd_target_pd(cmd, disk_id);
		} else {
			if (!vd_entry->isNvme && !vd_entry->isSsd) {
				goto _out;
			}
			switch(vd_entry->raidLevel) {
				case RAID0:
					ps3_r0_target_pd_get(cmd);
					break;
				case RAID1:
					ps3_r1_target_pd_get(cmd);
					break;
				case RAID10:
					ps3_r10_target_pd_get(cmd);
					break;
				case RAID1E:
					ps3_r1e_target_pd_get(cmd);
					break;
				case RAID00:
					ps3_r00_target_pd_get(cmd);
					break;
				case RAID5:
					ps3_r5_target_pd_get(cmd);
					break;
				case RAID6:
					ps3_r6_target_pd_get(cmd);
					break;
				case RAID50:
					ps3_r50_target_pd_get(cmd);
					break;
				case RAID60:
					ps3_r60_target_pd_get(cmd);
					break;
				default:
					LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "trace_id:0x%llx host_no:%u vd level:%d is illegal\n",
						 cmd->trace_id, PS3_HOST(cmd->instance),
						cmd->io_attr.vd_entry->raidLevel);
			}

			ps3_qos_target_pd_adjust(cmd);
		}
	} else {
		ps3_update_cmd_target_pd(cmd, cmd->io_attr.disk_id);
	}
_out:
	LOG_DEBUG("qos target pd calc. host_no:%u cmd[%u,%u] pd_cnt:%u dev_t:%u diskid:%u\n",
		PS3_HOST(cmd->instance), cmd->index, cmd->cmd_word.type, cmd->target_pd_count,
		cmd->io_attr.dev_type, cmd->io_attr.disk_id);
}

U16 ps3_odd_r1x_judge(struct PS3VDEntry *vd_entry)
{
	U16 ret = PS3_IS_SSD_EVEN_R1X_VD;
	U8 span_idx = 0;
	if (!vd_entry->isSsd) {
		ret = PS3_IS_HDD_R1X_VD;
		goto l_out;
	}
	switch (vd_entry->raidLevel) {
		case RAID10:
			for (; span_idx < vd_entry->spanCount; span_idx++) {
				if (VD_SPAN_PD_NUM(vd_entry, span_idx) & 1) {
					ret = PS3_IS_SSD_ODD_R1X_VD;
					goto l_out;
				}
			}
			break;
		case RAID1E:
			if (VD_SPAN_PD_NUM(vd_entry, 0) & 1) {
				ret = PS3_IS_SSD_ODD_R1X_VD;
				goto l_out;
			}
			break;
		default:
			ret = PS3_IS_VALID_R1X_VD;
			break;
	}
l_out:
	return ret;
}

