// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

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

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_pci_sriov.h"
#include "sss_pci_id_tbl.h"
#include "sss_adapter.h"
#include "sss_adapter_mgmt.h"
#include "sss_pci_global.h"
#include "sss_tool_comm.h"
#include "sss_hw_export.h"
#include "sss_tool_hw.h"
#include "sss_tool.h"

#ifndef SSS_PF_NUM_MAX
#define SSS_PF_NUM_MAX (16)
#endif

#define SSS_ADAPTER_CNT_TIMEOUT		10000
#define SSS_WAIT_ADAPTER_USLEEP_MIN	9900
#define SSS_WAIT_ADAPTER_USLEEP_MAX	10000

#define SSS_CHIP_NODE_HOLD_TIMEOUT	(10 * 60 * 1000)
#define SSS_WAIT_CHIP_NODE_CHANGED	(10 * 60 * 1000)
#define SSS_PRINT_TIMEOUT_INTERVAL	10000
#define SSS_MICRO_SECOND		1000
#define SSS_CHIP_NODE_USLEEP_MIN	900
#define SSS_CHIP_NODE_USLEEP_MAX	1000

#define SSS_CARD_CNT_MAX	64

#define SSS_IS_SPU_DEV(pdev)	((pdev)->device == SSS_DEV_ID_SPU)

enum sss_node_state {
	SSS_NODE_CHANGE	= BIT(0),
};

struct sss_chip_node_lock {
	struct mutex	chip_mutex; /* lock for chip list */
	unsigned long	state;
	atomic_t	ref_cnt;
};

static struct sss_chip_node_lock g_chip_node_lock;

static unsigned long g_index_bit_map;

LIST_HEAD(g_chip_list);

struct list_head *sss_get_chip_list(void)
{
	return &g_chip_list;
}

void lld_dev_hold(struct sss_hal_dev *dev)
{
	struct sss_pci_adapter *pci_adapter = pci_get_drvdata(dev->pdev);

	atomic_inc(&pci_adapter->ref_cnt);
}

void lld_dev_put(struct sss_hal_dev *dev)
{
	struct sss_pci_adapter *pci_adapter = pci_get_drvdata(dev->pdev);

	atomic_dec(&pci_adapter->ref_cnt);
}

void sss_chip_node_lock(void)
{
	unsigned long end;
	bool timeout = true;
	u32 loop_cnt;

	mutex_lock(&g_chip_node_lock.chip_mutex);

	loop_cnt = 0;
	end = jiffies + msecs_to_jiffies(SSS_WAIT_CHIP_NODE_CHANGED);
	do {
		if (!test_and_set_bit(SSS_NODE_CHANGE, &g_chip_node_lock.state)) {
			timeout = false;
			break;
		}

		loop_cnt++;
		if (loop_cnt % SSS_PRINT_TIMEOUT_INTERVAL == 0)
			pr_warn("Wait for adapter change complete for %us\n",
				loop_cnt / SSS_MICRO_SECOND);

		/* if sleep 1ms, use usleep_range to be more precise */
		usleep_range(SSS_CHIP_NODE_USLEEP_MIN, SSS_CHIP_NODE_USLEEP_MAX);
	} while (time_before(jiffies, end));

	if (timeout && test_and_set_bit(SSS_NODE_CHANGE, &g_chip_node_lock.state))
		pr_warn("Wait for adapter change complete timeout when trying to get adapter lock\n");

	loop_cnt = 0;
	timeout = true;
	end = jiffies + msecs_to_jiffies(SSS_WAIT_CHIP_NODE_CHANGED);
	do {
		if (!atomic_read(&g_chip_node_lock.ref_cnt)) {
			timeout = false;
			break;
		}

		loop_cnt++;
		if (loop_cnt % SSS_PRINT_TIMEOUT_INTERVAL == 0)
			pr_warn("Wait for adapter unused for %us, reference count: %d\n",
				loop_cnt / SSS_MICRO_SECOND,
				atomic_read(&g_chip_node_lock.ref_cnt));

		usleep_range(SSS_CHIP_NODE_USLEEP_MIN,
			     SSS_CHIP_NODE_USLEEP_MAX);
	} while (time_before(jiffies, end));

	if (timeout && atomic_read(&g_chip_node_lock.ref_cnt))
		pr_warn("Wait for adapter unused timeout\n");

	mutex_unlock(&g_chip_node_lock.chip_mutex);
}

void sss_chip_node_unlock(void)
{
	clear_bit(SSS_NODE_CHANGE, &g_chip_node_lock.state);
}

void sss_hold_chip_node(void)
{
	unsigned long end;
	u32 loop_cnt = 0;

	mutex_lock(&g_chip_node_lock.chip_mutex);

	end = jiffies + msecs_to_jiffies(SSS_CHIP_NODE_HOLD_TIMEOUT);
	do {
		if (!test_bit(SSS_NODE_CHANGE, &g_chip_node_lock.state))
			break;

		loop_cnt++;

		if (loop_cnt % SSS_PRINT_TIMEOUT_INTERVAL == 0)
			pr_warn("Wait adapter change complete for %us\n",
				loop_cnt / SSS_MICRO_SECOND);
		/* if sleep 1ms, use usleep_range to be more precise */
		usleep_range(SSS_CHIP_NODE_USLEEP_MIN, SSS_CHIP_NODE_USLEEP_MAX);
	} while (time_before(jiffies, end));

	if (test_bit(SSS_NODE_CHANGE, &g_chip_node_lock.state))
		pr_warn("Wait adapter change complete timeout when trying to adapter dev\n");

	atomic_inc(&g_chip_node_lock.ref_cnt);
	mutex_unlock(&g_chip_node_lock.chip_mutex);
}

void sss_put_chip_node(void)
{
	atomic_dec(&g_chip_node_lock.ref_cnt);
}

void sss_pre_init(void)
{
	mutex_init(&g_chip_node_lock.chip_mutex);
	atomic_set(&g_chip_node_lock.ref_cnt, 0);
	sss_init_uld_lock();
}

struct sss_pci_adapter *sss_get_adapter_by_pcidev(struct pci_dev *pdev)
{
	struct sss_pci_adapter *adapter = pci_get_drvdata(pdev);

	if (!pdev)
		return NULL;

	return adapter;
}

static bool sss_chip_node_exist(struct sss_pci_adapter *adapter,
				unsigned char bus_id)
{
	struct sss_card_node *chip_node = NULL;

	sss_chip_node_lock();
	if (bus_id != 0) {
		list_for_each_entry(chip_node, &g_chip_list, node) {
			if (chip_node->bus_id == bus_id) {
				adapter->chip_node = chip_node;
				sss_chip_node_unlock();
				return true;
			}
		}
	} else if (SSS_IS_VF_DEV(adapter->pcidev) ||
		   SSS_IS_SPU_DEV(adapter->pcidev)) {
		list_for_each_entry(chip_node, &g_chip_list, node) {
			if (chip_node) {
				adapter->chip_node = chip_node;
				sss_chip_node_unlock();
				return true;
			}
		}
	}
	sss_chip_node_unlock();

	return false;
}

static unsigned char sss_get_pci_bus_id(struct sss_pci_adapter *adapter)
{
	struct pci_dev *pf_pdev = NULL;
	unsigned char bus_id = 0;

	if (!pci_is_root_bus(adapter->pcidev->bus))
		bus_id = adapter->pcidev->bus->number;

	if (bus_id == 0)
		return bus_id;

	if (adapter->pcidev->is_virtfn) {
		pf_pdev = adapter->pcidev->physfn;
		bus_id = pf_pdev->bus->number;
	}

	return bus_id;
}

static bool sss_alloc_card_id(u8 *id)
{
	unsigned char i;

	sss_chip_node_lock();
	for (i = 0; i < SSS_CARD_CNT_MAX; i++) {
		if (test_and_set_bit(i, &g_index_bit_map) == 0) {
			sss_chip_node_unlock();
			*id = i;
			return true;
		}
	}
	sss_chip_node_unlock();

	return false;
}

static void sss_free_card_id(u8 id)
{
	clear_bit(id, &g_index_bit_map);
}

int sss_alloc_chip_node(struct sss_pci_adapter *adapter)
{
	struct sss_card_node *chip_node = NULL;
	unsigned char card_id;
	unsigned char bus_id;

	bus_id = sss_get_pci_bus_id(adapter);

	if (sss_chip_node_exist(adapter, bus_id))
		return 0;

	chip_node = kzalloc(sizeof(*chip_node), GFP_KERNEL);
	if (!chip_node)
		return -ENOMEM;

	chip_node->bus_id = bus_id;

	if (!sss_alloc_card_id(&card_id)) {
		kfree(chip_node);
		sdk_err(&adapter->pcidev->dev, "chip node is exceed\n");
		return -EINVAL;
	}

	if (snprintf(chip_node->chip_name, IFNAMSIZ, "%s%u", SSS_CHIP_NAME, card_id) < 0) {
		sss_free_card_id(card_id);
		kfree(chip_node);
		return -EINVAL;
	}

	INIT_LIST_HEAD(&chip_node->func_list);
	sss_chip_node_lock();
	list_add_tail(&chip_node->node, &g_chip_list);
	sss_chip_node_unlock();
	adapter->chip_node = chip_node;
	sdk_info(&adapter->pcidev->dev,
		 "Success to add new chip %s to global list\n", chip_node->chip_name);

	return 0;
}

void sss_free_chip_node(struct sss_pci_adapter *adapter)
{
	struct sss_card_node *chip_node = adapter->chip_node;
	int id;
	int ret;

	sss_chip_node_lock();
	if (list_empty(&chip_node->func_list)) {
		list_del(&chip_node->node);
		sdk_info(&adapter->pcidev->dev,
			 "Success to delete chip %s from global list\n",
			 chip_node->chip_name);
		ret = sscanf(chip_node->chip_name, SSS_CHIP_NAME "%d", &id);
		if (ret < 0)
			sdk_err(&adapter->pcidev->dev, "Fail to get nic id\n");

		sss_free_card_id(id);
		kfree(chip_node);
	}
	sss_chip_node_unlock();
}

void sss_add_func_list(struct sss_pci_adapter *adapter)
{
	sss_chip_node_lock();
	list_add_tail(&adapter->node, &adapter->chip_node->func_list);
	sss_chip_node_unlock();
}

void sss_del_func_list(struct sss_pci_adapter *adapter)
{
	sss_chip_node_lock();
	list_del(&adapter->node);
	sss_chip_node_unlock();
}

static struct sss_card_node *sss_get_chip_node_by_hwdev(const void *hwdev)
{
	struct sss_card_node *chip_node = NULL;
	struct sss_card_node *node_tmp = NULL;
	struct sss_pci_adapter *dev = NULL;

	if (!hwdev)
		return NULL;

	sss_hold_chip_node();

	list_for_each_entry(node_tmp, &g_chip_list, node) {
		if (!chip_node) {
			list_for_each_entry(dev, &node_tmp->func_list, node) {
				if (dev->hwdev == hwdev) {
					chip_node = node_tmp;
					break;
				}
			}
		}
	}

	sss_put_chip_node();

	return chip_node;
}

static bool sss_is_func_valid(struct sss_pci_adapter *dev)
{
	if (sss_get_func_type(dev->hwdev) == SSS_FUNC_TYPE_VF)
		return false;

	return true;
}

static int sss_get_dynamic_uld_dev_name(struct sss_pci_adapter *dev, enum sss_service_type type,
					char *ifname)
{
	u32 out_size = IFNAMSIZ;
	struct sss_uld_info *uld_info = sss_get_uld_info();

	if (!uld_info[type].ioctl)
		return -EFAULT;

	return uld_info[type].ioctl(dev->uld_dev[type], SSS_TOOL_GET_ULD_DEV_NAME,
				    NULL, 0, ifname, &out_size);
}

static bool sss_support_service_type(void *hwdev)
{
	struct sss_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	return !dev->mgmt_info->svc_cap.chip_svc_type;
}

void sss_get_card_info(const void *hwdev, void *bufin)
{
	struct sss_card_node *chip_node = NULL;
	struct sss_tool_card_info *info = (struct sss_tool_card_info *)bufin;
	struct sss_pci_adapter *dev = NULL;
	void *fun_hwdev = NULL;
	u32 i = 0;

	info->pf_num = 0;

	chip_node = sss_get_chip_node_by_hwdev(hwdev);
	if (!chip_node)
		return;

	sss_hold_chip_node();

	list_for_each_entry(dev, &chip_node->func_list, node) {
		if (!sss_is_func_valid(dev))
			continue;

		fun_hwdev = dev->hwdev;

		if (sss_support_nic(fun_hwdev)) {
			if (dev->uld_dev[SSS_SERVICE_TYPE_NIC]) {
				info->pf[i].pf_type |= (u32)BIT(SSS_SERVICE_TYPE_NIC);
				sss_get_dynamic_uld_dev_name(dev, SSS_SERVICE_TYPE_NIC,
							     info->pf[i].name);
			}
		}

		if (sss_support_ppa(fun_hwdev, NULL)) {
			if (dev->uld_dev[SSS_SERVICE_TYPE_PPA]) {
				info->pf[i].pf_type |= (u32)BIT(SSS_SERVICE_TYPE_PPA);
				sss_get_dynamic_uld_dev_name(dev, SSS_SERVICE_TYPE_PPA,
							     info->pf[i].name);
			}
		}

		if (sss_support_service_type(fun_hwdev))
			strscpy(info->pf[i].name, "FOR_MGMT", IFNAMSIZ);

		strscpy(info->pf[i].bus_info, pci_name(dev->pcidev),
			sizeof(info->pf[i].bus_info));
		info->pf_num++;
		i = info->pf_num;
	}

	sss_put_chip_node();
}

bool sss_is_in_host(void)
{
	struct sss_card_node *node = NULL;
	struct sss_pci_adapter *adapter = NULL;

	sss_hold_chip_node();
	list_for_each_entry(node, &g_chip_list, node) {
		list_for_each_entry(adapter, &node->func_list, node) {
			if (sss_get_func_type(adapter->hwdev) != SSS_FUNC_TYPE_VF) {
				sss_put_chip_node();
				return true;
			}
		}
	}
	sss_put_chip_node();

	return false;
}

void sss_get_all_chip_id(void *id_info)
{
	int i = 0;
	int id;
	int ret;
	struct sss_card_id *card_id = (struct sss_card_id *)id_info;
	struct sss_card_node *node = NULL;

	sss_hold_chip_node();
	list_for_each_entry(node, &g_chip_list, node) {
		ret = sscanf(node->chip_name, SSS_CHIP_NAME "%d", &id);
		if (ret < 0) {
			pr_err("Fail to get chip id\n");
			continue;
		}
		card_id->id[i] = (u32)id;
		i++;
	}
	sss_put_chip_node();

	card_id->num = (u32)i;
}

void *sss_get_pcidev_hdl(void *hwdev)
{
	struct sss_hwdev *dev = (struct sss_hwdev *)hwdev;

	if (!hwdev)
		return NULL;

	return dev->pcidev_hdl;
}

struct sss_card_node *sss_get_card_node(struct sss_hal_dev *hal_dev)
{
	struct sss_pci_adapter *adapter = pci_get_drvdata(hal_dev->pdev);

	return adapter->chip_node;
}

void sss_get_card_func_info(const char *chip_name, struct sss_card_func_info *card_func)
{
	struct sss_card_node *card_node = NULL;
	struct sss_pci_adapter *adapter = NULL;
	struct sss_func_pdev_info *info = NULL;

	card_func->pf_num = 0;

	sss_hold_chip_node();

	list_for_each_entry(card_node, &g_chip_list, node) {
		if (strncmp(card_node->chip_name, chip_name, IFNAMSIZ))
			continue;

		list_for_each_entry(adapter, &card_node->func_list, node) {
			if (sss_get_func_type(adapter->hwdev) == SSS_FUNC_TYPE_VF)
				continue;

			info = &card_func->pdev_info[card_func->pf_num];
			info->bar1_size =
				pci_resource_len(adapter->pcidev, SSS_PF_PCI_CFG_REG_BAR);
			info->bar1_pa =
				pci_resource_start(adapter->pcidev, SSS_PF_PCI_CFG_REG_BAR);

			info->bar3_size =
				pci_resource_len(adapter->pcidev, SSS_PCI_MGMT_REG_BAR);
			info->bar3_pa =
				pci_resource_start(adapter->pcidev, SSS_PCI_MGMT_REG_BAR);

			card_func->pf_num++;
			if (card_func->pf_num >= SSS_PF_NUM_MAX) {
				sss_put_chip_node();
				return;
			}
		}
	}

	sss_put_chip_node();
}

int sss_get_pf_id(struct sss_card_node *card_node, u32 port_id, u32 *pf_id, u32 *valid)
{
	struct sss_pci_adapter *adapter = NULL;

	sss_hold_chip_node();
	list_for_each_entry(adapter, &card_node->func_list, node) {
		if (sss_get_func_type(adapter->hwdev) == SSS_FUNC_TYPE_VF)
			continue;

		if (SSS_TO_PHY_PORT_ID(adapter->hwdev) == port_id) {
			*pf_id = sss_get_func_id(adapter->hwdev);
			*valid = 1;
			break;
		}
	}
	sss_put_chip_node();

	return 0;
}

void *sss_get_uld_dev(struct sss_hal_dev *hal_dev, enum sss_service_type type)
{
	struct sss_pci_adapter *dev = NULL;
	void *uld = NULL;

	if (!hal_dev)
		return NULL;

	dev = pci_get_drvdata(hal_dev->pdev);
	if (!dev)
		return NULL;

	spin_lock_bh(&dev->uld_lock);
	if (!dev->uld_dev[type] || !test_bit(type, &dev->uld_attach_state)) {
		spin_unlock_bh(&dev->uld_lock);
		return NULL;
	}
	uld = dev->uld_dev[type];

	atomic_inc(&dev->uld_ref_cnt[type]);
	spin_unlock_bh(&dev->uld_lock);

	return uld;
}

void sss_uld_dev_put(struct sss_hal_dev *hal_dev, enum sss_service_type type)
{
	struct sss_pci_adapter *pci_adapter = pci_get_drvdata(hal_dev->pdev);

	atomic_dec(&pci_adapter->uld_ref_cnt[type]);
}

static bool sss_is_pcidev_match_dev_name(const char *dev_name, struct sss_pci_adapter *dev,
					 enum sss_service_type type)
{
	enum sss_service_type i;
	char nic_uld_name[IFNAMSIZ] = {0};
	int err;

	if (type > SSS_SERVICE_TYPE_MAX)
		return false;

	if (type == SSS_SERVICE_TYPE_MAX) {
		for (i = SSS_SERVICE_TYPE_OVS; i < SSS_SERVICE_TYPE_MAX; i++) {
			if (!strncmp(dev->uld_dev_name[i], dev_name, IFNAMSIZ))
				return true;
		}
	} else {
		if (!strncmp(dev->uld_dev_name[type], dev_name, IFNAMSIZ))
			return true;
	}

	err = sss_get_dynamic_uld_dev_name(dev, SSS_SERVICE_TYPE_NIC, (char *)nic_uld_name);
	if (err == 0) {
		if (!strncmp(nic_uld_name, dev_name, IFNAMSIZ))
			return true;
	}

	return false;
}

struct sss_hal_dev *sss_get_lld_dev_by_dev_name(const char *dev_name, enum sss_service_type type)
{
	struct sss_card_node *chip_node = NULL;
	struct sss_pci_adapter *dev = NULL;

	sss_hold_chip_node();

	list_for_each_entry(chip_node, &g_chip_list, node) {
		list_for_each_entry(dev, &chip_node->func_list, node) {
			if (sss_is_pcidev_match_dev_name(dev_name, dev, type)) {
				lld_dev_hold(&dev->hal_dev);
				sss_put_chip_node();
				return &dev->hal_dev;
			}
		}
	}

	sss_put_chip_node();

	return NULL;
}

static bool sss_is_pcidev_match_chip_name(const char *ifname, struct sss_pci_adapter *dev,
					  struct sss_card_node *chip_node, enum sss_func_type type)
{
	if (!strncmp(chip_node->chip_name, ifname, IFNAMSIZ)) {
		if (sss_get_func_type(dev->hwdev) != type)
			return false;
		return true;
	}

	return false;
}

static struct sss_hal_dev *sss_get_dst_type_lld_dev_by_chip_name(const char *ifname,
								 enum sss_func_type type)
{
	struct sss_card_node *chip_node = NULL;
	struct sss_pci_adapter *dev = NULL;

	list_for_each_entry(chip_node, &g_chip_list, node) {
		list_for_each_entry(dev, &chip_node->func_list, node) {
			if (sss_is_pcidev_match_chip_name(ifname, dev, chip_node, type))
				return &dev->hal_dev;
		}
	}

	return NULL;
}

struct sss_hal_dev *sss_get_lld_dev_by_chip_name(const char *chip_name)
{
	struct sss_hal_dev *dev = NULL;

	sss_hold_chip_node();

	dev = sss_get_dst_type_lld_dev_by_chip_name(chip_name, SSS_FUNC_TYPE_PPF);
	if (dev)
		goto out;

	dev = sss_get_dst_type_lld_dev_by_chip_name(chip_name, SSS_FUNC_TYPE_PF);
	if (dev)
		goto out;

	dev = sss_get_dst_type_lld_dev_by_chip_name(chip_name, SSS_FUNC_TYPE_VF);
out:
	if (dev)
		lld_dev_hold(dev);
	sss_put_chip_node();

	return dev;
}

struct sss_hal_dev *sss_get_lld_dev_by_chip_and_port(const char *chip_name, u8 port_id)
{
	struct sss_card_node *chip_node = NULL;
	struct sss_pci_adapter *dev = NULL;

	sss_hold_chip_node();
	list_for_each_entry(chip_node, &g_chip_list, node) {
		list_for_each_entry(dev, &chip_node->func_list, node) {
			if (sss_get_func_type(dev->hwdev) == SSS_FUNC_TYPE_VF)
				continue;

			if (SSS_TO_PHY_PORT_ID(dev->hwdev) == port_id &&
			    !strncmp(chip_node->chip_name, chip_name, IFNAMSIZ)) {
				lld_dev_hold(&dev->hal_dev);
				sss_put_chip_node();

				return &dev->hal_dev;
			}
		}
	}
	sss_put_chip_node();

	return NULL;
}
