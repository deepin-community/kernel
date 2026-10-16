// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include "main.h"
/* TODO: Adding this here is not ideal. Can we remove this warning now? */
#include "icrdma_hw.h"
#include <linux/debugfs.h>
#include "zrdma_kcompat.h"
#include "slib.h"
#include <linux/if_vlan.h>
#include <linux/if_macvlan.h>
#include <net/dcbnl.h>
#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/namei.h>

#define DRV_VER_MAJOR 26
#define DRV_VER_MINOR 10
#define DRV_VER_BUILD 01
#define DRV_VER_PATCH 07
#define DRV_VER                                                     \
	__stringify(DRV_VER_MAJOR) "." __stringify(DRV_VER_MINOR) "." \
	__stringify(DRV_VER_BUILD) "." __stringify(DRV_VER_PATCH)

#define FW_MAJOR_VER		0
#define FW_MINOR_FW_VER		0
#define FW_MINOR_DRV_VER	0

#define DRV_MAJOR_VER 		0
#define DRV_NET_MINOR_VER	0
#define DRV_RDMA_MINOR_VER	0
#define ZXDH_FW_VER_OFFSET 0x5400
#define MODULE_RDMA_ID 11

#ifndef SPEED_200000
#define SPEED_200000 200000
#endif

#define HMC_QPC_SIZE         (512)
#define HMC_CQC_SIZE         (64)
#define HMC_SRQC_SIZE        (64)
#define HMC_TXWINDOW_SIZE    (64 * 512)
#define HMC_IRD_SIZE         (64 * 32)
#define HMC_AHC_SIZE         (64)
#define HMC_MRTE_SIZE        (64)
#define HMC_PBLE_SIZE        (8)

struct rdma_sriov_glb_info pf_sriov_glb_info[HOST_RDMA_MAX_PF] = {0};

static u8 resource_profile;
module_param(resource_profile, byte, 0444);
MODULE_PARM_DESC(
	resource_profile,
	"Resource Profile: 0=PF only(default), 1=Weighted VF, 2=Even Distribution");

static u8 max_rdma_vfs = 1;
module_param(max_rdma_vfs, byte, 0444);
MODULE_PARM_DESC(max_rdma_vfs, "Maximum VF count: 0-32, default=1");

bool zxdh_upload_context;
module_param(zxdh_upload_context, bool, 0444);
MODULE_PARM_DESC(zxdh_upload_context, "Upload QP context, default=false");

bool enable_iova_cap = false;
module_param(enable_iova_cap, bool, 0444);
MODULE_PARM_DESC(enable_iova_cap, "Enable iova cap, default=false");

static unsigned int limits_sel = 3;
module_param(limits_sel, uint, 0444);
MODULE_PARM_DESC(limits_sel, "Resource limits selector, Range: 0-7, default=3");

static unsigned int gen1_limits_sel = 1;
module_param(gen1_limits_sel, uint, 0444);
MODULE_PARM_DESC(gen1_limits_sel,
		 "x722 resource limits selector, Range: 0-5, default=1");

static unsigned int roce_ena = 1;
module_param(roce_ena, uint, 0444);
MODULE_PARM_DESC(
	roce_ena,
	"RoCE enable: 1=enable RoCEv2 on all ports (not supported on x722), 0=iWARP(default)");

static ulong roce_port_cfg;
module_param(roce_port_cfg, ulong, 0444);
MODULE_PARM_DESC(
	roce_port_cfg,
	"RoCEv2 per port enable: 1=port0 RoCEv2 all others iWARP, 2=port1 RoCEv2 etc. not supported on X722");

static bool en_rem_endpoint_trk;
module_param(en_rem_endpoint_trk, bool, 0444);
MODULE_PARM_DESC(
	en_rem_endpoint_trk,
	"Remote Endpoint Tracking: 1=enabled (not supported on x722), 0=disabled(default)");

static u8 fragment_count_limit = 6;
module_param(fragment_count_limit, byte, 0444);
MODULE_PARM_DESC(
	fragment_count_limit,
	"adjust maximum values for queue depth and inline data size, default=4, Range: 2-13");

/******************Advanced RoCEv2 congestion knobs***********************************************/
static bool dcqcn_enable;
module_param(dcqcn_enable, bool, 0444);
MODULE_PARM_DESC(
	dcqcn_enable,
	"enables DCQCN algorithm for RoCEv2 on all ports, default=false ");

static bool dcqcn_cc_cfg_valid;
module_param(dcqcn_cc_cfg_valid, bool, 0444);
MODULE_PARM_DESC(dcqcn_cc_cfg_valid,
		 "set DCQCN parameters to be valid, default=false");

static u8 dcqcn_min_dec_factor = 1;
module_param(dcqcn_min_dec_factor, byte, 0444);
MODULE_PARM_DESC(
	dcqcn_min_dec_factor,
	"set minimum percentage factor by which tx rate can be changed for CNP, Range: 1-100, default=1");

static u8 dcqcn_min_rate_MBps;
module_param(dcqcn_min_rate_MBps, byte, 0444);
MODULE_PARM_DESC(dcqcn_min_rate_MBps,
		 "set minimum rate limit value, in MBits per second, default=0");

static u8 dcqcn_F;
module_param(dcqcn_F, byte, 0444);
MODULE_PARM_DESC(
	dcqcn_F,
	"set number of times to stay in each stage of bandwidth recovery, default=0");

static unsigned short dcqcn_T;
module_param(dcqcn_T, ushort, 0444);
MODULE_PARM_DESC(
	dcqcn_T,
	"set number of usecs that should elapse before increasing the CWND in DCQCN mode, default=0");

static unsigned int dcqcn_B;
module_param(dcqcn_B, uint, 0444);
MODULE_PARM_DESC(
	dcqcn_B,
	"set number of MSS to add to the congestion window in additive increase mode, default=0");

static unsigned short dcqcn_rai_factor;
module_param(dcqcn_rai_factor, ushort, 0444);
MODULE_PARM_DESC(
	dcqcn_rai_factor,
	"set number of MSS to add to the congestion window in additive increase mode, default=0");

static unsigned short dcqcn_hai_factor;
module_param(dcqcn_hai_factor, ushort, 0444);
MODULE_PARM_DESC(
	dcqcn_hai_factor,
	"set number of MSS to add to the congestion window in hyperactive increase mode, default=0");

static unsigned int dcqcn_rreduce_mperiod;
module_param(dcqcn_rreduce_mperiod, uint, 0444);
MODULE_PARM_DESC(
	dcqcn_rreduce_mperiod,
	"set minimum time between 2 consecutive rate reductions for a single flow, default=0");

static u8 display_drv_side_fw_ver = 0;
module_param(display_drv_side_fw_ver, byte, 0444);
MODULE_PARM_DESC(display_drv_side_fw_ver, "display fw ver, display=1, not display=0");

static u8 display_drv_side_net_ver = 0;
module_param(display_drv_side_net_ver, byte, 0444);
MODULE_PARM_DESC(display_drv_side_net_ver, "display drv ver, display=1, not display=0");

/****************************************************************************************************************/

MODULE_ALIAS("zrdma");
MODULE_AUTHOR("ZTE");
MODULE_DESCRIPTION("ZTE(R) Ethernet Protocol Driver for RDMA");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(DRV_VER);
struct mutex rdma_name_lock;
struct list_head zxdh_rdma_list;

static void zxdh_rdma_name_lock_init(void)
{
    mutex_init(&rdma_name_lock);
}

static void zxdh_rdma_name_lock_deinit(void)
{
    mutex_destroy(&rdma_name_lock);
}

static int zxdh_add_rdma_name(const char *sbdf, const char *name)
{
    struct zxdh_rdma_node *new_node, *pos;
    u8 len = 0;
    
    if ((sbdf == NULL) || (name == NULL))
        return -EINVAL;
    if (zrdma_strlen_s(sbdf) > PCI_SBDF_SAFE_LEN) {
        pr_err("[zxdh_rdma] %s[%d]sbdf len out of range\n",__func__,__LINE__);
        return -EINVAL;
    }
	if (zrdma_strlen_s(name) > IB_DEVICE_NAME_MAX) {
        pr_err("[zxdh_rdma] %s[%d]name len out of range\n",__func__,__LINE__);
        return -EINVAL;
    }

    mutex_lock(&rdma_name_lock);
    list_for_each_entry(pos, &zxdh_rdma_list, list) {
        len = zrdma_strlen_s(pos->sbdf);
        if ((zrdma_strlen_s(sbdf) == len)) {
            if (strncasecmp(sbdf, pos->sbdf, len) == 0) {
                mutex_unlock(&rdma_name_lock);
                return -EEXIST;
            }
        }
    }

    new_node = kzalloc(sizeof(struct zxdh_rdma_node), GFP_ATOMIC);
    if (!new_node) {
        pr_err("[zxdh_rdma]%s alloc memory failed!\n", __func__);
        mutex_unlock(&rdma_name_lock);
        return -ENOMEM;
    }
    zrdma_snprintf_s(new_node->sbdf, PCI_SBDF_SAFE_LEN, "%s", sbdf);
    zrdma_snprintf_s(new_node->name, IB_DEVICE_NAME_MAX, "%s", name);
    list_add_tail(&new_node->list, &zxdh_rdma_list);
    mutex_unlock(&rdma_name_lock);

    return 0;
}

static void zxdh_del_all_rdma_name(void)
{
    struct zxdh_rdma_node *tmp, *pos;
    mutex_lock(&rdma_name_lock);
    list_for_each_entry_safe(pos, tmp, &zxdh_rdma_list, list) {
        list_del(&pos->list);
        kfree(pos);
    }
    mutex_unlock(&rdma_name_lock);
}
int zxdh_get_del_rdma_name(const char *sbdf, char *name)
{
    struct zxdh_rdma_node *pos, *tmp;
    u8 len = 0;
    
    if ((sbdf == NULL) || (name == NULL)) {
        return -EINVAL;
    }

    if (zrdma_strlen_s(name) > IB_DEVICE_NAME_MAX) {
        pr_err("[zxdh_rdma] %s[%d]name len out of range\n",__func__,__LINE__);
        return -EINVAL;
    }
    
    mutex_lock(&rdma_name_lock);
    list_for_each_entry_safe(pos, tmp, &zxdh_rdma_list, list) {
        len = zrdma_strlen_s(pos->sbdf);
        if ((zrdma_strlen_s(sbdf) == len)) {
            if (strncasecmp(sbdf, pos->sbdf, len) == 0) {
                zrdma_snprintf_s(name, IB_DEVICE_NAME_MAX, "%s", pos->name);
                list_del(&pos->list);
                kfree(pos);
                mutex_unlock(&rdma_name_lock);
                return 0;
            }
        } 
    }
    mutex_unlock(&rdma_name_lock);
    return -ENOMEM;
}

u16 zxdh_fw_feature_get(struct zxdh_pci_f *rf, uint32_t feature_bit)
{
	struct iidc_core_dev_info *cdev_info = rf->cdev;
	void __iomem *base;
	u32 fw_lo, fw_hi, features32;
    u16 ret =0;

	if (!cdev_info)
		return ret;

	if (feature_bit >= ZXDH_FW_FEATURE_BIT_SIZE)
		return ret;

	base = cdev_info->hw_addr + ZXDH_FW_FEATURE_ADDR_OFFSET;
	fw_lo = readl(base + 4);
	fw_hi = readl(base + 8);
	features32 = (fw_lo >> 16) | (fw_hi << 16);
    ret = (u16)((features32 >> feature_bit) & 0x1);
    return ret;
}

u16 zxdh_mtu_enum_to_int(u16 mtu)
{
	if (mtu >= ZXDH_PMTU_ENUM_4096)
		return 4096;
	else if (mtu >= ZXDH_PMTU_ENUM_2048)
		return 2048;
	else if (mtu >= ZXDH_PMTU_ENUM_1024)
		return 1024;
	else if (mtu >= ZXDH_PMTU_ENUM_512)
		return 512;
	else
		return 256;
}

int zxdh_dpp_tbl_pmtu_info_add(struct zxdh_pci_f *rf, dpp_pf_info_t* pf_info, u32 pmtu, u32 qp_num)
{
	zxdh_dpp_pmtu_info_t pmtu_info;
	int ret = -EINVAL;
	zrdma_memset_s(&pmtu_info, 0, sizeof(pmtu_info));
	
	if (rf == NULL)
		return -ENOMEM;

	if (pf_info == NULL)
        return -ENOMEM;

	if ((rf->ftype == 1) && (rf->vm_info.vm_mode)) {
		return -EINVAL;
	}

	if (rf->drv_np_cap == ZXDH_RDMA_COMMON_FUNC_CAP) {
        if (rf->gen_ops.zxdh_common_func) {
		    pmtu_info.pf_info.slot = pf_info->slot;
			pmtu_info.pf_info.vport = pf_info->vport;
			pmtu_info.pmtu_info.key.qp = qp_num;
		    pmtu_info.pmtu_info.entry.pmtu = pmtu;
            ret = rf->gen_ops.zxdh_common_func(&pmtu_info, NULL, ZXDH_FUNC_PMTU_INFO_ADD);
		}
	} else
	    return -EINVAL;
	return ret;
}

int zxdh_dpp_tbl_pmtu_info_del(struct zxdh_pci_f *rf, dpp_pf_info_t* pf_info, u32 qp_num)
{
	zxdh_dpp_pmtu_info_t pmtu_info;
	int ret = -EINVAL;

	zrdma_memset_s(&pmtu_info, 0, sizeof(pmtu_info));
	
	if (rf == NULL)
		return -ENOMEM;

	if (pf_info == NULL)
        return -ENOMEM;

	if ((rf->ftype == 1) && (rf->vm_info.vm_mode)) {
		return -EINVAL;
	}

	if (rf->drv_np_cap == ZXDH_RDMA_COMMON_FUNC_CAP) {
        if (rf->gen_ops.zxdh_common_func) {
			pmtu_info.pf_info.slot = pf_info->slot;
			pmtu_info.pf_info.vport = pf_info->vport;
		    pmtu_info.pmtu_info.key.qp = qp_num;
            ret = rf->gen_ops.zxdh_common_func(&pmtu_info, NULL, ZXDH_FUNC_PMTU_INFO_DEL);
		}
	} else
	    return -EINVAL;
	return ret;
}

int zxdh_dpp_tbl_qp_info_del_ex(struct zxdh_pci_f *rf, dpp_pf_info_t* pf_info, u32 qp_num)
{
	zxdh_dpp_qp_info_t qp_info;
	int ret = -EINVAL;

	zrdma_memset_s(&qp_info, 0, sizeof(qp_info));
	
	if (rf == NULL)
		return -ENOMEM;

	if (pf_info == NULL)
        return -ENOMEM;
	
	if ((rf->ftype == 1) && (rf->vm_info.vm_mode)) {
		return -EINVAL;
	}

	if (rf->drv_np_cap == ZXDH_RDMA_COMMON_FUNC_CAP) {
        if (rf->gen_ops.zxdh_common_func) {
			qp_info.pf_info.slot = pf_info->slot;
			qp_info.pf_info.vport = pf_info->vport;
		    qp_info.qp_info.key.qp = qp_num;
            ret = rf->gen_ops.zxdh_common_func(&qp_info, NULL, ZXDH_FUNC_QP_INFO_DEL_WX);
		}
	} else
	    return -EINVAL;
	return ret;
}

static void zxdh_init_mac_list(struct zxdh_device *iwdev)
{
    INIT_LIST_HEAD(&iwdev->mac_node_list);
    spin_lock_init(&iwdev->mac_node_list_lock);
}

static struct zxdh_mac_node *zxdh_find_mac_node(struct zxdh_device *iwdev, const u8 *mac)
{
    struct zxdh_mac_node *mac_node = NULL;

    list_for_each_entry(mac_node, &iwdev->mac_node_list, lnode) {
        if (memcmp(mac_node->mac, mac, ETH_ALEN) == 0) {
            return mac_node;
        }
    }
    return NULL;
}

static void zxdh_find_mac_of_dev(struct zxdh_find_result *find_result,
		struct zxdh_device *iwdev, struct net_device *netdev)
{
	struct zxdh_mac_node *mac_node = NULL;
	struct net_dev_entry *entry;
	unsigned long flags;

	spin_lock_irqsave(&iwdev->mac_node_list_lock, flags);

	list_for_each_entry(mac_node, &iwdev->mac_node_list, lnode) {
		list_for_each_entry(entry, &mac_node->head, lnode) {
			if (entry->dev == netdev) {
				memcpy(find_result->mac, mac_node->mac, ETH_ALEN);
				find_result->found = true;
				find_result->mac_node = mac_node;
				break;
			}
		}
	}

	spin_unlock_irqrestore(&iwdev->mac_node_list_lock, flags);
}


static void zxdh_lookup_dev_in_mac_node(struct zxdh_find_result *find_result,
			struct zxdh_device *iwdev, struct net_device *netdev)
{
    struct zxdh_mac_node *mac_node = NULL;
    struct net_dev_entry *entry;
    unsigned long flags;
    const u8 *mac = netdev->dev_addr;

    spin_lock_irqsave(&iwdev->mac_node_list_lock, flags);

    mac_node = zxdh_find_mac_node(iwdev, mac);

    if (mac_node == NULL) {
        goto unlock_out;
    }

    find_result->mac_node = mac_node;

    list_for_each_entry(entry, &mac_node->head, lnode) {
        if (entry->dev == netdev) {
            memcpy(find_result->mac, mac_node->mac, ETH_ALEN);
            find_result->found = true;
            break;
        }
    }

unlock_out:
    spin_unlock_irqrestore(&iwdev->mac_node_list_lock, flags);
}

static int zxdh_add_dev_entry(struct zxdh_device *iwdev,
			struct net_device *netdev, struct zxdh_mac_node *mac_node)
{
    struct net_dev_entry *new_entry;
    int cur_dev_num = -1;

    unsigned long flags;
    const u8 *mac = netdev->dev_addr;

    spin_lock_irqsave(&iwdev->mac_node_list_lock, flags);

    if (mac_node == NULL) {
        // 1. 创建新的根节点
        mac_node = kzalloc(sizeof(*mac_node), GFP_ATOMIC);
        if (!mac_node) {
            pr_err("[zxdh_rdma] %s[%d]:Failed to allocate mac_root_node\n", __func__, __LINE__);
            goto err_out;
        }

        memcpy(mac_node->mac, mac, ETH_ALEN);
        atomic_set(&mac_node->child_count, 0);
        INIT_LIST_HEAD(&mac_node->head);
        list_add(&mac_node->lnode, &iwdev->mac_node_list);
    }

    // 2. 创建子节点并插入链表
    new_entry = kzalloc(sizeof(*new_entry), GFP_ATOMIC);
    if (!new_entry) {
        pr_err("[zxdh_rdma] %s[%d]:Failed to allocate net_dev_entry\n", __func__, __LINE__);
        if (atomic_read(&mac_node->child_count) == 0)
            kfree(mac_node);
        goto err_out;
    }

    new_entry->dev = netdev;
    list_add(&new_entry->lnode, &mac_node->head);
    cur_dev_num = atomic_inc_return(&mac_node->child_count);

err_out:
    spin_unlock_irqrestore(&iwdev->mac_node_list_lock, flags);
    return cur_dev_num;
}


static int zxdh_del_mac_dev_node(struct zxdh_device *iwdev,
			struct net_device *netdev, struct zxdh_mac_node *mac_node)
{
    struct net_dev_entry *entry, *entry_tmp;
    int cur_dev_num = -1;

    unsigned long flags;
    spin_lock_irqsave(&iwdev->mac_node_list_lock, flags);

    list_for_each_entry_safe(entry, entry_tmp, &mac_node->head, lnode) {
        if (entry->dev == netdev) {
            list_del(&entry->lnode);
            kfree(entry);

            cur_dev_num = atomic_dec_return(&mac_node->child_count);
            if (cur_dev_num == 0) {
                list_del(&mac_node->lnode); 
                kfree(mac_node);
            }
            goto out;
        }
    }

out:
    spin_unlock_irqrestore(&iwdev->mac_node_list_lock, flags);
    return cur_dev_num;
}

int zxdh_dpp_get_trans_item_pos(struct zxdh_pci_f *rf, dpp_pf_info_t* pf_info, const void *mac, struct zxdh_np_zcam_pos_info *p_pos_info)
{
	zxdh_dpp_pf_mac_info_t pf_mac_info;
	int ret = -EINVAL;

	if (rf == NULL || pf_info == NULL) {
		pr_err("[zxdh_rdma][%s][%d]: rf:0x%p or pf_info:0x%p is NULL!\n",
		       __func__, __LINE__, rf, pf_info);
		return -EINVAL;
	}

	if (rf->drv_np_cap == ZXDH_RDMA_COMMON_FUNC_CAP) {
        if (rf->gen_ops.zxdh_common_func) {
		    pf_mac_info.mac = mac;
		    pf_mac_info.pf_info.slot = pf_info->slot;
		    pf_mac_info.pf_info.vport = pf_info->vport;
            ret = rf->gen_ops.zxdh_common_func(&pf_mac_info, p_pos_info, ZXDH_FUNC_NP_MAC);
		}
	} else
	    return -EINVAL;
	return ret;
}

static int zxdh_dpp_get_vhca(struct zxdh_pci_f *rf, dpp_pf_info_t* pf_info, const void *mac, uint32_t *vhca_id)
{
	zxdh_dpp_pf_mac_info_t pf_mac_info;
	int ret = 0;

	if (rf == NULL || pf_info == NULL) {
		pr_err("[zxdh_rdma][%s][%d]: rf:0x%p or pf_info:0x%p is NULL!\n",
		       __func__, __LINE__, rf, pf_info);
		return -EINVAL;
	}

	if (rf->drv_np_cap == ZXDH_RDMA_COMMON_FUNC_CAP) {
		if (rf->gen_ops.zxdh_common_func) {
			pf_mac_info.mac = mac;
			pf_mac_info.pf_info.slot = pf_info->slot;
			pf_mac_info.pf_info.vport = pf_info->vport;
			ret = rf->gen_ops.zxdh_common_func(&pf_mac_info, vhca_id, ZXDH_FUNC_SEARCH_MAC_FROM_FW);
		}
	}

	if (0xff == ret) {
		pr_err("[zxdh_rdma][%s][%d]: zxdh_kernel doesn't support search mac!\n", __func__, __LINE__);
	}

	return ret;
}

static int pf_handle_mac_to_np(struct zxdh_pci_f *rf, const u8 *mac, u8 op_code)
{
	dpp_pf_info_t pf_info = { 0 };
	struct zxdh_np_zcam_pos_info pos_info = { 0 };
	int ret = 0;
	int pos_sta = 0;
	struct iidc_core_dev_info *cdev_info = rf->cdev;
	bool mac_in_use = false;
	zxdh_mac_info_t mac_info = {0};
	
	pf_info.vport = cdev_info->vport_id;
	pf_info.slot = cdev_info->slot_id;
	mac_info.mac_addr = mac;
	mac_info.pf_info = &pf_info;
	mac_info.vhca_id = rf->sc_dev.vhca_id;
	mac_in_use = zxdh_check_mac_in_use(rf, &mac_info);
	if (mac_in_use) {
		if (ZXDH_CMD_NP_MAC_ADD == op_code) {
			pr_info("[zxdh_rdma][%s][%d] mac:%02x:%02x:%02x:%02x:%02x:%02x is in use, cannot add mac to vhca:%u\n",
			       __func__, __LINE__, mac[0], mac[1], mac[2],
			       mac[3], mac[4], mac[5], mac_info.vhca_id);
		}

		return 0;
	}

	if (ZXDH_CMD_NP_MAC_ADD == op_code) {
		ret = dpp_add_rdma_trans_item(&pf_info, mac, rf->sc_dev.vhca_id);
		if (ret) {
			pr_err("[zxdh_rdma] %s[%d]:dpp_add_rdma_trans_item failed ret:%d\n", __func__, __LINE__, ret);
			return ret;
		}
		pos_sta = zxdh_dpp_get_trans_item_pos(rf, &pf_info, mac, &pos_info);
		if (pos_sta) {
			pr_err("[zxdh_rdma] %s[%d]:add mac to np failed failed ret:%d\n", __func__, __LINE__, pos_sta);
			return pos_sta;
		}

		zxdh_process_np_mac(rf, ZXDH_CMD_NP_MAC_ADD, rf->sc_dev.vhca_id, &pos_info);

	} else if (ZXDH_CMD_NP_MAC_DEL == op_code) {
		pos_sta = zxdh_dpp_get_trans_item_pos(rf, &pf_info, mac, &pos_info);
		if (pos_sta) {
			pr_err("[zxdh_rdma] %s[%d]:get mac in np tble failed ret:%d\n", __func__, __LINE__, pos_sta);
			return pos_sta;
		}

		ret = dpp_del_rdma_trans_item(&pf_info, mac);
		if (ret) {
			pr_err("[zxdh_rdma] %s[%d]:dpp_del_rdma_trans_item failed ret:%d\n", __func__, __LINE__, ret);
			return ret;
		}
		zxdh_process_np_mac(rf, ZXDH_CMD_NP_MAC_DEL, rf->sc_dev.vhca_id, &pos_info);
	}
	return ret;
}

static int vf_handle_mac_to_np(struct zxdh_pci_f *rf, const u8 *mac, u8 op_code)
{
	dpp_pf_info_t pf_info = { 0 };
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct iidc_core_dev_info *cdev_info = rf->cdev;
	u64 mac_msg3 = 0;
	u32 cnt = 0, val = 0;

	pf_info.vport = cdev_info->vport_id;
	pf_info.slot = cdev_info->slot_id;

	memcpy(&mac_msg3, mac, ETH_ALEN);

	if (ZXDH_CMD_NP_MAC_ADD == op_code) {
		writel(0, (u32 __iomem *)(dev->hw->hw_addr + C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id)));

		zxdh_sc_send_mailbox_cmd(dev, ZTE_ZXDH_OP_REQ_NP_MAC_ADD,
				pf_info.vport, mac_msg3, 0, rf->vf_id);

		do {
			val = readl(dev->hw->hw_addr + C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id));
			if (cnt++ > ZXDH_MAILBOX_CYC_NUM * dev->hw_attrs.max_done_count) {
				pr_info("[zxdh_rdma] vhca_id:%d waiting completed NP_MAC_ADD mailbox too long time,timeout!\n", dev->vhca_id);
				return -ETIMEDOUT;
			}
			if (dev->hw_attrs.skip_hw == true) {
				pr_info("[zxdh_rdma] [%s]%d skip_hw is true\n", __func__, __LINE__);
				return -ETIMEDOUT;
			}
			usleep_range(ZXDH_MAILBOX_SLEEP_TIME, ZXDH_MAILBOX_SLEEP_TIME + 10);
		} while (!val);

	} else if (ZXDH_CMD_NP_MAC_DEL == op_code) {

		writel(0, (u32 __iomem *)(dev->hw->hw_addr + C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id)));

		zxdh_sc_send_mailbox_cmd(dev, ZTE_ZXDH_OP_REQ_NP_MAC_DEL,
				pf_info.vport, mac_msg3, 0, rf->vf_id);

		do {
			val = readl(dev->hw->hw_addr + C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id));
			if (cnt++ > ZXDH_MAILBOX_CYC_NUM * dev->hw_attrs.max_done_count) {
				pr_info("[zxdh_rdma] vhca_id:%d waiting completed NP_MAC_DEL mailbox too long time,timeout!\n", dev->vhca_id);
				return -ETIMEDOUT;
			}
			if (dev->hw_attrs.skip_hw == true) {
				pr_info("[zxdh_rdma] [%s]%d skip_hw is true\n", __func__, __LINE__);
				return -ETIMEDOUT;
			}
			usleep_range(ZXDH_MAILBOX_SLEEP_TIME, ZXDH_MAILBOX_SLEEP_TIME + 10);
		} while (!val);
	}
	return 0;
}

bool zxdh_check_mac_in_use(struct zxdh_pci_f *rf, zxdh_mac_info_t *mac_info)
{
	uint32_t vhca_id = 0;
	int ret = 0;
	const u8 *mac;

	mac = mac_info->mac_addr;
	ret = zxdh_dpp_get_vhca(rf, mac_info->pf_info, mac_info->mac_addr, &vhca_id);
	if (ret != 0) {
		// 没找到mac, 可以继续进行正常流程
		return false;
	}

	if (vhca_id != mac_info->vhca_id) {
		// 找到了这个mac， 这个mac被别的设备使用，不可继续
		pr_info("[zxdh_rdma][%s][%d] mac:%02x:%02x:%02x:%02x:%02x:%02x is used by vhca_id:%u, input vhca_id:%u\n", __func__, __LINE__,
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], vhca_id, mac_info->vhca_id);
		return true;
	}

	return false;
}

void zxdh_add_dpp_mac_tbl(struct zxdh_device *iwdev, struct net_device *netdev)
{
	struct zxdh_pci_f *rf = iwdev->rf;
	int ret = 0;
	const u8 *new_mac;
	int cur_dev_num = 0;
	struct zxdh_find_result *find_result;

	if (!netdev)
		return;

	find_result = kzalloc(sizeof(*find_result), GFP_KERNEL);
	if (!find_result) {
		pr_err("[zxdh_rdma] %s[%d]:kzalloc failed!\n", __func__, __LINE__);
		return;
	}

	new_mac = netdev->dev_addr;
	zxdh_lookup_dev_in_mac_node(find_result, iwdev, netdev);
	if (find_result->found == true) {
		pr_info("[zxdh_rdma] %s[%d]:find_result->found %d\n", __func__, __LINE__, find_result->found);
		goto err_out;
	}

	cur_dev_num = zxdh_add_dev_entry(iwdev, netdev, find_result->mac_node);

	if (cur_dev_num <= 0) {
		pr_err("[zxdh_rdma] %s[%d]:cur_dev_num should bigger than 0!\n", __func__, __LINE__);
		goto err_out;
	}

	if (cur_dev_num == 1) {
		if (!iwdev->rf->ftype) {
			ret = pf_handle_mac_to_np(rf, new_mac, ZXDH_CMD_NP_MAC_ADD);
		} else {
			ret = vf_handle_mac_to_np(rf, new_mac, ZXDH_CMD_NP_MAC_ADD);
		}
		if (ret) {
			pr_err("[zxdh_rdma] %s[%d]:handle mac to np failed! ret:%d\n", __func__, __LINE__, ret);
		}
	}

err_out:
	kfree(find_result);
	return;
}

int zxdh_del_dpp_mac_tbl(struct zxdh_device *iwdev, struct net_device *netdev, bool find_all)
{
	struct zxdh_pci_f *rf = iwdev->rf;
	int ret = 0;
	int cur_dev_num;
	struct zxdh_find_result *find_result;

	if (!netdev || list_empty(&iwdev->mac_node_list)) {
		return -ENODEV;
	}

	find_result = kzalloc(sizeof(*find_result), GFP_KERNEL);
	if (!find_result) {
		pr_err("[zxdh_rdma] %s[%d]:kzalloc failed!\n", __func__, __LINE__);
		return -ENOMEM;
	}
	if (find_all) {
		zxdh_find_mac_of_dev(find_result, iwdev, netdev);
	} else {
		zxdh_lookup_dev_in_mac_node(find_result, iwdev, netdev);
	}

	if (find_result->mac_node == NULL || find_result->found == false) {
		pr_info("[zxdh_rdma] %s[%d]: didn't find mac node or netdev(%s)\n", __func__, __LINE__, netdev->name);
		ret = -ENODEV;
		goto err_out;
	}

	cur_dev_num = zxdh_del_mac_dev_node(iwdev, netdev, find_result->mac_node);
	if (cur_dev_num == 0) {
		if (!iwdev->rf->ftype) {
			ret = pf_handle_mac_to_np(rf, find_result->mac, ZXDH_CMD_NP_MAC_DEL);
		} else {
			ret = vf_handle_mac_to_np(rf, find_result->mac, ZXDH_CMD_NP_MAC_DEL);
		}
		if (ret) {
			pr_err("[zxdh_rdma] %s[%d]:handle mac to np failed! ret:%d\n", __func__, __LINE__, ret);
		}
	}

err_out:
	kfree(find_result);
	return ret;
}


void zxdh_update_dpp_mac_tbl(struct zxdh_device *iwdev, struct net_device *netdev)
{
	if (!netdev)
		return;

	zxdh_del_dpp_mac_tbl(iwdev, netdev, true);

	zxdh_add_dpp_mac_tbl(iwdev, netdev);

	return;
}

static void zxdh_cleanup_dpp_mac_tbl(struct zxdh_device *iwdev)
{
    struct zxdh_mac_node *mac_node, *mac_tmp, *mac_list, *mac_entry;
    struct net_dev_entry *entry, *entry_tmp;
    struct zxdh_pci_f *rf = iwdev->rf;
    unsigned long flags;

    // list头，以及初始化
    mac_list = kzalloc(sizeof(*mac_list), GFP_KERNEL);
    if (unlikely(!mac_list))
        return;
    INIT_LIST_HEAD(&mac_list->lnode);

    spin_lock_irqsave(&iwdev->mac_node_list_lock, flags);

    list_for_each_entry_safe(mac_node, mac_tmp, &iwdev->mac_node_list, lnode) {
        list_for_each_entry_safe(entry, entry_tmp, &mac_node->head, lnode) {
            list_del(&entry->lnode);
            kfree(entry); 
        }

        mac_entry = kmalloc(sizeof(*mac_entry), GFP_ATOMIC);
        if (unlikely(!mac_entry)) {
            pr_err("Failed to allocate memory. Stopping capture.\n");
            goto clear_out;
        }
        memcpy(mac_entry->mac, mac_node->mac, ETH_ALEN);
        list_add_tail(&mac_entry->lnode, &mac_list->lnode);
        list_del(&mac_node->lnode);
        kfree(mac_node);
    }

clear_out:
    spin_unlock_irqrestore(&iwdev->mac_node_list_lock, flags);
    list_for_each_entry_safe(mac_entry, mac_tmp, &mac_list->lnode, lnode) {

        if (!iwdev->rf->ftype) {
            pf_handle_mac_to_np(rf, mac_entry->mac, ZXDH_CMD_NP_MAC_DEL);
        } else {
            vf_handle_mac_to_np(rf, mac_entry->mac, ZXDH_CMD_NP_MAC_DEL);
        }
        list_del(&mac_entry->lnode);
        kfree(mac_entry);
    }

    kfree(mac_list);
}

static enum ib_port_state get_port_state(struct net_device *netdev)
{
	if (netif_carrier_ok(netdev) && netif_running(netdev))
		return IB_PORT_ACTIVE;

	return IB_PORT_DOWN;
}

static int zxdh_netdevice_master_event(struct zxdh_device *iwdev,
				unsigned long event, struct net_device *netdev)
{
	enum ib_port_state state;
	u8 last_iw_status = iwdev->iw_status;
	switch (event) {
	case NETDEV_CHANGE:
	case NETDEV_UP:
	case NETDEV_DOWN:
		state = get_port_state(netdev);

		if (refcount_read(&iwdev->trace_switch.t_switch)) {
			if (state == IB_PORT_ACTIVE)
				ibdev_notice(&iwdev->ibdev, "IB port up\n");
			else
				ibdev_notice(&iwdev->ibdev, "IB port down\n");
		}
		if (state == IB_PORT_DOWN) {
			iwdev->iw_status = 0;
		} else if (state == IB_PORT_ACTIVE) {
			iwdev->iw_status = 1;
		} else {
			break;
		}
		if (last_iw_status != iwdev->iw_status) {
			zxdh_port_ibevent(iwdev);
		}

		break;
	case NETDEV_CHANGEMTU:
		pr_info("[zxdh_rdma] %s changed mtu to %d\n", netdev->name, netdev->mtu);
		break;
	case NETDEV_CHANGEADDR:
		pr_info("[zxdh_rdma] %s[%d]: process NETDEV_CHANGEADDR event=%ld, for %s update dpp mac tbl\n", __func__, __LINE__, event, netdev->name);
		zxdh_update_dpp_mac_tbl(iwdev, netdev);
		break;
	case NETDEV_REGISTER:
		pr_info("[zxdh_rdma] %s[%d]: process NETDEV_REGISTER event=%ld, for %s add dpp mac tbl\n", __func__, __LINE__, event, netdev->name);
		zxdh_add_dpp_mac_tbl(iwdev, netdev);
		break;
	case NETDEV_UNREGISTER:
		pr_info("[zxdh_rdma] %s[%d]: process NETDEV_UNREGISTER event=%ld, for %s del dpp mac tbl\n", __func__, __LINE__, event, netdev->name);
		zxdh_del_dpp_mac_tbl(iwdev, netdev, false);
		break;
	default:
		pr_info("[zxdh_rdma] %s[%d]: ignoring netdev event=%ld for %s\n", __func__, __LINE__, event, netdev->name);
		break;
	}

	return NOTIFY_DONE;
}

static int zxdh_netdevice_vlan_event(struct zxdh_device *iwdev,
				unsigned long event, struct net_device *netdev)
{
	enum ib_port_state state;

	switch (event) {
	case NETDEV_UP:
	case NETDEV_DOWN:
		if (!refcount_read(&iwdev->trace_switch.t_switch))
			break;
		state = get_port_state(netdev);
		if (state == IB_PORT_ACTIVE)
			ibdev_notice(&iwdev->ibdev, "IB port up\n");
		else
			ibdev_notice(&iwdev->ibdev, "IB port down\n");
		break;
	case NETDEV_CHANGEMTU:
		pr_info("[zxdh_rdma] %s changed mtu to %d\n", netdev->name, netdev->mtu);
		break;
	case NETDEV_CHANGEADDR:
		pr_info("[zxdh_rdma] %s[%d]: process NETDEV_CHANGEADDR event=%ld, for %s update dpp mac tbl\n", __func__, __LINE__, event, netdev->name);
		zxdh_update_dpp_mac_tbl(iwdev, netdev);
		break;
	case NETDEV_REGISTER:
		pr_info("[zxdh_rdma] %s[%d]: process NETDEV_REGISTER event=%ld, for %s update dpp mac tbl\n", __func__, __LINE__, event, netdev->name);
		zxdh_add_dpp_mac_tbl(iwdev, netdev);
		break;
	case NETDEV_UNREGISTER:
		pr_info("[zxdh_rdma] %s[%d]: process NETDEV_UNREGISTER event=%ld, for %s update dpp mac tbl\n", __func__, __LINE__, event, netdev->name);
		zxdh_del_dpp_mac_tbl(iwdev, netdev, false);
		break;
	default:
		pr_info("[zxdh_rdma] %s[%d]: ignoring netdev event=%ld for %s\n", __func__, __LINE__, event, netdev->name);
		break;
	}

	return NOTIFY_DONE;
}

static int zxdh_netdevice_macvlan_event(struct zxdh_device *iwdev,
				unsigned long event, struct net_device *netdev)
{
	enum ib_port_state state;

	switch (event) {
	case NETDEV_UP:
		pr_info("[zxdh_rdma] %s[%d]: process NETDEV_UP event=%ld, for %s update dpp mac tbl\n", __func__, __LINE__, event, netdev->name);
		zxdh_add_dpp_mac_tbl(iwdev, netdev);
		__attribute__((__fallthrough__));

	case NETDEV_CHANGE:
	case NETDEV_DOWN:
		if (!refcount_read(&iwdev->trace_switch.t_switch))
			break;
		state = get_port_state(netdev);
		if (state == IB_PORT_ACTIVE)
			ibdev_notice(&iwdev->ibdev, "IB port up\n");
		else
			ibdev_notice(&iwdev->ibdev, "IB port down\n");
		break;

	case NETDEV_CHANGEMTU:
		pr_info("[zxdh_rdma] %s changed mtu to %d\n", netdev->name, netdev->mtu);
		break;
	case NETDEV_CHANGEADDR:
		pr_info("[zxdh_rdma] %s[%d]: process NETDEV_CHANGEADDR event=%ld, for %s update dpp mac tbl\n", __func__, __LINE__, event, netdev->name);
		zxdh_update_dpp_mac_tbl(iwdev, netdev);
		break;
	case NETDEV_UNREGISTER:
		pr_info("[zxdh_rdma] %s[%d]: process NETDEV_UNREGISTER event=%ld, for %s update dpp mac tbl\n", __func__, __LINE__, event, netdev->name);
		zxdh_del_dpp_mac_tbl(iwdev, netdev, false);
		break;
	default:
		pr_info("[zxdh_rdma] %s[%d]: ignoring netdev event=%ld for %s\n", __func__, __LINE__, event, netdev->name);
		break;
	}

	return NOTIFY_DONE;
}

static struct zxdh_device *get_iwdev_from_netdev(struct net_device *netdev)
{
	struct net_device *real_dev = NULL;
	struct ib_device *ibdev;
	struct zxdh_device *iwdev = NULL;

	if (!netdev)
		return NULL;

	if (is_vlan_dev(netdev))
		real_dev = vlan_dev_real_dev(netdev);
	else if (netif_is_macvlan(netdev))
		real_dev = macvlan_dev_real_dev(netdev);
	else
		real_dev = netdev;

	if (real_dev) {
		ibdev = ib_device_get_by_netdev(real_dev, RDMA_DRIVER_ZXDH);
		if (!ibdev)
			return NULL;

		iwdev = to_iwdev(ibdev);
		ib_device_put(ibdev);
	}
	return iwdev;
}


static int zxdh_netdevice_event(struct notifier_block *not_blk,
				unsigned long event, void *arg)
{
	struct zxdh_device *iwdev;
	struct net_device *netdev = netdev_notifier_info_to_dev(arg);

	iwdev = get_iwdev_from_netdev(netdev);
	if (!iwdev)
		return NOTIFY_DONE;

	if (is_vlan_dev(netdev))
		zxdh_netdevice_vlan_event(iwdev, event, netdev);
	else if (netif_is_macvlan(netdev))
		zxdh_netdevice_macvlan_event(iwdev, event, netdev);
	else {
		zxdh_netdevice_master_event(iwdev, event, netdev);
	}

	return NOTIFY_DONE;
}

static struct notifier_block zxdh_netdevice_notifier = {
	.notifier_call = zxdh_netdevice_event
};

static void zxdh_register_notifiers(void)
{
	register_netdevice_notifier(&zxdh_netdevice_notifier);
}

static void zxdh_unregister_notifiers(void)
{
	unregister_netdevice_notifier(&zxdh_netdevice_notifier);
}

extern struct zxdh_rdma_hb_if hwbond_ops;
/**
 * set_protocol_used - set protocol_used against HW generation and roce_ena flag
 * @rf: RDMA PCI function
 * @roce_ena: RoCE enabled bit flag
 */
static inline void set_protocol_used(struct zxdh_pci_f *rf, uint roce_ena)
{
	switch (rf->rdma_ver) {
	case ZXDH_GEN_2:
		rf->protocol_used =
			roce_ena & BIT(PCI_FUNC(rf->pcidev->devfn)) ?
				ZXDH_ROCE_PROTOCOL_ONLY :
				      ZXDH_IWARP_PROTOCOL_ONLY;

		break;
	case ZXDH_GEN_1:
		rf->protocol_used = ZXDH_IWARP_PROTOCOL_ONLY;
		break;
	}
}

/**
 * zxdh_set_rf_user_cfg_params - Setup RF configurations from module parameters
 * @rf: RDMA PCI function
 */
void zxdh_set_rf_user_cfg_params(struct zxdh_pci_f *rf)
{
	/*TODO: Fixup range checks on all integer module params */
	if (limits_sel > 7)
		limits_sel = 7;

	if (gen1_limits_sel > 5)
		gen1_limits_sel = 5;

	rf->limits_sel = (rf->rdma_ver == ZXDH_GEN_1) ? gen1_limits_sel :
							      limits_sel;
	if (roce_ena)
		pr_warn_once(
			"zrdma: Because roce_ena is ENABLED, roce_port_cfg will be ignored.");
	set_protocol_used(rf, roce_ena ? 0xFFFFFFFF : roce_port_cfg);
	rf->rsrc_profile =
		(resource_profile < ZXDH_HMC_PROFILE_EQUAL) ?
			(u8)resource_profile + ZXDH_HMC_PROFILE_DEFAULT :
			      ZXDH_HMC_PROFILE_DEFAULT;
	if (max_rdma_vfs > ZXDH_MAX_PE_ENA_VF_COUNT) {
		pr_warn_once(
			"zrdma: Requested VF count [%d] is above max supported. Setting to %d.",
			max_rdma_vfs, ZXDH_MAX_PE_ENA_VF_COUNT);
		max_rdma_vfs = ZXDH_MAX_PE_ENA_VF_COUNT;
	}
	//rf->max_rdma_vfs = (rf->rsrc_profile != ZXDH_HMC_PROFILE_DEFAULT)?
	//max_rdma_vfs : 0;
	rf->en_rem_endpoint_trk = en_rem_endpoint_trk;
	rf->fragcnt_limit = fragment_count_limit;
	if (rf->fragcnt_limit > 13 || rf->fragcnt_limit < 2) {
		rf->fragcnt_limit = 6;
		pr_warn_once(
			"zrdma: Requested [%d] fragment count limit out of range (2-13), setting to default=6.",
			fragment_count_limit);
	}
	rf->dcqcn_ena = dcqcn_enable;

	/* Skip over all checking if no dcqcn */
	if (!dcqcn_enable)
		return;

	rf->dcqcn_params.cc_cfg_valid = dcqcn_cc_cfg_valid;
	rf->dcqcn_params.dcqcn_b = dcqcn_B;

#define DCQCN_B_MAX GENMASK(25, 0)
	if (rf->dcqcn_params.dcqcn_b > DCQCN_B_MAX) {
		rf->dcqcn_params.dcqcn_b = DCQCN_B_MAX;
		pr_warn_once(
			"zrdma: Requested [%d] dcqcn_b value too high, setting to %d.",
			dcqcn_B, rf->dcqcn_params.dcqcn_b);
	}

#define DCQCN_F_MAX 8
	rf->dcqcn_params.dcqcn_f = dcqcn_F;
	if (dcqcn_F > DCQCN_F_MAX) {
		rf->dcqcn_params.dcqcn_f = DCQCN_F_MAX;
		pr_warn_once(
			"zrdma: Requested [%d] dcqcn_f value too high, setting to %d.",
			dcqcn_F, DCQCN_F_MAX);
	}

	rf->dcqcn_params.dcqcn_t = dcqcn_T;
	rf->dcqcn_params.hai_factor = dcqcn_hai_factor;
	rf->dcqcn_params.min_dec_factor = dcqcn_min_dec_factor;
	if (dcqcn_min_dec_factor < 1 || dcqcn_min_dec_factor > 100) {
		rf->dcqcn_params.dcqcn_b = 1;
		pr_warn_once(
			"zrdma: Requested [%d] dcqcn_min_dec_factor out of range (1-100) , setting to default=1",
			dcqcn_min_dec_factor);
	}

	rf->dcqcn_params.min_rate = dcqcn_min_rate_MBps;
	rf->dcqcn_params.rai_factor = dcqcn_rai_factor;
	rf->dcqcn_params.rreduce_mperiod = dcqcn_rreduce_mperiod;
}

static void zxdh_iidc_event_handler(struct iidc_core_dev_info *cdev_info,
				    struct iidc_event *event)
{
}

/**
 * zxdh_request_reset - Request a reset
 * @rf: RDMA PCI function
 */
static void zxdh_request_reset(struct zxdh_pci_f *rf)
{
	struct iidc_core_dev_info *cdev_info = rf->cdev;
	if(rf->sc_dev.hw_attrs.skip_hw == false)
		dev_warn(idev_to_dev(&rf->sc_dev), "Requesting a reset\n");
	rf->sc_dev.vchnl_up = false;
	cdev_info->ops->request_reset(rf->cdev, IIDC_PFR);
}

/**
 * zxdh_dev_ibevent - indicate dev event
 * @iwdev: zrdma device
 */
static void zxdh_dev_ibevent(struct zxdh_device *iwdev)
{
	struct ib_event event;

	event.device = &iwdev->ibdev;
	event.element.port_num = 1;
	event.event = IB_EVENT_DEVICE_FATAL;
	ib_dispatch_event(&event);
}

int rdma_get_rp_link_status(struct pci_dev *pdev)
{
    struct pci_dev *rp_dev = NULL;
    int pcie_cap = 0;
    u16 data = 0;
    rp_dev = pcie_find_root_port(pdev);
    if (!rp_dev)
    {
        // pr_err("[zxdh_rdma] rdma can not find RP\n");
        return -ENODEV;
    }
    pcie_cap = pci_find_capability(rp_dev, PCI_CAP_ID_EXP);
    if (!pcie_cap)
    {
        pr_err("[zxdh_rdma] rdma can not find PCI Express CAP\n");
        return -ENXIO;
    }
    pci_read_config_word(rp_dev, pcie_cap + PCI_EXP_LNKSTA, &data);
    return (data & PCI_EXP_LNKSTA_DLLLA) ? ZXDH_PCIE_LINK_UP : ZXDH_PCIE_LINK_DOWN;
}

static int rdma_get_upstream_port_link_status(struct pci_dev *pdev)
{
    struct pci_dev *up_stream_dev = NULL;
    int pcie_cap = 0;
    u16 data = 0;

    up_stream_dev = pci_upstream_bridge(pdev);
    if (!up_stream_dev)
    {
        pr_err("[zxdh_rdma] rdma can not find RP\n");
        return -ENODEV;
    }
    pcie_cap = pci_find_capability(up_stream_dev, PCI_CAP_ID_EXP);
    if (!pcie_cap)
    {
        pr_err("[zxdh_rdma] rdma can not find PCI Express CAP\n");
        return -ENXIO;
    }
    pci_read_config_word(up_stream_dev, pcie_cap + PCI_EXP_LNKSTA, &data);
    return (data & PCI_EXP_LNKSTA_DLLLA) ? ZXDH_PCIE_LINK_UP : ZXDH_PCIE_LINK_DOWN;
}

int zxdh_detect_pcie_link(struct zxdh_pci_f *rf, int* cnt, bool* flag)
{
    u32 val = 0;
    int valid = 0;

    // 连续5次读到异常值(0xFFFFFFFF), 认为PCIe link异常, 设置 err_flag 为 true
    val = readl(rf->sc_dev.hw->hw_addr + C_RDMAIO_TABLE3(rf->sc_dev.ep_id));
    if (val == ZXDH_BAR_ERR_DEFAULT) {
        (*cnt)++;
        valid = -1; // 如果valid为-1, err_flag为false, 这种情况不能表示PCIe link正常
        if (*cnt >= ZXDH_MAX_DETECT_CNT) {
            *flag = true;
            valid = 0;
        }
    } else {
        *cnt = 0;
        *flag = false;
    }

    return valid;
}

static int zxdh_rdma_check_remove_state(struct pci_dev *pdev)
{
    if (!rdma_get_rp_link_status(pdev))
        return ZXDH_PCIE_LINK_DOWN;

    return rdma_get_upstream_port_link_status(pdev);
}

static int zxdh_wake_pending_cqp_requests(struct zxdh_pci_f *rf,
					   const char *caller_func)
{
	struct zxdh_cqp_request *req, *tmp;
	unsigned long flags;
	int woken = 0;

	if (!rf)
		return 0;

	spin_lock_irqsave(&rf->cqp.req_lock, flags);
	list_for_each_entry_safe(req, tmp, &rf->cqp.cqp_pending_reqs, list) {
		if (req->waiting) {
			req->compl_info.error = true;
			req->compl_info.maj_err_code = 0xFFFF;
			req->compl_info.min_err_code = 0xFFFF;
			req->request_done = true;
			wake_up(&req->waitq);
			woken++;
		}
	}
	spin_unlock_irqrestore(&rf->cqp.req_lock, flags);

	if (woken)
		pr_info("[zxdh_rdma][%s] vhca_id:%d woke %d pending cqp requests.\n",
		       caller_func, rf->sc_dev.vhca_id, woken);

	return woken;
}

int zxdh_rdma_skip_hw_cfg(struct zxdh_pci_f *rf)
{
    struct zxdh_ceq *iwceq;
    u32 i;
    struct zxdh_msix_info *msix_info;

    if (!rf) {
        return -ENODEV;
    }

    rf->sc_dev.hw_attrs.cqp_timeout_threshold = CQP_MIN_TIMEOUT_THRESHOLD;
    rf->sc_dev.hw_attrs.max_done_count = ZXDH_MIN_DONE_COUNT;
    rf->sc_dev.hw_attrs.skip_hw = true;

    zxdh_wake_pending_cqp_requests(rf, __func__);

    if (rf->aeq.irq_sta == true) {
        rf->aeq.irq_sta = false;
        msix_info = &rf->aeq.msix_info;
        if (rf->net_irq_cap == false) {
            irq_set_affinity_hint(rf->aeq.irq, NULL);
            free_irq(rf->aeq.irq, rf);
        } else {
            rf->gen_ops.zxdh_common_func(msix_info, NULL, ZXDH_FUNC_IRQ_FREE);	
		}
            free_cpumask_var(msix_info->mask);
    }

    for (i = 0; i< rf->ceqs_count; i++) {
        iwceq = &rf->ceqlist[i];
        if (iwceq->irq_sta == true) {
            iwceq->irq_sta = false;
            msix_info = &iwceq->msix_info;
            if (rf->net_irq_cap == false) {
                irq_set_affinity_hint(iwceq->irq, NULL);
                free_irq(iwceq->irq, iwceq);
            } else {
                rf->gen_ops.zxdh_common_func(msix_info, NULL, ZXDH_FUNC_IRQ_FREE);
            }
            if (cpumask_available(msix_info->mask))
            free_cpumask_var(msix_info->mask);
        }
    }
    zxdh_dev_ibevent(rf->iwdev);
    pr_info("[zxdh_rdma][%s][%d] vhca_id:%d rdma_skip_hw become true\n",
	    __func__, __LINE__, rf->sc_dev.vhca_id);
    return 0;
}

static int zxdh_rdma_hotplug_event(struct zxdh_pci_f *rf)
{
	struct zxdh_ceq *iwceq;
	u32 i;
	struct zxdh_msix_info *msix_info;

	if (!rf) {
		return -ENODEV;
	}

	rf->sc_dev.hw_attrs.cqp_timeout_threshold = CQP_MIN_TIMEOUT_THRESHOLD;
	rf->sc_dev.hw_attrs.max_done_count = ZXDH_MIN_DONE_COUNT;
	rf->sc_dev.hw_attrs.skip_hw = true;

	zxdh_wake_pending_cqp_requests(rf, __func__);

	if (rf->aeq.irq_sta == true) {
		rf->aeq.irq_sta = false;
		msix_info = &rf->aeq.msix_info;
		if (rf->net_irq_cap == false) {
		    irq_set_affinity_hint(rf->aeq.irq, NULL);
		    free_irq(rf->aeq.irq, rf);
		} else {
		    rf->gen_ops.zxdh_common_func(msix_info, NULL, ZXDH_FUNC_IRQ_FREE);	
		}
		if (cpumask_available(msix_info->mask))
		    free_cpumask_var(msix_info->mask);
	}

	for (i = 0; i< rf->ceqs_count; i++) {
        iwceq = &rf->ceqlist[i];
        if (iwceq->irq_sta == true) {
            iwceq->irq_sta = false;
            msix_info = &iwceq->msix_info;
            if (rf->net_irq_cap == false) {
                irq_set_affinity_hint(iwceq->irq, NULL);
                free_irq(iwceq->irq, iwceq);
            } else {
                rf->gen_ops.zxdh_common_func(msix_info, NULL, ZXDH_FUNC_IRQ_FREE);
            }
            if (cpumask_available(msix_info->mask))
                free_cpumask_var(msix_info->mask);
        }
	}
	pr_info("[zxdh_rdma][%s][%d] vhca_id:%d rdma_skip_hw become true\n",
		__func__, __LINE__, rf->sc_dev.vhca_id);
	return 0;
}
static int process_rdma_health_event(struct net_device *netdev)
{
	struct zxdh_device *iwdev;
	struct zxdh_pci_f *rf;
	struct zxdh_ceq *iwceq;
	u32 i;
	struct zxdh_msix_info *msix_info;

	iwdev = zxdh_device_get_by_source_netdev(netdev);
	if (!iwdev) {
		return -ENODEV;
	}
	rf = iwdev->rf;
	if (!rf) {
		return -ENODEV;
	}

	rf->sc_dev.hw_attrs.cqp_timeout_threshold = CQP_MIN_TIMEOUT_THRESHOLD;
	rf->sc_dev.hw_attrs.max_done_count = ZXDH_MIN_DONE_COUNT;
	rf->sc_dev.hw_attrs.skip_hw = true;

	zxdh_wake_pending_cqp_requests(rf, __func__);

	if (rf->sc_dev.driver_load == false) {
		pr_err("[zxdh_rdma][%s][%d] the driver has been uninstalled\n",__func__, __LINE__);
		return 0;
	}
	if (rf->aeq.irq_sta == true) {
		rf->aeq.irq_sta = false;
		msix_info = &rf->aeq.msix_info;
		if (rf->net_irq_cap == false) {
		    irq_set_affinity_hint(rf->aeq.irq, NULL);
		    free_irq(rf->aeq.irq, rf);
		} else {
		    rf->gen_ops.zxdh_common_func(msix_info, NULL, ZXDH_FUNC_IRQ_FREE);	
		}
		if (cpumask_available(msix_info->mask))
		    free_cpumask_var(msix_info->mask);
	}

	for (i = 0; i< rf->ceqs_count; i++) {
        iwceq = &rf->ceqlist[i];
        if (iwceq->irq_sta == true) {
            iwceq->irq_sta = false;
            msix_info = &iwceq->msix_info;
            if (rf->net_irq_cap == false) {
                irq_set_affinity_hint(iwceq->irq, NULL);
                free_irq(iwceq->irq, iwceq);
            } else {
                rf->gen_ops.zxdh_common_func(msix_info, NULL, ZXDH_FUNC_IRQ_FREE);
            }
            if (cpumask_available(msix_info->mask))
                free_cpumask_var(msix_info->mask);
        }
	}
	zxdh_dev_ibevent(iwdev);
	zxdh_add_rdma_name(pci_name(rf->pcidev), iwdev->ibdev.name);
	zxdh_handle_internal_error(rf);
	pr_info("[zxdh_rdma][%s][%d] vhca_id:%d rdma_skip_hw become true\n", __func__, __LINE__, rf->sc_dev.vhca_id);
	return 0;
}

static struct zxdh_pci_f *zxdh_get_rf_from_pdev(struct pci_dev *pdev)
{
	struct zxdh_pci_f *rf = NULL;
	int i;

	for (i = 0; i < HOST_RDMA_MAX_PF; i++)
	{
		if (pf_sriov_glb_info[i].rdma_pf_enable && pf_sriov_glb_info[i].pdev == pdev)
		{
			rf = pf_sriov_glb_info[i].rf;
			break;
		}
		else
		{
			continue;
		}
	}

	return rf;
}

static void update_vf_pblem_info(struct zxdh_sc_dev *dev, u16 num_vfs, u64 vf_pblem_cnt)
{
	struct zxdh_vfdev *vf_dev = NULL;
	struct zxdh_hmc_obj_info *hmc_obj;
	u16 vf_id;

	dev->hmc_pf_manager_info.vf_pblemr_cnt = vf_pblem_cnt;
	pr_info("[zxdh_rdma] %s %d Update vf pblem cnt to 0x%llx\n", __func__, __LINE__, vf_pblem_cnt);

	for (vf_id = 0; vf_id < num_vfs; vf_id++) {
		vf_dev = zxdh_find_vf_dev(dev, vf_id);
		if (vf_dev) 
		{
			hmc_obj = vf_dev->hmc_info.hmc_obj;
			hmc_obj[ZXDH_HMC_IW_PBLE_MR].max_cnt =
				dev->hmc_pf_manager_info.vf_pblemr_cnt;
			hmc_obj[ZXDH_HMC_IW_PBLE_MR].cnt = dev->hmc_pf_manager_info.vf_pblemr_cnt;
			hmc_obj[ZXDH_HMC_IW_PBLE_MR].size = dev->hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE_MR].size;
			hmc_obj[ZXDH_HMC_IW_PBLE_MR].type = dev->hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE_MR].type;
			hmc_obj[ZXDH_HMC_IW_PBLE_MR].base = dev->hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE_MR].base +
									(dev->hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE_MR].cnt +
									hmc_obj[ZXDH_HMC_IW_PBLE_MR].cnt * vf_id) *
										hmc_obj[ZXDH_HMC_IW_PBLE_MR].size;
			zxdh_put_vfdev(dev, vf_dev);
		}
		else
		{
			continue;
		}
	}

	return;
}

static int process_rdma_sriov_event(void *data)
{
	struct zxdh_rdma_sriov_event_info *sriov_info;
	struct zxdh_pci_f *rf;
	u64 vf_pblem_cnt;
	int ret = 0;

	if (NULL == data)
		return -EINVAL;

	sriov_info = (struct zxdh_rdma_sriov_event_info *)data;
	rf = zxdh_get_rf_from_pdev(sriov_info->pdev);
	if (rf && rf->sc_dev.active_vfs_num == sriov_info->num_vfs)
	{
		return 0;
	}

	ret = set_rdma_vf_num(sriov_info, &vf_pblem_cnt);
	if (ret) {
		pr_err("[zxdh_rdma] %s set_rdma_vf_num failed, ret=%d\n", __func__, ret);
		return ret;
	}

	if (rf)
	{
		rf->sc_dev.active_vfs_num = sriov_info->num_vfs;
		update_vf_pblem_info(&rf->sc_dev, sriov_info->num_vfs, vf_pblem_cnt);
	}

	return 0;
}

static int zxdh_req_config_rdma_ets(struct zxdh_device *iwdev, struct zxdh_rdma_ets_set_cmd *cmd_info)
{
	int ret = 0;
	struct zxdh_pci_f *rf = iwdev->rf;
	uint8_t rep_valid = 0;
	uint16_t rep_len = 0;
	uint8_t *rep_ptr;
	struct zxdh_mgr mgr = { 0 };
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct iidc_core_dev_info *cdev_info;
	struct zxdh_rdma_ets_set_cmd *req_msg = cmd_info;
	struct dh_rdma_ets_set_resp *resp_msg;
	size_t recv_len;
	void *recv_buffer;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	if (!rf)
		return -ENOMEM;
	if (rf->sc_dev.driver_load == false)
		cnt_num = ZXDH_BAR_MSG_DEFAULT_NUM;

	cdev_info = (struct iidc_core_dev_info *)rf->cdev;

	// query pcie id
	mgr.pdev = cdev_info->pdev;
	ret = dh_rdma_pf_pcie_id_get(&mgr);
	if (ret) {
		pr_err("[zxdh_rdma] [%s] get pf pcie_id failed, ret=%d\n", __func__, ret);
		return -EINVAL;
	}

	recv_len = ZXDH_CHAN_REPS_LEN + sizeof(struct dh_rdma_ets_set_resp);
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer)
		return -ENOMEM;

	in.payload_addr = (void *)req_msg;
	in.payload_len = sizeof(struct zxdh_rdma_ets_set_cmd);
	in.src = rf->ftype == 0 ? MSG_CHAN_END_PF : MSG_CHAN_END_VF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = (u64)cdev_info->hw_addr + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	}while(cnt < cnt_num);

	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d cnt=%d\n", __func__, ret, cnt);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n",
		       __func__, rep_valid);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != recv_len - ZXDH_CHAN_REPS_LEN) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x\n", __func__,
		       rep_len);
		kfree(recv_buffer);
		return -ERANGE;
	}

	resp_msg = (struct dh_rdma_ets_set_resp *)(rep_ptr + ZXDH_CHAN_REPS_LEN);
	ret = resp_msg->status_code;
	if (ret != STATUS_OK) {
		pr_err("[zxdh_rdma] [%s] set rdma ets failed, err code=%u\n", __func__, resp_msg->status_code);
	}

	kfree(recv_buffer);

	return ret;
}

/**
 * zxdh_ets_token_mgr_init - 初始化ETS动态令牌管理器
 * @rf: zxdh_pci_f结构体指针
 */
void zxdh_ets_token_mgr_init(struct zxdh_pci_f *rf)
{
	struct zxdh_ets_token_mgr *mgr = &rf->ets_token_mgr;
	int i;

	if (mgr->init_done) {
		pr_warn("[zxdh_rdma] ETS token manager already initialized, skipping\n");
		return;
	}

	mutex_init(&mgr->lock);
	mgr->enabled = false;
	mgr->config_type = ZXDH_ETS_CONFIG_NONE;
	mgr->ets_tc_bitmap = 0;
	mgr->active_tc_bitmap = 0;

	for (i = 0; i < ZXDH_ETS_MAX_TC_NUM; i++) {
		mgr->tc_weight[i] = 0;
		mgr->tc_tsa[i] = 0;
		atomic_set(&mgr->tc_qp_cnt[i], 0);
	}

	mgr->init_done = true;
}

/**
 * zxdh_ets_token_mgr_deinit - 反初始化ETS动态令牌管理器
 * @rf: zxdh_pci_f结构体指针
 */
void zxdh_ets_token_mgr_deinit(struct zxdh_pci_f *rf)
{
	struct zxdh_ets_token_mgr *mgr = &rf->ets_token_mgr;
	int i;

	if (!mgr->init_done) {
		pr_warn("[zxdh_rdma] ETS token manager not initialized, skipping deinit\n");
		return;
	}

	mutex_lock(&mgr->lock);
	mgr->enabled = false;
	mgr->config_type = ZXDH_ETS_CONFIG_NONE;
	mgr->ets_tc_bitmap = 0;
	mgr->active_tc_bitmap = 0;

	/* 清理所有TC的QP计数，防止重新启用时计数错误 */
	for (i = 0; i < ZXDH_ETS_MAX_TC_NUM; i++) {
		mgr->tc_weight[i] = 0;
		mgr->tc_tsa[i] = 0;
		atomic_set(&mgr->tc_qp_cnt[i], 0);
	}
	mutex_unlock(&mgr->lock);

	mgr->init_done = false;
}

/**
 * zxdh_ets_save_config - 保存ETS配置信息并启用动态令牌功能
 * @rf: zxdh_pci_f结构体指针
 * @rdma_ets: ETS配置数据
 *
 * tc_tsa: TSA类型，0=strict(严格优先级)，非0=ETS(增强传输选择)
 * tc_tx_bw: TC带宽百分比，8个TC总和为100
 */
static void zxdh_ets_save_config(struct zxdh_pci_f *rf,
				 struct zxdh_dcbnl_ieee_rdma_ets *rdma_ets)
{
	struct zxdh_ets_token_mgr *mgr = &rf->ets_token_mgr;
	int i;

	mutex_lock(&mgr->lock);

	mgr->ets_tc_bitmap = 0;
	for (i = 0; i < ZXDH_ETS_MAX_TC_NUM; i++) {
		mgr->tc_weight[i] = 0;
		mgr->tc_tsa[i] = 0;
	}

	for (i = 0; i < ZXDH_ETS_MAX_TC_NUM; i++) {
		mgr->tc_tsa[i] = rdma_ets->tc_tsa[i];
		mgr->tc_weight[i] = rdma_ets->tc_tx_bw[i];

		/* 只有TSA为ETS模式(非0)且配置了带宽百分比(tc_tx_bw>0)的TC才参与动态令牌限速 */
		if (rdma_ets->tc_tsa[i] != 0 && rdma_ets->tc_tx_bw[i] > 0) {
			mgr->ets_tc_bitmap |= BIT(i);
		}
	}

	/* 设置配置类型为权重配置 */
	mgr->config_type = ZXDH_ETS_CONFIG_WEIGHT;

	/* 检查是否有配置了ETS的TC，若有则启用动态令牌功能 */
	if (mgr->ets_tc_bitmap != 0) {
		mgr->enabled = true;
		pr_debug("[zxdh_rdma] ETS dynamic token enabled, ets_tc_bitmap=0x%x\n",
			mgr->ets_tc_bitmap);
	} else {
		mgr->enabled = false;
		pr_debug("[zxdh_rdma] No TC with ETS configured, dynamic token not enabled\n");
	}

	mutex_unlock(&mgr->lock);
}

/**
 * zxdh_ets_set_other_config_type - 设置其他ETS配置类型
 * @rf: zxdh_pci_f结构体指针
 * @type: 配置类型
 *
 * 当用户配置MAXRATE或ETS_SWITCH_OFF时，禁用动态令牌功能
 */
static void zxdh_ets_set_other_config_type(struct zxdh_pci_f *rf,
					   enum zxdh_ets_config_type type)
{
	struct zxdh_ets_token_mgr *mgr = &rf->ets_token_mgr;
	int i;

	mutex_lock(&mgr->lock);

	mgr->enabled = false;
	mgr->config_type = type;

	/* 清理所有TC的QP计数，防止重新启用时计数错误 */
	for (i = 0; i < ZXDH_ETS_MAX_TC_NUM; i++) {
		atomic_set(&mgr->tc_qp_cnt[i], 0);
	}

	mutex_unlock(&mgr->lock);
}

/**
 * zxdh_ets_get_active_tc_bitmap - 获取当前活跃TC的位图
 * @mgr: ETS令牌管理器指针
 *
 * 活跃TC定义：有QP（tc_qp_cnt > 0）的TC
 * 注意：只有活跃且有ETS配置的TC才会参与限速比例计算
 *
 * 返回: 活跃TC位图
 */
static u32 zxdh_ets_get_active_tc_bitmap(struct zxdh_ets_token_mgr *mgr)
{
	u32 bitmap = 0;
	int i;

	for (i = 0; i < ZXDH_ETS_MAX_TC_NUM; i++) {
		if (atomic_read(&mgr->tc_qp_cnt[i]) > 0) {
			bitmap |= BIT(i);
		}
	}

	return bitmap;
}

/**
 * zxdh_ets_recalculate_tokens - 重新计算并下发令牌配置
 * @rf: zxdh_pci_f结构体指针
 *
 * 根据活跃TC的权重比例重新分配带宽令牌比例
 * 策略：保留权重最大的TC不限速，其他TC按权重比例分配
 */
static int zxdh_ets_recalculate_tokens(struct zxdh_pci_f *rf)
{
	struct zxdh_ets_token_mgr *mgr = &rf->ets_token_mgr;
	struct zxdh_device *iwdev = rf->iwdev;
	struct zxdh_rdma_ets_set_cmd cmd_info;
	u32 active_bitmap;
	u32 weight_sum = 0;
	u64 tc_ratio[ZXDH_ETS_MAX_TC_NUM] = { 0 };
	u32 max_weight = 0;
	u8 max_weight_tc = 0;
	int i, active_count = 0;
	int status_result;

	zrdma_memset_s(&cmd_info, 0, sizeof(cmd_info));
	cmd_info.fw_opcode = RDMA_ETS_CFG;
	cmd_info.ep_id = rf->ep_id;
	cmd_info.pf_id = rf->pf_id;
	cmd_info.ets_cmd_mode = ZXDH_RDMA_TURN_TO_RR_ENABLE_RL;

	mutex_lock(&mgr->lock);

	active_bitmap = zxdh_ets_get_active_tc_bitmap(mgr);
	mgr->active_tc_bitmap = active_bitmap;

	if (active_bitmap == 0) {
		pr_debug("[zxdh_rdma] No active TC, skip token update\n");
		mutex_unlock(&mgr->lock);
		return 0;
	}

	active_bitmap &= mgr->ets_tc_bitmap;
	if (active_bitmap == 0) {
		pr_debug("[zxdh_rdma] No ETS-configured active TC, skip token update\n");
		mutex_unlock(&mgr->lock);
		return 0;
	}

	/* 计算活跃TC的权重总和，并找到权重最大的TC */
	for (i = 0; i < ZXDH_ETS_MAX_TC_NUM; i++) {
		if (active_bitmap & BIT(i)) {
			weight_sum += mgr->tc_weight[i];
			active_count++;
			if (mgr->tc_weight[i] > max_weight) {
				max_weight = mgr->tc_weight[i];
				max_weight_tc = i;
			}
		}
	}

	if (weight_sum == 0) {
		pr_debug("[zxdh_rdma] ETS weight sum is 0, cannot allocate tokens\n");
		mutex_unlock(&mgr->lock);
		return 0;
	}

	pr_debug("[zxdh_rdma] ETS: %d active TCs, bitmap=0x%x, total_weight=%u\n",
		active_count, active_bitmap, weight_sum);

	if (active_count > 1) {
		/* 多条流：保留权重最大的TC不限速（ratio=0），其他有流的TC按权重比例限速 */
		for (i = 0; i < ZXDH_ETS_MAX_TC_NUM; i++) {
			if ((active_bitmap & BIT(i)) && (i != max_weight_tc)) {
				tc_ratio[i] = mgr->tc_weight[i] * 100 / weight_sum;
			}
		}
	}

	for (i = 0; i < ZXDH_ETS_MAX_TC_NUM; i++) {
		cmd_info.tc_rate_limit.tc_ets[i] = tc_ratio[i];
		pr_debug("[zxdh_rdma] ETS: TC%d ratio=%llu%% (weight=%u)\n", i, cmd_info.tc_rate_limit.tc_ets[i], mgr->tc_weight[i]);
	}

	mutex_unlock(&mgr->lock);

	status_result = zxdh_req_config_rdma_ets(iwdev, &cmd_info);

	if (STATUS_OK == status_result) {
		return 0;
	} else {
		pr_err("[zxdh_rdma] ETS token update failed, ret=%d\n", status_result);
		return status_result;
	}
}

/**
 * zxdh_ets_qp_update - 更新TC上的QP计数
 * @rf: zxdh_pci_f结构体指针
 * @pri_index: 优先级索引 (0-7)
 * @is_add: true=添加QP, false=移除QP
 *
 * 返回: 0成功，其他值失败
 */
int zxdh_ets_qp_update(struct zxdh_pci_f *rf, u8 pri_index, bool is_add)
{
	struct zxdh_ets_token_mgr *mgr;
	int old_cnt, new_cnt;

	if (!rf || !rf->iwdev)
		return -EINVAL;

	mgr = &rf->ets_token_mgr;

	if (pri_index >= ZXDH_ETS_MAX_TC_NUM) {
		pr_err("[zxdh_rdma] ETS: invalid pri_index=%u, must be < %d\n",
			pri_index, ZXDH_ETS_MAX_TC_NUM);
		return -EINVAL;
	}

	if (!(mgr->ets_tc_bitmap & BIT(pri_index))) {
		pr_debug("[zxdh_rdma] TC%d not in ETS mode, skip\n", pri_index);
		return 0;
	}

	old_cnt = atomic_read(&mgr->tc_qp_cnt[pri_index]);

	if (is_add) {
		new_cnt = old_cnt + 1;
		atomic_set(&mgr->tc_qp_cnt[pri_index], new_cnt);

		/* 检测状态变化：0 → 1，TC首次有流量 */
		if (old_cnt == 0) {
			pr_debug("[zxdh_rdma] ETS: TC%d becomes ACTIVE\n", pri_index);
			return zxdh_ets_recalculate_tokens(rf);
		}
	} else {
		new_cnt = old_cnt - 1;
		if (new_cnt < 0) {
			pr_debug("[zxdh_rdma] ETS: TC%d tc_qp_cnt already 0, clamped\n", pri_index);
			new_cnt = 0;
		}
		atomic_set(&mgr->tc_qp_cnt[pri_index], new_cnt);

		/* 检测状态变化：1 → 0，TC变空闲 */
		if (old_cnt == 1 && new_cnt == 0) {
			pr_debug("[zxdh_rdma] ETS: TC%d becomes IDLE\n", pri_index);
			return zxdh_ets_recalculate_tokens(rf);
		}
	}

	return 0;
}

static bool check_all_strict(uint8_t *tc_tsa)
{
	bool all_strict = true;
	int i;
	const char *str[8] = { NULL };
	const char *str_srtict = "strict";
	const char *str_ets = "ets";

	for (i = 0; i < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; i++) {
		str[i] = str_srtict;
		/* 0 -- strict */
		if (0 != tc_tsa[i]) {
			all_strict = false;
			str[i] = str_ets;
		}
	}
	pr_debug("[zxdh_rdma] %s[%d] tc_tsa:0:%s, 1:%s, 2:%s, 3:%s, 4:%s, 5:%s, 6:%s, 7:%s\n",
		__func__, __LINE__, str[0], str[1], str[2], str[3], str[4], str[5], str[6], str[7]);
	return all_strict;
}

static int zxdh_handle_ets_event(struct net_device *netdev, void *data)
{
	struct zxdh_device *iwdev;
	struct zxdh_rdma_ets_set_cmd cmd_info = { 0 };
	int status_result = 0;
	struct zxdh_dcbnl_ieee_rdma_ets *rdma_ets = NULL;
	bool all_strict = false;

	if (NULL == data)
		return -EINVAL;

	iwdev = zxdh_device_get_by_source_netdev(netdev);
	if (!iwdev || !iwdev->rf) {
		return -ENODEV;
	}
	if (iwdev->rf->ftype) {
		// VF
		pr_err("[zxdh_rdma] %s[%d] only PF can send message to fw, but %s is VF!\n",
			__func__, __LINE__, netdev->name);
		return -EINVAL;
	}

	rdma_ets = (struct zxdh_dcbnl_ieee_rdma_ets *)data;

	cmd_info.fw_opcode = RDMA_ETS_CFG;
	cmd_info.ep_id = iwdev->rf->ep_id;
	cmd_info.pf_id = iwdev->rf->pf_id;

	if (1 == rdma_ets->mode) {
		all_strict = check_all_strict(rdma_ets->tc_tsa);
		if (true == all_strict) {
			cmd_info.ets_cmd_mode = ZXDH_RDMA_TURN_TO_SP;
			pr_info("[zxdh_rdma] %s[%d] fw priority_mode: SP!\n", __func__, __LINE__);
			/* SP模式下不启用动态令牌功能 */
			iwdev->rf->ets_token_mgr.enabled = false;
		}
		else {
			cmd_info.ets_cmd_mode = ZXDH_RDMA_TURN_TO_RR_ENABLE_RL;
			pr_info("[zxdh_rdma] %s[%d] fw priority_mode: RR and enable rate limit!\n", __func__, __LINE__);

			/* 保存ETS配置并启用动态令牌功能 */
			zxdh_ets_save_config(iwdev->rf, rdma_ets);
		}

		status_result = zxdh_req_config_rdma_ets(iwdev, &cmd_info);
		if (STATUS_OK == status_result)
			status_result = 0;
	} else {
		pr_warn("[zxdh_rdma] %s[%d] ethtool rdma-ets-switch should be \"on\" !\n", __func__, __LINE__);
		/* mode为off时禁用动态令牌功能 */
		zxdh_ets_set_other_config_type(iwdev->rf, ZXDH_ETS_CONFIG_SWITCH_OFF);
	}

	return status_result;
}

static int zxdh_rdma_set_maxrate(struct net_device *netdev, void *data)
{
	struct zxdh_device *iwdev;
	struct zxdh_rdma_ets_set_cmd cmd_info = { 0 };
	int status_result = 0;
	struct zxdh_dcbnl_ieee_rdma_maxrate *maxrate_info = NULL;
	uint32_t maxrate[8] = { 0 };
	int i;

	if (NULL == data)
		return -EINVAL;

	iwdev = zxdh_device_get_by_source_netdev(netdev);
	if (!iwdev || !iwdev->rf) {
		return -ENODEV;
	}
	if (iwdev->rf->ftype) {
		// VF
		pr_err("[zxdh_rdma] %s[%d] only PF can send message to fw, but %s is VF!\n",
			__func__, __LINE__, netdev->name);
		return -EINVAL;
	}

	maxrate_info = (struct zxdh_dcbnl_ieee_rdma_maxrate *)data;

	cmd_info.fw_opcode = RDMA_MAX_RATE_CONFIG;
	cmd_info.ep_id = iwdev->rf->ep_id;
	cmd_info.pf_id = iwdev->rf->pf_id;
	for (i = 0; i < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; i++) {
		cmd_info.tc_rate_limit.tc_maxrate[i] = maxrate_info->tc_maxrate[i];
		maxrate[i] = (uint32_t)(maxrate_info->tc_maxrate[i] / 1000 / 1000);
	}

	pr_info("[zxdh_rdma] %s[%d] maxrate(Gbit) 0-%d, 1-%d, 2-%d, 3-%d, 4-%d, 5-%d, 6-%d, 7-%d!\n", __func__, __LINE__,
		maxrate[0], maxrate[1], maxrate[2],maxrate[3], maxrate[4], maxrate[5], maxrate[6], maxrate[7]);

	/* 用户配置了TC maxrate，禁用动态令牌功能 */
	zxdh_ets_set_other_config_type(iwdev->rf, ZXDH_ETS_CONFIG_MAXRATE);

	status_result = zxdh_req_config_rdma_ets(iwdev, &cmd_info);

	if (STATUS_OK == status_result)
		status_result = 0;

	return status_result;
}

static int zxdh_rdma_ets_mode_switch_to_off(struct net_device *netdev, void *data)
{
	struct zxdh_device *iwdev;
	struct zxdh_rdma_ets_set_cmd cmd_info = { 0 };
	int status_result = 0;
	struct  zxdh_rdma_ets_status *ets_status;

	if (NULL == data)
		return -EINVAL;

	ets_status = (struct  zxdh_rdma_ets_status*)data;
	pr_info("[zxdh_rdma] %s[%d] ethtool operate %s ets_switch to %s!\n",
		__func__, __LINE__, netdev->name, 0 == ets_status->mode ? "off" : "on");

	if (0 != ets_status->mode) {
		// 代表是on，不操作，但是也不能返回错误。
		return 0;
	}

	iwdev = zxdh_device_get_by_source_netdev(netdev);
	if (!iwdev || !iwdev->rf) {
		return -ENODEV;
	}
	if (iwdev->rf->ftype) {
		// VF
		pr_err("[zxdh_rdma] %s[%d] only PF can send message to fw, but %s is VF!\n",
			__func__, __LINE__, netdev->name);
		return -EINVAL;
	}

	cmd_info.fw_opcode = RDMA_ETS_SWITCH;
	cmd_info.ep_id = iwdev->rf->ep_id;
	cmd_info.pf_id = iwdev->rf->pf_id;
	cmd_info.ets_cmd_mode = ZXDH_RDMA_TURN_TO_RR_DISABLE_RL;

	/* ETS switch关闭时，禁用动态令牌功能 */
	zxdh_ets_set_other_config_type(iwdev->rf, ZXDH_ETS_CONFIG_SWITCH_OFF);

	status_result = zxdh_req_config_rdma_ets(iwdev, &cmd_info);

	if (STATUS_OK == status_result)
		status_result = 0;

	return status_result;
}

/***
 * @brief process RDMA speed change event
 *
 * @param netdev Network device pointer
 * @param data Event data pointer (points to u32 speed value in Mbps)
 * @return Return value 0 is OK, error is another value
 */
static int process_rdma_speed_change_event(struct net_device *netdev, void *data)
{
	struct zxdh_device *iwdev;
	struct ib_device *ibdev;
	u32 bps;
	int ret;

	if (netdev == NULL) {
		pr_err("[zxdh_rdma] %s: netdev is null\n", __func__);
		return -EINVAL;
	}

	if (data == NULL) {
		pr_err("[zxdh_rdma] %s: data is null\n", __func__);
		return -EINVAL;
	}

	/* Get RDMA device instance */
	ibdev = ib_device_get_by_netdev(netdev, RDMA_DRIVER_ZXDH);
	if (!ibdev) {
		pr_err("[zxdh_rdma] %s: failed to get ib_device\n", __func__);
		return -ENODEV;
	}

	iwdev = to_iwdev(ibdev);
	if (!iwdev) {
		ib_device_put(ibdev);
		return -ENODEV;
	}

	/* Extract speed value from event data */
	bps = *(u32 *)data;

	/* Speed filtering: only configure for 100G or 200G */
	if (bps != SPEED_100000 && bps != SPEED_200000) {
		pr_debug("[zxdh_rdma] Speed %u Mbps is not 100G or 200G, skipping configuration\n", bps);
		ib_device_put(ibdev);
		return 0;
	}

	/* Skip if speed unchanged */
	if (iwdev->netdev_speed == bps) {
		pr_debug("[zxdh_rdma] Speed unchanged (%u Mbps), skipping configuration\n", bps);
		ib_device_put(ibdev);
		return 0;
	}

	pr_info("[zxdh_rdma] Speed change event: netdev=%s, old_speed=%u, new_speed=%u\n",
		netdev->name, iwdev->netdev_speed, bps);

	/* Release device reference before calling set_rdma_port_speed */
	ib_device_put(ibdev);

	/* Call port speed configuration function */
	ret = set_rdma_port_speed(netdev, bps);
	if (ret) {
		pr_err("[zxdh_rdma] %s: failed to set port speed\n", __func__);
		return ret;
	}

	return 0;
}

static int zxdh_rdma_config_trust_type(struct net_device *netdev, void *data)
{
    struct zxdh_device *iwdev;
    struct zxdh_pci_f *rf;
    u32 val, cnt, i;
    u16 vf_id[126];
    u16 active_vf_num = 0;
    u32 trust_type;
    struct zxdh_vf_id_node *pos, *tmp;
    unsigned long flags;

    iwdev = zxdh_device_get_by_source_netdev(netdev);
    if (!iwdev || !iwdev->rf) {
        return -ENODEV;
    }

    rf = iwdev->rf;

    if (iwdev->rf->ftype) {
        pr_err("[zxdh_rdma] %s[%d] only PF can get turst type from zxdh kernel, but %s is VF!\n",
            __func__, __LINE__, netdev->name);
        return -EINVAL;
    }

    if (data == NULL) {
        pr_err("[zxdh_rdma] %s[%d] data is NULL!\n", __func__, __LINE__);
        return -EINVAL;
    }
    trust_type = *((uint32_t *)data); 
    if (trust_type > ZXDH_TRUST_DSCP) {
        pr_err("[zxdh_rdma] %s[%d] pf=%u, vf=%u, is_vf=%u trust_type is out of range! trust_type=%u\n",
            __func__, __LINE__, iwdev->rf->pf_id, iwdev->rf->vf_id, iwdev->rf->ftype, trust_type);
        return -EINVAL;
    }

    iwdev->trust_type = trust_type; 
    zxdh_set_trust_type_to_fw(iwdev);

    pr_info("[zxdh_rdma] pf=%u, vf=%u, is_vf=%u change trust to %u.\n", 
            iwdev->rf->pf_id, iwdev->rf->vf_id, iwdev->rf->ftype, iwdev->trust_type);

    spin_lock_irqsave(&rf->vf_list_lock, flags);
    list_for_each_entry_safe(pos, tmp, &rf->vf_list, list) {
        vf_id[active_vf_num++] = pos->vf_id;
    }
    spin_unlock_irqrestore(&rf->vf_list_lock, flags);

    for (i = 0; i < active_vf_num; i++) {
        writel(0, (u32 __iomem *)(rf->sc_dev.hw->hw_addr + C_RDMA_CQP_CQ_DISTRIBUTE_DONE(rf->sc_dev.ep_id)));
        zxdh_sc_send_mailbox_cmd(&rf->sc_dev, ZTE_ZXDH_OP_CONFIG_TRUST_TYPE, iwdev->trust_type, 0, 0, vf_id[i]);
        cnt = 0;
        do {
            val = readl(rf->sc_dev.hw->hw_addr + C_RDMA_CQP_CQ_DISTRIBUTE_DONE(rf->sc_dev.ep_id));	
            if (cnt++ > ZXDH_MAILBOX_CYC_NUM_MIN) {
                pr_info("[zxdh_rdma] vhca_id:%d waiting completed pf to vf mailbox too long time,timeout!\n",rf->sc_dev.vhca_id);
                break;
            }
            if (rf->sc_dev.hw_attrs.skip_hw == true) {
               return 0;
            }
            usleep_range(ZXDH_MAILBOX_SLEEP_TIME, ZXDH_MAILBOX_SLEEP_TIME + 10);
        } while (!val);
    }

    return 0;
}

static int process_rdma_vf_vlan_event(struct net_device *netdev, void *data)
{
	struct zxdh_device *iwdev;
	struct zxdh_rdma_vlan_info *vlan_info;

	iwdev = zxdh_device_get_by_source_netdev(netdev);
	if (!iwdev) {
		return -ENODEV;
	}
	if (!iwdev->rf->ftype) {
		return 0;
	}

	if (NULL == data)
		return -EINVAL;

	vlan_info = (struct zxdh_rdma_vlan_info *)data;
	if (iwdev->rf->vf_id != vlan_info->vf_idx) {
		pr_err("[zxdh_rdma] %s set vf%d vlan failed, Current vf is %d\n", __func__, vlan_info->vf_idx, iwdev->vf_vlan.vf_idx);
		return -ENODEV;
	}
	iwdev->vf_vlan.vlan_id = vlan_info->vlan_id;
	iwdev->vf_vlan.qos = vlan_info->qos;
	iwdev->vf_vlan.vf_idx = vlan_info->vf_idx;
	iwdev->vf_vlan.protocol = vlan_info->protocol;
	if (iwdev->vf_vlan.vlan_id > MAX_VLAN_ID) {
		iwdev->vf_vlan.vlan_id = DEFAULT_VLAN_ID;
		pr_info("[zxdh_rdma] get invalid sriov vlan_id, set sriov vlan_id to %d\n", iwdev->vf_vlan.vlan_id);
	}
	if (iwdev->vf_vlan.qos > MAX_QOS_ID) {
		iwdev->vf_vlan.qos = DEFAULT_QOS_ID;
		pr_info("[zxdh_rdma] get invalid sriov qos, set sriov qos to %d\n", iwdev->vf_vlan.qos);
	}
	pr_info("[zxdh_rdma] %s vf%d vlan_id:%d qos:%d\n", __func__, iwdev->vf_vlan.vf_idx, iwdev->vf_vlan.vlan_id, iwdev->vf_vlan.qos);

	return 0;
}

/***
 * @brief zxdh_rdma event handler
 *
 * @param netdev zxdh_net device，Netdev is the network structure pointer corresponding to the PF or VF that needs to release resources
 * @param event_type，Types of events handled
 * @param data，Incoming parameters, default NULL
 * @return Return value 0 is OK, error is another value
 */
static int zxdh_rdma_event_handler(struct net_device *netdev, u8 event_type, void *data)
{
	int ret = 0;

	switch (event_type) {
	case ZXDH_RDMA_HEALTH_EVENT:
		ret = process_rdma_health_event(netdev);
		break;
	case ZXDH_RDMA_SRIOV_EVENT:
		ret = process_rdma_sriov_event(data);
		break;

	case ZXDH_RDMA_ETS_EVENT:
		ret = zxdh_handle_ets_event(netdev, data);
		break;

	case ZXDH_RDMA_TC_MAX_RATE_EVENT:
		ret = zxdh_rdma_set_maxrate(netdev, data);
		break;

	case ZXDH_RDMA_ETS_SWITCH_EVENT:
		ret = zxdh_rdma_ets_mode_switch_to_off(netdev, data);
		break;

	case ZXDH_RDMA_SPEED_CHANGE_EVENT:
		ret = process_rdma_speed_change_event(netdev, data);
		break;

	case ZXDH_RDMA_TRUST_EVENT:
		ret = zxdh_rdma_config_trust_type(netdev, data);
		break;
	case ZXDH_RDMA_VLAN_EVENT:
		ret = process_rdma_vf_vlan_event(netdev, data);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static u32 zxdh_get_sq_delta(u32 head, u32 tail, u32 size)
{
	u32 delta = 0;

	if ( (head > size) || (tail > size))
		return delta;
	if (head > tail) {
		delta = (head - tail);
	} else if (head < tail) {
		delta = (head + size - tail);
	}
	return delta;
}

static void zxdh_self_health_wait_res_free(u32 delta)
{
	if (delta <= 10)
		mdelay(5);
	else if (delta <= 26)
		mdelay(15);
	else if (delta <= 50)
		mdelay(30);
	else if (delta <= 100)
		mdelay(100);
	else if (delta <= 150)
		mdelay(150);
	else 
		mdelay(200);
}

void zxdh_handle_internal_error(struct zxdh_pci_f *rf) 
{
	struct zxdh_qp *qp;
	struct zxdh_cq *send_cq;
	struct zxdh_sc_dev *dev;
	__le64 *cqe;
	unsigned long flags_qp;
	unsigned long flags_cp;
	u32 wqe_idx;
	u64 hdr;
	bool wait_flag = false;
	struct zxdh_ring temp_sq_ring;
	struct zxdh_cq_uk temp_cq;
	u32 delta = 0;
    
	if (rf == NULL) {
        pr_err("[zxdh_rdma][%s] rf is NULL\n",__func__);
        return;
	}

	if (rf->iwdev == NULL) {
        pr_err("[zxdh_rdma][%s] iwdev is NULL\n",__func__);
        return;
	}

	qp = rf->iwdev->qp1;
	if (qp == NULL)
		return ;

	dev = &rf->sc_dev;
	if (dev == NULL)
		return ;

	if (dev->hw_attrs.skip_hw == false)
		return ;
	spin_lock_irqsave(&qp->lock, flags_qp);
	if (ZXDH_RING_CURRENT_HEAD(qp->sc_qp.qp_uk.sq_ring) != ZXDH_RING_CURRENT_TAIL(qp->sc_qp.qp_uk.sq_ring)) {
		delta = zxdh_get_sq_delta(qp->sc_qp.qp_uk.sq_ring.head, qp->sc_qp.qp_uk.sq_ring.tail, 
			qp->sc_qp.qp_uk.sq_ring.size);
		wait_flag = true;
		temp_sq_ring.head = qp->sc_qp.qp_uk.sq_ring.head;
		temp_sq_ring.tail = qp->sc_qp.qp_uk.sq_ring.tail;
		temp_sq_ring.size = qp->sc_qp.qp_uk.sq_ring.size;
		pr_info("[zxdh_rdma] %s vhca_id:%d\n",__func__,dev->vhca_id);
		send_cq = qp->iwscq;
		temp_cq.cq_base = send_cq->sc_cq.cq_uk.cq_base;
		temp_cq.cq_ring.head = send_cq->sc_cq.cq_uk.cq_ring.head;
		temp_cq.cq_ring.tail = send_cq->sc_cq.cq_uk.cq_ring.tail;
		temp_cq.cq_ring.size = send_cq->sc_cq.cq_uk.cq_ring.size;
		temp_cq.polarity = send_cq->sc_cq.cq_uk.polarity;
		spin_lock_irqsave(&send_cq->lock, flags_cp);
		if (!send_cq->user_mode)
			send_cq->armed = false;
		if (send_cq->ibcq.comp_handler && (send_cq->sc_cq.cq_uk.valid_cq == true)) {
			do {
				cqe = ZXDH_GET_CURRENT_EXTENDED_CQ_ELEM(&temp_cq);
				set_64bit_val(cqe, 8, qp->ctx_info.qp_compl_ctx);
				set_64bit_val(cqe, 24, 0);
				hdr =  FIELD_PREP(IRDMACQ_QPID, qp->ibqp.qp_num);

				dma_wmb(); /* make sure WQE is written before valid bit is set */
				set_64bit_val(cqe, 16, hdr);
				wqe_idx = ZXDH_RING_CURRENT_TAIL(temp_sq_ring);
				hdr = FIELD_PREP(ZXDH_CQPSQ_OPCODE, ZXDH_OP_TYPE_UD_SEND) |
					FIELD_PREP(ZXDH_CQ_WQEIDX, wqe_idx) |
					FIELD_PREP(ZXDH_CQ_ERROR, 1) |
					FIELD_PREP(ZXDH_CQ_MAJERR, ZXDH_FLUSH_MAJOR_ERR) |
					FIELD_PREP(ZXDH_CQ_MINERR, FLUSH_GENERAL_ERR) |
					FIELD_PREP(IRDMACQ_SOEVENT, 1) |
					FIELD_PREP(ZXDH_CQ_VALID, temp_cq.polarity) |
					FIELD_PREP(ZXDH_CQ_SQ, ZXDH_CQE_QTYPE_SQ) |
					FIELD_PREP(ZXDH_CQ_TYPE, 0) ;
					dma_wmb(); /* make sure WQE is written before valid bit is set */

				set_64bit_val(cqe, 0, hdr);
				pr_info("[zxdh_rdma] %s vhca_id:%d wqe_idx:%d sq_head:%d sq_tail:%d cq_head:%d\n",
					__func__,dev->vhca_id,wqe_idx,temp_sq_ring.head,temp_sq_ring.tail,temp_cq.cq_ring.head);
				ZXDH_RING_SET_TAIL(
				temp_sq_ring,
				wqe_idx + qp->sc_qp.qp_uk.sq_wrtrk_array[wqe_idx].quanta);
				ZXDH_RING_MOVE_HEAD_NOCHECK(temp_cq.cq_ring);
				if (!ZXDH_RING_CURRENT_HEAD(temp_cq.cq_ring))
					temp_cq.polarity ^= 1;
			}while(temp_sq_ring.head != temp_sq_ring.tail);
			send_cq->ibcq.comp_handler(&send_cq->ibcq, send_cq->ibcq.cq_context);
		}
		spin_unlock_irqrestore(&send_cq->lock,flags_cp);
	}
	spin_unlock_irqrestore(&qp->lock,flags_qp);
	if (wait_flag == true) {
		cancel_delayed_work_sync(&qp->dwork_flush);
		zxdh_self_health_wait_res_free(delta);
	}
}

static void zxdh_store_rdma_pf_glb (struct zxdh_pci_f *rf)
{
	int i;

	for (i = 0; i < HOST_RDMA_MAX_PF; i++)
	{
		if (pf_sriov_glb_info[i].rdma_pf_enable)
		{
			continue;
		}
		else
		{
			pf_sriov_glb_info[i].pdev = rf->pcidev;
			pf_sriov_glb_info[i].rf = rf;
			pf_sriov_glb_info[i].rdma_pf_enable = true;
			break;
		}
	}

	if (i >= HOST_RDMA_MAX_PF)
	{
		pr_err("[zxdh_rdma] rdma_pf_num over limit:%d\n", HOST_RDMA_MAX_PF);
	}

	return;
}

void zxdh_delete_rdma_pf_glb (struct zxdh_pci_f *rf)
{
	int i;

	for (i = 0; i < HOST_RDMA_MAX_PF; i++)
	{
		if (pf_sriov_glb_info[i].rdma_pf_enable && pf_sriov_glb_info[i].rf == rf)
		{
			pf_sriov_glb_info[i].pdev = NULL;
			pf_sriov_glb_info[i].rf = NULL;
			pf_sriov_glb_info[i].rdma_pf_enable = false;
			break;
		}
		else
		{
			continue;
		}
	}

	return;
}

static int zxdh_process_np_mac_check(struct zxdh_pci_f *rf, u8 opcode, struct zxdh_np_zcam_pos_info *pos_info)
{
	u8 flag = 0;

	if (!rf) {
		pr_info("[zxdh_rdma] [%s] rf is NULL!\n",__func__);
		return -ENOMEM;
	}

	if (!pos_info) {
		pr_info("[zxdh_rdma] [%s] pos_info is NULL!\n",__func__);
		return -ENOMEM;
	}

	if (rf->sc_dev.fw_np_mac == ZXDH_NP_MAC_CAP_FALSE) {
		return -EINVAL;
	}

	if (opcode == ZXDH_CMD_NP_MAC_ADD) {
		if (rf->sc_dev.flr_query == ZXDH_FLR_QUERY_FLAG) {
			flag = readl(rf->hw.hw_addr + C_RDMA_CQP_CONTEXT_6(rf->sc_dev.ep_id));
			if (flag == ZXDH_FLR_OP_FLAG) {
				return -EINVAL;
			}
		}
	}

	return 0;
}

int zxdh_set_trust_type_to_fw(struct zxdh_device *iwdev)
{
	int ret = 0;
	struct zxdh_rdma_common_req_msg req;

	req.opcode = ZXDH_CMD_SET_TRUST_TYPE;
	req.len = 3;
	req.buf[0] = iwdev->rf->ep_id;
	req.buf[1] = iwdev->rf->pf_id;
	req.buf[2] = iwdev->trust_type;
	ret = zxdh_rdma_send_common_msg(iwdev->rf, &req, NULL);
	return ret;
}

int zxdh_get_trust_type_from_fw(struct zxdh_device *iwdev)
{
	int ret = 0;
	struct zxdh_rdma_common_req_msg req; 
	struct zxdh_rdma_common_resp_msg resp;

	req.opcode = ZXDH_CMD_GET_TRUST_TYPE;
	req.len = 2;
	req.buf[0] = iwdev->rf->ep_id;
	req.buf[1] = iwdev->rf->pf_id;

	ret = zxdh_rdma_send_common_msg(iwdev->rf, &req, &resp);
	if (ret) {
		return ret;
	}

	if (resp.len != 2) {
		pr_err("[zxdh_rdma] fail to get trust_type! resp_len = %u\n", resp.len);
	}

	iwdev->trust_type = resp.buf[0];
	
	return ret;
}

static int zxdh_four_byte_little_endian(u8 *buf, u32 *val)
{
	if ((!buf) || (!val))
	    return -ENOMEM;

	buf[0] = *val;
	buf[1] = *val >> 8;
	buf[2] = *val >> 16;
	buf[3] = *val >> 24;

	return 0;
}

int zxdh_process_np_mac(struct zxdh_pci_f *rf, u8 opcode, u32 vhca_id, struct zxdh_np_zcam_pos_info *pos_info)
{
	int ret = 0;
	struct zxdh_rdma_common_req_msg req;

	ret = zxdh_process_np_mac_check(rf, opcode, pos_info);
	if (ret)
		return ret;
	req.opcode = opcode;
	req.len = sizeof(u32) + sizeof(struct zxdh_np_zcam_pos_info) + sizeof(u8);
	req.buf[0] = rf->ep_id;
	zxdh_four_byte_little_endian(&req.buf[1], &vhca_id);
	zxdh_four_byte_little_endian(&req.buf[5], &pos_info->entry_addr);
	zxdh_four_byte_little_endian(&req.buf[9], &pos_info->entry_size);
	zxdh_four_byte_little_endian(&req.buf[13], &pos_info->entry_pos);
	ret = zxdh_rdma_send_common_msg(rf, &req, NULL);
	return ret;
}

void zxdh_flr_config(struct zxdh_pci_f *rf)
{
    struct zxdh_sc_dev *dev;
    dev = &rf->sc_dev;

    writel(ZXDH_HW_TABLE4_EP_ID_15, (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE4(dev->ep_id)));
    writel(ZXDH_HW_TABLE2_SID_61, (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE2(dev->ep_id)));
    writel(ZXDH_HW_SCHEDULE_OFF, (u32 __iomem *)(dev->hw->hw_addr + RDMATX_QUEUE_VHCA_FLAG(dev->ep_id)));
}

int zxdh_add_vf_id(struct zxdh_pci_f *rf, u16 vhca_id, u16 vf_id)
{
    struct zxdh_vf_id_node *new_node, *pos;
    unsigned long flags;

    spin_lock_irqsave(&rf->vf_list_lock, flags);
    list_for_each_entry(pos, &rf->vf_list, list){
        if (pos->vhca_id == vhca_id) {
            spin_unlock_irqrestore(&rf->vf_list_lock, flags);
            return -EEXIST;
        }
    }
	spin_unlock_irqrestore(&rf->vf_list_lock, flags);
    new_node = kmalloc(sizeof(struct zxdh_vf_id_node), GFP_ATOMIC);
    if (!new_node) {
        pr_err("[zxdh_rdma]%s alloc memory failed!\n", __func__);
        return -ENOMEM;
    }
    new_node->vhca_id = vhca_id;
    new_node->vf_id = vf_id;
	spin_lock_irqsave(&rf->vf_list_lock, flags);
    list_add_tail(&new_node->list, &rf->vf_list);
    spin_unlock_irqrestore(&rf->vf_list_lock, flags);
    return 0;
}

int zxdh_del_vf_id(struct zxdh_pci_f *rf, u16 vhca_id)
{
    struct zxdh_vf_id_node *tmp, *pos;
    unsigned long flags;

    spin_lock_irqsave(&rf->vf_list_lock, flags);
    list_for_each_entry_safe(pos, tmp, &rf->vf_list, list){
        if (pos->vhca_id == vhca_id) {
            list_del(&pos->list);
            spin_unlock_irqrestore(&rf->vf_list_lock, flags);
			kfree(pos);
            return 0;
        }
    }
    spin_unlock_irqrestore(&rf->vf_list_lock, flags);
    return 0;
}

static int zxdh_get_del_vf_id(struct zxdh_pci_f *rf, u16 *vf_id)
{
    struct zxdh_vf_id_node *pos, *tmp;
    unsigned long flags;

    spin_lock_irqsave(&rf->vf_list_lock, flags);
    list_for_each_entry_safe(pos, tmp, &rf->vf_list, list) {
        *vf_id = pos->vf_id;
        list_del(&pos->list);
        spin_unlock_irqrestore(&rf->vf_list_lock, flags);
		kfree(pos);
        return 0;
    }
    spin_unlock_irqrestore(&rf->vf_list_lock, flags);
    return -ENOMEM;
}

static bool zxdh_vf_list_empty(struct zxdh_pci_f *rf)
{
    unsigned long flags;
    bool empty = 1;

    spin_lock_irqsave(&rf->vf_list_lock, flags);
    empty = list_empty(&rf->vf_list);
    spin_unlock_irqrestore(&rf->vf_list_lock, flags);
    return empty;
}

int zxdh_send_mailbox_msg(struct zxdh_pci_f *rf, u8 opcode, u64 msg2, u64 msg3, u64 msg4)
{
    struct zxdh_sc_dev *dev = &rf->sc_dev;
    u32 cnt = 0, val = 0, status = 0;
    u32 num = 0;
    
    if (opcode >= ZTE_ZXDH_OP_MAX_NUM) {
        pr_err("[zxdh_rdma]%s invalid opcode:%d\n", __func__, opcode);
        return -EINVAL;
    }

    if (opcode == ZTE_ZXDH_OP_DEVICE_UNLOAD)
        num = ZXDH_MAILBOX_CYC_NUM_DEFAULT * dev->hw_attrs.max_done_count;
    else 
        num = ZXDH_MAILBOX_CYC_NUM * dev->hw_attrs.max_done_count;

    writel(0, (u32 __iomem *)(dev->hw->hw_addr + C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id)));

    zxdh_sc_send_mailbox_cmd(dev, opcode, msg2, msg3, msg4, dev->vf_id);

    do {
        val = readl(dev->hw->hw_addr + C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id));			
        if (cnt++ > num) {
            status = -ETIMEDOUT;
            pr_info("[zxdh_rdma] vhca_id:%d waiting completed opcode:%d mailbox too long time,timeout!\n", dev->vhca_id, opcode);
            break;
        }
        if (dev->hw_attrs.skip_hw == true) {
            status = -ETIMEDOUT;
            break;
        }
        usleep_range(ZXDH_MAILBOX_SLEEP_TIME, ZXDH_MAILBOX_SLEEP_TIME + 5);
    } while (!val);

    return status;
}

void zxdh_set_device_completed(struct zxdh_pci_f *rf)
{
    u32 val = 0;
	
    val = readl((u32 __iomem *)(rf->sc_dev.hw->hw_addr + C_RDMA_CQP_CONTEXT_7(rf->sc_dev.ep_id)));
    val &= (~ZXDH_32BIT_1_2_MASK);
    val |= ZXDH_DEVICE_INIT_COMPLETED;
    writel(val, (u32 __iomem *)(rf->sc_dev.hw->hw_addr + C_RDMA_CQP_CONTEXT_7(rf->sc_dev.ep_id)));
}

static int zxdh_send_vf_flr(struct zxdh_pci_f *rf, u8 opcode)
{
    int ret = 0;
    struct zxdh_rdma_common_req_msg req;

    req.opcode = opcode;
    req.len = 4*sizeof(u8);
    req.buf[0] = rf->ep_id;
    req.buf[1] = rf->pf_id;
    req.buf[2] = rf->vf_id;
    req.buf[3] = rf->ftype;// vf active;
    ret = zxdh_rdma_send_common_msg(rf, &req, NULL);
    return ret;
}

static void zxdh_send_del_pf_to_vf(struct zxdh_pci_f *rf)
{
    int ret = 0;
    u32 cnt = 0, val = 0;
    bool empty = 1;
    u8 vf_flag = 0;
    u16 vf_id = 0;
	
    if (rf->sc_dev.hw_attrs.skip_hw == true)
        return ;
    do
    {
        empty = zxdh_vf_list_empty(rf);
        if (empty == 0) {
            ret = zxdh_get_del_vf_id(rf, &vf_id);
            if (ret == 0) {
                if (vf_flag == 0) 
                    vf_flag = 1;
                writel(0, (u32 __iomem *)(rf->sc_dev.hw->hw_addr +
                    C_RDMA_CQP_CQ_DISTRIBUTE_DONE(rf->sc_dev.ep_id)));
                    zxdh_sc_send_mailbox_cmd(&rf->sc_dev, ZTE_ZXDH_OP_PF_UNLOAD, 0, 0, 0, vf_id);
                cnt = 0;
                do {
                    val = readl(rf->sc_dev.hw->hw_addr +
                        C_RDMA_CQP_CQ_DISTRIBUTE_DONE(rf->sc_dev.ep_id));			
                    if (cnt++ > ZXDH_MAILBOX_CYC_NUM_MIN) {
                        pr_info("[zxdh_rdma] vhca_id:%d waiting completed pf to vf mailbox too long time,timeout!\n",rf->sc_dev.vhca_id);
                        break;
                    }
                    if (rf->sc_dev.hw_attrs.skip_hw == true) {
                       return ;
                    }
                    usleep_range(ZXDH_MAILBOX_SLEEP_TIME, ZXDH_MAILBOX_SLEEP_TIME + 5);
                } while (!val);
            }
        }
    } while (!empty);
    if (rf->sc_dev.vf_flr_cap && vf_flag) {
        ret = zxdh_send_vf_flr(rf, ZXDH_CMD_SEND_VF_FLR);
        if (ret)
            pr_err("[zxdh_rdma] send vf flr msg failed\n");
   }
}

static void zxdh_set_device_valid(struct zxdh_auxiliary_dev *iidc_adev)
{
    if (iidc_adev == NULL) {
        pr_err("[zxdh_rdma][%s] iidc_adev is NULL\n",__func__);
        return ;
    }
    if (iidc_adev->zxdh_info == NULL) {
        pr_err("[zxdh_rdma][%s] iidc_adev->zxdh_info is NULL\n",__func__);
        return ;
    }
    iidc_adev->zxdh_info->ver.support = iidc_adev->zxdh_info->ver.support + (1 << ZXDH_HIGH_25BIT_MAP);
}

static void zxdh_set_device_invalid(struct iidc_auxiliary_dev *iidc_adev)
{
    u64 value = 1;
    
    if (iidc_adev == NULL) {
        pr_err("[zxdh_rdma][%s] iidc_adev is NULL\n",__func__);
        return ;
    }
    if (iidc_adev->cdev_info == NULL) {
        pr_err("[zxdh_rdma][%s] iidc_adev->cdev_info is NULL\n",__func__);
        return ;
    }
    value = (1 << ZXDH_HIGH_25BIT_MAP);
    value = ~value;

    iidc_adev->cdev_info->ver.support = (iidc_adev->cdev_info->ver.support & value);
}

static int zxdh_get_device_sta(struct iidc_auxiliary_dev *iidc_adev)
{
    u8 value = 0;

    if (iidc_adev == NULL) {
        pr_err("[zxdh_rdma][%s] iidc_adev is NULL\n",__func__);
        return -ENOMEM;
    }
    if (iidc_adev->cdev_info == NULL) {
        pr_err("[zxdh_rdma][%s] iidc_adev->cdev_info is NULL\n",__func__);
        return -ENOMEM;
    }
    value = (u8)FIELD_GET(ZXDH_RDMA_DEVICE_STA, iidc_adev->cdev_info->ver.support); 
    return value;
}

static int zxdh_refresh_qp1_qpc_after_tx_package_err(struct zxdh_qp *iwqp)
{
	int ret = 0;
	struct zxdh_modify_qp_info info = { 0 };

	if (iwqp == NULL) {
		pr_err("[zxdh_rdma][%s][%d] iwqp is NULL!\n", __func__, __LINE__);
		return -EINVAL;
	}
	if (iwqp->ibqp.qp_num != 1) {
		return -EPERM;
	}

	info.qpc_tx_mask_low = RDMAQPC_MASK_INIT;
	info.qpc_tx_mask_high = RDMAQPC_MASK_INIT;
	info.qpc_rx_mask_low = RDMAQPC_MASK_INIT;
	info.qpc_rx_mask_high = RDMAQPC_MASK_INIT;

	ret = zxdh_hw_modify_qp(iwqp->iwdev, iwqp, &info, true);
	if (ret) {
		pr_err("[zxdh_rdma][%s][%d] zxdh_hw_modify_qp error! ret:%d\n",
		       __func__, __LINE__, ret);
		return -EINVAL;
	}

	return 0;
}

static int zxdh_modify_qp_link_in(struct zxdh_qp *iwqp, u64 link_in)
{
	int ret = 0;
	__le64 *qp_ctx = iwqp->host_ctx.va;
	struct zxdh_modify_qp_info info = {0};
	u64 hdr;
	u64 mask;

	get_64bit_val(qp_ctx, 0xa8, &hdr);
	mask = FIELD_PREP(RDMAQPC_TX_QPSTATE, 0x7);
	hdr &= ~mask;
	hdr |= FIELD_PREP(RDMAQPC_TX_QPSTATE, 0);

	mask = FIELD_PREP(RDMAQPC_TX_QP_LINK_IN, 0x1);
	hdr &= ~mask;
	hdr |= FIELD_PREP(RDMAQPC_TX_QP_LINK_IN, link_in);
	set_64bit_val(qp_ctx, 0xa8, hdr);

	info.qpc_tx_mask_low = 0;
	info.qpc_tx_mask_high = ((0x1UL << (72 - 64)) | (0x1UL << (74 - 64)));
	info.qpc_rx_mask_low = 0;
	info.qpc_rx_mask_high = 0;

	ret = zxdh_hw_modify_qp(iwqp->iwdev, iwqp, &info, true);
	if (ret) {
		pr_err("[zxdh_rdma][%s][%d] zxdh_hw_modify_qp error! ret:%d\n",
		       __func__, __LINE__, ret);
		return -EINVAL;
	}

	return 0;
}

static int zxdh_sc_qp_query_qpc(struct zxdh_qp *iwqp, uint16_t index)
{
	int ret = 0;
	u32 qp_num;
	struct zxdh_device *iwdev = iwqp->iwdev;
	struct zxdh_pci_f *rf = iwdev->rf;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_dma_mem qpc_buf = { 0 };
	struct zxdh_sc_qp temp_qp = { 0 };

	qpc_buf.size = ALIGN(ZXDH_QP_CTX_SIZE, ZXDH_QPC_ALIGNMENT);
	qpc_buf.va = dma_alloc_coherent(iwqp->iwdev->rf->sc_dev.hw->device,
					qpc_buf.size, &qpc_buf.pa, GFP_KERNEL);

	if (!qpc_buf.va) {
		pr_err("[zxdh_rdma][%s][%d] no memory\n", __func__, __LINE__);
		return -ENOMEM;
	}

	if (index < rf->max_qp - 2) {
		qp_num = dev->base_qpn + 2 + index;
	} else {
		qp_num = dev->base_qpn - index;
	}

	if (qp_num == iwqp->sc_qp.qp_ctx_num) {
		pr_warn("[zxdh_rdma][%s][%d]skip query this qp_num:%u == qp1.\n", __func__, __LINE__, qp_num);
		ret = -EINVAL;
		goto error;
	}

	temp_qp.qp_ctx_num = qp_num;
	temp_qp.qp_uk.qp_id = qp_num;
	temp_qp.dev = dev;
	zxdh_query_qpc(&temp_qp, &qpc_buf);

error:
	dma_free_coherent(iwqp->iwdev->rf->sc_dev.hw->device, qpc_buf.size,
			  qpc_buf.va, qpc_buf.pa);

	return ret;
}

int check_rdma_tx_package_err(struct zxdh_qp *iwqp)
{
	int ret = 0;
	u16 i = 0;
	u64 temp = 0;
	u32 tx_package_err_flag, cur_sq_link_in, old_sq_link_in, reg_val;
	u32 hmc_qpc_tx_base_low, hmc_qpc_tx_base_high;

	struct zxdh_dma_mem qpc_buf = { 0 };
	struct zxdh_sc_qp *qp = &iwqp->sc_qp;
	struct zxdh_device *iwdev = iwqp->iwdev;
	struct zxdh_pci_f *rf = iwdev->rf;
	struct zxdh_sc_dev *dev = &rf->sc_dev;

	if (dev->hw_attrs.skip_hw == true){
		pr_info("[zxdh_rdma][%s][%d] skip_hw\n", __FUNCTION__, __LINE__);
		return 0;
	}

	qpc_buf.size = ALIGN(ZXDH_QP_CTX_SIZE, ZXDH_QPC_ALIGNMENT);
	qpc_buf.va = dma_alloc_coherent(iwqp->iwdev->rf->sc_dev.hw->device,
					qpc_buf.size, &qpc_buf.pa, GFP_KERNEL);
	if (!qpc_buf.va) {
		pr_err("[zxdh_rdma] no memory\n");
		return -ENOMEM;
	}

	ret = zxdh_rdma_reg_read(rf, RDMATX_NUM_OF_LINK_IN_ADDR, &reg_val);
	if (ret != 0) {
		pr_err("[zxdh_rdma][%s][%d] zxdh_rdma_reg_read failed, ret:%d\n",
		       __FUNCTION__, __LINE__, ret);
		goto error;
	}
	old_sq_link_in = FIELD_GET(GENMASK_ULL(31, 16), reg_val);

	ret = zxdh_query_qpc(qp, &qpc_buf);
	if (ret != 0) {
		pr_err("[zxdh_rdma][%s][%d] query qpc failed, ret:%d, old_sq_link_in:%u\n",
		       __FUNCTION__, __LINE__, ret, old_sq_link_in);
		goto error;
	}
	get_64bit_val((__le64 *)qpc_buf.va, 0x30, &temp);
	tx_package_err_flag = FIELD_GET(RDMAQPC_TX_PACKAGE_ERR_FLAG, temp);

	if (tx_package_err_flag) {
		writel(ZXDH_HW_SCHEDULE_OFF, (u32 __iomem *)(dev->hw->hw_addr + RDMATX_QUEUE_VHCA_FLAG(dev->ep_id)));

		for (i = 0; i < 2048; i++) {
			zxdh_sc_qp_query_qpc(iwqp, i); //查询qp，让qp1老化
		}
		mdelay(5);

		hmc_qpc_tx_base_low =
			readl((u32 __iomem *)(dev->hw->hw_addr +
					      C_HMC_QPC_TX_BASE_LOW(dev->ep_id)));
		hmc_qpc_tx_base_high =
			readl((u32 __iomem *)(dev->hw->hw_addr +
					      C_HMC_QPC_TX_BASE_HIGH(dev->ep_id)));
		mdelay(1);

		writel(0xB8000000, (u32 __iomem *)(dev->hw->hw_addr +
						   C_HMC_QPC_TX_BASE_LOW(dev->ep_id)));
		writel(0x7, (u32 __iomem *)(dev->hw->hw_addr + C_HMC_QPC_TX_BASE_HIGH(dev->ep_id)));

		zxdh_modify_qp_link_in(iwqp, 1);

		writel(hmc_qpc_tx_base_low, (u32 __iomem *)(dev->hw->hw_addr + C_HMC_QPC_TX_BASE_LOW(dev->ep_id)));
		writel(hmc_qpc_tx_base_high, (u32 __iomem *)(dev->hw->hw_addr + C_HMC_QPC_TX_BASE_HIGH(dev->ep_id)));

		writel(ZXDH_HW_SCHEDULE_ON, (u32 __iomem *)(dev->hw->hw_addr + RDMATX_QUEUE_VHCA_FLAG(dev->ep_id)));

		mdelay(1);
		for (i = 0; i < 2048; i++) {
			zxdh_sc_qp_query_qpc(iwqp, i); //查询qp，让qp1老化
		}
		mdelay(5);

		zxdh_refresh_qp1_qpc_after_tx_package_err(iwqp);

		ret = zxdh_rdma_reg_read(rf, RDMATX_NUM_OF_LINK_IN_ADDR, &reg_val);
		if (ret != 0) {
			pr_err("[zxdh_rdma][%s][%d] zxdh_rdma_reg_read failed, ret:%d\n",
			       __FUNCTION__, __LINE__, ret);
			goto error;
		}
		cur_sq_link_in = FIELD_GET(GENMASK_ULL(31, 16), reg_val);

		ret = zxdh_query_qpc(qp, &qpc_buf);
		if (ret != 0) {
			pr_err("[zxdh_rdma][%s][%d] query qpc failed, ret:%d.\n",
				__FUNCTION__, __LINE__, ret);
			goto error;
		}

		get_64bit_val((__le64 *)qpc_buf.va, 0x30, &temp);
		tx_package_err_flag = FIELD_GET(RDMAQPC_TX_PACKAGE_ERR_FLAG, temp);
		if (tx_package_err_flag) {
			pr_info("[zxdh_rdma][%s][%d]need to recheck vhca:%u! tx_package_err_flag:%u sq_link_in:%u->%u\n",
				__func__, __LINE__, rf->sc_dev.vhca_id,
				tx_package_err_flag, old_sq_link_in,
				cur_sq_link_in);
			ret = -EAGAIN;
		} else {
			pr_info("[zxdh_rdma][%s][%d]success to recovery vhca:%u! tx_package_err_flag:%u sq_link_in:%u->%u\n",
				__func__, __LINE__, rf->sc_dev.vhca_id,
				tx_package_err_flag, old_sq_link_in,
				cur_sq_link_in);
		}
	}

error:
	dma_free_coherent(iwqp->iwdev->rf->sc_dev.hw->device, qpc_buf.size,
			  qpc_buf.va, qpc_buf.pa);
	return ret;
}

static int zxdh_remove(struct zxdh_auxiliary_device *aux_dev)
{
	dpp_pf_info_t pf_info = { 0 };
	struct iidc_auxiliary_dev *iidc_adev =
		container_of(aux_dev, struct iidc_auxiliary_dev, adev);
	struct iidc_core_dev_info *cdev_info = iidc_adev->cdev_info;
	struct zxdh_device *iwdev;
	struct zxdh_sc_dev *dev;
	int ret = 0;

	if (zxdh_get_device_sta(iidc_adev) != ZXDH_RDMA_DEVICE_VALID) {
		pr_err("[zxdh_rdma][%s][%d] dev is free\n", __func__, __LINE__);
		return 0;
	}
    zxdh_set_device_invalid(iidc_adev);

    iwdev = dev_get_drvdata(&aux_dev->dev);
    if (iwdev == NULL) {
        pr_err("[zxdh_rdma]%s[%d] iwdev is NULL\n",__func__,__LINE__);
        return 0;
    }
    if (iwdev->rf == NULL) {
        pr_err("[zxdh_rdma]%s[%d] rf is NULL\n",__func__,__LINE__);
        return 0;
    }
    dev = &iwdev->rf->sc_dev;
    if (dev->driver_load == false) {
        pr_err("[zxdh_rdma]%s[%d] the driver has benn uninstalled\n",__func__,__LINE__);
        return 0;
    }

	dev->driver_load = false;
	if ((dev->hw_attrs.skip_hw == false) && (zxdh_rdma_check_remove_state(iwdev->rf->pcidev)) == ZXDH_PCIE_LINK_DOWN) {
		zxdh_rdma_hotplug_event(iwdev->rf);
	}

	if (iwdev->rf->ftype  && (iwdev->rf->sc_dev.hw_attrs.skip_hw == false)) {
	    ret = zxdh_send_mailbox_msg(iwdev->rf, ZTE_ZXDH_OP_DEVICE_UNLOAD, 0, 0, 0);
		if (ret) 
		    pr_err("[zxdh_rdma]Unload VF failed to be sent to PF\n");
	}

	if (iwdev->rf->sc_dev.np_mode_low_lat)
		zxdh_cleanup_dpp_mac_tbl(iwdev);
    if (iwdev->rf->ftype == 0) {
        zxdh_send_del_pf_to_vf(iwdev->rf);
	}
	if ((iwdev->rf->ftype == 0) && (dev->hw_attrs.skip_hw == false)) {
		if (!iwdev->rf->sc_dev.np_mode_low_lat) {
			pf_info.vport = cdev_info->vport_id;
			pf_info.slot = cdev_info->slot_id;
			ret = dpp_vport_attr_set(&pf_info, EGR_FLAG_RDMA_OFFLOAD_EN_OFF,
				   EGR_RDMA_OFFLOAD_OFF);
			if (ret != 0) {
				pr_err("[zxdh_rdma] %s[%d]: dpp vport attr set EGR_FLAG_RDMA_OFFLOAD_EN_OFF fail! ret=%u!\n", __func__, __LINE__, ret);
		    }
		}
	}

	zxdh_handle_internal_error(iwdev->rf);
	zrdma_cleanup_rdma_tools_cfg(iwdev->rf);
	zrdma_cleanup_debugfs_entry(iwdev->rf);
	zxdh_ib_unregister_device(iwdev);

	if (iwdev->rf->cdev) {
		kfree(iwdev->rf->cdev);
		iwdev->rf->cdev = NULL;
	}
	if (iwdev->rf->config_path) {
		kfree(iwdev->rf->config_path);
		iwdev->rf->config_path = NULL;
	}

#ifndef IB_DEALLOC_DRIVER_SUPPORT
	/* In newer kernels core issues callback zxdh_ib_dealloc_device to cleanup on ib unregister
	 * Older kernels require cleanup here
	 */
    zxdh_destory_eth_info_hlist(iwdev);
	/* 反初始化ETS动态令牌管理器 */
	zxdh_ets_token_mgr_deinit(iwdev->rf);
	zxdh_destroy_dip_info_hlist(iwdev->rf);

	if (!iwdev->rf->ftype)
	{
		zxdh_delete_rdma_pf_glb(iwdev->rf);
	}

	zxdh_rt_deinit_hw(iwdev);
	zxdh_ctrl_deinit_hw(iwdev->rf);
	zxdh_del_handler(iwdev->hdl);
#ifdef MSIX_DEBUG
	pci_free_irq_vectors(cdev_info->pdev);
#endif
	if (iwdev->rf->iw_msixtbl) {
		kfree(iwdev->rf->iw_msixtbl);
		iwdev->rf->iw_msixtbl = NULL;
	}
	if (iwdev->rf->use_ext_mem_flag) {
		// 销毁扩展内存池
		zxdh_destroy_ext_mem_pool(iwdev->rf);

		zxdh_cleanup_mem_info(iwdev->rf);
	}

	if (iwdev->hdl) {
		kfree(iwdev->hdl);
		iwdev->hdl = NULL;
	}
	ib_dealloc_device(&iwdev->ibdev);
#endif /* IB_DEALLOC_DRIVER_SUPPORT */
	kfree(iwdev->rf);
	iwdev->rf = NULL;
    iwdev = NULL;
    dev_info(&cdev_info->pdev->dev,
	     "[zxdh_rdma] remove PF[%d] device success\n",
	     PCI_FUNC(cdev_info->pdev->devfn));
    return 0;
}

/**
 * zxdh_shutdown - trigger when reboot
 * @aux_dev: auxiliary device ptr
 */
static void zxdh_shutdown(struct zxdh_auxiliary_device *aux_dev)
{
	zxdh_remove(aux_dev);
}

#ifdef MSIX_DEBUG

static int ft_debug_msix_interrupt(struct pci_dev *pdev,
				   struct msix_entry *msix, u32 msix_num)
{
	struct msix_entry *temp_msix;
	int ret;
	int i;

	temp_msix = msix;
	if (pci_enable_device(pdev)) {
		pr_info("[zxdh_rdma] %s enable pcie msix failed!\n", __func__);
		return -1;
	}
	ret = pci_alloc_irq_vectors_affinity(pdev, msix_num, msix_num,
					     PCI_IRQ_MSIX, NULL);
	if (ret < 0) {
		pr_info("[zxdh_rdma] %s alloc irq vectors failed!\n", __func__);
		return -1;
	}
	pr_info("[zxdh_rdma] %s alloc irq vectors ret:%d\n", __func__, ret);

	for (i = 0; i < msix_num; i++) {
		temp_msix->vector = pci_irq_vector(pdev, i);
		temp_msix->entry = i;
		pr_info("[zxdh_rdma] %s vector:%d entry:%d\n", __func__, temp_msix->vector,
			temp_msix->entry);
		temp_msix++;
	}

	return 0;
}
#endif

void zxdh_handle_upper_dev(struct zxdh_device *iwdev, u8 opcode)
{
	struct net_dev_entry *netdev_list, *tmp, *entry;
	struct net_device *slave;
	struct list_head *iter;

	// list头，以及初始化
	netdev_list = kzalloc(sizeof(*netdev_list), GFP_KERNEL);
	if (!netdev_list)
		return;
	INIT_LIST_HEAD(&netdev_list->lnode);

	rcu_read_lock();
	// 创建list节点, 获取slave, 并添加到list中
	netdev_for_each_upper_dev_rcu(iwdev->netdev, slave, iter) {
		entry = kmalloc(sizeof(*entry), GFP_ATOMIC);
		if (unlikely(!entry)) {
			pr_err("Failed to allocate memory. Stopping capture.\n");
			goto clear_out;
		}

		entry->dev = slave;
		list_add_tail(&entry->lnode, &netdev_list->lnode);
		pr_info("[zxdh_rdma] %s[%d]: master:%s slave:%s slave_addr:%02x:%02x:%02x:%02x:%02x:%02x\n",
			__func__, __LINE__, iwdev->netdev->name, slave->name, slave->dev_addr[0], slave->dev_addr[1], slave->dev_addr[2], slave->dev_addr[3], slave->dev_addr[4],slave->dev_addr[5]);
	}
	rcu_read_unlock();

clear_out:
	// 遍历list，将每个设备重新加入到mac表中。并释放节点
	list_for_each_entry_safe(entry, tmp, &netdev_list->lnode, lnode) {

		if(ZXDH_CMD_NP_MAC_ADD == opcode) {
			zxdh_add_dpp_mac_tbl(iwdev, entry->dev);
		} else if (ZXDH_CMD_NP_MAC_DEL == opcode) {
			zxdh_del_dpp_mac_tbl(iwdev, entry->dev, false);
		}

		list_del(&entry->lnode);
		kfree(entry);
	}
	// 释放list头
	kfree(netdev_list);
}

static void zxdh_cfg_dpp(struct zxdh_device *iwdev,
			 struct iidc_core_dev_info *cdev_info)
{
	dpp_pf_info_t pf_info = { 0 };
    u32 ret = 0;

	pf_info.vport = cdev_info->vport_id;
	pf_info.slot = cdev_info->slot_id;

	if (!iwdev->rf->sc_dev.np_mode_low_lat) {
		dpp_vport_vhca_id_add(&pf_info, iwdev->rf->sc_dev.vhca_id);
		ret = dpp_vport_attr_set(&pf_info, EGR_FLAG_VHCA,
				   iwdev->rf->sc_dev.vhca_id);
        if (ret != 0) {
            pr_err("[zxdh_rdma] %s[%d]: dpp vport attr set EGR_FLAG_VHCA fail! ret=%u!\n", __func__, __LINE__, ret);
			return;
        }
		ret = dpp_vport_attr_set(&pf_info, EGR_FLAG_RDMA_OFFLOAD_EN_OFF,
				   EGR_RDMA_OFFLOAD_EN);
        if (ret != 0) {
            pr_err("[zxdh_rdma] %s[%d]: dpp vport attr set EGR_FLAG_RDMA_OFFLOAD_EN_OFF fail! ret=%u!\n", __func__, __LINE__, ret);
			return;
        }
	}
}

static void zxdh_init_eth_info_hlist(struct zxdh_device *iwdev)
{
    int i = 0;

    iwdev->eth_info_hlist = (struct hlist_head *)kmalloc(sizeof(struct hlist_head) * ETH_INFO_HASH_COUNT, GFP_ATOMIC);
    if (!iwdev->eth_info_hlist) {
        pr_err("[zxdh_rdma] %s[%d]: kmalloc return NULL!", __func__, __LINE__);
		return;
    }
    for(i = 0; i < ETH_INFO_HASH_COUNT; i++)        // hash数组动态初始化
        INIT_HLIST_HEAD(&iwdev->eth_info_hlist[i]);
    mutex_init(&iwdev->eth_info_list_mtx_lock);
}

void zxdh_destory_eth_info_hlist(struct zxdh_device *iwdev)
{
    struct zxdh_eth_info *hnode = NULL;
    struct hlist_node *hlist = NULL;
    int i = 0;
    int valid_hlist_num = 0;

    for (i = 0; i < ETH_INFO_HASH_COUNT; i++) {
	    // 遍历每一个槽，有结点就删除
	    hlist_for_each_entry_safe(hnode, hlist, &iwdev->eth_info_hlist[i], list) {
		    valid_hlist_num++;
		    hlist_del(&hnode->list);
		    kfree(hnode); // kmalloc in zxdh_eth_info_hlist_add
	    }
    }
    kfree(iwdev->eth_info_hlist);
    iwdev->eth_info_hlist = NULL;

    pr_debug("[zxdh_rdma][%s][%d]: valid_hlist_num=%d\n", __func__, __LINE__, valid_hlist_num);
}

void zxdh_eth_info_hlist_display(struct zxdh_device *iwdev)
{
    struct zxdh_eth_info *hnode = NULL;
    struct hlist_node *hlist = NULL;
    int i = 0;
    int valid_hlist_num = 0;
    int valid_node_num = 0;
    int ip_cfg_ref_num = 0;

    for(i=0; i < ETH_INFO_HASH_COUNT; i++) {
        if (!hlist_empty(&iwdev->eth_info_hlist[i])) {
            valid_hlist_num++;
        }

        hlist_for_each_entry_safe(hnode, hlist, &iwdev->eth_info_hlist[i], list) {
            valid_node_num++;
            ip_cfg_ref_num += hnode->ip_cfg_ref_cnt;
            pr_debug("[zxdh_rdma] %s[%d]: hlist key=%d, src_ip=0x%x-0x%x-0x%x-0x%x, dst_ip=0x%x-0x%x-0x%x-0x%x, name=%s, ip_cfg_ref_cnt=%d\n", __func__, __LINE__, i,
                hnode->rdma_to_eth_ip_para.src_ip[0], hnode->rdma_to_eth_ip_para.src_ip[1], hnode->rdma_to_eth_ip_para.src_ip[2], hnode->rdma_to_eth_ip_para.src_ip[3],
                hnode->rdma_to_eth_ip_para.dst_ip[0], hnode->rdma_to_eth_ip_para.dst_ip[1], hnode->rdma_to_eth_ip_para.dst_ip[2], hnode->rdma_to_eth_ip_para.dst_ip[3], hnode->rdma_to_eth_ip_para.ifname, hnode->ip_cfg_ref_cnt);
        }
    }
    pr_debug("[zxdh_rdma] %s[%d]: valid_hlist_num=%d, valid_node_num=%d, ip_cfg_ref_num=%d\n", __func__, __LINE__, valid_hlist_num, valid_node_num, ip_cfg_ref_num);
}

static int zxdh_eth_info_cmp(struct zxdh_rdma_to_eth_ip_para *ip_para, struct zxdh_eth_info *info2)
{
    if (ip_para->src_ip[0] == info2->rdma_to_eth_ip_para.src_ip[0] && ip_para->src_ip[1] == info2->rdma_to_eth_ip_para.src_ip[1] && ip_para->src_ip[2] == info2->rdma_to_eth_ip_para.src_ip[2] && ip_para->src_ip[3] == info2->rdma_to_eth_ip_para.src_ip[3] &&
        ip_para->dst_ip[0] == info2->rdma_to_eth_ip_para.dst_ip[0] && ip_para->dst_ip[1] == info2->rdma_to_eth_ip_para.dst_ip[1] && ip_para->dst_ip[2] == info2->rdma_to_eth_ip_para.dst_ip[2] && ip_para->dst_ip[3] == info2->rdma_to_eth_ip_para.dst_ip[3] &&
        memcmp(ip_para->ifname, info2->rdma_to_eth_ip_para.ifname, strlen(ip_para->ifname)) == 0) {
            return 0;
    }

    return 1;
}

static u32 src_dst_ipv4_hash(const struct zxdh_rdma_to_eth_ip_para *ip_para)
{
    u32 hash = jhash(&ip_para->src_ip[3], sizeof(ip_para->src_ip[3]), 0);
    u32 key = jhash(&ip_para->dst_ip[3], sizeof(ip_para->dst_ip[3]), hash);

    return key % ETH_INFO_HASH_COUNT;
}

static u32 src_dst_ipv6_hash(const struct zxdh_rdma_to_eth_ip_para *ip_para)
{
    u32 hash = jhash(ip_para->src_ip, sizeof(ip_para->src_ip), 0);
    u32 key = jhash(ip_para->dst_ip, sizeof(ip_para->dst_ip), hash);

    return key % ETH_INFO_HASH_COUNT;
}

int zxdh_eth_info_hlist_add(struct zxdh_device *iwdev, struct zxdh_rdma_to_eth_ip_para *ip_para)
{
    struct zxdh_eth_info *hnode = NULL;
	struct hlist_node *hlist = NULL;
    u32 key = 0;

    if (ip_para->ipv4 == true) {
        key = src_dst_ipv4_hash(ip_para);
    } else {
        key = src_dst_ipv6_hash(ip_para);
    }

	hlist_for_each_entry_safe(hnode, hlist, &iwdev->eth_info_hlist[key], list) {
		if (zxdh_eth_info_cmp(ip_para, hnode) == 0) {
			hnode->ip_cfg_ref_cnt += 1;
			pr_debug("[zxdh_rdma] %s[%d]: hlist add ref success, key=%u, ipv4=%d, name=%s, src_ip=0x%x-0x%x-0x%x-0x%x, dst_ip=0x%x-0x%x-0x%x-0x%x, src_mac=0x%llx, dst_mac=0x%llx, ip_cfg_ref_cnt=%d\n",
				__func__, __LINE__, key, ip_para->ipv4, ip_para->ifname, ip_para->src_ip[0], ip_para->src_ip[1], ip_para->src_ip[2], ip_para->src_ip[3],
				ip_para->dst_ip[0], ip_para->dst_ip[1], ip_para->dst_ip[2], ip_para->dst_ip[3], ip_para->src_mac, ip_para->dst_mac, hnode->ip_cfg_ref_cnt);
			goto finish;
		}
	}

    // 分配结点
    hnode = (struct zxdh_eth_info *)kmalloc(sizeof(struct zxdh_eth_info), GFP_ATOMIC); // kfree in zxdh_eth_info_hlist_delete/zxdh_destory_eth_info_hlist
    if (!hnode) {
        pr_err("[zxdh_rdma] %s[%d]: kmalloc fail, name=%s, src_ip=0x%x-0x%x-0x%x-0x%x, dst_ip=0x%x-0x%x-0x%x-0x%x\n", __func__, __LINE__, ip_para->ifname,
			ip_para->src_ip[0], ip_para->src_ip[1], ip_para->src_ip[2], ip_para->src_ip[3],
			ip_para->dst_ip[0], ip_para->dst_ip[1], ip_para->dst_ip[2], ip_para->dst_ip[3]);
        return -1;
    }

    INIT_HLIST_NODE(&hnode->list);
	memcpy(&hnode->rdma_to_eth_ip_para, ip_para, sizeof(struct zxdh_rdma_to_eth_ip_para));
	hnode->netdev = iwdev->netdev;
	hnode->ip_cfg_ref_cnt = 1;
    hlist_add_head(&hnode->list, &iwdev->eth_info_hlist[key]); // 添加到链表首部

	pr_debug("[zxdh_rdma] %s[%d]: hlist add node success, key=%u, ipv4=%d, name=%s, src_ip=0x%x-0x%x-0x%x-0x%x, dst_ip=0x%x-0x%x-0x%x-0x%x, src_mac=0x%llx, dst_mac=0x%llx, ip_cfg_ref_cnt=%d\n",
				__func__, __LINE__, key, ip_para->ipv4, ip_para->ifname, ip_para->src_ip[0], ip_para->src_ip[1], ip_para->src_ip[2], ip_para->src_ip[3],
				ip_para->dst_ip[0], ip_para->dst_ip[1], ip_para->dst_ip[2], ip_para->dst_ip[3], ip_para->src_mac, ip_para->dst_mac, hnode->ip_cfg_ref_cnt);

	rdma_update_remote_ip(ip_para);
	zxdh_eth_info_hlist_display(iwdev);

finish:
    return 0;
}

int zxdh_eth_info_hlist_delete(struct zxdh_device *iwdev, struct zxdh_rdma_to_eth_ip_para *ip_para)
{
    struct zxdh_eth_info *hnode = NULL;
    struct hlist_node *hlist = NULL;
    u32 key;

    if (ip_para->ipv4 == true) {
        key = src_dst_ipv4_hash(ip_para);
    } else {
        key = src_dst_ipv6_hash(ip_para);
    }

    if (hlist_empty(&iwdev->eth_info_hlist[key])) {
        pr_debug("[zxdh_rdma] %s[%d]: hlist key(%d) not exit, ipv4=%d, name=%s, src_ip=0x%x-0x%x-0x%x-0x%x, dst_ip=0x%x-0x%x-0x%x-0x%x, src_mac=0x%llx, dst_mac=0x%llx\n",
			__func__, __LINE__, key, ip_para->ipv4, ip_para->ifname, ip_para->src_ip[0], ip_para->src_ip[1], ip_para->src_ip[2], ip_para->src_ip[3],
			ip_para->dst_ip[0], ip_para->dst_ip[1], ip_para->dst_ip[2], ip_para->dst_ip[3], ip_para->src_mac, ip_para->dst_mac);
        return 0;
    } else {
        // 遍历对应的槽，匹配值就删除
        hlist_for_each_entry_safe(hnode, hlist, &iwdev->eth_info_hlist[key], list) {
            if(zxdh_eth_info_cmp(ip_para, hnode) == 0) {
				hnode->ip_cfg_ref_cnt -= 1;
				if (hnode->ip_cfg_ref_cnt == 0) {
					pr_debug("[zxdh_rdma] %s[%d]: hlist delete node success, key=%u, ipv4=%d, name=%s, src_ip=0x%x-0x%x-0x%x-0x%x, dst_ip=0x%x-0x%x-0x%x-0x%x, src_mac=0x%llx, dst_mac=0x%llx, ip_cfg_ref_cnt=%d\n",
						__func__, __LINE__, key, ip_para->ipv4, ip_para->ifname, ip_para->src_ip[0], ip_para->src_ip[1], ip_para->src_ip[2], ip_para->src_ip[3],
						ip_para->dst_ip[0], ip_para->dst_ip[1], ip_para->dst_ip[2], ip_para->dst_ip[3], ip_para->src_mac, ip_para->dst_mac, hnode->ip_cfg_ref_cnt);
                    hlist_del(&hnode->list);
					kfree(hnode); // kmalloc in zxdh_eth_info_hlist_add
					rdma_update_remote_ip(ip_para);
					zxdh_eth_info_hlist_display(iwdev);
				} else {
					pr_debug("[zxdh_rdma] %s[%d]: hlist delete ref success, key=%u, ipv4=%d, name=%s, src_ip=0x%x-0x%x-0x%x-0x%x, dst_ip=0x%x-0x%x-0x%x-0x%x, src_mac=0x%llx, dst_mac=0x%llx, ip_cfg_ref_cnt=%d\n",
						__func__, __LINE__, key, ip_para->ipv4, hnode->netdev->name, ip_para->src_ip[0], ip_para->src_ip[1], ip_para->src_ip[2], ip_para->src_ip[3],
						ip_para->dst_ip[0], ip_para->dst_ip[1], ip_para->dst_ip[2], ip_para->dst_ip[3], ip_para->src_mac, ip_para->dst_mac, hnode->ip_cfg_ref_cnt);
				}
                return 0;
            }
        }
    }

    pr_err("[zxdh_rdma] %s[%d]: delete data fail, key=%u, ipv4=%d, name=%s, src_ip=0x%x-0x%x-0x%x-0x%x, dst_ip=0x%x-0x%x-0x%x-0x%x, src_mac=0x%llx, dst_mac=0x%llx\n",
		__func__, __LINE__, key, ip_para->ipv4, ip_para->ifname, ip_para->src_ip[0], ip_para->src_ip[1], ip_para->src_ip[2], ip_para->src_ip[3],
		ip_para->dst_ip[0], ip_para->dst_ip[1], ip_para->dst_ip[2], ip_para->dst_ip[3], ip_para->src_mac, ip_para->dst_mac);
    return -1;
}

static void zxdh_get_net_irq_cap(struct zxdh_pci_f *rf)
{
    u32 opcode = 0;

    if (rf->gen_ops.zxdh_common_func) {
        opcode = rf->gen_ops.zxdh_common_func(NULL, NULL, ZXDH_FUNC_NUM_REQUIRE);
        if ((opcode > ZXDH_FUNC_IRQ_FREE) && (opcode != 0xFF))
            rf->net_irq_cap = true;
    }
}

static void zxdh_get_common_func_num_max(struct zxdh_pci_f *rf)
{
	u32 num_max = 0;

	if (rf->gen_ops.zxdh_common_func) {
		num_max = rf->gen_ops.zxdh_common_func(NULL, NULL,
						       ZXDH_FUNC_NUM_REQUIRE);
		if (num_max != 0xFF) {
			rf->common_func_num_max = num_max;
			return;
		}
	}
	rf->common_func_num_max = -1;
}

static void zxdh_get_dh_dev(struct zxdh_pci_f *rf, struct zxdh_auxiliary_dev *iidc_adev)
{
	if (rf->net_irq_cap == true) {
        if (iidc_adev && iidc_adev->zxdh_info)
            rf->dh_dev = iidc_adev->zxdh_info->dh_dev;
	}
}

static void zxdh_fill_device_info(struct zxdh_device *iwdev,
				  struct iidc_core_dev_info *cdev_info)
{
	struct zxdh_pci_f *rf = iwdev->rf;

	rf->ftype = (cdev_info->vport_id >> 11) & 0x1;
	rf->pf_id = (cdev_info->vport_id >> 8) & 0x7;
	rf->sc_dev.ep_id = (cdev_info->vport_id >> 12) & 0x7;
	rf->ep_id = rf->sc_dev.ep_id;
	rf->sc_dev.driver_load = true;

	rf->cdev = cdev_info;
	rf->pcidev = cdev_info->pdev;
	rf->hw.pci_hw_addr = cdev_info->hw_addr;

	rf->msix_count = cdev_info->msix_count;
#ifdef MSIX_DEBUG
	ft_debug_msix_interrupt(cdev_info->pdev, cdev_info->msix_entries,
				rf->msix_count);
#endif
	rf->msix_entries = cdev_info->msix_entries;
	rf->sc_dev.max_ceqs = (rf->msix_count - 1);
	rf->protocol_used = cdev_info->rdma_protocol ==
					    IIDC_RDMA_PROTOCOL_ROCEV2 ?
				    ZXDH_ROCE_PROTOCOL_ONLY :
					  ZXDH_IWARP_PROTOCOL_ONLY;
	rf->rdma_ver = ZXDH_GEN_2;
	rf->rsrc_profile = ZXDH_HMC_PROFILE_DEFAULT;
	rf->rst_to = ZXDH_RST_TIMEOUT_HZ;
	rf->gen_ops.request_reset = zxdh_request_reset;
	rf->check_fc = zxdh_check_fc_for_qp;
	rf->qp_index = 0;
	rf->gen_ops.zxdh_common_func = NULL;
	rf->net_irq_cap = false;
	rf->dh_dev = NULL;
	rf->drv_np_cap = (bool)FIELD_GET(ZXDH_RDMA_COMM_FUNC, cdev_info->ver.support); 
	if (rf->drv_np_cap == ZXDH_RDMA_COMMON_FUNC_CAP) {
		if ((cdev_info->ops != NULL) && (cdev_info->ops->zxdh_common_func != NULL))
            rf->gen_ops.zxdh_common_func = cdev_info->ops->zxdh_common_func;
		else 
			pr_info("[zxdh_rdma] [%s] zxdh_common_func is NULL\n",__func__);
	}

	zxdh_get_net_irq_cap(rf);
	zxdh_get_common_func_num_max(rf);

	/* Can override limits_sel, protocol_used */
	zxdh_set_rf_user_cfg_params(rf);
	rf->iwdev = iwdev;

	INIT_LIST_HEAD(&iwdev->ah_list);
	mutex_init(&iwdev->ah_list_lock);
	iwdev->netdev = cdev_info->netdev;
	iwdev->source_netdev = cdev_info->netdev;
	iwdev->init_state = INITIAL_STATE;
	iwdev->roce_cwnd = ZXDH_ROCE_CWND_DEFAULT;
	iwdev->roce_ackcreds = ZXDH_ROCE_ACKCREDS_DEFAULT;
	iwdev->rcv_wnd = ZXDH_CM_DEFAULT_RCV_WND_SCALED;
	iwdev->rcv_wscale = ZXDH_CM_DEFAULT_RCV_WND_SCALE;
	iwdev->qp1 = NULL;
#if IS_ENABLED(CONFIG_CONFIGFS_FS)
	iwdev->iwarp_ecn_en = true;
	iwdev->iwarp_rtomin = 5;
	iwdev->up_up_map = ZXDH_DEFAULT_UP_UP_MAP;
#endif
	if (rf->protocol_used == ZXDH_ROCE_PROTOCOL_ONLY) {
#if IS_ENABLED(CONFIG_CONFIGFS_FS)
		iwdev->roce_rtomin = 5;
#endif
		//iwdev->roce_dcqcn_en = iwdev->rf->dcqcn_ena;
		iwdev->roce_dcqcn_en = true; //dcqcn/ecn is set to default on
		iwdev->roce_mode = true;
	}
	atomic_set(&iwdev->tx_port_affinity, 0);
	atomic_set(&iwdev->sport_counter, 0);
	INIT_LIST_HEAD(&rf->vf_list);
	spin_lock_init(&rf->vf_list_lock);
	zxdh_init_eth_info_hlist(iwdev);
}

int zxdh_init_dip_info_hlist(struct zxdh_pci_f *rf)
{
    int i, j;

	// (1) 分配8个优先级的哈希表指针
	rf->dip_hlist = (struct hlist_head **)kmalloc(ZXDH_MAX_USER_PRIORITY * sizeof(struct hlist_head *), GFP_ATOMIC);
    if (!rf->dip_hlist) {
        pr_err("[zxdh_rdma] [%s][%d]Failed to allocate dip_hlist pointer array\n", __func__, __LINE__);
        return -ENOMEM;
    }

    // (2) 为每个优先级分配哈希桶表头，哈希桶深为64
    for (i = 0; i < ZXDH_MAX_USER_PRIORITY; i++) {
        rf->dip_hlist[i] = (struct hlist_head *)kmalloc(ZXDH_DIP_HASH_COUNT * sizeof(struct hlist_head), GFP_ATOMIC);
        if (!rf->dip_hlist[i]) {
            pr_err("[zxdh_rdma] [%s][%d]Failed to allocate hash table for priority %d\n", __func__, __LINE__, i);
            goto error;
        }
        
        // (3) 初始化每个哈希桶
        for (j = 0; j < ZXDH_DIP_HASH_COUNT; j++) {
            INIT_HLIST_HEAD(&rf->dip_hlist[i][j]);
        }
    }

	// (4) 初始化rf的dip哈希表和互斥锁
    mutex_init(&rf->dip_hlist_mtx_lock);

    pr_info("[zxdh_rdma] [vhca%u][%s][%d]Successfully allocated %d priority hash tables\n", 
			rf->sc_dev.vhca_id, __func__, __LINE__, ZXDH_MAX_USER_PRIORITY);
	
    return 0;

error:
    // 释放已成功分配的内存
    while (i > 0) {
		i--;
        kfree(rf->dip_hlist[i]);
    }
    kfree(rf->dip_hlist);
    rf->dip_hlist = NULL;

    return -ENOMEM;
}

void zxdh_destroy_dip_info_hlist(struct zxdh_pci_f *rf)
{
    int i, j;
    struct zxdh_dip_info *hnode = NULL;
    struct hlist_node *tmp = NULL;
    
    if (!rf->dip_hlist) {		
        return;
    }
    
    // (1) 清理每个优先级的哈希表
    for (i = 0; i < ZXDH_MAX_USER_PRIORITY; i++) {
        if (!rf->dip_hlist[i]) {
            continue;
        }
        
        // (2) 清理当前哈希表中每一个哈希桶中的节点
        for (j = 0; j < ZXDH_DIP_HASH_COUNT; j++) {
            hlist_for_each_entry_safe(hnode, tmp, &rf->dip_hlist[i][j], list) {
                hlist_del(&hnode->list);
                kfree(hnode);
            }
        }
        
        // (3) 释放当前哈希表
        kfree(rf->dip_hlist[i]);
        rf->dip_hlist[i] = NULL;
    }
    
    // (4) 释放指针数组
    kfree(rf->dip_hlist);
    rf->dip_hlist = NULL;
    
    pr_info("[zxdh_rdma][%s][%d]Cleaned up vhca:%u all priority hash tables\n", 
			 __func__, __LINE__, rf->sc_dev.vhca_id);
}

static u32 dip_hash(u32 *dst_ip, u32 size)
{ 
    u32 key = jhash(dst_ip, sizeof(dst_ip[0]) * size, 0);

    return key % ZXDH_DIP_HASH_COUNT;
}

int zxdh_dip_cmp(u32 dst_ip[4], u32 cmp_ip[4])
{
    if (dst_ip[0] == cmp_ip[0] && 
		dst_ip[1] == cmp_ip[1] && 
		dst_ip[2] == cmp_ip[2] && 
		dst_ip[3] == cmp_ip[3]) {
        return 0;
    }

    return 1;
}

void zxdh_dip_copy(u32 dst_ip[4], u32 src_ip[4])
{
    dst_ip[0] = src_ip[0];
	dst_ip[1] = src_ip[1];
	dst_ip[2] = src_ip[2];
	dst_ip[3] = src_ip[3];
}

// 寻找数组 array 中从 start 开始的 size 个元素的最小值的下标
static u32 find_min_elem_idx(u32 * array, u32 start, u32 size)
{
	u32 i;
	u32 min_idx = 0;

	for (i = 0; i < size; i++) {
		if(array[start + i] < array[start + min_idx]){
			min_idx = i;
		}
	}

	return (start + min_idx);
}

int zxdh_get_8k_index_from_hlist(struct zxdh_pci_f *rf, struct zxdh_sc_qp *qp, u32 dst_ip[4], u16 *out_8k_idx)
{
	u32 key = 0;
	u32 rc_8k_idx_oft = 0;
	struct zxdh_dip_info *hnode = NULL;
	struct hlist_node *hlist = NULL;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	u16 bitmap_8k_idx_start;
	u16 bitmap_8k_idx_cnt;
	u16 bitmap_8k_idx_oft;
	u16 bitmap_8k_idx_end;
	int ret;

	// (1) 计算 ipv4/ipv6 的哈希值，统一基于 u32*4 大小进行哈希
	key = dip_hash(dst_ip, 4);

	// 上锁
	mutex_lock(&rf->dip_hlist_mtx_lock);

	// (2) 查找 qp->pri_index 对应的哈希表中键值为key的哈希桶中是否存在当前ip节点
	hlist_for_each_entry_safe(hnode, hlist, &rf->dip_hlist[qp->pri_index][key], list) {
		if(zxdh_dip_cmp(hnode->dst_ip, dst_ip) == 0){
			// (3) 如果查找到当前ip节点，直接使用里面的 8k_idx
			// (3.1) 更新 rf->qp_cnt_8k_idxs[rc_8k_idx_oft]++
			rc_8k_idx_oft = hnode->rc_8k_idx - dev->vhca_8k_index_start;
			rf->qp_cnt_8k_idxs[rc_8k_idx_oft]++;

			// (3.2) 更新 ip节点的 qp_cnt++
			hnode->qp_cnt++;

			// (3.3) 返回 8k_idx
			*out_8k_idx = hnode->rc_8k_idx;

			mutex_unlock(&rf->dip_hlist_mtx_lock);
			return 0;
		}
	}

	// (4) 如果没有查找到当前ip节点，则遍历当前优先级的8k_idx bitmap区间，寻找首个空闲的8k_idx
	bitmap_8k_idx_start = dev->pri_8k_index_info[qp->pri_index].pri_8k_index_start - dev->vhca_8k_index_start;
	bitmap_8k_idx_cnt = dev->pri_8k_index_info[qp->pri_index].pri_8k_index_cnt;
	bitmap_8k_idx_end = bitmap_8k_idx_start + bitmap_8k_idx_cnt - 1;
	bitmap_8k_idx_oft = rf->pri_8k_idx_oft[qp->pri_index];

	ret = zxdh_alloc_rsrc_in_range_nolock(rf->allocated_8k_idx, rf->max_8k_idx, 
					bitmap_8k_idx_start, bitmap_8k_idx_oft, bitmap_8k_idx_end, &rc_8k_idx_oft);
	if (ret) {
		// (4.1) 如果没有空闲的bit，则遍历 rf->qp_cnt_8k_idxs，找到当前优先级范围内，关联qp数量最少的8k_idx
		rc_8k_idx_oft = find_min_elem_idx(rf->qp_cnt_8k_idxs, bitmap_8k_idx_start, bitmap_8k_idx_cnt);
		// 更新 rf->qp_cnt_8k_idxs[8k_idx]++
		rf->qp_cnt_8k_idxs[rc_8k_idx_oft]++;
	} else {
		// (4.2) 如果有空闲的bit，作为新的 8k_idx
		// 更新 rf->qp_cnt_8k_idxs[8k_idx] = 1
		rf->qp_cnt_8k_idxs[rc_8k_idx_oft] = 1;
	}

	// (5) 动态申请一个哈希节点并初始化：dst_ip、rc_8k_idx、qp_cnt
	hnode = (struct zxdh_dip_info *)kmalloc(sizeof(struct zxdh_dip_info), GFP_ATOMIC);  // kfree in zxdh_del_8k_index_from_hlist
	if(!hnode){
		pr_err("[zxdh_rdma] [%s][%d]Failed to allocate zxdh_dip_info memory\n", __func__, __LINE__);

		mutex_unlock(&rf->dip_hlist_mtx_lock);		
		return -ENOMEM;
	}
	hnode->qp_cnt = 1;
	hnode->rc_8k_idx = rc_8k_idx_oft + dev->vhca_8k_index_start;  // rc_8k_idx_oft 是基于0的偏移量
	zxdh_dip_copy(hnode->dst_ip, dst_ip);
	INIT_HLIST_NODE(&hnode->list);
	
	// (6) 把新的哈希节点插入到当前优先级的哈希表中
	hlist_add_head(&hnode->list, &rf->dip_hlist[qp->pri_index][key]);

	// 返回 8k_idx
	*out_8k_idx = hnode->rc_8k_idx;

	// 解锁
	mutex_unlock(&rf->dip_hlist_mtx_lock);
	return 0;
}

int zxdh_del_8k_index_from_hlist(struct zxdh_pci_f *rf, u32 dst_ip[4], u8 pri_index)
{
	u32 key = 0;
	u8 found = 0;
	struct zxdh_dip_info *hnode = NULL;
	struct hlist_node *hlist = NULL;
	u32 rc_8k_idx_oft = 0;
	struct zxdh_device *iwdev = rf->iwdev;

	// (1) 计算 ipv4/ipv6 的哈希值，统一基于 u32*4 大小进行哈希
	key = dip_hash(dst_ip, 4);

	// 上锁
	mutex_lock(&rf->dip_hlist_mtx_lock);

	// (2) 查找 pri_index 对应的哈希表中键值为key的哈希桶中是否存在当前ip节点
	if (hlist_empty(&rf->dip_hlist[pri_index][key])) {
		pr_info("[zxdh_rdma] [vhca%u][%s][%d] key=%u hlist_empty\n", rf->sc_dev.vhca_id, __func__, __LINE__, key);
		found = 0;
	} else {
		hlist_for_each_entry_safe(hnode, hlist, &rf->dip_hlist[pri_index][key], list) {
			if(zxdh_dip_cmp(hnode->dst_ip, dst_ip) == 0){
				// (3.1) 更新 ip节点的 qp_cnt--
				hnode->qp_cnt--;

				// (3.2) 更新 rf->qp_cnt_8k_idxs[8k_idx]--
				rc_8k_idx_oft = hnode->rc_8k_idx - rf->sc_dev.vhca_8k_index_start;
				rf->qp_cnt_8k_idxs[rc_8k_idx_oft]--;

				if (refcount_read(&iwdev->trace_switch.t_switch)){
					ibdev_notice(&iwdev->ibdev ,"[vhca%u][%s] key=%u hnode->qp_cnt=%u rf->qp_cnt_8k_idxs[%u]=%u\n",
						rf->sc_dev.vhca_id, __func__, key, hnode->qp_cnt, rc_8k_idx_oft, rf->qp_cnt_8k_idxs[rc_8k_idx_oft]);
				}

				// (3.3) 判断 hnode->qp_cnt 是否为0，如果为0，则从哈希表中删除该节点
				if (hnode->qp_cnt == 0) {
					hlist_del(&hnode->list);
					kfree(hnode);  // kmalloc in zxdh_get_8k_index_from_hlist
				}

				// (3.4) 判断 rf->qp_cnt_8k_idxs[rc_8k_idx_oft] 是否为0，如果为0，则清除该 bitmap
				if (rf->qp_cnt_8k_idxs[rc_8k_idx_oft] == 0) {
					zxdh_free_rsrc_nolock(rf->allocated_8k_idx, rc_8k_idx_oft);
				}

				// (3.5) 标记找到哈希节点
				found = 1;
			}
		}
	}

	// 解锁
	mutex_unlock(&rf->dip_hlist_mtx_lock);

	if (!found){
		pr_err("[zxdh_rdma] [%s][vhca%u][%s][%d] key=%u cannot be found in hlist[%u]! \n", 
				rf->iwdev->ibdev.name, rf->sc_dev.vhca_id, __func__, __LINE__, key, pri_index);
		return -1;
	}

	return 0;
}

/*zxdh_auxiliary_dev中的netdev字段上移，此处重新赋�?*/
static void zxdh_to_iidc(struct iidc_core_dev_info *cdev_info,
			 struct zxdh_auxiliary_dev *iidc_adev)
{
	cdev_info->pdev = iidc_adev->zxdh_info->pdev;
	cdev_info->adev = iidc_adev->zxdh_info->adev;
	cdev_info->hw_addr = iidc_adev->zxdh_info->hw_addr;
	cdev_info->cdev_info_id = iidc_adev->zxdh_info->cdev_info_id;
	cdev_info->ver = iidc_adev->zxdh_info->ver;
	cdev_info->auxiliary_priv = iidc_adev->zxdh_info->auxiliary_priv;
	cdev_info->vport_id = iidc_adev->zxdh_info->vport_id;
	cdev_info->slot_id = iidc_adev->zxdh_info->slot_id;
	cdev_info->rdma_protocol = iidc_adev->zxdh_info->rdma_protocol;
	cdev_info->qos_info = iidc_adev->zxdh_info->qos_info;
	cdev_info->msix_entries = &iidc_adev->zxdh_info->msix_entries;
	cdev_info->msix_count = iidc_adev->zxdh_info->msix_count;
	cdev_info->ops = iidc_adev->zxdh_info->ops;
	cdev_info->netdev =
		iidc_adev->rdma_ops->get_rdma_netdev(iidc_adev->parent);
}

static void zxdh_fw_ver_get(struct zxdh_device *iwdev) 
{
	struct zxdh_fw_compat *fw_ver = NULL;
	u64 addr_offset = 0;
	struct iidc_core_dev_info *cdev_info = iwdev->rf->cdev;

	addr_offset = ZXDH_FW_VER_OFFSET + (MODULE_RDMA_ID - 1)*sizeof(struct zxdh_fw_compat);
	fw_ver = (struct zxdh_fw_compat *)((void __iomem*)cdev_info->hw_addr + addr_offset);
	memcpy(&iwdev->fw_ver, fw_ver , sizeof(struct zxdh_fw_compat));

}

static int zxdh_fw_ver_check(struct iidc_core_dev_info *cdev_info) 
{
	struct zxdh_fw_compat *ver = NULL;
	u64 addr_offset = 0;
	u8 fw_minor_fw_ver = FW_MINOR_FW_VER;
	u8 fw_minor_drv_ver = FW_MINOR_DRV_VER;

	addr_offset = ZXDH_FW_VER_OFFSET + (MODULE_RDMA_ID - 2)*sizeof(struct zxdh_fw_compat);
    ver = (struct zxdh_fw_compat *)((void __iomem*)cdev_info->hw_addr + addr_offset);
    if ((MODULE_RDMA_ID-1) == ver->module_id)
    {
	    addr_offset = ZXDH_FW_VER_OFFSET + (MODULE_RDMA_ID - 3)*sizeof(struct zxdh_fw_compat);
	    ver = (struct zxdh_fw_compat *)((void __iomem*)cdev_info->hw_addr + addr_offset);
	    if ((MODULE_RDMA_ID-2) == ver->module_id)
	    {
		    addr_offset = ZXDH_FW_VER_OFFSET + (MODULE_RDMA_ID - 1)*sizeof(struct zxdh_fw_compat);
			ver = (struct zxdh_fw_compat *)((void __iomem*)cdev_info->hw_addr + addr_offset);
			if (MODULE_RDMA_ID == ver->module_id) {
				if (ver->major != FW_MAJOR_VER) {
				    pr_err("[zxdh_rdma] fw major rdma side ver:%u-%u-%u is not match fw side ver:%u-%u-%u\n",
					    FW_MAJOR_VER, FW_MINOR_FW_VER, FW_MINOR_DRV_VER, ver->major, ver->fw_minor, ver->drv_minor);
					return -EINVAL;
				}

				if (fw_minor_fw_ver > ver->fw_minor) {
					pr_err("[zxdh_rdma] fw minor rdma side ver:%u-%u-%u is higher than fw side ver:%u-%u-%u\n",
						FW_MAJOR_VER, FW_MINOR_FW_VER, FW_MINOR_DRV_VER, ver->major, ver->fw_minor, ver->drv_minor);
					return -EINVAL;
				}

				if (fw_minor_drv_ver < ver->drv_minor) {
					pr_err("[zxdh_rdma] fw rdma minor rdma side ver:%u-%u-%u is lower than fw side ver:%u-%u-%u\n",
						FW_MAJOR_VER, FW_MINOR_FW_VER, FW_MINOR_DRV_VER, ver->major, ver->fw_minor, ver->drv_minor);
					return -EINVAL;
				}
				pr_info("[zxdh_rdma] [%s] fw ver:%u-%u-%u-%u match success!\n", __FUNCTION__, ver->major, ver->fw_minor, ver->drv_minor, ver->patch);
			}
		}
    }
	return 0;
}

static int zxdh_drv_ver_check(struct iidc_core_dev_info *cdev_info) 
{
	u8 net_major = 0;
	u8 net_minor = 0;
	u8 rdma_minor = 0;
	u8 drv_net_minor_ver = DRV_NET_MINOR_VER;
	u8 drv_rdma_minor_ver = DRV_RDMA_MINOR_VER;

	net_major = (u8)FIELD_GET(ZXDH_NET_MAJOR_IDX, cdev_info->ver.support); 
	net_minor = (u8)FIELD_GET(ZXDH_NET_MINOR_IDX, cdev_info->ver.support); 
	rdma_minor = (u8)FIELD_GET(ZXDH_RDMA_MINOR_IDX, cdev_info->ver.support); 

	if (net_major != DRV_MAJOR_VER) {
	    pr_err("[zxdh_rdma] drv major rdma side ver:%u-%u-%u is not match net side ver:%u-%u-%u\n",
	        DRV_MAJOR_VER, DRV_NET_MINOR_VER, DRV_RDMA_MINOR_VER, net_major, net_minor, rdma_minor);
	    return -EINVAL;
	}

	if (drv_net_minor_ver > net_minor) {
	    pr_err("[zxdh_rdma] drv net minor rdma side ver:%u-%u-%u is higher than net side ver:%u-%u-%u\n",
	        DRV_MAJOR_VER, DRV_NET_MINOR_VER, DRV_RDMA_MINOR_VER, net_major, net_minor, rdma_minor);
	    return -EINVAL;
	}

	if (drv_rdma_minor_ver < rdma_minor) {
	    pr_err("[zxdh_rdma] drv rdma minor rdma side ver:%u-%u-%u is lower than net side ver:%u-%u-%u\n",
	        DRV_MAJOR_VER, DRV_NET_MINOR_VER, DRV_RDMA_MINOR_VER, net_major, net_minor, rdma_minor);
	    return -EINVAL;
	}
	return 0;
}

static int zxdh_compat_ver_check(struct iidc_core_dev_info *cdev_info) 
{
	if (zxdh_fw_ver_check(cdev_info))
	  return -EINVAL; 

	if (zxdh_drv_ver_check(cdev_info))
	  return -EINVAL; 
	return 0;
}

static void zxdh_rdma_init_sriov(struct zxdh_pci_f *rf)
{
	struct zxdh_rdma_sriov_event_info sriov_info;
	u64 vf_pblem_cnt;
	int active_vf_num = pci_num_vf(rf->pcidev);
	struct iidc_core_dev_info *cdev_info = rf->cdev;
	int ret = 0;

	if (active_vf_num <= 0)
	{
		return;
	}

	sriov_info.pdev = cdev_info->pdev;
	sriov_info.bar0_virt_addr = (u64)cdev_info->hw_addr;
	sriov_info.vport_id = cdev_info->vport_id;
	sriov_info.num_vfs = active_vf_num;

	if (set_rdma_vf_num(&sriov_info, &vf_pblem_cnt)) {
		pr_err("[zxdh_rdma] %s set_rdma_vf_num failed, ret=%d\n", __func__, ret);
		return;
	}

	rf->sc_dev.active_vfs_num = active_vf_num;
	pr_info("[zxdh_rdma] %s active_vf_num:%d vf_pblem_cnt:0x%llx\n", __func__, active_vf_num, vf_pblem_cnt);

	return;
}

static int zxdh_clear_chip_srq_mem(struct zxdh_sc_dev *dev, u64 chip_srq_base_paddr, u64 size)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status;

	if (!dev) {
		pr_info("[zxdh_rdma] %s[%d]: dev is NULL\n", __func__, __LINE__);
		return -ENOMEM;
	}

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request) {
		pr_info("[zxdh_rdma] %s[%d]: get cqp request failed\n", __func__, __LINE__);
		return -ENOMEM;
	}

	dev->nof_clear_dpu_mem.size = size;
	dev->nof_clear_dpu_mem.va =
		dma_alloc_coherent(dev->hw->device, dev->nof_clear_dpu_mem.size,
				   &dev->nof_clear_dpu_mem.pa, GFP_KERNEL);
	if (!dev->nof_clear_dpu_mem.va) {
		pr_info("[zxdh_rdma] %s[%d]: dma alloc failed\n", __func__, __LINE__);
		return -ENOMEM;
	}
	zrdma_memset_s(dev->nof_clear_dpu_mem.va, 0, dev->nof_clear_dpu_mem.size);

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = ZXDH_OP_DMA_WRITE;
	cqp_info->in.u.dma_writeread.cqp = dev->cqp;
	cqp_info->in.u.dma_writeread.src_dest.src = dev->nof_clear_dpu_mem.pa;
	cqp_info->in.u.dma_writeread.src_dest.len = dev->nof_clear_dpu_mem.size;
	cqp_info->in.u.dma_writeread.src_dest.dest = chip_srq_base_paddr;

	cqp_info->in.u.dma_writeread.src_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.src_path_index.obj_id = ZXDH_DMA_OBJ_ID;
	cqp_info->in.u.dma_writeread.src_path_index.path_select = ZXDH_INDICATE_HOST_NOSMMU;
	cqp_info->in.u.dma_writeread.src_path_index.inter_select = ZXDH_INTERFACE_NOTCACHE;

	cqp_info->in.u.dma_writeread.dest_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.dest_path_index.obj_id = ZXDH_DMA_OBJ_ID;
	cqp_info->in.u.dma_writeread.dest_path_index.path_select = ZXDH_INDICATE_DPU_DDR;
	cqp_info->in.u.dma_writeread.dest_path_index.inter_select =	ZXDH_INTERFACE_NOTCACHE;
	pr_info("[zxdh_rdma] %s[%d]: clear chip srq pa=0x%llx size=0x%x\n", __func__, __LINE__, chip_srq_base_paddr, dev->nof_clear_dpu_mem.size);
	cqp_info->in.u.dma_writeread.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);

	if (dev->nof_clear_dpu_mem.va) {
		dma_free_coherent(dev->hw->device, dev->nof_clear_dpu_mem.size, dev->nof_clear_dpu_mem.va, dev->nof_clear_dpu_mem.pa);
		dev->nof_clear_dpu_mem.va = NULL;
	}

	return status;
}

static int zxdh_get_srq_mem_info(struct zxdh_pci_f *rf, struct dh_get_srq_mem_info_resp *srq_mem_info_resp)
{
	int ret = 0;
    u32 function_id = 0;
	u8 rep_valid = 0;
	u16 rep_len = 0;
	u8 *rep_ptr;
	struct zxdh_mgr mgr = { 0 };
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct dh_get_srq_mem_info_req get_cmd = { 0 };
	struct iidc_core_dev_info *cdev_info;
	size_t recv_len;
	void *recv_buffer;
	struct dh_get_srq_mem_info_resp *get_resp;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	if (!rf) {
        pr_err("[zxdh_rdma] %s[%d]: rf is null\n", __func__, __LINE__);
		return -EINVAL;
    }
	if (rf->sc_dev.driver_load == false)
		cnt_num = ZXDH_BAR_MSG_DEFAULT_NUM;

	cdev_info = (struct iidc_core_dev_info *)rf->cdev;
	// query pcie id
	mgr.pdev = cdev_info->pdev;
	ret = dh_rdma_pf_pcie_id_get(&mgr);
	if (ret) {
		pr_err("[zxdh_rdma] [%s] get pf pcie_id failed, ret=%d\n", __func__, ret);
		return -EINVAL;
	}
	rf->vf_id = mgr.pcie_id & 0xff;
	function_id = DH_FUNC_ID_GEN(rf->ftype, rf->ep_id, 0, rf->pf_id, rf->vf_id);
	recv_len = ZXDH_CHAN_REPS_LEN + sizeof(struct dh_get_srq_mem_info_resp);
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer) {
        pr_err("[zxdh_rdma] %s[%d]: kzalloc failed!\n", __func__, __LINE__);
		return -ENOMEM;
    }

	// commnad preparation
	get_cmd.op_code = GET_SRQ_L2D_ADDR;
	get_cmd.function_id = function_id;
    pr_info("[zxdh_rdma] %s[%d]: function_id=0x%x ftype=%d ep_id=%d pf_id=%d vf_id=%d\n", __func__, __LINE__,
        function_id, rf->ftype, rf->ep_id, rf->pf_id, rf->vf_id);

	// get message preparation
	in.payload_addr = (void *)&get_cmd;
	in.payload_len = sizeof(struct dh_get_srq_mem_info_req);
	in.src = rf->ftype == 0 ? MSG_CHAN_END_PF : MSG_CHAN_END_VF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = (u64)cdev_info->hw_addr + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	}while(cnt < cnt_num);

	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d cnt=%d\n", __func__, ret, cnt);
        ret = -EPROTO;
        goto finish;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n", __func__, rep_valid);
		ret = -EPROTO;
        goto finish;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != (recv_len - ZXDH_CHAN_REPS_LEN)) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x recv_len=0x%zx\n", __func__, rep_len, recv_len);
		ret = -EPROTO;
        goto finish;
	}

	get_resp = (struct dh_get_srq_mem_info_resp *)(rep_ptr + ZXDH_CHAN_REPS_LEN);
	if (get_resp->status_code != BAR_MSG_STATUS_OK) {
		pr_err("[zxdh_rdma] [%s] response status invalid, statuc_code=0x%x\n", __func__, get_resp->status_code);
		ret = -EPROTO;
        goto finish;
	}

    zrdma_memcpy_s(srq_mem_info_resp, get_resp, sizeof(struct dh_get_srq_mem_info_resp));
	pr_info("[zxdh_rdma] %s[%d]: resp srq_mem_paddr=0x%llx srq_mem_size=0x%x rdma_ext_bar_offset=0x%x\n", __func__, __LINE__, get_resp->srq_mem_paddr, get_resp->srq_mem_size, get_resp->rdma_ext_bar_offset);

finish:
	// *outdata = get_resp->val;
	kfree(recv_buffer);
    recv_buffer = NULL;
	return ret;
}

static void zxdh_set_srq_mem_info(struct zxdh_pci_f *rf)
{
    struct dh_get_srq_mem_info_resp srq_mem_info_resp = {0};
    int ret = 0;

    if (!rf) {
        pr_err("[zxdh_rdma] %s[%d] error: rf is null\n", __func__, __LINE__);
        return;
    }

    /*if (rf->ftype == FUNCTION_TYPE_VF) {
        rf->chip_srq_base_paddr = 0;
        rf->srq_mem_size = 0;
        rf->rdma_ext_bar_offset = 0;
        pr_info("[zxdh_rdma] %s[%d]: vf not support srq\n", __func__, __LINE__);
        return;
    }*/

    ret = zxdh_get_srq_mem_info(rf, &srq_mem_info_resp);
    if (ret) {
        rf->chip_srq_base_paddr = 0;
        rf->srq_mem_size = 0;
        rf->rdma_ext_bar_offset = 0;
        pr_warn("[zxdh_rdma] %s: get srq mem failed, use ddr! ret=%d srq_mem_paddr=0x%llx srq_mem_size=0x%x rdma_ext_bar_offset=0x%x status_code=%d\n", __func__,
            ret, srq_mem_info_resp.srq_mem_paddr, srq_mem_info_resp.srq_mem_size, srq_mem_info_resp.rdma_ext_bar_offset, srq_mem_info_resp.status_code);
    } else {
        rf->chip_srq_base_paddr = srq_mem_info_resp.srq_mem_paddr;
        rf->srq_mem_size = srq_mem_info_resp.srq_mem_size;
        rf->rdma_ext_bar_offset = srq_mem_info_resp.rdma_ext_bar_offset;
        pr_debug("[zxdh_rdma] %s: get srq mem success! srq_mem_paddr=0x%llx srq_mem_size=0x%x rdma_ext_bar_offset=0x%x status_code=%d\n", __func__,
            srq_mem_info_resp.srq_mem_paddr, srq_mem_info_resp.srq_mem_size, srq_mem_info_resp.rdma_ext_bar_offset, srq_mem_info_resp.status_code);
    }  
}

int zxdh_req_config_rdma_switch(struct zxdh_device *iwdev, enum switch_name_e switch_type, uint32_t value)
{
	int ret = 0;
	struct zxdh_pci_f *rf = iwdev->rf;
	uint8_t rep_valid = 0;
	uint16_t rep_len = 0;
	uint8_t *rep_ptr;
	struct zxdh_mgr mgr = { 0 };
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct iidc_core_dev_info *cdev_info;
	struct dh_rdma_config_switch_req req_msg = { 0 };
	struct dh_rdma_config_switch_resp *resp_msg;
	size_t recv_len;
	void *recv_buffer;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	if (!rf)
		return -ENOMEM;
	if (rf->sc_dev.driver_load == false)
		cnt_num = ZXDH_BAR_MSG_DEFAULT_NUM;
	
	cdev_info = (struct iidc_core_dev_info *)rf->cdev;

	// query pcie id
	mgr.pdev = cdev_info->pdev;
	ret = dh_rdma_pf_pcie_id_get(&mgr);
	if (ret) {
		pr_err("[zxdh_rdma] [%s] get pf pcie_id failed, ret=%d\n", __func__, ret);
		return -EINVAL;
	}

	recv_len = ZXDH_CHAN_REPS_LEN + sizeof(struct dh_rdma_config_switch_resp);
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer)
		return -ENOMEM;

	// commnad preparation
	req_msg.opcode = RDMA_SWITCH_CONFIG;
	req_msg.type = switch_type;
	req_msg.value = value;

	// get message preparation
	in.payload_addr = (void *)&req_msg;
	in.payload_len = sizeof(struct dh_rdma_config_switch_req);
	in.src = rf->ftype == 0 ? MSG_CHAN_END_PF : MSG_CHAN_END_VF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = (u64)cdev_info->hw_addr + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	}while(cnt < cnt_num);
	
	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d cnt=%d\n", __func__, ret, cnt);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n",
		       __func__, rep_valid);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != recv_len - ZXDH_CHAN_REPS_LEN) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x\n", __func__,
		       rep_len);
		kfree(recv_buffer);
		return -ERANGE;
	}

	resp_msg = (struct dh_rdma_config_switch_resp *)(rep_ptr + ZXDH_CHAN_REPS_LEN);
	ret = resp_msg->status_code;
	if (ret != STATUS_OK) {
		pr_err("[zxdh_rdma] [%s] config rdma switch failed, err code=%u\n", __func__, resp_msg->status_code);
	}

	kfree(recv_buffer);

	return ret;
}

int zxdh_req_query_rdma_switch(struct zxdh_device *iwdev, enum switch_name_e switch_type, uint32_t *value)
{
	int ret = 0;
	struct zxdh_pci_f *rf = iwdev->rf;
	uint8_t rep_valid = 0;
	uint16_t rep_len = 0;
	uint8_t *rep_ptr;
	struct zxdh_mgr mgr = { 0 };
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct iidc_core_dev_info *cdev_info;
	struct dh_rdma_query_switch_req req_msg = { 0 };
	struct dh_rdma_query_switch_resp *resp_msg;
	size_t recv_len;
	void *recv_buffer;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	if (!rf)
		return -ENOMEM;
	if (rf->sc_dev.driver_load == false)
		cnt_num = ZXDH_BAR_MSG_DEFAULT_NUM;

	cdev_info = (struct iidc_core_dev_info *)rf->cdev;

	// query pcie id
	mgr.pdev = cdev_info->pdev;
	ret = dh_rdma_pf_pcie_id_get(&mgr);
	if (ret) {
		pr_err("[zxdh_rdma] [%s] get pf pcie_id failed, ret=%d\n", __func__, ret);
		return -EINVAL;
	}

	recv_len = ZXDH_CHAN_REPS_LEN + sizeof(struct dh_rdma_query_switch_resp);
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer)
		return -ENOMEM;

	// commnad preparation
	req_msg.opcode = RDMA_SWITCH_QUERY;
	req_msg.type = switch_type;

	// get message preparation
	in.payload_addr = (void *)&req_msg;
	in.payload_len = sizeof(struct dh_rdma_query_switch_req);
	in.src = rf->ftype == 0 ? MSG_CHAN_END_PF : MSG_CHAN_END_VF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = (u64)cdev_info->hw_addr + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	} while(cnt < cnt_num);

	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d cnt=%d\n", __func__, ret, cnt);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n",
		       __func__, rep_valid);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != recv_len - ZXDH_CHAN_REPS_LEN) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x\n", __func__,
		       rep_len);
		kfree(recv_buffer);
		return -ERANGE;
	}

	resp_msg = (struct dh_rdma_query_switch_resp *)(rep_ptr + ZXDH_CHAN_REPS_LEN);
	if (STATUS_OK == resp_msg->status_code) {
		*value = resp_msg->value;
	} else {
		*value = 0;
		ret = resp_msg->status_code;
		pr_err("[zxdh_rdma] [%s] query rdma switch %d failed, err code=%u\n", __func__, switch_type, resp_msg->status_code);
	}

	kfree(recv_buffer);

	return ret;
}

bool zxdh_use_ext_mem(struct zxdh_pci_f *rf)
{
	u32 board_type = 0;

	board_type = readl(rf->hw.pci_hw_addr + 0x1000);
	if (((board_type >> 8) & 0xFF) == ZXNIC_USE_EXT_MEM)
		return true;
	else
		return false;
}

/**
 * zxdh_check_and_load_evas - Check and load evas.ko module for ext_mem mode
 * @rf: RDMA PCI function structure
 *
 * In ext_mem mode, the driver depends on evas.ko module. This function checks
 * if evas.ko is loaded, and attempts to load it if not present. It waits up to
 * 5 seconds for the module to become available.
 *
 * Return: 0 on success (evas.ko is loaded), -errno on failure
 */
static int zxdh_check_and_load_evas(struct zxdh_pci_f *rf)
{
	int ret = 0;
	int wait_count = 0;
	const int max_wait_seconds = 5;
	const int check_interval_ms = 500;  /* Check every 500ms */
	const int max_checks = (max_wait_seconds * 1000) / check_interval_ms;

	/* First check if evas module is already loaded */
	ret = request_module("evas");
	if (ret == 0) {
		return 0;
	}

	/* Wait for evas.ko to be loaded (may be loaded by another process) */
	for (wait_count = 0; wait_count < max_checks; wait_count++) {
		struct path path;
		if (kern_path("/sys/module/evas", LOOKUP_FOLLOW, &path) == 0) {
			path_put(&path);
			pr_info("[zxdh_rdma] evas.ko detected after %d seconds\n",
					(wait_count * check_interval_ms) / 1000);
			return 0;
		}

		/* Wait before next check */
		msleep(check_interval_ms);
	}

	pr_err("[zxdh_rdma] evas.ko load failed\n");
	return -ENODEV;
}

/**
 * configure_initial_port_speed - Configure initial RDMA port speed based on current netdev speed
 * @iwdev: Pointer to the RDMA device structure
 *
 * Return: 0 on success, -errno on failure
 *
 * This function is called during device probe to configure the initial RDMA
 * port speed tokens based on the current network device speed.
 */
static int configure_initial_port_speed(struct zxdh_device *iwdev)
{
	u32 initial_speed;
	int err;

	if (!iwdev->netdev || iwdev->rf->ftype)
		return 0;

	/* Get current netdev speed */
	initial_speed = zxdh_get_eth_netdev_speed(iwdev, 1);
	if (initial_speed == (u32)SPEED_UNKNOWN || initial_speed <= SPEED_1000) {
		pr_info("[zxdh_rdma] %s: Netdev speed %u Mbps is not effctive, skipping initial configuration\n",
			__func__, initial_speed);
		return 0;
	}

	pr_info("[zxdh_rdma] %s: Initial speed: netdev=%s, speed=%u Mbps\n",
		__func__, iwdev->netdev->name, initial_speed);

	/* Configure RDMA port speed tokens */
	err = set_rdma_port_speed(iwdev->netdev, initial_speed);
	if (err) {
		pr_warn("[zxdh_rdma] %s: Failed to configure initial port speed tokens\n",
			__func__);
		/* Don't fail probe, speed can be configured later via event */
		return err;
	}

	return 0;
}

int zxdh_ext_mem_info_get(struct zxdh_pci_f *rf)
{
	int err;
	u32 i;
    u32 chunk_size;
    u32 current_chunk;
	static u8 zero_buffer[64 * 1024] __aligned(8) = {0};
	u64 sw_qpc_size;
	struct zxdh_mapped_region *mapped_hmc;
	struct zxdh_mapped_region *mapped_sw_qpc;

	//ext_mem信息的获取，优先从BAR读取配置
	err = zxdh_read_ext_mem_info(rf,READ_FROM_BAR);
	if (err) {
		pr_info("[zxdh_rdma] Failed to read config from BAR: %d\n", err);
		// bar空间读不到再从文件中获取
		err = zxdh_read_ext_mem_info(rf,READ_FROM_FILE);
		if (err) {
			pr_info("[zxdh_rdma] No valid config file found\n");
			return -EINVAL;
		}	
	}
	else {
		// 从bar空间获取信息成功，将配置写入文件
		err = zxdh_write_ext_mem_info(rf);
		if (err)
			pr_info("[zxdh_rdma] Failed to write config to file: %d\n", err);
	}

	rf->mem_info.total_size = rf->config.size;
	rf->mem_info.base_pa = rf->config.ext_host;  //ext_pa映射到host上的物理地址(通过bar映射)
	rf->mem_info.base_va = NULL;                 // 按需映射时，base_va 为 NULL
	rf->mem_info.offset = 0;  
	pr_info("[zxdh_rdma] ext_mem base_va=0x%llx ext_mem base_pa=0x%llx ext_mem total_size=0x%llx\n",(u64)rf->mem_info.base_va, rf->mem_info.base_pa, rf->mem_info.total_size);

	// 初始化按需映射的链表
	INIT_LIST_HEAD(&rf->mem_info.mapped_list);

	//ext_mem中第一部分给HMC空间使用
	rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_QP].base = rf->config.ext_pa;
	rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_CQ].base = rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_QP].base + rf->max_qp * HMC_QPC_SIZE;
	rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_TXWINDOW].base = rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_CQ].base + rf->max_cq * HMC_CQC_SIZE;;
	rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_IRD].base = rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_TXWINDOW].base + rf->max_qp * HMC_TXWINDOW_SIZE;
	rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_AH].base = rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_IRD].base + rf->max_qp * HMC_IRD_SIZE;
	rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_MR].base = rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_AH].base + rf->max_ah * HMC_AHC_SIZE;
	
	pr_info("[zxdh_rdma] qpc_base=0x%llx\n",rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_QP].base);
	pr_info("[zxdh_rdma] cpc_base=0x%llx\n",rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_CQ].base);
	pr_info("[zxdh_rdma] txwindow_base=0x%llx\n",rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_TXWINDOW].base);
	pr_info("[zxdh_rdma] ird_base=0x%llx\n",rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_IRD].base);
	pr_info("[zxdh_rdma] ah_base=0x%llx\n",rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_AH].base);
	pr_info("[zxdh_rdma] mr_base=0x%llx\n",rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_MR].base);

	rf->mem_info.offset = rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_MR].base - rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_QP].base + rf->max_mr * HMC_MRTE_SIZE;
	rf->mem_info.offset = ALIGN(rf->mem_info.offset, ZXDH_HW_PAGE_SIZE);
	pr_info("[zxdh_rdma] ext_mem offset(HMC use ext mem size)=0x%llx\n",rf->mem_info.offset);

	rf->mem_info.base_va = (void *)ioremap(rf->mem_info.base_pa, rf->mem_info.offset);
	if (!rf->mem_info.base_va)
	{
		pr_err("[zxdh_rdma] Failed ioremap ext mem for HMC!\n");
		return -EINVAL;
	}

	chunk_size = sizeof(zero_buffer);
	for (i = 0; i < rf->mem_info.offset; i += chunk_size) {
		current_chunk = (rf->mem_info.offset - i < chunk_size) ? (rf->mem_info.offset - i) : chunk_size;
		memcpy_toio(rf->mem_info.base_va + i, zero_buffer, current_chunk);
	}

	// 记录映射信息（用于后续清理，避免内存泄漏）
    mapped_hmc = kmalloc(sizeof(*mapped_hmc), GFP_KERNEL);
    if (!mapped_hmc) {
        pr_err("[zxdh_rdma] Failed to allocate mapped_region\n");
		iounmap(rf->mem_info.base_va);
        return -EINVAL;
    }
	mapped_hmc->va = rf->mem_info.base_va;
    mapped_hmc->host_pa = rf->mem_info.base_pa;
    mapped_hmc->size = rf->mem_info.offset;
    INIT_LIST_HEAD(&mapped_hmc->list);
    list_add_tail(&mapped_hmc->list, &rf->mem_info.mapped_list);

	//ext_mem中第二部分给软件缓存的QPC使用
	sw_qpc_size = rf->max_qp * ALIGN(ZXDH_QP_CTX_SIZE, ZXDH_QPC_ALIGNMENT);
	sw_qpc_size = ALIGN(sw_qpc_size, ZXDH_HW_PAGE_SIZE);
	rf->mem_info.sw_qpc_pa = rf->config.ext_pa + rf->mem_info.offset;
	rf->mem_info.sw_qpc = (void *)ioremap((rf->mem_info.base_pa + rf->mem_info.offset), sw_qpc_size);
	if (!rf->mem_info.sw_qpc)
	{
		pr_err("[zxdh_rdma] Failed ioremap ext mem for sw_qpc!\n");
		zxdh_free_ext_mem(rf,rf->mem_info.base_va,rf->mem_info.offset);
		return -EINVAL;
	}

	for (i = 0; i < sw_qpc_size; i += chunk_size) {
		current_chunk = (sw_qpc_size - i < chunk_size) ? (sw_qpc_size - i) : chunk_size;
		memcpy_toio(rf->mem_info.sw_qpc + i, zero_buffer, current_chunk);
	}

	// 记录映射信息（用于后续清理，避免内存泄漏）
    mapped_sw_qpc = kmalloc(sizeof(*mapped_sw_qpc), GFP_KERNEL);
    if (!mapped_sw_qpc) {
        pr_err("[zxdh_rdma] Failed to allocate mapped_region\n");
		zxdh_free_ext_mem(rf,rf->mem_info.base_va,rf->mem_info.offset);
		iounmap(rf->mem_info.sw_qpc);
        return -EINVAL;
    }
	mapped_sw_qpc->va = rf->mem_info.sw_qpc;
    mapped_sw_qpc->host_pa = rf->mem_info.base_pa + rf->mem_info.offset;
    mapped_sw_qpc->size = sw_qpc_size;
    INIT_LIST_HEAD(&mapped_sw_qpc->list);
    list_add_tail(&mapped_sw_qpc->list, &rf->mem_info.mapped_list);

	rf->mem_info.offset +=  sw_qpc_size;

	/*
	 * 在 ext_mem 的最后 16MB 区域创建内存池管理系统
	 * 提供高效的内存分配和释放机制，支持内存重用
	 */
	err = zxdh_init_ext_mem_pool(rf);
	if (err) {
		pr_err("[zxdh_rdma] Failed to initialize memory pool: %d\n", err);
		
		zxdh_free_ext_mem(rf, rf->mem_info.base_va, rf->mem_info.offset);
		return err;
	}

	return 0;
}

static int zxdh_probe(struct zxdh_auxiliary_device *aux_dev,
		      const struct zxdh_auxiliary_device_id *id)
{
	struct zxdh_auxiliary_dev *iidc_adev =
		container_of(aux_dev, struct zxdh_auxiliary_dev, adev);
	struct zxdh_device *iwdev;
	struct zxdh_pci_f *rf;
	int err;
	struct zxdh_handler *hdl;
	struct iidc_core_dev_info *cdev_info =
		kzalloc(sizeof(struct iidc_core_dev_info), GFP_KERNEL);
	struct  zxdh_rdma_ets_status ets_data = { 0 };

	if (!cdev_info)
		return -ENOMEM;
	zxdh_to_iidc(cdev_info, iidc_adev);
	if (cdev_info->netdev == NULL) {
		pr_err("[zxdh_rdma] %s[%d]: netdev is NULL\n", __func__, __LINE__);
		err = -ENODEV;
		goto err_cdev_info;		
	}
	if (cdev_info->netdev->dev_addr == NULL) {
		pr_err("[zxdh_rdma] %s[%d]: netdev dev_addr is NULL\n", __func__, __LINE__);
		err = -ENODEV;
		goto err_cdev_info;
	}

	if (zxdh_compat_ver_check(cdev_info)) {
	    err = -EINVAL;
		goto err_cdev_info;
	}

	if (cdev_info->ver.major != IIDC_MAJOR_VER) {
		pr_err("[zxdh_rdma] version mismatch:\n");
		pr_err("[zxdh_rdma] expected major ver %d, caller specified major ver %d\n",
		       IIDC_MAJOR_VER, cdev_info->ver.major);
		pr_err("[zxdh_rdma] expected minor ver %d, caller specified minor ver %d\n",
		       IIDC_MINOR_VER, cdev_info->ver.minor);
		err = -EINVAL;
		goto err_cdev_info;
	}
	if (cdev_info->ver.minor != IIDC_MINOR_VER)
		pr_info("[zxdh_rdma] probe: minor version mismatch: expected %0d.%0d caller specified %0d.%0d\n",
			IIDC_MAJOR_VER, IIDC_MINOR_VER, cdev_info->ver.major,
			cdev_info->ver.minor);

	iwdev = ib_alloc_device(zxdh_device, ibdev);
	if (!iwdev) {
		pr_err("[zxdh_rdma] %s:%d alloc ib device failed!\n", __func__, __LINE__);
		err = -ENOMEM;
		goto err_cdev_info;
	}
	iwdev->iidc_adev = iidc_adev;
	iwdev->rf = kzalloc(sizeof(*rf), GFP_KERNEL);
	if (!iwdev->rf) {
		pr_err("[zxdh_rdma] %s:%d alloc rf mem failed! size=%zd refcount=%d\n", __func__, __LINE__, sizeof(*rf), refcount_read(&iwdev->ibdev.refcount));
		kfree(iwdev);
        err = -ENOMEM;
		goto err_cdev_info;
	}
	spin_lock_init(&iwdev->trace_switch.lock);
	zxdh_fill_device_info(iwdev, cdev_info);
	zxdh_get_dh_dev(iwdev->rf, iidc_adev);
	zxdh_req_cmd_ver(iwdev->rf);
	zxdh_vm_env_check(iwdev->rf);

	iwdev->rf->use_ext_mem_flag = false;
	if (zxdh_use_ext_mem(iwdev->rf)) {
		iwdev->rf->use_ext_mem_flag = true;
	}

	if (iwdev->rf->use_ext_mem_flag == false) {
		zxdh_set_srq_mem_info(iwdev->rf);
	}

	zxdh_fw_ver_get(iwdev);
	if (zxdh_req_query_rdma_switch(iwdev, WQE_RATE_LIMIT, &iwdev->wqe_rate_limit)) {
		iwdev->wqe_rate_limit = 0;
	}

	if (iwdev->rf->ftype == 0) {		
		err = iwdev->rf->gen_ops.zxdh_common_func(iwdev->iidc_adev->parent,
						  &iwdev->trust_type,
						  ZXDH_COMM_FUNC_GET_TRUST_TYPE);
		if (err == 0) {
			zxdh_set_trust_type_to_fw(iwdev);
		}
	} else {
		zxdh_get_trust_type_from_fw(iwdev);
	}
	pr_info("[zxdh_rdma] pf=%u, vf=%u, is_vf=%u change trust to %u.\n", 
		iwdev->rf->pf_id, iwdev->rf->vf_id, iwdev->rf->ftype, iwdev->trust_type);

	if (zxdh_req_query_rdma_switch(iwdev, RQ_CREDIT, &iwdev->rq_credit)) {
		iwdev->rq_credit = true;
	}
	pr_info("[zxdh_rdma] %s[%d]:rq_credit is %d\n", __func__, __LINE__, iwdev->rq_credit);

    if (zxdh_req_query_rdma_switch(iwdev, FLOWLABEL_ENABLE, &iwdev->flowlabel_enable)) {
		iwdev->flowlabel_enable = true;
	}
	if (!iwdev->rf->ftype)
	{
		zxdh_rdma_init_sriov(iwdev->rf);
	}

	if(iwdev->rf->use_ext_mem_flag == true)
	{
		//仅初始化pf0主口
		if(iwdev->rf->pf_id != 0)
		{
			kfree(iwdev->rf);
			kfree(iwdev);
			kfree(cdev_info);
			return 0;
		}

		/* Check and load evas.ko if ext_mem mode is detected */
		err = zxdh_check_and_load_evas(iwdev->rf);
		if (err != 0) {
			pr_err("[zxdh_rdma] Failed to load evas.ko, aborting driver initialization\n");
			kfree(iwdev->rf);
			kfree(iwdev);
			kfree(cdev_info);
			return err;
		}
	}

	err = zxdh_manager_init(iwdev->rf, cdev_info);
	if (err != 0) {
		if (err != (-ENODEV)) {
			pr_warn("[zxdh_rdma] zxdh_manager_init failed! err=%d\n", err);
		}
		goto err_mgr_init;
	}

	//保证每个RDMA设备有独立的文件，避免冲突
	set_hbm_config_path(iwdev->rf);

	if (iwdev->rf->use_ext_mem_flag)
	{
		err = zxdh_ext_mem_info_get(iwdev->rf);
		if (err != 0) {
			pr_warn("[zxdh_rdma] zxdh_ext_mem_info_get failed!\n");
			goto err_ext_mem_init;
		}
	}

	// 锁和链表初始化
	zxdh_init_mac_list(iwdev);

	if (!iwdev->rf->ftype) {
		zxdh_cfg_dpp(iwdev, cdev_info);

		ets_data.mode = 0;
		zxdh_rdma_ets_mode_switch_to_off(iwdev->netdev, &ets_data);
	}

	rf = iwdev->rf;

	hdl = kzalloc(sizeof(*hdl), GFP_KERNEL);
	if (!hdl) {
		zxdh_cleanup_dpp_mac_tbl(iwdev);
		err = -ENOMEM;
		goto err_iwdev_rf;
	}

	hdl->iwdev = iwdev;
	iwdev->hdl = hdl;
	iwdev->netdev_speed = (u32)SPEED_UNKNOWN;

	err = zxdh_ctrl_init_hw(rf);
	if (err) {
		pr_err("[zxdh_rdma] %s[%d]: ctrl init hw failed! err=%d\n", __func__, __LINE__, err);
		goto err_ctrl_init;
	}

	err = zxdh_rt_init_hw(iwdev);
	if (err) {
		pr_err("[zxdh_rdma] %s[%d]: rt init hw failed! err=%d\n", __func__, __LINE__, err);
		goto err_rt_init;
	}

	if (iwdev->rf->use_ext_mem_flag == false) {
		if (rf->chip_srq_base_paddr != 0 && rf->srq_mem_size != 0) {
			zxdh_clear_chip_srq_mem(&rf->sc_dev, rf->chip_srq_base_paddr, rf->srq_mem_size);
		}
	}

	zxdh_add_handler(hdl);
	err = zxdh_ib_register_device(iwdev);
	if (err) {
		pr_err("[zxdh_rdma] %s[%d]: ib dev reg failed! err=%d\n", __func__, __LINE__, err);
		goto err_ibreg;
	}

	refcount_set(&iwdev->trace_switch.t_switch, 0);
	dev_set_drvdata(&aux_dev->dev, iwdev);
	if (!rf->ftype)
	{
		zxdh_store_rdma_pf_glb(rf);
	}

	create_debugfs_entry(rf);

	/* Create AEQ polling debugfs entries (with built-in duplicate protection) */
	zxdh_create_aeq_poll_debugfs(rf);

	if (!rf->ftype)
		zxdh_hwbond_register_rdma_ops(&hwbond_ops);
	zxdh_set_device_valid(iidc_adev);
	if (rf->ftype) {
	    err = zxdh_send_mailbox_msg(rf, ZTE_ZXDH_OP_DEVICE_LOAD, 0, 0, 0);
		if (err) 
		    pr_err("[zxdh_rdma]Load VF failed to be sent to PF\n");
		if (rf->gen_ops.zxdh_common_func) {
			if (rf->gen_ops.zxdh_common_func(iwdev->netdev, &iwdev->vf_vlan, ZXDH_FUNC_VF_VLAN_GET)) {
				iwdev->vf_vlan.vlan_id = DEFAULT_VLAN_ID;
				iwdev->vf_vlan.qos = DEFAULT_QOS_ID;
				pr_info("[zxdh_rdma] not support set sriov_vlan\n");
			} else {
				if (iwdev->vf_vlan.vlan_id > MAX_VLAN_ID) {
					iwdev->vf_vlan.vlan_id = DEFAULT_VLAN_ID;
					pr_info("[zxdh_rdma] get invalid sriov vlan_id, set sriov vlan_id to %d\n", iwdev->vf_vlan.vlan_id);
				}
				if (iwdev->vf_vlan.qos > MAX_QOS_ID) {
					iwdev->vf_vlan.qos = DEFAULT_QOS_ID;
					pr_info("[zxdh_rdma] get invalid sriov qos, set sriov qos to %d\n", iwdev->vf_vlan.qos);
				}
				pr_info("[zxdh_rdma] vf_id:%d get vlan_id:%d qos:%d\n", iwdev->rf->vf_id, iwdev->vf_vlan.vlan_id, iwdev->vf_vlan.qos);
			}
		}
	} else {
        zxdh_set_device_completed(rf);
	}
	atomic_set(&iwdev->psn_wrap_qp_num, 0);

	if (rf->sc_dev.np_mode_low_lat) {
		if (iwdev->netdev->dev_addr) {
			zxdh_add_dpp_mac_tbl(iwdev, iwdev->netdev);
			zxdh_handle_upper_dev(iwdev, ZXDH_CMD_NP_MAC_ADD);
		} else {
			pr_err("[zxdh_rdma] %s[%d] netdev dev_addr is null!\n", __func__, __LINE__);
		}
	}

	/* 解bond时副口会调用probe流程。仅PF配置 */
	if (!rf->ftype)
		configure_initial_port_speed(iwdev);

	pr_info("[zxdh_rdma] INIT: device[%d] probe success\n", rf->sc_dev.vhca_id);
	return 0;

err_ibreg:
	zxdh_rt_deinit_hw(iwdev);
err_rt_init:
	zxdh_ctrl_deinit_hw(rf);
#ifdef MSIX_DEBUG
	pci_free_irq_vectors(cdev_info->pdev);
#endif
err_ctrl_init:
	kfree(hdl);
	zxdh_cleanup_dpp_mac_tbl(iwdev);
err_ext_mem_init:
	if(iwdev->rf->config_path)
		kfree(iwdev->rf->config_path);
err_mgr_init:
err_iwdev_rf:
#ifndef IB_DEALLOC_DRIVER_SUPPORT
	ib_dealloc_device(&iwdev->ibdev);
#endif /* IB_DEALLOC_DRIVER_SUPPORT */
	kfree(iwdev->rf);
	iwdev->rf = NULL;
err_cdev_info:
	kfree(cdev_info);
	cdev_info = NULL;

	return err;
}

static const struct zxdh_auxiliary_device_id zxdh_auxiliary_id_table[] = {
	{
        .name = ZXDH_PF_NAME "." ZXDH_RDMA_DEV_NAME,
	},
	{},
};

MODULE_DEVICE_TABLE(auxiliary, zxdh_auxiliary_id_table);

static struct iidc_auxiliary_drv zxdh_auxiliary_drv = {
	.adrv = {
        .name = ZXDH_RDMA_DEV_NAME,
	    .id_table = zxdh_auxiliary_id_table,
	    .probe = zxdh_probe,
	    .remove = zxdh_remove,
		.shutdown = zxdh_shutdown,
	},
	.event_handler = zxdh_iidc_event_handler,
};

static void zxdh_show_ver(void) 
{
	if (display_drv_side_fw_ver == 1)
	    pr_info("[zxdh_rdma] zrdma driver side fw version: %d.%d.%d\n", FW_MAJOR_VER,
	        FW_MINOR_FW_VER, FW_MINOR_DRV_VER);
	if (display_drv_side_net_ver == 1)
	    pr_info("[zxdh_rdma] zrdma driver side network version: %d.%d.%d\n", DRV_MAJOR_VER,
	        DRV_NET_MINOR_VER, DRV_RDMA_MINOR_VER);
}

static int __init zxdh_init_module(void)
{
	int ret;
	zxdh_create_hasg_tbl();
    INIT_LIST_HEAD(&zxdh_rdma_list);
    zxdh_rdma_name_lock_init();
    pr_info("[zxdh_rdma] zrdma driver version: %d.%d.%d.%d\n", DRV_VER_MAJOR, DRV_VER_MINOR, DRV_VER_BUILD, DRV_VER_PATCH);
	zxdh_show_ver();

	zrdma_register_debugfs();
	ret = zxdh_auxiliary_driver_register(&zxdh_auxiliary_drv.adrv);
	if (ret)
		return ret;

	pr_info("[zxdh_rdma] [%s] install hwbond callback function\n", __func__);
	zxdh_hwbond_register_rdma_ops(&hwbond_ops);
	zxdh_rdma_events_register(&zxdh_rdma_event_handler);
	zxdh_register_notifiers();

	return 0;
}

static void __exit zxdh_exit_module(void)
{
	zxdh_unregister_notifiers();

	pr_info("[zxdh_rdma] [%s] remove hwbond callback function\n", __func__);
	hwbond_ops.cfg_rdma_hb_master = NULL;
	hwbond_ops.cfg_rdma_hb_speed = NULL;
	zxdh_hwbond_unregister_rdma_ops();
	zxdh_rdma_events_unregister();
	zxdh_auxiliary_driver_unregister(&zxdh_auxiliary_drv.adrv);
	zrdma_unregister_debugfs();
	zxdh_remove_hashtable();
	zxdh_del_all_rdma_name();
	zxdh_rdma_name_lock_deinit();
}

module_init(zxdh_init_module);
module_exit(zxdh_exit_module);
