// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 3SNIC Information Technology, Ltd */

/* 3SNIC RAID SSSXXX Series Linux Driver */

#include <linux/version.h>
#include <linux/pci.h>
#include <linux/aer.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/sysfs.h>
#include <linux/gfp.h>
#include <linux/types.h>
#include <linux/ratelimit.h>
#include <linux/debugfs.h>
#include <linux/blkdev.h>
#include <linux/bsg-lib.h>
#include <linux/sort.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_transport.h>
#include <scsi/scsi_dbg.h>

#include <asm/unaligned.h>
#include <linux/once.h>
#include <linux/sched/signal.h>
#include <linux/io-64-nonatomic-lo-hi.h>

#include "sssraid.h"
#include "sssraid_debug.h"

static int sssraid_wait_ready(struct sssraid_ioc *sdioc, u64 cap, bool enabled)
{
	unsigned long timeout =
	((SSSRAID_CAP_TIMEOUT(cap) + 1) * SSSRAID_CAP_TIMEOUT_UNIT_MS) + jiffies;
	u32 bit = enabled ? SSSRAID_CSTS_RDY : 0;

	while ((readl(sdioc->bar + SSSRAID_REG_CSTS) & SSSRAID_CSTS_RDY) != bit) {
		usleep_range(1000, 2000);
		if (fatal_signal_pending(current))
			return -EINTR;

		if (time_after(jiffies, timeout)) {
			ioc_err(sdioc, "controller not ready, aborting %s\n",
				enabled ? "initialization" : "reset");
			return -ENODEV;
		}
	}
	return 0;
}

static int sssraid_enable_ctrl(struct sssraid_ioc *sdioc)
{
	u64 cap = sdioc->cap;
	u32 dev_page_min = SSSRAID_CAP_MPSMIN(cap) + 12;
	u32 page_shift = PAGE_SHIFT;

	if (page_shift < dev_page_min) {
		ioc_err(sdioc, "err: minimum ioc page size[%u], too large for host[%u]\n",
			1U << dev_page_min, 1U << page_shift);
		return -ENODEV;
	}

	page_shift = min_t(unsigned int, SSSRAID_CAP_MPSMAX(cap) + 12, PAGE_SHIFT);
	sdioc->page_size = 1U << page_shift;

	sdioc->ctrl_config = SSSRAID_CC_CSS_NVM;
	sdioc->ctrl_config |= (page_shift - 12) << SSSRAID_CC_MPS_SHIFT;
	sdioc->ctrl_config |= SSSRAID_CC_AMS_RR | SSSRAID_CC_SHN_NONE;
	sdioc->ctrl_config |= SSSRAID_CC_IOSQES | SSSRAID_CC_IOCQES;
	sdioc->ctrl_config |= SSSRAID_CC_ENABLE;
	writel(sdioc->ctrl_config, sdioc->bar + SSSRAID_REG_CC);

	return sssraid_wait_ready(sdioc, cap, true);
}

static int sssraid_disable_ctrl(struct sssraid_ioc *sdioc)
{
	sdioc->ctrl_config &= ~SSSRAID_CC_SHN_MASK;
	sdioc->ctrl_config &= ~SSSRAID_CC_ENABLE;
	writel(sdioc->ctrl_config, sdioc->bar + SSSRAID_REG_CC);

	return sssraid_wait_ready(sdioc, sdioc->cap, false);
}

static int sssraid_shutdown_ctrl(struct sssraid_ioc *sdioc)
{
	unsigned long timeout = le32_to_cpu(sdioc->ctrl_info->rtd3e) / 1000000 * HZ + jiffies;

	sdioc->ctrl_config &= ~SSSRAID_CC_SHN_MASK;
	sdioc->ctrl_config |= SSSRAID_CC_SHN_NORMAL;
	writel(sdioc->ctrl_config, sdioc->bar + SSSRAID_REG_CC);

	while ((readl(sdioc->bar + SSSRAID_REG_CSTS) & SSSRAID_CSTS_SHST_MASK) !=
		SSSRAID_CSTS_SHST_CMPLT) {
		msleep(100);
		if (fatal_signal_pending(current))
			return -EINTR;
		if (time_after(jiffies, timeout)) {
			ioc_err(sdioc, "ioc shutdown incomplete, abort shutdown\n");
			return -ENODEV;
		}
	}
	return 0;
}

static int sssraid_remap_bar(struct sssraid_ioc *sdioc, u32 size)
{
	struct pci_dev *pdev = sdioc->pdev;

	if (size > pci_resource_len(pdev, 0)) {
		ioc_err(sdioc, "err: input size[%u] exceed bar0 length[%llu]\n",
			size, pci_resource_len(pdev, 0));
		return -ENODEV;
	}

	if (sdioc->bar)
		iounmap(sdioc->bar);

	sdioc->bar = ioremap(pci_resource_start(pdev, 0), size);
	if (!sdioc->bar) {
		ioc_err(sdioc, "err: ioremap for bar0 failed\n");
		return -ENODEV;
	}
	sdioc->dbs = sdioc->bar + SSSRAID_REG_DBS;

	return 0;
}

static int sssraid_create_dma_pools(struct sssraid_ioc *sdioc)
{
	int i;
	char poolname[20] = { 0 };

	sdioc->prp_page_pool = dma_pool_create("prp list page", &sdioc->pdev->dev,
					      PAGE_SIZE, PAGE_SIZE, 0);

	if (!sdioc->prp_page_pool) {
		ioc_err(sdioc, "err: create prp page pool failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < small_pool_num; i++) {
		sprintf(poolname, "prp_list_256_%d", i);
		sdioc->prp_small_pool[i] = dma_pool_create(poolname, &sdioc->pdev->dev,
								SMALL_POOL_SIZE,
								SMALL_POOL_SIZE, 0);

		if (!sdioc->prp_small_pool[i]) {
			ioc_err(sdioc, "err: create prp small pool %d failed\n", i);
			goto destroy_prp_small_pool;
		}
	}

	return 0;

destroy_prp_small_pool:
	while (i > 0)
		dma_pool_destroy(sdioc->prp_small_pool[--i]);
	dma_pool_destroy(sdioc->prp_page_pool);

	return -ENOMEM;
}

static void sssraid_destroy_dma_pools(struct sssraid_ioc *sdioc)
{
	int i;

	for (i = 0; i < small_pool_num; i++)
		dma_pool_destroy(sdioc->prp_small_pool[i]);
	dma_pool_destroy(sdioc->prp_page_pool);
}

static int sssraid_alloc_resources(struct sssraid_ioc *sdioc)
{
	int retval, nqueue;

	sdioc->ctrl_info = kzalloc_node(sizeof(*sdioc->ctrl_info), GFP_KERNEL, sdioc->numa_node);
	if (!sdioc->ctrl_info)
		return -ENOMEM;

	retval = sssraid_create_dma_pools(sdioc);
	if (retval) {
		ioc_err(sdioc, "err: failure at create dma pool!\n");
		goto free_ctrl_info;
	}

	/* not num_online_cpus */
	nqueue = num_possible_cpus() + 1;
	sdioc->cqinfo = kcalloc_node(nqueue, sizeof(struct sssraid_cqueue),
			    GFP_KERNEL, sdioc->numa_node);
	if (!sdioc->cqinfo) {
		retval = -ENOMEM;
		ioc_err(sdioc, "err: failure at alloc memory for cqueue!");
		goto destroy_dma_pools;
	}

	sdioc->sqinfo = kcalloc_node(nqueue, sizeof(struct sssraid_squeue),
			    GFP_KERNEL, sdioc->numa_node);
	if (!sdioc->sqinfo) {
		retval = -ENOMEM;
		ioc_err(sdioc, "err: failure at alloc memory for squeue!");
		goto free_cqueues;
	}

	/* sssraid_alloc_admin_cmds moved to sssraid_init_ioc */

	ioc_info(sdioc, "Request Queues Count: %d\n", nqueue);

	return 0;

free_cqueues:
	kfree(sdioc->cqinfo);
destroy_dma_pools:
	sssraid_destroy_dma_pools(sdioc);
free_ctrl_info:
	kfree(sdioc->ctrl_info);

	return retval;
}

void sssraid_ioc_enable_intr(struct sssraid_ioc *sdioc)
{
	sdioc->intr_enabled = 1;
}

void sssraid_ioc_disable_intr(struct sssraid_ioc *sdioc)
{
	u16 i, max_vectors;

	sdioc->intr_enabled = 0;
	max_vectors = sdioc->intr_info_count;

	for (i = 0; i < max_vectors; i++)
		synchronize_irq(pci_irq_vector(sdioc->pdev, i));
}

static int sssraid_setup_resources(struct sssraid_ioc *sdioc)
{
	struct pci_dev *pdev = sdioc->pdev;
	int retval = 0;
	u64 maskbit = SSSRAID_DMA_MSK_BIT_MAX;

	if (pci_enable_device_mem(pdev)) {
		ioc_err(sdioc, "err: pci_enable_device_mem failed\n");
		retval = -ENODEV;
		goto out_failed;
	}

	retval = pci_request_mem_regions(pdev, SSSRAID_DRIVER_NAME);
	if (retval) {
		ioc_err(sdioc, "err: fail to request memory regions\n");
		retval = -ENODEV;
		goto out_failed;
	}

	/* get cap value at first, so keep
	 * sssraid_remap_bar(hdev, SSSRAID_REG_DBS + 4096)
	 * ioremap(pci_resource_start(..)) still in sssraid_remap_bar
	 */
	retval = sssraid_remap_bar(sdioc, SSSRAID_REG_DBS + 4096);
	if (retval) {
		ioc_err(sdioc, "Failed to re-map bar, error %d\n", retval);
		goto out_failed;
	}

	pci_set_master(pdev);

	if (readl(sdioc->bar + SSSRAID_REG_CSTS) == U32_MAX) {
		retval = -ENODEV;
		ioc_err(sdioc, "read BAR offset:csts register failed\n");
		goto out_failed;
	}

	sdioc->cap = lo_hi_readq(sdioc->bar + SSSRAID_REG_CAP);
	sdioc->ioq_depth = min_t(u32, SSSRAID_CAP_MQES(sdioc->cap) + 1, io_queue_depth);
	sdioc->scsi_qd = sdioc->ioq_depth - SSSRAID_PTCMDS_PERQ;
	sdioc->db_stride = 1 << SSSRAID_CAP_STRIDE(sdioc->cap);

	maskbit = SSSRAID_CAP_DMAMASK(sdioc->cap);
	if (maskbit < 32 || maskbit > SSSRAID_DMA_MSK_BIT_MAX) {
		ioc_notice(sdioc, "err: DMA MASK BIT invalid[%llu], set to default\n", maskbit);
		maskbit = SSSRAID_DMA_MSK_BIT_MAX;
	}

	if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(maskbit))) {
		if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32))) {
			ioc_err(sdioc, "err: Set DMA MASK: 32 BIT and coherent failed\n");
			retval = -ENODEV;
			goto out_failed;
		}
		ioc_info(sdioc, "Set DMA MASK: 32 BIT success\n");
	} else {
		ioc_info(sdioc, "Set DMA MASK: %llu BIT success\n", maskbit);
	}

	/* pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES) moved to setup_isr */

	pci_set_drvdata(pdev, sdioc->shost);

	pci_save_state(pdev);

	sssraid_ioc_disable_intr(sdioc);

	return retval;

out_failed:
	sssraid_cleanup_resources(sdioc);
	return retval;
}

static int sssraid_alloc_admin_cmds(struct sssraid_ioc *sdioc)
{
	u16 i;

	INIT_LIST_HEAD(&sdioc->adm_cmd_list);
	spin_lock_init(&sdioc->adm_cmd_lock);

	sdioc->adm_cmds = kcalloc_node(SSSRAID_AMDQ_BLK_MQ_DEPTH, sizeof(struct sssraid_cmd),
				      GFP_KERNEL, sdioc->numa_node);

	if (!sdioc->adm_cmds) {
		ioc_err(sdioc, "Alloc admin cmds failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < SSSRAID_AMDQ_BLK_MQ_DEPTH; i++) {
		sdioc->adm_cmds[i].qid = 0;
		sdioc->adm_cmds[i].cid = i;
		list_add_tail(&(sdioc->adm_cmds[i].list), &sdioc->adm_cmd_list);
	}

	ioc_info(sdioc, "Alloc admin cmds success, count: %d\n", SSSRAID_AMDQ_BLK_MQ_DEPTH);

	return 0;
}

static int sssraid_alloc_qpair(struct sssraid_ioc *sdioc, u16 qidx, u16 depth)
{
	struct sssraid_cqueue *cqinfo = &sdioc->cqinfo[qidx];
	struct sssraid_squeue *sqinfo = &sdioc->sqinfo[qidx];
	int retval = 0;

	if (sdioc->init_done_queue_cnt > qidx) {
		ioc_warn(sdioc, "warn: queue: %d exists!\n", qidx);
		return 0;
	}

	cqinfo->cqes = dma_alloc_coherent(&sdioc->pdev->dev, CQ_SIZE(depth),
					   &cqinfo->cq_dma_addr, GFP_KERNEL | __GFP_ZERO);
	if (!cqinfo->cqes) {
		ioc_err(sdioc, "failure at alloc dma space for cqueue.\n");
		return -ENOMEM;
	}

	sqinfo->sq_cmds = dma_alloc_coherent(&sdioc->pdev->dev, SQ_SIZE(qidx, depth),
					      &sqinfo->sq_dma_addr, GFP_KERNEL);
	if (!sqinfo->sq_cmds) {
		retval = -ENOMEM;
		ioc_err(sdioc, "failure at alloc dma space for squeue cmds.\n");
		goto  free_cqes;
	}

	/* alloc sense buffer */
	sqinfo->sense = dma_alloc_coherent(&sdioc->pdev->dev, SENSE_SIZE(depth),
					    &sqinfo->sense_dma_addr, GFP_KERNEL | __GFP_ZERO);
	if (!sqinfo->sense) {
		retval = -ENOMEM;
		ioc_err(sdioc, "failure at alloc dma space for sense data.\n");
		goto free_sq_cmds;
	}

	spin_lock_init(&sqinfo->sq_lock);
	spin_lock_init(&cqinfo->cq_lock);
	cqinfo->sdioc = sdioc;
	sqinfo->sdioc = sdioc;
	sqinfo->q_depth = depth;
	sqinfo->qidx = qidx;
	/* cq_vector replaced by msix_index */

	/*
	 * online_queues: completely initialized queue count: sssraid_init_queue
	 * queue_count: allocated but not completely initialized queue count: sssraid_alloc_queue
	 * online_queues/queue_count replaced by init_done_queue_cnt.
	 */
	sdioc->init_done_queue_cnt++;

	return 0;

free_sq_cmds:
	dma_free_coherent(&sdioc->pdev->dev, SQ_SIZE(qidx, depth), (void *)sqinfo->sq_cmds,
			  sqinfo->sq_dma_addr);
free_cqes:
	dma_free_coherent(&sdioc->pdev->dev, CQ_SIZE(depth), (void *)cqinfo->cqes,
			  cqinfo->cq_dma_addr);
	return retval;
}

static void sssraid_init_queue(struct sssraid_ioc *sdioc, u16 qidx)
{
	struct sssraid_cqueue *cqinfo = &sdioc->cqinfo[qidx];
	struct sssraid_squeue *sqinfo = &sdioc->sqinfo[qidx];

	memset((void *)cqinfo->cqes, 0, CQ_SIZE(sqinfo->q_depth));

	sqinfo->sq_tail = 0;
	cqinfo->cq_head = 0;
	cqinfo->cq_phase = 1;
	sqinfo->q_db = &sdioc->dbs[qidx * 2 * sdioc->db_stride];
	sqinfo->prp_small_pool = sdioc->prp_small_pool[qidx % small_pool_num];
}

static int sssraid_setup_admin_qpair(struct sssraid_ioc *sdioc)
{
	struct sssraid_cqueue *cqinfo = &sdioc->cqinfo[0];
	struct sssraid_squeue *sqinfo = &sdioc->sqinfo[0];
	u32 aqa;
	int retval;

	ioc_info(sdioc, "Starting disable ctrl...\n");

	retval = sssraid_disable_ctrl(sdioc);
	if (retval) {
		ioc_err(sdioc, "disable ctrl failed\n");
		return retval;
	}

	/* this func don't alloc admin queue */

	aqa = sqinfo->q_depth - 1;
	aqa |= aqa << 16;
	writel(aqa, sdioc->bar + SSSRAID_REG_AQA);
	lo_hi_writeq(sqinfo->sq_dma_addr, sdioc->bar + SSSRAID_REG_ASQ);
	lo_hi_writeq(cqinfo->cq_dma_addr, sdioc->bar + SSSRAID_REG_ACQ);

	ioc_info(sdioc, "Starting enable ctrl...\n");

	retval = sssraid_enable_ctrl(sdioc);
	if (retval) {
		ioc_err(sdioc, "enable ctrl failed\n");
		retval = -ENODEV;
		return retval;
	}

	/* interrupt registry not here */
	/* cq_vector replaced by msix_index */

	sssraid_init_queue(sdioc, 0);

	ioc_info(sdioc, "success, init done queuecount:[%d], pagesize[%d]\n",
		 sdioc->init_done_queue_cnt, sdioc->page_size);

	return 0;
}

static void sssraid_cleanup_isr(struct sssraid_ioc *sdioc)
{
	u16 i;

	sssraid_ioc_disable_intr(sdioc);

	if (!sdioc->intr_info)
		return;

	for (i = 0; i < sdioc->intr_info_count; i++)
		free_irq(pci_irq_vector(sdioc->pdev, i),
			(sdioc->intr_info + i));

	kfree(sdioc->intr_info);
	sdioc->intr_info = NULL;
	sdioc->intr_info_count = 0;
	pci_free_irq_vectors(sdioc->pdev);
}

static void sssraid_complete_adminq_cmnd(struct sssraid_ioc *sdioc, u16 qidx,
						struct sssraid_completion *cqe)
{
	struct sssraid_cmd *adm_cmd;

	adm_cmd = sdioc->adm_cmds + le16_to_cpu(cqe->cmd_id);
	if (unlikely(adm_cmd->state == SSSRAID_CMDSTAT_IDLE)) {
		ioc_warn(sdioc, "warn: invalid cmd id %d completed on queue %d\n",
			 le16_to_cpu(cqe->cmd_id), le16_to_cpu(cqe->sq_id));
		return;
	}

	adm_cmd->status = le16_to_cpu(cqe->status) >> 1;
	adm_cmd->result0 = le32_to_cpu(cqe->result);
	adm_cmd->result1 = le32_to_cpu(cqe->result1);

	complete(&adm_cmd->cmd_done);
}

static inline bool sssraid_cqe_pending(struct sssraid_cqueue *cqinfo)
{
	return (le16_to_cpu(cqinfo->cqes[cqinfo->cq_head].status) & 1) ==
		cqinfo->cq_phase;
}

static inline void sssraid_update_cq_head(struct sssraid_ioc *sdioc, u16 qidx)
{
	struct sssraid_cqueue *cqinfo = &sdioc->cqinfo[qidx];
	struct sssraid_squeue *sqinfo = &sdioc->sqinfo[qidx];

	if (++cqinfo->cq_head == sqinfo->q_depth) {
		cqinfo->cq_head = 0;
		cqinfo->cq_phase = !cqinfo->cq_phase;
	}
}

static inline bool sssraid_process_cq(struct sssraid_ioc *sdioc, u16 qidx, u16 *start,
					u16 *end, int tag)
{
	bool found = false;
	struct sssraid_cqueue *cqinfo = &sdioc->cqinfo[qidx];
	struct sssraid_squeue *sqinfo = &sdioc->sqinfo[qidx];

	*start = cqinfo->cq_head;
	while (!found && sssraid_cqe_pending(cqinfo)) {
		if (le16_to_cpu(cqinfo->cqes[cqinfo->cq_head].cmd_id) == tag)
			found = true;
		sssraid_update_cq_head(sdioc, qidx);
	}
	*end = cqinfo->cq_head;

	if (*start != *end)
		writel(cqinfo->cq_head, sqinfo->q_db + sqinfo->sdioc->db_stride);

	return found;
}

static irqreturn_t sssraid_isr(int irq, void *privdata)
{
	struct sssraid_intr_info *intr_info = privdata;
	struct sssraid_ioc *sdioc = intr_info->sdioc;
	irqreturn_t ret = IRQ_NONE;
	struct sssraid_cqueue *cqinfo;
	u16 midx, start, end;

	if (!intr_info)
		return IRQ_NONE;

	if (!sdioc->intr_enabled)
		return IRQ_NONE;

	midx = intr_info->msix_index;
	cqinfo = &sdioc->cqinfo[midx];

	spin_lock(&cqinfo->cq_lock);
	if (cqinfo->cq_head != cqinfo->last_cq_head)
		ret = IRQ_HANDLED;

	sssraid_process_cq(sdioc, midx, &start, &end, -1);
	cqinfo->last_cq_head = cqinfo->cq_head;
	spin_unlock(&cqinfo->cq_lock);

	if (start != end) {
		sssraid_complete_cqes(sdioc, midx, start, end);
		ret = IRQ_HANDLED;
	}
	return ret;
}

irqreturn_t sssraid_isr_poll(int irq, void *privdata)
{
	return IRQ_NONE;
}

bool sssraid_poll_cq(struct sssraid_ioc *sdioc, u16 qidx, int cid)
{
	u16 start, end;
	bool found;
	struct sssraid_cqueue *cqinfo = &sdioc->cqinfo[qidx];

	if (!sssraid_cqe_pending(cqinfo))
		return 0;

	spin_lock_irq(&cqinfo->cq_lock);
	found = sssraid_process_cq(sdioc, qidx, &start, &end, cid);
	spin_unlock_irq(&cqinfo->cq_lock);

	sssraid_complete_cqes(sdioc, qidx, start, end);
	return found;
}

static inline int sssraid_request_irq(struct sssraid_ioc *sdioc, u16 index)
{
	struct pci_dev *pdev = sdioc->pdev;
	struct sssraid_intr_info *intr_info = sdioc->intr_info + index;
	int retval = 0;

	intr_info->sdioc = sdioc;
	intr_info->msix_index = index;
	intr_info->cqinfo = NULL;

	snprintf(intr_info->name, SSSRAID_NAME_LENGTH, "%s%d-msix%d",
	    SSSRAID_DRIVER_NAME, sdioc->instance, index);

	retval = request_threaded_irq(pci_irq_vector(pdev, index), sssraid_isr,
	    sssraid_isr_poll, IRQF_SHARED, intr_info->name, intr_info);

	if (retval) {
		ioc_err(sdioc, "Err: %s: unable to allocate interrupt on vector %d!\n",
		    intr_info->name, pci_irq_vector(pdev, index));
		return retval;
	}

	return retval;
}

static int sssraid_setup_isr(struct sssraid_ioc *sdioc, u8 setup_one)
{
	unsigned int irq_flags = PCI_IRQ_MSIX;
	u16 max_vectors = 0, i;
	int retval = 0;

	struct irq_affinity desc = { .pre_vectors =  1};

	sssraid_cleanup_isr(sdioc);

	if (setup_one)
		max_vectors = 1;
	else {
		max_vectors = sdioc->before_affinity_msix_cnt;

		ioc_info(sdioc, "Before affinity, MSI-x vectors requested: %d\n", max_vectors);
	}

	irq_flags |= PCI_IRQ_AFFINITY | PCI_IRQ_ALL_TYPES;

	i = pci_alloc_irq_vectors_affinity(sdioc->pdev,
		1, max_vectors, irq_flags, &desc);

	if (i <= 0) {
		ioc_err(sdioc, "Err: alloc irq vectors fail.\n");
		goto out_failed;
	}
	if (i != max_vectors) {
		ioc_warn(sdioc,
		    "Allocated vectors (%d) are less than requested (%d)\n",
		    i, max_vectors);

		max_vectors = i;
	}

	sdioc->intr_info = kzalloc(sizeof(struct sssraid_intr_info) * max_vectors,
	    GFP_KERNEL);
	if (!sdioc->intr_info) {
		retval = -ENOMEM;
		ioc_err(sdioc, "err: failed to alloc memory for intr_info!\n");
		pci_free_irq_vectors(sdioc->pdev);
		goto out_failed;
	}

	for (i = 0; i < max_vectors; i++) {
		retval = sssraid_request_irq(sdioc, i);
		if (retval) {
			ioc_err(sdioc, "err: request irq for pci device failed.\n");
			sdioc->intr_info_count = i; /* =i is for offload interrupt loop counter */
			goto out_failed;
		}
	}

	/* intr_info_count replace max_qid */
	sdioc->intr_info_count = max_vectors;
	sssraid_ioc_enable_intr(sdioc);
	return retval;
out_failed:
	sssraid_cleanup_isr(sdioc);

	return retval;
}

static bool sssraid_adm_need_reset(struct sssraid_admin_command *cmd)
{
	switch (cmd->common.opcode) {
	case SSSRAID_ADM_DELETE_SQ:
	case SSSRAID_ADM_CREATE_SQ:
	case SSSRAID_ADM_DELETE_CQ:
	case SSSRAID_ADM_CREATE_CQ:
	case SSSRAID_ADM_SET_FEATURES:
		return false;
	default:
		return true;
	}
}

void sssraid_submit_cmd(struct sssraid_squeue *sqinfo, const void *cmd)
{
	u32 sqes = SQE_SIZE(sqinfo->qidx);
	unsigned long flags;
	struct sssraid_admin_common_command *acd = (struct sssraid_admin_common_command *)cmd;

	spin_lock_irqsave(&sqinfo->sq_lock, flags);
	memcpy((sqinfo->sq_cmds + sqes * sqinfo->sq_tail), cmd, sqes);
	if (++sqinfo->sq_tail == sqinfo->q_depth)
		sqinfo->sq_tail = 0;

	writel(sqinfo->sq_tail, sqinfo->q_db);
	spin_unlock_irqrestore(&sqinfo->sq_lock, flags);

	dbgprint(sqinfo->sdioc, "cid[%d] qidx[%d], opcode[0x%x], flags[0x%x], hdid[%u]\n",
		    le16_to_cpu(acd->command_id), sqinfo->qidx, acd->opcode, acd->flags,
		    le32_to_cpu(acd->hdid));
}

int sssraid_submit_admin_sync_cmd(struct sssraid_ioc *sdioc, struct sssraid_admin_command *cmd,
					u32 *result0, u32 *result1, u32 timeout)
{
	struct sssraid_cmd *adm_cmd = sssraid_get_cmd(sdioc, SSSRAID_CMD_ADM);

	if (!adm_cmd) {
		ioc_err(sdioc, "err: get admin cmd failed\n");
		return -EFAULT;
	}

	timeout = timeout ? timeout : ADMIN_TIMEOUT;

	/*
	 * watch dog not as optimized as
	 * init_completion/complete
	 */
	init_completion(&adm_cmd->cmd_done);

	cmd->common.command_id = cpu_to_le16(adm_cmd->cid);
	sssraid_submit_cmd(&sdioc->sqinfo[0], cmd);

	if (!wait_for_completion_timeout(&adm_cmd->cmd_done, timeout)) {
		ioc_err(sdioc, "err: cid[%d] qidx[%d] timeout, opcode[0x%x] subopcode[0x%x]\n",
			adm_cmd->cid, adm_cmd->qid, cmd->usr_cmd.opcode,
			cmd->usr_cmd.info_0.subopcode);

		/* reset controller if admin timeout */
		if (sssraid_adm_need_reset(cmd))
			sssraid_adm_timeout(sdioc, adm_cmd);

		sssraid_put_cmd(sdioc, adm_cmd, SSSRAID_CMD_ADM);
		return -ETIME;
	}

	if (result0)
		*result0 = adm_cmd->result0;
	if (result1)
		*result1 = adm_cmd->result1;

	sssraid_put_cmd(sdioc, adm_cmd, SSSRAID_CMD_ADM);

	return adm_cmd->status;
}

static int sssraid_get_ctrl_info(struct sssraid_ioc *sdioc, struct sssraid_ctrl_info *ctrl_info)
{
	struct sssraid_admin_command admin_cmd;
	u8 *data_ptr = NULL;
	dma_addr_t data_dma = 0;
	int retval;

	data_ptr = dma_alloc_coherent(&sdioc->pdev->dev, PAGE_SIZE, &data_dma, GFP_KERNEL);
	if (!data_ptr)
		return -ENOMEM;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.get_info.opcode = SSSRAID_ADM_GET_INFO;
	admin_cmd.get_info.type = SSSRAID_GET_INFO_CTRL;
	admin_cmd.common.dptr.prp1 = cpu_to_le64(data_dma);

	retval = sssraid_submit_admin_sync_cmd(sdioc, &admin_cmd, NULL, NULL, 0);
	if (!retval)
		memcpy(ctrl_info, data_ptr, sizeof(struct sssraid_ctrl_info));

	dma_free_coherent(&sdioc->pdev->dev, PAGE_SIZE, data_ptr, data_dma);

	return retval;
}

int sssraid_init_ctrl_info(struct sssraid_ioc *sdioc)
{
	int retval;

	sdioc->ctrl_info->nd = cpu_to_le32(240);
	sdioc->ctrl_info->mdts = 8;
	sdioc->ctrl_info->max_cmds = cpu_to_le16(4096);
	sdioc->ctrl_info->max_num_sge = cpu_to_le16(128);
	sdioc->ctrl_info->max_channel = cpu_to_le16(4);
	sdioc->ctrl_info->max_tgt_id = cpu_to_le32(3239);
	sdioc->ctrl_info->max_lun = cpu_to_le16(2);

	retval = sssraid_get_ctrl_info(sdioc, sdioc->ctrl_info);
	if (retval)
		ioc_err(sdioc, "err: fetch controller info fail, ret = %d\n", retval);

	ioc_info(sdioc, "support disk cnt   = %d\n", le32_to_cpu(sdioc->ctrl_info->nd));
	ioc_info(sdioc, "max concurrent cmd = %d\n", le16_to_cpu(sdioc->ctrl_info->max_cmds));
	ioc_info(sdioc, "max channel        = %d\n", le16_to_cpu(sdioc->ctrl_info->max_channel));
	ioc_info(sdioc, "max target         = %d\n", le32_to_cpu(sdioc->ctrl_info->max_tgt_id));
	ioc_info(sdioc, "max lun            = %d\n", le16_to_cpu(sdioc->ctrl_info->max_lun));
	ioc_info(sdioc, "max sg entry cnt   = %d\n", le16_to_cpu(sdioc->ctrl_info->max_num_sge));
	ioc_info(sdioc, "lun boot num       = %d\n",
			le16_to_cpu(sdioc->ctrl_info->lun_num_in_boot));
	ioc_info(sdioc, "buf in 4K size     = %d\n", sdioc->ctrl_info->mdts);
	ioc_info(sdioc, "ACL = %d\n", sdioc->ctrl_info->acl);
	ioc_info(sdioc, "async evt req depth= %d\n", sdioc->ctrl_info->aerl);
	ioc_info(sdioc, "card type          = %d\n", sdioc->ctrl_info->card_type);
	ioc_info(sdioc, "timeout in microsec= %d\n", le32_to_cpu(sdioc->ctrl_info->rtd3e));
	ioc_info(sdioc, "serial number      = %s\n", sdioc->ctrl_info->sn);
	ioc_info(sdioc, "FW version         = %s\n", sdioc->ctrl_info->fr);

	if (!sdioc->ctrl_info->aerl)
		sdioc->ctrl_info->aerl = 1;
	if (sdioc->ctrl_info->aerl > SSSRAID_NR_AEN_CMDS)
		sdioc->ctrl_info->aerl = SSSRAID_NR_AEN_CMDS;

	return 0;
}

static int sssraid_set_features(struct sssraid_ioc *sdioc, u32 fid, u32 dword11, void *buffer,
			       size_t buflen, u32 *result)
{
	struct sssraid_admin_command admin_cmd;
	int ret;
	u8 *data_ptr = NULL;
	dma_addr_t data_dma = 0;

	if (buffer && buflen) {
		data_ptr = dma_alloc_coherent(&sdioc->pdev->dev, buflen, &data_dma, GFP_KERNEL);
		if (!data_ptr)
			return -ENOMEM;

		memcpy(data_ptr, buffer, buflen);
	}

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.features.opcode = SSSRAID_ADM_SET_FEATURES;
	admin_cmd.features.fid = cpu_to_le32(fid);
	admin_cmd.features.dword11 = cpu_to_le32(dword11);
	admin_cmd.common.dptr.prp1 = cpu_to_le64(data_dma);

	ret = sssraid_submit_admin_sync_cmd(sdioc, &admin_cmd, result, NULL, 0);

	if (data_ptr)
		dma_free_coherent(&sdioc->pdev->dev, buflen, data_ptr, data_dma);

	return ret;
}

static int sssraid_set_queue_cnt(struct sssraid_ioc *sdioc, u32 *cnt)
{
	u32 q_cnt = (*cnt - 1) | ((*cnt - 1) << 16);
	u32 nr_ioqs, result;
	int status;

	status = sssraid_set_features(sdioc, SSSRAID_FEAT_NUM_QUEUES, q_cnt, NULL, 0, &result);
	if (status) {
		ioc_err(sdioc, "err: set queue count failed, status: %d\n",
			status);
		return -EIO;
	}

	nr_ioqs = min(result & 0xffff, result >> 16) + 1;
	*cnt = min(*cnt, nr_ioqs);
	if (*cnt == 0) {
		ioc_err(sdioc, "err: illegal queue count: zero\n");
		return -EIO;
	}
	return 0;
}

static int sssraid_create_cq(struct sssraid_ioc *sdioc, u16 qidx)
{
	struct sssraid_cqueue *cqinfo = &sdioc->cqinfo[qidx];
	struct sssraid_squeue *sqinfo = &sdioc->sqinfo[qidx];
	struct sssraid_admin_command admin_cmd;
	int flags = SSSRAID_QUEUE_PHYS_CONTIG | SSSRAID_CQ_IRQ_ENABLED;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.create_cq.opcode = SSSRAID_ADM_CREATE_CQ;
	admin_cmd.create_cq.prp1 = cpu_to_le64(cqinfo->cq_dma_addr);
	admin_cmd.create_cq.cqid = cpu_to_le16(qidx);
	admin_cmd.create_cq.qsize = cpu_to_le16(sqinfo->q_depth - 1);
	admin_cmd.create_cq.cq_flags = cpu_to_le16(flags);
	admin_cmd.create_cq.irq_vector = cpu_to_le16(qidx);

	return sssraid_submit_admin_sync_cmd(sdioc, &admin_cmd, NULL, NULL, 0);
}

static int sssraid_create_io_cq(struct sssraid_ioc *sdioc, u16 qidx)
{
	int retval;
	struct sssraid_cqueue *cqinfo = sdioc->cqinfo + qidx;
	u16 midx = qidx;

	retval = sssraid_create_cq(sdioc, qidx);
	if (retval)
		return retval;

	/*
	 * cqinfo initialization at sssraid_init_queue
	 */
	sdioc->intr_info[midx].cqinfo = cqinfo;

	return retval;
}

static int sssraid_create_sq(struct sssraid_ioc *sdioc, u16 qidx)
{
	struct sssraid_squeue *sqinfo = &sdioc->sqinfo[qidx];
	struct sssraid_admin_command admin_cmd;
	int flags = SSSRAID_QUEUE_PHYS_CONTIG;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.create_sq.opcode = SSSRAID_ADM_CREATE_SQ;
	admin_cmd.create_sq.prp1 = cpu_to_le64(sqinfo->sq_dma_addr);
	admin_cmd.create_sq.sqid = cpu_to_le16(qidx);
	admin_cmd.create_sq.qsize = cpu_to_le16(sqinfo->q_depth - 1);
	admin_cmd.create_sq.sq_flags = cpu_to_le16(flags);
	admin_cmd.create_sq.cqid = cpu_to_le16(qidx);

	return sssraid_submit_admin_sync_cmd(sdioc, &admin_cmd, NULL, NULL, 0);
}

static int sssraid_create_io_sq(struct sssraid_ioc *sdioc, u16 qidx)
{
	return sssraid_create_sq(sdioc, qidx);
}

int sssraid_get_dev_list(struct sssraid_ioc *sdioc, struct sssraid_dev_info *devices)
{
	u32 nd = le32_to_cpu(sdioc->ctrl_info->nd);
	struct sssraid_admin_command admin_cmd;
	struct sssraid_dev_list *list_buf;
	dma_addr_t data_dma = 0;
	u32 i, idx, hdid, ndev;
	int ret = 0;

	list_buf = dma_alloc_coherent(&sdioc->pdev->dev, PAGE_SIZE, &data_dma, GFP_KERNEL);
	if (!list_buf)
		return -ENOMEM;

	for (idx = 0; idx < nd;) {
		memset(&admin_cmd, 0, sizeof(admin_cmd));
		admin_cmd.get_info.opcode = SSSRAID_ADM_GET_INFO;
		admin_cmd.get_info.type = SSSRAID_GET_INFO_DEV_LIST;
		admin_cmd.get_info.cdw11 = cpu_to_le32(idx);
		admin_cmd.common.dptr.prp1 = cpu_to_le64(data_dma);

		ret = sssraid_submit_admin_sync_cmd(sdioc, &admin_cmd, NULL, NULL, 0);

		if (ret) {
			ioc_err(sdioc, "Err: Get FW disk list failed, support nd: %u, idx: %u, ret: %d\n",
				nd, idx, ret);
			goto out;
		}
		ndev = le32_to_cpu(list_buf->dev_num);

		ioc_info(sdioc, "ndev numbers: %u\n", ndev);

		for (i = 0; i < ndev; i++) {
			hdid = le32_to_cpu(list_buf->devices[i].hdid);
			ioc_info(sdioc, "Get FW disk: %u, hdid: %u, target: %d, channel: %d, lun: %d, attr[0x%x]\n",
				 i, hdid, le16_to_cpu(list_buf->devices[i].target),
				 list_buf->devices[i].channel,
				 list_buf->devices[i].lun,
				 list_buf->devices[i].attr);
			if (hdid > nd || hdid == 0) {
				ioc_err(sdioc, "Err: hdid: %d invalid\n", hdid);
				continue;
			}
			memcpy(&devices[hdid - 1], &list_buf->devices[i],
			       sizeof(struct sssraid_dev_info));
		}
		idx += ndev;

		if (ndev < MAX_DEV_ENTRY_PER_PAGE_4K)
			break;
	}

out:
	dma_free_coherent(&sdioc->pdev->dev, PAGE_SIZE, list_buf, data_dma);
	return ret;
}

/* send abort command by admin queue temporary */
int sssraid_send_abort_cmd(struct sssraid_ioc *sdioc, u32 hdid, u16 qidx, u16 cid)
{
	struct sssraid_admin_command admin_cmd;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.abort.opcode = SSSRAID_ADM_ABORT_CMD;
	admin_cmd.abort.hdid = cpu_to_le32(hdid);
	admin_cmd.abort.sqid = cpu_to_le16(qidx);
	admin_cmd.abort.cid = cpu_to_le16(cid);

	return sssraid_submit_admin_sync_cmd(sdioc, &admin_cmd, NULL, NULL, 0);
}

/* send reset command by admin quueue temporary */
int sssraid_send_reset_cmd(struct sssraid_ioc *sdioc, u8 type, u32 hdid)
{
	struct sssraid_admin_command admin_cmd;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.reset.opcode = SSSRAID_ADM_RESET;
	admin_cmd.reset.hdid = cpu_to_le32(hdid);
	admin_cmd.reset.type = type;

	return sssraid_submit_admin_sync_cmd(sdioc, &admin_cmd, NULL, NULL, 0);
}

static int sssraid_delete_queue(struct sssraid_ioc *sdioc, u8 op, u16 qidx)
{
	struct sssraid_admin_command admin_cmd;
	int retval;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.delete_queue.opcode = op;
	admin_cmd.delete_queue.qid = cpu_to_le16(qidx);

	retval = sssraid_submit_admin_sync_cmd(sdioc, &admin_cmd, NULL, NULL, 0);

	if (retval)
		ioc_err(sdioc, "Err: Delete %s:[%d] failed\n",
			(op == SSSRAID_ADM_DELETE_CQ) ? "cq" : "sq", qidx);

	return retval;
}

static int sssraid_delete_cq(struct sssraid_ioc *sdioc, u16 qidx)
{
	return sssraid_delete_queue(sdioc, SSSRAID_ADM_DELETE_CQ, qidx);
}

void sssraid_adm_timeout(struct sssraid_ioc *sdioc, struct sssraid_cmd *cmd)
{
	/* command may be returned because controller reset */
	if (READ_ONCE(cmd->state) == SSSRAID_CMDSTAT_COMPLETE)
		return;

	if (!sssraid_change_host_state(sdioc, SSSRAID_RESETTING)) {
		ioc_info(sdioc, "Can't change to reset state\n");
		return;
	}
	sssraid_soft_reset_handler(sdioc);
}

static int sssraid_create_io_qpair(struct sssraid_ioc *sdioc, u16 qidx)
{
	int retval;

	retval = sssraid_create_io_cq(sdioc, qidx);
	if (retval)
		return retval;

	retval = sssraid_create_io_sq(sdioc, qidx);
	if (retval)
		goto delete_cq;

	/* intr_info.msix_index substitute cq_vector */

	/* io interrupt registry:
	 * not here, put above
	 */

	sssraid_init_queue(sdioc, qidx);

	return 0;

delete_cq:
	sssraid_delete_cq(sdioc, qidx);

	return retval;
}

static int sssraid_setup_io_qpair(struct sssraid_ioc *sdioc)
{
	u32 i, num_queues;
	int retval = 0;

	num_queues = min(sdioc->intr_info_count, sdioc->init_done_queue_cnt - 1);
	for (i = 1; i <= num_queues; i++) {
		retval = sssraid_create_io_qpair(sdioc, i);
		if (retval) {
			ioc_err(sdioc, "Err: Create queue[%d] failed\n", i);
			break;
		}
	}

	ioc_info(sdioc, "init_done_queue_cnt[%d], intr_info_count[%d] num_queues[%d]",
		 sdioc->init_done_queue_cnt,
		 sdioc->intr_info_count, num_queues);

	return retval >= 0 ? 0 : retval;
}

static int sssraid_alloc_ioq_ptcmds(struct sssraid_ioc *sdioc)
{
	int i;
	int ptnum = SSSRAID_NR_IOQ_PTCMDS;

	INIT_LIST_HEAD(&sdioc->ioq_pt_list);
	spin_lock_init(&sdioc->ioq_pt_lock);

	sdioc->ioq_ptcmds = kcalloc_node(ptnum, sizeof(struct sssraid_cmd),
					GFP_KERNEL, sdioc->numa_node);

	if (!sdioc->ioq_ptcmds) {
		ioc_err(sdioc, "Err: Alloc sync ioq ptcmds failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < ptnum; i++) {
		sdioc->ioq_ptcmds[i].qid = i / SSSRAID_PTCMDS_PERQ + 1;
		sdioc->ioq_ptcmds[i].cid = i % SSSRAID_PTCMDS_PERQ + SSSRAID_IO_BLK_MQ_DEPTH;
		list_add_tail(&(sdioc->ioq_ptcmds[i].list), &sdioc->ioq_pt_list);
	}

	ioc_info(sdioc, "Alloc sync ioq ptcmds success, ptnum: %d\n", ptnum);

	return 0;
}

int sssraid_send_event_ack(struct sssraid_ioc *sdioc, u8 event,
	u32 event_ctx, u16 cid)
{
	/* event,event_ctx no use at this time */
	struct sssraid_squeue *sqinfo = &sdioc->sqinfo[0];
	struct sssraid_admin_command admin_cmd;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.common.opcode = SSSRAID_ADM_ASYNC_EVENT;
	admin_cmd.common.command_id = cpu_to_le16(cid);

	sssraid_submit_cmd(sqinfo, &admin_cmd);
	ioc_info(sdioc, "send async evt ack, cid[%d]\n", cid);

	return 0;
}

static void sssraid_handle_aen_notice(struct sssraid_ioc *sdioc, u32 result)
{
	switch ((result & 0xff00) >> 8) {
	case SSSRAID_AEN_DEV_CHANGED:
		sssraid_scan_disk(sdioc);
		break;
	case SSSRAID_AEN_FW_ACT_START:
		ioc_info(sdioc, "Activating FW starting\n");
		break;
	case SSSRAID_AEN_HOST_PROBING:
		break;
	default:
		ioc_warn(sdioc, "warn: async evt result %08x\n", result);
	}
}

static void sssraid_handle_aen_vs(struct sssraid_ioc *sdioc, u32 result, u32 result1)
{
	switch ((result & 0xff00) >> 8) {
	case SSSRAID_AEN_TIMESYN:
		sssraid_configure_timestamp(sdioc);
		break;
	case SSSRAID_AEN_FW_ACT_FINISH:
		ioc_info(sdioc, "Activating FW finish\n");
		if (sssraid_init_ctrl_info(sdioc))
			ioc_err(sdioc, "Err: fetch ctrl info failed after fw act\n");
		break;
	case SSSRAID_AEN_EVENT_MIN ... SSSRAID_AEN_EVENT_MAX:
		ioc_info(sdioc, "Rcv card async evt[%d], param1[0x%x] param2[0x%x]\n",
			 (result & 0xff00) >> 8, result, result1);
		break;
	default:
		ioc_warn(sdioc, "warn: async evt result: 0x%x\n", result);
	}
}

static inline void sssraid_send_all_aen(struct sssraid_ioc *sdioc)
{
	u16 i;

	for (i = 0; i < sdioc->ctrl_info->aerl; i++)
		sssraid_send_event_ack(sdioc, 0, 0, i + SSSRAID_AMDQ_BLK_MQ_DEPTH);
}

static int sssraid_disk_list_init(struct sssraid_ioc *sdioc)
{
	u32 nd = le32_to_cpu(sdioc->ctrl_info->nd);

	sdioc->devices = kzalloc_node(nd * sizeof(struct sssraid_dev_info),
				     GFP_KERNEL, sdioc->numa_node);
	if (!sdioc->devices) {
		ioc_err(sdioc, "err: failed to alloc memory for device info.\n");
		return -ENOMEM;
	}

	return 0;
}

int sssraid_configure_timestamp(struct sssraid_ioc *sdioc)
{
	__le64 ts;
	int retval;

	ts = cpu_to_le64(ktime_to_ms(ktime_get_real()));
	retval = sssraid_set_features(sdioc, SSSRAID_FEAT_TIMESTAMP, 0, &ts, sizeof(ts), NULL);

	if (retval)
		ioc_err(sdioc, "Err: set timestamp fail, ret: %d\n", retval);
	return retval;
}

int sssraid_init_ioc(struct sssraid_ioc *sdioc, u8 re_init)
{
	int retval = 0;
	int i;
	u32 nr_ioqs, bar_size;

	if (!re_init) {
		sdioc->cpu_count = num_online_cpus();

		retval = sssraid_alloc_resources(sdioc);
		if (retval) {
			ioc_err(sdioc, "Err: Failed to alloc resources, ret %d\n",
			    retval);
			goto out_nocleanup;
		}
	}

	/* reset need re-setup */
	retval = sssraid_setup_resources(sdioc);
	if (retval) {
		ioc_err(sdioc, "Err: Failed to setup resources, ret %d\n",
		    retval);
		goto out_failed;
	}

	if (!re_init) {
		retval = sssraid_alloc_admin_cmds(sdioc);
		if (retval) {
			ioc_err(sdioc, "Err: Failed to alloc admin cmds, ret %d\n",
			    retval);
			goto out_failed;
		}
		/* put here:
		 * alloc admin queue
		 */
		retval = sssraid_alloc_qpair(sdioc, 0, SSSRAID_ADMQ_DEPTH);
		if (retval) {
			ioc_err(sdioc, "Err: Failed to alloc admin queue, ret %d\n",
			    retval);
			goto out_failed;
		}
	}

	retval = sssraid_setup_admin_qpair(sdioc);
	if (retval) {
		ioc_err(sdioc, "Err: Failed to setup admin queue, ret %d\n",
		    retval);
		goto out_failed;
	}

	/* 1. unregister all interrupt
	 * 2. admin interrupt registry
	 */
	retval = sssraid_setup_isr(sdioc, 1);
	if (retval) {
		ioc_err(sdioc, "Failed to setup ISR error %d\n",
		    retval);
		goto out_failed;
	}

	retval = sssraid_init_ctrl_info(sdioc);
	if (retval) {
		ioc_err(sdioc, "Failed to get ctrl info error %d\n",
		    retval);
		goto out_failed;
	}

	nr_ioqs = sdioc->cpu_count;
	retval = sssraid_set_queue_cnt(sdioc, &nr_ioqs);
	if (retval) {
		ioc_err(sdioc, "Failed to set queue cnt error %d\n",
		    retval);
		goto out_failed;
	}

	sdioc->before_affinity_msix_cnt = nr_ioqs + 1;

	/* 1. unregister all interrupt
	 * 2. admin interrupt re-registry
	 * 3. io interrupt registry
	 */
	retval = sssraid_setup_isr(sdioc, 0);
	if (retval) {
		ioc_err(sdioc, "Failed to re-setup ISR, error %d\n",
		    retval);
		goto out_failed;
	}

	/* remap */
	bar_size = SSSRAID_REG_DBS + ((nr_ioqs + 1) * 8 * sdioc->db_stride);
	retval = sssraid_remap_bar(sdioc, bar_size);
	if (retval) {
		ioc_err(sdioc, "Failed to re-map bar, error %d\n",
			    retval);
		goto out_failed;
	}
	sdioc->sqinfo[0].q_db = sdioc->dbs;

	/* num_vecs no sense, abandon */

	if (!re_init) {
		for (i = sdioc->init_done_queue_cnt; i < sdioc->intr_info_count; i++) {
			retval = sssraid_alloc_qpair(sdioc, i, sdioc->ioq_depth);
			if (retval) {
				ioc_err(sdioc, "Failed to alloc io queue:error %d\n",
					retval);
				goto out_failed;
			}
		}
		ioc_info(sdioc, "intr_info_count: %d, init_done_queue_cnt: %d, ioq_depth: %d\n",
			sdioc->intr_info_count, sdioc->init_done_queue_cnt, sdioc->ioq_depth);
	}

	retval = sssraid_setup_io_qpair(sdioc);
	if (retval) {
		ioc_err(sdioc, "Failed to setup io qpair, error %d\n",
			    retval);
		goto out_failed;
	}

	if (!re_init) {
		retval = sssraid_alloc_ioq_ptcmds(sdioc);
		if (retval) {
			ioc_err(sdioc, "Failed to alloc ioq ptcmds, error %d\n",
					retval);
			goto out_failed;
		}
	}

	sssraid_send_all_aen(sdioc);

	if (!re_init) {
		retval = sssraid_disk_list_init(sdioc);
		if (retval) {
			ioc_err(sdioc, "Failed to init device list, error %d\n",
					retval);
			goto out_failed;
		}

		retval = sssraid_configure_timestamp(sdioc);
		if (retval) {
			ioc_err(sdioc, "Failed to configure timestamp, error %d\n",
					retval);
			goto out_failed;
		}
	}

	return retval;

out_failed:
	sssraid_cleanup_ioc(sdioc, re_init);
out_nocleanup:
	return retval;
}

void sssraid_cleanup_resources(struct sssraid_ioc *sdioc)
{
	struct pci_dev *pdev = sdioc->pdev;

	pci_set_drvdata(pdev, NULL);
	sssraid_cleanup_isr(sdioc);

	if (sdioc->bar) {
		iounmap(sdioc->bar);
		sdioc->bar = NULL;
	}

	if (pci_is_enabled(pdev)) {
		pci_release_mem_regions(pdev);
		pci_disable_device(pdev);
	}
}

static void sssraid_free_disk_list(struct sssraid_ioc *sdioc)
{
	kfree(sdioc->devices);
	sdioc->devices = NULL;
}

static void sssraid_free_ioq_ptcmds(struct sssraid_ioc *sdioc)
{
	kfree(sdioc->ioq_ptcmds);
	sdioc->ioq_ptcmds = NULL;

	INIT_LIST_HEAD(&sdioc->ioq_pt_list);
}

static void sssraid_delete_io_queues(struct sssraid_ioc *sdioc)
{
	u16 queues = sdioc->init_done_queue_cnt - SSSRAID_ADM_QUEUE_NUM;
	u8 opcode = SSSRAID_ADM_DELETE_SQ;
	u16 i, pass;

	if (!pci_device_is_present(sdioc->pdev)) {
		ioc_err(sdioc, "Err: controller is not present, skip disable io queues\n");
		return;
	}

	if (sdioc->init_done_queue_cnt <= SSSRAID_ADM_QUEUE_NUM) {
		ioc_err(sdioc, "Err: io queue has been delete\n");
		return;
	}

	for (pass = 0; pass < 2; pass++) {
		for (i = queues; i > 0; i--)
			if (sssraid_delete_queue(sdioc, opcode, i))
				break;

		opcode = SSSRAID_ADM_DELETE_CQ;
	}
}

void sssraid_complete_aen(struct sssraid_ioc *sdioc, struct sssraid_completion *cqe)
{
	u32 result = le32_to_cpu(cqe->result);

	ioc_info(sdioc, "Rcv async evt, cid[%d], status[0x%x], result[0x%x]\n",
		 le16_to_cpu(cqe->cmd_id), le16_to_cpu(cqe->status) >> 1, result);

	/*
	 * The response to event moved from this func.
	 * sssraid_send_aen changed to name sssraid_send_event_ack
	 */

	if ((le16_to_cpu(cqe->status) >> 1) != SSSRAID_SC_SUCCESS)
		return;
	switch (result & 0x7) {
	case SSSRAID_AEN_NOTICE:
		sssraid_handle_aen_notice(sdioc, result);
		break;
	case SSSRAID_AEN_VS:
		sssraid_handle_aen_vs(sdioc, result, le32_to_cpu(cqe->result1));
		break;
	default:
		ioc_warn(sdioc, "warn: unsupported async event type: %u\n",
			 result & 0x7);
		break;
	}
}

void sssraid_free_iod_res(struct sssraid_ioc *sdioc, struct sssraid_iod *iod)
{
	const int last_prp = sdioc->page_size / sizeof(__le64) - 1;
	dma_addr_t dma_addr, next_dma_addr;
	struct sssraid_sgl_desc *sg_list;
	__le64 *prp_list;
	void *addr;
	int i;

	dma_addr = iod->first_dma;
	if (iod->npages == 0)
		dma_pool_free(iod->sqinfo->prp_small_pool, sssraid_iod_list(iod)[0], dma_addr);

	for (i = 0; i < iod->npages; i++) {
		addr = sssraid_iod_list(iod)[i];

		if (iod->use_sgl) {
			sg_list = addr;
			next_dma_addr =
				le64_to_cpu((sg_list[SGES_PER_PAGE - 1]).addr);
		} else {
			prp_list = addr;
			next_dma_addr = le64_to_cpu(prp_list[last_prp]);
		}

		dma_pool_free(sdioc->prp_page_pool, addr, dma_addr);
		dma_addr = next_dma_addr;
	}

	iod->sense = NULL;
	iod->npages = -1;
}

static void sssraid_complete_ioq_sync_cmnd(struct sssraid_ioc *sdioc, u16 qidx,
						struct sssraid_completion *cqe)
{
	struct sssraid_cmd *ptcmd;
	struct sssraid_squeue *sqinfo = &sdioc->sqinfo[qidx];

	ptcmd = sdioc->ioq_ptcmds + (sqinfo->qidx - 1) * SSSRAID_PTCMDS_PERQ +
		le16_to_cpu(cqe->cmd_id) - SSSRAID_IO_BLK_MQ_DEPTH;

	ptcmd->status = le16_to_cpu(cqe->status) >> 1;
	ptcmd->result0 = le32_to_cpu(cqe->result);
	ptcmd->result1 = le32_to_cpu(cqe->result1);

	complete(&ptcmd->cmd_done);
}

static void sssraid_complete_ioq_cmnd(struct sssraid_ioc *sdioc, u16 qidx,
					struct sssraid_completion *cqe)
{
	struct sssraid_squeue *sqinfo = &sdioc->sqinfo[qidx];

	struct blk_mq_tags *tags;
	struct scsi_cmnd *scmd;
	struct sssraid_iod *iod;
	struct request *req;
	unsigned long elapsed;

	tags = sdioc->shost->tag_set.tags[sqinfo->qidx - 1];

	req = blk_mq_tag_to_rq(tags, le16_to_cpu(cqe->cmd_id));
	if (unlikely(!req || !blk_mq_request_started(req))) {
		ioc_warn(sdioc, "warn: invalid cmd id %d completed on queue %d\n",
			 le16_to_cpu(cqe->cmd_id), sqinfo->qidx);
		return;
	}

	scmd = blk_mq_rq_to_pdu(req);
	iod = scsi_cmd_priv(scmd);

	elapsed = jiffies - scmd->jiffies_at_alloc;
	dbgprint(sdioc, "cid[%d] qidx[%d] finish IO cost %3ld.%3ld seconds\n",
		    le16_to_cpu(cqe->cmd_id), sqinfo->qidx, elapsed / HZ, elapsed % HZ);

	if (cmpxchg(&iod->state, SSSRAID_CMDSTAT_FLIGHT, SSSRAID_CMDSTAT_COMPLETE) !=
		SSSRAID_CMDSTAT_FLIGHT) {
		ioc_warn(sdioc, "warn: cid[%d] qidx[%d] enters abnormal handler, cost %3ld.%3ld seconds\n",
			 le16_to_cpu(cqe->cmd_id), sqinfo->qidx, elapsed / HZ, elapsed % HZ);
		WRITE_ONCE(iod->state, SSSRAID_CMDSTAT_TMO_COMPLETE);

		if (iod->nsge) {
			iod->nsge = 0;
			scsi_dma_unmap(scmd);
		}
		sssraid_free_iod_res(sdioc, iod);

		return;
	}

	sssraid_map_status(iod, scmd, cqe);
	if (iod->nsge) {
		iod->nsge = 0;
		scsi_dma_unmap(scmd);
	}
	sssraid_free_iod_res(sdioc, iod);
	scsi_done(scmd);
}

static void sssraid_process_admin_cq(struct sssraid_ioc *sdioc,
	struct sssraid_squeue *sqinfo,
	struct sssraid_completion *cqe)
{
	struct sssraid_fwevt *fwevt = NULL;
	u16 cid = le16_to_cpu(cqe->cmd_id), sz;

	if (likely(cid < SSSRAID_AMDQ_BLK_MQ_DEPTH))
		sssraid_complete_adminq_cmnd(sdioc, sqinfo->qidx, cqe);
	else {
		sz = sizeof(*cqe);
		fwevt = sssraid_alloc_fwevt(sz);
		if (!fwevt) {
			ioc_err(sdioc, "%s :failure at %s:%d/%s()!\n",
			    __func__, __FILE__, __LINE__, __func__);
			return;
		}

		memcpy(fwevt->event_data, cqe, sz);
		fwevt->sdioc = sdioc;
		fwevt->event_id = 0; /* evt_type:0 */
		fwevt->send_ack = 1; /* ack_req:1 */
		fwevt->process_evt = 1; /* process_evt_bh:1 */
		fwevt->evt_ctx = 0; /* 0 */
		sssraid_fwevt_add_to_list(sdioc, fwevt);
	}
}

static void sssraid_process_io_cq(struct sssraid_ioc *sdioc,
	struct sssraid_squeue *sqinfo,
	struct sssraid_completion *cqe)
{
	u16 cid = le16_to_cpu(cqe->cmd_id);

	if (likely(cid < SSSRAID_IO_BLK_MQ_DEPTH))
		sssraid_complete_ioq_cmnd(sdioc, sqinfo->qidx, cqe);
	else /* io sync handle */
		sssraid_complete_ioq_sync_cmnd(sdioc, sqinfo->qidx, cqe);
}

static inline void sssraid_handle_cqe(struct sssraid_ioc *sdioc, u16 mdix, u16 didx)
{
	struct sssraid_cqueue *cqinfo = &sdioc->cqinfo[mdix];
	struct sssraid_squeue *sqinfo = &sdioc->sqinfo[mdix];

	struct sssraid_completion *cqe = &cqinfo->cqes[didx];
	u16 cid = le16_to_cpu(cqe->cmd_id);

	if (unlikely(cid >= sqinfo->q_depth)) {
		ioc_err(sdioc, "Err: invalid command id[%d] completed on queue %d\n",
			cid, cqe->sq_id);
		return;
	}

	dbgprint(sdioc, "cid[%d] mdix[%d], result[0x%x], sq_id[%d], status[0x%x]\n",
		    cid, sqinfo->qidx, le32_to_cpu(cqe->result),
		    le16_to_cpu(cqe->sq_id), le16_to_cpu(cqe->status));

	if (!mdix) /* admin */
		sssraid_process_admin_cq(sdioc, sqinfo, cqe);
	else /* io */
		sssraid_process_io_cq(sdioc, sqinfo, cqe);
}

void sssraid_complete_cqes(struct sssraid_ioc *sdioc, u16 midx, u16 start, u16 end)
{
	struct sssraid_squeue *sqinfo = &sdioc->sqinfo[midx];

	while (start != end) {
		sssraid_handle_cqe(sdioc, midx, start);
		if (++start == sqinfo->q_depth)
			start = 0;
	}
}

static int sssraid_disable_admin_queue(struct sssraid_ioc *sdioc, bool shutdown)
{
	struct sssraid_cqueue *adm_cqinfo = &sdioc->cqinfo[0];
	u16 start, end;
	int ret = 0;

	if (pci_device_is_present(sdioc->pdev)) {
		if (shutdown)
			sssraid_shutdown_ctrl(sdioc);
		else
			ret = sssraid_disable_ctrl(sdioc);
	}

	if (sdioc->init_done_queue_cnt == 0) {
		ioc_err(sdioc, "err: admin queue has been delete\n");
		return -ENODEV;
	}

	spin_lock_irq(&adm_cqinfo->cq_lock);
	sssraid_process_cq(sdioc, 0, &start, &end, -1);
	spin_unlock_irq(&adm_cqinfo->cq_lock);
	sssraid_complete_cqes(sdioc, 0, start, end);

	return ret;
}

static void sssraid_free_all_queues(struct sssraid_ioc *sdioc)
{
	int i;
	struct sssraid_cqueue *cqinfo;
	struct sssraid_squeue *sqinfo;

	for (i = 0; i < sdioc->init_done_queue_cnt; i++) {
		cqinfo = &sdioc->cqinfo[i];
		sqinfo = &sdioc->sqinfo[i];
		dma_free_coherent(&sdioc->pdev->dev, CQ_SIZE(sqinfo->q_depth),
				(void *)cqinfo->cqes, cqinfo->cq_dma_addr);
		dma_free_coherent(&sdioc->pdev->dev, SQ_SIZE(sqinfo->qidx, sqinfo->q_depth),
				sqinfo->sq_cmds, sqinfo->sq_dma_addr);
		dma_free_coherent(&sdioc->pdev->dev, SENSE_SIZE(sqinfo->q_depth),
				sqinfo->sense, sqinfo->sense_dma_addr);
	}

	sdioc->init_done_queue_cnt = 0;
}

static void sssraid_free_admin_cmds(struct sssraid_ioc *sdioc)
{
	kfree(sdioc->adm_cmds);
	sdioc->adm_cmds = NULL;
	INIT_LIST_HEAD(&sdioc->adm_cmd_list);
}

static void sssraid_free_resources(struct sssraid_ioc *sdioc)
{
	sssraid_free_admin_cmds(sdioc);
	kfree(sdioc->sqinfo);
	kfree(sdioc->cqinfo);
	sssraid_destroy_dma_pools(sdioc);
	kfree(sdioc->ctrl_info);
}

void sssraid_cleanup_ioc(struct sssraid_ioc *sdioc, u8 re_init)
{
	if (!re_init) {
		sssraid_free_disk_list(sdioc);
		sssraid_free_ioq_ptcmds(sdioc);
	}

	sssraid_delete_io_queues(sdioc);
	sssraid_disable_admin_queue(sdioc, !re_init);

	if (!re_init)
		sssraid_free_all_queues(sdioc);

	sssraid_ioc_disable_intr(sdioc);
	sssraid_cleanup_resources(sdioc);

	if (!re_init)
		sssraid_free_resources(sdioc);

}

int sssraid_soft_reset_handler(struct sssraid_ioc *sdioc)
{
	int retval = 0;

	if (sdioc->state != SSSRAID_RESETTING) {
		ioc_err(sdioc, "err: host is not reset state\n");
		return retval;
	}

	ioc_info(sdioc, "host reset entry\n");

	sssraid_cleanup_fwevt_list(sdioc);

	/*
	 * realize sssraid_dev_disable,
	 * i.e. sssraid_cleanup_ioc(1)
	 */
	if (sdioc->ctrl_config & SSSRAID_CC_ENABLE) {
		ioc_info(sdioc, "start disable admin queue\n");
		retval = sssraid_disable_admin_queue(sdioc, 0);
	}

	sssraid_cleanup_resources(sdioc);

	/* realize above here:
	 * sssraid_dev_disable -> sssraid_back_all_io
	 */
	sssraid_back_all_io(sdioc);

	if (retval)
		goto host_reset_failed;

	retval = sssraid_init_ioc(sdioc, 1);
	if (retval)
		goto cleanup_resources;

	sssraid_change_host_state(sdioc, SSSRAID_LIVE);
	return 0;

cleanup_resources:
	sssraid_cleanup_resources(sdioc);
host_reset_failed:
	sssraid_change_host_state(sdioc, SSSRAID_DEAD);
	ioc_err(sdioc, "err, host reset failed\n");
	return retval;
}
