/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 3SNIC Information Technology, Ltd */

/* 3SNIC RAID SSSXXX Series Linux Driver */

#ifndef __SSSRAID_H_
#define __SSSRAID_H_

#define SSSRAID_DRIVER_VERSION	"1.0.0.0"
#define SSSRAID_DRIVER_RELDATE	"12-December-2022"

#define SSSRAID_DRIVER_NAME	"sssraid"

#define SSSRAID_NAME_LENGTH 32
#define BSG_NAME_SIZE 15
/*
 * SSSRAID Vendor ID and Device IDs
 */
#define PCI_VENDOR_ID_3SNIC_LOGIC		0x1F3F

#define SSSRAID_SERVER_DEVICE_HBA_DID		0x2100
#define SSSRAID_SERVER_DEVICE_RAID_DID		0x2200

#define SSSRAID_CAP_MQES(cap) ((cap) & 0xffff)
#define SSSRAID_CAP_STRIDE(cap) (((cap) >> 32) & 0xf)
#define SSSRAID_CAP_MPSMIN(cap) (((cap) >> 48) & 0xf)
#define SSSRAID_CAP_MPSMAX(cap) (((cap) >> 52) & 0xf)
#define SSSRAID_CAP_TIMEOUT(cap) (((cap) >> 24) & 0xff)
#define SSSRAID_CAP_DMAMASK(cap) (((cap) >> 37) & 0xff)

#define SSSRAID_DEFAULT_MAX_CHANNEL 4
#define SSSRAID_DEFAULT_MAX_ID 240
#define SSSRAID_DEFAULT_MAX_LUN_PER_HOST 8
#define MAX_SECTORS 2048

/*
 * Time define
 */
#define SSSRAID_WATCHDOG_INTERVAL		1000 /* in milli seconds */
#define SSSRAID_PORTENABLE_TIMEOUT		300

/*
 * SSSRAID queue and entry size for Admin and I/O type
 */
#define IOCMD_SQE_SIZE sizeof(struct sssraid_ioq_command)
#define ADMIN_SQE_SIZE sizeof(struct sssraid_admin_command)
#define SQE_SIZE(qid) (((qid) > 0) ? IOCMD_SQE_SIZE : ADMIN_SQE_SIZE)
#define CQ_SIZE(depth) ((depth) * sizeof(struct sssraid_completion))
#define SQ_SIZE(qid, depth) ((depth) * SQE_SIZE(qid))

#define SENSE_SIZE(depth)	((depth) * SCSI_SENSE_BUFFERSIZE)

#define SSSRAID_ADMQ_DEPTH 128
#define SSSRAID_NR_AEN_CMDS 16
#define SSSRAID_AMDQ_BLK_MQ_DEPTH (SSSRAID_ADMQ_DEPTH - SSSRAID_NR_AEN_CMDS)
#define SSSRAID_AMDQ_MQ_TAG_DEPTH (SSSRAID_AMDQ_BLK_MQ_DEPTH - 1)

#define SSSRAID_ADM_QUEUE_NUM 1
#define SSSRAID_PTCMDS_PERQ 1
#define SSSRAID_IO_BLK_MQ_DEPTH (sdioc->scsi_qd)
#define SSSRAID_NR_HW_QUEUES (sdioc->init_done_queue_cnt - 1)
#define SSSRAID_NR_IOQ_PTCMDS (SSSRAID_PTCMDS_PERQ * SSSRAID_NR_HW_QUEUES)

#define FUA_MASK 0x08
#define SSSRAID_MINORS BIT(MINORBITS)
#define SSSRAID_RW_FUA	BIT(14)

#define COMMAND_IS_WRITE(cmd) ((cmd)->common.opcode & 1)

#define SSSRAID_IO_IOSQES 7
#define SSSRAID_IO_IOCQES 4
#define PRP_ENTRY_SIZE 8

#define SMALL_POOL_SIZE 256
#define MAX_SMALL_POOL_NUM 16
#define MAX_CMD_PER_DEV 64
#define MAX_CDB_LEN 16

#define SSSRAID_UP_TO_MULTY4(x) (((x) + 4) & (~0x03))

#define CQE_STATUS_SUCCESS (0x0)

#define IO_6_DEFAULT_TX_LEN 256

#define SGES_PER_PAGE    (PAGE_SIZE / sizeof(struct sssraid_sgl_desc))

#define SSSRAID_CAP_TIMEOUT_UNIT_MS	(HZ / 2)

extern u32 admin_tmout;
#define ADMIN_TIMEOUT		(admin_tmout * HZ)

#define SSSRAID_WAIT_ABNL_CMD_TIMEOUT	6

#define SSSRAID_DMA_MSK_BIT_MAX	64

enum {
	SCSI_6_BYTE_CDB_LEN = 6,
	SCSI_10_BYTE_CDB_LEN = 10,
	SCSI_12_BYTE_CDB_LEN = 12,
	SCSI_16_BYTE_CDB_LEN = 16,
};

enum {
	SSSRAID_SGL_FMT_DATA_DESC     = 0x00,
	SSSRAID_SGL_FMT_SEG_DESC      = 0x02,
	SSSRAID_SGL_FMT_LAST_SEG_DESC    = 0x03,
	SSSRAID_KEY_SGL_FMT_DATA_DESC    = 0x04,
	SSSRAID_TRANSPORT_SGL_DATA_DESC  = 0x05
};


enum {
	SSSRAID_REQ_CANCELLED = (1 << 0),
	SSSRAID_REQ_USERCMD = (1 << 1),
};

enum {
	SSSRAID_SC_SUCCESS = 0x0,
	SSSRAID_SC_INVALID_OPCODE = 0x1,
	SSSRAID_SC_INVALID_FIELD  = 0x2,

	SSSRAID_SC_ABORT_LIMIT = 0x103,
	SSSRAID_SC_ABORT_MISSING = 0x104,
	SSSRAID_SC_ASYNC_LIMIT = 0x105,

	SSSRAID_SC_DNR = 0x4000,
};

enum {
	SSSRAID_REG_CAP  = 0x0000,
	SSSRAID_REG_CC   = 0x0014,
	SSSRAID_REG_CSTS = 0x001c,
	SSSRAID_REG_AQA  = 0x0024,
	SSSRAID_REG_ASQ  = 0x0028,
	SSSRAID_REG_ACQ  = 0x0030,
	SSSRAID_REG_DBS  = 0x1000,
};

enum {
	SSSRAID_CC_ENABLE     = 1 << 0,
	SSSRAID_CC_CSS_NVM    = 0 << 4,
	SSSRAID_CC_MPS_SHIFT  = 7,
	SSSRAID_CC_AMS_SHIFT  = 11,
	SSSRAID_CC_SHN_SHIFT  = 14,
	SSSRAID_CC_IOSQES_SHIFT = 16,
	SSSRAID_CC_IOCQES_SHIFT = 20,
	SSSRAID_CC_AMS_RR       = 0 << SSSRAID_CC_AMS_SHIFT,
	SSSRAID_CC_SHN_NONE     = 0 << SSSRAID_CC_SHN_SHIFT,
	SSSRAID_CC_IOSQES       = SSSRAID_IO_IOSQES << SSSRAID_CC_IOSQES_SHIFT,
	SSSRAID_CC_IOCQES       = SSSRAID_IO_IOCQES << SSSRAID_CC_IOCQES_SHIFT,
	SSSRAID_CC_SHN_NORMAL   = 1 << SSSRAID_CC_SHN_SHIFT,
	SSSRAID_CC_SHN_MASK     = 3 << SSSRAID_CC_SHN_SHIFT,
	SSSRAID_CSTS_CFS_SHIFT  = 1,
	SSSRAID_CSTS_SHST_SHIFT = 2,
	SSSRAID_CSTS_PP_SHIFT   = 5,
	SSSRAID_CSTS_RDY	       = 1 << 0,
	SSSRAID_CSTS_SHST_CMPLT = 2 << 2,
	SSSRAID_CSTS_SHST_MASK  = 3 << 2,
	SSSRAID_CSTS_CFS_MASK   = 1 << SSSRAID_CSTS_CFS_SHIFT,
	SSSRAID_CSTS_PP_MASK    = 1 << SSSRAID_CSTS_PP_SHIFT,
};

enum {
	SSSRAID_ADM_DELETE_SQ = 0x00,
	SSSRAID_ADM_CREATE_SQ = 0x01,
	SSSRAID_ADM_DELETE_CQ = 0x04,
	SSSRAID_ADM_CREATE_CQ = 0x05,
	SSSRAID_ADM_ABORT_CMD = 0x08,
	SSSRAID_ADM_SET_FEATURES = 0x09,
	SSSRAID_ADM_ASYNC_EVENT = 0x0c,
	SSSRAID_ADM_GET_INFO = 0xc6,
	SSSRAID_ADM_RESET = 0xc8,
};

enum {
	SSSRAID_GET_INFO_CTRL = 0,
	SSSRAID_GET_INFO_DEV_LIST = 1,
};

enum sssraid_scsi_rst_type {
	SSSRAID_RESET_TARGET = 0,
	SSSRAID_RESET_BUS = 1,
};

enum {
	SSSRAID_AEN_ERROR = 0,
	SSSRAID_AEN_NOTICE = 2,
	SSSRAID_AEN_VS = 7,
};

enum {
	SSSRAID_AEN_DEV_CHANGED = 0x00,
	SSSRAID_AEN_FW_ACT_START = 0x01,
	SSSRAID_AEN_HOST_PROBING = 0x10,
};

enum {
	SSSRAID_AEN_TIMESYN = 0x00,
	SSSRAID_AEN_FW_ACT_FINISH = 0x02,
	SSSRAID_AEN_EVENT_MIN = 0x80,
	SSSRAID_AEN_EVENT_MAX = 0xff,
};

enum {
	SSSRAID_IOCMD_WRITE = 0x01,
	SSSRAID_IOCMD_READ = 0x02,

	SSSRAID_IOCMD_NONRW_NODIR = 0x80,
	SSSRAID_IOCMD_NONRW_TODEV = 0x81,
	SSSRAID_IOCMD_NONRW_FROMDEV = 0x82,
};

enum {
	SSSRAID_QUEUE_PHYS_CONTIG = (1 << 0),
	SSSRAID_CQ_IRQ_ENABLED = (1 << 1),

	SSSRAID_FEAT_NUM_QUEUES = 0x07,
	SSSRAID_FEAT_ASYNC_EVENT = 0x0b,
	SSSRAID_FEAT_TIMESTAMP = 0x0e,
};

enum sssraid_state {
	SSSRAID_NEW,
	SSSRAID_LIVE,
	SSSRAID_RESETTING,
	SSSRAID_DELETING,
	SSSRAID_DEAD,
};

enum {
	SSSRAID_CARD_HBA,
	SSSRAID_CARD_RAID,
};

enum sssraid_cmd_type {
	SSSRAID_CMD_ADM,
	SSSRAID_CMD_IOPT,
};

/*
 * SSSRAID completion queue entry struct
 */
struct sssraid_completion {
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

/*
 * SSSRAID firmware controller properties
 */
struct sssraid_ctrl_info {
	__le32 nd;
	__le16 max_cmds;
	__le16 max_channel;
	__le32 max_tgt_id;
	__le16 max_lun;
	__le16 max_num_sge;
	__le16 lun_num_in_boot;
	__u8   mdts;
	__u8   acl;
	__u8   aerl;
	__u8   card_type;
	__u16  rsvd;
	__le32 rtd3e;
	__u8   sn[32];
	__u8   fr[16];
	__u8   rsvd1[4020];
};

struct sssraid_intr_info {
	struct sssraid_ioc *sdioc;
	u16 msix_index;
	struct sssraid_cqueue *cqinfo;
	char name[SSSRAID_NAME_LENGTH];
};

struct sssraid_fwevt {
	struct list_head list;
	struct work_struct work;
	struct sssraid_ioc *sdioc;
	u16 event_id;
	bool send_ack;
	bool process_evt;
	u32 evt_ctx;
	struct kref ref_count;
	char event_data[0] __aligned(4);
};

/*
 * SSSRAID private device struct definition
 */
struct sssraid_ioc {
	struct pci_dev *pdev;
	struct Scsi_Host *shost;
	struct sssraid_squeue *sqinfo;
	struct sssraid_cqueue *cqinfo;
	struct dma_pool *prp_page_pool;
	struct dma_pool *prp_small_pool[MAX_SMALL_POOL_NUM];
	void __iomem *bar;

	u32 init_done_queue_cnt;
	u32 ioq_depth;
	u32 db_stride;
	u32 __iomem *dbs;
	struct rw_semaphore devices_rwsem;
	int numa_node;
	u32 page_size;
	u32 ctrl_config;
	u64 cap;
	u32 instance;
	u32 scsi_qd;
	struct sssraid_ctrl_info *ctrl_info;
	struct sssraid_dev_info *devices;

	int logging_level;

	char name[SSSRAID_NAME_LENGTH];
	int cpu_count;
	/*
	 * before_affinity_msix_cnt is
	 * min("FW support IO Queue count", num_online_cpus)+1
	 */
	u16 before_affinity_msix_cnt;

	struct sssraid_cmd *adm_cmds;
	struct list_head adm_cmd_list;
	spinlock_t adm_cmd_lock;

	struct sssraid_cmd *ioq_ptcmds;
	struct list_head ioq_pt_list;
	spinlock_t ioq_pt_lock;

	int reset_flag;

	enum sssraid_state state;
	spinlock_t state_lock;

	struct request_queue *bsg_queue;

	u8 intr_enabled;

	struct sssraid_intr_info *intr_info;
	u32 intr_info_count;

	char watchdog_work_q_name[20];
	struct workqueue_struct *watchdog_work_q;
	struct delayed_work watchdog_work;
	spinlock_t watchdog_lock;

	char fwevt_worker_name[SSSRAID_NAME_LENGTH];
	struct workqueue_struct	*fwevt_worker_thread;
	spinlock_t fwevt_lock;
	struct list_head fwevt_list;

	struct sssraid_fwevt *current_event;
};

/*
 * SSSRAID scatter list descriptor
 */
struct sssraid_sgl_desc {
	__le64 addr;
	__le32 length;
	__u8   rsvd[3];
	__u8   type;
};

union sssraid_data_ptr {
	struct {
		__le64 prp1;
		__le64 prp2;
	};
	struct sssraid_sgl_desc sgl;
};

/*
 * SSSRAID general admin class command format struct
 */
struct sssraid_admin_common_command {
	__u8	opcode;
	__u8	flags;
	__le16	command_id;
	__le32	hdid;
	__le32	cdw2[4];
	union sssraid_data_ptr	dptr;
	__le32	cdw10;
	__le32	cdw11;
	__le32	cdw12;
	__le32	cdw13;
	__le32	cdw14;
	__le32	cdw15;
};

struct sssraid_features {
	__u8	opcode;
	__u8	flags;
	__le16	command_id;
	__le32	hdid;
	__u64	rsvd2[2];
	union sssraid_data_ptr dptr;
	__le32	fid;
	__le32	dword11;
	__le32	dword12;
	__le32	dword13;
	__le32	dword14;
	__le32	dword15;
};

/*
 * SSSRAID create completion queue command struct
 */
struct sssraid_create_cq {
	__u8	opcode;
	__u8	flags;
	__le16	command_id;
	__u32	rsvd1[5];
	__le64	prp1;
	__u64	rsvd8;
	__le16	cqid;
	__le16	qsize;
	__le16	cq_flags;
	__le16	irq_vector;
	__u32	rsvd12[4];
};

/*
 * SSSRAID create submission queue command struct
 */
struct sssraid_create_sq {
	__u8	opcode;
	__u8	flags;
	__le16	command_id;
	__u32	rsvd1[5];
	__le64	prp1;
	__u64	rsvd8;
	__le16	sqid;
	__le16	qsize;
	__le16	sq_flags;
	__le16	cqid;
	__u32	rsvd12[4];
};

/*
 * SSSRAID delete submission queue command struct
 */
struct sssraid_delete_queue {
	__u8	opcode;
	__u8	flags;
	__le16	command_id;
	__u32	rsvd1[9];
	__le16	qid;
	__u16	rsvd10;
	__u32	rsvd11[5];
};

/*
 * SSSRAID access to information command struct
 */
struct sssraid_get_info {
	__u8	opcode;
	__u8	flags;
	__le16	command_id;
	__le32	hdid;
	__u32	rsvd2[4];
	union sssraid_data_ptr	dptr;
	__u8	type;
	__u8	rsvd10[3];
	__le32	cdw11;
	__u32	rsvd12[4];
};

/*
 * User command struct
 */
struct sssraid_usr_cmd {
	__u8	opcode;
	__u8	flags;
	__le16	command_id;
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
	union sssraid_data_ptr	dptr;
	__le32 cdw10;
	__le32 cdw11;
	__le32 cdw12;
	__le32 cdw13;
	__le32 cdw14;
	__le32 cdw15;
};

enum {
	SSSRAID_CMD_FLAG_SGL_METABUF = (1 << 6),
	SSSRAID_CMD_FLAG_SGL_METASEG = (1 << 7),
	SSSRAID_CMD_FLAG_SGL_ALL     = SSSRAID_CMD_FLAG_SGL_METABUF | SSSRAID_CMD_FLAG_SGL_METASEG,
};

enum sssraid_cmd_state {
	SSSRAID_CMDSTAT_IDLE = 0,
	SSSRAID_CMDSTAT_FLIGHT = 1,
	SSSRAID_CMDSTAT_COMPLETE = 2,
	SSSRAID_CMDSTAT_TIMEOUT = 3,
	SSSRAID_CMDSTAT_TMO_COMPLETE = 4,
};

/*
 * SSSRAID abort command struct
 */
struct sssraid_abort_cmd {
	__u8	opcode;
	__u8	flags;
	__le16	command_id;
	__le32	hdid;
	__u64	rsvd2[4];
	__le16	sqid;
	__le16	cid;
	__u32	rsvd11[5];
};

/*
 * SSSRAID reset command struct
 */
struct sssraid_reset_cmd {
	__u8	opcode;
	__u8	flags;
	__le16	command_id;
	__le32	hdid;
	__u64	rsvd2[4];
	__u8	type;
	__u8	rsvd10[3];
	__u32	rsvd11[5];
};

/*
 * SSSRAID admin class command set struct
 */
struct sssraid_admin_command {
	union {
		struct sssraid_admin_common_command common;
		struct sssraid_features features;
		struct sssraid_create_cq create_cq;
		struct sssraid_create_sq create_sq;
		struct sssraid_delete_queue delete_queue;
		struct sssraid_get_info get_info;
		struct sssraid_abort_cmd abort;
		struct sssraid_reset_cmd reset;
		struct sssraid_usr_cmd usr_cmd;
	};
};

/*
 * SSSRAID general IO class command format struct
 */
struct sssraid_ioq_common_command {
	__u8	opcode;
	__u8	flags;
	__le16	command_id;
	__le32	hdid;
	__le16	sense_len;
	__u8	cdb_len;
	__u8	rsvd2;
	__le32	cdw3[3];
	union sssraid_data_ptr	dptr;
	__le32	cdw10[6];
	__u8	cdb[32];
	__le64	sense_addr;
	__le32	cdw26[6];
};

/*
 * SSSRAID read or write command struct
 */
struct sssraid_rw_command {
	__u8	opcode;
	__u8	flags;
	__le16	command_id;
	__le32	hdid;
	__le16	sense_len;
	__u8	cdb_len;
	__u8	rsvd2;
	__u32	rsvd3[3];
	union sssraid_data_ptr	dptr;
	__le64	slba;
	__le16	nlb;
	__le16	control;
	__u32	rsvd13[3];
	__u8	cdb[32];
	__le64	sense_addr;
	__u32	rsvd26[6];
};

struct sssraid_scsi_nonio {
	__u8	opcode;
	__u8	flags;
	__le16	command_id;
	__le32	hdid;
	__le16	sense_len;
	__u8	cdb_length;
	__u8	rsvd2;
	__u32	rsvd3[3];
	union sssraid_data_ptr	dptr;
	__u32	rsvd10[5];
	__le32	buffer_len;
	__u8	cdb[32];
	__le64	sense_addr;
	__u32	rsvd26[6];
};

/*
 * SSSRAID IO class command struct
 */
struct sssraid_ioq_command {
	union {
		struct sssraid_ioq_common_command common;
		struct sssraid_rw_command rw;
		struct sssraid_scsi_nonio scsi_nonio;
	};
};

/*
 * SSSRAID passthru command struct
 */
struct sssraid_passthru_common_cmd {
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

struct sssraid_ioq_passthru_cmd {
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

struct sssraid_bsg_request {
	u32  msgcode;
	u32 control;
	union {
		struct sssraid_passthru_common_cmd admcmd;
		struct sssraid_ioq_passthru_cmd    ioqcmd;
	};
};

enum {
	SSSRAID_BSG_ADM,
	SSSRAID_BSG_IOQ,
};

/*
 * define the transfer command struct
 */
struct sssraid_cmd {
	u16 qid;
	u16 cid;
	u32 result0;
	u32 result1;
	u16 status;
	void *priv;
	enum sssraid_cmd_state state;
	struct completion cmd_done;
	struct list_head list;
};

/*
 * define the SSSRAID physical queue struct
 */
struct sssraid_squeue {
	struct sssraid_ioc *sdioc;
	spinlock_t sq_lock; /* spinlock for lock handling */

	void *sq_cmds;

	dma_addr_t sq_dma_addr;
	u32 __iomem *q_db;
	u8 cq_phase;
	u8 sqes;
	u16 qidx;
	u16 sq_tail;
	u16 last_cq_head;
	u16 q_depth;
	void *sense;
	dma_addr_t sense_dma_addr;
	struct dma_pool *prp_small_pool;
};

struct sssraid_cqueue {
	struct sssraid_ioc *sdioc;

	spinlock_t cq_lock ____cacheline_aligned_in_smp; /* spinlock for lock handling */

	struct sssraid_completion *cqes;

	dma_addr_t cq_dma_addr;
	u8 cq_phase;
	u16 cq_head;
	u16 last_cq_head;
};

/*
 * define the SSSRAID IO queue descriptor struct
 */
struct sssraid_iod {
	struct sssraid_squeue *sqinfo;
	enum sssraid_cmd_state state;
	int npages;
	u32 nsge;
	u32 length;
	bool use_sgl;
	dma_addr_t first_dma;
	void *sense;
	dma_addr_t sense_dma;
	struct scatterlist *sg;
	void *list[];
};

/*
 * define the SSSRAID scsi device attribution and information
 */
#define SSSRAID_DISK_INFO_ATTR_BOOT(attr) ((attr) & 0x01)
#define SSSRAID_DISK_INFO_ATTR_VD(attr) (((attr) & 0x02) == 0x0)
#define SSSRAID_DISK_INFO_ATTR_PT(attr) (((attr) & 0x22) == 0x02)
#define SSSRAID_DISK_INFO_ATTR_RAW(attr) ((attr) & 0x20)
#define SSSRAID_DISK_TYPE(attr) ((attr) & 0x1e)

#define SSSRAID_DISK_INFO_FLAG_VALID(flag) ((flag) & 0x01)
#define SSSRAID_DISK_INFO_FLAG_CHANGE(flag) ((flag) & 0x02)

/*
 * define the SSSRAID scsi device identifier
 */
enum {
	SSSRAID_SAS_HDD_VD  = 0x04,
	SSSRAID_SATA_HDD_VD = 0x08,
	SSSRAID_SAS_SSD_VD  = 0x0c,
	SSSRAID_SATA_SSD_VD = 0x10,
	SSSRAID_NVME_SSD_VD = 0x14,
	SSSRAID_SAS_HDD_PD  = 0x06,
	SSSRAID_SATA_HDD_PD = 0x0a,
	SSSRAID_SAS_SSD_PD  = 0x0e,
	SSSRAID_SATA_SSD_PD = 0x12,
	SSSRAID_NVME_SSD_PD = 0x16,
};

/*
 * define the SSSRAID scsi device queue depth
 */
#define SSSRAID_HDD_PD_QD 64
#define SSSRAID_HDD_VD_QD 256
#define SSSRAID_SSD_PD_QD 64
#define SSSRAID_SSD_VD_QD 256

#define BGTASK_TYPE_REBUILD 4
#define USR_CMD_READ 0xc2
#define USR_CMD_RDLEN 0x1000
#define USR_CMD_VDINFO 0x704
#define USR_CMD_BGTASK 0x504
#define VDINFO_PARAM_LEN 0x04

/*
 * SSSRAID virtual device information struct
 */
struct sssraid_vd_info {
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

#define MAX_REALTIME_BGTASK_NUM 32

struct bgtask_info {
	__u8 type;
	__u8 progress;
	__u8 rate;
	__u8 rsvd0;
	__le16 vd_id;
	__le16 time_left;
	__u8 rsvd1[4];
};

struct sssraid_bgtask {
	__u8 sw;
	__u8 task_num;
	__u8 rsvd[6];
	struct bgtask_info bgtask[MAX_REALTIME_BGTASK_NUM];
};

/*
 * SSSRAID scsi device information struct
 */
struct sssraid_dev_info {
	__le32	hdid;
	__le16	target;
	__u8	channel;
	__u8	lun;
	__u8	attr;
	__u8	flag;
	__le16	max_io_kb;
};

#define IOQ_PT_DATA_LEN			4096
#define MAX_DEV_ENTRY_PER_PAGE_4K	340
struct sssraid_dev_list {
	__le32	dev_num;
	__u32	rsvd0[3];
	struct sssraid_dev_info	devices[MAX_DEV_ENTRY_PER_PAGE_4K];
};

/*
 * SSSRAID scsi device host data struct
 */
struct sssraid_sdev_hostdata {
	u32 hdid;
	u16 max_io_kb;
	u8 attr;
	u8 flag;
	u8 rg_id;
	u8 rsvd[3];
};

extern unsigned char small_pool_num;
extern u32 io_queue_depth;
irqreturn_t sssraid_isr_poll(int irq, void *privdata);
bool sssraid_poll_cq(struct sssraid_ioc *sdioc, u16 qidx, int cid);
void sssraid_submit_cmd(struct sssraid_squeue *sqinfo, const void *cmd);
int sssraid_get_dev_list(struct sssraid_ioc *sdioc, struct sssraid_dev_info *devices);
int sssraid_submit_admin_sync_cmd(struct sssraid_ioc *sdioc, struct sssraid_admin_command *cmd,
					u32 *result0, u32 *result1, u32 timeout);
int sssraid_send_abort_cmd(struct sssraid_ioc *sdioc, u32 hdid, u16 qidx, u16 cid);
int sssraid_send_reset_cmd(struct sssraid_ioc *sdioc, u8 type, u32 hdid);
void sssraid_adm_timeout(struct sssraid_ioc *sdioc, struct sssraid_cmd *cmd);
int sssraid_init_ioc(struct sssraid_ioc *sdioc, u8 re_init);
void sssraid_cleanup_ioc(struct sssraid_ioc *sdioc, u8 re_init);
int sssraid_soft_reset_handler(struct sssraid_ioc *sdioc);
void sssraid_free_iod_res(struct sssraid_ioc *sdioc, struct sssraid_iod *iod);
bool sssraid_change_host_state(struct sssraid_ioc *sdioc, enum sssraid_state newstate);
int sssraid_configure_timestamp(struct sssraid_ioc *sdioc);
int sssraid_init_ctrl_info(struct sssraid_ioc *sdioc);
struct sssraid_cmd *sssraid_get_cmd(struct sssraid_ioc *sdioc, enum sssraid_cmd_type type);
void sssraid_put_cmd(struct sssraid_ioc *sdioc, struct sssraid_cmd *cmd,
			   enum sssraid_cmd_type type);
int sssraid_send_event_ack(struct sssraid_ioc *sdioc, u8 event,
	u32 event_ctx, u16 cid);
struct sssraid_fwevt *sssraid_alloc_fwevt(int len);
void sssraid_fwevt_add_to_list(struct sssraid_ioc *sdioc,
			struct sssraid_fwevt *fwevt);
void sssraid_cleanup_fwevt_list(struct sssraid_ioc *sdioc);
void sssraid_ioc_enable_intr(struct sssraid_ioc *sdioc);
void sssraid_ioc_disable_intr(struct sssraid_ioc *sdioc);
void sssraid_cleanup_resources(struct sssraid_ioc *sdioc);
void sssraid_complete_cqes(struct sssraid_ioc *sdioc, u16 qidx, u16 start, u16 end);
int sssraid_io_map_data(struct sssraid_ioc *sdioc, struct sssraid_iod *iod,
			      struct scsi_cmnd *scmd, struct sssraid_ioq_command *ioq_cmd);
void sssraid_map_status(struct sssraid_iod *iod, struct scsi_cmnd *scmd,
			      struct sssraid_completion *cqe);
void sssraid_scan_disk(struct sssraid_ioc *sdioc);
void sssraid_complete_aen(struct sssraid_ioc *sdioc, struct sssraid_completion *cqe);
void sssraid_back_all_io(struct sssraid_ioc *sdioc);

static inline void **sssraid_iod_list(struct sssraid_iod *iod)
{
	return iod->list;
}

#endif
