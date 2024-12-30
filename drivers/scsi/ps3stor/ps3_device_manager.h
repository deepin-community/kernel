
#ifndef _PS3_DEVICE_MANAGER_H_
#define _PS3_DEVICE_MANAGER_H_

#include "ps3_platform_utils.h"
#ifdef _WINDOWS
#include "ps3_dev_adp.h"
#else
#include <linux/version.h>
#include <linux/workqueue.h>
#include <scsi/scsi_device.h>
#include <linux/completion.h>

#endif

#include "ps3_meta.h"
#include "ps3_dev_type.h"
#include "ps3_htp.h"
#include "ps3_err_def.h"

#define  PS3_QUEUE_DEPTH_DEFAULT	(256)
#define  PS3_QUEUE_DEPTH_SATA		(32)
#define  PS3_QUEUE_DEPTH_SAS		(64)
#define  PS3_QUEUE_DEPTH_NVME		(32)

#define  PS3_INVALID_VALUE		(0)
#define  PS3_INVALID_DEV_ID		(0) 
#define  PS3_INVALID_LIST_ID		(0)
#define  PS3_DMA_ALIGN_SHIFT_MAX	(15)

#define PS3_VD_TABLE_NUM		(2)
#define PS3_SCSI_4B_ALINNMENT_MASK	(4)
#define PS3_SCSI_ALINNMENT_MASK		(0x3)
#define PS3_SCSI_32B_ALINNMENT_MASK		(0x1f)
#define PS3_SCSI_512B_ALINNMENT_MASK		(0x000001FF)
#define PS3_SCSI_4K_ALINNMENT_MASK		(0xFFF)
#define PS3_512B_SHIFT			(9)
#define PS3_SECTORSIZE_512B		(512)

#define PS3_MAX_VD_COUNT(instance) \
	((instance)->ctrl_info.maxVdCount)
#define PS3_MAX_PD_COUNT(instance) \
	((instance)->ctrl_info.maxPdCount)

#define PS3_CHANNEL(pos) \
	((U8)((pos)->diskDev.ps3Dev.softChan))
#define PS3_TARGET(pos) \
	((U16)((pos)->diskDev.ps3Dev.devID))
#define PS3_DISKID(pos) \
	((pos)->diskDev.diskID)
#define PS3_PDID(pos) \
	((pos)->diskDev.ps3Dev.phyDiskID)
#define PS3_VDID(pos) \
	((pos)->diskDev.ps3Dev.virtDiskID)
#define PS3_DEV(pos) \
	((struct PS3Dev*)(&(pos)->diskDev.ps3Dev))
#define PS3_VDID_OFFSET(instance) \
	((instance)->ctrl_info.offsetOfVDID)

#define PS3_DEV_INVALID(pos) \
	((pos).diskDev.diskID == PS3_INVALID_DEV_ID)
#define PS3_IS_VD_CHANNEL(ins, chan) \
	((ins->dev_context.vd_table[0].vd_idxs[chan] != NULL) ? \
		PS3_DRV_TRUE : PS3_DRV_FALSE)

#define PS3_IS_PD_CHANNEL(ins, chan) \
	((ins->dev_context.pd_table.pd_idxs[chan] != NULL) ? \
		PS3_DRV_TRUE : PS3_DRV_FALSE)

#define PS3_MAX_PD_NUM_ONE_VD (PS3_MAX_PD_COUNT_IN_SPAN * PS3_MAX_SPAN_IN_VD)

struct ps3_instance;

#define MR_STREM_BITMAP (0xfedcba9876543210)
#define PS3_IO_MAX_STREAMS_TRACKED (16)
#define BITS_PER_INDEX_STREAM (4)
#define BITS_PER_INDEX_STREAM_SHIFT (2)
#define STREAM_MASK ((1ULL << BITS_PER_INDEX_STREAM) - 1)
#define ZERO_LAST_STREAM (0x0FFFFFFFFFFFFFFF)

#define MAX_QUE_DEPTH (16)
#define TEST_IO_BLOCK_SIZE  (8)
#define IO_STREAM_DETECT_RANGE  (MAX_QUE_DEPTH * TEST_IO_BLOCK_SIZE)
#define PS3_IS_HAC_LIMIT_TYPE(type) \
	((type) == PS3_DEV_TYPE_SAS_HDD || \
	(type) == PS3_DEV_TYPE_SATA_HDD || \
	(type) == PS3_DEV_TYPE_SATA_SSD || \
	(type) == PS3_DEV_TYPE_SAS_SSD || \
	(type) == PS3_DEV_TYPE_SES)

struct ps3_stream_detect {
    U64 next_seq_lba;  
    U8 rw_type;        
    U8 reserved[7];
};


struct ps3_vd_stream_detect {
    ps3_spinlock  ps3_sequence_stream_lock;
    U64 mru_bit_map;   
    struct ps3_stream_detect stream_track[PS3_IO_MAX_STREAMS_TRACKED];
};
enum PS3DevType {
	PS3_DEV_TYPE_UNKNOWN    = 0,
	PS3_DEV_TYPE_VD         = 1,
	PS3_DEV_TYPE_SAS_HDD    = 2,
	PS3_DEV_TYPE_SATA_HDD   = 3,
	PS3_DEV_TYPE_SATA_SSD   = 4,
	PS3_DEV_TYPE_SAS_SSD    = 5,
	PS3_DEV_TYPE_NVME_SSD   = 6,
	PS3_DEV_TYPE_SES        = 7,
	PS3_DEV_TYPE_VEP	= 8,
	PS3_DEV_TYPE_COUNT,
};

enum PS3DevTypeV1 {
	PS3_DEV_TYPE_SAS_SATA   = 0,
	PS3_DEV_TYPE_NVME       = 1,
};

static inline const S8 *namePS3DevType(enum PS3DevType e)
{
	static const S8 *myNames[] = {
		[PS3_DEV_TYPE_UNKNOWN]	 = "DEV_T_UNKNOWN",
		[PS3_DEV_TYPE_VD]	 = "DEV_T_VD",
		[PS3_DEV_TYPE_SAS_HDD]	 = "DEV_T_SAS_HDD",
		[PS3_DEV_TYPE_SATA_HDD]  = "DEV_T_SATA_HDD",
		[PS3_DEV_TYPE_SATA_SSD]  = "DEV_T_SATA_SSD",
		[PS3_DEV_TYPE_SAS_SSD]	 = "DEV_T_SAS_SSD",
		[PS3_DEV_TYPE_NVME_SSD]  = "DEV_T_NVME_SSD",
		[PS3_DEV_TYPE_SES]	 = "DEV_T_SES",
		[PS3_DEV_TYPE_VEP]	 = "DEV_T_VEP"
	};

	return myNames[e];
}

enum ps3_dev_io_stat_type {
	PS3_DEV_IO_STAT_TYPE_R_SEND = 1,
	PS3_DEV_IO_STAT_TYPE_R_SEND_OK,
	PS3_DEV_IO_STAT_TYPE_R_SEND_WAIT,
	PS3_DEV_IO_STAT_TYPE_R_SEND_ERR,
	PS3_DEV_IO_STAT_TYPE_R_RECV,
	PS3_DEV_IO_STAT_TYPE_R_RECV_OK,
	PS3_DEV_IO_STAT_TYPE_R_RECV_ERR,
	PS3_DEV_IO_STAT_TYPE_R_OK_BYTES,

	PS3_DEV_IO_STAT_TYPE_W_SEND,
	PS3_DEV_IO_STAT_TYPE_W_SEND_OK,
	PS3_DEV_IO_STAT_TYPE_W_SEND_WAIT,
	PS3_DEV_IO_STAT_TYPE_W_SEND_ERR,
	PS3_DEV_IO_STAT_TYPE_W_RECV,
	PS3_DEV_IO_STAT_TYPE_W_RECV_OK,
	PS3_DEV_IO_STAT_TYPE_W_RECV_ERR,
	PS3_DEV_IO_STAT_TYPE_W_OK_BYTES,
	PS3_DEV_IO_STAT_TYPE_MAX,
};

struct ps3_dev_io_statis {
	ps3_atomic64 read_send_cnt;
	ps3_atomic64 read_send_ok_cnt;
	ps3_atomic64 read_send_wait_cnt;
	ps3_atomic64 read_send_err_cnt;
	ps3_atomic64 read_recv_cnt;
	ps3_atomic64 read_recv_ok_cnt;
	ps3_atomic64 read_recv_err_cnt;
	ps3_atomic64 read_ok_bytes;

	ps3_atomic64 write_send_cnt;
	ps3_atomic64 write_send_ok_cnt;
	ps3_atomic64 write_send_err_cnt;
	ps3_atomic64 write_send_wait_cnt;
	ps3_atomic64 write_recv_cnt;
	ps3_atomic64 write_recv_ok_cnt;
	ps3_atomic64 write_recv_err_cnt;
	ps3_atomic64 write_ok_bytes;

    ps3_atomic64 qos_processing_cnt;
};

static inline U8 ps3_disk_type(enum PS3DevType e)
{
	U8 disk_type = PS3_DISK_TYPE_UNKNOWN;

	switch(e) {
		case PS3_DEV_TYPE_VD:
			disk_type = PS3_DISK_TYPE_VD;
			break;
		case PS3_DEV_TYPE_SAS_HDD:
		case PS3_DEV_TYPE_SATA_HDD:
		case PS3_DEV_TYPE_SATA_SSD:
		case PS3_DEV_TYPE_SAS_SSD:
		case PS3_DEV_TYPE_NVME_SSD:
		case PS3_DEV_TYPE_SES:
		case PS3_DEV_TYPE_VEP:
			disk_type = PS3_DISK_TYPE_PD;
			break;
		default:
			disk_type = PS3_DISK_TYPE_UNKNOWN;
			break;
	}
	return disk_type;
}

static inline Bool ps3_is_fake_pd(U8 dev_type)
{
	return (dev_type == PS3_DEV_TYPE_SES || dev_type == PS3_DEV_TYPE_VEP);
}

struct ps3_r1x_read_balance_info {
	ps3_atomic32	scsi_outstanding_cmds[PS3_MAX_PD_NUM_ONE_VD + 1];	 
	U64				last_accessed_block[PS3_MAX_PD_NUM_ONE_VD + 1];      
};

struct ps3_r1x_lock_mgr {
	S32 (*try_lock)(struct ps3_r1x_lock_mgr *mgr, void *cmd);       	
	S32 (*resend_try_lock)(struct ps3_r1x_lock_mgr *mgr, void *cmd);       	
	void (*unlock)(struct ps3_r1x_lock_mgr *mgr, void *cmd);          	
	ps3_spinlock mgr_lock;                                           	
	void *hash_mgr;
	ps3_list_head conflict_cmd_list;
	struct task_struct *conflict_send_th;
	void *hash_mgr_conflict;
	S32 force_ret_code;
	U32 cmd_count_in_q;
	Bool dev_deling;
	Bool thread_stop;
#ifdef _WINDOWS
	KEVENT thread_sync;
#else
	struct completion thread_sync;	
#endif

};

#ifndef _WINDOWS
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0) || \
	(defined(KYLIN_MAJOR) && defined(CONFIG_X86) && (KYLIN_MAJOR == 10 && KYLIN_MINOR == 4)))
#define DRIVER_SUPPORT_PRIV_BUSY
#endif

struct ps3_scsi_priv_data {
	struct PS3DiskDevPos disk_pos;	
	struct ps3_dev_io_statis statis;
	struct ps3_vd_stream_detect vd_sd[2];
	struct ps3_r1x_lock_mgr lock_mgr;	
	struct ps3_r1x_read_balance_info *r1x_rb_info; 
	U8 is_taskmgmt_enable;
	U8 task_manager_busy;
	U8 dev_type;	    
	U8 task_abort_timeout;  
	U8 task_reset_timeout;  
	U8 encl_id;	
	U8 phy_id;	
	U8 reserved;
	ps3_atomic32 rd_io_outstand; 
	ps3_atomic32 wr_io_outstand; 
	ps3_atomic32 r1x_read_cmd_swap_total_cnt; 
	ps3_atomic32 r1x_read_cmd_swap_res_cnt; 
#if defined DRIVER_SUPPORT_PRIV_BUSY
	ps3_atomic32 sdev_priv_busy; 
#else
	U8 reserved1[4];
#endif
	ULong ata_cmd_busy;	
	Bool dev_deling; 
	Bool swap_flag; 
};
#else
struct ps3_scsi_priv_data {
	struct PS3DiskDevPos disk_pos;	
	struct ps3_dev_io_statis statis;
    struct ps3_vd_stream_detect vd_sd[2];
	struct ps3_r1x_lock_mgr lock_mgr;	
	U8 is_taskmgmt_enable : 1;
	U8 task_manager_busy : 1;
	U8 reserved1 : 6;
	U8 dev_type;			
	U8 task_abort_timeout;  
	U8 task_reset_timeout;  
	ULong ata_cmd_busy;	
};
#endif

#ifdef _WINDOWS

struct scsi_device_real {
	struct scsi_device sdev;			
	struct ps3_scsi_priv_data hostdata;	
};

#define PS3_SDEV_PRI_DATA(sdev) \
	((struct ps3_scsi_priv_data*)&(((struct scsi_device_real *)(sdev))->hostdata))
#define PS3_SDEV_POS(sdev) \
	(&(PS3_SDEV_PRI_DATA(sdev)->disk_pos))
#define PS3_SDEV_CHANNEL(sdev) \
	(PS3_CHANNEL(PS3_SDEV_POS(sdev)))
#define PS3_SDEV_TARGET(sdev) \
	(PS3_TARGET(PS3_SDEV_POS(sdev)))
#define PS3_SDEV_PDID(sdev) \
	(PS3_PDID(PS3_SDEV_POS(sdev)))
#define PS3_SDEV_MAGIC(sdev) \
	(PS3_SDEV_POS(sdev)->diskMagicNum)
#else
#define PS3_SDEV_CHANNEL(sdev) \
	((sdev)->channel)
#define PS3_SDEV_TARGET(sdev) \
	((sdev)->id)

#define PS3_SDEV_PRI_DATA(sdev) \
	((struct ps3_scsi_priv_data*)((sdev)->hostdata))
#endif

struct ps3_channel {
	U8 channel;
	U8 reserved0;
	U16 max_dev_num;
	U16 channel_start_num;
	U8 reserved1[2];
};

struct ps3_dev_pool {
	union PS3Device *devs_buffer;
	union PS3Device *devs[PS3_MAX_CHANNEL_NUM];
};

struct ps3_pd_entry {
	struct PS3DiskDevPos disk_pos;
	U8 state;
	U8 config_flag;
	U8 RWCT;
	U8 scsi_interface_type;
	U8 task_abort_timeout;
	U8 task_reset_timeout;
	U8 dev_type;
	union {
		struct {
			U8 support_ncq:1;       
			U8 protect:1;           
			U8 reserved:6;          
		};
		U8 pd_flags;                
	};
	U32 max_io_size;
	U32 dev_queue_depth;
	U8 encl_id;
	U8 phy_id;
	U16 sector_size;
	U16 dma_addr_alignment;
	U16 dma_len_alignment;
	struct sas_rphy *sas_rphy;
	U16 normal_quota;
	U16 direct_quota;
};

struct ps3_pd_table {
	U16 *pd_idxs_array;
	U16 *pd_idxs[PS3_MAX_CHANNEL_NUM];
};

struct ps3_vd_table {
	U16 *vd_idxs_array;
	U16 *vd_idxs[PS3_MAX_CHANNEL_NUM];
};

struct ps3_pri_data_table {
	struct ps3_scsi_priv_data **vd_pri_data_idxs_array;
	struct ps3_scsi_priv_data **vd_pri_data_idxs[PS3_MAX_CHANNEL_NUM];
};

struct ps3_dev_context {
	struct ps3_vd_table     vd_table[PS3_VD_TABLE_NUM];
	struct ps3_pd_table     pd_table;
	struct PS3VDEntry       *vd_entries_array[PS3_VD_TABLE_NUM];
	struct ps3_pd_entry     *pd_entries_array;
	struct ps3_channel channel_pd[PS3_MAX_CHANNEL_NUM];
	struct ps3_channel channel_vd[PS3_MAX_CHANNEL_NUM];
	struct ps3_pri_data_table vd_pri_data_table;

	U16 max_dev_in_channel[PS3_MAX_CHANNEL_NUM];
	volatile U8 subwork;
	U8 reserved[3];
	struct ps3_dev_pool    vd_pool;
	struct ps3_dev_pool    pd_pool;

	dma_addr_t		pd_list_buf_phys;
	struct PS3DevList	*pd_list_buf;
	dma_addr_t		vd_list_buf_phys;
	struct PS3DevList	*vd_list_buf;
	dma_addr_t		pd_info_buf_phys;
	struct PS3PDInfo	*pd_info_buf;
	dma_addr_t		vd_info_buf_phys_sync;
	struct PS3VDInfo	*vd_info_buf_sync;
	dma_addr_t		vd_info_buf_phys_async;
	struct PS3VDInfo	*vd_info_buf_async;

	struct ps3_cmd		*vd_pending_cmd;
	struct ps3_cmd 		*vdpending_abort_cmd;  
	volatile U32 abort_vdpending_cmd; 
    ps3_atomic32        is_vdpending_abort;    

	U16 total_vd_count;
	U16 total_pd_count;
	U16 max_dev_per_channel;
	U8 vd_table_idx;
	U8 pd_channel_count;
	U8 vd_channel_count;
	ps3_mutex dev_priv_lock;
#ifdef _WINDOWS
	U8 channel_map_rang_num;
	U8 total_os_channel;
	struct ps3_windows_private_table windows_table;

	U8 reserved2[3];
	KEVENT disk_sync;
	ps3_spinlock dev_lock;
#else
	U8 reserved2[7];
#endif
	ps3_mutex dev_scan_lock;
};

S32 ps3_dev_mgr_cli_register(void);

S32 ps3_device_mgr_init(struct ps3_instance *instance);

void ps3_device_mgr_exit(struct ps3_instance *instance);

S32 ps3_device_mgr_data_init(struct ps3_instance *instance);

S32 ps3_device_mgr_data_exit(struct ps3_instance *instance);

S32 ps3_dev_mgr_vd_info_subscribe(struct ps3_instance *instance);

S32 ps3_dev_mgr_vd_info_unsubscribe(struct ps3_instance *instance);
S32 ps3_dev_mgr_vd_info_resubscribe(struct ps3_instance *instance);

void ps3_dev_mgr_vd_info_clear(struct ps3_instance *instance);

S32 ps3_dev_mgr_pd_info_get(struct ps3_instance *instance, U16 channel,
	U16 target_id, U16 pd_id);

struct ps3_pd_entry *ps3_dev_mgr_pd_info_find_by_id(
	struct ps3_instance *instance, U16 disk_id);

S32 ps3_dev_mgr_pd_list_get(struct ps3_instance *instance);

S32 ps3_dev_mgr_vd_list_get(struct ps3_instance *instance);

S32 ps3_vd_info_get_all(struct ps3_instance *instance);

static inline U16 get_offset_of_vdid(U16 offsetOfVDID, U16 virtDiskID)
{
	U16 virtDiskIdx = PS3_INVALID_DEV_ID;

	if ( virtDiskID >= offsetOfVDID ) {
		virtDiskIdx = virtDiskID - offsetOfVDID;
	}

	return virtDiskIdx;
}

Bool ps3_dev_id_valid_check(struct ps3_instance *instance, U8 channel,
	U16 target_id, U8 dev_type);

U8 ps3_get_vd_raid_level(
	struct ps3_instance *instance, U8 channel, U16 target_id);

struct ps3_scsi_priv_data *ps3_dev_mgr_lookup_vd_pri_data(
	struct ps3_instance *instance, U8 channel, U16 target_id);

struct PS3VDEntry *ps3_dev_mgr_lookup_vd_info(
	struct ps3_instance *instance, U8 channel, U16 target_id);

struct ps3_pd_entry *ps3_dev_mgr_lookup_pd_info(
	struct ps3_instance *instance, U8 channel, U16 target_id);

struct PS3VDEntry *ps3_dev_mgr_lookup_vd_info_by_id(
	struct ps3_instance *instance, U16 disk_id);

struct ps3_pd_entry *ps3_dev_mgr_lookup_pd_info_by_id(
	struct ps3_instance *instance, U16 disk_id);

union PS3Device* ps3_dev_mgr_lookup_vd_list(
	struct ps3_instance *instance, U8 channel, U16 target_id);

union PS3Device* ps3_dev_mgr_lookup_pd_list(
	struct ps3_instance *instance, U8 channel, U16 target_id);

#ifndef _WINDOWS
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)
S32 ps3_change_queue_depth(struct scsi_device *sdev,
	S32 queue_depth, S32 reason);
#else
S32 ps3_change_queue_depth(struct scsi_device *sdev,
	S32 queue_depth);
#endif

S32 ps3_scsi_slave_alloc(struct scsi_device *sdev);

void ps3_scsi_slave_destroy(struct scsi_device *sdev);

S32 ps3_scsi_slave_configure(struct scsi_device *sdev);
#endif
static inline Bool ps3_check_pd_is_vd_member(U8 config_flag)
{
	return (config_flag != MIC_PD_STATE_JBOD &&
		config_flag != MIC_PD_STATE_READY &&
		config_flag != MIC_PD_STATE_UNKNOWN
		);
}

static inline U8 ps3_get_converted_dev_type(U8 driverType, U8 mediumType)
{
	U8 dev_type = PS3_DEV_TYPE_UNKNOWN;

	switch (driverType) {
	case DRIVER_TYPE_SAS:
		if (mediumType == DEVICE_TYPE_HDD) {
			dev_type = PS3_DEV_TYPE_SAS_HDD;
		} else if (mediumType == DEVICE_TYPE_SSD) {
			dev_type = PS3_DEV_TYPE_SAS_SSD;
		}
		break;
	case DRIVER_TYPE_SATA:
		if (mediumType == DEVICE_TYPE_HDD) {
			dev_type = PS3_DEV_TYPE_SATA_HDD;
		} else if (mediumType == DEVICE_TYPE_SSD) {
			dev_type = PS3_DEV_TYPE_SATA_SSD;
		}
		break;
	case DRIVER_TYPE_SES:
		if (mediumType == DEVICE_TYPE_ENCLOSURE) {
			dev_type = PS3_DEV_TYPE_SES;
		}
		break;
	case DRIVER_TYPE_VEP:
		if (mediumType == DEVICE_TYPE_ENCLOSURE) {
			dev_type = PS3_DEV_TYPE_VEP;
		}
		break;
	case DRIVER_TYPE_NVME:
		if (mediumType == DEVICE_TYPE_SSD) {
			dev_type = PS3_DEV_TYPE_NVME_SSD;
		}
		break;
	default:
		break;
	}
	return dev_type;
};

void ps3_change_sdev_max_sector(struct ps3_instance *instance,
	struct PS3VDEntry *vd_entry);

void ps3_vd_info_show(const struct ps3_instance *instance,
	const struct PS3VDEntry *vd_entry);
S32 ps3_adjust_queue_depth(struct ps3_instance *instance,
	U8 dev_type, U32 queue_depth);

static inline const char *ps3_get_vd_access_plolicy_str(VDAccessPolicy_e policy)
{
	static const char *vdAccessPolicyName[] = {
		[VD_ACCESS_POLICY_READ_WRITE]         = "RW",
		[VD_ACCESS_POLICY_READ_ONLY]          = "RO",
		[VD_ACCESS_POLICY_BLOCK]              = "BLOCK",
		[VD_ACCESS_POLICY_REMOVE_ACCESS]      = "REMOVE_ACCESS",
	};

	return (policy < ARRAY_SIZE(vdAccessPolicyName)) ?
		vdAccessPolicyName[policy] : "Unknown";
}

static inline Bool ps3_is_hdd_pd(U8 dev_type)
{
	return (dev_type == PS3_DEV_TYPE_SAS_HDD || dev_type == PS3_DEV_TYPE_SATA_HDD);
}

void ps3_vd_busy_scale_get(struct PS3VDEntry *vd_entry);
void ps3_sdev_bdi_stable_writes_set(struct ps3_instance *instance, struct scsi_device *sdev);

void ps3_sdev_bdi_stable_writes_clear(struct ps3_instance *instance, struct scsi_device *sdev);
S32 ps3_sdev_bdi_stable_writes_get(struct scsi_device *sdev);

#endif
