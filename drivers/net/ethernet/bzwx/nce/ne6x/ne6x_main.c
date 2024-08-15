// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include <net/gre.h>
#include <uapi/linux/if.h>
#include <net/geneve.h>
#include <net/vxlan.h>

#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <net/sch_generic.h>

#include "ne6x.h"
#include "ne6x_portmap.h"
#include "ne6x_reg.h"
#include "ne6x_dev.h"
#include "ne6x_debugfs.h"
#include "ne6x_arfs.h"
#include "version.h"
#include "ne6x_netlink.h"
#include "ne6x_interrupt.h"

#define SUMMARY "Chengdu BeiZhongWangXin Ethernet Connection N5/N6 Series Linux Driver"
#define COPYRIGHT "Copyright(c) 2020 - 2023 Chengdu BeiZhongWangXin Technology Co., Ltd."

char ne6x_driver_name[] = "ncepf";

static const char ne6x_driver_string[] = SUMMARY;

const char ne6x_driver_version_str[] = VERSION;
static const char ne6x_copyright[] = COPYRIGHT;

/* ne6x_pci_tbl - PCI Device ID Table
 *
 * Last entry must be all 0s
 *
 * { Vendor ID, Device ID, SubVendor ID, SubDevice ID,
 *   Class, Class Mask, private data (not used) }
 */
static const struct pci_device_id ne6x_pci_tbl[] = {
	{PCI_VDEVICE(BZWX, 0x5010), 0},
	{PCI_VDEVICE(BZWX, 0x5011), 0},
	{PCI_VDEVICE(BZWX, 0x6010), 0},
	{PCI_VDEVICE(BZWX, 0x6011), 0},
	/* required last entry */
	{0, 0},
};

MODULE_DEVICE_TABLE(pci, ne6x_pci_tbl);
MODULE_AUTHOR("Chengdu BeiZhongWangXin Technology Co., Ltd., <support@bzwx-kj.com>");
MODULE_DESCRIPTION("Chengdu BeiZhongWangXin Ethernet Connection N5/N6 Series Linux Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION);

static struct workqueue_struct *ne6x_wq;
static const struct net_device_ops ne6x_netdev_ops;

bool netif_is_ne6x(struct net_device *dev)
{
	return dev && (dev->netdev_ops == &ne6x_netdev_ops);
}

int ne6x_hw_init(struct ne6x_hw *hw)
{
	int cpu_num = num_online_cpus();

	/* max phy_port */
	hw->pf_port = ne6x_dev_get_port_num(hw->back);
	/* expect vp queue */
	hw->expect_vp = NE6X_MAX_VP_NUM / hw->pf_port;
	/* actal max vp queue */
	hw->max_queue = min_t(int, cpu_num, hw->expect_vp);

	hw->port_info = devm_kzalloc(ne6x_hw_to_dev(hw), sizeof(*hw->port_info), GFP_KERNEL);
	if (!hw->port_info)
		return -EIO;

	/* set the back pointer to HW */
	hw->port_info->hw = hw;

	if (!is_valid_ether_addr(hw->port_info->mac.perm_addr))
		eth_random_addr(hw->port_info->mac.perm_addr);

	return 0;
}

int ne6x_aq_get_phy_capabilities(struct ne6x_adapter *adpt, bool is_up, bool get_hw_stats)
{
	struct ne6x_port_info *port_info = adpt->port_info;

	/* read link states */
	if (get_hw_stats)
		ne6x_dev_get_link_status(adpt, &port_info->link_status);

	if (is_up) {
		if (port_info->link_status.link) {
			port_info->phy.link_info.link_info |= NE6X_AQ_LINK_UP;

			switch (port_info->link_status.speed) {
			case NE6X_LINK_SPEED_10GB:
				port_info->phy.link_info.phy_type_low = NE6X_PHY_TYPE_10GBASE;
				port_info->phy.link_info.link_speed = NE6X_LINK_SPEED_10GB;
				break;
			case NE6X_LINK_SPEED_25GB:
				port_info->phy.link_info.phy_type_low = NE6X_PHY_TYPE_25GBASE;
				port_info->phy.link_info.link_speed = NE6X_LINK_SPEED_25GB;
				break;
			case NE6X_LINK_SPEED_40GB:
				port_info->phy.link_info.phy_type_low = NE6X_PHY_TYPE_40GBASE;
				port_info->phy.link_info.link_speed = NE6X_LINK_SPEED_40GB;
				break;
			case NE6X_LINK_SPEED_100GB:
				port_info->phy.link_info.phy_type_low = NE6X_PHY_TYPE_100GBASE;
				port_info->phy.link_info.link_speed = NE6X_LINK_SPEED_100GB;
				break;
			case NE6X_LINK_SPEED_200GB:
				port_info->phy.link_info.phy_type_low = NE6X_PHY_TYPE_200GBASE;
				port_info->phy.link_info.link_speed = NE6X_LINK_SPEED_200GB;
				break;
			default:
				dev_info(&adpt->back->pdev->dev, "WARNING: Unrecognized link_speed (0x%x).\n",
					 NE6X_LINK_SPEED_UNKNOWN);
				break;
			}

			port_info->phy.media_type = NE6X_MEDIA_FIBER;
			return 0;
		}
	}

	port_info->phy.link_info.phy_type_low = NE6X_PHY_TYPE_UNKNOWN;
	port_info->phy.link_info.link_speed   = NE6X_LINK_SPEED_UNKNOWN;
	port_info->phy.media_type             = NE6X_MEDIA_UNKNOWN;
	port_info->phy.link_info.link_info   &= ~NE6X_AQ_LINK_UP;

	return 0;
}

int ne6x_aq_get_vf_link_status(struct ne6x_adapter *adpt, bool is_up)
{
	struct ne6x_pf *pf = adpt->back;
	struct ne6x_adapter *pf_adpt = pf->adpt[(adpt->port_info->lport >= pf->hw.pf_port) ?
					 (pf->hw.pf_port - 1) : adpt->port_info->lport];
	struct ne6x_link_info *pf_link_status = &pf_adpt->port_info->link_status;
	struct ne6x_port_info *vf_port_info = adpt->port_info;

	if (is_up) {
		if (pf_link_status->link) {
			vf_port_info->phy.link_info.link_info |= NE6X_AQ_LINK_UP;

			switch (pf_link_status->speed) {
			case NE6X_LINK_SPEED_10GB:
				vf_port_info->phy.link_info.phy_type_low = NE6X_PHY_TYPE_10GBASE;
				vf_port_info->phy.link_info.link_speed   = NE6X_LINK_SPEED_10GB;
				break;
			case NE6X_LINK_SPEED_25GB:
				vf_port_info->phy.link_info.phy_type_low = NE6X_PHY_TYPE_25GBASE;
				vf_port_info->phy.link_info.link_speed   = NE6X_LINK_SPEED_25GB;
				break;
			case NE6X_LINK_SPEED_40GB:
				vf_port_info->phy.link_info.phy_type_low = NE6X_PHY_TYPE_40GBASE;
				vf_port_info->phy.link_info.link_speed   = NE6X_LINK_SPEED_40GB;
				break;
			case NE6X_LINK_SPEED_100GB:
				vf_port_info->phy.link_info.phy_type_low = NE6X_PHY_TYPE_100GBASE;
				vf_port_info->phy.link_info.link_speed   = NE6X_LINK_SPEED_100GB;
				break;
			case NE6X_LINK_SPEED_200GB:
				vf_port_info->phy.link_info.phy_type_low = NE6X_PHY_TYPE_200GBASE;
				vf_port_info->phy.link_info.link_speed   = NE6X_LINK_SPEED_200GB;
				break;
			default:
				dev_info(&adpt->back->pdev->dev, "WARNING: Unrecognized link_speed (0x%x).\n",
					 NE6X_LINK_SPEED_UNKNOWN);
				break;
			}

			vf_port_info->phy.media_type = NE6X_MEDIA_FIBER;
			return 0;
		}
	}

	vf_port_info->phy.link_info.phy_type_low = NE6X_PHY_TYPE_UNKNOWN;
	vf_port_info->phy.link_info.link_speed   = NE6X_LINK_SPEED_UNKNOWN;
	vf_port_info->phy.media_type             = NE6X_MEDIA_UNKNOWN;
	vf_port_info->phy.link_info.link_info   &= ~NE6X_AQ_LINK_UP;

	return 0;
}

static void ne6x_adpt_link_event(struct ne6x_adapter *adpt, bool link_up)
{
	if (!adpt)
		return;

	if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state) || !adpt->netdev)
		return;

	if (link_up == netif_carrier_ok(adpt->netdev))
		return;

	if (link_up) {
		netif_carrier_on(adpt->netdev);
		netif_tx_wake_all_queues(adpt->netdev);
	} else {
		netif_carrier_off(adpt->netdev);
		netif_tx_stop_all_queues(adpt->netdev);
	}
}

void ne6x_print_link_message(struct ne6x_adapter *adpt, bool isup)
{
	char *speed = "Unknown ";
	char *an = "False";
	u16 new_speed;

	if (isup)
		new_speed = adpt->port_info->phy.link_info.link_speed;
	else
		new_speed = NE6X_LINK_SPEED_UNKNOWN;

	if (adpt->current_isup == isup && adpt->current_speed == new_speed)
		return;

	adpt->current_isup = isup;
	adpt->current_speed = new_speed;

	if (!isup) {
		netdev_info(adpt->netdev, "NIC Link is Down\n");
		return;
	}

	switch (adpt->port_info->phy.link_info.link_speed) {
	case NE6X_LINK_SPEED_40GB:
		speed = "40 G";
		break;
	case NE6X_LINK_SPEED_100GB:
		speed = "100 G";
		break;
	case NE6X_LINK_SPEED_10GB:
		speed = "10 G";
		break;
	case NE6X_LINK_SPEED_25GB:
		speed = "25 G";
		break;
	case NE6X_LINK_SPEED_200GB:
		speed = "200 G";
		break;
	default:
		break;
	}

	if (adpt->port_info->phy.link_info.an_info)
		an = "True";

	netdev_info(adpt->netdev, "NIC Link is Up, %sbps Full Duplex, Autoneg: %s\n", speed, an);
}

static void ne6x_link_event(struct ne6x_pf *pf)
{
	struct ne6x_phy_info *phy_info;
	struct ne6x_adapter *adpt = NULL;
	u32 old_link_speed;
	bool old_link;
	bool link_up;
	int i;
#ifdef CONFIG_PCI_IOV
	struct ne6x_vf *vf;
	int vf_id;
#endif

	for (i = 0; i < pf->num_alloc_adpt; i++) {
		link_up = false;
		adpt = pf->adpt[i];
		phy_info = &adpt->port_info->phy;
		phy_info->link_info_old = phy_info->link_info;

		if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state))
			ne6x_aq_get_phy_capabilities(adpt, false, true);
		else
			ne6x_aq_get_phy_capabilities(adpt, true, true);

		/* add sfp online state begin */
		ne6x_dev_get_sfp_status(adpt, &phy_info->link_info.ext_info);
		if (phy_info->link_info.ext_info != phy_info->link_info_old.ext_info) {
			if (phy_info->link_info.ext_info == 0)
				netdev_info(adpt->netdev, "adpt->id= %d,optical module unplugged",
					    adpt->idx);
			else
				netdev_info(adpt->netdev, "adpt->id= %d,optical module plugged",
					    adpt->idx);
		}

		/* end sfp online state */
		old_link = !!(adpt->port_info->phy.link_info_old.link_info & NE6X_AQ_LINK_UP);
		old_link_speed = adpt->port_info->phy.link_info_old.link_speed;
		/* Check if the link state is up after updating link info, and treat
		 * this event as an UP event since the link is actually UP now.
		 */
		if (adpt->port_info->phy.link_info.link_info & NE6X_AQ_LINK_UP)
			link_up = true;

		/* if the old link up/down is the same as the new */
		if (link_up == old_link) {
			if (link_up && old_link_speed != adpt->port_info->phy.link_info.link_speed)
				ne6x_print_link_message(adpt, link_up);

			continue;
		}

		ne6x_adpt_link_event(adpt, link_up);
		ne6x_print_link_message(adpt, link_up);
	}

#ifdef CONFIG_PCI_IOV
	ne6x_for_each_vf(pf, vf_id) {
		vf = &pf->vf[vf_id];
		adpt = vf->adpt;

		if (test_bit(NE6X_VF_STATE_INIT, vf->vf_states)) {
			if (!vf->rx_tx_state) {
				adpt->port_info->phy.link_info.link_info = 0x0;
				vf->rx_tx_state = true;
			}
			link_up = false;
			phy_info = &adpt->port_info->phy;
			phy_info->link_info_old = phy_info->link_info;
			ne6x_aq_get_vf_link_status(adpt, true);
			old_link = !!(adpt->port_info->phy.link_info_old.link_info
				      & NE6X_AQ_LINK_UP);
			old_link_speed = adpt->port_info->phy.link_info_old.link_speed;

			if (adpt->port_info->phy.link_info.link_info & NE6X_AQ_LINK_UP)
				link_up = true;

			if (link_up == old_link &&
			    old_link_speed == adpt->port_info->phy.link_info.link_speed)
				continue;

			pf->hw.mbx_snapshot.state = NE6X_MAL_VF_DETECT_STATE_DETECT;
			ne6x_vc_notify_link_state(vf);
		}
	}
#endif
}

static void ne6x_clean_link_status_subtask(struct ne6x_pf *pf)
{
	if (!test_bit(NE6X_LINK_POOLING, pf->state))
		return;

	ne6x_link_event(pf);
}

void ne6x_service_event_schedule(struct ne6x_pf *pf)
{
	if (!test_bit(NE6X_DOWN, pf->state))
		queue_work(ne6x_wq, &pf->serv_task);
}

static void ne6x_adpt_reinit_locked(struct ne6x_adapter *adpt);

static void ne6x_do_reset(struct ne6x_pf *pf, u32 reset_flags, bool lock_acquired)
{
	struct ne6x_adapter *adpt = NULL;
	int i;

	WARN_ON(in_interrupt());

	if (reset_flags & BIT_ULL(NE6X_PF_RESET_REQUESTED)) {
		for (i = 0; i < pf->num_alloc_adpt; i++) {
			adpt = pf->adpt[i];
			if (test_bit(NE6X_ADPT_RECOVER, adpt->comm.state)) {
				ne6x_adpt_reinit_locked(adpt);
				clear_bit(NE6X_ADPT_RECOVER, adpt->comm.state);
			}
		}
	} else if (reset_flags & BIT_ULL(NE6X_CORE_RESET_REQUESTED)) {
		/* hardware reset:include PCIE,CORE.etc. */
		dev_info(&pf->pdev->dev, "timeout info: CORE reset\n");
	} else {
		dev_info(&pf->pdev->dev, "bad reset request 0x%08x\n", reset_flags);
	}
}

static void ne6x_recover_hang_subtask(struct ne6x_pf *pf)
{
	u32 reset_flags = 0;

	if (test_and_clear_bit(NE6X_PF_RESET_REQUESTED, pf->state))
		reset_flags |= BIT(NE6X_PF_RESET_REQUESTED);

	if (test_and_clear_bit(NE6X_CORE_RESET_REQUESTED, pf->state))
		reset_flags |= BIT(NE6X_CORE_RESET_REQUESTED);

	if (test_and_clear_bit(NE6X_GLOBAL_RESET_REQUESTED, pf->state))
		reset_flags |= BIT(NE6X_GLOBAL_RESET_REQUESTED);

	if (test_and_clear_bit(NE6X_DOWN_REQUESTED, pf->state))
		reset_flags |= BIT(NE6X_DOWN_REQUESTED);

	/* If there's a recovery already waiting, it takes
	 * precedence before starting a new reset sequence.
	 */
	if (test_bit(NE6X_RESET_INTR_RECEIVED, pf->state)) {
		clear_bit(NE6X_RESET_INTR_RECEIVED, pf->state);
		test_and_clear_bit(NE6X_TIMEOUT_RECOVERY_PENDING, pf->state);
	}

	/* If we're already down or resetting, just bail */
	if (reset_flags && !test_bit(NE6X_DOWN, pf->state) &&
	    !test_bit(NE6X_CONFIG_BUSY, pf->state))
		ne6x_do_reset(pf, reset_flags, false);
}

static void ne6x_service_timer(struct timer_list *t)
{
	struct ne6x_pf *pf = from_timer(pf, t, serv_tmr);

	if (pf->num_alloc_vfs)
		mod_timer(&pf->serv_tmr, round_jiffies(jiffies + pf->service_timer_period));

	ne6x_service_event_schedule(pf);
}

void ne6x_linkscan_schedule(struct ne6x_pf *pf)
{
	if (!test_bit(NE6X_DOWN, pf->state))
		queue_work(ne6x_wq, &pf->linkscan_work);
}

static void ne6x_linkscan_timer(struct timer_list *t)
{
	struct ne6x_pf *pf = from_timer(pf, t, linkscan_tmr);

	if (pf->irq_pile->num_entries < NE6X_MAX_MSIX_NUM)
		mod_timer(&pf->linkscan_tmr, round_jiffies(jiffies + HZ));
	else
		mod_timer(&pf->linkscan_tmr, round_jiffies(jiffies + HZ * 30));

	if (!test_bit(NE6X_DOWN, pf->state))
		queue_work(ne6x_wq, &pf->linkscan_work);
}

static void ne6x_service_task(struct work_struct *work)
{
	struct ne6x_pf *pf = container_of(work, struct ne6x_pf, serv_task);
	unsigned long start_time = jiffies;

#ifdef CONFIG_PCI_IOV
	/* vf command process */
	ne6x_vc_process_vf_msg(pf);
#endif

	ne6x_recover_hang_subtask(pf);

	ne6x_sync_arfs_fltrs(pf);

	/* If the tasks have taken longer than one timer cycle or there
	 * is more work to be done, reschedule the service task now
	 * rather than wait for the timer to tick again.
	 */
	if (time_after(jiffies, (start_time + pf->service_timer_period)) ||
	    test_bit(NE6X_MAILBOXQ_EVENT_PENDING, pf->state) ||
	    test_bit(NE6X_RESET_INTR_RECEIVED, pf->state))
		ne6x_service_event_schedule(pf);
}

static void ne6x_linkscan_work(struct work_struct *work)
{
	struct ne6x_pf *pf = container_of(work, struct ne6x_pf, linkscan_work);

	ne6x_clean_link_status_subtask(pf);
}

irqreturn_t ne6x_linkint_irq_handler(int irq, void *data)
{
	struct ne6x_pf *pf = data;
	u64 intval = rd64_bar4(&pf->hw,
			       NE6X_PFINT_DYN_CTLN(NE6X_NIC_INT_VP - NE6X_PF_VP0_NUM,
						   NE6X_VP_INT));

	wr64_bar4(&pf->hw,
		  NE6X_PFINT_DYN_CTLN(NE6X_NIC_INT_VP - NE6X_PF_VP0_NUM,
				      NE6X_VP_INT),
		  intval);
	ne6x_linkscan_schedule(pf);

	return IRQ_HANDLED;
}

int ne6x_pf_init(struct ne6x_pf *pf)
{
	pf->ctrl_adpt_idx = 0;
	mutex_init(&pf->switch_mutex);

	/* set up periodic task facility */
	timer_setup(&pf->serv_tmr, ne6x_service_timer, 0);
	pf->service_timer_period = HZ;
	timer_setup(&pf->linkscan_tmr, ne6x_linkscan_timer, 0);
	add_timer(&pf->serv_tmr);

	INIT_WORK(&pf->serv_task, ne6x_service_task);
	INIT_WORK(&pf->linkscan_work, ne6x_linkscan_work);

	clear_bit(NE6X_SERVICE_SCHED, pf->state);

	pf->next_adpt = 0;
	pf->num_alloc_adpt = pf->hw.pf_port;
	pf->num_alloc_vfs = 0;
	pf->mailbox_int_irq_ready = false;
	pf->link_int_irq_ready = false;

	ne6x_dbg_pf_init(pf);
	ne6x_proc_pf_init(pf);

	/* init key list head node */
	spin_lock_init(&pf->key_list_lock);
	INIT_LIST_HEAD(&pf->key_filter_list);

	return 0;
}

static void ne6x_set_num_rings_in_adpt(struct ne6x_adapter *adpt)
{
	adpt->base_queue    = adpt->port_info->hw_queue_base;
	adpt->num_q_vectors = adpt->port_info->queue;
	adpt->num_queue     = adpt->num_q_vectors;
	adpt->num_tx_desc   = ALIGN(NE6X_DEFAULT_NUM_DESCRIPTORS, NE6X_REQ_DESCRIPTOR_MULTIPLE);
	adpt->num_rx_desc   = ALIGN(NE6X_DEFAULT_NUM_DESCRIPTORS, NE6X_REQ_DESCRIPTOR_MULTIPLE);
	adpt->num_cq_desc   = adpt->num_tx_desc + adpt->num_rx_desc;
	adpt->num_tg_desc   = adpt->num_tx_desc;
	adpt->irqs_ready    = false;
}

static irqreturn_t ne6x_msix_clean_rings(int irq, void *data)
{
	struct ne6x_q_vector *q_vector = data;
	struct ne6x_adapter *adpt = (struct ne6x_adapter *)q_vector->adpt;
	struct ne6x_hw *hw = &adpt->back->hw;

	if (!q_vector->tx.ring && !q_vector->rx.ring && !q_vector->cq.ring && !q_vector->tg.ring)
		return IRQ_HANDLED;

	if (q_vector->reg_idx < NE6X_PF_VP0_NUM)
		wr64(hw, NE6X_VPINT_DYN_CTLN(q_vector->reg_idx, NE6X_VP_INT_MASK),
		     0xffffffffffffffff);
	else
		wr64_bar4(hw,
			  NE6X_PFINT_DYN_CTLN(q_vector->reg_idx - NE6X_PF_VP0_NUM,
					      NE6X_VP_INT_MASK),
			  0xffffffffffffffff);

	napi_schedule_irqoff(&q_vector->napi);

	return IRQ_HANDLED;
}

int ne6x_adpt_mem_alloc(struct ne6x_pf *pf, struct ne6x_adapter *adpt)
{
	struct ne6x_ring **next_rings;
	int ret = -ENODEV;
	int size;

	/* Need to protect the allocation of the adapters at the PF level */
	mutex_lock(&pf->switch_mutex);

	adpt->netdev_registered = false;
	size = sizeof(struct ne6x_ring *) * adpt->num_queue * 4;
	adpt->tx_rings = kzalloc(size, GFP_KERNEL);
	if (!adpt->tx_rings)
		goto err_rings;

	next_rings = adpt->tx_rings + adpt->num_queue;
	adpt->cq_rings = next_rings;
	next_rings += adpt->num_queue;
	adpt->rx_rings = next_rings;
	adpt->tg_rings = adpt->rx_rings + adpt->num_queue;

	/* allocate memory for q_vector pointers */
	size = sizeof(struct ne6x_q_vector *) * adpt->num_q_vectors;
	adpt->q_vectors = kzalloc(size, GFP_KERNEL);
	if (!adpt->q_vectors) {
		kfree(adpt->tx_rings);
		ret = -ENOMEM;
		goto err_rings;
	}

	/* Setup default MSIX irq handler for adapter */
	ne6x_adpt_setup_irqhandler(adpt, ne6x_msix_clean_rings);
	ret = 0;

err_rings:
	mutex_unlock(&pf->switch_mutex);
	return ret;
}

static int ne6x_force_link_state(struct ne6x_adapter *adpt, bool is_up)
{
	int err;

	err = ne6x_aq_get_phy_capabilities(adpt, is_up, true);
	if (err)
		return err;

	if (is_up)
		test_and_set_bit(NE6X_LINK_POOLING, adpt->back->state);

	return 0;
}

int ne6x_adpt_restart_vp(struct ne6x_adapter *adpt, bool enable)
{
	struct ne6x_pf *pf = adpt->back;
	struct ne6x_hw *hw = &pf->hw;
	int i, pf_q;

	pf_q = adpt->base_queue;
	for (i = 0; i < adpt->num_queue; i++, pf_q++) {
		if (pf_q < NE6X_PF_VP0_NUM)
			wr64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_VP_RELOAD), enable);
		else
			wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM, NE6X_VP_RELOAD),
				  enable);

		usleep_range(1000, 2000);
		if (!enable) {
			ne6x_tail_update(adpt->rx_rings[i], 0);
			ne6x_tail_update(adpt->tx_rings[i], 0);
		}
	}

	return 0;
}

int ne6x_adpt_configure(struct ne6x_adapter *adpt)
{
	int err;
	int i;

	err = ne6x_adpt_restart_vp(adpt, true);
	if (!err)
		err = ne6x_adpt_configure_tx(adpt);

	if (!err)
		err = ne6x_adpt_configure_cq(adpt);

	if (!err)
		err = ne6x_adpt_configure_rx(adpt);

	if (!err)
		err = ne6x_adpt_restart_vp(adpt, false);

	if (!err) {
		for (i = 0; i < adpt->num_queue && !err; i++)
			ne6x_alloc_rx_buffers(adpt->rx_rings[i],
					      NE6X_DESC_UNUSED(adpt->rx_rings[i]));
	}

	return err;
}

static void ne6x_napi_enable_all(struct ne6x_adapter *adpt)
{
	int q_idx;

	if (!adpt->netdev)
		return;

	for (q_idx = 0; q_idx < adpt->num_q_vectors; q_idx++) {
		struct ne6x_q_vector *q_vector = adpt->q_vectors[q_idx];

		if (q_vector->tx.ring || q_vector->rx.ring || q_vector->cq.ring)
			napi_enable(&q_vector->napi);
	}
}

static int ne6x_up_complete(struct ne6x_adapter *adpt)
{
	struct ne6x_pf *pf = adpt->back;

	ne6x_adpt_configure_msix(adpt);

	clear_bit(NE6X_ADPT_DOWN, adpt->comm.state);
	ne6x_napi_enable_all(adpt);
	ne6x_adpt_enable_irq(adpt);

	if ((adpt->port_info->phy.link_info.link_info & NE6X_AQ_LINK_UP) && adpt->netdev) {
		ne6x_print_link_message(adpt, true);
		netif_tx_start_all_queues(adpt->netdev);
		netif_carrier_on(adpt->netdev);
	}

	/* On the next run of the service_task, notify any clients of the new
	 * opened netdev
	 */
	set_bit(NE6X_CLIENT_SERVICE_REQUESTED, pf->state);
	ne6x_linkscan_schedule(pf);

	return 0;
}

static void ne6x_napi_disable_all(struct ne6x_adapter *adpt)
{
	int q_idx;

	if (!adpt->netdev)
		return;

	for (q_idx = 0; q_idx < adpt->num_q_vectors; q_idx++) {
		struct ne6x_q_vector *q_vector = adpt->q_vectors[q_idx];

		if (q_vector->tx.ring || q_vector->rx.ring || q_vector->cq.ring)
			napi_disable(&q_vector->napi);
	}
}

static void ne6x_unmap_and_free_tx_resource(struct ne6x_ring *ring, struct ne6x_tx_buf *tx_buffer)
{
	if (tx_buffer->skb) {
		dev_kfree_skb_any(tx_buffer->skb);
		if (dma_unmap_len(tx_buffer, len))
			dma_unmap_single(ring->dev, dma_unmap_addr(tx_buffer, dma),
					 dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
	} else if (dma_unmap_len(tx_buffer, len)) {
		dma_unmap_page(ring->dev, dma_unmap_addr(tx_buffer, dma),
			       dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
	}

	tx_buffer->next_to_watch = NULL;
	tx_buffer->skb = NULL;
	dma_unmap_len_set(tx_buffer, len, 0);
}

void ne6x_clean_tx_ring(struct ne6x_ring *tx_ring)
{
	unsigned long bi_size;
	u16 i;

	/* ring already cleared, nothing to do */
	if (!tx_ring->tx_buf)
		return;

	/* Free all the Tx ring sk_buffs */
	for (i = 0; i < tx_ring->count; i++)
		ne6x_unmap_and_free_tx_resource(tx_ring, &tx_ring->tx_buf[i]);

	bi_size = sizeof(struct ne6x_tx_buf) * tx_ring->count;
	memset(tx_ring->tx_buf, 0, bi_size);

	/* Zero out the descriptor ring */
	memset(tx_ring->desc, 0, tx_ring->size);

	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;
	tx_ring->cq_last_expect = 0;

	if (!tx_ring->netdev)
		return;

	/* cleanup Tx queue statistics */
	netdev_tx_reset_queue(txring_txq(tx_ring));
}

void ne6x_clean_rx_ring(struct ne6x_ring *rx_ring)
{
	unsigned long bi_size;
	u16 i;

	/* ring already cleared, nothing to do */
	if (!rx_ring->rx_buf)
		return;

	if (rx_ring->skb) {
		dev_kfree_skb(rx_ring->skb);
		rx_ring->skb = NULL;
	}

	/* Free all the Rx ring sk_buffs */
	for (i = 0; i < rx_ring->count; i++) {
		struct ne6x_rx_buf *rx_bi = &rx_ring->rx_buf[i];

		if (!rx_bi->page)
			continue;

		/* Invalidate cache lines that may have been written to by
		 * device so that we avoid corrupting memory.
		 */
		dma_sync_single_range_for_cpu(rx_ring->dev, rx_bi->dma, rx_bi->page_offset,
					      rx_ring->rx_buf_len, DMA_FROM_DEVICE);

		/* free resources associated with mapping */
		dma_unmap_page_attrs(rx_ring->dev, rx_bi->dma, ne6x_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE, NE6X_RX_DMA_ATTR);

		__page_frag_cache_drain(rx_bi->page, rx_bi->pagecnt_bias);

		rx_bi->page = NULL;
		rx_bi->page_offset = 0;
	}

	bi_size = sizeof(struct ne6x_rx_buf) * rx_ring->count;
	memset(rx_ring->rx_buf, 0, bi_size);

	/* Zero out the descriptor ring */
	memset(rx_ring->desc, 0, rx_ring->size);

	rx_ring->next_to_alloc = 0;
	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;
	rx_ring->cq_last_expect = 0;
}

static void ne6x_clean_cq_ring(struct ne6x_ring *cq_ring)
{
	/* Zero out the descriptor ring */
	memset(cq_ring->desc, 0, cq_ring->size);

	cq_ring->next_to_clean = 0;
	cq_ring->next_to_use = 0;
}

void ne6x_down(struct ne6x_adapter *adpt)
{
	int i;

	/* It is assumed that the caller of this function
	 * sets the adpt->comm.state NE6X_ADPT_DOWN bit.
	 */
	if (adpt->netdev) {
		netif_carrier_off(adpt->netdev);
		netif_tx_disable(adpt->netdev);
	}

	ne6x_adpt_disable_irq(adpt);
	ne6x_adpt_restart_vp(adpt, true);
	ne6x_force_link_state(adpt, false);
	ne6x_napi_disable_all(adpt);

	for (i = 0; i < adpt->num_queue; i++) {
		ne6x_clean_tx_ring(adpt->tx_rings[i]);
		ne6x_clean_cq_ring(adpt->cq_rings[i]);
		ne6x_clean_rx_ring(adpt->rx_rings[i]);
	}
}

void ne6x_free_rx_resources(struct ne6x_ring *rx_ring)
{
	ne6x_clean_rx_ring(rx_ring);
	kfree(rx_ring->rx_buf);
	rx_ring->rx_buf = NULL;

	if (rx_ring->desc) {
		dma_free_coherent(rx_ring->dev, rx_ring->size, rx_ring->desc, rx_ring->dma);
		rx_ring->desc = NULL;
	}
}

static void ne6x_adpt_free_rx_resources(struct ne6x_adapter *adpt)
{
	int i;

	if (!adpt->rx_rings)
		return;

	for (i = 0; i < adpt->num_queue; i++) {
		if (adpt->rx_rings[i] && adpt->rx_rings[i]->desc)
			ne6x_free_rx_resources(adpt->rx_rings[i]);
	}
}

void ne6x_free_tx_resources(struct ne6x_ring *tx_ring)
{
	ne6x_clean_tx_ring(tx_ring);
	kfree(tx_ring->tx_buf);
	tx_ring->tx_buf = NULL;

	if (tx_ring->desc) {
		dma_free_coherent(tx_ring->dev, tx_ring->size, tx_ring->desc, tx_ring->dma);
		tx_ring->desc = NULL;
	}
}

void ne6x_free_cq_resources(struct ne6x_ring *cq_ring)
{
	ne6x_clean_cq_ring(cq_ring);
	if (cq_ring->desc) {
		dma_free_coherent(cq_ring->dev, cq_ring->size, cq_ring->desc, cq_ring->dma);
		cq_ring->desc = NULL;
	}
}

static void ne6x_adpt_free_tx_resources(struct ne6x_adapter *adpt)
{
	int i;

	if (adpt->tx_rings) {
		for (i = 0; i < adpt->num_queue; i++) {
			if (adpt->tx_rings[i] && adpt->tx_rings[i]->desc)
				ne6x_free_tx_resources(adpt->tx_rings[i]);
			kfree(adpt->tx_rings[i]->sgl);
		}
	}

	if (adpt->cq_rings) {
		for (i = 0; i < adpt->num_queue; i++) {
			if (adpt->cq_rings[i] && adpt->cq_rings[i]->desc)
				ne6x_free_cq_resources(adpt->cq_rings[i]);
		}
	}

	if (adpt->tg_rings) {
		for (i = 0; i < adpt->num_queue; i++) {
			if (adpt->tg_rings[i] && adpt->tg_rings[i]->desc)
				/* tg_ring == cq_ring */
				ne6x_free_cq_resources(adpt->tg_rings[i]);
		}
	}
}

int ne6x_up(struct ne6x_adapter *adpt)
{
	int err;

	ne6x_force_link_state(adpt, true);

	err = ne6x_adpt_configure(adpt);
	if (!err)
		err = ne6x_up_complete(adpt);

	return err;
}

int ne6x_adpt_open(struct ne6x_adapter *adpt)
{
	char int_name[NE6X_INT_NAME_STR_LEN];
	struct ne6x_pf *pf = adpt->back;
	int err;

	/* allocate descriptors */
	err = ne6x_adpt_setup_tx_resources(adpt);
	if (err)
		goto err_setup_tx;

	err = ne6x_adpt_setup_rx_resources(adpt);
	if (err)
		goto err_setup_rx;

	err = ne6x_adpt_configure(adpt);
	if (err)
		goto err_setup_rx;

	if (adpt->netdev) {
		snprintf(int_name, sizeof(int_name) - 1, "%s-%s", dev_driver_string(&pf->pdev->dev),
			 adpt->netdev->name);
		err = ne6x_adpt_request_irq(adpt, int_name);
		if (err)
			goto err_setup_rx;

		/* Notify the stack of the actual queue counts. */
		err = netif_set_real_num_tx_queues(adpt->netdev, adpt->num_queue);
		if (err)
			goto err_set_queues;

		/* When reducing the number of Tx queues, any pre-existing
		 * skbuffs might target a now removed queue. Older versions of
		 * the Linux kernel do not check for this, and it can result
		 * in a kernel panic. Avoid this by flushing all skbs now, so
		 * that we avoid attempting to transmit one that has an
		 * invalid queue mapping.
		 */
		qdisc_reset_all_tx_gt(adpt->netdev, 0);

		err = netif_set_real_num_rx_queues(adpt->netdev, adpt->num_queue);
		if (err)
			goto err_set_queues;
	} else {
		err = -EINVAL;
		goto err_setup_rx;
	}

	err = ne6x_up_complete(adpt);
	if (err)
		goto err_up_complete;

	ne6x_dev_set_tx_rx_state(adpt, true, true);
	return 0;

err_up_complete:
	ne6x_down(adpt);
err_set_queues:
	ne6x_adpt_free_irq(adpt);
err_setup_rx:
	ne6x_adpt_free_rx_resources(adpt);
err_setup_tx:
	ne6x_adpt_free_tx_resources(adpt);

	return err;
}

int ne6x_open(struct net_device *netdev)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	int err;

	netdev_info(netdev, "open !!!\n");
	set_bit(NE6X_ADPT_OPEN, adpt->comm.state);

	netif_carrier_off(netdev);

	if (ne6x_force_link_state(adpt, true))
		return -EAGAIN;

	err = ne6x_adpt_open(adpt);
	if (err)
		return err;

	ne6x_sync_features(netdev);

	ne6x_dev_set_if_state(adpt, NE6000_IF_INTERFACE_UP);

	return 0;
}

void ne6x_adpt_close(struct ne6x_adapter *adpt)
{
	struct ne6x_pf *pf = adpt->back;

	ne6x_dev_set_tx_rx_state(adpt, false, false);
	if (!test_and_set_bit(NE6X_ADPT_DOWN, adpt->comm.state))
		ne6x_down(adpt);

	ne6x_adpt_free_irq(adpt);
	ne6x_adpt_free_tx_resources(adpt);
	ne6x_adpt_free_rx_resources(adpt);
	set_bit(NE6X_CLIENT_SERVICE_REQUESTED, pf->state);
}

int ne6x_close(struct net_device *netdev)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	clear_bit(NE6X_ADPT_OPEN, adpt->comm.state);
	adpt->current_isup = false;
	adpt->current_speed = NE6X_LINK_SPEED_UNKNOWN;
	ne6x_adpt_close(adpt);
	if (test_bit(NE6X_ADPT_F_LINKDOWN_ON_CLOSE, adpt->flags))
		ne6x_dev_set_if_state(adpt, NE6000_IF_INTERFACE_DOWN);

	netdev_info(netdev, "close !!!\n");

	return 0;
}

static void ne6x_adpt_reinit_locked(struct ne6x_adapter *adpt)
{
	struct ne6x_pf *pf = adpt->back;

	WARN_ON(in_interrupt());
	while (test_and_set_bit(NE6X_CONFIG_BUSY, pf->state))
		usleep_range(1000, 2000);

	ne6x_down(adpt);
	ne6x_up(adpt);
	clear_bit(NE6X_CONFIG_BUSY, pf->state);
}

static int ne6x_change_mtu(struct net_device *netdev, int new_mtu)
{
	int max_frame = new_mtu + NE6X_PACKET_HDR_PAD;
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	if (new_mtu < NE6X_MIN_MTU_SIZE) {
		netdev_err(netdev, "mtu < MIN MTU size");
		return -EINVAL;
	}

	max_frame = new_mtu + ETH_HLEN + ETH_FCS_LEN;
	if (max_frame > NE6X_MAX_RXBUFFER) {
		netdev_err(netdev, "mtu > MAX MTU size");
		return -EINVAL;
	}

	netdev_info(netdev, "changing MTU from %d to %d\n", netdev->mtu, new_mtu);
	netdev->mtu = new_mtu;

	if (netif_running(netdev)) {
		if (adpt->back->num_alloc_vfs == 0)
			ne6x_adpt_reinit_locked(adpt);
	}

	return 0;
}

static void ne6x_tx_timeout(struct net_device *netdev, __always_unused unsigned int txqueue)
{
	struct ne6x_ring *tx_ring = NULL;
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_pf *pf = adpt->back;
	unsigned int hung_queue = 0;
	u64 head, intr, tail;

	hung_queue = txqueue;
	tx_ring = adpt->tx_rings[hung_queue];
	pf->tx_timeout_count++;

	if (time_after(jiffies, (pf->tx_timeout_last_recovery + HZ * 20)))
		pf->tx_timeout_recovery_level = 1; /* reset after some time */
	else if (time_before(jiffies, (pf->tx_timeout_last_recovery + netdev->watchdog_timeo)))
		return; /* don't do any new action before the next timeout */

	/* don't kick off another recovery if one is already pending */
	if (test_and_set_bit(NE6X_TIMEOUT_RECOVERY_PENDING, pf->state))
		return;

	if (tx_ring) {
		if (tx_ring->reg_idx < NE6X_PF_VP0_NUM) {
			head = rd64(&pf->hw,
				    NE6X_VPINT_DYN_CTLN(tx_ring->reg_idx, NE6X_SQ_HD_POINTER));
			/* Read interrupt register */
			intr = rd64(&pf->hw, NE6X_VPINT_DYN_CTLN(tx_ring->reg_idx, NE6X_VP_INT));
			tail = rd64(&pf->hw,
				    NE6X_VPINT_DYN_CTLN(tx_ring->reg_idx,
							NE6X_SQ_TAIL_POINTER));
		} else {
			head = rd64_bar4(&pf->hw,
					 NE6X_PFINT_DYN_CTLN(tx_ring->reg_idx -
							     NE6X_PF_VP0_NUM,
							     NE6X_SQ_HD_POINTER));
			intr = rd64_bar4(&pf->hw,
					 NE6X_PFINT_DYN_CTLN(tx_ring->reg_idx -
							     NE6X_PF_VP0_NUM,
							     NE6X_VP_INT));
			tail = rd64_bar4(&pf->hw,
					 NE6X_PFINT_DYN_CTLN(tx_ring->reg_idx -
							     NE6X_PF_VP0_NUM,
							     NE6X_SQ_TAIL_POINTER));
		}

		netdev_info(netdev, "tx_timeout: adapter: %u, Q: %u, NTC: 0x%x, HEAD: 0x%llx, NTU: 0x%x, TAIL: 0x%llx, INTR: 0x%llx\n",
			    adpt->idx, hung_queue, tx_ring->next_to_clean, head,
			    tx_ring->next_to_use, tail, intr);
	}

	pf->tx_timeout_last_recovery = jiffies;
	netdev_info(netdev, "tx_timeout recovery level %d, hung_queue %d\n",
		    pf->tx_timeout_recovery_level, hung_queue);

	switch (pf->tx_timeout_recovery_level) {
	case 1:
		set_bit(NE6X_ADPT_RECOVER, adpt->comm.state);
		set_bit(NE6X_PF_RESET_REQUESTED, pf->state);
		set_bit(NE6X_RESET_INTR_RECEIVED, pf->state);
		break;
	case 2:
		set_bit(NE6X_CORE_RESET_REQUESTED, pf->state);
		break;
	default:
		netdev_err(netdev, "tx_timeout recovery unsuccessful, device is in non-recoverable state.\n");
		set_bit(NE6X_DOWN_REQUESTED, pf->state);
		set_bit(NE6X_ADPT_DOWN_REQUESTED, adpt->comm.state);
		break;
	}

	ne6x_service_event_schedule(pf);
	pf->tx_timeout_recovery_level++;
}

static void ne6x_get_netdev_stats_struct_tx(struct ne6x_ring *ring, struct rtnl_link_stats64 *stats)
{
	u64 bytes, packets;
	unsigned int start;

	do {
		start = u64_stats_fetch_begin(&ring->syncp);
		packets = ring->stats.packets;
		bytes = ring->stats.bytes;
	} while (u64_stats_fetch_retry(&ring->syncp, start));

	stats->tx_packets += packets;
	stats->tx_bytes += bytes;
}

struct rtnl_link_stats64 *ne6x_get_adpt_stats_struct(struct ne6x_adapter *adpt)
{
	return &adpt->net_stats;
}

static void ne6x_get_netdev_stats_struct(struct net_device *netdev, struct rtnl_link_stats64 *stats)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct rtnl_link_stats64 *adpt_stats = ne6x_get_adpt_stats_struct(adpt);
	struct ne6x_ring *tx_ring, *rx_ring;
	u64 bytes, packets;
	unsigned int start;
	int i;

	if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state))
		return;

	if (!adpt->tx_rings)
		return;

	rcu_read_lock();
	for (i = 0; i < adpt->num_queue; i++) {
		tx_ring = READ_ONCE(adpt->tx_rings[i]);
		if (!tx_ring)
			continue;

		ne6x_get_netdev_stats_struct_tx(tx_ring, stats);
		rx_ring = &tx_ring[2];

		do {
			start = u64_stats_fetch_begin(&rx_ring->syncp);
			packets = rx_ring->stats.packets;
			bytes = rx_ring->stats.bytes;
		} while (u64_stats_fetch_retry(&rx_ring->syncp, start));

		stats->rx_packets += packets;
		stats->rx_bytes += bytes;
	}

	adpt_stats->rx_dropped = 0;
	rcu_read_unlock();

	/* following stats updated by ne6x_watchdog_subtask() */
	stats->multicast = adpt_stats->multicast;
	stats->tx_errors = adpt_stats->tx_errors;
	stats->tx_dropped = adpt_stats->tx_dropped;
	stats->rx_errors = adpt_stats->rx_errors;
	stats->rx_dropped = adpt_stats->rx_dropped;
	stats->rx_crc_errors = adpt_stats->rx_crc_errors;
	stats->rx_length_errors = adpt_stats->rx_length_errors;
}

void ne6x_update_pf_stats(struct ne6x_adapter *adpt)
{
	struct rtnl_link_stats64 *ons;
	struct rtnl_link_stats64 *ns; /* netdev stats */
	struct ne6x_eth_stats *oes;
	struct ne6x_eth_stats *es; /* device's eth stats */
	struct ne6x_ring *tx_ring;
	struct ne6x_ring *rx_ring;
	u32 tx_restart, tx_busy;
	u32 rx_page, rx_buf;
	u64 bytes, packets;
	unsigned int start;
	struct vf_stat vf_stat;
	u64 tx_linearize;
	u64 tx_force_wb;
	u64 rx_p, rx_b;
	u64 tx_p, tx_b;
	u64 tx_e, rx_e;
	u64 rx_l, rx_c;
	u16 i;

	if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state))
		return;

	ns = ne6x_get_adpt_stats_struct(adpt);
	ons = &adpt->net_stats_offsets;
	es = &adpt->eth_stats;
	oes = &adpt->eth_stats_offsets;

	rx_p = 0;
	rx_b = 0;
	tx_p = 0;
	tx_b = 0;
	rx_e = 0;
	tx_e = 0;
	rx_c = 0;
	rx_l = 0;
	tx_force_wb = 0;
	tx_linearize = 0;
	tx_busy = 0;
	tx_restart = 0;
	rx_page = 0;
	rx_buf = 0;

	rcu_read_lock();
	for (i = 0; i < adpt->num_queue; i++) {
		/* locate Tx ring */
		tx_ring = READ_ONCE(adpt->tx_rings[i]);

		do {
			start = u64_stats_fetch_begin(&tx_ring->syncp);
			packets = tx_ring->stats.packets;
			bytes = tx_ring->stats.bytes;
		} while (u64_stats_fetch_retry(&tx_ring->syncp, start));

		tx_b += bytes;
		tx_p += packets;
		tx_restart += tx_ring->tx_stats.restart_q;
		tx_busy += tx_ring->tx_stats.tx_busy;
		tx_linearize += tx_ring->tx_stats.tx_linearize;
		tx_e += tx_ring->tx_stats.csum_err + tx_ring->tx_stats.tx_drop_addr +
			tx_ring->tx_stats.tx_pcie_read_err;

		rx_ring = &tx_ring[2];

		do {
			start = u64_stats_fetch_begin(&rx_ring->syncp);
			packets = rx_ring->stats.packets;
			bytes = rx_ring->stats.bytes;
		} while (u64_stats_fetch_retry(&rx_ring->syncp, start));

		rx_b += bytes;
		rx_p += packets;
		rx_buf += rx_ring->rx_stats.alloc_buf_failed;
		rx_page += rx_ring->rx_stats.alloc_page_failed;
		rx_e += rx_ring->rx_stats.csum_err + rx_ring->rx_stats.rx_err +
			rx_ring->rx_stats.rx_mem_error;
		rx_l += rx_ring->rx_stats.rx_mem_error;
	}

	rcu_read_unlock();

	adpt->tx_restart = tx_restart;
	adpt->tx_busy = tx_busy;
	adpt->rx_page_failed = rx_page;
	adpt->rx_buf_failed = rx_buf;

	ns->rx_packets = rx_p;
	ns->rx_bytes = rx_b;
	ns->tx_packets = tx_p;
	ns->tx_bytes = tx_b;
	ns->tx_errors = tx_e;
	ns->rx_errors = rx_e;
	ns->rx_length_errors = rx_l;
	ns->rx_crc_errors = rx_c;

	ns->rx_dropped = 0;
	ne6x_dev_get_vf_stat(adpt, &vf_stat);
	es->rx_broadcast = vf_stat.rx_broadcast_pkts;
	es->rx_miss = vf_stat.rx_drop_pkts;
	es->rx_multicast = vf_stat.rx_multicast_pkts;
	es->rx_unicast  = vf_stat.rx_unicast_pkts;
	es->tx_broadcast = vf_stat.tx_broadcast_pkts;
	es->tx_multicast = vf_stat.tx_multicast_pkts;
	es->tx_unicast   = vf_stat.tx_unicast_pkts;
	es->rx_malform   = vf_stat.rx_malform_pkts;
	es->tx_malform   = vf_stat.tx_malform_pkts;
}

static void ne6x_netpoll(struct net_device *netdev)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	int i;

	/* if interface is down do nothing */
	if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state))
		return;

	for (i = 0; i < adpt->num_q_vectors; i++)
		ne6x_msix_clean_rings(0, adpt->q_vectors[i]);
}

static int ne6x_set_mac(struct net_device *netdev, void *p)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_mac_info *mac = &adpt->port_info->mac;
	struct sockaddr *addr = p;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	if (ether_addr_equal(netdev->dev_addr, addr->sa_data)) {
		netdev_info(netdev, "already using mac address %pM\n", addr->sa_data);
		return 0;
	}

	if (ether_addr_equal(mac->perm_addr, addr->sa_data))
		netdev_info(netdev, "returning to hw mac address %pM\n", mac->perm_addr);
	else
		netdev_info(netdev, "set new mac address %pM\n", addr->sa_data);

	ne6x_adpt_del_mac(adpt, mac->perm_addr, true);
	eth_hw_addr_set(netdev, addr->sa_data);
	memcpy(mac->perm_addr, addr->sa_data, netdev->addr_len);
	ne6x_adpt_add_mac(adpt, mac->perm_addr, true);
	ne6x_dev_set_port_mac(adpt, mac->perm_addr);

	return 0;
}

static int ne6x_vlan_rx_add_vid(struct net_device *netdev, __be16 proto, u16 vid)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_vlan vlan;
	int ret;

	netdev_info(netdev, "vlan_rx_add_vid proto = 0x%04X vid = %d\n", proto, vid);

	if (!vid)
		return 0;

	/* Add a switch rule for this VLAN ID so its corresponding VLAN tagged
	 * packets aren't pruned by the device's internal switch on Rx
	 */
	vlan = NE6X_VLAN(be16_to_cpu(proto), vid, 0);

	if (vlan.vid > 0 && vlan.vid < (VLAN_N_VID - 1)) {
		ret = ne6x_adpt_add_vlan(adpt, vlan);
		if (!ret)
			set_bit(NE6X_ADPT_VLAN_FLTR_CHANGED, adpt->comm.state);
	} else {
		return -EINVAL;
	}

	return ret;
}

static int ne6x_vlan_rx_kill_vid(struct net_device *netdev, __be16 proto, u16 vid)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_vlan vlan;
	int ret;

	netdev_info(netdev, "vlan_rx_add_vid proto = 0x%04X vid = %d\n", proto, vid);

	if (!vid)
		return 0;

	/* Make sure VLAN delete is successful before updating VLAN
	 * information
	 */
	vlan = NE6X_VLAN(be16_to_cpu(proto), vid, 0);
	ret = ne6x_adpt_del_vlan(adpt, vlan);
	if (ret)
		return ret;

	set_bit(NE6X_ADPT_VLAN_FLTR_CHANGED, adpt->comm.state);

	return 0;
}

static struct mac_addr_node *ne6x_find_addr(struct ne6x_adapter *adpt,
					    const u8 *macaddr, bool is_unicast)
{
	struct mac_addr_head *addr_head = NULL;
	struct mac_addr_node *addr_node = NULL;

	if (!macaddr)
		return NULL;

	if (is_unicast)
		addr_head = &adpt->uc_mac_addr;
	else
		addr_head = &adpt->mc_mac_addr;

	list_for_each_entry(addr_node, &addr_head->list, list) {
		if (ether_addr_equal(macaddr, addr_node->addr))
			return addr_node;
	}

	return NULL;
}

int ne6x_adpt_add_mac(struct ne6x_adapter *adpt, const u8 *addr, bool is_unicast)
{
	int (*ne6x_vc_cfg_mac)(struct ne6x_adapter *adpt, u8 *mac);
	struct mac_addr_head *addr_head = NULL;
	struct mac_addr_node *addr_node = NULL;
	int rc = 0;

	if (!addr)
		return -EINVAL;

	if (is_unicast) {
		addr_head = &adpt->uc_mac_addr;
		ne6x_vc_cfg_mac = ne6x_dev_add_unicast;
	} else {
		addr_head = &adpt->mc_mac_addr;
		ne6x_vc_cfg_mac = ne6x_dev_add_multicast;
	}

	mutex_lock(&addr_head->mutex);

	if (ne6x_find_addr(adpt, addr, is_unicast))
		goto out_unlock;

	/* Update MAC list value */
	addr_node = kzalloc(sizeof(*addr_node), GFP_KERNEL);
	if (!addr_node) {
		rc = -ENOMEM;
		goto out_unlock;
	}

	ether_addr_copy(addr_node->addr, addr);
	list_add_tail(&addr_node->list, &addr_head->list);
	/* Send the value of the updated MAC linked list to the SDK */
	ne6x_vc_cfg_mac(adpt, addr_node->addr);

out_unlock:
	mutex_unlock(&addr_head->mutex);

	return rc;
}

int ne6x_adpt_del_mac(struct ne6x_adapter *adpt, const u8 *addr, bool is_unicast)
{
	int (*ne6x_vc_cfg_mac)(struct ne6x_adapter *adpt, u8 *mac);
	struct mac_addr_head *addr_head = NULL;
	struct mac_addr_node *addr_node = NULL;

	if (is_unicast) {
		addr_head = &adpt->uc_mac_addr;
		ne6x_vc_cfg_mac = ne6x_dev_del_unicast;
	} else {
		addr_head = &adpt->mc_mac_addr;
		ne6x_vc_cfg_mac = ne6x_dev_del_multicast;
	}

	mutex_lock(&addr_head->mutex);
	addr_node = ne6x_find_addr(adpt, addr, is_unicast);
	if (!addr_node)
		goto out_unlock;

	list_del(&addr_node->list);
	ne6x_vc_cfg_mac(adpt, addr_node->addr);
	kfree(addr_node);

out_unlock:
	mutex_unlock(&addr_head->mutex);

	return 0;
}

static int ne6x_mc_addr_sync(struct net_device *netdev, const u8 *addr)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	return ne6x_adpt_add_mac(adpt, addr, false);
}

static int ne6x_mc_addr_unsync(struct net_device *netdev, const u8 *addr)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	return ne6x_adpt_del_mac(adpt, addr, false);
}

static int ne6x_uc_addr_sync(struct net_device *netdev, const u8 *addr)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	return ne6x_adpt_add_mac(adpt, addr, true);
}

static int ne6x_uc_addr_unsync(struct net_device *netdev, const u8 *addr)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	return ne6x_adpt_del_mac(adpt, addr, true);
}

void ne6x_adpt_clear_ddos(struct ne6x_pf *pf)
{
	u32 data;

	ne6x_reg_get_user_data(pf, NP_USER_DATA_HW_FLAGS, &data);
	data &= ~NE6X_F_DDOS_ENABLED;
	ne6x_reg_set_user_data(pf, NP_USER_DATA_HW_FLAGS, data);
}

int ne6x_adpt_clear_mac_vlan(struct ne6x_adapter *adpt)
{
	struct mac_addr_node *temp_node = NULL, *addr_node = NULL;
	struct ne6x_vlan_filter *f = NULL, *temp_filter = NULL;
	struct mac_addr_head *addr_head = NULL;
	struct list_head temp_header;
	int ret = 0;

	INIT_LIST_HEAD(&temp_header);
	spin_lock_bh(&adpt->mac_vlan_list_lock);
	list_for_each_entry(f, &adpt->vlan_filter_list, list) {
		if (f->vlan.vid) {
			temp_filter = kzalloc(sizeof(*temp_filter), GFP_ATOMIC);
			memcpy(temp_filter, f, sizeof(struct ne6x_vlan_filter));
			list_add_tail(&temp_filter->list, &temp_header);
		}
	}
	spin_unlock_bh(&adpt->mac_vlan_list_lock);

	list_for_each_entry_safe(f, temp_filter, &temp_header, list) {
		if (f->vlan.vid)
			ret |= ne6x_adpt_del_vlan(adpt, f->vlan);

		list_del(&f->list);
		kfree(f);
	}

	addr_head = &adpt->uc_mac_addr;
	mutex_lock(&addr_head->mutex);
	list_for_each_entry_safe(addr_node, temp_node, &addr_head->list, list) {
		ret |= ne6x_dev_del_unicast(adpt, addr_node->addr);
		list_del(&addr_node->list);
		kfree(addr_node);
	}
	mutex_unlock(&addr_head->mutex);

	addr_head = &adpt->mc_mac_addr;
	mutex_lock(&addr_head->mutex);
	list_for_each_entry_safe(addr_node, temp_node, &addr_head->list, list) {
		ret |= ne6x_dev_del_multicast(adpt, addr_node->addr);
		list_del(&addr_node->list);
		kfree(addr_node);
	}
	mutex_unlock(&addr_head->mutex);

	return ret;
}

static void ne6x_set_rx_mode_task(struct work_struct *work)
{
	struct ne6x_adapter *adpt = container_of(work, struct ne6x_adapter, set_rx_mode_task);
	struct net_device *netdev = adpt->netdev;

	/* Check for Promiscuous modes */
	if (netdev->flags & IFF_PROMISC) {
		ne6x_dev_set_uc_promiscuous_enable(adpt, true);
		ne6x_dev_set_mc_promiscuous_enable(adpt, true);
	} else {
		ne6x_dev_set_uc_promiscuous_enable(adpt, false);
		ne6x_dev_set_mc_promiscuous_enable(adpt, false);
		/* Check for All Multicast modes */
		if (netdev->flags & IFF_ALLMULTI)
			ne6x_dev_set_mc_promiscuous_enable(adpt, true);
		else
			__dev_mc_sync(netdev, ne6x_mc_addr_sync, ne6x_mc_addr_unsync);
	}

	__dev_uc_sync(netdev, ne6x_uc_addr_sync, ne6x_uc_addr_unsync);
}

static void ne6x_set_rx_mode(struct net_device *netdev)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	if (!adpt)
		return;

	queue_work(ne6x_wq, &adpt->set_rx_mode_task);
}

static int ne6x_set_tx_maxrate(struct net_device *netdev, int queue_index, u32 maxrate)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	if (!adpt)
		return -1;

	return 0;
}

#define NETIF_VLAN_OFFLOAD_FEATURES	(NETIF_F_HW_VLAN_CTAG_RX | \
					 NETIF_F_HW_VLAN_CTAG_TX | \
					 NETIF_F_HW_VLAN_STAG_RX | \
					 NETIF_F_HW_VLAN_STAG_TX)

#define NETIF_VLAN_FILTERING_FEATURES	(NETIF_F_HW_VLAN_CTAG_FILTER | \
					 NETIF_F_HW_VLAN_STAG_FILTER)

#define NETIF_UDP_TNL_FEATURES	(NETIF_F_GSO_UDP_TUNNEL | \
				 NETIF_F_GSO_UDP_TUNNEL_CSUM)

static netdev_features_t ne6x_fix_features(struct net_device *netdev, netdev_features_t features)
{
	if (features & NETIF_F_HW_VLAN_CTAG_RX)
		features &= ~NETIF_F_HW_VLAN_STAG_RX;

	if (features & NETIF_F_HW_VLAN_STAG_RX)
		features &= ~NETIF_F_HW_VLAN_CTAG_RX;

	if (features & NETIF_F_HW_VLAN_CTAG_TX)
		features &= ~NETIF_F_HW_VLAN_STAG_TX;

	if (features & NETIF_F_HW_VLAN_STAG_TX)
		features &= ~NETIF_F_HW_VLAN_CTAG_TX;

	if (features & NETIF_VLAN_FILTERING_FEATURES)
		features |= NETIF_VLAN_FILTERING_FEATURES;

	return features;
}

static int ne6x_set_features(struct net_device *netdev, netdev_features_t features)
{
	netdev_features_t changed = features ^ netdev->features;
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	u32 value;

	value = ne6x_dev_get_features(adpt);

	if (changed & (NETIF_F_GSO_UDP_TUNNEL | NETIF_F_GSO_UDP_TUNNEL_CSUM)) {
		if (features & NETIF_F_GSO_UDP_TUNNEL_CSUM)
			value |= NE6X_F_TX_UDP_TNL_SEG;
		else
			value &= ~NE6X_F_TX_UDP_TNL_SEG;
	}

	if (changed & NETIF_VLAN_OFFLOAD_FEATURES || changed & NETIF_VLAN_FILTERING_FEATURES) {
		/* keep cases separate because one ethertype for offloads can be
		 * disabled at the same time as another is disabled, so check for an
		 * enabled ethertype first, then check for disabled. Default to
		 * ETH_P_8021Q so an ethertype is specified if disabling insertion and
		 * stripping.
		 */

		if (features & NETIF_F_HW_VLAN_CTAG_RX)
			value |= NE6X_F_RX_VLAN_STRIP;
		else
			value &= ~NE6X_F_RX_VLAN_STRIP;

		if (features & NETIF_F_HW_VLAN_CTAG_TX)
			value |= NE6X_F_TX_VLAN;
		else
			value &= ~NE6X_F_TX_VLAN;

		if (features & NETIF_F_HW_VLAN_STAG_RX)
			value |= NE6X_F_RX_QINQ_STRIP;
		else
			value &= ~NE6X_F_RX_QINQ_STRIP;

		if (features & NETIF_F_HW_VLAN_STAG_TX)
			value |= NE6X_F_TX_QINQ;
		else
			value &= ~NE6X_F_TX_QINQ;

		if (features & (NETIF_F_HW_VLAN_CTAG_FILTER | NETIF_F_HW_VLAN_STAG_FILTER))
			value |= NE6X_F_RX_VLAN_FILTER;
		else
			value &= ~NE6X_F_RX_VLAN_FILTER;
	}

	if (changed & (NETIF_F_RXCSUM | NETIF_F_LRO)) {
		if (features & NETIF_F_RXCSUM)
			value |= NE6X_OFFLOAD_RXCSUM;
		else
			value &= ~NE6X_OFFLOAD_RXCSUM;

		/* update hardware LRO capability accordingly */
		if (features & NETIF_F_LRO)
			value |= NE6X_OFFLOAD_LRO;
		else
			value &= ~NE6X_OFFLOAD_LRO;
	}

	if (changed & (NETIF_F_TSO6 | NETIF_F_TSO)) {
		if (features & (NETIF_F_TSO | NETIF_F_TSO6))
			value |= NE6X_OFFLOAD_TSO;
		else
			value &= ~NE6X_OFFLOAD_TSO;
	}

	if (changed & (NETIF_F_TSO6 | NETIF_F_TSO)) {
		if (features & (NETIF_F_TSO | NETIF_F_TSO6))
			value |= NE6X_OFFLOAD_TSO;
		else
			value &= ~NE6X_OFFLOAD_TSO;
	}

	if (changed & NETIF_F_GSO_UDP) {
		if (features & NETIF_F_GSO_UDP)
			value |= NE6X_OFFLOAD_UFO;
		else
			value &= ~NE6X_OFFLOAD_UFO;
	}

	if (changed & NETIF_F_IP_CSUM) {
		if (features & NETIF_F_IP_CSUM)
			value |= NE6X_OFFLOAD_TXCSUM;
		else
			value &= ~NE6X_OFFLOAD_TXCSUM;
	}

	if (changed & NETIF_F_RXHASH) {
		if (features & NETIF_F_RXHASH)
			value |= NE6X_OFFLOAD_RSS;
		else
			value &= ~NE6X_OFFLOAD_RSS;
	}

	if (changed & NETIF_F_HW_L2FW_DOFFLOAD) {
		if (features & NETIF_F_HW_L2FW_DOFFLOAD)
			value |= NE6X_OFFLOAD_L2;
		else
			value &= ~NE6X_OFFLOAD_L2;
	}

	if (changed & NETIF_F_SCTP_CRC) {
		if (features & NETIF_F_SCTP_CRC)
			value |= NE6X_OFFLOAD_SCTP_CSUM;
		else
			value &= ~NE6X_OFFLOAD_SCTP_CSUM;
	}

	if (changed & NETIF_F_NTUPLE) {
		if (features & NETIF_F_NTUPLE)
			value |= NE6X_F_FLOW_STEERING;
		else
			value &= ~NE6X_F_FLOW_STEERING;
	}
	return ne6x_dev_set_features(adpt, value);
}

static netdev_features_t ne6x_features_check(struct sk_buff *skb, struct net_device *dev,
					     netdev_features_t features)
{
	size_t len;

	/* No point in doing any of this if neither checksum nor GSO are
	 * being requested for this frame.  We can rule out both by just
	 * checking for CHECKSUM_PARTIAL
	 */
	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return features;

	/* We cannot support GSO if the MSS is going to be less than
	 * 64 bytes.  If it is then we need to drop support for GSO.
	 */
	if (skb_is_gso(skb) && (skb_shinfo(skb)->gso_size < 64))
		features &= ~NETIF_F_GSO_MASK;

	/* MACLEN can support at most 63 words */
	len = skb_network_header(skb) - skb->data;
	if (len & ~(63 * 2))
		goto out_err;

	/* IPLEN and EIPLEN can support at most 127 dwords */
	len = skb_transport_header(skb) - skb_network_header(skb);
	if (len & ~(127 * 4))
		goto out_err;

	/* No need to validate L4LEN as TCP is the only protocol with a
	 * a flexible value and we support all possible values supported
	 * by TCP, which is at most 15 dwords
	 */
	return features;

out_err:
	return features & ~(NETIF_F_CSUM_MASK | NETIF_F_GSO_MASK);
}

int ne6x_link_speed_to_rate(int link_speed)
{
	switch (link_speed) {
	case NE6X_LINK_SPEED_100GB:
		return SPEED_100000;
	case NE6X_LINK_SPEED_40GB:
		return SPEED_40000;
	case NE6X_LINK_SPEED_25GB:
		return SPEED_25000;
	case NE6X_LINK_SPEED_10GB:
		return SPEED_10000;
	default:
		return SPEED_25000;
	}
}

int ne6x_validata_tx_rate(struct ne6x_adapter *adpt, int vf_id, int min_tx_rate, int max_tx_rate)
{
	if (!adpt)
		return -EINVAL;

	if (min_tx_rate) {
		dev_err(&adpt->back->pdev->dev, "Invalid min tx rate (%d) (greater than 0) specified for VF %d.\n",
			min_tx_rate, vf_id);
		return -EINVAL;
	}

	if (max_tx_rate > ne6x_link_speed_to_rate(adpt->port_info->phy.link_info.link_speed)) {
		dev_err(&adpt->back->pdev->dev, "Invalid max tx rate (%d) (greater than link_speed) specified for VF %d.\n",
			max_tx_rate, vf_id);
		return -EINVAL;
	}

	return 0;
}

static struct ne6x_key_filter *ne6x_find_key(struct ne6x_pf *pf, struct ne6x_key key)
{
	struct ne6x_key_filter *f;

	list_for_each_entry(f, &pf->key_filter_list, list) {
		if (f->key.pi == key.pi && ether_addr_equal(f->key.mac_addr, key.mac_addr))
			return f;
	}

	return NULL;
}

struct ne6x_key_filter *ne6x_add_key_list(struct ne6x_pf *pf, struct ne6x_key key)
{
	struct ne6x_key_filter *f = NULL;

	spin_lock_bh(&pf->key_list_lock);

	f = ne6x_find_key(pf, key);
	if (!f) {
		f = kzalloc(sizeof(*f), GFP_ATOMIC);
		if (!f)
			goto clearout;

		f->key = key;

		list_add_tail(&f->list, &pf->key_filter_list);
		f->add = true;
	} else {
		f->refcnt++;
	}

clearout:
	spin_unlock_bh(&pf->key_list_lock);

	return f;
}

int ne6x_del_key_list(struct ne6x_pf *pf, struct ne6x_key key)
{
	struct ne6x_key_filter *f;

	spin_lock_bh(&pf->key_list_lock);

	f = ne6x_find_key(pf, key);
	if (f) {
		if (f->refcnt) {
			f->refcnt--;
			spin_unlock_bh(&pf->key_list_lock);
			return -1;
		}

		list_del(&f->list);
		kfree(f);
	}

	spin_unlock_bh(&pf->key_list_lock);

	return 0;
}

int ne6x_add_key(struct ne6x_adapter *adpt, u8 *mac_addr, u8 size)
{
	struct ne6x_key_filter *f;
	struct ne6x_key key;

	memset(&key, 0, sizeof(struct ne6x_key));
	key.pi = ADPT_LPORT(adpt);
	memcpy(key.mac_addr, mac_addr, size);

	f = ne6x_add_key_list(adpt->back, key);
	if (f->refcnt)
		return -1;

	return 0;
}

int ne6x_del_key(struct ne6x_adapter *adpt, u8 *mac_addr, u8 size)
{
	struct ne6x_key key;
	int ret;

	memset(&key, 0, sizeof(struct ne6x_key));
	key.pi = ADPT_LPORT(adpt);
	memcpy(key.mac_addr, mac_addr, size);

	ret = ne6x_del_key_list(adpt->back, key);
	if (ret)
		return -1;

	return 0;
}

static struct ne6x_vlan_filter *ne6x_find_vlan(struct ne6x_adapter *adpt, struct ne6x_vlan vlan)
{
	struct ne6x_vlan_filter *f;

	list_for_each_entry(f, &adpt->vlan_filter_list, list) {
		if (f->vlan.vid == vlan.vid && f->vlan.tpid == vlan.tpid)
			return f;
	}

	return NULL;
}

struct ne6x_vlan_filter *ne6x_add_vlan_list(struct ne6x_adapter *adpt, struct ne6x_vlan vlan)
{
	struct ne6x_vlan_filter *f = NULL;

	spin_lock_bh(&adpt->mac_vlan_list_lock);

	f = ne6x_find_vlan(adpt, vlan);
	if (!f) {
		f = kzalloc(sizeof(*f), GFP_ATOMIC);
		if (!f)
			goto clearout;

		f->vlan = vlan;

		list_add_tail(&f->list, &adpt->vlan_filter_list);
		f->add = true;
	} else {
		f->refcnt++;
	}

clearout:
	spin_unlock_bh(&adpt->mac_vlan_list_lock);

	return f;
}

int ne6x_del_vlan_list(struct ne6x_adapter *adpt, struct ne6x_vlan vlan)
{
	struct ne6x_vlan_filter *f;

	spin_lock_bh(&adpt->mac_vlan_list_lock);

	f = ne6x_find_vlan(adpt, vlan);
	if (f) {
		if (f->refcnt) {
			f->refcnt--;
			spin_unlock_bh(&adpt->mac_vlan_list_lock);
			return -1;
		}

		list_del(&f->list);
		kfree(f);
	}

	spin_unlock_bh(&adpt->mac_vlan_list_lock);

	return 0;
}

int ne6x_adpt_add_vlan(struct ne6x_adapter *adpt, struct ne6x_vlan vlan)
{
	struct ne6x_vlan_filter *f = ne6x_add_vlan_list(adpt, vlan);

	if (f->refcnt == 0)
		ne6x_dev_vlan_add(adpt, &vlan);

	return 0;
}

int ne6x_adpt_del_vlan(struct ne6x_adapter *adpt, struct ne6x_vlan vlan)
{
	int ret;

	ret = ne6x_del_vlan_list(adpt, vlan);
	if (ret == 0)
		ne6x_dev_vlan_del(adpt, &vlan);

	return 0;
}

int ne6x_set_vf_port_vlan(struct net_device *netdev, int vf_id, u16 vlan_id,
			  u8 qos, __be16 vlan_proto)
{
	struct ne6x_netdev_priv *np = netdev_priv(netdev);
	struct ne6x_pf *pf = ne6x_netdev_to_pf(netdev);
	u16 local_vlan_proto = ntohs(vlan_proto);
	u16 vid_temp = 0, tpid_temp = 0;
	struct ne6x_vlan vlan;
	struct ne6x_adapter *adpt;
	struct device *dev;
	struct ne6x_vf *vf;
	int lport;

	dev = ne6x_pf_to_dev(pf);

	if (vf_id < 0 || vf_id >= pf->num_alloc_vfs / 2 || vlan_id >= (VLAN_N_VID - 1) || qos > 7) {
		dev_err(dev, "Invalid Port VLAN parameters for VF %d,vlan ID %d, QoS %d\n",
			vf_id, vlan_id, qos);
		return -EINVAL;
	}

	if (!ne6x_is_supported_port_vlan_proto(&pf->hw, local_vlan_proto)) {
		dev_err(dev, "VF VLAN protocol 0x%04x is not supported\n",
			local_vlan_proto);
		return -EPROTONOSUPPORT;
	}

	lport = ADPT_LPORT(np->adpt);
	vf_id += (pf->num_alloc_vfs / 2) * lport;

	vf = ne6x_get_vf_by_id(pf, vf_id);
	if (!vf)
		return -EINVAL;

	vf->port_vlan_info = NE6X_VLAN(local_vlan_proto, vlan_id, qos);
	if (vf->port_vlan_info.prio || vf->port_vlan_info.vid)
		dev_info(dev, "Setting VLAN %u, QoS %u, TPID 0x%04x on VF %d\n",
			 vlan_id, qos, local_vlan_proto, vf_id);
	else
		dev_info(dev, "Clearing port VLAN on VF %d\n", vf_id);

	adpt = vf->adpt;

	dev_info(dev, "%s: net_name:%s  TPID:%08x vlan_id:%d qos:%d lport:%d vport:%d vlan_id:%d tpid:%04x %d\n",
		 __func__, netdev->name, local_vlan_proto, vlan_id, qos, ADPT_LPORT(adpt),
		 ADPT_VPORT(adpt), vf->port_vlan_info.vid, vf->port_vlan_info.tpid, vf->vfp_vid);

	vlan = NE6X_VLAN(local_vlan_proto, vlan_id, qos);

	if (vlan.vid == 0) {
		if (vf->vfp_tpid == vlan.tpid) {
			vlan.vid = vf->vfp_vid;
			vlan.tpid = vf->vfp_tpid;
			vf->vfp_vid = 0;
			vf->vfp_tpid = 0;
			ne6x_dev_del_vf_qinq(vf, vlan.tpid, vlan.vid);
			ne6x_adpt_del_vlan(vf->adpt, vlan);
		} else {
			vlan.vid = vf->vfp_vid;
			vlan.tpid = vf->vfp_tpid;
			vf->vfp_vid = 0;
			vf->vfp_tpid = 0;
			ne6x_dev_del_vf_qinq(vf, vlan.tpid, vlan.vid);
			ne6x_adpt_del_vlan(vf->adpt, vlan);
		}

	} else if (vlan.vid > 0 && vlan.vid < (VLAN_N_VID - 1)) {
		vid_temp = vlan.vid;
		tpid_temp = vlan.tpid;
		vlan.vid = vf->vfp_vid;
		vlan.tpid = vf->vfp_tpid;

		if (vf->vfp_vid == vid_temp) {
			ne6x_dev_del_vf_qinq(vf, vlan.tpid, vlan.vid);
			ne6x_adpt_del_vlan(vf->adpt, vlan);
		}

		vlan.vid = vid_temp;
		vlan.tpid = tpid_temp;
		vid_temp = (qos << VLAN_PRIO_SHIFT) | (vlan.vid & VLAN_VID_MASK);
		vf->vfp_vid = vf->port_vlan_info.vid;
		vf->vfp_tpid = vf->port_vlan_info.tpid;
		ne6x_dev_add_vf_qinq(vf, tpid_temp, vid_temp);
		ne6x_adpt_add_vlan(vf->adpt, vlan);
	} else {
		return -EINVAL;
	}

	return 0;
}

static void *ne6x_fwd_add_macvlan(struct net_device *netdev, struct net_device *vdev)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_macvlan *mv = NULL;
	u8 mac[ETH_ALEN];

	ether_addr_copy(mac, vdev->dev_addr);
	mv = devm_kzalloc(ne6x_pf_to_dev(adpt->back), sizeof(*mv), GFP_KERNEL);
	if (!mv)
		return NULL;

	ne6x_adpt_add_mac(adpt, mac, true);
	INIT_LIST_HEAD(&mv->list);
	mv->vdev = vdev;
	ether_addr_copy(mv->mac, mac);
	list_add(&mv->list, &adpt->macvlan_list);
	netdev_info(netdev, "MACVLAN offloads for %s are on\n", vdev->name);

	return mv;
}

static void ne6x_fwd_del_macvlan(struct net_device *netdev, void *accel_priv)
{
	struct ne6x_macvlan *mv = (struct ne6x_macvlan *)accel_priv;
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	if (!accel_priv)
		return;

	ne6x_adpt_del_mac(adpt, mv->mac, true);
	list_del(&mv->list);
	devm_kfree(ne6x_pf_to_dev(adpt->back), mv);

	netdev_info(netdev, "MACVLAN offloads for %s are off\n", mv->vdev->name);
}

static const struct net_device_ops ne6x_netdev_ops = {
	.ndo_open = ne6x_open,
	.ndo_stop = ne6x_close,
	.ndo_start_xmit = ne6x_lan_xmit_frame,
	.ndo_get_stats64 = ne6x_get_netdev_stats_struct,
	.ndo_set_rx_mode = ne6x_set_rx_mode,
	.ndo_set_mac_address = ne6x_set_mac,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_change_mtu = ne6x_change_mtu,
	.ndo_tx_timeout = ne6x_tx_timeout,
	.ndo_poll_controller = ne6x_netpoll,
	.ndo_set_vf_rate = ne6x_ndo_set_vf_bw,
	.ndo_set_tx_maxrate = ne6x_set_tx_maxrate,
	.ndo_set_vf_mac = ne6x_set_vf_mac,
	.ndo_get_vf_config = ne6x_get_vf_config,
	.ndo_set_vf_trust = ne6x_set_vf_trust,
	.ndo_set_vf_vlan = ne6x_set_vf_port_vlan,
	.ndo_set_vf_link_state = ne6x_set_vf_link_state,
	.ndo_vlan_rx_add_vid = ne6x_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = ne6x_vlan_rx_kill_vid,
	.ndo_set_features = ne6x_set_features,
	.ndo_features_check = ne6x_features_check,
#ifdef CONFIG_RFS_ACCEL
	.ndo_rx_flow_steer = ne6x_rx_flow_steer,
#endif
	.ndo_tx_timeout = ne6x_tx_timeout,
	.ndo_dfwd_add_station = ne6x_fwd_add_macvlan,
	.ndo_dfwd_del_station = ne6x_fwd_del_macvlan,
	.ndo_fix_features = ne6x_fix_features,
	.ndo_set_features = ne6x_set_features,
};

void ne6x_sync_features(struct net_device *netdev)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	u32 value;

	value = ne6x_dev_get_features(adpt);

	if (netdev->features & NETIF_F_GSO_UDP_TUNNEL_CSUM)
		value |= NE6X_F_TX_UDP_TNL_SEG;
	else
		value &= ~NE6X_F_TX_UDP_TNL_SEG;

	if (netdev->features & NETIF_F_HW_VLAN_CTAG_RX)
		value |= NE6X_F_RX_VLAN_STRIP;
	else
		value &= ~NE6X_F_RX_VLAN_STRIP;

	if (netdev->features & NETIF_F_HW_VLAN_CTAG_TX)
		value |= NE6X_F_TX_VLAN;
	else
		value &= ~NE6X_F_TX_VLAN;

	if (netdev->features & NETIF_F_HW_VLAN_STAG_RX)
		value |= NE6X_F_RX_QINQ_STRIP;
	else
		value &= ~NE6X_F_RX_QINQ_STRIP;

	if (netdev->features & NETIF_F_HW_VLAN_STAG_TX)
		value |= NE6X_F_TX_QINQ;
	else
		value &= ~NE6X_F_TX_QINQ;

	if (netdev->features & (NETIF_F_HW_VLAN_CTAG_FILTER | NETIF_F_HW_VLAN_STAG_FILTER))
		value |= NE6X_F_RX_VLAN_FILTER;
	else
		value &= ~NE6X_F_RX_VLAN_FILTER;

	if (netdev->features & NETIF_F_RXCSUM)
		value |= NE6X_OFFLOAD_RXCSUM;
	else
		value &= ~NE6X_OFFLOAD_RXCSUM;

	/* update hardware LRO capability accordingly */
	if (netdev->features & NETIF_F_LRO)
		value |= NE6X_OFFLOAD_LRO;
	else
		value &= ~NE6X_OFFLOAD_LRO;

	if (netdev->features & (NETIF_F_TSO | NETIF_F_TSO6))
		value |= NE6X_OFFLOAD_TSO;
	else
		value &= ~NE6X_OFFLOAD_TSO;

	if (netdev->features & NETIF_F_GSO_UDP)
		value |= NE6X_OFFLOAD_UFO;
	else
		value &= ~NE6X_OFFLOAD_UFO;

	if (netdev->features & NETIF_F_IP_CSUM)
		value |= NE6X_OFFLOAD_TXCSUM;
	else
		value &= ~NE6X_OFFLOAD_TXCSUM;

	if (netdev->features & NETIF_F_RXHASH)
		value |= NE6X_OFFLOAD_RSS;
	else
		value &= ~NE6X_OFFLOAD_RSS;

	if (netdev->features & NETIF_F_HW_L2FW_DOFFLOAD)
		value |= NE6X_OFFLOAD_L2;
	else
		value &= ~NE6X_OFFLOAD_L2;

	if (netdev->features & NETIF_F_SCTP_CRC)
		value |= NE6X_OFFLOAD_SCTP_CSUM;
	else
		value &= ~NE6X_OFFLOAD_SCTP_CSUM;

	if (netdev->features & NETIF_F_NTUPLE)
		value |= NE6X_F_FLOW_STEERING;
	else
		value &= ~NE6X_F_FLOW_STEERING;

	ne6x_dev_set_features(adpt, value);
}

static void ne6x_set_netdev_features(struct net_device *netdev)
{
	struct ne6x_pf *pf = ne6x_netdev_to_pf(netdev);
	netdev_features_t vlano_features = 0u;
	netdev_features_t csumo_features;
	netdev_features_t dflt_features;
	netdev_features_t tso_features;

	dflt_features = NETIF_F_SG	|
			NETIF_F_HIGHDMA	|
			NETIF_F_NTUPLE	|
			NETIF_F_RXHASH;

	csumo_features = NETIF_F_RXCSUM	  |
			 NETIF_F_IP_CSUM  |
			 NETIF_F_SCTP_CRC |
			 NETIF_F_IPV6_CSUM;

	vlano_features = NETIF_F_HW_VLAN_CTAG_FILTER |
			 NETIF_F_HW_VLAN_CTAG_TX     |
			 NETIF_F_HW_VLAN_CTAG_RX;

	tso_features = NETIF_F_TSO			|
		       NETIF_F_TSO_ECN			|
		       NETIF_F_TSO6			|
		       NETIF_F_GSO_GRE			|
		       NETIF_F_GSO_UDP_TUNNEL		|
		       NETIF_F_LRO			|
		       NETIF_F_LOOPBACK			|
		       NETIF_F_GSO_GRE_CSUM		|
		       NETIF_F_GSO_UDP_TUNNEL_CSUM	|
		       NETIF_F_GSO_PARTIAL		|
		       NETIF_F_GSO_IPXIP4		|
		       NETIF_F_GSO_IPXIP6		|
		       NETIF_F_GSO_UDP_L4		|
		       NETIF_F_GSO_SCTP			|
		       0;

	netdev->gso_partial_features |= NETIF_F_GSO_UDP_TUNNEL_CSUM | NETIF_F_GSO_GRE_CSUM;

	/* set features that user can change */
	netdev->hw_features = dflt_features | csumo_features | vlano_features | tso_features;

	/* add support for HW_CSUM on packets with MPLS header */
	netdev->mpls_features =  NETIF_F_HW_CSUM;

	netdev->hw_features |= NETIF_F_HW_L2FW_DOFFLOAD;

	/* enable features */
	netdev->features |= netdev->hw_features;
	/* encap and VLAN devices inherit default, csumo and tso features */
	netdev->hw_enc_features |= dflt_features | csumo_features | tso_features;
	netdev->vlan_features |= dflt_features | csumo_features | tso_features;
	netdev->hw_features |= NETIF_F_HW_TC;
	pf->hw.dvm_ena = 0x1;

	netdev->hw_features |= NETIF_F_HW_VLAN_STAG_RX |
			       NETIF_F_HW_VLAN_STAG_TX |
			       NETIF_F_HW_VLAN_STAG_FILTER;
}

static int ne6x_config_netdev(struct ne6x_adapter *adpt)
{
	struct ne6x_rss_info *rss_info = &adpt->rss_info;
	struct ne6x_pf *pf = adpt->back;
	struct ne6x_netdev_priv *np;
	struct net_device *netdev;
	char name[IFNAMSIZ] = {0};
	int etherdev_size, index;
	u8 mac_addr[ETH_ALEN];

	if (pf->hw.bus.domain_num)
		sprintf(name, "enP%dp%ds0f%d",
			pf->hw.bus.domain_num, pf->hw.bus.bus_num, adpt->idx);
	else
		sprintf(name, "enp%ds0f%d", pf->hw.bus.bus_num, adpt->idx);

	etherdev_size = sizeof(struct ne6x_netdev_priv);

	netdev = alloc_netdev_mq(etherdev_size, name, NET_NAME_USER, ether_setup, adpt->num_queue);
	if (!netdev)
		return -ENOMEM;

	adpt->netdev = netdev;
	np = netdev_priv(netdev);
	np->adpt = adpt;

	/* begin rss info */
	rss_info->hash_type = NE6X_RSS_HASH_TYPE_IPV4_TCP |
			      NE6X_RSS_HASH_TYPE_IPV4_UDP |
			      NE6X_RSS_HASH_TYPE_IPV4 |
			      NE6X_RSS_HASH_TYPE_IPV6_TCP |
			      NE6X_RSS_HASH_TYPE_IPV6_UDP |
			      NE6X_RSS_HASH_TYPE_IPV6;
	rss_info->hash_func = NE6X_RSS_HASH_FUNC_TOEPLITZ;
	rss_info->hash_key_size = NE6X_RSS_MAX_KEY_SIZE;
	rss_info->ind_table_size = NE6X_RSS_MAX_IND_TABLE_SIZE;
	netdev_rss_key_fill(rss_info->hash_key, sizeof(rss_info->hash_key));

	for (index = 0; index < rss_info->ind_table_size; index++)
		rss_info->ind_table[index] = ethtool_rxfh_indir_default(index, adpt->num_queue);

	ne6x_dev_set_rss(adpt, rss_info); /* end rss info */

	ne6x_set_netdev_features(netdev);

	SET_NETDEV_DEV(netdev, &pf->pdev->dev);
	ether_addr_copy(mac_addr, adpt->port_info->mac.perm_addr);
	eth_hw_addr_set(netdev, mac_addr);
	ether_addr_copy(netdev->perm_addr, mac_addr);

	netdev->netdev_ops = &ne6x_netdev_ops;
	netdev->watchdog_timeo = 5 * HZ;
	ne6x_set_ethtool_ops(netdev);

/* MTU range: 128 - 15342 */
	netdev->min_mtu = NE6X_MIN_MTU_SIZE;
	netdev->max_mtu = NE6X_MAX_RXBUFFER - NE6X_PACKET_HDR_PAD - ETH_FCS_LEN;
	netdev->gso_max_size = 65535;
	netdev->needed_headroom = 32;
	netdev->needed_tailroom = 32;
	ne6x_dev_set_mtu(adpt, netdev->mtu);
	ne6x_sync_features(netdev);

	return 0;
}

static void ne6x_map_vector_to_qp(struct ne6x_adapter *adpt, int v_idx, int qp_idx)
{
	struct ne6x_q_vector *q_vector = adpt->q_vectors[v_idx];
	struct ne6x_ring *tx_ring = adpt->tx_rings[qp_idx];
	struct ne6x_ring *rx_ring = adpt->rx_rings[qp_idx];
	struct ne6x_ring *cq_ring = adpt->cq_rings[qp_idx];
	struct ne6x_ring *tg_ring = adpt->tg_rings[qp_idx];

	tx_ring->q_vector = q_vector;
	tx_ring->next = q_vector->tx.ring;
	q_vector->tx.ring = tx_ring;
	q_vector->tx.count++;

	cq_ring->q_vector = q_vector;
	cq_ring->next = q_vector->cq.ring;
	q_vector->cq.ring = cq_ring;
	q_vector->cq.count++;
	tg_ring->q_vector = q_vector;
	tg_ring->next = q_vector->cq.ring;
	q_vector->tg.ring = tg_ring;
	q_vector->tg.count++;

	rx_ring->q_vector = q_vector;
	rx_ring->next = q_vector->rx.ring;
	q_vector->rx.ring = rx_ring;
	q_vector->rx.count++;
}

void ne6x_adpt_map_rings_to_vectors(struct ne6x_adapter *adpt)
{
	int q_vectors = adpt->num_q_vectors;
	int qp_remaining = adpt->num_queue;
	struct ne6x_q_vector *q_vector;
	int num_ringpairs;
	int v_start = 0;
	int qp_idx = 0;

	/* If we don't have enough vectors for a 1-to-1 mapping, we'll have to
	 * group them so there are multiple queues per vector.
	 * It is also important to go through all the vectors available to be
	 * sure that if we don't use all the vectors, that the remaining vectors
	 * are cleared. This is especially important when decreasing the
	 * number of queues in use.
	 */
	for (; v_start < q_vectors; v_start++) {
		q_vector = adpt->q_vectors[v_start];

		num_ringpairs = DIV_ROUND_UP(qp_remaining, q_vectors - v_start);

		q_vector->num_ringpairs = num_ringpairs;
		q_vector->reg_idx = q_vector->v_idx + adpt->base_vector;

		q_vector->rx.count = 0;
		q_vector->tx.count = 0;
		q_vector->cq.count = 0;
		q_vector->tg.count = 0;
		q_vector->rx.ring = NULL;
		q_vector->tx.ring = NULL;
		q_vector->cq.ring = NULL;
		q_vector->tg.ring = NULL;

		while (num_ringpairs--) {
			ne6x_map_vector_to_qp(adpt, v_start, qp_idx);
			qp_idx++;
			qp_remaining--;
		}
	}
}

void ne6x_adpt_reset_stats(struct ne6x_adapter *adpt)
{
	struct rtnl_link_stats64 *ns;
	int i;

	if (!adpt)
		return;

	ns = ne6x_get_adpt_stats_struct(adpt);
	memset(ns, 0, sizeof(*ns));
	memset(&adpt->net_stats_offsets, 0, sizeof(adpt->net_stats_offsets));
	memset(&adpt->eth_stats, 0, sizeof(adpt->eth_stats));
	memset(&adpt->eth_stats_offsets, 0, sizeof(adpt->eth_stats_offsets));

	if (adpt->rx_rings && adpt->rx_rings[0]) {
		for (i = 0; i < adpt->num_queue; i++) {
			memset(&adpt->rx_rings[i]->stats, 0,
			       sizeof(adpt->rx_rings[i]->stats));
			memset(&adpt->rx_rings[i]->rx_stats, 0,
			       sizeof(adpt->rx_rings[i]->rx_stats));
			memset(&adpt->rx_rings[i]->cq_stats, 0,
			       sizeof(adpt->rx_rings[i]->cq_stats));
			memset(&adpt->tx_rings[i]->stats, 0,
			       sizeof(adpt->tx_rings[i]->stats));
			memset(&adpt->tx_rings[i]->tx_stats, 0,
			       sizeof(adpt->tx_rings[i]->tx_stats));
		}
	}
}

static int ne6x_adpt_setup(struct ne6x_pf *pf)
{
	struct ne6x_adapter *adpt = NULL;
	u32 is_write_proterct = false;
	struct ne6x_hw *hw = &pf->hw;
	int i, ret = 0;
	u32 value;

	/* PF + VP */
	pf->adpt = kcalloc(NE6X_MAX_VP_NUM + 4, sizeof(*pf->adpt), GFP_KERNEL);
	if (!pf->adpt)
		return -ENOMEM;

	ne6x_dev_get_norflash_write_protect(pf, &is_write_proterct);

	/* Need to protect the allocation of the adapters at the PF level */
	for (i = pf->num_alloc_adpt - 1; i >= 0; i--) {
		struct ne6x_vlan vlan = {0};

		adpt = kzalloc(sizeof(*adpt), GFP_KERNEL);
		adpt->back = pf;
		pf->adpt[i] = adpt;
		adpt->idx = i;
		adpt->vport = NE6X_PF_VP0_NUM + i; /*vport*/
		set_bit(NE6X_ADPT_DOWN, adpt->comm.state);

		value = ne6x_dev_get_features(adpt);
		if (value & NE6X_F_RX_FW_LLDP)
			clear_bit(NE6X_ADPT_F_DISABLE_FW_LLDP, adpt->flags);
		else
			set_bit(NE6X_ADPT_F_DISABLE_FW_LLDP, adpt->flags);

		clear_bit(NE6X_ADPT_F_LINKDOWN_ON_CLOSE, adpt->flags);
		clear_bit(NE6X_ADPT_F_DDOS_SWITCH, adpt->flags);
		clear_bit(NE6X_ADPT_F_ACL, adpt->flags);

		if (is_write_proterct)
			set_bit(NE6X_ADPT_F_NORFLASH_WRITE_PROTECT, adpt->flags);
		else
			clear_bit(NE6X_ADPT_F_NORFLASH_WRITE_PROTECT, adpt->flags);

		INIT_WORK(&adpt->set_rx_mode_task, ne6x_set_rx_mode_task);

		/* init multicast MAC addr list head node */
		INIT_LIST_HEAD(&adpt->mc_mac_addr.list);
		mutex_init(&adpt->mc_mac_addr.mutex);

		/* init unicast MAC addr list head node */
		INIT_LIST_HEAD(&adpt->uc_mac_addr.list);
		mutex_init(&adpt->uc_mac_addr.mutex);

		/* init vlan list head node */
		spin_lock_init(&adpt->mac_vlan_list_lock);
		INIT_LIST_HEAD(&adpt->vlan_filter_list);

		INIT_LIST_HEAD(&adpt->macvlan_list);
		init_waitqueue_head(&adpt->recv_notify);

		adpt->port_info = kzalloc(sizeof(*adpt->port_info), GFP_KERNEL);
		if (!adpt->port_info) {
			ret = -ENOMEM;
			goto err_portinfo;
		}

		adpt->port_info->lport         = i; /* logical port */
		adpt->port_info->hw_trunk_id   = i;
		adpt->port_info->hw_port_id    = ne6x_dev_get_pport(adpt);
		adpt->port_info->queue         = pf->hw.max_queue;
		adpt->port_info->hw_max_queue  = adpt->port_info->queue;
		adpt->port_info->hw_queue_base = pf->hw.expect_vp * i;
		adpt->comm.port_info = adpt->port_info->lport | (adpt->vport << 8);
		adpt->port_info->hw = hw;
		adpt->port_info->phy.curr_user_speed_req = 0x0;

		ne6x_dev_get_mac_addr(adpt, adpt->port_info->mac.perm_addr);
		ne6x_set_num_rings_in_adpt(adpt);

		ret = ne6x_adpt_mem_alloc(pf, adpt);
		if (ret)
			goto err_netdev;

		ret = ne6x_config_netdev(adpt);
		if (ret)
			goto err_configdev;

		/* The unicast MAC address delivers the SDK */
		vlan = NE6X_VLAN(ETH_P_8021Q, 0xfff, 0);
		ne6x_adpt_add_vlan(adpt, vlan);
		ne6x_adpt_add_mac(adpt, adpt->port_info->mac.perm_addr, true);
		ne6x_dev_add_broadcast_leaf(adpt);

		/* set up vectors and rings if needed */
		ret = ne6x_adpt_setup_vectors(adpt);
		if (ret)
			goto err_msix;

		ret = ne6x_alloc_rings(adpt);
		if (ret)
			goto err_rings;

		ne6x_init_arfs(adpt);

		ret = ne6x_set_cpu_rx_rmap(adpt);
		if (ret)
			netdev_info(adpt->netdev, "adpt rx rmap err: %d", ret);

		/* map all of the rings to the q_vectors */
		ne6x_adpt_map_rings_to_vectors(adpt);
		ne6x_adpt_reset_stats(adpt);
		ne6x_dev_set_port2pi(adpt);
		ne6x_dev_set_pi2port(adpt);
		ne6x_dev_set_vport(adpt);
		ne6x_dev_set_rss(adpt, &adpt->rss_info);
	}

	for (i = pf->num_alloc_adpt - 1; i >= 0; i--) {
		adpt = pf->adpt[i];
		ret = ne6x_adpt_register_netdev(adpt);
		if (ret)
			goto err_configdev;

		adpt->netdev_registered = true;
		netif_carrier_off(adpt->netdev);
		/* make sure transmit queues start off as stopped */
		netif_tx_stop_all_queues(adpt->netdev);
	}

	return ret;

err_rings:
	ne6x_adpt_free_q_vectors(adpt);
err_msix:
	if (adpt->netdev_registered) {
		adpt->netdev_registered = false;
		unregister_netdev(adpt->netdev);
		free_netdev(adpt->netdev);
		adpt->netdev = NULL;
	}
err_configdev:
	kfree(adpt->tx_rings);
	kfree(adpt->q_vectors);
err_netdev:
	kfree(adpt->port_info);
err_portinfo:
	kfree(adpt);

	return ret;
}

int ne6x_adpt_register_netdev(struct ne6x_adapter *adpt)
{
	int ret;

	ret = register_netdev(adpt->netdev);
	if (ret) {
		struct net_device *device = adpt->netdev;
		struct ne6x_pf *pf = adpt->back;
		char name[IFNAMSIZ] = {0};

		sprintf(name, "enp%ds0f%%d", pf->hw.bus.bus_num);
		strcpy(device->name, name);
		return register_netdev(adpt->netdev);
	}

	return ret;
}

void ne6x_adjust_adpt_port_max_queue(struct ne6x_pf *pf)
{
	int cpu_num = num_online_cpus();

	if (pf->irq_pile->num_entries < NE6X_MAX_MSIX_NUM) {
		pf->hw.expect_vp = pf->irq_pile->num_entries / pf->hw.pf_port;
		/* actal max vp queue */
		pf->hw.max_queue = min_t(int, cpu_num, pf->hw.expect_vp);
		dev_info(&pf->pdev->dev, "%s:hw->expect_vp = %d hw->max_queue = %d cpu_num = %d\n",
			 __func__, pf->hw.expect_vp, pf->hw.max_queue, cpu_num);
	}
}

static int ne6x_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct ne6x_pf *pf;
	struct ne6x_hw *hw;
	u32 ioremap_len;
	int err;

	if (PCI_FUNC(pdev->devfn) != 1)
		return 0;

	/* initialize device for use with memory space */
	err = pci_enable_device_mem(pdev);
	if (err)
		return err;

	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(48));
	if (err) {
		err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
		if (err) {
			dev_err(&pdev->dev, "DMA configuration failed: 0x%x\n", err);
			goto err_dma;
		}
	}

	/* set up pci connections */
	err = pci_request_mem_regions(pdev, ne6x_driver_name);
	if (err) {
		dev_info(&pdev->dev, "pci_request_mem_regions failed %d\n", err);
		goto err_pci_reg;
	}
	pci_set_master(pdev);
	/* Now that we have a PCI connection, we need to do the
	 * low level device setup. This is primarily setting up
	 * the Admin Queue structures and then querying for the
	 * device's current profile information.
	 */
	pf = kzalloc(sizeof(*pf), GFP_KERNEL);
	if (!pf) {
		err = -ENOMEM;
		goto err_pf_alloc;
	}
	pf->next_adpt = 0;
	pf->pdev = pdev;
	pci_set_drvdata(pdev, pf);
	set_bit(NE6X_DOWN, pf->state);

	hw = &pf->hw;
	hw->back = pf;

	ioremap_len = pci_resource_len(pdev, 0);
	hw->hw_addr0 = ioremap(pci_resource_start(pdev, 0), ioremap_len);
	if (!hw->hw_addr0) {
		err = -EIO;
		dev_info(&pdev->dev, "ioremap bar0 (0x%04x, 0x%04x) failed: 0x%x\n",
			 (unsigned int)pci_resource_start(pdev, 0), ioremap_len, err);
		goto err_ioremap_hw_addr0;
	}

	ioremap_len = pci_resource_len(pdev, 2);
	hw->hw_addr2 = ioremap(pci_resource_start(pdev, 2), ioremap_len);
	if (!hw->hw_addr2) {
		err = -EIO;
		dev_info(&pdev->dev, "ioremap bar2 (0x%04x, 0x%04x) failed: 0x%x\n",
			 (unsigned int)pci_resource_start(pdev, 2), ioremap_len, err);
		goto err_ioremap_hw_addr2;
	}

	ioremap_len = pci_resource_len(pdev, 4);
	hw->hw_addr4 = ioremap(pci_resource_start(pdev, 4), ioremap_len);
	if (!hw->hw_addr4) {
		err = -EIO;
		dev_info(&pdev->dev, "ioremap bar4 (0x%04x, 0x%04x) failed: 0x%x\n",
			 (unsigned int)pci_resource_start(pdev, 4), ioremap_len, err);
		goto err_ioremap_hw_addr4;
	}

	hw->vendor_id = pdev->vendor;
	hw->device_id = pdev->device;
	hw->revision_id = pdev->revision;
	hw->subsystem_vendor_id = pdev->subsystem_vendor;
	hw->subsystem_device_id = pdev->subsystem_device;

	hw->bus.domain_num = pci_domain_nr(pdev->bus);
	hw->bus.bus_num = pdev->bus->number;
	hw->bus.device = PCI_SLOT(pdev->devfn);
	hw->bus.func = PCI_FUNC(pdev->devfn);

	usleep_range(10, 20);

	mutex_init(&pf->mbus_comm_mutex);
	if (ne6x_dev_init(pf)) {
		err = -EIO;
		dev_info(&pdev->dev, "sdk init failed!\n");
		goto error_sdk_init_failed;
	}
	usleep_range(10, 20);

	pci_save_state(pdev);

	/* hardware resource initialization */
	err = ne6x_hw_init(hw);
	if (err)
		goto err_unroll_alloc;

	/* driver private resource initialization */
	err = ne6x_pf_init(pf);
	if (err)
		goto err_pf_reset;

	/* interrupt resource initialization */
	err = ne6x_init_interrupt_scheme(pf);
	if (err)
		goto err_interrupt_scheme;

	ne6x_adjust_adpt_port_max_queue(pf);

	err = ne6x_adpt_setup(pf);
	if (err)
		goto err_adpts;

	ne6x_dev_set_nic_start(pf, 0);
	add_timer(&pf->linkscan_tmr);
	ne6x_enable_link_irq(pf);
	pcie_print_link_status(pdev);
	/* ready to go, so clear down state bit */
	clear_bit(NE6X_DOWN, pf->state);
	return 0;

err_adpts:
	set_bit(NE6X_DOWN, pf->state);
	ne6x_clear_interrupt_scheme(pf);
err_interrupt_scheme:
	del_timer_sync(&pf->serv_tmr);
err_pf_reset:
	devm_kfree(ne6x_hw_to_dev(hw), hw->port_info);
	hw->port_info = NULL;
err_unroll_alloc:
error_sdk_init_failed:
	iounmap(hw->hw_addr4);
err_ioremap_hw_addr4:
	iounmap(hw->hw_addr2);
	hw->hw_addr2 = NULL;
err_ioremap_hw_addr2:
	iounmap(hw->hw_addr0);
err_ioremap_hw_addr0:
	kfree(pf);
err_pf_alloc:
	pci_release_mem_regions(pdev);
err_pci_reg:
err_dma:
	pci_disable_device(pdev);
	return err;
}

void ne6x_adpt_free_arrays(struct ne6x_adapter *adpt, bool free_qvectors)
{
	/* free the ring and vector containers */
	if (free_qvectors) {
		kfree(adpt->q_vectors);
		adpt->q_vectors = NULL;
	}

	kfree(adpt->tx_rings);
	adpt->tx_rings = NULL;
	adpt->rx_rings = NULL;
	adpt->cq_rings = NULL;
}

static int ne6x_adpt_clear(struct ne6x_adapter *adpt)
{
	struct ne6x_pf *pf;

	if (!adpt)
		return 0;

	if (!adpt->back)
		goto free_adpt;

	pf = adpt->back;

	mutex_lock(&pf->switch_mutex);
	if (!pf->adpt[adpt->idx]) {
		dev_err(&pf->pdev->dev, "pf->adpt[%d] is NULL, just free adpt[%d](type %d)\n",
			adpt->idx, adpt->idx, adpt->type);
		goto unlock_adpt;
	}

	if (pf->adpt[adpt->idx] != adpt) {
		dev_err(&pf->pdev->dev, "pf->adpt[%d](type %d) != adpt[%d](type %d): no free!\n",
			pf->adpt[adpt->idx]->idx, pf->adpt[adpt->idx]->type, adpt->idx, adpt->type);
		goto unlock_adpt;
	}

	/* updates the PF for this cleared adpt */
	ne6x_adpt_free_arrays(adpt, true);

	pf->adpt[adpt->idx] = NULL;
	if (adpt->idx < pf->next_adpt)
		pf->next_adpt = adpt->idx;

unlock_adpt:
	mutex_unlock(&pf->switch_mutex);
free_adpt:
	kfree(adpt);

	return 0;
}

int ne6x_adpt_release(struct ne6x_adapter *adpt)
{
	struct mac_addr_head *mc_head = &adpt->mc_mac_addr;
	struct mac_addr_head *uc_head = &adpt->uc_mac_addr;
	struct mac_addr_node *temp_node, *addr_node;
	struct ne6x_vlan_filter *vlf, *vlftmp;
	struct ne6x_key_filter *klf, *klftmp;
	struct ne6x_macvlan *mv, *mv_tmp;
	struct ne6x_pf *pf = adpt->back;

	if (!test_bit(NE6X_DOWN, pf->state)) {
		dev_info(&pf->pdev->dev, "Can't remove PF adapter\n");
		return -ENODEV;
	}

	set_bit(NE6X_ADPT_RELEASING, adpt->comm.state);

	ne6x_remove_arfs(adpt);
	ne6x_adpt_clear_ddos(pf);
	ne6x_adpt_clear_mac_vlan(adpt);
	ne6x_dev_del_broadcast_leaf(adpt);
	/* release adpt multicast addr list resource */
	mutex_lock(&mc_head->mutex);
	list_for_each_entry_safe(addr_node, temp_node, &mc_head->list, list) {
		list_del(&addr_node->list);
		kfree(addr_node);
	}
	mutex_unlock(&mc_head->mutex);

	/* release adpt unicast addr list resource */
	mutex_lock(&uc_head->mutex);
	list_for_each_entry_safe(addr_node, temp_node, &uc_head->list, list) {
		list_del(&addr_node->list);
		kfree(addr_node);
	}
	mutex_unlock(&uc_head->mutex);

	spin_lock_bh(&adpt->mac_vlan_list_lock);
	/* release adpt vlan list resource */
	list_for_each_entry_safe(vlf, vlftmp, &adpt->vlan_filter_list, list) {
		list_del(&vlf->list);
		kfree(vlf);
	}
	spin_unlock_bh(&adpt->mac_vlan_list_lock);

	spin_lock_bh(&adpt->back->key_list_lock);
	/* release adpt vlan list resource */
	list_for_each_entry_safe(klf, klftmp, &adpt->back->key_filter_list, list) {
		list_del(&klf->list);
		kfree(klf);
	}
	spin_unlock_bh(&adpt->back->key_list_lock);

	list_for_each_entry_safe(mv, mv_tmp, &adpt->macvlan_list, list)
		ne6x_fwd_del_macvlan(adpt->netdev, mv);

	if (adpt->netdev_registered) {
		adpt->netdev_registered = false;
		if (adpt->netdev)
			/* results in a call to i40e_close() */
			unregister_netdev(adpt->netdev);
	}

	ne6x_free_cpu_rx_rmap(adpt);
	ne6x_adpt_disable_irq(adpt);

	/* clear the sync flag on all filters */
	if (adpt->netdev) {
		__dev_uc_unsync(adpt->netdev, NULL);
		__dev_mc_unsync(adpt->netdev, NULL);
	}

	ne6x_adpt_free_q_vectors(adpt);
	if (adpt->netdev) {
		free_netdev(adpt->netdev);
		adpt->netdev = NULL;
	}

	/*add for lldp*/
	ne6x_dev_set_fw_lldp(adpt, false);
	ne6x_adpt_clear_rings(adpt);
	ne6x_adpt_clear(adpt);

	return 0;
}

static void ne6x_remove(struct pci_dev *pdev)
{
	struct ne6x_pf *pf = pci_get_drvdata(pdev);
	struct ne6x_hw *hw = &pf->hw;
	int i;

	if (PCI_FUNC(pdev->devfn) != 1)
		return;

	ne6x_proc_pf_exit(pf);
	ne6x_dbg_pf_exit(pf);

	ne6x_dev_set_nic_stop(pf, 0);

#ifdef CONFIG_PCI_IOV
	if (pf->num_alloc_vfs) {
		set_bit(NE6X_REMOVE, pf->state);
		ne6x_sriov_configure(pdev, 0);
	}
#endif

	/* no more scheduling of any task */
	set_bit(NE6X_DOWN, pf->state);
	if (pf->serv_tmr.function)
		del_timer_sync(&pf->serv_tmr);

	if (pf->serv_task.func)
		cancel_work_sync(&pf->serv_task);

	if (pf->linkscan_tmr.function)
		del_timer_sync(&pf->linkscan_tmr);

	if (pf->linkscan_work.func)
		cancel_work_sync(&pf->linkscan_work);

	/* Now we can shutdown the PF's adapter, just before we kill
	 * adminq and hmc.
	 */
	for (i = 0; i < pf->num_alloc_adpt; i++)
		ne6x_adpt_release(pf->adpt[i]);

	/* Clear all dynamic memory lists of rings, q_vectors, and adapters */
	rtnl_lock();
	ne6x_clear_interrupt_scheme(pf);
	for (i = 0; i < pf->num_alloc_adpt; i++) {
		if (pf->adpt[i]) {
			ne6x_adpt_clear_rings(pf->adpt[i]);
			ne6x_adpt_clear(pf->adpt[i]);
			pf->adpt[i] = NULL;
		}
	}
	rtnl_unlock();

	kfree(pf->adpt);

	iounmap(hw->hw_addr4);
	iounmap(hw->hw_addr2);
	hw->hw_addr2 = NULL;
	iounmap(hw->hw_addr0);
	kfree(pf);
	pci_release_mem_regions(pdev);
	pci_disable_device(pdev);
}

static struct pci_driver ne6x_driver = {
	.name = ne6x_driver_name,
	.id_table = ne6x_pci_tbl,
	.probe = ne6x_probe,
	.remove = ne6x_remove,
	.sriov_configure = ne6x_sriov_configure,
};

int __init ne6x_init_module(void)
{
	pr_info("%s: %s - version %s\n", ne6x_driver_name, ne6x_driver_string,
		ne6x_driver_version_str);
	pr_info("%s: %s\n", ne6x_driver_name, ne6x_copyright);

	ne6x_wq = create_singlethread_workqueue(ne6x_driver_name);
	if (!ne6x_wq) {
		pr_err("%s: Failed to create workqueue\n", ne6x_driver_name);
		return -ENOMEM;
	}

	ne6x_dbg_init();
	ne6x_proc_init();
	ne6x_netlink_init();

	return pci_register_driver(&ne6x_driver);
}

module_init(ne6x_init_module);

void __exit ne6x_exit_module(void)
{
	pci_unregister_driver(&ne6x_driver);
	destroy_workqueue(ne6x_wq);
	ne6x_netlink_exit();
	ne6x_proc_exit();
	ne6x_dbg_exit();
}

module_exit(ne6x_exit_module);
