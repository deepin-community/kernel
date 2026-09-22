#ifndef ZXDH_EXT_MEM_CONFIG_H
#define ZXDH_EXT_MEM_CONFIG_H

#include <linux/fs.h>      // 包含kern_path、vfs_mkdir等函数的声明
#include <linux/path.h>    // 包含struct path结构体的定义
#include <linux/namei.h>   // 包含LOOKUP_*标志的定义
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/limits.h>
#include <linux/version.h>
#include <linux/user_namespace.h>
#include <linux/list.h>    // 包含链表相关定义
#include <linux/dma-mapping.h>

// 条件编译：只在支持的内核版本中包含 mnt_idmapping.h
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,3,0)
    #include <linux/mnt_idmapping.h>
#endif

#define READ_FROM_BAR    0
#define READ_FROM_FILE   1

// 按需 ioremap 的映射节点（用于跟踪和释放）
struct zxdh_mapped_region {
    void __iomem *va;           // ioremap 返回的虚拟地址
    u64 host_pa;                // 映射的物理地址
    u32 size;                   // 映射的大小
    struct list_head list;      // 链表节点
};

// 内存管理结构
struct zxdh_mem_info {
    void *base_va;              // 基础虚拟地址（当前未使用，保留用于兼容性）
    dma_addr_t base_pa;         // 基础物理地址
    u64 total_size;             // 总大小
    u64 offset;
    struct list_head mapped_list;  // 按需映射的链表头
    void *sw_qpc;
    dma_addr_t sw_qpc_pa;
};

/* 内存池块大小定义 */
#define ZXDH_MEM_BLOCK_16KB     (16 * 1024)      /* 16KB 内存块 */
#define ZXDH_MEM_BLOCK_64KB     (64 * 1024)      /* 64KB 内存块 */
#define ZXDH_MEM_BLOCK_512KB    (512 * 1024)     /* 512KB 内存块 */
#define ZXDH_MEM_BLOCK_1MB      (1024 * 1024)    /* 1MB 内存块 */

/* 内存池配置 */
#define ZXDH_MEM_POOL_16KB_COUNT    256          /* 16KB 内存块数量 */
#define ZXDH_MEM_POOL_64KB_COUNT    64           /* 64KB 内存块数量 */
#define ZXDH_MEM_POOL_512KB_COUNT   8            /* 512KB 内存块数量 */
#define ZXDH_MEM_POOL_1MB_COUNT     4            /* 1MB 内存块数量 */

/* 内存池总大小：16MB */
#define ZXDH_MEM_POOL_TOTAL_SIZE    (16 * 1024 * 1024)

/* 内存块状态 */
enum zxdh_mem_block_status {
    ZXDH_MEM_BLOCK_FREE = 0,     /* 空闲状态 */
    ZXDH_MEM_BLOCK_ALLOCATED     /* 已分配状态 */
};

/* 单个内存块结构 */
struct zxdh_mem_block {
    u64 pa;                      /* 物理地址（相对于 ext_pa 的偏移） */
    u32 size;                    /* 内存块大小 */
    enum zxdh_mem_block_status status;  /* 状态：空闲/已分配 */
    struct list_head list;       /* 链表节点 */
};

/* 内存池结构 */
struct zxdh_mem_pool {
    struct zxdh_mem_block *blocks;      /* 内存块数组 */
    u32 block_size;                     /* 块大小 */
    u32 block_count;                    /* 块数量 */
    u32 free_count;                     /* 空闲块数量 */
    spinlock_t lock;                    /* 内存池锁 */
};

/* 扩展内存池管理结构 */
struct zxdh_ext_mem_pool {
    struct zxdh_mem_pool pool_16kb;     /* 16KB 内存池 */
    struct zxdh_mem_pool pool_64kb;     /* 64KB 内存池 */
    struct zxdh_mem_pool pool_512kb;    /* 512KB 内存池 */
    struct zxdh_mem_pool pool_1mb;      /* 1MB 内存池 */
    u64 base_pa;                        /* 内存池基地址物理地址 */
    u64 base_host_pa;                   /* 内存池基地址 Host 物理地址 */
    void __iomem *base_va;              /* 内存池基地址虚拟地址 */
    u32 total_size;                     /* 总大小 */
    bool initialized;                   /* 是否已初始化 */
};

struct zxdh_pci_f;
struct zxdh_device;

int zxdh_read_ext_mem_info(struct zxdh_pci_f *rf, int position);
int zxdh_write_ext_mem_info(struct zxdh_pci_f *rf);
void set_hbm_config_path(struct zxdh_pci_f *rf);
int create_simple_directory(const char *path);
int zxdh_wait_gpu_done(struct zxdh_pci_f *rf);
void zxdh_cleanup_mem_info(struct zxdh_pci_f *rf);
void __iomem *zxdh_get_ext_mem(struct zxdh_pci_f *rf, u32 size, dma_addr_t *pa);
void zxdh_free_ext_mem(struct zxdh_pci_f *rf, void __iomem *va, u32 size);

/* 新的扩展内存池管理接口 */
int zxdh_init_ext_mem_pool(struct zxdh_pci_f *rf);
void zxdh_destroy_ext_mem_pool(struct zxdh_pci_f *rf);
void __iomem *zxdh_get_ext_mem_from_pool(struct zxdh_pci_f *rf, u32 size, dma_addr_t *pa);
u64 zxdh_get_ext_mem_host_pa(struct zxdh_pci_f *rf, void __iomem *va);
void zxdh_free_ext_mem_to_pool(struct zxdh_pci_f *rf, void __iomem *va, u32 size);

/* 通用内存分配/释放接口 */
void *zxdh_alloc_raw_mem(struct zxdh_device *iwdev, size_t size, dma_addr_t *dma_handle);
void zxdh_free_raw_mem(struct zxdh_device *iwdev, void *vaddr, size_t size, dma_addr_t dma_handle);

#endif