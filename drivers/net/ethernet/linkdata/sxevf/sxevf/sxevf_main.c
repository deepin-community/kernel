#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#include "sxe_version.h"
#include "sxe_log.h"
#include "sxevf_netdev.h"
#include "sxevf.h"
#include "sxevf_pci.h"
#include "sxevf_ring.h"
#include "sxevf_irq.h"
#include "sxevf_msg.h"

#define SXEVF_MSG_LEVEL_DEFAULT (NETIF_MSG_DRV|NETIF_MSG_PROBE|NETIF_MSG_LINK)
#define SXEVF_WAIT_RST_DONE_TIMES		200

static struct workqueue_struct *sxevf_wq;
struct net_device *g_netdev;

void sxevf_start_adapter(struct sxevf_adapter *adapter)
{
	ether_addr_copy(adapter->mac_filter_ctxt.cur_uc_addr,
		        adapter->mac_filter_ctxt.def_uc_addr);
	clear_bit(SXEVF_HW_STOP, &adapter->hw.state);

	return;
}

s32 sxevf_dev_reset(struct sxevf_hw *hw)
{
	u32 retry = SXEVF_RST_CHECK_NUM;
	s32 ret;
	struct sxevf_rst_msg msg = {};
	struct sxevf_adapter *adapter = hw->adapter;

	set_bit(SXEVF_HW_STOP, &hw->state);

	hw->setup.ops->hw_stop(hw);

	adapter->mbx_version = SXEVF_MBX_API_10;
	hw->setup.ops->reset(hw);

	if (hw->board_type == SXE_BOARD_VF_HV) {
		retry = SXEVF_RST_CHECK_NUM_HV;
	}

	while (!sxevf_pf_rst_check(hw) && retry) {
		retry--;
		udelay(5);
	}
	if (!retry) {
		ret = -SXEVF_ERR_RESET_FAILED;
		LOG_ERROR_BDF("retry use up, pf has not reset done.(err:%d)\n", ret);
		goto l_out;
	}

	retry = SXEVF_WAIT_RST_DONE_TIMES;
	while (!hw->setup.ops->reset_done(hw) && retry) {
		retry--;
		msleep(50);
	}
	if (!retry) {
		ret = -SXEVF_ERR_RESET_FAILED;
		LOG_ERROR_BDF("retry use up, vflr has not reset done.(err:%d)\n", ret);
		goto l_out;
	}

	hw->mbx.retry = SXEVF_MBX_RETRY_COUNT;

	msg.msg_type = SXEVF_RESET;

	ret = sxevf_send_and_rcv_msg(hw, (u32 *)&msg,
				    SXEVF_MSG_NUM(sizeof(msg)));

	if (ret) {
		LOG_ERROR_BDF("vf reset msg:%d len:%zu mailbox fail.(err:%d)\n",
			  msg.msg_type, SXEVF_MSG_NUM(sizeof(msg)), ret);
		goto l_out;
	}

	sxevf_sw_mtu_set(adapter, msg.sw_mtu);

	if (msg.msg_type == (SXEVF_RESET | SXEVF_MSGTYPE_ACK)) {
		ether_addr_copy(adapter->mac_filter_ctxt.def_uc_addr,
				(u8 *)(msg.mac_addr));
	} else if (msg.msg_type != (SXEVF_RESET | SXEVF_MSGTYPE_NACK)) {
		ret = -SXEVF_ERR_MAC_ADDR_INVALID;
		LOG_ERROR_BDF("pf handle vf reset msg fail, rcv msg:0x%x.(err:%d)\n",
			 msg.msg_type, ret);
		goto l_out;
	}

	adapter->mac_filter_ctxt.mc_filter_type = msg.mc_fiter_type;

	LOG_INFO_BDF("vf get mc filter type:%d default mac addr:%pM from pf "
		     "sw_mtu:%u.\n",
		  adapter->mac_filter_ctxt.mc_filter_type,
		  adapter->mac_filter_ctxt.def_uc_addr,
		  msg.sw_mtu);

l_out:
	return ret;
}

STATIC int sxevf_config_dma_mask(struct sxevf_adapter * adapter)
{
	int ret = 0;

	if (dma_set_mask_and_coherent(&adapter->pdev->dev,
				DMA_BIT_MASK(SXEVF_DMA_BIT_WIDTH_64))) {
		LOG_ERROR_BDF("device[pci_id %u] 64 dma mask and coherent set failed\n",
					adapter->pdev->dev.id);
		ret = dma_set_mask_and_coherent(&adapter->pdev->dev,
					DMA_BIT_MASK(SXEVF_DMA_BIT_WIDTH_32));
		if (ret) {
			LOG_DEV_ERR("device[pci_id %u] 32 dma mask and coherent set failed\n",
					adapter->pdev->dev.id);
		}
	}

	return ret;
}

void sxevf_mbx_api_version_init(struct sxevf_adapter *adapter)
{
	s32 ret;
	struct sxevf_hw *hw = &adapter->hw;
	static const int api[] = {
		SXEVF_MBX_API_14,
		SXEVF_MBX_API_13,
		SXEVF_MBX_API_12,
		SXEVF_MBX_API_11,
		SXEVF_MBX_API_10,
		SXEVF_MBX_API_NR
	};
	u32 idx = 0;
	struct sxevf_mbx_api_msg msg;

	spin_lock_bh(&adapter->mbx_lock);

	while (api[idx] != SXEVF_MBX_API_NR) {
		msg.msg_type = SXEVF_API_NEGOTIATE;
		msg.api_version = api[idx];

		ret = sxevf_send_and_rcv_msg(hw, (u32 *)&msg, SXEVF_MSG_NUM(sizeof(msg)));
		if (!ret && (msg.msg_type == (SXEVF_API_NEGOTIATE | SXEVF_MSGTYPE_ACK))) {
			adapter->mbx_version = api[idx];
			break;
		} else {
			idx++;
		}
	}

	spin_unlock_bh(&adapter->mbx_lock);

	LOG_INFO_BDF("mbx version:%u.\n", adapter->mbx_version);
	return;
}

STATIC int sxevf_pci_init(struct sxevf_adapter * adapter)
{
	int ret;
	size_t len;
	resource_size_t bar_base_paddr;
	struct pci_dev *pdev = adapter->pdev;

	ret = pci_enable_device(pdev);
	if (ret) {
		LOG_ERROR_BDF("device[pci_id %u] pci enable failed\n", pdev->dev.id);
		goto l_pci_enable_device_mem_failed;
	}

	ret = pci_request_regions(pdev, SXEVF_DRV_NAME);
	if (ret) {
		LOG_DEV_ERR("device[pci_id %u] request IO memory failed\n", pdev->dev.id);
		goto l_pci_request_mem_failed;
	}

	pci_set_master(pdev);
	pci_save_state(pdev);

	bar_base_paddr = pci_resource_start(pdev, 0);
	len = pci_resource_len(pdev, 0);
	adapter->hw.reg_base_addr = ioremap(bar_base_paddr, len);
	if (!adapter->hw.reg_base_addr) {
		ret = -EIO;
		LOG_ERROR_BDF("device[pci_id %u] ioremap[bar_base_paddr = 0x%llx, len = %zu] failed\n",
					pdev->dev.id, (u64)bar_base_paddr, len);
		goto l_ioremap_failed;
	} else {
		pci_set_drvdata(pdev, adapter);
	}

	LOG_INFO_BDF("bar_base_paddr = 0x%llx, bar len = %zu, reg_base_addr = %p\n",
		(u64)bar_base_paddr, len, adapter->hw.reg_base_addr);
	return 0;

l_ioremap_failed:
	pci_release_regions(pdev);
l_pci_request_mem_failed:
	pci_disable_device(pdev);
l_pci_enable_device_mem_failed:
	return ret;
}

STATIC void sxevf_pci_exit(struct sxevf_adapter * adapter)
{
	if (adapter->hw.reg_base_addr) {
		iounmap(adapter->hw.reg_base_addr);
		adapter->hw.reg_base_addr = NULL;
	}

	if (pci_is_enabled(adapter->pdev)) {
		pci_release_regions(adapter->pdev);
		pci_disable_device(adapter->pdev);
		pci_set_drvdata(adapter->pdev, NULL);
	}

	return;
}

STATIC struct sxevf_adapter *sxevf_adapter_create(struct pci_dev *pdev)
{
	struct net_device *netdev;
	struct sxevf_adapter * adapter = NULL;

	netdev = alloc_etherdev_mq(sizeof(struct sxevf_adapter),
		SXEVF_TXRX_RING_NUM_MAX);
	if (!netdev) {
		LOG_ERROR("max:%d device[pci_id %u] sxe net device alloc failed\n",
			  SXEVF_TXRX_RING_NUM_MAX, pdev->dev.id);
		goto l_netdev_alloc_failed;
	}

	adapter = netdev_priv(netdev);
	adapter->pdev = pdev;
	adapter->netdev = netdev;
	adapter->msg_enable = netif_msg_init(-1,
					SXEVF_MSG_LEVEL_DEFAULT);

	LOG_INFO_BDF("adapter:0x%pK netdev:0x%pK pdev:0x%pK\n", adapter, netdev, pdev);

l_netdev_alloc_failed:
	return adapter;
}

static inline u32 sxevf_readl(const volatile void *reg)
{
	return readl(reg);
}

static inline void sxevf_writel(u32 value, volatile void *reg)
{
	writel(value, reg);
	return;
}

STATIC int sxevf_hw_base_init(struct sxevf_adapter *adapter)
{
	int ret;
	struct sxevf_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;

	hw->adapter = adapter;
	adapter->mbx_version = SXEVF_MBX_API_10;

	sxevf_hw_ops_init(hw);
	sxevf_hw_reg_handle_init(hw, sxevf_readl, sxevf_writel);

	sxevf_mbx_init(hw);
	spin_lock_init(&adapter->mbx_lock);

	ret = sxevf_dev_reset(hw);
	if (ret) {
		LOG_DEV_WARN("vf reset fail during probe.(err:%d)\n", ret);
	} else {
		sxevf_start_adapter(adapter);
		sxevf_mbx_api_version_init(adapter);

		ether_addr_copy(adapter->mac_filter_ctxt.cur_uc_addr, adapter->mac_filter_ctxt.def_uc_addr);
		if (is_zero_ether_addr(adapter->mac_filter_ctxt.cur_uc_addr)) {
			LOG_DEV_INFO("vf reset done, but pf don't assign valid"
				 " mac addr for vf.\n");
		}
#ifndef HAVE_ETH_HW_ADDR_SET_API
		ether_addr_copy(netdev->dev_addr, adapter->mac_filter_ctxt.cur_uc_addr);
#else
		eth_hw_addr_set(netdev, adapter->mac_filter_ctxt.cur_uc_addr);
#endif
	}

	if (!is_valid_ether_addr(netdev->dev_addr)) {
		eth_hw_addr_random(netdev);
		ether_addr_copy(adapter->mac_filter_ctxt.cur_uc_addr, netdev->dev_addr);
		ether_addr_copy(adapter->mac_filter_ctxt.def_uc_addr, netdev->dev_addr);

		LOG_DEV_INFO("vf use random mac addr:%pM.\n",
			 adapter->mac_filter_ctxt.def_uc_addr);
	}

	adapter->link.link_enable = true;

	return 0;
}

static void sxevf_sw_base_init1(struct sxevf_adapter *adapter)
{
	set_bit(SXEVF_DOWN, &adapter->state);

	sxevf_ring_feature_init(adapter);

	return;
}

s32 sxevf_ring_irq_init(struct sxevf_adapter *adapter)
{
	s32 ret;

	sxevf_ring_num_set(adapter);

	ret = sxevf_irq_ctxt_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("interrupt context init fail.(err:%d)\n", ret);
	}

	LOG_DEV_DEBUG("Multiqueue %s: Rx Queue count = %u, "
		"Tx Queue count = %u XDP Queue count %u\n",
	       (adapter->rx_ring_ctxt.num > 1) ? "Enabled" : "Disabled",
	       adapter->rx_ring_ctxt.num, adapter->tx_ring_ctxt.num,
	       adapter->xdp_ring_ctxt.num);

	return ret;
}

void sxevf_ring_irq_exit(struct sxevf_adapter *adapter)
{
	sxevf_irq_ctxt_exit(adapter);

	return;
}

void sxevf_save_reset_stats(struct sxevf_adapter *adapter)
{
	struct sxevf_hw_stats *stats = &adapter->stats.hw;

	if (stats->vfgprc || stats->vfgptc) {
		stats->saved_reset_vfgprc += stats->vfgprc - stats->base_vfgprc;
		stats->saved_reset_vfgptc += stats->vfgptc - stats->base_vfgptc;
		stats->saved_reset_vfgorc += stats->vfgorc - stats->base_vfgorc;
		stats->saved_reset_vfgotc += stats->vfgotc - stats->base_vfgotc;
		stats->saved_reset_vfmprc += stats->vfmprc - stats->base_vfmprc;
	}

	return;
}

void sxevf_last_counter_stats_init(struct sxevf_adapter *adapter)
{
	struct sxevf_hw_stats *stats = &adapter->stats.hw;
	struct sxevf_hw *hw = &adapter->hw;

	hw->stat.ops->stats_init_value_get(hw, stats);

	adapter->stats.hw.base_vfgprc = stats->last_vfgprc;
	adapter->stats.hw.base_vfgorc = stats->last_vfgorc;
	adapter->stats.hw.base_vfgptc = stats->last_vfgptc;
	adapter->stats.hw.base_vfgotc = stats->last_vfgotc;
	adapter->stats.hw.base_vfmprc = stats->last_vfmprc;

	return;
}

static void sxevf_sw_base_init2(struct sxevf_adapter *adapter)
{
	sxevf_monitor_init(adapter);

#ifdef SXE_IPSEC_CONFIGURE
	sxevf_ipsec_offload_init(adapter);
#endif

	sxevf_last_counter_stats_init(adapter);
	return;
}

STATIC int sxevf_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int ret;
	struct sxevf_adapter *adapter;
	const char *device_name = dev_name(&pdev->dev);

	adapter = sxevf_adapter_create(pdev);
	if (!adapter) {
		LOG_ERROR("adapter create failed.\n");
		ret = -ENOMEM;
		goto l_adapter_create_failed;
	}

	strlcpy(adapter->dev_name, device_name,
		min_t(u32, strlen(device_name) + 1, DEV_NAME_LEN));
	adapter->hw.board_type = id ? id->driver_data : SXE_BOARD_VF;

	ret = sxevf_pci_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("pci init failed.(ret:%d)\n", ret);
		goto l_pci_init_failed;
	}

	ret = sxevf_config_dma_mask(adapter);
	if (ret) {
		LOG_ERROR_BDF("config dma mask failed.(ret:%d)\n", ret);
		goto l_config_dma_mask_failed;
	}

	sxevf_netdev_init(adapter, pdev);

	ret = sxevf_hw_base_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("hardware base init failed.(ret:%d)\n", ret);
		goto l_config_dma_mask_failed;
	}

	sxevf_sw_base_init1(adapter);

	ret = sxevf_ring_irq_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("interrupt ring assign scheme init failed, err=%d\n", ret);
		goto l_config_dma_mask_failed;
	}

	sxevf_sw_base_init2(adapter);

	strcpy(adapter->netdev->name, "eth%d");
	ret = register_netdev(adapter->netdev);
	if (ret) {
		LOG_ERROR_BDF("register netdev failed.(ret:%d)\n", ret);
		goto l_irq_init_failed;
	}

	set_bit(SXEVF_DOWN, &adapter->state);
	netif_carrier_off(adapter->netdev);

	LOG_DEV_INFO("%pM\n", adapter->netdev->dev_addr);
	LOG_DEV_INFO("%s %s %s %s %s vf deviceId:0x%x mbx version:%u "
		 " probe done.\n",
		 dev_driver_string(pdev->dev.parent),
		 dev_name(pdev->dev.parent),
		 netdev_name(adapter->netdev),
		 dev_driver_string(&pdev->dev),
		 dev_name(&pdev->dev),
		 pdev->device,
		 adapter->mbx_version);

	return 0;

l_irq_init_failed:
	sxevf_ring_irq_exit(adapter);
l_config_dma_mask_failed:
	sxevf_pci_exit(adapter);
l_pci_init_failed:
	free_netdev(adapter->netdev);
l_adapter_create_failed:
	return ret;

}

static void sxevf_fuc_exit(struct sxevf_adapter *adapter)
{
	cancel_work_sync(&adapter->monitor_ctxt.work);

	return;
}

STATIC void sxevf_remove(struct pci_dev *pdev)
{
	struct sxevf_adapter * adapter = pci_get_drvdata(pdev);
	struct net_device *netdev;

	LOG_INFO_BDF("sxevf remove.\n");
	if (!adapter) {
		goto l_end;
	}

	set_bit(SXEVF_REMOVING, &adapter->state);
	netdev = adapter->netdev;

	sxevf_fuc_exit(adapter);

	if (NETREG_REGISTERED == netdev->reg_state) {
		unregister_netdev(netdev);
	}

#ifdef SXE_IPSEC_CONFIGURE
	sxevf_ipsec_offload_exit(adapter);
#endif

	sxevf_irq_ctxt_exit(adapter);

	sxevf_pci_exit(adapter);
	LOG_DEV_DEBUG("remove sxevf complete\n");

	free_netdev(netdev);

	LOG_INFO("%s %s %s %s deviceId:0x%x remove done.\n",
		 dev_driver_string(pdev->dev.parent),
		 dev_name(pdev->dev.parent),
		 dev_driver_string(&pdev->dev),
		 dev_name(&pdev->dev),
		 pdev->device);

l_end:
	return;
}

STATIC s32 sxevf_suspend(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct sxevf_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;
	s32 ret = 0;

	rtnl_lock();
	netif_device_detach(netdev);

	if (netif_running(netdev)) {
		sxevf_terminate(adapter);
	}

	sxevf_ring_irq_exit(adapter);
	rtnl_unlock();

#ifdef CONFIG_PM
	ret = pci_save_state(pdev);
	if (ret) {
		LOG_ERROR_BDF("save pci state fail.(err:%d)\n", ret);
		return ret;
	}
#endif

	if (!test_and_set_bit(SXEVF_DISABLED, &adapter->state)) {
		pci_disable_device(pdev);
	}

	return ret;
}

#ifdef CONFIG_PM
 STATIC s32 sxevf_resume(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct sxevf_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;
	s32 ret;

	pci_restore_state(pdev);
	pci_save_state(pdev);

	ret = pci_enable_device_mem(pdev);
	if (ret) {
		LOG_DEV_ERR("enable pci device from suspend fail.(err:%d)", ret);
		goto l_end;
	}

	smp_mb__before_atomic();
	clear_bit(SXEVF_DISABLED, &adapter->state);
	pci_set_master(pdev);
	sxevf_reset(adapter);

	rtnl_lock();
	sxevf_ring_num_set(adapter);
	ret = sxevf_irq_ctxt_init(adapter);
	if (!ret && netif_running(netdev)) {
		ret = sxevf_open(netdev);
	}
	rtnl_unlock();

	if (ret) {
		LOG_ERROR_BDF("pci device resume fail.(err:%d)\n", ret);
		goto l_end;
	}

	netif_device_attach(netdev);

l_end:
	return ret;
}
#endif

STATIC void sxevf_shutdown(struct pci_dev *pdev)
{
	sxevf_suspend(&pdev->dev);

	return;
}

STATIC void sxevf_io_resume(struct pci_dev *pdev)
{
	struct sxevf_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;

	LOG_DEBUG_BDF("oops,vf pci dev[%p] got io resume\n",pdev);

	rtnl_lock();
	if (netif_running(netdev)) {
		LOG_DEBUG_BDF("netdev running resume adapter.\n");
		sxevf_open(netdev);
	}

	netif_device_attach(netdev);
	rtnl_unlock();

	LOG_INFO_BDF("vf pci dev[%p] io resume done.\n",pdev);

	return;
}

STATIC pci_ers_result_t sxevf_io_slot_reset(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	pci_ers_result_t ret;

	LOG_INFO_BDF("oops, vf pci dev[%p] got io slot reset\n",pdev);

	if (pci_enable_device_mem(pdev)) {
		LOG_DEV_ERR("cannot re-enable PCI device after reset.\n");
		ret = PCI_ERS_RESULT_DISCONNECT;
		goto l_out;
	}

	smp_mb__before_atomic();
	clear_bit(SXEVF_DISABLED, &adapter->state);
	pci_set_master(pdev);

	sxevf_reset(adapter);
	ret = PCI_ERS_RESULT_RECOVERED;

l_out:
	LOG_INFO_BDF("vf pci dev[%p] io slot reset done. ret=0x%x\n",
		pdev, (u32)ret);
	return ret;
}

STATIC pci_ers_result_t sxevf_io_error_detected(struct pci_dev *pdev,
						  pci_channel_state_t state)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	pci_ers_result_t ret;

	LOG_DEBUG_BDF("oops,vf pci dev[%p] got io error detect, state=0x%x\n",
				pdev, (u32)state);

	if (!test_bit(SXEVF_MONITOR_WORK_INITED, &adapter->state)) {
		LOG_ERROR_BDF("vf monitor not inited\n");
		ret = PCI_ERS_RESULT_DISCONNECT;
		goto l_out;
	}

	rtnl_lock();
	netif_device_detach(netdev);

	if (netif_running(netdev)) {
		sxevf_terminate(adapter);
	}

	if (state == pci_channel_io_perm_failure) {
		rtnl_unlock();
		ret = PCI_ERS_RESULT_DISCONNECT;
		goto l_out;
	}

	if (!test_and_set_bit(SXEVF_DISABLED, &adapter->state)) {
		LOG_DEBUG_BDF("vf set disabled\n");
		pci_disable_device(pdev);
	}

	rtnl_unlock();

	ret = PCI_ERS_RESULT_NEED_RESET;

l_out:
	LOG_INFO_BDF("vf detected io error detected end, ret=0x%x.\n", ret);
	return ret;
}

static const struct pci_device_id sxevf_pci_tbl[] = {
	{PCI_VENDOR_ID_STARS, SXEVF_DEV_ID_ASIC, PCI_ANY_ID, PCI_ANY_ID, 0, 0, SXE_BOARD_VF},
	{PCI_VENDOR_ID_STARS, SXEVF_DEV_ID_ASIC_HV, PCI_ANY_ID, PCI_ANY_ID, 0, 0, SXE_BOARD_VF_HV},
	{0, }
};

static const struct pci_error_handlers sxevf_err_handler = {
	.error_detected = sxevf_io_error_detected,
	.slot_reset = sxevf_io_slot_reset,
	.resume = sxevf_io_resume,
};

static SIMPLE_DEV_PM_OPS(sxevf_pm_ops, sxevf_suspend, sxevf_resume);
static struct pci_driver sxevf_pci_driver = {
	.name         = SXEVF_DRV_NAME,
	.id_table     = sxevf_pci_tbl,
	.probe        = sxevf_probe,
	.remove       = sxevf_remove,
	.driver.pm    = &sxevf_pm_ops,
	 .shutdown    = sxevf_shutdown,
	 .err_handler = &sxevf_err_handler,
};

STATIC int __init sxevf_init(void)
{
	int ret;

	LOG_PRVF_INFO("version[%s], commit_id[%s],"
		"branch[%s], build_time[%s]\n",
		SXE_VERSION,
		SXE_COMMIT_ID,
		SXE_BRANCH,
		SXE_BUILD_TIME);

#ifndef SXE_DRIVER_RELEASE
	ret = sxe_log_init(true);
	if (ret < 0) {
		LOG_PRVF_ERR("sxe log init fail.(err:%d)\n", ret);
		goto l_end;
	}
#endif

	sxevf_wq = create_singlethread_workqueue(SXEVF_DRV_NAME);
	if (!sxevf_wq) {
		LOG_PRVF_ERR("failed to create workqueue\n");
		ret = -ENOMEM;
		goto l_log_exit;
	}

	ret = pci_register_driver(&sxevf_pci_driver);
	if (ret) {
		LOG_ERROR("%s driver register fail.(err:%d)\n",
			  sxevf_pci_driver.name, ret);
		goto l_pci_register_driver_failed;
	}

	LOG_INFO("pci driver:%s init done.\n", sxevf_pci_driver.name);

	return 0;

l_pci_register_driver_failed:
	destroy_workqueue(sxevf_wq);
	sxevf_wq = NULL;

l_log_exit:

#ifndef SXE_DRIVER_RELEASE
	sxe_log_exit();
l_end:
#endif
	return ret;
}

struct workqueue_struct *sxevf_wq_get(void)
{
	return sxevf_wq;
}

STATIC void __exit sxevf_exit(void)
{
	pci_unregister_driver(&sxevf_pci_driver);

	if (sxevf_wq) {
		destroy_workqueue(sxevf_wq);
		sxevf_wq = NULL;
	}

	LOG_INFO("pci driver:%s exit done.\n", sxevf_pci_driver.name);

#ifndef SXE_DRIVER_RELEASE
	sxe_log_exit();
#endif

	return;
}

MODULE_DEVICE_TABLE(pci, sxevf_pci_tbl);
MODULE_INFO(build_time, SXE_BUILD_TIME);
MODULE_INFO(branch, SXE_BRANCH);
MODULE_INFO(commit_id, SXE_COMMIT_ID);
MODULE_DESCRIPTION(SXEVF_DRV_DESCRIPTION);
MODULE_AUTHOR(SXEVF_DRV_AUTHOR);
MODULE_VERSION(SXE_VERSION);
MODULE_LICENSE(SXE_DRV_LICENSE);

module_init(sxevf_init);
module_exit(sxevf_exit);

