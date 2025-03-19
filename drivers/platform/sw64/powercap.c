// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 WXIAT
 */

#define pr_fmt(fmt) "sunway-powercap: " fmt

#include <linux/acpi.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/ipmi.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/timer.h>

#define SUNWAY_POWERCAP_NETFN 0x3A

#define SUNWAY_POWERCAP_ACPI_NOTIFY_VALUE 0x84

enum sunway_powercap_version {
	SUNWAY_POWERCAP_V1 = 1,
	SUNWAY_POWERCAP_VERSION_MAX,
};

enum sunway_powercap_mode {
	SUNWAY_POWERCAP_MODE_POLL = 0,
	SUNWAY_POWERCAP_MODE_INTERRUPT,
};

enum sunway_powercap_poll_interval {
	SUNWAY_POWERCAP_POLL_INTERVAL0 = 50,
	SUNWAY_POWERCAP_POLL_INTERVAL1 = 100,
	SUNWAY_POWERCAP_POLL_INTERVAL2 = 200,
	SUNWAY_POWERCAP_POLL_INTERVAL3 = 250,
};

enum sunway_powercap_cmd {
	SUNWAY_POWERCAP_CMD_GET_CFG = 0x30,
	SUNWAY_POWERCAP_CMD_GET_FREQ = 0x31,
	SUNWAY_POWERCAP_CMD_ACK = 0x32,
};

enum sunway_powercap_state {
	SUNWAY_POWERCAP_STATE_FREE  = 0x0F,
	SUNWAY_POWERCAP_STATE_LIMIT = 0xF0,
};

#pragma pack(1)

struct sunway_powercap_cfg {
	u8 version;
	u8 mode;
	u8 poll_interval;
	u8 reserved;
};

#define FREQ_FLAG_ENABLE	(1 << 0)
#define FREQ_FLAG_FREE		(1 << 1)
#define FREQ_FLAG_TERMINATE	(1 << 2)

struct sunway_powercap_freq {
	u32 target_freq;
	u16 target_core;
	u16 flags;
};

#define ACK_FLAG_VALID_SIZE	(1 << 0)
#define ACK_FLAG_VALID_VERSION	(1 << 1)
#define ACK_FLAG_VALID_MODE	(1 << 2)
#define ACK_FLAG_VALID_INTERVAL	(1 << 3)
#define ACK_FLAG_VALID_FREQ	(1 << 4)
#define ACK_FLAG_VALID_NODE	(1 << 5)
#define ACK_FLAG_VALID_CORE	(1 << 6)

struct sunway_powercap_ack {
	u8 cmd;
	u8 flags;
	u16 reserved;
};

/* Reset to default packing */
#pragma pack()

struct sunway_powercap_bmc_data {
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

struct sunway_powercap_driver_data {
	struct device *dev;

	unsigned char version;
	unsigned char mode;
	unsigned char poll_interval;

	struct timer_list timer;
	struct work_struct work;

	struct ipmi_smi_watcher bmc_events;
	struct ipmi_user_hndl ipmi_hndlrs;
	struct sunway_powercap_bmc_data bmc_data;
};

struct sunway_powercap_cpu {
	unsigned int state;
	unsigned int node;
	unsigned int core;
	struct cpufreq_policy *policy;
	struct freq_qos_request *qos_req;
};

static void sunway_powercap_register_bmc(int iface, struct device *dev);
static void sunway_powercap_bmc_gone(int iface);
static void sunway_powercap_msg_handler(struct ipmi_recv_msg *msg,
		void *user_msg_data);

static struct sunway_powercap_driver_data driver_data = {
	.bmc_events = {
		.new_smi = sunway_powercap_register_bmc,
		.smi_gone = sunway_powercap_bmc_gone,
	},

	.ipmi_hndlrs = {
		.ipmi_recv_hndl = sunway_powercap_msg_handler,
	},
};

static struct sunway_powercap_cpu powercap_cpu_data[NR_CPUS];

static unsigned char powercap_freq_ack[IPMI_MAX_MSG_LENGTH];

static int sunway_powercap_send_message(struct sunway_powercap_bmc_data *bmc_data)
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

static int sunway_powercap_send_cmd(struct sunway_powercap_bmc_data *bmc_data,
		unsigned char cmd, const unsigned char *data, unsigned short data_len)
{
	bmc_data->tx_message.cmd = cmd;
	bmc_data->tx_message.data_len = data_len;

	if (data_len)
		memcpy(bmc_data->tx_msg_data, data, data_len);

	return sunway_powercap_send_message(bmc_data);
}

static int sunway_powercap_query(struct sunway_powercap_bmc_data *bmc_data,
		unsigned char cmd, const char *info)
{
	int ret;

	ret = sunway_powercap_send_cmd(bmc_data, cmd, NULL, 0);
	if (ret) {
		dev_err(bmc_data->bmc_device, "unable to query %s\n", info);
		return ret;
	}

	wait_for_completion(&bmc_data->complete);

	if (bmc_data->rx_result) {
		dev_err(bmc_data->bmc_device, "rx error 0x%x when query %s\n",
				bmc_data->rx_result, info);
		return -EINVAL;
	}

	return 0;
}

static int sunway_powercap_ack_bmc(struct sunway_powercap_bmc_data *bmc_data,
		const struct sunway_powercap_ack *ack, int num)
{
	unsigned char cmd = SUNWAY_POWERCAP_CMD_ACK;
	int ret;

	ret = sunway_powercap_send_cmd(bmc_data, cmd,
			(const char *)ack, sizeof(*ack) * num);
	if (ret) {
		dev_err(bmc_data->bmc_device, "unable to send ack\n");
		return ret;
	}

	wait_for_completion(&bmc_data->complete);

	return 0;
}

static inline unsigned int
powercap_target_node(const struct sunway_powercap_freq *freq)
{
	return freq->target_core & 0x3F;
}

static inline unsigned int
powercap_target_core(const struct sunway_powercap_freq *freq)
{
	return (freq->target_core >> 6) & 0x3FF;
}

static inline bool is_powercap_cpu_match(const struct sunway_powercap_cpu *data,
		unsigned int node, unsigned int core)
{
	if ((node != 0x3F) && (data->node != node))
		return false;

	if ((core != 0x3FF) && (data->core != core))
		return false;

	return true;
}

static int sunway_powercap_validate_freq(const struct sunway_powercap_freq *freq,
		struct sunway_powercap_ack *ack)
{
	unsigned int node = powercap_target_node(freq);
	unsigned int core = powercap_target_core(freq);
	int i;

	/* Currently, core must be 0x3FF(all bits are 1) */
	if (core != 0x3FF)
		goto out_validate_freq;

	for (i = 0; i < ARRAY_SIZE(powercap_cpu_data); i++) {
		struct cpufreq_policy *policy = powercap_cpu_data[i].policy;
		unsigned int target_freq = freq->target_freq;

		if (!policy)
			continue;

		if (!is_powercap_cpu_match(&powercap_cpu_data[i], node, core))
			continue;

		/* Now we confirm that core and node are valid */
		ack->flags |= ACK_FLAG_VALID_NODE;
		ack->flags |= ACK_FLAG_VALID_CORE;

		if (cpufreq_frequency_table_get_index(policy, target_freq) < 0) {
			pr_err("invalid target freq %u\n", target_freq);
			return -EINVAL;
		}

		ack->flags |= ACK_FLAG_VALID_FREQ;

		return 0;
	}

out_validate_freq:
	pr_err("invalid core %u on node %u\n", core, node);

	return -EINVAL;
}

static inline bool is_powercap_enabled(const struct sunway_powercap_freq *freq)
{
	return !!(freq->flags & FREQ_FLAG_ENABLE);
}

static inline bool is_powercap_no_limit(const struct sunway_powercap_freq *freq)
{
	return !!(freq->flags & FREQ_FLAG_FREE);
}

static inline bool
is_powercap_freq_data_terminate(const struct sunway_powercap_freq *freq)
{
	return !!(freq->flags & FREQ_FLAG_TERMINATE);
}

static int sunway_powercap_handle_free_cpus(struct cpufreq_policy *policy,
		struct freq_qos_request *req, const struct sunway_powercap_freq *freq)
{
	int ret, related_cpu;

	if (!is_powercap_enabled(freq) || is_powercap_no_limit(freq))
		return 0;

	ret = freq_qos_add_request(&policy->constraints,
			req, FREQ_QOS_MAX, freq->target_freq);
	if (ret < 0) {
		pr_err("unable to add qos request on cpus %*pbl\n",
				cpumask_pr_args(policy->related_cpus));
		return ret;
	}

	for_each_cpu(related_cpu, policy->related_cpus)
		powercap_cpu_data[related_cpu].state = SUNWAY_POWERCAP_STATE_LIMIT;

	return 0;
}

static int sunway_powercap_handle_limit_cpus(struct cpufreq_policy *policy,
		struct freq_qos_request *req, const struct sunway_powercap_freq *freq)
{
	int ret, related_cpu;

	if (is_powercap_enabled(freq) && !is_powercap_no_limit(freq)) {
		ret = freq_qos_update_request(req, freq->target_freq);
		if (ret < 0)
			pr_err("unable to update qos request on cpus %*pbl\n",
					cpumask_pr_args(policy->related_cpus));
		return ret;
	}

	ret = freq_qos_remove_request(req);
	if (ret < 0) {
		pr_err("unable to remove qos request on cpus %*pbl\n",
				cpumask_pr_args(policy->related_cpus));
		return ret;
	}

	for_each_cpu(related_cpu, policy->related_cpus)
		powercap_cpu_data[related_cpu].state = SUNWAY_POWERCAP_STATE_FREE;

	return 0;
}

static int sunway_powercap_handle_one_freq(const struct sunway_powercap_freq *freq,
		struct sunway_powercap_ack *ack)
{
	int i;
	unsigned int node = powercap_target_node(freq);
	unsigned int core = powercap_target_core(freq);
	unsigned int state;
	struct freq_qos_request *req;
	struct cpufreq_policy *policy;
	cpumask_var_t done;

	/* Ack freq */
	ack->cmd = SUNWAY_POWERCAP_CMD_GET_FREQ;

	/* Size must be valid here */
	ack->flags |= ACK_FLAG_VALID_SIZE;

	if (sunway_powercap_validate_freq(freq, ack))
		return -EINVAL;

	if (!alloc_cpumask_var(&done, GFP_KERNEL))
		return -ENOMEM;

	cpumask_clear(done);

	for (i = 0; i < ARRAY_SIZE(powercap_cpu_data); i++) {
		policy = powercap_cpu_data[i].policy;

		if (!policy || policy_is_inactive(policy))
			continue;

		if (cpumask_test_cpu(i, done))
			continue;

		if (!is_powercap_cpu_match(&powercap_cpu_data[i], node, core))
			continue;

		state = powercap_cpu_data[i].state;
		req = powercap_cpu_data[i].qos_req;

		if (state == SUNWAY_POWERCAP_STATE_FREE)
			sunway_powercap_handle_free_cpus(policy, req, freq);
		else if (state == SUNWAY_POWERCAP_STATE_LIMIT)
			sunway_powercap_handle_limit_cpus(policy, req, freq);
		else
			pr_err("cpu %d with invalid state 0x%x\n", i, state);

		cpumask_or(done, done, policy->related_cpus);
	}

	free_cpumask_var(done);

	return 0;
}

static int sunway_powercap_poll_once(struct sunway_powercap_bmc_data *bmc_data)
{
	struct sunway_powercap_ack *ack;
	struct sunway_powercap_freq *freq;
	unsigned char cmd = SUNWAY_POWERCAP_CMD_GET_FREQ;
	int ret, num, i;

query_freq:
	/* Clean ACK data */
	memset(powercap_freq_ack, 0, sizeof(powercap_freq_ack));

	ret = sunway_powercap_query(bmc_data, cmd, "freq");
	if (ret)
		return ret;

	ack = (struct sunway_powercap_ack *)&powercap_freq_ack[0];
	freq = (struct sunway_powercap_freq *)&bmc_data->rx_msg_data[0];

	/* Number of freq data */
	num = bmc_data->rx_msg_len >> 3;

	if (!num || (bmc_data->rx_msg_len & 0x7)) {
		dev_err(bmc_data->bmc_device, "invalid freq size %d\n",
				bmc_data->rx_msg_len);

		/**
		 * The size must be multiple of 8 bytes, otherwise
		 * send only one ack with invalid size.
		 */
		ack->cmd = cmd;
		ack->flags &= ~ACK_FLAG_VALID_SIZE;
		sunway_powercap_ack_bmc(bmc_data, ack, 1);

		return -EINVAL;
	}

	/* Handle freq data one by one */
	for (i = 0; i < num; i++)
		sunway_powercap_handle_one_freq(freq + i, ack + i);

	sunway_powercap_ack_bmc(bmc_data, ack, num);

	/* More freq Data needs to be queried */
	if (!is_powercap_freq_data_terminate(&freq[num - 1]))
		goto query_freq;

	return 0;
}

static inline bool is_legal_poll_interval(u8 interval)
{
	return (interval == SUNWAY_POWERCAP_POLL_INTERVAL0) ||
		(interval == SUNWAY_POWERCAP_POLL_INTERVAL1) ||
		(interval == SUNWAY_POWERCAP_POLL_INTERVAL2) ||
		(interval == SUNWAY_POWERCAP_POLL_INTERVAL3);
}

static int sunway_powercap_validate_cfg(const struct sunway_powercap_cfg *cfg,
		struct sunway_powercap_ack *ack)
{
	bool valid = true;

	if (!cfg->version || (cfg->version >= SUNWAY_POWERCAP_VERSION_MAX)) {
		pr_err("invalid version %d\n", cfg->version);
		valid = false;
	} else
		ack->flags |= ACK_FLAG_VALID_VERSION;

	if (cfg->mode > SUNWAY_POWERCAP_MODE_INTERRUPT) {
		pr_err("invalid mode %d\n", cfg->mode);
		valid = false;
	} else
		ack->flags |= ACK_FLAG_VALID_MODE;

	if ((cfg->mode == SUNWAY_POWERCAP_MODE_POLL) &&
			!is_legal_poll_interval(cfg->poll_interval)) {
		pr_err("invalid poll interval %dms\n", cfg->poll_interval);
		valid = false;
	} else
		ack->flags |= ACK_FLAG_VALID_INTERVAL;

	return valid ? 0 : -EINVAL;
}

static void sunway_powercap_add_timer(void)
{
	unsigned long expire;
	struct timer_list *timer = &driver_data.timer;

	expire = jiffies + msecs_to_jiffies(driver_data.poll_interval);
	timer->expires = round_jiffies_relative(expire);
	add_timer(timer);
}

static void sunway_powercap_poll_func(struct timer_list *t)
{
	struct work_struct *work = &driver_data.work;

	schedule_work(work);
	sunway_powercap_add_timer();
}

static void sunway_powercap_acpi_notify(acpi_handle device, u32 value, void *data)
{
	struct device *dev = driver_data.dev;
	struct work_struct *work = &driver_data.work;

	if (value != SUNWAY_POWERCAP_ACPI_NOTIFY_VALUE) {
		dev_err(dev, "unknown acpi notify value\n");
		return;
	}

	schedule_work(work);
}

static int sunway_powercap_setup_cfg(const struct sunway_powercap_cfg *cfg)
{
	bool is_poll_mode = (cfg->mode == SUNWAY_POWERCAP_MODE_POLL);
	struct device *dev = driver_data.dev;
	struct acpi_device *adev;
	acpi_status status;

	driver_data.version = cfg->version;
	driver_data.mode = cfg->mode;
	driver_data.poll_interval = cfg->poll_interval;

	if (is_poll_mode) {
		timer_setup(&driver_data.timer, sunway_powercap_poll_func, 0);
		sunway_powercap_add_timer();
	} else {
		/* Must be interrupt mode */

		adev = ACPI_COMPANION(dev);
		if (WARN_ON(!adev))
			return -EINVAL;

		status = acpi_install_notify_handler(adev->handle,
				ACPI_DEVICE_NOTIFY,
				sunway_powercap_acpi_notify,
				NULL);
		if (ACPI_FAILURE(status)) {
			dev_err(dev, "unable to register notifier %08x\n",
					status);
			return -EINVAL;
		}
	}

	dev_info(dev, "found with version %d and %s mode\n",
			driver_data.version,
			is_poll_mode ? "polling" : "interrupt");

	return 0;
}

static int sunway_powercap_init_cfg(struct sunway_powercap_bmc_data *bmc_data)
{
	struct sunway_powercap_cfg cfg = { 0 };
	struct sunway_powercap_ack ack = { 0 };
	unsigned char cmd = SUNWAY_POWERCAP_CMD_GET_CFG;
	int ret;

	ret = sunway_powercap_query(bmc_data, cmd, "cfg");
	if (ret)
		return ret;

	ack.cmd = cmd;

	if (bmc_data->rx_msg_len != sizeof(cfg)) {
		dev_err(bmc_data->bmc_device, "invalid cfg size %d\n",
				bmc_data->rx_msg_len);
		ret = -EINVAL;
	}

	if (!ret) {
		ack.flags |= ACK_FLAG_VALID_SIZE;
		memcpy(&cfg, bmc_data->rx_msg_data, sizeof(cfg));

		ret = sunway_powercap_validate_cfg(&cfg, &ack);
		if (!ret)
			ret = sunway_powercap_setup_cfg(&cfg);
	}

	sunway_powercap_ack_bmc(bmc_data, &ack, 1);

	return ret;
}

static void sunway_powercap_register_bmc(int iface, struct device *dev)
{
	struct sunway_powercap_bmc_data *bmc_data = &driver_data.bmc_data;
	int ret;

	/* Multiple BMC for suwnay powercap are not supported */
	if (bmc_data->initialized) {
		dev_err(dev, "unable to register sunway-powercap repeatedly\n");
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
	bmc_data->tx_message.netfn = SUNWAY_POWERCAP_NETFN;
	bmc_data->tx_message.data = bmc_data->tx_msg_data;

	init_completion(&bmc_data->complete);

	ret = sunway_powercap_init_cfg(bmc_data);
	if (ret) {
		dev_err(dev, "unable to initialize powercap configuration\n");
		goto out_destroy_user;
	}

	bmc_data->initialized = true;

	return;

out_destroy_user:
	ipmi_destroy_user(bmc_data->user);
}

static void sunway_powercap_bmc_gone(int iface)
{
	struct sunway_powercap_bmc_data *bmc_data = &driver_data.bmc_data;

	if (WARN_ON(bmc_data->interface != iface))
		return;

	ipmi_destroy_user(bmc_data->user);
}

static void sunway_powercap_msg_handler(struct ipmi_recv_msg *msg,
		void *user_msg_data)
{
	struct sunway_powercap_bmc_data *bmc_data = user_msg_data;

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

static void do_powercap(struct work_struct *work)
{
	sunway_powercap_poll_once(&driver_data.bmc_data);
}

static int sunway_powercap_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cpufreq_policy *policy;
	int cpu;

	driver_data.dev = dev;

	INIT_WORK(&driver_data.work, do_powercap);

	for_each_possible_cpu(cpu) {
		int related_cpu, rcid = cpu_physical_id(cpu);
		struct freq_qos_request *req;

		/* Initial state */
		powercap_cpu_data[related_cpu].state = SUNWAY_POWERCAP_STATE_FREE;

		powercap_cpu_data[related_cpu].core = rcid_to_core_id(rcid);
		powercap_cpu_data[related_cpu].node = rcid_to_domain_id(rcid);

		if (powercap_cpu_data[cpu].policy)
			continue;

		policy = cpufreq_cpu_get(cpu);
		if (!policy)
			continue;

		req = devm_kzalloc(dev, sizeof(*req), GFP_KERNEL);
		if (!req)
			return -ENOMEM;

		for_each_cpu(related_cpu, policy->related_cpus) {
			powercap_cpu_data[related_cpu].policy = policy;
			powercap_cpu_data[related_cpu].qos_req = req;
		}
	}

	return ipmi_smi_watcher_register(&driver_data.bmc_events);
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id sunway_powercap_acpi_match[] = {
	{ "SUNW0203", 0 },
	{},
};
#endif

static struct platform_driver sunway_powercap_driver = {
	.probe = sunway_powercap_probe,
	.driver = {
		.name = "sunway-powercap",
		.acpi_match_table = ACPI_PTR(sunway_powercap_acpi_match),
	},
};

static int __init sunway_powercap_driver_init(void)
{
	return platform_driver_register(&sunway_powercap_driver);
}
late_initcall(sunway_powercap_driver_init);
