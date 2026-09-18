#include <linux/io.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/pci.h>
#include "zxic_common.h"
#include "dpp_dtb_cfg.h"
#include "dpp_kernel_init.h"
#include "dpp_dtb_table_api.h"

/*hash dma地址信息*/
typedef struct hash_dma_addr_info
{
    ZXIC_UINT32 slot_id;            /*np所在的槽位号*/ 
    ZXIC_UINT32 dma_size;           /*hash申请的dma大小*/
    dma_addr_t dma_phy_addr;        /* dma 物理地址*/
    ZXIC_VOID *dma_vir_addr;        /* dma 内核虚拟地址*/
}HASH_DMA_ADDR_INFO;

/*DMA空间信息*/
typedef struct dpp_dma_info
{
    DTB_QUEUE_DMA_ADDR_INFO dtb_queue_info[DPP_DTB_QUEUE_NUM_MAX];   /*dtb队列*/
    HASH_DMA_ADDR_INFO hash_dma_info;                                /*hash上送*/
}DPP_KERNEL_DMA_INFO;

static DPP_KERNEL_DMA_INFO g_dpp_dma_info[DPP_PCIE_SLOT_MAX] = {0};
static ZXIC_UINT32 queue_used_flag[DPP_PCIE_SLOT_MAX][4] = {0};

static ZXIC_VOID dpp_dtb_queue_dma_flag_set(ZXIC_UINT32 slot_id, ZXIC_UINT32 queue_id)
{
    ZXIC_UINT32 bit_shift = 0;
    ZXIC_UINT32 reg_shift = 0;

    reg_shift = queue_id / 32;
    bit_shift = queue_id % 32;

    queue_used_flag[slot_id][reg_shift] =  queue_used_flag[slot_id][reg_shift] | (0x1 << bit_shift);
}

static ZXIC_VOID dpp_dtb_queue_dma_flag_clear(ZXIC_UINT32 slot_id, ZXIC_UINT32 queue_id)
{
    ZXIC_UINT32 bit_shift = 0;
    ZXIC_UINT32 reg_shift = 0;

    reg_shift = queue_id / 32;
    bit_shift = queue_id % 32;

    queue_used_flag[slot_id][reg_shift] =  queue_used_flag[slot_id][reg_shift] & ~(0x1 << bit_shift);
}

static ZXIC_UINT32 dpp_dtb_queue_dma_flag_get(ZXIC_UINT32 slot_id, ZXIC_UINT32 queue_id)
{
    ZXIC_UINT32 bit_shift = 0;
    ZXIC_UINT32 reg_shift = 0;

    ZXIC_UINT32 flag = 0;

    reg_shift = queue_id / 32;
    bit_shift = queue_id % 32;

    flag = (queue_used_flag[slot_id][reg_shift] >> bit_shift) & 0x1;

    ZXIC_COMM_TRACE_NOTICE("slot %d queue %d flag %d!\n", slot_id, queue_id, flag);

    return flag;
}

//申请队列DMA空间，通过传入DMA队列号，返回申请到DMA内存的物理地址
ZXIC_SINT32 dpp_dtb_queue_dma_mem_alloc(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_UINT32 size)
{
    dma_addr_t dma_handle;
    ZXIC_VOID* cpu_addr = NULL;
    ZXIC_UINT32 slot_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);

    slot_id = (ZXIC_UINT32)DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot_id, 0, DPP_DEV_SLOT_MAX - 1);

    if (queue_id > (DPP_DTB_QUEUE_NUM_MAX - 1))
    {
        return -ENOMEM;
    }

    cpu_addr = dma_alloc_coherent(&(DEV_PCIE_DEV(dev)->dev), size, &dma_handle, GFP_KERNEL);

    if(!cpu_addr)
    {
        dev_err(&(DEV_PCIE_DEV(dev)->dev),"dpp_dtb_queue_dma_mem_alloc buff allocation failed\n");
        return -ENOMEM;
    }

    g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].slot_id = slot_id;
    g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].queue_id = queue_id;
    g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].dma_vir_addr = ZXIC_COMM_PTR_TO_VAL(cpu_addr);
    g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].dma_phy_addr = dma_handle;
    g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].dma_size = size;

    ZXIC_COMM_TRACE_NOTICE("slot %d queue %d kernel phy addr :0x%016llx !\n",
                                                                slot_id, queue_id, dma_handle);
    ZXIC_COMM_TRACE_NOTICE("slot %d queue %d kernel vir addr :0x%016llx !\n",
                                                                slot_id, queue_id, ZXIC_COMM_PTR_TO_VAL(cpu_addr));

    dpp_dtb_queue_dma_flag_set(slot_id, queue_id);
    dpp_dtb_queue_dma_flag_get(slot_id, queue_id);

    return DPP_OK;
}

ZXIC_SINT32 dpp_dtb_queue_dma_mem_get(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, DTB_QUEUE_DMA_ADDR_INFO *dmaAddrInfo)
{
    ZXIC_UINT32 slot_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);

    slot_id = (ZXIC_UINT32)DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot_id, 0, DPP_DEV_SLOT_MAX - 1);

    if(queue_id > (DPP_DTB_QUEUE_NUM_MAX - 1))
    {
        ZXIC_COMM_PRINT("[dpp_dtb_queue_dma_mem_get]:queue id max.\n");
        return -1;
    }

    if (g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].queue_id != queue_id)
    {
        ZXIC_COMM_PRINT("[dpp_dtb_queue_dma_mem_get]:slot %d queue %d error !\n", slot_id, queue_id);
        return -1;
    }
    dmaAddrInfo->dma_phy_addr = g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].dma_phy_addr;
    dmaAddrInfo->dma_vir_addr = g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].dma_vir_addr;
    dmaAddrInfo->dma_size = g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].dma_size;
    dmaAddrInfo->queue_id = queue_id;
    dmaAddrInfo->slot_id = slot_id;

    return DPP_OK;
}

/*dtb队列dma内存释放，通过传入dtb队列号，在维护的队列地址中找到对应的虚拟地址和物理地址，进行释放*/
ZXIC_SINT32 dpp_dtb_queue_dma_mem_release(DPP_DEV_T *dev, ZXIC_UINT32 queue_id)
{
    dma_addr_t dma_handle = 0;
    ZXIC_VOID* cpu_addr = NULL;
    ZXIC_UINT32 dma_size = 0;
    ZXIC_UINT32 slot_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);

    slot_id = (ZXIC_UINT32)DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot_id, 0, DPP_DEV_SLOT_MAX - 1);

    if(queue_id > (DPP_DTB_QUEUE_NUM_MAX - 1))
    {
        ZXIC_COMM_PRINT("queue id max.\n");
        return -1;
    }

    if (g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].queue_id != queue_id)
    {
        ZXIC_COMM_PRINT("slot %u queue %u error!\n", slot_id, queue_id);
        return -1;
    }
    
    dma_handle = g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].dma_phy_addr;
    cpu_addr = ZXIC_COMM_VAL_TO_PTR(g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].dma_vir_addr);
    dma_size = g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].dma_size;

    if (!dma_handle)
    {
        return -EFAULT;
    }

    dma_free_coherent(&(DEV_PCIE_DEV(dev)->dev), dma_size, cpu_addr, dma_handle);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x queue: %d release success.\n", slot_id, DEV_PCIE_VPORT(dev), queue_id);

    g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].slot_id = 0;
    g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].queue_id = 0;
    g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].dma_phy_addr = 0;
    g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].dma_vir_addr = 0;
    g_dpp_dma_info[slot_id].dtb_queue_info[queue_id].dma_size = 0;

    dpp_dtb_queue_dma_flag_clear(slot_id, queue_id);
    dpp_dtb_queue_dma_flag_get(slot_id, queue_id);

    return 0;
}

ZXIC_SINT32 dtb_sdt_dump_dma_alloc(DPP_DEV_T *dev,
                                   ZXIC_UINT32 dma_size, 
                                   ZXIC_UINT64 * p_dma_phy_addr, 
                                   ZXIC_UINT64 * p_dma_vir_addr)
{
    int rc = 0;

    dma_addr_t dma_handle;
    ZXIC_VOID* cpu_addr = NULL;

    cpu_addr = dma_alloc_coherent(&(DEV_PCIE_DEV(dev)->dev), dma_size, &dma_handle, GFP_KERNEL);

    if(!cpu_addr)
    {
        dev_err(&(DEV_PCIE_DEV(dev)->dev),"dtb_sdt_dump_dma_alloc buff allocation failed\n");
        return -ENOMEM;
    }

    *p_dma_phy_addr = (ZXIC_UINT64)dma_handle;
    *p_dma_vir_addr = (ZXIC_UINT64)(ZXIC_COMM_PTR_TO_VAL(cpu_addr));

    return rc;
}

ZXIC_SINT32 dtb_sdt_dump_dma_release(DPP_DEV_T *dev,
                            ZXIC_UINT32 dma_size, 
                            ZXIC_UINT64 dma_phy_addr, 
                            ZXIC_UINT64 dma_vir_addr)
{
    dma_addr_t dma_handle = 0;
    ZXIC_VOID* cpu_addr = NULL;

    dma_handle = (dma_addr_t)dma_phy_addr;
    cpu_addr = ZXIC_COMM_VAL_TO_PTR(dma_vir_addr);

    if (!dma_handle)
    {
        return -EFAULT;
    }

    dma_free_coherent(&(DEV_PCIE_DEV(dev)->dev), dma_size, cpu_addr, dma_handle);

    return 0;
}
