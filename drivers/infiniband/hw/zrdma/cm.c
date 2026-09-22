// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */
#include "main.h"
#include "trace.h"

/**
 * zxdh_copy_ip_htonl - copy IP address from host to network order
 * @dst: IP address in network order (big endian)
 * @src: IP address in host order
 */
void zxdh_copy_ip_htonl(__be32 *dst, u32 *src)
{
	*dst++ = htonl(*src++);
	*dst++ = htonl(*src++);
	*dst++ = htonl(*src++);
	*dst = htonl(*src);
}

/**
 * zxdh_netdev_vlan_ipv6 - Gets the netdev and mac
 * @addr: local IPv6 address
 * @vlan_id: vlan id for the given IPv6 address
 * @mac: mac address for the given IPv6 address
 *
 * Returns the net_device of the IPv6 address and also sets the
 * vlan id and mac for that address.
 */
struct net_device *zxdh_netdev_vlan_ipv6(u32 *addr, u16 *vlan_id, u8 *mac)
{
	struct net_device *ip_dev = NULL;
	struct in6_addr laddr6;

	if (!IS_ENABLED(CONFIG_IPV6))
		return NULL;

	zxdh_copy_ip_htonl(laddr6.in6_u.u6_addr32, addr);
	if (vlan_id)
		*vlan_id = 0xFFFF; /* Match rdma_vlan_dev_vlan_id() */
	if (mac)
		eth_zero_addr(mac);

	rcu_read_lock();
	for_each_netdev_rcu(&init_net, ip_dev) {
		if (ipv6_chk_addr(&init_net, &laddr6, ip_dev, 1)) {
			if (vlan_id)
				*vlan_id = rdma_vlan_dev_vlan_id(ip_dev);
			if (ip_dev->dev_addr && mac)
				ether_addr_copy(mac, ip_dev->dev_addr);
			break;
		}
	}
	rcu_read_unlock();

	return ip_dev;
}

/**
 * zxdh_get_vlan_ipv4 - Returns the vlan_id for IPv4 address
 * @addr: local IPv4 address
 */
u16 zxdh_get_vlan_ipv4(u32 *addr)
{
	struct net_device *netdev;
	u16 vlan_id = 0xFFFF;

	netdev = ip_dev_find(&init_net, htonl(addr[0]));
	if (netdev) {
		vlan_id = rdma_vlan_dev_vlan_id(netdev);
		dev_put(netdev);
	}

	return vlan_id;
}

/**
 * zxdh_ipv4_is_lpb - check if loopback
 * @loc_addr: local addr to compare
 * @rem_addr: remote address
 */
bool zxdh_ipv4_is_lpb(u32 loc_addr, u32 rem_addr)
{
	return ipv4_is_loopback(htonl(rem_addr)) || (loc_addr == rem_addr);
}

/**
 * zxdh_ipv6_is_lpb - check if loopback
 * @loc_addr: local addr to compare
 * @rem_addr: remote address
 */
bool zxdh_ipv6_is_lpb(u32 *loc_addr, u32 *rem_addr)
{
	struct in6_addr raddr6;

	zxdh_copy_ip_htonl(raddr6.in6_u.u6_addr32, rem_addr);

	return !memcmp(loc_addr, rem_addr, 16) || ipv6_addr_loopback(&raddr6);
}

/**
 * zxdh_aeq_qp_event - called by worker thread to disconnect qp
 * @iwqp: associate qp for the connection
 */
static void zxdh_aeq_qp_event(struct zxdh_qp *iwqp)
{
	struct zxdh_sc_qp *qp = &iwqp->sc_qp;
	unsigned long flags;
	struct ib_qp_attr attr;

	spin_lock_irqsave(&iwqp->lock, flags);

	if (iwqp->flush_issued || iwqp->sc_qp.qp_uk.destroy_pending) {
		spin_unlock_irqrestore(&iwqp->lock, flags);
		return;
	}
	spin_unlock_irqrestore(&iwqp->lock, flags);

	attr.qp_state = IB_QPS_ERR;
	zxdh_modify_qp_roce(&iwqp->ibqp, &attr, IB_QP_STATE, NULL);
	zxdh_ib_qp_event(iwqp, qp->event_type);
}

/**
 * zxdh_aeq_qp_worker - worker for aeq handle qp
 * @work: points or disconn structure
 */
static void zxdh_aeq_qp_worker(struct work_struct *work)
{
	struct aeq_qp_work *dwork =
		container_of(work, struct aeq_qp_work, work);
	struct zxdh_qp *iwqp = dwork->iwqp;

	kfree(dwork);
	zxdh_aeq_qp_event(iwqp);
	zxdh_qp_rem_ref(&iwqp->ibqp);
}

/**
 * zxdh_aeq_qp_disconn - when a connection is being closed
 * @iwqp: associated qp for the connection
 */
void zxdh_aeq_qp_disconn(struct zxdh_qp *iwqp)
{
	struct zxdh_device *iwdev = iwqp->iwdev;
	struct aeq_qp_work *work;
	unsigned long flags;

	work = kzalloc(sizeof(*work), GFP_ATOMIC);
	if (!work) {
		pr_err("[zxdh_rdma] kzalloc work failed!\n");
		return;
	}
	spin_lock_irqsave(&iwdev->rf->qptable_lock, flags);
	if (!iwdev->rf->qp_table[iwqp->sc_qp.qp_ctx_num -
				 iwdev->rf->sc_dev.base_qpn]) {
		spin_unlock_irqrestore(&iwdev->rf->qptable_lock, flags);
		zxdh_dbg(iwdev_to_idev(iwdev),
			 "CM: qp_id %d is already freed\n", iwqp->sc_qp.qp_ctx_num);
		kfree(work);
		return;
	}
	zxdh_qp_add_ref(&iwqp->ibqp);
	spin_unlock_irqrestore(&iwdev->rf->qptable_lock, flags);

	work->iwqp = iwqp;
	INIT_WORK(&work->work, zxdh_aeq_qp_worker);
	queue_work(iwdev->cleanup_wq, &work->work);
}

/**
 * zxdh_aeq_entry_err_worker - worker for aeq 8f5 handle qpc
 * @work: work task structure
 */
static void zxdh_aeq_entry_err_worker(struct work_struct *work)
{
	struct aeq_qp_work *dwork =
		container_of(work, struct aeq_qp_work, work);
	struct zxdh_qp *iwqp = dwork->iwqp;
	struct zxdh_sc_qp *qp = &iwqp->sc_qp;
	struct zxdh_dma_mem qpc_buf = {};
	u64 temp;
	u32 tx_last_ack_psn;
	int ret;

	qpc_buf.size = ALIGN(ZXDH_QP_CTX_SIZE, ZXDH_QPC_ALIGNMENT);
	qpc_buf.va = dma_alloc_coherent(iwqp->iwdev->rf->sc_dev.hw->device,
					qpc_buf.size, &qpc_buf.pa, GFP_KERNEL);

	kfree(dwork);
	if (!qpc_buf.va) {
		pr_err("[zxdh_rdma] no memory\n");
		zxdh_qp_rem_ref(&iwqp->ibqp);
		return;
	}
	ret = zxdh_query_qpc(qp, &qpc_buf);
	if (ret)
		goto free_rsrc;
	get_64bit_val((__le64 *)qpc_buf.va, 0, &temp);
	tx_last_ack_psn = FIELD_GET(RDMAQPC_TX_LAST_ACK_PSN, temp);
	if (tx_last_ack_psn != qp->aeq_entry_err_last_psn) {
		// qp restart success
		qp->entry_err_cnt = 0;
	}
	qp->aeq_entry_err_last_psn = tx_last_ack_psn;

	if (qp->entry_err_cnt >= ZXDH_AEQ_RETRY_LIMIT) {
		// AEQ reported. counts out of limit.
		qp->event_type = ZXDH_QP_EVENT_CATASTROPHIC;
		zxdh_aeq_qp_event(iwqp);
	} else {
		// AEQ not reported
		pr_info("[zxdh_rdma] 8f5 entry_err_cnt: %d\n", qp->entry_err_cnt);
		qp->entry_err_cnt++;
	}

free_rsrc:
	dma_free_coherent(iwqp->iwdev->rf->sc_dev.hw->device, qpc_buf.size,
			  qpc_buf.va, qpc_buf.pa);
	zxdh_qp_rem_ref(&iwqp->ibqp);
}

/**
 * zxdh_aeq_process_entry_err - query qpc when aeq 8f5 is triggered
 * @iwqp: associated qp for the connection
 */
void zxdh_aeq_process_entry_err(struct zxdh_qp *iwqp)
{
	struct aeq_qp_work *work;
	struct zxdh_device *iwdev = iwqp->iwdev;
	unsigned long flags;

	work = kzalloc(sizeof(*work), GFP_ATOMIC);
	if (!work) {
		pr_err("[zxdh_rdma] kzalloc work failed!\n");
		return;
	}

	spin_lock_irqsave(&iwdev->rf->qptable_lock, flags);

	if (!iwdev->rf->qp_table[iwqp->sc_qp.qp_ctx_num -
				 iwdev->rf->sc_dev.base_qpn]) {
		spin_unlock_irqrestore(&iwdev->rf->qptable_lock, flags);
		zxdh_dbg(iwdev_to_idev(iwdev),
			 "CM: qp_id %d is already freed\n", iwqp->sc_qp.qp_ctx_num);
		kfree(work);
		return;
	}
	zxdh_qp_add_ref(&iwqp->ibqp);
	spin_unlock_irqrestore(&iwdev->rf->qptable_lock, flags);

	work->iwqp = iwqp;
	INIT_WORK(&work->work, zxdh_aeq_entry_err_worker);
	queue_work(iwdev->cleanup_wq, &work->work);
}

/**
 * zxdh_aeq_retry_err_worker - worker for aeq 8f3 handle qpc
 * @work: work task structure
 */
static void zxdh_aeq_retry_err_worker(struct work_struct *work)
{
	struct aeq_qp_work *dwork =
		container_of(work, struct aeq_qp_work, work);
	struct zxdh_qp *iwqp = dwork->iwqp;
	struct zxdh_sc_qp *qp = &iwqp->sc_qp;
	struct zxdh_dma_mem qpc_buf = {};
	u64 temp;
	u32 ack_err_flag, tx_last_ack_psn, retry_cqe_sq_opcode, recv_err_flag;
	int ret;

	qpc_buf.size = ALIGN(ZXDH_QP_CTX_SIZE, ZXDH_QPC_ALIGNMENT);
	qpc_buf.va = dma_alloc_coherent(iwqp->iwdev->rf->sc_dev.hw->device,
					qpc_buf.size, &qpc_buf.pa, GFP_KERNEL);

	kfree(dwork);
	if (!qpc_buf.va) {
		pr_err("[zxdh_rdma] no memory\n");
		zxdh_qp_rem_ref(&iwqp->ibqp);
		return;
	}
	ret = zxdh_query_qpc(qp, &qpc_buf);
	if (ret)
		goto free_rsrc;
	get_64bit_val((__le64 *)qpc_buf.va, 0, &temp);
	tx_last_ack_psn = FIELD_GET(RDMAQPC_TX_LAST_ACK_PSN, temp);
	get_64bit_val((__le64 *)qpc_buf.va, 56, &temp);
	retry_cqe_sq_opcode =
		FIELD_GET(RDMAQPC_TX_RETRY_CQE_SQ_OPCODE_FLAG, temp);
	get_64bit_val((__le64 *)qpc_buf.va, 48, &temp);
	recv_err_flag = FIELD_GET(RDMAQPC_TX_RECV_ERR_FLAG, temp);
	get_64bit_val((__le64 *)qpc_buf.va, 40, &temp);
	ack_err_flag = FIELD_GET(BIT_ULL(48), temp);

	if (ack_err_flag != 1) {
		pr_info("[zxdh_rdma] qp %d has been restarted!\n", qp->qp_uk.qp_id);
		goto free_rsrc;
	}

	if (!((retry_cqe_sq_opcode >= 32) &&
	      (recv_err_flag == 1 || recv_err_flag == 2))) {
		pr_debug("[zxdh_rdma] Timeout! 800f3 aeq reported!\n");
		qp->event_type = ZXDH_QP_EVENT_CATASTROPHIC;
		zxdh_aeq_qp_event(iwqp);
		goto free_rsrc;
	}

	if (tx_last_ack_psn != qp->aeq_retry_err_last_psn) {
		// qp restart success
		pr_info("[zxdh_rdma] retry_err_cnt reset\n");
		qp->retry_err_cnt = 0;
	}
	qp->aeq_retry_err_last_psn = tx_last_ack_psn;

	if (qp->retry_err_cnt >= ZXDH_AEQ_RETRY_LIMIT) {
		// AEQ reported. counts out of limit.
		qp->event_type = ZXDH_QP_EVENT_CATASTROPHIC;
		zxdh_aeq_qp_event(iwqp);
	} else {
		// AEQ not reported
		pr_info("[zxdh_rdma] 8f3 retry_err_cnt: %d\n", qp->retry_err_cnt);
		qp->retry_err_cnt++;
	}
free_rsrc:
	dma_free_coherent(iwqp->iwdev->rf->sc_dev.hw->device, qpc_buf.size,
			  qpc_buf.va, qpc_buf.pa);
	zxdh_qp_rem_ref(&iwqp->ibqp);
}

/**
 * zxdh_aeq_process_retry_err - query qpc when aeq 8f3 is triggered
 * @iwqp: associated qp for the connection
 */
void zxdh_aeq_process_retry_err(struct zxdh_qp *iwqp)
{
	struct aeq_qp_work *work;
	struct zxdh_device *iwdev = iwqp->iwdev;
	unsigned long flags;

	work = kzalloc(sizeof(*work), GFP_ATOMIC);
	if (!work) {
		pr_err("[zxdh_rdma] kzalloc work failed!\n");
		return;
	}

	spin_lock_irqsave(&iwdev->rf->qptable_lock, flags);

	if (!iwdev->rf->qp_table[iwqp->sc_qp.qp_ctx_num -
				 iwdev->rf->sc_dev.base_qpn]) {
		spin_unlock_irqrestore(&iwdev->rf->qptable_lock, flags);
		zxdh_dbg(iwdev_to_idev(iwdev),
			 "CM: qp_id %d is already freed\n", iwqp->sc_qp.qp_ctx_num);
		kfree(work);
		return;
	}
	zxdh_qp_add_ref(&iwqp->ibqp);
	spin_unlock_irqrestore(&iwdev->rf->qptable_lock, flags);

	work->iwqp = iwqp;
	INIT_WORK(&work->work, zxdh_aeq_retry_err_worker);
	queue_work(iwdev->cleanup_wq, &work->work);
}
