
#ifndef _PS3_SCSIH_H_
#define _PS3_SCSIH_H_

#ifndef _WINDOWS
#include <scsi/scsi_host.h>
#include <scsi/scsi_cmnd.h>
#else
#include "ps3_cmd_adp.h"
#endif

#include "ps3_htp_def.h"
#include "ps3_inner_data.h"

#define PS3_PAGE_MODE_ABOVE_3_ADDR_MASK 0xFFFFFF8000000000ULL
#define PS3_PAGE_MODE_ABOVE_4_ADDR_MASK 0xFFFF000000000000ULL

#define PS3_IS_R0J1(raidlevel) \
	((raidlevel) == RAID0 || (raidlevel) == RAID1 || \
	(raidlevel) == RAID10 || (raidlevel) == RAID1E || \
	(raidlevel) == RAID00)

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,15,0))
#define SCMD_GET_REQUEST(scmd)		scsi_cmd_to_rq(scmd)
#else
#define SCMD_GET_REQUEST(scmd)		scmd->request
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,16,0))
#define SCMD_IO_DONE(scmd)		scsi_done(scmd)
#else
#define SCMD_IO_DONE(scmd)		scmd->scsi_done(scmd)
#endif

#if 0
#define PS3_IF_QUIT_STREAM_DIRECT_DETECT(raidlevel) \
	((ps3_direct_check_stream_query() == PS3_FALSE) && PS3_IS_R0J1(raidlevel))
#endif

#define PS3_IF_QUIT_STREAM_DIRECT_DETECT() \
	(ps3_direct_check_stream_query() == PS3_FALSE)

typedef S32 (*dev_type_to_proc_func)(struct ps3_cmd*);
struct disk_type_to_proc_func_table {
	U8 type;
	dev_type_to_proc_func func;
};
#define CMND_LEN16 			(16)
#define FRAME_CMD_MASK_SHIFT		(0x1)
#define FRAME_CMD_MASK_BITS		(0x07)

enum PS3_FRAME_CMD_TYPE {
	SCSI_FRAME_CMD =  0,
	SAS_FRAME_CMD  =  1,
	SATA_FRAME_CMD =  2,
	NVME_FRAME_CMD  = 3,
	UNKNOWN_FRAME_CMD,
};
enum PS3_RW_CMD_TYPE {
	SCSI_RW_UNUSED_CMD =  0,
	SCSI_RW_SEQ_CMD =  1,
	SCSI_RW_RANDOM_CMD  =  2,
};

struct scsi_cmd_parse_table {
	U8 cmd_type;
	U8 rw_attr;
};

static inline void ps3_put_unaligned_be64(U8 *p, U32 val_hi, U32 val_lo)
{
	p[0] = (U8) (val_hi >> PS3_SHIFT_3BYTE) & 0xff;
	p[1] = (U8) (val_hi >> PS3_SHIFT_WORD) & 0xff;
	p[2] = (U8) (val_hi >> PS3_SHIFT_BYTE) & 0xff;
	p[3] = (U8) val_hi & 0xff;

	p[4] = (U8) (val_lo >> PS3_SHIFT_3BYTE) & 0xff;
	p[5] = (U8) (val_lo >> PS3_SHIFT_WORD) & 0xff;
	p[6] = (U8) (val_lo >> PS3_SHIFT_BYTE) & 0xff;
	p[7] = (U8) val_lo & 0xff;
}

static inline void ps3_put_unaligned_be32(U8 *p, U32 val)
{
	p[0] = (U8) (val >> PS3_SHIFT_3BYTE) & 0xff;
	p[1] = (U8) (val >> PS3_SHIFT_WORD) & 0xff;
	p[2] = (U8) (val >> PS3_SHIFT_BYTE) & 0xff;
	p[3] = (U8) val & 0xff;
}

static inline void ps3_put_unaligned_be16(U8 *p, U16 val)
{
	p[0] = (U8) (val >> PS3_SHIFT_BYTE) & 0xff;
	p[1] = (U8) val & 0xff;
}

static inline U16 ps3_get_unaligned_be16(U8 *p)
{
	return (U16)((p[0] << PS3_SHIFT_BYTE) | p[1]);
}

#ifndef _WINDOWS

typedef struct scatterlist ps3_scatter_gather_element;

S32 ps3_scsih_queue_command(struct Scsi_Host *s_host,
	struct scsi_cmnd *s_cmd);
#else
Bool ps3_scsih_sys_state_check(struct ps3_instance *instance, S32 *host_status);
#endif

S32 ps3_scsih_cmd_build(struct ps3_cmd *cmd);

void ps3_scsih_direct_to_normal_req_frame_rebuild(
	struct ps3_cmd *cmd);

S32 ps3_scsih_io_done(struct ps3_cmd *cmd, U16 reply_flags);

void ps3_scsi_dma_unmap(struct ps3_cmd *cmd);

S32 ps3_scsi_dma_map(struct ps3_cmd *cmd);

#if 0
struct disk_type_to_proc_func_table* ps3_req_func_entry_query(U8 dev_type);
#endif

Bool ps3_scsih_sata_direct_is_support(struct ps3_cmd *cmd,
	const struct ps3_pd_entry *pd_entry);

Bool ps3_scsih_stream_is_detect(struct ps3_cmd * cmd);
Bool ps3_raid_scsih_stream_is_direct(const struct ps3_cmd * cmd);
Bool ps3_hba_scsih_stream_is_direct(const struct ps3_cmd * cmd);

void ps3_scsih_print_req(struct ps3_cmd *cmd, U8 log_level);

S32 ps3_get_requeue_or_reset(void);

Bool ps3_scsih_sata_direct_is_need(struct ps3_cmd *cmd);

Bool ps3_scsih_is_sata_jbod_mgr_cmd(const struct ps3_cmd *cmd);

U32 ps3_scsih_xfer_cnt_get(const struct ps3_cmd *cmd);

Bool ps3_is_r1x_write_cmd(const struct ps3_cmd *cmd);

S32 ps3_vd_direct_req_frame_build(struct ps3_cmd *cmd);

Bool ps3_write_direct_enable(struct ps3_cmd *cmd);

#endif
