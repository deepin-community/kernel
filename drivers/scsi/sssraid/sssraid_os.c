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
#include <linux/msi.h>

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

u32 admin_tmout = 60;
module_param(admin_tmout, uint, 0644);
MODULE_PARM_DESC(admin_tmout, "admin commands timeout (seconds)");

static u32 scmd_tmout_rawdisk = 180;
module_param(scmd_tmout_rawdisk, uint, 0644);
MODULE_PARM_DESC(scmd_tmout_rawdisk, "scsi commands timeout for rawdisk(seconds)");

static u32 scmd_tmout_vd = 180;
module_param(scmd_tmout_vd, uint, 0644);
MODULE_PARM_DESC(scmd_tmout_vd, "scsi commands timeout for vd(seconds)");

static int ioq_depth_set(const char *val, const struct kernel_param *kp);
static const struct kernel_param_ops ioq_depth_ops = {
	.set = ioq_depth_set,
	.get = param_get_uint,
};

u32 io_queue_depth = 1024;
module_param_cb(io_queue_depth, &ioq_depth_ops, &io_queue_depth, 0644);
MODULE_PARM_DESC(io_queue_depth, "set io queue depth, should >= 2");

static int logging_level_set(const char *val, const struct kernel_param *kp)
{
	u8 n = 0;
	int ret;

	ret = kstrtou8(val, 10, &n);
	if (ret != 0)
		return -EINVAL;

	return param_set_byte(val, kp);
}

static const struct kernel_param_ops logging_level_ops = {
	.set = logging_level_set,
	.get = param_get_byte,
};

static unsigned char logging_level;
module_param_cb(logging_level, &logging_level_ops, &logging_level, 0644);
MODULE_PARM_DESC(logging_level, "set log level, default zero for switch off");

static int small_pool_num_set(const char *val, const struct kernel_param *kp)
{
	u8 n = 0;
	int ret;

	ret = kstrtou8(val, 10, &n);
	if (ret != 0)
		return -EINVAL;
	if (n > MAX_SMALL_POOL_NUM)
		n = MAX_SMALL_POOL_NUM;
	if (n < 1)
		n = 1;
	*((u8 *)kp->arg) = n;

	return 0;
}

static const struct kernel_param_ops small_pool_num_ops = {
	.set = small_pool_num_set,
	.get = param_get_byte,
};

/* Small pools are used to save PRP for small IOs.It was
 * found that the spinlock of a single pool conflicts a
 * lot with multiple CPUs.So multiple pools are introduced
 * to reduce the conflictions.
 */
unsigned char small_pool_num = 4;
module_param_cb(small_pool_num, &small_pool_num_ops, &small_pool_num, 0644);
MODULE_PARM_DESC(small_pool_num, "set prp small pool num, default 4, MAX 16");

//static struct class *sssraid_class;

enum FW_STAT_CODE {
	FW_STAT_OK = 0,
	FW_STAT_NEED_CHECK,
	FW_STAT_ERROR,
	FW_STAT_EP_PCIE_ERROR,
	FW_STAT_NAC_DMA_ERROR,
	FW_STAT_ABORTED,
	FW_STAT_NEED_RETRY
};

static const char * const raid_levels[] = {"0", "1", "5", "6", "10", "50", "60", "NA"};

static const char * const raid_states[] = {
	"NA", "NORMAL", "FAULT", "DEGRADE", "NOT_FORMATTED", "FORMATTING", "SANITIZING",
	"INITIALIZING", "INITIALIZE_FAIL", "DELETING", "DELETE_FAIL", "WRITE_PROTECT"
};

static int ioq_depth_set(const char *val, const struct kernel_param *kp)
{
	int n = 0;
	int ret;

	ret = kstrtoint(val, 10, &n);
	if (ret != 0 || n < 2)
		return -EINVAL;

	return param_set_int(val, kp);
}

/*
 * common
 */
static struct class *sssraid_class;

struct sssraid_fwevt *sssraid_alloc_fwevt(int len)
{
	struct sssraid_fwevt *fwevt;

	fwevt = kzalloc(sizeof(*fwevt) + len, GFP_ATOMIC);
	if (!fwevt)
		return NULL;

	kref_init(&fwevt->ref_count);
	return fwevt;

}

static void sssraid_fwevt_free(struct kref *r)
{
	kfree(container_of(r, struct sssraid_fwevt, ref_count));
}

static void sssraid_fwevt_get(struct sssraid_fwevt *fwevt)
{
	kref_get(&fwevt->ref_count);
}

static void sssraid_fwevt_put(struct sssraid_fwevt *fwevt)
{
	kref_put(&fwevt->ref_count, sssraid_fwevt_free);
}

static void sssraid_fwevt_del_from_list(struct sssraid_ioc *sdioc,
	struct sssraid_fwevt *fwevt)
{
	unsigned long flags;

	spin_lock_irqsave(&sdioc->fwevt_lock, flags);
	if (!list_empty(&fwevt->list)) {
		list_del_init(&fwevt->list);
		/*
		 * Put fwevt reference count after
		 * removing it from fwevt_list
		 */
		sssraid_fwevt_put(fwevt);
	}
	spin_unlock_irqrestore(&sdioc->fwevt_lock, flags);
}

static void sssraid_fwevt_bh(struct sssraid_ioc *sdioc,
	struct sssraid_fwevt *fwevt)
{
	struct sssraid_completion *cqe;

	sdioc->current_event = fwevt;
	sssraid_fwevt_del_from_list(sdioc, fwevt);

	cqe = (struct sssraid_completion *)fwevt->event_data;

	if (!fwevt->process_evt)
		goto evt_ack;

	sssraid_complete_aen(sdioc, cqe);

evt_ack:
	/* event response put here: event has been handled. */
	sssraid_send_event_ack(sdioc, fwevt->event_id,
		    fwevt->evt_ctx, le16_to_cpu(cqe->cmd_id));
	sssraid_fwevt_put(fwevt);
	sdioc->current_event = NULL;
}

static void sssraid_fwevt_worker(struct work_struct *work)
{
	struct sssraid_fwevt *fwevt = container_of(work, struct sssraid_fwevt,
	    work);
	sssraid_fwevt_bh(fwevt->sdioc, fwevt);
	/*
	 * Put fwevt reference count after
	 * dequeuing it from worker queue
	 */
	sssraid_fwevt_put(fwevt);
}

void sssraid_fwevt_add_to_list(struct sssraid_ioc *sdioc,
			struct sssraid_fwevt *fwevt)
{
	unsigned long flags;

	if (!sdioc->fwevt_worker_thread)
		return;

	spin_lock_irqsave(&sdioc->fwevt_lock, flags);
	/* get fwevt reference count while adding it to fwevt_list */
	sssraid_fwevt_get(fwevt);
	INIT_LIST_HEAD(&fwevt->list);
	list_add_tail(&fwevt->list, &sdioc->fwevt_list);
	INIT_WORK(&fwevt->work, sssraid_fwevt_worker);
	/* get fwevt reference count while enqueueing it to worker queue */
	sssraid_fwevt_get(fwevt);
	queue_work(sdioc->fwevt_worker_thread, &fwevt->work);
	spin_unlock_irqrestore(&sdioc->fwevt_lock, flags);
}

static struct sssraid_fwevt *sssraid_dequeue_fwevt(
	struct sssraid_ioc *sdioc)
{
	unsigned long flags;
	struct sssraid_fwevt *fwevt = NULL;

	spin_lock_irqsave(&sdioc->fwevt_lock, flags);
	if (!list_empty(&sdioc->fwevt_list)) {
		fwevt = list_first_entry(&sdioc->fwevt_list,
		    struct sssraid_fwevt, list);
		list_del_init(&fwevt->list);
		/*
		 * Put fwevt reference count after
		 * removing it from fwevt_list
		 */
		sssraid_fwevt_put(fwevt);
	}
	spin_unlock_irqrestore(&sdioc->fwevt_lock, flags);

	return fwevt;
}

void sssraid_cleanup_fwevt_list(struct sssraid_ioc *sdioc)
{
	struct sssraid_fwevt *fwevt = NULL;

	if ((list_empty(&sdioc->fwevt_list) && !sdioc->current_event) ||
	    !sdioc->fwevt_worker_thread)
		return;

	while ((fwevt = sssraid_dequeue_fwevt(sdioc)) ||
	    (fwevt = sdioc->current_event)) {
		/*
		 * Wait on the fwevt to complete. If this returns 1, then
		 * the event was never executed, and we need a put for the
		 * reference the work had on the fwevt.
		 *
		 * If it did execute, we wait for it to finish, and the put will
		 * happen from sssraid_process_fwevt()
		 */
		if (cancel_work_sync(&fwevt->work)) {
			/*
			 * Put fwevt reference count after
			 * dequeuing it from worker queue
			 */
			sssraid_fwevt_put(fwevt);
			/*
			 * Put fwevt reference count to neutralize
			 * kref_init increment
			 */
			sssraid_fwevt_put(fwevt);
		}
	}
}

/*
 * common 1
 */
static int sssraid_npages_prp(struct sssraid_ioc *sdioc)
{
	u32 size = (1U << ((sdioc->ctrl_info->mdts) * 1U)) << 12;
	u32 nprps = DIV_ROUND_UP(size + sdioc->page_size, sdioc->page_size);

	return DIV_ROUND_UP(PRP_ENTRY_SIZE * nprps, sdioc->page_size - PRP_ENTRY_SIZE);
}

static int sssraid_npages_sgl(struct sssraid_ioc *sdioc)
{
	u32 nsge = le16_to_cpu(sdioc->ctrl_info->max_num_sge);

	return DIV_ROUND_UP(nsge * sizeof(struct sssraid_sgl_desc), sdioc->page_size);
}

static u32 sssraid_cmd_size(struct sssraid_ioc *sdioc)
{
	u32 alloc_size = sizeof(__le64 *) * max(sssraid_npages_prp(sdioc),
				sssraid_npages_sgl(sdioc));

	ioc_info(sdioc, "iod structure size: %lu, alloc for shost cmd_size: %u\n",
		sizeof(struct sssraid_iod), alloc_size);

	return sizeof(struct sssraid_iod) + alloc_size;
}

static int sssraid_setup_prps(struct sssraid_ioc *sdioc, struct sssraid_iod *iod)
{
	struct scatterlist *sg = iod->sg;
	u64 dma_addr = sg_dma_address(sg);
	int dma_len = sg_dma_len(sg);
	__le64 *prp_list, *old_prp_list;
	int page_size = sdioc->page_size;
	int offset = dma_addr & (page_size - 1);
	void **list = sssraid_iod_list(iod);
	int length = iod->length;
	struct dma_pool *pool;
	dma_addr_t prp_dma;
	int nprps, i;

	length -= (page_size - offset);
	if (length <= 0) {
		iod->first_dma = 0;
		return 0;
	}

	dma_len -= (page_size - offset);
	if (dma_len) {
		dma_addr += (page_size - offset);
	} else {
		sg = sg_next(sg);
		dma_addr = sg_dma_address(sg);
		dma_len = sg_dma_len(sg);
	}

	if (length <= page_size) {
		iod->first_dma = dma_addr;
		return 0;
	}

	nprps = DIV_ROUND_UP(length, page_size);
	if (nprps <= (SMALL_POOL_SIZE / PRP_ENTRY_SIZE)) {
		pool = iod->sqinfo->prp_small_pool;
		iod->npages = 0;
	} else {
		pool = sdioc->prp_page_pool;
		iod->npages = 1;
	}

	prp_list = dma_pool_alloc(pool, GFP_ATOMIC, &prp_dma);
	if (!prp_list) {
		dev_err_ratelimited(&sdioc->pdev->dev, "Allocate first prp_list memory failed\n");
		iod->first_dma = dma_addr;
		iod->npages = -1;
		return -ENOMEM;
	}
	list[0] = prp_list;
	iod->first_dma = prp_dma;
	i = 0;
	for (;;) {
		if (i == page_size / PRP_ENTRY_SIZE) {
			old_prp_list = prp_list;

			prp_list = dma_pool_alloc(pool, GFP_ATOMIC, &prp_dma);
			if (!prp_list) {
				dev_err_ratelimited(&sdioc->pdev->dev, "Allocate %dth prp_list memory failed\n",
						    iod->npages + 1);
				return -ENOMEM;
			}
			list[iod->npages++] = prp_list;
			prp_list[0] = old_prp_list[i - 1];
			old_prp_list[i - 1] = cpu_to_le64(prp_dma);
			i = 1;
		}
		prp_list[i++] = cpu_to_le64(dma_addr);
		dma_len -= page_size;
		dma_addr += page_size;
		length -= page_size;
		if (length <= 0)
			break;
		if (dma_len > 0)
			continue;
		if (unlikely(dma_len < 0))
			goto bad_sgl;
		sg = sg_next(sg);
		dma_addr = sg_dma_address(sg);
		dma_len = sg_dma_len(sg);
	}

	return 0;

bad_sgl:
	ioc_err(sdioc, "Setup prps: invalid SGL for payload len: %d sg entry count: %d\n",
		iod->length, iod->nsge);
	return -EIO;
}

static inline bool sssraid_is_rw_scmd(struct scsi_cmnd *scmd)
{
	switch (scmd->cmnd[0]) {
	case READ_6:
	case READ_10:
	case READ_12:
	case READ_16:
	case WRITE_6:
	case WRITE_10:
	case WRITE_12:
	case WRITE_16:
		return true;
	default:
		return false;
	}
}

static bool sssraid_is_prp(struct sssraid_ioc *sdioc, struct scsi_cmnd *scmd, u32 nsge)
{
	struct scatterlist *sg = scsi_sglist(scmd);
	u32 page_mask = sdioc->page_size - 1;
	bool is_prp = true;
	int i = 0;

	scsi_for_each_sg(scmd, sg, nsge, i) {
		if (i != 0 && i != nsge - 1) {
			if ((sg_dma_len(sg) & page_mask) ||
			    (sg_dma_address(sg) & page_mask)) {
				is_prp = false;
				break;
			}
		}

		if (nsge > 1 && i == 0) {
			if ((sg_dma_address(sg) + sg_dma_len(sg)) & page_mask) {
				is_prp = false;
				break;
			}
		}

		if (nsge > 1 && i == (nsge - 1)) {
			if (sg_dma_address(sg) & page_mask) {
				is_prp = false;
				break;
			}
		}
	}

	return is_prp;
}

static void sssraid_sgl_set_data(struct sssraid_sgl_desc *sge, struct scatterlist *sg)
{
	sge->addr = cpu_to_le64(sg_dma_address(sg));
	sge->length = cpu_to_le32(sg_dma_len(sg));
	sge->type = SSSRAID_SGL_FMT_DATA_DESC << 4;
}

static void sssraid_sgl_set_seg(struct sssraid_sgl_desc *sge, dma_addr_t dma_addr, int entries)
{
	sge->addr = cpu_to_le64(dma_addr);
	if (entries <= SGES_PER_PAGE) {
		sge->length = cpu_to_le32(entries * sizeof(*sge));
		sge->type = SSSRAID_SGL_FMT_LAST_SEG_DESC << 4;
	} else {
		sge->length = cpu_to_le32(PAGE_SIZE);
		sge->type = SSSRAID_SGL_FMT_SEG_DESC << 4;
	}
}


static int sssraid_setup_ioq_cmd_sgl(struct sssraid_ioc *sdioc,
				    struct scsi_cmnd *scmd, struct sssraid_ioq_command *ioq_cmd,
				    struct sssraid_iod *iod)
{
	struct sssraid_sgl_desc *sg_list, *link, *old_sg_list;
	struct scatterlist *sg = scsi_sglist(scmd);
	void **list = sssraid_iod_list(iod);
	struct dma_pool *pool;
	int nsge = iod->nsge;
	dma_addr_t sgl_dma;
	int i = 0;

	ioq_cmd->common.flags |= SSSRAID_CMD_FLAG_SGL_METABUF;

	if (nsge == 1) {
		sssraid_sgl_set_data(&ioq_cmd->common.dptr.sgl, sg);
		return 0;
	}

	if (nsge <= (SMALL_POOL_SIZE / sizeof(struct sssraid_sgl_desc))) {
		pool = iod->sqinfo->prp_small_pool;
		iod->npages = 0;
	} else {
		pool = sdioc->prp_page_pool;
		iod->npages = 1;
	}

	sg_list = dma_pool_alloc(pool, GFP_ATOMIC, &sgl_dma);
	if (!sg_list) {
		dev_err_ratelimited(&sdioc->pdev->dev, "Allocate first sgl_list failed\n");
		iod->npages = -1;
		return -ENOMEM;
	}

	list[0] = sg_list;
	iod->first_dma = sgl_dma;
	sssraid_sgl_set_seg(&ioq_cmd->common.dptr.sgl, sgl_dma, nsge);
	do {
		if (i == SGES_PER_PAGE) {
			old_sg_list = sg_list;
			link = &old_sg_list[SGES_PER_PAGE - 1];

			sg_list = dma_pool_alloc(pool, GFP_ATOMIC, &sgl_dma);
			if (!sg_list) {
				dev_err_ratelimited(&sdioc->pdev->dev, "Allocate %dth sgl_list failed\n",
						    iod->npages + 1);
				return -ENOMEM;
			}
			list[iod->npages++] = sg_list;

			i = 0;
			memcpy(&sg_list[i++], link, sizeof(*link));
			sssraid_sgl_set_seg(link, sgl_dma, nsge);
		}

		sssraid_sgl_set_data(&sg_list[i++], sg);
		sg = sg_next(sg);
	} while (--nsge > 0);

	return 0;
}

static void sssraid_shost_init(struct sssraid_ioc *sdioc)
{
	struct pci_dev *pdev = sdioc->pdev;
	u8 domain, bus;
	u32 dev_func;

	domain = pci_domain_nr(pdev->bus);
	bus = pdev->bus->number;
	dev_func = pdev->devfn;

	sdioc->shost->nr_hw_queues = SSSRAID_NR_HW_QUEUES;
	sdioc->shost->can_queue = (sdioc->ioq_depth - SSSRAID_PTCMDS_PERQ);

	sdioc->shost->sg_tablesize = le16_to_cpu(sdioc->ctrl_info->max_num_sge);
	/* 512B per sector */
	sdioc->shost->max_sectors = (1U << ((sdioc->ctrl_info->mdts) * 1U) << 12) / 512;
	sdioc->shost->cmd_per_lun = MAX_CMD_PER_DEV;
	sdioc->shost->max_channel = le16_to_cpu(sdioc->ctrl_info->max_channel) - 1;
	sdioc->shost->max_id = le32_to_cpu(sdioc->ctrl_info->max_tgt_id);
	sdioc->shost->max_lun = le16_to_cpu(sdioc->ctrl_info->max_lun);

	sdioc->shost->this_id = -1;
	sdioc->shost->unique_id = (domain << 16) | (bus << 8) | dev_func;
	sdioc->shost->max_cmd_len = MAX_CDB_LEN;
}

static inline void sssraid_get_tag_from_scmd(struct scsi_cmnd *scmd, u16 *qidx, u16 *cid)
{
	u32 tag = blk_mq_unique_tag(scsi_cmd_to_rq(scmd));

	*qidx = blk_mq_unique_tag_to_hwq(tag) + 1;
	*cid = blk_mq_unique_tag_to_tag(tag);
}

static int sssraid_setup_rw_cmd(struct sssraid_ioc *sdioc,
				struct sssraid_rw_command *rw,
				struct scsi_cmnd *scmd)
{
	u32 start_lba_lo, start_lba_hi;
	u32 datalength = 0;
	u16 control = 0;

	start_lba_lo = 0;
	start_lba_hi = 0;

	if (scmd->sc_data_direction == DMA_TO_DEVICE) {
		rw->opcode = SSSRAID_IOCMD_WRITE;
	} else if (scmd->sc_data_direction == DMA_FROM_DEVICE) {
		rw->opcode = SSSRAID_IOCMD_READ;
	} else {
		ioc_err(sdioc, "err: unsupported data direction: %d, SCSI IO cmd invalid\n",
			scmd->sc_data_direction);
		WARN_ON(1);
		return -EINVAL;
	}

	/* 6-byte READ(0x08) or WRITE(0x0A) cdb */
	if (scmd->cmd_len == SCSI_6_BYTE_CDB_LEN) {
		datalength = (u32)(scmd->cmnd[4] == 0 ?
				IO_6_DEFAULT_TX_LEN : scmd->cmnd[4]);
		start_lba_lo = (u32)get_unaligned_be24(&scmd->cmnd[1]);

		start_lba_lo &= 0x1FFFFF;
	}

	/* 10-byte READ(0x28) or WRITE(0x2A) cdb */
	else if (scmd->cmd_len == SCSI_10_BYTE_CDB_LEN) {
		datalength = (u32)get_unaligned_be16(&scmd->cmnd[7]);
		start_lba_lo = get_unaligned_be32(&scmd->cmnd[2]);

		if (scmd->cmnd[1] & FUA_MASK)
			control |= SSSRAID_RW_FUA;
	}

	/* 12-byte READ(0xA8) or WRITE(0xAA) cdb */
	else if (scmd->cmd_len == SCSI_12_BYTE_CDB_LEN) {
		datalength = get_unaligned_be32(&scmd->cmnd[6]);
		start_lba_lo = get_unaligned_be32(&scmd->cmnd[2]);

		if (scmd->cmnd[1] & FUA_MASK)
			control |= SSSRAID_RW_FUA;
	}
	/* 16-byte READ(0x88) or WRITE(0x8A) cdb */
	else if (scmd->cmd_len == SCSI_16_BYTE_CDB_LEN) {
		datalength = get_unaligned_be32(&scmd->cmnd[10]);
		start_lba_lo = get_unaligned_be32(&scmd->cmnd[6]);
		start_lba_hi = get_unaligned_be32(&scmd->cmnd[2]);

		if (scmd->cmnd[1] & FUA_MASK)
			control |= SSSRAID_RW_FUA;
	}

	if (unlikely(datalength > U16_MAX || datalength == 0)) {
		ioc_err(sdioc, "err: illegal transfer data length: %u, Invalid IO\n", datalength);
		WARN_ON(1);
		return -EINVAL;
	}

	rw->slba = cpu_to_le64(((u64)start_lba_hi << 32) | start_lba_lo);
	/* 0base for nlb */
	rw->nlb = cpu_to_le16((u16)(datalength - 1));
	rw->control = cpu_to_le16(control);

	return 0;
}

static int sssraid_setup_nonio_cmd(struct sssraid_ioc *sdioc,
				   struct sssraid_scsi_nonio *scsi_nonio, struct scsi_cmnd *scmd)
{
	scsi_nonio->buffer_len = cpu_to_le32(scsi_bufflen(scmd));

	switch (scmd->sc_data_direction) {
	case DMA_NONE:
		scsi_nonio->opcode = SSSRAID_IOCMD_NONRW_NODIR;
		break;
	case DMA_TO_DEVICE:
		scsi_nonio->opcode = SSSRAID_IOCMD_NONRW_TODEV;
		break;
	case DMA_FROM_DEVICE:
		scsi_nonio->opcode = SSSRAID_IOCMD_NONRW_FROMDEV;
		break;
	default:
		ioc_err(sdioc, "err: unsupported data direction: %d, invalid SCSI NON_IO cmd\n",
			scmd->sc_data_direction);
		WARN_ON(1);
		return -EINVAL;
	}

	return 0;
}

static int sssraid_setup_ioq_cmd(struct sssraid_ioc *sdioc,
				 struct sssraid_ioq_command *ioq_cmd, struct scsi_cmnd *scmd)
{
	memcpy(ioq_cmd->common.cdb, scmd->cmnd, scmd->cmd_len);
	ioq_cmd->common.cdb_len = scmd->cmd_len;

	if (sssraid_is_rw_scmd(scmd))
		return sssraid_setup_rw_cmd(sdioc, &ioq_cmd->rw, scmd);
	else
		return sssraid_setup_nonio_cmd(sdioc, &ioq_cmd->scsi_nonio, scmd);
}

static inline void sssraid_init_iod(struct sssraid_iod *iod)
{
	iod->nsge = 0;
	iod->npages = -1;
	iod->use_sgl = false;
	WRITE_ONCE(iod->state, SSSRAID_CMDSTAT_IDLE);
}

int sssraid_io_map_data(struct sssraid_ioc *sdioc, struct sssraid_iod *iod,
			      struct scsi_cmnd *scmd, struct sssraid_ioq_command *ioq_cmd)
{
	int retval;

	retval = scsi_dma_map(scmd);
	if (unlikely(retval < 0))
		return retval;
	iod->nsge = retval;
	/* No data to DMA, it may be scsi no-rw command */
	if (unlikely(iod->nsge == 0))
		return 0;

	iod->length = scsi_bufflen(scmd);
	iod->sg = scsi_sglist(scmd);
	iod->use_sgl = !sssraid_is_prp(sdioc, scmd, iod->nsge);

	if (iod->use_sgl) {
		retval = sssraid_setup_ioq_cmd_sgl(sdioc, scmd, ioq_cmd, iod);
	} else {
		retval = sssraid_setup_prps(sdioc, iod);
		ioq_cmd->common.dptr.prp1 =
				cpu_to_le64(sg_dma_address(iod->sg));
		ioq_cmd->common.dptr.prp2 = cpu_to_le64(iod->first_dma);
	}

	if (retval)
		scsi_dma_unmap(scmd);

	return retval;
}

void sssraid_map_status(struct sssraid_iod *iod, struct scsi_cmnd *scmd,
			      struct sssraid_completion *cqe)
{
	struct sssraid_ioc *sdioc = iod->sqinfo->sdioc;

	scsi_set_resid(scmd, 0);

	switch ((le16_to_cpu(cqe->status) >> 1) & 0x7f) {
	case FW_STAT_OK:
		set_host_byte(scmd, DID_OK);
		break;
	case FW_STAT_NEED_CHECK:
		set_host_byte(scmd, DID_OK);
		scmd->result |= le16_to_cpu(cqe->status) >> 8;
		if (scmd->result & SAM_STAT_CHECK_CONDITION) {
			memset(scmd->sense_buffer, 0, SCSI_SENSE_BUFFERSIZE);
			memcpy(scmd->sense_buffer, iod->sense, SCSI_SENSE_BUFFERSIZE);
		}
		break;
	case FW_STAT_ABORTED:
		set_host_byte(scmd, DID_ABORT);
		break;
	case FW_STAT_NEED_RETRY:
		set_host_byte(scmd, DID_REQUEUE);
		break;
	default:
		set_host_byte(scmd, DID_BAD_TARGET);
		ioc_warn(sdioc, "warn: cid[%d] qid[%d] unsupport status[0x%x]\n",
				le16_to_cpu(cqe->cmd_id), le16_to_cpu(cqe->sq_id),
				le16_to_cpu(cqe->status));
		break;
	}
}


struct sssraid_cmd *sssraid_get_cmd(struct sssraid_ioc *sdioc, enum sssraid_cmd_type type)
{
	struct sssraid_cmd *cmd = NULL;
	unsigned long flags;
	struct list_head *head = &sdioc->adm_cmd_list;
	spinlock_t *slock = &sdioc->adm_cmd_lock;

	if (type == SSSRAID_CMD_IOPT) {
		head = &sdioc->ioq_pt_list;
		slock = &sdioc->ioq_pt_lock;
	}

	spin_lock_irqsave(slock, flags);
	if (list_empty(head)) {
		spin_unlock_irqrestore(slock, flags);
		ioc_err(sdioc, "err: tool get cmd[%d] list empty\n", type);
		return NULL;
	}
	cmd = list_entry(head->next, struct sssraid_cmd, list);
	list_del_init(&cmd->list);
	spin_unlock_irqrestore(slock, flags);

	WRITE_ONCE(cmd->state, SSSRAID_CMDSTAT_FLIGHT);

	return cmd;
}

static int sssraid_add_device(struct sssraid_ioc *sdioc, struct sssraid_dev_info *device)
{
	struct Scsi_Host *shost = sdioc->shost;
	struct scsi_device *sdev;

	ioc_info(sdioc, "add scsi disk, hdid: %u target: %d, channel: %d, lun: %d, attr[0x%x]\n",
			le32_to_cpu(device->hdid), le16_to_cpu(device->target),
			device->channel, device->lun, device->attr);

	sdev = scsi_device_lookup(shost, device->channel, le16_to_cpu(device->target), 0);
	if (sdev) {
		ioc_warn(sdioc, "warn: scsi disk already exist, channel: %d, target_id: %d, lun: %d\n",
			 device->channel, le16_to_cpu(device->target), 0);
		scsi_device_put(sdev);
		return -EEXIST;
	}
	scsi_add_device(shost, device->channel, le16_to_cpu(device->target), 0);
	return 0;
}

static int sssraid_rescan_device(struct sssraid_ioc *sdioc, struct sssraid_dev_info *device)
{
	struct Scsi_Host *shost = sdioc->shost;
	struct scsi_device *sdev;

	ioc_info(sdioc, "rescan scsi disk, hdid: %u target: %d, channel: %d, lun: %d, attr[0x%x]\n",
			le32_to_cpu(device->hdid), le16_to_cpu(device->target),
			device->channel, device->lun, device->attr);

	sdev = scsi_device_lookup(shost, device->channel, le16_to_cpu(device->target), 0);
	if (!sdev) {
		ioc_warn(sdioc, "warn: rescan, scsi disk not exist, channel: %d, target_id: %d, lun: %d\n",
			 device->channel, le16_to_cpu(device->target), 0);
		return -ENODEV;
	}

	scsi_rescan_device(sdev);
	scsi_device_put(sdev);
	return 0;
}

static int sssraid_remove_device(struct sssraid_ioc *sdioc, struct sssraid_dev_info *org_device)
{
	struct Scsi_Host *shost = sdioc->shost;
	struct scsi_device *sdev;

	ioc_info(sdioc, "remove scsi disk, hdid: %u target: %d, channel: %d, lun: %d, attr[0x%x]\n",
			le32_to_cpu(org_device->hdid), le16_to_cpu(org_device->target),
			org_device->channel, org_device->lun, org_device->attr);

	sdev = scsi_device_lookup(shost, org_device->channel, le16_to_cpu(org_device->target), 0);
	if (!sdev) {
		ioc_warn(sdioc, "warn: remove, scsi disk not exist, channel: %d, target_id: %d, lun: %d\n",
			 org_device->channel, le16_to_cpu(org_device->target), 0);
		return -ENODEV;
	}

	scsi_remove_device(sdev);
	scsi_device_put(sdev);
	return 0;
}

static int luntarget_cmp_func(const void *l, const void *r)
{
	const struct sssraid_dev_info *ln = l;
	const struct sssraid_dev_info *rn = r;
	int l_attr = SSSRAID_DISK_INFO_ATTR_BOOT(ln->attr);
	int r_attr = SSSRAID_DISK_INFO_ATTR_BOOT(rn->attr);

	/* boot first */
	if (l_attr != r_attr)
		return (r_attr - l_attr);

	if (ln->channel == rn->channel)
		return le16_to_cpu(ln->target) - le16_to_cpu(rn->target);

	return ln->channel - rn->channel;
}

void sssraid_scan_disk(struct sssraid_ioc *sdioc)
{
	struct sssraid_dev_info *devices, *org_devices;
	struct sssraid_dev_info *sortdevice;
	u32 nd = le32_to_cpu(sdioc->ctrl_info->nd);
	u8 flag, org_flag;
	int i, ret;
	int count = 0;

	devices = kcalloc(nd, sizeof(struct sssraid_dev_info), GFP_KERNEL);
	if (!devices)
		return;

	sortdevice = kcalloc(nd, sizeof(struct sssraid_dev_info), GFP_KERNEL);
	if (!sortdevice)
		goto free_list;

	ret = sssraid_get_dev_list(sdioc, devices);
	if (ret)
		goto free_all;
	org_devices = sdioc->devices;
	for (i = 0; i < nd; i++) {
		org_flag = org_devices[i].flag;
		flag = devices[i].flag;

		dbgprint(sdioc, "i: %d, org_flag: 0x%x, flag: 0x%x\n", i, org_flag, flag);

		if (SSSRAID_DISK_INFO_FLAG_VALID(flag)) {
			if (!SSSRAID_DISK_INFO_FLAG_VALID(org_flag)) {
				down_write(&sdioc->devices_rwsem);
				memcpy(&org_devices[i], &devices[i],
						sizeof(struct sssraid_dev_info));
				memcpy(&sortdevice[count++], &devices[i],
						sizeof(struct sssraid_dev_info));
				up_write(&sdioc->devices_rwsem);
			} else if (SSSRAID_DISK_INFO_FLAG_CHANGE(flag)) {
				sssraid_rescan_device(sdioc, &devices[i]);
			}
		} else {
			if (SSSRAID_DISK_INFO_FLAG_VALID(org_flag)) {
				down_write(&sdioc->devices_rwsem);
				org_devices[i].flag &= 0xfe;
				up_write(&sdioc->devices_rwsem);
				sssraid_remove_device(sdioc, &org_devices[i]);
			}
		}
	}

	ioc_info(sdioc, "scan work add device count = %d\n", count);

	sort(sortdevice, count, sizeof(sortdevice[0]), luntarget_cmp_func, NULL);

	for (i = 0; i < count; i++)
		sssraid_add_device(sdioc, &sortdevice[i]);

free_all:
	kfree(sortdevice);
free_list:
	kfree(devices);
}

static int sssraid_wait_abnl_cmd_done(struct sssraid_iod *iod)
{
	u16 times = 0;

	do {
		if (READ_ONCE(iod->state) == SSSRAID_CMDSTAT_TMO_COMPLETE)
			break;
		msleep(500);
		times++;
	} while (times <= SSSRAID_WAIT_ABNL_CMD_TIMEOUT);

	/* wait command completion timeout after abort/reset success */
	if (times >= SSSRAID_WAIT_ABNL_CMD_TIMEOUT)
		return -ETIMEDOUT;

	return 0;
}

static bool sssraid_check_scmd_completed(struct scsi_cmnd *scmd)
{
	struct sssraid_ioc *sdioc = shost_priv(scmd->device->host);
	struct sssraid_iod *iod = scsi_cmd_priv(scmd);
	struct sssraid_squeue *sqinfo;
	u16 hwq, cid;

	sssraid_get_tag_from_scmd(scmd, &hwq, &cid);
	sqinfo = &sdioc->sqinfo[hwq];
	if (READ_ONCE(iod->state) == SSSRAID_CMDSTAT_COMPLETE || sssraid_poll_cq(sdioc, hwq, cid)) {
		ioc_warn(sdioc, "warn: cid[%d] qidx[%d] has completed\n",
			 cid, sqinfo->qidx);
		return true;
	}
	return false;
}

static int sssraid_scsi_reset(struct scsi_cmnd *scmd, enum sssraid_scsi_rst_type rst)
{
	struct sssraid_ioc *sdioc = shost_priv(scmd->device->host);
	struct sssraid_iod *iod = scsi_cmd_priv(scmd);
	struct sssraid_sdev_hostdata *hostdata;
	u16 hwq, cid;
	int ret;

	scsi_print_command(scmd);

	if (sdioc->state != SSSRAID_LIVE || !sssraid_wait_abnl_cmd_done(iod) ||
	    sssraid_check_scmd_completed(scmd))
		return SUCCESS;

	hostdata = scmd->device->hostdata;
	sssraid_get_tag_from_scmd(scmd, &hwq, &cid);

	ioc_warn(sdioc, "warn: cid[%d] qidx[%d] timeout, %s reset\n", cid, hwq,
		rst ? "bus" : "target");
	ret = sssraid_send_reset_cmd(sdioc, rst, hostdata->hdid);
	if (ret == 0) {
		ret = sssraid_wait_abnl_cmd_done(iod);
		if (ret) {
			ioc_warn(sdioc, "warn: cid[%d] qidx[%d] %s reset failed, no found\n",
				 cid, hwq, rst ? "bus" : "target");
			return FAILED;
		}

		ioc_warn(sdioc, "cid[%d] qidx[%d] %s reset success\n", cid, hwq,
			rst ? "bus" : "target");
		return SUCCESS;
	}

	ioc_warn(sdioc, "warn: cid[%d] qidx[%d] ret[%d] %s reset failed\n", cid, hwq, ret,
		rst ? "bus" : "target");
	return FAILED;
}

bool sssraid_change_host_state(struct sssraid_ioc *sdioc, enum sssraid_state newstate)
{
	unsigned long flags;
	enum sssraid_state oldstate;
	bool change = false;

	spin_lock_irqsave(&sdioc->state_lock, flags);

	oldstate = sdioc->state;
	switch (newstate) {
	case SSSRAID_LIVE:
		switch (oldstate) {
		case SSSRAID_NEW:
		case SSSRAID_RESETTING:
			change = true;
			break;
		default:
			break;
		}
		break;
	case SSSRAID_RESETTING:
		switch (oldstate) {
		case SSSRAID_LIVE:
			change = true;
			break;
		default:
			break;
		}
		break;
	case SSSRAID_DELETING:
		if (oldstate != SSSRAID_DELETING)
			change = true;
		break;
	case SSSRAID_DEAD:
		switch (oldstate) {
		case SSSRAID_NEW:
		case SSSRAID_LIVE:
		case SSSRAID_RESETTING:
			change = true;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	if (change)
		sdioc->state = newstate;
	spin_unlock_irqrestore(&sdioc->state_lock, flags);

	ioc_info(sdioc, "[%d]->[%d], change[%d]\n", oldstate, newstate, change);

	return change;
}

static int sssraid_get_qd_by_disk(u8 attr)
{
	switch (SSSRAID_DISK_TYPE(attr)) {
	case SSSRAID_SAS_HDD_VD:
	case SSSRAID_SATA_HDD_VD:
		return SSSRAID_HDD_VD_QD;
	case SSSRAID_SAS_SSD_VD:
	case SSSRAID_SATA_SSD_VD:
	case SSSRAID_NVME_SSD_VD:
		return SSSRAID_SSD_VD_QD;
	case SSSRAID_SAS_HDD_PD:
	case SSSRAID_SATA_HDD_PD:
		return SSSRAID_HDD_PD_QD;
	case SSSRAID_SAS_SSD_PD:
	case SSSRAID_SATA_SSD_PD:
	case SSSRAID_NVME_SSD_PD:
		return SSSRAID_SSD_PD_QD;
	default:
		return MAX_CMD_PER_DEV;
	}
}

static int sssraid_match_dev(struct sssraid_ioc *sdioc, u16 idx, struct scsi_device *sdev)
{
	if (SSSRAID_DISK_INFO_FLAG_VALID(sdioc->devices[idx].flag)) {
		if (sdev->channel == sdioc->devices[idx].channel &&
		    sdev->id == le16_to_cpu(sdioc->devices[idx].target) &&
		    sdev->lun < sdioc->devices[idx].lun) {
			ioc_info(sdioc, "Match device success, channel:target:lun[%d:%d:%d]\n",
				 sdioc->devices[idx].channel,
				 sdioc->devices[idx].target,
				 sdioc->devices[idx].lun);
			return 1;
		}
	}

	return 0;
}

static int sssraid_bsg_map_data(struct sssraid_ioc *sdioc, struct bsg_job *job,
			       struct sssraid_admin_command *cmd)
{
	struct request *rq = blk_mq_rq_from_pdu(job);
	struct sssraid_iod *iod = job->dd_data;
	enum dma_data_direction dma_dir = rq_data_dir(rq) ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
	int ret = 0;

	iod->sg = job->request_payload.sg_list;
	iod->nsge = job->request_payload.sg_cnt;
	iod->length = job->request_payload.payload_len;
	iod->use_sgl = false;
	iod->npages = -1;

	if (!iod->nsge)
		goto out;

	ret = dma_map_sg_attrs(&sdioc->pdev->dev, iod->sg, iod->nsge, dma_dir, DMA_ATTR_NO_WARN);
	if (!ret)
		goto out;

	ret = sssraid_setup_prps(sdioc, iod);
	if (ret)
		goto unmap;

	cmd->common.dptr.prp1 = cpu_to_le64(sg_dma_address(iod->sg));
	cmd->common.dptr.prp2 = cpu_to_le64(iod->first_dma);

	return 0;

unmap:
	dma_unmap_sg(&sdioc->pdev->dev, iod->sg, iod->nsge, dma_dir);
out:
	return ret;
}

static void sssraid_bsg_unmap_data(struct sssraid_ioc *sdioc, struct bsg_job *job)
{
	struct request *rq = blk_mq_rq_from_pdu(job);
	struct sssraid_iod *iod = job->dd_data;
	enum dma_data_direction dma_dir = rq_data_dir(rq) ? DMA_TO_DEVICE : DMA_FROM_DEVICE;

	if (iod->nsge)
		dma_unmap_sg(&sdioc->pdev->dev, iod->sg, iod->nsge, dma_dir);

	sssraid_free_iod_res(sdioc, iod);
}

void sssraid_put_cmd(struct sssraid_ioc *sdioc, struct sssraid_cmd *cmd,
			   enum sssraid_cmd_type type)
{
	unsigned long flags;
	struct list_head *head = &sdioc->adm_cmd_list;
	spinlock_t *slock = &sdioc->adm_cmd_lock;

	if (type == SSSRAID_CMD_IOPT) {
		head = &sdioc->ioq_pt_list;
		slock = &sdioc->ioq_pt_lock;
	}

	spin_lock_irqsave(slock, flags);
	WRITE_ONCE(cmd->state, SSSRAID_CMDSTAT_IDLE);
	list_add_tail(&cmd->list, head);
	spin_unlock_irqrestore(slock, flags);
}

static int sssraid_user_admin_cmd(struct sssraid_ioc *sdioc, struct bsg_job *job)
{
	struct sssraid_bsg_request *bsg_req = job->request;
	struct sssraid_passthru_common_cmd *cmd = &(bsg_req->admcmd);
	struct sssraid_admin_command admin_cmd;
	u32 timeout = msecs_to_jiffies(cmd->timeout_ms);
	u32 result[2] = {0};
	int status;

	if (sdioc->state >= SSSRAID_RESETTING) {
		ioc_err(sdioc, "err: tool adm host state:[%d] is not right\n",
			sdioc->state);
		return -EBUSY;
	}

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.common.opcode = cmd->opcode;
	admin_cmd.common.flags = cmd->flags;
	admin_cmd.common.hdid = cpu_to_le32(cmd->nsid);
	admin_cmd.common.cdw2[0] = cpu_to_le32(cmd->cdw2);
	admin_cmd.common.cdw2[1] = cpu_to_le32(cmd->cdw3);
	admin_cmd.common.cdw10 = cpu_to_le32(cmd->cdw10);
	admin_cmd.common.cdw11 = cpu_to_le32(cmd->cdw11);
	admin_cmd.common.cdw12 = cpu_to_le32(cmd->cdw12);
	admin_cmd.common.cdw13 = cpu_to_le32(cmd->cdw13);
	admin_cmd.common.cdw14 = cpu_to_le32(cmd->cdw14);
	admin_cmd.common.cdw15 = cpu_to_le32(cmd->cdw15);

	status = sssraid_bsg_map_data(sdioc, job, &admin_cmd);
	if (status) {
		ioc_err(sdioc, "err: bsg map data failed\n");
		return status;
	}

	status = sssraid_submit_admin_sync_cmd(sdioc, &admin_cmd, &result[0], &result[1], timeout);
	if (status >= 0) {
		job->reply_len = sizeof(result);
		memcpy(job->reply, result, sizeof(result));
	}

	if (status)
		ioc_info(sdioc, "tool adm opcode[0x%x] subopcode[0x%x], status[0x%x] result0[0x%x] result1[0x%x]\n",
			cmd->opcode, cmd->info_0.subopcode, status, result[0], result[1]);

	sssraid_bsg_unmap_data(sdioc, job);

	return status;
}

static int sssraid_submit_ioq_sync_cmd(struct sssraid_ioc *sdioc, struct sssraid_ioq_command *cmd,
				      u32 *result, u32 *reslen, u32 timeout)
{
	int ret;
	dma_addr_t sense_dma;
	struct sssraid_squeue *sqinfo;
	void *sense_addr = NULL;
	struct sssraid_cmd *pt_cmd = sssraid_get_cmd(sdioc, SSSRAID_CMD_IOPT);

	if (!pt_cmd) {
		ioc_err(sdioc, "err: sync ioq get sqinfo cmd failed\n");
		return -EFAULT;
	}

	timeout = timeout ? timeout : ADMIN_TIMEOUT;

	init_completion(&pt_cmd->cmd_done);

	sqinfo = &sdioc->sqinfo[pt_cmd->qid];
	ret = pt_cmd->cid * SCSI_SENSE_BUFFERSIZE;
	sense_addr = sqinfo->sense + ret;
	sense_dma = sqinfo->sense_dma_addr + ret;

	cmd->common.sense_addr = cpu_to_le64(sense_dma);
	cmd->common.sense_len = cpu_to_le16(SCSI_SENSE_BUFFERSIZE);
	cmd->common.command_id = cpu_to_le16(pt_cmd->cid);

	sssraid_submit_cmd(sqinfo, cmd);

	if (!wait_for_completion_timeout(&pt_cmd->cmd_done, timeout)) {
		ioc_err(sdioc, "err: sync ioq cid[%d] qid[%d] timeout, opcode[0x%x] subopcode[0x%x]\n",
			pt_cmd->cid, pt_cmd->qid, cmd->common.opcode,
			(le32_to_cpu(cmd->common.cdw3[0]) & 0xffff));

		/* reset controller if admin timeout */
		sssraid_adm_timeout(sdioc, pt_cmd);

		sssraid_put_cmd(sdioc, pt_cmd, SSSRAID_CMD_IOPT);
		return -ETIME;
	}

	if (result && reslen) {
		if ((pt_cmd->status & 0x17f) == 0x101) {
			memcpy(result, sense_addr, SCSI_SENSE_BUFFERSIZE);
			*reslen = SCSI_SENSE_BUFFERSIZE;
		}
	}

	sssraid_put_cmd(sdioc, pt_cmd, SSSRAID_CMD_IOPT);

	return pt_cmd->status;
}

static int sssraid_user_ioq_cmd(struct sssraid_ioc *sdioc, struct bsg_job *job)
{
	struct sssraid_bsg_request *bsg_req = (struct sssraid_bsg_request *)(job->request);
	struct sssraid_ioq_passthru_cmd *cmd = &(bsg_req->ioqcmd);
	struct sssraid_ioq_command ioq_cmd;
	int status = 0;
	u32 timeout = msecs_to_jiffies(cmd->timeout_ms);

	if (cmd->data_len > IOQ_PT_DATA_LEN) {
		ioc_err(sdioc, "err: tool ioq data len bigger than 4k\n");
		return -EFAULT;
	}

	if (sdioc->state != SSSRAID_LIVE) {
		ioc_err(sdioc, "err: tool ioq host state:[%d] is not live\n",
			sdioc->state);
		return -EBUSY;
	}

	ioc_info(sdioc, "tool ioq opcode[0x%x] subopcode[0x%x] init, datalen[%d]\n",
		 cmd->opcode, cmd->info_1.subopcode, cmd->data_len);

	memset(&ioq_cmd, 0, sizeof(ioq_cmd));
	ioq_cmd.common.opcode = cmd->opcode;
	ioq_cmd.common.flags = cmd->flags;
	ioq_cmd.common.hdid = cpu_to_le32(cmd->nsid);
	ioq_cmd.common.sense_len = cpu_to_le16(cmd->info_0.res_sense_len);
	ioq_cmd.common.cdb_len = cmd->info_0.cdb_len;
	ioq_cmd.common.rsvd2 = cmd->info_0.rsvd0;
	ioq_cmd.common.cdw3[0] = cpu_to_le32(cmd->cdw3);
	ioq_cmd.common.cdw3[1] = cpu_to_le32(cmd->cdw4);
	ioq_cmd.common.cdw3[2] = cpu_to_le32(cmd->cdw5);

	ioq_cmd.common.cdw10[0] = cpu_to_le32(cmd->cdw10);
	ioq_cmd.common.cdw10[1] = cpu_to_le32(cmd->cdw11);
	ioq_cmd.common.cdw10[2] = cpu_to_le32(cmd->cdw12);
	ioq_cmd.common.cdw10[3] = cpu_to_le32(cmd->cdw13);
	ioq_cmd.common.cdw10[4] = cpu_to_le32(cmd->cdw14);
	ioq_cmd.common.cdw10[5] = cpu_to_le32(cmd->data_len);

	memcpy(ioq_cmd.common.cdb, &cmd->cdw16, cmd->info_0.cdb_len);

	ioq_cmd.common.cdw26[0] = cpu_to_le32(cmd->cdw26[0]);
	ioq_cmd.common.cdw26[1] = cpu_to_le32(cmd->cdw26[1]);
	ioq_cmd.common.cdw26[2] = cpu_to_le32(cmd->cdw26[2]);
	ioq_cmd.common.cdw26[3] = cpu_to_le32(cmd->cdw26[3]);

	status = sssraid_bsg_map_data(sdioc, job, (struct sssraid_admin_command *)&ioq_cmd);
	if (status) {
		ioc_err(sdioc, "err: map bsg data failed\n");
		return status;
	}

	status = sssraid_submit_ioq_sync_cmd(sdioc, &ioq_cmd, job->reply, &job->reply_len, timeout);
	if (status)
		ioc_info(sdioc, "tool ioq opcode[0x%x] subopcode[0x%x], status[0x%x], reply_len[%d]\n",
			cmd->opcode, cmd->info_1.subopcode, status, job->reply_len);

	sssraid_bsg_unmap_data(sdioc, job);

	return status;
}


/* bsg dispatch user command */
static int sssraid_bsg_host_dispatch(struct bsg_job *job)
{
	struct Scsi_Host *shost = dev_to_shost(job->dev);
	struct sssraid_ioc *sdioc = shost_priv(shost);
	struct request *rq = blk_mq_rq_from_pdu(job);
	struct sssraid_bsg_request *bsg_req = job->request;
	int ret = -ENOMSG;

	job->reply_len = 0;

	if (bsg_req == NULL || job->request_len != sizeof(struct sssraid_bsg_request)) {
		bsg_job_done(job, ret, 0);
		return 0;
	}
	dbgprint(sdioc, "bsg msgcode[%d] msglen[%d] timeout[%d];"
		"reqnsge[%d], reqlen[%d]\n",
		 bsg_req->msgcode, job->request_len, rq->timeout,
		 job->request_payload.sg_cnt, job->request_payload.payload_len);

	switch (bsg_req->msgcode) {
	case SSSRAID_BSG_ADM:
		ret = sssraid_user_admin_cmd(sdioc, job);
		break;
	case SSSRAID_BSG_IOQ:
		ret = sssraid_user_ioq_cmd(sdioc, job);
		break;
	default:
		ioc_info(sdioc, "bsg unsupport msgcode[%d]\n", bsg_req->msgcode);
		break;
	}

	if (ret > 0)
		ret = ret | (ret << 8);

	bsg_job_done(job, ret, 0);
	return 0;
}

static void sssraid_back_fault_cqe(struct sssraid_squeue *sqinfo, struct sssraid_completion *cqe)
{
	struct sssraid_ioc *sdioc = sqinfo->sdioc;
	struct blk_mq_tags *tags;
	struct scsi_cmnd *scmd;
	struct sssraid_iod *iod;
	struct request *req;

	tags = sdioc->shost->tag_set.tags[sqinfo->qidx - 1];
	req = blk_mq_tag_to_rq(tags, le16_to_cpu(cqe->cmd_id));
	if (unlikely(!req || !blk_mq_request_started(req)))
		return;

	scmd = blk_mq_rq_to_pdu(req);
	iod = scsi_cmd_priv(scmd);

	if (READ_ONCE(iod->state) != SSSRAID_CMDSTAT_FLIGHT &&
	    READ_ONCE(iod->state) != SSSRAID_CMDSTAT_TIMEOUT)
		return;

	WRITE_ONCE(iod->state, SSSRAID_CMDSTAT_TMO_COMPLETE);
	set_host_byte(scmd, DID_NO_CONNECT);
	if (iod->nsge)
		scsi_dma_unmap(scmd);
	sssraid_free_iod_res(sdioc, iod);
	scsi_done(scmd);
	ioc_warn(sdioc, "warn: back fault CQE, cid[%d] qidx[%d]\n",
		 le16_to_cpu(cqe->cmd_id), sqinfo->qidx);
}

void sssraid_back_all_io(struct sssraid_ioc *sdioc)
{
	int i, j;
	struct sssraid_squeue *sqinfo;
	struct sssraid_completion cqe = { 0 };

	scsi_block_requests(sdioc->shost);

	for (i = 1; i <= sdioc->shost->nr_hw_queues; i++) {
		sqinfo = &sdioc->sqinfo[i];
		for (j = 0; j < sdioc->scsi_qd; j++) {
			cqe.cmd_id = cpu_to_le16(j);
			sssraid_back_fault_cqe(sqinfo, &cqe);
		}
	}

	scsi_unblock_requests(sdioc->shost);

	j = SSSRAID_AMDQ_BLK_MQ_DEPTH;
	for (i = 0; i < j; i++) {
		if (READ_ONCE(sdioc->adm_cmds[i].state) == SSSRAID_CMDSTAT_FLIGHT) {
			ioc_info(sdioc, "backup adm, cid[%d]\n", i);
			sdioc->adm_cmds[i].status = 0xFFFF;
			WRITE_ONCE(sdioc->adm_cmds[i].state, SSSRAID_CMDSTAT_COMPLETE);
			complete(&(sdioc->adm_cmds[i].cmd_done));
		}
	}

	j = SSSRAID_NR_IOQ_PTCMDS;
	for (i = 0; i < j; i++) {
		if (READ_ONCE(sdioc->ioq_ptcmds[i].state) == SSSRAID_CMDSTAT_FLIGHT) {
			sdioc->ioq_ptcmds[i].status = 0xFFFF;
			WRITE_ONCE(sdioc->ioq_ptcmds[i].state, SSSRAID_CMDSTAT_COMPLETE);
			complete(&(sdioc->ioq_ptcmds[i].cmd_done));
		}
	}
}

static int sssraid_get_first_sibling(unsigned int cpu)
{
	unsigned int ret;

	ret = cpumask_first(topology_sibling_cpumask(cpu));
	if (ret < nr_cpu_ids)
		return ret;

	return cpu;
}

/*
 * static struct scsi_host_template sssraid_driver_template
 */
static int sssraid_scan_finished(struct Scsi_Host *shost,
	unsigned long time)
{
	struct sssraid_ioc *sdioc = shost_priv(shost);

	sssraid_scan_disk(sdioc);

	return 1;
}

/* eh_target_reset_handler call back */
static int sssraid_eh_target_reset(struct scsi_cmnd *scmd)
{
	return sssraid_scsi_reset(scmd, SSSRAID_RESET_TARGET);
}

/* eh_bus_reset_handler call back */
static int sssraid_bus_reset_handler(struct scsi_cmnd *scmd)
{
	return sssraid_scsi_reset(scmd, SSSRAID_RESET_BUS);
}

/* eh_host_reset_handler call back */
static int sssraid_eh_host_reset(struct scsi_cmnd *scmd)
{
	u16 hwq, cid;
	struct sssraid_ioc *sdioc = shost_priv(scmd->device->host);

	scsi_print_command(scmd);
	if (sdioc->state != SSSRAID_LIVE || sssraid_check_scmd_completed(scmd))
		return SUCCESS;

	sssraid_get_tag_from_scmd(scmd, &hwq, &cid);
	ioc_warn(sdioc, "warn: cid[%d] qidx[%d] host reset\n", cid, hwq);

	/* It's useless:
	 * old code sssraid_reset_work_sync
	 * queue_work(reset_work) at first,
	 * then flush_work to synchronize.
	 */
	if (!sssraid_change_host_state(sdioc, SSSRAID_RESETTING)) {
		ioc_info(sdioc, "can't change to reset state\n");
		return FAILED;
	}
	if (sssraid_soft_reset_handler(sdioc)) {
		ioc_warn(sdioc, "warn: cid[%d] qidx[%d] host reset failed\n", cid, hwq);
		return FAILED;
	}

	ioc_warn(sdioc, "cid[%d] qidx[%d] host reset success\n", cid, hwq);

	return SUCCESS;
}

/* host_reset call back */
static int sssraid_sysfs_host_reset(struct Scsi_Host *shost, int reset_type)
{
	int ret;
	struct sssraid_ioc *sdioc = shost_priv(shost);

	ioc_info(sdioc, "start sysfs host reset cmd\n");
	if (!sssraid_change_host_state(sdioc, SSSRAID_RESETTING)) {
		ioc_info(sdioc, "can't change to reset state\n");
		return -EBUSY;
	}
	ret = sssraid_soft_reset_handler(sdioc);
	ioc_info(sdioc, "stop sysfs host reset cmd[%d]\n", ret);

	return ret;
}

static void sssraid_map_queues(struct Scsi_Host *shost)
{
	struct sssraid_ioc *sdioc = shost_priv(shost);
	struct pci_dev *pdev = sdioc->pdev;
	struct msi_desc *entry = NULL;
	struct irq_affinity_desc *affinity = NULL;
	struct blk_mq_tag_set *tag_set = &shost->tag_set;
	struct blk_mq_queue_map *queue_map = &tag_set->map[HCTX_TYPE_DEFAULT];
	const struct cpumask *node_mask = NULL;
	unsigned int queue_offset = queue_map->queue_offset;
	unsigned int *map = queue_map->mq_map;
	unsigned int nr_queues = queue_map->nr_queues;
	unsigned int node_id, node_id_last = 0xFFFFFFFF;
	int cpu, first_sibling, cpu_index = 0;
	u8 node_count = 0, i;
	unsigned int node_id_array[100];

	msi_for_each_desc(entry, &pdev->dev, MSI_DESC_ALL) {
		if (entry->irq) {
			affinity = entry->affinity;
			node_mask = &affinity->mask;

			cpu = cpumask_first(node_mask);
			node_id = cpu_to_node(cpu);
			if (node_id_last == node_id)
				continue;

			for (i = 0; i < node_count; i++) {
				if (node_id == node_id_array[i])
					goto get_next_numa_node;
			}
			node_id_array[node_count++] = node_id;
			node_id_last = node_id;
		}
get_next_numa_node:
		continue;
	}

	for (i = 0; i < node_count; i++) {
		node_mask = cpumask_of_node(node_id_array[i]);
		dbgprint(sdioc, "NUMA_node = %d\n", node_id_array[i]);
		for_each_cpu(cpu, node_mask) {
			if (cpu_index < nr_queues) {
				map[cpu_index++] = queue_offset + (cpu % nr_queues);
			} else {
				first_sibling = sssraid_get_first_sibling(cpu);
				if (first_sibling == cpu)
					map[cpu_index++] = queue_offset + (cpu % nr_queues);
				else
					map[cpu_index++] = map[first_sibling];
			}
			dbgprint(sdioc, "map[%d] = %d\n", cpu_index - 1, map[cpu_index - 1]);
		}
	}

}

/* queuecommand	call back */
static int sssraid_qcmd(struct Scsi_Host *shost,
	struct scsi_cmnd *scmd)
{
	struct sssraid_iod *iod = scsi_cmd_priv(scmd);
	struct sssraid_ioc *sdioc = shost_priv(shost);
	struct scsi_device *sdev = scmd->device;
	struct sssraid_sdev_hostdata *hostdata = sdev->hostdata;
	u16 hwq, cid;
	struct sssraid_squeue *sq;
	struct sssraid_ioq_command ioq_cmd;
	int retval;

	if (unlikely(sdioc->state == SSSRAID_RESETTING))
		return SCSI_MLQUEUE_HOST_BUSY;

	if (unlikely(sdioc->state != SSSRAID_LIVE)) {
		set_host_byte(scmd, DID_NO_CONNECT);
		scsi_done(scmd);
		return 0;
	}

	if (unlikely(sdioc->logging_level & SSSRAID_DEBUG))
		scsi_print_command(scmd);

	sssraid_get_tag_from_scmd(scmd, &hwq, &cid);
	hostdata = sdev->hostdata;
	sq = &sdioc->sqinfo[hwq];

	memset(&ioq_cmd, 0, sizeof(ioq_cmd));
	ioq_cmd.rw.hdid = cpu_to_le32(hostdata->hdid);
	ioq_cmd.rw.command_id = cpu_to_le16(cid);

	retval = sssraid_setup_ioq_cmd(sdioc, &ioq_cmd, scmd);
	if (unlikely(retval)) {
		set_host_byte(scmd, DID_ERROR);
		scsi_done(scmd);
		return 0;
	}

	iod->sense = sq->sense + retval;
	iod->sense_dma = sq->sense_dma_addr + retval;
	ioq_cmd.common.sense_addr = cpu_to_le64(iod->sense_dma);
	ioq_cmd.common.sense_len = cpu_to_le16(SCSI_SENSE_BUFFERSIZE);

	sssraid_init_iod(iod);

	iod->sqinfo = sq;
	retval = sssraid_io_map_data(sdioc, iod, scmd, &ioq_cmd);
	if (unlikely(retval)) {
		ioc_err(sdioc, "err: io map data fail.\n");
		set_host_byte(scmd, DID_ERROR);
		scsi_done(scmd);
		retval = 0;
		goto deinit_iod;
	}

	WRITE_ONCE(iod->state, SSSRAID_CMDSTAT_FLIGHT);
	sssraid_submit_cmd(sq, &ioq_cmd);

	return 0;

deinit_iod:
	sssraid_free_iod_res(sdioc, iod);
	return retval;
}

/* change_queue_depth call back:
 * keep as old
 */

/* slave_configure call back */
static int sssraid_slave_configure(struct scsi_device *sdev)
{
	int qd = MAX_CMD_PER_DEV;
	unsigned int timeout = scmd_tmout_rawdisk * HZ;
	struct sssraid_ioc *sdioc = shost_priv(sdev->host);
	struct sssraid_sdev_hostdata *hostdata = sdev->hostdata;
	u32 max_sec = sdev->host->max_sectors;

	if (hostdata) {
		if (SSSRAID_DISK_INFO_ATTR_VD(hostdata->attr))
			timeout = scmd_tmout_vd * HZ;
		else if (SSSRAID_DISK_INFO_ATTR_RAW(hostdata->attr))
			timeout = scmd_tmout_rawdisk * HZ;
		max_sec = hostdata->max_io_kb << 1;
		qd = sssraid_get_qd_by_disk(hostdata->attr);
	} else {
		ioc_err(sdioc, "err: scsi dev hostdata is null\n");
	}

	blk_queue_rq_timeout(sdev->request_queue, timeout);
	sdev->eh_timeout = timeout;
	scsi_change_queue_depth(sdev, qd);

	if ((max_sec == 0) || (max_sec > sdev->host->max_sectors))
		max_sec = sdev->host->max_sectors;

	blk_queue_max_hw_sectors(sdev->request_queue, max_sec);

	ioc_info(sdioc, "scsi dev channel:id:lun[%d:%d:%lld], scmd_timeout[%d]s, maxsec[%d]\n",
		 sdev->channel, sdev->id, sdev->lun, timeout / HZ, max_sec);

	return 0;
}

/* slave_alloc call back */
static int sssraid_slave_alloc(struct scsi_device *sdev)
{
	struct sssraid_sdev_hostdata *hostdata;
	struct sssraid_ioc *sdioc;
	u16 idx;

	sdioc = shost_priv(sdev->host);
	hostdata = kzalloc(sizeof(*hostdata), GFP_KERNEL);
	if (!hostdata) {
		ioc_err(sdioc, "err: alloc scsi host data failed\n");
		return -ENOMEM;
	}

	down_read(&sdioc->devices_rwsem);
	for (idx = 0; idx < le32_to_cpu(sdioc->ctrl_info->nd); idx++) {
		if (sssraid_match_dev(sdioc, idx, sdev))
			goto scan_host;
	}
	up_read(&sdioc->devices_rwsem);

	kfree(hostdata);
	return -ENXIO;

scan_host:
	hostdata->hdid = le32_to_cpu(sdioc->devices[idx].hdid);
	hostdata->max_io_kb = le16_to_cpu(sdioc->devices[idx].max_io_kb);
	hostdata->attr = sdioc->devices[idx].attr;
	hostdata->flag = sdioc->devices[idx].flag;
	hostdata->rg_id = 0xff;
	sdev->hostdata = hostdata;
	up_read(&sdioc->devices_rwsem);
	return 0;
}

/* slave_destroy call back */
static void sssraid_slave_destroy(struct scsi_device *sdev)
{
	kfree(sdev->hostdata);
	sdev->hostdata = NULL;
}

/* eh_timed_out call back */
static enum scsi_timeout_action sssraid_scmd_timeout(struct scsi_cmnd *scmd)
{
	struct sssraid_iod *iod = scsi_cmd_priv(scmd);
	unsigned int timeout = scmd->device->request_queue->rq_timeout;

	if (sssraid_check_scmd_completed(scmd))
		goto out;

	if (time_after(jiffies, scmd->jiffies_at_alloc + timeout)) {
		if (cmpxchg(&iod->state, SSSRAID_CMDSTAT_FLIGHT, SSSRAID_CMDSTAT_TIMEOUT) ==
		    SSSRAID_CMDSTAT_FLIGHT) {
			return SCSI_EH_DONE;
		}
	}
out:
	return SCSI_EH_RESET_TIMER;
}

/* eh_abort_handler call back */
static int sssraid_abort_handler(struct scsi_cmnd *scmd)
{
	struct sssraid_ioc *sdioc = shost_priv(scmd->device->host);
	struct sssraid_iod *iod = scsi_cmd_priv(scmd);
	struct sssraid_sdev_hostdata *hostdata;
	u16 hwq, cid;
	int ret;

	scsi_print_command(scmd);

	if (sdioc->state != SSSRAID_LIVE || !sssraid_wait_abnl_cmd_done(iod) ||
	    sssraid_check_scmd_completed(scmd))
		return SUCCESS;

	hostdata = scmd->device->hostdata;
	sssraid_get_tag_from_scmd(scmd, &hwq, &cid);

	ioc_warn(sdioc, "warn: cid[%d] qidx[%d] timeout, aborting\n", cid, hwq);
	ret = sssraid_send_abort_cmd(sdioc, hostdata->hdid, hwq, cid);
	if (ret != -ETIME) {
		ret = sssraid_wait_abnl_cmd_done(iod);
		if (ret) {
			ioc_warn(sdioc, "warn: cid[%d] qidx[%d] abort failed\n", cid, hwq);
			return FAILED;
		}
		ioc_warn(sdioc, "cid[%d] qidx[%d] abort success\n", cid, hwq);
		return SUCCESS;
	}
	ioc_warn(sdioc, "warn: cid[%d] qidx[%d] abort failed, timeout\n", cid, hwq);
	return FAILED;
}

static ssize_t csts_pp_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct sssraid_ioc *sdioc = shost_priv(shost);
	int ret = -1;

	if (pci_device_is_present(sdioc->pdev)) {
		ret = (readl(sdioc->bar + SSSRAID_REG_CSTS) & SSSRAID_CSTS_PP_MASK);
		ret >>= SSSRAID_CSTS_PP_SHIFT;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t csts_shst_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct sssraid_ioc *sdioc = shost_priv(shost);
	int ret = -1;

	if (pci_device_is_present(sdioc->pdev)) {
		ret = (readl(sdioc->bar + SSSRAID_REG_CSTS) & SSSRAID_CSTS_SHST_MASK);
		ret >>= SSSRAID_CSTS_SHST_SHIFT;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t csts_cfs_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct sssraid_ioc *sdioc = shost_priv(shost);
	int ret = -1;

	if (pci_device_is_present(sdioc->pdev)) {
		ret = (readl(sdioc->bar + SSSRAID_REG_CSTS) & SSSRAID_CSTS_CFS_MASK);
		ret >>= SSSRAID_CSTS_CFS_SHIFT;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t csts_rdy_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct sssraid_ioc *sdioc = shost_priv(shost);
	int ret = -1;

	if (pci_device_is_present(sdioc->pdev))
		ret = (readl(sdioc->bar + SSSRAID_REG_CSTS) & SSSRAID_CSTS_RDY);

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t fw_version_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct sssraid_ioc *sdioc = shost_priv(shost);

	return snprintf(buf, PAGE_SIZE, "%s\n", sdioc->ctrl_info->fr);
}

static DEVICE_ATTR_RO(csts_pp);
static DEVICE_ATTR_RO(csts_shst);
static DEVICE_ATTR_RO(csts_cfs);
static DEVICE_ATTR_RO(csts_rdy);
static DEVICE_ATTR_RO(fw_version);

static struct attribute *sssraid_host_attrs[] = {
	&dev_attr_csts_pp.attr,
	&dev_attr_csts_shst.attr,
	&dev_attr_csts_cfs.attr,
	&dev_attr_csts_rdy.attr,
	&dev_attr_fw_version.attr,
	NULL,
};

ATTRIBUTE_GROUPS(sssraid_host);

static int sssraid_get_vd_info(struct sssraid_ioc *sdioc, struct sssraid_vd_info *vd_info, u16 vid)
{
	struct sssraid_admin_command admin_cmd;
	u8 *data_ptr = NULL;
	dma_addr_t data_dma = 0;
	int ret;

	if (sdioc->state >= SSSRAID_RESETTING) {
		ioc_err(sdioc, "err: host state:%d invalid\n", sdioc->state);
		return -EBUSY;
	}

	data_ptr = dma_alloc_coherent(&sdioc->pdev->dev, PAGE_SIZE, &data_dma, GFP_KERNEL);
	if (!data_ptr)
		return -ENOMEM;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.usr_cmd.opcode = USR_CMD_READ;
	admin_cmd.usr_cmd.info_0.subopcode = cpu_to_le16(USR_CMD_VDINFO);
	admin_cmd.usr_cmd.info_1.data_len = cpu_to_le16(USR_CMD_RDLEN);
	admin_cmd.usr_cmd.info_1.param_len = cpu_to_le16(VDINFO_PARAM_LEN);
	admin_cmd.usr_cmd.cdw10 = cpu_to_le32(vid);
	admin_cmd.common.dptr.prp1 = cpu_to_le64(data_dma);

	ret = sssraid_submit_admin_sync_cmd(sdioc, &admin_cmd, NULL, NULL, 0);
	if (!ret)
		memcpy(vd_info, data_ptr, sizeof(struct sssraid_vd_info));

	dma_free_coherent(&sdioc->pdev->dev, PAGE_SIZE, data_ptr, data_dma);

	return ret;
}

static int sssraid_get_bgtask(struct sssraid_ioc *sdioc, struct sssraid_bgtask *bgtask)
{
	struct sssraid_admin_command admin_cmd;
	u8 *data_ptr = NULL;
	dma_addr_t data_dma = 0;
	int ret;

	if (sdioc->state >= SSSRAID_RESETTING) {
		ioc_err(sdioc, "err: host state:%d invalid\n", sdioc->state);
		return -EBUSY;
	}

	data_ptr = dma_alloc_coherent(&sdioc->pdev->dev, PAGE_SIZE, &data_dma, GFP_KERNEL);
	if (!data_ptr)
		return -ENOMEM;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.usr_cmd.opcode = USR_CMD_READ;
	admin_cmd.usr_cmd.info_0.subopcode = cpu_to_le16(USR_CMD_BGTASK);
	admin_cmd.usr_cmd.info_1.data_len = cpu_to_le16(USR_CMD_RDLEN);
	admin_cmd.common.dptr.prp1 = cpu_to_le64(data_dma);

	ret = sssraid_submit_admin_sync_cmd(sdioc, &admin_cmd, NULL, NULL, 0);
	if (!ret)
		memcpy(bgtask, data_ptr, sizeof(struct sssraid_bgtask));

	dma_free_coherent(&sdioc->pdev->dev, PAGE_SIZE, data_ptr, data_dma);

	return ret;
}

static ssize_t raid_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct scsi_device *sdev;
	struct sssraid_ioc *sdioc;
	struct sssraid_vd_info *vd_info;
	struct sssraid_sdev_hostdata *hostdata;
	int ret;

	sdev = to_scsi_device(dev);
	sdioc = shost_priv(sdev->host);
	hostdata = sdev->hostdata;

	vd_info = kmalloc(sizeof(*vd_info), GFP_KERNEL);
	if (!vd_info || !SSSRAID_DISK_INFO_ATTR_VD(hostdata->attr))
		return snprintf(buf, PAGE_SIZE, "NA\n");

	ret = sssraid_get_vd_info(sdioc, vd_info, sdev->id);
	if (ret)
		vd_info->rg_level = ARRAY_SIZE(raid_levels) - 1;

	ret = (vd_info->rg_level < ARRAY_SIZE(raid_levels)) ?
	       vd_info->rg_level : (ARRAY_SIZE(raid_levels) - 1);

	kfree(vd_info);

	return snprintf(buf, PAGE_SIZE, "RAID-%s\n", raid_levels[ret]);
}

static ssize_t raid_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct scsi_device *sdev;
	struct sssraid_ioc *sdioc;
	struct sssraid_vd_info *vd_info;
	struct sssraid_sdev_hostdata *hostdata;
	int ret;

	sdev = to_scsi_device(dev);
	sdioc = shost_priv(sdev->host);
	hostdata = sdev->hostdata;

	vd_info = kmalloc(sizeof(*vd_info), GFP_KERNEL);
	if (!vd_info || !SSSRAID_DISK_INFO_ATTR_VD(hostdata->attr))
		return snprintf(buf, PAGE_SIZE, "NA\n");

	ret = sssraid_get_vd_info(sdioc, vd_info, sdev->id);
	if (ret) {
		vd_info->vd_status = 0;
		vd_info->rg_id = 0xff;
	}

	ret = (vd_info->vd_status < ARRAY_SIZE(raid_states)) ? vd_info->vd_status : 0;

	kfree(vd_info);

	return snprintf(buf, PAGE_SIZE, "%s\n", raid_states[ret]);
}

static ssize_t raid_resync_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct scsi_device *sdev;
	struct sssraid_ioc *sdioc;
	struct sssraid_vd_info *vd_info;
	struct sssraid_bgtask *bgtask;
	struct sssraid_sdev_hostdata *hostdata;
	u8 rg_id, i, progress = 0;
	int ret;

	sdev = to_scsi_device(dev);
	sdioc = shost_priv(sdev->host);
	hostdata = sdev->hostdata;

	vd_info = kmalloc(sizeof(*vd_info), GFP_KERNEL);
	if (!vd_info || !SSSRAID_DISK_INFO_ATTR_VD(hostdata->attr))
		return snprintf(buf, PAGE_SIZE, "NA\n");

	ret = sssraid_get_vd_info(sdioc, vd_info, sdev->id);
	if (ret)
		goto out;

	rg_id = vd_info->rg_id;

	bgtask = (struct sssraid_bgtask *)vd_info;
	ret = sssraid_get_bgtask(sdioc, bgtask);
	if (ret)
		goto out;
	for (i = 0; i < bgtask->task_num; i++) {
		if ((bgtask->bgtask[i].type == BGTASK_TYPE_REBUILD) &&
		    (le16_to_cpu(bgtask->bgtask[i].vd_id) == rg_id))
			progress = bgtask->bgtask[i].progress;
	}

out:
	kfree(vd_info);
	return snprintf(buf, PAGE_SIZE, "%d\n", progress);
}

static DEVICE_ATTR_RO(raid_level);
static DEVICE_ATTR_RO(raid_state);
static DEVICE_ATTR_RO(raid_resync);

static struct attribute *sssraid_dev_attrs[] = {
	&dev_attr_raid_level.attr,
	&dev_attr_raid_state.attr,
	&dev_attr_raid_resync.attr,
	NULL,
};

ATTRIBUTE_GROUPS(sssraid_dev);

static struct scsi_host_template sssraid_driver_template = {
	.module			= THIS_MODULE,
	.name			= "3SNIC Logic sssraid driver",
	.proc_name		= "sssraid",
	.queuecommand		= sssraid_qcmd,
	.map_queues		= sssraid_map_queues,
	.slave_alloc		= sssraid_slave_alloc,
	.slave_destroy		= sssraid_slave_destroy,
	.slave_configure	= sssraid_slave_configure,
	.scan_finished		= sssraid_scan_finished,
	.eh_timed_out		= sssraid_scmd_timeout,
	.eh_abort_handler	= sssraid_abort_handler,
	.eh_target_reset_handler	= sssraid_eh_target_reset,
	.eh_bus_reset_handler		= sssraid_bus_reset_handler,
	.eh_host_reset_handler		= sssraid_eh_host_reset,
	.change_queue_depth		= scsi_change_queue_depth,
	.host_tagset			= 0,
	.this_id			= -1,
	.shost_groups                   = sssraid_host_groups,
	.sdev_groups                    = sssraid_dev_groups,
	.host_reset			= sssraid_sysfs_host_reset,
	.cmd_size                       = sizeof(struct sssraid_iod) + 8,
};

/**
 * sssraid_probe - PCI probe callback
 * @pdev: PCI device instance
 * @id: PCI device ID details
 *
 * controller initialization routine.
 * Allocate per adapter instance through shost_priv and
 * initialize controller specific data structures, initializae
 * the controller hardware, add shost to the SCSI subsystem.
 *
 * Return: 0 on success, non-zero on failure.
 */

static int
sssraid_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct sssraid_ioc *sdioc;
	struct Scsi_Host *shost;
	int node;
	char bsg_name[BSG_NAME_SIZE];
	int retval = 0;

	node = dev_to_node(&pdev->dev);
	if (node == NUMA_NO_NODE) {
		node = first_memory_node;
		set_dev_node(&pdev->dev, node);
	}

	shost = scsi_host_alloc(&sssraid_driver_template, sizeof(*sdioc));
	if (!shost) {
		retval = -ENODEV;
		dev_err(&pdev->dev, "err: failed to allocate scsi host\n");
		goto shost_failed;
	}

	sdioc = shost_priv(shost);
	sdioc->numa_node = node;
	sdioc->instance = shost->host_no; /* for device instance */
	snprintf(sdioc->name, sizeof(sdioc->name),
			"%s%d", SSSRAID_DRIVER_NAME, sdioc->instance);

	init_rwsem(&sdioc->devices_rwsem);
	spin_lock_init(&sdioc->state_lock);

	spin_lock_init(&sdioc->fwevt_lock);
	spin_lock_init(&sdioc->watchdog_lock);

	INIT_LIST_HEAD(&sdioc->fwevt_list);

	sdioc->logging_level = logging_level; /* according to log_debug_switch*/

	snprintf(sdioc->fwevt_worker_name, sizeof(sdioc->fwevt_worker_name),
	    "%s%d_fwevt_wrkr", SSSRAID_DRIVER_NAME, sdioc->instance);
	sdioc->fwevt_worker_thread = alloc_ordered_workqueue(
	    sdioc->fwevt_worker_name, WQ_MEM_RECLAIM);
	if (!sdioc->fwevt_worker_thread) {
		ioc_err(sdioc, "err: fail to alloc workqueue for fwevt_work!\n");
		retval = -ENODEV;
		goto out_fwevtthread_failed;
	}

	sdioc->shost = shost;
	sdioc->pdev = pdev;

	if (sssraid_init_ioc(sdioc, 0)) {
		ioc_err(sdioc, "err: failure at init sssraid_ioc!\n");
		retval = -ENODEV;
		goto out_iocinit_failed;
	}

	sssraid_shost_init(sdioc);

	retval = scsi_add_host(shost, &pdev->dev);
	if (retval) {
		ioc_err(sdioc, "err: add shost to system failed!\n");
		goto addhost_failed;
	}

	snprintf(bsg_name, sizeof(bsg_name), "%s%d", SSSRAID_DRIVER_NAME, shost->host_no);
	sdioc->bsg_queue = bsg_setup_queue(&shost->shost_gendev, bsg_name,
				sssraid_bsg_host_dispatch,  NULL, sssraid_cmd_size(sdioc));
	if (IS_ERR(sdioc->bsg_queue)) {
		ioc_err(sdioc, "err: setup bsg failed!\n");
		sdioc->bsg_queue = NULL;
		goto bsg_setup_failed;
	}

	if (!sssraid_change_host_state(sdioc, SSSRAID_LIVE)) {
		retval = -ENODEV;
		ioc_err(sdioc, "err: change host state failed!\n");
		goto sssraid_state_change_failed;
	}

	scsi_scan_host(shost);
	return retval;

sssraid_state_change_failed:
	bsg_remove_queue(sdioc->bsg_queue);
bsg_setup_failed:
	scsi_remove_host(shost);
addhost_failed:
	sssraid_cleanup_ioc(sdioc, 0);
out_iocinit_failed:
	destroy_workqueue(sdioc->fwevt_worker_thread);
out_fwevtthread_failed:
	scsi_host_put(shost);
shost_failed:
	return retval;
}

static void sssraid_remove(struct pci_dev *pdev)
{
	struct Scsi_Host *shost = pci_get_drvdata(pdev);
	struct sssraid_ioc *sdioc = NULL;

	if (!shost) {
		dev_err(&pdev->dev, "driver probe process failed, remove not be allowed.\n");
		return;
	}
	sdioc = shost_priv(shost);

	ioc_info(sdioc, "sssraid remove entry\n");
	sssraid_change_host_state(sdioc, SSSRAID_DELETING);

	if (!pci_device_is_present(pdev))
		sssraid_back_all_io(sdioc);

	sssraid_cleanup_fwevt_list(sdioc);
	destroy_workqueue(sdioc->fwevt_worker_thread);

	bsg_remove_queue(sdioc->bsg_queue);
	scsi_remove_host(shost);
	sssraid_cleanup_ioc(sdioc, 0);

	scsi_host_put(shost);
}

static void sssraid_shutdown(struct pci_dev *pdev)
{
	struct Scsi_Host *shost = pci_get_drvdata(pdev);
	struct sssraid_ioc *sdioc;

	if (!shost)
		return;

	sdioc = shost_priv(shost);

	sssraid_cleanup_fwevt_list(sdioc);
	destroy_workqueue(sdioc->fwevt_worker_thread);
	sssraid_cleanup_ioc(sdioc, 0);
}

#ifdef CONFIG_PM
static int sssraid_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct Scsi_Host *shost = pci_get_drvdata(pdev);
	struct sssraid_ioc *sdioc;
	pci_power_t device_state;

	if (!shost)
		return 0;

	sdioc = shost_priv(shost);

	while (sdioc->state == SSSRAID_RESETTING)
		ssleep(1);
	sssraid_cleanup_fwevt_list(sdioc);
	scsi_block_requests(shost);
	sssraid_cleanup_ioc(sdioc, 1);

	device_state = pci_choose_state(pdev, state);
	pci_save_state(pdev);
	pci_set_power_state(pdev, device_state);

	return 0;
}

static int sssraid_resume(struct pci_dev *pdev)
{
	struct Scsi_Host *shost = pci_get_drvdata(pdev);
	struct sssraid_ioc *sdioc;
	pci_power_t device_state = pdev->current_state;

	if (!shost)
		return 0;

	sdioc = shost_priv(shost);

	ioc_info(sdioc, "pdev=0x%p, slot=%s, previous operating state [D%d]\n",
	    pdev, pci_name(pdev), device_state);
	pci_set_power_state(pdev, PCI_D0);
	pci_enable_wake(pdev, PCI_D0, 0);
	pci_restore_state(pdev);
	sdioc->pdev = pdev;
	sdioc->cpu_count = num_online_cpus();

	/* sssraid_setup_resources in sssraid_init_ioc */
	sssraid_init_ioc(sdioc, 1);
	scsi_unblock_requests(shost);

	return 0;
}
#endif

static pci_ers_result_t sssraid_pci_error_detected(struct pci_dev *pdev,
						  pci_channel_state_t state)
{
	struct Scsi_Host *shost = pci_get_drvdata(pdev);
	struct sssraid_ioc *sdioc;

	if (!shost)
		return PCI_ERS_RESULT_NONE;

	sdioc = shost_priv(shost);

	ioc_info(sdioc, "pci error detect entry, state:%d\n", state);

	switch (state) {
	case pci_channel_io_normal:
		ioc_warn(sdioc, "pci channel is normal, do nothing\n");

		return PCI_ERS_RESULT_CAN_RECOVER;
	case pci_channel_io_frozen:
		ioc_warn(sdioc, "pci channel io frozen, need reset controller\n");

		scsi_block_requests(sdioc->shost);

		sssraid_change_host_state(sdioc, SSSRAID_RESETTING);

		return PCI_ERS_RESULT_NEED_RESET;
	case pci_channel_io_perm_failure:
		ioc_warn(sdioc, "pci channel io failure, request disconnect\n");

		return PCI_ERS_RESULT_DISCONNECT;
	}

	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t sssraid_pci_slot_reset(struct pci_dev *pdev)
{
	struct Scsi_Host *shost = pci_get_drvdata(pdev);
	struct sssraid_ioc *sdioc;

	if (!shost)
		return PCI_ERS_RESULT_NONE;

	sdioc = shost_priv(shost);

	ioc_info(sdioc, "restart after pci slot reset\n");

	pci_restore_state(pdev);

	sssraid_soft_reset_handler(sdioc);

	scsi_unblock_requests(sdioc->shost);

	return PCI_ERS_RESULT_RECOVERED;
}

static void sssraid_reset_done(struct pci_dev *pdev)
{
	struct Scsi_Host *shost = pci_get_drvdata(pdev);
	struct sssraid_ioc *sdioc;

	if (!shost)
		return;

	sdioc = shost_priv(shost);

	ioc_info(sdioc, "sssraid reset exit\n");
}

static struct pci_error_handlers sssraid_err_handler = {
	.error_detected = sssraid_pci_error_detected,
	.slot_reset = sssraid_pci_slot_reset,
	.reset_done = sssraid_reset_done,
};

static const struct pci_device_id sssraid_pci_id_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_3SNIC_LOGIC, SSSRAID_SERVER_DEVICE_HBA_DID) },
	{ PCI_DEVICE(PCI_VENDOR_ID_3SNIC_LOGIC, SSSRAID_SERVER_DEVICE_RAID_DID) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, sssraid_pci_id_table);

static struct pci_driver sssraid_pci_driver = {
	.name = SSSRAID_DRIVER_NAME,
	.id_table = sssraid_pci_id_table,
	.probe = sssraid_probe,
	.remove = sssraid_remove,
	.shutdown = sssraid_shutdown,
#ifdef CONFIG_PM
	.suspend = sssraid_suspend,
	.resume = sssraid_resume,
#endif
	.err_handler = &sssraid_err_handler,
};

static int __init sssraid_init(void)
{
	int ret_val;

	pr_info("Loading %s version %s\n", SSSRAID_DRIVER_NAME,
	    SSSRAID_DRIVER_VERSION);

	sssraid_class = class_create("sssraid");
	if (IS_ERR(sssraid_class)) {
		ret_val = PTR_ERR(sssraid_class);
		return ret_val;
	}

	ret_val = pci_register_driver(&sssraid_pci_driver);

	return ret_val;
}

static void __exit sssraid_exit(void)
{
	pci_unregister_driver(&sssraid_pci_driver);
	class_destroy(sssraid_class);

	pr_info("Unloading %s version %s\n", SSSRAID_DRIVER_NAME,
		SSSRAID_DRIVER_VERSION);
}

MODULE_AUTHOR("steven.song@3snic.com");
MODULE_DESCRIPTION("3SNIC Information Technology SSSRAID Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(SSSRAID_DRIVER_VERSION);
module_init(sssraid_init);
module_exit(sssraid_exit);
