// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include "main.h"
#include "icrdma_hw.h"
#include <rdma/ib_pma.h>

#define ZXDH_CHECK_AND_RETURN(func_call) do { \
    int ret = (func_call); \
    if (ret) { \
        return ret; \
    } \
} while (0)

u32 dpp_stat_port_RDMA_packet_msg_tx_cnt_get(dpp_pf_info_t *pf_info, u32 index,
					     u32 mode, u64 *p_pkB_cnt,
					     u64 *p_pk_cnt);

LIST_HEAD(zxdh_handlers);
DEFINE_SPINLOCK(zxdh_handler_lock);
DEFINE_MUTEX(zxdh_rdma_stats_ram_lock);
DEFINE_MUTEX(zxdh_cqp_ram_access_lock);

/**
 * wr32 - write 32 bits to hw register
 * @hw: hardware information including registers
 * @reg: register offset
 * @val: value to write to register
 */
inline void wr32(struct zxdh_hw *hw, u32 reg, u32 val)
{
	writel(val, hw->hw_addr + reg);
}

/**
 * rd32 - read a 32 bit hw register
 * @hw: hardware information including registers
 * @reg: register offset
 *
 * Return value of register content
 */
inline u32 rd32(struct zxdh_hw *hw, u32 reg)
{
	return readl(hw->hw_addr + reg);
}

/**
 * rd64 - read a 64 bit hw register
 * @hw: hardware information including registers
 * @reg: register offset
 *
 * Return value of register content
 */
inline u64 rd64(struct zxdh_hw *hw, u32 reg)
{
	return readq(hw->hw_addr + reg);
}

/**
 * zxdh_add_handler - add a handler to the list
 * @hdl: handler to be added to the handler list
 */
void zxdh_add_handler(struct zxdh_handler *hdl)
{
	unsigned long flags;

	spin_lock_irqsave(&zxdh_handler_lock, flags);
	list_add(&hdl->list, &zxdh_handlers);
	spin_unlock_irqrestore(&zxdh_handler_lock, flags);
}

/**
 * zxdh_del_handler - delete a handler from the list
 * @hdl: handler to be deleted from the handler list
 */
void zxdh_del_handler(struct zxdh_handler *hdl)
{
	unsigned long flags;

	if (hdl) {
		spin_lock_irqsave(&zxdh_handler_lock, flags);
		list_del(&hdl->list);
		spin_unlock_irqrestore(&zxdh_handler_lock, flags);
	}
}

/**
 * zxdh_alloc_and_get_cqp_request - get cqp struct
 * @cqp: device cqp ptr
 * @wait: cqp to be used in wait mode
 */
struct zxdh_cqp_request *zxdh_alloc_and_get_cqp_request(struct zxdh_cqp *cqp,
							bool wait)
{
	struct zxdh_cqp_request *cqp_request = NULL;
	unsigned long flags;

	spin_lock_irqsave(&cqp->req_lock, flags);
	if (!list_empty(&cqp->cqp_avail_reqs)) {
		cqp_request = list_entry(cqp->cqp_avail_reqs.next,
					 struct zxdh_cqp_request, list);
		list_del_init(&cqp_request->list);
	}
	spin_unlock_irqrestore(&cqp->req_lock, flags);
	if (!cqp_request) {
		cqp_request = kzalloc(sizeof(*cqp_request), GFP_ATOMIC);
		if (cqp_request) {
			cqp_request->dynamic = true;
			if (wait)
				init_waitqueue_head(&cqp_request->waitq);
		}
	}
	if (!cqp_request) {
		pr_err("[zxdh_rdma] %s[%d]: ERR: CQP Request Fail: No Memory\n", __func__, __LINE__);
		return NULL;
	}

	cqp_request->waiting = wait;
	refcount_set(&cqp_request->refcnt, 1);
	memset(&cqp_request->compl_info, 0, sizeof(cqp_request->compl_info));

	return cqp_request;
}

/**
 * zxdh_get_cqp_request - increase refcount for cqp_request
 * @cqp_request: pointer to cqp_request instance
 */
static inline void zxdh_get_cqp_request(struct zxdh_cqp_request *cqp_request)
{
	refcount_inc(&cqp_request->refcnt);
}

/**
 * zxdh_free_cqp_request - free cqp request
 * @cqp: cqp ptr
 * @cqp_request: to be put back in cqp list
 */
void zxdh_free_cqp_request(struct zxdh_cqp *cqp,
			   struct zxdh_cqp_request *cqp_request)
{
	unsigned long flags;

	if (cqp_request->dynamic) {
		kfree(cqp_request);
	} else {
		cqp_request->request_done = false;
		cqp_request->callback_fcn = NULL;
		cqp_request->waiting = false;

		spin_lock_irqsave(&cqp->req_lock, flags);
		list_add_tail(&cqp_request->list, &cqp->cqp_avail_reqs);
		spin_unlock_irqrestore(&cqp->req_lock, flags);
	}
	wake_up(&cqp->remove_wq);
}

/**
 * zxdh_put_cqp_request - dec ref count and free if 0
 * @cqp: cqp ptr
 * @cqp_request: to be put back in cqp list
 */
void zxdh_put_cqp_request(struct zxdh_cqp *cqp,
			  struct zxdh_cqp_request *cqp_request)
{
	if (refcount_dec_and_test(&cqp_request->refcnt))
		zxdh_free_cqp_request(cqp, cqp_request);
}

/**
 * zxdh_free_pending_cqp_request -free pending cqp request objs
 * @cqp: cqp ptr
 * @cqp_request: to be put back in cqp list
 */
static void zxdh_free_pending_cqp_request(struct zxdh_cqp *cqp,
					  struct zxdh_cqp_request *cqp_request)
{
	if (cqp_request->waiting) {
		cqp_request->compl_info.error = true;
		cqp_request->request_done = true;
		wake_up(&cqp_request->waitq);
	}
	wait_event_timeout(cqp->remove_wq,
			   refcount_read(&cqp_request->refcnt) == 1, 1000);
	zxdh_put_cqp_request(cqp, cqp_request);
}

/**
 * zxdh_cleanup_pending_cqp_op - clean-up cqp with no
 * completions
 * @rf: RDMA PCI function
 */
void zxdh_cleanup_pending_cqp_op(struct zxdh_pci_f *rf)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_cqp *cqp = &rf->cqp;
	struct zxdh_cqp_request *cqp_request = NULL;
	struct cqp_cmds_info *pcmdinfo = NULL;
	u32 i, pending_work, wqe_idx;

	pending_work = ZXDH_RING_USED_QUANTA(cqp->sc_cqp.sq_ring);
	wqe_idx = ZXDH_RING_CURRENT_TAIL(cqp->sc_cqp.sq_ring);
	for (i = 0; i < pending_work; i++) {
		cqp_request = (struct zxdh_cqp_request *)(unsigned long)
				      cqp->scratch_array[wqe_idx];
		if (cqp_request)
			zxdh_free_pending_cqp_request(cqp, cqp_request);
		wqe_idx = (wqe_idx + 1) % ZXDH_RING_SIZE(cqp->sc_cqp.sq_ring);
	}

	while (!list_empty(&dev->cqp_cmd_head)) {
		pcmdinfo = zxdh_remove_cqp_head(dev);
		cqp_request =
			container_of(pcmdinfo, struct zxdh_cqp_request, info);
		if (cqp_request)
			zxdh_free_pending_cqp_request(cqp, cqp_request);
	}
}

/**
 * zxdh_wait_event - wait for completion
 * @rf: RDMA PCI function
 * @cqp_request: cqp request to wait
 */
static int zxdh_wait_event(struct zxdh_pci_f *rf,
			   struct zxdh_cqp_request *cqp_request)
{
	struct zxdh_cqp_timeout cqp_timeout = {};
	bool cqp_error = false;
	int err_code = 0;
	u32 cnt = 0;
	u32 cqp_err_cnt = 0;
	bool cqp_err = false;
	bool cnt_flag = false;
	u32 compare_count = 0;
	u32 cqp_status, cqp_done;

	cqp_timeout.compl_cqp_cmds =
		rf->sc_dev.cqp_cmd_stats[ZXDH_OP_CMPL_CMDS];
	compare_count = (rf->sc_dev.hw_attrs.cqp_timeout_threshold / 2);

	do {
		int wait_time_ms =
			rf->sc_dev.hw_attrs.max_cqp_compl_wait_time_ms;

		zxdh_cqp_ce_handler(rf, &rf->ccq.sc_cq);
		if (wait_event_timeout(cqp_request->waitq,
				       cqp_request->request_done,
				       msecs_to_jiffies(wait_time_ms)))
			break;

		zxdh_check_cqp_progress(&cqp_timeout, &rf->sc_dev);
		if (cqp_timeout.count == compare_count)
			cnt_flag = true;

		if (cnt_flag == true) {
			cnt++;
			if (cnt == CQP_STATUS_CHECK_CNT) {
				cnt = 0;
				cqp_status = readl((u32 __iomem *)(rf->sc_dev.hw->hw_addr +
					    C_RDMA_CQP_STATUS(rf->sc_dev.ep_id)));
				cqp_done = readl((u32 __iomem *)(rf->sc_dev.hw->hw_addr +
					    C_RDMA_CQP_CONFIG_DONE(rf->sc_dev.ep_id)));
				if ((cqp_status != CQP_STATUS_TRUE) && (cqp_done != CQP_DONE_TRUE)) {
					cnt = 0;
					cqp_err_cnt++;
					if (cqp_err_cnt == CQP_ERR_CNT_VALUE) {
						cqp_err = true;
						cnt_flag = false;
					}
				} else
					cqp_err_cnt = 0;
			}
		}

		if (rf->sc_dev.hw_attrs.skip_hw == true)
			return 0;

		if (cqp_timeout.count < rf->sc_dev.hw_attrs.cqp_timeout_threshold)
			continue;

		if (!rf->reset) {
			// rf->reset = true;
			rf->gen_ops.request_reset(rf);
		}
		if (cqp_err == true) {
			pr_err("[zxdh_rdma][%s][%d] vhca_id:%u skip cqp operation, skip_hw become true\n",
			       __func__, __LINE__, rf->sc_dev.vhca_id);
			rf->sc_dev.hw_attrs.skip_hw = true;
			rf->sc_dev.hw_attrs.cqp_timeout_threshold = CQP_MIN_TIMEOUT_THRESHOLD;
			rf->sc_dev.hw_attrs.max_done_count = ZXDH_MIN_DONE_COUNT;
		}
		return -ETIMEDOUT;
	} while (1);

	cqp_error = cqp_request->compl_info.error;
	if (cqp_error) {
		err_code = -EIO;
		if (cqp_request->compl_info.maj_err_code == 0xFFFF) {
			if (cqp_request->compl_info.min_err_code == 0x8002) {
				err_code = -EBUSY;
			} else if (cqp_request->compl_info.min_err_code ==
				   0x8029) {
				if (!rf->reset) {
					// rf->reset = true;
					//rf->gen_ops.request_reset(rf);
				}
			}
		}
	}

	return err_code;
}

static const char *const zxdh_cqp_cmd_names[ZXDH_MAX_CQP_OPS] = {
	[ZXDH_OP_CEQ_DESTROY] = "Destroy CEQ Cmd",
	[ZXDH_OP_AEQ_DESTROY] = "Destroy AEQ Cmd",
	[ZXDH_OP_DELETE_ARP_CACHE_ENTRY] = "Delete ARP Cache Cmd",
	[ZXDH_OP_MANAGE_APBVT_ENTRY] = "Manage APBV Table Entry Cmd",
	[ZXDH_OP_CEQ_CREATE] = "CEQ Create Cmd",
	[ZXDH_OP_AEQ_CREATE] = "AEQ Destroy Cmd",
	[ZXDH_OP_MANAGE_QHASH_TABLE_ENTRY] = "Manage Quad Hash Table Entry Cmd",
	[ZXDH_OP_QP_MODIFY] = "Modify QP Cmd",
	[ZXDH_OP_QP_UPLOAD_CONTEXT] = "Upload Context Cmd",
	[ZXDH_OP_CQ_CREATE] = "Create CQ Cmd",
	[ZXDH_OP_CQ_DESTROY] = "Destroy CQ Cmd",
	[ZXDH_OP_QP_CREATE] = "Create QP Cmd",
	[ZXDH_OP_QP_DESTROY] = "Destroy QP Cmd",
	[ZXDH_OP_ALLOC_STAG] = "Allocate STag Cmd",
	[ZXDH_OP_MR_REG_NON_SHARED] = "Register Non-Shared MR Cmd",
	[ZXDH_OP_DEALLOC_STAG] = "Deallocate STag Cmd",
	[ZXDH_OP_MW_ALLOC] = "Allocate Memory Window Cmd",
	[ZXDH_OP_QP_FLUSH_WQES] = "Flush QP Cmd",
	[ZXDH_OP_ADD_ARP_CACHE_ENTRY] = "Add ARP Cache Cmd",
	[ZXDH_OP_MANAGE_PUSH_PAGE] = "Manage Push Page Cmd",
	[ZXDH_OP_MANAGE_HMC_PM_FUNC_TABLE] = "Manage HMC PM Function Table Cmd",
	[ZXDH_OP_SUSPEND] = "Suspend QP Cmd",
	[ZXDH_OP_RESUME] = "Resume QP Cmd",
	[ZXDH_OP_MANAGE_VF_PBLE_BP] = "Manage VF PBLE Backing Pages Cmd",
	[ZXDH_OP_QUERY_FPM_VAL] = "Query FPM Values Cmd",
	[ZXDH_OP_COMMIT_FPM_VAL] = "Commit FPM Values Cmd",
	[ZXDH_OP_AH_CREATE] = "Create Address Handle Cmd",
	[ZXDH_OP_AH_MODIFY] = "Modify Address Handle Cmd",
	[ZXDH_OP_AH_DESTROY] = "Destroy Address Handle Cmd",
	[ZXDH_OP_MC_CREATE] = "Create Multicast Group Cmd",
	[ZXDH_OP_MC_DESTROY] = "Destroy Multicast Group Cmd",
	[ZXDH_OP_MC_MODIFY] = "Modify Multicast Group Cmd",
	[ZXDH_OP_STATS_ALLOCATE] = "Add Statistics Instance Cmd",
	[ZXDH_OP_STATS_FREE] = "Free Statistics Instance Cmd",
	[ZXDH_OP_STATS_GATHER] = "Gather Statistics Cmd",
	[ZXDH_OP_WS_ADD_NODE] = "Add Work Scheduler Node Cmd",
	[ZXDH_OP_WS_MODIFY_NODE] = "Modify Work Scheduler Node Cmd",
	[ZXDH_OP_WS_DELETE_NODE] = "Delete Work Scheduler Node Cmd",
	[ZXDH_OP_SET_UP_MAP] = "Set UP-UP Mapping Cmd",
	[ZXDH_OP_GEN_AE] = "Generate AE Cmd",
	[ZXDH_OP_QUERY_RDMA_FEATURES] = "RDMA Get Features Cmd",
	[ZXDH_OP_ADD_LOCAL_MAC_ENTRY] = "Add Local MAC Entry Cmd",
	[ZXDH_OP_DELETE_LOCAL_MAC_ENTRY] = "Delete Local MAC Entry Cmd",
	[ZXDH_OP_CQ_MODIFY] = "CQ Modify Cmd",
	[ZXDH_OP_CONFIG_PTE_TAB] = "Config PTE Tab Cmd",
	[ZXDH_OP_QUERY_PTE_TAB] = "Query PTE Tab Cmd",
	[ZXDH_OP_CONFIG_PBLE_TAB] = "Config PBLE Tab Cmd",
	[ZXDH_OP_CONFIG_MAILBOX] = "Config Mailbox Cmd",
	[ZXDH_OP_DMA_WRITE] = "Dma Write Cmd",
	[ZXDH_OP_DMA_WRITE32] = "Dma Write32 Cmd",
	[ZXDH_OP_DMA_WRITE64] = "Dma Write64 Cmd",
	[ZXDH_OP_DMA_READ] = "Dma Read Cmd",
	[ZXDH_OP_DMA_READ_USE_CQE] = "Dma Read Use Cqe Cmd",
	[ZXDH_OP_QUERY_QPC] = "Query HW QPC Cmd",
	[ZXDH_OP_QUERY_CQC] = "Query HW CQC Cmd",
	[ZXDH_OP_QUERY_SRQC] = "Query HW SRQC Cmd",
	[ZXDH_OP_QUERY_CEQC] = "Query HW CEQC Cmd",
	[ZXDH_OP_QUERY_AEQC] = "Query HW AEQC Cmd",
	[ZXDH_OP_QUERY_HW_OBJECT_INFO] = "Query HW object data",
	[ZXDH_OP_CQ_MODIFY_OVERFLOW_CHECK_EN] = "Modify CQ Overflow Check En Cmd",
};

static const struct zxdh_cqp_err_info zxdh_noncrit_err_list[] = {
	{ 0xffff, 0x8002, "Invalid State" },
	{ 0xffff, 0x8006, "Flush No Wqe Pending" },
	{ 0xffff, 0x8007, "Modify QP Bad Close" },
	{ 0xffff, 0x8009, "LLP Closed" },
	{ 0xffff, 0x800a, "Reset Not Sent" }
};

/**
 * zxdh_cqp_crit_err - check if CQP error is critical
 * @dev: pointer to dev structure
 * @cqp_cmd: code for last CQP operation
 * @maj_err_code: major error code
 * @min_err_code: minot error code
 */
bool zxdh_cqp_crit_err(struct zxdh_sc_dev *dev, u8 cqp_cmd, u16 maj_err_code,
		       u16 min_err_code)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(zxdh_noncrit_err_list); ++i) {
		if (maj_err_code == zxdh_noncrit_err_list[i].maj &&
		    min_err_code == zxdh_noncrit_err_list[i].min) {
			pr_err("[zxdh_rdma] CQP: [%s Error][%s] maj=0x%x min=0x%x\n",
			       zxdh_noncrit_err_list[i].desc,
			       zxdh_cqp_cmd_names[cqp_cmd], maj_err_code,
			       min_err_code);
			return false;
		}
	}
	return true;
}

int zxdh_check_cqp_cmd(struct cqp_cmds_info *info)
{
	int status = 0;
	switch (info->cqp_cmd) {
		case ZXDH_OP_CEQ_CREATE:
		case ZXDH_OP_AEQ_CREATE:
		case ZXDH_OP_QP_UPLOAD_CONTEXT:
		case ZXDH_OP_CQ_CREATE:
		case ZXDH_OP_CQ_MODIFY:
		case ZXDH_OP_CQ_MODIFY_MODERATION:
		case ZXDH_OP_CQ_MODIFY_OVERFLOW_CHECK_EN:
		case ZXDH_OP_MANAGE_HMC_PM_FUNC_TABLE:
		case ZXDH_OP_MANAGE_VF_PBLE_BP:
		case ZXDH_OP_QUERY_RDMA_FEATURES:
		case ZXDH_OP_QP_MODIFY:
		case ZXDH_OP_QP_CREATE:
		case ZXDH_OP_ALLOC_STAG:
		case ZXDH_OP_MR_REG_NON_SHARED:
		case ZXDH_OP_MW_ALLOC:
		case ZXDH_OP_ADD_ARP_CACHE_ENTRY:
		// case ZXDH_OP_AH_CREATE:
		case ZXDH_OP_CONFIG_PTE_TAB:
		case ZXDH_OP_CONFIG_PBLE_TAB:
		case ZXDH_OP_DMA_WRITE:
		case ZXDH_OP_QUERY_PTE_TAB:
		case ZXDH_OP_QUERY_HW_OBJECT_INFO:
		case ZXDH_OP_DMA_READ:
		case ZXDH_OP_CONFIG_MAILBOX:
		case ZXDH_OP_DMA_READ_USE_CQE:
		case ZXDH_OP_DMA_WRITE32:
		case ZXDH_OP_DMA_WRITE64:
		case ZXDH_OP_QUERY_QPC:
		case ZXDH_OP_QUERY_CQC:
		case ZXDH_OP_QUERY_CEQC:
		case ZXDH_OP_QUERY_AEQC:
		case ZXDH_OP_QUERY_SRQC:
		case ZXDH_OP_SRQ_MODIFY:
		case ZXDH_OP_SRQ_CREATE:
			status = -EBUSY;
		break;
		default:
			status = 0;
		break;
	}
	return status;
}

/**
 * zxdh_add_pending_cqp_request - enqueue cqp_request onto cqp_pending_reqs under req_lock
 * @rf: RDMA PCI function
 * @cqp_request: request to enqueue
 */
static inline void zxdh_add_pending_cqp_request(struct zxdh_pci_f *rf,
						struct zxdh_cqp_request *cqp_request)
{
	unsigned long flags;

	spin_lock_irqsave(&rf->cqp.req_lock, flags);
	list_add_tail(&cqp_request->list, &rf->cqp.cqp_pending_reqs);
	spin_unlock_irqrestore(&rf->cqp.req_lock, flags);
}

/**
 * zxdh_del_pending_cqp_request - dequeue cqp_request from cqp_pending_reqs under req_lock
 * @rf: RDMA PCI function
 * @cqp_request: request to dequeue
 */
static inline void zxdh_del_pending_cqp_request(struct zxdh_pci_f *rf,
						struct zxdh_cqp_request *cqp_request)
{
	unsigned long flags;

	spin_lock_irqsave(&rf->cqp.req_lock, flags);
	list_del_init(&cqp_request->list);
	spin_unlock_irqrestore(&rf->cqp.req_lock, flags);
}

/**
 * zxdh_handle_cqp_op - process cqp command
 * @rf: RDMA PCI function
 * @cqp_request: cqp request to process
 */
int zxdh_handle_cqp_op(struct zxdh_pci_f *rf,
		       struct zxdh_cqp_request *cqp_request)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_sc_dev *ext_dev = &rf->ext_sc_dev;
	struct cqp_cmds_info *info = &cqp_request->info;
	int status;
	bool put_cqp_request = true;

	if (rf->reset) {
		pr_err("[zxdh_rdma] %s[%d]: device is resetting\n", __func__, __LINE__);
		return -EBUSY;
	}

	zxdh_get_cqp_request(cqp_request);
	if (rf->sc_dev.hw_attrs.skip_hw == true) {
		status = zxdh_check_cqp_cmd(info);
		zxdh_put_cqp_request(&rf->cqp, cqp_request);
		return status;
	}
		
	status = zxdh_process_cqp_cmd(dev, info);
	if (status) {
		pr_err("[zxdh_rdma][%s][%d]: cqp command process failed, status=%d\n", __func__, __LINE__, status);
		goto err;
	}
	if (cqp_request->waiting) {
		put_cqp_request = false;
		zxdh_add_pending_cqp_request(rf, cqp_request);
		status = zxdh_wait_event(rf, cqp_request);
		zxdh_del_pending_cqp_request(rf, cqp_request);

		if (status) {
			pr_err("[zxdh_rdma][%s][%d]: vhca_id:%u wait_event failed, status:%d\n", __func__, __LINE__, rf->sc_dev.vhca_id, status);
			goto err;
		}
		if(rf->use_blue_flame_flag && (!rf->ftype)){
			flush_cache0_all_space(ext_dev);
		}
	}

	return 0;

err:
	if (zxdh_cqp_crit_err(dev, info->cqp_cmd,
			      cqp_request->compl_info.maj_err_code,
			      cqp_request->compl_info.min_err_code))
		if (dev->hw_attrs.skip_hw == false)
			dev_err(idev_to_dev(dev),
				"[%s Error][op_code=%d] status=%d waiting=%d completion_err=%d maj=0x%x min=0x%x\n",
				zxdh_cqp_cmd_names[info->cqp_cmd], info->cqp_cmd,
				status, cqp_request->waiting,
				cqp_request->compl_info.error,
				cqp_request->compl_info.maj_err_code,
				cqp_request->compl_info.min_err_code);

	if (put_cqp_request)
		zxdh_put_cqp_request(&rf->cqp, cqp_request);

	return status;
}

void zxdh_qp_add_ref(struct ib_qp *ibqp)
{
	struct zxdh_qp *iwqp = to_iwqp(ibqp);

	refcount_inc(&iwqp->refcnt);
}

void zxdh_qp_rem_ref(struct ib_qp *ibqp)
{
	struct zxdh_qp *iwqp = to_iwqp(ibqp);
	struct zxdh_device *iwdev = iwqp->iwdev;
	unsigned long flags;

	spin_lock_irqsave(&iwdev->rf->qptable_lock, flags);
	if (!refcount_dec_and_test(&iwqp->refcnt)) {
		spin_unlock_irqrestore(&iwdev->rf->qptable_lock, flags);
		return;
	}

	iwdev->rf->qp_table[iwqp->sc_qp.qp_ctx_num - iwqp->sc_qp.dev->base_qpn] =
		NULL;
	spin_unlock_irqrestore(&iwdev->rf->qptable_lock, flags);
	complete(&iwqp->free_qp);
}

void zxdh_cq_add_ref(struct ib_cq *ibcq)
{
	struct zxdh_cq *iwcq = to_iwcq(ibcq);

	refcount_inc(&iwcq->refcnt);
}

void zxdh_cq_rem_ref(struct ib_cq *ibcq)
{
	struct zxdh_cq *iwcq = to_iwcq(ibcq);
	struct zxdh_pci_f *rf =
		container_of(iwcq->sc_cq.dev, struct zxdh_pci_f, sc_dev);
	unsigned long flags;

	spin_lock_irqsave(&rf->cqtable_lock, flags);
	if (!refcount_dec_and_test(&iwcq->refcnt)) {
		spin_unlock_irqrestore(&rf->cqtable_lock, flags);
		return;
	}

	rf->cq_table[iwcq->cq_num - rf->sc_dev.base_cqn] = NULL;
	spin_unlock_irqrestore(&rf->cqtable_lock, flags);
	complete(&iwcq->free_cq);
}

struct ib_device *zxdh_get_ibdev(struct zxdh_sc_dev *dev)
{
	return &(container_of(dev, struct zxdh_pci_f, sc_dev))->iwdev->ibdev;
}

/**
 * zxdh_remove_cqp_head - return head entry and remove
 * @dev: device
 */
void *zxdh_remove_cqp_head(struct zxdh_sc_dev *dev)
{
	struct list_head *entry;
	struct list_head *list = &dev->cqp_cmd_head;

	if (list_empty(list))
		return NULL;

	entry = list->next;
	list_del(entry);

	return entry;
}

/**
 * zxdh_terminate_del_timer - delete terminate timeout
 * @qp: hardware control qp
 */
void zxdh_terminate_del_timer(struct zxdh_sc_qp *qp)
{
	struct zxdh_qp *iwqp;
	int ret;

	iwqp = qp->qp_uk.back_qp;
	ret = del_timer(&iwqp->terminate_timer);
	if (ret)
		zxdh_qp_rem_ref(&iwqp->ibqp);
}

/**
 * zxdh_cq_wq_destroy - send cq destroy cqp
 * @rf: RDMA PCI function
 * @cq: hardware control cq
 */
void zxdh_cq_wq_destroy(struct zxdh_pci_f *rf, struct zxdh_sc_cq *cq)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return;

	cqp_info = &cqp_request->info;
	cqp_info->cqp_cmd = ZXDH_OP_CQ_DESTROY;
	cqp_info->post_sq = 1;
	cqp_info->in.u.cq_destroy.cq = cq;
	cqp_info->in.u.cq_destroy.scratch = (uintptr_t)cqp_request;

	zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
}

/**
 * zxdh_hw_modify_qp - setup cqp for modify qp
 * @iwdev: RDMA device
 * @iwqp: qp ptr (user or kernel)
 * @info: info for modify qp
 * @wait: flag to wait or not for modify qp completion
 */
int zxdh_hw_modify_qp(struct zxdh_device *iwdev, struct zxdh_qp *iwqp,
		      struct zxdh_modify_qp_info *info, bool wait)
{
	int status;
	struct zxdh_pci_f *rf = iwdev->rf;
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_modify_qp_info *m_info;

	wait = true;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, wait);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	m_info = &cqp_info->in.u.qp_modify.info;
	memcpy(m_info, info, sizeof(*m_info));
	cqp_info->cqp_cmd = ZXDH_OP_QP_MODIFY;
	cqp_info->post_sq = 1;
	cqp_info->in.u.qp_modify.qp = &iwqp->sc_qp;
	cqp_info->in.u.qp_modify.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);

	return status;
}

/**
 * zxdh_cqp_qp_destroy_cmd - destroy the cqp
 * @dev: device pointer
 * @qp: pointer to qp
 */
int zxdh_cqp_qp_destroy_cmd(struct zxdh_sc_dev *dev, struct zxdh_sc_qp *qp)
{
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	struct zxdh_cqp *iwcqp = &rf->cqp;
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(iwcqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	memset(cqp_info, 0, sizeof(*cqp_info));
	cqp_info->cqp_cmd = ZXDH_OP_QP_DESTROY;
	cqp_info->post_sq = 1;
	cqp_info->in.u.qp_destroy.qp = qp;
	cqp_info->in.u.qp_destroy.scratch = (uintptr_t)cqp_request;

	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);

	return status;
}

static void zxdh_set_rx_ram_reg(struct zxdh_sc_dev *dev, u32 ram_num,
				u32 ram_width, u32 ram_addr, u32 ram_read_cnt)
{
	writel(ram_num, (u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
	writel(ram_width, (u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_WIDTH(dev->ep_id)));
	writel(ram_addr, (u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
	writel(ram_read_cnt,
	       (u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_READ_LENGTH(dev->ep_id)));
	writel(0, (u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_READ_FLAG(dev->ep_id)));
}

static int zxdh_read_rx_ram_flag(struct zxdh_sc_dev *dev)
{
	u32 val;

	udelay(1000); //to be modified smaller
	val = readl((u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_READ_FLAG(dev->ep_id)));
	if (val != 1) {
		udelay(2000); //to be modified smaller
		val = readl((u32 __iomem *)(dev->hw->hw_addr +
					    RDMARX_RAM_READ_FLAG(dev->ep_id)));
		if (val != 1)
			return -EIO;
	}
	val = readl((u32 __iomem *)(dev->hw->hw_addr + RDMARX_READ_ERROR_FLAG(dev->ep_id)));
	val |= readl((u32 __iomem *)(dev->hw->hw_addr + RDMARX_READ_CNT_ERROR(dev->ep_id)));
	val |= readl((u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_REDUN_FLAG(dev->ep_id)));
	val |= readl(
		(u32 __iomem *)(dev->hw->hw_addr + RDMARX_DOUBLE_VLD_FLAG(dev->ep_id)));
	if (val != 0)
		return -EIO;

	return val;
}

static u32 zxdh_read_rx_ram_data(struct zxdh_sc_dev *dev, u32 offset_idx)
{
	u32 val;

	val = readl((u32 __iomem *)(dev->hw->hw_addr +
				    RDMARX_RAM_MAINTENANCE_RAM(offset_idx, dev->ep_id)));
	return val;
}

static void zxdh_set_tx_ram_reg(struct zxdh_sc_dev *dev, u32 ram_num,
				u32 ram_width, u32 ram_addr, u32 ram_read_cnt)
{
	writel(ram_num, (u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_NUM(dev->ep_id)));
	writel(ram_width, (u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_WIDTH(dev->ep_id)));
	writel(ram_addr, (u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_ADDR(dev->ep_id)));
	writel(ram_read_cnt,
	       (u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_READ_LENGTH(dev->ep_id)));
	writel(0, (u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_READ_FLAG(dev->ep_id)));
}

static int zxdh_read_tx_ram_flag(struct zxdh_sc_dev *dev)
{
	u32 val;

	udelay(1000); //to be modified smaller
	val = readl((u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_READ_FLAG(dev->ep_id)));
	if (val != 1) {
		udelay(2000); //to be modified smaller
		val = readl((u32 __iomem *)(dev->hw->hw_addr +
					    RDMATX_RAM_READ_FLAG(dev->ep_id)));
		if (val != 1)
			return -EIO;
	}
	val = readl((u32 __iomem *)(dev->hw->hw_addr + RDMATX_READ_ERROR_FLAG(dev->ep_id)));
	val |= readl((u32 __iomem *)(dev->hw->hw_addr + RDMATX_READ_CNT_ERROR(dev->ep_id)));
	val |= readl((u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_REDUN_FLAG(dev->ep_id)));
	val |= readl(
		(u32 __iomem *)(dev->hw->hw_addr + RDMATX_DOUBLE_VLD_FLAG(dev->ep_id)));
	if (val != 0)
		return -EIO;
	return val;
}

static u32 zxdh_read_tx_ram_data(struct zxdh_sc_dev *dev, u32 offset_idx)
{
	u32 val;

	val = readl((u32 __iomem *)(dev->hw->hw_addr +
				    RDMATX_RAM_MAINTENANCE_RAM(offset_idx, dev->ep_id)));
	return val;
}

static u32 zxdh_get_vhca_ram(u32 vhca_id)
{
	u32 ram_num;

	if (vhca_id <= 255)
		ram_num = ZXDH_RAM_H12;
	else if (vhca_id <= 511)
		ram_num = ZXDH_RAM_H13;
	else if (vhca_id <= 767)
		ram_num = ZXDH_RAM_H14;
	else
		ram_num = ZXDH_RAM_H15;
	return ram_num;
}

static u32 zxdh_get_vhca_ram_addr(u32 vhca_id)
{
	u32 ram_addr = 0;

	if (vhca_id <= 255)
		ram_addr = vhca_id;
	else if (vhca_id <= 511)
		ram_addr = (vhca_id - 256);
	else if (vhca_id <= 767)
		ram_addr = (vhca_id - 512);
	else
		ram_addr = (vhca_id - 768);
	return ram_addr;
}
static int zxdh_get_ram_msg_h11(struct zxdh_sc_dev *dev,
				struct zxdh_rdma_stats_get *rdma_stats)
{
	u64 val = 0;
	u32 check_ram_num, check_ram_addr;
	int ret;
	int i;
	u32 rtt_cfg;

	rtt_cfg = readl((u32 __iomem *)(dev->hw->hw_addr + RDMATX_RTT_CFG(dev->ep_id)));
	/* if rtt enabled, rp_cnp_handled not count */
	if (rtt_cfg != 0)
		return 0;

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_rx_ram_reg(dev, ZXDH_RAM_H11, ZXDH_RAM_WIDTH_64_BIT,
				    dev->vhca_id, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_rx_ram_flag(dev);
		if (ret)
			continue;

		/* Read RAM data first before checking */
		val = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_1);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ZXDH_RAM_H11) ||
		    (check_ram_addr != dev->vhca_id))
			continue;

		rdma_stats->rdma_stats_entry[HW_STAT_RP_CNP_HANDLED] = val;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_RP_CNP_HANDLED] =
			ZXDH_HW_STATS_VALID;
		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}
static void zxdh_get_ram_for_rx_stats(struct zxdh_sc_dev *dev, u64 *p_pkB_cnt,
				      u64 *p_pk_cnt)
{
	u64 temp_val = 0;
	u64 stat_val1, stat_val2;

	// ipv6 unicast
	stat_val1 = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_2);
	stat_val1 = (stat_val1 << IRMDA_BIT_WIDTH_16);
	temp_val = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_1);
	stat_val1 |=
		((temp_val & ZXDH_32_BIT_MASK_16_31) >> IRMDA_BIT_WIDTH_16);
	// ipv4 unicast
	stat_val2 = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_1);
	stat_val2 = ((stat_val2 & ZXDH_32_BIT_MASK_0_15) << IRMDA_BIT_WIDTH_32);
	temp_val = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_0);
	stat_val2 |= temp_val;
	*p_pk_cnt = stat_val1 + stat_val2;

	// ipv6
	stat_val1 = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_12);
	temp_val = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_13);
	stat_val1 |= ((temp_val & ZXDH_32_BIT_MASK_0_15) << IRMDA_BIT_WIDTH_32);
	// ipv4
	stat_val2 = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_9);
	temp_val = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_10);
	stat_val2 |= ((temp_val & ZXDH_32_BIT_MASK_0_15) << IRMDA_BIT_WIDTH_32);
	*p_pkB_cnt = stat_val1 + stat_val2;
}
static int zxdh_get_rx_stat(struct zxdh_sc_dev *dev, u64 *p_pkB_cnt,
			    u64 *p_pk_cnt)
{
	u64 pkB_cnt, pk_cnt;
	u32 check_ram_num, check_ram_addr;
	u32 ram_num;
	u32 ram_addr;
	int ret;
	int i;

	ram_num = zxdh_get_vhca_ram(dev->vhca_id);
	ram_addr = zxdh_get_vhca_ram_addr(dev->vhca_id);

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_rx_ram_reg(dev, ram_num, ZXDH_RAM_WIDTH_480_BIT,
				    ram_addr, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_rx_ram_flag(dev);
		if (ret)
			continue;

		/* Read RAM data first before checking */
		zxdh_get_ram_for_rx_stats(dev, &pkB_cnt, &pk_cnt);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ram_num) ||
		    (check_ram_addr != ram_addr))
			continue;

		*p_pkB_cnt = pkB_cnt;
		*p_pk_cnt = pk_cnt;
		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}

static int zxdh_get_ram_msg_h12_to_h15(struct zxdh_sc_dev *dev,
				       struct zxdh_rdma_stats_get *rdma_stats)
{
	u64 pkB_cnt, pkts_cnt;
	u64 val = 0;
	u64 temp_val = 0;
	u32 check_ram_num, check_ram_addr;
	u32 ram_num;
	u32 ram_addr;
	int ret;
	int i;

	ram_num = zxdh_get_vhca_ram(dev->vhca_id);
	ram_addr = zxdh_get_vhca_ram_addr(dev->vhca_id);

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_rx_ram_reg(dev, ram_num, ZXDH_RAM_WIDTH_480_BIT,
				    ram_addr, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_rx_ram_flag(dev);
		if (ret)
			continue;

		/* Read all RAM data first before checking */
		zxdh_get_ram_for_rx_stats(dev, &pkB_cnt, &pkts_cnt);
		val = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_3);
		temp_val = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_4);
		val |= ((temp_val & ZXDH_32_BIT_MASK_0_15)
			<< IRMDA_BIT_WIDTH_32);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ram_num) ||
		    (check_ram_addr != ram_addr))
			continue;

		rdma_stats->rdma_stats_entry[HW_STAT_RDMA_RX_BYTES] = pkB_cnt;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_RDMA_RX_BYTES] =
			ZXDH_HW_STATS_VALID;
		rdma_stats->rdma_stats_entry[HW_STAT_RDMA_RX_PKTS] = pkts_cnt;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_RDMA_RX_PKTS] =
			ZXDH_HW_STATS_VALID;
		rdma_stats->rdma_stats_entry[HW_STAT_RX_ICRC_ENCAPSULATED] =
			val;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_RX_ICRC_ENCAPSULATED] =
			ZXDH_HW_STATS_VALID;

		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}

static int zxdh_get_ram_msg_h25(struct zxdh_sc_dev *dev,
				struct zxdh_rdma_stats_get *rdma_stats)
{
	u64 val = 0;
	u32 check_ram_num, check_ram_addr;
	int ret;
	int i;

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_tx_ram_reg(dev, ZXDH_RAM_H25, ZXDH_RAM_WIDTH_128_BIT,
				    dev->vhca_id, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_tx_ram_flag(dev);
		if (ret)
			continue;

		/* Read RAM data first before checking */
		val = zxdh_read_tx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_2);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ZXDH_RAM_H25) ||
		    (check_ram_addr != dev->vhca_id))
			continue;

		rdma_stats->rdma_stats_entry[HW_STAT_RNR_NAK_RETRY_ERR] = val;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_RNR_NAK_RETRY_ERR] =
			ZXDH_HW_STATS_VALID;
		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}

static int zxdh_get_ram_msg_h26(struct zxdh_sc_dev *dev,
				struct zxdh_rdma_stats_get *rdma_stats)
{
	u64 val0 = 0, val1 = 0, val2 = 0, val3 = 0;
	u32 check_ram_num, check_ram_addr;
	int ret;
	int i;

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_tx_ram_reg(dev, ZXDH_RAM_H26, ZXDH_RAM_WIDTH_128_BIT,
				    dev->vhca_id, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_tx_ram_flag(dev);
		if (ret)
			continue;

		/* Read all RAM data first before checking */
		val0 = zxdh_read_tx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_0);
		val1 = zxdh_read_tx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_1);
		val2 = zxdh_read_tx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_2);
		val3 = zxdh_read_tx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_3);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ZXDH_RAM_H26) ||
		    (check_ram_addr != dev->vhca_id))
			continue;

		rdma_stats->rdma_stats_entry[HW_STAT_PACKET_SEQ_ERR] = val0;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_PACKET_SEQ_ERR] =
			ZXDH_HW_STATS_VALID;
		rdma_stats->rdma_stats_entry[HW_STAT_REQ_REMOTE_INVALID_REQUEST] = val1;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_REQ_REMOTE_INVALID_REQUEST] =
			ZXDH_HW_STATS_VALID;
		rdma_stats->rdma_stats_entry[HW_STAT_REQ_REMOTE_ACCESS_ERRORS] = val2;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_REQ_REMOTE_ACCESS_ERRORS] =
			ZXDH_HW_STATS_VALID;
		rdma_stats->rdma_stats_entry[HW_STAT_REQ_REMOTE_OPERATION_ERRORS] = val3;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_REQ_REMOTE_OPERATION_ERRORS] =
			ZXDH_HW_STATS_VALID;
		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}

static int zxdh_get_ram_msg_h63(struct zxdh_sc_dev *dev,
				struct zxdh_rdma_stats_get *rdma_stats)
{
	u64 val = 0;
	u32 check_ram_num, check_ram_addr;
	int ret;
	int i;

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_rx_ram_reg(dev, ZXDH_RAM_H63, ZXDH_RAM_WIDTH_32_BIT,
				    dev->vhca_id, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_rx_ram_flag(dev);
		if (ret)
			continue;

		/* Read RAM data first before checking */
		val = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_0);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ZXDH_RAM_H63) ||
		    (check_ram_addr != dev->vhca_id))
			continue;

		rdma_stats->rdma_stats_entry[HW_STAT_DUPLICATE_REQUEST] = val;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_DUPLICATE_REQUEST] =
			ZXDH_HW_STATS_VALID;
		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}

static int zxdh_get_ram_msg_h29(struct zxdh_sc_dev *dev,
				struct zxdh_rdma_stats_get *rdma_stats)
{
	u64 val = 0;
	u32 check_ram_num, check_ram_addr;
	int ret;
	int i;

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_tx_ram_reg(dev, ZXDH_RAM_H29, ZXDH_RAM_WIDTH_128_BIT,
				    dev->vhca_id, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_tx_ram_flag(dev);
		if (ret)
			continue;

		/* Read RAM data first before checking */
		val = zxdh_read_tx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_1);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ZXDH_RAM_H29) ||
		    (check_ram_addr != dev->vhca_id))
			continue;

		rdma_stats->rdma_stats_entry[HW_STAT_REQ_LOCAL_LENGTH_ERROR] = val;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_REQ_LOCAL_LENGTH_ERROR] =
			ZXDH_HW_STATS_VALID;
		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}
static int zxdh_get_ram_msg_h61(struct zxdh_sc_dev *dev,
				struct zxdh_rdma_stats_get *rdma_stats)
{
	u64 val = 0;
	u32 check_ram_num, check_ram_addr;
	int ret;
	int i;

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_rx_ram_reg(dev, ZXDH_RAM_H61, ZXDH_RAM_WIDTH_32_BIT,
				    dev->vhca_id, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_rx_ram_flag(dev);
		if (ret)
			continue;

		/* Read RAM data first before checking */
		val = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_0);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ZXDH_RAM_H61) ||
		    (check_ram_addr != dev->vhca_id))
			continue;

		rdma_stats->rdma_stats_entry[HW_STAT_RX_WRITE_REQUESTS] = val;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_RX_WRITE_REQUESTS] =
			ZXDH_HW_STATS_VALID;
		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}

static int zxdh_get_ram_msg_h62(struct zxdh_sc_dev *dev,
				struct zxdh_rdma_stats_get *rdma_stats)
{
	u64 val = 0;
	u32 check_ram_num, check_ram_addr;
	int ret;
	int i;

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_rx_ram_reg(dev, ZXDH_RAM_H62, ZXDH_RAM_WIDTH_32_BIT,
				    dev->vhca_id, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_rx_ram_flag(dev);
		if (ret)
			continue;

		/* Read RAM data first before checking */
		val = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_0);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ZXDH_RAM_H62) ||
		    (check_ram_addr != dev->vhca_id))
			continue;

		rdma_stats->rdma_stats_entry[HW_STAT_RX_READ_REQUESTS] = val;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_RX_READ_REQUESTS] =
			ZXDH_HW_STATS_VALID;
		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}

static int zxdh_get_ram_msg_h64(struct zxdh_sc_dev *dev,
				struct zxdh_rdma_stats_get *rdma_stats)
{
	u64 val = 0;
	u32 check_ram_num, check_ram_addr;
	int ret;
	int i;

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_rx_ram_reg(dev, ZXDH_RAM_H64, ZXDH_RAM_WIDTH_32_BIT,
				    dev->vhca_id, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_rx_ram_flag(dev);
		if (ret)
			continue;

		/* Read RAM data first before checking */
		val = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_0);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ZXDH_RAM_H64) ||
		    (check_ram_addr != dev->vhca_id))
			continue;

		rdma_stats->rdma_stats_entry[HW_STAT_ROCE_SLOW_RESTART_CNPS] = val;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_ROCE_SLOW_RESTART_CNPS] =
			ZXDH_HW_STATS_VALID;
		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}

static int zxdh_get_ram_msg_h104(struct zxdh_sc_dev *dev,
				 struct zxdh_rdma_stats_get *rdma_stats)
{
	u64 val0 = 0, val1 = 0, val2 = 0, val3 = 0;
	u32 check_ram_num, check_ram_addr;
	int ret;
	int i;

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_rx_ram_reg(dev, ZXDH_RAM_H104, ZXDH_RAM_WIDTH_128_BIT,
				    dev->vhca_id, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_rx_ram_flag(dev);
		if (ret)
			continue;

		/* Read all RAM data first before checking */
		val0 = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_0);
		val1 = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_1);
		val2 = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_2);
		val3 = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_3);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ZXDH_RAM_H104) ||
		    (check_ram_addr != dev->vhca_id))
			continue;

		rdma_stats->rdma_stats_entry[HW_STAT_OUT_OF_SEQUENCE] = val0;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_OUT_OF_SEQUENCE] =
			ZXDH_HW_STATS_VALID;
		rdma_stats->rdma_stats_entry[HW_STAT_RESP_RNR_NAK] = val1;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_RESP_RNR_NAK] =
			ZXDH_HW_STATS_VALID;
		rdma_stats->rdma_stats_entry[HW_STAT_RESP_REMOTE_INVALID_REQUEST] = val2;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_RESP_REMOTE_INVALID_REQUEST] =
			ZXDH_HW_STATS_VALID;
		rdma_stats->rdma_stats_entry[HW_STAT_RESP_REMOTE_ACCESS_ERRORS] = val3;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_RESP_REMOTE_ACCESS_ERRORS] =
			ZXDH_HW_STATS_VALID;
		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}

static int zxdh_get_ram_msg_h105(struct zxdh_sc_dev *dev,
				 struct zxdh_rdma_stats_get *rdma_stats)
{
	u64 val = 0;
	u32 check_ram_num, check_ram_addr;
	int ret;
	int i;

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_rx_ram_reg(dev, ZXDH_RAM_H105, ZXDH_RAM_WIDTH_128_BIT,
				    dev->vhca_id, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_rx_ram_flag(dev);
		if (ret)
			continue;

		/* Read RAM data first before checking */
		val = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_0);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ZXDH_RAM_H105) ||
		    (check_ram_addr != dev->vhca_id))
			continue;

		rdma_stats->rdma_stats_entry[HW_STAT_RESP_REMOTE_OPERATION_ERRORS] = val;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_RESP_REMOTE_OPERATION_ERRORS] =
			ZXDH_HW_STATS_VALID;
		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}

static int zxdh_get_ram_msg_h106(struct zxdh_sc_dev *dev,
				 struct zxdh_rdma_stats_get *rdma_stats)
{
	u64 val0 = 0, val1 = 0;
	u32 check_ram_num, check_ram_addr;
	int ret;
	int i;

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_rx_ram_reg(dev, ZXDH_RAM_H106, ZXDH_RAM_WIDTH_64_BIT,
				    dev->vhca_id, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_rx_ram_flag(dev);
		if (ret)
			continue;

		/* Read all RAM data first before checking */
		val0 = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_0);
		val1 = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_1);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ZXDH_RAM_H106) ||
		    (check_ram_addr != dev->vhca_id))
			continue;

		rdma_stats->rdma_stats_entry[HW_STAT_NP_ECN_MARKED_ROCE_PACKETS] = val0;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_NP_ECN_MARKED_ROCE_PACKETS] =
			ZXDH_HW_STATS_VALID;
		rdma_stats->rdma_stats_entry[HW_STAT_NP_CNP_SENT] = val1;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_NP_CNP_SENT] =
			ZXDH_HW_STATS_VALID;
		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}

static int zxdh_get_ram_msg_h19D(struct zxdh_sc_dev *dev,
				 struct zxdh_rdma_stats_get *rdma_stats)
{
	u64 val = 0;
	u64 temp_val = 0;
	u64 raw_val0 = 0, raw_val1 = 0;
	u64 req_cqe_error = 0, resp_cqe_error = 0;
	u32 check_ram_num, check_ram_addr;
	int ret;
	int i;

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_rx_ram_reg(dev, ZXDH_RAM_H19D, ZXDH_RAM_WIDTH_128_BIT,
				    dev->vhca_id, ZXDH_RAM_WIDTH_LEN_UNIT_1);
		ret = zxdh_read_rx_ram_flag(dev);
		if (ret)
			continue;

		/* Read all RAM data first before checking */
		raw_val0 = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_0);
		raw_val1 = zxdh_read_rx_ram_data(dev, ZXDH_RAM_32_BIT_IDX_1);

		/* Check if RAM data is consistent */
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ZXDH_RAM_H19D) ||
		    (check_ram_addr != dev->vhca_id))
			continue;

		/* Calculate req_cqe_error */
		val = (raw_val0 & ZXDH_32_BIT_MASK_0_15);
		temp_val = ((raw_val0 & ZXDH_32_BIT_MASK_16_31) >>
			    IRMDA_BIT_WIDTH_16);
		if (val >= temp_val)
			req_cqe_error = val - temp_val;
		else
			req_cqe_error = val + (ZXDH_CQE_ERR_MAX - temp_val);

		/* Calculate resp_cqe_error */
		val = (raw_val1 & ZXDH_32_BIT_MASK_0_15);
		temp_val = ((raw_val1 & ZXDH_32_BIT_MASK_16_31) >>
			    IRMDA_BIT_WIDTH_16);
		if (val >= temp_val)
			resp_cqe_error = val - temp_val;
		else
			resp_cqe_error = val + (ZXDH_CQE_ERR_MAX - temp_val);

		rdma_stats->rdma_stats_entry[HW_STAT_REQ_CQE_ERROR] = req_cqe_error;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_REQ_CQE_ERROR] =
			ZXDH_HW_STATS_VALID;
		rdma_stats->rdma_stats_entry[HW_STAT_RESP_CQE_ERROR] = resp_cqe_error;
		rdma_stats->rdma_stats_entry_sta[HW_STAT_RESP_CQE_ERROR] =
			ZXDH_HW_STATS_VALID;

		return 0;
	}

	/* All retries failed */
	pr_err("[zxdh_rdma] %s: get ram data failed after %d retries\n",
	       __func__, ZXDH_RAM_REPEAT_READ_CNT);
	return -EIO;
}

static void zxdh_get_np_tx_stats(struct zxdh_sc_dev *dev,
				 struct zxdh_rdma_stats_get *rdma_stats)
{
	struct iidc_core_dev_info *cdev_info;
	struct zxdh_pci_f *rf = NULL;
	u16 vhca_id_512 = 512;
	u16 vhca_id_513 = 513;
	u16 vhca_id_0 = 0;
	u16 vhca_id_127 = 127;
	u64 tx_pkts = 0;
	u64 tx_bytes = 0;
	u64 vhca_id_512_tx_pkts = 0;
	u64 vhca_id_512_tx_bytes = 0;
	u64 vhca_id_513_tx_pkts = 0;
	u64 vhca_id_513_tx_bytes = 0;
	dpp_pf_info_t pf_info = { 0 };

	rf = container_of(dev, struct zxdh_pci_f, sc_dev);
	cdev_info = rf->cdev;
	pf_info.vport = cdev_info->vport_id;
	pf_info.slot = cdev_info->slot_id;
	if ((rf->ftype != 1) || (!rf->vm_info.vm_mode)) {
		dpp_stat_port_RDMA_packet_msg_tx_cnt_get(&pf_info, dev->vhca_id,
			ZXDH_STAT_RD_MODE_UNCLR,
			&tx_bytes, &tx_pkts);
		if (rf->use_blue_flame_flag) {
			if (dev->vhca_id == vhca_id_0) {
				dpp_stat_port_RDMA_packet_msg_tx_cnt_get(&pf_info, vhca_id_512,
					ZXDH_STAT_RD_MODE_UNCLR,
					&vhca_id_512_tx_bytes, &vhca_id_512_tx_pkts);
				tx_pkts += vhca_id_512_tx_pkts;
				tx_bytes += vhca_id_512_tx_bytes;
			}
			if (dev->vhca_id == vhca_id_127) {
				dpp_stat_port_RDMA_packet_msg_tx_cnt_get(&pf_info, vhca_id_513,
					ZXDH_STAT_RD_MODE_UNCLR,
					&vhca_id_513_tx_bytes, &vhca_id_513_tx_pkts);
					tx_pkts += vhca_id_513_tx_pkts;
					tx_bytes += vhca_id_513_tx_bytes;
			}
		}
	}

	rdma_stats->rdma_stats_entry[HW_STAT_RDMA_TX_PKTS] = tx_pkts;
	rdma_stats->rdma_stats_entry_sta[HW_STAT_RDMA_TX_PKTS] =
		ZXDH_HW_STATS_VALID;

	rdma_stats->rdma_stats_entry[HW_STAT_RDMA_TX_BYTES] = tx_bytes;
	rdma_stats->rdma_stats_entry_sta[HW_STAT_RDMA_TX_BYTES] =
		ZXDH_HW_STATS_VALID;
}

static int zxdh_rdma_stats_ram_num_read(struct zxdh_sc_dev *dev,
					 struct zxdh_rdma_stats_get *rdma_stats)
{
	ZXDH_CHECK_AND_RETURN(zxdh_get_ram_msg_h11(dev, rdma_stats));
	ZXDH_CHECK_AND_RETURN(zxdh_get_ram_msg_h12_to_h15(dev, rdma_stats));
	ZXDH_CHECK_AND_RETURN(zxdh_get_ram_msg_h25(dev, rdma_stats));
	ZXDH_CHECK_AND_RETURN(zxdh_get_ram_msg_h26(dev, rdma_stats));
	ZXDH_CHECK_AND_RETURN(zxdh_get_ram_msg_h63(dev, rdma_stats));
	ZXDH_CHECK_AND_RETURN(zxdh_get_ram_msg_h29(dev, rdma_stats));
	ZXDH_CHECK_AND_RETURN(zxdh_get_ram_msg_h61(dev, rdma_stats));
	ZXDH_CHECK_AND_RETURN(zxdh_get_ram_msg_h62(dev, rdma_stats));
	ZXDH_CHECK_AND_RETURN(zxdh_get_ram_msg_h64(dev, rdma_stats));
	ZXDH_CHECK_AND_RETURN(zxdh_get_ram_msg_h104(dev, rdma_stats));
	ZXDH_CHECK_AND_RETURN(zxdh_get_ram_msg_h105(dev, rdma_stats));
	ZXDH_CHECK_AND_RETURN(zxdh_get_ram_msg_h106(dev, rdma_stats));
	ZXDH_CHECK_AND_RETURN(zxdh_get_ram_msg_h19D(dev, rdma_stats));
	return 0;
}

int zxdh_rdma_stats_read(struct zxdh_sc_dev *dev,
			  struct zxdh_rdma_stats_get *rdma_stats)
{
	int ret = 0;

	if (dev->ram_stats_first_call) {
		dev->ram_stats_first_call = false;
		return 0;
	}

	zxdh_get_np_tx_stats(dev, rdma_stats);
	mutex_lock(&zxdh_rdma_stats_ram_lock);
	ret = zxdh_rdma_stats_ram_num_read(dev, rdma_stats);
	mutex_unlock(&zxdh_rdma_stats_ram_lock);
	return ret;
}

static int zxdh_get_pma_cnt_ext(struct zxdh_sc_dev *dev,
				 struct ib_pma_portcounters_ext *pma_cnt_ext)
{
	struct iidc_core_dev_info *cdev_info;
	int ret = 0;
	u64 val;
	u64 rx_pkts = 0;
	u64 rx_bytes = 0;
	u64 tx_pkts = 0;
	u64 tx_bytes = 0;
	struct zxdh_pci_f *rf = NULL;
	dpp_pf_info_t pf_info = { 0 };

	if (dev->ram_stats_first_call) {
		dev->ram_stats_first_call = false;
		return 0;
	}

	rf = container_of(dev, struct zxdh_pci_f, sc_dev);
	cdev_info = rf->cdev;
	pf_info.vport = cdev_info->vport_id;
	pf_info.slot = cdev_info->slot_id;
	if ((rf->ftype != 1) || (!rf->vm_info.vm_mode)) {
		dpp_stat_port_RDMA_packet_msg_tx_cnt_get(&pf_info, dev->vhca_id,
			ZXDH_STAT_RD_MODE_UNCLR,
			&tx_bytes, &tx_pkts);
	}

	mutex_lock(&zxdh_rdma_stats_ram_lock);
	ret = zxdh_get_rx_stat(dev, &rx_bytes, &rx_pkts);
	mutex_unlock(&zxdh_rdma_stats_ram_lock);
	if(ret){
		return ret;
	}
	val = tx_bytes;
	/* Total number of data octets, divided by 4 (lanes), transmitted on all VLs. This is 64 bit counter. */
	val = (val / 4);
	val = cpu_to_be64(val);
	pma_cnt_ext->port_xmit_data = val;

	val = rx_bytes;
	/* Total number of data octets, divided by 4 (lanes), received on all VLs. This is 64 bit counter. */
	val = (val / 4);
	val = cpu_to_be64(val);
	pma_cnt_ext->port_rcv_data = val;

	val = tx_pkts;
	val = cpu_to_be64(val);
	pma_cnt_ext->port_xmit_packets = val;

	val = rx_pkts;
	val = cpu_to_be64(val);
	pma_cnt_ext->port_rcv_packets = val;

	val = tx_pkts;
	val = cpu_to_be64(val);
	pma_cnt_ext->port_unicast_xmit_packets = val;

	val = rx_pkts;
	val = cpu_to_be64(val);
	pma_cnt_ext->port_unicast_rcv_packets = val;
	pma_cnt_ext->port_multicast_xmit_packets = 0;
	pma_cnt_ext->port_multicast_rcv_packets = 0;
	return 0;
}

static void zxdh_get_pma_cnt(struct zxdh_sc_dev *dev,
			     struct ib_pma_portcounters *pma_cnt)
{
	pma_cnt->symbol_error_counter = 0;
	pma_cnt->link_error_recovery_counter = 0;
	pma_cnt->link_downed_counter = 0;
	pma_cnt->port_rcv_errors = 0;
	pma_cnt->port_rcv_remphys_errors = 0;
	pma_cnt->port_rcv_switch_relay_errors = 0;
	pma_cnt->port_xmit_discards = 0;
	pma_cnt->port_xmit_constraint_errors = 0;
	pma_cnt->port_xmit_wait = 0;
	pma_cnt->port_rcv_constraint_errors = 0;
	pma_cnt->link_overrun_errors = 0;
	pma_cnt->vl15_dropped = 0;
}
/**
 * zxdh_process_pma_cmd - process pma cmd
 * @dev: pointer to device structure
 * @port: the port number this packet came in on
 * @in_mad: the incoming MAD
 * @out_mad: any outgoing MAD reply
 */
int zxdh_process_pma_cmd(struct zxdh_sc_dev *dev, u8 port,
			 const struct ib_mad *in_mad, struct ib_mad *out_mad)
{
	// *out_mad = *in_mad;
	int ret = 0;
	pr_debug(
		"%s %d vhca_id:%d attr_id:0x%x counters_ext:0x%x counter:0x%x\n",
		__func__, __LINE__, dev->vhca_id, in_mad->mad_hdr.attr_id,
		IB_PMA_PORT_COUNTERS_EXT, IB_PMA_PORT_COUNTERS);
	/* Declaring support of extended counters */
	if (in_mad->mad_hdr.attr_id == IB_PMA_CLASS_PORT_INFO) {
		struct ib_class_port_info cpi = {};

		cpi.capability_mask = IB_PMA_CLASS_CAP_EXT_WIDTH;
		memcpy((out_mad->data + 40), &cpi, sizeof(cpi));
		return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_REPLY;
	}

	if (in_mad->mad_hdr.attr_id == IB_PMA_PORT_COUNTERS_EXT) {
		struct ib_pma_portcounters_ext *pma_cnt_ext =
			(struct ib_pma_portcounters_ext *)(out_mad->data + 40);
		ret = zxdh_get_pma_cnt_ext(dev, pma_cnt_ext);
		if(ret){
			return IB_MAD_RESULT_FAILURE;
		}

	} else if (in_mad->mad_hdr.attr_id == IB_PMA_PORT_COUNTERS) {
		struct ib_pma_portcounters *pma_cnt =
			(struct ib_pma_portcounters *)(out_mad->data + 40);
		zxdh_get_pma_cnt(dev, pma_cnt);
	}
	return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_REPLY;
}

/**
 * zxdh_cqp_ceq_cmd - Create/Destroy CEQ's after CEQ 0
 * @dev: pointer to device info
 * @sc_ceq: pointer to ceq structure
 * @op: Create or Destroy
 */
int zxdh_cqp_ceq_cmd(struct zxdh_sc_dev *dev, struct zxdh_sc_ceq *sc_ceq, u8 op)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = op;
	cqp_info->in.u.ceq_create.ceq = sc_ceq;
	cqp_info->in.u.ceq_create.scratch = (uintptr_t)cqp_request;

	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);

	return status;
}

/**
 * zxdh_cqp_aeq_cmd - Create/Destroy AEQ
 * @dev: pointer to device info
 * @sc_aeq: pointer to aeq structure
 * @op: Create or Destroy
 */
int zxdh_cqp_aeq_cmd(struct zxdh_sc_dev *dev, struct zxdh_sc_aeq *sc_aeq, u8 op)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = op;
	cqp_info->in.u.aeq_create.aeq = sc_aeq;
	cqp_info->in.u.aeq_create.scratch = (uintptr_t)cqp_request;

	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);

	return status;
}

/**
 * zxdh_cqp_up_map_cmd - Set the up-up mapping
 * @dev: pointer to device structure
 * @cmd: map command
 * @map_info: pointer to up map info
 */
int zxdh_cqp_up_map_cmd(struct zxdh_sc_dev *dev, u8 cmd,
			struct zxdh_up_info *map_info)
{
	return 0;
}

/**
 * zxdh_ah_cqp_op - perform an AH cqp operation
 * @rf: RDMA PCI function
 * @sc_ah: address handle
 * @cmd: AH operation
 * @wait: wait if true
 * @callback_fcn: Callback function on CQP op completion
 * @cb_param: parameter for callback function
 *
 * returns errno
 */
int zxdh_ah_cqp_op(struct zxdh_pci_f *rf, struct zxdh_sc_ah *sc_ah, u8 cmd,
		   bool wait, void (*callback_fcn)(struct zxdh_cqp_request *),
		   void *cb_param)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	int status;

	if (cmd != ZXDH_OP_AH_CREATE && cmd != ZXDH_OP_AH_DESTROY)
		return -EINVAL;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, wait);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->cqp_cmd = cmd;
	cqp_info->post_sq = 1;
	if (cmd == ZXDH_OP_AH_CREATE) {
		cqp_info->in.u.ah_create.info = sc_ah->ah_info;
		cqp_info->in.u.ah_create.scratch = (uintptr_t)cqp_request;
		cqp_info->in.u.ah_create.cqp = &rf->cqp.sc_cqp;
	} else if (cmd == ZXDH_OP_AH_DESTROY) {
		cqp_info->in.u.ah_destroy.info = sc_ah->ah_info;
		cqp_info->in.u.ah_destroy.scratch = (uintptr_t)cqp_request;
		cqp_info->in.u.ah_destroy.cqp = &rf->cqp.sc_cqp;
	}

	if (!wait) {
		cqp_request->callback_fcn = callback_fcn;
		cqp_request->param = cb_param;
	}

	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);

	if (status)
		return -ENOMEM;

	if (wait)
		sc_ah->ah_info.ah_valid = (cmd == ZXDH_OP_AH_CREATE);

	return 0;
}

/**
 * zxdh_gsi_ud_qp_ah_cb - callback after creation of AH for GSI/ID QP
 * @cqp_request: pointer to cqp_request of create AH
 */
void zxdh_gsi_ud_qp_ah_cb(struct zxdh_cqp_request *cqp_request)
{
	struct zxdh_sc_ah *sc_ah = cqp_request->param;

	if (!cqp_request->compl_info.op_ret_val)
		sc_ah->ah_info.ah_valid = true;
	else
		sc_ah->ah_info.ah_valid = false;
}

/**
 * zxdh_prm_add_pble_mem - add moemory to pble resources
 * @pprm: pble resource manager
 * @pchunk: chunk of memory to add
 */
int zxdh_prm_add_pble_mem(struct zxdh_pble_prm *pprm, struct zxdh_chunk *pchunk)
{
	u64 sizeofbitmap;

	if (pchunk->size & 0xfff)
		return -EINVAL;

	sizeofbitmap = (u64)pchunk->size >> pprm->pble_shift;

	pchunk->bitmapmem.size = sizeofbitmap >> 3;
	pchunk->bitmapmem.va = kzalloc(pchunk->bitmapmem.size, GFP_KERNEL);

	if (!pchunk->bitmapmem.va)
		return -ENOMEM;

	pchunk->bitmapbuf = pchunk->bitmapmem.va;
	bitmap_zero(pchunk->bitmapbuf, sizeofbitmap);

	pchunk->sizeofbitmap = sizeofbitmap;
	pchunk->max_bits_available = sizeofbitmap;
	/* each pble is 8 bytes hence shift by 3 */
	pprm->total_pble_alloc += pchunk->size >> 3;
	pprm->free_pble_cnt += pchunk->size >> 3;

	return 0;
}

/**
 * zxdh_prm_get_pbles - get pble's from prm
 * @pprm: pble resource manager
 * @chunkinfo: nformation about chunk where pble's were acquired
 * @mem_size: size of pble memory needed
 * @vaddr: returns virtual address of pble memory
 * @fpm_addr: returns fpm address of pble memory
 * @paaddr: returns pa address of pble memory
 */
int zxdh_prm_get_pbles(struct zxdh_pble_prm *pprm,
		       struct zxdh_pble_chunkinfo *chunkinfo, u64 mem_size,
		       u64 **vaddr, u64 *fpm_addr, dma_addr_t *paaddr)
{
	u64 bits_needed;
	u64 bit_idx = PBLE_INVALID_IDX;
	struct zxdh_chunk *pchunk = NULL;
	struct list_head *chunk_entry = pprm->clist.next;
	u32 offset;
	unsigned long flags;
	*vaddr = NULL;
	*fpm_addr = 0;
	*paaddr = 0;

	bits_needed = DIV_ROUND_UP_ULL(mem_size, BIT_ULL(pprm->pble_shift));

	spin_lock_irqsave(&pprm->prm_lock, flags);
	while (chunk_entry != &pprm->clist) {
		pchunk = (struct zxdh_chunk *)chunk_entry;
		if (pchunk->max_bits_available < bits_needed) {
			chunk_entry = pchunk->list.next;
			continue;
		}

		bit_idx = bitmap_find_next_zero_area(pchunk->bitmapbuf,
						     pchunk->sizeofbitmap, 0,
						     bits_needed, 0);
		if (bit_idx < pchunk->sizeofbitmap)
			break;

		pchunk->max_bits_available = bits_needed - 1;
		/* list.next used macro */
		chunk_entry = pchunk->list.next;
	}

	if (!pchunk || bit_idx >= pchunk->sizeofbitmap) {
		spin_unlock_irqrestore(&pprm->prm_lock, flags);
		return -ENOMEM;
	}

	bitmap_set(pchunk->bitmapbuf, bit_idx, bits_needed);
	offset = bit_idx << pprm->pble_shift;
	*vaddr = pchunk->vaddr + offset;
	*fpm_addr = pchunk->fpm_addr + offset;
	*paaddr = pchunk->pa + offset;
	chunkinfo->pchunk = pchunk;
	chunkinfo->bit_idx = bit_idx;
	chunkinfo->bits_used = bits_needed;
	/* 3 is sizeof pble divide */
	pprm->free_pble_cnt -= chunkinfo->bits_used << (pprm->pble_shift - 3);
	spin_unlock_irqrestore(&pprm->prm_lock, flags);

	return 0;
}

/**
 * zxdh_prm_return_pbles - return pbles back to prm
 * @pprm: pble resource manager
 * @chunkinfo: chunk where pble's were acquired and to be freed
 */
void zxdh_prm_return_pbles(struct zxdh_pble_prm *pprm,
			   struct zxdh_pble_chunkinfo *chunkinfo)
{
	unsigned long flags;

	spin_lock_irqsave(&pprm->prm_lock, flags);
	pprm->free_pble_cnt += chunkinfo->bits_used << (pprm->pble_shift - 3);
	bitmap_clear(chunkinfo->pchunk->bitmapbuf, chunkinfo->bit_idx,
		     chunkinfo->bits_used);
	chunkinfo->pchunk->max_bits_available = chunkinfo->pchunk->sizeofbitmap;
	spin_unlock_irqrestore(&pprm->prm_lock, flags);
}

int zxdh_map_vm_page_list(struct zxdh_hw *hw, void *va, dma_addr_t *pg_dma,
			  u32 pg_cnt)
{
	struct page *vm_page;
	int i;
	u8 *addr;

	addr = (u8 *)(uintptr_t)va;
	for (i = 0; i < pg_cnt; i++) {
		vm_page = vmalloc_to_page(addr);
		if (!vm_page)
			goto err;

		pg_dma[i] = dma_map_page(hw->device, vm_page, 0, PAGE_SIZE,
					 DMA_BIDIRECTIONAL);
		if (dma_mapping_error(hw->device, pg_dma[i]))
			goto err;

		addr += PAGE_SIZE;
	}

	return 0;

err:
	zxdh_unmap_vm_page_list(hw, pg_dma, i);
	return -ENOMEM;
}

void zxdh_unmap_vm_page_list(struct zxdh_hw *hw, dma_addr_t *pg_dma, u32 pg_cnt)
{
	int i;

	for (i = 0; i < pg_cnt; i++)
		dma_unmap_page(hw->device, pg_dma[i], PAGE_SIZE,
			       DMA_BIDIRECTIONAL);
}

/**
 * zxdh_pble_free_paged_mem - free virtual paged memory
 * @chunk: chunk to free with paged memory
 */
void zxdh_pble_free_paged_mem(struct zxdh_chunk *chunk)
{
	if (!chunk->pg_cnt)
		goto done;

	zxdh_unmap_vm_page_list(chunk->dev->hw, chunk->dmainfo.dmaaddrs,
				chunk->pg_cnt);

done:
	kfree(chunk->dmainfo.dmaaddrs);
	chunk->dmainfo.dmaaddrs = NULL;
	vfree(chunk->vaddr);
	chunk->vaddr = NULL;
	chunk->type = 0;
}

/**
 * zxdh_modify_qp_to_err - Modify a QP to error
 * @sc_qp: qp structure
 */
void zxdh_modify_qp_to_err(struct zxdh_sc_qp *sc_qp)
{
	struct zxdh_qp *qp = sc_qp->qp_uk.back_qp;
	struct ib_qp_attr attr;

	if (qp->iwdev->rf->reset)
		return;
	attr.qp_state = IB_QPS_ERR;

	zxdh_modify_qp_roce(&qp->ibqp, &attr, IB_QP_STATE, NULL);
}

void zxdh_ib_qp_event(struct zxdh_qp *iwqp, enum zxdh_qp_event_type event)
{
	struct ib_event ibevent;

	if (!iwqp->ibqp.event_handler)
		return;

	switch (event) {
	case ZXDH_QP_EVENT_CATASTROPHIC:
		ibevent.event = IB_EVENT_QP_FATAL;
		break;
	case ZXDH_QP_EVENT_ACCESS_ERR:
		ibevent.event = IB_EVENT_QP_ACCESS_ERR;
		break;
	case ZXDH_QP_EVENT_REQ_ERR:
		ibevent.event = IB_EVENT_QP_REQ_ERR;
		break;
	}
	ibevent.device = iwqp->ibqp.device;
	ibevent.element.qp = &iwqp->ibqp;
	iwqp->ibqp.event_handler(&ibevent, iwqp->ibqp.qp_context);
}

/**
 * zxdh_upload_qp_context - upload raw QP context
 * @iwqp: QP pointer
 * @freeze: freeze QP
 * @raw: raw context flag
 */
int zxdh_upload_qp_context(struct zxdh_qp *iwqp, bool freeze, bool raw)
{
	return 0;
}

int zxdh_cqp_rdma_read_cmd(struct zxdh_sc_dev *dev,
			   struct zxdh_src_copy_dest *src_dest, u8 src_dir,
			   u8 dest_dir)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = ZXDH_OP_DMA_READ;
	cqp_info->in.u.dma_writeread.cqp = dev->cqp;
	cqp_info->in.u.dma_writeread.src_dest.src = src_dest->src;
	cqp_info->in.u.dma_writeread.src_dest.len = src_dest->len;
	cqp_info->in.u.dma_writeread.src_dest.dest = src_dest->dest;

	cqp_info->in.u.dma_writeread.src_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.src_path_index.obj_id = ZXDH_DMA_OBJ_ID;

	cqp_info->in.u.dma_writeread.src_path_index.path_select = src_dir;

	cqp_info->in.u.dma_writeread.src_path_index.inter_select =
		ZXDH_INTERFACE_NOTCACHE;

	cqp_info->in.u.dma_writeread.dest_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.dest_path_index.obj_id = ZXDH_DMA_OBJ_ID;

	if (dev->cache_id == 0) {
		cqp_info->in.u.dma_writeread.dest_path_index.path_select =
			dest_dir;
		cqp_info->in.u.dma_writeread.dest_path_index.inter_select =
			ZXDH_INTERFACE_NOTCACHE;
	} else {
		cqp_info->in.u.dma_writeread.dest_path_index.path_select =
			dev->cache_id;
		cqp_info->in.u.dma_writeread.dest_path_index.inter_select =
			ZXDH_INTERFACE_CACHE;
	}

	cqp_info->in.u.dma_writeread.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	return status;
}

int zxdh_cqp_damreadbycqe_cmd(struct zxdh_sc_dev *dev,
			      struct zxdh_dam_read_bycqe *dmadata,
			      struct zxdh_path_index *src_path_index, u64 *arr)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status, i = 0;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = ZXDH_OP_DMA_READ_USE_CQE;
	cqp_info->in.u.dma_read_cqe.cqp = dev->cqp;
	cqp_info->in.u.dma_read_cqe.dma_rcqe.num = dmadata->num;
	cqp_info->in.u.dma_read_cqe.dma_rcqe.bitwidth = dmadata->bitwidth;
	cqp_info->in.u.dma_read_cqe.dma_rcqe.valuetype = dmadata->valuetype;
	for (i = 0; i < dmadata->num; i++) {
		cqp_info->in.u.dma_read_cqe.dma_rcqe.addrbuf[i] =
			dmadata->addrbuf[i];
	}

	cqp_info->in.u.dma_read_cqe.src_path_index.vhca_id =
		src_path_index->vhca_id;
	cqp_info->in.u.dma_read_cqe.src_path_index.obj_id =
		src_path_index->obj_id;
	cqp_info->in.u.dma_read_cqe.src_path_index.path_select =
		src_path_index->path_select;
	cqp_info->in.u.dma_read_cqe.src_path_index.inter_select =
		src_path_index->inter_select;

	cqp_info->in.u.dma_read_cqe.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);

	for (i = 0; i < 5; i++)
		arr[i] = cqp_request->compl_info.addrbuf[i];

	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	return status;
}

int zxdh_cqp_rdma_write32_cmd(struct zxdh_sc_dev *dev,
			      struct zxdh_dma_write32_date *dma_data)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status, i = 0;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = ZXDH_OP_DMA_WRITE32;
	cqp_info->in.u.dma_write32data.cqp = dev->cqp;
	cqp_info->in.u.dma_write32data.dest_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_write32data.dma_data.num = dma_data->num;
	cqp_info->in.u.dma_write32data.dma_data.inter_sour_sel =
		dma_data->inter_sour_sel;
	cqp_info->in.u.dma_write32data.dma_data.need_inter =
		dma_data->need_inter;
	for (i = 0; i < dma_data->num; i++) {
		cqp_info->in.u.dma_write32data.dma_data.addrbuf[i] =
			dma_data->addrbuf[i];
		cqp_info->in.u.dma_write32data.dma_data.databuf[i] =
			dma_data->databuf[i];
	}

	cqp_info->in.u.dma_write32data.dest_path_index.obj_id = ZXDH_REG_OBJ_ID;
	cqp_info->in.u.dma_write32data.dest_path_index.path_select =
		ZXDH_INDICATE_REGISTER;
	cqp_info->in.u.dma_write32data.dest_path_index.inter_select =
		ZXDH_INTERFACE_NOTCACHE;
	cqp_info->in.u.dma_write32data.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	return status;
}

int zxdh_dpuddr_to_host_cmd(struct zxdh_sc_dev *dev,
			    struct zxdh_src_copy_dest *src_dest)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = ZXDH_OP_DMA_WRITE;
	cqp_info->in.u.dma_writeread.cqp = dev->cqp;
	cqp_info->in.u.dma_writeread.src_dest.src = src_dest->src;
	cqp_info->in.u.dma_writeread.src_dest.len = src_dest->len;
	cqp_info->in.u.dma_writeread.src_dest.dest = src_dest->dest;

	cqp_info->in.u.dma_writeread.src_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.src_path_index.obj_id = ZXDH_DMA_OBJ_ID;

	cqp_info->in.u.dma_writeread.src_path_index.path_select =
		ZXDH_INDICATE_DPU_DDR;

	cqp_info->in.u.dma_writeread.src_path_index.inter_select =
		ZXDH_INTERFACE_NOTCACHE;

	cqp_info->in.u.dma_writeread.dest_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.dest_path_index.obj_id = ZXDH_DMA_OBJ_ID;

	cqp_info->in.u.dma_writeread.dest_path_index.path_select =
		ZXDH_INDICATE_HOST_NOSMMU;
	cqp_info->in.u.dma_writeread.dest_path_index.inter_select =
		ZXDH_INTERFACE_NOTCACHE;

	cqp_info->in.u.dma_writeread.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	return status;
}

int zxdh_cqp_rdma_write_cmd(struct zxdh_sc_dev *dev,
			    struct zxdh_src_copy_dest *src_dest, u8 src_dir,
			    u8 dest_dir)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = ZXDH_OP_DMA_WRITE;
	cqp_info->in.u.dma_writeread.cqp = dev->cqp;
	cqp_info->in.u.dma_writeread.src_dest.src = src_dest->src;
	cqp_info->in.u.dma_writeread.src_dest.len = src_dest->len;
	cqp_info->in.u.dma_writeread.src_dest.dest = src_dest->dest;

	cqp_info->in.u.dma_writeread.src_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.src_path_index.obj_id = ZXDH_DMA_OBJ_ID;

	cqp_info->in.u.dma_writeread.src_path_index.path_select = src_dir;

	cqp_info->in.u.dma_writeread.src_path_index.inter_select =
		ZXDH_INTERFACE_NOTCACHE;

	cqp_info->in.u.dma_writeread.dest_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.dest_path_index.obj_id = ZXDH_DMA_OBJ_ID;

	if (dev->cache_id == 0) {
		cqp_info->in.u.dma_writeread.dest_path_index.path_select =
			dest_dir;
		cqp_info->in.u.dma_writeread.dest_path_index.inter_select =
			ZXDH_INTERFACE_NOTCACHE;
	} else {
		cqp_info->in.u.dma_writeread.dest_path_index.path_select =
			dev->cache_id;
		cqp_info->in.u.dma_writeread.dest_path_index.inter_select =
			ZXDH_INTERFACE_CACHE;
	}

	cqp_info->in.u.dma_writeread.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	return status;
}

int zxdh_cqp_rdma_readreg_cmd(struct zxdh_sc_dev *dev,
			      struct zxdh_src_copy_dest *src_dest)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = ZXDH_OP_DMA_READ;
	cqp_info->in.u.dma_writeread.cqp = dev->cqp;
	cqp_info->in.u.dma_writeread.src_dest.src = src_dest->src;
	cqp_info->in.u.dma_writeread.src_dest.len = src_dest->len;
	cqp_info->in.u.dma_writeread.src_dest.dest = src_dest->dest;

	cqp_info->in.u.dma_writeread.src_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.src_path_index.obj_id = ZXDH_REG_OBJ_ID;

	cqp_info->in.u.dma_writeread.src_path_index.path_select =
		ZXDH_INDICATE_REGISTER;

	cqp_info->in.u.dma_writeread.src_path_index.inter_select =
		ZXDH_INTERFACE_NOTCACHE;

	cqp_info->in.u.dma_writeread.dest_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.dest_path_index.obj_id = ZXDH_DMA_OBJ_ID;

	cqp_info->in.u.dma_writeread.dest_path_index.path_select =
		ZXDH_INDICATE_HOST_NOSMMU;
	cqp_info->in.u.dma_writeread.dest_path_index.inter_select =
		ZXDH_INTERFACE_NOTCACHE;

	cqp_info->in.u.dma_writeread.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	return status;
}

int zxdh_cqp_rdma_read_mrte_cmd(struct zxdh_sc_dev *dev,
				struct zxdh_src_copy_dest *src_dest)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = ZXDH_OP_DMA_READ;
	cqp_info->in.u.dma_writeread.cqp = dev->cqp;
	cqp_info->in.u.dma_writeread.src_dest.src = src_dest->src;
	cqp_info->in.u.dma_writeread.src_dest.len = src_dest->len;
	cqp_info->in.u.dma_writeread.src_dest.dest = src_dest->dest;

	cqp_info->in.u.dma_writeread.src_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.src_path_index.obj_id = ZXDH_MR_OBJ_ID;
	cqp_info->in.u.dma_writeread.src_path_index.path_select = dev->cache_id;
	cqp_info->in.u.dma_writeread.src_path_index.inter_select =
		ZXDH_INTERFACE_CACHE;

	cqp_info->in.u.dma_writeread.dest_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.dest_path_index.obj_id = ZXDH_DMA_OBJ_ID;
	cqp_info->in.u.dma_writeread.dest_path_index.path_select =
		ZXDH_INDICATE_HOST_NOSMMU;
	cqp_info->in.u.dma_writeread.dest_path_index.inter_select =
		ZXDH_INTERFACE_NOTCACHE;

	cqp_info->in.u.dma_writeread.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	return status;
}

int zxdh_cqp_rdma_read_tx_window_cmd(struct zxdh_sc_dev *dev,
				     struct zxdh_src_copy_dest *src_dest)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = ZXDH_OP_DMA_READ;
	cqp_info->in.u.dma_writeread.cqp = dev->cqp;
	cqp_info->in.u.dma_writeread.src_dest.src = src_dest->src;
	cqp_info->in.u.dma_writeread.src_dest.len = src_dest->len;
	cqp_info->in.u.dma_writeread.src_dest.dest = src_dest->dest;

	cqp_info->in.u.dma_writeread.src_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.src_path_index.obj_id =
		ZXDH_TX_WINDOW_OBJ_ID;
	cqp_info->in.u.dma_writeread.src_path_index.path_select = dev->cache_id;
	cqp_info->in.u.dma_writeread.src_path_index.inter_select =
		ZXDH_INTERFACE_CACHE;

	cqp_info->in.u.dma_writeread.dest_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.dest_path_index.obj_id = ZXDH_DMA_OBJ_ID;
	cqp_info->in.u.dma_writeread.dest_path_index.path_select =
		ZXDH_INDICATE_HOST_NOSMMU;
	cqp_info->in.u.dma_writeread.dest_path_index.inter_select =
		ZXDH_INTERFACE_NOTCACHE;

	cqp_info->in.u.dma_writeread.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	return status;
}

int zxdh_cqp_config_pble_table_cmd(struct zxdh_sc_dev *dev,
				   struct zxdh_pble_info *pbleinfo, u32 len,
				   bool pbletype)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status;
	u64 baseaddr = 0;

    if (rf->use_ext_mem_flag)
		return 0;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = ZXDH_OP_CONFIG_PBLE_TAB;
	cqp_info->in.u.dma_writeread.cqp = dev->cqp;
	cqp_info->in.u.dma_writeread.src_dest.src = pbleinfo->pa;
	cqp_info->in.u.dma_writeread.src_dest.len = len;
	cqp_info->in.u.dma_writeread.src_dest.dest = pbleinfo->smmu_fpm_addr;

	cqp_info->in.u.dma_writeread.src_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.src_path_index.obj_id =
		ZXDH_DMA_OBJ_ID; // 0 | 1
	cqp_info->in.u.dma_writeread.src_path_index.waypartion = 0;

	cqp_info->in.u.dma_writeread.src_path_index.path_select =
		ZXDH_INDICATE_HOST_NOSMMU; // 到host不经过SMMU

	cqp_info->in.u.dma_writeread.src_path_index.inter_select =
		ZXDH_INTERFACE_NOTCACHE; // 不经过cache

	cqp_info->in.u.dma_writeread.dest_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.dest_path_index.obj_id =
		(pbletype == true) ? ZXDH_PBLE_MR_OBJ_ID :
					   ZXDH_PBLE_QUEUE_OBJ_ID; //  0 | 1
	cqp_info->in.u.dma_writeread.dest_path_index.waypartion = 0;

	if (dev->cache_id == 0) {
		if (dev->hmc_use_dpu_ddr == true) {
			cqp_info->in.u.dma_writeread.dest_path_index
				.path_select = ZXDH_INDICATE_DPU_DDR; //
		} else {
			cqp_info->in.u.dma_writeread.dest_path_index
				.path_select = ZXDH_INDICATE_HOST_SMMU;
		}
		cqp_info->in.u.dma_writeread.dest_path_index.inter_select =
			ZXDH_INTERFACE_NOTCACHE; // 经过cache
		cqp_info->in.u.dma_writeread.src_dest.dest =
			pbleinfo->smmu_fpm_addr;
	} else {
		if (pbletype == true) {
			baseaddr =
				dev->hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE_MR].base;
		} else {
			baseaddr =
				dev->hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE].base;
		}
		cqp_info->in.u.dma_writeread.src_dest.dest =
			pbleinfo->smmu_fpm_addr - baseaddr;
		cqp_info->in.u.dma_writeread.dest_path_index.path_select =
			dev->cache_id; //
		cqp_info->in.u.dma_writeread.dest_path_index.inter_select =
			ZXDH_INTERFACE_CACHE; // 经过cache
	}

	cqp_info->in.u.dma_writeread.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	return status;
}

bool zxdh_cq_empty(struct zxdh_cq *iwcq)
{
	struct zxdh_cq_uk *ukcq;
	u64 qword0;
	__le64 *cqe;
	u8 polarity;

	ukcq = &iwcq->sc_cq.cq_uk;
	if (ukcq->valid_cq == false)
		return 0;
	cqe = ZXDH_GET_CURRENT_CQ_ELEM(ukcq);
	get_64bit_val(cqe, 0, &qword0);
	polarity = (u8)FIELD_GET(ZXDH_CQ_VALID, qword0);

	return polarity != ukcq->polarity;
}

void zxdh_remove_cmpls_list(struct zxdh_cq *iwcq)
{
	struct zxdh_cmpl_gen *cmpl_node;
	struct list_head *tmp_node, *list_node;

	list_for_each_safe(list_node, tmp_node, &iwcq->cmpl_generated) {
		cmpl_node = list_entry(list_node, struct zxdh_cmpl_gen, list);
		list_del(&cmpl_node->list);
		kfree(cmpl_node);
	}
}

enum zxdh_status_code zxdh_generated_cmpls(struct zxdh_cq *iwcq,
			 struct zxdh_cq_poll_info *cq_poll_info)
{
	struct zxdh_cmpl_gen *cmpl;

	if (!iwcq || list_empty(&iwcq->cmpl_generated))
		return ZXDH_ERR_Q_EMPTY;
	cmpl = list_first_entry_or_null(&iwcq->cmpl_generated,
					struct zxdh_cmpl_gen, list);
	list_del(&cmpl->list);
	memcpy(cq_poll_info, &cmpl->cpi, sizeof(*cq_poll_info));
	kfree(cmpl);

	return  ZXDH_SUCCESS;
}

/**
 * zxdh_set_cpi_common_values - fill in values for polling info struct
 * @cpi: resulting structure of cq_poll_info type
 * @qp: QPair
 * @qp_num: id of the QP
 */
static void zxdh_set_cpi_common_values(struct zxdh_cq_poll_info *cpi,
				       struct zxdh_qp_uk *qp, u32 qp_num)
{
	cpi->comp_status = ZXDH_COMPL_STATUS_FLUSHED;
	cpi->error = 1;
	cpi->major_err = ZXDH_FLUSH_MAJOR_ERR;
	cpi->minor_err = FLUSH_GENERAL_ERR;
	cpi->qp_handle = (zxdh_qp_handle)(uintptr_t)qp;
	cpi->qp_id = qp_num;
}

static inline void zxdh_comp_handler(struct zxdh_cq *cq)
{
	if (cq->sc_cq.cq_uk.armed && cq->ibcq.comp_handler)
		cq->ibcq.comp_handler(&cq->ibcq, cq->ibcq.cq_context);
}

/**
 * zxdh_generate_flush_completions - generate completion from WRs
 * @iwqp: pointer to QP
 */
void zxdh_generate_flush_completions(struct zxdh_qp *iwqp)
{
	struct zxdh_qp_uk *qp = &iwqp->sc_qp.qp_uk;
	struct zxdh_ring *sq_ring = &qp->sq_ring;
	struct zxdh_ring *rq_ring = &qp->rq_ring;
	struct zxdh_cmpl_gen *cmpl;
	__le64 *sw_wqe;
	u64 wqe_qword;
	u32 wqe_idx;
	u8 compl_generated = 0;
	unsigned long flags;

#define SQ_COMPL_GENERATED (0x01)
#define RQ_COMPL_GENERATED (0x02)

	spin_lock_irqsave(&iwqp->iwscq->lock, flags);
	if (zxdh_cq_empty(iwqp->iwscq)) {
		while (ZXDH_RING_MORE_WORK(*sq_ring)) {
			cmpl = kzalloc(sizeof(*cmpl), GFP_ATOMIC);
			if (!cmpl) {
				spin_unlock_irqrestore(&iwqp->iwscq->lock,
						       flags);
				return;
			}

			wqe_idx = sq_ring->tail;
			zxdh_set_cpi_common_values(&cmpl->cpi, qp, qp->qp_id);

			cmpl->cpi.wr_id = qp->sq_wrtrk_array[wqe_idx].wrid;
			sw_wqe = qp->sq_base[wqe_idx].elem;
			get_64bit_val(sw_wqe, 24, &wqe_qword);
			cmpl->cpi.op_type =
				(u8)FIELD_GET(IRDMAQPSQ_OPCODE, wqe_qword);
			/* remove the SQ WR by moving SQ tail*/
			ZXDH_RING_SET_TAIL(
				*sq_ring,
				sq_ring->tail +
					qp->sq_wrtrk_array[sq_ring->tail]
						.quanta);

			list_add_tail(&cmpl->list,
				      &iwqp->iwscq->cmpl_generated);
			compl_generated |= SQ_COMPL_GENERATED;
		}
	} else {
		mod_delayed_work(iwqp->iwdev->cleanup_wq, &iwqp->dwork_flush,
				 ZXDH_FLUSH_DELAY_MS / 2);
	}
	spin_unlock_irqrestore(&iwqp->iwscq->lock, flags);

	spin_lock_irqsave(&iwqp->iwrcq->lock, flags);
	if (zxdh_cq_empty(iwqp->iwrcq)) {
		while (ZXDH_RING_MORE_WORK(*rq_ring)) {
			cmpl = kzalloc(sizeof(*cmpl), GFP_ATOMIC);
			if (!cmpl) {
				spin_unlock_irqrestore(&iwqp->iwrcq->lock,
						       flags);
				return;
			}

			wqe_idx = rq_ring->tail;
			zxdh_set_cpi_common_values(&cmpl->cpi, qp, qp->qp_id);

			cmpl->cpi.wr_id = qp->rq_wrid_array[wqe_idx];
			cmpl->cpi.op_type = ZXDH_OP_TYPE_REC;
			/* remove the RQ WR by moving RQ tail */
			ZXDH_RING_SET_TAIL(*rq_ring, rq_ring->tail + 1);
			list_add_tail(&cmpl->list,
				      &iwqp->iwrcq->cmpl_generated);

			compl_generated |= RQ_COMPL_GENERATED;
		}
	} else {
		mod_delayed_work(iwqp->iwdev->cleanup_wq, &iwqp->dwork_flush,
				 ZXDH_FLUSH_DELAY_MS / 2);
	}
	spin_unlock_irqrestore(&iwqp->iwrcq->lock, flags);

	if (iwqp->iwscq == iwqp->iwrcq) {
		if (compl_generated)
			zxdh_comp_handler(iwqp->iwscq);
		return;
	}
	if (compl_generated & SQ_COMPL_GENERATED)
		zxdh_comp_handler(iwqp->iwscq);
	if (compl_generated & RQ_COMPL_GENERATED)
		zxdh_comp_handler(iwqp->iwrcq);
	if (compl_generated)
		pr_info("[zxdh_rdma] VERBS: 0x%X (SQ 0x1, RQ 0x2, both 0x3) completions generated for QP %d\n",
			compl_generated, iwqp->ibqp.qp_num);
}

u64 zxdh_get_path_index(struct zxdh_path_index *path_index)
{
	u64 path_index_result = 0, tmp = 0;

	tmp = path_index->inter_select;
	path_index_result |= tmp;

	tmp = path_index->path_select;
	tmp <<= 8;
	path_index_result |= tmp;

	tmp = path_index->waypartion;
	tmp <<= 12;
	path_index_result |= tmp;

	tmp = path_index->obj_id;
	tmp <<= 16;
	path_index_result |= tmp;

	tmp = path_index->vhca_id;
	tmp <<= 24;
	path_index_result |= tmp;

	return path_index_result;
}

int zxdh_cqp_config_pte_table_cmd(struct zxdh_sc_dev *dev,
				  struct zxdh_src_copy_dest scr_dest)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = ZXDH_OP_CONFIG_PTE_TAB;
	cqp_info->in.u.dma_writeread.cqp = dev->cqp;
	cqp_info->in.u.dma_writeread.src_dest.src = scr_dest.src;
	cqp_info->in.u.dma_writeread.src_dest.len = scr_dest.len;
	cqp_info->in.u.dma_writeread.src_dest.dest = scr_dest.dest;

	cqp_info->in.u.dma_writeread.src_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.src_path_index.obj_id =
		ZXDH_DMA_OBJ_ID; // DMA 搬移 使用宏定义
	cqp_info->in.u.dma_writeread.src_path_index.path_select =
		ZXDH_INDICATE_HOST_NOSMMU; // 到host不经过SMMU
	cqp_info->in.u.dma_writeread.src_path_index.inter_select =
		ZXDH_INTERFACE_NOTCACHE; // 不经过cache

	cqp_info->in.u.dma_writeread.dest_path_index.vhca_id = dev->vhca_id;
	cqp_info->in.u.dma_writeread.dest_path_index.obj_id =
		ZXDH_L2D_OBJ_ID; // L2D
	cqp_info->in.u.dma_writeread.dest_path_index.path_select =
		ZXDH_INDICATE_L2D; // L2D
	cqp_info->in.u.dma_writeread.dest_path_index.inter_select =
		ZXDH_INTERFACE_NOTCACHE; // 不经过cache
	cqp_info->in.u.dma_writeread.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	return status;
}

int zxdh_sc_send_mailbox_cmd(struct zxdh_sc_dev *dev, u8 opt, u64 msg2,
			     u64 msg3, u64 msg4, u16 dst_vf_id)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = ZXDH_OP_CONFIG_MAILBOX;
	cqp_info->in.u.hmc_mb.cqp = dev->cqp;
	cqp_info->in.u.hmc_mb.dst_vf_id = dst_vf_id;
	cqp_info->in.u.hmc_mb.mbhead_data.msg0 = opt;
	cqp_info->in.u.hmc_mb.mbhead_data.msg1 = dev->vhca_id;
	cqp_info->in.u.hmc_mb.mbhead_data.msg2 = msg2;
	cqp_info->in.u.hmc_mb.mbhead_data.msg3 = msg3;
	cqp_info->in.u.hmc_mb.mbhead_data.msg4 = msg4;
	cqp_info->in.u.hmc_mb.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	return status;
}

int zxdh_sc_query_mkey_cmd(struct zxdh_sc_dev *dev, u32 mekyindex)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = ZXDH_OP_QUERY_MKEY;
	cqp_info->in.u.query_mkey.cqp = dev->cqp;
	cqp_info->in.u.query_mkey.mkeyindex = mekyindex;
	cqp_info->in.u.query_mkey.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	return status;
}

static const char *const _zxdh_qp_state_to_string[ZXDH_QPS_RSV] = {
	[ZXDH_QPS_RESET] = "RESET", [ZXDH_QPS_INIT] = "INIT",
	[ZXDH_QPS_RTR] = "RTR",	    [ZXDH_QPS_RTS] = "RTS",
	[ZXDH_QPS_SQE] = "SQE",	    [ZXDH_QPS_SQD] = "SQD",
	[ZXDH_QPS_ERR] = "ERROR",
};

const char *zxdh_qp_state_to_string(enum ib_qp_state state)
{
	return _zxdh_qp_state_to_string[state];
}

int get_pci_board_bdf(char *pci_board_bdf, struct zxdh_pci_f *rf)
{
	int domain;
	int bus;
	int device;
	if (rf->pcidev == NULL || rf->pcidev->bus == NULL) {
		pr_info("[zxdh_rdma] get_pci_board_bdf fail:rf pcidev is null\n");
		return -EIO;
	}
	domain = pci_domain_nr(rf->pcidev->bus);
	bus = rf->pcidev->bus->number;
	device = PCI_SLOT(rf->pcidev->devfn);
	sprintf(pci_board_bdf, "%04d:%02x:%02x", domain, bus, device);
	pr_info("[zxdh_rdma] %s succ:%s\n", __func__, pci_board_bdf);
	return 0;
}

int zxdh_read_ram_32bit_value(struct zxdh_sc_dev *dev, u32 ram_num, u32 ram_width,
		     u32 ram_read_cnt, u32 offset_idx, u32 *value)
{
	u32 check_ram_num, check_ram_addr;
	int ret;
	int i;

	mutex_lock(&zxdh_rdma_stats_ram_lock);

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_rx_ram_reg(dev, ram_num, ram_width, dev->vhca_id,
				    ram_read_cnt);
		ret = zxdh_read_rx_ram_flag(dev);
		if (ret) {
			continue;
		}
		check_ram_num = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_num != ram_num) ||
		    (check_ram_addr != dev->vhca_id)) {
			pr_err("[zxdh_rdma] %s: get ram data failed! ram_num:0x%x, rdma_addr:0x%x\n",
			       __func__, check_ram_num, check_ram_addr);
			ret = -ERANGE;
			goto unlock;
		}
		*value = zxdh_read_rx_ram_data(dev, offset_idx);
		ret = 0;
		goto unlock;
	}
	pr_err("[zxdh_rdma] %s: get ram data failed !\n", __func__);
	ret = -EIO;

unlock:
	mutex_unlock(&zxdh_rdma_stats_ram_lock);
	return ret;
}

int zxdh_read_ram_rx_values(struct zxdh_sc_dev *dev,
			    struct read_ram_info *ram_info, u32 *value)
{
	u32 check_ram_id, check_ram_addr;
	int ret;
	int i, offset_idx;

	mutex_lock(&zxdh_rdma_stats_ram_lock);

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_rx_ram_reg(dev, ram_info->ram_num, ram_info->ram_width,
				    ram_info->ram_addr, ram_info->ram_read_cnt);
		ret = zxdh_read_rx_ram_flag(dev);
		if (ret) {
			continue;
		}
		check_ram_id = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_id != ram_info->ram_num) ||
		    (check_ram_addr != ram_info->ram_addr)) {
			pr_err("[zxdh_rdma] %s: get ram data failed! ram_num:0x%x, rdma_addr:0x%x\n",
			       __func__, check_ram_id, check_ram_addr);
			ret = -ERANGE;
			goto unlock;
		}
		for (offset_idx = 0; offset_idx < ram_info->offset_idx;
		     offset_idx++) {
			*value = zxdh_read_rx_ram_data(dev, offset_idx);
			value++;
		}
		ret = 0;
		goto unlock;
	}
	pr_err("[zxdh_rdma] %s: get ram data failed !\n", __func__);
	ret = -EIO;

unlock:
	mutex_unlock(&zxdh_rdma_stats_ram_lock);
	return ret;
}

int zxdh_read_ram_tx_values(struct zxdh_sc_dev *dev,
			    struct read_ram_info *ram_info, u32 *value)
{
	u32 check_ram_id, check_ram_addr;
	int ret;
	int i, offset_idx;

	mutex_lock(&zxdh_rdma_stats_ram_lock);

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		zxdh_set_tx_ram_reg(dev, ram_info->ram_num, ram_info->ram_width,
				    ram_info->ram_addr, ram_info->ram_read_cnt);
		ret = zxdh_read_tx_ram_flag(dev);
		if (ret) {
			continue;
		}
		check_ram_id = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_NUM(dev->ep_id)));
		check_ram_addr = readl(
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_RAM_ADDR(dev->ep_id)));
		if ((check_ram_id != ram_info->ram_num) ||
		    (check_ram_addr != ram_info->ram_addr)) {
			pr_err("[zxdh_rdma] %s: get ram data failed! ram_num:0x%x, rdma_addr:0x%x\n",
			       __func__, check_ram_id, check_ram_addr);
			ret = -ERANGE;
			goto unlock;
		}
		for (offset_idx = 0; offset_idx < ram_info->offset_idx;
		     offset_idx++) {
			*value = zxdh_read_tx_ram_data(dev, offset_idx);
			value++;
		}
		ret = 0;
		goto unlock;
	}
	pr_err("[zxdh_rdma] %s: get ram data failed !\n", __func__);
	ret = -EIO;

unlock:
	mutex_unlock(&zxdh_rdma_stats_ram_lock);
	return ret;
}

static int zxdh_set_cqp_ram_reg(struct zxdh_pci_f *rf, u32 ram_num,
				u32 ram_width, u32 ram_addr, u32 ram_read_cnt)
{
	int ret;
	size_t i;
	struct {
		u64 reg;
		u32 val;
	} reg_vals[] = { { C_RDMACQP_RDRAM_NUM, ram_num },
			 { C_RDMACQP_RDRAM_DATA_WIDTH, ram_width },
			 { C_RDMACQP_RDRAM_TIME_LIMIT,
			   IRDMARX_RD_TIME_LIMIT_VALUE },
			 { C_RDMACQP_RDRAM_ADDR, ram_addr },
			 { C_RDMACQP_RDRAM_READ_LENGTH, ram_read_cnt },
			 { C_RDMACQP_RDRAM_READ_FLAG, 0 } };
	for (i = 0; i < sizeof(reg_vals) / sizeof(reg_vals[0]); i++) {
		ret = zxdh_rdma_reg_write(rf, reg_vals[i].reg, reg_vals[i].val);
		if (ret) {
			pr_err("[zxdh_rdma] Error: ret=%d, failed to write rdma cqp read ram reg 0x%llx!\n",
			       ret, reg_vals[i].reg);
			return ret;
		}
	}
	return 0;
}

static int zxdh_read_cqp_ram_flag(struct zxdh_pci_f *rf)
{
	int ret;
	u32 val, err_reg, cnt_err_reg;
	udelay(1000);
	ret = zxdh_rdma_reg_read(rf, C_RDMACQP_RDRAM_RD_FINISH, &val);
	if (ret) {
		udelay(2000);
		ret = zxdh_rdma_reg_read(rf, C_RDMACQP_RDRAM_RD_FINISH, &val);
		if (ret) {
			pr_err("[zxdh_rdma] Error: ret=%d, failed to read rdma cqp ram reg 0x%lx!\n",
			       ret, C_RDMACQP_RDRAM_RD_FINISH);
			return -EIO;
		}
	}
	ret = zxdh_rdma_reg_read(rf, C_RDMACQP_RDRAM_RD_ERROR, &err_reg);
	if (ret) {
		pr_err("[zxdh_rdma] Error: ret=%d, failed to read rdma cqp ram reg 0x%lx!\n",
		       ret, C_RDMACQP_RDRAM_RD_ERROR);
		return -EIO;
	}
	ret = zxdh_rdma_reg_read(rf, C_RDMACQP_RDRAM_RD_CNT_ERR, &cnt_err_reg);
	if (ret) {
		pr_err("[zxdh_rdma] Error: ret=%d, failed to read rdma cqp ram reg 0x%lx!\n",
		       ret, C_RDMACQP_RDRAM_RD_CNT_ERR);
		return -EIO;
	}
	if (val == 1 && !err_reg && !cnt_err_reg)
		return 0;
	pr_err("[zxdh_rdma] Error: cqp read ram failed, reg 0x%lx, value: 0x%x; reg 0x%lx, value 0x%x",
	       C_RDMACQP_RDRAM_RD_ERROR, err_reg, C_RDMACQP_RDRAM_RD_CNT_ERR,
	       cnt_err_reg);
	return -1;
}

int zxdh_read_ram_cqp_values(struct zxdh_sc_dev *dev,
			     struct read_ram_info *ram_info, u32 *value)
{
	u32 check_ram_id, check_ram_addr;
	int ret;
	int i, offset_idx;
	struct zxdh_pci_f *rf = container_of(dev, struct zxdh_pci_f, sc_dev);

	mutex_lock(&zxdh_cqp_ram_access_lock);

	for (i = 0; i < ZXDH_RAM_REPEAT_READ_CNT; i++) {
		ret = zxdh_set_cqp_ram_reg(rf, ram_info->ram_num,
					   ram_info->ram_width,
					   ram_info->ram_addr,
					   ram_info->ram_read_cnt);
		if (ret)
			continue;
		ret = zxdh_read_cqp_ram_flag(rf);
		if (ret) {
			continue;
		}
		ret = zxdh_rdma_reg_read(rf, C_RDMACQP_RDRAM_NUM,
					 &check_ram_id);
		if (ret) {
			pr_err("[zxdh_rdma] Error: read cqp reg 0x%lx failed!\n",
			       C_RDMACQP_RDRAM_NUM);
			ret = -EIO;
			goto unlock;
		}
		ret = zxdh_rdma_reg_read(rf, C_RDMACQP_RDRAM_ADDR,
					 &check_ram_addr);
		if (ret) {
			pr_err("[zxdh_rdma] Error: read cqp reg 0x%lx failed!\n",
			       C_RDMACQP_RDRAM_ADDR);
			ret = -EIO;
			goto unlock;
		}
		if ((check_ram_id != ram_info->ram_num) ||
		    (check_ram_addr != ram_info->ram_addr)) {
			pr_err("[zxdh_rdma] %s: get ram data failed! ram_num:0x%x, rdma_addr:0x%x\n",
			       __func__, check_ram_id, check_ram_addr);
			ret = -ERANGE;
			goto unlock;
		}
		for (offset_idx = 0; offset_idx < ram_info->offset_idx;
		     offset_idx++) {
			ret = zxdh_rdma_reg_read(
				rf,
				C_RDMACQP_RDRAM_RD_MAINTENANCE_RAM(offset_idx),
				value);
			if (ret) {
				ret = ret;
				goto unlock;
			}
			value++;
		}
		ret = 0;
		goto unlock;
	}
	pr_err("[zxdh_rdma] %s: get ram data failed !\n", __func__);
	ret = -EIO;

unlock:
	mutex_unlock(&zxdh_cqp_ram_access_lock);
	return ret;
}
