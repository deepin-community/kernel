#include "ps3_irq.h"

#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#endif

#include "ps3_instance_manager.h"
#include "ps3_inner_data.h"
#include "ps3_cmd_complete.h"
#include "ps3_ioc_manager.h"
#include "ps3_util.h"
#include "ps3_dump.h"
#include "ps3_scsih.h"

#ifndef _WINDOWS

const U32 PS3_INTERRUPT_STATUS_NO_IRQ = 0x00;
const U32 PS3_INTERRUPT_CMD_DISABLE_ALL_MASK = 0x02;
const U32 PS3_INTERRUPT_CMD_ENABLE_MSIX = 0x01;
const U32 PS3_INTERRUPT_MASK_DISABLE = 0x00000002;
const U32 PS3_INTERRUPT_STATUS_EXIST_IRQ = 0x00000001;
const U32 PS3_INTERRUPT_CLEAR_IRQ= 0x00000001;

const U32 PS3_SSD_IOPS_MSIX_VECTORS = 8;
const U32 PS3_HDD_IOPS_MSIX_VECTORS = 8;
const U32 PS3_IRQ_POLL_SCHED_THRESHOLD = 1024;
const U32 PS3_HIGH_IOPS_VECTOR_BATCH_COUNT_SHIFT = 5;
const U32 PS3_BALANCE_MODE_MIN_CPU_NUM = 16;
const S32 PS3_SSD_MAX_BUSY_THRESHOLD = 40;
const U32 PS3_IOPS_MSIX_VECTORS = 12;
const S32 PS3_HYGON_BUSY_ADJUST_THRESHOLD = 32;
const U32 PS3_MSIX_COMBINED_MODE_MSIX_VECTORS = 16;

#define PS3_IRQCTX_HOST(irq_context) \
	ps3_container_of((irq_context), struct ps3_instance, irq_context)->host->host_no

#define PS3_MGR_CMD_MSIX_INDEX(irq_context) ((irq_context)->high_iops_msix_vectors)
#define IS_HIGN_OPS_IRQ(irq_context, irq) \
	(((irq_context)->high_iops_msix_vectors > (irq)->isrSN))

#define PS3_MULTI_DATA_DISK_BUSY_THRESHOLD(var, base) ((var) > (base))
static void msix_irq_mode_check(struct ps3_irq_context *irq_context,
	U32 max_vectors)
{
	U32 online_cpu_num = num_online_cpus();
	U16 speed = 0;
	U16 lnksta = 0;
	U32 iops_msix_cnt = 0;

	if (!ps3_ioc_multi_func_support(irq_context->instance) ||
			ps3_get_pci_function(irq_context->instance->pdev) == PS3_FUNC_ID_1) {
		iops_msix_cnt = PS3_IOPS_MSIX_VECTORS;
	} else {
		iops_msix_cnt = 0;
	}

	pcie_capability_read_word(irq_context->instance->pdev, PCI_EXP_LNKSTA, &lnksta);
	speed = lnksta & PCI_EXP_LNKSTA_CLS;
	if ((iops_msix_cnt > 0) && (online_cpu_num >= PS3_BALANCE_MODE_MIN_CPU_NUM) &&
		(iops_msix_cnt < max_vectors) && (speed >= 0x4)) {
		irq_context->high_iops_msix_vectors = iops_msix_cnt;
		irq_context->valid_msix_vector_count =
			min(irq_context->high_iops_msix_vectors + online_cpu_num, max_vectors);
		irq_context->is_support_balance = PS3_TRUE;
		irq_context->is_balance_current_perf_mode = PS3_TRUE;
	} else {
		irq_context->high_iops_msix_vectors = 0;
		irq_context->is_support_balance = PS3_FALSE;
		irq_context->is_balance_current_perf_mode = PS3_FALSE;
		irq_context->valid_msix_vector_count =
			min(online_cpu_num, max_vectors);
	}

	return;
}

static U32 __ps3_irq_vectors_alloc(struct pci_dev *pdev,
	struct ps3_irq_context *irq_context)
{
	int msix_vectors = 0;
	U32 irq_flags = PCI_IRQ_MSIX | PCI_IRQ_MSI;
#if defined(PS3_OS_MANAGED_IRQ_SUPPORT)
	struct irq_affinity desc = { .pre_vectors = irq_context->high_iops_msix_vectors };
	struct irq_affinity *descp = &desc;
#endif
	struct ps3_instance *instance = irq_context->instance;
	U32 ps3_irq_mode = ps3_pci_irq_mode_query();

	switch (ps3_irq_mode) {
		case PS3_PCI_IRQ_MODE_LEGACY:
			irq_flags = PCI_IRQ_LEGACY;
			break;
		case PS3_PCI_IRQ_MODE_MSI:
			irq_flags = PCI_IRQ_MSI;
			break;
		case PS3_PCI_IRQ_MODE_MSIX:
			irq_flags = PCI_IRQ_MSIX;
			break;
		default:
			break;
	}

#if ((defined(DRIVER_SUPPORT_KERNEL_IRQ_AFFINITY)) || (!defined(PS3_OS_MANAGED_IRQ_SUPPORT)))
	if (irq_context->instance->smp_affinity_enable) {
		irq_flags |= PCI_IRQ_AFFINITY;
	}
#if defined(PS3_OS_MANAGED_IRQ_SUPPORT)
	else {
		descp = NULL;
	}
#endif
#else
	if (irq_context->is_support_balance == PS3_TRUE &&
		irq_context->is_balance_current_perf_mode == PS3_TRUE) {
		descp = NULL;
	} else {
		if (irq_context->instance->smp_affinity_enable) {
			irq_flags |= PCI_IRQ_AFFINITY;
		} else {
			descp = NULL;
		}
	}
#endif

	LOG_INFO("host_no:%u pci_irq_mode:%d specified, msix_vectors:%d max:%d\n",
		PS3_IRQCTX_HOST(irq_context), ps3_irq_mode,
		irq_context->high_iops_msix_vectors,
		irq_context->valid_msix_vector_count);
#if defined(PS3_OS_MANAGED_IRQ_SUPPORT)
	msix_vectors = pci_alloc_irq_vectors_affinity(pdev,
		max(instance->min_intr_count, irq_context->high_iops_msix_vectors),
		irq_context->valid_msix_vector_count, irq_flags, descp);
#else
	msix_vectors = pci_alloc_irq_vectors(pdev,
		max(instance->min_intr_count, irq_context->high_iops_msix_vectors),
		irq_context->valid_msix_vector_count, irq_flags);
#endif

	if (msix_vectors <= 0) {
		LOG_WARN("host_no:%u alloc msi irq fail! msix_vectors:%d\n",
			PS3_IRQCTX_HOST(irq_context), msix_vectors);
		msix_vectors = 0;
	} else {
		if (pdev->msix_enabled == 1) {
			irq_context->pci_irq_type = PS3_PCI_IRQ_MSIX;
			LOG_DEBUG("host_no:%u alloc msix count:%d\n",
				PS3_IRQCTX_HOST(irq_context), msix_vectors);
		} else if (pdev->msi_enabled == 1) {
			irq_context->pci_irq_type = PS3_PCI_IRQ_MSI;
			LOG_DEBUG("host_no:%u alloc msi count:%d\n",
				PS3_IRQCTX_HOST(irq_context), msix_vectors);
		} else {
			LOG_ERROR("host_no:%u msi/msix all disable, msix_vectors:%d\n",
				PS3_IRQCTX_HOST(irq_context), msix_vectors);
		}
	}

	return (U32)msix_vectors;
}

static void ps3_iops_vectors_affinity_hint_set(struct pci_dev *pdev,
	struct ps3_irq_context *irq_context)
{
#ifndef DRIVER_SUPPORT_KERNEL_IRQ_AFFINITY
	(void)pdev;
	(void)irq_context;
	goto l_out;
#else
	U32 msix_vector = 0;
	U32 os_vector = 0;
	S32 node = -1;
	const struct cpumask* mask = NULL;
	S32 cpu_id = 0;

	if (!irq_context->is_support_balance) {
		goto l_out;
	}

	if (!irq_context->instance->smp_affinity_enable) {
		goto l_out;
	}

	node = dev_to_node(&pdev->dev);
	if (node >= 0) {
		mask = cpumask_of_node(node);
	}

	if (mask == NULL) {
		mask = cpu_online_mask;
	}

	for_each_cpu_and(cpu_id, mask, cpu_online_mask) {
		LOG_DEBUG("host_no:%u affinity_hint cpu_id:%d, node:%d \n",
			PS3_IRQCTX_HOST(irq_context), cpu_id, node);
	}

	for (msix_vector = 0;
		msix_vector < irq_context->high_iops_msix_vectors;
		msix_vector++) {
		os_vector = pci_irq_vector(pdev, msix_vector);
		irq_set_affinity_hint(os_vector, mask);
	}
#endif
l_out:
	return;
}

static U32 ps3_irq_vectors_alloc(struct pci_dev *pdev,
	struct ps3_irq_context *irq_context,
	U32 max_vectors)
{
	U32 msix_vectors = 0;
	S32 nr_entries = 0;
	msix_irq_mode_check(irq_context, max_vectors);

	msix_vectors = __ps3_irq_vectors_alloc(pdev, irq_context);
	if ((msix_vectors != irq_context->valid_msix_vector_count) &&
		(irq_context->is_support_balance)) {
		pci_free_irq_vectors(pdev);
		irq_context->is_support_balance = PS3_FALSE;
		irq_context->is_balance_current_perf_mode = PS3_FALSE;
		irq_context->high_iops_msix_vectors = 0;
		irq_context->valid_msix_vector_count =
			min(num_online_cpus(), max_vectors);
		msix_vectors = __ps3_irq_vectors_alloc(pdev, irq_context);
		LOG_DEBUG("host_no:%u alloc resource for normal perf mode! \n",
			PS3_IRQCTX_HOST(irq_context));
	}
	INJECT_START(PS3_ERR_IJ_IRQ_VECTORS_ALLOC_FAILED, &msix_vectors)
	if (msix_vectors > 0) {
		irq_context->valid_msix_vector_count = msix_vectors;
		ps3_iops_vectors_affinity_hint_set(pdev, irq_context);
	} else {
		irq_context->high_iops_msix_vectors = 0;
		irq_context->is_support_balance = PS3_FALSE;
		irq_context->is_balance_current_perf_mode = PS3_FALSE;
		nr_entries = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_LEGACY);
		INJECT_START(PS3_ERR_IJ_ALLOC_LEGACY_VECTOR_FAILED, &nr_entries)
		if (nr_entries != 1) {
			LOG_ERROR("host_no:%u alloc irq fail! \n",
				PS3_IRQCTX_HOST(irq_context));
			irq_context->valid_msix_vector_count = 0;
		} else {
			irq_context->valid_msix_vector_count = 1;
			irq_context->pci_irq_type = PS3_PCI_IRQ_LEGACY;
			LOG_DEBUG("host_no:%u alloc legacy irq\n",
				PS3_IRQCTX_HOST(irq_context));
		}
	}
	return irq_context->valid_msix_vector_count;
}

static inline Bool ps3_irq_set_affinity_hint(struct pci_dev *pdev,
	U32 msix_vector, const struct cpumask *mask, Bool smp_affinity_enable) {
	U32 os_vector = pci_irq_vector(pdev, msix_vector);
	Bool ret = PS3_TRUE;

	if (smp_affinity_enable) {
		if (irq_set_affinity_hint(os_vector, mask)) {
			ret = PS3_FALSE;
		}
	}
	return ret;
}

static void __ps3_cpu_msix_table_init(struct pci_dev *pdev,
	struct ps3_irq_context *irq_context)
{
	const struct cpumask *mask = NULL;
	S32 cpu_id = 0;
	U32 normal_msix_index = irq_context->high_iops_msix_vectors;
	Bool smp_affinify_enable = irq_context->instance->smp_affinity_enable;

	for (; normal_msix_index < irq_context->valid_msix_vector_count;
		normal_msix_index++) {
		mask = pci_irq_get_affinity(pdev, normal_msix_index);
		if (mask == NULL) {
			goto l_get_affinity_failed;
		}

		for_each_cpu_and(cpu_id, mask, cpu_online_mask) {
			if (cpu_id >= irq_context->cpu_msix_table_sz) {
				break;
			}
			irq_context->cpu_msix_table[cpu_id] = normal_msix_index;
			LOG_DEBUG("host_no:%u affinity cpu_id:%d, msix_index:%d \n",
				PS3_IRQCTX_HOST(irq_context), cpu_id, normal_msix_index);
		}
	}
	return;

l_get_affinity_failed:
	cpu_id = cpumask_first(cpu_online_mask);
	normal_msix_index = irq_context->high_iops_msix_vectors;
	for (; normal_msix_index < irq_context->valid_msix_vector_count;
		normal_msix_index++) {
		mask = get_cpu_mask(cpu_id);
		if (!ps3_irq_set_affinity_hint(pdev, normal_msix_index,
			mask, smp_affinify_enable)) {
			LOG_ERROR("host_no:%u set affinity failed, %d.\n",
				PS3_IRQCTX_HOST(irq_context), normal_msix_index);
		} else {
			irq_context->cpu_msix_table[cpu_id] = normal_msix_index;
			LOG_DEBUG("host_no:%u affinity cpu_id:%d, msix_index:%d \n",
					PS3_IRQCTX_HOST(irq_context), cpu_id, normal_msix_index);
		}
		cpu_id = cpumask_next(cpu_id, cpu_online_mask);
	}
	return;
}

static S32 ps3_cpu_msix_table_init(struct pci_dev *pdev,
	struct ps3_irq_context *irq_context)
{
	S32 last_cpu_id = 0;
	S32 cpu_id = 0;
	for_each_online_cpu(cpu_id) {
		last_cpu_id = cpu_id;
	}
	irq_context->cpu_msix_table_sz = last_cpu_id + 1;

	irq_context->cpu_msix_table =
		(U32*)kcalloc(irq_context->cpu_msix_table_sz, sizeof(S32), GFP_KERNEL);
	INJECT_START(PS3_ERR_IJ_CPU_MSIX_TABLE_ALLOC_FAILED, &(irq_context->cpu_msix_table))
	if (irq_context->cpu_msix_table == NULL) {
		LOG_ERROR("host_no:%u kcalloc fail! \n",
			PS3_IRQCTX_HOST(irq_context));
		return -PS3_FAILED;
	}

	__ps3_cpu_msix_table_init(pdev, irq_context) ;

	return PS3_SUCCESS;
}

static S32 ps3_reply_fifo_desc_alloc(struct pci_dev *pdev,
	struct ps3_irq_context *irq_context)
{
	size_t reply_fifo_desc_buf_size = sizeof(struct PS3ReplyFifoDesc) *
		irq_context->valid_msix_vector_count;
	irq_context->reply_fifo_desc_buf_pool = (struct dma_pool *)
		ps3_dma_pool_create("PS3 reply fifo desc pool",
			&pdev->dev, reply_fifo_desc_buf_size,
			sizeof(struct PS3ReplyFifoDesc), 0);
	INJECT_START(PS3_ERR_IJ_REPLY_FIFO_DESC_BUF_POOL_ALLOC_FAILED, &(irq_context->reply_fifo_desc_buf_pool))
	if (irq_context->reply_fifo_desc_buf_pool == NULL) {
		LOG_ERROR("host_no:%u ps3_dma_pool_create fail! \n",
			PS3_IRQCTX_HOST(irq_context));
		goto l_desc_buf_pool_failed;
	}

	irq_context->reply_fifo_desc_buf = (struct PS3ReplyFifoDesc*)
		ps3_dma_pool_zalloc(irq_context->instance, irq_context->reply_fifo_desc_buf_pool,
			GFP_KERNEL,
			&irq_context->reply_fifo_desc_buf_phys);
	INJECT_START(PS3_ERR_IJ_REPLY_FIFO_DESC_BUF_ALLOC_FAILED, &(irq_context->reply_fifo_desc_buf))
	if (irq_context->reply_fifo_desc_buf == NULL) {
		LOG_ERROR("host_no:%u ps3_dma_pool_create fail! \n",
			PS3_IRQCTX_HOST(irq_context));
		goto l_desc_buf_alloc_failed;
	}

	return PS3_SUCCESS;

l_desc_buf_alloc_failed:
	ps3_dma_pool_destroy(irq_context->reply_fifo_desc_buf_pool);
	irq_context->reply_fifo_desc_buf_pool = NULL;
l_desc_buf_pool_failed:
	return -PS3_FAILED;
}

static void ps3_reply_fifo_desc_free(struct ps3_irq_context *irq_context)
{
	if (irq_context->reply_fifo_desc_buf != NULL) {
		ps3_dma_pool_free(
			irq_context->reply_fifo_desc_buf_pool,
			irq_context->reply_fifo_desc_buf,
			irq_context->reply_fifo_desc_buf_phys);
			irq_context->reply_fifo_desc_buf = NULL;
			irq_context->reply_fifo_desc_buf_phys = 0;
	}

	if (irq_context->reply_fifo_desc_buf_pool != NULL) {
		ps3_dma_pool_destroy(irq_context->reply_fifo_desc_buf_pool);
		irq_context->reply_fifo_desc_buf_pool = NULL;
	}

	return;
}

static S32 __ps3_reply_fifo_alloc(struct pci_dev *pdev,
	struct ps3_irq_context *irq_context)
{
	struct PS3ReplyWord **virt_base = irq_context->reply_fifo_virt_base_addr_buf;
	dma_addr_t *phys_base = irq_context->reply_fifo_phys_base_addr_buf;
	size_t reply_fifo_size = sizeof(struct PS3ReplyWord) *
		irq_context->reply_fifo_depth;
	U32 i = 0;
	(void)pdev;

	for (; i < irq_context->valid_msix_vector_count; i++) {
		virt_base[i] = (struct PS3ReplyWord *)
			ps3_dma_pool_alloc(irq_context->instance, irq_context->reply_fifo_pool,
			GFP_KERNEL,
			&phys_base[i]);
		INJECT_START(PS3_ERR_IJ_REPLY_VIRT_BASE_ALLOC_FAILED, &(virt_base[i]))
		if (virt_base[i] == NULL) {
			LOG_ERROR("host_no:%u ps3_dma_pool_zalloc fail! \n",
				PS3_IRQCTX_HOST(irq_context));
			goto l_alloc_failed;
		}
		memset(virt_base[i], 0xff, reply_fifo_size);
		ps3_get_so_addr_ranger(irq_context->instance, phys_base[i], reply_fifo_size);
		LOG_DEBUG("host_no:%u reply_fifo index:%u phy addr:0x%llx! \n",
				PS3_IRQCTX_HOST(irq_context), i, phys_base[i]);
	}

	return PS3_SUCCESS;

l_alloc_failed:
	for (; i > 0; i--) {
		ps3_dma_pool_free(
			irq_context->reply_fifo_pool,
			virt_base[i-1], phys_base[i-1]);
		virt_base[i-1] = NULL;
		phys_base[i-1] = 0;
	}
	return -PS3_FAILED;
}

static S32 ps3_reply_fifo_alloc(struct pci_dev *pdev,
	struct ps3_irq_context *irq_context)
{
	size_t reply_fifo_size = sizeof(struct PS3ReplyWord) *
		irq_context->reply_fifo_depth;
	irq_context->reply_fifo_pool =
		ps3_dma_pool_create("PS3 reply fifo pool",
		&pdev->dev, reply_fifo_size, DMA_ALIGN_BYTES_4K, 0);
	INJECT_START(PS3_ERR_IJ_REPLY_FIFO_POOL_ALLOC_FAILED,& (irq_context->reply_fifo_pool))
	if (irq_context->reply_fifo_pool == NULL) {
		LOG_ERROR("host_no:%u ps3_dma_pool_create fail! \n",
			PS3_IRQCTX_HOST(irq_context));
		goto l_reply_fifo_pool_failed;
	}

	if (__ps3_reply_fifo_alloc(pdev, irq_context) != PS3_SUCCESS) {
		goto l_reply_fifo_alloc_failed;
	}

	return PS3_SUCCESS;

l_reply_fifo_alloc_failed:
	ps3_dma_pool_destroy(irq_context->reply_fifo_pool);
	irq_context->reply_fifo_pool = NULL;
l_reply_fifo_pool_failed:
	return -PS3_FAILED;
}

static void ps3_reply_fifo_free(struct ps3_irq_context *irq_context)
{
	struct PS3ReplyWord **virt_base = irq_context->reply_fifo_virt_base_addr_buf;
	dma_addr_t *phys_base = irq_context->reply_fifo_phys_base_addr_buf;
	U32 i = 0;

	if (irq_context->reply_fifo_pool == NULL) {
		goto l_out;
	}

	for (; i < irq_context->valid_msix_vector_count; i++) {

		if (virt_base[i] == NULL) {
			continue;
		}

		ps3_dma_pool_free(
			irq_context->reply_fifo_pool,
			virt_base[i], phys_base[i]);
		virt_base[i] = NULL;
		phys_base[i] = 0;
	}

	ps3_dma_pool_destroy(irq_context->reply_fifo_pool);
	irq_context->reply_fifo_pool = NULL;
l_out:
	return;
}

static S32 ps3_irq_resource_alloc(struct pci_dev *pdev,
	struct ps3_irq_context *irq_context)
{
	if (ps3_reply_fifo_desc_alloc(pdev, irq_context) != PS3_SUCCESS) {
		goto l_desc_alloc_failed;
	}

	if (ps3_reply_fifo_alloc(pdev, irq_context) != PS3_SUCCESS) {
		goto l_reply_fifo_alloc_failed;
	}

	return PS3_SUCCESS;

l_reply_fifo_alloc_failed:
	ps3_reply_fifo_desc_free(irq_context);
l_desc_alloc_failed:
	return -PS3_FAILED;
}

static void ps3_irq_resource_free(struct ps3_irq_context *irq_context)
{
	ps3_reply_fifo_free(irq_context);
	ps3_reply_fifo_desc_free(irq_context);

	return;
}

static void ps3_reply_fifo_desc_set(struct pci_dev *pdev,
	struct ps3_irq_context *irq_context)
{
	struct PS3ReplyFifoDesc *desc_buf = irq_context->reply_fifo_desc_buf;
	dma_addr_t *phys_base = irq_context->reply_fifo_phys_base_addr_buf;
	U32 i = 0;
	U32 iops_msix_cnt = 0;

	if (ps3_get_pci_function(irq_context->instance->pdev) == PS3_FUNC_ID_1) {
		iops_msix_cnt = PS3_IOPS_MSIX_VECTORS;
	} else {
		iops_msix_cnt = PS3_IOPS_MSIX_VECTORS >> 1;
	}

	for (; i < irq_context->valid_msix_vector_count; i++) {
		desc_buf[i].ReplyFifoBaseAddr = cpu_to_le64(phys_base[i]);
		desc_buf[i].irqNo = cpu_to_le32(pci_irq_vector(pdev, i));
		desc_buf[i].depthReplyFifo = irq_context->reply_fifo_depth;

		if (irq_context->is_support_balance &&
				i < irq_context->high_iops_msix_vectors) {
			if (i < iops_msix_cnt) {
				desc_buf[i].isrAccMode = PS3_ISR_ACC_MODE_IOPS_VER0;
			}
		}
		else {
			desc_buf[i].isrAccMode = PS3_ISR_ACC_MODE_LATENCY;
		}
	}

	return;
}

static inline void __ps3_irqs_exit(struct ps3_irq *irq, struct ps3_irq_context *irq_context)
{
	irq_set_affinity_hint(irq->irqNo, NULL);

	if (irq->is_irq_poll_disabled == PS3_FALSE) {
		ps3_irq_poll_disable(&irq->irqpoll);
		irq->is_irq_poll_disabled = PS3_TRUE;
	} else {
		LOG_INFO("host_no:%u irq poll(%u) already disabled!\n",
				PS3_HOST(irq_context->instance), irq->isrSN);
	}

	free_irq(irq->irqNo, irq);
}

S32 ps3_irqs_init(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct PS3ReplyWord **virt_base = irq_context->reply_fifo_virt_base_addr_buf;
	struct pci_dev *pdev = instance->pdev;
	struct ps3_irq *irqs = NULL;
	U32 i = 0;
    U32 dump_irq_index;
	S32	retval;
	irq_context->irqs = (struct ps3_irq*)
		ps3_kcalloc(instance, irq_context->valid_msix_vector_count,
		sizeof(struct ps3_irq));
	INJECT_START(PS3_ERR_IJ_IRQS_ALLOC_FAILED, &(irq_context->irqs))
	if (irq_context->irqs == NULL) {
		LOG_ERROR("host_no:%u kcalloc fail! \n",
			PS3_IRQCTX_HOST(irq_context));
		goto l_out;
	}
	irqs = irq_context->irqs;

	for (i=0; i < irq_context->valid_msix_vector_count; i++) {
		irqs[i].irqNo = pci_irq_vector(pdev, i);
		irqs[i].isrSN = i;
		irqs[i].reply_fifo_virt_base_addr = virt_base[i];
		irqs[i].instance = instance;
		irqs[i].irq_poll_sched_threshold = PS3_IRQ_POLL_SCHED_THRESHOLD;
		irqs[i].is_irq_poll_disabled = PS3_FALSE;
		irqs[i].is_sched_irq_poll = PS3_FALSE;
		irqs[i].is_enable_irq = PS3_TRUE;
		atomic_set(&irqs[i].is_busy, 0);
		irqs[i].last_reply_idx = 0;
		snprintf(irqs[i].name, PS3_IRQ_NAME_LENGTH, "ps3_irq_%d_%d",
			instance->host->host_no, i);
		retval = request_irq(irqs[i].irqNo, instance->ioc_adpter->isr,
			IRQF_SHARED, irqs[i].name, &(irqs[i]));
		INJECT_START(PS3_ERR_IJ_REPLY_REQ_IRQS_FAILED, &retval)
		if (retval) {
			LOG_ERROR("host_no:%u request_irq failed! SN:%d\n",
				PS3_HOST(instance), i);

			goto l_failed;
		}

		ps3_irq_poll_init(&irqs[i].irqpoll, PS3_IRQ_POLL_SCHED_THRESHOLD,
			ps3_irqpoll_service);
	}

    dump_irq_index = PS3_MGR_CMD_MSIX_INDEX(irq_context);
	retval = request_irq(irqs[dump_irq_index].irqNo, ps3_dump_irq_handler, IRQF_SHARED, "ps3_dump_irq", (void *)instance);
	INJECT_START(PS3_ERR_IJ_DUMP_REQ_IRQS_FAILED, &retval)
    if (retval) {
        LOG_ERROR("host_no:%u request dump irq failed! SN:%d\n",
				PS3_HOST(instance), dump_irq_index);
        goto l_failed;
    }
	irq_context->dump_isrSN = dump_irq_index;

	instance->ioc_adpter->irq_disable(instance);

	return PS3_SUCCESS;
l_failed:
	for (; i > 0; i--) {
		__ps3_irqs_exit(&irqs[i-1], irq_context);
	}

	if (irq_context->irqs != NULL) {
		ps3_kfree(instance, irq_context->irqs);
		irq_context->irqs = NULL;
	}
l_out:
	return -PS3_FAILED;
}

S32 ps3_irqs_init_switch(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct PS3ReplyWord **virt_base = irq_context->reply_fifo_virt_base_addr_buf;
	struct pci_dev *pdev = instance->pdev;
	struct ps3_irq *irqs = NULL;
	struct ps3_irq_recovery *irqs_recovery = NULL;
	U32 i = 0;
	U32 dump_irq_index;
	U32 watch_irq_index = PS3_SWITCH_IRQ_INDEX;

	irq_context->irqs = (struct ps3_irq*)
		ps3_kcalloc(instance, irq_context->valid_msix_vector_count,
		sizeof(struct ps3_irq));
	if (irq_context->irqs == NULL) {
		LOG_ERROR("host_no:%u kcalloc fail! \n",
			PS3_IRQCTX_HOST(irq_context));
		goto l_out;
	}
	irqs = irq_context->irqs;

	for (i=0; i < irq_context->valid_msix_vector_count; i++) {
		irqs[i].irqNo = pci_irq_vector(pdev, i);
		irqs[i].isrSN = i;
		irqs[i].reply_fifo_virt_base_addr = virt_base[i];
		irqs[i].instance = instance;
		irqs[i].irq_poll_sched_threshold = PS3_IRQ_POLL_SCHED_THRESHOLD;
		irqs[i].is_irq_poll_disabled = PS3_FALSE;
		irqs[i].is_sched_irq_poll = PS3_FALSE;
		irqs[i].is_enable_irq = PS3_TRUE;
		atomic_set(&irqs[i].is_busy, 0);
		irqs[i].last_reply_idx = 0;
		snprintf(irqs[i].name, PS3_IRQ_NAME_LENGTH, "ps3_irq_%d_%d",
			instance->host->host_no, i);

		if (request_irq(irqs[i].irqNo, instance->ioc_adpter->isr,
			IRQF_SHARED, irqs[i].name, &(irqs[i]))) {
			LOG_ERROR("host_no:%u request_irq failed! SN:%d\n",
				PS3_HOST(instance), i);

			goto l_failed;
		}

		ps3_irq_poll_init(&irqs[i].irqpoll, PS3_IRQ_POLL_SCHED_THRESHOLD,
			ps3_irqpoll_service);
	}

    dump_irq_index = PS3_MGR_CMD_MSIX_INDEX(irq_context);
    if (request_irq(irqs[dump_irq_index].irqNo, ps3_dump_irq_handler, IRQF_SHARED, "ps3_dump_irq", (void *)instance)) {
        LOG_ERROR("host_no:%u request dump irq failed! SN:%d\n",
				PS3_HOST(instance), dump_irq_index);
        goto l_failed;
    }
	irq_context->dump_isrSN = dump_irq_index;

	irq_context->irq_recovery = (struct ps3_irq_recovery*)
		ps3_kcalloc(instance, 1,
		sizeof(struct ps3_irq_recovery));
	if (irq_context->irq_recovery == NULL) {
		LOG_ERROR("host_no:%u kcalloc irq_recovery fail! \n",
			PS3_IRQCTX_HOST(irq_context));
		goto l_free_dump;
	}
	irqs_recovery = irq_context->irq_recovery;
	irqs_recovery->irqNo = pci_irq_vector(pdev, 0);
	irqs_recovery->isrSN = watch_irq_index;
	irqs_recovery->instance = instance;

	if (request_irq(irqs_recovery[watch_irq_index].irqNo, ps3_recovery_irq_handler,
			IRQF_SHARED, "ps3_watchdog_irq", (void *)irqs_recovery)) {
		LOG_ERROR("host_no:%u request watchdog irq failed! SN:%d\n",
				PS3_HOST(instance), watch_irq_index);
		goto l_free_dump;
	}
	instance->ioc_adpter->irq_disable(instance);

	return PS3_SUCCESS;
l_free_dump:
	free_irq(irqs[dump_irq_index].irqNo, instance);
l_failed:
	for (; i > 0; i--) {
		__ps3_irqs_exit(&irqs[i-1], irq_context);
	}

	if (irq_context->irqs != NULL) {
		ps3_kfree(instance, irq_context->irqs);
		irq_context->irqs = NULL;
	}

	if (irq_context->irq_recovery != NULL) {
		ps3_kfree(instance, irq_context->irq_recovery);
		irq_context->irq_recovery = NULL;
	}
l_out:
	return -PS3_FAILED;
}

void ps3_irqs_exit(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct ps3_irq *irqs = irq_context->irqs;
	struct pci_dev *pdev = instance->pdev;
	U32 i = 0;
    U32 dump_irq_index;
	U32 watch_irq_index = PS3_SWITCH_IRQ_INDEX;
	struct ps3_irq_recovery * irq_recovery;

	if (irqs == NULL) {
		goto l_out;
	}

	dump_irq_index = PS3_MGR_CMD_MSIX_INDEX(irq_context);
	irq_set_affinity_hint(irqs[dump_irq_index].irqNo, NULL);
	free_irq(irqs[dump_irq_index].irqNo, instance);

	if (irq_context->irq_recovery != NULL) {
		irq_set_affinity_hint(irq_context->irq_recovery[watch_irq_index].irqNo, NULL);
		free_irq(irq_context->irq_recovery[watch_irq_index].irqNo, irq_context->irq_recovery);
		irq_recovery = irq_context->irq_recovery;
		irq_context->irq_recovery = NULL;
		ps3_kfree(instance, irq_recovery);
	}

	for (i=0; i < irq_context->valid_msix_vector_count; i++) {
		__ps3_irqs_exit(&irqs[i], irq_context);
	}

	irq_context->irqs = NULL;
	kfree(irqs);

	if (pdev->msix_enabled || pdev->msi_enabled) {
		pci_free_irq_vectors(pdev);
	}

l_out:
	return;
}

static Bool ps3_irq_max_vectors_calc(struct ps3_instance *instance,
	S32 *max_vectors)
{
	Bool ret = PS3_TRUE;
	U32 max_replyq_count = 0;

	if (!instance->ioc_adpter->max_replyq_count_get(instance, &max_replyq_count)) {
		ret = PS3_FALSE;
		goto l_out;
	}

	if (max_replyq_count > PS3_MSIX_COMBINED_MODE_MSIX_VECTORS) {
		instance->msix_combined = PS3_TRUE;
	}

	LOG_INFO("reqlyq max_replyq_count:%d\n", max_replyq_count);

	*max_vectors = pci_msix_vec_count(instance->pdev);
	INJECT_START(PS3_ERR_IJ_GET_MSIX_VEC_COUNT_INVALID, max_vectors)
	if (*max_vectors <= 0) {
		*max_vectors = pci_msi_vec_count(instance->pdev);
		INJECT_START(PS3_ERR_IJ_GET_MSIX_VEC_COUNT_INVALID, max_vectors)
		if (*max_vectors < 0) {
			*max_vectors = 0;
			goto l_out;
		}
	}
	*max_vectors = min_t(S32, *max_vectors, max_replyq_count);

l_out:
	return ret;
}

S32 ps3_irq_context_init(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct pci_dev *pdev = instance->pdev;
	S32 max_vectors = 0;
	if (!ps3_irq_max_vectors_calc(instance, &max_vectors)) {
		goto l_irq_vectors_alloc_failed;
	}

	if (!ps3_ioc_mgr_max_fw_cmd_get(instance,
			&irq_context->reply_fifo_depth)) {
		goto l_irq_vectors_alloc_failed;
	}

	irq_context->reply_fifo_depth += instance->reply_fifo_depth_addition;
	irq_context->instance = instance;

	LOG_INFO("max_vectors:%d replyQ_depth:%d\n",
		max_vectors, irq_context->reply_fifo_depth);

	if (max_vectors <= 0) {
		LOG_ERROR("host_no:%u IOC max_vectors invliad:%d \n",
			PS3_IRQCTX_HOST(irq_context), max_vectors);
		goto l_irq_vectors_alloc_failed;
	}

	if (!instance->msix_combined) {
		instance->smp_affinity_enable = PS3_FALSE;
	}

	if (ps3_irq_vectors_alloc(pdev, irq_context, max_vectors) <
		instance->min_intr_count) {
		LOG_ERROR("host_no:%u alloc irq NOK![cpunum:%d][irq_type:%d][min_intr:%d] \n",
			PS3_IRQCTX_HOST(irq_context), num_online_cpus(),
			irq_context->pci_irq_type, instance->min_intr_count);
		if (irq_context->valid_msix_vector_count > 0) {
			pci_free_irq_vectors(pdev);
			irq_context->high_iops_msix_vectors = 0;
			irq_context->valid_msix_vector_count = 0;
		}
		goto l_irq_vectors_alloc_failed;
	}

	if (ps3_cpu_msix_table_init(pdev, irq_context) != PS3_SUCCESS) {
		goto l_cpu_msix_table_init_failed;
	}

	if (ps3_irq_resource_alloc(pdev, irq_context) != PS3_SUCCESS) {
		goto l_irq_resource_alloc_failed;
	}

	ps3_reply_fifo_desc_set(pdev, irq_context);
	instance->is_support_irq = PS3_TRUE;

	return PS3_SUCCESS;

l_irq_resource_alloc_failed:
	kfree(irq_context->cpu_msix_table);
	irq_context->cpu_msix_table = NULL;
l_cpu_msix_table_init_failed:
	pci_free_irq_vectors(pdev);
	irq_context->high_iops_msix_vectors = 0;
	irq_context->valid_msix_vector_count = 0;
l_irq_vectors_alloc_failed:
	memset(irq_context, 0, sizeof(struct ps3_irq_context));
	return -PS3_FAILED;
}

S32 ps3_irq_context_exit(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct pci_dev *pdev = instance->pdev;

	ps3_irq_resource_free(irq_context);

	if (irq_context->cpu_msix_table != NULL) {
		kfree(irq_context->cpu_msix_table);
		irq_context->cpu_msix_table = NULL;
	}

	if (pdev->msix_enabled || pdev->msi_enabled) {
		pci_free_irq_vectors(pdev);
	}

	memset(irq_context, 0, sizeof(struct ps3_irq_context));
	return PS3_SUCCESS;
}

static inline Bool ps3_is_hdd_cmd(struct ps3_cmd *cmd)
{
	Bool is_hdd_io = PS3_FALSE;
	const struct PS3VDEntry *vd_entry = NULL;

	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		vd_entry = cmd->io_attr.vd_entry;
		if (!vd_entry->isNvme && !vd_entry->isSsd) {
			is_hdd_io = PS3_TRUE;
		}
	} else {
		if (ps3_is_hdd_pd(cmd->io_attr.pd_entry->dev_type)) {
			is_hdd_io = PS3_TRUE;
		}
	}

	return is_hdd_io;
}

static inline Bool ps3_sdev_is_high_load(struct ps3_cmd *cmd, U16 scale)
{
	S32 busy_base = cmd->instance->device_busy_threshold;
	S32 busy_threshold = busy_base * scale;
	S32 device_busy = 0;
#if defined DRIVER_SUPPORT_PRIV_BUSY
	struct ps3_scsi_priv_data *device_priv_data = (struct ps3_scsi_priv_data*)cmd->scmd->device->hostdata;
	if (device_priv_data == NULL) {
		return PS3_FALSE;
	}
	device_busy = atomic_read(&device_priv_data->sdev_priv_busy);
#else
	device_busy = atomic_read(&cmd->scmd->device->device_busy);
#endif

	if (PS3_MULTI_DATA_DISK_BUSY_THRESHOLD(busy_threshold, busy_base) && !ps3_is_hdd_cmd(cmd)) {
		if (ps3_host_vendor_get() == PS3_HOST_VENDOR_HYGON &&
			busy_threshold <= PS3_HYGON_BUSY_ADJUST_THRESHOLD) {
			busy_threshold = busy_base;
		} else if (busy_threshold > PS3_SSD_MAX_BUSY_THRESHOLD) {
			busy_threshold = PS3_SSD_MAX_BUSY_THRESHOLD;
		}
	}

	if (device_busy > busy_threshold) {
		return PS3_TRUE;
	}
	return PS3_FALSE;
}

static inline U32 ps3_iops_msix_index_get(struct ps3_irq_context *irq_context)
{
	U32 msix_index = 0;
	U32 ioc_count = atomic_add_return(1, &irq_context->high_iops_io_count);
	U32 batch_num = ioc_count >> PS3_HIGH_IOPS_VECTOR_BATCH_COUNT_SHIFT;

	msix_index = batch_num % PS3_IOPS_MSIX_VECTORS;
	return msix_index;
}

U32 ps3_msix_index_get(struct ps3_cmd *cmd, U16 scale)
{
	int processor_id = 0;
	U32 msix_index = 0;
	struct ps3_irq_context *irq_context = &cmd->instance->irq_context;

	if (irq_context->valid_msix_vector_count == 1) {
		msix_index = 0;
		goto l_out;
	}

	if (cmd->cmd_word.type == PS3_CMDWORD_TYPE_MGR) {
		msix_index = PS3_MGR_CMD_MSIX_INDEX(irq_context);
		goto l_out;
	}

	if (PS3_CMD_TYPE_IS_RW(cmd->cmd_word.type) &&
		(irq_context->is_balance_current_perf_mode) &&
		ps3_sdev_is_high_load(cmd, scale)) {
		msix_index = ps3_iops_msix_index_get(irq_context);
	} else if (cmd->instance->host->nr_hw_queues > 1) {
 		msix_index = blk_mq_unique_tag_to_hwq(blk_mq_unique_tag(SCMD_GET_REQUEST(cmd->scmd))) +
 			irq_context->high_iops_msix_vectors;
	}  else {
		processor_id = raw_smp_processor_id();
		msix_index = irq_context->cpu_msix_table[processor_id];

		if (msix_index == PS3_MGR_CMD_MSIX_INDEX(irq_context)) {
			msix_index++;
		}
	}

l_out:
	return msix_index;
}
void ps3_perf_update(struct ps3_instance *instance, U8 iocPerfMode)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	Bool support_balance = irq_context->is_support_balance;
	Bool *is_balance = &irq_context->is_balance_current_perf_mode;

	if (support_balance) {
		if (iocPerfMode == PS3_PERF_MODE_BALANCE) {
			*is_balance = PS3_TRUE;
		} else {
			*is_balance = PS3_FALSE;
		}
	} else {
		*is_balance = PS3_FALSE;
	}

	return;
}
void ps3_irqs_enable(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;

	LOG_DEBUG("host_no:%u irq enable\n", PS3_HOST(instance));

	switch (irq_context->pci_irq_type)
	{
	case PS3_PCI_IRQ_LEGACY:
		ps3_ioc_legacy_irqs_enable(instance);
		break;
	case PS3_PCI_IRQ_MSI:
		ps3_ioc_msi_enable(instance);
		break;
	case PS3_PCI_IRQ_MSIX:
		ps3_ioc_msix_enable(instance);
		break;
	default:
		LOG_ERROR("host_no:%u irq type is NOK :%d \n",
			PS3_HOST(instance), irq_context->pci_irq_type);
		break;
	}
	irq_context->is_enable_interrupts = PS3_DRV_TRUE;
	mb();
	return;
}

void ps3_irqs_disable(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;

	LOG_DEBUG("host_no:%u irq disable\n", PS3_HOST(instance));

	if(irq_context->is_enable_interrupts == PS3_DRV_FALSE) {
		goto l_out;
	}

	irq_context->is_enable_interrupts = PS3_DRV_FALSE;
	mb();
	switch (irq_context->pci_irq_type)
	{
	case PS3_PCI_IRQ_LEGACY:
		ps3_ioc_legacy_irqs_disable(instance);
		break;
	case PS3_PCI_IRQ_MSI:
		ps3_ioc_msi_disable(instance);
		break;
	case PS3_PCI_IRQ_MSIX:
		ps3_ioc_msix_disable(instance);
		break;
	default:
		LOG_ERROR("host_no:%u irq type is NOK :%d \n",
			PS3_HOST(instance), irq_context->pci_irq_type);
		break;
	}

l_out:
	return;
}

irqreturn_t ps3_irqs_service(int irq_so, void *priv)
{
	irqreturn_t ret = IRQ_NONE;
	int complete_num = 0;
	struct ps3_irq *irq = (struct ps3_irq *)priv;
	struct ps3_irq_context *irq_context = NULL;

	if (irq == NULL) {
		LOG_ERROR_IN_IRQ(irq->instance, "irq is null !\n");
		ret = IRQ_NONE;
		goto l_out;
	}
	irq_context = &irq->instance->irq_context;

	if ((U32)irq_so != irq->irqNo) {
		LOG_ERROR_IN_IRQ(irq->instance,
			"irq_so:%d != irq->irqNo:%d !\n",
			irq_so, irq->irqNo);
		ret = IRQ_NONE;
		goto l_out;
	}

	if (!irq_context->is_enable_interrupts) {
		LOG_INFO_IN_IRQ(irq->instance,
			"host_no:%u interrupt has disabled !\n",
			PS3_HOST(irq->instance));
		ret = IRQ_NONE;
		goto l_out;
	}

	if (irq->is_sched_irq_poll) {
		LOG_WARN_IN_IRQ(irq->instance,
			"host_no:%u had enter into irq poll !\n",
			PS3_HOST(irq->instance));
		ret = IRQ_HANDLED;
		goto l_out;
	}

	complete_num = ps3_cmd_complete(irq);
	if (complete_num <= 0) {
		LOG_DEBUG("host_no:%u there is no completed ps3_cmd !\n",
			PS3_HOST(irq->instance));
		ret = IRQ_NONE;
	} else {
		ret = IRQ_HANDLED;
	}

l_out:
	return ret;
}

S32 ps3_irqpoll_service(struct irq_poll *irqpoll, int budget)
{
	S32 complete_num = 0;
	struct ps3_irq *irq = ps3_container_of(irqpoll, struct ps3_irq, irqpoll);

	if (irq->is_enable_irq) {
		disable_irq(irq->irqNo);
		irq->is_enable_irq = PS3_DRV_FALSE;
	}

	complete_num = ps3_cmd_complete(irq);
	if (complete_num < budget) {
		ps3_irq_poll_complete(irqpoll);
		irq->is_sched_irq_poll = PS3_DRV_FALSE;
		enable_irq(irq->irqNo);
		irq->is_enable_irq = PS3_DRV_TRUE;
	}

	return complete_num;
}

void ps3_irqpolls_enable(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct ps3_irq *irqs = irq_context->irqs;
	U32 i = 0;

	for (; i < irq_context->valid_msix_vector_count; i++) {
		if (irqs[i].is_irq_poll_disabled == PS3_TRUE) {
			ps3_irq_poll_enable(&irqs[i].irqpoll);
			irqs[i].is_irq_poll_disabled = PS3_FALSE;
		} else {
			LOG_INFO("host_no:%u irq poll(%d) not disabled!\n",
					PS3_HOST(instance), i);
		}
	}

	return;
}

void ps3_irqs_sync(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct ps3_irq *irqs = irq_context->irqs;
	U32 i = 0;
	if (irqs == NULL) {
		goto l_out;
	}
	for (; i < irq_context->valid_msix_vector_count; i++) {
		synchronize_irq(irqs[i].irqNo);

		if (irqs[i].is_irq_poll_disabled == PS3_FALSE) {
			ps3_irq_poll_disable(&irqs[i].irqpoll);
			irqs[i].is_irq_poll_disabled = PS3_TRUE;
		} else {
			LOG_INFO("host_no:%u irq poll(%d) already disabled!\n",
					PS3_HOST(instance), i);
		}
	}
l_out:
	return;
}

#else

static void ps3_reply_fifo_desc_free(struct ps3_instance *instance);
static void ps3_reply_fifo_free(struct ps3_instance *instance);
static void ps3_irq_resource_free(struct ps3_instance *instance);

static Bool ps3_irq_max_vectors_calc(struct ps3_instance *instance,
	U32 *max_vectors)
{
	Bool ret = PS3_TRUE;
	U32 max_replyq_count = 0;

	if (!instance->ioc_adpter->max_replyq_count_get(instance,
			&max_replyq_count)) {
		ret = PS3_FALSE;
		goto l_out;
	}

	LOG_INFO("reqlyq max_replyq_count:%u\n", max_replyq_count);

	*max_vectors = instance->pci_dev_context.irq_vec_count;

	*max_vectors = PS3_MIN(*max_vectors, max_replyq_count);
	*max_vectors = PS3_MIN(*max_vectors, (U32)num_online_cpus() + 1);
l_out:
	return ret;
}

static S32 ps3_reply_fifo_desc_alloc(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;

	irq_context->reply_fifo_desc_buf_size = sizeof(struct PS3ReplyFifoDesc) *
		irq_context->valid_msix_vector_count;

	irq_context->reply_fifo_desc_buf = (struct PS3ReplyFifoDesc*)
		ps3_dma_alloc_coherent(instance,
			irq_context->reply_fifo_desc_buf_size,
			&irq_context->reply_fifo_desc_buf_phys);
	if (irq_context->reply_fifo_desc_buf == NULL) {
		LOG_ERROR("host_no:%u dma alloc fail! \n",
			PS3_HOST(instance));
		goto l_failed;
	}

	return PS3_SUCCESS;
l_failed:
	ps3_reply_fifo_desc_free(instance);
	return -PS3_FAILED;
}

static void ps3_reply_fifo_desc_free(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	if (irq_context->reply_fifo_desc_buf != NULL) {
		ps3_dma_free_coherent(instance,
			irq_context->reply_fifo_desc_buf_size,
			irq_context->reply_fifo_desc_buf,
			irq_context->reply_fifo_desc_buf_phys);

		irq_context->reply_fifo_desc_buf = NULL;
		irq_context->reply_fifo_desc_buf_phys = 0;
	}

	return;
}

static S32 ps3_reply_fifo_alloc(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct PS3ReplyWord **virt_base = irq_context->reply_fifo_virt_base_addr_buf;
	dma_addr_t *phys_base = irq_context->reply_fifo_phys_base_addr_buf;
	U32 i = 0;

	irq_context->reply_fifo_size = sizeof(struct PS3ReplyWord) *
		irq_context->reply_fifo_depth;

	for (; i < irq_context->valid_msix_vector_count; i++) {
		virt_base[i] = (struct PS3ReplyWord*)
			ps3_dma_alloc_coherent(instance,
				irq_context->reply_fifo_size,
				&phys_base[i]);
		if (virt_base[i] == NULL) {
			LOG_ERROR("host_no:%u ps3_dma_pool_zalloc fail! \n",
				PS3_HOST(instance));
			goto l_failed;
		}
		memset(virt_base[i], 0xff, irq_context->reply_fifo_size);
	}

	return PS3_SUCCESS;

l_failed:
	ps3_reply_fifo_free(instance);
	return -PS3_FAILED;
}

static void ps3_reply_fifo_free(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct PS3ReplyWord **virt_base = irq_context->reply_fifo_virt_base_addr_buf;
	dma_addr_t *phys_base = irq_context->reply_fifo_phys_base_addr_buf;
	U32 i = 0;

	for (; i < irq_context->valid_msix_vector_count; i++) {

		if (virt_base[i] == NULL) {
			continue;
		}

		ps3_dma_free_coherent(instance,
			irq_context->reply_fifo_size,
			virt_base[i],
			phys_base[i]);

		virt_base[i] = NULL;
		phys_base[i] = 0;
	}

	return;
}

static inline S32 ps3_irq_group_affinity_alloc(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	U32 size = sizeof(GROUP_AFFINITY) * irq_context->valid_msix_vector_count;

	irq_context->group_affinity = ps3_kzalloc(instance, size);
	if (irq_context->group_affinity == NULL) {
		LOG_ERROR("host_no:%u, group_affinity alloc failed\n", PS3_HOST(instance));
		return -PS3_FAILED;
	}
	return PS3_SUCCESS;
}

static inline void ps3_irq_group_affinity_free(struct ps3_instance *instance)
{
	if (instance->irq_context.group_affinity != NULL) {
		ps3_kfree(instance, instance->irq_context.group_affinity);
		instance->irq_context.group_affinity = NULL;
	}
}

static S32 ps3_irq_resource_alloc(struct ps3_instance *instance)
{
	if (ps3_reply_fifo_desc_alloc(instance) != PS3_SUCCESS) {
		goto l_failed;
	}

	if (ps3_reply_fifo_alloc(instance) != PS3_SUCCESS) {
		goto l_failed;
	}

	if (ps3_irq_group_affinity_alloc(instance) != PS3_SUCCESS) {
		goto l_failed;
	}

	return PS3_SUCCESS;

l_failed:
	ps3_irq_resource_free(instance);
	return -PS3_FAILED;
}

static void ps3_irq_resource_free(struct ps3_instance *instance)
{
	ps3_irq_group_affinity_free(instance);
	ps3_reply_fifo_free(instance);
	ps3_reply_fifo_desc_free(instance);

	return;
}

static void ps3_reply_fifo_desc_set(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct PS3ReplyFifoDesc *desc_buf = irq_context->reply_fifo_desc_buf;
	dma_addr_t *phys_base = irq_context->reply_fifo_phys_base_addr_buf;

	U32 i = 0;
	for (; i < irq_context->valid_msix_vector_count; i++) {
		desc_buf[i].ReplyFifoBaseAddr = cpu_to_le64(phys_base[i]);
		desc_buf[i].irqNo = i;
		desc_buf[i].depthReplyFifo = (U16)irq_context->reply_fifo_depth;

		if (i < irq_context->high_iops_msix_vectors) {
			desc_buf[i].isHighIops = PS3_TRUE;
		} else {
			desc_buf[i].isHighIops = PS3_FALSE;
		}
	}

	return;
}

S32 ps3_irq_context_init(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	U32 max_vectors = 0;
	if (!ps3_irq_max_vectors_calc(instance, &max_vectors)) {
		goto l_failed;
	}

	if (!ps3_ioc_mgr_max_fw_cmd_get(instance,
			&irq_context->reply_fifo_depth)) {
		goto l_failed;
	}

	irq_context->reply_fifo_depth += instance->reply_fifo_depth_addition;
	irq_context->instance = instance;

	irq_context->high_iops_msix_vectors = 0;

	LOG_INFO("max_vectors:%d replyQ_depth:%d\n",
		max_vectors, irq_context->reply_fifo_depth);

	if (max_vectors <= instance->min_intr_count) {
		LOG_ERROR("host_no:%u alloc irq failed![cpunum:%d][irq_type:%d][min_intr:%d] \n",
			PS3_HOST(instance), num_online_cpus(),
			irq_context->pci_irq_type, instance->min_intr_count);
		goto l_failed;
	}

	irq_context->pci_irq_type = instance->pci_dev_context.pci_irq_type;
	irq_context->valid_msix_vector_count = max_vectors;
	if (ps3_irq_resource_alloc(instance) != PS3_SUCCESS) {
		goto l_failed;
	}

	ps3_reply_fifo_desc_set(instance);

	return PS3_SUCCESS;

l_failed:
	ps3_irq_context_exit(instance);
	return -PS3_FAILED;
}

S32 ps3_irq_context_exit(struct ps3_instance *instance)
{
	ps3_irq_resource_free(instance);

	return PS3_SUCCESS;
}

U8 ps3_irqs_service(void *priv, ULong irq_no)
{
	struct ps3_instance *instance = (struct ps3_instance*)priv;
	U32 isrSN = (U32)irq_no;
	if (isrSN >= instance->irq_context.valid_msix_vector_count) {
		goto l_out;
	}

	if (!instance->irq_context.is_enable_interrupts) {
		goto l_out;
	}

	StorPortIssueDpc(instance,
		&instance->irq_context.irqs[isrSN].dpc,
		&instance->irq_context.irqs[isrSN].isrSN,
		NULL);

l_out:
	return PS3_TRUE;
}

static void ps3_irq_dpc_routine(PSTOR_DPC dpc,
	void *context,
	void *isr_no,
	void *arg
)
{
	struct ps3_instance *instance = (struct ps3_instance*)context;
	U32 isrSN = *((U32*)isr_no);
	struct ps3_irq *irqs = &instance->irq_context.irqs[isrSN];
	(void)arg;
	(void)dpc;

	LOG_DEBUG("%d\n", isrSN);
	irqs->is_dpc_running = PS3_TRUE;

	ps3_cmd_complete(irqs);

	irqs->is_dpc_running = PS3_FALSE;
}

S32 ps3_irqs_dpc_init(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct ps3_irq *irqs = NULL;
	U32 i = 0;

	irqs = irq_context->irqs;
	if (irq_context->irqs == NULL) {
		LOG_ERROR("host_no:%u irqs fail! \n",
			PS3_HOST(instance));
		goto l_out;
	}

	for (i = 0; i < irq_context->valid_msix_vector_count; i++) {
		irqs[i].is_dpc_running = PS3_FALSE;
		StorPortInitializeDpc(instance, &irqs[i].dpc, ps3_irq_dpc_routine);
	}
	instance->ioc_adpter->irq_disable(instance);
	return PS3_SUCCESS;

l_out:
	return -PS3_FAILED;
}

S32 ps3_irqs_init(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct PS3ReplyWord **virt_base = irq_context->reply_fifo_virt_base_addr_buf;
	struct ps3_irq *irqs = NULL;
	U32 i = 0;

	irq_context->irqs = (struct ps3_irq*)
		ps3_kcalloc(instance, irq_context->valid_msix_vector_count,
			sizeof(struct ps3_irq));
	if (irq_context->irqs == NULL) {
		LOG_ERROR("host_no:%u kcalloc fail! \n",
			PS3_HOST(instance));
		goto l_out;
	}
	irqs = irq_context->irqs;

	for (i = 0; i < irq_context->valid_msix_vector_count; i++) {
		memset(&irqs[i], 0, sizeof(irqs[i]));
		irqs[i].isrSN = i;
		irqs[i].reply_fifo_virt_base_addr = virt_base[i];
		irqs[i].instance = instance;
		ps3_atomic_set(&irqs[i].is_busy, 0);
		irqs[i].last_reply_idx = 0;
	}

	return PS3_SUCCESS;

l_out:
	return -PS3_FAILED;
}

S32 ps3_irqs_init_switch(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct PS3ReplyWord **virt_base = irq_context->reply_fifo_virt_base_addr_buf;
	struct ps3_irq *irqs = NULL;
	U32 i = 0;

	irq_context->irqs = (struct ps3_irq*)
		ps3_kcalloc(instance, irq_context->valid_msix_vector_count,
			sizeof(struct ps3_irq));
	if (irq_context->irqs == NULL) {
		LOG_ERROR("host_no:%u kcalloc fail! \n",
			PS3_HOST(instance));
		goto l_out;
	}
	irqs = irq_context->irqs;

	for (i = 0; i < irq_context->valid_msix_vector_count; i++) {
		memset(&irqs[i], 0, sizeof(irqs[i]));
		irqs[i].isrSN = i;
		irqs[i].reply_fifo_virt_base_addr = virt_base[i];
		irqs[i].instance = instance;
		ps3_atomic_set(&irqs[i].is_busy, 0);
		irqs[i].last_reply_idx = 0;
	}

	return PS3_SUCCESS;

l_out:
	return -PS3_FAILED;
}

static void ps3_irq_dpc_sync(struct ps3_instance *instance, struct ps3_irq *irqs)
{
	ULong status = STOR_STATUS_SUCCESS;
	U8 cancel_ret = PS3_TRUE;

	if (irqs == NULL) {
		LOG_ERROR("host_no:%u irqs fail! \n",
			PS3_HOST(instance));
		goto l_out;
	}

	status = StorPortCancelDpc(instance, &irqs->dpc, &cancel_ret);
	if (status != STOR_STATUS_SUCCESS) {
		LOG_ERROR("host_no:%u, cancel dpc failed,status:0x%x\n", PS3_HOST(instance), status);
	}

	if (cancel_ret) {
		goto l_out;
	}

	while (irqs->is_dpc_running) {
		ps3_msleep(10);
	}
l_out:
	return;
}

void ps3_irqs_exit(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct ps3_irq *irqs = irq_context->irqs;

	if (irqs == NULL) {
		goto l_out;
	}
	instance->ioc_adpter->irq_disable(instance);

	ps3_kfree(instance, irqs);
	irq_context->irqs = NULL;

l_out:
	return;
}

void ps3_irqs_enable(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;

	LOG_DEBUG("host_no:%u irq enable\n", PS3_HOST(instance));

	switch (irq_context->pci_irq_type)
	{
	case PS3_PCI_IRQ_LEGACY:
		ps3_ioc_legacy_irqs_enable(instance);
		break;
	case PS3_PCI_IRQ_MSI:
		ps3_ioc_msi_enable(instance);
		break;
	case PS3_PCI_IRQ_MSIX:
		ps3_ioc_msix_enable(instance);
		break;
	default:
		LOG_ERROR("host_no:%u irq type is failed :%d \n",
			PS3_HOST(instance), irq_context->pci_irq_type);
		break;
	}
	irq_context->is_enable_interrupts = PS3_DRV_TRUE;
	mb();
	return;
}

void ps3_irqs_disable(struct ps3_instance *instance)
{
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct ps3_irq *irqs = irq_context->irqs;
	U32 i = 0;
	LOG_DEBUG("host_no:%u irq disable\n", PS3_HOST(instance));

	if(irq_context->is_enable_interrupts == PS3_DRV_FALSE) {
		goto l_out;
	}
	switch (irq_context->pci_irq_type)
	{
	case PS3_PCI_IRQ_LEGACY:
		ps3_ioc_legacy_irqs_disable(instance);
		break;
	case PS3_PCI_IRQ_MSI:
		ps3_ioc_msi_disable(instance);
		break;
	case PS3_PCI_IRQ_MSIX:
		ps3_ioc_msix_disable(instance);
		break;
	default:
		LOG_ERROR("host_no:%u irq type is failed :%d \n",
			PS3_HOST(instance), irq_context->pci_irq_type);
		break;
	}

	if (irqs == NULL) {
		LOG_ERROR("host_no:%u irqs fail! \n",
			PS3_HOST(instance));
		goto l_out;
	}
	for (i = 0; i < irq_context->valid_msix_vector_count; i++) {
		ps3_irq_dpc_sync(instance, &irqs[i]);
	}
	irq_context->is_enable_interrupts = PS3_DRV_FALSE;
	mb();

l_out:
	return;
}

#endif

void ps3_all_reply_fifo_init(struct ps3_instance *instance)
{
	U32 reply_fifo_size = sizeof(struct PS3ReplyWord) *
		instance->irq_context.reply_fifo_depth;
	struct ps3_irq *irq = NULL;
	U32 i = 0;

	LOG_DEBUG("host_no:%u start to reply fifo init!\n", PS3_HOST(instance));
	for (; i < instance->irq_context.valid_msix_vector_count; ++i) {
		irq = instance->irq_context.irqs + i;
		memset(irq->reply_fifo_virt_base_addr, 0xff, reply_fifo_size);
		irq->last_reply_idx = 0;
	}
	LOG_DEBUG("host_no:%u end to reply fifo init!\n", PS3_HOST(instance));

	return;
}

