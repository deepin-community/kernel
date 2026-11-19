/* SPDX-License-Identifier: GPL-2.0 */
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
#include "ps3_kernel_version.h"

#define PS3_HW_VD_MAX_IO_SIZE_1M (1ULL << 20)
#define PS3_PAGE_MODE_ABOVE_3_ADDR_MASK 0xFFFFFF8000000000ULL
#define PS3_PAGE_MODE_ABOVE_4_ADDR_MASK 0xFFFF000000000000ULL

#define PS3_IS_R0J1(raidlevel)                                                 \
	((raidlevel) == RAID0 || (raidlevel) == RAID1 ||                       \
	 (raidlevel) == RAID10 || (raidlevel) == RAID1E ||                     \
	 (raidlevel) == RAID00)

#if defined(PS3_SCMD_GET_REQUEST)
#define SCMD_GET_REQUEST(scmd) scsi_cmd_to_rq(scmd)
#else
#define SCMD_GET_REQUEST(scmd) scmd->request
#endif

#if defined(PS3_SCMD_IO_DONE)
#define SCMD_IO_DONE(scmd) scsi_done(scmd)
#else
#define SCMD_IO_DONE(scmd) scmd->scsi_done(scmd)
#endif

#define PS3_IF_QUIT_STREAM_DIRECT_DETECT()                                     \
	(ps3_direct_check_stream_query() == PS3_FALSE)

struct disk_type_to_proc_func_table {
	unsigned char type;
	int (*func)(struct ps3_cmd *cmd);
};
#define CMND_LEN16 (16)
#define FRAME_CMD_MASK_SHIFT (0x1)
#define FRAME_CMD_MASK_BITS (0x07)

enum PS3_FRAME_CMD_TYPE {
	SCSI_FRAME_CMD = 0,
	SAS_FRAME_CMD = 1,
	SATA_FRAME_CMD = 2,
	NVME_FRAME_CMD = 3,
	UNKNOWN_FRAME_CMD,
};
enum PS3_RW_CMD_TYPE {
	SCSI_RW_UNUSED_CMD = 0,
	SCSI_RW_SEQ_CMD = 1,
	SCSI_RW_RANDOM_CMD = 2,
};

struct scsi_cmd_parse_table {
	unsigned char cmd_type;
	unsigned char rw_attr;
};

static inline void ps3_put_unaligned_be64(unsigned char *p, unsigned int val_hi,
					  unsigned int val_lo)
{
	p[0] = (unsigned char)(val_hi >> PS3_SHIFT_3BYTE) & 0xff;
	p[1] = (unsigned char)(val_hi >> PS3_SHIFT_WORD) & 0xff;
	p[2] = (unsigned char)(val_hi >> PS3_SHIFT_BYTE) & 0xff;
	p[3] = (unsigned char)val_hi & 0xff;

	p[4] = (unsigned char)(val_lo >> PS3_SHIFT_3BYTE) & 0xff;
	p[5] = (unsigned char)(val_lo >> PS3_SHIFT_WORD) & 0xff;
	p[6] = (unsigned char)(val_lo >> PS3_SHIFT_BYTE) & 0xff;
	p[7] = (unsigned char)val_lo & 0xff;
}

static inline void ps3_put_unaligned_be32(unsigned char *p, unsigned int val)
{
	p[0] = (unsigned char)(val >> PS3_SHIFT_3BYTE) & 0xff;
	p[1] = (unsigned char)(val >> PS3_SHIFT_WORD) & 0xff;
	p[2] = (unsigned char)(val >> PS3_SHIFT_BYTE) & 0xff;
	p[3] = (unsigned char)val & 0xff;
}

static inline void ps3_put_unaligned_be16(unsigned char *p, unsigned short val)
{
	p[0] = (unsigned char)(val >> PS3_SHIFT_BYTE) & 0xff;
	p[1] = (unsigned char)val & 0xff;
}

static inline unsigned short ps3_get_unaligned_be16(unsigned char *p)
{
	return (unsigned short)((p[0] << PS3_SHIFT_BYTE) | p[1]);
}

#ifndef _WINDOWS

int ps3_scsih_queue_command(struct Scsi_Host *s_host, struct scsi_cmnd *s_cmd);
#else
unsigned char ps3_scsih_sys_state_check(struct ps3_instance *instance,
					int *host_status);
#endif

int ps3_scsih_cmd_build(struct ps3_cmd *cmd);

void ps3_scsih_direct_to_normal_req_frame_rebuild(struct ps3_cmd *cmd);

int ps3_scsih_io_done(struct ps3_cmd *cmd, unsigned short reply_flags);

void ps3_scsi_dma_unmap(struct ps3_cmd *cmd);

int ps3_scsi_dma_map(struct ps3_cmd *cmd);

unsigned char
ps3_scsih_sata_direct_is_support(struct ps3_cmd *cmd,
				 const struct ps3_pd_entry *pd_entry);

unsigned char ps3_scsih_stream_is_detect(struct ps3_cmd *cmd);
unsigned char ps3_raid_scsih_stream_is_direct(const struct ps3_cmd *cmd);
unsigned char ps3_hba_scsih_stream_is_direct(const struct ps3_cmd *cmd);

void ps3_scsih_print_req(struct ps3_cmd *cmd, unsigned char log_level);

int ps3_get_requeue_or_reset(void);

unsigned char ps3_scsih_sata_direct_is_need(struct ps3_cmd *cmd);

unsigned char ps3_scsih_is_sata_jbod_mgr_cmd(const struct ps3_cmd *cmd);

unsigned int ps3_scsih_xfer_cnt_get(const struct ps3_cmd *cmd);

unsigned char ps3_is_r1x_write_cmd(const struct ps3_cmd *cmd);

int ps3_vd_direct_req_frame_build(struct ps3_cmd *cmd);

unsigned char ps3_write_direct_enable(struct ps3_cmd *cmd);

unsigned char ps3_ssd_vd_qmask_calculate_hba(struct ps3_cmd *cmd);

void ps3_scsih_pd_cmd_word_cpu_set(struct ps3_cmd *cmd);

#endif
