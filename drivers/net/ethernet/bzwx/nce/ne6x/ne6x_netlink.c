// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include <linux/mutex.h>
#include <linux/netlink.h>

#include "ne6x.h"
#include "ne6x_reg.h"
#include "ne6x_debugfs.h"
#include "ne6x_dev.h"
#include "ne6x_netlink.h"

static struct sock *ne6x_nlsock;
static DEFINE_MUTEX(ne6x_msg_mutex);

static int ne6x_netlink_tab_add(struct ne6x_pf *pf, struct ne6x_rule *rule)
{
	struct ne6x_debug_table *table_info;
	struct device *dev;
	u32 table_id = 0xFFFFFFFF;
	int err;

	table_info = kzalloc(sizeof(*table_info), GFP_KERNEL);
	if (unlikely(!table_info))
		return -ENOMEM;

	dev = ne6x_pf_to_dev(pf);
	table_info->table = NE6X_REG_ACL_TABLE;
	table_info->size = NE6X_HASH_KEY_SIZE;
	memcpy(table_info->data, rule, sizeof(*rule));

	err = ne6x_reg_table_search(pf, table_info->table, &table_info->data[0],
				    table_info->size, NULL, table_info->size);
	if (err == -ENOENT) {
		table_info->size = NE6X_HASH_KEY_SIZE + NE6X_HASH_DATA_SIZE;
		err = ne6x_reg_table_insert(pf, table_info->table, &table_info->data[0],
					    table_info->size, &table_id);
	} else {
		dev_info(dev, "table exist\n");
		kfree(table_info);
		return -EEXIST;
	}

	if (err == 0) {
		dev_info(dev, "insert rule_id = 0x%x success!\n", table_id);
	} else if (err != -ETIMEDOUT) {
		dev_info(dev, "insert rule_id = 0x%x fail!\n", table_id);
		err = -EIO;
	} else {
		dev_info(dev, "insert rule_id = 0x%x timeout!\n", table_id);
		err = EAGAIN;
	}

	kfree(table_info);
	return err;
}

static int ne6x_netlink_tab_del(struct ne6x_pf *pf, struct ne6x_rule *rule)
{
	struct ne6x_debug_table *table_info;
	struct device *dev;
	int err;

	table_info = kzalloc(sizeof(*table_info), GFP_KERNEL);
	if (unlikely(!table_info))
		return -ENOMEM;

	dev = ne6x_pf_to_dev(pf);
	table_info->table = NE6X_REG_ACL_TABLE;
	table_info->size = NE6X_HASH_KEY_SIZE;
	memcpy(table_info->data, rule, sizeof(*rule));

	err = ne6x_reg_table_delete(pf, table_info->table, &table_info->data[0], table_info->size);
	dev_info(dev, "%s: %s\n", __func__, (err == 0) ? "success!" : "timeout!");
	kfree(table_info);

	return err;
}

static int ne6x_netlink_meter_write(struct ne6x_pf *pf, struct ne6x_meter *meter)
{
	struct meter_table vf_bw;
	struct device *dev;
	u32 cir_maxnum = 0xfffff;
	u32 cbs_maxnum = 0xffffff;
	u32 type_flag = 0;
	u32 type_map = 0;
	u32 cir;
	int err;

	if (meter->type_num > NE6X_METER_TYPE_MAX ||
	    meter->opcode > NE6X_METER_OPCODE_MAX)
		return -EINVAL;

	dev = ne6x_pf_to_dev(pf);
	type_flag |= BIT(meter->type_num);

	err = ne6x_reg_get_user_data(pf, NP_USER_DATA_DDOS_FLAG, &type_map);
	if (err)
		return err;

	if (meter->opcode)
		type_map |= type_flag;
	else
		type_map &= ~type_flag;

	err = ne6x_reg_set_user_data(pf, NP_USER_DATA_DDOS_FLAG, type_map);
	if (err)
		return err;

	cir = meter->value * 1000 + 1023;
	cir = min(cir / 1024, cir_maxnum);

	vf_bw.cir = cir;
	vf_bw.pir = min(cir + cir / 10, cir_maxnum);

	vf_bw.cbs = min(vf_bw.cir * 10000, cbs_maxnum);
	vf_bw.pbs = min(vf_bw.pir * 10000, cbs_maxnum);

	err = ne6x_reg_config_meter(pf, NE6X_METER1_TABLE |
					NE6X_METER_SUBSET(NE6X_METER_SUBSET0) |
					meter->type_num, (u32 *)&vf_bw, sizeof(vf_bw));

	dev_info(dev, "%s\n", err ? "write meter fail!" : "write meter success!");

	return err;
}

static int ne6x_netlink_rcv_msg(struct nlmsghdr *nlh)
{
	char name[IFNAMSIZ] = {0};
	struct net_device *dev;
	struct ne6x_pf *pf;
	void *data;
	int err;

	strncpy(name, nlmsg_data(nlh), IFNAMSIZ - 1);
	dev = __dev_get_by_name(&init_net, name);
	if (unlikely(!dev))
		return -ENODEV;

	if (unlikely(!netif_is_ne6x(dev)))
		return -ENOTSUPP;

	pf = ne6x_netdev_to_pf(dev);
	data = nlmsg_data(nlh) + IFNAMSIZ;

	switch (nlh->nlmsg_type) {
	case NE6X_NLMSG_TAB_ADD:
		/* if entry exists, treat it as insertion success */
		err = ne6x_netlink_tab_add(pf, data);
		if (err == -EEXIST)
			err = 0;
		break;
	case NE6X_NLMSG_TAB_DEL:
		err = ne6x_netlink_tab_del(pf, data);
		break;
	case NE6X_NLMSG_METER_WRITE:
		err = ne6x_netlink_meter_write(pf, data);
		break;
	default:
		return -ENOTSUPP;
	}

	return err;
}

static void ne6x_netlink_ack(struct sk_buff *in_skb, unsigned long *status)
{
	struct sk_buff *skb_out;
	struct nlmsghdr *nlh;
	size_t payload;

	payload = BITS_TO_LONGS(NE6X_RULE_BATCH_MAX) * sizeof(unsigned long);
	skb_out = nlmsg_new(payload, GFP_KERNEL);
	if (unlikely(!skb_out)) {
		NETLINK_CB(in_skb).sk->sk_err = ENOBUFS;
		NETLINK_CB(in_skb).sk->sk_error_report(NETLINK_CB(in_skb).sk);
		return;
	}

	nlh = nlmsg_put(skb_out, NETLINK_CB(in_skb).portid, 0, NLMSG_DONE, payload, 0);
	if (unlikely(!nlh)) {
		nlmsg_free(skb_out);
		return;
	}

	NETLINK_CB(skb_out).dst_group = 0;
	bitmap_copy(nlmsg_data(nlh), status, NE6X_RULE_BATCH_MAX);

	nlmsg_unicast(in_skb->sk, skb_out, NETLINK_CB(in_skb).portid);
}

static void ne6x_netlink_rcv(struct sk_buff *skb)
{
	DECLARE_BITMAP(status, NE6X_RULE_BATCH_MAX);
	u32 idx = 0;

	bitmap_zero(status, NE6X_RULE_BATCH_MAX);
	mutex_lock(&ne6x_msg_mutex);
	while (skb->len >= nlmsg_total_size(0) && idx < NE6X_RULE_BATCH_MAX) {
		struct nlmsghdr *nlh;
		int msglen, err;

		nlh = nlmsg_hdr(skb);

		if (unlikely(nlh->nlmsg_len < NLMSG_HDRLEN || skb->len < nlh->nlmsg_len)) {
			set_bit(idx, status);
			goto skip;
		}

		err = ne6x_netlink_rcv_msg(nlh);
		if (err)
			set_bit(idx, status);

skip:
		msglen = NLMSG_ALIGN(nlh->nlmsg_len);
		if (unlikely(msglen > skb->len))
			msglen = skb->len;

		idx++;
		skb_pull(skb, msglen);
	}

	ne6x_netlink_ack(skb, status);
	mutex_unlock(&ne6x_msg_mutex);
}

/**
 * ne6x_netlink_init - start up netlink resource for the driver
 **/
void ne6x_netlink_init(void)
{
	struct netlink_kernel_cfg ne6x_netlink_cfg = {
		.input = ne6x_netlink_rcv,
	};

	ne6x_nlsock = netlink_kernel_create(&init_net, NE6X_NETLINK, &ne6x_netlink_cfg);
	if (unlikely(!ne6x_nlsock))
		pr_warn("Init of netlink failed\n");
}

/**
 * ne6x_netlink_exit - clean out the driver's netlink resource
 **/
void ne6x_netlink_exit(void)
{
	netlink_kernel_release(ne6x_nlsock);
	ne6x_nlsock = NULL;
}
