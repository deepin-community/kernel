/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_HWDEV_H
#define HINIC3_HWDEV_H

#include <linux/workqueue.h>
#include "hinic3_mt.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "mpu_inband_cmd_defs.h"
#include "hinic3_profile.h"
#include "vram_common.h"

struct cfg_mgmt_info;

struct hinic3_hwif;
struct hinic3_aeqs;
struct hinic3_ceqs;
struct hinic3_mbox;
struct hinic3_msg_pf_to_mgmt;
struct hinic3_hwdev;

#define HINIC3_CHANNEL_DETECT_PERIOD (5 * 1000)

struct hinic3_page_addr {
	void *virt_addr;
	u64 phys_addr;
};

struct mqm_addr_trans_tbl_info {
	u32 chunk_num;
	u32 search_gpa_num;
	u32 page_size;
	u32 page_num;
	struct hinic3_dma_addr_align *brm_srch_page_addr;
};

struct hinic3_devlink {
	struct hinic3_hwdev *hwdev;
	u8 activate_fw; /* 0 ~ 7 */
	u8 switch_cfg;  /* 0 ~ 7 */
};

enum hinic3_func_mode {
	/* single host */
	FUNC_MOD_NORMAL_HOST,
	/* multi host, bare-metal, sdi side */
	FUNC_MOD_MULTI_BM_MASTER,
	/* multi host, bare-metal, host side */
	FUNC_MOD_MULTI_BM_SLAVE,
	/* multi host, vm mode, sdi side */
	FUNC_MOD_MULTI_VM_MASTER,
	/* multi host, vm mode, host side */
	FUNC_MOD_MULTI_VM_SLAVE,
};

#define IS_BMGW_MASTER_HOST(hwdev)	\
		((hwdev)->func_mode == FUNC_MOD_MULTI_BM_MASTER)
#define IS_BMGW_SLAVE_HOST(hwdev)	\
		((hwdev)->func_mode == FUNC_MOD_MULTI_BM_SLAVE)
#define IS_VM_MASTER_HOST(hwdev)	\
		((hwdev)->func_mode == FUNC_MOD_MULTI_VM_MASTER)
#define IS_VM_SLAVE_HOST(hwdev)		\
		((hwdev)->func_mode == FUNC_MOD_MULTI_VM_SLAVE)

#define IS_MASTER_HOST(hwdev)		\
		(IS_BMGW_MASTER_HOST(hwdev) || IS_VM_MASTER_HOST(hwdev))

#define IS_SLAVE_HOST(hwdev)		\
		(IS_BMGW_SLAVE_HOST(hwdev) || IS_VM_SLAVE_HOST(hwdev))

#define IS_MULTI_HOST(hwdev)		\
		(IS_BMGW_MASTER_HOST(hwdev) || IS_BMGW_SLAVE_HOST(hwdev) || \
		 IS_VM_MASTER_HOST(hwdev) || IS_VM_SLAVE_HOST(hwdev))

#define NEED_MBOX_FORWARD(hwdev)	IS_BMGW_SLAVE_HOST(hwdev)

enum hinic3_host_mode_e {
	HINIC3_MODE_NORMAL = 0,
	HINIC3_SDI_MODE_VM,
	HINIC3_SDI_MODE_BM,
	HINIC3_SDI_MODE_MAX,
};

enum hinic3_hot_plug_mode {
	HOT_PLUG_ENABLE,
	HOT_PLUG_DISABLE,
};

enum hinic3_os_hot_replace_mode {
	HOT_REPLACE_DISABLE,
	HOT_REPLACE_ENABLE,
};

#define UNSUPPORT_HOT_PLUG(hwdev)	\
		((hwdev)->hot_plug_mode == HOT_PLUG_DISABLE)

#define SUPPORT_HOT_PLUG(hwdev)	\
		((hwdev)->hot_plug_mode == HOT_PLUG_ENABLE)

#define MULTI_HOST_CHIP_MODE_SHIFT		0
#define MULTI_HOST_MASTER_MBX_STS_SHIFT		17
#define MULTI_HOST_PRIV_DATA_SHIFT		0x8

#define MULTI_HOST_CHIP_MODE_MASK		0xF
#define MULTI_HOST_MASTER_MBX_STS_MASK		0x1
#define MULTI_HOST_PRIV_DATA_MASK		0xFFFF

#define MULTI_HOST_REG_SET(val, member)			\
				(((val) & MULTI_HOST_##member##_MASK) \
					<< MULTI_HOST_##member##_SHIFT)
#define MULTI_HOST_REG_GET(val, member)			\
				(((val) >> MULTI_HOST_##member##_SHIFT) \
					& MULTI_HOST_##member##_MASK)
#define MULTI_HOST_REG_CLEAR(val, member)	\
				((val) & (~(MULTI_HOST_##member##_MASK \
					<< MULTI_HOST_##member##_SHIFT)))

struct mqm_eqm_vram_name_s {
	char vram_name[VRAM_NAME_MAX_LEN];
};

struct hinic3_hwdev {
	void *adapter_hdl;  /* pointer to hinic3_pcidev or NDIS_Adapter */
	void *pcidev_hdl;   /* pointer to pcidev or Handler */
	void *dev_hdl;      /* pointer to pcidev->dev or Handler, for
			     * sdk_err() or dma_alloc()
			     */

	void *service_adapter[SERVICE_T_MAX];
	void *chip_node;
	struct semaphore ppf_sem;
	void *ppf_hwdev;

	u32 wq_page_size;
	int chip_present_flag;
	bool poll;	   /* use polling mode or int mode */
	u32 rsvd1;

	struct hinic3_hwif *hwif; /* include void __iomem *bar */
	struct comm_global_attr glb_attr;
	u64 features[COMM_MAX_FEATURE_QWORD];

	struct cfg_mgmt_info *cfg_mgmt;

	struct hinic3_cmdqs *cmdqs;
	struct hinic3_aeqs *aeqs;
	struct hinic3_ceqs *ceqs;
	struct hinic3_mbox *func_to_func;
	struct hinic3_msg_pf_to_mgmt *pf_to_mgmt;
	struct hinic3_clp_pf_to_mgmt *clp_pf_to_mgmt;

	void *cqm_hdl;
	struct mqm_addr_trans_tbl_info mqm_att;
	struct hinic3_page_addr page_pa0;
	struct hinic3_page_addr page_pa1;
	u32 stateful_ref_cnt;
	u32 rsvd2;

	struct hinic3_multi_host_mgmt *mhost_mgmt;
	char mhost_mgmt_name[VRAM_NAME_MAX_LEN];

	struct mqm_eqm_vram_name_s *mqm_eqm_vram_name;

	struct mutex stateful_mutex; /* protect cqm init and deinit */

	struct hinic3_hw_stats hw_stats;
	u8 *chip_fault_stats;

	hinic3_event_handler event_callback;
	void *event_pri_handle;

	struct hinic3_board_info board_info;

	struct delayed_work	sync_time_task;
	struct delayed_work	channel_detect_task;
	struct hisdk3_prof_attr *prof_attr;
	struct hinic3_prof_adapter *prof_adap;

	struct workqueue_struct *workq;

	u32 rd_bar_err_cnt;
	bool pcie_link_down;
	bool heartbeat_lost;
	struct timer_list heartbeat_timer;
	struct work_struct heartbeat_lost_work;

	ulong func_state;
	spinlock_t channel_lock; /* protect channel init and deinit */

	u16 probe_fault_level;

	struct hinic3_devlink *devlink_dev;

	enum hinic3_func_mode	func_mode;
	enum hinic3_hot_plug_mode hot_plug_mode;
	enum hinic3_os_hot_replace_mode hot_replace_mode;
	u32 rsvd3;

	DECLARE_BITMAP(func_probe_in_host, MAX_FUNCTION_NUM);
	DECLARE_BITMAP(netdev_setup_state, MAX_FUNCTION_NUM);

	u64 cur_recv_aeq_cnt;
	u64 last_recv_aeq_cnt;
	u16 aeq_busy_cnt;

	u64 mbox_send_cnt;
	u64 mbox_ack_cnt;

	u64 rsvd4[5];
	u8 cmdq_mode;
};

#define HINIC3_DRV_FEATURE_QW0 \
	(COMM_F_API_CHAIN | COMM_F_CLP | COMM_F_MBOX_SEGMENT | \
	 COMM_F_CMDQ_NUM | COMM_F_VIRTIO_VQ_SIZE | COMM_F_USE_REAL_RX_BUF_SIZE | \
	 COMM_F_CMD_BUF_SIZE | COMM_F_ONLY_ENHANCE_CMDQ)

#define HINIC3_MAX_HOST_NUM(hwdev)	((hwdev)->glb_attr.max_host_num)
#define HINIC3_MAX_PF_NUM(hwdev)	((hwdev)->glb_attr.max_pf_num)
#define HINIC3_MGMT_CPU_NODE_ID(hwdev)	((hwdev)->glb_attr.mgmt_host_node_id)

#define COMM_FEATURE_QW0(hwdev, feature)	\
		((hwdev)->features[0] & COMM_F_##feature)
#define COMM_SUPPORT_API_CHAIN(hwdev)	COMM_FEATURE_QW0(hwdev, API_CHAIN)
#define COMM_SUPPORT_CLP(hwdev)		COMM_FEATURE_QW0(hwdev, CLP)
#define COMM_SUPPORT_CHANNEL_DETECT(hwdev) COMM_FEATURE_QW0(hwdev, CHANNEL_DETECT)
#define COMM_SUPPORT_MBOX_SEGMENT(hwdev) (hinic3_pcie_itf_id(hwdev) == SPU_HOST_ID)
#define COMM_SUPPORT_CMDQ_NUM(hwdev) COMM_FEATURE_QW0(hwdev, CMDQ_NUM)
#define COMM_SUPPORT_CMD_BUF_SIZE(hwdev) COMM_FEATURE_QW0(hwdev, CMD_BUF_SIZE)
#define COMM_SUPPORT_VIRTIO_VQ_SIZE(hwdev) COMM_FEATURE_QW0(hwdev, VIRTIO_VQ_SIZE)
#define COMM_IS_USE_REAL_RX_BUF_SIZE(hwdev) COMM_FEATURE_QW0(hwdev, USE_REAL_RX_BUF_SIZE)
#define COMM_SUPPORT_ONLY_ENHANCE_CMDQ(hwdev) COMM_FEATURE_QW0(hwdev, ONLY_ENHANCE_CMDQ)

void hinic3_set_func_host_mode(struct hinic3_hwdev *hwdev,
			       enum hinic3_func_mode mode);

#endif
