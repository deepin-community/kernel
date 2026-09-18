#ifndef __PCIE_ZTE_ZF_HDMA_H
#define __PCIE_ZTE_ZF_HDMA_H

#include <linux/acpi.h>
#include <linux/acpi_dma.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/idr.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/mempool.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/numa.h>
#include <linux/of_dma.h>
#include <linux/percpu.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/rculist.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/dinghai/log.h>
#include "pcie-zte-zf-epc.h"
#include "virt-dma.h"

#define ZF_MPF_HDMA_MODE_ENABLE          0

#define ZF_HDMA_DRIVER_NAME              "dh_hdma"
#define ZF_HDMA_ADDR_OFFSET              (0x6800000)
#define ZF_HDMA_PER_CHANNEL_SIZE         (0x200)
#define ZF_HDMA_RDCH_OFFSET              (0x100)
#define HDMA_RD                          1
#define HDMA_WR                          0

/* ZF HDMA init*/
#define DPU_HDMA_CHAN_NUM                (36)
#define ZF_HDMA_CHAN_FIRST_IDX           (18)
#define ZF_HDMA_CHAN_NUM                 (DPU_HDMA_CHAN_NUM - ZF_HDMA_CHAN_FIRST_IDX)
#define ZF_HDMA_VIRT_CHAN_NUM            ZF_HDMA_CHAN_NUM
#define ZF_HDMA_ALIGN_SIZE               (1)
#define ZF_HDMA_DMA_BUSWIDTHS            (BIT(DMA_SLAVE_BUSWIDTH_4_BYTES))
#define ZF_HDMA_WAIT_SLEEP_TIMES         (10)
#define ZF_HDMA_MAX_WAIT_TIMES           (100)

/* HDMA register*/
#define HDMA_EN_OFF                      (0x0)
#define HDMA_DOORBELL_OFF                (0x4)
#define HDMA_ELEM_PF_OFF                 (0x8)
#define HDMA_HANDSHAKE_OFF               (0xc)
#define HDMA_LLP_LOW_OFF                 (0x10)
#define HDMA_LLP_HIGH_OFF                (0x14)
#define HDMA_CYCLE_OFF                   (0x18)
#define HDMA_XFERSIZE_OFF                (0x1c)
#define HDMA_XFERSIZE_OFF_COMPLETE       (0x0)
#define HDMA_SAR_LOW_OFF                 (0x20)
#define HDMA_SAR_HIGH_OFF                (0x24)
#define HDMA_DAR_LOW_OFF                 (0x28)
#define HDMA_DAR_HIGH_OFF                (0x2c)
#define HDMA_WATERMARK_EN_OFF            (0x30)
#define HDMA_CONTROL1_OFF                (0x34)
#define HDMA_FUNC_NUM_OFF                (0x38)
#define HDMA_FUNC_NUM_OFF_VF_ENABLE      (16)
#define HDMA_FUNC_NUM_OFF_VF             (17)
#define HDMA_QOS_OFF                     (0x3c)

#define HDMA_STATUS_OFF                  (0x80)
#define HDMA_STATUS_OFF_STATUS           (0x3)
#define HDMA_STATUS_STOPPED              (0x3)
#define HDMA_INT_STATUS_OFF              (0x84)
#define HDMA_STOP_INT_STATUS             (0x1)
#define HDMA_INT_SETUP_OFF               (0x88)
#define HDMA_INT_MASK_BIT                (0x0)
#define HDMA_INT_MASK                    (0x7)
#define HDMA_LSIE_BIT                    (0x4)
#define HDMA_LSIE_MASK                   (0x1)
#define HDMA_INT_CLEAR_OFF               (0x8c)
#define HDMA_MSI_STOP_LOW_OFF            (0x90)
#define HDMA_MSI_STOP_HIGH_OFF           (0x94)
#define HDMA_MSI_WATERMARK_LOW_OFF       (0x98)
#define HDMA_MSI_WATERMARK_HIGH_OFF      (0x9c)
#define HDMA_MSI_ABORT_LOW_OFF           (0xa0)
#define HDMA_MSI_ABORT_HIGH_OFF          (0xa4)
#define HDMA_MSI_MSI_MSGD_OFF            (0xa8)

/* HDMA register val*/
#define HDMA_EN                          (BIT(0))
#define HDMA_DOORBELL_STOP               (BIT(1))
#define HDMA_DOORBELL_START              (BIT(0))
#define HDMA_TRANSFER_DONE               (0x3)

/*LL module*/
#define HDMA_LL_CONTROL_OFFSET           0x0
#define HDMA_LL_SIZE_OFFSET              0x4
#define HDMA_LL_SAR_LOW_OFFSET           0x8
#define HDMA_LL_SAR_HIGH_OFFSET          0xc
#define HDMA_LL_DAR_LOW_OFFSET           0x10
#define HDMA_LL_DAR_HIGH_OFFSET          0x14
#define HDMA_LL_NEXT_ELEMENT             0x18
#define HDMA_LL_LINK_CONTROL_OFFSET      0x0
#define HDMA_LL_LINK_EMPTY_OFFSET        0x4
#define HDMA_LL_LINK_POINTER_LOW_OFFSET  0x8
#define HDMA_LL_LINK_POINTER_HIGH_OFFSET 0xc
#define HDMA_LL_DATA_CONTROL             0x1
#define HDMA_LL_LINK_CONTROL             0x6
#define HDMA_LL_PREFETCH                 0x8
#define PF_DEPTH                         0x1f

enum hdma_mode {
    HDMA_MODE_LEGACY = 0,
    HDMA_MODE_SLAVE,
    HDMA_MODE_UNROLL
};

enum hdma_chan_status {
    HDMA_CHAN_IDLE = 0,
    HDMA_CHAN_USED,
    HDMA_CHAN_UNDEFINE
};

struct zf_hdma_sqe {
    size_t length;
    dma_addr_t src_addr;
    dma_addr_t dst_addr;
    struct zf_hdma_sqe *next;
};

struct zf_hdma_tx {
    int tx_id;
    struct dma_async_tx_descriptor *tx_desc;
    dma_async_tx_callback callback;
    void *callback_param;
    struct zf_hdma_tx *next;
};

struct hdma_ll_element {
    size_t length;
    dma_addr_t src_addr;
    dma_addr_t dst_addr;
};

struct zf_hdma_chan {
    u32 id;
    struct list_head list;
    const char *name;
    struct pci_dev *ep_pdev;
    struct zxdh_virt_dma_chan zxdh_vc;
    struct zxdh_virt_dma_desc zxdh_vd;
    void __iomem *base_addr; // 对应HDMA寄存器基址
    struct zf_hdma_sqe *sqe_list;
    struct zf_hdma_tx *tx_list;
    int is_busy;
    u8 func_no;
    u8 vfunc_no;
};

#endif