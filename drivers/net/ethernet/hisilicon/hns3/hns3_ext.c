// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023 Hisilicon Limited.

#include "hns3_ext.h"

int nic_netdev_match_check(struct net_device *ndev)
{
#define HNS3_DRIVER_NAME_LEN 5

	struct ethtool_drvinfo drv_info;
	struct hnae3_handle *h;

	if (!ndev || !ndev->ethtool_ops ||
	    !ndev->ethtool_ops->get_drvinfo)
		return -EINVAL;

	ndev->ethtool_ops->get_drvinfo(ndev, &drv_info);

	if (strncmp(drv_info.driver, "hns3", HNS3_DRIVER_NAME_LEN))
		return -EINVAL;

	h = hns3_get_handle(ndev);
	if (h->flags & HNAE3_SUPPORT_VF)
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL(nic_netdev_match_check);

static int nic_invoke_pri_ops(struct net_device *ndev, int opcode,
			      void *data, size_t length)

{
	struct hnae3_handle *h;
	int ret;

	if ((!data && length) || (data && !length)) {
		netdev_err(ndev, "failed to check data and length");
		return -EINVAL;
	}

	if (nic_netdev_match_check(ndev))
		return -ENODEV;

	h = hns3_get_handle(ndev);
	if (!h->ae_algo->ops->priv_ops)
		return -EOPNOTSUPP;

	ret = h->ae_algo->ops->priv_ops(h, opcode, data, length);
	if (ret)
		netdev_err(ndev,
			   "failed to invoke pri ops, opcode = %#x, ret = %d\n",
			   opcode, ret);

	return ret;
}

void nic_chip_recover_handler(struct net_device *ndev,
			      enum hnae3_event_type_custom event_t)
{
	dev_info(&ndev->dev, "reset type is %d!!\n", event_t);

	if (event_t == HNAE3_PPU_POISON_CUSTOM)
		event_t = HNAE3_FUNC_RESET_CUSTOM;

	if (event_t != HNAE3_FUNC_RESET_CUSTOM &&
	    event_t != HNAE3_GLOBAL_RESET_CUSTOM &&
	    event_t != HNAE3_IMP_RESET_CUSTOM) {
		dev_err(&ndev->dev, "reset type err!!\n");
		return;
	}

	nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_RESET, &event_t, sizeof(event_t));
}
EXPORT_SYMBOL(nic_chip_recover_handler);

static int nic_check_pfc_storm_para(u32 dir, u32 enable, u32 period_ms,
				    u32 times, u32 recovery_period_ms)
{
	if ((dir != HNS3_PFC_STORM_PARA_DIR_RX &&
	     dir != HNS3_PFC_STORM_PARA_DIR_TX) ||
	     (enable != HNS3_PFC_STORM_PARA_DISABLE &&
	      enable != HNS3_PFC_STORM_PARA_ENABLE))
		return -EINVAL;

	if (period_ms < HNS3_PFC_STORM_PARA_PERIOD_MIN ||
	    period_ms > HNS3_PFC_STORM_PARA_PERIOD_MAX ||
	    recovery_period_ms < HNS3_PFC_STORM_PARA_PERIOD_MIN ||
	    recovery_period_ms > HNS3_PFC_STORM_PARA_PERIOD_MAX ||
	    times <= 0)
		return -EINVAL;

	return 0;
}

int nic_set_pfc_storm_para(struct net_device *ndev, u32 dir, u32 enable,
			   u32 period_ms, u32 times, u32 recovery_period_ms)
{
	struct hnae3_pfc_storm_para para;

	if (nic_check_pfc_storm_para(dir, enable, period_ms, times,
				     recovery_period_ms)) {
		dev_err(&ndev->dev,
			"set pfc storm para failed because invalid input param.\n");
		return -EINVAL;
	}

	para.dir = dir;
	para.enable = enable;
	para.period_ms = period_ms;
	para.times = times;
	para.recovery_period_ms = recovery_period_ms;

	return nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_SET_PFC_STORM_PARA,
				  &para, sizeof(para));
}
EXPORT_SYMBOL(nic_set_pfc_storm_para);

int nic_get_pfc_storm_para(struct net_device *ndev, u32 dir, u32 *enable,
			   u32 *period_ms, u32 *times, u32 *recovery_period_ms)
{
	struct hnae3_pfc_storm_para para;
	int ret;

	if (!enable || !period_ms || !times || !recovery_period_ms ||
	    (dir != HNS3_PFC_STORM_PARA_DIR_RX &&
	     dir != HNS3_PFC_STORM_PARA_DIR_TX)) {
		dev_err(&ndev->dev,
			"get pfc storm para failed because invalid input param.\n");
		return -EINVAL;
	}

	para.dir = dir;
	ret = nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_GET_PFC_STORM_PARA,
				 &para, sizeof(para));
	if (ret)
		return ret;

	*enable = para.enable;
	*period_ms = para.period_ms;
	*times = para.times;
	*recovery_period_ms = para.recovery_period_ms;
	return 0;
}
EXPORT_SYMBOL(nic_get_pfc_storm_para);

int nic_set_notify_pkt_param(struct net_device *ndev,
			     struct hnae3_notify_pkt_param *param)
{
	return nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_SET_NOTIFY_PARAM,
				  param, sizeof(*param));
}
EXPORT_SYMBOL(nic_set_notify_pkt_param);

int nic_set_notify_pkt_start(struct net_device *ndev)
{
	return nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_SET_NOTIFY_START, NULL, 0);
}
EXPORT_SYMBOL(nic_set_notify_pkt_start);

int nic_set_torus_param(struct net_device *ndev, struct hnae3_torus_param *param)
{
	if (!param || (param->enable != 0 && param->enable != 1))
		return -EINVAL;

	return nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_SET_TORUS_PARAM,
				  param, sizeof(*param));
}
EXPORT_SYMBOL(nic_set_torus_param);

int nic_get_torus_param(struct net_device *ndev, struct hnae3_torus_param *param)
{
	return nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_GET_TORUS_PARAM,
				  param, sizeof(*param));
}
EXPORT_SYMBOL(nic_get_torus_param);

int nic_clean_stats64(struct net_device *ndev, struct rtnl_link_stats64 *stats)
{
	struct hnae3_knic_private_info *kinfo;
	struct hns3_enet_ring *ring;
	struct hns3_nic_priv *priv;
	struct hnae3_handle *h;
	int i, ret;

	priv = netdev_priv(ndev);
	h = hns3_get_handle(ndev);
	kinfo = &h->kinfo;

	rtnl_lock();
	if (!test_bit(HNS3_NIC_STATE_INITED, &priv->state) ||
	    test_bit(HNS3_NIC_STATE_RESETTING, &priv->state)) {
		ret = -EBUSY;
		goto end_unlock;
	}

	ret = nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_CLEAN_STATS64,
				 NULL, 0);
	if (ret)
		goto end_unlock;

	for (i = 0; i < kinfo->num_tqps; i++) {
		ring = &priv->ring[i];
		memset(&ring->stats, 0, sizeof(struct ring_stats));
		ring = &priv->ring[i + kinfo->num_tqps];
		memset(&ring->stats, 0, sizeof(struct ring_stats));
	}

	memset(&ndev->stats, 0, sizeof(struct net_device_stats));
	netdev_info(ndev, "clean stats succ\n");

end_unlock:
	rtnl_unlock();
	return ret;
}
EXPORT_SYMBOL(nic_clean_stats64);

int nic_set_cpu_affinity(struct net_device *ndev, cpumask_t *affinity_mask)
{
	struct hns3_enet_tqp_vector *tqp_vector;
	struct hns3_nic_priv *priv;
	int ret = 0;
	u16 i;

	if (!ndev || !affinity_mask) {
		netdev_err(ndev,
			   "Invalid input param when set ethernet cpu affinity\n");
		return -EINVAL;
	}

	if (nic_netdev_match_check(ndev))
		return -ENODEV;

	priv = netdev_priv(ndev);
	rtnl_lock();
	if (!test_bit(HNS3_NIC_STATE_INITED, &priv->state) ||
	    test_bit(HNS3_NIC_STATE_RESETTING, &priv->state)) {
		ret = -EBUSY;
		goto err_unlock;
	}

	if (test_bit(HNS3_NIC_STATE_DOWN, &priv->state)) {
		netdev_err(ndev,
			   "ethernet is down, not support cpu affinity set\n");
		ret = -ENETDOWN;
		goto err_unlock;
	}

	for (i = 0; i < priv->vector_num; i++) {
		tqp_vector = &priv->tqp_vector[i];
		if (tqp_vector->irq_init_flag != HNS3_VECTOR_INITED)
			continue;

		tqp_vector->affinity_mask = *affinity_mask;

		ret = irq_set_affinity_hint(tqp_vector->vector_irq, NULL);
		if (ret) {
			netdev_err(ndev,
				   "failed to reset affinity hint, ret = %d\n", ret);
			goto err_unlock;
		}

		ret = irq_set_affinity_hint(tqp_vector->vector_irq,
					    &tqp_vector->affinity_mask);
		if (ret) {
			netdev_err(ndev,
				   "failed to set affinity hint, ret = %d\n", ret);
			goto err_unlock;
		}
	}

	netdev_info(ndev, "set nic cpu affinity %*pb succeed\n",
		    cpumask_pr_args(affinity_mask));

err_unlock:
	rtnl_unlock();
	return ret;
}
EXPORT_SYMBOL(nic_set_cpu_affinity);

static int nic_get_ext_id_info(struct net_device *ndev,
			       struct hane3_port_ext_id_info *id_info)
{
	return nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_GET_PORT_EXT_ID_INFO,
				  id_info, sizeof(*id_info));
}

int nic_get_chipid(struct net_device *ndev, u32 *chip_id)
{
	struct hane3_port_ext_id_info info;
	int ret;

	if (!chip_id)
		return -EINVAL;

	ret = nic_get_ext_id_info(ndev, &info);
	if (ret)
		return ret;

	*chip_id = info.chip_id;
	return 0;
}
EXPORT_SYMBOL(nic_get_chipid);

int nic_get_mac_id(struct net_device *ndev, u32 *mac_id)
{
	struct hane3_port_ext_id_info info;
	int ret;

	if (!mac_id)
		return -EINVAL;

	ret = nic_get_ext_id_info(ndev, &info);
	if (ret)
		return ret;

	*mac_id = info.mac_id;
	return 0;
}
EXPORT_SYMBOL(nic_get_mac_id);

int nic_get_io_die_id(struct net_device *ndev, u32 *io_die_id)
{
	struct hane3_port_ext_id_info info;
	int ret;

	if (!io_die_id)
		return -EINVAL;

	ret = nic_get_ext_id_info(ndev, &info);
	if (ret)
		return ret;

	*io_die_id = info.io_die_id;
	return 0;
}
EXPORT_SYMBOL(nic_get_io_die_id);

static int nic_get_ext_num_info(struct net_device *ndev,
				struct hane3_port_ext_num_info *num_info)
{
	return nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_GET_PORT_EXT_NUM_INFO,
				  num_info, sizeof(*num_info));
}

int nic_get_chip_num(struct net_device *ndev, u32 *chip_num)
{
	struct hane3_port_ext_num_info info;
	int ret;

	if (!chip_num)
		return -EINVAL;

	ret = nic_get_ext_num_info(ndev, &info);
	if (ret)
		return ret;

	*chip_num = info.chip_num;
	return 0;
}
EXPORT_SYMBOL(nic_get_chip_num);

int nic_get_io_die_num(struct net_device *ndev, u32 *io_die_num)
{
	struct hane3_port_ext_num_info info;
	int ret;

	if (!io_die_num)
		return -EINVAL;

	ret = nic_get_ext_num_info(ndev, &info);
	if (ret)
		return ret;

	*io_die_num = info.io_die_num;
	return 0;
}
EXPORT_SYMBOL(nic_get_io_die_num);

int nic_get_port_num_of_die(struct net_device *ndev, u32 *port_num)
{
	return nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_GET_PORT_NUM,
				  port_num, sizeof(*port_num));
}
EXPORT_SYMBOL(nic_get_port_num_of_die);

int nic_get_port_num_per_chip(struct net_device *ndev, u32 *port_num)
{
	return nic_get_port_num_of_die(ndev, port_num);
}
EXPORT_SYMBOL(nic_get_port_num_per_chip);

int nic_set_tx_timeout(struct net_device *ndev, int tx_timeout)
{
	if (nic_netdev_match_check(ndev))
		return -ENODEV;

	if (tx_timeout <= 0 || tx_timeout > HNS3_MAX_TX_TIMEOUT)
		return -EINVAL;

	ndev->watchdog_timeo = tx_timeout * HZ;

	return 0;
}
EXPORT_SYMBOL(nic_set_tx_timeout);
