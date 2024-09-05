#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <net/rtnetlink.h>
#include <linux/moduleparam.h>
#include <linux/aer.h>

#ifdef SXE_TPH_CONFIGURE
#include <linux/dca.h>
#endif

#include "sxe_version.h"
#include "sxe_pci.h"
#include "sxe_ring.h"
#include "sxe.h"
#include "sxe_log.h"
#include "sxe_netdev.h"
#include "sxe_hw.h"
#include "sxe_irq.h"
#include "sxe_monitor.h"
#include "sxe_filter.h"
#include "sxe_debug.h"
#include "sxe_dcb.h"
#include "sxe_sriov.h"
#include "sxe_debugfs.h"
#include "sxe_phy.h"
#include "sxe_host_cli.h"
#include "sxe_host_hdc.h"
#include "sxe_ethtool.h"

#ifdef SXE_DRIVER_TRACE
#include "sxe_trace.h"
#endif

#define SXE_MSG_LEVEL_DEFAULT (NETIF_MSG_DRV|NETIF_MSG_PROBE|NETIF_MSG_LINK)

#define SXE_DW1_OFFSET              (4)
#define SXE_DW2_OFFSET              (8)
#define SXE_DW3_OFFSET              (12)
#define SXE_REQ_ID_SHIFT            (16)
#define SXE_REQ_ID_VF_MASK          (0x0080)
#define SXE_REQ_ID_PF_MASK          (0x0001)
#define SXE_REQ_ID_VF_ID_MASK       (0x007F)

static struct workqueue_struct *sxe_workqueue;

struct workqueue_struct *sxe_fnav_workqueue;
#define SXE_FNAV_NAME                   "sxe_fnav"

struct kmem_cache *fnav_cache;

static bool allow_inval_mac;
module_param(allow_inval_mac, bool, false);
MODULE_PARM_DESC(allow_inval_mac,
    "Indicates device can be probed successfully or not when mac addr invalid.");

bool sxe_allow_inval_mac(void)
{
	return !!allow_inval_mac;
}

#ifndef SXE_DRIVER_RELEASE

static int irq_mode;
module_param(irq_mode, int, 0);
MODULE_PARM_DESC(irq_mode, "select irq mode(0:MSIx(default), 1:MSI, 2:INTx)");

bool sxe_is_irq_msi_mode(void)
{
	return (irq_mode == SXE_IRQ_MSI_MODE) ? true : false;
}

bool sxe_is_irq_intx_mode(void)
{
	return (irq_mode == SXE_IRQ_INTX_MODE) ? true : false;
}

#else

bool sxe_is_irq_msi_mode(void)
{
	return false;
}

bool sxe_is_irq_intx_mode(void)
{
	return  false;
}

#endif

#ifdef SXE_TPH_CONFIGURE
static void sxe_tx_tph_update(struct sxe_adapter *adapter,
				struct sxe_ring *tx_ring,
				int cpu)
{
	u8 tag = 0;
	struct sxe_hw *hw = &adapter->hw;

	if (adapter->cap & SXE_TPH_ENABLE) {
		tag = dca3_get_tag(tx_ring->dev, cpu);
	}

	hw->dma.ops->tx_tph_update(hw, tx_ring->reg_idx, tag);

	return;
}

static void sxe_rx_tph_update(struct sxe_adapter *adapter,
				struct sxe_ring *rx_ring,
				int cpu)
{
	u8 tag = 0;
	struct sxe_hw *hw = &adapter->hw;

	if (adapter->cap & SXE_TPH_ENABLE) {
		tag = dca3_get_tag(rx_ring->dev, cpu);
	}

	hw->dma.ops->rx_tph_update(hw, rx_ring->reg_idx, tag);

	return;
}

void sxe_tph_update(struct sxe_irq_data *irq_data)
{
	struct sxe_adapter *adapter = irq_data->adapter;
	struct sxe_ring *ring;
	int cpu = get_cpu();

	if (irq_data->cpu == cpu) {
		goto out_no_update;
	}

	sxe_for_each_ring(ring, irq_data->tx.list) {
		sxe_tx_tph_update(adapter, ring, cpu);
	}

	if (irq_data->tx.xdp_ring) {
		sxe_tx_tph_update(adapter, irq_data->tx.xdp_ring, cpu);
	}

	sxe_for_each_ring(ring, irq_data->rx.list) {
		sxe_rx_tph_update(adapter, ring, cpu);
	}

	irq_data->cpu = cpu;

out_no_update:
	put_cpu();
	return;
}

void sxe_tph_setup(struct sxe_adapter *adapter)
{
	int i;
	struct sxe_hw *hw = &adapter->hw;

	if (adapter->cap & SXE_TPH_ENABLE) {
		hw->dma.ops->tph_switch(hw, true);
	} else {
		hw->dma.ops->tph_switch(hw, false);
	}

	for (i = 0; i < adapter->irq_ctxt.ring_irq_num; i++) {
		adapter->irq_ctxt.irq_data[i]->cpu = -1;
		sxe_tph_update(adapter->irq_ctxt.irq_data[i]);
	}

	return;
}

static void sxe_tph_init(struct sxe_adapter *adapter)
{
	struct device *dev = &adapter->pdev->dev;

	if (dca_add_requester(dev) == 0) {
		adapter->cap |= SXE_TPH_ENABLE;
		sxe_tph_setup(adapter);
	}

	return;
}

static void sxe_tph_uninit(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw  = &adapter->hw;
	struct device *dev = &adapter->pdev->dev;

	if (adapter->cap & SXE_TPH_ENABLE) {
		adapter->cap &= ~SXE_TPH_ENABLE;
		dca_remove_requester(dev);
		hw->dma.ops->tph_switch(hw, false);
	}

	return;
}

static int __sxe_tph_notify(struct device *dev, void *data)
{
	struct sxe_adapter *adapter = dev_get_drvdata(dev);
	struct sxe_hw *hw  = &adapter->hw;
	unsigned long event = *(unsigned long *)data;

	if (!(adapter->cap & SXE_TPH_CAPABLE)) {
		goto l_end;
	}

	switch (event) {
	case DCA_PROVIDER_ADD:
		if (adapter->cap & SXE_TPH_ENABLE) {
			break;
		}

		if (dca_add_requester(dev) == 0) {
			adapter->cap |= SXE_TPH_ENABLE;
			hw->dma.ops->tph_switch(hw, true);
			break;
		}
		fallthrough;
	case DCA_PROVIDER_REMOVE:
		if (adapter->cap & SXE_TPH_ENABLE) {
			dca_remove_requester(dev);
			adapter->cap &= ~SXE_TPH_ENABLE;
			hw->dma.ops->tph_switch(hw, false);
		}
		break;
	}

l_end:
	return 0;
}
#endif 

STATIC int sxe_config_dma_mask(struct sxe_adapter * adapter)
{
	int ret = 0;

	if (dma_set_mask_and_coherent(&adapter->pdev->dev,
				DMA_BIT_MASK(SXE_DMA_BIT_WIDTH_64))) {
		LOG_ERROR_BDF("device[pci_id %u] 64 dma mask and coherent set failed\n",
					adapter->pdev->dev.id);
		ret = dma_set_mask_and_coherent(&adapter->pdev->dev,
					DMA_BIT_MASK(SXE_DMA_BIT_WIDTH_32));
		if (ret) {
			LOG_DEV_ERR("device[pci_id %u] 32 dma mask and coherent set failed\n",
					adapter->pdev->dev.id);
		}
	}

	return ret;
}

STATIC int sxe_pci_init(struct sxe_adapter * adapter)
{
	int ret;
	size_t len;
	resource_size_t bar_base_paddr;
	struct pci_dev *pdev = adapter->pdev;

	ret = pci_enable_device_mem(pdev);
	if (ret) {
		LOG_ERROR_BDF("device[pci_id %u] pci enable failed\n", pdev->dev.id);
		goto l_pci_enable_device_mem_failed;
	}

	ret = pci_request_mem_regions(pdev, SXE_DRV_NAME);
	if (ret) {
		LOG_DEV_ERR("device[pci_id %u] request IO memory failed\n", pdev->dev.id);
		goto l_pci_request_mem_failed;
	}

#ifndef DELETE_PCIE_ERROR_REPORTING
	pci_enable_pcie_error_reporting(pdev);
#endif
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

	LOG_INFO_BDF("bar_base_paddr = 0x%llx, len = %zu, reg_set_vaddr = 0x%p\n",
	(u64)bar_base_paddr, len, adapter->hw.reg_base_addr);
	return 0;

l_ioremap_failed:
#ifndef DELETE_PCIE_ERROR_REPORTING
	pci_disable_pcie_error_reporting(pdev);
#endif
	pci_release_mem_regions(pdev);
l_pci_request_mem_failed:
	pci_disable_device(pdev);
l_pci_enable_device_mem_failed:
	return ret;
}

STATIC void sxe_pci_exit(struct sxe_adapter * adapter)
{
	bool disable_dev;
	struct pci_dev *pdev = adapter->pdev;

	if (adapter->hw.reg_base_addr) {
		iounmap(adapter->hw.reg_base_addr);
		adapter->hw.reg_base_addr = NULL;
	}

	disable_dev = !test_and_set_bit(SXE_DISABLED, &adapter->state);

	if (pci_is_enabled(pdev)) {
#ifndef DELETE_PCIE_ERROR_REPORTING
		pci_disable_pcie_error_reporting(pdev);
#endif
		pci_release_mem_regions(pdev);
		if (disable_dev) {
			pci_disable_device(pdev);
		}
		pci_set_drvdata(pdev, NULL);
	}

	return;
}

STATIC s32 sxe_get_mac_addr_from_fw(struct sxe_adapter *adapter)
{
	s32 ret;
	struct sxe_driver_cmd cmd;
	struct sxe_default_mac_addr_resp mac;
	struct sxe_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;

	cmd.req      = NULL;
	cmd.req_len  = 0;
	cmd.resp     = &mac;
	cmd.resp_len = sizeof(mac);
	cmd.trace_id = 0;
	cmd.opcode   = SXE_CMD_R0_MAC_GET;
	cmd.is_interruptible = true;
	ret = sxe_driver_cmd_trans(hw, &cmd);
	if (ret) {
		LOG_ERROR_BDF("hdc trans failed ret=%d, cmd:mac addr get\n", ret);
		ret = -EIO;
	} else {
#ifndef HAVE_ETH_HW_ADDR_SET_API
		memcpy(netdev->dev_addr, mac.addr, SXE_MAC_ADDR_LEN);
#else
		eth_hw_addr_set(netdev, mac.addr);
#endif
	}

	return ret;
}

STATIC s32 sxe_default_mac_addr_get(struct sxe_adapter *adapter)
{
	s32 ret;
	struct net_device *netdev = adapter->netdev;

	ret = sxe_get_mac_addr_from_fw(adapter);
	if (ret || (!sxe_allow_inval_mac() && !is_valid_ether_addr(netdev->dev_addr))) {
		LOG_DEV_WARN("invalid default mac addr:%pM result:%d\n",
			     netdev->dev_addr, ret);
		ret = -EIO;
		goto l_out;
	}

	LOG_DEV_INFO("default mac addr = %pM\n", netdev->dev_addr);
	ether_addr_copy(adapter->mac_filter_ctxt.def_mac_addr, netdev->dev_addr);
	ether_addr_copy(adapter->mac_filter_ctxt.cur_mac_addr, netdev->dev_addr);

l_out:
	return ret;
}

STATIC s32 sxe_mac_addr_init(struct sxe_hw *hw, struct net_device *netdev)
{
	s32 ret;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	ret = sxe_mac_filter_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("rar entry num:%u mta entry num:%u "
			  "mac filter init fail.(ret:%d)\n",
			   SXE_UC_ENTRY_NUM_MAX,
			   SXE_MTA_ENTRY_NUM_MAX,
			   ret);
		goto l_ret;
	}

	sxe_mac_addr_set(adapter);

	LOG_INFO_BDF("hw adapter:%p reg_base_addr:%p "
		  "current mac addr:%pM "
		  "perm mac addr:%pM\n",
		  adapter, hw->reg_base_addr,
		  adapter->mac_filter_ctxt.cur_mac_addr,
		  adapter->mac_filter_ctxt.def_mac_addr);

l_ret:
	return ret;
}

STATIC struct sxe_adapter *sxe_adapter_create(struct pci_dev *pdev)
{
	struct net_device *netdev;
	struct sxe_adapter * adapter = NULL;

	netdev = alloc_etherdev_mq(sizeof(struct sxe_adapter),
				SXE_TXRX_RING_NUM_MAX);
	if (!netdev) {
		LOG_ERROR("device[pci_id %u] sxe net device alloc failed\n", pdev->dev.id);
		goto l_netdev_alloc_failed;
	}

	adapter = netdev_priv(netdev);
	adapter->pdev = pdev;
	adapter->netdev = netdev;

	adapter->msg_enable = netif_msg_init(-1,
					SXE_MSG_LEVEL_DEFAULT);

	LOG_INFO_BDF("adapter:%pK netdev:%pK pdev:%pK\n", adapter, netdev, pdev);

l_netdev_alloc_failed:
	return adapter;
}

void sxe_hw_start(struct sxe_hw *hw)
{
	hw->mac.auto_restart = true;

	hw->filter.vlan.ops->filter_array_clear(hw);

	hw->stat.ops->stats_clear(hw);

	hw->setup.ops->no_snoop_disable(hw);

	hw->dma.ops->dcb_rate_limiter_clear(hw, SXE_TXRX_RING_NUM_MAX);

	hw->mac.ops->fc_autoneg_localcap_set(hw);

	LOG_INFO("init auto_restart:%u\n", hw->mac.auto_restart);
}

STATIC s32 sxe_mng_reset(struct sxe_adapter *adapter, bool enable)
{
	s32 ret;
	struct sxe_driver_cmd cmd;
	sxe_mng_rst_s mng_rst;
	struct sxe_hw *hw = &adapter->hw;

	mng_rst.enable = enable;
	LOG_INFO_BDF("mng reset, enable=%x\n", enable);

	cmd.req              = &mng_rst;
	cmd.req_len          = sizeof(mng_rst);
	cmd.resp             = NULL;
	cmd.resp_len         = 0;
	cmd.trace_id         = 0;
	cmd.opcode           = SXE_CMD_MNG_RST;
	cmd.is_interruptible = true;
	ret = sxe_driver_cmd_trans(hw, &cmd);
	if (ret) {
		LOG_ERROR_BDF("mng reset failed, ret=%d\n", ret);
		goto l_end;
	}

	LOG_INFO_BDF("mng reset success, enable=%x\n", enable);

l_end:
	return ret;
}

s32 sxe_hw_reset(struct sxe_adapter *adapter)
{
	s32 ret;
	struct sxe_hw *hw = &adapter->hw;

	hw->dbu.ops->rx_cap_switch_off(hw);

	hw->irq.ops->all_irq_disable(hw);

	hw->irq.ops->pending_irq_read_clear(hw);

	hw->dma.ops->all_ring_disable(hw, SXE_TXRX_RING_NUM_MAX);

	ret = sxe_mng_reset(adapter, false);
	if (ret) {
		LOG_ERROR_BDF("mng reset disable failed, ret=%d\n", ret);
		ret = -EPERM;
		goto l_end;
	}


	ret = hw->setup.ops->reset(hw);
	if (ret) {
		LOG_ERROR_BDF("nic reset failed, ret=%d\n", ret);
		ret = -EPERM;
		goto l_end;
	}

	msleep(50);

	ret = sxe_mng_reset(adapter, true);
	if (ret) {
		LOG_ERROR_BDF("mng reset enable failed, ret=%d\n", ret);
		ret = -EPERM;
		goto l_end;
	}

	hw->filter.mac.ops->uc_addr_clear(hw);

	hw->filter.mac.ops->vt_disable(hw);

l_end:
	return ret;
}

static s32 sxe_led_reset(struct sxe_adapter *adapter)
{
	s32 ret;
	s32 resp;
	struct sxe_led_ctrl ctrl;
	struct sxe_driver_cmd cmd;
	struct sxe_hw *hw = &adapter->hw;

	ctrl.mode = SXE_IDENTIFY_LED_RESET; 
	ctrl.duration = 0;

	cmd.req      = &ctrl;
	cmd.req_len  = sizeof(ctrl);
	cmd.resp     = &resp;
	cmd.resp_len = sizeof(resp);
	cmd.trace_id = 0;
	cmd.opcode   = SXE_CMD_LED_CTRL;
	cmd.is_interruptible = false;
	ret = sxe_driver_cmd_trans(hw, &cmd);
	if (ret) {
		LOG_ERROR_BDF("hdc trans failed ret=%d, cmd:led reset\n", ret);
		ret = -EIO;
	}

	LOG_INFO_BDF("led reset\n");

	return ret;
}

static void sxe_link_fc_init(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	hw->mac.ops->fc_param_init(hw);
	return;
}

static inline u32 sxe_readl(const volatile void *reg)
{
	return readl(reg);
}

static inline void sxe_writel(u32 value, volatile void *reg)
{
	writel(value, reg);
	return;
}

STATIC int sxe_hw_base_init(struct sxe_adapter *adapter)
{
	int ret;
	struct sxe_hw *hw = &adapter->hw;

	hw->adapter = adapter;
	adapter->vlan_ctxt.vlan_table_size = SXE_VFT_TBL_SIZE;

	sxe_hw_ops_init(hw);
	sxe_hw_reg_handle_init(hw, sxe_readl, sxe_writel);

	sxe_hdc_channel_init(&adapter->hdc_ctxt);

	hw->hdc.ops->drv_status_set(hw, (U32)true);

	ret = sxe_phy_init(adapter);
	if (ret == -SXE_ERR_SFF_NOT_SUPPORTED) {
		LOG_DEV_ERR("sfp is not sfp+, not supported, ret=%d\n", ret);
		ret = -EPERM;
		goto l_ret;
	} else if (ret) {
		LOG_ERROR_BDF("phy init failed, ret=%d\n", ret);
	}

	ret = sxe_default_mac_addr_get(adapter);
	if (ret) {
		LOG_ERROR_BDF("get valid default mac addr failed, ret=%d\n", ret);
		goto l_ret;
	}

	sxe_link_fc_init(adapter);

	ret = sxe_hw_reset(adapter);
	if (ret < 0) {
		LOG_ERROR_BDF("hw init failed, ret=%d\n", ret);
		goto l_ret;
	} else {
		sxe_hw_start(hw);
	}

	ret = sxe_mac_addr_init(hw, adapter->netdev);
	if (ret) {
		LOG_ERROR_BDF("mac addr init fail, ret=%d\n", ret);
	}

	sxe_led_reset(adapter);

l_ret:
	if (ret) {
		hw->hdc.ops->drv_status_set(hw, (U32)false);
	}
	return ret;
}

static void sxe_vt_init(struct sxe_adapter *adapter)
{
	set_bit(0, adapter->vt_ctxt.pf_pool_bitmap);

	sxe_sriov_init(adapter);

	return;
}

STATIC void sxe_fnav_ctxt_init(struct sxe_adapter *adapter)
{
	adapter->fnav_ctxt.sample_rate = SXE_FNAV_DEFAULT_SAMPLE_RATE;
	adapter->fnav_ctxt.rules_table_size = SXE_FNAV_RULES_TABLE_SIZE_64K;
	spin_lock_init(&adapter->fnav_ctxt.specific_lock);

	adapter->fnav_ctxt.fdir_overflow_time = 0;
	spin_lock_init(&adapter->fnav_ctxt.sample_lock);
	adapter->fnav_ctxt.sample_rules_cnt = 0;

	return ;
}

#ifdef HAVE_AF_XDP_ZERO_COPY
static s32 sxe_xdp_mem_alloc(struct sxe_adapter *adapter)
{
	s32 ret = 0;

	adapter->af_xdp_zc_qps = bitmap_zalloc(SXE_XDP_RING_NUM_MAX, GFP_KERNEL);
	if (!adapter->af_xdp_zc_qps) {
		ret = -ENOMEM;
	}

	return ret;
}

static void sxe_xdp_mem_free(struct sxe_adapter *adapter)
{
	bitmap_free(adapter->af_xdp_zc_qps);
	adapter->af_xdp_zc_qps = NULL;
	return;
}
#endif
void sxe_fw_version_get(struct sxe_adapter *adapter)
{
	s32 ret;
	sxe_version_resp_s resp;
	struct sxe_driver_cmd cmd;
	struct sxe_hw *hw = &adapter->hw;

	cmd.req      = NULL;
	cmd.req_len  = 0;
	cmd.resp     = &resp;
	cmd.resp_len = sizeof(resp);
	cmd.trace_id = 0;
	cmd.opcode   = SXE_CMD_FW_VER_GET;
	cmd.is_interruptible = true;
	ret = sxe_driver_cmd_trans(hw, &cmd);
	if (ret) {
		LOG_ERROR_BDF("get version failed, ret=%d\n", ret);
		memset(adapter->fw_info.fw_version, 0, SXE_VERSION_LEN);
	} else {
		memcpy(adapter->fw_info.fw_version, resp.fw_version,
			SXE_VERSION_LEN);
	}

	return;
}

static s32 sxe_sw_base_init1(struct sxe_adapter *adapter)
{
	s32 ret;

	set_bit(SXE_DOWN, &adapter->state);

	adapter->irq_ctxt.rx_irq_interval = 1;
	adapter->irq_ctxt.tx_irq_interval = 1;

	spin_lock_init(&adapter->irq_ctxt.event_irq_lock);

	ret = sxe_config_space_irq_num_get(adapter);
	if (ret) {
		LOG_ERROR_BDF("get pci cfg irq num fail.(err:%d)\n", ret);
		goto l_out;
	}

	sxe_vt_init(adapter);

	sxe_ring_feature_init(adapter);

	sxe_fnav_ctxt_init(adapter);

#ifdef HAVE_AF_XDP_ZERO_COPY
	ret = sxe_xdp_mem_alloc(adapter);
	if (ret) {
		LOG_ERROR_BDF("xdp mem alloc failed, ret=%d\n", ret);
		goto l_out;
	}
#endif
	sxe_fw_version_get(adapter);

	LOG_INFO_BDF("adapter rx_irq_interval:%u tx_irq_interval:%u.\n",
		    adapter->irq_ctxt.rx_irq_interval,
		    adapter->irq_ctxt.tx_irq_interval);

l_out:
	return ret;
}

s32 sxe_ring_irq_init(struct sxe_adapter *adapter)
{
	s32 ret;

	sxe_ring_num_set(adapter);

	ret = sxe_irq_ctxt_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("interrupt context init fail.(err:%d)\n", ret);
		goto l_end;
	}

	sxe_ring_reg_map(adapter);

	LOG_DEV_INFO("multiqueue %s: rx queue count = %u, tx queue count = %u xdp queue count = %u\n",
			(adapter->rx_ring_ctxt.num > 1) ? "enabled" : "disabled",
			adapter->rx_ring_ctxt.num, adapter->tx_ring_ctxt.num,
			adapter->xdp_ring_ctxt.num);

l_end:
	return ret;
}

void sxe_ring_irq_exit(struct sxe_adapter *adapter)
{
	sxe_irq_ctxt_exit(adapter);

	return;
}

#ifdef SXE_WOL_CONFIGURE

void sxe_wol_init(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;

	hw->filter.mac.ops->wol_status_set(hw);
	adapter->wol = 0;
	sxe_fw_wol_set(adapter, 0);
	device_set_wakeup_enable(&adapter->pdev->dev, adapter->wol);

	return;
}

#endif

static void sxe_sw_base_init2(struct sxe_adapter *adapter)
{
	sxe_ring_stats_init(adapter);

	sxe_monitor_init(adapter);

#ifdef SXE_TPH_CONFIGURE
	adapter->cap |= SXE_TPH_CAPABLE;
#endif

#ifdef SXE_DCB_CONFIGURE
	sxe_dcb_init(adapter);
#endif

#ifdef SXE_IPSEC_CONFIGURE
	sxe_ipsec_offload_init(adapter);
#endif

#ifdef SXE_WOL_CONFIGURE
	sxe_wol_init(adapter);
#endif
	return;
}

STATIC int sxe_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int ret;
	struct sxe_adapter *adapter;
	const char *device_name = dev_name(&pdev->dev);

	adapter = sxe_adapter_create(pdev);
	if (!adapter) {
		LOG_ERROR("adapter create failed.\n");
		ret = -ENOMEM;
		goto l_adapter_create_failed;
	}

	strlcpy(adapter->dev_name, device_name,
		min_t(u32, strlen(device_name) + 1, DEV_NAME_LEN));

	ret = sxe_pci_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("pci init failed.(ret:%d)\n", ret);
		goto l_pci_init_failed;
	}

	ret = sxe_config_dma_mask(adapter);
	if (ret) {
		LOG_ERROR_BDF("config dma mask failed.(ret:%d)\n", ret);
		goto l_config_dma_mask_failed;
	}

	sxe_netdev_init(adapter->netdev, pdev);

	ret = sxe_hw_base_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("hardware base init failed.(ret:%d)\n", ret);
		goto l_config_dma_mask_failed;
	}

	ret = sxe_sw_base_init1(adapter);
	if (ret) {
		LOG_ERROR_BDF("sw base init1 failed.(ret:%d)\n", ret);
		goto l_sw_base_init1_failed;
	}

	ret = sxe_ring_irq_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("interrupt ring assign scheme init failed, err=%d\n", ret);
		goto l_sw_base_init1_failed;
	}

	ret = sxe_cli_cdev_create(adapter);
	if (ret) {
		LOG_ERROR_BDF("create cli char dev failed, ret = %d\n", ret);
		goto l_create_cdev_failed;
	}

	sxe_sw_base_init2(adapter);

	if (adapter->phy_ctxt.ops->sfp_tx_laser_disable) {
		adapter->phy_ctxt.ops->sfp_tx_laser_disable(adapter);
	}

	strcpy(adapter->netdev->name, "eth%d");
	ret = register_netdev(adapter->netdev);
	if (ret) {
		LOG_ERROR_BDF("register netdev failed.(ret:%d)\n", ret);
		goto l_register_netdev_failed;
	}

	netif_carrier_off(adapter->netdev);

#ifdef SXE_TPH_CONFIGURE
	sxe_tph_init(adapter);
#endif

	sxe_debugfs_entries_init(adapter);

#ifdef SXE_PHY_CONFIGURE
	sxe_mdiobus_init(adapter);
#endif

	LOG_INFO_BDF("%s %s %s %s %s pf deviceId:0x%x probe done.\n",
		 dev_driver_string(pdev->dev.parent),
		 dev_name(pdev->dev.parent),
		 netdev_name(adapter->netdev),
		 dev_driver_string(&pdev->dev),
		 dev_name(&pdev->dev),
		 pdev->device);

	return 0;

l_register_netdev_failed:
	sxe_cli_cdev_delete(adapter);
l_create_cdev_failed:
	sxe_ring_irq_exit(adapter);
l_sw_base_init1_failed:
#ifdef HAVE_AF_XDP_ZERO_COPY
	if (adapter->af_xdp_zc_qps) {
		sxe_xdp_mem_free(adapter);
	}
#endif
	sxe_mac_filter_destroy(adapter);
l_config_dma_mask_failed:
	sxe_pci_exit(adapter);
l_pci_init_failed:
	free_netdev(adapter->netdev);
l_adapter_create_failed:
	return ret;
}

static void sxe_fuc_exit(struct sxe_adapter *adapter)
{

	cancel_work_sync(&adapter->monitor_ctxt.work);
	cancel_work_sync(&adapter->hdc_ctxt.time_sync_work);

#ifdef SXE_PHY_CONFIGURE
	sxe_mdiobus_exit(adapter);
#endif

#ifdef SXE_TPH_CONFIGURE
	sxe_tph_uninit(adapter);
#endif

	return;
}

static void sxe_vt_exit(struct sxe_adapter *adapter)
{
	sxe_vf_exit(adapter);

	return;
}

STATIC void sxe_remove(struct pci_dev *pdev)
{
	struct sxe_adapter * adapter = pci_get_drvdata(pdev);
	struct sxe_hw *hw = &adapter->hw;
	struct net_device *netdev;

	if (!adapter) {
		goto l_end;
	}

	set_bit(SXE_REMOVING, &adapter->state);
	netdev = adapter->netdev;

	hw->hdc.ops->drv_status_set(hw, (U32)false);

	sxe_debugfs_entries_exit(adapter);

	sxe_fuc_exit(adapter);

	sxe_vt_exit(adapter);

	if (NETREG_REGISTERED == netdev->reg_state) {
		unregister_netdev(netdev);
	}

	sxe_cli_cdev_delete(adapter);

#ifdef SXE_IPSEC_CONFIGURE
	sxe_ipsec_offload_exit(adapter);
#endif

	sxe_ring_irq_exit(adapter);

#ifdef SXE_DCB_CONFIGURE
	sxe_dcb_exit(adapter);
#endif

	sxe_pci_exit(adapter);

	LOG_DEV_INFO("complete\n");

	sxe_mac_filter_destroy(adapter);
#ifdef HAVE_AF_XDP_ZERO_COPY
	sxe_xdp_mem_free(adapter);
#endif
	free_netdev(adapter->netdev);

	LOG_INFO("%s %s %s %s deviceId:0x%x remove done.\n",
		 dev_driver_string(pdev->dev.parent),
		 dev_name(pdev->dev.parent),
		 dev_driver_string(&pdev->dev),
		 dev_name(&pdev->dev),
		 pdev->device);

l_end:
	return;
}

static int __sxe_shutdown(struct pci_dev *pdev, bool *enable_wake)
{
	struct sxe_adapter *adapter = pci_get_drvdata(pdev);
	struct sxe_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;

#ifdef SXE_WOL_CONFIGURE
	u32 wol = adapter->wol;
#else
	u32 wol = 0;
#endif
	int ret = 0;

	rtnl_lock();
	netif_device_detach(netdev);

	if (netif_running(netdev)) {
		sxe_terminate(adapter);
	}

	hw->hdc.ops->drv_status_set(hw, (U32)false);

	sxe_ring_irq_exit(adapter);
	rtnl_unlock();

#ifdef CONFIG_PM
	ret = pci_save_state(pdev);
	if (ret) {
		LOG_DEBUG_BDF("pci save state err:%d\n", ret);
		return ret;
	}
#endif

#ifdef SXE_WOL_CONFIGURE
	if (wol) {
		__sxe_set_rx_mode(netdev, true);
		hw->filter.mac.ops->wol_mode_set(hw, wol);
	} else {
		hw->filter.mac.ops->wol_mode_clean(hw);
	}
#endif

	pci_wake_from_d3(pdev, !!wol);

	*enable_wake = !!wol;

#ifdef SXE_WOL_CONFIGURE
	if (wol) {
		hw->dbu.ops->rx_func_switch_on(hw);
	}
#endif

	if (!test_and_set_bit(SXE_DISABLED, &adapter->state)) {
		pci_disable_device(pdev);
	}

	return ret;
}

STATIC void sxe_shutdown(struct pci_dev *pdev)
{
	bool wol_enable = false;

	__sxe_shutdown(pdev, &wol_enable);

	if (system_state == SYSTEM_POWER_OFF) {
		pci_wake_from_d3(pdev, wol_enable);
		pci_set_power_state(pdev, PCI_D3hot);
	}

	return;
}

#ifdef CONFIG_PM
static int sxe_suspend(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	int ret;
	bool wol_enable;
	struct sxe_adapter *adapter = pci_get_drvdata(pdev);
	struct sxe_hw *hw = &adapter->hw;

	ret = __sxe_shutdown(pdev, &wol_enable);

	cancel_work_sync(&adapter->monitor_ctxt.work);
	cancel_work_sync(&adapter->hdc_ctxt.time_sync_work);

	sxe_hdc_channel_destroy(hw);
	if (ret) {
		LOG_ERROR_BDF("driver shutdown err:%d\n", ret);
		goto l_ret;
	}

	LOG_DEBUG_BDF("pci dev[%p], wol_enable:%s\n",
			pdev, wol_enable ? "yes" : "no");

	if (wol_enable) {
		pci_prepare_to_sleep(pdev);
	} else {
		pci_wake_from_d3(pdev, false);
		pci_set_power_state(pdev, PCI_D3hot);
	}

l_ret:
	return ret;
}

static int sxe_resume(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct sxe_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;
	s32 ret;
#ifdef SXE_WOL_CONFIGURE
	struct sxe_hw *hw = &adapter->hw;
#endif

	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);

	pci_save_state(pdev);

	ret = pci_enable_device_mem(pdev);
	if (ret) {
		LOG_DEV_ERR("cannot enable pci device from suspend\n");
		goto l_ret;
	}

	smp_mb__before_atomic();
	clear_bit(SXE_DISABLED, &adapter->state);
	pci_set_master(pdev);

	pci_wake_from_d3(pdev, false);

	sxe_hdc_available_set(1);

	sxe_reset(adapter);

#ifdef SXE_WOL_CONFIGURE
	hw->filter.mac.ops->wol_status_set(hw);
#endif

	INIT_WORK(&adapter->hdc_ctxt.time_sync_work, sxe_time_sync_handler);
	INIT_WORK(&adapter->monitor_ctxt.work, sxe_work_cb);

	rtnl_lock();
	ret = sxe_ring_irq_init(adapter);
	LOG_DEBUG_BDF("ring irq init finish, ret=%d\n", ret);
	if (!ret && netif_running(netdev)) {
		ret = sxe_open(netdev);
		LOG_DEBUG_BDF("sxe open adapter finish, ret=%d\n", ret);
	}

	if (!ret) {
		netif_device_attach(netdev);
	}

	rtnl_unlock();

l_ret:
	return ret;
}
#endif 

#ifdef CONFIG_PCI_IOV
#ifdef HAVE_NO_PCIE_FLR
static inline void sxe_issue_vf_flr(struct sxe_adapter *adapter,
				      struct pci_dev *vf_dev)
{
	int pos, i;
	u16 status;

	for (i = 0; i < 4; i++) {
		if (i)
			msleep((1 << (i - 1)) * 100);

		pcie_capability_read_word(vf_dev, PCI_EXP_DEVSTA, &status);
		if (!(status & PCI_EXP_DEVSTA_TRPND))
			goto clear;
	}

	LOG_DEV_WARN("Issuing VFLR with pending transactions\n");

clear:
	pos = pci_find_capability(vf_dev, PCI_CAP_ID_EXP);
	if (!pos)
		return;

	LOG_DEV_ERR("Issuing VFLR for VF %s\n", pci_name(vf_dev));
	pci_write_config_word(vf_dev, pos + PCI_EXP_DEVCTL,
			      PCI_EXP_DEVCTL_BCR_FLR);
	msleep(100);
}
#endif 
#endif 

static pci_ers_result_t sxe_io_error_detected(struct pci_dev *pdev,
						pci_channel_state_t state)
{
	pci_ers_result_t ret = PCI_ERS_RESULT_DISCONNECT;
	struct sxe_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;

#ifdef CONFIG_PCI_IOV
	struct sxe_hw *hw = &adapter->hw;
	struct pci_dev *bdev, *vfdev;
	u32 dw0, dw1, dw2, dw3;
	int vf, pos;
	u16 req_id, pf_func;

	LOG_DEBUG_BDF("sriov open, vf error process\n");
	if (adapter->vt_ctxt.num_vfs == 0) {
		LOG_DEBUG_BDF("num vfs == 0\n");
		goto skip_bad_vf_detection;
	}

	bdev = pdev->bus->self;
	while (bdev && (pci_pcie_type(bdev) != PCI_EXP_TYPE_ROOT_PORT)) {
		bdev = bdev->bus->self;
	}

	if (!bdev) {
		LOG_DEBUG_BDF("no pci dev\n");
		goto skip_bad_vf_detection;
	}

	pos = pci_find_ext_capability(bdev, PCI_EXT_CAP_ID_ERR);
	if (!pos) {
		LOG_DEBUG_BDF("pci dev not support aer\n");
		goto skip_bad_vf_detection;
	}

	dw0 = sxe_read_pci_cfg_dword(adapter, pos + PCI_ERR_HEADER_LOG);
	dw1 = sxe_read_pci_cfg_dword(adapter, pos + PCI_ERR_HEADER_LOG + SXE_DW1_OFFSET);
	dw2 = sxe_read_pci_cfg_dword(adapter, pos + PCI_ERR_HEADER_LOG + SXE_DW2_OFFSET);
	dw3 = sxe_read_pci_cfg_dword(adapter, pos + PCI_ERR_HEADER_LOG + SXE_DW3_OFFSET);
	if (sxe_is_hw_fault(hw)) {
		LOG_ERROR_BDF("hw is fault\n");
		goto skip_bad_vf_detection;
	}

	req_id = dw1 >> SXE_REQ_ID_SHIFT;
	if (!(req_id & SXE_REQ_ID_VF_MASK)) {
		LOG_DEBUG_BDF("this is not a vf\n");
		goto skip_bad_vf_detection;
	}

	pf_func = req_id & SXE_REQ_ID_PF_MASK;
	if ((pf_func & 1) == (pdev->devfn & 1)) {
		u32 device_id = SXE_DEV_ID_ASIC;

		vf = (req_id & SXE_REQ_ID_VF_ID_MASK) >> 1;
		LOG_DEV_ERR("vf %d has caused a pcie error\n", vf);
		LOG_DEV_ERR("tlp: dw0: %8.8x\tdw1: %8.8x\tdw2: "
				"%8.8x\tdw3: %8.8x\n",
		 		dw0, dw1, dw2, dw3);

		vfdev = pci_get_device(PCI_VENDOR_ID_STARS, device_id, NULL);
		while (vfdev) {
			if (vfdev->devfn == (req_id & 0xFF)) {
				break;
			}
			vfdev = pci_get_device(PCI_VENDOR_ID_STARS,
					       device_id, vfdev);
		}

		if (vfdev) {
			LOG_DEBUG_BDF("vfdev[%p] miss, do flr\n",vfdev);
#ifdef HAVE_NO_PCIE_FLR
			sxe_issue_vf_flr(adapter, vfdev);
#else
			pcie_flr(vfdev);
#endif
			pci_dev_put(vfdev);
		}
	}

	adapter->vt_ctxt.err_refcount++;
	LOG_ERROR_BDF("vf err count=%u\n", adapter->vt_ctxt.err_refcount);
	ret = PCI_ERS_RESULT_RECOVERED;
	goto l_ret;

skip_bad_vf_detection:
#endif 
	LOG_DEBUG_BDF("oops, pci dev[%p] got io error detect, state=0x%x\n",
			pdev, (u32)state);

	if (!test_bit(SXE_MONITOR_WORK_INITED, &adapter->monitor_ctxt.state)) {
		LOG_ERROR_BDF("driver adapter service not init\n");
		goto l_ret;
	}

	if (!netif_device_present(netdev)) {
		LOG_ERROR_BDF("pci netdev not present\n");
		goto l_ret;
	}

	rtnl_lock();
	netif_device_detach(netdev);

	LOG_DEBUG_BDF("netdev[%p], detached and continue\n",netdev);

	if (netif_running(netdev)) {
		sxe_terminate(adapter);
	}

	if (state == pci_channel_io_perm_failure) {
		rtnl_unlock();
		LOG_ERROR_BDF("pci channel io perm failure\n");
		goto l_ret;
	}

	if (!test_and_set_bit(SXE_DISABLED, &adapter->state)) {
		pci_disable_device(pdev);
	}

	rtnl_unlock();
	ret = PCI_ERS_RESULT_NEED_RESET;

l_ret:
	LOG_DEBUG_BDF("netdev[%p] error detected end and ret=0x%x\n",netdev, ret);
	return ret;
}

static pci_ers_result_t sxe_io_slot_reset(struct pci_dev *pdev)
{
	struct sxe_adapter *adapter = pci_get_drvdata(pdev);
	pci_ers_result_t result;
#ifdef SXE_WOL_CONFIGURE
	struct sxe_hw *hw = &adapter->hw;
#endif

	LOG_DEBUG_BDF("oops, pci dev[%p] got io slot reset\n",pdev);

	if (pci_enable_device_mem(pdev)) {
		LOG_MSG_ERR(probe, "pci dev[%p] cannot re-enable "
				"pci device after reset.\n",pdev);
		result = PCI_ERS_RESULT_DISCONNECT;
	} else {
		LOG_DEBUG_BDF("pci dev[%p] start re-enable\n", pdev);
		smp_mb__before_atomic();
		clear_bit(SXE_DISABLED, &adapter->state);
		pci_set_master(pdev);
		pci_restore_state(pdev);
		pci_save_state(pdev);

		pci_wake_from_d3(pdev, false);

		sxe_reset(adapter);

#ifdef SXE_WOL_CONFIGURE
		hw->filter.mac.ops->wol_status_set(hw);
#endif

		result = PCI_ERS_RESULT_RECOVERED;
	}

	LOG_DEBUG_BDF("pci dev[%p] io slot reset end result=0x%x\n",
			pdev, (u32)result);

	return result;
}

static void sxe_io_resume(struct pci_dev *pdev)
{
	struct sxe_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;

	LOG_DEBUG_BDF("oops, pci dev[%p] got io resume\n",pdev);

#ifdef CONFIG_PCI_IOV
	if (adapter->vt_ctxt.err_refcount) {
		LOG_DEV_INFO("resuming after vf err\n");
		adapter->vt_ctxt.err_refcount--;
		goto l_ret;
	}

#endif
	rtnl_lock();
	if (netif_running(netdev)) {
		LOG_DEBUG_BDF("netdev running, open adapter");
		sxe_open(netdev);
	}

	netif_device_attach(netdev);
	rtnl_unlock();
	LOG_DEBUG_BDF("pci dev[%p] io resume end\n",pdev);

#ifdef CONFIG_PCI_IOV
l_ret:
#endif
	return;
}

static const struct pci_error_handlers sxe_err_handler = {
	.error_detected = sxe_io_error_detected,
	.slot_reset = sxe_io_slot_reset,
	.resume = sxe_io_resume,
};

static const struct pci_device_id sxe_pci_tbl[] = {
	{PCI_VENDOR_ID_STARS, SXE_DEV_ID_ASIC, PCI_ANY_ID, PCI_ANY_ID, 0, 0 ,0},

	{0, }
};

static SIMPLE_DEV_PM_OPS(sxe_pm_ops, sxe_suspend, sxe_resume);
static struct pci_driver sxe_pci_driver = {
	.name            = SXE_DRV_NAME,
	.id_table        = sxe_pci_tbl,
	.probe           = sxe_probe,
	.remove          = sxe_remove,
	.driver.pm       = &sxe_pm_ops,
	.shutdown        = sxe_shutdown,
	.sriov_configure = sxe_sriov_configure,
	.err_handler     = &sxe_err_handler
};

#ifdef SXE_TPH_CONFIGURE
static int sxe_tph_notify(struct notifier_block *nb, unsigned long event,
			    void *p)
{
	int ret_val;

	ret_val = driver_for_each_device(&sxe_pci_driver.driver, NULL, &event,
					 __sxe_tph_notify);

	return ret_val ? NOTIFY_BAD : NOTIFY_DONE;
}

static struct notifier_block tph_notifier = {
	.notifier_call = sxe_tph_notify,
	.next          = NULL,
	.priority      = 0
};
#endif

#ifndef SXE_DRIVER_RELEASE
static ssize_t sxe_log_level_store(struct device_driver *dd,
		 const char *buf, size_t count)
{
	int level = 0;
	ssize_t ret = count;

	if (dd == NULL || buf == NULL) {
		goto l_out;
	}

	if (kstrtoint(buf, 10, &level)) {
		LOG_ERROR("invalid log level, could not set log level\n");
		ret = -EINVAL;
		goto l_out;
	}
	LOG_WARN("set log level to %d\n", level);

	sxe_level_set(level);
l_out:
	return ret;
}

static ssize_t sxe_log_level_show(struct device_driver *dd, char *buf)
{
	ssize_t ret = 0;
	s32 level = sxe_level_get();

	if (dd == NULL || buf == NULL) {
		goto l_out;
	}
	ret = snprintf(buf, PAGE_SIZE, "%d\n", level);

	LOG_DEBUG("get log level to %d\n", level);
l_out:
	return ret;
}

static ssize_t log_level_store(struct device_driver *dd,
		const char *buf, size_t count)
{
	return sxe_log_level_store(dd, buf, count);
}

static inline ssize_t log_level_show(struct device_driver *dd, char *buf)
{
	return sxe_log_level_show(dd, buf);
}

static ssize_t sxe_dump_status_store(struct device_driver *dd,
		 const char *buf, size_t count)
{
	int status = 0;
	ssize_t ret = count;

	if (dd == NULL || buf == NULL) {
		goto l_out;
	}

	if (kstrtoint(buf, 10, &status)) {
		LOG_ERROR("invalid status, could not set dump status\n");
		ret = -EINVAL;
		goto l_out;
	}
	LOG_WARN("set dump status to %d\n", status);

	sxe_bin_status_set(!!status);
l_out:
	return ret;
}

static ssize_t sxe_dump_status_show(struct device_driver *dd, char *buf)
{
	ssize_t ret = 0;
	s32 status = sxe_bin_status_get();

	if (dd == NULL || buf == NULL) {
		goto l_out;
	}
	ret = snprintf(buf, PAGE_SIZE, "%d\n", status);

	LOG_DEBUG("get log level to %d\n", status);
l_out:
	return ret;
}

static ssize_t dump_status_store(struct device_driver *dd,
		const char *buf, size_t count)
{
	return sxe_dump_status_store(dd, buf, count);
}

static inline ssize_t dump_status_show(struct device_driver *dd, char *buf)
{
	return sxe_dump_status_show(dd, buf);
}

static DRIVER_ATTR_RW(log_level);
static DRIVER_ATTR_RW(dump_status);

static s32 sxe_driver_create_file(void)
{
	s32 ret;

	ret = driver_create_file(&sxe_pci_driver.driver, &driver_attr_log_level);
	if (ret) {
		LOG_ERROR("driver create file attr log level failed\n");
		goto l_ret;
	}

	ret = driver_create_file(&sxe_pci_driver.driver, &driver_attr_dump_status);
	if (ret) {
		LOG_ERROR("driver create file attr dump status failed\n");
		goto l_remove_log_level;
	}

	goto l_ret;

l_remove_log_level:
	driver_remove_file(&sxe_pci_driver.driver, &driver_attr_log_level);
l_ret:
	return ret;
}

static void sxe_driver_remove_file(void)
{
	driver_remove_file(&sxe_pci_driver.driver, &driver_attr_log_level);
	driver_remove_file(&sxe_pci_driver.driver, &driver_attr_dump_status);
	return;
}
#endif

#ifdef SXE_DRIVER_TRACE
ssize_t trace_dump_store(struct device_driver *dd, const char *buf, size_t count)
{
    ssize_t ret = count;

    if (dd == NULL || buf == NULL) {
        goto l_out;
    }

    sxe_trace_dump();

l_out:
    return ret;
}

static inline ssize_t trace_dump_show(struct device_driver *dd, char *buf)
{
	return 0;
}

static DRIVER_ATTR_RW(trace_dump);

s32 sxe_trace_dump_create_file(void)
{
	s32 ret;

	ret = driver_create_file(&sxe_pci_driver.driver, &driver_attr_trace_dump);
	if (ret) {
		LOG_ERROR("driver create file attr log level failed\n");
	}

	return ret;
}

void sxe_trace_dump_remove_file(void)
{
	driver_remove_file(&sxe_pci_driver.driver, &driver_attr_trace_dump);
	return;
}
#endif 

static int __init sxe_init(void)
{
	int ret;

	LOG_PR_INFO("version[%s], commit_id[%s],"
		"branch[%s], build_time[%s]\n",
		SXE_VERSION, SXE_COMMIT_ID,
		SXE_BRANCH, SXE_BUILD_TIME);

#ifndef SXE_DRIVER_RELEASE
	ret = sxe_log_init(false);
	if (ret < 0) {
		LOG_PR_ERR("sxe log init fail.(err:%d)\n", ret);
		goto l_end;
	}
#endif

	sxe_workqueue = create_singlethread_workqueue(SXE_DRV_NAME);
	if (!sxe_workqueue) {
		LOG_PR_ERR("failed to create workqueue\n");
		ret = -ENOMEM;
		goto l_create_workque_failed;
	}

	sxe_fnav_workqueue = create_singlethread_workqueue(SXE_FNAV_NAME);
	if (!sxe_fnav_workqueue) {
		LOG_PR_ERR("failed to create fnav workqueue\n");
		ret = -ENOMEM;
		goto l_create_fnav_workque_failed;
	}

	fnav_cache = kmem_cache_create("fnav_sample_cache",
		sizeof(struct sxe_fnav_sample_work_info), 0, SLAB_HWCACHE_ALIGN, NULL);
	if(!fnav_cache) {
		LOG_PR_ERR("failed to create fnav kmem cache\n");
		ret = -ENOMEM;
		goto l_create_fnav_kmem_failed;
	}

	sxe_debugfs_init();

	ret = sxe_cli_cdev_register();
	if (ret) {
		LOG_ERROR("register cli char dev failed\n");
		goto l_cli_cdev_register_failed;
	}

	ret = pci_register_driver(&sxe_pci_driver);
	if (ret) {
		goto l_pci_register_driver_failed;
	}

#ifndef SXE_DRIVER_RELEASE
	ret = sxe_driver_create_file();
	if (ret) {
		goto l_register_driver_failed;
	}
#endif

#ifdef SXE_DRIVER_TRACE
	ret = sxe_trace_dump_create_file();
	if (ret) {
		LOG_ERROR("sxe_trace_dump_create_file failed: %d\n", ret);
	}
#endif 

#ifdef SXE_TPH_CONFIGURE
	dca_register_notify(&tph_notifier);
#endif
	LOG_INFO("sxe module init success\n");
	return 0;

#ifndef SXE_DRIVER_RELEASE
l_register_driver_failed:
	pci_unregister_driver(&sxe_pci_driver);
#endif

l_pci_register_driver_failed:
	sxe_cli_cdev_unregister();
l_cli_cdev_register_failed:
	kmem_cache_destroy(fnav_cache);
	fnav_cache = NULL;
	sxe_debugfs_exit();
l_create_fnav_kmem_failed:
	destroy_workqueue(sxe_fnav_workqueue);
	sxe_fnav_workqueue = NULL;
l_create_fnav_workque_failed:
	destroy_workqueue(sxe_workqueue);
	sxe_workqueue = NULL;
l_create_workque_failed:
#ifndef SXE_DRIVER_RELEASE
	sxe_log_exit();
l_end:
#endif
	LOG_INFO("sxe module init done, ret=%d\n", ret);
	return ret;
}

struct workqueue_struct *sxe_workqueue_get(void)
{
	return sxe_workqueue;
}

static void __exit sxe_exit(void)
{

#ifdef SXE_TPH_CONFIGURE
	dca_unregister_notify(&tph_notifier);
#endif

#ifndef SXE_DRIVER_RELEASE
	sxe_driver_remove_file();
#endif 

#ifdef SXE_DRIVER_TRACE
	sxe_trace_dump_remove_file();
#endif 

	pci_unregister_driver(&sxe_pci_driver);

	sxe_cli_cdev_unregister();

	sxe_debugfs_exit();

	if (sxe_workqueue) {
		destroy_workqueue(sxe_workqueue);
		sxe_workqueue = NULL;
	}

	if (sxe_fnav_workqueue) {
		destroy_workqueue(sxe_fnav_workqueue);
		sxe_fnav_workqueue = NULL;
	}

	if (fnav_cache) {
		kmem_cache_destroy(fnav_cache);
		fnav_cache = NULL;
	}

#ifndef SXE_DRIVER_RELEASE
	sxe_log_exit();
#endif 

	return;
}

MODULE_DEVICE_TABLE(pci, sxe_pci_tbl);
MODULE_INFO(build_time, SXE_BUILD_TIME);
MODULE_INFO(branch, SXE_BRANCH);
MODULE_INFO(commit_id, SXE_COMMIT_ID);
MODULE_DESCRIPTION(SXE_DRV_DESCRIPTION);
MODULE_AUTHOR(SXE_DRV_AUTHOR);
MODULE_VERSION(SXE_VERSION);
MODULE_LICENSE(SXE_DRV_LICENSE);

module_init(sxe_init);
module_exit(sxe_exit);
