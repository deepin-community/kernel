// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [TOOL]" fmt

#include <net/sock.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/time.h>

#include "sss_adapter_mgmt.h"
#include "sss_linux_kernel.h"
#include "sss_hw.h"
#include "sss_tool_comm.h"
#include "sss_tool_hw.h"
#include "sss_tool.h"

#define SSS_TOOL_DEV_PATH		"/dev/sssnic_nictool_dev"
#define SSS_TOOL_DEV_CLASS		"sssnic_nictool_class"
#define SSS_TOOL_DEV_NAME		"sssnic_nictool_dev"

#define	SSS_TOOL_CTRLQ_BUF_SIZE_MAX	2048U
#define SSS_TOOL_MSG_IN_SIZE_MAX	(2048 * 1024)
#define SSS_TOOL_MSG_OUT_SIZE_MAX	(2048 * 1024)
#define SSS_TOOL_BUF_SIZE_MAX		(2048 * 1024)

typedef int (*sss_tool_deal_handler_fun)(struct sss_hal_dev *hal_dev, struct sss_tool_msg *tool_msg,
			      void *in_buf, u32 in_len, void *out_buf, u32 *out_len);

struct sss_tool_deal_handler {
	enum module_name msg_name;
	sss_tool_deal_handler_fun func;
};

static int g_nictool_ref_cnt;

static dev_t g_dev_id = {0};

static struct class *g_nictool_class;
static struct cdev g_nictool_cdev;

static void *g_card_node_array[SSS_TOOL_CARD_MAX] = {0};
void *g_card_va[SSS_TOOL_CARD_MAX] = {0};
u64 g_card_pa[SSS_TOOL_CARD_MAX] = {0};
int g_card_id;

static int sss_tool_msg_to_nic(struct sss_hal_dev *hal_dev, struct sss_tool_msg *tool_msg,
			       void *in_buf, u32 in_len, void *out_buf, u32 *out_len)
{
	int ret = -EINVAL;
	void *uld_dev = NULL;
	enum sss_service_type service_type;
	struct sss_uld_info *uld_info = sss_get_uld_info();

	service_type = tool_msg->module - SSS_TOOL_MSG_TO_SRV_DRV_BASE;
	if (service_type >= SSS_SERVICE_TYPE_MAX) {
		tool_err("Invalid input module id: %u\n", tool_msg->module);
		return -EINVAL;
	}

	uld_dev = sss_get_uld_dev(hal_dev, service_type);
	if (!uld_dev) {
		if (tool_msg->msg_formate == SSS_TOOL_GET_DRV_VERSION)
			return 0;

		tool_err("Fail to get uld device\n");
		return -EINVAL;
	}

	if (uld_info[service_type].ioctl)
		ret = uld_info[service_type].ioctl(uld_dev, tool_msg->msg_formate,
					     in_buf, in_len, out_buf, out_len);
	sss_uld_dev_put(hal_dev, service_type);

	return ret;
}

void sss_tool_free_in_buf(void *hwdev, const struct sss_tool_msg *tool_msg, void *in_buf)
{
	if (!in_buf)
		return;

	if (tool_msg->module == SSS_TOOL_MSG_TO_NPU)
		sss_free_ctrlq_msg_buf(hwdev, in_buf);
	else
		kfree(in_buf);
}

void sss_tool_free_out_buf(void *hwdev, struct sss_tool_msg *tool_msg,
			   void *out_buf)
{
	if (!out_buf)
		return;

	if (tool_msg->module == SSS_TOOL_MSG_TO_NPU &&
	    !tool_msg->npu_cmd.direct_resp)
		sss_free_ctrlq_msg_buf(hwdev, out_buf);
	else
		kfree(out_buf);
}

int sss_tool_alloc_in_buf(void *hwdev, struct sss_tool_msg *tool_msg,
			  u32 in_len, void **in_buf)
{
	void *msg_buf = NULL;

	if (!in_len)
		return 0;

	if (tool_msg->module == SSS_TOOL_MSG_TO_NPU) {
		struct sss_ctrl_msg_buf *cmd_buf = NULL;

		if (in_len > SSS_TOOL_CTRLQ_BUF_SIZE_MAX) {
			tool_err("Invalid ctrlq in len(%u) more than %u\n",
				 in_len, SSS_TOOL_CTRLQ_BUF_SIZE_MAX);
			return -ENOMEM;
		}

		cmd_buf = sss_alloc_ctrlq_msg_buf(hwdev);
		if (!cmd_buf) {
			tool_err("Fail to alloc ctrlq msg buf\n");
			return -ENOMEM;
		}
		*in_buf = (void *)cmd_buf;
		cmd_buf->size = (u16)in_len;
	} else {
		if (in_len > SSS_TOOL_MSG_IN_SIZE_MAX) {
			tool_err("Invalid in len(%u) more than %u\n",
				 in_len, SSS_TOOL_MSG_IN_SIZE_MAX);
			return -ENOMEM;
		}
		msg_buf = kzalloc(in_len, GFP_KERNEL);
		*in_buf = msg_buf;
	}

	if (!(*in_buf)) {
		tool_err("Fail to alloc in buf\n");
		return -ENOMEM;
	}

	return 0;
}

int sss_tool_alloc_out_buf(void *hwdev, struct sss_tool_msg *tool_msg,
			   u32 out_len, void **out_buf)
{
	if (!out_len) {
		tool_info("out len is 0, need not alloc buf\n");
		return 0;
	}

	if (tool_msg->module == SSS_TOOL_MSG_TO_NPU &&
	    !tool_msg->npu_cmd.direct_resp) {
		struct sss_ctrl_msg_buf *msg_buf = NULL;

		if (out_len > SSS_TOOL_CTRLQ_BUF_SIZE_MAX) {
			tool_err("Invalid ctrlq out len(%u) more than %u\n",
				 out_len, SSS_TOOL_CTRLQ_BUF_SIZE_MAX);
			return -ENOMEM;
		}

		msg_buf = sss_alloc_ctrlq_msg_buf(hwdev);
		*out_buf = (void *)msg_buf;
	} else {
		if (out_len > SSS_TOOL_MSG_OUT_SIZE_MAX) {
			tool_err("Invalid out len(%u) more than %u\n",
				 out_len, SSS_TOOL_MSG_OUT_SIZE_MAX);
			return -ENOMEM;
		}
		*out_buf = kzalloc(out_len, GFP_KERNEL);
	}
	if (!(*out_buf)) {
		tool_err("Fail to alloc out buf\n");
		return -ENOMEM;
	}

	return 0;
}

int sss_tool_copy_to_user(struct sss_tool_msg *tool_msg,
			  u32 out_len, void *out_buf)
{
	void *out_msg = NULL;

	if (tool_msg->module == SSS_TOOL_MSG_TO_NPU && !tool_msg->npu_cmd.direct_resp) {
		out_msg = ((struct sss_ctrl_msg_buf *)out_buf)->buf;
		if (copy_to_user(tool_msg->out_buf, out_msg, out_len))
			return -EFAULT;
		return 0;
	}

	if (copy_to_user(tool_msg->out_buf, out_buf, out_len))
		return -EFAULT;

	return 0;
}

static int sss_tool_alloc_buf(void *hwdev, struct sss_tool_msg *tool_msg, u32 in_len,
			      void **in_buf, u32 out_len, void **out_buf)
{
	int ret;

	ret = sss_tool_alloc_in_buf(hwdev, tool_msg, in_len, in_buf);
	if (ret) {
		tool_err("Fail to alloc tool msg in buf\n");
		return ret;
	}

	if (copy_from_user(*in_buf, tool_msg->in_buf, in_len)) {
		tool_err("Fail to copy tool_msg to in buf\n");
		sss_tool_free_in_buf(hwdev, tool_msg, *in_buf);
		return -EFAULT;
	}

	ret = sss_tool_alloc_out_buf(hwdev, tool_msg, out_len, out_buf);
	if (ret) {
		tool_err("Fail to alloc tool msg out buf\n");
		goto alloc_out_buf_err;
	}

	return 0;

alloc_out_buf_err:
	sss_tool_free_in_buf(hwdev, tool_msg, *in_buf);

	return ret;
}

static void sss_tool_free_buf(void *hwdev, struct sss_tool_msg *tool_msg,
			      void *in_buf, void *out_buf)
{
	sss_tool_free_out_buf(hwdev, tool_msg, out_buf);
	sss_tool_free_in_buf(hwdev, tool_msg, in_buf);
}

const struct sss_tool_deal_handler g_deal_msg_handle[] = {
	{SSS_TOOL_MSG_TO_NPU,		sss_tool_msg_to_npu},
	{SSS_TOOL_MSG_TO_MPU,		sss_tool_msg_to_mpu},
	{SSS_TOOL_MSG_TO_SM,		sss_tool_msg_to_sm},
	{SSS_TOOL_MSG_TO_HW_DRIVER,	sss_tool_msg_to_hw},
	{SSS_TOOL_MSG_TO_NIC_DRIVER,	sss_tool_msg_to_nic}
};

static int sss_tool_deal_cmd(struct sss_hal_dev *hal_dev, struct sss_tool_msg *tool_msg,
			     void *in_buf, u32 in_len, void *out_buf, u32 *out_len)
{
	int ret = 0;
	int index;
	int msg_num = ARRAY_LEN(g_deal_msg_handle);

	for (index = 0; index < msg_num; index++) {
		if (tool_msg->module != g_deal_msg_handle[index].msg_name)
			continue;

		ret = g_deal_msg_handle[index].func(hal_dev, tool_msg,
						    in_buf, in_len, out_buf, out_len);
		break;
	}

	if (index == msg_num)
		ret = sss_tool_msg_to_nic(hal_dev, tool_msg,
					  in_buf, in_len, out_buf, out_len);

	return ret;
}

static struct sss_hal_dev *sss_tool_get_hal_dev_by_msg(struct sss_tool_msg *tool_msg)
{
	struct sss_hal_dev *hal_dev = NULL;

	if (tool_msg->module >= SSS_TOOL_MSG_TO_SRV_DRV_BASE &&
	    tool_msg->module < SSS_TOOL_MSG_TO_DRIVER_MAX &&
	    tool_msg->msg_formate != SSS_TOOL_GET_DRV_VERSION) {
		hal_dev = sss_get_lld_dev_by_dev_name(tool_msg->device_name,
						      tool_msg->module -
						      SSS_TOOL_MSG_TO_SRV_DRV_BASE);
	} else {
		hal_dev = sss_get_lld_dev_by_chip_name(tool_msg->device_name);
		if (!hal_dev)
			hal_dev = sss_get_lld_dev_by_dev_name(tool_msg->device_name,
							      SSS_SERVICE_TYPE_MAX);
	}

	if (tool_msg->module == SSS_TOOL_MSG_TO_NIC_DRIVER &&
	    (tool_msg->msg_formate == SSS_TOOL_GET_XSFP_INFO ||
	    tool_msg->msg_formate == SSS_TOOL_GET_XSFP_PRESENT))
		hal_dev = sss_get_lld_dev_by_chip_and_port(tool_msg->device_name,
							   tool_msg->port_id);

	return hal_dev;
}

static int sss_tool_check_msg_valid(struct sss_tool_msg *tool_msg)
{
	if (tool_msg->buf_out_size > SSS_TOOL_BUF_SIZE_MAX ||
	    tool_msg->buf_in_size > SSS_TOOL_BUF_SIZE_MAX) {
		tool_err("Invalid in buf len: %u or out buf len: %u\n",
			 tool_msg->buf_in_size, tool_msg->buf_out_size);
		return -EFAULT;
	}

	return 0;
}

static long sss_tool_msg_ioctl(unsigned long arg)
{
	int ret = 0;
	u32 in_len = 0;
	u32 expect_out_len = 0;
	u32 out_len = 0;
	void *in_buf = NULL;
	void *out_buf = NULL;
	struct sss_hal_dev *hal_dev = NULL;
	struct sss_tool_msg tool_msg = {0};

	if (copy_from_user(&tool_msg, (void *)arg, sizeof(tool_msg))) {
		tool_err("Fail to copy msg from user space\n");
		return -EFAULT;
	}

	if (sss_tool_check_msg_valid(&tool_msg)) {
		tool_err("Fail to check msg valid\n");
		return -EFAULT;
	}

	tool_msg.device_name[IFNAMSIZ - 1] = '\0';
	expect_out_len = tool_msg.buf_out_size;
	in_len = tool_msg.buf_in_size;

	hal_dev = sss_tool_get_hal_dev_by_msg(&tool_msg);
	if (!hal_dev) {
		if (tool_msg.msg_formate != SSS_TOOL_DEV_NAME_TEST)
			tool_err("Fail to find device %s for module %d\n",
				 tool_msg.device_name, tool_msg.module);
		return -ENODEV;
	}

	if (tool_msg.msg_formate == SSS_TOOL_DEV_NAME_TEST)
		return 0;

	ret = sss_tool_alloc_buf(hal_dev->hwdev, &tool_msg,
				 in_len, &in_buf, expect_out_len, &out_buf);
	if (ret) {
		tool_err("Fail to alloc cmd buf\n");
		goto out_free_lock;
	}

	out_len = expect_out_len;

	ret = sss_tool_deal_cmd(hal_dev, &tool_msg, in_buf, in_len, out_buf, &out_len);
	if (ret) {
		tool_err("Fail to execute cmd, module: %u, ret: %d.\n", tool_msg.module, ret);
		goto out_free_buf;
	}

	if (out_len > expect_out_len) {
		ret = -EFAULT;
		tool_err("Fail to execute cmd, expected out len from user: %u, out len: %u\n",
			 expect_out_len, out_len);
		goto out_free_buf;
	}

	ret = sss_tool_copy_to_user(&tool_msg, out_len, out_buf);
	if (ret)
		tool_err("Fail to copy return information to user space\n");

out_free_buf:
	sss_tool_free_buf(hal_dev->hwdev, &tool_msg, in_buf, out_buf);

out_free_lock:
	lld_dev_put(hal_dev);
	return (long)ret;
}

static long sss_tool_knl_ffm_info_rd(struct sss_tool_dbg_param *dbg_param,
				     struct sss_tool_knl_dbg_info *dbg_info)
{
	if (copy_to_user(dbg_param->param.ffm_rd, dbg_info->ffm,
			 (unsigned int)sizeof(*dbg_param->param.ffm_rd))) {
		tool_err("Fail to copy ffm_info to user space\n");
		return -EFAULT;
	}

	return 0;
}

static struct sss_card_node *sss_tool_find_card_node(char *chip_name)
{
	int i;
	struct sss_card_node *card_node = NULL;

	for (i = 0; i < SSS_TOOL_CARD_MAX; i++) {
		card_node = (struct sss_card_node *)g_card_node_array[i];
		if (!card_node)
			continue;
		if (!strncmp(chip_name, card_node->chip_name, IFNAMSIZ))
			break;
	}
	if (i == SSS_TOOL_CARD_MAX || !card_node)
		return NULL;

	g_card_id = i;

	return card_node;
}

static long sss_tool_dbg_ioctl(unsigned int cmd_type, unsigned long arg)
{
	struct sss_tool_knl_dbg_info *dbg_info = NULL;
	struct sss_card_node *card_node = NULL;
	struct sss_tool_dbg_param param = {0};
	long ret;

	if (copy_from_user(&param, (void *)arg, sizeof(param))) {
		tool_err("Fail to copy msg param from user\n");
		return -EFAULT;
	}

	sss_hold_chip_node();

	card_node = sss_tool_find_card_node(param.chip_name);
	if (!card_node) {
		sss_put_chip_node();
		tool_err("Fail to find card node %s\n", param.chip_name);
		return -EFAULT;
	}

	dbg_info = (struct sss_tool_knl_dbg_info *)card_node->dbgtool_info;

	down(&dbg_info->dbgtool_sem);

	if (cmd_type == SSS_TOOL_DBG_CMD_FFM_RD) {
		ret = sss_tool_knl_ffm_info_rd(&param, dbg_info);
	} else if (cmd_type == SSS_TOOL_DBG_CMD_MSG_2_UP) {
		tool_info("cmd(0x%x) not suppose.\n", cmd_type);
		ret = 0;
	} else {
		tool_err("Fail to execute cmd(0x%x) ,it is not support\n", cmd_type);
		ret = -EFAULT;
	}

	up(&dbg_info->dbgtool_sem);

	sss_put_chip_node();

	return ret;
}

static int sss_tool_release(struct inode *pnode, struct file *pfile)
{
	return 0;
}

static int sss_tool_open(struct inode *pnode, struct file *pfile)
{
	return 0;
}

static ssize_t sss_tool_read(struct file *pfile, char __user *ubuf,
			     size_t size, loff_t *ppos)
{
	return 0;
}

static ssize_t sss_tool_write(struct file *pfile, const char __user *ubuf,
			      size_t size, loff_t *ppos)
{
	return 0;
}

static long sss_tool_unlocked_ioctl(struct file *pfile,
				    unsigned int cmd, unsigned long arg)
{
	unsigned int cmd_type = _IOC_NR(cmd);

	if (cmd_type == SSS_TOOL_CMD_TYPE)
		return sss_tool_msg_ioctl(arg);

	return sss_tool_dbg_ioctl(cmd_type, arg);
}

static int sss_tool_mem_mmap(struct file *filp, struct vm_area_struct *mem_area)
{
	unsigned long mem_size = mem_area->vm_end - mem_area->vm_start;
	phys_addr_t offset = (phys_addr_t)mem_area->vm_pgoff << PAGE_SHIFT;
	phys_addr_t phy_addr;

	if (mem_size > SSS_TOOL_MEM_MAP_SIZE) {
		tool_err("Fail to map mem, mem_size :%ld, alloc size: %ld\n",
			 mem_size, SSS_TOOL_MEM_MAP_SIZE);
		return -EAGAIN;
	}

	phy_addr = offset ? offset : g_card_pa[g_card_id];
	if (!phy_addr) {
		tool_err("Fail to map mem, card_id = %d phy_addr is 0\n", g_card_id);
		return -EAGAIN;
	}

	mem_area->vm_page_prot = pgprot_noncached(mem_area->vm_page_prot);
	if (remap_pfn_range(mem_area, mem_area->vm_start, (phy_addr >> PAGE_SHIFT),
			    mem_size, mem_area->vm_page_prot)) {
		tool_err("Fail to remap pfn range.\n");
		return -EAGAIN;
	}

	return 0;
}

static const struct file_operations sss_tool_file_ops = {
	.owner = THIS_MODULE,
	.release = sss_tool_release,
	.open = sss_tool_open,
	.read = sss_tool_read,
	.write = sss_tool_write,
	.unlocked_ioctl = sss_tool_unlocked_ioctl,
	.mmap = sss_tool_mem_mmap,
};

static struct sss_tool_knl_dbg_info *sss_tool_alloc_dbg_info(void *hwdev)
{
	struct sss_tool_knl_dbg_info *dbg_info = NULL;

	dbg_info = (struct sss_tool_knl_dbg_info *)
		   kzalloc(sizeof(struct sss_tool_knl_dbg_info), GFP_KERNEL);
	if (!dbg_info)
		return NULL;

	dbg_info->ffm = (struct sss_tool_ffm_record_info *)
			kzalloc(sizeof(*dbg_info->ffm), GFP_KERNEL);
	if (!dbg_info->ffm) {
		tool_err("Fail to alloc ffm_record_info\n");
		kfree(dbg_info);
		return NULL;
	}

	return dbg_info;
}

static void sss_tool_free_dbg_info(struct sss_tool_knl_dbg_info *dbg_info)
{
	kfree(dbg_info->ffm);
	kfree(dbg_info);
}

static int sss_tool_get_node_id(struct sss_card_node *card_node, int *node_id)
{
	int ret;

	ret = sscanf(card_node->chip_name, SSS_CHIP_NAME "%d", node_id);
	if (ret < 0) {
		tool_err("Fail to get card id\n");
		return -ENOMEM;
	}

	return 0;
}

static int sss_tool_add_func_to_card_node(void *hwdev, struct sss_card_node *card_node)
{
	int func_id = sss_get_func_id(hwdev);
	struct sss_tool_knl_dbg_info *dbg_info = NULL;
	int ret;
	int node_id;

	if (sss_get_func_type(hwdev) != SSS_FUNC_TYPE_VF)
		card_node->func_handle_array[func_id] = hwdev;

	if (card_node->func_num++)
		return 0;

	dbg_info = sss_tool_alloc_dbg_info(hwdev);
	if (!dbg_info) {
		ret = -ENOMEM;
		tool_err("Fail to alloc dbg_info\n");
		goto alloc_dbg_info_err;
	}
	card_node->dbgtool_info = dbg_info;
	sema_init(&dbg_info->dbgtool_sem, 1);

	ret = sss_tool_get_node_id(card_node, &node_id);
	if (ret) {
		tool_err("Fail to add node to global array\n");
		goto get_node_id_err;
	}
	g_card_node_array[node_id] = card_node;

	return 0;

get_node_id_err:
	sss_tool_free_dbg_info(dbg_info);
	card_node->dbgtool_info = NULL;

alloc_dbg_info_err:
	card_node->func_num--;
	if (sss_get_func_type(hwdev) != SSS_FUNC_TYPE_VF)
		card_node->func_handle_array[func_id] = NULL;

	return ret;
}

static void sss_tool_del_func_in_card_node(void *hwdev, struct sss_card_node *card_node)
{
	struct sss_tool_knl_dbg_info *dbg_info = card_node->dbgtool_info;
	int func_id = sss_get_func_id(hwdev);
	int node_id;

	if (sss_get_func_type(hwdev) != SSS_FUNC_TYPE_VF)
		card_node->func_handle_array[func_id] = NULL;

	if (--card_node->func_num)
		return;

	sss_tool_get_node_id(card_node, &node_id);
	if (node_id < SSS_TOOL_CARD_MAX)
		g_card_node_array[node_id] = NULL;

	sss_tool_free_dbg_info(dbg_info);
	card_node->dbgtool_info = NULL;

	if (node_id < SSS_TOOL_CARD_MAX)
		(void)sss_tool_free_card_mem(node_id);
}

static int sss_tool_create_dev(void)
{
	int ret;
	struct device *pdevice = NULL;

	ret = alloc_chrdev_region(&g_dev_id, 0, 1, SSS_TOOL_DEV_NAME);
	if (ret) {
		tool_err("Fail to alloc sssnic_nictool_dev region(0x%x)\n", ret);
		return ret;
	}

#ifdef CLASS_CREATE_WITH_ONE_PARAM
	g_nictool_class = class_create(SSS_TOOL_DEV_CLASS);
#else
	g_nictool_class = class_create(THIS_MODULE, SSS_TOOL_DEV_CLASS);
#endif
	if (IS_ERR(g_nictool_class)) {
		tool_err("Fail to create sssnic_nictool_class\n");
		ret = -EFAULT;
		goto create_class_err;
	}

	cdev_init(&g_nictool_cdev, &sss_tool_file_ops);

	ret = cdev_add(&g_nictool_cdev, g_dev_id, 1);
	if (ret < 0) {
		tool_err("Fail to add sssnic_nictool_dev to operating system (0x%x)\n", ret);
		goto add_cdev_err;
	}

	pdevice = device_create(g_nictool_class, NULL, g_dev_id, NULL, SSS_TOOL_DEV_NAME);
	if (IS_ERR(pdevice)) {
		tool_err("Fail to create sssnic_nictool_dev on operating system\n");
		ret = -EFAULT;
		goto create_device_err;
	}

	tool_info("Success to register sssnic_nictool_dev to system\n");

	return 0;

create_device_err:
		cdev_del(&g_nictool_cdev);

add_cdev_err:
		class_destroy(g_nictool_class);

create_class_err:
		g_nictool_class = NULL;
		unregister_chrdev_region(g_dev_id, 1);

	return ret;
}

static void sss_tool_destroy_dev(void)
{
	device_destroy(g_nictool_class, g_dev_id);
	cdev_del(&g_nictool_cdev);
	class_destroy(g_nictool_class);
	g_nictool_class = NULL;
	unregister_chrdev_region(g_dev_id, 1);
	tool_info("Success to unregister sssnic_nictool_dev to system\n");
}

int sss_tool_init(void *hwdev, void *chip_node)
{
	struct sss_card_node *card_node = (struct sss_card_node *)chip_node;
	int ret;

	ret = sss_tool_add_func_to_card_node(hwdev, card_node);
	if (ret) {
		tool_err("Fail to add func to card node\n");
		return ret;
	}

	if (g_nictool_ref_cnt++) {
		tool_info("sssnic_nictool_dev has already create\n");
		return 0;
	}

	ret = sss_tool_create_dev();
	if (ret) {
		tool_err("Fail to create sssnic_nictool_dev\n");
		goto out;
	}

	return 0;

out:
	g_nictool_ref_cnt--;
	sss_tool_del_func_in_card_node(hwdev, card_node);

	return ret;
}

void sss_tool_uninit(void *hwdev, void *chip_node)
{
	struct sss_card_node *chip_info = (struct sss_card_node *)chip_node;

	sss_tool_del_func_in_card_node(hwdev, chip_info);

	if (g_nictool_ref_cnt == 0)
		return;

	if (--g_nictool_ref_cnt)
		return;

	if (!g_nictool_class || IS_ERR(g_nictool_class)) {
		tool_err("Fail to uninit sssnictool, tool class is NULL.\n");
		return;
	}

	sss_tool_destroy_dev();
}
