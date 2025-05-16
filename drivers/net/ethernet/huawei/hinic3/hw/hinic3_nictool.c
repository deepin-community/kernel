// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [COMM]" fmt

#include <net/sock.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/pci.h>

#include "ossl_knl.h"
#include "hinic3_mt.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_hw_cfg.h"
#include "hinic3_hwdev.h"
#include "hinic3_lld.h"
#include "hinic3_hw_mt.h"
#include "hinic3_dev_mgmt.h"
#include "hinic3_nictool.h"

static int g_nictool_ref_cnt;

static dev_t g_dev_id;
static struct class *g_nictool_class;
static struct cdev g_nictool_cdev;

#define HINIC3_MAX_BUF_SIZE (2048 * 1024)

void *g_card_node_array[MAX_CARD_NUM];
void *g_card_vir_addr[MAX_CARD_NUM];
u64 g_card_phy_addr[MAX_CARD_NUM];
int card_id;

#define HIADM3_DEV_PATH		"/dev/hinic3_nictool_dev"
#define HIADM3_DEV_CLASS	"hinic3_nictool_class"
#define HIADM3_DEV_NAME		"hinic3_nictool_dev"

typedef int (*hw_driv_module)(struct hinic3_lld_dev *lld_dev, const void *buf_in,
			      u32 in_size, void *buf_out, u32 *out_size);

struct hw_drv_module_handle {
	enum driver_cmd_type	driv_cmd_name;
	hw_driv_module		driv_func;
};

static int get_single_card_info(struct hinic3_lld_dev *lld_dev, const void *buf_in,
				u32 in_size, void *buf_out, u32 *out_size)
{
	if (!buf_out || *out_size != sizeof(struct card_info)) {
		pr_err("buf_out is NULL, or out_size != %lu\n", sizeof(struct card_info));
		return -EINVAL;
	}

	hinic3_get_card_info(hinic3_get_sdk_hwdev_by_lld(lld_dev), buf_out);

	return 0;
}

static int is_driver_in_vm(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			   void *buf_out, u32 *out_size)
{
	bool in_host = false;

	if (!buf_out || (*out_size != sizeof(u8))) {
		pr_err("buf_out is NULL, or out_size != %lu\n", sizeof(u8));
		return -EINVAL;
	}

	in_host = hinic3_is_in_host();
	if (in_host)
		*((u8 *)buf_out) = 0;
	else
		*((u8 *)buf_out) = 1;

	return 0;
}

static int get_all_chip_id_cmd(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			       void *buf_out, u32 *out_size)
{
	if (*out_size != sizeof(struct nic_card_id) || !buf_out) {
		pr_err("Invalid parameter: out_buf_size %u, expect %lu\n",
		       *out_size, sizeof(struct nic_card_id));
		return -EFAULT;
	}

	hinic3_get_all_chip_id(buf_out);

	return 0;
}

#ifndef __HIFC__
static int get_os_hot_replace_info(struct hinic3_lld_dev *lld_dev,
				   const void *buf_in, u32 in_size,
				   void *buf_out, u32 *out_size)
{
	if (*out_size != sizeof(struct os_hot_replace_info) || !buf_out) {
		pr_err("Invalid parameter: out_buf_size %u, expect %lu\n",
		       *out_size, sizeof(struct os_hot_replace_info));
		return -EFAULT;
	}

	hinic3_get_os_hot_replace_info(buf_out);

	return 0;
}
#endif

static int get_card_usr_api_chain_mem(int card_idx)
{
	unsigned char *tmp = NULL;
	int i;

	card_id = card_idx;
	if (!g_card_vir_addr[card_idx]) {
		g_card_vir_addr[card_idx] =
			(void *)__get_free_pages(GFP_KERNEL,
						 DBGTOOL_PAGE_ORDER);
		if (!g_card_vir_addr[card_idx]) {
			pr_err("Alloc api chain memory fail for card %d!\n", card_idx);
			return -EFAULT;
		}

		memset(g_card_vir_addr[card_idx], 0,
		       PAGE_SIZE * (1 << DBGTOOL_PAGE_ORDER));

		g_card_phy_addr[card_idx] =
			virt_to_phys(g_card_vir_addr[card_idx]);
		if (!g_card_phy_addr[card_idx]) {
			pr_err("phy addr for card %d is 0\n", card_idx);
			free_pages((unsigned long)g_card_vir_addr[card_idx], DBGTOOL_PAGE_ORDER);
			g_card_vir_addr[card_idx] = NULL;
			return -EFAULT;
		}

		tmp = g_card_vir_addr[card_idx];
		for (i = 0; i < (1 << DBGTOOL_PAGE_ORDER); i++) {
			SetPageReserved(virt_to_page(tmp));
			tmp += PAGE_SIZE;
		}
	}

	return 0;
}

static void chipif_get_all_pf_dev_info(struct pf_dev_info *dev_info, int card_idx,
				       void **g_func_handle_array)
{
	u32 func_idx;
	void *hwdev = NULL;
	struct pci_dev *pdev = NULL;

	for (func_idx = 0; func_idx < PF_DEV_INFO_NUM; func_idx++) {
		hwdev = (void *)g_func_handle_array[func_idx];

		dev_info[func_idx].phy_addr = g_card_phy_addr[card_idx];

		if (!hwdev) {
			dev_info[func_idx].bar0_size = 0;
			dev_info[func_idx].bus = 0;
			dev_info[func_idx].slot = 0;
			dev_info[func_idx].func = 0;
		} else {
			pdev = (struct pci_dev *)hinic3_get_pcidev_hdl(hwdev);
			dev_info[func_idx].bar0_size =
					pci_resource_len(pdev, 0);
			dev_info[func_idx].bus = pdev->bus->number;
			dev_info[func_idx].slot = PCI_SLOT(pdev->devfn);
			dev_info[func_idx].func = PCI_FUNC(pdev->devfn);
		}
	}
}

static int get_pf_dev_info(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			   void *buf_out, u32 *out_size)
{
	struct pf_dev_info *dev_info = buf_out;
	struct card_node *card_info = hinic3_get_chip_node_by_lld(lld_dev);
	int id, err;

	if (!buf_out || *out_size != sizeof(struct pf_dev_info) * PF_DEV_INFO_NUM) {
		pr_err("Invalid parameter: out_buf_size %u, expect %lu\n",
		       *out_size, sizeof(*dev_info) * PF_DEV_INFO_NUM);
		return -EFAULT;
	}

	err = sscanf(card_info->chip_name, HINIC3_CHIP_NAME "%d", &id);
	if (err < 0) {
		pr_err("Failed to get card id\n");
		return err;
	}

	if (id >= MAX_CARD_NUM || id < 0) {
		pr_err("chip id %d exceed limit[0-%d]\n", id, MAX_CARD_NUM - 1);
		return -EINVAL;
	}

	chipif_get_all_pf_dev_info(dev_info, id, card_info->func_handle_array);

	err = get_card_usr_api_chain_mem(id);
	if (err) {
		pr_err("Faile to get api chain memory for userspace %s\n",
		       card_info->chip_name);
		return -EFAULT;
	}

	return 0;
}

static void dbgtool_knl_free_mem(int id)
{
	unsigned char *tmp = NULL;
	int i;

	if (id < 0 || id >= MAX_CARD_NUM) {
		pr_err("Invalid card id\n");
		return;
	}

	if (!g_card_vir_addr[id])
		return;

	tmp = g_card_vir_addr[id];
	for (i = 0; i < (1 << DBGTOOL_PAGE_ORDER); i++) {
		ClearPageReserved(virt_to_page(tmp));
		tmp += PAGE_SIZE;
	}

	free_pages((unsigned long)g_card_vir_addr[id], DBGTOOL_PAGE_ORDER);
	g_card_vir_addr[id] = NULL;
	g_card_phy_addr[id] = 0;

}

static int free_knl_mem(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			void *buf_out, u32 *out_size)
{
	struct card_node *card_info = hinic3_get_chip_node_by_lld(lld_dev);
	int id, err;

	err = sscanf(card_info->chip_name, HINIC3_CHIP_NAME "%d", &id);
	if (err < 0) {
		pr_err("Failed to get card id\n");
		return err;
	}

	if (id >= MAX_CARD_NUM || id < 0) {
		pr_err("chip id %d exceed limit[0-%d]\n", id, MAX_CARD_NUM - 1);
		return -EINVAL;
	}

	dbgtool_knl_free_mem(id);

	return 0;
}

static int card_info_param_valid(char *dev_name, const void *buf_out, u32 buf_out_size, int *id)
{
	int err;

	if (!buf_out || buf_out_size != sizeof(struct hinic3_card_func_info)) {
		pr_err("Invalid parameter: out_buf_size %u, expect %lu\n",
		       buf_out_size, sizeof(struct hinic3_card_func_info));
		return -EINVAL;
	}

	err = memcmp(dev_name, HINIC3_CHIP_NAME, strlen(HINIC3_CHIP_NAME));
	if (err) {
		pr_err("Invalid chip name %s\n", dev_name);
		return err;
	}

	err = sscanf(dev_name, HINIC3_CHIP_NAME "%d", id);
	if (err < 0) {
		pr_err("Failed to get card id\n");
		return err;
	}

	if (*id >= MAX_CARD_NUM || *id < 0) {
		pr_err("chip id %d exceed limit[0-%d]\n",
		       *id, MAX_CARD_NUM - 1);
		return -EINVAL;
	}

	return 0;
}

static int get_card_func_info(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			      void *buf_out, u32 *out_size)
{
	struct hinic3_card_func_info *card_func_info = buf_out;
	struct card_node *card_info = hinic3_get_chip_node_by_lld(lld_dev);
	int err, id = 0;

	err = card_info_param_valid(card_info->chip_name, buf_out, *out_size, &id);
	if (err)
		return err;

	hinic3_get_card_func_info_by_card_name(card_info->chip_name, card_func_info);

	if (!card_func_info->num_pf) {
		pr_err("None function found for %s\n", card_info->chip_name);
		return -EFAULT;
	}

	err = get_card_usr_api_chain_mem(id);
	if (err) {
		pr_err("Faile to get api chain memory for userspace %s\n",
		       card_info->chip_name);
		return -EFAULT;
	}

	card_func_info->usr_api_phy_addr = g_card_phy_addr[id];

	return 0;
}

static int get_pf_cap_info(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			   void *buf_out, u32 *out_size)
{
	struct service_cap *func_cap = NULL;
	struct hinic3_hwdev *hwdev = NULL;
	struct card_node *card_info = hinic3_get_chip_node_by_lld(lld_dev);
	struct svc_cap_info *svc_cap_info_in = (struct svc_cap_info *)buf_in;
	struct svc_cap_info *svc_cap_info_out = (struct svc_cap_info *)buf_out;

	if (*out_size != sizeof(struct svc_cap_info) || in_size != sizeof(struct svc_cap_info) ||
	    !buf_in || !buf_out) {
		pr_err("Invalid parameter: out_buf_size %u, in_size: %u, expect %lu\n",
		       *out_size, in_size, sizeof(struct svc_cap_info));
		return -EINVAL;
	}

	if (svc_cap_info_in->func_idx >= MAX_FUNCTION_NUM) {
		pr_err("func_idx is illegal. func_idx: %u, max_num: %u\n",
		       svc_cap_info_in->func_idx, MAX_FUNCTION_NUM);
		return -EINVAL;
	}

	lld_hold();
	hwdev = (struct hinic3_hwdev *)(card_info->func_handle_array)[svc_cap_info_in->func_idx];
	if (!hwdev) {
		lld_put();
		return -EINVAL;
	}

	func_cap = &hwdev->cfg_mgmt->svc_cap;
	memcpy(&svc_cap_info_out->cap, func_cap, sizeof(struct service_cap));
	lld_put();

	return 0;
}

static int get_hw_drv_version(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			      void *buf_out, u32 *out_size)
{
	struct drv_version_info *ver_info = buf_out;

	if (!buf_out) {
		pr_err("Buf_out is NULL.\n");
		return -EINVAL;
	}

	if (*out_size != sizeof(*ver_info)) {
		pr_err("Unexpect out buf size from user :%u, expect: %lu\n",
		       *out_size, sizeof(*ver_info));
		return -EINVAL;
	}

	snprintf(ver_info->ver, sizeof(ver_info->ver), "%s  %s", HINIC3_DRV_VERSION,
		 "2025-05-08_00:00:08");

	return 0;
}

static int get_pf_id(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
		     void *buf_out, u32 *out_size)
{
	struct hinic3_pf_info *pf_info = NULL;
	struct card_node *chip_node = hinic3_get_chip_node_by_lld(lld_dev);
	u32 port_id;
	int err;

	if (!chip_node)
		return -ENODEV;

	if (!buf_out || (*out_size != sizeof(*pf_info)) || !buf_in || in_size != sizeof(u32)) {
		pr_err("Unexpect out buf size from user :%u, expect: %lu, in size:%u\n",
		       *out_size, sizeof(*pf_info), in_size);
		return -EINVAL;
	}

	port_id = *((u32 *)buf_in);
	pf_info = (struct hinic3_pf_info *)buf_out;
	err = hinic3_get_pf_id(chip_node, port_id, &pf_info->pf_id, &pf_info->isvalid);
	if (err)
		return err;

	*out_size = sizeof(*pf_info);

	return 0;
}

#ifndef __HIFC__
/* not support fc yet */
static int get_mbox_cnt(struct hinic3_lld_dev *lld_dev, const void *buf_in,
				u32 in_size, void *buf_out, u32 *out_size)
{
	if (!buf_out || *out_size != sizeof(struct card_mbox_cnt_info)) {
		pr_err("buf_out is NULL, or out_size != %lu\n", sizeof(struct card_info));
		return -EINVAL;
	}

	hinic3_get_mbox_cnt(hinic3_get_sdk_hwdev_by_lld(lld_dev), buf_out);

	return 0;
}
#endif

struct hw_drv_module_handle hinic3_hw_driv_module_cmd_handle[] = {
	{.driv_cmd_name = FUNC_TYPE,		.driv_func = get_func_type},
	{.driv_cmd_name = GET_FUNC_IDX,		.driv_func = get_func_id},
	{.driv_cmd_name = GET_HW_STATS,		.driv_func = (hw_driv_module)get_hw_driver_stats},
	{.driv_cmd_name = CLEAR_HW_STATS,	.driv_func = clear_hw_driver_stats},
	{.driv_cmd_name = GET_SELF_TEST_RES,	.driv_func = get_self_test_result},
	{.driv_cmd_name = GET_CHIP_FAULT_STATS,	.driv_func = (hw_driv_module)get_chip_faults_stats},
	{.driv_cmd_name = GET_SINGLE_CARD_INFO,	.driv_func = (hw_driv_module)get_single_card_info},
	{.driv_cmd_name = IS_DRV_IN_VM,		.driv_func = is_driver_in_vm},
	{.driv_cmd_name = GET_CHIP_ID,		.driv_func = get_all_chip_id_cmd},
	{.driv_cmd_name = GET_PF_DEV_INFO,	.driv_func = get_pf_dev_info},
	{.driv_cmd_name = CMD_FREE_MEM,		.driv_func = free_knl_mem},
	{.driv_cmd_name = GET_CHIP_INFO,	.driv_func = get_card_func_info},
	{.driv_cmd_name = GET_FUNC_CAP,		.driv_func = get_pf_cap_info},
	{.driv_cmd_name = GET_DRV_VERSION,	.driv_func = get_hw_drv_version},
	{.driv_cmd_name = GET_PF_ID,		.driv_func = get_pf_id},
	{.driv_cmd_name = GET_OS_HOT_REPLACE_INFO, .driv_func = get_os_hot_replace_info},
	{.driv_cmd_name = GET_MBOX_CNT,		.driv_func = (hw_driv_module)get_mbox_cnt},
};

static int alloc_tmp_buf(void *hwdev, struct msg_module *nt_msg, u32 in_size,
			 void **buf_in, u32 out_size, void **buf_out)
{
	int ret;

	ret = alloc_buff_in(hwdev, nt_msg, in_size, buf_in);
	if (ret) {
		pr_err("Alloc tool cmd buff in failed\n");
		return ret;
	}

	ret = alloc_buff_out(hwdev, nt_msg, out_size, buf_out);
	if (ret) {
		pr_err("Alloc tool cmd buff out failed\n");
		goto out_free_buf_in;
	}

	return 0;

out_free_buf_in:
	free_buff_in(hwdev, nt_msg, *buf_in);

	return ret;
}

static void free_tmp_buf(void *hwdev, struct msg_module *nt_msg,
			 void *buf_in, void *buf_out)
{
	free_buff_out(hwdev, nt_msg, buf_out);
	free_buff_in(hwdev, nt_msg, buf_in);
}

static int send_to_hw_driver(struct hinic3_lld_dev *lld_dev, struct msg_module *nt_msg,
			     const void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
	int index, num_cmds = (int)(sizeof(hinic3_hw_driv_module_cmd_handle) /
				sizeof(hinic3_hw_driv_module_cmd_handle[0]));
	enum driver_cmd_type cmd_type =
				(enum driver_cmd_type)(nt_msg->msg_formate);
	int err = 0;

	for (index = 0; index < num_cmds; index++) {
		if (cmd_type ==
			hinic3_hw_driv_module_cmd_handle[index].driv_cmd_name) {
			err = hinic3_hw_driv_module_cmd_handle[index].driv_func
					(lld_dev, buf_in, in_size, buf_out, out_size);
			break;
		}
	}

	if (index == num_cmds) {
		pr_err("Can't find callback for %d\n", cmd_type);
		return -EINVAL;
	}

	return err;
}

static int send_to_service_driver(struct hinic3_lld_dev *lld_dev, struct msg_module *nt_msg,
				  const void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
	const char **service_name = NULL;
	enum hinic3_service_type type;
	void *uld_dev = NULL;
	int ret = -EINVAL;

	service_name = hinic3_get_uld_names();
	type = nt_msg->module - SEND_TO_SRV_DRV_BASE;
	if (type >= SERVICE_T_MAX) {
		pr_err("Ioctl input module id: %u is incorrectly\n", nt_msg->module);
		return -EINVAL;
	}

	uld_dev = hinic3_get_uld_dev(lld_dev, type);
	if (!uld_dev) {
		if (nt_msg->msg_formate == GET_DRV_VERSION)
			return 0;

		pr_err("Can not get the uld dev correctly: %s driver may be not register\n",
		       service_name[type]);
		return -EINVAL;
	}

	if (g_hinic3_uld_info[type].ioctl)
		ret = g_hinic3_uld_info[type].ioctl(uld_dev, nt_msg->msg_formate,
					     buf_in, in_size, buf_out, out_size);
	uld_dev_put(lld_dev, type);

	return ret;
}

static int nictool_exec_cmd(struct hinic3_lld_dev *lld_dev, struct msg_module *nt_msg,
			    void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
	int ret = 0;

	switch (nt_msg->module) {
	case SEND_TO_HW_DRIVER:
		ret = send_to_hw_driver(lld_dev, nt_msg, buf_in, in_size, buf_out, out_size);
		break;
	case SEND_TO_MPU:
		ret = send_to_mpu(hinic3_get_sdk_hwdev_by_lld(lld_dev),
				  nt_msg, buf_in, in_size, buf_out, out_size);
		break;
	case SEND_TO_SM:
		ret = send_to_sm(hinic3_get_sdk_hwdev_by_lld(lld_dev),
				 nt_msg, buf_in, in_size, buf_out, out_size);
		break;
	case SEND_TO_NPU:
		ret = send_to_npu(hinic3_get_sdk_hwdev_by_lld(lld_dev),
				  nt_msg, buf_in, in_size, buf_out, out_size);
		break;
	default:
		ret = send_to_service_driver(lld_dev, nt_msg, buf_in, in_size, buf_out, out_size);
		break;
	}

	return ret;
}

static int cmd_parameter_valid(struct msg_module *nt_msg, unsigned long arg,
			       u32 *out_size_expect, u32 *in_size)
{
	if (copy_from_user(nt_msg, (void *)arg, sizeof(*nt_msg))) {
		pr_err("Copy information from user failed\n");
		return -EFAULT;
	}

	*out_size_expect = nt_msg->buf_out_size;
	*in_size = nt_msg->buf_in_size;
	if (*out_size_expect > HINIC3_MAX_BUF_SIZE ||
	    *in_size > HINIC3_MAX_BUF_SIZE) {
		pr_err("Invalid in size: %u or out size: %u\n",
		       *in_size, *out_size_expect);
		return -EFAULT;
	}

	nt_msg->device_name[IFNAMSIZ - 1] = '\0';

	return 0;
}

static struct hinic3_lld_dev *get_lld_dev_by_nt_msg(struct msg_module *nt_msg)
{
	struct hinic3_lld_dev *lld_dev = NULL;

	if (nt_msg->module == SEND_TO_NIC_DRIVER && (nt_msg->msg_formate == GET_XSFP_INFO ||
	    nt_msg->msg_formate == GET_XSFP_PRESENT ||
	    nt_msg->msg_formate == GET_XSFP_INFO_COMP_CMIS)) {
		lld_dev = hinic3_get_lld_dev_by_chip_and_port(nt_msg->device_name, nt_msg->port_id);
	} else if (nt_msg->module == SEND_TO_CUSTOM_DRIVER &&
		   nt_msg->msg_formate == CMD_CUSTOM_BOND_GET_CHIP_NAME) {
		lld_dev = hinic3_get_lld_dev_by_dev_name(nt_msg->device_name, SERVICE_T_MAX);
	} else if (nt_msg->module == SEND_TO_VBS_DRIVER || nt_msg->module == SEND_TO_BIFUR_DRIVER) {
		lld_dev = hinic3_get_lld_dev_by_chip_name(nt_msg->device_name);
	} else  if (nt_msg->module >= SEND_TO_SRV_DRV_BASE && nt_msg->module < SEND_TO_DRIVER_MAX &&
		nt_msg->msg_formate != GET_DRV_VERSION) {
		lld_dev = hinic3_get_lld_dev_by_dev_name(nt_msg->device_name,
							 nt_msg->module - SEND_TO_SRV_DRV_BASE);
	} else {
		lld_dev = hinic3_get_lld_dev_by_chip_name(nt_msg->device_name);
		if (!lld_dev)
			lld_dev = hinic3_get_lld_dev_by_dev_name(nt_msg->device_name,
								 SERVICE_T_MAX);
	}

	return lld_dev;
}

static long hinicadm_k_unlocked_ioctl(struct file *pfile, unsigned long arg)
{
	struct hinic3_lld_dev *lld_dev = NULL;
	struct msg_module nt_msg;
	void *buf_out = NULL;
	void *buf_in = NULL;
	u32 out_size_expect = 0;
	u32 out_size = 0;
	u32 in_size = 0;
	int ret = 0;

	memset(&nt_msg, 0, sizeof(nt_msg));
	if (cmd_parameter_valid(&nt_msg, arg, &out_size_expect, &in_size))
		return -EFAULT;

	lld_dev = get_lld_dev_by_nt_msg(&nt_msg);
	if (!lld_dev) {
		if (nt_msg.msg_formate != DEV_NAME_TEST)
			pr_err("Can not find device %s for module %d\n",
			       nt_msg.device_name, nt_msg.module);

		return -ENODEV;
	}

	if (nt_msg.msg_formate == DEV_NAME_TEST) {
		hinic3_lld_dev_put(lld_dev);
		return 0;
	}

	ret = alloc_tmp_buf(hinic3_get_sdk_hwdev_by_lld(lld_dev), &nt_msg,
			    in_size, &buf_in, out_size_expect, &buf_out);
	if (ret) {
		pr_err("Alloc tmp buff failed\n");
		goto out_free_lock;
	}

	out_size = out_size_expect;

	ret = nictool_exec_cmd(lld_dev, &nt_msg, buf_in, in_size, buf_out, &out_size);
	if (ret) {
		pr_err("nictool_exec_cmd failed, module: %u, ret: %d.\n", nt_msg.module, ret);
		goto out_free_buf;
	}

	if (out_size > out_size_expect) {
		ret = -EFAULT;
		pr_err("Out size is greater than expected out size from user: %u, out size: %u\n",
		       out_size_expect, out_size);
		goto out_free_buf;
	}

	ret = copy_buf_out_to_user(&nt_msg, out_size, buf_out);
	if (ret)
		pr_err("Copy information to user failed\n");

out_free_buf:
	free_tmp_buf(hinic3_get_sdk_hwdev_by_lld(lld_dev), &nt_msg, buf_in, buf_out);

out_free_lock:
	hinic3_lld_dev_put(lld_dev);
	return (long)ret;
}

/**
 * dbgtool_knl_ffm_info_rd - Read ffm information
 * @para: the dbgtool parameter
 * @dbgtool_info: the dbgtool info
 **/
static long dbgtool_knl_ffm_info_rd(struct dbgtool_param *para,
				    struct dbgtool_k_glb_info *dbgtool_info)
{
	if (!para->param.ffm_rd || !dbgtool_info->ffm)
		return -EINVAL;

	/* Copy the ffm_info to user mode */
	if (copy_to_user(para->param.ffm_rd, dbgtool_info->ffm,
			 (unsigned int)sizeof(struct ffm_record_info))) {
		pr_err("Copy ffm_info to user fail\n");
		return -EFAULT;
	}

	return 0;
}

static long dbgtool_k_unlocked_ioctl(struct file *pfile,
				     unsigned int real_cmd,
				     unsigned long arg)
{
	long ret;
	struct dbgtool_param param;
	struct dbgtool_k_glb_info *dbgtool_info = NULL;
	struct card_node *card_info = NULL;
	int i;

	(void)memset(&param, 0, sizeof(param));

	if (copy_from_user(&param, (void *)arg, sizeof(param))) {
		pr_err("Copy param from user fail\n");
		return -EFAULT;
	}

	lld_hold();
	for (i = 0; i < MAX_CARD_NUM; i++) {
		card_info = (struct card_node *)g_card_node_array[i];
		if (!card_info)
			continue;
		if (memcmp(param.chip_name, card_info->chip_name,
			   strlen(card_info->chip_name) + 1) == 0)
			break;
	}

	if (i == MAX_CARD_NUM || !card_info) {
		lld_put();
		pr_err("Can't find this card.\n");
		return -EFAULT;
	}

	card_id = i;
	dbgtool_info = (struct dbgtool_k_glb_info *)card_info->dbgtool_info;

	down(&dbgtool_info->dbgtool_sem);

	switch (real_cmd) {
	case DBGTOOL_CMD_FFM_RD:
		ret = dbgtool_knl_ffm_info_rd(&param, dbgtool_info);
		break;
	case DBGTOOL_CMD_MSG_2_UP:
		pr_err("Not suppose to use this cmd(0x%x).\n", real_cmd);
		ret = 0;
		break;
	default:
		pr_err("Dbgtool cmd(0x%x) not support now\n", real_cmd);
		ret = -EFAULT;
		break;
	}

	up(&dbgtool_info->dbgtool_sem);

	lld_put();

	return ret;
}

static int nictool_k_release(struct inode *pnode, struct file *pfile)
{
	return 0;
}

static int nictool_k_open(struct inode *pnode, struct file *pfile)
{
	return 0;
}

static ssize_t nictool_k_read(struct file *pfile, char __user *ubuf,
			      size_t size, loff_t *ppos)
{
	return 0;
}

static ssize_t nictool_k_write(struct file *pfile, const char __user *ubuf,
			       size_t size, loff_t *ppos)
{
	return 0;
}

static long nictool_k_unlocked_ioctl(struct file *pfile,
				     unsigned int cmd, unsigned long arg)
{
	unsigned int real_cmd;

	real_cmd = _IOC_NR(cmd);

	return (real_cmd == NICTOOL_CMD_TYPE) ?
		hinicadm_k_unlocked_ioctl(pfile, arg) :
		dbgtool_k_unlocked_ioctl(pfile, real_cmd, arg);
}

static int hinic3_mem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	pgprot_t vm_page_prot;
	unsigned long vmsize = vma->vm_end - vma->vm_start;
	phys_addr_t offset = (phys_addr_t)vma->vm_pgoff << PAGE_SHIFT;
	phys_addr_t phy_addr;
	int err = 0;

	if (vmsize > (PAGE_SIZE * (1 << DBGTOOL_PAGE_ORDER))) {
		pr_err("Map size = %lu is bigger than alloc\n", vmsize);
		return -EAGAIN;
	}

	/* old version of tool set vma->vm_pgoff to 0 */
	phy_addr = offset ? offset : g_card_phy_addr[card_id];

	/* check phy_addr valid */
	if (phy_addr != g_card_phy_addr[card_id]) {
		err = hinic3_bar_mmap_param_valid(phy_addr, vmsize);
		if (err != 0) {
			pr_err("mmap param invalid, err: %d\n", err);
			return err;
		}
	}

	/* Disable cache and write buffer in the mapping area */
	vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_page_prot = vm_page_prot;
	if (remap_pfn_range(vma, vma->vm_start, (phy_addr >> PAGE_SHIFT),
			    vmsize, vma->vm_page_prot)) {
		pr_err("Remap pfn range failed.\n");
		return -EAGAIN;
	}

	return 0;
}

static const struct file_operations fifo_operations = {
	.owner = THIS_MODULE,
	.release = nictool_k_release,
	.open = nictool_k_open,
	.read = nictool_k_read,
	.write = nictool_k_write,
	.unlocked_ioctl = nictool_k_unlocked_ioctl,
	.mmap = hinic3_mem_mmap,
};

static void free_dbgtool_info(void *hwdev, struct card_node *chip_info)
{
	struct dbgtool_k_glb_info *dbgtool_info = NULL;

	if (hinic3_func_type(hwdev) != TYPE_VF)
		chip_info->func_handle_array[hinic3_global_func_id(hwdev)] = NULL;

	if (--chip_info->func_num)
		return;

	if (chip_info->chip_id >= 0 && chip_info->chip_id < MAX_CARD_NUM)
		g_card_node_array[chip_info->chip_id] = NULL;

	dbgtool_info = chip_info->dbgtool_info;
	/* FFM deinit */
	if (dbgtool_info && dbgtool_info->ffm) {
		kfree(dbgtool_info->ffm);
		dbgtool_info->ffm = NULL;
	}

	kfree(dbgtool_info);

	chip_info->dbgtool_info = NULL;

	if (chip_info->chip_id >= 0 && chip_info->chip_id < MAX_CARD_NUM)
		dbgtool_knl_free_mem(chip_info->chip_id);
}

static int alloc_dbgtool_info(void *hwdev, struct card_node *chip_info)
{
	struct dbgtool_k_glb_info *dbgtool_info = NULL;
	int err, id = 0;

	if (hinic3_func_type(hwdev) != TYPE_VF)
		chip_info->func_handle_array[hinic3_global_func_id(hwdev)] = hwdev;

	if (chip_info->func_num++)
		return 0;

	dbgtool_info = (struct dbgtool_k_glb_info *)
			kzalloc(sizeof(struct dbgtool_k_glb_info), GFP_KERNEL);
	if (!dbgtool_info) {
		pr_err("Failed to allocate dbgtool_info\n");
		goto dbgtool_info_fail;
	}

	chip_info->dbgtool_info = dbgtool_info;

	/* FFM init */
	dbgtool_info->ffm = (struct ffm_record_info *)
			kzalloc(sizeof(struct ffm_record_info), GFP_KERNEL);
	if (!dbgtool_info->ffm) {
		pr_err("Failed to allocate cell contexts for a chain\n");
		goto dbgtool_info_ffm_fail;
	}

	sema_init(&dbgtool_info->dbgtool_sem, 1);

	err = sscanf(chip_info->chip_name, HINIC3_CHIP_NAME "%d", &id);
	if (err < 0) {
		pr_err("Failed to get card id\n");
		goto sscanf_chdev_fail;
	}

	g_card_node_array[id] = chip_info;

	return 0;

sscanf_chdev_fail:
	kfree(dbgtool_info->ffm);

dbgtool_info_ffm_fail:
	kfree(dbgtool_info);
	chip_info->dbgtool_info = NULL;

dbgtool_info_fail:
	if (hinic3_func_type(hwdev) != TYPE_VF)
		chip_info->func_handle_array[hinic3_global_func_id(hwdev)] = NULL;
	chip_info->func_num--;
	return -ENOMEM;
}

/**
 * nictool_k_init - initialize the hw interface
 **/
/* temp for dbgtool_info */

int nictool_k_init(void *hwdev, void *chip_node)
{
	struct card_node *chip_info = (struct card_node *)chip_node;
	struct device *pdevice = NULL;
	int err;

	err = alloc_dbgtool_info(hwdev, chip_info);
	if (err)
		return err;

	if (g_nictool_ref_cnt++) {
		/* already initialized */
		return 0;
	}

	err = alloc_chrdev_region(&g_dev_id, 0, 1, HIADM3_DEV_NAME);
	if (err) {
		pr_err("Register nictool_dev failed(0x%x)\n", err);
		goto alloc_chdev_fail;
	}

	/* Create equipment */
	g_nictool_class = class_create(HIADM3_DEV_CLASS);
	if (IS_ERR(g_nictool_class)) {
		pr_err("Create nictool_class fail\n");
		err = -EFAULT;
		goto class_create_err;
	}

	/* Initializing the character device */
	cdev_init(&g_nictool_cdev, &fifo_operations);

	/* Add devices to the operating system */
	err = cdev_add(&g_nictool_cdev, g_dev_id, 1);
	if (err < 0) {
		pr_err("Add nictool_dev to operating system fail(0x%x)\n", err);
		goto cdev_add_err;
	}

	/* Export device information to user space
	 * (/sys/class/class name/device name)
	 */
	pdevice = device_create(g_nictool_class, NULL,
				g_dev_id, NULL, HIADM3_DEV_NAME);
	if (IS_ERR(pdevice)) {
		pr_err("Export nictool device information to user space fail\n");
		err = -EFAULT;
		goto device_create_err;
	}

	pr_info("Register nictool_dev to system succeed\n");

	return 0;

device_create_err:
	cdev_del(&g_nictool_cdev);

cdev_add_err:
	class_destroy(g_nictool_class);

class_create_err:
	g_nictool_class = NULL;
	unregister_chrdev_region(g_dev_id, 1);

alloc_chdev_fail:
	g_nictool_ref_cnt--;
	free_dbgtool_info(hwdev, chip_info);

	return err;
}

void nictool_k_uninit(void *hwdev, void *chip_node)
{
	struct card_node *chip_info = (struct card_node *)chip_node;

	free_dbgtool_info(hwdev, chip_info);

	if (!g_nictool_ref_cnt)
		return;

	if (--g_nictool_ref_cnt)
		return;

	if (!g_nictool_class || IS_ERR(g_nictool_class)) {
		pr_err("Nictool class is NULL.\n");
		return;
	}

	device_destroy(g_nictool_class, g_dev_id);
	cdev_del(&g_nictool_cdev);
	class_destroy(g_nictool_class);
	g_nictool_class = NULL;

	unregister_chrdev_region(g_dev_id, 1);

	pr_info("Unregister nictool_dev succeed\n");
}

