// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 Huawei Technologies Co., Ltd */

/* Huawei Raid Series Linux Driver */

#define pr_fmt(fmt) "hiraid: " fmt

#include <linux/sched/signal.h>
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
#include <linux/once.h>
#include <linux/debugfs.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/blkdev.h>
#include <linux/bsg-lib.h>
#include <asm/unaligned.h>
#include <linux/sort.h>
#include <target/target_core_backend.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_transport.h>
#include <scsi/scsi_dbg.h>
#include <scsi/sg.h>

#include "hiraid.h"

static u32 admin_tmout = 60;
module_param(admin_tmout, uint, 0644);
MODULE_PARM_DESC(admin_tmout, "admin commands timeout (seconds)");

static u32 scmd_tmout_rawdisk = 180;
module_param(scmd_tmout_rawdisk, uint, 0644);
MODULE_PARM_DESC(scmd_tmout_rawdisk, "scsi commands timeout for rawdisk(seconds)");

static u32 scmd_tmout_vd = 180;
module_param(scmd_tmout_vd, uint, 0644);
MODULE_PARM_DESC(scmd_tmout_vd, "scsi commands timeout for vd(seconds)");

static bool max_io_force;
module_param(max_io_force, bool, 0644);
MODULE_PARM_DESC(max_io_force, "force max_hw_sectors_kb = 1024, default false(performance first)");

static bool work_mode;
module_param(work_mode, bool, 0444);
MODULE_PARM_DESC(work_mode, "work mode switch, default false for multi hw queues");

#define MAX_IO_QUEUES		128
#define MIN_IO_QUEUES		1

static int ioq_num_set(const char *val, const struct kernel_param *kp)
{
	int n = 0;
	int ret;

	ret = kstrtoint(val, 10, &n);
	if (ret != 0 || n < MIN_IO_QUEUES || n > MAX_IO_QUEUES)
		return -EINVAL;

	return param_set_int(val, kp);
}

static const struct kernel_param_ops max_hwq_num_ops = {
	.set = ioq_num_set,
	.get = param_get_uint,
};

static u32 max_hwq_num = 128;
module_param_cb(max_hwq_num, &max_hwq_num_ops, &max_hwq_num, 0444);
MODULE_PARM_DESC(max_hwq_num, "max num of hw io queues, should >= 1, default 128");

static int io_queue_depth_set(const char *val, const struct kernel_param *kp)
{
	int n = 0;
	int ret;

	ret = kstrtoint(val, 10, &n);
	if (ret != 0 || n < 2)
		return -EINVAL;

	return param_set_int(val, kp);
}

static const struct kernel_param_ops io_queue_depth_ops = {
	.set = io_queue_depth_set,
	.get = param_get_uint,
};

static u32 io_queue_depth = 1024;
module_param_cb(io_queue_depth, &io_queue_depth_ops, &io_queue_depth, 0644);
MODULE_PARM_DESC(io_queue_depth, "set io queue depth, should >= 2");

static u32 log_debug_switch;
module_param(log_debug_switch, uint, 0644);
MODULE_PARM_DESC(log_debug_switch, "set log state, default zero for switch off");

static int extra_pool_num_set(const char *val, const struct kernel_param *kp)
{
	u8 n = 0;
	int ret;

	ret = kstrtou8(val, 10, &n);
	if (ret != 0)
		return -EINVAL;
	if (n > MAX_EXTRA_POOL_NUM)
		n = MAX_EXTRA_POOL_NUM;
	if (n < 1)
		n = 1;
	*((u8 *)kp->arg) = n;

	return 0;
}

static const struct kernel_param_ops small_pool_num_ops = {
	.set = extra_pool_num_set,
	.get = param_get_byte,
};

/* It was found that the spindlock of a single pool conflicts
 * a lot with multiple CPUs.So multiple pools are introduced
 * to reduce the conflictions.
 */
static unsigned char extra_pool_num = 4;
module_param_cb(extra_pool_num, &small_pool_num_ops, &extra_pool_num, 0644);
MODULE_PARM_DESC(extra_pool_num, "set prp extra pool num, default 4, MAX 16");

static void hiraid_handle_async_notice(struct hiraid_dev *hdev, u32 result);
static void hiraid_handle_async_vs(struct hiraid_dev *hdev, u32 result, u32 result1);

static struct class *hiraid_class;

#define HIRAID_CAP_TIMEOUT_UNIT_MS	(HZ / 2)

static struct workqueue_struct *work_queue;

#define dev_log_dbg(dev, fmt, ...)	do { \
	if (unlikely(log_debug_switch))	\
		dev_info(dev, "[%s] " fmt,	\
			__func__, ##__VA_ARGS__);	\
} while (0)

#define HIRAID_DRV_VERSION	"1.1.0.0"

#define ADMIN_TIMEOUT		(admin_tmout * HZ)
#define USRCMD_TIMEOUT		(180 * HZ)
#define CTL_RST_TIME		(600 * HZ)

#define HIRAID_WAIT_ABNL_CMD_TIMEOUT	6
#define HIRAID_WAIT_RST_IO_TIMEOUT		10

#define HIRAID_DMA_MSK_BIT_MAX	64

#define HIRAID_ALLOC_SIZE_MAX 8

#define IOQ_PT_DATA_LEN		    4096
#define IOQ_PT_SGL_DATA_LEN		(1024 * 1024)

#define MAX_CAN_QUEUE		(4096 - 1)
#define MIN_CAN_QUEUE		(1024 - 1)

enum SENSE_STATE_CODE {
	SENSE_STATE_OK = 0,
	SENSE_STATE_NEED_CHECK,
	SENSE_STATE_ERROR,
	SENSE_STATE_EP_PCIE_ERROR,
	SENSE_STATE_NAC_DMA_ERROR,
	SENSE_STATE_ABORTED,
	SENSE_STATE_NEED_RETRY
};

enum {
	FW_EH_OK = 0,
	FW_EH_DEV_NONE = 0x701
};

static const char * const raid_levels[] = {"0", "1", "5", "6", "10", "50", "60", "NA"};

static const char * const raid_states[] = {
	"NA", "NORMAL", "FAULT", "DEGRADE", "NOT_FORMATTED", "FORMATTING", "SANITIZING",
	"INITIALIZING", "INITIALIZE_FAIL", "DELETING", "DELETE_FAIL", "WRITE_PROTECT"
};

static int hiraid_remap_bar(struct hiraid_dev *hdev, u32 size)
{
	struct pci_dev *pdev = hdev->pdev;

	if (size > pci_resource_len(pdev, 0)) {
		dev_err(hdev->dev, "input size[%u] exceed bar0 length[%llu]\n",
			size, pci_resource_len(pdev, 0));
		return -ENOMEM;
	}

	if (hdev->bar)
		iounmap(hdev->bar);

	hdev->bar = ioremap(pci_resource_start(pdev, 0), size);
	if (!hdev->bar) {
		dev_err(hdev->dev, "ioremap for bar0 failed\n");
		return -ENOMEM;
	}
	hdev->dbs = hdev->bar + HIRAID_REG_DBS;

	return 0;
}

static int hiraid_dev_map(struct hiraid_dev *hdev)
{
	struct pci_dev *pdev = hdev->pdev;
	int ret;

	ret = pci_request_mem_regions(pdev, "hiraid");
	if (ret) {
		dev_err(hdev->dev, "fail to request memory regions\n");
		return ret;
	}

	ret = hiraid_remap_bar(hdev, HIRAID_REG_DBS + 4096);
	if (ret) {
		pci_release_mem_regions(pdev);
		return ret;
	}

	return 0;
}

static void hiraid_dev_unmap(struct hiraid_dev *hdev)
{
	struct pci_dev *pdev = hdev->pdev;

	if (hdev->bar) {
		iounmap(hdev->bar);
		hdev->bar = NULL;
	}
	pci_release_mem_regions(pdev);
}

static int hiraid_pci_enable(struct hiraid_dev *hdev)
{
	struct pci_dev *pdev = hdev->pdev;
	int ret = -ENOMEM;
	u64 maskbit = HIRAID_DMA_MSK_BIT_MAX;

	if (pci_enable_device_mem(pdev)) {
		dev_err(hdev->dev, "enable pci device memory resources failed\n");
		return ret;
	}
	pci_set_master(pdev);

	if (readl(hdev->bar + HIRAID_REG_CSTS) == U32_MAX) {
		ret = -ENODEV;
		dev_err(hdev->dev, "read CSTS register failed\n");
		goto disable;
	}

	hdev->cap = lo_hi_readq(hdev->bar + HIRAID_REG_CAP);
	hdev->ioq_depth = min_t(u32, HIRAID_CAP_MQES(hdev->cap) + 1, io_queue_depth);
	hdev->db_stride = 1 << HIRAID_CAP_STRIDE(hdev->cap);

	maskbit = HIRAID_CAP_DMAMASK(hdev->cap);
	if (maskbit < 32 || maskbit > HIRAID_DMA_MSK_BIT_MAX) {
		dev_err(hdev->dev, "err, dma mask invalid[%llu], set to default\n", maskbit);
		maskbit = HIRAID_DMA_MSK_BIT_MAX;
	}

	if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(maskbit))) {
		if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32))) {
			dev_err(hdev->dev, "set dma mask[32] and coherent failed\n");
			goto disable;
		}
		dev_info(hdev->dev, "set dma mask[32] success\n");
	} else {
		dev_info(hdev->dev, "set dma mask[%llu] success\n", maskbit);
	}

	ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
	if (ret < 0) {
		dev_err(hdev->dev, "allocate one IRQ for setup admin queue failed\n");
		goto disable;
	}

	pci_save_state(pdev);

	return 0;

disable:
	pci_disable_device(pdev);
	return ret;
}


/*
 * It is fact that first prp and last prp may be not full page.
 * The size to count total nprps for the io equal to size + page_size,
 * it may be a slightly overestimate.
 *
 * 8B per prp address. It may be there is one prp_list address per page,
 * prp_list address does not count in io data prps. So divisor equal to
 * PAGE_SIZE - 8, it may be a slightly overestimate.
 */
static int hiraid_prp_pagenum(struct hiraid_dev *hdev)
{
	u32 size = 1U << ((hdev->ctrl_info->mdts) * 1U) << 12;
	u32 nprps = DIV_ROUND_UP(size + hdev->page_size, hdev->page_size);

	return DIV_ROUND_UP(PRP_ENTRY_SIZE * nprps, hdev->page_size - PRP_ENTRY_SIZE);
}

/*
 * Calculates the number of pages needed for the SGL segments. For example a 4k
 * page can accommodate 256 SGL descriptors.
 */
static int hiraid_sgl_pagenum(struct hiraid_dev *hdev)
{
	u32 nsge = le16_to_cpu(hdev->ctrl_info->max_num_sge);

	return DIV_ROUND_UP(nsge * sizeof(struct hiraid_sgl_desc), hdev->page_size);
}

static inline void **hiraid_mapbuf_list(struct hiraid_mapmange *mapbuf)
{
	return mapbuf->list;
}

static u32 hiraid_get_max_cmd_size(struct hiraid_dev *hdev)
{
	u32 alloc_size = sizeof(__le64 *) * max(hiraid_prp_pagenum(hdev), hiraid_sgl_pagenum(hdev));

	dev_info(hdev->dev, "mapbuf size[%lu], alloc_size[%u]\n",
		 sizeof(struct hiraid_mapmange), alloc_size);

	return sizeof(struct hiraid_mapmange) + alloc_size;
}

static int hiraid_build_passthru_prp(struct hiraid_dev *hdev, struct hiraid_mapmange *mapbuf)
{
	struct scatterlist *sg = mapbuf->sgl;
	__le64 *phy_regpage, *prior_list;
	u64 buf_addr = sg_dma_address(sg);
	int buf_length = sg_dma_len(sg);
	u32 page_size = hdev->page_size;
	int offset = buf_addr & (page_size - 1);
	void **list = hiraid_mapbuf_list(mapbuf);
	int maplen = mapbuf->len;
	struct dma_pool *pool;
	dma_addr_t buffer_phy;
	int i;

	maplen -= (page_size - offset);
	if (maplen <= 0) {
		mapbuf->first_dma = 0;
		return 0;
	}

	buf_length -= (page_size - offset);
	if (buf_length) {
		buf_addr += (page_size - offset);
	} else {
		sg = sg_next(sg);
		buf_addr = sg_dma_address(sg);
		buf_length = sg_dma_len(sg);
	}

	if (maplen <= page_size) {
		mapbuf->first_dma = buf_addr;
		return 0;
	}

	pool = hdev->prp_page_pool;
	mapbuf->page_cnt = 1;

	phy_regpage = dma_pool_alloc(pool, GFP_ATOMIC, &buffer_phy);
	if (!phy_regpage) {
		dev_err_ratelimited(hdev->dev, "allocate first admin prp_list memory failed\n");
		mapbuf->first_dma = buf_addr;
		mapbuf->page_cnt = -1;
		return -ENOMEM;
	}
	list[0] = phy_regpage;
	mapbuf->first_dma = buffer_phy;
	i = 0;
	for (;;) {
		if (i == page_size / PRP_ENTRY_SIZE) {
			prior_list = phy_regpage;

			phy_regpage = dma_pool_alloc(pool, GFP_ATOMIC, &buffer_phy);
			if (!phy_regpage) {
				dev_err_ratelimited(hdev->dev, "allocate [%d]th admin prp list memory failed\n",
						mapbuf->page_cnt + 1);
				return -ENOMEM;
			}
			list[mapbuf->page_cnt++] = phy_regpage;
			phy_regpage[0] = prior_list[i - 1];
			prior_list[i - 1] = cpu_to_le64(buffer_phy);
			i = 1;
		}
		phy_regpage[i++] = cpu_to_le64(buf_addr);
		buf_addr += page_size;
		buf_length -= page_size;
		maplen -= page_size;
		if (maplen <= 0)
			break;
		if (buf_length > 0)
			continue;
		if (unlikely(buf_length < 0))
			goto bad_admin_sgl;
		sg = sg_next(sg);
		buf_addr = sg_dma_address(sg);
		buf_length = sg_dma_len(sg);
	}

	return 0;

bad_admin_sgl:
	dev_err(hdev->dev, "setup prps, invalid admin SGL for payload[%d] nents[%d]\n",
		mapbuf->len, mapbuf->sge_cnt);
	return -EIO;
}

static int hiraid_build_prp(struct hiraid_dev *hdev, struct hiraid_mapmange *mapbuf)
{
	struct scatterlist *sg = mapbuf->sgl;
	__le64 *phy_regpage, *prior_list;
	u64 buf_addr = sg_dma_address(sg);
	int buf_length = sg_dma_len(sg);
	u32 page_size = hdev->page_size;
	int offset = buf_addr & (page_size - 1);
	void **list = hiraid_mapbuf_list(mapbuf);
	int maplen = mapbuf->len;
	struct dma_pool *pool;
	dma_addr_t buffer_phy;
	int nprps, i;

	maplen -= (page_size - offset);
	if (maplen <= 0) {
		mapbuf->first_dma = 0;
		return 0;
	}

	buf_length -= (page_size - offset);
	if (buf_length) {
		buf_addr += (page_size - offset);
	} else {
		sg = sg_next(sg);
		buf_addr = sg_dma_address(sg);
		buf_length = sg_dma_len(sg);
	}

	if (maplen <= page_size) {
		mapbuf->first_dma = buf_addr;
		return 0;
	}

	nprps = DIV_ROUND_UP(maplen, page_size);
	if (nprps <= (EXTRA_POOL_SIZE / PRP_ENTRY_SIZE)) {
		pool = mapbuf->hiraidq->prp_small_pool;
		mapbuf->page_cnt = 0;
	} else {
		pool = hdev->prp_page_pool;
		mapbuf->page_cnt = 1;
	}

	phy_regpage = dma_pool_alloc(pool, GFP_ATOMIC, &buffer_phy);
	if (!phy_regpage) {
		dev_err_ratelimited(hdev->dev, "allocate first prp_list memory failed\n");
		mapbuf->first_dma = buf_addr;
		mapbuf->page_cnt = -1;
		return -ENOMEM;
	}
	list[0] = phy_regpage;
	mapbuf->first_dma = buffer_phy;
	i = 0;
	for (;;) {
		if (i == page_size / PRP_ENTRY_SIZE) {
			prior_list = phy_regpage;

			phy_regpage = dma_pool_alloc(pool, GFP_ATOMIC, &buffer_phy);
			if (!phy_regpage) {
				dev_err_ratelimited(hdev->dev, "allocate [%d]th prp list memory failed\n",
						    mapbuf->page_cnt + 1);
				return -ENOMEM;
			}
			list[mapbuf->page_cnt++] = phy_regpage;
			phy_regpage[0] = prior_list[i - 1];
			prior_list[i - 1] = cpu_to_le64(buffer_phy);
			i = 1;
		}
		phy_regpage[i++] = cpu_to_le64(buf_addr);
		buf_addr += page_size;
		buf_length -= page_size;
		maplen -= page_size;
		if (maplen <= 0)
			break;
		if (buf_length > 0)
			continue;
		if (unlikely(buf_length < 0))
			goto bad_sgl;
		sg = sg_next(sg);
		buf_addr = sg_dma_address(sg);
		buf_length = sg_dma_len(sg);
	}

	return 0;

bad_sgl:
	dev_err(hdev->dev, "setup prps, invalid SGL for payload[%d] nents[%d]\n",
		mapbuf->len, mapbuf->sge_cnt);
	return -EIO;
}

#define SGES_PER_PAGE    (PAGE_SIZE / sizeof(struct hiraid_sgl_desc))

static void hiraid_submit_cmd(struct hiraid_queue *hiraidq, const void *cmd)
{
	u32 sqes = SQE_SIZE(hiraidq->qid);
	unsigned long flags;
	struct hiraid_admin_com_cmd *acd = (struct hiraid_admin_com_cmd *)cmd;

	spin_lock_irqsave(&hiraidq->sq_lock, flags);
	memcpy((hiraidq->sq_cmds + sqes * hiraidq->sq_tail), cmd, sqes);
	if (++hiraidq->sq_tail == hiraidq->q_depth)
		hiraidq->sq_tail = 0;

	writel(hiraidq->sq_tail, hiraidq->q_db);
	spin_unlock_irqrestore(&hiraidq->sq_lock, flags);

	dev_log_dbg(hiraidq->hdev->dev, "cid[%d] qid[%d] opcode[0x%x] flags[0x%x] hdid[%u]\n",
		    le16_to_cpu(acd->cmd_id), hiraidq->qid, acd->opcode, acd->flags,
		    le32_to_cpu(acd->hdid));
}

static inline bool hiraid_is_rw_scmd(struct scsi_cmnd *scmd)
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

/*
 * checks if prps can be built for the IO cmd
 */
static bool hiraid_is_prp(struct hiraid_dev *hdev, struct scatterlist *sgl, u32 nsge)
{
	struct scatterlist *sg = sgl;
	u32 page_mask = hdev->page_size - 1;
	bool is_prp = true;
	u32 i = 0;

	for_each_sg(sgl, sg, nsge, i) {
		/*
		 * Data length of the middle sge multiple of page_size,
		 * address page_size aligned.
		 */
		if (i != 0 && i != nsge - 1) {
			if ((sg_dma_len(sg) & page_mask) ||
			    (sg_dma_address(sg) & page_mask)) {
				is_prp = false;
				break;
			}
		}

		/*
		 * The first sge addr plus the data length meets
		 * the page_size alignment.
		 */
		if (nsge > 1 && i == 0) {
			if ((sg_dma_address(sg) + sg_dma_len(sg)) & page_mask) {
				is_prp = false;
				break;
			}
		}

		/* The last sge addr meets the page_size alignment. */
		if (nsge > 1 && i == (nsge - 1)) {
			if (sg_dma_address(sg) & page_mask) {
				is_prp = false;
				break;
			}
		}
	}

	return is_prp;
}

enum {
	HIRAID_SGL_FMT_DATA_DESC     = 0x00,
	HIRAID_SGL_FMT_SEG_DESC      = 0x02,
	HIRAID_SGL_FMT_LAST_SEG_DESC    = 0x03,
	HIRAID_KEY_SGL_FMT_DATA_DESC    = 0x04,
	HIRAID_TRANSPORT_SGL_DATA_DESC  = 0x05
};

static void hiraid_sgl_set_data(struct hiraid_sgl_desc *sge, struct scatterlist *sg)
{
	sge->addr = cpu_to_le64(sg_dma_address(sg));
	sge->length = cpu_to_le32(sg_dma_len(sg));
	sge->type = HIRAID_SGL_FMT_DATA_DESC << 4;
}

static void hiraid_sgl_set_seg(struct hiraid_sgl_desc *sge, dma_addr_t buffer_phy, int entries)
{
	sge->addr = cpu_to_le64(buffer_phy);
	if (entries <= SGES_PER_PAGE) {
		sge->length = cpu_to_le32(entries * sizeof(*sge));
		sge->type = HIRAID_SGL_FMT_LAST_SEG_DESC << 4;
	} else {
		sge->length = cpu_to_le32(PAGE_SIZE);
		sge->type = HIRAID_SGL_FMT_SEG_DESC << 4;
	}
}

static int hiraid_build_passthru_sgl(struct hiraid_dev *hdev,
					struct hiraid_admin_command *admin_cmd,
					struct hiraid_mapmange *mapbuf)
{
	struct hiraid_sgl_desc *sg_list, *link, *old_sg_list;
	struct scatterlist *sg = mapbuf->sgl;
	void **list = hiraid_mapbuf_list(mapbuf);
	struct dma_pool *pool;
	int nsge = mapbuf->sge_cnt;
	dma_addr_t buffer_phy;
	int i = 0;

	admin_cmd->common.flags |= SQE_FLAG_SGL_METABUF;

	if (nsge == 1) {
		hiraid_sgl_set_data(&admin_cmd->common.dptr.sgl, sg);
		return 0;
	}

	pool = hdev->prp_page_pool;
	mapbuf->page_cnt = 1;

	sg_list = dma_pool_alloc(pool, GFP_ATOMIC, &buffer_phy);
	if (!sg_list) {
		dev_err_ratelimited(hdev->dev, "allocate first admin sgl_list failed\n");
		mapbuf->page_cnt = -1;
		return -ENOMEM;
	}

	list[0] = sg_list;
	mapbuf->first_dma = buffer_phy;
	hiraid_sgl_set_seg(&admin_cmd->common.dptr.sgl, buffer_phy, nsge);
	do {
		if (i == SGES_PER_PAGE) {
			old_sg_list = sg_list;
			link = &old_sg_list[SGES_PER_PAGE - 1];

			sg_list = dma_pool_alloc(pool, GFP_ATOMIC, &buffer_phy);
			if (!sg_list) {
				dev_err_ratelimited(hdev->dev, "allocate [%d]th admin sgl_list failed\n",
						    mapbuf->page_cnt + 1);
				return -ENOMEM;
			}
			list[mapbuf->page_cnt++] = sg_list;

			i = 0;
			memcpy(&sg_list[i++], link, sizeof(*link));
			hiraid_sgl_set_seg(link, buffer_phy, nsge);
		}

		hiraid_sgl_set_data(&sg_list[i++], sg);
		sg = sg_next(sg);
	} while (--nsge > 0);

	return 0;
}


static int hiraid_build_sgl(struct hiraid_dev *hdev, struct hiraid_scsi_io_cmd *io_cmd,
				struct hiraid_mapmange *mapbuf)
{
	struct hiraid_sgl_desc *sg_list, *link, *old_sg_list;
	struct scatterlist *sg = mapbuf->sgl;
	void **list = hiraid_mapbuf_list(mapbuf);
	struct dma_pool *pool;
	int nsge = mapbuf->sge_cnt;
	dma_addr_t buffer_phy;
	int i = 0;

	io_cmd->common.flags |= SQE_FLAG_SGL_METABUF;

	if (nsge == 1) {
		hiraid_sgl_set_data(&io_cmd->common.dptr.sgl, sg);
		return 0;
	}

	if (nsge <= (EXTRA_POOL_SIZE / sizeof(struct hiraid_sgl_desc))) {
		pool = mapbuf->hiraidq->prp_small_pool;
		mapbuf->page_cnt = 0;
	} else {
		pool = hdev->prp_page_pool;
		mapbuf->page_cnt = 1;
	}

	sg_list = dma_pool_alloc(pool, GFP_ATOMIC, &buffer_phy);
	if (!sg_list) {
		dev_err_ratelimited(hdev->dev, "allocate first sgl_list failed\n");
		mapbuf->page_cnt = -1;
		return -ENOMEM;
	}

	list[0] = sg_list;
	mapbuf->first_dma = buffer_phy;
	hiraid_sgl_set_seg(&io_cmd->common.dptr.sgl, buffer_phy, nsge);
	do {
		if (i == SGES_PER_PAGE) {
			old_sg_list = sg_list;
			link = &old_sg_list[SGES_PER_PAGE - 1];

			sg_list = dma_pool_alloc(pool, GFP_ATOMIC, &buffer_phy);
			if (!sg_list) {
				dev_err_ratelimited(hdev->dev, "allocate [%d]th sgl_list failed\n",
						mapbuf->page_cnt + 1);
				return -ENOMEM;
			}
			list[mapbuf->page_cnt++] = sg_list;

			i = 0;
			memcpy(&sg_list[i++], link, sizeof(*link));
			hiraid_sgl_set_seg(link, buffer_phy, nsge);
		}

		hiraid_sgl_set_data(&sg_list[i++], sg);
		sg = sg_next(sg);
	} while (--nsge > 0);

	return 0;
}

#define HIRAID_RW_FUA	BIT(14)
#define RW_LENGTH_ZERO	(67)

static int hiraid_setup_rw_cmd(struct hiraid_dev *hdev,
				struct hiraid_scsi_rw_cmd *io_cmd,
				struct scsi_cmnd *scmd,
				struct hiraid_mapmange *mapbuf)
{
	u32 ret = 0;
	u32 start_lba_lo, start_lba_hi;
	u32 datalength = 0;
	u16 control = 0;
	struct scsi_device *sdev = scmd->device;
	u32 buf_len = cpu_to_le32(scsi_bufflen(scmd));

	start_lba_lo = 0;
	start_lba_hi = 0;

	if (scmd->sc_data_direction == DMA_TO_DEVICE) {
		io_cmd->opcode = HIRAID_CMD_WRITE;
	} else if (scmd->sc_data_direction == DMA_FROM_DEVICE) {
		io_cmd->opcode = HIRAID_CMD_READ;
	} else if (scmd->sc_data_direction == DMA_NONE) {
		ret = RW_LENGTH_ZERO;
	} else {
		dev_err(hdev->dev, "invalid RW_IO for unsupported data direction[%d]\n",
			scmd->sc_data_direction);
		WARN_ON(1);
		return -EINVAL;
	}

	if (ret == RW_LENGTH_ZERO)
		return ret;

	/* 6-byte READ(0x08) or WRITE(0x0A) cdb */
	if (scmd->cmd_len == 6) {
		datalength = (u32)(scmd->cmnd[4] == 0 ?
				IO_6_DEFAULT_TX_LEN : scmd->cmnd[4]);
		start_lba_lo = (u32)get_unaligned_be24(&scmd->cmnd[1]);

		start_lba_lo &= 0x1FFFFF;
	}

	/* 10-byte READ(0x28) or WRITE(0x2A) cdb */
	else if (scmd->cmd_len == 10) {
		datalength = (u32)get_unaligned_be16(&scmd->cmnd[7]);
		start_lba_lo = get_unaligned_be32(&scmd->cmnd[2]);

		if (scmd->cmnd[1] & FUA_MASK)
			control |= HIRAID_RW_FUA;
	}

	/* 12-byte READ(0xA8) or WRITE(0xAA) cdb */
	else if (scmd->cmd_len == 12) {
		datalength = get_unaligned_be32(&scmd->cmnd[6]);
		start_lba_lo = get_unaligned_be32(&scmd->cmnd[2]);

		if (scmd->cmnd[1] & FUA_MASK)
			control |= HIRAID_RW_FUA;
	}
	/* 16-byte READ(0x88) or WRITE(0x8A) cdb */
	else if (scmd->cmd_len == 16) {
		datalength = get_unaligned_be32(&scmd->cmnd[10]);
		start_lba_lo = get_unaligned_be32(&scmd->cmnd[6]);
		start_lba_hi = get_unaligned_be32(&scmd->cmnd[2]);

		if (scmd->cmnd[1] & FUA_MASK)
			control |= HIRAID_RW_FUA;
	}

	if (unlikely(datalength > U16_MAX)) {
		dev_err(hdev->dev, "invalid IO for illegal transfer data length[%u]\n", datalength);
		WARN_ON(1);
		return -EINVAL;
	}

	if (unlikely(datalength == 0))
		return RW_LENGTH_ZERO;

	io_cmd->slba = cpu_to_le64(((u64)start_lba_hi << 32) | start_lba_lo);
	/* 0base for nlb */
	io_cmd->nlb = cpu_to_le16((u16)(datalength - 1));
	io_cmd->control = cpu_to_le16(control);

	mapbuf->cdb_data_len = (u32)((io_cmd->nlb + 1) * sdev->sector_size);
	if (mapbuf->cdb_data_len > buf_len) {
		/* return DID_ERROR */
		dev_err(hdev->dev, "error: buf len[0x%x] is smaller than actual length[0x%x] sectorsize[0x%x]\n",
			buf_len, mapbuf->cdb_data_len, sdev->sector_size);
		return -EINVAL;
	} else if (mapbuf->cdb_data_len < buf_len) {
		dev_warn(hdev->dev, "warn: buf_len[0x%x] cdb_data_len[0x%x] nlb[0x%x] sectorsize[0x%x]\n",
			buf_len, mapbuf->cdb_data_len, io_cmd->nlb, sdev->sector_size);
	}
	return 0;
}

static int hiraid_setup_nonrw_cmd(struct hiraid_dev *hdev,
				struct hiraid_scsi_nonrw_cmd *io_cmd, struct scsi_cmnd *scmd)
{
	io_cmd->buf_len = cpu_to_le32(scsi_bufflen(scmd));

	switch (scmd->sc_data_direction) {
	case DMA_NONE:
		io_cmd->opcode = HIRAID_CMD_NONRW_NONE;
		break;
	case DMA_TO_DEVICE:
		io_cmd->opcode = HIRAID_CMD_NONRW_TODEV;
		break;
	case DMA_FROM_DEVICE:
		io_cmd->opcode = HIRAID_CMD_NONRW_FROMDEV;
		break;
	default:
		dev_err(hdev->dev, "invalid NON_IO for unsupported data direction[%d]\n",
			scmd->sc_data_direction);
		WARN_ON(1);
		return -EINVAL;
	}

	return 0;
}

static int hiraid_setup_io_cmd(struct hiraid_dev *hdev,
				struct hiraid_scsi_io_cmd *io_cmd, struct scsi_cmnd *scmd,
				struct hiraid_mapmange *mapbuf)
{
	memcpy(io_cmd->common.cdb, scmd->cmnd, scmd->cmd_len);
	io_cmd->common.cdb_len = scmd->cmd_len;

	/* init cdb_data_len */
	mapbuf->cdb_data_len = cpu_to_le32(scsi_bufflen(scmd));

	if (hiraid_is_rw_scmd(scmd))
		return hiraid_setup_rw_cmd(hdev, &io_cmd->rw, scmd, mapbuf);
	else
		return hiraid_setup_nonrw_cmd(hdev, &io_cmd->nonrw, scmd);
}

static inline void hiraid_init_mapbuff(struct hiraid_mapmange *mapbuf)
{
	mapbuf->sge_cnt = 0;
	mapbuf->page_cnt = -1;
	mapbuf->use_sgl = false;
	WRITE_ONCE(mapbuf->state, CMD_IDLE);
}

static void hiraid_free_mapbuf(struct hiraid_dev *hdev, struct hiraid_mapmange *mapbuf)
{
	const int last_prp = hdev->page_size / sizeof(__le64) - 1;
	dma_addr_t buffer_phy, next_buffer_phy;
	struct hiraid_sgl_desc *sg_list;
	__le64 *prp_list;
	void *addr;
	int i;

	buffer_phy = mapbuf->first_dma;
	if (mapbuf->page_cnt == 0)
		dma_pool_free(mapbuf->hiraidq->prp_small_pool,
			hiraid_mapbuf_list(mapbuf)[0], buffer_phy);

	for (i = 0; i < mapbuf->page_cnt; i++) {
		addr = hiraid_mapbuf_list(mapbuf)[i];

		if (mapbuf->use_sgl) {
			sg_list = addr;
			next_buffer_phy =
				le64_to_cpu((sg_list[SGES_PER_PAGE - 1]).addr);
		} else {
			prp_list = addr;
			next_buffer_phy = le64_to_cpu(prp_list[last_prp]);
		}

		dma_pool_free(hdev->prp_page_pool, addr, buffer_phy);
		buffer_phy = next_buffer_phy;
	}

	mapbuf->sense_buffer_virt = NULL;
	mapbuf->page_cnt = -1;
}

static int hiraid_io_map_data(struct hiraid_dev *hdev, struct hiraid_mapmange *mapbuf,
				struct scsi_cmnd *scmd, struct hiraid_scsi_io_cmd *io_cmd)
{
	int ret;

	ret = scsi_dma_map(scmd);
	if (unlikely(ret < 0))
		return ret;
	mapbuf->sge_cnt = ret;

	/* No data to DMA, it may be scsi no-rw command */
	if (unlikely(mapbuf->sge_cnt == 0))
		return 0;

	mapbuf->len = scsi_bufflen(scmd);
	mapbuf->sgl = scsi_sglist(scmd);
	mapbuf->use_sgl = !hiraid_is_prp(hdev, mapbuf->sgl, mapbuf->sge_cnt);

	if (mapbuf->use_sgl) {
		ret = hiraid_build_sgl(hdev, io_cmd, mapbuf);
	} else {
		ret = hiraid_build_prp(hdev, mapbuf);
		io_cmd->common.dptr.prp1 =
				cpu_to_le64(sg_dma_address(mapbuf->sgl));
		io_cmd->common.dptr.prp2 = cpu_to_le64(mapbuf->first_dma);
	}

	if (ret)
		scsi_dma_unmap(scmd);

	return ret;
}

static void hiraid_check_status(struct hiraid_mapmange *mapbuf, struct scsi_cmnd *scmd,
				struct hiraid_completion *cqe)
{
	u32 datalength = cpu_to_le32(scsi_bufflen(scmd));

	if (datalength > mapbuf->cdb_data_len)
		scsi_set_resid(scmd, datalength - mapbuf->cdb_data_len);
	else
		scsi_set_resid(scmd, 0);

	switch ((le16_to_cpu(cqe->status) >> 1) & 0x7f) {
	case SENSE_STATE_OK:
		set_host_byte(scmd, DID_OK);
		break;
	case SENSE_STATE_NEED_CHECK:
		set_host_byte(scmd, DID_OK);
		scmd->result |= le16_to_cpu(cqe->status) >> 8;
		if (scmd->result & SAM_STAT_CHECK_CONDITION) {
			memset(scmd->sense_buffer, 0, SCSI_SENSE_BUFFERSIZE);
			memcpy(scmd->sense_buffer,
				mapbuf->sense_buffer_virt, SCSI_SENSE_BUFFERSIZE);
			scmd->result = (scmd->result & 0x00ffffff) | (DRIVER_SENSE << 24);
		}
		break;
	case SENSE_STATE_ABORTED:
		set_host_byte(scmd, DID_ABORT);
		break;
	case SENSE_STATE_NEED_RETRY:
		set_host_byte(scmd, DID_REQUEUE);
		break;
	default:
		set_host_byte(scmd, DID_BAD_TARGET);
		dev_warn_ratelimited(mapbuf->hiraidq->hdev->dev, "cid[%d] qid[%d] sdev[%d:%d] opcode[%.2x] bad status[0x%x]\n",
			le16_to_cpu(cqe->cmd_id), le16_to_cpu(cqe->sq_id), scmd->device->channel,
			scmd->device->id, scmd->cmnd[0], le16_to_cpu(cqe->status));
		break;
	}
}

static inline void hiraid_query_scmd_tag(struct scsi_cmnd *scmd, u16 *qid, u16 *cid,
				struct hiraid_dev *hdev, struct hiraid_sdev_hostdata *hostdata)
{
	u32 tag = blk_mq_unique_tag(blk_mq_rq_from_pdu((void *)scmd));

	if (work_mode) {
		if ((hdev->hdd_dispatch == DISPATCH_BY_DISK) && (hostdata->hwq != 0))
			*qid = hostdata->hwq;
		else
			*qid = raw_smp_processor_id() % (hdev->online_queues - 1) + 1;
	} else {
		*qid = blk_mq_unique_tag_to_hwq(tag) + 1;
	}
	*cid = blk_mq_unique_tag_to_tag(tag);
}

static int hiraid_queue_command(struct Scsi_Host *shost, struct scsi_cmnd *scmd)
{
	struct hiraid_mapmange *mapbuf = scsi_cmd_priv(scmd);
	struct hiraid_dev *hdev = shost_priv(shost);
	struct scsi_device *sdev = scmd->device;
	struct hiraid_sdev_hostdata *hostdata;
	struct hiraid_scsi_io_cmd io_cmd;
	struct hiraid_queue *ioq;
	u16 hwq, cid;
	int ret;

	if (unlikely(hdev->state == DEV_RESETTING))
		return SCSI_MLQUEUE_HOST_BUSY;

	if (unlikely(hdev->state != DEV_LIVE)) {
		set_host_byte(scmd, DID_NO_CONNECT);
		scsi_done(scmd);
		return 0;
	}

	if (log_debug_switch)
		scsi_print_command(scmd);

	hostdata = sdev->hostdata;
	hiraid_query_scmd_tag(scmd, &hwq, &cid, hdev, hostdata);
	ioq = &hdev->queues[hwq];

	if (unlikely(atomic_inc_return(&ioq->inflight) >
		(hdev->ioq_depth - HIRAID_PTHRU_CMDS_PERQ))) {
		atomic_dec(&ioq->inflight);
		return SCSI_MLQUEUE_HOST_BUSY;
	}

	memset(&io_cmd, 0, sizeof(io_cmd));
	io_cmd.rw.hdid = cpu_to_le32(hostdata->hdid);
	io_cmd.rw.cmd_id = cpu_to_le16(cid);

	ret = hiraid_setup_io_cmd(hdev, &io_cmd, scmd, mapbuf);
	if (unlikely(ret)) {
		if (ret == RW_LENGTH_ZERO) {
			scsi_set_resid(scmd, scsi_bufflen(scmd));
			set_host_byte(scmd, DID_OK);
		} else {
			set_host_byte(scmd, DID_ERROR);
		}
		scsi_done(scmd);
		atomic_dec(&ioq->inflight);
		return 0;
	}

	ret = cid * SCSI_SENSE_BUFFERSIZE;
	if (work_mode) {
		mapbuf->sense_buffer_virt = hdev->sense_buffer_virt + ret;
		mapbuf->sense_buffer_phy = hdev->sense_buffer_phy + ret;
	} else {
		mapbuf->sense_buffer_virt = ioq->sense_buffer_virt + ret;
		mapbuf->sense_buffer_phy = ioq->sense_buffer_phy + ret;
	}
	io_cmd.common.sense_addr = cpu_to_le64(mapbuf->sense_buffer_phy);
	io_cmd.common.sense_len = cpu_to_le16(SCSI_SENSE_BUFFERSIZE);

	hiraid_init_mapbuff(mapbuf);

	mapbuf->hiraidq = ioq;
	mapbuf->cid = cid;
	ret = hiraid_io_map_data(hdev, mapbuf, scmd, &io_cmd);
	if (unlikely(ret)) {
		dev_err(hdev->dev, "io map data err\n");
		set_host_byte(scmd, DID_ERROR);
		scsi_done(scmd);
		ret = 0;
		goto deinit_iobuf;
	}

	WRITE_ONCE(mapbuf->state, CMD_FLIGHT);
	hiraid_submit_cmd(ioq, &io_cmd);

	return 0;

deinit_iobuf:
	atomic_dec(&ioq->inflight);
	hiraid_free_mapbuf(hdev, mapbuf);
	return ret;
}

static int hiraid_match_dev(struct hiraid_dev *hdev, u16 idx, struct scsi_device *sdev)
{
	if (HIRAID_DEV_INFO_FLAG_VALID(hdev->dev_info[idx].flag)) {
		if (sdev->channel == hdev->dev_info[idx].channel &&
		sdev->id == le16_to_cpu(hdev->dev_info[idx].target) &&
		sdev->lun < hdev->dev_info[idx].lun) {
			dev_info(hdev->dev, "match device success, channel:target:lun[%d:%d:%d]\n",
				 hdev->dev_info[idx].channel,
				 hdev->dev_info[idx].target,
				 hdev->dev_info[idx].lun);
			return 1;
		}
	}

	return 0;
}

static int hiraid_disk_qd(u8 attr)
{
	switch (HIRAID_DEV_DISK_TYPE(attr)) {
	case HIRAID_SAS_HDD_VD:
	case HIRAID_SATA_HDD_VD:
		return HIRAID_HDD_VD_QD;
	case HIRAID_SAS_SSD_VD:
	case HIRAID_SATA_SSD_VD:
	case HIRAID_NVME_SSD_VD:
		return HIRAID_SSD_VD_QD;
	case HIRAID_SAS_HDD_PD:
	case HIRAID_SATA_HDD_PD:
		return HIRAID_HDD_PD_QD;
	case HIRAID_SAS_SSD_PD:
	case HIRAID_SATA_SSD_PD:
	case HIRAID_NVME_SSD_PD:
		return HIRAID_SSD_PD_QD;
	default:
		return MAX_CMD_PER_DEV;
	}
}

static bool hiraid_disk_is_hdd(u8 attr)
{
	switch (HIRAID_DEV_DISK_TYPE(attr)) {
	case HIRAID_SAS_HDD_VD:
	case HIRAID_SATA_HDD_VD:
	case HIRAID_SAS_HDD_PD:
	case HIRAID_SATA_HDD_PD:
		return true;
	default:
		return false;
	}
}

static int hiraid_slave_alloc(struct scsi_device *sdev)
{
	struct hiraid_sdev_hostdata *hostdata;
	struct hiraid_dev *hdev;
	u16 idx;

	hdev = shost_priv(sdev->host);
	hostdata = kzalloc(sizeof(*hostdata), GFP_KERNEL);
	if (!hostdata) {
		dev_err(hdev->dev, "alloc scsi host data memory failed\n");
		return -ENOMEM;
	}

	down_read(&hdev->dev_rwsem);
	for (idx = 0; idx < le32_to_cpu(hdev->ctrl_info->nd); idx++) {
		if (hiraid_match_dev(hdev, idx, sdev))
			goto scan_host;
	}
	up_read(&hdev->dev_rwsem);

	kfree(hostdata);
	return -ENXIO;

scan_host:
	hostdata->hdid = le32_to_cpu(hdev->dev_info[idx].hdid);
	hostdata->max_io_kb = le16_to_cpu(hdev->dev_info[idx].max_io_kb);
	hostdata->attr = hdev->dev_info[idx].attr;
	hostdata->flag = hdev->dev_info[idx].flag;
	hostdata->rg_id = 0xff;
	sdev->hostdata = hostdata;
	up_read(&hdev->dev_rwsem);
	return 0;
}

static void hiraid_slave_destroy(struct scsi_device *sdev)
{
	kfree(sdev->hostdata);
	sdev->hostdata = NULL;
}

static int hiraid_slave_configure(struct scsi_device *sdev)
{
	unsigned int timeout = scmd_tmout_rawdisk * HZ;
	struct hiraid_dev *hdev = shost_priv(sdev->host);
	struct hiraid_sdev_hostdata *hostdata = sdev->hostdata;
	u32 max_sec = sdev->host->max_sectors;
	int qd = MAX_CMD_PER_DEV;

	if (hostdata) {
		if (HIRAID_DEV_INFO_ATTR_VD(hostdata->attr))
			timeout = scmd_tmout_vd * HZ;
		else if (HIRAID_DEV_INFO_ATTR_RAWDISK(hostdata->attr))
			timeout = scmd_tmout_rawdisk * HZ;
		max_sec = hostdata->max_io_kb << 1;
		qd = hiraid_disk_qd(hostdata->attr);

		if (hiraid_disk_is_hdd(hostdata->attr))
			hostdata->hwq = hostdata->hdid % (hdev->online_queues - 1) + 1;
		else
			hostdata->hwq = 0;
	} else {
		dev_err(hdev->dev, "err, sdev->hostdata is null\n");
	}

	blk_queue_rq_timeout(sdev->request_queue, timeout);
	sdev->eh_timeout = timeout;
	scsi_change_queue_depth(sdev, qd);

	if ((max_sec == 0) || (max_sec > sdev->host->max_sectors))
		max_sec = sdev->host->max_sectors;

	if (!max_io_force)
		blk_queue_max_hw_sectors(sdev->request_queue, max_sec);

	dev_info(hdev->dev, "sdev->channel:id:lun[%d:%d:%lld] scmd_timeout[%d]s maxsec[%d]\n",
		 sdev->channel, sdev->id, sdev->lun, timeout / HZ, max_sec);

	return 0;
}

static void hiraid_shost_init(struct hiraid_dev *hdev)
{
	struct pci_dev *pdev = hdev->pdev;
	u8 domain, bus;
	u32 dev_func;

	domain = pci_domain_nr(pdev->bus);
	bus = pdev->bus->number;
	dev_func = pdev->devfn;

	hdev->shost->nr_hw_queues = work_mode ? 1 : hdev->online_queues - 1;
	hdev->shost->can_queue = hdev->scsi_qd;

	hdev->shost->sg_tablesize = le16_to_cpu(hdev->ctrl_info->max_num_sge);
	/* 512B per sector */
	hdev->shost->max_sectors = (1U << ((hdev->ctrl_info->mdts) * 1U) << 12) / 512;
	hdev->shost->cmd_per_lun = MAX_CMD_PER_DEV;
	hdev->shost->max_channel = le16_to_cpu(hdev->ctrl_info->max_channel) - 1;
	hdev->shost->max_id = le32_to_cpu(hdev->ctrl_info->max_tgt_id);
	hdev->shost->max_lun = le16_to_cpu(hdev->ctrl_info->max_lun);

	hdev->shost->this_id = -1;
	hdev->shost->unique_id = (domain << 16) | (bus << 8) | dev_func;
	hdev->shost->max_cmd_len = MAX_CDB_LEN;
}

static int hiraid_alloc_queue(struct hiraid_dev *hdev, u16 qid, u16 depth)
{
	struct hiraid_queue *hiraidq = &hdev->queues[qid];
	int ret = 0;

	if (hdev->queue_count > qid) {
		dev_info(hdev->dev, "warn: queue[%d] is exist\n", qid);
		return 0;
	}

	hiraidq->cqes = dma_alloc_coherent(hdev->dev, CQ_SIZE(depth),
					&hiraidq->cq_buffer_phy, GFP_KERNEL | __GFP_ZERO);
	if (!hiraidq->cqes)
		return -ENOMEM;

	hiraidq->sq_cmds = dma_alloc_coherent(hdev->dev, SQ_SIZE(qid, depth),
					&hiraidq->sq_buffer_phy, GFP_KERNEL);
	if (!hiraidq->sq_cmds) {
		ret = -ENOMEM;
		goto  free_cqes;
	}

	/*
	 * if single hw queue, we do not need to alloc sense buffer for every queue,
	 * we have alloced all on hiraid_alloc_resources.
	 */
	if (work_mode)
		goto initq;

	/* alloc sense buffer */
	hiraidq->sense_buffer_virt = dma_alloc_coherent(hdev->dev, SENSE_SIZE(depth),
					&hiraidq->sense_buffer_phy, GFP_KERNEL | __GFP_ZERO);
	if (!hiraidq->sense_buffer_virt) {
		ret = -ENOMEM;
		goto free_sq_cmds;
	}

initq:
	spin_lock_init(&hiraidq->sq_lock);
	spin_lock_init(&hiraidq->cq_lock);
	hiraidq->hdev = hdev;
	hiraidq->q_depth = depth;
	hiraidq->qid = qid;
	hiraidq->cq_vector = -1;
	hdev->queue_count++;

	return 0;

free_sq_cmds:
	dma_free_coherent(hdev->dev, SQ_SIZE(qid, depth), (void *)hiraidq->sq_cmds,
			  hiraidq->sq_buffer_phy);
free_cqes:
	dma_free_coherent(hdev->dev, CQ_SIZE(depth), (void *)hiraidq->cqes,
			  hiraidq->cq_buffer_phy);
	return ret;
}

static int hiraid_wait_control_ready(struct hiraid_dev *hdev, u64 cap, bool enabled)
{
	unsigned long timeout =
	((HIRAID_CAP_TIMEOUT(cap) + 1) * HIRAID_CAP_TIMEOUT_UNIT_MS) + jiffies;
	u32 bit = enabled ? HIRAID_CSTS_RDY : 0;

	while ((readl(hdev->bar + HIRAID_REG_CSTS) & HIRAID_CSTS_RDY) != bit) {
		usleep_range(1000, 2000);
		if (fatal_signal_pending(current))
			return -EINTR;

		if (time_after(jiffies, timeout)) {
			dev_err(hdev->dev, "device not ready; aborting %s\n",
				enabled ? "initialisation" : "reset");
			return -ENODEV;
		}
	}
	return 0;
}

static int hiraid_shutdown_control(struct hiraid_dev *hdev)
{
	unsigned long timeout = le32_to_cpu(hdev->ctrl_info->rtd3e) / 1000000 * HZ + jiffies;

	hdev->ctrl_config &= ~HIRAID_CC_SHN_MASK;
	hdev->ctrl_config |= HIRAID_CC_SHN_NORMAL;
	writel(hdev->ctrl_config, hdev->bar + HIRAID_REG_CC);

	while ((readl(hdev->bar + HIRAID_REG_CSTS) & HIRAID_CSTS_SHST_MASK) !=
		HIRAID_CSTS_SHST_CMPLT) {
		msleep(100);
		if (fatal_signal_pending(current))
			return -EINTR;
		if (time_after(jiffies, timeout)) {
			dev_err(hdev->dev, "device shutdown incomplete, abort shutdown\n");
			return -ENODEV;
		}
	}
	return 0;
}

static int hiraid_disable_control(struct hiraid_dev *hdev)
{
	hdev->ctrl_config &= ~HIRAID_CC_SHN_MASK;
	hdev->ctrl_config &= ~HIRAID_CC_ENABLE;
	writel(hdev->ctrl_config, hdev->bar + HIRAID_REG_CC);

	return hiraid_wait_control_ready(hdev, hdev->cap, false);
}

static int hiraid_enable_control(struct hiraid_dev *hdev)
{
	u64 cap = hdev->cap;
	u32 dev_page_min = HIRAID_CAP_MPSMIN(cap) + 12;
	u32 page_shift = PAGE_SHIFT;

	if (page_shift < dev_page_min) {
		dev_err(hdev->dev, "minimum device page size[%u], too large for host[%u]\n",
			1U << dev_page_min, 1U << page_shift);
		return -ENODEV;
	}

	page_shift = min_t(unsigned int, HIRAID_CAP_MPSMAX(cap) + 12, PAGE_SHIFT);
	hdev->page_size = 1U << page_shift;

	hdev->ctrl_config = HIRAID_CC_CSS_NVM;
	hdev->ctrl_config |= (page_shift - 12) << HIRAID_CC_MPS_SHIFT;
	hdev->ctrl_config |= HIRAID_CC_AMS_RR | HIRAID_CC_SHN_NONE;
	hdev->ctrl_config |= HIRAID_CC_IOSQES | HIRAID_CC_IOCQES;
	hdev->ctrl_config |= HIRAID_CC_ENABLE;
	writel(hdev->ctrl_config, hdev->bar + HIRAID_REG_CC);

	return hiraid_wait_control_ready(hdev, cap, true);
}

static void hiraid_init_queue(struct hiraid_queue *hiraidq, u16 qid)
{
	struct hiraid_dev *hdev = hiraidq->hdev;

	memset((void *)hiraidq->cqes, 0, CQ_SIZE(hiraidq->q_depth));

	hiraidq->sq_tail = 0;
	hiraidq->cq_head = 0;
	hiraidq->cq_phase = 1;
	hiraidq->q_db = &hdev->dbs[qid * 2 * hdev->db_stride];
	hiraidq->prp_small_pool = hdev->prp_extra_pool[qid % extra_pool_num];
	hdev->online_queues++;
	atomic_set(&hiraidq->inflight, 0);
}

static inline bool hiraid_cqe_pending(struct hiraid_queue *hiraidq)
{
	return (le16_to_cpu(hiraidq->cqes[hiraidq->cq_head].status) & 1) ==
		hiraidq->cq_phase;
}

static void hiraid_complete_io_cmnd(struct hiraid_queue *ioq, struct hiraid_completion *cqe)
{
	struct hiraid_dev *hdev = ioq->hdev;
	struct blk_mq_tags *tags;
	struct scsi_cmnd *scmd;
	struct hiraid_mapmange *mapbuf;
	struct request *req;
	unsigned long elapsed;

	atomic_dec(&ioq->inflight);

	if (work_mode)
		tags = hdev->shost->tag_set.tags[0];
	else
		tags = hdev->shost->tag_set.tags[ioq->qid - 1];
	req = blk_mq_tag_to_rq(tags, le16_to_cpu(cqe->cmd_id));
	if (unlikely(!req || !blk_mq_request_started(req))) {
		dev_warn(hdev->dev, "invalid id[%d] completed on queue[%d]\n",
			 le16_to_cpu(cqe->cmd_id), ioq->qid);
		return;
	}

	scmd = blk_mq_rq_to_pdu(req);
	mapbuf = scsi_cmd_priv(scmd);

	elapsed = jiffies - scmd->jiffies_at_alloc;
	dev_log_dbg(hdev->dev, "cid[%d] qid[%d] finish IO cost %3ld.%3ld seconds\n",
		    le16_to_cpu(cqe->cmd_id), ioq->qid, elapsed / HZ, elapsed % HZ);

	if (cmpxchg(&mapbuf->state, CMD_FLIGHT, CMD_COMPLETE) != CMD_FLIGHT) {
		dev_warn(hdev->dev, "cid[%d] qid[%d] enters abnormal handler, cost %3ld.%3ld seconds\n",
			 le16_to_cpu(cqe->cmd_id), ioq->qid, elapsed / HZ, elapsed % HZ);
		WRITE_ONCE(mapbuf->state, CMD_TMO_COMPLETE);

		if (mapbuf->sge_cnt) {
			mapbuf->sge_cnt = 0;
			scsi_dma_unmap(scmd);
		}
		hiraid_free_mapbuf(hdev, mapbuf);

		return;
	}

	hiraid_check_status(mapbuf, scmd, cqe);
	if (mapbuf->sge_cnt) {
		mapbuf->sge_cnt = 0;
		scsi_dma_unmap(scmd);
	}
	hiraid_free_mapbuf(hdev, mapbuf);
	scsi_done(scmd);
}

static void hiraid_complete_admin_cmnd(struct hiraid_queue *adminq, struct hiraid_completion *cqe)
{
	struct hiraid_dev *hdev = adminq->hdev;
	struct hiraid_cmd *adm_cmd;

	adm_cmd = hdev->adm_cmds + le16_to_cpu(cqe->cmd_id);
	if (unlikely(adm_cmd->state == CMD_IDLE)) {
		dev_warn(adminq->hdev->dev, "invalid id[%d] completed on queue[%d]\n",
			 le16_to_cpu(cqe->cmd_id), le16_to_cpu(cqe->sq_id));
		return;
	}

	adm_cmd->status = le16_to_cpu(cqe->status) >> 1;
	adm_cmd->result0 = le32_to_cpu(cqe->result);
	adm_cmd->result1 = le32_to_cpu(cqe->result1);

	complete(&adm_cmd->cmd_done);
}

static void hiraid_send_async_event(struct hiraid_dev *hdev, u16 cid);

static void hiraid_complete_async_event(struct hiraid_queue *hiraidq, struct hiraid_completion *cqe)
{
	struct hiraid_dev *hdev = hiraidq->hdev;
	u32 result = le32_to_cpu(cqe->result);

	dev_info(hdev->dev, "recv async event, cid[%d] status[0x%x] result[0x%x]\n",
		 le16_to_cpu(cqe->cmd_id), le16_to_cpu(cqe->status) >> 1, result);

	hiraid_send_async_event(hdev, le16_to_cpu(cqe->cmd_id));

	if ((le16_to_cpu(cqe->status) >> 1) != HIRAID_SC_SUCCESS)
		return;
	switch (result & 0x7) {
	case HIRAID_ASYN_EVENT_NOTICE:
		hiraid_handle_async_notice(hdev, result);
		break;
	case HIRAID_ASYN_EVENT_VS:
		hiraid_handle_async_vs(hdev, result, le32_to_cpu(cqe->result1));
		break;
	default:
		dev_warn(hdev->dev, "unsupported async event type[%u]\n", result & 0x7);
		break;
	}
}

static void hiraid_complete_pthru_cmnd(struct hiraid_queue *ioq, struct hiraid_completion *cqe)
{
	struct hiraid_dev *hdev = ioq->hdev;
	struct hiraid_cmd *ptcmd;

	ptcmd = hdev->io_ptcmds + (ioq->qid - 1) * HIRAID_PTHRU_CMDS_PERQ +
		le16_to_cpu(cqe->cmd_id) - hdev->scsi_qd;

	ptcmd->status = le16_to_cpu(cqe->status) >> 1;
	ptcmd->result0 = le32_to_cpu(cqe->result);
	ptcmd->result1 = le32_to_cpu(cqe->result1);

	complete(&ptcmd->cmd_done);
}

static inline void hiraid_handle_cqe(struct hiraid_queue *hiraidq, u16 idx)
{
	struct hiraid_completion *cqe = &hiraidq->cqes[idx];
	struct hiraid_dev *hdev = hiraidq->hdev;
	u16 cid = le16_to_cpu(cqe->cmd_id);

	if (unlikely(!work_mode && (cid >= hiraidq->q_depth))) {
		dev_err(hdev->dev, "invalid command id[%d] completed on queue[%d]\n",
			cid, cqe->sq_id);
		return;
	}

	dev_log_dbg(hdev->dev, "cid[%d] qid[%d] result[0x%x] sqid[%d] status[0x%x]\n",
		    cid, hiraidq->qid, le32_to_cpu(cqe->result),
		    le16_to_cpu(cqe->sq_id), le16_to_cpu(cqe->status));

	if (unlikely(hiraidq->qid == 0 && cid >= HIRAID_AQ_BLK_MQ_DEPTH)) {
		hiraid_complete_async_event(hiraidq, cqe);
		return;
	}

	if (unlikely(hiraidq->qid && cid >= hdev->scsi_qd)) {
		hiraid_complete_pthru_cmnd(hiraidq, cqe);
		return;
	}

	if (hiraidq->qid)
		hiraid_complete_io_cmnd(hiraidq, cqe);
	else
		hiraid_complete_admin_cmnd(hiraidq, cqe);
}

static void hiraid_complete_cqes(struct hiraid_queue *hiraidq, u16 start, u16 end)
{
	while (start != end) {
		hiraid_handle_cqe(hiraidq, start);
		if (++start == hiraidq->q_depth)
			start = 0;
	}
}

static inline void hiraid_update_cq_head(struct hiraid_queue *hiraidq)
{
	if (++hiraidq->cq_head == hiraidq->q_depth) {
		hiraidq->cq_head = 0;
		hiraidq->cq_phase = !hiraidq->cq_phase;
	}
}

static inline bool hiraid_process_cq(struct hiraid_queue *hiraidq, u16 *start, u16 *end, int tag)
{
	bool found = false;

	*start = hiraidq->cq_head;
	while (!found && hiraid_cqe_pending(hiraidq)) {
		if (le16_to_cpu(hiraidq->cqes[hiraidq->cq_head].cmd_id) == tag)
			found = true;
		hiraid_update_cq_head(hiraidq);
	}
	*end = hiraidq->cq_head;

	if (*start != *end)
		writel(hiraidq->cq_head, hiraidq->q_db + hiraidq->hdev->db_stride);

	return found;
}

static bool hiraid_poll_cq(struct hiraid_queue *hiraidq, int cid)
{
	u16 start, end;
	bool found;

	if (!hiraid_cqe_pending(hiraidq))
		return 0;

	spin_lock_irq(&hiraidq->cq_lock);
	found = hiraid_process_cq(hiraidq, &start, &end, cid);
	spin_unlock_irq(&hiraidq->cq_lock);

	hiraid_complete_cqes(hiraidq, start, end);
	return found;
}

static irqreturn_t hiraid_handle_irq(int irq, void *data)
{
	struct hiraid_queue *hiraidq = data;
	irqreturn_t ret = IRQ_NONE;
	u16 start, end;

	spin_lock(&hiraidq->cq_lock);
	if (hiraidq->cq_head != hiraidq->last_cq_head)
		ret = IRQ_HANDLED;

	hiraid_process_cq(hiraidq, &start, &end, -1);
	hiraidq->last_cq_head = hiraidq->cq_head;
	spin_unlock(&hiraidq->cq_lock);

	if (start != end) {
		hiraid_complete_cqes(hiraidq, start, end);
		ret = IRQ_HANDLED;
	}
	return ret;
}

static int hiraid_setup_admin_queue(struct hiraid_dev *hdev)
{
	struct hiraid_queue *adminq = &hdev->queues[0];
	u32 aqa;
	int ret;

	dev_info(hdev->dev, "start disable controller\n");

	ret = hiraid_disable_control(hdev);
	if (ret)
		return ret;

	ret = hiraid_alloc_queue(hdev, 0, HIRAID_AQ_DEPTH);
	if (ret)
		return ret;

	aqa = adminq->q_depth - 1;
	aqa |= aqa << 16;
	writel(aqa, hdev->bar + HIRAID_REG_AQA);
	lo_hi_writeq(adminq->sq_buffer_phy, hdev->bar + HIRAID_REG_ASQ);
	lo_hi_writeq(adminq->cq_buffer_phy, hdev->bar + HIRAID_REG_ACQ);

	dev_info(hdev->dev, "start enable controller\n");

	ret = hiraid_enable_control(hdev);
	if (ret) {
		ret = -ENODEV;
		return ret;
	}

	adminq->cq_vector = 0;
	ret = pci_request_irq(hdev->pdev, adminq->cq_vector, hiraid_handle_irq, NULL,
			      adminq, "hiraid%d_q%d", hdev->instance, adminq->qid);
	if (ret) {
		adminq->cq_vector = -1;
		return ret;
	}

	hiraid_init_queue(adminq, 0);

	dev_info(hdev->dev, "setup admin queue success, queuecount[%d] online[%d] pagesize[%d]\n",
		 hdev->queue_count, hdev->online_queues, hdev->page_size);

	return 0;
}

static u32 hiraid_get_bar_size(struct hiraid_dev *hdev, u32 nr_ioqs)
{
	return (HIRAID_REG_DBS + ((nr_ioqs + 1) * 8 * hdev->db_stride));
}

static int hiraid_create_admin_cmds(struct hiraid_dev *hdev)
{
	u16 i;

	INIT_LIST_HEAD(&hdev->adm_cmd_list);
	spin_lock_init(&hdev->adm_cmd_lock);

	hdev->adm_cmds = kcalloc_node(HIRAID_AQ_BLK_MQ_DEPTH, sizeof(struct hiraid_cmd),
				      GFP_KERNEL, hdev->numa_node);

	if (!hdev->adm_cmds) {
		dev_err(hdev->dev, "alloc admin cmds failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < HIRAID_AQ_BLK_MQ_DEPTH; i++) {
		hdev->adm_cmds[i].qid = 0;
		hdev->adm_cmds[i].cid = i;
		list_add_tail(&(hdev->adm_cmds[i].list), &hdev->adm_cmd_list);
	}

	dev_info(hdev->dev, "alloc admin cmds success, num[%d]\n", HIRAID_AQ_BLK_MQ_DEPTH);

	return 0;
}

static void hiraid_free_admin_cmds(struct hiraid_dev *hdev)
{
	kfree(hdev->adm_cmds);
	hdev->adm_cmds = NULL;
	INIT_LIST_HEAD(&hdev->adm_cmd_list);
}

static struct hiraid_cmd *hiraid_get_cmd(struct hiraid_dev *hdev, enum hiraid_cmd_type type)
{
	struct hiraid_cmd *cmd = NULL;
	unsigned long flags;
	struct list_head *head = &hdev->adm_cmd_list;
	spinlock_t *slock = &hdev->adm_cmd_lock;

	if (type == HIRAID_CMD_PTHRU) {
		head = &hdev->io_pt_list;
		slock = &hdev->io_pt_lock;
	}

	spin_lock_irqsave(slock, flags);
	if (list_empty(head)) {
		spin_unlock_irqrestore(slock, flags);
		dev_err(hdev->dev, "err, cmd[%d] list empty\n", type);
		return NULL;
	}
	cmd = list_entry(head->next, struct hiraid_cmd, list);
	list_del_init(&cmd->list);
	spin_unlock_irqrestore(slock, flags);

	WRITE_ONCE(cmd->state, CMD_FLIGHT);

	return cmd;
}

static void hiraid_put_cmd(struct hiraid_dev *hdev, struct hiraid_cmd *cmd,
				enum hiraid_cmd_type type)
{
	unsigned long flags;
	struct list_head *head = &hdev->adm_cmd_list;
	spinlock_t *slock = &hdev->adm_cmd_lock;

	if (type == HIRAID_CMD_PTHRU) {
		head = &hdev->io_pt_list;
		slock = &hdev->io_pt_lock;
	}

	spin_lock_irqsave(slock, flags);
	WRITE_ONCE(cmd->state, CMD_IDLE);
	list_add_tail(&cmd->list, head);
	spin_unlock_irqrestore(slock, flags);
}

static bool hiraid_admin_need_reset(struct hiraid_admin_command *cmd)
{
	switch (cmd->common.opcode) {
	case HIRAID_ADMIN_DELETE_SQ:
	case HIRAID_ADMIN_CREATE_SQ:
	case HIRAID_ADMIN_DELETE_CQ:
	case HIRAID_ADMIN_CREATE_CQ:
	case HIRAID_ADMIN_SET_FEATURES:
		return false;
	default:
		return true;
	}
}

static int hiraid_reset_work_sync(struct hiraid_dev *hdev);
static inline void hiraid_admin_timeout(struct hiraid_dev *hdev, struct hiraid_cmd *cmd)
{
	/* command may be returned because controller reset */
	if (READ_ONCE(cmd->state) == CMD_COMPLETE)
		return;
	if (hiraid_reset_work_sync(hdev) == -EBUSY)
		flush_work(&hdev->reset_work);
}

static int hiraid_put_admin_sync_request(struct hiraid_dev *hdev, struct hiraid_admin_command *cmd,
						u32 *result0, u32 *result1, u32 timeout)
{
	struct hiraid_cmd *adm_cmd = hiraid_get_cmd(hdev, HIRAID_CMD_ADMIN);

	if (!adm_cmd) {
		dev_err(hdev->dev, "err, get admin cmd failed\n");
		return -EFAULT;
	}

	timeout = timeout ? timeout : ADMIN_TIMEOUT;

	init_completion(&adm_cmd->cmd_done);

	cmd->common.cmd_id = cpu_to_le16(adm_cmd->cid);
	hiraid_submit_cmd(&hdev->queues[0], cmd);

	if (!wait_for_completion_timeout(&adm_cmd->cmd_done, timeout)) {
		dev_err(hdev->dev, "cid[%d] qid[%d] timeout, opcode[0x%x] subopcode[0x%x]\n",
			 adm_cmd->cid, adm_cmd->qid, cmd->usr_cmd.opcode,
			cmd->usr_cmd.info_0.subopcode);

		/* reset controller if admin timeout */
		if (hiraid_admin_need_reset(cmd))
			hiraid_admin_timeout(hdev, adm_cmd);

		hiraid_put_cmd(hdev, adm_cmd, HIRAID_CMD_ADMIN);
		return -ETIME;
	}

	if (result0)
		*result0 = adm_cmd->result0;
	if (result1)
		*result1 = adm_cmd->result1;

	hiraid_put_cmd(hdev, adm_cmd, HIRAID_CMD_ADMIN);

	return adm_cmd->status;
}

/**
 * hiraid_create_cq - send cmd to controller for create controller cq
 */
static int hiraid_create_complete_queue(struct hiraid_dev *hdev, u16 qid,
						struct hiraid_queue *hiraidq, u16 cq_vector)
{
	struct hiraid_admin_command admin_cmd;
	int flags = HIRAID_QUEUE_PHYS_CONTIG | HIRAID_CQ_IRQ_ENABLED;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.create_cq.opcode = HIRAID_ADMIN_CREATE_CQ;
	admin_cmd.create_cq.prp1 = cpu_to_le64(hiraidq->cq_buffer_phy);
	admin_cmd.create_cq.cqid = cpu_to_le16(qid);
	admin_cmd.create_cq.qsize = cpu_to_le16(hiraidq->q_depth - 1);
	admin_cmd.create_cq.cq_flags = cpu_to_le16(flags);
	admin_cmd.create_cq.irq_vector = cpu_to_le16(cq_vector);

	return hiraid_put_admin_sync_request(hdev, &admin_cmd, NULL, NULL, 0);
}

/**
 * hiraid_create_sq - send cmd to controller for create controller sq
 */
static int hiraid_create_send_queue(struct hiraid_dev *hdev, u16 qid,
					struct hiraid_queue *hiraidq)
{
	struct hiraid_admin_command admin_cmd;
	int flags = HIRAID_QUEUE_PHYS_CONTIG;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.create_sq.opcode = HIRAID_ADMIN_CREATE_SQ;
	admin_cmd.create_sq.prp1 = cpu_to_le64(hiraidq->sq_buffer_phy);
	admin_cmd.create_sq.sqid = cpu_to_le16(qid);
	admin_cmd.create_sq.qsize = cpu_to_le16(hiraidq->q_depth - 1);
	admin_cmd.create_sq.sq_flags = cpu_to_le16(flags);
	admin_cmd.create_sq.cqid = cpu_to_le16(qid);

	return hiraid_put_admin_sync_request(hdev, &admin_cmd, NULL, NULL, 0);
}

static void hiraid_free_all_queues(struct hiraid_dev *hdev)
{
	int i;
	struct hiraid_queue *hq;

	for (i = 0; i < hdev->queue_count; i++) {
		hq = &hdev->queues[i];
		dma_free_coherent(hdev->dev, CQ_SIZE(hq->q_depth),
				  (void *)hq->cqes, hq->cq_buffer_phy);
		dma_free_coherent(hdev->dev, SQ_SIZE(hq->qid, hq->q_depth),
				  hq->sq_cmds, hq->sq_buffer_phy);
		if (!work_mode)
			dma_free_coherent(hdev->dev, SENSE_SIZE(hq->q_depth),
					  hq->sense_buffer_virt, hq->sense_buffer_phy);
	}

	hdev->queue_count = 0;
}

static void hiraid_free_sense_buffer(struct hiraid_dev *hdev)
{
	if (hdev->sense_buffer_virt) {
		dma_free_coherent(hdev->dev,
			SENSE_SIZE(hdev->scsi_qd + max_hwq_num * HIRAID_PTHRU_CMDS_PERQ),
			hdev->sense_buffer_virt, hdev->sense_buffer_phy);
		hdev->sense_buffer_virt = NULL;
	}
}

static int hiraid_delete_queue(struct hiraid_dev *hdev, u8 opcode, u16 qid)
{
	struct hiraid_admin_command admin_cmd;
	int ret;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.delete_queue.opcode = opcode;
	admin_cmd.delete_queue.qid = cpu_to_le16(qid);

	ret = hiraid_put_admin_sync_request(hdev, &admin_cmd, NULL, NULL, 0);

	if (ret)
		dev_err(hdev->dev, "delete %s:[%d] failed\n",
			(opcode == HIRAID_ADMIN_DELETE_CQ) ? "cq" : "sq", qid);

	return ret;
}

static int hiraid_delete_complete_queue(struct hiraid_dev *hdev, u16 cqid)
{
	return hiraid_delete_queue(hdev, HIRAID_ADMIN_DELETE_CQ, cqid);
}

static int hiraid_delete_send_queue(struct hiraid_dev *hdev, u16 sqid)
{
	return hiraid_delete_queue(hdev, HIRAID_ADMIN_DELETE_SQ, sqid);
}

static int hiraid_create_queue(struct hiraid_queue *hiraidq, u16 qid)
{
	struct hiraid_dev *hdev = hiraidq->hdev;
	u16 cq_vector;
	int ret;

	cq_vector = (hdev->num_vecs == 1) ? 0 : qid;
	ret = hiraid_create_complete_queue(hdev, qid, hiraidq, cq_vector);
	if (ret)
		return ret;

	ret = hiraid_create_send_queue(hdev, qid, hiraidq);
	if (ret)
		goto delete_cq;

	hiraidq->cq_vector = cq_vector;
	ret = pci_request_irq(hdev->pdev, cq_vector, hiraid_handle_irq, NULL,
			      hiraidq, "hiraid%d_q%d", hdev->instance, qid);
	if (ret) {
		hiraidq->cq_vector = -1;
		dev_err(hdev->dev, "request queue[%d] irq failed\n", qid);
		goto delete_sq;
	}

	hiraid_init_queue(hiraidq, qid);

	return 0;

delete_sq:
	hiraid_delete_send_queue(hdev, qid);
delete_cq:
	hiraid_delete_complete_queue(hdev, qid);

	return ret;
}

static int hiraid_create_io_queues(struct hiraid_dev *hdev)
{
	u32 i, max;
	int ret = 0;

	max = min(hdev->max_qid, hdev->queue_count - 1);
	for (i = hdev->online_queues; i <= max; i++) {
		ret = hiraid_create_queue(&hdev->queues[i], i);
		if (ret) {
			dev_err(hdev->dev, "create queue[%d] failed\n", i);
			break;
		}
	}

	if (!hdev->last_qcnt)
		hdev->last_qcnt = hdev->online_queues;

	dev_info(hdev->dev, "queue_count[%d] online_queue[%d] last_online[%d]",
		 hdev->queue_count, hdev->online_queues, hdev->last_qcnt);

	return ret >= 0 ? 0 : ret;
}

static int hiraid_set_features(struct hiraid_dev *hdev, u32 fid, u32 dword11, void *buffer,
				size_t buflen, u32 *result)
{
	struct hiraid_admin_command admin_cmd;
	int ret;
	u8 *data_ptr = NULL;
	dma_addr_t buffer_phy = 0;

	if (buffer && buflen) {
		data_ptr = dma_alloc_coherent(hdev->dev, buflen, &buffer_phy, GFP_KERNEL);
		if (!data_ptr)
			return -ENOMEM;

		memcpy(data_ptr, buffer, buflen);
	}

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.features.opcode = HIRAID_ADMIN_SET_FEATURES;
	admin_cmd.features.fid = cpu_to_le32(fid);
	admin_cmd.features.dword11 = cpu_to_le32(dword11);
	admin_cmd.common.dptr.prp1 = cpu_to_le64(buffer_phy);

	ret = hiraid_put_admin_sync_request(hdev, &admin_cmd, result, NULL, 0);

	if (data_ptr)
		dma_free_coherent(hdev->dev, buflen, data_ptr, buffer_phy);

	return ret;
}

static int hiraid_configure_timestamp(struct hiraid_dev *hdev)
{
	__le64 timestamp;
	int ret;

	timestamp = cpu_to_le64(ktime_to_ms(ktime_get_real()));
	ret = hiraid_set_features(hdev, HIRAID_FEATURE_TIMESTAMP, 0,
				&timestamp, sizeof(timestamp), NULL);

	if (ret)
		dev_err(hdev->dev, "set timestamp failed[%d]\n", ret);
	return ret;
}

static int hiraid_get_queue_cnt(struct hiraid_dev *hdev, u32 *cnt)
{
	u32 q_cnt = (*cnt - 1) | ((*cnt - 1) << 16);
	u32 nr_ioqs, result;
	int status;

	status = hiraid_set_features(hdev, HIRAID_FEATURE_NUM_QUEUES, q_cnt, NULL, 0, &result);
	if (status) {
		dev_err(hdev->dev, "set queue count failed, status[%d]\n",
			status);
		return -EIO;
	}

	nr_ioqs = min(result & 0xffff, result >> 16) + 1;
	*cnt = min(*cnt, nr_ioqs);
	if (*cnt == 0) {
		dev_err(hdev->dev, "illegal qcount: zero, nr_ioqs[%d], cnt[%d]\n", nr_ioqs, *cnt);
		return -EIO;
	}
	return 0;
}

static int hiraid_setup_io_queues(struct hiraid_dev *hdev)
{
	struct hiraid_queue *adminq = &hdev->queues[0];
	struct pci_dev *pdev = hdev->pdev;
	u32 i, size, nr_ioqs;
	int ret;

	struct irq_affinity affd = {
		.pre_vectors = 1
	};

	/* alloc IO sense buffer for single hw queue mode */
	if (work_mode && !hdev->sense_buffer_virt) {
		hdev->sense_buffer_virt = dma_alloc_coherent(hdev->dev,
			SENSE_SIZE(hdev->scsi_qd + max_hwq_num * HIRAID_PTHRU_CMDS_PERQ),
			&hdev->sense_buffer_phy, GFP_KERNEL | __GFP_ZERO);
		if (!hdev->sense_buffer_virt)
			return -ENOMEM;
	}

	nr_ioqs = min(num_online_cpus(), max_hwq_num);
	ret = hiraid_get_queue_cnt(hdev, &nr_ioqs);
	if (ret < 0)
		return ret;

	size = hiraid_get_bar_size(hdev, nr_ioqs);
	ret = hiraid_remap_bar(hdev, size);
	if (ret)
		return -ENOMEM;

	adminq->q_db = hdev->dbs;

	pci_free_irq(pdev, 0, adminq);
	pci_free_irq_vectors(pdev);
	hdev->online_queues--;

	ret = pci_alloc_irq_vectors_affinity(pdev, 1, (nr_ioqs + 1),
					PCI_IRQ_ALL_TYPES | PCI_IRQ_AFFINITY, &affd);
	if (ret <= 0)
		return -EIO;

	hdev->num_vecs = ret;
	hdev->max_qid = max(ret - 1, 1);

	ret = pci_request_irq(pdev, adminq->cq_vector, hiraid_handle_irq, NULL,
			adminq, "hiraid%d_q%d", hdev->instance, adminq->qid);
	if (ret) {
		dev_err(hdev->dev, "request admin irq failed\n");
		adminq->cq_vector = -1;
		return ret;
	}

	hdev->online_queues++;

	for (i = hdev->queue_count; i <= hdev->max_qid; i++) {
		ret = hiraid_alloc_queue(hdev, i, hdev->ioq_depth);
		if (ret)
			break;
	}
	dev_info(hdev->dev, "max_qid[%d] queuecount[%d] onlinequeue[%d] ioqdepth[%d]\n",
		 hdev->max_qid, hdev->queue_count, hdev->online_queues, hdev->ioq_depth);

	return hiraid_create_io_queues(hdev);
}

static void hiraid_delete_io_queues(struct hiraid_dev *hdev)
{
	u16 queues = hdev->online_queues - 1;
	u8 opcode = HIRAID_ADMIN_DELETE_SQ;
	u16 i, pass;

	if (!pci_device_is_present(hdev->pdev)) {
		dev_err(hdev->dev, "pci_device is not present, skip disable io queues\n");
		return;
	}

	if (hdev->online_queues < 2) {
		dev_err(hdev->dev, "err, io queue has been delete\n");
		return;
	}

	for (pass = 0; pass < 2; pass++) {
		for (i = queues; i > 0; i--)
			if (hiraid_delete_queue(hdev, opcode, i))
				break;

		opcode = HIRAID_ADMIN_DELETE_CQ;
	}
}

static void hiraid_pci_disable(struct hiraid_dev *hdev)
{
	struct pci_dev *pdev = hdev->pdev;
	u32 i;

	for (i = 0; i < hdev->online_queues; i++)
		pci_free_irq(pdev, hdev->queues[i].cq_vector, &hdev->queues[i]);
	pci_free_irq_vectors(pdev);
	if (pci_is_enabled(pdev))
		pci_disable_device(pdev);
	hdev->online_queues = 0;
}

static void hiraid_disable_admin_queue(struct hiraid_dev *hdev, bool shutdown)
{
	struct hiraid_queue *adminq = &hdev->queues[0];
	u16 start, end;

	if (pci_device_is_present(hdev->pdev)) {
		if (shutdown)
			hiraid_shutdown_control(hdev);
		else
			hiraid_disable_control(hdev);
	}

	if (hdev->queue_count == 0) {
		dev_err(hdev->dev, "err, admin queue has been delete\n");
		return;
	}

	spin_lock_irq(&adminq->cq_lock);
	hiraid_process_cq(adminq, &start, &end, -1);
	spin_unlock_irq(&adminq->cq_lock);
	hiraid_complete_cqes(adminq, start, end);
}

static int hiraid_create_prp_pools(struct hiraid_dev *hdev)
{
	int i;
	char poolname[20] = { 0 };

	hdev->prp_page_pool = dma_pool_create("prp list page", hdev->dev,
					PAGE_SIZE, PAGE_SIZE, 0);

	if (!hdev->prp_page_pool) {
		dev_err(hdev->dev, "create prp_page_pool failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < extra_pool_num; i++) {
		sprintf(poolname, "prp_list_256_%d", i);
		hdev->prp_extra_pool[i] = dma_pool_create(poolname, hdev->dev, EXTRA_POOL_SIZE,
							EXTRA_POOL_SIZE, 0);

		if (!hdev->prp_extra_pool[i]) {
			dev_err(hdev->dev, "create prp extra pool[%d] failed\n", i);
			goto destroy_prp_extra_pool;
		}
	}

	return 0;

destroy_prp_extra_pool:
	while (i > 0)
		dma_pool_destroy(hdev->prp_extra_pool[--i]);
	dma_pool_destroy(hdev->prp_page_pool);

	return -ENOMEM;
}

static void hiraid_free_prp_pools(struct hiraid_dev *hdev)
{
	int i;

	for (i = 0; i < extra_pool_num; i++)
		dma_pool_destroy(hdev->prp_extra_pool[i]);
	dma_pool_destroy(hdev->prp_page_pool);
}

static int hiraid_request_devices(struct hiraid_dev *hdev, struct hiraid_dev_info *dev)
{
	u32 nd = le32_to_cpu(hdev->ctrl_info->nd);
	struct hiraid_admin_command admin_cmd;
	struct hiraid_dev_list *list_buf;
	dma_addr_t buffer_phy = 0;
	u32 i, idx, hdid, ndev;
	int ret = 0;

	list_buf = dma_alloc_coherent(hdev->dev, PAGE_SIZE, &buffer_phy, GFP_KERNEL);
	if (!list_buf)
		return -ENOMEM;

	for (idx = 0; idx < nd;) {
		memset(&admin_cmd, 0, sizeof(admin_cmd));
		admin_cmd.get_info.opcode = HIRAID_ADMIN_GET_INFO;
		admin_cmd.get_info.type = HIRAID_GET_DEVLIST_INFO;
		admin_cmd.get_info.cdw11 = cpu_to_le32(idx);
		admin_cmd.common.dptr.prp1 = cpu_to_le64(buffer_phy);

		ret = hiraid_put_admin_sync_request(hdev, &admin_cmd, NULL, NULL, 0);

		if (ret) {
			dev_err(hdev->dev, "get device list failed, nd[%u] idx[%u] ret[%d]\n",
				nd, idx, ret);
			goto out;
		}
		ndev = le32_to_cpu(list_buf->dev_num);

		dev_info(hdev->dev, "get dev list ndev num[%u]\n", ndev);

		for (i = 0; i < ndev; i++) {
			hdid = le32_to_cpu(list_buf->devinfo[i].hdid);
			dev_info(hdev->dev, "devices[%d], hdid[%u] target[%d] channel[%d] lun[%d] attr[0x%x]\n",
				 i, hdid, le16_to_cpu(list_buf->devinfo[i].target),
				 list_buf->devinfo[i].channel,
				 list_buf->devinfo[i].lun,
				 list_buf->devinfo[i].attr);
			if (hdid > nd || hdid == 0) {
				dev_err(hdev->dev, "err, hdid[%d] invalid\n", hdid);
				continue;
			}
			memcpy(&dev[hdid - 1], &list_buf->devinfo[i],
			       sizeof(struct hiraid_dev_info));
		}
		idx += ndev;

		if (ndev < MAX_DEV_ENTRY_PER_PAGE_4K)
			break;
	}

out:
	dma_free_coherent(hdev->dev, PAGE_SIZE, list_buf, buffer_phy);
	return ret;
}

static void hiraid_send_async_event(struct hiraid_dev *hdev, u16 cid)
{
	struct hiraid_queue *adminq = &hdev->queues[0];
	struct hiraid_admin_command admin_cmd;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.common.opcode = HIRAID_ADMIN_ASYNC_EVENT;
	admin_cmd.common.cmd_id = cpu_to_le16(cid);

	hiraid_submit_cmd(adminq, &admin_cmd);
	dev_info(hdev->dev, "send async event to controller, cid[%d]\n", cid);
}

static inline void hiraid_init_async_event(struct hiraid_dev *hdev)
{
	u16 i;

	for (i = 0; i < hdev->ctrl_info->asynevent; i++)
		hiraid_send_async_event(hdev, i + HIRAID_AQ_BLK_MQ_DEPTH);
}

static int hiraid_add_device(struct hiraid_dev *hdev, struct hiraid_dev_info *devinfo)
{
	struct Scsi_Host *shost = hdev->shost;
	struct scsi_device *sdev;

	dev_info(hdev->dev, "add device, hdid[%u] target[%d] channel[%d] lun[%d] attr[0x%x]\n",
			le32_to_cpu(devinfo->hdid), le16_to_cpu(devinfo->target),
			devinfo->channel, devinfo->lun, devinfo->attr);

	sdev = scsi_device_lookup(shost, devinfo->channel, le16_to_cpu(devinfo->target), 0);
	if (sdev) {
		dev_warn(hdev->dev, "device is already exist, channel[%d] targetid[%d] lun[%d]\n",
			 devinfo->channel, le16_to_cpu(devinfo->target), 0);
		scsi_device_put(sdev);
		return -EEXIST;
	}
	scsi_add_device(shost, devinfo->channel, le16_to_cpu(devinfo->target), 0);
	return 0;
}

static int hiraid_rescan_device(struct hiraid_dev *hdev, struct hiraid_dev_info *devinfo)
{
	struct Scsi_Host *shost = hdev->shost;
	struct scsi_device *sdev;

	dev_info(hdev->dev, "rescan device, hdid[%u] target[%d] channel[%d] lun[%d] attr[0x%x]\n",
			le32_to_cpu(devinfo->hdid), le16_to_cpu(devinfo->target),
			devinfo->channel, devinfo->lun, devinfo->attr);

	sdev = scsi_device_lookup(shost, devinfo->channel, le16_to_cpu(devinfo->target), 0);
	if (!sdev) {
		dev_warn(hdev->dev, "device is not exit rescan it, channel[%d] target_id[%d] lun[%d]\n",
			 devinfo->channel, le16_to_cpu(devinfo->target), 0);
		return -ENODEV;
	}

	scsi_rescan_device(sdev);
	scsi_device_put(sdev);
	return 0;
}

static int hiraid_delete_device(struct hiraid_dev *hdev, struct hiraid_dev_info *devinfo)
{
	struct Scsi_Host *shost = hdev->shost;
	struct scsi_device *sdev;

	dev_info(hdev->dev, "remove device, hdid[%u] target[%d] channel[%d] lun[%d] attr[0x%x]\n",
			le32_to_cpu(devinfo->hdid), le16_to_cpu(devinfo->target),
			devinfo->channel, devinfo->lun, devinfo->attr);

	sdev = scsi_device_lookup(shost, devinfo->channel, le16_to_cpu(devinfo->target), 0);
	if (!sdev) {
		dev_warn(hdev->dev, "device is not exit remove it, channel[%d] target_id[%d] lun[%d]\n",
			 devinfo->channel, le16_to_cpu(devinfo->target), 0);
		return -ENODEV;
	}

	scsi_remove_device(sdev);
	scsi_device_put(sdev);
	return 0;
}

static int hiraid_dev_list_init(struct hiraid_dev *hdev)
{
	u32 nd = le32_to_cpu(hdev->ctrl_info->nd);

	hdev->dev_info = kzalloc_node(nd * sizeof(struct hiraid_dev_info),
				     GFP_KERNEL, hdev->numa_node);
	if (!hdev->dev_info)
		return -ENOMEM;

	return 0;
}

static int hiraid_luntarget_sort(const void *l, const void *r)
{
	const struct hiraid_dev_info *ln = l;
	const struct hiraid_dev_info *rn = r;
	int l_attr = HIRAID_DEV_INFO_ATTR_BOOT(ln->attr);
	int r_attr = HIRAID_DEV_INFO_ATTR_BOOT(rn->attr);

	/* boot first */
	if (l_attr != r_attr)
		return (r_attr - l_attr);

	if (ln->channel == rn->channel)
		return le16_to_cpu(ln->target) - le16_to_cpu(rn->target);

	return ln->channel - rn->channel;
}

static void hiraid_scan_work(struct work_struct *work)
{
	struct hiraid_dev *hdev =
		container_of(work, struct hiraid_dev, scan_work);
	struct hiraid_dev_info *dev, *old_dev, *new_dev;
	u32 nd = le32_to_cpu(hdev->ctrl_info->nd);
	u8 flag, org_flag;
	int i, ret;
	int count = 0;

	dev = kcalloc(nd, sizeof(struct hiraid_dev_info), GFP_KERNEL);
	if (!dev)
		return;

	new_dev = kcalloc(nd, sizeof(struct hiraid_dev_info), GFP_KERNEL);
	if (!new_dev)
		goto free_list;

	ret = hiraid_request_devices(hdev, dev);
	if (ret)
		goto free_all;
	old_dev = hdev->dev_info;
	for (i = 0; i < nd; i++) {
		org_flag = old_dev[i].flag;
		flag = dev[i].flag;

		dev_log_dbg(hdev->dev, "i[%d] org_flag[0x%x] flag[0x%x]\n", i, org_flag, flag);

		if (HIRAID_DEV_INFO_FLAG_VALID(flag)) {
			if (!HIRAID_DEV_INFO_FLAG_VALID(org_flag)) {
				down_write(&hdev->dev_rwsem);
				memcpy(&old_dev[i], &dev[i],
						sizeof(struct hiraid_dev_info));
				memcpy(&new_dev[count++], &dev[i],
						sizeof(struct hiraid_dev_info));
				up_write(&hdev->dev_rwsem);
			} else if (HIRAID_DEV_INFO_FLAG_CHANGE(flag)) {
				hiraid_rescan_device(hdev, &dev[i]);
			}
		} else {
			if (HIRAID_DEV_INFO_FLAG_VALID(org_flag)) {
				down_write(&hdev->dev_rwsem);
				old_dev[i].flag &= 0xfe;
				up_write(&hdev->dev_rwsem);
				hiraid_delete_device(hdev, &old_dev[i]);
			}
		}
	}

	dev_info(hdev->dev, "scan work add device num[%d]\n", count);

	sort(new_dev, count, sizeof(new_dev[0]), hiraid_luntarget_sort, NULL);

	for (i = 0; i < count; i++)
		hiraid_add_device(hdev, &new_dev[i]);

free_all:
	kfree(new_dev);
free_list:
	kfree(dev);
}

static void hiraid_timesyn_work(struct work_struct *work)
{
	struct hiraid_dev *hdev =
		container_of(work, struct hiraid_dev, timesyn_work);

	hiraid_configure_timestamp(hdev);
}

static int hiraid_init_control_info(struct hiraid_dev *hdev);
static void hiraid_fwactive_work(struct work_struct *work)
{
	struct hiraid_dev *hdev = container_of(work, struct hiraid_dev, fwact_work);

	if (hiraid_init_control_info(hdev))
		dev_err(hdev->dev, "get controller info failed after fw activation\n");
}

static void hiraid_queue_scan(struct hiraid_dev *hdev)
{
	queue_work(work_queue, &hdev->scan_work);
}

static void hiraid_handle_async_notice(struct hiraid_dev *hdev, u32 result)
{
	switch ((result & 0xff00) >> 8) {
	case HIRAID_ASYN_DEV_CHANGED:
		hiraid_queue_scan(hdev);
		break;
	case HIRAID_ASYN_FW_ACT_START:
		dev_info(hdev->dev, "fw activation starting\n");
		break;
	case HIRAID_ASYN_HOST_PROBING:
		break;
	default:
		dev_warn(hdev->dev, "async event result[%08x]\n", result);
	}
}

static void hiraid_handle_async_vs(struct hiraid_dev *hdev, u32 result, u32 result1)
{
	switch ((result & 0xff00) >> 8) {
	case HIRAID_ASYN_TIMESYN:
		queue_work(work_queue, &hdev->timesyn_work);
		break;
	case HIRAID_ASYN_FW_ACT_FINISH:
		dev_info(hdev->dev, "fw activation finish\n");
		queue_work(work_queue, &hdev->fwact_work);
		break;
	case HIRAID_ASYN_EVENT_MIN ... HIRAID_ASYN_EVENT_MAX:
		dev_info(hdev->dev, "recv card event[%d] param1[0x%x] param2[0x%x]\n",
			 (result & 0xff00) >> 8, result, result1);
		break;
	default:
		dev_warn(hdev->dev, "async event result[0x%x]\n", result);
	}
}

static int hiraid_alloc_resources(struct hiraid_dev *hdev)
{
	int ret, nqueue;

	hdev->ctrl_info = kzalloc_node(sizeof(*hdev->ctrl_info), GFP_KERNEL, hdev->numa_node);
	if (!hdev->ctrl_info)
		return -ENOMEM;

	ret = hiraid_create_prp_pools(hdev);
	if (ret)
		goto free_ctrl_info;
	nqueue = min(num_possible_cpus(), max_hwq_num) + 1;
	hdev->queues = kcalloc_node(nqueue, sizeof(struct hiraid_queue),
				    GFP_KERNEL, hdev->numa_node);
	if (!hdev->queues) {
		ret = -ENOMEM;
		goto destroy_dma_pools;
	}

	ret = hiraid_create_admin_cmds(hdev);
	if (ret)
		goto free_queues;

	dev_info(hdev->dev, "total queues num[%d]\n", nqueue);

	return 0;

free_queues:
	kfree(hdev->queues);
destroy_dma_pools:
	hiraid_free_prp_pools(hdev);
free_ctrl_info:
	kfree(hdev->ctrl_info);

	return ret;
}

static void hiraid_free_resources(struct hiraid_dev *hdev)
{
	hiraid_free_admin_cmds(hdev);
	kfree(hdev->queues);
	hiraid_free_prp_pools(hdev);
	kfree(hdev->ctrl_info);
}

static void hiraid_bsg_buf_unmap(struct hiraid_dev *hdev, struct bsg_job *job)
{
	struct request *rq = blk_mq_rq_from_pdu(job);
	struct hiraid_mapmange *mapbuf = job->dd_data;
	enum dma_data_direction dma_dir = rq_data_dir(rq) ? DMA_TO_DEVICE : DMA_FROM_DEVICE;

	if (mapbuf->sge_cnt)
		dma_unmap_sg(hdev->dev, mapbuf->sgl, mapbuf->sge_cnt, dma_dir);

	hiraid_free_mapbuf(hdev, mapbuf);
}

static int hiraid_bsg_buf_map(struct hiraid_dev *hdev, struct bsg_job *job,
				struct hiraid_admin_command *cmd)
{
	struct hiraid_bsg_request *bsg_req = job->request;
	struct request *rq = blk_mq_rq_from_pdu(job);
	struct hiraid_mapmange *mapbuf = job->dd_data;
	enum dma_data_direction dma_dir = rq_data_dir(rq) ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
	int ret = 0;

	/* No data to DMA, it may be scsi no-rw command */
	mapbuf->sge_cnt = job->request_payload.sg_cnt;
	mapbuf->sgl = job->request_payload.sg_list;
	mapbuf->len = job->request_payload.payload_len;
	mapbuf->page_cnt = -1;
	if (unlikely(mapbuf->sge_cnt == 0))
		goto out;

	ret = dma_map_sg_attrs(hdev->dev, mapbuf->sgl, mapbuf->sge_cnt, dma_dir, DMA_ATTR_NO_WARN);
	if (!ret)
		goto out;

	mapbuf->use_sgl = !hiraid_is_prp(hdev, mapbuf->sgl, mapbuf->sge_cnt);

	if ((mapbuf->use_sgl == (bool)true) && (bsg_req->msgcode == HIRAID_BSG_IOPTHRU) &&
		(hdev->ctrl_info->pt_use_sgl != (bool)false)) {
		ret = hiraid_build_passthru_sgl(hdev, cmd, mapbuf);
	} else {
		mapbuf->use_sgl = false;

		ret = hiraid_build_passthru_prp(hdev, mapbuf);
		cmd->common.dptr.prp1 = cpu_to_le64(sg_dma_address(mapbuf->sgl));
		cmd->common.dptr.prp2 = cpu_to_le64(mapbuf->first_dma);
	}

	if (ret)
		goto unmap;

	return 0;

unmap:
	dma_unmap_sg(hdev->dev, mapbuf->sgl, mapbuf->sge_cnt, dma_dir);
out:
	return ret;
}

static int hiraid_get_control_info(struct hiraid_dev *hdev, struct hiraid_ctrl_info *ctrl_info)
{
	struct hiraid_admin_command admin_cmd;
	u8 *data_ptr = NULL;
	dma_addr_t buffer_phy = 0;
	int ret;

	data_ptr = dma_alloc_coherent(hdev->dev, PAGE_SIZE, &buffer_phy, GFP_KERNEL);
	if (!data_ptr)
		return -ENOMEM;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.get_info.opcode = HIRAID_ADMIN_GET_INFO;
	admin_cmd.get_info.type = HIRAID_GET_CTRL_INFO;
	admin_cmd.common.dptr.prp1 = cpu_to_le64(buffer_phy);

	ret = hiraid_put_admin_sync_request(hdev, &admin_cmd, NULL, NULL, 0);
	if (!ret)
		memcpy(ctrl_info, data_ptr, sizeof(struct hiraid_ctrl_info));

	dma_free_coherent(hdev->dev, PAGE_SIZE, data_ptr, buffer_phy);

	return ret;
}

static int hiraid_init_control_info(struct hiraid_dev *hdev)
{
	int ret;

	hdev->ctrl_info->nd = cpu_to_le32(240);
	hdev->ctrl_info->mdts = 8;
	hdev->ctrl_info->max_cmds = cpu_to_le16(4096);
	hdev->ctrl_info->max_num_sge = cpu_to_le16(128);
	hdev->ctrl_info->max_channel = cpu_to_le16(4);
	hdev->ctrl_info->max_tgt_id = cpu_to_le32(3239);
	hdev->ctrl_info->max_lun = cpu_to_le16(2);

	ret = hiraid_get_control_info(hdev, hdev->ctrl_info);
	if (ret)
		dev_err(hdev->dev, "get controller info failed[%d]\n", ret);

	dev_info(hdev->dev, "device_num = %d\n", hdev->ctrl_info->nd);
	dev_info(hdev->dev, "max_cmd = %d\n", hdev->ctrl_info->max_cmds);
	dev_info(hdev->dev, "max_channel = %d\n", hdev->ctrl_info->max_channel);
	dev_info(hdev->dev, "max_tgt_id = %d\n", hdev->ctrl_info->max_tgt_id);
	dev_info(hdev->dev, "max_lun = %d\n", hdev->ctrl_info->max_lun);
	dev_info(hdev->dev, "max_num_sge = %d\n", hdev->ctrl_info->max_num_sge);
	dev_info(hdev->dev, "lun_num_boot = %d\n", hdev->ctrl_info->lun_num_boot);
	dev_info(hdev->dev, "max_data_transfer_size = %d\n", hdev->ctrl_info->mdts);
	dev_info(hdev->dev, "abort_cmd_limit = %d\n", hdev->ctrl_info->acl);
	dev_info(hdev->dev, "asyn_event_num = %d\n", hdev->ctrl_info->asynevent);
	dev_info(hdev->dev, "card_type = %d\n", hdev->ctrl_info->card_type);
	dev_info(hdev->dev, "pt_use_sgl = %d\n", hdev->ctrl_info->pt_use_sgl);
	dev_info(hdev->dev, "rtd3e = %d\n", hdev->ctrl_info->rtd3e);
	dev_info(hdev->dev, "serial_num = %s\n", hdev->ctrl_info->sn);
	dev_info(hdev->dev, "fw_verion = %s\n", hdev->ctrl_info->fw_version);

	if (!hdev->ctrl_info->asynevent)
		hdev->ctrl_info->asynevent = 1;
	if (hdev->ctrl_info->asynevent > HIRAID_ASYN_COMMANDS)
		hdev->ctrl_info->asynevent = HIRAID_ASYN_COMMANDS;

	hdev->scsi_qd = work_mode ?
		le16_to_cpu(hdev->ctrl_info->max_cmds) : (hdev->ioq_depth - HIRAID_PTHRU_CMDS_PERQ);

	return 0;
}

static int hiraid_user_send_admcmd(struct hiraid_dev *hdev, struct bsg_job *job)
{
	struct hiraid_bsg_request *bsg_req = job->request;
	struct hiraid_passthru_common_cmd *ptcmd = &(bsg_req->admcmd);
	struct hiraid_admin_command admin_cmd;
	u32 timeout = msecs_to_jiffies(ptcmd->timeout_ms);
	u32 result[2] = {0};
	int status;

	if (hdev->state >= DEV_RESETTING) {
		dev_err(hdev->dev, "err, host state[%d] is not right\n",
			hdev->state);
		return -EBUSY;
	}

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.common.opcode = ptcmd->opcode;
	admin_cmd.common.flags = ptcmd->flags;
	admin_cmd.common.hdid = cpu_to_le32(ptcmd->nsid);
	admin_cmd.common.cdw2[0] = cpu_to_le32(ptcmd->cdw2);
	admin_cmd.common.cdw2[1] = cpu_to_le32(ptcmd->cdw3);
	admin_cmd.common.cdw10 = cpu_to_le32(ptcmd->cdw10);
	admin_cmd.common.cdw11 = cpu_to_le32(ptcmd->cdw11);
	admin_cmd.common.cdw12 = cpu_to_le32(ptcmd->cdw12);
	admin_cmd.common.cdw13 = cpu_to_le32(ptcmd->cdw13);
	admin_cmd.common.cdw14 = cpu_to_le32(ptcmd->cdw14);
	admin_cmd.common.cdw15 = cpu_to_le32(ptcmd->cdw15);

	status = hiraid_bsg_buf_map(hdev, job, &admin_cmd);
	if (status) {
		dev_err(hdev->dev, "err, map data failed\n");
		return status;
	}

	status = hiraid_put_admin_sync_request(hdev, &admin_cmd, &result[0], &result[1], timeout);
	if (status >= 0) {
		job->reply_len = sizeof(result);
		memcpy(job->reply, result, sizeof(result));
	}
	if (status)
		dev_info(hdev->dev, "opcode[0x%x] subopcode[0x%x] status[0x%x] result0[0x%x];"
			"result1[0x%x]\n", ptcmd->opcode, ptcmd->info_0.subopcode, status,
			result[0], result[1]);

	hiraid_bsg_buf_unmap(hdev, job);

	return status;
}

static int hiraid_alloc_io_ptcmds(struct hiraid_dev *hdev)
{
	u32 i;
	u32 ptnum = HIRAID_TOTAL_PTCMDS(hdev->online_queues - 1);

	INIT_LIST_HEAD(&hdev->io_pt_list);
	spin_lock_init(&hdev->io_pt_lock);

	hdev->io_ptcmds = kcalloc_node(ptnum, sizeof(struct hiraid_cmd),
					GFP_KERNEL, hdev->numa_node);

	if (!hdev->io_ptcmds) {
		dev_err(hdev->dev, "alloc io pthrunum failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < ptnum; i++) {
		hdev->io_ptcmds[i].qid = i / HIRAID_PTHRU_CMDS_PERQ + 1;
		hdev->io_ptcmds[i].cid = i % HIRAID_PTHRU_CMDS_PERQ + hdev->scsi_qd;
		list_add_tail(&(hdev->io_ptcmds[i].list), &hdev->io_pt_list);
	}

	dev_info(hdev->dev, "alloc io pthru cmd success, pthrunum[%d]\n", ptnum);

	return 0;
}

static void hiraid_free_io_ptcmds(struct hiraid_dev *hdev)
{
	kfree(hdev->io_ptcmds);
	hdev->io_ptcmds = NULL;

	INIT_LIST_HEAD(&hdev->io_pt_list);
}

static int hiraid_put_io_sync_request(struct hiraid_dev *hdev, struct hiraid_scsi_io_cmd *io_cmd,
					u32 *result, u32 *reslen, u32 timeout)
{
	int ret;
	dma_addr_t buffer_phy;
	struct hiraid_queue *ioq;
	void *sense_addr = NULL;
	struct hiraid_cmd *pt_cmd = hiraid_get_cmd(hdev, HIRAID_CMD_PTHRU);

	if (!pt_cmd) {
		dev_err(hdev->dev, "err, get ioq cmd failed\n");
		return -EFAULT;
	}

	timeout = timeout ? timeout : ADMIN_TIMEOUT;

	init_completion(&pt_cmd->cmd_done);

	ioq = &hdev->queues[pt_cmd->qid];
	if (work_mode) {
		ret = ((pt_cmd->qid - 1) * HIRAID_PTHRU_CMDS_PERQ + pt_cmd->cid) *
			SCSI_SENSE_BUFFERSIZE;
		sense_addr = hdev->sense_buffer_virt + ret;
		buffer_phy = hdev->sense_buffer_phy + ret;
	} else {
		ret = pt_cmd->cid * SCSI_SENSE_BUFFERSIZE;
		sense_addr = ioq->sense_buffer_virt + ret;
		buffer_phy = ioq->sense_buffer_phy + ret;
	}

	io_cmd->common.sense_addr = cpu_to_le64(buffer_phy);
	io_cmd->common.sense_len = cpu_to_le16(SCSI_SENSE_BUFFERSIZE);
	io_cmd->common.cmd_id = cpu_to_le16(pt_cmd->cid);

	hiraid_submit_cmd(ioq, io_cmd);

	if (!wait_for_completion_timeout(&pt_cmd->cmd_done, timeout)) {
		dev_err(hdev->dev, "cid[%d] qid[%d] timeout, opcode[0x%x] subopcode[0x%x]\n",
			pt_cmd->cid, pt_cmd->qid, io_cmd->common.opcode,
			(le32_to_cpu(io_cmd->common.cdw3[0]) & 0xffff));

		hiraid_admin_timeout(hdev, pt_cmd);

		hiraid_put_cmd(hdev, pt_cmd, HIRAID_CMD_PTHRU);
		return -ETIME;
	}

	if (result && reslen) {
		if ((pt_cmd->status & 0x17f) == 0x101) {
			memcpy(result, sense_addr, SCSI_SENSE_BUFFERSIZE);
			*reslen = SCSI_SENSE_BUFFERSIZE;
		}
	}

	hiraid_put_cmd(hdev, pt_cmd, HIRAID_CMD_PTHRU);

	return pt_cmd->status;
}

static int hiraid_user_send_ptcmd(struct hiraid_dev *hdev, struct bsg_job *job)
{
	struct hiraid_bsg_request *bsg_req = (struct hiraid_bsg_request *)(job->request);
	struct hiraid_passthru_io_cmd *cmd = &(bsg_req->pthrucmd);
	struct hiraid_scsi_io_cmd pthru_cmd;
	int status = 0;
	u32 timeout = msecs_to_jiffies(cmd->timeout_ms);
	// data len is 4k before use sgl, now len is 1M
	u32 io_pt_data_len = (hdev->ctrl_info->pt_use_sgl == (bool)true) ?
		IOQ_PT_SGL_DATA_LEN : IOQ_PT_DATA_LEN;

	if (cmd->data_len > io_pt_data_len) {
		dev_err(hdev->dev, "data len bigger than %d\n", io_pt_data_len);
		return -EFAULT;
	}

	if (hdev->state != DEV_LIVE) {
		dev_err(hdev->dev, "err, host state[%d] is not live\n", hdev->state);
		return -EBUSY;
	}

	memset(&pthru_cmd, 0, sizeof(pthru_cmd));
	pthru_cmd.common.opcode = cmd->opcode;
	pthru_cmd.common.flags = cmd->flags;
	pthru_cmd.common.hdid = cpu_to_le32(cmd->nsid);
	pthru_cmd.common.sense_len = cpu_to_le16(cmd->info_0.res_sense_len);
	pthru_cmd.common.cdb_len = cmd->info_0.cdb_len;
	pthru_cmd.common.rsvd2 = cmd->info_0.rsvd0;
	pthru_cmd.common.cdw3[0] = cpu_to_le32(cmd->cdw3);
	pthru_cmd.common.cdw3[1] = cpu_to_le32(cmd->cdw4);
	pthru_cmd.common.cdw3[2] = cpu_to_le32(cmd->cdw5);

	pthru_cmd.common.cdw10[0] = cpu_to_le32(cmd->cdw10);
	pthru_cmd.common.cdw10[1] = cpu_to_le32(cmd->cdw11);
	pthru_cmd.common.cdw10[2] = cpu_to_le32(cmd->cdw12);
	pthru_cmd.common.cdw10[3] = cpu_to_le32(cmd->cdw13);
	pthru_cmd.common.cdw10[4] = cpu_to_le32(cmd->cdw14);
	pthru_cmd.common.cdw10[5] = cpu_to_le32(cmd->data_len);

	memcpy(pthru_cmd.common.cdb, &cmd->cdw16, cmd->info_0.cdb_len);

	pthru_cmd.common.cdw26[0] = cpu_to_le32(cmd->cdw26[0]);
	pthru_cmd.common.cdw26[1] = cpu_to_le32(cmd->cdw26[1]);
	pthru_cmd.common.cdw26[2] = cpu_to_le32(cmd->cdw26[2]);
	pthru_cmd.common.cdw26[3] = cpu_to_le32(cmd->cdw26[3]);

	status = hiraid_bsg_buf_map(hdev, job, (struct hiraid_admin_command *)&pthru_cmd);
	if (status) {
		dev_err(hdev->dev, "err, map data failed\n");
		return status;
	}

	status = hiraid_put_io_sync_request(hdev, &pthru_cmd, job->reply, &job->reply_len, timeout);

	if (status)
		dev_info(hdev->dev, "opcode[0x%x] subopcode[0x%x] status[0x%x] replylen[%d]\n",
			 cmd->opcode, cmd->info_1.subopcode, status, job->reply_len);

	hiraid_bsg_buf_unmap(hdev, job);

	return status;
}

static bool hiraid_check_scmd_finished(struct scsi_cmnd *scmd)
{
	struct hiraid_dev *hdev = shost_priv(scmd->device->host);
	struct hiraid_mapmange *mapbuf = scsi_cmd_priv(scmd);
	struct hiraid_queue *hiraidq;

	hiraidq = mapbuf->hiraidq;
	if (!hiraidq)
		return false;
	if (READ_ONCE(mapbuf->state) == CMD_COMPLETE || hiraid_poll_cq(hiraidq, mapbuf->cid)) {
		dev_warn(hdev->dev, "cid[%d] qid[%d] has been completed\n",
			 mapbuf->cid, hiraidq->qid);
		return true;
	}
	return false;
}

static enum scsi_timeout_action hiraid_timed_out(struct scsi_cmnd *scmd)
{
	struct hiraid_mapmange *mapbuf = scsi_cmd_priv(scmd);
	unsigned int timeout = scmd->device->request_queue->rq_timeout;

	if (hiraid_check_scmd_finished(scmd))
		goto out;

	if (time_after(jiffies, scmd->jiffies_at_alloc + timeout)) {
		if (cmpxchg(&mapbuf->state, CMD_FLIGHT, CMD_TIMEOUT) == CMD_FLIGHT)
			return SCSI_EH_DONE;
	}
out:
	return SCSI_EH_RESET_TIMER;
}

/* send abort command by admin queue temporary */
static int hiraid_send_abort_cmd(struct hiraid_dev *hdev, u32 hdid, u16 qid, u16 cid)
{
	struct hiraid_admin_command admin_cmd;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.abort.opcode = HIRAID_ADMIN_ABORT_CMD;
	admin_cmd.abort.hdid = cpu_to_le32(hdid);
	admin_cmd.abort.sqid = cpu_to_le16(qid);
	admin_cmd.abort.cid = cpu_to_le16(cid);

	return hiraid_put_admin_sync_request(hdev, &admin_cmd, NULL, NULL, 0);
}

/* send reset command by admin quueue temporary */
static int hiraid_send_reset_cmd(struct hiraid_dev *hdev, u8 type, u32 hdid)
{
	struct hiraid_admin_command admin_cmd;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.reset.opcode = HIRAID_ADMIN_RESET;
	admin_cmd.reset.hdid = cpu_to_le32(hdid);
	admin_cmd.reset.type = type;

	return hiraid_put_admin_sync_request(hdev, &admin_cmd, NULL, NULL, 0);
}

static bool hiraid_dev_state_trans(struct hiraid_dev *hdev, enum hiraid_dev_state new_state)
{
	unsigned long flags;
	enum hiraid_dev_state old_state;
	bool change = false;

	spin_lock_irqsave(&hdev->state_lock, flags);

	old_state = hdev->state;
	switch (new_state) {
	case DEV_LIVE:
		switch (old_state) {
		case DEV_NEW:
		case DEV_RESETTING:
			change = true;
			break;
		default:
			break;
		}
		break;
	case DEV_RESETTING:
		switch (old_state) {
		case DEV_LIVE:
			change = true;
			break;
		default:
			break;
		}
		break;
	case DEV_DELETING:
		if (old_state != DEV_DELETING)
			change = true;
		break;
	case DEV_DEAD:
		switch (old_state) {
		case DEV_NEW:
		case DEV_LIVE:
		case DEV_RESETTING:
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
		hdev->state = new_state;
	spin_unlock_irqrestore(&hdev->state_lock, flags);

	dev_info(hdev->dev, "oldstate[%d]->newstate[%d], change[%d]\n",
		old_state, new_state, change);

	return change;
}

static void hiraid_drain_pending_ios(struct hiraid_dev *hdev);

static void hiraid_flush_running_cmds(struct hiraid_dev *hdev)
{
	int i, j;

	scsi_block_requests(hdev->shost);
	hiraid_drain_pending_ios(hdev);
	scsi_unblock_requests(hdev->shost);

	j = HIRAID_AQ_BLK_MQ_DEPTH;
	for (i = 0; i < j; i++) {
		if (READ_ONCE(hdev->adm_cmds[i].state) == CMD_FLIGHT) {
			dev_info(hdev->dev, "flush admin, cid[%d]\n", i);
			hdev->adm_cmds[i].status = 0xFFFF;
			WRITE_ONCE(hdev->adm_cmds[i].state, CMD_COMPLETE);
			complete(&(hdev->adm_cmds[i].cmd_done));
		}
	}

	j = HIRAID_TOTAL_PTCMDS(hdev->online_queues - 1);
	for (i = 0; i < j; i++) {
		if (READ_ONCE(hdev->io_ptcmds[i].state) == CMD_FLIGHT) {
			hdev->io_ptcmds[i].status = 0xFFFF;
			WRITE_ONCE(hdev->io_ptcmds[i].state, CMD_COMPLETE);
			complete(&(hdev->io_ptcmds[i].cmd_done));
		}
	}
}

static int hiraid_dev_disable(struct hiraid_dev *hdev, bool shutdown)
{
	int ret = -ENODEV;
	struct hiraid_queue *adminq = &hdev->queues[0];
	u16 start, end;

	if (pci_device_is_present(hdev->pdev)) {
		if (shutdown)
			hiraid_shutdown_control(hdev);
		else
			ret = hiraid_disable_control(hdev);
	}

	if (hdev->queue_count == 0) {
		dev_err(hdev->dev, "warn: queue has been delete\n");
		return ret;
	}

	spin_lock_irq(&adminq->cq_lock);
	hiraid_process_cq(adminq, &start, &end, -1);
	spin_unlock_irq(&adminq->cq_lock);
	hiraid_complete_cqes(adminq, start, end);

	hiraid_pci_disable(hdev);

	hiraid_flush_running_cmds(hdev);

	return ret;
}

static void hiraid_reset_work(struct work_struct *work)
{
	int ret = 0;
	struct hiraid_dev *hdev = container_of(work, struct hiraid_dev, reset_work);

	if (hdev->state != DEV_RESETTING) {
		dev_err(hdev->dev, "err, host is not reset state\n");
		return;
	}

	dev_info(hdev->dev, "enter host reset\n");

	if (hdev->ctrl_config & HIRAID_CC_ENABLE) {
		dev_info(hdev->dev, "start dev_disable\n");
		ret = hiraid_dev_disable(hdev, false);
	}

	if (ret)
		goto out;

	ret = hiraid_pci_enable(hdev);
	if (ret)
		goto out;

	ret = hiraid_setup_admin_queue(hdev);
	if (ret)
		goto pci_disable;

	ret = hiraid_setup_io_queues(hdev);
	if (ret || hdev->online_queues != hdev->last_qcnt)
		goto pci_disable;

	hiraid_dev_state_trans(hdev, DEV_LIVE);

	hiraid_init_async_event(hdev);

	hiraid_queue_scan(hdev);

	return;

pci_disable:
	hiraid_pci_disable(hdev);
out:
	hiraid_dev_state_trans(hdev, DEV_DEAD);
	dev_err(hdev->dev, "err, host reset failed\n");
}

static int hiraid_reset_work_sync(struct hiraid_dev *hdev)
{
	if (!hiraid_dev_state_trans(hdev, DEV_RESETTING)) {
		dev_info(hdev->dev, "can't change to reset state\n");
		return -EBUSY;
	}

	if (!queue_work(work_queue, &hdev->reset_work)) {
		dev_err(hdev->dev, "err, host is already in reset state\n");
		return -EBUSY;
	}

	flush_work(&hdev->reset_work);
	if (hdev->state != DEV_LIVE)
		return -ENODEV;

	return 0;
}

static int hiraid_wait_io_completion(struct hiraid_mapmange *mapbuf)
{
	u16 times = 0;

	do {
		if (READ_ONCE(mapbuf->state) == CMD_TMO_COMPLETE)
			break;
		msleep(500);
		times++;
	} while (times <= HIRAID_WAIT_ABNL_CMD_TIMEOUT);

	/* wait command completion timeout after abort/reset success */
	if (times >= HIRAID_WAIT_ABNL_CMD_TIMEOUT)
		return -ETIMEDOUT;

	return 0;
}

static bool hiraid_tgt_rst_pending_io_count(struct request *rq, void *data)
{
	unsigned int id = *(unsigned int *)data;
	struct scsi_cmnd *scmd = blk_mq_rq_to_pdu(rq);
	struct hiraid_mapmange *mapbuf;
	struct hiraid_sdev_hostdata *hostdata;

	if (scmd) {
		mapbuf = scsi_cmd_priv(scmd);
		if ((mapbuf->state == CMD_FLIGHT) || (mapbuf->state == CMD_TIMEOUT)) {
			if ((scmd->device) && (scmd->device->id == id)) {
				hostdata = scmd->device->hostdata;
				hostdata->pend_count++;
			}
		}
	}
	return true;
}
static bool hiraid_clean_pending_io(struct request *rq, void *data)
{
	struct hiraid_dev *hdev = data;
	struct scsi_cmnd *scmd;
	struct hiraid_mapmange *mapbuf;

	if (unlikely(!rq || !blk_mq_request_started(rq)))
		return true;

	scmd = blk_mq_rq_to_pdu(rq);
	mapbuf = scsi_cmd_priv(scmd);

	if ((cmpxchg(&mapbuf->state, CMD_FLIGHT, CMD_COMPLETE) != CMD_FLIGHT) &&
	    (cmpxchg(&mapbuf->state, CMD_TIMEOUT, CMD_COMPLETE) != CMD_TIMEOUT))
		return true;

	set_host_byte(scmd, DID_NO_CONNECT);
	if (mapbuf->sge_cnt)
		scsi_dma_unmap(scmd);
	hiraid_free_mapbuf(hdev, mapbuf);
	dev_warn_ratelimited(hdev->dev, "back unfinished CQE, cid[%d] qid[%d]\n",
		 mapbuf->cid, mapbuf->hiraidq->qid);
	scsi_done(scmd);

	return true;
}

static void hiraid_drain_pending_ios(struct hiraid_dev *hdev)
{
	blk_mq_tagset_busy_iter(&hdev->shost->tag_set, hiraid_clean_pending_io, (void *)(hdev));
}

static int wait_tgt_reset_io_done(struct scsi_cmnd *scmd)
{
	u16 timeout = 0;
	struct hiraid_sdev_hostdata *hostdata;
	struct hiraid_dev *hdev = shost_priv(scmd->device->host);

	hostdata = scmd->device->hostdata;

	do {
		hostdata->pend_count = 0;
		blk_mq_tagset_busy_iter(&hdev->shost->tag_set, hiraid_tgt_rst_pending_io_count,
			(void *)(&scmd->device->id));

		if (!hostdata->pend_count)
			return 0;

		msleep(500);
		timeout++;
	} while (timeout <= HIRAID_WAIT_RST_IO_TIMEOUT);

	return -ETIMEDOUT;
}

static int hiraid_abort(struct scsi_cmnd *scmd)
{
	struct hiraid_dev *hdev = shost_priv(scmd->device->host);
	struct hiraid_mapmange *mapbuf = scsi_cmd_priv(scmd);
	struct hiraid_sdev_hostdata *hostdata;
	u16 hwq, cid;
	int ret;

	scsi_print_command(scmd);

	if (hdev->state != DEV_LIVE || !hiraid_wait_io_completion(mapbuf) ||
	    hiraid_check_scmd_finished(scmd))
		return SUCCESS;

	hostdata = scmd->device->hostdata;
	cid = mapbuf->cid;
	hwq = mapbuf->hiraidq->qid;

	dev_warn(hdev->dev, "cid[%d] qid[%d] timeout, send abort\n", cid, hwq);
	ret = hiraid_send_abort_cmd(hdev, hostdata->hdid, hwq, cid);
	if (ret != -ETIME) {
		ret = hiraid_wait_io_completion(mapbuf);
		if (ret) {
			dev_warn(hdev->dev, "cid[%d] qid[%d] abort failed, not found\n", cid, hwq);
			return FAILED;
		}
		dev_warn(hdev->dev, "cid[%d] qid[%d] abort succ\n", cid, hwq);
		return SUCCESS;
	}
	dev_warn(hdev->dev, "cid[%d] qid[%d] abort failed, timeout\n", cid, hwq);
	return FAILED;
}

static int hiraid_scsi_reset(struct scsi_cmnd *scmd, enum hiraid_rst_type rst)
{
	struct hiraid_dev *hdev = shost_priv(scmd->device->host);
	struct hiraid_sdev_hostdata *hostdata;
	int ret;

	if (hdev->state != DEV_LIVE)
		return SUCCESS;

	hostdata = scmd->device->hostdata;

	dev_warn(hdev->dev, "sdev[%d:%d] send %s reset\n", scmd->device->channel, scmd->device->id,
		 rst ? "bus" : "target");
	ret = hiraid_send_reset_cmd(hdev, rst, hostdata->hdid);
	if ((ret == 0) || (ret == FW_EH_DEV_NONE && rst == HIRAID_RESET_TARGET)) {
		if (rst == HIRAID_RESET_TARGET) {
			ret = wait_tgt_reset_io_done(scmd);
			if (ret) {
				dev_warn(hdev->dev, "sdev[%d:%d] target has %d peding cmd, target reset failed\n",
					scmd->device->channel, scmd->device->id,
					hostdata->pend_count);
				return FAILED;
			}
		}
		dev_warn(hdev->dev, "sdev[%d:%d] %s reset success\n",
			scmd->device->channel, scmd->device->id, rst ? "bus" : "target");
		return SUCCESS;
	}

	dev_warn(hdev->dev, "sdev[%d:%d] %s reset failed\n",
		scmd->device->channel, scmd->device->id, rst ? "bus" : "target");
	return FAILED;
}

static int hiraid_target_reset(struct scsi_cmnd *scmd)
{
	return hiraid_scsi_reset(scmd, HIRAID_RESET_TARGET);
}

static int hiraid_bus_reset(struct scsi_cmnd *scmd)
{
	return hiraid_scsi_reset(scmd, HIRAID_RESET_BUS);
}

static int hiraid_host_reset(struct scsi_cmnd *scmd)
{
	struct hiraid_dev *hdev = shost_priv(scmd->device->host);

	if (hdev->state != DEV_LIVE)
		return SUCCESS;

	dev_warn(hdev->dev, "sdev[%d:%d] send host reset\n",
		scmd->device->channel, scmd->device->id);
	if (hiraid_reset_work_sync(hdev) == -EBUSY)
		flush_work(&hdev->reset_work);

	if (hdev->state != DEV_LIVE) {
		dev_warn(hdev->dev, "sdev[%d:%d] host reset failed\n",
			scmd->device->channel, scmd->device->id);
		return FAILED;
	}

	dev_warn(hdev->dev, "sdev[%d:%d] host reset success\n",
		scmd->device->channel, scmd->device->id);

	return SUCCESS;
}

static pci_ers_result_t hiraid_pci_error_detected(struct pci_dev *pdev,
						  pci_channel_state_t state)
{
	struct hiraid_dev *hdev = pci_get_drvdata(pdev);

	dev_info(hdev->dev, "pci error detected, state[%d]\n", state);

	switch (state) {
	case pci_channel_io_normal:
		dev_warn(hdev->dev, "channel is normal, do nothing\n");

		return PCI_ERS_RESULT_CAN_RECOVER;
	case pci_channel_io_frozen:
		dev_warn(hdev->dev, "channel io frozen, need reset controller\n");

		scsi_block_requests(hdev->shost);

		hiraid_dev_state_trans(hdev, DEV_RESETTING);

		return PCI_ERS_RESULT_NEED_RESET;
	case pci_channel_io_perm_failure:
		dev_warn(hdev->dev, "channel io failure, disconnect\n");

		return PCI_ERS_RESULT_DISCONNECT;
	}

	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t hiraid_pci_slot_reset(struct pci_dev *pdev)
{
	struct hiraid_dev *hdev = pci_get_drvdata(pdev);

	dev_info(hdev->dev, "restart after slot reset\n");

	pci_restore_state(pdev);

	if (!queue_work(work_queue, &hdev->reset_work)) {
		dev_err(hdev->dev, "err, the device is resetting state\n");
		return PCI_ERS_RESULT_NONE;
	}

	flush_work(&hdev->reset_work);

	scsi_unblock_requests(hdev->shost);

	return PCI_ERS_RESULT_RECOVERED;
}

static void hiraid_reset_pci_finish(struct pci_dev *pdev)
{
	struct hiraid_dev *hdev = pci_get_drvdata(pdev);

	dev_info(hdev->dev, "enter hiraid reset finish\n");
}

static ssize_t csts_pp_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct hiraid_dev *hdev = shost_priv(shost);
	int ret = -1;

	if (pci_device_is_present(hdev->pdev)) {
		ret = (readl(hdev->bar + HIRAID_REG_CSTS) & HIRAID_CSTS_PP_MASK);
		ret >>= HIRAID_CSTS_PP_SHIFT;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t csts_shst_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct hiraid_dev *hdev = shost_priv(shost);
	int ret = -1;

	if (pci_device_is_present(hdev->pdev)) {
		ret = (readl(hdev->bar + HIRAID_REG_CSTS) & HIRAID_CSTS_SHST_MASK);
		ret >>= HIRAID_CSTS_SHST_SHIFT;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t csts_cfs_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct hiraid_dev *hdev = shost_priv(shost);
	int ret = -1;

	if (pci_device_is_present(hdev->pdev)) {
		ret = (readl(hdev->bar + HIRAID_REG_CSTS) & HIRAID_CSTS_CFS_MASK);
		ret >>= HIRAID_CSTS_CFS_SHIFT;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t csts_rdy_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct hiraid_dev *hdev = shost_priv(shost);
	int ret = -1;

	if (pci_device_is_present(hdev->pdev))
		ret = (readl(hdev->bar + HIRAID_REG_CSTS) & HIRAID_CSTS_RDY);

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t fw_version_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct hiraid_dev *hdev = shost_priv(shost);

	return snprintf(buf, PAGE_SIZE, "%s\n", hdev->ctrl_info->fw_version);
}

static ssize_t hdd_dispatch_store(struct device *cdev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	int val = 0;
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct hiraid_dev *hdev = shost_priv(shost);

	if (kstrtoint(buf, 0, &val) != 0)
		return -EINVAL;
	if (val < DISPATCH_BY_CPU || val > DISPATCH_BY_DISK)
		return -EINVAL;
	hdev->hdd_dispatch = val;

	return strlen(buf);
}
static ssize_t hdd_dispatch_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct hiraid_dev *hdev = shost_priv(shost);

	return snprintf(buf, PAGE_SIZE, "%d\n", hdev->hdd_dispatch);
}

static DEVICE_ATTR_RO(csts_pp);
static DEVICE_ATTR_RO(csts_shst);
static DEVICE_ATTR_RO(csts_cfs);
static DEVICE_ATTR_RO(csts_rdy);
static DEVICE_ATTR_RO(fw_version);
static DEVICE_ATTR_RW(hdd_dispatch);

static struct attribute *hiraid_host_attrs[] = {
	&dev_attr_csts_rdy.attr,
	&dev_attr_csts_pp.attr,
	&dev_attr_csts_cfs.attr,
	&dev_attr_fw_version.attr,
	&dev_attr_csts_shst.attr,
	&dev_attr_hdd_dispatch.attr,
	NULL,
};

static const struct attribute_group hiraid_host_attrs_group = {
	.attrs = hiraid_host_attrs
};

const struct attribute_group *hiraid_host_attrs_groups[] = {
	&hiraid_host_attrs_group,
	NULL
};

static int hiraid_get_vd_info(struct hiraid_dev *hdev, struct hiraid_vd_info *vd_info, u16 vid)
{
	struct hiraid_admin_command admin_cmd;
	u8 *data_ptr = NULL;
	dma_addr_t buffer_phy = 0;
	int ret;

	if (hdev->state >= DEV_RESETTING) {
		dev_err(hdev->dev, "err, host state[%d] is not right\n", hdev->state);
		return -EBUSY;
	}

	data_ptr = dma_alloc_coherent(hdev->dev, PAGE_SIZE, &buffer_phy, GFP_KERNEL);
	if (!data_ptr)
		return -ENOMEM;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.usr_cmd.opcode = USR_CMD_READ;
	admin_cmd.usr_cmd.info_0.subopcode = cpu_to_le16(USR_CMD_VDINFO);
	admin_cmd.usr_cmd.info_1.data_len = cpu_to_le16(USR_CMD_RDLEN);
	admin_cmd.usr_cmd.info_1.param_len = cpu_to_le16(VDINFO_PARAM_LEN);
	admin_cmd.usr_cmd.cdw10 = cpu_to_le32(vid);
	admin_cmd.common.dptr.prp1 = cpu_to_le64(buffer_phy);

	ret = hiraid_put_admin_sync_request(hdev, &admin_cmd, NULL, NULL, USRCMD_TIMEOUT);
	if (!ret)
		memcpy(vd_info, data_ptr, sizeof(struct hiraid_vd_info));

	dma_free_coherent(hdev->dev, PAGE_SIZE, data_ptr, buffer_phy);

	return ret;
}

static int hiraid_get_bgtask(struct hiraid_dev *hdev, struct hiraid_bgtask *bgtask)
{
	struct hiraid_admin_command admin_cmd;
	u8 *data_ptr = NULL;
	dma_addr_t buffer_phy = 0;
	int ret;

	if (hdev->state >= DEV_RESETTING) {
		dev_err(hdev->dev, "err, host state[%d] is not right\n", hdev->state);
		return -EBUSY;
	}

	data_ptr = dma_alloc_coherent(hdev->dev, PAGE_SIZE, &buffer_phy, GFP_KERNEL);
	if (!data_ptr)
		return -ENOMEM;

	memset(&admin_cmd, 0, sizeof(admin_cmd));
	admin_cmd.usr_cmd.opcode = USR_CMD_READ;
	admin_cmd.usr_cmd.info_0.subopcode = cpu_to_le16(USR_CMD_BGTASK);
	admin_cmd.usr_cmd.info_1.data_len = cpu_to_le16(USR_CMD_RDLEN);
	admin_cmd.common.dptr.prp1 = cpu_to_le64(buffer_phy);

	ret = hiraid_put_admin_sync_request(hdev, &admin_cmd, NULL, NULL, USRCMD_TIMEOUT);
	if (!ret)
		memcpy(bgtask, data_ptr, sizeof(struct hiraid_bgtask));

	dma_free_coherent(hdev->dev, PAGE_SIZE, data_ptr, buffer_phy);

	return ret;
}

static ssize_t raid_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct scsi_device *sdev;
	struct hiraid_dev *hdev;
	struct hiraid_vd_info *vd_info;
	struct hiraid_sdev_hostdata *hostdata;
	int ret;

	sdev = to_scsi_device(dev);
	hdev = shost_priv(sdev->host);
	hostdata = sdev->hostdata;

	vd_info = kmalloc(sizeof(*vd_info), GFP_KERNEL);
	if (!vd_info || !HIRAID_DEV_INFO_ATTR_VD(hostdata->attr))
		return snprintf(buf, PAGE_SIZE, "NA\n");

	ret = hiraid_get_vd_info(hdev, vd_info, sdev->id);
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
	struct hiraid_dev *hdev;
	struct hiraid_vd_info *vd_info;
	struct hiraid_sdev_hostdata *hostdata;
	int ret;

	sdev = to_scsi_device(dev);
	hdev = shost_priv(sdev->host);
	hostdata = sdev->hostdata;

	vd_info = kmalloc(sizeof(*vd_info), GFP_KERNEL);
	if (!vd_info || !HIRAID_DEV_INFO_ATTR_VD(hostdata->attr))
		return snprintf(buf, PAGE_SIZE, "NA\n");

	ret = hiraid_get_vd_info(hdev, vd_info, sdev->id);
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
	struct hiraid_dev *hdev;
	struct hiraid_vd_info *vd_info;
	struct hiraid_bgtask *bgtask;
	struct hiraid_sdev_hostdata *hostdata;
	u8 rg_id, i, progress = 0;
	int ret;

	sdev = to_scsi_device(dev);
	hdev = shost_priv(sdev->host);
	hostdata = sdev->hostdata;

	vd_info = kmalloc(sizeof(*vd_info), GFP_KERNEL);
	if (!vd_info || !HIRAID_DEV_INFO_ATTR_VD(hostdata->attr))
		return snprintf(buf, PAGE_SIZE, "NA\n");

	ret = hiraid_get_vd_info(hdev, vd_info, sdev->id);
	if (ret)
		goto out;

	rg_id = vd_info->rg_id;

	bgtask = (struct hiraid_bgtask *)vd_info;
	ret = hiraid_get_bgtask(hdev, bgtask);
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

static ssize_t dispatch_hwq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hiraid_sdev_hostdata *hostdata;

	hostdata = to_scsi_device(dev)->hostdata;
	return snprintf(buf, PAGE_SIZE, "%d\n", hostdata->hwq);
}

static ssize_t dispatch_hwq_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	int val;
	struct hiraid_dev *hdev;
	struct scsi_device *sdev;
	struct hiraid_sdev_hostdata *hostdata;

	sdev = to_scsi_device(dev);
	hdev = shost_priv(sdev->host);
	hostdata = sdev->hostdata;

	if (kstrtoint(buf, 0, &val) != 0)
		return -EINVAL;
	if (val <= 0 || val >= hdev->online_queues)
		return -EINVAL;
	if (!hiraid_disk_is_hdd(hostdata->attr))
		return -EINVAL;

	hostdata->hwq = val;
	return strlen(buf);
}

static DEVICE_ATTR_RO(raid_level);
static DEVICE_ATTR_RO(raid_state);
static DEVICE_ATTR_RO(raid_resync);
static DEVICE_ATTR_RW(dispatch_hwq);

static struct attribute *hiraid_dev_attrs[] = {
	&dev_attr_raid_state.attr,
	&dev_attr_raid_level.attr,
	&dev_attr_raid_resync.attr,
	&dev_attr_dispatch_hwq.attr,
	NULL,
};

static const struct attribute_group hiraid_dev_group = {
	.attrs = hiraid_dev_attrs
};

const struct attribute_group *hiraid_dev_groups[] = {
	&hiraid_dev_group,
	NULL
};

static struct pci_error_handlers hiraid_err_handler = {
	.error_detected = hiraid_pci_error_detected,
	.slot_reset = hiraid_pci_slot_reset,
	.reset_done = hiraid_reset_pci_finish,
};

static int hiraid_sysfs_host_reset(struct Scsi_Host *shost, int reset_type)
{
	int ret;
	struct hiraid_dev *hdev = shost_priv(shost);

	dev_info(hdev->dev, "start sysfs host reset cmd\n");
	ret = hiraid_reset_work_sync(hdev);
	dev_info(hdev->dev, "stop sysfs host reset cmd[%d]\n", ret);

	return ret;
}

static int hiraid_scan_finished(struct Scsi_Host *shost, unsigned long time)
{
	struct hiraid_dev *hdev = shost_priv(shost);

	hiraid_scan_work(&hdev->scan_work);

	return 1;
}

static const struct scsi_host_template hiraid_driver_template = {
	.module			= THIS_MODULE,
	.name			= "hiraid",
	.proc_name		= "hiraid",
	.queuecommand		= hiraid_queue_command,
	.slave_alloc		= hiraid_slave_alloc,
	.slave_destroy		= hiraid_slave_destroy,
	.slave_configure	= hiraid_slave_configure,
	.scan_finished		= hiraid_scan_finished,
	.eh_timed_out		= hiraid_timed_out,
	.eh_abort_handler	= hiraid_abort,
	.eh_target_reset_handler	= hiraid_target_reset,
	.eh_bus_reset_handler		= hiraid_bus_reset,
	.eh_host_reset_handler		= hiraid_host_reset,
	.change_queue_depth		= scsi_change_queue_depth,
	.this_id			= -1,
	.shost_groups			= hiraid_host_attrs_groups,
	.sdev_groups			= hiraid_dev_groups,
	.host_reset			= hiraid_sysfs_host_reset,
	.cmd_size			= sizeof(struct hiraid_mapmange) + HIRAID_ALLOC_SIZE_MAX,
};

static void hiraid_shutdown(struct pci_dev *pdev)
{
	struct hiraid_dev *hdev = pci_get_drvdata(pdev);

	hiraid_delete_io_queues(hdev);
	hiraid_disable_admin_queue(hdev, true);
}

static bool hiraid_bsg_is_valid(struct bsg_job *job)
{
	u64 timeout = 0;
	struct request *rq = blk_mq_rq_from_pdu(job);
	struct hiraid_bsg_request *bsg_req = job->request;
	struct hiraid_dev *hdev = shost_priv(dev_to_shost(job->dev));

	if (bsg_req == NULL || job->request_len != sizeof(struct hiraid_bsg_request))
		return false;

	switch (bsg_req->msgcode) {
	case HIRAID_BSG_ADMIN:
		timeout = msecs_to_jiffies(bsg_req->admcmd.timeout_ms);
		break;
	case HIRAID_BSG_IOPTHRU:
		timeout = msecs_to_jiffies(bsg_req->pthrucmd.timeout_ms);
		break;
	default:
		dev_info(hdev->dev, "bsg unsupport msgcode[%d]\n", bsg_req->msgcode);
		return false;
	}

	if ((timeout + CTL_RST_TIME) > rq->timeout) {
		dev_err(hdev->dev, "bsg invalid time\n");
		return false;
	}

	return true;
}

/* bsg dispatch user command */
static int hiraid_bsg_dispatch(struct bsg_job *job)
{
	struct Scsi_Host *shost = dev_to_shost(job->dev);
	struct hiraid_dev *hdev = shost_priv(shost);
	struct request *rq = blk_mq_rq_from_pdu(job);
	struct hiraid_bsg_request *bsg_req = job->request;
	int ret = -ENOMSG;

	job->reply_len = 0;

	if (!hiraid_bsg_is_valid(job)) {
		bsg_job_done(job, ret, 0);
		return 0;
	}

	dev_log_dbg(hdev->dev, "bsg msgcode[%d] msglen[%d] timeout[%d];"
		"reqnsge[%d], reqlen[%d]\n",
		bsg_req->msgcode, job->request_len, rq->timeout,
		job->request_payload.sg_cnt, job->request_payload.payload_len);

	switch (bsg_req->msgcode) {
	case HIRAID_BSG_ADMIN:
		ret = hiraid_user_send_admcmd(hdev, job);
		break;
	case HIRAID_BSG_IOPTHRU:
		ret = hiraid_user_send_ptcmd(hdev, job);
		break;
	default:
		break;
	}

	if (ret > 0)
		ret = ret | (ret << 8);

	bsg_job_done(job, ret, 0);
	return 0;
}

static inline void hiraid_unregist_bsg(struct hiraid_dev *hdev)
{
	bsg_remove_queue(hdev->bsg_queue);
}
static int hiraid_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct hiraid_dev *hdev;
	struct Scsi_Host *shost;
	int node, ret;
	char bsg_name[15];

	shost = scsi_host_alloc(&hiraid_driver_template, sizeof(*hdev));
	if (!shost) {
		dev_err(&pdev->dev, "failed to allocate scsi host\n");
		return -ENOMEM;
	}
	hdev = shost_priv(shost);
	hdev->pdev = pdev;
	hdev->dev = get_device(&pdev->dev);

	node = dev_to_node(hdev->dev);
	if (node == NUMA_NO_NODE) {
		node = first_memory_node;
		set_dev_node(hdev->dev, node);
	}
	hdev->numa_node = node;
	hdev->shost = shost;
	hdev->instance = shost->host_no;
	pci_set_drvdata(pdev, hdev);

	ret = hiraid_dev_map(hdev);
	if (ret)
		goto put_dev;

	init_rwsem(&hdev->dev_rwsem);
	INIT_WORK(&hdev->scan_work, hiraid_scan_work);
	INIT_WORK(&hdev->timesyn_work, hiraid_timesyn_work);
	INIT_WORK(&hdev->reset_work, hiraid_reset_work);
	INIT_WORK(&hdev->fwact_work, hiraid_fwactive_work);
	spin_lock_init(&hdev->state_lock);

	ret = hiraid_alloc_resources(hdev);
	if (ret)
		goto dev_unmap;

	ret = hiraid_pci_enable(hdev);
	if (ret)
		goto resources_free;

	ret = hiraid_setup_admin_queue(hdev);
	if (ret)
		goto pci_disable;

	ret = hiraid_init_control_info(hdev);
	if (ret)
		goto disable_admin_q;

	ret = hiraid_setup_io_queues(hdev);
	if (ret)
		goto disable_admin_q;

	hiraid_shost_init(hdev);

	ret = scsi_add_host(hdev->shost, hdev->dev);
	if (ret) {
		dev_err(hdev->dev, "add shost to system failed, ret[%d]\n", ret);
		goto remove_io_queues;
	}

	snprintf(bsg_name, sizeof(bsg_name), "hiraid%d", shost->host_no);
	hdev->bsg_queue = bsg_setup_queue(&shost->shost_gendev, bsg_name, hiraid_bsg_dispatch,
					  NULL, hiraid_get_max_cmd_size(hdev));
	if (IS_ERR(hdev->bsg_queue)) {
		dev_err(hdev->dev, "err, setup bsg failed\n");
		hdev->bsg_queue = NULL;
		goto remove_io_queues;
	}

	if (hdev->online_queues == HIRAID_ADMIN_QUEUE_NUM) {
		dev_warn(hdev->dev, "warn: only admin queue can be used\n");
		return 0;
	}

	hdev->state = DEV_LIVE;

	hiraid_init_async_event(hdev);

	ret = hiraid_dev_list_init(hdev);
	if (ret)
		goto unregist_bsg;

	ret = hiraid_configure_timestamp(hdev);
	if (ret)
		dev_warn(hdev->dev, "time synchronization failed\n");

	ret = hiraid_alloc_io_ptcmds(hdev);
	if (ret)
		goto unregist_bsg;

	scsi_scan_host(hdev->shost);

	return 0;

unregist_bsg:
	hiraid_unregist_bsg(hdev);
remove_io_queues:
	hiraid_delete_io_queues(hdev);
disable_admin_q:
	hiraid_free_sense_buffer(hdev);
	hiraid_disable_admin_queue(hdev, false);
pci_disable:
	hiraid_free_all_queues(hdev);
	hiraid_pci_disable(hdev);
resources_free:
	hiraid_free_resources(hdev);
dev_unmap:
	hiraid_dev_unmap(hdev);
put_dev:
	put_device(hdev->dev);
	scsi_host_put(shost);

	return -ENODEV;
}

static void hiraid_remove(struct pci_dev *pdev)
{
	struct hiraid_dev *hdev = pci_get_drvdata(pdev);
	struct Scsi_Host *shost = hdev->shost;

	dev_info(hdev->dev, "enter hiraid remove\n");

	hiraid_dev_state_trans(hdev, DEV_DELETING);
	flush_work(&hdev->reset_work);

	if (!pci_device_is_present(pdev))
		hiraid_flush_running_cmds(hdev);

	hiraid_unregist_bsg(hdev);
	scsi_remove_host(shost);
	hiraid_free_io_ptcmds(hdev);
	kfree(hdev->dev_info);
	hiraid_delete_io_queues(hdev);
	hiraid_free_sense_buffer(hdev);
	hiraid_disable_admin_queue(hdev, false);
	hiraid_free_all_queues(hdev);
	hiraid_pci_disable(hdev);
	hiraid_free_resources(hdev);
	hiraid_dev_unmap(hdev);
	put_device(hdev->dev);
	scsi_host_put(shost);

	dev_info(hdev->dev, "exit hiraid remove\n");
}

static const struct pci_device_id hiraid_hw_card_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_HUAWEI_LOGIC, HIRAID_SERVER_DEVICE_HBA_DID) },
	{ PCI_DEVICE(PCI_VENDOR_ID_HUAWEI_LOGIC, HIRAID_SERVER_DEVICE_HBAS_DID) },
	{ PCI_DEVICE(PCI_VENDOR_ID_HUAWEI_LOGIC, HIRAID_SERVER_DEVICE_RAID_DID) },
	{ PCI_DEVICE(PCI_VENDOR_ID_HUAWEI_LOGIC, HIRAID_SERVER_DEVICE_RAIDS_DID) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, hiraid_hw_card_ids);

static struct pci_driver hiraid_driver = {
	.name		= "hiraid",
	.id_table	= hiraid_hw_card_ids,
	.probe		= hiraid_probe,
	.remove		= hiraid_remove,
	.shutdown	= hiraid_shutdown,
	.err_handler	= &hiraid_err_handler,
};

static int __init hiraid_init(void)
{
	int ret;

	work_queue = alloc_workqueue("hiraid-wq", WQ_UNBOUND | WQ_MEM_RECLAIM | WQ_SYSFS, 0);
	if (!work_queue)
		return -ENOMEM;

	hiraid_class = class_create("hiraid");
	if (IS_ERR(hiraid_class)) {
		ret = PTR_ERR(hiraid_class);
		goto destroy_wq;
	}

	ret = pci_register_driver(&hiraid_driver);
	if (ret < 0)
		goto destroy_class;

	return 0;

destroy_class:
	class_destroy(hiraid_class);
destroy_wq:
	destroy_workqueue(work_queue);

	return ret;
}

static void __exit hiraid_exit(void)
{
	pci_unregister_driver(&hiraid_driver);
	class_destroy(hiraid_class);
	destroy_workqueue(work_queue);
}

MODULE_AUTHOR("Huawei Technologies CO., Ltd");
MODULE_DESCRIPTION("Huawei RAID driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(HIRAID_DRV_VERSION);
module_init(hiraid_init);
module_exit(hiraid_exit);