/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 Huawei Technologies Co., Ltd */

#ifndef __HIRAID_H_
#define __HIRAID_H_

#define HIRAID_HDD_PD_QD 64
#define HIRAID_HDD_VD_QD 256
#define HIRAID_SSD_PD_QD 64
#define HIRAID_SSD_VD_QD 256

#define BGTASK_TYPE_REBUILD 4
#define USR_CMD_READ 0xc2
#define USR_CMD_RDLEN 0x1000
#define USR_CMD_VDINFO 0x704
#define USR_CMD_BGTASK 0x504
#define VDINFO_PARAM_LEN 0x04

#define HIRAID_DEFAULT_MAX_CHANNEL 4
#define HIRAID_DEFAULT_MAX_ID 240
#define HIRAID_DEFAULT_MAX_LUN_PER_HOST 8

#define FUA_MASK 0x08

#define HIRAID_IO_SQES 7
#define HIRAID_IO_CQES 4
#define PRP_ENTRY_SIZE 8

#define EXTRA_POOL_SIZE 256
#define MAX_EXTRA_POOL_NUM 16
#define MAX_CMD_PER_DEV 64
#define MAX_CDB_LEN 16

#define HIRAID_AQ_DEPTH 128
#define HIRAID_ASYN_COMMANDS 16
#define HIRAID_AQ_BLK_MQ_DEPTH (HIRAID_AQ_DEPTH - HIRAID_ASYN_COMMANDS)
#define HIRAID_AQ_MQ_TAG_DEPTH (HIRAID_AQ_BLK_MQ_DEPTH - 1)

#define HIRAID_ADMIN_QUEUE_NUM 1
#define HIRAID_PTHRU_CMDS_PERQ 1
#define HIRAID_TOTAL_PTCMDS(qn) (HIRAID_PTHRU_CMDS_PERQ * (qn))

#define HIRAID_DEV_INFO_ATTR_BOOT(attr) ((attr) & 0x01)
#define HIRAID_DEV_INFO_ATTR_VD(attr) (((attr) & 0x02) == 0x0)
#define HIRAID_DEV_INFO_ATTR_PT(attr) (((attr) & 0x22) == 0x02)
#define HIRAID_DEV_INFO_ATTR_RAWDISK(attr) ((attr) & 0x20)
#define HIRAID_DEV_DISK_TYPE(attr) ((attr) & 0x1e)

#define HIRAID_DEV_INFO_FLAG_VALID(flag) ((flag) & 0x01)
#define HIRAID_DEV_INFO_FLAG_CHANGE(flag) ((flag) & 0x02)

#define HIRAID_CAP_MQES(cap) ((cap) & 0xffff)
#define HIRAID_CAP_STRIDE(cap) (((cap) >> 32) & 0xf)
#define HIRAID_CAP_MPSMIN(cap) (((cap) >> 48) & 0xf)
#define HIRAID_CAP_MPSMAX(cap) (((cap) >> 52) & 0xf)
#define HIRAID_CAP_TIMEOUT(cap) (((cap) >> 24) & 0xff)
#define HIRAID_CAP_DMAMASK(cap) (((cap) >> 37) & 0xff)

#define IO_SQE_SIZE sizeof(struct hiraid_scsi_io_cmd)
#define ADMIN_SQE_SIZE sizeof(struct hiraid_admin_command)
#define SQE_SIZE(qid) (((qid) > 0) ? IO_SQE_SIZE : ADMIN_SQE_SIZE)
#define CQ_SIZE(depth) ((depth) * sizeof(struct hiraid_completion))
#define SQ_SIZE(qid, depth) ((depth) * SQE_SIZE(qid))

#define SENSE_SIZE(depth)	((depth) * SCSI_SENSE_BUFFERSIZE)

#define IO_6_DEFAULT_TX_LEN 256

#define MAX_DEV_ENTRY_PER_PAGE_4K	340

#define MAX_REALTIME_BGTASK_NUM 32

#define PCI_VENDOR_ID_HUAWEI_LOGIC 0x19E5
#define HIRAID_SERVER_DEVICE_HBA_DID	0x3858
#define HIRAID_SERVER_DEVICE_HBAS_DID	0x3918
#define HIRAID_SERVER_DEVICE_RAID_DID	0x3758
#define HIRAID_SERVER_DEVICE_RAIDS_DID	0x38D8


enum {
	HIRAID_SC_SUCCESS = 0x0,
	HIRAID_SC_INVALID_OPCODE = 0x1,
	HIRAID_SC_INVALID_FIELD  = 0x2,

	HIRAID_SC_ABORT_LIMIT = 0x103,
	HIRAID_SC_ABORT_MISSING = 0x104,
	HIRAID_SC_ASYNC_LIMIT = 0x105,

	HIRAID_SC_DNR = 0x4000,
};

enum {
	HIRAID_REG_CAP  = 0x0000,
	HIRAID_REG_CC   = 0x0014,
	HIRAID_REG_CSTS = 0x001c,
	HIRAID_REG_AQA  = 0x0024,
	HIRAID_REG_ASQ  = 0x0028,
	HIRAID_REG_ACQ  = 0x0030,
	HIRAID_REG_DBS  = 0x1000,
};

enum {
	HIRAID_CC_ENABLE     = 1 << 0,
	HIRAID_CC_CSS_NVM    = 0 << 4,
	HIRAID_CC_MPS_SHIFT  = 7,
	HIRAID_CC_AMS_SHIFT  = 11,
	HIRAID_CC_SHN_SHIFT  = 14,
	HIRAID_CC_IOSQES_SHIFT = 16,
	HIRAID_CC_IOCQES_SHIFT = 20,
	HIRAID_CC_AMS_RR       = 0 << HIRAID_CC_AMS_SHIFT,
	HIRAID_CC_SHN_NONE     = 0 << HIRAID_CC_SHN_SHIFT,
	HIRAID_CC_IOSQES       = HIRAID_IO_SQES << HIRAID_CC_IOSQES_SHIFT,
	HIRAID_CC_IOCQES       = HIRAID_IO_CQES << HIRAID_CC_IOCQES_SHIFT,
	HIRAID_CC_SHN_NORMAL   = 1 << HIRAID_CC_SHN_SHIFT,
	HIRAID_CC_SHN_MASK     = 3 << HIRAID_CC_SHN_SHIFT,
	HIRAID_CSTS_CFS_SHIFT  = 1,
	HIRAID_CSTS_SHST_SHIFT = 2,
	HIRAID_CSTS_PP_SHIFT   = 5,
	HIRAID_CSTS_RDY	       = 1 << 0,
	HIRAID_CSTS_SHST_CMPLT = 2 << 2,
	HIRAID_CSTS_SHST_MASK  = 3 << 2,
	HIRAID_CSTS_CFS_MASK   = 1 << HIRAID_CSTS_CFS_SHIFT,
	HIRAID_CSTS_PP_MASK    = 1 << HIRAID_CSTS_PP_SHIFT,
};

enum {
	HIRAID_ADMIN_DELETE_SQ = 0x00,
	HIRAID_ADMIN_CREATE_SQ = 0x01,
	HIRAID_ADMIN_DELETE_CQ = 0x04,
	HIRAID_ADMIN_CREATE_CQ = 0x05,
	HIRAID_ADMIN_ABORT_CMD = 0x08,
	HIRAID_ADMIN_SET_FEATURES = 0x09,
	HIRAID_ADMIN_ASYNC_EVENT = 0x0c,
	HIRAID_ADMIN_GET_INFO = 0xc6,
	HIRAID_ADMIN_RESET = 0xc8,
};

enum {
	HIRAID_GET_CTRL_INFO = 0,
	HIRAID_GET_DEVLIST_INFO = 1,
};

enum hiraid_rst_type {
	HIRAID_RESET_TARGET = 0,
	HIRAID_RESET_BUS = 1,
};

enum {
	HIRAID_ASYN_EVENT_ERROR = 0,
	HIRAID_ASYN_EVENT_NOTICE = 2,
	HIRAID_ASYN_EVENT_VS = 7,
};

enum {
	HIRAID_ASYN_DEV_CHANGED = 0x00,
	HIRAID_ASYN_FW_ACT_START = 0x01,
	HIRAID_ASYN_HOST_PROBING = 0x10,
};

enum {
	HIRAID_ASYN_TIMESYN = 0x00,
	HIRAID_ASYN_FW_ACT_FINISH = 0x02,
	HIRAID_ASYN_EVENT_MIN = 0x80,
	HIRAID_ASYN_EVENT_MAX = 0xff,
};

enum {
	HIRAID_CMD_WRITE = 0x01,
	HIRAID_CMD_READ = 0x02,

	HIRAID_CMD_NONRW_NONE = 0x80,
	HIRAID_CMD_NONRW_TODEV = 0x81,
	HIRAID_CMD_NONRW_FROMDEV = 0x82,
};

enum {
	HIRAID_QUEUE_PHYS_CONTIG = (1 << 0),
	HIRAID_CQ_IRQ_ENABLED = (1 << 1),

	HIRAID_FEATURE_NUM_QUEUES = 0x07,
	HIRAID_FEATURE_ASYNC_EVENT = 0x0b,
	HIRAID_FEATURE_TIMESTAMP = 0x0e,
};

enum hiraid_dev_state {
	DEV_NEW,
	DEV_LIVE,
	DEV_RESETTING,
	DEV_DELETING,
	DEV_DEAD,
};

enum {
	HIRAID_CARD_HBA,
	HIRAID_CARD_RAID,
};

enum hiraid_cmd_type {
	HIRAID_CMD_ADMIN,
	HIRAID_CMD_PTHRU,
};

enum {
	SQE_FLAG_SGL_METABUF = (1 << 6),
	SQE_FLAG_SGL_METASEG = (1 << 7),
	SQE_FLAG_SGL_ALL     = SQE_FLAG_SGL_METABUF | SQE_FLAG_SGL_METASEG,
};

enum hiraid_cmd_state {
	CMD_IDLE = 0,
	CMD_FLIGHT = 1,
	CMD_COMPLETE = 2,
	CMD_TIMEOUT = 3,
	CMD_TMO_COMPLETE = 4,
};

enum {
	HIRAID_BSG_ADMIN,
	HIRAID_BSG_IOPTHRU,
};

enum {
	HIRAID_SAS_HDD_VD  = 0x04,
	HIRAID_SATA_HDD_VD = 0x08,
	HIRAID_SAS_SSD_VD  = 0x0c,
	HIRAID_SATA_SSD_VD = 0x10,
	HIRAID_NVME_SSD_VD = 0x14,
	HIRAID_SAS_HDD_PD  = 0x06,
	HIRAID_SATA_HDD_PD = 0x0a,
	HIRAID_SAS_SSD_PD  = 0x0e,
	HIRAID_SATA_SSD_PD = 0x12,
	HIRAID_NVME_SSD_PD = 0x16,
};

enum {
	DISPATCH_BY_CPU,
	DISPATCH_BY_DISK,
};

struct hiraid_completion {
	__le32 result;
	union {
		struct {
			__u8	sense_len;
			__u8	resv[3];
		};
		__le32	result1;
	};
	__le16 sq_head;
	__le16 sq_id;
	__le16 cmd_id;
	__le16 status;
};

struct hiraid_ctrl_info {
	__le32 nd;
	__le16 max_cmds;
	__le16 max_channel;
	__le32 max_tgt_id;
	__le16 max_lun;
	__le16 max_num_sge;
	__le16 lun_num_boot;
	__u8   mdts;
	__u8   acl;
	__u8   asynevent;
	__u8   card_type;
	__u8   pt_use_sgl;
	__u8   rsvd;
	__le32 rtd3e;
	__u8   sn[32];
	__u8   fw_version[16];
	__u8   rsvd1[4020];
};

struct hiraid_dev {
	struct pci_dev *pdev;
	struct device *dev;
	struct Scsi_Host *shost;
	struct hiraid_queue *queues;
	struct dma_pool *prp_page_pool;
	struct dma_pool *prp_extra_pool[MAX_EXTRA_POOL_NUM];
	void __iomem *bar;
	u32 max_qid;
	u32 num_vecs;
	u32 queue_count;
	u32 ioq_depth;
	u32 db_stride;
	u32 __iomem *dbs;
	struct rw_semaphore dev_rwsem;
	int numa_node;
	u32 page_size;
	u32 ctrl_config;
	u32 online_queues;
	u64 cap;
	u32 scsi_qd;
	u32 instance;
	struct hiraid_ctrl_info *ctrl_info;
	struct hiraid_dev_info *dev_info;

	struct hiraid_cmd *adm_cmds;
	struct list_head adm_cmd_list;
	spinlock_t adm_cmd_lock;

	struct hiraid_cmd *io_ptcmds;
	struct list_head io_pt_list;
	spinlock_t io_pt_lock;

	struct work_struct scan_work;
	struct work_struct timesyn_work;
	struct work_struct reset_work;
	struct work_struct fwact_work;

	enum hiraid_dev_state state;
	spinlock_t state_lock;

	void *sense_buffer_virt;
	dma_addr_t sense_buffer_phy;
	u32 last_qcnt;
	u8 hdd_dispatch;

	struct request_queue *bsg_queue;
};

struct hiraid_sgl_desc {
	__le64 addr;
	__le32 length;
	__u8   rsvd[3];
	__u8   type;
};

union hiraid_data_ptr {
	struct {
		__le64 prp1;
		__le64 prp2;
	};
	struct hiraid_sgl_desc sgl;
};

struct hiraid_admin_com_cmd {
	__u8	opcode;
	__u8	flags;
	__le16	cmd_id;
	__le32	hdid;
	__le32	cdw2[4];
	union hiraid_data_ptr	dptr;
	__le32	cdw10;
	__le32	cdw11;
	__le32	cdw12;
	__le32	cdw13;
	__le32	cdw14;
	__le32	cdw15;
};

struct hiraid_features {
	__u8	opcode;
	__u8	flags;
	__le16	cmd_id;
	__le32	hdid;
	__u64	rsvd2[2];
	union hiraid_data_ptr dptr;
	__le32	fid;
	__le32	dword11;
	__le32	dword12;
	__le32	dword13;
	__le32	dword14;
	__le32	dword15;
};

struct hiraid_create_cq {
	__u8	opcode;
	__u8	flags;
	__le16	cmd_id;
	__u32	rsvd1[5];
	__le64	prp1;
	__u64	rsvd8;
	__le16	cqid;
	__le16	qsize;
	__le16	cq_flags;
	__le16	irq_vector;
	__u32	rsvd12[4];
};

struct hiraid_create_sq {
	__u8	opcode;
	__u8	flags;
	__le16	cmd_id;
	__u32	rsvd1[5];
	__le64	prp1;
	__u64	rsvd8;
	__le16	sqid;
	__le16	qsize;
	__le16	sq_flags;
	__le16	cqid;
	__u32	rsvd12[4];
};

struct hiraid_delete_queue {
	__u8	opcode;
	__u8	flags;
	__le16	cmd_id;
	__u32	rsvd1[9];
	__le16	qid;
	__u16	rsvd10;
	__u32	rsvd11[5];
};

struct hiraid_get_info {
	__u8	opcode;
	__u8	flags;
	__le16	cmd_id;
	__le32	hdid;
	__u32	rsvd2[4];
	union hiraid_data_ptr	dptr;
	__u8	type;
	__u8	rsvd10[3];
	__le32	cdw11;
	__u32	rsvd12[4];
};

struct hiraid_usr_cmd {
	__u8	opcode;
	__u8	flags;
	__le16	cmd_id;
	__le32	hdid;
	union {
		struct {
			__le16 subopcode;
			__le16 rsvd1;
		} info_0;
		__le32 cdw2;
	};
	union {
		struct {
			__le16 data_len;
			__le16 param_len;
		} info_1;
		__le32 cdw3;
	};
	__u64 metadata;
	union hiraid_data_ptr	dptr;
	__le32 cdw10;
	__le32 cdw11;
	__le32 cdw12;
	__le32 cdw13;
	__le32 cdw14;
	__le32 cdw15;
};

struct hiraid_abort_cmd {
	__u8	opcode;
	__u8	flags;
	__le16	cmd_id;
	__le32	hdid;
	__u64	rsvd2[4];
	__le16	sqid;
	__le16	cid;
	__u32	rsvd11[5];
};

struct hiraid_reset_cmd {
	__u8	opcode;
	__u8	flags;
	__le16	cmd_id;
	__le32	hdid;
	__u64	rsvd2[4];
	__u8	type;
	__u8	rsvd10[3];
	__u32	rsvd11[5];
};

struct hiraid_admin_command {
	union {
		struct hiraid_admin_com_cmd common;
		struct hiraid_features features;
		struct hiraid_create_cq create_cq;
		struct hiraid_create_sq create_sq;
		struct hiraid_delete_queue delete_queue;
		struct hiraid_get_info get_info;
		struct hiraid_abort_cmd abort;
		struct hiraid_reset_cmd reset;
		struct hiraid_usr_cmd usr_cmd;
	};
};

struct hiraid_scsi_io_com_cmd {
	__u8	opcode;
	__u8	flags;
	__le16	cmd_id;
	__le32	hdid;
	__le16	sense_len;
	__u8	cdb_len;
	__u8	rsvd2;
	__le32	cdw3[3];
	union hiraid_data_ptr	dptr;
	__le32	cdw10[6];
	__u8	cdb[32];
	__le64	sense_addr;
	__le32	cdw26[6];
};

struct hiraid_scsi_rw_cmd {
	__u8	opcode;
	__u8	flags;
	__le16	cmd_id;
	__le32	hdid;
	__le16	sense_len;
	__u8	cdb_len;
	__u8	rsvd2;
	__u32	rsvd3[3];
	union hiraid_data_ptr	dptr;
	__le64	slba;
	__le16	nlb;
	__le16	control;
	__u32	rsvd13[3];
	__u8	cdb[32];
	__le64	sense_addr;
	__u32	rsvd26[6];
};

struct hiraid_scsi_nonrw_cmd {
	__u8	opcode;
	__u8	flags;
	__le16	cmd_id;
	__le32	hdid;
	__le16	sense_len;
	__u8	cdb_length;
	__u8	rsvd2;
	__u32	rsvd3[3];
	union hiraid_data_ptr	dptr;
	__u32	rsvd10[5];
	__le32	buf_len;
	__u8	cdb[32];
	__le64	sense_addr;
	__u32	rsvd26[6];
};

struct hiraid_scsi_io_cmd {
	union {
		struct hiraid_scsi_io_com_cmd common;
		struct hiraid_scsi_rw_cmd rw;
		struct hiraid_scsi_nonrw_cmd nonrw;
	};
};

struct hiraid_passthru_common_cmd {
	__u8	opcode;
	__u8	flags;
	__u16	rsvd0;
	__u32	nsid;
	union {
		struct {
			__u16 subopcode;
			__u16 rsvd1;
		} info_0;
		__u32 cdw2;
	};
	union {
		struct {
			__u16 data_len;
			__u16 param_len;
		} info_1;
		__u32 cdw3;
	};
	__u64 metadata;

	__u64 addr;
	__u64 prp2;

	__u32 cdw10;
	__u32 cdw11;
	__u32 cdw12;
	__u32 cdw13;
	__u32 cdw14;
	__u32 cdw15;
	__u32 timeout_ms;
	__u32 result0;
	__u32 result1;
};

struct hiraid_passthru_io_cmd {
	__u8  opcode;
	__u8  flags;
	__u16 rsvd0;
	__u32 nsid;
	union {
		struct {
			__u16 res_sense_len;
			__u8  cdb_len;
			__u8  rsvd0;
		} info_0;
		__u32 cdw2;
	};
	union {
		struct {
			__u16 subopcode;
			__u16 rsvd1;
		} info_1;
		__u32 cdw3;
	};
	union {
		struct {
			__u16 rsvd;
			__u16 param_len;
		} info_2;
		__u32 cdw4;
	};
	__u32 cdw5;
	__u64 addr;
	__u64 prp2;
	union {
		struct {
			__u16 eid;
			__u16 sid;
		} info_3;
		__u32 cdw10;
	};
	union {
		struct {
			__u16 did;
			__u8  did_flag;
			__u8  rsvd2;
		} info_4;
		__u32 cdw11;
	};
	__u32 cdw12;
	__u32 cdw13;
	__u32 cdw14;
	__u32 data_len;
	__u32 cdw16;
	__u32 cdw17;
	__u32 cdw18;
	__u32 cdw19;
	__u32 cdw20;
	__u32 cdw21;
	__u32 cdw22;
	__u32 cdw23;
	__u64 sense_addr;
	__u32 cdw26[4];
	__u32 timeout_ms;
	__u32 result0;
	__u32 result1;
};

struct hiraid_bsg_request {
	u32  msgcode;
	u32 control;
	union {
		struct hiraid_passthru_common_cmd admcmd;
		struct hiraid_passthru_io_cmd   pthrucmd;
	};
};

struct hiraid_cmd {
	u16 qid;
	u16 cid;
	u32 result0;
	u32 result1;
	u16 status;
	void *priv;
	enum hiraid_cmd_state state;
	struct completion cmd_done;
	struct list_head list;
};

struct hiraid_queue {
	struct hiraid_dev *hdev;
	spinlock_t sq_lock;

	spinlock_t cq_lock ____cacheline_aligned_in_smp;

	void *sq_cmds;

	struct hiraid_completion *cqes;

	dma_addr_t sq_buffer_phy;
	dma_addr_t cq_buffer_phy;
	u32 __iomem *q_db;
	u8 cq_phase;
	u8 sqes;
	u16 qid;
	u16 sq_tail;
	u16 cq_head;
	u16 last_cq_head;
	u16 q_depth;
	s16 cq_vector;
	atomic_t inflight;
	void *sense_buffer_virt;
	dma_addr_t sense_buffer_phy;
	struct dma_pool *prp_small_pool;
};

struct hiraid_mapmange {
	struct hiraid_queue *hiraidq;
	enum hiraid_cmd_state state;
	u16 cid;
	int page_cnt;
	u32 sge_cnt;
	u32 len;
	bool use_sgl;
	u32 cdb_data_len;
	dma_addr_t first_dma;
	void *sense_buffer_virt;
	dma_addr_t sense_buffer_phy;
	struct scatterlist *sgl;
	void *list[];
};

struct hiraid_vd_info {
	__u8 name[32];
	__le16 id;
	__u8 rg_id;
	__u8 rg_level;
	__u8 sg_num;
	__u8 sg_disk_num;
	__u8 vd_status;
	__u8 vd_type;
	__u8 rsvd1[4056];
};

struct bgtask_info {
	__u8 type;
	__u8 progress;
	__u8 rate;
	__u8 rsvd0;
	__le16 vd_id;
	__le16 time_left;
	__u8 rsvd1[4];
};

struct hiraid_bgtask {
	__u8 sw;
	__u8 task_num;
	__u8 rsvd[6];
	struct bgtask_info bgtask[MAX_REALTIME_BGTASK_NUM];
};

struct hiraid_dev_info {
	__le32	hdid;
	__le16	target;
	__u8	channel;
	__u8	lun;
	__u8	attr;
	__u8	flag;
	__le16	max_io_kb;
};

struct hiraid_dev_list {
	__le32	dev_num;
	__u32	rsvd0[3];
	struct hiraid_dev_info	devinfo[MAX_DEV_ENTRY_PER_PAGE_4K];
};

struct hiraid_sdev_hostdata {
	u32 hdid;
	u16 max_io_kb;
	u8 attr;
	u8 flag;
	u8 rg_id;
	u8 hwq;
	u16 pend_count;
};

#endif