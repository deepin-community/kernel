// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <net/sock.h>
#include <net/bonding.h>
#include <linux/rtnetlink.h>
#include <linux/net.h>
#include <linux/mutex.h>
#include <linux/netdevice.h>
#include <linux/version.h>

#include "hinic3_lld.h"
#include "hinic3_srv_nic.h"
#include "hinic3_nic_dev.h"
#include "hinic3_hw.h"
#include "mpu_inband_cmd.h"
#include "hinic3_hwdev.h"
#include "hinic3_bond.h"

#define PORT_INVALID_ID         0xFF

#define STATE_SYNCHRONIZATION_INDEX 3

struct hinic3_bond_dev {
	char name[BOND_NAME_MAX_LEN];
	struct bond_attr bond_attr;
	struct bond_attr new_attr;
	struct bonding *bond;
	void *ppf_hwdev;
	struct kref ref;
#define BOND_DEV_STATUS_IDLE         0x0
#define BOND_DEV_STATUS_ACTIVATED    0x1
	u8 status;
	u8 slot_used[HINIC3_BOND_USER_NUM];
	struct workqueue_struct *wq;
	struct delayed_work bond_work;
	struct bond_tracker tracker;
	spinlock_t lock; /* lock for change status */
};

typedef void (*bond_service_func)(const char *bond_name, void *bond_attr,
				  enum bond_service_proc_pos pos);

static DEFINE_MUTEX(g_bond_service_func_mutex);

static bond_service_func g_bond_service_func[HINIC3_BOND_USER_NUM];

struct hinic3_bond_mngr {
	u32 cnt;
	struct hinic3_bond_dev *bond_dev[BOND_MAX_NUM];
	struct socket *rtnl_sock;
};

static struct hinic3_bond_mngr bond_mngr = { .cnt = 0 };
static DEFINE_MUTEX(g_bond_mutex);

static bool bond_dev_is_activated(const struct hinic3_bond_dev *bdev)
{
	return bdev->status == BOND_DEV_STATUS_ACTIVATED;
}

#define PCI_DBDF(dom, bus, dev, func) \
	(((dom) << 16) | ((bus) << 8) | ((dev) << 3) | ((func) & 0x7))

#ifdef __PCLINT__
static inline bool netif_is_bond_master(const struct net_device *dev)
{
	return (dev->flags & IFF_MASTER) && (dev->priv_flags & IFF_BONDING);
}
#endif

static u32 bond_gen_uplink_id(struct hinic3_bond_dev *bdev)
{
	u32 uplink_id = 0;
	u8 i;
	struct hinic3_nic_dev *nic_dev = NULL;
	struct pci_dev *pdev = NULL;
	u32 domain, bus, dev, func;

	spin_lock(&bdev->lock);
	for (i = 0; i < BOND_PORT_MAX_NUM; i++) {
		if (BITMAP_JUDGE(bdev->bond_attr.slaves, i)) {
			if (!bdev->tracker.ndev[i])
				continue;
			nic_dev = netdev_priv(bdev->tracker.ndev[i]);
			pdev = nic_dev->pdev;
			domain = (u32)pci_domain_nr(pdev->bus);
			bus = pdev->bus->number;
			dev = PCI_SLOT(pdev->devfn);
			func = PCI_FUNC(pdev->devfn);
			uplink_id = PCI_DBDF(domain, bus, dev, func);
			break;
		}
	}
	spin_unlock(&bdev->lock);

	return uplink_id;
}

static struct hinic3_nic_dev *get_nic_dev_safe(struct net_device *ndev)
{
	struct hinic3_lld_dev *lld_dev = NULL;

	lld_dev = hinic3_get_lld_dev_by_netdev(ndev);
	if (!lld_dev)
		return NULL;

	return netdev_priv(ndev);
}

static u8 bond_get_slaves_bitmap(struct hinic3_bond_dev *bdev,
				 struct bonding *bond)
{
	struct slave *slave = NULL;
	struct list_head *iter = NULL;
	struct hinic3_nic_dev *nic_dev = NULL;
	u8 bitmap = 0;
	u8 port_id;

	rcu_read_lock();
	bond_for_each_slave_rcu(bond, slave, iter) {
		nic_dev = get_nic_dev_safe(slave->dev);
		if (!nic_dev)
			continue;

		port_id = hinic3_physical_port_id(nic_dev->hwdev);
		BITMAP_SET(bitmap, port_id);
		(void)iter;
	}
	rcu_read_unlock();

	return bitmap;
}

static void bond_update_attr(struct hinic3_bond_dev *bdev, struct bonding *bond)
{
	spin_lock(&bdev->lock);

	bdev->new_attr.bond_mode = (u16)bond->params.mode;
	bdev->new_attr.bond_id = bdev->bond_attr.bond_id;
	bdev->new_attr.up_delay = (u16)bond->params.updelay;
	bdev->new_attr.down_delay = (u16)bond->params.downdelay;
	bdev->new_attr.slaves = 0;
	bdev->new_attr.active_slaves = 0;
	bdev->new_attr.lacp_collect_slaves = 0;
	bdev->new_attr.first_roce_func = DEFAULT_ROCE_BOND_FUNC;

	/* Only support L2/L34/L23 three policy */
	if (bond->params.xmit_policy <= BOND_XMIT_POLICY_LAYER23)
		bdev->new_attr.xmit_hash_policy = (u8)bond->params.xmit_policy;
	else
		bdev->new_attr.xmit_hash_policy = BOND_XMIT_POLICY_LAYER2;

	bdev->new_attr.slaves = bond_get_slaves_bitmap(bdev, bond);

	spin_unlock(&bdev->lock);
}

static u8 bond_get_netdev_idx(const struct hinic3_bond_dev *bdev,
			      const struct net_device *ndev)
{
	u8 i;

	for (i = 0; i < BOND_PORT_MAX_NUM; i++) {
		if (bdev->tracker.ndev[i] == ndev)
			return i;
	}

	return PORT_INVALID_ID;
}

static u8 bond_dev_track_port(struct hinic3_bond_dev *bdev,
			      struct net_device *ndev)
{
	u8 port_id;
	void *ppf_hwdev = NULL;
	struct hinic3_nic_dev *nic_dev = NULL;
	struct hinic3_lld_dev *ppf_lld_dev = NULL;

	nic_dev = get_nic_dev_safe(ndev);
	if (!nic_dev) {
		pr_warn("hinic3_bond: invalid slave: %s\n", ndev->name);
		return PORT_INVALID_ID;
	}

	ppf_lld_dev = hinic3_get_ppf_lld_dev_unsafe(nic_dev->lld_dev);
	if (ppf_lld_dev)
		ppf_hwdev = ppf_lld_dev->hwdev;

	pr_info("hinic3_bond: track ndev:%s", ndev->name);
	port_id = hinic3_physical_port_id(nic_dev->hwdev);

	spin_lock(&bdev->lock);
	/* attach netdev to the port position associated with it */
	if (bdev->tracker.ndev[port_id]) {
		pr_warn("hinic3_bond: Old ndev:%s is replaced\n",
			bdev->tracker.ndev[port_id]->name);
	} else {
		bdev->tracker.cnt++;
	}
	bdev->tracker.ndev[port_id] = ndev;
	bdev->tracker.netdev_state[port_id].link_up = 0;
	bdev->tracker.netdev_state[port_id].tx_enabled = 0;
	if (!bdev->ppf_hwdev)
		bdev->ppf_hwdev = ppf_hwdev;
	pr_info("TRACK cnt: %d, slave_name(%s)\n",
		bdev->tracker.cnt, ndev->name);
	spin_unlock(&bdev->lock);

	return port_id;
}

static void bond_dev_untrack_port(struct hinic3_bond_dev *bdev, u8 idx)
{
	spin_lock(&bdev->lock);

	if (bdev->tracker.ndev[idx]) {
		pr_info("hinic3_bond: untrack port:%u ndev:%s cnt:%d\n", idx,
			bdev->tracker.ndev[idx]->name, bdev->tracker.cnt);
		bdev->tracker.ndev[idx] = NULL;
		bdev->tracker.cnt--;
	}

	spin_unlock(&bdev->lock);
}

static void bond_slave_event(struct hinic3_bond_dev *bdev, struct slave *slave)
{
	u8 idx;

	idx = bond_get_netdev_idx(bdev, slave->dev);
	if (idx == PORT_INVALID_ID)
		idx = bond_dev_track_port(bdev, slave->dev);
	if (idx == PORT_INVALID_ID)
		return;

	spin_lock(&bdev->lock);
	bdev->tracker.netdev_state[idx].link_up = bond_slave_is_up(slave);
	bdev->tracker.netdev_state[idx].tx_enabled = bond_slave_is_up(slave) &&
		bond_is_active_slave(slave);
	spin_unlock(&bdev->lock);

	queue_delayed_work(bdev->wq, &bdev->bond_work, 0);
}

static bool bond_eval_bonding_stats(const struct hinic3_bond_dev *bdev,
				    struct bonding *bond)
{
	int mode;

	mode = BOND_MODE(bond);
	if (mode != BOND_MODE_8023AD &&
	    mode != BOND_MODE_XOR &&
	    mode != BOND_MODE_ACTIVEBACKUP) {
		pr_err("hinic3_bond: Wrong mode:%d\n", mode);
		return false;
	}

	return bdev->tracker.cnt > 0;
}

static void bond_master_event(struct hinic3_bond_dev *bdev,
			      struct bonding *bond)
{
	spin_lock(&bdev->lock);
	bdev->tracker.is_bonded = bond_eval_bonding_stats(bdev, bond);
	spin_unlock(&bdev->lock);

	queue_delayed_work(bdev->wq, &bdev->bond_work, 0);
}

static struct hinic3_bond_dev *bond_get_bdev(struct bonding *bond)
{
	struct hinic3_bond_dev *bdev = NULL;
	int bid;

	if (!bond) {
		pr_err("hinic3_bond: bond is NULL\n");
		return NULL;
	}

	mutex_lock(&g_bond_mutex);
	for (bid = BOND_FIRST_ID; bid <= BOND_MAX_ID; bid++) {
		bdev = bond_mngr.bond_dev[bid];
		if (!bdev)
			continue;

		if (bond == bdev->bond) {
			mutex_unlock(&g_bond_mutex);
			return bdev;
		}

		if (strncmp(bond->dev->name,
			    bdev->name, BOND_NAME_MAX_LEN) == 0) {
			bdev->bond = bond;
			return bdev;
		}
	}
	mutex_unlock(&g_bond_mutex);
	return NULL;
}

static struct bonding *get_bonding_by_netdev(struct net_device *ndev)
{
	struct bonding *bond = NULL;
	struct slave *slave = NULL;

	if (netif_is_bond_master(ndev)) {
		bond = netdev_priv(ndev);
	} else if (netif_is_bond_slave(ndev)) {
		slave = bond_slave_get_rtnl(ndev);
		if (slave) {
			bond = bond_get_bond_by_slave(slave);
		}
	}

	return bond;
}

bool hinic3_is_bond_dev_status_actived(struct net_device *ndev)
{
	struct hinic3_bond_dev *bdev = NULL;
	struct bonding *bond = NULL;

	if (!ndev) {
		pr_err("hinic3_bond: netdev is NULL\n");
		return false;
	}

	bond = get_bonding_by_netdev(ndev);
	bdev = bond_get_bdev(bond);
	if (!bdev)
		return false;

	return bdev->status == BOND_DEV_STATUS_ACTIVATED;
}
EXPORT_SYMBOL(hinic3_is_bond_dev_status_actived);
/*lint +e580 +e546*/

static void bond_handle_rtnl_event(struct net_device *ndev)
{
	struct hinic3_bond_dev *bdev = NULL;
	struct bonding *bond = NULL;
	struct slave *slave = NULL;

	bond = get_bonding_by_netdev(ndev);
	bdev = bond_get_bdev(bond);
	if (!bdev)
		return;

	bond_update_attr(bdev, bond);

	if (netif_is_bond_slave(ndev)) {
		slave = bond_slave_get_rtnl(ndev);
		bond_slave_event(bdev, slave);
	} else {
		bond_master_event(bdev, bond);
	}
}

static void bond_rtnl_data_ready(struct sock *sk)
{
	struct net_device *ndev = NULL;
	struct ifinfomsg *ifinfo = NULL;
	struct nlmsghdr *hdr = NULL;
	struct sk_buff *skb = NULL;
	int err = 0;

	skb = skb_recv_datagram(sk, 0, &err);
	if (err != 0 || !skb)
		return;

	hdr = (struct nlmsghdr *)skb->data;
	if (!hdr ||
	    !NLMSG_OK(hdr, skb->len) ||
	    hdr->nlmsg_type != RTM_NEWLINK ||
	    !rtnl_is_locked()) {
		goto free_skb;
	}

	ifinfo = nlmsg_data(hdr);
	ndev = dev_get_by_index(&init_net, ifinfo->ifi_index);
	if (ndev) {
		bond_handle_rtnl_event(ndev);
		dev_put(ndev);
	}

free_skb:
	kfree_skb(skb);
}

static int bond_enable_netdev_event(void)
{
	struct sockaddr_nl addr = {
		.nl_family = AF_NETLINK,
		.nl_groups = RTNLGRP_LINK,
	};
	int err;
	struct socket **rtnl_sock = &bond_mngr.rtnl_sock;

	err = sock_create_kern(&init_net, AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE,
			       rtnl_sock);
	if (err) {
		pr_err("hinic3_bond: Couldn't create rtnl socket.\n");
		*rtnl_sock = NULL;
		return err;
	}

	(*rtnl_sock)->sk->sk_data_ready = bond_rtnl_data_ready;
	(*rtnl_sock)->sk->sk_allocation = GFP_KERNEL;

	err = kernel_bind(*rtnl_sock, (struct sockaddr *)(u8 *)&addr,
			  sizeof(addr));
	if (err) {
		pr_err("hinic3_bond: Couldn't bind rtnl socket.\n");
		sock_release(*rtnl_sock);
		*rtnl_sock = NULL;
	}

	return err;
}

static void bond_disable_netdev_event(void)
{
	if (bond_mngr.rtnl_sock)
		sock_release(bond_mngr.rtnl_sock);
}

static int bond_send_upcmd(struct hinic3_bond_dev *bdev, struct bond_attr *attr,
			   u8 cmd_type)
{
	int err, ret, len;
	struct hinic3_bond_cmd cmd = {0};
	u16 out_size = sizeof(cmd);

	cmd.sub_cmd = 0;
	cmd.ret_status = 0;

	if (attr) {
		memcpy(&cmd.attr, attr, sizeof(*attr));
	} else {
		cmd.attr.bond_id = bdev->bond_attr.bond_id;
		cmd.attr.slaves = bdev->bond_attr.slaves;
	}

	len = sizeof(cmd.bond_name);
	if (cmd_type == MPU_CMD_BOND_CREATE) {
		ret = strscpy(cmd.bond_name, bdev->name, len);
		if (ret < 0)
			pr_err("strscpy bond name failed\n");
		cmd.bond_name[sizeof(cmd.bond_name) - 1] = '\0';
	}

	err = hinic3_msg_to_mgmt_sync(bdev->ppf_hwdev, HINIC3_MOD_OVS, cmd_type,
				      &cmd, sizeof(cmd), &cmd, &out_size, 0,
				      HINIC3_CHANNEL_NIC);
	if (err != 0 || !out_size || cmd.ret_status != 0) {
		pr_err("hinic3_bond: uP cmd: %u failed, err: %d, sts: %u, out size: %u\n",
		       cmd_type, err, cmd.ret_status, out_size);
		err = -EIO;
	}

	return err;
}

static int bond_upcmd_deactivate(struct hinic3_bond_dev *bdev)
{
	int err;
	u16 id_tmp;

	if (bdev->status == BOND_DEV_STATUS_IDLE)
		return 0;

	pr_info("hinic3_bond: deactivate bond: %u\n", bdev->bond_attr.bond_id);

	err = bond_send_upcmd(bdev, NULL, MPU_CMD_BOND_DELETE);
	if (err == 0) {
		id_tmp = bdev->bond_attr.bond_id;
		memset(&bdev->bond_attr, 0, sizeof(bdev->bond_attr));
		bdev->status = BOND_DEV_STATUS_IDLE;
		bdev->bond_attr.bond_id = id_tmp;
		if (!bdev->tracker.cnt)
			bdev->ppf_hwdev = NULL;
	}

	return err;
}

static void bond_pf_bitmap_set(struct hinic3_bond_dev *bdev, u8 index)
{
	struct hinic3_nic_dev *nic_dev = NULL;
	u8 pf_id;

	nic_dev = netdev_priv(bdev->tracker.ndev[index]);
	if (!nic_dev)
		return;

	pf_id = hinic3_pf_id_of_vf(nic_dev->hwdev);
	BITMAP_SET(bdev->new_attr.bond_pf_bitmap, pf_id);
}

static void bond_update_slave_info(struct hinic3_bond_dev *bdev,
				   struct bond_attr *attr)
{
	struct net_device *ndev = NULL;
	u8 i;

	if (!netif_running(bdev->bond->dev))
		return;

	if (attr->bond_mode == BOND_MODE_ACTIVEBACKUP) {
		rcu_read_lock();
		ndev = bond_option_active_slave_get_rcu(bdev->bond);
		rcu_read_unlock();
	}

	for (i = 0; i < BOND_PORT_MAX_NUM; i++) {
		if (!BITMAP_JUDGE(attr->slaves, i)) {
			if (BITMAP_JUDGE(bdev->bond_attr.slaves, i))
				bond_dev_untrack_port(bdev, i);

			continue;
		}

		if (!bdev->tracker.ndev[i])
			continue;

		bond_pf_bitmap_set(bdev, i);

		if (!bdev->tracker.netdev_state[i].tx_enabled)
			continue;

		if (attr->bond_mode == BOND_MODE_8023AD) {
			BITMAP_SET(attr->active_slaves, i);
			BITMAP_SET(attr->lacp_collect_slaves, i);
		} else if (attr->bond_mode == BOND_MODE_XOR) {
			BITMAP_SET(attr->active_slaves, i);
		} else if (ndev && (ndev == bdev->tracker.ndev[i])) {
			/* BOND_MODE_ACTIVEBACKUP */
			BITMAP_SET(attr->active_slaves, i);
			break;
		}
	}
}

static int bond_upcmd_config(struct hinic3_bond_dev *bdev,
			     struct bond_attr *attr)
{
	int err;

	bond_update_slave_info(bdev, attr);
	attr->bond_pf_bitmap = bdev->new_attr.bond_pf_bitmap;

	if (memcmp(&bdev->bond_attr, attr, sizeof(struct bond_attr)) == 0)
		return 0;

	pr_info("hinic3_bond: Config bond: %u\n", attr->bond_id);
	pr_info("mode:%u, up_d:%u, down_d:%u, hash:%u, slaves:%u, ap:%u, cs:%u\n",
		attr->bond_mode,
		attr->up_delay,
		attr->down_delay,
		attr->xmit_hash_policy,
		attr->slaves,
		attr->active_slaves,
		attr->lacp_collect_slaves);
	pr_info("bond_pf_bitmap: 0x%x\n", attr->bond_pf_bitmap);
	pr_info("bond user_bitmap 0x%x\n", attr->user_bitmap);

	err = bond_send_upcmd(bdev, attr, MPU_CMD_BOND_SET_ATTR);
	if (!err)
		memcpy(&bdev->bond_attr, attr, sizeof(*attr));

	return err;
}

static int bond_upcmd_activate(struct hinic3_bond_dev *bdev,
			       struct bond_attr *attr)
{
	int err;

	if (bond_dev_is_activated(bdev))
		return 0;

	pr_info("hinic3_bond: active bond: %u\n", bdev->bond_attr.bond_id);

	err = bond_send_upcmd(bdev, attr, MPU_CMD_BOND_CREATE);
	if (err == 0) {
		bdev->status = BOND_DEV_STATUS_ACTIVATED;
		bdev->bond_attr.bond_mode = attr->bond_mode;
		err = bond_upcmd_config(bdev, attr);
	}

	return err;
}

static void bond_call_service_func(struct hinic3_bond_dev *bdev,
				   struct bond_attr *attr,
				   enum bond_service_proc_pos pos,
				   int bond_status)
{
	int i;

	if (bond_status)
		return;

	mutex_lock(&g_bond_service_func_mutex);
	for (i = 0; i < HINIC3_BOND_USER_NUM; i++) {
		if (g_bond_service_func[i])
			g_bond_service_func[i](bdev->name, (void *)attr, pos);
	}
	mutex_unlock(&g_bond_service_func_mutex);
}

static u32 bond_get_user_bitmap(struct hinic3_bond_dev *bdev)
{
	u32 user_bitmap = 0;
	u8 user;

	for (user = HINIC3_BOND_USER_OVS; user < HINIC3_BOND_USER_NUM; user++) {
		if (bdev->slot_used[user] == 1)
			BITMAP_SET(user_bitmap, user);
	}
	return user_bitmap;
}

static void bond_do_work(struct hinic3_bond_dev *bdev)
{
	bool is_bonded = 0;
	struct bond_attr attr;
	int err = 0;

	spin_lock(&bdev->lock);
	is_bonded = bdev->tracker.is_bonded;
	attr = bdev->new_attr;
	spin_unlock(&bdev->lock);
	attr.user_bitmap = bond_get_user_bitmap(bdev);

	/* is_bonded indicates whether bond should be activated. */
	if (is_bonded && !bond_dev_is_activated(bdev)) {
		bond_call_service_func(bdev, &attr, BOND_BEFORE_ACTIVE, 0);
		err = bond_upcmd_activate(bdev, &attr);
		bond_call_service_func(bdev, &attr, BOND_AFTER_ACTIVE, err);
	} else if (is_bonded && bond_dev_is_activated(bdev)) {
		bond_call_service_func(bdev, &attr, BOND_BEFORE_MODIFY, 0);
		err = bond_upcmd_config(bdev, &attr);
		bond_call_service_func(bdev, &attr, BOND_AFTER_MODIFY, err);
	} else if (!is_bonded && bond_dev_is_activated(bdev)) {
		bond_call_service_func(bdev, &attr, BOND_BEFORE_DEACTIVE, 0);
		err = bond_upcmd_deactivate(bdev);
		bond_call_service_func(bdev, &attr, BOND_AFTER_DEACTIVE, err);
	}

	if (err)
		pr_err("hinic3_bond: Do bond failed\n");
}

#define MIN_BOND_SLAVE_CNT 2
static void bond_try_do_work(struct work_struct *work)
{
	struct delayed_work *delayed_work = to_delayed_work(work);
	struct hinic3_bond_dev *bdev =
		container_of(delayed_work, struct hinic3_bond_dev, bond_work);
	int status;

	status = mutex_trylock(&g_bond_mutex);
	if (status == 0) {
		/* Delay 1 sec and retry */
		queue_delayed_work(bdev->wq, &bdev->bond_work, HZ);
	} else {
		bond_do_work(bdev);
		mutex_unlock(&g_bond_mutex);
	}
}

static int bond_dev_init(struct hinic3_bond_dev *bdev, const char *name)
{
	int err = 0;

	bdev->wq = create_singlethread_workqueue("hinic3_bond_wq");
	if (!bdev->wq) {
		pr_err("hinic3_bond: Failed to create workqueue\n");
		return -ENODEV;
	}

	INIT_DELAYED_WORK(&bdev->bond_work, bond_try_do_work);
	bdev->status = BOND_DEV_STATUS_IDLE;
	err = strscpy(bdev->name, name, strlen(name));
	if (err < 0) {
		pr_err("hinic3_bond: Failed to init bond dev\n");
		cancel_delayed_work_sync(&bdev->bond_work);
		destroy_workqueue(bdev->wq);
		return err;
	}

	spin_lock_init(&bdev->lock);

	return 0;
}

static int bond_dev_release(struct hinic3_bond_dev *bdev)
{
	int err;
	u8 i;
	u32 bond_cnt;

	err = bond_upcmd_deactivate(bdev);
	if (err) {
		pr_err("hinic3_bond: Failed to deactivate dev\n");
		mutex_unlock(&g_bond_mutex);
		return err;
	}

	for (i = BOND_FIRST_ID; i <= BOND_MAX_ID; i++) {
		if (bond_mngr.bond_dev[i] == bdev) {
			bond_mngr.bond_dev[i] = NULL;
			bond_mngr.cnt--;
			pr_info("hinic3_bond: Free bond, id: %u mngr_cnt:%u\n",
				i, bond_mngr.cnt);
			break;
		}
	}

	bond_cnt = bond_mngr.cnt;
	mutex_unlock(&g_bond_mutex);
	if (!bond_cnt)
		bond_disable_netdev_event();

	cancel_delayed_work_sync(&bdev->bond_work);
	destroy_workqueue(bdev->wq);
	kfree(bdev);

	return err;
}

static void bond_dev_free(struct kref *ref)
{
	struct hinic3_bond_dev *bdev = NULL;

	bdev = container_of(ref, struct hinic3_bond_dev, ref);
	bond_dev_release(bdev);
}

static struct hinic3_bond_dev *bond_dev_alloc(const char *name)
{
	struct hinic3_bond_dev *bdev = NULL;
	u16 i;
	int err;

	bdev = kzalloc(sizeof(*bdev), GFP_KERNEL);
	if (!bdev) {
		mutex_unlock(&g_bond_mutex);
		return NULL;
	}

	err = bond_dev_init(bdev, name);
	if (err) {
		kfree(bdev);
		mutex_unlock(&g_bond_mutex);
		return NULL;
	}

	if (!bond_mngr.cnt) {
		err = bond_enable_netdev_event();
		if (err) {
			bond_dev_release(bdev);
			return NULL;
		}
	}

	for (i = BOND_FIRST_ID; i <= BOND_MAX_ID; i++) {
		if (!bond_mngr.bond_dev[i]) {
			bdev->bond_attr.bond_id = i;
			bond_mngr.bond_dev[i] = bdev;
			bond_mngr.cnt++;
			pr_info("hinic3_bond: Create bond dev, id:%u cnt:%u\n",
				i, bond_mngr.cnt);
			break;
		}
	}

	if (i > BOND_MAX_ID) {
		bond_dev_release(bdev);
		bdev = NULL;
		pr_err("hinic3_bond: Failed to get free bond id\n");
	}

	return bdev;
}

static void update_bond_info(struct hinic3_bond_dev *bdev, struct bonding *bond)
{
	struct slave *slave = NULL;
	struct list_head *iter = NULL;
	struct net_device *ndev[BOND_PORT_MAX_NUM];
	int i = 0;

	bdev->bond = bond;

	rtnl_lock();
	bond_for_each_slave(bond, slave, iter) {
		if (bond_dev_track_port(bdev, slave->dev) == PORT_INVALID_ID)
			continue;
		ndev[i] = slave->dev;
		dev_hold(ndev[i++]);
		if (i >= BOND_PORT_MAX_NUM)
			break;
		(void)iter;
	}

	bond_for_each_slave(bond, slave, iter) {
		bond_handle_rtnl_event(slave->dev);
		(void)iter;
	}

	bond_handle_rtnl_event(bond->dev);

	rtnl_unlock();
	/* In case user queries info before bonding is complete */
	flush_delayed_work(&bdev->bond_work);

	rtnl_lock();
	while (i)
		dev_put(ndev[--i]);
	rtnl_unlock();
}

static struct hinic3_bond_dev *bond_dev_by_name(const char *name)
{
	struct hinic3_bond_dev *bdev = NULL;
	int i;

	for (i = BOND_FIRST_ID; i <= BOND_MAX_ID; i++) {
		if (bond_mngr.bond_dev[i] &&
		    (strcmp(bond_mngr.bond_dev[i]->name, name) == 0)) {
			bdev = bond_mngr.bond_dev[i];
			break;
		}
	}

	return bdev;
}

static void bond_dev_user_attach(struct hinic3_bond_dev *bdev,
				 enum hinic3_bond_user user)
{
	u32 user_bitmap;

	if (user < 0 || user >= HINIC3_BOND_USER_NUM)
		return;

	if (bdev->slot_used[user])
		return;

	bdev->slot_used[user] = 1;
	if (!kref_get_unless_zero(&bdev->ref)) {
		kref_init(&bdev->ref);
	} else {
		user_bitmap = bond_get_user_bitmap(bdev);
		pr_info("hinic3_bond: user %u attach bond %s, user_bitmap %#x\n",
			user, bdev->name, user_bitmap);
		queue_delayed_work(bdev->wq, &bdev->bond_work, 0);
	}
}

static void bond_dev_user_detach(struct hinic3_bond_dev *bdev,
				 enum hinic3_bond_user user, bool *freed)
{
	if (bdev->slot_used[user]) {
		bdev->slot_used[user] = 0;
		if (kref_read(&bdev->ref) == 1)
			*freed = true;
		kref_put(&bdev->ref, bond_dev_free);
	}
}

static struct bonding *bond_get_knl_bonding(const char *name)
{
	struct net_device *ndev_tmp = NULL;

	rcu_read_lock();
	for_each_netdev(&init_net, ndev_tmp) {
		if (netif_is_bond_master(ndev_tmp) &&
		    !strcmp(ndev_tmp->name, name)) {
			rcu_read_unlock();
			return netdev_priv(ndev_tmp);
			}
	}
	rcu_read_unlock();
	return NULL;
}

void hinic3_bond_set_user_bitmap(struct bond_attr *attr,
				 enum hinic3_bond_user user)
{
	if (!BITMAP_JUDGE(attr->user_bitmap, user))
		BITMAP_SET(attr->user_bitmap, user);
}
EXPORT_SYMBOL(hinic3_bond_set_user_bitmap);

int hinic3_bond_attach(const char *name, enum hinic3_bond_user user,
		       u16 *bond_id)
{
	struct hinic3_bond_dev *bdev = NULL;
	struct bonding *bond = NULL;
	bool new_dev = false;

	if (!name || !bond_id)
		return -EINVAL;

	bond = bond_get_knl_bonding(name);
	if (!bond) {
		pr_warn("hinic3_bond: Kernel bond %s not exist.\n", name);
		return -ENODEV;
	}

	mutex_lock(&g_bond_mutex);
	bdev = bond_dev_by_name(name);
	if (!bdev) {
		bdev = bond_dev_alloc(name);
		new_dev = true;
	} else {
		pr_info("hinic3_bond: %s already exist\n", name);
	}

	if (!bdev) {
		// lock has beed released in bond_dev_alloc
		return -ENODEV;
	}

	bond_dev_user_attach(bdev, user);
	mutex_unlock(&g_bond_mutex);

	if (new_dev)
		update_bond_info(bdev, bond);

	*bond_id = bdev->bond_attr.bond_id;
	return 0;
}
EXPORT_SYMBOL(hinic3_bond_attach);

int hinic3_bond_detach(u16 bond_id, enum hinic3_bond_user user)
{
	int err = 0;
	bool lock_freed = false;

	if (!BOND_ID_IS_VALID(bond_id) || user >= HINIC3_BOND_USER_NUM) {
		pr_warn("hinic3_bond: Invalid bond id or user, bond_id: %u, user: %d\n",
			bond_id, user);
		return -EINVAL;
	}

	mutex_lock(&g_bond_mutex);
	if (!bond_mngr.bond_dev[bond_id])
		err = -ENODEV;
	else
		bond_dev_user_detach(bond_mngr.bond_dev[bond_id],
				     user, &lock_freed);

	if (!lock_freed)
		mutex_unlock(&g_bond_mutex);
	return err;
}
EXPORT_SYMBOL(hinic3_bond_detach);

void hinic3_bond_clean_user(enum hinic3_bond_user user)
{
	int i = 0;
	bool lock_freed = false;

	mutex_lock(&g_bond_mutex);
	for (i = BOND_FIRST_ID; i <= BOND_MAX_ID; i++) {
		if (bond_mngr.bond_dev[i]) {
			bond_dev_user_detach(bond_mngr.bond_dev[i],
					     user, &lock_freed);
			if (lock_freed) {
				mutex_lock(&g_bond_mutex);
				lock_freed = false;
			}
		}
	}
	if (!lock_freed)
		mutex_unlock(&g_bond_mutex);
}
EXPORT_SYMBOL(hinic3_bond_clean_user);

int hinic3_bond_get_uplink_id(u16 bond_id, u32 *uplink_id)
{
	if (!BOND_ID_IS_VALID(bond_id) || !uplink_id) {
		pr_warn("hinic3_bond: Invalid args, id: %u, uplink: %d\n",
			bond_id, !!uplink_id);
		return -EINVAL;
	}

	mutex_lock(&g_bond_mutex);
	if (bond_mngr.bond_dev[bond_id])
		*uplink_id = bond_gen_uplink_id(bond_mngr.bond_dev[bond_id]);
	mutex_unlock(&g_bond_mutex);

	return 0;
}
EXPORT_SYMBOL(hinic3_bond_get_uplink_id);

int hinic3_bond_register_service_func(enum hinic3_bond_user user, void (*func)
				      (const char *bond_name, void *bond_attr,
				      enum bond_service_proc_pos pos))
{
	if (user >= HINIC3_BOND_USER_NUM)
		return -EINVAL;

	mutex_lock(&g_bond_service_func_mutex);
	g_bond_service_func[user] = func;
	mutex_unlock(&g_bond_service_func_mutex);

	return 0;
}
EXPORT_SYMBOL(hinic3_bond_register_service_func);

int hinic3_bond_unregister_service_func(enum hinic3_bond_user user)
{
	if (user >= HINIC3_BOND_USER_NUM)
		return -EINVAL;

	mutex_lock(&g_bond_service_func_mutex);
	g_bond_service_func[user] = NULL;
	mutex_unlock(&g_bond_service_func_mutex);

	return 0;
}
EXPORT_SYMBOL(hinic3_bond_unregister_service_func);

int hinic3_bond_get_slaves(u16 bond_id, struct hinic3_bond_info_s *info)
{
	struct bond_tracker *tracker = NULL;
	int size;
	int i;
	int len;

	if (!info || !BOND_ID_IS_VALID(bond_id)) {
		pr_warn("hinic3_bond: Invalid args, info: %d,id: %u\n",
			!!info, bond_id);
		return -EINVAL;
	}

	size = ARRAY_LEN(info->slaves_name);
	if (size < BOND_PORT_MAX_NUM) {
		pr_warn("hinic3_bond: Invalid args, size: %u\n",
			size);
		return -EINVAL;
	}

	mutex_lock(&g_bond_mutex);
	if (bond_mngr.bond_dev[bond_id]) {
		info->slaves = bond_mngr.bond_dev[bond_id]->bond_attr.slaves;
		tracker = &bond_mngr.bond_dev[bond_id]->tracker;
		info->cnt = 0;
		for (i = 0; i < BOND_PORT_MAX_NUM; i++) {
			if (BITMAP_JUDGE(info->slaves, i) && tracker->ndev[i]) {
				len = sizeof(info->slaves_name[0]);
				strscpy(info->slaves_name[info->cnt],
					tracker->ndev[i]->name, len);
				info->cnt++;
			}
		}
	}
	mutex_unlock(&g_bond_mutex);
	return 0;
}
EXPORT_SYMBOL(hinic3_bond_get_slaves);

struct net_device *hinic3_bond_get_netdev_by_portid(const char *bond_name,
						    u8 port_id)
{
	struct hinic3_bond_dev *bdev = NULL;

	if (port_id >= BOND_PORT_MAX_NUM)
		return NULL;
	mutex_lock(&g_bond_mutex);
	bdev = bond_dev_by_name(bond_name);
	if (!bdev) {
		mutex_unlock(&g_bond_mutex);
		return NULL;
	}
	mutex_unlock(&g_bond_mutex);
	return bdev->tracker.ndev[port_id];
}
EXPORT_SYMBOL(hinic3_bond_get_netdev_by_portid);

int hinic3_get_hw_bond_infos(void *hwdev, struct hinic3_hw_bond_infos *infos,
			     u16 channel)
{
	struct comm_cmd_hw_bond_infos bond_infos;
	u16 out_size = sizeof(bond_infos);
	int err;

	if (!hwdev || !infos)
		return -EINVAL;

	memset(&bond_infos, 0, sizeof(bond_infos));

	bond_infos.infos.bond_id = infos->bond_id;

	err = hinic3_msg_to_mgmt_sync(hwdev, HINIC3_MOD_COMM,
				      COMM_MGMT_CMD_GET_HW_BOND,
				      &bond_infos, sizeof(bond_infos),
				      &bond_infos, &out_size, 0, channel);
	if (bond_infos.head.status || err || !out_size) {
		sdk_err(((struct hinic3_hwdev *)hwdev)->dev_hdl,
			"Failed to get hw bond information, err: %d, status: 0x%x, out size: 0x%x, channel: 0x%x\n",
			err, bond_infos.head.status, out_size, channel);
		return -EIO;
	}

	memcpy(infos, &bond_infos.infos, sizeof(*infos));

	return 0;
}
EXPORT_SYMBOL(hinic3_get_hw_bond_infos);

int hinic3_get_bond_tracker_by_name(const char *name,
				    struct bond_tracker *tracker)
{
	struct hinic3_bond_dev *bdev = NULL;
	int i;

	mutex_lock(&g_bond_mutex);
	for (i = BOND_FIRST_ID; i <= BOND_MAX_ID; i++) {
		if (bond_mngr.bond_dev[i] &&
		    (strcmp(bond_mngr.bond_dev[i]->name, name) == 0)) {
			bdev = bond_mngr.bond_dev[i];
			spin_lock(&bdev->lock);
			*tracker = bdev->tracker;
			spin_unlock(&bdev->lock);
			mutex_unlock(&g_bond_mutex);
			return 0;
		}
	}
	mutex_unlock(&g_bond_mutex);
	return -ENODEV;
}
EXPORT_SYMBOL(hinic3_get_bond_tracker_by_name);
