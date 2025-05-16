// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [COMM]" fmt

#include <net/addrconf.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/io-mapping.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <linux/debugfs.h>

#include "ossl_knl.h"
#include "hinic3_mt.h"
#include "hinic3_crm.h"
#include "hinic3_lld.h"
#include "hinic3_sriov.h"
#include "hinic3_nictool.h"
#include "hinic3_pci_id_tbl.h"
#include "hinic3_hwdev.h"
#include "cfg_mgmt_mpu_cmd_defs.h"
#include "mpu_cmd_base_defs.h"
#include "hinic3_dev_mgmt.h"

#define HINIC3_WAIT_TOOL_CNT_TIMEOUT	10000
#define HINIC3_WAIT_TOOL_MIN_USLEEP_TIME	9900
#define HINIC3_WAIT_TOOL_MAX_USLEEP_TIME	10000
#define HIGHT_BDF 8

static unsigned long card_bit_map;

LIST_HEAD(g_hinic3_chip_list);

struct list_head *get_hinic3_chip_list(void)
{
	return &g_hinic3_chip_list;
}

void uld_dev_hold(struct hinic3_lld_dev *lld_dev, enum hinic3_service_type type)
{
	struct hinic3_pcidev *pci_adapter = pci_get_drvdata(lld_dev->pdev);

	atomic_inc(&pci_adapter->uld_ref_cnt[type]);
}
EXPORT_SYMBOL(uld_dev_hold);

void uld_dev_put(struct hinic3_lld_dev *lld_dev, enum hinic3_service_type type)
{
	struct hinic3_pcidev *pci_adapter = pci_get_drvdata(lld_dev->pdev);

	atomic_dec(&pci_adapter->uld_ref_cnt[type]);
}
EXPORT_SYMBOL(uld_dev_put);

void lld_dev_cnt_init(struct hinic3_pcidev *pci_adapter)
{
	atomic_set(&pci_adapter->ref_cnt, 0);
}

void hinic3_lld_dev_hold(struct hinic3_lld_dev *dev)
{
	struct hinic3_pcidev *pci_adapter = NULL;

	if (!dev)
		return;

	pci_adapter = pci_get_drvdata(dev->pdev);

	atomic_inc(&pci_adapter->ref_cnt);
}

void hinic3_lld_dev_put(struct hinic3_lld_dev *dev)
{
	struct hinic3_pcidev *pci_adapter = NULL;

	if (!dev)
		return;

	pci_adapter = pci_get_drvdata(dev->pdev);

	atomic_dec(&pci_adapter->ref_cnt);
}

void wait_lld_dev_unused(struct hinic3_pcidev *pci_adapter)
{
	unsigned long end;

	end = jiffies + msecs_to_jiffies(HINIC3_WAIT_TOOL_CNT_TIMEOUT);
	do {
		if (!atomic_read(&pci_adapter->ref_cnt))
			return;

		/* if sleep 10ms, use usleep_range to be more precise */
		usleep_range(HINIC3_WAIT_TOOL_MIN_USLEEP_TIME,
			     HINIC3_WAIT_TOOL_MAX_USLEEP_TIME);
	} while (time_before(jiffies, end));
}

enum hinic3_lld_status {
	HINIC3_NODE_CHANGE	= BIT(0),
};

struct hinic3_lld_lock {
	/* lock for chip list */
	struct mutex		lld_mutex;
	unsigned long		status;
	atomic_t		dev_ref_cnt;
};

struct hinic3_lld_lock g_lld_lock;

#define WAIT_LLD_DEV_HOLD_TIMEOUT	(10 * 60 * 1000) /* 10minutes */
#define WAIT_LLD_DEV_NODE_CHANGED	(10 * 60 * 1000) /* 10minutes */
#define WAIT_LLD_DEV_REF_CNT_EMPTY	(2 * 60 * 1000)	 /* 2minutes */
#define PRINT_TIMEOUT_INTERVAL		10000
#define MS_PER_SEC			1000
#define LLD_LOCK_MIN_USLEEP_TIME	900
#define LLD_LOCK_MAX_USLEEP_TIME	1000

/* node in chip_node will changed, tools or driver can't get node
 * during this situation
 */
void lld_lock_chip_node(void)
{
	unsigned long end;
	bool timeout = true;
	u32 loop_cnt;

	mutex_lock(&g_lld_lock.lld_mutex);

	loop_cnt = 0;
	end = jiffies + msecs_to_jiffies(WAIT_LLD_DEV_NODE_CHANGED);
	do {
		if (!test_and_set_bit(HINIC3_NODE_CHANGE, &g_lld_lock.status)) {
			timeout = false;
			break;
		}

		loop_cnt++;
		if (loop_cnt % PRINT_TIMEOUT_INTERVAL == 0)
			pr_warn("Wait for lld node change complete for %us\n",
				loop_cnt / MS_PER_SEC);

		/* if sleep 1ms, use usleep_range to be more precise */
		usleep_range(LLD_LOCK_MIN_USLEEP_TIME,
			     LLD_LOCK_MAX_USLEEP_TIME);
	} while (time_before(jiffies, end));

	if (timeout && test_and_set_bit(HINIC3_NODE_CHANGE, &g_lld_lock.status))
		pr_warn("Wait for lld node change complete timeout when trying to get lld lock\n");

	loop_cnt = 0;
	timeout = true;
	end = jiffies + msecs_to_jiffies(WAIT_LLD_DEV_NODE_CHANGED);
	do {
		if (!atomic_read(&g_lld_lock.dev_ref_cnt)) {
			timeout = false;
			break;
		}

		loop_cnt++;
		if (loop_cnt % PRINT_TIMEOUT_INTERVAL == 0)
			pr_warn("Wait for lld dev unused for %us, reference count: %d\n",
				loop_cnt / MS_PER_SEC,
				atomic_read(&g_lld_lock.dev_ref_cnt));

		/* if sleep 1ms, use usleep_range to be more precise */
		usleep_range(LLD_LOCK_MIN_USLEEP_TIME,
			     LLD_LOCK_MAX_USLEEP_TIME);
	} while (time_before(jiffies, end));

	if (timeout && atomic_read(&g_lld_lock.dev_ref_cnt))
		pr_warn("Wait for lld dev unused timeout\n");

	mutex_unlock(&g_lld_lock.lld_mutex);
}

void lld_unlock_chip_node(void)
{
	clear_bit(HINIC3_NODE_CHANGE, &g_lld_lock.status);
}

/* When tools or other drivers want to get node of chip_node, use this function
 * to prevent node be freed
 */
void lld_hold(void)
{
	unsigned long end;
	u32 loop_cnt = 0;

	/* ensure there have not any chip node in changing */
	mutex_lock(&g_lld_lock.lld_mutex);

	end = jiffies + msecs_to_jiffies(WAIT_LLD_DEV_HOLD_TIMEOUT);
	do {
		if (!test_bit(HINIC3_NODE_CHANGE, &g_lld_lock.status))
			break;

		loop_cnt++;

		if (loop_cnt % PRINT_TIMEOUT_INTERVAL == 0)
			pr_warn("Wait lld node change complete for %us\n",
				loop_cnt / MS_PER_SEC);
		/* if sleep 1ms, use usleep_range to be more precise */
		usleep_range(LLD_LOCK_MIN_USLEEP_TIME,
			     LLD_LOCK_MAX_USLEEP_TIME);
	} while (time_before(jiffies, end));

	if (test_bit(HINIC3_NODE_CHANGE, &g_lld_lock.status))
		pr_warn("Wait lld node change complete timeout when trying to hode lld dev\n");

	atomic_inc(&g_lld_lock.dev_ref_cnt);
	mutex_unlock(&g_lld_lock.lld_mutex);
}

void lld_put(void)
{
	atomic_dec(&g_lld_lock.dev_ref_cnt);
}

void hinic3_lld_lock_init(void)
{
	mutex_init(&g_lld_lock.lld_mutex);
	atomic_set(&g_lld_lock.dev_ref_cnt, 0);
}

void hinic3_get_all_chip_id(void *id_info)
{
	struct nic_card_id *card_id = (struct nic_card_id *)id_info;
	struct card_node *chip_node = NULL;
	int i = 0;
	int id, err;

	lld_hold();
	list_for_each_entry(chip_node, &g_hinic3_chip_list, node) {
		err = sscanf(chip_node->chip_name, HINIC3_CHIP_NAME "%d", &id);
		if (err < 0) {
			pr_err("Failed to get hinic3 id\n");
			continue;
		}
		card_id->id[i] = (u32)id;
		i++;
	}
	lld_put();
	card_id->num = (u32)i;
}

int hinic3_bar_mmap_param_valid(phys_addr_t phy_addr, unsigned long vmsize)
{
	struct card_node *chip_node = NULL;
	struct hinic3_pcidev *dev = NULL;
	u64 bar1_phy_addr = 0;
	u64 bar3_phy_addr = 0;
	u64 bar1_size = 0;
	u64 bar3_size = 0;

	lld_hold();

	/* get PF bar1 or bar3 physical address to verify */
	list_for_each_entry(chip_node, &g_hinic3_chip_list, node) {
		list_for_each_entry(dev, &chip_node->func_list, node) {
			if (hinic3_func_type(dev->hwdev) == TYPE_VF)
				continue;

			bar1_phy_addr = pci_resource_start(dev->pcidev, HINIC3_PF_PCI_CFG_REG_BAR);
			bar1_size = pci_resource_len(dev->pcidev, HINIC3_PF_PCI_CFG_REG_BAR);

			bar3_phy_addr = pci_resource_start(dev->pcidev, HINIC3_PCI_MGMT_REG_BAR);
			bar3_size = pci_resource_len(dev->pcidev, HINIC3_PCI_MGMT_REG_BAR);
			if ((phy_addr == bar1_phy_addr && vmsize <= bar1_size) ||
			    (phy_addr == bar3_phy_addr && vmsize <= bar3_size)) {
				lld_put();
				return 0;
			}
		}
	}

	lld_put();
	return -EINVAL;
}

void hinic3_get_card_func_info_by_card_name(const char *chip_name,
					    struct hinic3_card_func_info *card_func)
{
	struct card_node *chip_node = NULL;
	struct hinic3_pcidev *dev = NULL;
	struct func_pdev_info *pdev_info = NULL;

	card_func->num_pf = 0;

	lld_hold();

	list_for_each_entry(chip_node, &g_hinic3_chip_list, node) {
		if (strncmp(chip_node->chip_name, chip_name, IFNAMSIZ))
			continue;

		list_for_each_entry(dev, &chip_node->func_list, node) {
			if (hinic3_func_type(dev->hwdev) == TYPE_VF)
				continue;

			pdev_info = &card_func->pdev_info[card_func->num_pf];
			pdev_info->bar1_size =
				pci_resource_len(dev->pcidev,
						 HINIC3_PF_PCI_CFG_REG_BAR);
			pdev_info->bar1_phy_addr =
				pci_resource_start(dev->pcidev,
						   HINIC3_PF_PCI_CFG_REG_BAR);

			pdev_info->bar3_size =
				pci_resource_len(dev->pcidev,
						 HINIC3_PCI_MGMT_REG_BAR);
			pdev_info->bar3_phy_addr =
				pci_resource_start(dev->pcidev,
						   HINIC3_PCI_MGMT_REG_BAR);

			card_func->num_pf++;
			if (card_func->num_pf >= MAX_SIZE) {
				lld_put();
				return;
			}
		}
	}

	lld_put();
}

static bool is_pcidev_match_chip_name(const char *ifname, struct hinic3_pcidev *dev,
				      struct card_node *chip_node, enum func_type type)
{
	if (!strncmp(chip_node->chip_name, ifname, IFNAMSIZ)) {
		if (hinic3_func_type(dev->hwdev) != type)
			return false;
		return true;
	}

	return false;
}

static struct hinic3_lld_dev *get_dst_type_lld_dev_by_chip_name(const char *ifname,
								enum func_type type)
{
	struct card_node *chip_node = NULL;
	struct hinic3_pcidev *dev = NULL;

	list_for_each_entry(chip_node, &g_hinic3_chip_list, node) {
		list_for_each_entry(dev, &chip_node->func_list, node) {
			if (is_pcidev_match_chip_name(ifname, dev, chip_node, type))
				return &dev->lld_dev;
		}
	}

	return NULL;
}

struct hinic3_lld_dev *hinic3_get_lld_dev_by_chip_name(const char *chip_name)
{
	struct hinic3_lld_dev *dev = NULL;

	lld_hold();

	dev = get_dst_type_lld_dev_by_chip_name(chip_name, TYPE_PPF);
	if (dev)
		goto out;

	dev = get_dst_type_lld_dev_by_chip_name(chip_name, TYPE_PF);
	if (dev)
		goto out;

	dev = get_dst_type_lld_dev_by_chip_name(chip_name, TYPE_VF);
out:
	if (dev)
		hinic3_lld_dev_hold(dev);
	lld_put();

	return dev;
}

static int get_dynamic_uld_dev_name(struct hinic3_pcidev *dev, enum hinic3_service_type type,
				    char *ifname)
{
	u32 out_size = IFNAMSIZ;

	if (!g_hinic3_uld_info[type].ioctl)
		return -EFAULT;

	return g_hinic3_uld_info[type].ioctl(dev->uld_dev[type],
				      GET_ULD_DEV_NAME,
				      NULL, 0, ifname, &out_size);
}

static bool is_pcidev_match_dev_name(const char *dev_name, struct hinic3_pcidev *dev,
				     enum hinic3_service_type type)
{
	enum hinic3_service_type i;
	char nic_uld_name[IFNAMSIZ] = {0};
	int err;

	if (type > SERVICE_T_MAX)
		return false;

	if (type == SERVICE_T_MAX) {
		for (i = SERVICE_T_OVS; i < SERVICE_T_MAX; i++) {
			if (!strncmp(dev->uld_dev_name[i], dev_name, IFNAMSIZ))
				return true;
		}
	} else {
		if (!strncmp(dev->uld_dev_name[type], dev_name, IFNAMSIZ))
			return true;
	}

	err = get_dynamic_uld_dev_name(dev, SERVICE_T_NIC, (char *)nic_uld_name);
	if (err == 0) {
		if (!strncmp(nic_uld_name, dev_name, IFNAMSIZ))
			return true;
	}

	return false;
}

static struct hinic3_lld_dev *get_lld_dev_by_dev_name(const char *dev_name,
						      enum hinic3_service_type type, bool hold)
{
	struct card_node *chip_node = NULL;
	struct hinic3_pcidev *dev = NULL;

	lld_hold();

	list_for_each_entry(chip_node, &g_hinic3_chip_list, node) {
		list_for_each_entry(dev, &chip_node->func_list, node) {
			if (is_pcidev_match_dev_name(dev_name, dev, type)) {
				if (hold)
					hinic3_lld_dev_hold(&dev->lld_dev);
				lld_put();
				return &dev->lld_dev;
			}
		}
	}

	lld_put();

	return NULL;
}

struct hinic3_lld_dev *hinic3_get_lld_dev_by_chip_and_port(const char *chip_name, u8 port_id)
{
	struct card_node *chip_node = NULL;
	struct hinic3_pcidev *dev = NULL;

	lld_hold();
	list_for_each_entry(chip_node, &g_hinic3_chip_list, node) {
		list_for_each_entry(dev, &chip_node->func_list, node) {
			if (hinic3_func_type(dev->hwdev) == TYPE_VF)
				continue;

			if (hinic3_physical_port_id(dev->hwdev) == port_id &&
			    !strncmp(chip_node->chip_name, chip_name, IFNAMSIZ)) {
				hinic3_lld_dev_hold(&dev->lld_dev);
				lld_put();

				return &dev->lld_dev;
			}
		}
	}
	lld_put();

	return NULL;
}

void *hinic3_get_ppf_dev(void)
{
	struct card_node *chip_node = NULL;
	struct hinic3_pcidev *pci_adapter = NULL;
	struct list_head *chip_list = NULL;

	lld_hold();
	chip_list = get_hinic3_chip_list();

	list_for_each_entry(chip_node, chip_list, node)
		list_for_each_entry(pci_adapter, &chip_node->func_list, node)
			if (hinic3_func_type(pci_adapter->hwdev) == TYPE_PPF) {
				pr_info("Get ppf_func_id:%u",
					hinic3_global_func_id(pci_adapter->hwdev));
				lld_put();
				return pci_adapter->lld_dev.hwdev;
			}

	lld_put();
	return NULL;
}
EXPORT_SYMBOL(hinic3_get_ppf_dev);

struct hinic3_lld_dev *hinic3_get_lld_dev_by_dev_name(const char *dev_name,
						      enum hinic3_service_type type)
{
	return get_lld_dev_by_dev_name(dev_name, type, true);
}
EXPORT_SYMBOL(hinic3_get_lld_dev_by_dev_name);

struct hinic3_lld_dev *hinic3_get_lld_dev_by_dev_name_unsafe(const char *dev_name,
							     enum hinic3_service_type type)
{
	return get_lld_dev_by_dev_name(dev_name, type, false);
}
EXPORT_SYMBOL(hinic3_get_lld_dev_by_dev_name_unsafe);

static void *get_uld_by_lld_dev(struct hinic3_lld_dev *lld_dev, enum hinic3_service_type type,
				bool hold)
{
	struct hinic3_pcidev *dev = NULL;
	void *uld = NULL;

	if (!lld_dev)
		return NULL;

	dev = pci_get_drvdata(lld_dev->pdev);
	if (!dev)
		return NULL;

	spin_lock_bh(&dev->uld_lock);
	if (!dev->uld_dev[type] || !test_bit(type, &dev->uld_state)) {
		spin_unlock_bh(&dev->uld_lock);
		return NULL;
	}
	uld = dev->uld_dev[type];

	if (hold)
		atomic_inc(&dev->uld_ref_cnt[type]);
	spin_unlock_bh(&dev->uld_lock);

	return uld;
}

void *hinic3_get_uld_dev(struct hinic3_lld_dev *lld_dev, enum hinic3_service_type type)
{
	return get_uld_by_lld_dev(lld_dev, type, true);
}
EXPORT_SYMBOL(hinic3_get_uld_dev);

void *hinic3_get_uld_dev_unsafe(struct hinic3_lld_dev *lld_dev, enum hinic3_service_type type)
{
	return get_uld_by_lld_dev(lld_dev, type, false);
}
EXPORT_SYMBOL(hinic3_get_uld_dev_unsafe);

static struct hinic3_lld_dev *get_ppf_lld_dev(struct hinic3_lld_dev *lld_dev, bool hold)
{
	struct hinic3_pcidev *pci_adapter = NULL;
	struct card_node *chip_node = NULL;
	struct hinic3_pcidev *dev = NULL;

	if (!lld_dev)
		return NULL;

	pci_adapter = pci_get_drvdata(lld_dev->pdev);
	if (!pci_adapter)
		return NULL;

	lld_hold();
	chip_node = pci_adapter->chip_node;
	list_for_each_entry(dev, &chip_node->func_list, node) {
		if (dev->hwdev && hinic3_func_type(dev->hwdev) == TYPE_PPF) {
			if (hold)
				hinic3_lld_dev_hold(&dev->lld_dev);
			lld_put();
			return &dev->lld_dev;
		}
	}
	lld_put();

	return NULL;
}

struct hinic3_lld_dev *hinic3_get_ppf_lld_dev(struct hinic3_lld_dev *lld_dev)
{
	return get_ppf_lld_dev(lld_dev, true);
}
EXPORT_SYMBOL(hinic3_get_ppf_lld_dev);

struct hinic3_lld_dev *hinic3_get_ppf_lld_dev_unsafe(struct hinic3_lld_dev *lld_dev)
{
	return get_ppf_lld_dev(lld_dev, false);
}
EXPORT_SYMBOL(hinic3_get_ppf_lld_dev_unsafe);

int hinic3_get_chip_name(struct hinic3_lld_dev *lld_dev, char *chip_name, u16 max_len)
{
	struct hinic3_pcidev *pci_adapter = NULL;
	int ret = 0;

	if (!lld_dev || !chip_name || !max_len)
		return -EINVAL;

	pci_adapter = pci_get_drvdata(lld_dev->pdev);
	if (!pci_adapter)
		return -EFAULT;

	lld_hold();
	if (strscpy(chip_name, pci_adapter->chip_node->chip_name, max_len) < 0)
		goto RELEASE;
	chip_name[max_len - 1] = '\0';

	lld_put();

	return 0;

RELEASE:
	lld_put();

	return ret;
}
EXPORT_SYMBOL(hinic3_get_chip_name);

struct hinic3_hwdev *hinic3_get_sdk_hwdev_by_lld(struct hinic3_lld_dev *lld_dev)
{
	return lld_dev->hwdev;
}

void hinic3_write_oshr_info(struct os_hot_replace_info *out_oshr_info,
			    struct hw_pf_info *info,
			    struct hinic3_board_info *board_info,
			    struct card_node *chip_node, u32 serivce_enable,
			    u32 func_info_idx)
{
	out_oshr_info->func_infos[func_info_idx].pf_idx = info->glb_func_idx;
	out_oshr_info->func_infos[func_info_idx].backup_pf =
			(((info->glb_func_idx) / (board_info->port_num)) %
			HOT_REPLACE_PARTITION_NUM == 0) ?
			((info->glb_func_idx) + (board_info->port_num)) :
			((info->glb_func_idx) - (board_info->port_num));
	out_oshr_info->func_infos[func_info_idx].partition =
			((info->glb_func_idx) / (board_info->port_num)) %
			HOT_REPLACE_PARTITION_NUM;
	out_oshr_info->func_infos[func_info_idx].port_id = info->port_id;
	out_oshr_info->func_infos[func_info_idx].bdf =
			(info->bus_num << HIGHT_BDF) + info->glb_func_idx;
	out_oshr_info->func_infos[func_info_idx].bus_num = chip_node->bus_num;
	out_oshr_info->func_infos[func_info_idx].valid = serivce_enable;
	memcpy(out_oshr_info->func_infos[func_info_idx].card_name,
	       chip_node->chip_name, IFNAMSIZ);
}

void hinic3_get_os_hot_replace_info(void *oshr_info)
{
	struct os_hot_replace_info *out_oshr_info =
					(struct os_hot_replace_info *)oshr_info;
	struct card_node *chip_node = NULL;
	struct hinic3_pcidev *dst_dev = NULL;
	struct hinic3_board_info *board_info = NULL;
	struct hw_pf_info *infos = NULL;
	struct hinic3_hw_pf_infos *pf_infos = NULL;
	struct hinic3_hwdev *hwdev = NULL;
	u32 func_info_idx = 0, func_id = 0, func_num, serivce_enable = 0;
	struct list_head *hinic3_chip_list = get_hinic3_chip_list();
	int err;

	lld_hold();
	pf_infos = kzalloc(sizeof(*pf_infos), GFP_KERNEL);
	if (!pf_infos) {
		pr_err("kzalloc pf_infos fail\n");
		lld_put();
		return;
	}
	list_for_each_entry(chip_node, hinic3_chip_list, node) {
		list_for_each_entry(dst_dev, &chip_node->func_list, node) {
			err = hinic3_get_hw_pf_infos(dst_dev->hwdev,
						     pf_infos,
						     HINIC3_CHANNEL_COMM);
			if (err != 0) {
				pr_err("get pf info failed\n");
				break;
			}
			hwdev = (struct hinic3_hwdev *)dst_dev->hwdev;
			serivce_enable = 0;
			infos = pf_infos->infos;
			board_info = &hwdev->board_info;
			if (hwdev->hot_replace_mode == HOT_REPLACE_ENABLE)
				serivce_enable = 1;
			break;
	}

		func_num = pf_infos->num_pfs;
		if (func_num <= 0) {
			pr_err("get pf num failed\n");
			break;
		}

		for (func_id = 0; func_id < func_num; func_id++) {
			hinic3_write_oshr_info(out_oshr_info, &infos[func_id],
					       board_info, chip_node,
					       serivce_enable, func_info_idx);
			func_info_idx++;
		}
	}
	out_oshr_info->func_cnt = func_info_idx;
	kfree(pf_infos);
	lld_put();
}

struct card_node *hinic3_get_chip_node_by_lld(struct hinic3_lld_dev *lld_dev)
{
	struct hinic3_pcidev *pci_adapter = pci_get_drvdata(lld_dev->pdev);

	return pci_adapter->chip_node;
}

static struct card_node *hinic3_get_chip_node_by_hwdev(const void *hwdev)
{
	struct card_node *chip_node = NULL;
	struct card_node *node_tmp = NULL;
	struct hinic3_pcidev *dev = NULL;

	if (!hwdev)
		return NULL;

	lld_hold();

	list_for_each_entry(node_tmp, &g_hinic3_chip_list, node) {
		if (!chip_node) {
			list_for_each_entry(dev, &node_tmp->func_list, node) {
				if (dev->hwdev == hwdev) {
					chip_node = node_tmp;
					break;
				}
			}
		}
	}

	lld_put();

	return chip_node;
}

static bool is_func_valid(struct hinic3_pcidev *dev)
{
	if (hinic3_func_type(dev->hwdev) == TYPE_VF)
		return false;

	return true;
}

void hinic3_get_card_info(const void *hwdev, void *bufin)
{
	struct card_node *chip_node = NULL;
	struct card_info *info = (struct card_info *)bufin;
	struct hinic3_pcidev *dev = NULL;
	void *fun_hwdev = NULL;
	u32 i = 0;

	info->pf_num = 0;

	chip_node = hinic3_get_chip_node_by_hwdev(hwdev);
	if (!chip_node)
		return;

	lld_hold();

	list_for_each_entry(dev, &chip_node->func_list, node) {
		if (!is_func_valid(dev))
			continue;

		fun_hwdev = dev->hwdev;

		if (hinic3_support_nic(fun_hwdev, NULL)) {
			if (dev->uld_dev[SERVICE_T_NIC]) {
				info->pf[i].pf_type |= (u32)BIT(SERVICE_T_NIC);
				get_dynamic_uld_dev_name(dev, SERVICE_T_NIC,
							 info->pf[i].name);
			}
		}

		if (hinic3_support_ppa(fun_hwdev, NULL)) {
			if (dev->uld_dev[SERVICE_T_PPA]) {
				info->pf[i].pf_type |= (u32)BIT(SERVICE_T_PPA);
				get_dynamic_uld_dev_name(dev, SERVICE_T_PPA,
							 info->pf[i].name);
			}
		}

		if (hinic3_func_for_mgmt(fun_hwdev))
			strscpy(info->pf[i].name, "FOR_MGMT", IFNAMSIZ);

		if (dev->lld_dev.pdev->subsystem_device ==
		    BIFUR_RESOURCE_PF_SSID)
			strscpy(info->pf[i].name, "bifur", IFNAMSIZ);

		strscpy(info->pf[i].bus_info, pci_name(dev->pcidev),
			sizeof(info->pf[i].bus_info));
		info->pf_num++;
		i = info->pf_num;
	}

	lld_put();
}

struct hinic3_sriov_info *hinic3_get_sriov_info_by_pcidev(struct pci_dev *pdev)
{
	struct hinic3_pcidev *pci_adapter = NULL;

	if (!pdev)
		return NULL;

	pci_adapter = pci_get_drvdata(pdev);
	if (!pci_adapter)
		return NULL;

	return &pci_adapter->sriov_info;
}

void *hinic3_get_hwdev_by_pcidev(struct pci_dev *pdev)
{
	struct hinic3_pcidev *pci_adapter = NULL;

	if (!pdev)
		return NULL;

	pci_adapter = pci_get_drvdata(pdev);
	if (!pci_adapter)
		return NULL;

	return pci_adapter->hwdev;
}

bool hinic3_is_in_host(void)
{
	struct card_node *chip_node = NULL;
	struct hinic3_pcidev *dev = NULL;

	lld_hold();
	list_for_each_entry(chip_node, &g_hinic3_chip_list, node) {
		list_for_each_entry(dev, &chip_node->func_list, node) {
			if (hinic3_func_type(dev->hwdev) != TYPE_VF) {
				lld_put();
				return true;
			}
		}
	}

	lld_put();

	return false;
}

static bool chip_node_is_exist(struct hinic3_pcidev *pci_adapter,
			       unsigned char *bus_number)
{
	struct card_node *chip_node = NULL;
	struct pci_dev *pf_pdev = NULL;

	if  (!pci_is_root_bus(pci_adapter->pcidev->bus))
		*bus_number = pci_adapter->pcidev->bus->number;

	if (*bus_number != 0) {
		if (pci_adapter->pcidev->is_virtfn) {
			pf_pdev = pci_adapter->pcidev->physfn;
			*bus_number = pf_pdev->bus->number;
		}

		list_for_each_entry(chip_node, &g_hinic3_chip_list, node) {
			if (chip_node->bus_num == *bus_number) {
				pci_adapter->chip_node = chip_node;
				return true;
			}
		}
	} else if (HINIC3_IS_VF_DEV(pci_adapter->pcidev) ||
		   HINIC3_IS_SPU_DEV(pci_adapter->pcidev)) {
		list_for_each_entry(chip_node, &g_hinic3_chip_list, node) {
			if (chip_node) {
				pci_adapter->chip_node = chip_node;
				return true;
			}
		}
	}

	return false;
}

int alloc_chip_node(struct hinic3_pcidev *pci_adapter)
{
	struct card_node *chip_node = NULL;
	unsigned char i;
	unsigned char bus_number = 0;
	int err;

	if (chip_node_is_exist(pci_adapter, &bus_number))
		return 0;

	for (i = 0; i < CARD_MAX_SIZE; i++) {
		if (test_and_set_bit(i, &card_bit_map) == 0)
			break;
	}

	if (i == CARD_MAX_SIZE) {
		sdk_err(&pci_adapter->pcidev->dev, "Failed to alloc card id\n");
		return -EFAULT;
	}

	chip_node = kzalloc(sizeof(*chip_node), GFP_KERNEL);
	if (!chip_node) {
		clear_bit(i, &card_bit_map);
		sdk_err(&pci_adapter->pcidev->dev,
			"Failed to alloc chip node\n");
		return -ENOMEM;
	}

	/* bus number */
	chip_node->bus_num = bus_number;

	snprintf(chip_node->chip_name, IFNAMSIZ, "%s%u", HINIC3_CHIP_NAME, i);

	err = sscanf(chip_node->chip_name, HINIC3_CHIP_NAME "%d", &chip_node->chip_id);
	if (err <= 0) {
		clear_bit(i, &card_bit_map);
		kfree(chip_node);
		return -EINVAL;
	}

	sdk_info(&pci_adapter->pcidev->dev,
		 "Add new chip %s to global list succeed\n",
		 chip_node->chip_name);

	list_add_tail(&chip_node->node, &g_hinic3_chip_list);

	INIT_LIST_HEAD(&chip_node->func_list);
	pci_adapter->chip_node = chip_node;

	return 0;
}

void free_chip_node(struct hinic3_pcidev *pci_adapter)
{
	struct card_node *chip_node = pci_adapter->chip_node;
	int id, err;

	if (list_empty(&chip_node->func_list)) {
		list_del(&chip_node->node);
		sdk_info(&pci_adapter->pcidev->dev,
			 "Delete chip %s from global list succeed\n",
			 chip_node->chip_name);
		err = sscanf(chip_node->chip_name, HINIC3_CHIP_NAME "%d", &id);
		if (err < 0)
			sdk_err(&pci_adapter->pcidev->dev, "Failed to get hinic3 id\n");

		clear_bit(id, &card_bit_map);

		kfree(chip_node);
	}
}

int hinic3_get_pf_id(struct card_node *chip_node, u32 port_id, u32 *pf_id, u32 *isvalid)
{
	struct hinic3_pcidev *dev = NULL;

	lld_hold();
	list_for_each_entry(dev, &chip_node->func_list, node) {
		if (hinic3_func_type(dev->hwdev) == TYPE_VF)
			continue;

		if (hinic3_physical_port_id(dev->hwdev) == port_id) {
			*pf_id = hinic3_global_func_id(dev->hwdev);
			*isvalid = 1;
			break;
		}
	}
	lld_put();

	return 0;
}

void hinic3_get_mbox_cnt(const void *hwdev, void *bufin)
{
	struct card_node *chip_node = NULL;
	struct card_mbox_cnt_info *info = (struct card_mbox_cnt_info *)bufin;
	struct hinic3_pcidev *dev = NULL;
	struct hinic3_hwdev *func_hwdev = NULL;
	u32 i = 0;

	info->func_num = 0;
	chip_node = hinic3_get_chip_node_by_hwdev(hwdev);
	if (!chip_node)
		return;

	lld_hold();

	list_for_each_entry(dev, &chip_node->func_list, node) {
		func_hwdev = (struct hinic3_hwdev *)dev->hwdev;
		strscpy(info->func_info[i].bus_info, pci_name(dev->pcidev),
			sizeof(info->func_info[i].bus_info));

		info->func_info[i].send_cnt = func_hwdev->mbox_send_cnt;
		info->func_info[i].ack_cnt = func_hwdev->mbox_ack_cnt;
		info->func_num++;
		i = info->func_num;
		if (i >= ARRAY_SIZE(info->func_info)) {
			sdk_err(&dev->pcidev->dev, "chip_node->func_list bigger than pf_max + vf_max\n");
			break;
		}
	}

	lld_put();
}
