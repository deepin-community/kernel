/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef CQM_MAIN_H
#define CQM_MAIN_H

#include <linux/pci.h>

#include "hinic3_crm.h"
#include "cqm_bloomfilter.h"
#include "hinic3_hwif.h"
#include "cqm_bat_cla.h"

#define GET_MAX	max
#define GET_MIN	min
#define CQM_DW_SHIFT       2
#define CQM_QW_SHIFT       3
#define CQM_BYTE_BIT_SHIFT 3
#define CQM_NUM_BIT_BYTE   8

#define CHIPIF_SUCCESS     0
#define CHIPIF_FAIL        (-1)

#define CQM_TIMER_ENABLE   1
#define CQM_TIMER_DISABLE  0

#define CQM_TIMER_NUM_MULTI   2

/* The value must be the same as that of hinic3_service_type in hinic3_crm.h. */
#define CQM_SERVICE_T_NIC    SERVICE_T_NIC
#define CQM_SERVICE_T_OVS    SERVICE_T_OVS
#define CQM_SERVICE_T_ROCE   SERVICE_T_ROCE
#define CQM_SERVICE_T_TOE    SERVICE_T_TOE
#define CQM_SERVICE_T_IOE    SERVICE_T_IOE
#define CQM_SERVICE_T_FC     SERVICE_T_FC
#define CQM_SERVICE_T_VBS    SERVICE_T_VBS
#define CQM_SERVICE_T_IPSEC  SERVICE_T_IPSEC
#define CQM_SERVICE_T_VIRTIO SERVICE_T_VIRTIO
#define CQM_SERVICE_T_PPA    SERVICE_T_PPA
#define CQM_SERVICE_T_MAX    SERVICE_T_MAX

struct tag_cqm_service {
	bool valid;	   /* Whether to enable this service on the function. */
	bool has_register; /* Registered or Not */
	u64 hardware_db_paddr;
	void __iomem *hardware_db_vaddr;
	u64 dwqe_paddr;
	void __iomem *dwqe_vaddr;
	u32 buf_order;     /* The size of each buf node is 2^buf_order pages. */
	struct tag_service_register_template service_template;
};

struct tag_cqm_fake_cfg {
	u32 parent_func;       /* The parent func_id of the fake vfs. */
	u32 child_func_start;  /* The start func_id of the child fake vfs. */
	u32 child_func_number; /* The number of the child fake vfs. */
};

#define CQM_MAX_FACKVF_GROUP 4

struct tag_cqm_func_capability {
	/* BAT_PTR table(SMLC) */
	bool ft_enable; /* BAT for flow table enable: support toe/ioe/fc service
			 */
	bool rdma_enable; /* BAT for rdma enable: support RoCE */
	/* VAT table(SMIR) */
	bool ft_pf_enable; /* Same as ft_enable. BAT entry for toe/ioe/fc on pf
			    */
	bool rdma_pf_enable; /* Same as rdma_enable. BAT entry for rdma on pf */

	/* Dynamic or static memory allocation during the application of
	 * specified QPC/SCQC for each service.
	 */
	bool qpc_alloc_static;
	bool scqc_alloc_static;

	u8 timer_enable;       /* Whether the timer function is enabled */
	u8 bloomfilter_enable; /* Whether the bloomgfilter function is enabled
				*/
	u32 flow_table_based_conn_number; /* Maximum number of connections for
					   * toe/ioe/fc, whitch cannot excedd
					   * qpc_number
					   */
	u32 flow_table_based_conn_cache_number; /* Maximum number of sticky
						 * caches
						 */
	u32 bloomfilter_length; /* Size of the bloomfilter table, 64-byte
				 * aligned
				 */
	u32 bloomfilter_addr; /* Start position of the bloomfilter table in the
			       * SMF main cache.
			       */
	u32 qpc_reserved;     /* Reserved bit in bitmap */
	u32 qpc_reserved_back; /* Reserved back bit in bitmap */
	u32 mpt_reserved;     /* The ROCE/IWARP MPT also has a reserved bit. */

	/* All basic_size must be 2^n-aligned. */
	u32 hash_number; /* The number of hash bucket. The size of BAT table is
			  * aliaed with 64 bucket. At least 64 buckets is
			  * required.
			  */
	u32 hash_basic_size; /* THe basic size of hash bucket is 64B, including
			      * 5 valid entry and one next entry.
			      */
	u32 qpc_number;
	u32 fake_vf_qpc_number;
	u32 qpc_basic_size;

	/* NUmber of PFs/VFs on the current host only for timer resource used */
	u32 pf_num;
	u32 pf_id_start;
	u32 vf_num;
	u32 vf_id_start;

	u8 timer_pf_num;
	u8 timer_pf_id_start;
	u16 timer_vf_num;
	u16 timer_vf_id_start;

	u32 lb_mode;
	/* Only lower 4bit is valid, indicating which SMFs are enabled.
	 * For example, 0101B indicates that SMF0 and SMF2 are enabled.
	 */
	u32 smf_pg;

	u32 fake_mode;
	u32 fake_func_type;  /* Whether the current function belongs to the fake
			      * group (parent or child)
			      */
	u32 fake_cfg_number; /* Number of current configuration groups */
	struct tag_cqm_fake_cfg fake_cfg[CQM_MAX_FACKVF_GROUP];

	/* Note: for cqm specail test */
	u32 pagesize_reorder;
	bool xid_alloc_mode;
	bool gpa_check_enable;
	u32 scq_reserved;
	u32 srq_reserved;

	u32 mpt_number;
	u32 mpt_basic_size;
	u32 scqc_number;
	u32 scqc_basic_size;
	u32 srqc_number;
	u32 srqc_basic_size;

	u32 gid_number;
	u32 gid_basic_size;
	u32 lun_number;
	u32 lun_basic_size;
	u32 taskmap_number;
	u32 taskmap_basic_size;
	u32 l3i_number;
	u32 l3i_basic_size;
	u32 childc_number;
	u32 childc_basic_size;
	u32 child_qpc_id_start; /* FC service Child CTX is global addressing. */
	u32 childc_number_all_function; /* The chip supports a maximum of 8096
					 * child CTXs.
					 */
	u32 timer_number;
	u32 timer_basic_size;
	u32 xid2cid_number;
	u32 xid2cid_basic_size;
	u32 reorder_number;
	u32 reorder_basic_size;
};

#define CQM_PF                             TYPE_PF
#define CQM_VF                             TYPE_VF
#define CQM_PPF                            TYPE_PPF
#define CQM_UNKNOWN                        TYPE_UNKNOWN
#define CQM_MAX_PF_NUM                     32

#define CQM_LB_MODE_NORMAL                 0xff
#define CQM_LB_MODE_0                      0
#define CQM_LB_MODE_1                      1
#define CQM_LB_MODE_2                      2

#define CQM_LB_SMF_MAX                     4

#define CQM_FPGA_MODE                      0
#define CQM_EMU_MODE                       1

#define CQM_FAKE_FUNC_NORMAL               0
#define CQM_FAKE_FUNC_PARENT               1
#define CQM_FAKE_FUNC_CHILD                2
#define CQM_FAKE_FUNC_CHILD_CONFLICT       3 /* The detected function is the
					      * function that is faked.
					      */

#define CQM_FAKE_FUNC_MAX                  32

#define CQM_SPU_HOST_ID                    4

#define CQM_QPC_ROCE_PER_DRCT              12
#define CQM_QPC_ROCE_NORMAL   0
#define CQM_QPC_ROCE_VBS_MODE 2

struct tag_cqm_toe_private_capability {
	/* TOE srq is different from other services
	 * and does not need to be managed by the CLA table.
	 */
	u32 toe_srqc_number;
	u32 toe_srqc_basic_size;
	u32 toe_srqc_start_id;

	struct tag_cqm_bitmap srqc_bitmap;
};

struct tag_cqm_secure_mem {
	u16 func_id;
	bool need_secure_mem;

	u32 mode;
	u32 gpa_len0;

	void __iomem *va_base;
	void __iomem *va_end;
	u64 pa_base;
	u32 page_num;

	/* bitmap mgmt */
	spinlock_t bitmap_lock;
	unsigned long *bitmap;
	u32 bits_nr;
	u32 alloc_cnt;
	u32 free_cnt;
};

struct tag_cqm_handle {
	struct hinic3_hwdev *ex_handle;
	struct pci_dev *dev;
	struct hinic3_func_attr func_attribute; /* vf/pf attributes */
	struct tag_cqm_func_capability func_capability; /* function capability set */
	struct tag_cqm_service service[CQM_SERVICE_T_MAX]; /* Service-related structure */
	struct tag_cqm_bat_table bat_table;
	struct tag_cqm_bloomfilter_table bloomfilter_table;
	/* fake-vf-related structure */
	struct tag_cqm_handle *fake_cqm_handle[CQM_FAKE_FUNC_MAX];
	struct tag_cqm_handle *parent_cqm_handle;

	struct tag_cqm_toe_private_capability toe_own_capability; /* TOE service-related
								   * capability set
								   */
	struct tag_cqm_secure_mem secure_mem;
	struct list_head node;
	char name[VRAM_NAME_APPLY_LEN];
};

#define CQM_CQN_FROM_CEQE(data)      ((data) & 0xfffff)
#define CQM_XID_FROM_CEQE(data)      ((data) & 0xfffff)
#define CQM_QID_FROM_CEQE(data)      (((data) >> 20) & 0x7)
#define CQM_TYPE_FROM_CEQE(data)     (((data) >> 23) & 0x7)

#define CQM_HASH_BUCKET_SIZE_64      64

#define CQM_MAX_QPC_NUM              0x100000U
#define CQM_MAX_SCQC_NUM             0x100000U
#define CQM_MAX_SRQC_NUM             0x100000U
#define CQM_MAX_CHILDC_NUM           0x100000U

#define CQM_QPC_SIZE_256             256U
#define CQM_QPC_SIZE_512             512U
#define CQM_QPC_SIZE_1024            1024U

#define CQM_SCQC_SIZE_32             32U
#define CQM_SCQC_SIZE_64             64U
#define CQM_SCQC_SIZE_128            128U

#define CQM_SRQC_SIZE_32             32
#define CQM_SRQC_SIZE_64             64
#define CQM_SRQC_SIZE_128            128

#define CQM_MPT_SIZE_64              64

#define CQM_GID_SIZE_32              32

#define CQM_LUN_SIZE_8               8

#define CQM_L3I_SIZE_8               8

#define CQM_TIMER_SIZE_32            32

#define CQM_XID2CID_SIZE_8           8

#define CQM_REORDER_SIZE_256         256

#define CQM_CHILDC_SIZE_256          256U

#define CQM_XID2CID_VBS_NUM          (2 * 1024) /* 2K nvme Q */

#define CQM_VBS_QPC_SIZE             512U

#define CQM_XID2CID_VIRTIO_NUM       (16 * 1024) /* 16K virt Q */

#define CQM_GID_RDMA_NUM             128

#define CQM_LUN_FC_NUM               64

#define CQM_TASKMAP_FC_NUM           4

#define CQM_L3I_COMM_NUM             64

#define CQM_CHILDC_ROCE_NUM          (8 * 1024)
#define CQM_CHILDC_OVS_VBS_NUM       (8 * 1024)

#define CQM_TIMER_SCALE_NUM          (2 * 1024)
#define CQM_TIMER_ALIGN_WHEEL_NUM    8
#define CQM_TIMER_ALIGN_SCALE_NUM \
	(CQM_TIMER_SCALE_NUM * CQM_TIMER_ALIGN_WHEEL_NUM)

#define CQM_QPC_OVS_RSVD             (1024 * 1024)
#define CQM_QPC_ROCE_RSVD            2
#define CQM_QPC_ROCEAA_SWITCH_QP_NUM 4
#define CQM_QPC_ROCEAA_RSVD \
	(4 * 1024 + CQM_QPC_ROCEAA_SWITCH_QP_NUM) /* 4096 Normal QP +
						   * 4 Switch QP
						   */

#define CQM_CQ_ROCEAA_RSVD           64
#define CQM_SRQ_ROCEAA_RSVD          64
#define CQM_QPC_ROCE_VBS_RSVD_BACK   204800  /* 200K */

#define CQM_OVS_PAGESIZE_ORDER       9
#define CQM_OVS_MAX_TIMER_FUNC       48

#define CQM_PPA_PAGESIZE_ORDER       8

#define CQM_FC_PAGESIZE_ORDER 0

#define CQM_QHEAD_ALIGN_ORDER 6

typedef void (*serv_cap_init_cb)(struct tag_cqm_handle *, void *);

struct cqm_srv_cap_init {
	u32 service_type;
	serv_cap_init_cb serv_cap_proc;
};

/* Only for llt test */
s32 cqm_capability_init(void *ex_handle);
/* Can be defined as static */
s32 cqm_mem_init(void *ex_handle);
void cqm_mem_uninit(void *ex_handle);
s32 cqm_event_init(void *ex_handle);
void cqm_event_uninit(void *ex_handle);
void cqm_scq_callback(void *ex_handle, u32 ceqe_data);
void cqm_ecq_callback(void *ex_handle, u32 ceqe_data);
void cqm_nocq_callback(void *ex_handle, u32 ceqe_data);
u8 cqm_aeq_callback(void *ex_handle, u8 event, u8 *data);
s32 cqm_get_fake_func_type(struct tag_cqm_handle *cqm_handle);
s32 cqm_get_child_func_start(struct tag_cqm_handle *cqm_handle);
s32 cqm_get_child_func_number(struct tag_cqm_handle *cqm_handle);

s32 cqm_init(void *ex_handle);
void cqm_uninit(void *ex_handle);
s32 cqm_service_register(void *ex_handle, struct tag_service_register_template *service_template);
void cqm_service_unregister(void *ex_handle, u32 service_type);

s32 cqm_fake_vf_num_set(void *ex_handle, u16 fake_vf_num_cfg);
#define CQM_LOG_ID 0

#define CQM_PTR_NULL(x)      "%s: " #x " is null\n", __func__
#define CQM_ALLOC_FAIL(x)    "%s: " #x " alloc fail\n", __func__
#define CQM_MAP_FAIL(x)      "%s: " #x " map fail\n", __func__
#define CQM_FUNCTION_FAIL(x) "%s: " #x " return failure\n", __func__
#define CQM_WRONG_VALUE(x)   "%s: " #x " %u is wrong\n", __func__, (u32)(x)

#define cqm_err(dev, format, ...)  dev_err(dev, "[CQM]" format, ##__VA_ARGS__)
#define cqm_warn(dev, format, ...) dev_warn(dev, "[CQM]" format, ##__VA_ARGS__)
#define cqm_notice(dev, format, ...) \
	dev_notice(dev, "[CQM]" format, ##__VA_ARGS__)
#define cqm_info(dev, format, ...) dev_info(dev, "[CQM]" format, ##__VA_ARGS__)
#ifdef __CQM_DEBUG__
#define cqm_dbg(format, ...) pr_info("[CQM]" format, ##__VA_ARGS__)
#else
#define cqm_dbg(format, ...)
#endif

#endif /* CQM_MAIN_H */
