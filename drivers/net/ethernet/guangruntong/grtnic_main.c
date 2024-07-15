#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/vmalloc.h>
#include <linux/pagemap.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/poll.h>
//#include <linux/version.h>

#include <linux/tcp.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#ifdef NETIF_F_TSO
#include <net/checksum.h>
#endif
#include <linux/ethtool.h>
#include <linux/prefetch.h>

#include "grtnic.h"
#include "grtnic_macphy.h"

#define DEFAULT_ETHER_ADDRESS "\02SUME\00"

MODULE_AUTHOR("Beijing GRT Corporation, <hushunkui@grt-china.com>");
MODULE_DESCRIPTION("GRTNIC Network Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,11,0)
MODULE_SUPPORTED_DEVICE(DRIVER_NAME);
#endif

static const struct grt_gigeth_info *grt_gigeth_info_tbl[] = {
	[board_902E_GRT_FF]		= &grt_902eff_info,
	[board_902T_GRT_FF]		= &grt_902tff_info,
	[board_901ELR_GRT_FF]	= &grt_901elr_info,
	[board_1001E_GRT_FF]	= &grt_1001eff_info,
	[board_1001E_QM_FF]		= &qm_1001eff_info,
	[board_1002E_GRT_FF]	= &grt_1002eff_info,
	[board_1005E_GRT_FX]	= &grt_1005efx_info
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const struct pci_device_id grtnic_pci_tbl[] = {
	{0x1E18, 0x0F82, PCI_ANY_ID, PCI_ANY_ID, 0, 0, board_902E_GRT_FF}, 	
	{0x1E18, 0x0F02, PCI_ANY_ID, PCI_ANY_ID, 0, 0, board_902T_GRT_FF}, 	
	{0x1E18, 0x0F21, PCI_ANY_ID, PCI_ANY_ID, 0, 0, board_901ELR_GRT_FF}, 	
	{0x1E18, 0x1F81, PCI_ANY_ID, PCI_ANY_ID, 0, 0, board_1001E_GRT_FF},
	{0x1E18, 0x1F21, PCI_ANY_ID, PCI_ANY_ID, 0, 0, board_1001E_QM_FF},
	{0x1E18, 0x1F82, PCI_ANY_ID, PCI_ANY_ID, 0, 0, board_1002E_GRT_FF},
	{0x1E18, 0x1F25, PCI_ANY_ID, PCI_ANY_ID, 0, 0, board_1005E_GRT_FX},
	/* required last entry */
	{0 /* end */}
};

MODULE_DEVICE_TABLE(pci, grtnic_pci_tbl);

#define DEFAULT_DEBUG_LEVEL_SHIFT 3

static struct workqueue_struct *grtnic_wq;

#if 0
//#if IS_ENABLED(CONFIG_DCA)
static int grtnic_notify_dca(struct notifier_block *, unsigned long event, void *p);
static struct notifier_block dca_notifier = {
	.notifier_call	= grtnic_notify_dca,
	.next		= NULL,
	.priority	= 0
};
#endif /* CONFIG_DCA */

#if 0
// These are not defined in the 2.x.y kernels, so just define them
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,39)
#define PCI_EXP_DEVCTL2_IDO_REQ_EN 0x100
#define PCI_EXP_DEVCTL2_IDO_CMP_EN 0x200
#else
/** 
 * These are badly named in pre-3.6.11 kernel versions.  We COULD do the same
 * check as above, however (annoyingly) linux for tegra (based on post-3.6.11)
 * picked up the header file from some pre-3.6.11 version, so we'll just make
 * our code ugly and handle the check here:
 */ 
#ifndef PCI_EXP_DEVCTL2_IDO_REQ_EN
#define PCI_EXP_DEVCTL2_IDO_REQ_EN PCI_EXP_IDO_REQ_EN
#endif
#ifndef PCI_EXP_DEVCTL2_IDO_CMP_EN
#define PCI_EXP_DEVCTL2_IDO_CMP_EN PCI_EXP_IDO_CMP_EN
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,5,0)

int pcie_capability_read_dword(struct pci_dev *dev, int pos, u32 *val)
{
	int ret;

	*val = 0;
	if (pos & 3)
		return -EINVAL;

	ret = pci_read_config_dword(dev, pci_pcie_cap(dev) + pos, val);
	/*
	 * Reset *val to 0 if pci_read_config_dword() fails, it may
	 * have been written as 0xFFFFFFFF if hardware error happens
	 * during pci_read_config_dword().
	 */
	if (ret)
		*val = 0;
	return ret;

}

int pcie_capability_write_dword(struct pci_dev *dev, int pos, u32 val)
{
	if (pos & 3)
		return -EINVAL;

	return pci_write_config_dword(dev, pci_pcie_cap(dev) + pos, val);
}

#endif

#endif

static int grtnic_map_bars(struct grtnic_adapter *adapter, struct pci_dev *pdev)
{
	struct device *dev = &pdev->dev;
	struct grtnic_hw *hw = &adapter->hw;

	resource_size_t bar_start;
	resource_size_t bar_end;
	resource_size_t bar_len;

	bar_start = pci_resource_start(pdev, 0);
	bar_end = pci_resource_end(pdev, 0);
	bar_len = bar_end - bar_start + 1;

  hw->user_bar_len = bar_len;
  hw->user_bar = pci_ioremap_bar(pdev, 0);

	if (!hw->user_bar)
	{
		dev_err(dev, "Could not map USER BAR");
		return -1;
	}
	dev_info(dev, "USER BAR mapped at 0x%p with length %llu", hw->user_bar, bar_len);

	bar_start = pci_resource_start(pdev, 1);
	bar_end = pci_resource_end(pdev, 1);
	bar_len = bar_end - bar_start + 1;

  hw->dma_bar_len = bar_len;
  hw->dma_bar = pci_ioremap_bar(pdev, 1);

	if (!hw->dma_bar)
	{
		dev_err(dev, "Could not map DMA BAR");
		return -1;
	}

	dev_info(dev, "DMA BAR mapped at 0x%p with length %llu", hw->dma_bar, bar_len);

    return 0;
}

static void grtnic_free_bars(struct grtnic_adapter *adapter, struct pci_dev *pdev)
{
	struct grtnic_hw *hw = &adapter->hw;

	if(hw->user_bar)
		pci_iounmap(pdev, hw->user_bar);
	if(hw->dma_bar)
		pci_iounmap(pdev, hw->dma_bar);
}


void grtnic_napi_enable_all(struct grtnic_adapter *adapter)
{
	struct grtnic_q_vector *q_vector;
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++) {
		q_vector = adapter->q_vector[q_idx];
#ifdef HAVE_NDO_BUSY_POLL
		grtnic_qv_init_lock(adapter->q_vector[q_idx]);
#endif
		napi_enable(&q_vector->napi);
	}
}

void grtnic_napi_disable_all(struct grtnic_adapter *adapter)
{
	struct grtnic_q_vector *q_vector;
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++) {
		q_vector = adapter->q_vector[q_idx];
		napi_disable(&q_vector->napi);
#ifdef HAVE_NDO_BUSY_POLL
		while(!grtnic_qv_disable(adapter->q_vector[q_idx])) {
			pr_info("QV %d locked\n", q_idx);
			usleep_range(1000, 20000);
		}
#endif
	}
}

/////////////////////////////////////////////////////////////////////////////////////

#if 0
//#if IS_ENABLED(CONFIG_DCA)

static void grtnic_update_tx_dca(struct grtnic_adapter *adapter, struct grtnic_ring *tx_ring, int cpu)
{
	u32 txctrl = 0;
	u16 reg_idx = tx_ring->reg_idx;

	if (adapter->flags & GRTNIC_GRTNIC_FLAG_DCA_ENABLED)
		txctrl = dca3_get_tag(tx_ring->dev, cpu);

	txctrl <<= GRTNIC_DCA_TXCTRL_CPUID_SHIFT;

	/*
	 * We can enable relaxed ordering for reads, but not writes when
	 * DCA is enabled.  This is due to a known issue in some chipsets
	 * which will cause the DCA tag to be cleared.
	 */
	txctrl |= GRTNIC_DCA_TXCTRL_DESC_RRO_EN |
		  GRTNIC_DCA_TXCTRL_DATA_RRO_EN |
		  GRTNIC_DCA_TXCTRL_DESC_DCA_EN;

	write_register(txctrl, adapter->dma_bar+ (TARGET_H2C<<12) + (reg_idx<<8) + ADDR_DCA_RXTXCTL*4);

}

static void grtnic_update_rx_dca(struct grtnic_adapter *adapter, struct grtnic_ring *rx_ring, int cpu)
{
	u32 rxctrl = 0;
	u8 reg_idx = rx_ring->reg_idx;

	if (adapter->flags & GRTNIC_GRTNIC_FLAG_DCA_ENABLED)
		rxctrl = dca3_get_tag(rx_ring->dev, cpu);

	rxctrl <<= GRTNIC_DCA_RXCTRL_CPUID_SHIFT;

	/*
	 * We can enable relaxed ordering for reads, but not writes when
	 * DCA is enabled.  This is due to a known issue in some chipsets
	 * which will cause the DCA tag to be cleared.
	 */
	rxctrl |= GRTNIC_DCA_RXCTRL_DESC_RRO_EN |
		  GRTNIC_DCA_RXCTRL_DATA_DCA_EN |
		  GRTNIC_DCA_RXCTRL_DESC_DCA_EN;

	write_register(rxctrl, adapter->dma_bar+ (TARGET_C2H<<12) + (reg_idx<<8) + ADDR_DCA_RXTXCTL*4);
}

void grtnic_update_dca(struct grtnic_q_vector *q_vector)
{
	struct grtnic_adapter *adapter = q_vector->adapter;
	struct grtnic_ring *ring;
	int cpu = get_cpu();

	if (q_vector->cpu == cpu)
		goto out_no_update;

	grtnic_for_each_ring(ring, q_vector->tx)
		grtnic_update_tx_dca(adapter, ring, cpu);

	grtnic_for_each_ring(ring, q_vector->rx)
		grtnic_update_rx_dca(adapter, ring, cpu);

	q_vector->cpu = cpu;
out_no_update:
	put_cpu();
}

void grtnic_setup_dca(struct grtnic_adapter *adapter)
{
	int v_idx;

	/* always use CB2 mode, difference is masked in the CB driver */
	if (adapter->flags & GRTNIC_FLAG_DCA_ENABLED)
		write_register(GRTNIC_DCA_CTRL_DCA_MODE_CB2, adapter->dma_bar+ (TARGET_CONFIG<<12) + ADDR_DCA_GTCL*4);
	else
		write_register(GRTNIC_DCA_CTRL_DCA_DISABLE,  adapter->dma_bar+ (TARGET_CONFIG<<12) + ADDR_DCA_GTCL*4);

	for (v_idx = 0; v_idx < adapter->num_q_vectors; v_idx++) {
		adapter->q_vector[v_idx]->cpu = -1;
		grtnic_update_dca(adapter->q_vector[v_idx]);
	}
}

static int __grtnic_notify_dca(struct device *dev, void *data)
{
	struct grtnic_adapter *adapter = dev_get_drvdata(dev);
	unsigned long event = *(unsigned long *)data;

	if (!(adapter->flags & GRTNIC_FLAG_DCA_CAPABLE))
		return 0;

	switch (event) {
	case DCA_PROVIDER_ADD:
		/* if we're already enabled, don't do it again */
		if (adapter->flags & GRTNIC_FLAG_DCA_ENABLED)
			break;
		if (dca_add_requester(dev) == 0) {
			adapter->flags |= GRTNIC_FLAG_DCA_ENABLED;
			write_register(GRTNIC_DCA_CTRL_DCA_MODE_CB2, adapter->dma_bar+ (TARGET_CONFIG<<12) + ADDR_DCA_GTCL*4);
			break;
		}
		/* fall through - DCA is disabled */
	case DCA_PROVIDER_REMOVE:
		if (adapter->flags & GRTNIC_FLAG_DCA_ENABLED) {
			dca_remove_requester(dev);
			adapter->flags &= ~GRTNIC_FLAG_DCA_ENABLED;
			write_register(GRTNIC_DCA_CTRL_DCA_DISABLE,  adapter->dma_bar+ (TARGET_CONFIG<<12) + ADDR_DCA_GTCL*4);
		}
		break;
	}

	return 0;
}
#endif /* CONFIG_DCA */


/**
 * grtnic_rss_indir_tbl_entries - Return RSS indirection table entries
 * @adapter: device handle
 *
 *  - 82598/82599/X540:     128
 *  - X550(non-SRIOV mode): 512
 *  - X550(SRIOV mode):     64
 */
u32 grtnic_rss_indir_tbl_entries(struct grtnic_adapter *adapter)
{
		return 128;
}

/**
 * grtnic_store_key - Write the RSS key to HW
 * @adapter: device handle
 *
 * Write the RSS key stored in adapter.rss_key to HW.
 */
void grtnic_store_key(struct grtnic_adapter *adapter)
{
	struct grtnic_hw *hw = &adapter->hw;
	int i;

	for (i = 0; i < 10; i++)
		GRTNIC_WRITE_REG(hw, (RSS_KEY_BEGIN + i*4), adapter->rss_key[i], 0);
}

/**
 * grtnic_init_rss_key - Initialize adapter RSS key
 * @adapter: device handle
 *
 * Allocates and initializes the RSS key if it is not allocated.
 **/
static inline int grtnic_init_rss_key(struct grtnic_adapter *adapter)
{

//	static const u32 rsskey[10] = { 0xDA565A6D, 0xC20E5B25, 0x3D256741,
//					0xB08FA343, 0xCB2BCAD0, 0xB4307BAE,
//					0xA32DCB77, 0x0CF23080, 0x3BB7426A,
//					0xFA01ACBE };
	u32 *rss_key;

	if (!adapter->rss_key) {
		rss_key = kzalloc(GRTNIC_RSS_KEY_SIZE, GFP_KERNEL);
		if (unlikely(!rss_key))
			return -ENOMEM;

		netdev_rss_key_fill(rss_key, GRTNIC_RSS_KEY_SIZE);
		adapter->rss_key = rss_key;
	}

	return 0;
}

/**
 * grtnic_store_reta - Write the RETA table to HW
 * @adapter: device handle
 *
 * Write the RSS redirection table stored in adapter.rss_indir_tbl[] to HW.
 */
void grtnic_store_reta(struct grtnic_adapter *adapter)
{
	u32 i = 0, reta_entries = grtnic_rss_indir_tbl_entries(adapter);
	struct grtnic_hw *hw = &adapter->hw;
	u8 *indir_tbl = adapter->rss_indir_tbl;

	/* Write redirection table to HW */
	while (i < reta_entries) {
		u32 val = 0;
		int j;

		for (j = 3; j >= 0; j--) {
			val <<= 8;
			val |= indir_tbl[i + j];
		}

		GRTNIC_WRITE_REG(hw, (RSS_RETA_BEGIN + i), val, 0);
		i += 4;

	}
}

static void grtnic_setup_reta(struct grtnic_adapter *adapter)
{
	u32 i, j;
	u32 reta_entries = grtnic_rss_indir_tbl_entries(adapter);
//	u16 rss_i = adapter->ring_feature[RING_F_RSS].indices;
	u16 rss_i = adapter->rss_queues;

	/* Fill out hash function seeds */
	grtnic_store_key(adapter);

	/* Fill out redirection table */
	memset(adapter->rss_indir_tbl, 0, sizeof(adapter->rss_indir_tbl));

	for (i = 0, j = 0; i < reta_entries; i++, j++) {
		if (j == rss_i)
			j = 0;

		adapter->rss_indir_tbl[i] = j;
	}

	grtnic_store_reta(adapter);
}

/**
 *  grtnic_setup_mrqc - configure the multiple receive queue control registers
 *  @adapter: Board private structure
 **/

void grtnic_setup_mrqc(struct grtnic_adapter *adapter)
{
//	u32 mrqc = 0, rss_field = 0; //暂时没用到，这个地方可以设置RSS的方式
	grtnic_setup_reta(adapter);
//	mrqc |= rss_field;
//	IXGBE_WRITE_REG(hw, IXGBE_MRQC, mrqc);
}


////////////////////////////////////////////////////////////////////////////////////////////
void grtnic_irq_disable(struct grtnic_adapter *adapter)
{
	u32 var;
	if (adapter->flags & GRTNIC_FLAG_MSIX_ENABLED)
		var = adapter->eims_enable_mask;
	else
		var = ~0;

	GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_IMC*4), var, 1);
	GRTNIC_WRITE_FLUSH(&adapter->hw); //flush

	if (adapter->flags & GRTNIC_FLAG_MSIX_ENABLED) {
		int vector;
		for (vector = 0; vector < adapter->num_q_vectors; vector++)
			synchronize_irq(adapter->msix_entries[vector].vector);

		synchronize_irq(adapter->msix_entries[vector++].vector); //other 

	} else {
		synchronize_irq(adapter->pdev->irq);
	}

}

void grtnic_irq_enable(struct grtnic_adapter *adapter)
{
	u32 var;

	if (adapter->flags & GRTNIC_FLAG_MSIX_ENABLED)
		var = adapter->eims_enable_mask;
	else
		var = ~0;

	GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_IAM*4), var, 1); //当发出中断后自动禁止所有中断
	GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_IMS*4), var, 1); //enable all interrupt
	GRTNIC_WRITE_FLUSH(&adapter->hw); //flush
}


void grtnic_free_irq(struct grtnic_adapter *adapter)
{
	if (adapter->flags & GRTNIC_FLAG_MSIX_ENABLED) {
		int vector = 0, i;

		for (i = 0; i < adapter->num_q_vectors; i++)
		{
#ifdef HAVE_IRQ_AFFINITY_HINT
			/* clear the affinity_mask in the IRQ descriptor */
			irq_set_affinity_hint(adapter->msix_entries[vector].vector, NULL);
#endif
			free_irq(adapter->msix_entries[vector++].vector, adapter->q_vector[i]);
		}

		free_irq(adapter->msix_entries[vector++].vector, adapter); //other

	} else {
		free_irq(adapter->pdev->irq, adapter);
	}
}

#define N0_QUEUE -1
static void grtnic_assign_vector(struct grtnic_q_vector *q_vector, int msix_vector)
{
	struct grtnic_adapter *adapter = q_vector->adapter;
	int rx_queue = N0_QUEUE;
	int tx_queue = N0_QUEUE;
	u8 ivar;

	if (q_vector->rx.ring)
		rx_queue = q_vector->rx.ring->reg_idx;
	if (q_vector->tx.ring)
		tx_queue = q_vector->tx.ring->reg_idx;

	if (rx_queue > N0_QUEUE)
	{
		GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_MODE*4), ((rx_queue*2+1)<<16 | 0x01), 1); //1: c2s eop interrupt mode
		ivar = adapter->ivar[rx_queue];
		/* clear any bits that are currently set */
		ivar &= 0x0F;
		ivar |= (msix_vector <<4);
		adapter->ivar[rx_queue] = ivar;
		GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADD_INTR_IVAR*4), (rx_queue<<16 | ivar), 1);
	}

	if (tx_queue > N0_QUEUE)
	{
		GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_MODE*4), ((tx_queue*2)<<16 | 0x00), 1); //0:s2c normal interrupt 1: no desc wb & no interrupt
		ivar = adapter->ivar[tx_queue];
		/* clear any bits that are currently set */
		ivar &= 0xF0;
		ivar |= msix_vector;
		adapter->ivar[tx_queue] = ivar;
		GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADD_INTR_IVAR*4), (tx_queue<<16 | ivar), 1);
	}

	q_vector->eims_value = BIT(msix_vector);

	/* add q_vector eims value to global eims_enable_mask */
	adapter->eims_enable_mask |= q_vector->eims_value;

}


/**
 * ixgbe_configure_msi_and_legacy - Initialize PIN (INTA...) and MSI interrupts
 * @adapter: board private structure
 *
 **/
void grtnic_configure_msi_and_legacy(struct grtnic_adapter *adapter)
{
	struct grtnic_q_vector *q_vector = adapter->q_vector[0];

	grtnic_write_itr(q_vector);

	GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_MODE*4), ((0*2+1)<<16 | 0x01), 1); //1: c2s eop interrupt mode
	GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_MODE*4), ((0*2)<<16 	 | 0x00), 1); //0:s2c normal interrupt 1: no desc wb & no interrupt

	adapter->eims_other = BIT(0);

	GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADD_INTR_IVAR_MISC*4), 0, 1);
	GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADD_INTR_IVAR*4), (0<<16 | 0x11), 1);
}

/**
 *  grtnic_configure_msix - Configure MSI-X hardware
 *  @adapter: board private structure
 *
 *  grtnic_configure_msix sets up the hardware to properly
 *  generate MSI-X interrupts.
 **/
void grtnic_configure_msix(struct grtnic_adapter *adapter)
{
	int i, vector = 0;

	adapter->eims_enable_mask = 0;

	for (i = 0; i < adapter->num_q_vectors; i++)
	{
		grtnic_assign_vector(adapter->q_vector[i], vector++);
		grtnic_write_itr(adapter->q_vector[i]);
	}

	/* enable msix_other interrupt */
	adapter->eims_other = BIT(vector);

	GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADD_INTR_IVAR_MISC*4), vector, 1);

	adapter->eims_enable_mask |= adapter->eims_other;

	GRTNIC_WRITE_FLUSH(&adapter->hw); //flush
}

/**
 *  grtnic_request_msix - Initialize MSI-X interrupts
 *  @adapter: board private structure
 *
 *  grtnic_request_msix allocates MSI-X vectors and requests interrupts from the
 *  kernel.
 **/
static int grtnic_request_msix(struct grtnic_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	unsigned int ri = 0, ti = 0;
	int vector, err;

	for (vector = 0; vector < adapter->num_q_vectors; vector++) {
		struct grtnic_q_vector *q_vector = adapter->q_vector[vector];
		struct msix_entry *entry = &adapter->msix_entries[vector];

		if (q_vector->tx.ring && q_vector->rx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name),
				 "%s-TxRx-%u", netdev->name, ri++);
			ti++;
		} else if (q_vector->rx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name),
				 "%s-rx-%u", netdev->name, ri++);
		} else if (q_vector->tx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name),
				 "%s-tx-%u", netdev->name, ti++);
		} else {
			/* skip this unused q_vector */
			continue;
		}

		err = request_irq(entry->vector, &grtnic_msix_ring, 0,
				  q_vector->name, q_vector);
		if (err) {
			e_err(probe, "request_irq failed for MSIX interrupt '%s' "
			      "Error: %d\n", q_vector->name, err);
			goto free_queue_irqs;
		}

#ifdef HAVE_IRQ_AFFINITY_HINT
		/* If Flow Director is enabled, set interrupt affinity */
//		if (adapter->flags & IXGBE_FLAG_FDIR_HASH_CAPABLE) {
			/* assign the mask for this irq */
			irq_set_affinity_hint(adapter->msix_entries[vector].vector, &q_vector->affinity_mask);
//		}
#endif /* HAVE_IRQ_AFFINITY_HINT */
	}

	err = request_irq(adapter->msix_entries[vector].vector, &grtnic_msix_other, 0, netdev->name, adapter);
	if (err) {
		e_err(probe, "request_irq for msix_other failed: %d\n", err);
		goto free_queue_irqs;
	}

	return GRTNIC_SUCCESS;

free_queue_irqs:
	while (vector) {
		vector--;
#ifdef HAVE_IRQ_AFFINITY_HINT
		irq_set_affinity_hint(adapter->msix_entries[vector].vector, NULL);
#endif
		free_irq(adapter->msix_entries[vector].vector,
			 adapter->q_vector[vector]);
	}
	adapter->flags &= ~GRTNIC_FLAG_MSIX_ENABLED;
	pci_disable_msix(adapter->pdev);
	kfree(adapter->msix_entries);
	adapter->msix_entries = NULL;
	return err;
}


int grtnic_request_irq(struct grtnic_adapter *adapter)
{
	int irq_flag;
	struct pci_dev *pdev = adapter->pdev;
	int err = 0;

	if (adapter->flags & GRTNIC_FLAG_MSIX_ENABLED)
		err = grtnic_request_msix(adapter);

	else
	{
		irq_flag = (adapter->flags & GRTNIC_FLAG_MSI_ENABLED) ? 0 : IRQF_SHARED;
		err = request_irq(pdev->irq, grtnic_isr, irq_flag, DRIVER_NAME, adapter);
	}

	if (err)
		e_err(probe, "request_irq failed, Error %d\n", err);

	return err;
}

void grtnic_reset_interrupt_capability(struct grtnic_adapter *adapter)
{
	if (adapter->flags & GRTNIC_FLAG_MSIX_ENABLED) {
		adapter->flags &= ~GRTNIC_FLAG_MSIX_ENABLED;
		pci_disable_msix(adapter->pdev);
		kfree(adapter->msix_entries);
		adapter->msix_entries = NULL;
	} else if (adapter->flags & GRTNIC_FLAG_MSI_ENABLED) {
		adapter->flags &= ~GRTNIC_FLAG_MSI_ENABLED;
		pci_disable_msi(adapter->pdev);
	}
}


/**
 * grtnic_set_rss_queues: Allocate queues for RSS
 * @adapter: board private structure to initialize
 *
 * This is our "base" multiqueue mode.  RSS (Receive Side Scaling) will try
 * to allocate one Rx queue per CPU, and if available, one Tx queue per CPU.
 *
 **/
static bool grtnic_set_rss_queues(struct grtnic_adapter *adapter)
{
	u16 rss_i = adapter->rss_queues;

	adapter->num_rx_queues = rss_i;
#ifdef HAVE_TX_MQ
	adapter->num_tx_queues = rss_i;
#endif

	return true;
}


/*
 * grtnic_set_num_queues: Allocate queues for device, feature dependent
 * @adapter: board private structure to initialize
 *
 * This is the top level queue allocation routine.  The order here is very
 * important, starting with the "most" number of features turned on at once,
 * and ending with the smallest set of features.  This way large combinations
 * can be allocated if they're turned on, and smaller combinations are the
 * fallthrough conditions.
 *
 **/
static void grtnic_set_num_queues(struct grtnic_adapter *adapter)
{
	/* Start with base case */
	adapter->num_rx_queues = 1;
	adapter->num_tx_queues = 1;

	grtnic_set_rss_queues(adapter);
}

/**
 * grtnic_acquire_msix_vectors - acquire MSI-X vectors
 * @adapter: board private structure
 *
 * Attempts to acquire a suitable range of MSI-X vector interrupts. Will
 * return a negative error code if unable to acquire MSI-X vectors for any
 * reason.
 */
static int grtnic_acquire_msix_vectors(struct grtnic_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pdev;
	int i, vectors;

	if (!(adapter->flags & GRTNIC_FLAG_MSIX_CAPABLE))
		return -EOPNOTSUPP;

	/* We start by asking for one vector per queue pair with XDP queues
	 * being stacked with TX queues.
	 */
	vectors = max(adapter->num_rx_queues, adapter->num_tx_queues);
	/* if tx handler is separate make it 1 for every queue */
//	if (!(adapter->flags & FLAG_QUEUE_PAIRS))
//		vectors = adapter->num_tx_queues + adapter->num_rx_queues;

	/* store the number of vectors reserved for queues */
	adapter->num_q_vectors = vectors;

	/* add 1 vector for link status interrupts */
	vectors++;

	adapter->msix_entries = kcalloc(vectors, sizeof(struct msix_entry), GFP_KERNEL);
	if (!adapter->msix_entries)
		return -ENOMEM;

	for (i = 0; i < vectors; i++)
		adapter->msix_entries[i].entry = i;

	vectors = pci_enable_msix_range(pdev, adapter->msix_entries, vectors, vectors);

	if (vectors < 0) {
		/* A negative count of allocated vectors indicates an error in
		 * acquiring within the specified range of MSI-X vectors */
		e_dev_warn("Failed to allocate MSI-X interrupts. Err: %d\n",
			   vectors);

		adapter->flags &= ~GRTNIC_FLAG_MSIX_ENABLED;
		kfree(adapter->msix_entries);
		adapter->msix_entries = NULL;

		return vectors;
	}

	/* we successfully allocated some number of vectors within our
	 * requested range.
	 */
	adapter->flags |= GRTNIC_FLAG_MSIX_ENABLED;
	return 0;
}


void grtnic_set_interrupt_capability(struct grtnic_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pdev;
	int err, i;

	for(i=0; i<MAX_Q_VECTORS; i++)
		adapter->ivar[i] = 0;

	/* We will try to get MSI-X interrupts first */
	if (!grtnic_acquire_msix_vectors(adapter))
		return;

	/* At this point, we do not have MSI-X capabilities. We need to
	 * reconfigure or disable various features which require MSI-X
	 * capability.
	 */
	/* Disable RSS */
	e_dev_warn("Disabling RSS support\n");
	adapter->rss_queues = 1;

	/* recalculate number of queues now that many features have been
	 * changed or disabled.
	 */
	grtnic_set_num_queues(adapter);
	adapter->num_q_vectors = 1;

	if (!(adapter->flags & GRTNIC_FLAG_MSI_CAPABLE))
		return;

	err = pci_enable_msi(pdev);
	if (err)
		e_dev_warn("Failed to allocate MSI interrupt, falling back to legacy. Error: %d\n",
			   err);
	else
		adapter->flags |= GRTNIC_FLAG_MSI_ENABLED;
}

/**
 *  grtnic_free_q_vector - Free memory allocated for specific interrupt vector
 *  @adapter: board private structure to initialize
 *  @v_idx: Index of vector to be freed
 *
 *  This function frees the memory allocated to the q_vector.
 **/
static void grtnic_free_q_vector(struct grtnic_adapter *adapter, int v_idx)
{
	struct grtnic_q_vector *q_vector = adapter->q_vector[v_idx];
	/* if we're coming from grtnic_set_interrupt_capability, the vectors are
	 * not yet allocated
	 */
	if (!q_vector)
		return;

	if (q_vector->tx.ring)
		adapter->tx_ring[q_vector->tx.ring->queue_index] = NULL;

	if (q_vector->rx.ring)
		adapter->rx_ring[q_vector->rx.ring->queue_index] = NULL;

	adapter->q_vector[v_idx] = NULL;

#ifdef HAVE_NDO_BUSY_POLL
	napi_hash_del(&q_vector->napi);
#endif
	netif_napi_del(&q_vector->napi);
	kfree_rcu(q_vector, rcu);
}

static void grtnic_add_ring(struct grtnic_ring *ring, struct grtnic_ring_container *head)
{
	ring->next = head->ring;
	head->ring = ring;
	head->count++;
}

/**
 *  grtnic_alloc_q_vector - Allocate memory for a single interrupt vector
 *  @adapter: board private structure to initialize
 *  @v_count: q_vectors allocated on adapter, used for ring interleaving
 *  @v_idx: index of vector in adapter struct
 *  @txr_count: total number of Tx rings to allocate
 *  @txr_idx: index of first Tx ring to allocate
 *  @rxr_count: total number of Rx rings to allocate
 *  @rxr_idx: index of first Rx ring to allocate
 *
 *  We allocate one q_vector.  If allocation fails we return -ENOMEM.
 **/
static int grtnic_alloc_q_vector(struct grtnic_adapter *adapter,
			      unsigned int v_count, unsigned int v_idx,
			      unsigned int txr_count, unsigned int txr_idx,
			      unsigned int rxr_count, unsigned int rxr_idx)
{
	struct grtnic_q_vector *q_vector;
	struct grtnic_ring *ring;
	int node = -1;
#ifdef HAVE_IRQ_AFFINITY_HINT
	int cpu = -1;
	u8 tcs = 0;
//	u8 tcs = netdev_get_num_tc(adapter->netdev);
#endif
	int ring_count,size;

	/* only supports 1 Tx and/or 1 Rx queue per vector */
	if (txr_count > 1 || rxr_count > 1)
		return -ENOMEM;

	ring_count = txr_count + rxr_count;
	size = sizeof(struct grtnic_q_vector) +
	       (sizeof(struct grtnic_ring) * ring_count);

#ifdef HAVE_IRQ_AFFINITY_HINT
	/* customize cpu for Flow Director mapping */
	if (tcs <= 1) {
			if (cpu_online(v_idx)) {
				cpu = v_idx;
				node = cpu_to_node(cpu);
			}
	}

#endif

	/* allocate q_vector and rings */
	q_vector = kzalloc_node(size, GFP_KERNEL, node);

	if (!q_vector)
		q_vector = kzalloc(size, GFP_KERNEL);
	if (!q_vector)
		return -ENOMEM;

	/* setup affinity mask and node */
#ifdef HAVE_IRQ_AFFINITY_HINT
	if (cpu != -1)
		cpumask_set_cpu(cpu, &q_vector->affinity_mask);
#endif
	q_vector->node = node;

	/* initialize CPU for DCA */
	q_vector->cpu = -1;

	/* initialize NAPI */
//	netif_napi_add(adapter->netdev, &q_vector->napi, grtnic_poll, 64);
	netif_napi_add(adapter->netdev, &q_vector->napi, grtnic_poll);

#ifndef HAVE_NETIF_NAPI_ADD_CALLS_NAPI_HASH_ADD
#ifdef HAVE_NDO_BUSY_POLL
	napi_hash_add(&q_vector->napi);
#endif
#endif

#ifdef HAVE_NDO_BUSY_POLL
	/* initialize busy poll */
	atomic_set(&q_vector->state, GRTNIC_QV_STATE_DISABLE);

#endif

	/* tie q_vector and adapter together */
	adapter->q_vector[v_idx] = q_vector;
	q_vector->adapter = adapter;
	q_vector->v_idx = v_idx;

	/* initialize work limits */
	q_vector->tx.work_limit = adapter->tx_work_limit;

	/* Initialize setting for adaptive ITR */
	q_vector->tx.itr = ITR_ADAPTIVE_MAX_USECS | ITR_ADAPTIVE_LATENCY;
	q_vector->rx.itr = ITR_ADAPTIVE_MAX_USECS | ITR_ADAPTIVE_LATENCY;

	/* intialize ITR */
	if (txr_count && !rxr_count) {
		/* tx only vector */
		if (adapter->tx_itr_setting == 1)
			q_vector->itr = GRTNIC_12K_ITR;
		else
			q_vector->itr = adapter->tx_itr_setting;
	} else {
		/* rx or rx/tx vector */
		if (adapter->rx_itr_setting == 1)
			q_vector->itr = GRTNIC_20K_ITR;
		else
			q_vector->itr = adapter->rx_itr_setting;
	}

	/* initialize pointer to rings */
	ring = q_vector->ring;

	if (txr_count) {
		/* assign generic ring traits */
		ring->dev = adapter->dev;
		ring->netdev = adapter->netdev;

		/* configure backlink on ring */
		ring->q_vector = q_vector;

		/* update q_vector Tx values */
		grtnic_add_ring(ring, &q_vector->tx);

		/* apply Tx specific ring traits */
		ring->count = adapter->tx_ring_count;
		ring->queue_index = txr_idx;

		/* assign ring to adapter */
		adapter->tx_ring[txr_idx] = ring;

		/* push pointer to next ring */
		ring++;
	}

	if (rxr_count) {
		/* assign generic ring traits */
		ring->dev = adapter->dev;
		ring->netdev = adapter->netdev;

		/* configure backlink on ring */
		ring->q_vector = q_vector;

		/* update q_vector Rx values */
		grtnic_add_ring(ring, &q_vector->rx);

		/* apply Rx specific ring traits */
		ring->count = adapter->rx_ring_count;
		ring->queue_index = rxr_idx;

		/* assign ring to adapter */
		adapter->rx_ring[rxr_idx] = ring;
	}

	return 0;
}

/**
 *  grtnic_free_q_vectors - Free memory allocated for interrupt vectors
 *  @adapter: board private structure to initialize
 *
 *  This function frees the memory allocated to the q_vectors.  In addition if
 *  NAPI is enabled it will delete any references to the NAPI struct prior
 *  to freeing the q_vector.
 **/
static void grtnic_free_q_vectors(struct grtnic_adapter *adapter)
{
	int v_idx = adapter->num_q_vectors;

	adapter->num_tx_queues = 0;
	adapter->num_rx_queues = 0;
	adapter->num_q_vectors = 0;

	while (v_idx--)
		grtnic_free_q_vector(adapter, v_idx);
}

/**
 *  grtnic_alloc_q_vectors - Allocate memory for interrupt vectors
 *  @adapter: board private structure to initialize
 *
 *  We allocate one q_vector per queue interrupt.  If allocation fails we
 *  return -ENOMEM.
 **/
static int grtnic_alloc_q_vectors(struct grtnic_adapter *adapter)
{
	int q_vectors = adapter->num_q_vectors;
	int rxr_remaining = adapter->num_rx_queues;
	int txr_remaining = adapter->num_tx_queues;
	int rxr_idx = 0, txr_idx = 0, v_idx = 0;
	int i;
	int err;

	if (q_vectors >= (rxr_remaining + txr_remaining)) {
		for (; rxr_remaining; v_idx++) {
			err = grtnic_alloc_q_vector(adapter, q_vectors, v_idx,
						 0, 0, 1, rxr_idx);

			if (err)
				goto err_out;

			/* update counts and index */
			rxr_remaining--;
			rxr_idx++;
		}
	}

	for (; v_idx < q_vectors; v_idx++) {
		int rqpv = DIV_ROUND_UP(rxr_remaining, q_vectors - v_idx);
		int tqpv = DIV_ROUND_UP(txr_remaining, q_vectors - v_idx);

		err = grtnic_alloc_q_vector(adapter, q_vectors, v_idx,
					 tqpv, txr_idx, rqpv, rxr_idx);

		if (err)
			goto err_out;

		/* update counts and index */
		rxr_remaining -= rqpv;
		txr_remaining -= tqpv;
		rxr_idx++;
		txr_idx++;
	}

	for (i = 0; i < adapter->num_rx_queues; i++) {
		if (adapter->rx_ring[i])
			adapter->rx_ring[i]->reg_idx = i;
	}

	for (i = 0; i < adapter->num_tx_queues; i++) {
		if (adapter->tx_ring[i])
			adapter->tx_ring[i]->reg_idx = i;
	}

	return 0;

err_out:
	adapter->num_tx_queues = 0;
	adapter->num_rx_queues = 0;
	adapter->num_q_vectors = 0;

	while (v_idx--)
		grtnic_free_q_vector(adapter, v_idx);

	return -ENOMEM;
}

/**
 *  grtnic_clear_interrupt_scheme - reset the device to a state of no interrupts
 *  @adapter: board private structure
 *
 *  This function resets the device so that it has 0 rx queues, tx queues, and
 *  MSI-X interrupts allocated.
 */
void grtnic_clear_interrupt_scheme(struct grtnic_adapter *adapter)
{
	adapter->num_tx_queues = 0;
	adapter->num_rx_queues = 0;

	grtnic_free_q_vectors(adapter);
	grtnic_reset_interrupt_capability(adapter);
}

/**
 * grtnic_init_interrupt_scheme - Determine proper interrupt scheme
 * @adapter: board private structure to initialize
 *
 * We determine which interrupt scheme to use based on...
 * - Kernel support (MSI, MSI-X)
 *   - which can be user-defined (via MODULE_PARAM)
 * - Hardware queue count (num_*_queues)
 *   - defined by miscellaneous hardware support/features (RSS, etc.)
 **/
int grtnic_init_interrupt_scheme(struct grtnic_adapter *adapter)
{
	int err;

	/* Number of supported queues */
	grtnic_set_num_queues(adapter);

	/* Set interrupt mode */
	grtnic_set_interrupt_capability(adapter);

	/* Allocate memory for queues */
	err = grtnic_alloc_q_vectors(adapter);

	if (err) {
		e_err(probe, "Unable to allocate memory for queue vectors\n");
		grtnic_reset_interrupt_capability(adapter);
		return err;
	}

//	ixgbe_cache_ring_register(adapter);

	set_bit(__GRTNIC_DOWN, &adapter->state);

	return GRTNIC_SUCCESS;
}


static int grtnic_sw_init(struct grtnic_adapter *adapter)
{
	int i, err = 0;
	struct grtnic_ring *tx_ring, *rx_ring;
	int card_type = adapter->ei->type;

	if (grtnic_init_rss_key(adapter)) {
		err = GRTNIC_ERR_OUT_OF_MEM;
		e_err(probe, "rss_key allocation failed: %d\n", err);
		goto out;
	}

//针对每个卡的phy设置，可以放在这里
	#if IS_ENABLED(CONFIG_DCA)
		adapter->flags |= GRTNIC_FLAG_DCA_CAPABLE;
	#endif
	adapter->flags |= (GRTNIC_FLAG_MSI_CAPABLE |	\
					   GRTNIC_FLAG_MSIX_CAPABLE |	\
					   GRTNIC_FLAG_MQ_CAPABLE);

//	/* default flow control settings */
//	hw->fc.requested_mode = ixgbe_fc_full;
//	hw->fc.current_mode = ixgbe_fc_full;	/* init for ethtool output */
//
//	adapter->last_lfc_mode = hw->fc.current_mode;
//	ixgbe_pbthresh_setup(adapter);
//	hw->fc.pause_time = IXGBE_DEFAULT_FCPAUSE;
//	hw->fc.send_xon = true;
//	hw->fc.disable_fc_autoneg = false;

	/* set default ring sizes */
	adapter->tx_ring_count = GRTNIC_DEFAULT_TXD;
	adapter->rx_ring_count = GRTNIC_DEFAULT_RXD;

	/* set default work limits */
	adapter->tx_work_limit = GRTNIC_DEFAULT_TX_WORK;

	adapter->max_frame_size = adapter->netdev->mtu + ETH_HLEN + ETH_FCS_LEN; //1500+18+4
	adapter->min_frame_size = ETH_ZLEN + ETH_FCS_LEN; //60+4

	grtnic_PhySetMdioDivisor(adapter->netdev, 24);

	if(card_type == board_902T_GRT_FF) //FF902T
	{
		u16 temp;
		u32 phy_addr = adapter->hw.phy_addr;

		grtnic_PhyRead(adapter->netdev, phy_addr, 0x00, &temp);
		grtnic_PhyWrite(adapter->netdev, phy_addr, 0x00, temp | PHY_RESET); //rst phy

		//clear EEE LED
		grtnic_PhyWrite(adapter->netdev, phy_addr, 0x1F, 0xd04); //1F:change page, d04:ExtPage 
		grtnic_PhyWrite(adapter->netdev, phy_addr, 0x11, 0x00); //EEELCR 
		//		  grtnic_PhyWrite(adapter->netdev[i], 0x01, 0x10, 0x207B); // LED config
		grtnic_PhyWrite(adapter->netdev, phy_addr, 0x10, 0x0D1B); // LED config

		grtnic_PhyWrite(adapter->netdev, phy_addr, 0x1F, 0x00); //page 0 
		grtnic_PhyWrite(adapter->netdev, phy_addr, 0x0D, 0x07); //1 is phy add, 0d is MACR reg, 7 is device address 
		grtnic_PhyWrite(adapter->netdev, phy_addr, 0x0E, 0x3C); //1 is phy add, 0e is MAADR reg, 0x3C is reg address 
		grtnic_PhyWrite(adapter->netdev, phy_addr, 0x0D, 0x4007); //1 is phy add, 0d is MACR reg, 4007 get data from device add 7 & reg 14 
		grtnic_PhyWrite(adapter->netdev, phy_addr, 0x0E, 0x00); // EEEAR
	}

//	else if(card_type == board_904T_GRT_FF || card_type == board_904E_GRT_FF)//FF904T & FF904E
//	{
//		u16 temp;
//		for (i = 0; i < grtnic_ports_max; i++)
//		{
//		  grtnic_PhyWrite(adapter->netdev[0], i, 0x1E, 0x00); //utp_ext_reg
//		  grtnic_PhyRead (adapter->netdev[0], i, 0x1F, &temp);
//
//		  grtnic_PhyWrite(adapter->netdev[0], i, 0x1E, 0x00);
//		  grtnic_PhyWrite(adapter->netdev[0], i, 0x1F, temp | 0x20); //jumbo enable
//
//		  grtnic_PhyRead (adapter->netdev[0], i, 0x00, &temp);
//		  grtnic_PhyWrite(adapter->netdev[0], i, 0x00, temp | 0x8000); //sw reset
//		}
//
//	}

	for (i = 0; i < adapter->num_tx_queues; i++)
	{

		tx_ring = adapter->tx_ring[i];
		memset(&tx_ring->stats, 0, sizeof(tx_ring->stats));
		memset(&tx_ring->tx_stats, 0, sizeof(tx_ring->tx_stats));
  }

//-----------------------------------------------------------------------------------
	for (i = 0; i < adapter->num_rx_queues; i++)
	{
		rx_ring = adapter->rx_ring[i];

		memset(&rx_ring->stats, 0, sizeof(rx_ring->stats));
		memset(&rx_ring->rx_stats, 0, sizeof(rx_ring->rx_stats));
	}

	set_bit(__GRTNIC_DOWN, &adapter->state);

out:
	return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * grtnic_watchdog_update_link - update the link status
 * @adapter: pointer to the device adapter structure
 **/
static void grtnic_watchdog_update_link(struct grtnic_adapter *adapter)
{
	struct grtnic_hw *hw = &adapter->hw;
	u32 link_speed = adapter->link_speed;
	bool link_up = adapter->link_up;
	u32 xphy_status;

	if (!(adapter->flags & GRTNIC_FLAG_NEED_LINK_UPDATE))
		return;

  xphy_status = GRTNIC_READ_REG(hw, XPHY_STATUS, 0);
  link_up = (xphy_status & 1) ? 1:0;
  link_speed = (xphy_status >> 1) & 0x03;

//	if (link_up) {
//		if (hw->phy.media_type == ixgbe_media_type_copper &&
//		    (ixgbe_device_supports_autoneg_fc(hw)))
//			ixgbe_setup_fc(hw);
//		hw->mac.ops.fc_enable(hw);
//
//	}

	if (link_up || time_after(jiffies, (adapter->link_check_timeout + GRTNIC_TRY_LINK_TIMEOUT))) {
		adapter->flags &= ~GRTNIC_FLAG_NEED_LINK_UPDATE;
		GRTNIC_WRITE_REG(hw, ((TARGET_IRQ<<12) + ADDR_INTR_IMS*4), adapter->eims_other, 1); //打开相应的中断,user_interrupt
		GRTNIC_WRITE_FLUSH(hw);
	}

	adapter->link_up = link_up;
	adapter->link_speed = link_speed;
}

/**
 * grtnic_watchdog_link_is_up - update netif_carrier status and
 *                             print link up message
 * @adapter: pointer to the device adapter structure
 **/
static void grtnic_watchdog_link_is_up(struct grtnic_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	u32 link_speed = adapter->link_speed;

	/* only continue if link was previously down */
	if (netif_carrier_ok(netdev))
		return;

	if(adapter->type==1) //copper rtl8211
  {
		grtnic_ResetRx(netdev);
    grtnic_SetSpeed(netdev, link_speed);
  }
  grtnic_SetRx(netdev, 1);   //start rx

	e_info(drv, "NIC Link is Up\n");
	netif_carrier_on(netdev);
	netif_tx_wake_all_queues(netdev);
}

/**
 * grtnic_watchdog_link_is_down - update netif_carrier status and
 *                               print link down message
 * @adapter: pointer to the adapter structure
 **/
static void grtnic_watchdog_link_is_down(struct grtnic_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	adapter->link_up = false;
	adapter->link_speed = 0;

	/* only continue if link was up previously */
	if (!netif_carrier_ok(netdev))
		return;

  grtnic_SetRx(netdev, 0);   //stop rx

	e_info(drv, "NIC Link is Down\n");
	netif_carrier_off(netdev);
	netif_tx_stop_all_queues(netdev);
}

static bool grtnic_ring_tx_pending(struct grtnic_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct grtnic_ring *tx_ring = adapter->tx_ring[i];

		if (tx_ring->next_to_use != tx_ring->next_to_clean)
			return true;
	}

	return false;
}

/**
 * grtnic_watchdog_flush_tx - flush queues on link down
 * @adapter: pointer to the device adapter structure
 **/
static void grtnic_watchdog_flush_tx(struct grtnic_adapter *adapter)
{
	if (!netif_carrier_ok(adapter->netdev)) {
		if (grtnic_ring_tx_pending(adapter)) {
			/* We've lost link, so the controller stops DMA,
			 * but we've got queued Tx work that's never going
			 * to get done, so reset controller to flush Tx.
			 * (Do the reset outside of interrupt context).
			 */
			e_warn(drv, "initiating reset due to lost link with pending Tx work\n");
			set_bit(__GRTNIC_RESET_REQUESTED, &adapter->state);
		}
	}
}

/**
 * grtnic_watchdog_subtask - check and bring link up
 * @adapter: pointer to the device adapter structure
 **/
static void grtnic_watchdog_subtask(struct grtnic_adapter *adapter)
{
	/* if interface is down, removing or resetting, do nothing */
	if (test_bit(__GRTNIC_DOWN, &adapter->state) ||
	    test_bit(__GRTNIC_REMOVING, &adapter->state) ||
	    test_bit(__GRTNIC_RESETTING, &adapter->state))
		return;

	grtnic_watchdog_update_link(adapter);

	if (adapter->link_up)
		grtnic_watchdog_link_is_up(adapter);
	else
		grtnic_watchdog_link_is_down(adapter);

//	grtnic_update_stats(adapter); //要检查

	grtnic_watchdog_flush_tx(adapter);
}


void grtnic_service_event_schedule(struct grtnic_adapter *adapter)
{
	if (!test_bit(__GRTNIC_DOWN, &adapter->state) &&
	    !test_bit(__GRTNIC_REMOVING, &adapter->state) &&
	    !test_and_set_bit(__GRTNIC_SERVICE_SCHED, &adapter->state))
		queue_work(grtnic_wq, &adapter->service_task);
}

static void grtnic_service_event_complete(struct grtnic_adapter *adapter)
{
	BUG_ON(!test_bit(__GRTNIC_SERVICE_SCHED, &adapter->state));

	/* flush memory to make sure state is correct before next watchog */
	smp_mb__before_atomic();
	clear_bit(__GRTNIC_SERVICE_SCHED, &adapter->state);
}

static void grtnic_remove_adapter(struct grtnic_hw *hw)
{
	struct grtnic_adapter *adapter = hw->back;

	if ((!hw->dma_bar) || (!hw->user_bar))
		return;
	hw->dma_bar = NULL;
	hw->user_bar = NULL;
	e_dev_err("Adapter removed\n");
	if (test_bit(__GRTNIC_SERVICE_INITED, &adapter->state))
		grtnic_service_event_schedule(adapter);
}

static u32 grtnic_check_remove(struct grtnic_hw *hw, u32 reg, u8 bar)
{
	u8 __iomem *reg_addr;
	u8 __iomem *userbar_reg_addr;
	u32 value;
	int i;

	reg_addr = bar ? hw->dma_bar : hw->user_bar;
	if (GRTNIC_REMOVED(reg_addr))
		return GRTNIC_FAILED_READ_REG;

	userbar_reg_addr = READ_ONCE(hw->user_bar);
	/* Register read of 0xFFFFFFFF can indicate the adapter has been
	 * removed, so perform several status register reads to determine if
	 * the adapter has been removed.
	 */
	for (i = 0; i < GRTNIC_FAILED_READ_RETRIES; ++i) {
		value = readl(userbar_reg_addr + XPHY_STATUS);
		if (value != GRTNIC_FAILED_READ_REG)
			break;
		mdelay(3);
	}

	if (value == GRTNIC_FAILED_READ_REG)
		grtnic_remove_adapter(hw);
	else
		value = readl(reg_addr + reg);

	return value;
}

static u32 grtnic_validate_register_read(struct grtnic_hw *_hw, u32 reg, u8 bar)
{
	int i;
	u32 value;
	u8 __iomem *reg_addr;
	struct grtnic_adapter *adapter = _hw->back;

	reg_addr = bar ? _hw->dma_bar : _hw->user_bar;
	if (GRTNIC_REMOVED(reg_addr))
		return GRTNIC_FAILED_READ_REG;
	for (i = 0; i < GRTNIC_DEAD_READ_RETRIES; ++i) {
		value = readl(reg_addr + reg);
		if (value != GRTNIC_DEAD_READ_REG)
			break;
	}

	if (value == GRTNIC_DEAD_READ_REG)
		e_err(drv, "%s: register %x read unchanged\n", __func__, reg);
	else
		e_warn(hw, "%s: register %x read recovered after %d retries\n",
			__func__, reg, i + 1);
	return value;
}

u32 grtnic_read_reg(struct grtnic_hw *hw, u32 reg, u8 bar)
{
	u32 value;
	u8 __iomem *reg_addr;

	reg_addr = bar ? hw->dma_bar : hw->user_bar;
	if (GRTNIC_REMOVED(reg_addr))
		return GRTNIC_FAILED_READ_REG;

	value = readl(reg_addr + reg);
	if (unlikely(value == GRTNIC_FAILED_READ_REG))
		value = grtnic_check_remove(hw, reg, bar);
	if (unlikely(value == GRTNIC_DEAD_READ_REG))
		value = grtnic_validate_register_read(hw, reg, bar);
	return value;
}

/**
 * grtnic_service_timer - Timer Call-back
 * @t: pointer to timer_list
 **/
static void grtnic_service_timer(struct timer_list *t)
{
	struct grtnic_adapter *adapter = from_timer(adapter, t, service_timer);
	unsigned long next_event_offset;

	/* poll faster when waiting for link */
	if (adapter->flags & GRTNIC_FLAG_NEED_LINK_UPDATE)
		next_event_offset = HZ / 10;
	else
		next_event_offset = HZ * 2;

	/* Reset the timer */
	mod_timer(&adapter->service_timer, next_event_offset + jiffies);

	grtnic_service_event_schedule(adapter);
}

/**
 * grtnic_service_task - manages and runs subtasks
 * @work: pointer to work_struct containing our data
 **/
static void grtnic_service_task(struct work_struct *work)
{
	struct grtnic_adapter *adapter = container_of(work, struct grtnic_adapter, service_task);
	if (GRTNIC_REMOVED(adapter->hw.dma_bar)) {
		if (!test_bit(__GRTNIC_DOWN, &adapter->state)) {
			rtnl_lock();
			grtnic_down(adapter);
			rtnl_unlock();
		}
		grtnic_service_event_complete(adapter);
		return;
	}

//	ixgbe_reset_subtask(adapter);
//	ixgbe_phy_interrupt_subtask(adapter);
//	ixgbe_sfp_detection_subtask(adapter);
//	ixgbe_sfp_link_config_subtask(adapter);
//	ixgbe_check_overtemp_subtask(adapter);
	grtnic_watchdog_subtask(adapter);
//	ixgbe_check_hang_subtask(adapter);
	grtnic_service_event_complete(adapter);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int grtnic_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct net_device *netdev;
	struct grtnic_adapter *adapter = NULL;
	struct grtnic_hw *hw = NULL;
	struct device *dev = &pdev->dev;
	static int cards_found;
	int err, pci_using_dac;
	int csum_tx_mode = 0, csum_rx_mode = 0;

#ifdef HAVE_TX_MQ
	unsigned int indices = MAX_TX_QUEUES;
#endif /* HAVE_TX_MQ */
	bool disable_dev = false;

	u32 coresettings;
	u8 mac_addr[6];

	const struct grt_gigeth_info *ei = grt_gigeth_info_tbl[ent->driver_data]; //根据vidpid来配置对应的driver_data

	dev_info(dev, "adapter PCI probe");

    // Enable device
	err = pci_enable_device_mem(pdev);
	if (err)
		return err;

	if (!dma_set_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(64)) &&
	    !dma_set_coherent_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(64))) {
		pci_using_dac = 1;
	} else {
		err = dma_set_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(32));
		if (err) {
			err = dma_set_coherent_mask(pci_dev_to_dev(pdev),
						    DMA_BIT_MASK(32));
			if (err) {
				dev_err(pci_dev_to_dev(pdev), "No usable DMA "
					"configuration, aborting\n");
				goto err_dma;
			}
		}
		pci_using_dac = 0;
	}

	err = pci_request_mem_regions(pdev, DRIVER_NAME);
	if (err) {
		dev_err(pci_dev_to_dev(pdev),
			"pci_request_selected_regions failed 0x%x\n", err);
		goto err_pci_reg;
	}

	// Disable ASPM
	pci_disable_link_state(pdev, PCIE_LINK_STATE_L0S | PCIE_LINK_STATE_L1 | PCIE_LINK_STATE_CLKPM);

#ifdef HAVE_PCI_ENABLE_PCIE_ERROR_REPORTING
	pci_enable_pcie_error_reporting(pdev);
#endif /* HAVE_PCI_ENABLE_PCIE_ERROR_REPORTING */

	// Enable bus mastering for DMA
	pci_set_master(pdev);

#ifdef HAVE_TX_MQ
	indices = min_t(int, ei->dma_channel_max, num_online_cpus());
	netdev = alloc_etherdev_mq(sizeof(struct grtnic_adapter), indices);
#else /* HAVE_TX_MQ */
	netdev = alloc_etherdev(sizeof(struct grtnic_adapter));
#endif /* HAVE_TX_MQ */
	if (!netdev) {
		err = -ENOMEM;
		goto err_alloc_etherdev;
	}

	SET_MODULE_OWNER(netdev);
	SET_NETDEV_DEV(netdev, dev);
	adapter = netdev_priv(netdev);

	adapter->netdev = netdev;
	adapter->dev = dev;
	adapter->pdev = pdev;
  adapter->func = PCI_FUNC(pdev->devfn);
	adapter->ei = ei;
	hw = &adapter->hw;
	hw->back = adapter;
	if(ei->type == board_902T_GRT_FF)
		hw->phy_addr = 0x01;
	else
		hw->phy_addr = 0x00;

	adapter->msg_enable = (1 << DEFAULT_DEBUG_LEVEL_SHIFT) - 1;
	adapter->flags = 0;

	adapter->type = ei->port_type; //fiber or copper?
 	adapter->speed = ei->port_speed;

	adapter->rss_queues = 1;
#ifdef HAVE_TX_MQ
	adapter->rss_queues = indices;
	printk("rss_queues = %d\n", adapter->rss_queues);
#endif

//	adapter->flags |= FLAG_QUEUE_PAIRS;

  // Map BARs
  err = grtnic_map_bars(adapter, pdev);
  if (err)
  {
      dev_err(dev, "Failed to map bar");
	err = -EIO;
      goto err_ioremap;
  }

	grtnic_assign_netdev_ops(netdev);
	strscpy(netdev->name, pci_name(pdev), sizeof(netdev->name));

	adapter->bd_number = cards_found;

	/* setup adapter struct */
	err = grtnic_sw_init(adapter);
	if (err)
		goto err_sw_init;

	/*
	 * check_options must be called before setup_link to set up
	 * hw->fc completely
	 */
	grtnic_check_options(adapter); //内核加载参数这里设置

//reset_hw
	coresettings = GRTNIC_READ_REG(hw, ((TARGET_CONFIG<<12) + ADDR_CORESETTINGS*4), 1); //main reason is for reset dma (RESET_LOGIC)
	printk("Number of channels:%d\n", ((coresettings>>0) & 0xf));
	printk("Bus interface width:%d\n", ((coresettings>>19) & 0xf)*32);
	printk("Bus master enable:%d\n", ((coresettings>>4) & 0x1));
	printk("Negotiated link width:X%d\n", ((coresettings>>5) & 0x3f));
	printk("Negotiated link rate:%d MTs\n", ((coresettings>>11) & 0x3)*2500);
	printk("Max downstream payload:%d bytes\n", 128 << ((coresettings>>13) & 0x7));
	printk("Max upstream payload:%d bytes\n", 128 << ((coresettings>>16) & 0x7));

	if(coresettings==GRTNIC_FAILED_READ_REG) {
		e_dev_err("HW Init failed\n");
		goto err_sw_init;
	}

	GRTNIC_WRITE_REG(hw, ASIC_RX_FIFO_RST, 0xff, 0); //reset all channel rx fifo data
	GRTNIC_WRITE_REG(hw, ASIC_TX_FIFO_RST, 0xff, 0); //reset all channel tx fifo data

	if(adapter->flags & GRTNIC_FLAG_TXCSUM_CAPABLE) csum_tx_mode = 1;
	if(adapter->flags & GRTNIC_FLAG_RXCSUM_CAPABLE) csum_rx_mode = 1;

	GRTNIC_WRITE_REG(hw, CSUM_ENABLE, (csum_rx_mode << 1 | csum_tx_mode), 0); //告诉asic, tx checksum offload
	GRTNIC_WRITE_REG(hw, MAX_LED_PKT_NUM, (100<<16 | 1), 0); //200 is delay time and 1 is pkt number

	netdev->features |= NETIF_F_SG;
	netdev->features |= NETIF_F_GSO;

	if (pci_using_dac)
	 netdev->features |= NETIF_F_HIGHDMA;
		
//		netdev->flags &= ~IFF_MULTICAST;

	if(csum_tx_mode)
		netdev->features |= NETIF_F_HW_CSUM;
#ifdef NETIF_F_RXCSUM
	if(csum_rx_mode)
		netdev->features |= NETIF_F_RXCSUM;
#endif
#ifdef NETIF_F_RXHASH
	netdev->features |= NETIF_F_RXHASH;
#endif /* NETIF_F_RXHASH */

#if defined(HAVE_NDO_SET_FEATURES) && !defined(HAVE_RHEL6_NET_DEVICE_OPS_EXT)
	netdev->hw_features = netdev->features;
#endif

#ifdef HAVE_NETDEVICE_MIN_MAX_MTU
	/* MTU range: 68 - 9710 */
#ifdef HAVE_RHEL7_EXTENDED_MIN_MAX_MTU
	netdev->extended->min_mtu = ETH_MIN_MTU;
	netdev->extended->max_mtu = GRTNIC_MAX_JUMBO_FRAME_SIZE - (ETH_HLEN + ETH_FCS_LEN);
#else
	netdev->min_mtu = ETH_MIN_MTU;
	netdev->max_mtu = GRTNIC_MAX_JUMBO_FRAME_SIZE - (ETH_HLEN + ETH_FCS_LEN);
#endif //HAVE_RHEL7_EXTENDED_MIN_MAX_MTU
#endif //HAVE_NETDEVICE_MIN_MAX_MTU

	hw->mac.fc.fc_autoneg = false;
	hw->mac.fc.current_mode = fc_rx_pause;

	grtnic_GetMacAddress(netdev, mac_addr);

	if (is_valid_ether_addr((unsigned char *)(mac_addr)))
		eth_hw_addr_set(netdev, mac_addr);
	else
	{
		memcpy(mac_addr, DEFAULT_ETHER_ADDRESS, netdev->addr_len);
		mac_addr[netdev->addr_len-1] = adapter->func;
		eth_hw_addr_set(netdev, mac_addr);
		grtnic_SetMacAddress(netdev, netdev->dev_addr); //added
	}

#ifdef ETHTOOL_GPERMADDR
	memcpy(netdev->perm_addr, mac_addr, netdev->addr_len);
#endif

	grtnic_SetMacPauseAddress(netdev, netdev->dev_addr);

	grtnic_SetPause(netdev, 1); //rx pause, tx off

	printk("add=%02x:%02x:%02x:%02x:%02x:%02x\n", netdev->dev_addr[5],netdev->dev_addr[4],netdev->dev_addr[3],netdev->dev_addr[2],netdev->dev_addr[1],netdev->dev_addr[0]);
	
	grtnic_ResetRx(netdev);
	grtnic_SetRx(netdev, 0);	//disable rx
	grtnic_ResetTx(netdev);
	grtnic_SetTx(netdev, 0);	//disable tx

	timer_setup(&adapter->service_timer, grtnic_service_timer, 0);
	INIT_WORK(&adapter->service_task, grtnic_service_task);

	set_bit(__GRTNIC_SERVICE_INITED, &adapter->state);
	clear_bit(__GRTNIC_SERVICE_SCHED, &adapter->state);

	err = grtnic_init_interrupt_scheme(adapter);
	if (err)
		goto err_sw_init;

//	err = hw->mac.ops.start_hw(hw);
//主要调用了ixgbe_start_hw_generic：Clear statistics registers & Setup flow control

	strscpy(netdev->name, "eth%d", sizeof(netdev->name));
	pci_set_drvdata(pdev, adapter);
	err = register_netdev(netdev);
	if (err)
		goto err_register;
	adapter->netdev_registered = true;


#ifdef HAVE_PCI_ERS
	/*
	 * call save state here in standalone driver because it relies on
	 * adapter struct to exist, and needs to call netdev_priv
	 */
	pci_save_state(pdev);

#endif

	/* power down the optics for 82599 SFP+ fiber */
	GRTNIC_WRITE_REG(hw, PHY_TX_DISABLE, 0x01, 1); //disable laser;

	/* carrier off reporting is important to ethtool even BEFORE open */
	netif_carrier_off(netdev);
	/* keep stopping all the transmit queues for older kernels */
	netif_tx_stop_all_queues(netdev);

#if 0
//#if IS_ENABLED(CONFIG_DCA)
	if (adapter->flags & GRTNIC_FLAG_DCA_CAPABLE) {
		ret = dca_add_requester(pci_dev_to_dev(pdev));
		switch (ret) {
		case 0:
			adapter->flags |= GRTNIC_FLAG_DCA_ENABLED;
			grtnic_setup_dca(adapter);
			break;
		/* -19 is returned from the kernel when no provider is found */
		case -19:
			printk("No DCA provider found. Please "
			       "start ioatdma for DCA functionality.\n");
			break;
		default:
			printk("DCA registration failed: %d\n", ret);
			break;
		}
	}
#endif

	cards_found++;

#ifdef GRTNIC_PROCFS
	if (grtnic_procfs_init(adapter))
		e_err(probe, "failed to allocate procfs resources\n");
#endif /* IXGBE_PROCFS */

  // probe complete
  return 0;

err_register:
	grtnic_clear_interrupt_scheme(adapter);
err_sw_init:
	kfree(adapter->rss_key);
	grtnic_free_bars(adapter, pdev);
err_ioremap:
	disable_dev = !test_and_set_bit(__GRTNIC_DISABLED, &adapter->state);
	free_netdev(netdev);
err_alloc_etherdev:
	pci_release_mem_regions(pdev);
err_pci_reg:
err_dma:
	if (!adapter || disable_dev)
		pci_disable_device(pdev);
	return err;
}


static void grtnic_pci_remove(struct pci_dev *pdev)
{
  struct grtnic_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev;
	bool disable_dev;

    dev_info(&pdev->dev, "grtnic PCI remove");

	/* if !adapter then we already cleaned up in probe */
	if (!adapter)
		return;

	netdev = adapter->netdev;

	set_bit(__GRTNIC_REMOVING, &adapter->state);
	cancel_work_sync(&adapter->service_task);

#if 0
//#if IS_ENABLED(CONFIG_DCA)
	if (adapter->flags & GRTNIC_FLAG_DCA_ENABLED) {
		adapter->flags &= ~GRTNIC_FLAG_DCA_ENABLED;
		dca_remove_requester(pci_dev_to_dev(pdev));
		write_register(GRTNIC_DCA_CTRL_DCA_DISABLE,  adapter->dma_bar+ (TARGET_CONFIG<<12) + ADDR_DCA_GTCL*4);
	}
#endif /* CONFIG_DCA */

#ifdef GRTNIC_PROCFS
	grtnic_procfs_exit(adapter);
#endif /* GRTNIC_PROCFS */

	if (adapter->netdev_registered) {
		unregister_netdev(netdev);
		adapter->netdev_registered = false;
	}

	grtnic_clear_interrupt_scheme(adapter);
	grtnic_free_bars(adapter, pdev);
	pci_release_regions(pdev);
	kfree(adapter->rss_key);
	disable_dev = !test_and_set_bit(__GRTNIC_DISABLED, &adapter->state);
	free_netdev(netdev);

#ifdef HAVE_PCI_ENABLE_PCIE_ERROR_REPORTING
	pci_disable_pcie_error_reporting(pdev);
#endif /* HAVE_PCI_ENABLE_PCIE_ERROR_REPORTING */

	if (disable_dev)
		pci_disable_device(pdev);
}


/*
 * __grtnic_shutdown is not used when power management
 * is disabled on older kernels (<2.6.12). causes a compile
 * warning/error, because it is defined and not used.
 */
#if defined(CONFIG_PM) || !defined(USE_REBOOT_NOTIFIER)
static int __grtnic_shutdown(struct pci_dev *pdev, bool *enable_wake)
{
	struct grtnic_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;
//	u32 wufc = adapter->wol;
	u32 wufc = 0;
#ifdef CONFIG_PM
	int retval = 0;
#endif

	rtnl_lock();
	netif_device_detach(netdev);

	if (netif_running(netdev))
		grtnic_close_suspend(adapter);

	grtnic_clear_interrupt_scheme(adapter);
	rtnl_unlock();

#ifdef CONFIG_PM
	retval = pci_save_state(pdev);
	if (retval)
		return retval;

#endif

	*enable_wake = !!wufc;

	if (!test_and_set_bit(__GRTNIC_DISABLED, &adapter->state))
		pci_disable_device(pdev);

	return 0;
}
#endif /* defined(CONFIG_PM) || !defined(USE_REBOOT_NOTIFIER) */


#ifndef USE_REBOOT_NOTIFIER
static void grtnic_pci_shutdown(struct pci_dev *pdev)
{
	bool wake;

	__grtnic_shutdown(pdev, &wake);

	if (system_state == SYSTEM_POWER_OFF) {
		pci_wake_from_d3(pdev, wake);
		pci_set_power_state(pdev, PCI_D3hot);
	}
}

#endif


static struct pci_driver grtnic_pci_driver = {
    .name = DRIVER_NAME,
    .id_table = grtnic_pci_tbl,
    .probe = grtnic_pci_probe,
    .remove = grtnic_pci_remove,
#ifndef USE_REBOOT_NOTIFIER
		.shutdown = grtnic_pci_shutdown,
#endif
};

static int __init grtnic_init(void)
{
	int ret;
	printk("Beijing GRT(R) NIC Network Driver - %s\n", DRIVER_VERSION);
	printk("Copyright(c) 2020-2022 Beijing GRT Corporation.\n");

	grtnic_wq = create_singlethread_workqueue(DRIVER_NAME);
	if (!grtnic_wq) {
		pr_err("%s: Failed to create workqueue\n", DRIVER_NAME);
		return -ENOMEM;
	}

#ifdef GRTNIC_PROCFS
	if (grtnic_procfs_topdir_init())
		pr_info("Procfs failed to initialize topdir\n");
#endif

	ret = pci_register_driver(&grtnic_pci_driver);
	if (ret)
	{
		destroy_workqueue(grtnic_wq);
#ifdef GRTNIC_PROCFS
		grtnic_procfs_topdir_exit();
#endif
	  return ret;
	}

//#if IS_ENABLED(CONFIG_DCA)
//	dca_register_notify(&dca_notifier);
//#endif

  return ret;
}

static void __exit grtnic_exit(void)
{
//#if IS_ENABLED(CONFIG_DCA)
//	dca_unregister_notify(&dca_notifier);
//#endif
  pci_unregister_driver(&grtnic_pci_driver);
#ifdef GRTNIC_PROCFS
	grtnic_procfs_topdir_exit();
#endif
	destroy_workqueue(grtnic_wq);
}

#if 0
//#if IS_ENABLED(CONFIG_DCA)
static int grtnic_notify_dca(struct notifier_block __always_unused *nb, unsigned long event, void __always_unused *p)
{
	int ret_val;

	ret_val = driver_for_each_device(&grtnic_pci_driver.driver, NULL, &event,
					 __grtnic_notify_dca);

	return ret_val ? NOTIFY_BAD : NOTIFY_DONE;
}
#endif

const struct grt_gigeth_info grt_902eff_info = {
	.type							= board_902E_GRT_FF,
	.dma_channel_max 	= 1,
	.port_type 				= 0,
	.port_speed				= 0,
};

const struct grt_gigeth_info grt_902tff_info = {
	.type							= board_902T_GRT_FF,
	.dma_channel_max 	= 1,
	.port_type 				= 1,
	.port_speed				= 0,
};

const struct grt_gigeth_info grt_901elr_info = {
	.type							= board_901ELR_GRT_FF,
	.dma_channel_max 	= 1,
	.port_type 				= 0,
	.port_speed				= 0,
};

const struct grt_gigeth_info grt_1002eff_info = {
	.type							= board_1002E_GRT_FF,
	.dma_channel_max 	= 8,
	.port_type 				= 0,
	.port_speed				= 1,
};

const struct grt_gigeth_info grt_1001eff_info = {
	.type							= board_1001E_GRT_FF,
	.dma_channel_max 	= 8,
	.port_type 				= 0,
	.port_speed				= 1,
};

const struct grt_gigeth_info qm_1001eff_info = {
	.type							= board_1001E_QM_FF,
	.dma_channel_max 	= 8,
	.port_type 				= 0,
	.port_speed				= 1,
};

const struct grt_gigeth_info grt_1005efx_info = {
	.type							= board_1005E_GRT_FX,
	.dma_channel_max 	= 8,
	.port_type 				= 0,
	.port_speed				= 1,
};

module_init(grtnic_init);
module_exit(grtnic_exit);