/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWDEV_H
#define SSS_HWDEV_H

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/timer.h>

#include "sss_hw_common.h"
#include "sss_hw_svc_cap.h"
#include "sss_hw_mbx_msg.h"
#include "sss_hw_statistics.h"
#include "sss_hw_event.h"

#include "sss_hwif.h"
#include "sss_mgmt_info.h"
#include "sss_ctrlq_info.h"
#include "sss_aeq_info.h"
#include "sss_ceq_info.h"
#include "sss_mbx_info.h"
#include "sss_mgmt_channel.h"

#define SSSNIC_CHANNEL_DETECT_PERIOD (5 * 1000)

enum sss_func_mode {
	SSS_FUNC_MOD_MIN,

	/* single host */
	SSS_FUNC_MOD_NORMAL_HOST = SSS_FUNC_MOD_MIN,

	/* multi host, bare-metal, sdi side */
	SSS_FUNC_MOD_MULTI_BM_MASTER,

	/* multi host, bare-metal, host side */
	SSS_FUNC_MOD_MULTI_BM_SLAVE,

	/* multi host, vm mode, sdi side */
	SSS_FUNC_MOD_MULTI_VM_MASTER,

	/* multi host, vm mode, host side */
	SSS_FUNC_MOD_MULTI_VM_SLAVE,

	SSS_FUNC_MOD_MAX = SSS_FUNC_MOD_MULTI_VM_SLAVE,
};

struct sss_page_addr {
	void	*virt_addr;
	u64		phys_addr;
};

struct sss_mqm_addr_trans_tbl_info {
	u32		chunk_num;
	u32		search_gpa_num;
	u32		page_size;
	u32		page_num;

	struct sss_page_addr *brm_srch_page_addr;
};

struct sss_devlink {
	void	*hwdev;
	u8		active_cfg_id; /* 1 ~ 8 */
	u8		switch_cfg_id;  /* 1 ~ 8 */
};

struct sss_heartbeat {
	u8					pcie_link_down;
	u8					heartbeat_lost;
	u16					rsvd;
	u32					pcie_link_down_cnt;
	struct timer_list	heartbeat_timer;
	struct work_struct	lost_work;
};

struct sss_aeq_stat {
	u16	busy_cnt;
	u16 rsvd;
	u64	cur_recv_cnt;
	u64	last_recv_cnt;
};

struct sss_clp_pf_to_mgmt {
	struct semaphore	clp_msg_lock;
	void			*clp_msg_buf;
};

struct sss_hwdev {
	void	*adapter_hdl; /* pointer to sss_pci_adapter or NDIS_Adapter */
	void	*pcidev_hdl; /* pointer to pcidev or Handler */

	/* pointer to pcidev->dev or Handler, for
	 * sdk_err() or dma_alloc()
	 */
	void	*dev_hdl;
	void	*chip_node;

	void	*service_adapter[SSS_SERVICE_TYPE_MAX];

	u32		wq_page_size;
	int		chip_present_flag;
	u8	poll; /* use polling mode or int mode */
	u8	rsvd[3];
	struct sss_hwif				*hwif; /* include void __iomem *bar */
	struct sss_comm_global_attr	glb_attr;
	u64							features[SSS_MAX_FEATURE_QWORD];

	struct	sss_mgmt_info		*mgmt_info;

	struct sss_ctrlq_info		*ctrlq_info;
	struct sss_aeq_info			*aeq_info;
	struct sss_ceq_info			*ceq_info;
	struct sss_mbx				*mbx; // mbx
	struct sss_msg_pf_to_mgmt	*pf_to_mgmt; // adm
	struct sss_clp_pf_to_mgmt *clp_pf_to_mgmt;

	struct sss_hw_stats			hw_stats;
	u8							*chip_fault_stats;

	sss_event_handler_t			event_handler;
	void						*event_handler_data;

	struct sss_board_info		board_info;

	struct delayed_work			sync_time_task;
	struct delayed_work			channel_detect_task;

	struct workqueue_struct		*workq;

	struct sss_heartbeat		heartbeat;

	ulong						func_state;
	spinlock_t			channel_lock; /* protect channel init and deinit */

	struct sss_devlink			*devlink_dev;

	enum sss_func_mode			func_mode;

	struct sss_aeq_stat			aeq_stat;

	u16 aeq_busy_cnt;
};

#define SSS_TO_HWDEV(ptr)			((struct sss_hwdev *)(ptr)->hwdev)
#define SSS_TO_DEV(hwdev)			(((struct sss_hwdev *)hwdev)->dev_hdl)
#define SSS_TO_HWIF(hwdev)			(((struct sss_hwdev *)hwdev)->hwif)
#define SSS_TO_MGMT_INFO(hwdev)		(((struct sss_hwdev *)hwdev)->mgmt_info)
#define SSS_TO_AEQ_INFO(hwdev)		(((struct sss_hwdev *)hwdev)->aeq_info)
#define SSS_TO_CEQ_INFO(hwdev)		(((struct sss_hwdev *)hwdev)->ceq_info)
#define SSS_TO_CTRLQ_INFO(hwdev)		(((struct sss_hwdev *)hwdev)->ctrlq_info)
#define SSS_TO_IRQ_INFO(hwdev)		(&((struct sss_hwdev *)hwdev)->mgmt_info->irq_info)
#define SSS_TO_SVC_CAP(hwdev)		(&(((struct sss_hwdev *)hwdev)->mgmt_info->svc_cap))
#define SSS_TO_NIC_CAP(hwdev)		(&(((struct sss_hwdev *)hwdev)->mgmt_info->svc_cap.nic_cap))
#define SSS_TO_MAX_SQ_NUM(hwdev)	\
	(((struct sss_hwdev *)hwdev)->mgmt_info->svc_cap.nic_cap.max_sq)
#define SSS_TO_PHY_PORT_ID(hwdev)	(((struct sss_hwdev *)hwdev)->mgmt_info->svc_cap.port_id)
#define SSS_TO_MAX_VF_NUM(hwdev)	(((struct sss_hwdev *)hwdev)->mgmt_info->svc_cap.max_vf)
#define SSS_TO_FUNC_COS_BITMAP(hwdev)	\
	(((struct sss_hwdev *)hwdev)->mgmt_info->svc_cap.cos_valid_bitmap)
#define SSS_TO_PORT_COS_BITMAP(hwdev)	\
	(((struct sss_hwdev *)hwdev)->mgmt_info->svc_cap.port_cos_valid_bitmap)

enum sss_servic_bit_define {
	SSS_SERVICE_BIT_NIC			= 0,
	SSS_SERVICE_BIT_ROCE		= 1,
	SSS_SERVICE_BIT_VBS			= 2,
	SSS_SERVICE_BIT_TOE			= 3,
	SSS_SERVICE_BIT_IPSEC		= 4,
	SSS_SERVICE_BIT_FC			= 5,
	SSS_SERVICE_BIT_VIRTIO		= 6,
	SSS_SERVICE_BIT_OVS			= 7,
	SSS_SERVICE_BIT_NVME		= 8,
	SSS_SERVICE_BIT_ROCEAA		= 9,
	SSS_SERVICE_BIT_CURRENET	= 10,
	SSS_SERVICE_BIT_PPA			= 11,
	SSS_SERVICE_BIT_MIGRATE		= 12,
	SSS_MAX_SERVICE_BIT
};

#define SSS_CFG_SERVICE_MASK_NIC		(0x1 << SSS_SERVICE_BIT_NIC)
#define SSS_CFG_SERVICE_MASK_ROCE		(0x1 << SSS_SERVICE_BIT_ROCE)
#define SSS_CFG_SERVICE_MASK_VBS		(0x1 << SSS_SERVICE_BIT_VBS)
#define SSS_CFG_SERVICE_MASK_TOE		(0x1 << SSS_SERVICE_BIT_TOE)
#define SSS_CFG_SERVICE_MASK_IPSEC		(0x1 << SSS_SERVICE_BIT_IPSEC)
#define SSS_CFG_SERVICE_MASK_FC			(0x1 << SSS_SERVICE_BIT_FC)
#define SSS_CFG_SERVICE_MASK_VIRTIO		(0x1 << SSS_SERVICE_BIT_VIRTIO)
#define SSS_CFG_SERVICE_MASK_OVS		(0x1 << SSS_SERVICE_BIT_OVS)
#define SSS_CFG_SERVICE_MASK_NVME		(0x1 << SSS_SERVICE_BIT_NVME)
#define SSS_CFG_SERVICE_MASK_ROCEAA		(0x1 << SSS_SERVICE_BIT_ROCEAA)
#define SSS_CFG_SERVICE_MASK_CURRENET	(0x1 << SSS_SERVICE_BIT_CURRENET)
#define SSS_CFG_SERVICE_MASK_PPA		(0x1 << SSS_SERVICE_BIT_PPA)
#define SSS_CFG_SERVICE_MASK_MIGRATE	(0x1 << SSS_SERVICE_BIT_MIGRATE)

#define SSS_CFG_SERVICE_RDMA_EN	SSS_CFG_SERVICE_MASK_ROCE

#define SSS_IS_NIC_TYPE(dev) \
	(((u32)(dev)->mgmt_info->svc_cap.chip_svc_type) & SSS_CFG_SERVICE_MASK_NIC)
#define SSS_IS_ROCE_TYPE(dev) \
	(((u32)(dev)->mgmt_info->svc_cap.chip_svc_type) & SSS_CFG_SERVICE_MASK_ROCE)
#define SSS_IS_VBS_TYPE(dev) \
	(((u32)(dev)->mgmt_info->svc_cap.chip_svc_type) & SSS_CFG_SERVICE_MASK_VBS)
#define SSS_IS_TOE_TYPE(dev) \
	(((u32)(dev)->mgmt_info->svc_cap.chip_svc_type) & SSS_CFG_SERVICE_MASK_TOE)
#define SSS_IS_IPSEC_TYPE(dev) \
	(((u32)(dev)->mgmt_info->svc_cap.chip_svc_type) & SSS_CFG_SERVICE_MASK_IPSEC)
#define SSS_IS_FC_TYPE(dev) \
	(((u32)(dev)->mgmt_info->svc_cap.chip_svc_type) & SSS_CFG_SERVICE_MASK_FC)
#define SSS_IS_OVS_TYPE(dev) \
	(((u32)(dev)->mgmt_info->svc_cap.chip_svc_type) & SSS_CFG_SERVICE_MASK_OVS)
#define SSS_IS_RDMA_TYPE(dev) \
	(((u32)(dev)->mgmt_info->svc_cap.chip_svc_type) & SSS_CFG_SERVICE_RDMA_EN)
#define SSS_IS_RDMA_ENABLE(dev) \
	((dev)->mgmt_info->svc_cap.sf_svc_attr.rdma_en)
#define SSS_IS_PPA_TYPE(dev) \
		(((u32)(dev)->mgmt_info->svc_cap.chip_svc_type) & SSS_CFG_SERVICE_MASK_PPA)
#define SSS_IS_MIGR_TYPE(dev) \
		(((u32)(dev)->mgmt_info->svc_cap.chip_svc_type) & SSS_CFG_SERVICE_MASK_MIGRATE)

#define SSS_MAX_HOST_NUM(hwdev)			((hwdev)->glb_attr.max_host_num)
#define SSS_MAX_PF_NUM(hwdev)			((hwdev)->glb_attr.max_pf_num)
#define SSS_MGMT_CPU_NODE_ID(hwdev) \
		((hwdev)->glb_attr.mgmt_host_node_id)

#define SSS_GET_FUNC_TYPE(hwdev)		((hwdev)->hwif->attr.func_type)
#define SSS_IS_PF(dev)					(SSS_GET_FUNC_TYPE(dev) == SSS_FUNC_TYPE_PF)
#define SSS_IS_VF(dev)					(SSS_GET_FUNC_TYPE(dev) == SSS_FUNC_TYPE_VF)
#define SSS_IS_PPF(dev) \
		(SSS_GET_FUNC_TYPE(dev) == SSS_FUNC_TYPE_PPF)

#define SSS_GET_FUNC_ID(hwdev)		((hwdev)->hwif->attr.func_id)

#define SSS_IS_BMGW_MASTER_HOST(hwdev)	\
		((hwdev)->func_mode == SSS_FUNC_MOD_MULTI_BM_MASTER)
#define SSS_IS_BMGW_SLAVE_HOST(hwdev)	\
		((hwdev)->func_mode == SSS_FUNC_MOD_MULTI_BM_SLAVE)
#define SSS_IS_VM_MASTER_HOST(hwdev)	\
		((hwdev)->func_mode == SSS_FUNC_MOD_MULTI_VM_MASTER)
#define SSS_IS_VM_SLAVE_HOST(hwdev)		\
		((hwdev)->func_mode == SSS_FUNC_MOD_MULTI_VM_SLAVE)

#define SSS_IS_MASTER_HOST(hwdev)		\
		(SSS_IS_BMGW_MASTER_HOST(hwdev) || SSS_IS_VM_MASTER_HOST(hwdev))

#define SSS_IS_SLAVE_HOST(hwdev)		\
		(SSS_IS_BMGW_SLAVE_HOST(hwdev) || SSS_IS_VM_SLAVE_HOST(hwdev))

#define SSS_IS_MULTI_HOST(hwdev)		\
		(SSS_IS_BMGW_MASTER_HOST(hwdev) || SSS_IS_BMGW_SLAVE_HOST(hwdev) || \
			SSS_IS_VM_MASTER_HOST(hwdev) || SSS_IS_VM_SLAVE_HOST(hwdev))

#define SSS_SPU_HOST_ID 4

#define SSS_SUPPORT_ADM_MSG(hwdev)		((hwdev)->features[0] & SSS_COMM_F_ADM)
#define SSS_SUPPORT_MBX_SEGMENT(hwdev)		\
			(SSS_GET_HWIF_PCI_INTF_ID((hwdev)->hwif) == SSS_SPU_HOST_ID)
#define SSS_SUPPORT_CTRLQ_NUM(hwdev)		\
			((hwdev)->features[0] & SSS_COMM_F_CTRLQ_NUM)
#define SSS_SUPPORT_VIRTIO_VQ_SIZE(hwdev)	\
			((hwdev)->features[0] & SSS_COMM_F_VIRTIO_VQ_SIZE)
#define SSS_SUPPORT_CHANNEL_DETECT(hwdev)	\
			((hwdev)->features[0] & SSS_COMM_F_CHANNEL_DETECT)
#define SSS_SUPPORT_CLP(hwdev)	\
			((hwdev)->features[0] & SSS_COMM_F_CLP)

enum {
	SSS_CFG_FREE = 0,
	SSS_CFG_BUSY = 1
};

int sss_init_pci(void);
void sss_exit_pci(void);

#endif
