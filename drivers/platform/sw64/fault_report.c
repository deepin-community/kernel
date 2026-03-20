// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 WXIAT
 */

#define pr_fmt(fmt) "sunway-fault-report: " fmt

#include <linux/ipmi.h>
#include <linux/kernel.h>

#include <asm/platform.h>

#define SUNWAY_FAULT_REPORT_NETFN	0x3A
#define SUNWAY_FAULT_REPORT_CMD		0x33

#define OFFSET_FAULT_SOURCE		0x0b00UL
#define OFFSET_SI_FAULT_STAT		0x3100UL
#define OFFSET_DLI_RLTD_FAULT		0x0980UL

struct fault_data_item {
	struct list_head list;
	u64 fault_data;
};

static LIST_HEAD(fault_data_list);
static DEFINE_SPINLOCK(fault_data_lock);

struct sunway_fault_report_bmc_data {
	struct device *bmc_device;
	struct ipmi_addr address;
	struct ipmi_user *user;
	struct completion complete;
	int interface;

	struct kernel_ipmi_msg tx_message;
	unsigned char tx_msg_data[IPMI_MAX_MSG_LENGTH];
	long tx_msgid;

	unsigned char rx_msg_data[IPMI_MAX_MSG_LENGTH];
	unsigned short rx_msg_len;
	unsigned char rx_result;
	int rx_recv_type;

	bool initialized;
};

struct sunway_fault_report_driver_data {
	struct work_struct work;

	struct ipmi_smi_watcher bmc_events;
	struct ipmi_user_hndl ipmi_hndlrs;
	struct sunway_fault_report_bmc_data bmc_data;
};

static void sunway_fault_report_register_bmc(int iface, struct device *dev);
static void sunway_fault_report_bmc_gone(int iface);
static void sunway_fault_report_msg_handler(struct ipmi_recv_msg *msg,
		void *user_msg_data);

static struct sunway_fault_report_driver_data driver_data = {
	.bmc_events = {
		.new_smi = sunway_fault_report_register_bmc,
		.smi_gone = sunway_fault_report_bmc_gone,
	},

	.ipmi_hndlrs = {
		.ipmi_recv_hndl = sunway_fault_report_msg_handler,
	},
};

static int sunway_fault_report_send_message(struct sunway_fault_report_bmc_data *bmc_data)
{
	int ret;

	ret = ipmi_validate_addr(&bmc_data->address, sizeof(bmc_data->address));
	if (ret) {
		dev_err(bmc_data->bmc_device, "invalid ipmi addr (%d)\n", ret);
		return ret;
	}

	bmc_data->tx_msgid++;
	ret = ipmi_request_settime(bmc_data->user, &bmc_data->address,
			bmc_data->tx_msgid, &bmc_data->tx_message,
			bmc_data, 0, 0, 0);
	if (ret) {
		dev_err(bmc_data->bmc_device,
				"unable to send message (%d)\n", ret);
		return ret;
	}

	return 0;
}

static int sunway_fault_report_send_cmd(struct sunway_fault_report_bmc_data *bmc_data,
		unsigned char cmd, const unsigned char *data, unsigned short data_len)
{
	bmc_data->tx_message.cmd = cmd;
	bmc_data->tx_message.data_len = data_len;

	if (data_len)
		memcpy(bmc_data->tx_msg_data, data, data_len);

	return sunway_fault_report_send_message(bmc_data);
}

static int sunway_fault_report_send_data(struct sunway_fault_report_bmc_data *bmc_data,
		const unsigned char *data, unsigned short data_len)
{
	int ret;

	ret = sunway_fault_report_send_cmd(bmc_data,
			SUNWAY_FAULT_REPORT_CMD, data, data_len);
	if (ret) {
		dev_err(bmc_data->bmc_device, "failed to send data\n");
		return ret;
	}

	wait_for_completion(&bmc_data->complete);

	if (bmc_data->rx_result) {
		dev_err(bmc_data->bmc_device, "rx error 0x%x\n",
				bmc_data->rx_result);
		return -EINVAL;
	}

	return 0;
}

static void sunway_fault_report_register_bmc(int iface, struct device *dev)
{
	struct sunway_fault_report_bmc_data *bmc_data = &driver_data.bmc_data;
	int ret;

	/* Multiple BMC for suwnay fault report are not supported */
	if (bmc_data->initialized) {
		dev_err(dev, "unable to register bmc repeatedly\n");
		return;
	}

	bmc_data->address.addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
	bmc_data->address.channel = IPMI_BMC_CHANNEL;
	bmc_data->address.data[0] = 0;
	bmc_data->interface = iface;
	bmc_data->bmc_device = dev;

	/* Create IPMI user */
	ret = ipmi_create_user(bmc_data->interface, &driver_data.ipmi_hndlrs,
			bmc_data, &bmc_data->user);
	if (ret) {
		dev_err(dev, "unable to register user with IPMI interface %d",
				bmc_data->interface);
		return;
	}

	/* Initialize message */
	bmc_data->tx_msgid = 0;
	bmc_data->tx_message.netfn = SUNWAY_FAULT_REPORT_NETFN;
	bmc_data->tx_message.data = bmc_data->tx_msg_data;

	init_completion(&bmc_data->complete);

	bmc_data->initialized = true;

	return;
}

static void sunway_fault_report_bmc_gone(int iface)
{
	struct sunway_fault_report_bmc_data *bmc_data = &driver_data.bmc_data;

	if (WARN_ON(bmc_data->interface != iface))
		return;

	ipmi_destroy_user(bmc_data->user);
}

static void sunway_fault_report_msg_handler(struct ipmi_recv_msg *msg,
		void *user_msg_data)
{
	struct sunway_fault_report_bmc_data *bmc_data = user_msg_data;

	if (msg->msgid != bmc_data->tx_msgid) {
		dev_err(bmc_data->bmc_device,
			"mismatch between rx msgid (0x%lx) and tx msgid (0x%lx)!\n",
			msg->msgid,
			bmc_data->tx_msgid);
		ipmi_free_recv_msg(msg);
		return;
	}

	bmc_data->rx_recv_type = msg->recv_type;
	if (msg->msg.data_len > 0)
		bmc_data->rx_result = msg->msg.data[0];
	else
		bmc_data->rx_result = IPMI_UNKNOWN_ERR_COMPLETION_CODE;

	if (msg->msg.data_len > 1) {
		bmc_data->rx_msg_len = msg->msg.data_len - 1;
		memcpy(bmc_data->rx_msg_data, msg->msg.data + 1,
				bmc_data->rx_msg_len);
	} else
		bmc_data->rx_msg_len = 0;

	ipmi_free_recv_msg(msg);
	complete(&bmc_data->complete);
}

static bool is_spbu_fault(void __iomem *spbu_base)
{
	if (readq(spbu_base + OFFSET_FAULT_SOURCE) & (0x1UL << 9))
		return !!(readq(spbu_base + OFFSET_SI_FAULT_STAT) & 0xFUL);

	return false;
}

static bool is_lcpm_fault(void __iomem *spbu_base)
{
	return !!(readq(spbu_base + OFFSET_FAULT_SOURCE) & (0x1UL << 1));
}

static bool is_gcpm_fault(void __iomem *spbu_base)
{
	if (readq(spbu_base + OFFSET_FAULT_SOURCE) & (0x1UL << 8))
		return !!(readq(spbu_base + OFFSET_DLI_RLTD_FAULT) & 0xFFUL);

	return false;
}

static bool is_mc_fault(void __iomem *spbu_base)
{
	return !!(readq(spbu_base + OFFSET_FAULT_SOURCE) & (0x1UL << 2));
}

static bool is_dli_fault(void __iomem *spbu_base)
{
	if (readq(spbu_base + OFFSET_FAULT_SOURCE) & (0x1UL << 8))
		return !!(readq(spbu_base + OFFSET_DLI_RLTD_FAULT) & (0x7UL << 16));

	return false;
}

static bool is_piu_fault(void __iomem *spbu_base)
{
	if (readq(spbu_base + OFFSET_FAULT_SOURCE) & (0x1UL << 9))
		return !!(readq(spbu_base + OFFSET_SI_FAULT_STAT) & 0x0008880000001110UL);

	return false;
}

static bool is_cgmn_fault(void __iomem *spbu_base)
{
	return true;
}

static bool is_memmn_fault(void __iomem *spbu_base)
{
	if (readq(spbu_base + OFFSET_FAULT_SOURCE) & (0x1UL << 8))
		return !!(readq(spbu_base + OFFSET_DLI_RLTD_FAULT) & (0xFFUL << 19));

	return false;
}

static bool is_dlmn_fault(void __iomem *spbu_base)
{
	return true;
}

static bool is_devmn_fault(void __iomem *spbu_base)
{
	if (readq(spbu_base + OFFSET_FAULT_SOURCE) & (0x1UL << 8))
		return !!(readq(spbu_base + OFFSET_DLI_RLTD_FAULT) & (0x1FUL << 27));

	return false;
}

static bool is_intpu_fault(void __iomem *spbu_base)
{
	if (readq(spbu_base + OFFSET_FAULT_SOURCE) & (0x1UL << 9))
		return !!(readq(spbu_base + OFFSET_SI_FAULT_STAT) & (0x1UL << 24));

	return false;
}

void sunway_fault_report(int node)
{
	struct work_struct *work = &driver_data.work;
	struct fault_data_item *item;
	void __iomem *spbu_base;
	u64 curr_data = 0;

	if (!driver_data.bmc_data.initialized) {
		pr_err("Report error but smi watcher not registered\n");
		return;
	}

	item = kzalloc(sizeof(*item), GFP_ATOMIC);
	if (!item)
		return;

	spbu_base = misc_platform_get_spbu_base(node);

	curr_data = 1;            /* version */
	curr_data |= (node << 8); /* node */

	if (is_spbu_fault(spbu_base))
		curr_data |= (0x1UL << 32);

	if (is_lcpm_fault(spbu_base))
		curr_data |= (0x1UL << 33);

	if (is_gcpm_fault(spbu_base))
		curr_data |= (0x1UL << 34);

	if (is_mc_fault(spbu_base))
		curr_data |= (0x1UL << 35);

	if (is_dli_fault(spbu_base))
		curr_data |= (0x1UL << 36);

	if (is_piu_fault(spbu_base))
		curr_data |= (0x1UL << 37);

	if (is_cgmn_fault(spbu_base))
		curr_data |= (0x1UL << 38);

	if (is_memmn_fault(spbu_base))
		curr_data |= (0x1UL << 39);

	if (is_dlmn_fault(spbu_base))
		curr_data |= (0x1UL << 40);

	if (is_devmn_fault(spbu_base))
		curr_data |= (0x1UL << 41);

	if (is_intpu_fault(spbu_base))
		curr_data |= (0x1UL << 42);

	item->fault_data = curr_data;

	spin_lock(&fault_data_lock);
	list_add_tail(&item->list, &fault_data_list);
	spin_unlock(&fault_data_lock);

	schedule_work(work);
}

static void fault_report_work_func(struct work_struct *work)
{
	struct sunway_fault_report_bmc_data *bmc_data = &driver_data.bmc_data;
	struct fault_data_item *item;
	u64 data;

	spin_lock(&fault_data_lock);
	if (list_empty(&fault_data_list)) {
		spin_unlock(&fault_data_lock);
		return;
	}

	item = list_first_entry(&fault_data_list, typeof(*item), list);
	list_del(&item->list);
	spin_unlock(&fault_data_lock);

	data = item->fault_data;

	if (data >> 32) {
		sunway_fault_report_send_data(bmc_data,
			(const unsigned char *)&data, sizeof(data));
	}

	kfree(item);
}

static int __init sunway_fault_report_driver_init(void)
{
	INIT_WORK(&driver_data.work, fault_report_work_func);

	return ipmi_smi_watcher_register(&driver_data.bmc_events);
}
late_initcall(sunway_fault_report_driver_init);
