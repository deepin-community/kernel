#ifndef _PS3_IRQ_H_
#define _PS3_IRQ_H_

#ifndef _WINDOWS
#include <linux/list.h>
#include <linux/irq_poll.h>
#include <scsi/scsi_host.h>

#include <linux/irqreturn.h>
#endif

#include "ps3_htp_def.h"
#include "ps3_err_def.h"
#include "ps3_cmd_channel.h"
#include "ps3_inner_data.h"

#define PS3_IRQ_NAME_LENGTH		(32)
#define PS3_SWITCH_IRQ_INDEX	(0)

struct ps3_instance;

#if ((defined(RHEL_MAJOR) && (RHEL_MAJOR == 8) && (RHEL_MINOR >= 2)) || \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)))
#define DRIVER_SUPPORT_KERNEL_IRQ_AFFINITY
#endif

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(4,9,0) || LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
#define PS3_OS_MANAGED_IRQ_SUPPORT
#endif

enum {
	PS3_PCI_IRQ_MODE_NONE_SPE = 0,
	PS3_PCI_IRQ_MODE_LEGACY = 1,
	PS3_PCI_IRQ_MODE_MSI = 2,
	PS3_PCI_IRQ_MODE_MSIX = 3,
};

struct ps3_irq {
	char name[PS3_IRQ_NAME_LENGTH];
	struct PS3ReplyWord *reply_fifo_virt_base_addr;
	struct ps3_instance *instance;

	U32 irqNo; 
	U32 isrSN; 
	U16 last_reply_idx;
	U8  reserved0[6];
#ifdef _WINDOWS
	STOR_DPC dpc;
	volatile U8 is_dpc_running;
#else
	struct irq_poll irqpoll;
	Bool is_irq_poll_disabled;
	U32 irq_poll_sched_threshold;
	Bool is_sched_irq_poll;
	Bool is_enable_irq;
	U8  reserved1[2];
#endif
	ps3_atomic32 is_busy;
};

struct ps3_irq_recovery {
	U32 irqNo; 
	U32 isrSN; 
	U8  reserved0[8];
	struct ps3_instance *instance;
};

struct ps3_irq_context {
	struct ps3_instance *instance;
	U32 reply_fifo_depth;
	U32 valid_msix_vector_count;
	U32 high_iops_msix_vectors;
	U32 dump_isrSN;
	U32 reply_fifo_desc_buf_size;
#ifndef _WINDOWS
	struct dma_pool *reply_fifo_desc_buf_pool;
#endif
	U32 reply_fifo_size;
	dma_addr_t reply_fifo_desc_buf_phys;
	struct PS3ReplyFifoDesc *reply_fifo_desc_buf;
#ifndef _WINDOWS
	struct dma_pool *reply_fifo_pool;
#endif
	struct PS3ReplyWord *reply_fifo_virt_base_addr_buf[PS3_MAX_REPLY_QUE_COUNT];
	dma_addr_t reply_fifo_phys_base_addr_buf[PS3_MAX_REPLY_QUE_COUNT];

	struct ps3_irq *irqs;
#ifndef _WINDOWS
	U32 *cpu_msix_table;
	atomic_t high_iops_io_count;
	S32 cpu_msix_table_sz;
#endif

	Bool is_enable_interrupts;
	Bool is_support_balance;
	Bool is_balance_current_perf_mode;
	U8   pci_irq_type;

#ifdef _WINDOWS
	PGROUP_AFFINITY	group_affinity;
#endif
	struct ps3_irq_recovery * irq_recovery;
};

S32 ps3_irq_context_init(struct ps3_instance *instance);

S32 ps3_irq_context_exit(struct ps3_instance *instance);

S32 ps3_irqs_dpc_init(struct ps3_instance *instance);

S32 ps3_irqs_init(struct ps3_instance *instance);

S32 ps3_irqs_init_switch(struct ps3_instance *instance);

void ps3_irqs_exit(struct ps3_instance *instance);
#ifndef _WINDOWS
U32 ps3_msix_index_get(struct ps3_cmd *cmd, U16 pd_count);

void ps3_perf_update(struct ps3_instance *instance, U8 iocPerfMode);
#endif
void ps3_irqs_enable(struct ps3_instance *instance);

void ps3_irqs_disable(struct ps3_instance *instance);
#ifndef _WINDOWS
void ps3_irqpolls_enable(struct ps3_instance *instance);

void ps3_irqs_sync(struct ps3_instance *instance);
#endif
#ifndef _WINDOWS
irqreturn_t ps3_irqs_service(int irq_so, void *priv);
#else
U8 ps3_irqs_service(void *priv, ULong irq_no);
#endif

#ifndef _WINDOWS
int ps3_irqpoll_service(struct irq_poll *irqpoll, int budget);
#endif
static inline Bool ps3_irq_busy_add(struct ps3_irq *irq)
{
	return ps3_atomic_add_unless(&irq->is_busy, 1, 1);
}

static inline void ps3_irq_busy_dec(struct ps3_irq *irq)
{
	ps3_atomic_dec(&irq->is_busy);
}

static inline Bool ps3_irq_is_enable(const struct ps3_irq_context *irq_ctx)
{
	return (irq_ctx->is_enable_interrupts == PS3_DRV_TRUE);
}

void ps3_all_reply_fifo_init(struct ps3_instance *instance);

#ifndef _WINDOWS
static inline void ps3_irq_poll_sched(struct irq_poll * iop)
{
#ifdef CONFIG_IRQ_POLL
	irq_poll_sched(iop);
#else
	(void)iop;
#endif
}

static inline void ps3_irq_poll_init(struct irq_poll *iop, int weight, irq_poll_fn *func)
{
#ifdef CONFIG_IRQ_POLL
	irq_poll_init(iop, weight, func);
#else
	(void)iop;
	(void)weight;
	(void)func;
#endif
}

static inline void ps3_irq_poll_complete(struct irq_poll * iop)
{
#ifdef CONFIG_IRQ_POLL
	irq_poll_complete(iop);
#else
	(void)iop;
#endif
}

static inline void ps3_irq_poll_enable(struct irq_poll * iop)
{
#ifdef CONFIG_IRQ_POLL
	irq_poll_enable(iop);
#else
	(void)iop;
#endif
}

static inline void ps3_irq_poll_disable(struct irq_poll * iop)
{
#ifdef CONFIG_IRQ_POLL
	irq_poll_disable(iop);
#else
	(void)iop;
#endif
}
#endif
#endif 
