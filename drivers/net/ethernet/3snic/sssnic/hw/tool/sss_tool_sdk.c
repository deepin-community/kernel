// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */
#define pr_fmt(fmt) KBUILD_MODNAME ": [TOOL]" fmt

#include <net/sock.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/time.h>

#include "sss_linux_kernel.h"
#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_tool.h"
#include "sss_csr.h"
#include "sss_adapter_mgmt.h"
#include "sss_mgmt_info.h"
#include "sss_pci_global.h"
#include "sss_hwif_api.h"

typedef int (*sss_tool_hw_cmd_func)(struct sss_hal_dev *hal_dev, const void *buf_in,
				    u32 in_size, void *buf_out, u32 *out_size);
struct sss_tool_hw_cmd_handle {
	enum sss_tool_driver_cmd_type	cmd_type;
	sss_tool_hw_cmd_func		func;
};

int sss_tool_get_func_type(struct sss_hal_dev *hal_dev, const void *buf_in, u32 in_size,
			   void *buf_out, u32 *out_size)
{
	if (*out_size != sizeof(u16) || !buf_out) {
		tool_err("Invalid out_size from user :%u, expect: %lu\n", *out_size, sizeof(u16));
		return -EFAULT;
	}

	*(u16 *)buf_out = (u16)sss_get_func_type(SSS_TO_HWDEV(hal_dev));

	return 0;
}

int sss_tool_get_func_id(struct sss_hal_dev *hal_dev, const void *buf_in, u32 in_size,
			 void *buf_out, u32 *out_size)
{
	if (*out_size != sizeof(u16) || !buf_out) {
		tool_err("Invalid out_size from user :%u, expect: %lu\n", *out_size, sizeof(u16));
		return -EFAULT;
	}

	*(u16 *)buf_out = (u16)sss_get_func_id(SSS_TO_HWDEV(hal_dev));

	return 0;
}

int sss_tool_get_hw_driver_stats(struct sss_hal_dev *hal_dev, const void *buf_in, u32 in_size,
				 void *buf_out, u32 *out_size)
{
	struct sss_hwdev *hwdev = hal_dev->hwdev;
	struct sss_card_node *node = hwdev->chip_node;
	struct sss_hw_stats *stats = buf_out;
	struct sss_hw_stats *tmp = stats;

	if (!hwdev)
		return -EINVAL;

	if (*out_size != sizeof(struct sss_hw_stats) || !stats) {
		tool_err("Invalid out_size from user :%u, expect: %lu\n",
			 *out_size, sizeof(struct sss_hw_stats));
		return -EFAULT;
	}

	memcpy(stats, &hwdev->hw_stats, sizeof(struct sss_hw_stats));

	atomic_set(&tmp->nic_ucode_event_stats[SSS_CHN_BUSY],
		   atomic_read(&node->channel_timeout_cnt));

	return 0;
}

static int sss_tool_clear_hw_driver_stats(struct sss_hal_dev *hal_dev, const void *buf_in,
					  u32 in_size, void *buf_out, u32 *out_size)
{
	struct sss_hwdev *hwdev = hal_dev->hwdev;
	struct sss_card_node *node = hwdev->chip_node;

	memset((void *)&hwdev->hw_stats, 0, sizeof(struct sss_hw_stats));
	memset((void *)hwdev->chip_fault_stats, 0, SSS_TOOL_CHIP_FAULT_SIZE);

	if (SSS_SUPPORT_CHANNEL_DETECT(hwdev) && atomic_read(&node->channel_timeout_cnt)) {
		atomic_set(&node->channel_timeout_cnt, 0);
		hwdev->aeq_busy_cnt = 0;
#if !defined(__UEFI__) && !defined(VMWARE)
		queue_delayed_work(hwdev->workq, &hwdev->channel_detect_task,
				   msecs_to_jiffies(SSSNIC_CHANNEL_DETECT_PERIOD));
#endif
	}

	if (*out_size != sizeof(struct sss_hw_stats)) {
		tool_err("Invalid out_size from user :%u, expect: %lu\n",
			 *out_size, sizeof(struct sss_hw_stats));
		return -EFAULT;
	}

	return 0;
}

static int sss_tool_get_self_test_result(struct sss_hal_dev *hal_dev,
					 const void *buf_in, u32 in_size,
					 void *buf_out, u32 *out_size)
{
	u32 val;

	if (*out_size != sizeof(u32) || !buf_out) {
		tool_err("Invalid out_size from user :%u, expect: %lu\n",
			 *out_size, sizeof(u32));
		return -EFAULT;
	}

	val = sss_chip_read_reg(SSS_TO_HWIF(hal_dev->hwdev), SSS_MGMT_HEALTH_STATUS_ADDR);
	*(u32 *)buf_out = val;

	return 0;
}

static void sss_tool_get_chip_fault_stats(const void *hwdev, u8 *chip_fault_stats, u32 offset)
{
	u32 size;

	if (offset >= SSS_TOOL_CHIP_FAULT_SIZE) {
		tool_err("Invalid chip offset value: %d\n", offset);
		return;
	}

	size = min(SSS_TOOL_DRV_BUF_SIZE_MAX, SSS_TOOL_CHIP_FAULT_SIZE - (int)offset);
	memcpy(chip_fault_stats, ((struct sss_hwdev *)hwdev)->chip_fault_stats
	       + offset, size);
}

static int sss_tool_get_chip_faults_stats(struct sss_hal_dev *hal_dev,
					  const void *buf_in, u32 in_size,
					  void *buf_out, u32 *out_size)
{
	u32 offset = 0;
	struct sss_tool_cmd_chip_fault_stats *info = NULL;

	if (!buf_in || !buf_out || *out_size != sizeof(*info) ||
	    in_size != sizeof(*info)) {
		tool_err("Invalid out_size from user: %d, expect: %lu\n", *out_size, sizeof(*info));
		return -EFAULT;
	}
	info = (struct sss_tool_cmd_chip_fault_stats *)buf_in;
	offset = info->offset;

	info = (struct sss_tool_cmd_chip_fault_stats *)buf_out;
	sss_tool_get_chip_fault_stats(hal_dev->hwdev,
				      info->chip_fault_stats, offset);

	return 0;
}

static int sss_tool_get_single_card_info(struct sss_hal_dev *hal_dev, const void *buf_in,
					 u32 in_size, void *buf_out, u32 *out_size)
{
	if (!buf_out || *out_size != sizeof(struct sss_tool_card_info)) {
		tool_err("Invalid buf out is NULL, or out_size != %lu\n",
			 sizeof(struct sss_tool_card_info));
		return -EINVAL;
	}

	sss_get_card_info(hal_dev->hwdev, buf_out);

	return 0;
}

static int sss_tool_is_driver_in_vm(struct sss_hal_dev *hal_dev,
				    const void *buf_in, u32 in_size,
				    void *buf_out, u32 *out_size)
{
	if (!buf_out || (*out_size != sizeof(u8))) {
		tool_err("Invalid parameter, buf_out is NULL or out_size != %lu\n", sizeof(u8));
		return -EINVAL;
	}

	*((u8 *)buf_out) = sss_is_in_host() ? 0 : 1;

	return 0;
}

static int sss_tool_get_all_chip_id_cmd(struct sss_hal_dev *hal_dev,
					const void *buf_in, u32 in_size,
					void *buf_out, u32 *out_size)
{
	if (*out_size != sizeof(struct sss_card_id) || !buf_out) {
		tool_err("Invalid parameter: out_size %u, expect %lu\n",
			 *out_size, sizeof(struct sss_card_id));
		return -EFAULT;
	}

	sss_get_all_chip_id(buf_out);

	return 0;
}

static int sss_tool_get_card_id(char *dev_name, int *id)
{
	int ret;

	ret = sscanf(dev_name, SSS_CHIP_NAME "%d", id);
	if (ret < 0) {
		tool_err("Fail to get card id\n");
		return ret;
	}

	if (*id >= SSS_TOOL_CARD_MAX || *id < 0) {
		tool_err("Invalid chip id %d, out of range: [0-%d]\n",  *id, SSS_TOOL_CARD_MAX - 1);
		return -EINVAL;
	}

	return 0;
}

static void sss_tool_get_pf_dev_info_param(struct sss_tool_pf_dev_info *dev_info, int card_id,
					   void **func_array)
{
	u32 func_id;
	void *hwdev = NULL;
	struct pci_dev *pdev = NULL;

	for (func_id = 0; func_id < SSS_TOOL_PF_DEV_MAX; func_id++) {
		hwdev = (void *)func_array[func_id];

		dev_info[func_id].phy_addr = g_card_pa[card_id];

		if (!hwdev) {
			dev_info[func_id].bar0_size = 0;
			dev_info[func_id].bus = 0;
			dev_info[func_id].slot = 0;
			dev_info[func_id].func = 0;
		} else {
			pdev = (struct pci_dev *)sss_get_pcidev_hdl(hwdev);
			dev_info[func_id].bar0_size = pci_resource_len(pdev, 0);
			dev_info[func_id].bus = pdev->bus->number;
			dev_info[func_id].slot = PCI_SLOT(pdev->devfn);
			dev_info[func_id].func = PCI_FUNC(pdev->devfn);
		}
	}
}

static int sss_tool_get_card_adm_mem(int card_id)
{
	int i;
	unsigned char *card_va = NULL;

	g_card_id = card_id;
	if (!g_card_va[card_id]) {
		g_card_va[card_id] =
			(void *)__get_free_pages(GFP_KERNEL, SSS_TOOL_PAGE_ORDER);
		if (!g_card_va[card_id]) {
			tool_err("Fail to alloc adm memory for card %d!\n", card_id);
			return -EFAULT;
		}

		memset(g_card_va[card_id], 0, PAGE_SIZE * (1 << SSS_TOOL_PAGE_ORDER));

		g_card_pa[card_id] = virt_to_phys(g_card_va[card_id]);
		if (!g_card_pa[card_id]) {
			tool_err("Invalid phy addr for card %d is 0\n", card_id);
			free_pages((unsigned long)g_card_va[card_id], SSS_TOOL_PAGE_ORDER);
			g_card_va[card_id] = NULL;
			return -EFAULT;
		}

		card_va = g_card_va[card_id];
		for (i = 0; i < (1 << SSS_TOOL_PAGE_ORDER); i++) {
			SetPageReserved(virt_to_page(card_va));
			card_va += PAGE_SIZE;
		}
	}

	return 0;
}

static int sss_tool_get_pf_dev_info(struct sss_hal_dev *hal_dev, const void *buf_in, u32 in_size,
				    void *buf_out, u32 *out_size)
{
	int id;
	int ret;
	struct sss_tool_pf_dev_info *info = buf_out;
	struct sss_card_node *node = sss_get_card_node(hal_dev);

	if (!buf_out || *out_size != sizeof(struct sss_tool_pf_dev_info) * SSS_TOOL_PF_DEV_MAX) {
		tool_err("Invalid param: out_size %u, expect %lu\n",
			 *out_size, sizeof(info) * SSS_TOOL_PF_DEV_MAX);
		return -EFAULT;
	}

	ret = sss_tool_get_card_id(node->chip_name, &id);
	if (ret)
		return ret;

	sss_tool_get_pf_dev_info_param(info, id, node->func_handle_array);

	ret = sss_tool_get_card_adm_mem(id);
	if (ret) {
		tool_err("Fail to get adm memory for userspace %s\n", node->chip_name);
		return -EFAULT;
	}

	return 0;
}

long sss_tool_free_card_mem(int id)
{
	unsigned char *va = NULL;
	int i;

	if (!g_card_va[id])
		return 0;

	va = g_card_va[id];
	for (i = 0; i < (1 << SSS_TOOL_PAGE_ORDER); i++) {
		ClearPageReserved(virt_to_page(va));
		va += PAGE_SIZE;
	}

	free_pages((unsigned long)g_card_va[id], SSS_TOOL_PAGE_ORDER);
	g_card_va[id] = NULL;
	g_card_pa[id] = 0;

	return 0;
}

static int sss_tool_free_all_card_mem(struct sss_hal_dev *hal_dev, const void *buf_in, u32 in_size,
				      void *buf_out, u32 *out_size)
{
	int id;
	int ret;
	struct sss_card_node *node = sss_get_card_node(hal_dev);

	ret = sss_tool_get_card_id(node->chip_name, &id);
	if (ret)
		return ret;

	sss_tool_free_card_mem(id);

	return 0;
}

static int sss_tool_check_card_info_param(char *dev_name, const void *buf_out, u32 out_size)
{
	int ret;

	if (!buf_out || out_size != sizeof(struct sss_card_func_info)) {
		tool_err("Invalid out_size %u, expect %lu\n",
			 out_size, sizeof(struct sss_card_func_info));
		return -EINVAL;
	}

	ret = memcmp(dev_name, SSS_CHIP_NAME, strlen(SSS_CHIP_NAME));
	if (ret) {
		tool_err("Invalid chip name %s\n", dev_name);
		return ret;
	}

	return 0;
}

static int sss_tool_get_card_func_info(struct sss_hal_dev *hal_dev, const void *buf_in, u32 in_size,
				       void *buf_out, u32 *out_size)
{
	int ret;
	int id = 0;
	struct sss_card_func_info *info = buf_out;
	struct sss_card_node *node = sss_get_card_node(hal_dev);

	ret = sss_tool_check_card_info_param(node->chip_name, buf_out, *out_size);
	if (ret)
		return ret;

	ret = sss_tool_get_card_id(node->chip_name, &id);
	if (ret)
		return ret;

	sss_get_card_func_info(node->chip_name, info);

	if (!info->pf_num) {
		tool_err("Fail to get card func info, chip name %s\n", node->chip_name);
		return -EFAULT;
	}

	ret = sss_tool_get_card_adm_mem(id);
	if (ret) {
		tool_err("Fail to get adm memory for userspace %s\n", node->chip_name);
		return -EFAULT;
	}

	info->usr_adm_pa = g_card_pa[id];

	return 0;
}

static int sss_tool_get_pf_cap_info(struct sss_hal_dev *hal_dev, const void *buf_in, u32 in_size,
				    void *buf_out, u32 *out_size)
{
	struct sss_hwdev *hwdev = NULL;
	struct sss_card_node *node = sss_get_card_node(hal_dev);
	struct sss_svc_cap_info *in_info = (struct sss_svc_cap_info *)buf_in;
	struct sss_svc_cap_info *out_info = (struct sss_svc_cap_info *)buf_out;

	if (*out_size != sizeof(struct sss_svc_cap_info) ||
	    in_size != sizeof(struct sss_svc_cap_info) ||
	    !buf_in || !buf_out) {
		tool_err("Invalid out_size %u, in_size: %u, expect %lu\n",
			 *out_size, in_size, sizeof(struct sss_svc_cap_info));
		return -EINVAL;
	}

	if (in_info->func_id >= SSS_MAX_FUNC) {
		tool_err("Invalid func id: %u, max_num: %u\n",
			 in_info->func_id, SSS_MAX_FUNC);
		return -EINVAL;
	}

	sss_hold_chip_node();
	hwdev = (struct sss_hwdev *)(node->func_handle_array)[in_info->func_id];
	if (!hwdev) {
		sss_put_chip_node();
		return -EINVAL;
	}

	memcpy(&out_info->cap, SSS_TO_SVC_CAP(hwdev), sizeof(struct sss_service_cap));
	sss_put_chip_node();

	return 0;
}

static int sss_tool_get_hw_drv_version(struct sss_hal_dev *hal_dev, const void *buf_in, u32 in_size,
				       void *buf_out, u32 *out_size)
{
	int ret;
	struct sss_tool_drv_version_info *info = buf_out;

	if (!buf_out || *out_size != sizeof(*info)) {
		tool_err("Invalid param, buf_out is NULL or out_size:%u, expect: %lu\n",
			 *out_size, sizeof(*info));
		return -EINVAL;
	}

	ret = snprintf(info->ver, sizeof(info->ver), "%s  %s", SSS_DRV_VERSION,
		       __TIME_STR__);
	if (ret < 0)
		return -EINVAL;

	return 0;
}

static int sss_tool_get_pf_id(struct sss_hal_dev *hal_dev, const void *buf_in, u32 in_size,
			      void *buf_out, u32 *out_size)
{
	struct sss_tool_pf_info *info = NULL;
	struct sss_card_node *node = sss_get_card_node(hal_dev);
	u32 port_id;
	int ret;

	if (!node)
		return -ENODEV;

	if (!buf_out || (*out_size != sizeof(*info)) || !buf_in || in_size != sizeof(port_id)) {
		tool_err("Invalid out_size from user: %u, expect: %lu, in_size:%u\n",
			 *out_size, sizeof(*info), in_size);
		return -EINVAL;
	}

	port_id = *((u32 *)buf_in);
	info = (struct sss_tool_pf_info *)buf_out;

	ret = sss_get_pf_id(node, port_id, &info->pf_id, &info->valid);
	if (ret != 0)
		return ret;

	*out_size = sizeof(*info);

	return 0;
}

struct sss_tool_hw_cmd_handle g_hw_cmd_handle[] = {
	{SSS_TOOL_FUNC_TYPE,		sss_tool_get_func_type},
	{SSS_TOOL_GET_FUNC_IDX,		sss_tool_get_func_id},
	{SSS_TOOL_GET_CHIP_INFO,	sss_tool_get_card_func_info},
	{SSS_TOOL_GET_DRV_VERSION,	sss_tool_get_hw_drv_version},
	{SSS_TOOL_GET_PF_ID,		sss_tool_get_pf_id},
	{SSS_TOOL_GET_FUNC_CAP,		sss_tool_get_pf_cap_info},
	{SSS_TOOL_GET_SELF_TEST_RES,	sss_tool_get_self_test_result},
	{SSS_TOOL_GET_CHIP_ID,		sss_tool_get_all_chip_id_cmd},
	{SSS_TOOL_GET_PF_DEV_INFO,	sss_tool_get_pf_dev_info},
	{SSS_TOOL_IS_DRV_IN_VM,		sss_tool_is_driver_in_vm},
	{SSS_TOOL_CMD_FREE_MEM,		sss_tool_free_all_card_mem},
	{SSS_TOOL_GET_CHIP_FAULT_STATS,	(sss_tool_hw_cmd_func)sss_tool_get_chip_faults_stats},
	{SSS_TOOL_GET_SINGLE_CARD_INFO,	(sss_tool_hw_cmd_func)sss_tool_get_single_card_info},
	{SSS_TOOL_GET_HW_STATS,		(sss_tool_hw_cmd_func)sss_tool_get_hw_driver_stats},
	{SSS_TOOL_CLEAR_HW_STATS,	sss_tool_clear_hw_driver_stats},
};

int sss_tool_msg_to_hw(struct sss_hal_dev *hal_dev, struct sss_tool_msg *tool_msg,
		       void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
	int id;
	int ret = 0;
	int cmd_num = ARRAY_LEN(g_hw_cmd_handle);
	enum sss_tool_driver_cmd_type cmd =
				(enum sss_tool_driver_cmd_type)(tool_msg->msg_formate);

	for (id = 0; id < cmd_num; id++) {
		if (cmd == g_hw_cmd_handle[id].cmd_type) {
			ret = g_hw_cmd_handle[id].func
			      (hal_dev, buf_in, in_size, buf_out, out_size);
			break;
		}
	}

	if (id == cmd_num) {
		tool_err("Fail to send msg to hw, cmd: %d out of range\n", cmd);
		return -EINVAL;
	}

	return ret;
}
