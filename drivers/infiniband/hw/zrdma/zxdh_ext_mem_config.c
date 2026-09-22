#include "zxdh_ext_mem_config.h"
#include "main.h"
#include <linux/spinlock.h>

#define HBM_CONFIG_DIR "/var/lib"

int zxdh_read_ext_mem_info(struct zxdh_pci_f *rf, int position)
{
    int err = 0;
    struct file *file = NULL;
    char *buffer = NULL;

    if (position == READ_FROM_BAR)
    {
        err = zxdh_wait_gpu_done(rf);
        if (err)  {
            pr_info("[zxdh_rdma] [%s] undone! completion mark may have not been fully writed.\n", __func__);
            return -EINVAL;
        }

        // 从BAR空间读取配置
        rf->config.sbdf = readq(rf->hw.hbm_info_page_addr);
        rf->config.ext_host = readq(rf->hw.hbm_info_page_addr + 0x8);
        rf->config.ext_pa = readq(rf->hw.hbm_info_page_addr + 0x10);
        rf->config.size = readq(rf->hw.hbm_info_page_addr + 0x18);
        rf->config.completion_mark = readq(rf->hw.hbm_info_page_addr + 0x20);

        pr_info("[zxdh_rdma] Read HBM config from BAR space\n");
    }
    else if (position == READ_FROM_FILE)
    {
        // 分配缓冲区
        buffer = kmalloc(sizeof(struct hbm_config), GFP_KERNEL);
        if (!buffer)
            return -ENOMEM;

        // 打开文件 - O_EXCL确保文件存在
        file = filp_open(rf->config_path, O_RDONLY, 0644);
        if (IS_ERR(file)) {
            err = PTR_ERR(file);
            pr_info("[zxdh_rdma] Failed to open config file %s: %d\n", rf->config_path, err);
            goto out_free;
        }

        // 使用kernel_read替代vfs_read
        if (kernel_read(file, buffer, sizeof(struct hbm_config), &file->f_pos) 
            != sizeof(struct hbm_config)) {
            err = -EIO;
            pr_err("[zxdh_rdma] Failed to read config from file %s\n", rf->config_path);
            goto out_close;
        }

        // 复制到配置结构
        memcpy(&rf->config, buffer, sizeof(struct hbm_config));

            // 验证完成标记
        if (rf->config.completion_mark != 0xabcd) {
            err = -EINVAL;
            pr_err("[zxdh_rdma] Invalid completion mark in config file %s\n", rf->config_path);
            goto out_close;
        }

        pr_info("[zxdh_rdma] Successfully read HBM config from file %s\n", rf->config_path);
    }
    else {
        pr_info("[zxdh_rdma] read ext mem info from unvalid position\n");
        return -EINVAL;
    }

    pr_info("[zxdh_rdma] SBDF: 0x%llx\n", rf->config.sbdf);
    pr_info("[zxdh_rdma] ext Host: 0x%llx\n", rf->config.ext_host);
    pr_info("[zxdh_rdma] ext PA: 0x%llx\n", rf->config.ext_pa);
    pr_info("[zxdh_rdma] Size: 0x%llx\n", rf->config.size);
    pr_info("[zxdh_rdma] Completion Mark: 0x%llx\n", rf->config.completion_mark);
    
    if (file)
        filp_close(file, NULL);
    if (buffer)
        kfree(buffer);

    return 0;

out_close:
    if (file)  // 关键检查：确保file是有效指针
        filp_close(file, NULL);
out_free:
    if (buffer)
        kfree(buffer);
    return err;
}

// 将配置写入文件
int zxdh_write_ext_mem_info(struct zxdh_pci_f *rf)
{
    struct file *file;
    char *buffer;
    int err = 0;
    char *dir_path;
    size_t dir_len;
    
    // 分配缓冲区
    buffer = kmalloc(sizeof(struct hbm_config), GFP_KERNEL);
    if (!buffer)
        return -ENOMEM;
    
    // 复制配置到缓冲区
    memcpy(buffer, &rf->config, sizeof(struct hbm_config));
    
    // 提取目录部分
    dir_len = strrchr(rf->config_path, '/') - rf->config_path;
    dir_path = kmalloc(dir_len + 1, GFP_KERNEL);
    if (!dir_path) {
        err = -ENOMEM;
        goto out_free;
    }
    memcpy(dir_path, rf->config_path, dir_len);
    dir_path[dir_len] = '\0';
    
    // 创建目录(如果不存在)
    err = create_simple_directory(dir_path);
    kfree(dir_path);
    if (err)
        goto out_free;
    
    // 打开文件 - O_CREAT创建文件，O_TRUNC截断现有内容
    file = filp_open(rf->config_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (IS_ERR(file)) {
        err = PTR_ERR(file);
        pr_err("[zxdh_rdma] Failed to open config file %s for writing: %d\n", 
              rf->config_path, err);
        goto out_free;
    }
    
     // 使用kernel_write替代vfs_write
    if (kernel_write(file, buffer, sizeof(struct hbm_config), &file->f_pos) 
        != sizeof(struct hbm_config)) {
        err = -EIO;
        pr_err("[zxdh_rdma] Failed to write config to file %s\n", rf->config_path);
        goto out_free;
    } else {
        pr_info("[zxdh_rdma] Successfully wrote HBM config to file %s\n", rf->config_path);
    }

	// 强制同步到磁盘 - 确保掉电保持
    err = vfs_fsync(file, 0);
    if (err) {
        pr_err("[zxdh_rdma] Failed to sync config file %s: %d\n", rf->config_path, err);
        goto out_free;
    }

    filp_close(file, NULL);

out_free:
    kfree(buffer);
    return err;
}

void set_hbm_config_path(struct zxdh_pci_f *rf)
{
    struct pci_dev *pdev = rf->pcidev;
    int ret;

    char *config_path = kmalloc(PATH_MAX, GFP_KERNEL);
    if (!config_path) return;

    ret = snprintf(config_path, PATH_MAX,
             HBM_CONFIG_DIR "/hbm_config_%04x_%02x_%02x_%x",
             pci_domain_nr(pdev->bus),
             pdev->bus->number,
             PCI_SLOT(pdev->devfn),
             PCI_FUNC(pdev->devfn));

    if (ret < 0 || ret >= PATH_MAX) {
        // snprintf失败或缓冲区不足
        kfree(config_path);
        rf->config_path = NULL;
        return;
    }

    rf->config_path = config_path;
}


// 创建单层目录
int create_simple_directory(const char *path)
{
    struct path parent_path;
    struct dentry *dentry;
    int err;
	const char *dirname;

    // 获取父目录路径
    err = kern_path(path, LOOKUP_PARENT, &parent_path);
    if (err) {
        pr_err("[zxdh_rdma] Failed to lookup parent directory: %d\n", err);
        return err;
    }

    // 从路径中提取目录名
    dirname = strrchr(path, '/');
    if (!dirname)
        dirname = path;
    else
        dirname++;

    // 创建目录项
    dentry = d_alloc_name(parent_path.dentry, dirname);
    if (!dentry) {
        err = -ENOMEM;
        goto out_path;
    }

    // 创建实际目录vfs_mkdir - 支持多个内核版本
#if defined(VFS_MKDIR_IDMAP)
    err = vfs_mkdir(&nop_mnt_idmap, parent_path.dentry->d_inode, dentry, 0755);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6,3,0)
    err = vfs_mkdir(&nop_mnt_idmap, parent_path.dentry->d_inode, dentry, 0755);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5,12,0)
    err = vfs_mkdir(&init_user_ns, parent_path.dentry->d_inode, dentry, 0755);
#else
    err = vfs_mkdir(parent_path.dentry->d_inode, dentry, 0755);
#endif

    if (err && err != -EEXIST) {
        pr_err("[zxdh_rdma] Failed to create directory: %d\n", err);
        dput(dentry);
        goto out_path;
    }

    dput(dentry);
out_path:
    path_put(&parent_path);
    return err;
}

int zxdh_wait_gpu_done(struct zxdh_pci_f *rf)
{
	u32 cnt = 0, status = 0;
	u64 val = 0;

	do {
		val = readq(rf->hw.hbm_info_page_addr + 0x20);			//读取完成标志
		if (cnt++ > 100000) {
			status = -ETIMEDOUT;
			break;
		}
		udelay(10);
	} while (val != 0xabcd);
	pr_info("[zxdh_rdma] [%s] wait time: %d us val=0x%llx\n", __func__, cnt * 10, val);

	return status;
}

void zxdh_cleanup_mem_info(struct zxdh_pci_f *rf)
{
    struct zxdh_mapped_region *mapped, *tmp;
    
    if (!rf)
        return;
    
    // 清理所有按需映射的内存
    list_for_each_entry_safe(mapped, tmp, &rf->mem_info.mapped_list, list) {
        if (mapped->va) {
            iounmap(mapped->va);
            pr_info("[zxdh_rdma] [%s] iounmap: va=0x%llx, host_pa=0x%llx, size=0x%x\n",
                    __func__, (u64)mapped->va, mapped->host_pa, mapped->size);
        }
        list_del(&mapped->list);
        kfree(mapped);
    }
    
    rf->mem_info.base_pa = 0;
    rf->mem_info.sw_qpc_pa = 0;
    rf->mem_info.total_size = 0;
    rf->mem_info.offset = 0;
}

// 获取ext_mem保留内存接口
// 返回值：虚拟地址，失败返回NULL
void __iomem *zxdh_get_ext_mem(struct zxdh_pci_f *rf, u32 size, dma_addr_t *pa)
{   
    u64 offset;
    void __iomem *va;
    u64 host_pa;
    struct zxdh_mapped_region *mapped;
    u32 i;
    u32 chunk_size;
    u32 current_chunk;
    static u8 zero_buffer[64 * 1024] __aligned(8) = {0};
    
    if (!rf || !pa || size == 0)
        return NULL;
      
    size = ALIGN(size, ZXDH_HW_PAGE_SIZE);
    offset = ALIGN(rf->mem_info.offset, ZXDH_HW_PAGE_SIZE);
    
    if((offset + size) > rf->mem_info.total_size)
    {
        pr_info("[zxdh_rdma] ext mem:get failed, not enough! offset=0x%llx,size=0x%x,total=0x%llx\n",
                offset, size, rf->mem_info.total_size);
        return NULL;
    }

    pr_info("[zxdh_rdma] ext mem enough! offset=0x%llx,size=0x%x, use_ext_mem = 0x%llx, total=0x%llx\n",
        offset, size, offset + size, rf->mem_info.total_size);

    // 计算地址
    host_pa = rf->mem_info.base_pa + offset;
    *pa = rf->config.ext_pa + offset;
    
    // 按需 ioremap 这一小段内存
    va = ioremap(host_pa, size);
    if (!va) {
        pr_err("[zxdh_rdma] Failed ioremap ext mem, host_pa=0x%llx, size=0x%x\n", host_pa, size);
        return NULL;
    }

    // 清零内存：ARM架构上不能使用memset/memset_io，使用memcpy_toio配合zero buffer
    // 对于大内存，分块处理以提高性能
    chunk_size = sizeof(zero_buffer);
    for (i = 0; i < size; i += chunk_size) {
        current_chunk = (size - i < chunk_size) ? (size - i) : chunk_size;
        memcpy_toio(va + i, zero_buffer, current_chunk);
    }

    // 记录映射信息（用于后续清理，避免内存泄漏）
    mapped = kmalloc(sizeof(*mapped), GFP_KERNEL);
    if (!mapped) {
        pr_err("[zxdh_rdma] Failed to allocate mapped_region, iounmap and return\n");
        // kmalloc失败时，必须iounmap已映射的内存，避免内存泄漏
        iounmap(va);
        return NULL;
    }
    
    mapped->va = va;
    mapped->host_pa = host_pa;
    mapped->size = size;
    INIT_LIST_HEAD(&mapped->list);
    list_add_tail(&mapped->list, &rf->mem_info.mapped_list);

    // 更新 offset
    rf->mem_info.offset = offset + size;
    
    
    return va;
}

// 释放单个 ext_mem 映射（用于错误处理路径）
// @va: zxdh_get_ext_mem 返回的虚拟地址
// @size: 分配的大小
// 
// 实现机制说明：
// 1. 链表作用：
//    - 记录所有通过ioremap映射的内存区域（mapped_region节点）
//    - 每个节点包含：虚拟地址(va)、物理地址(host_pa)、大小(size)
//    - 用于在zxdh_cleanup_mem_info时遍历并iounmap所有映射
//    - 用于在zxdh_free_ext_mem时查找并释放指定的映射
//
// 2. 使用场景：
//    - 正常流程：内存分配后一直使用，直到设备移除时通过zxdh_cleanup_mem_info统一清理
//    - 异常流程：在初始化错误处理路径中，释放已分配的ioremap，避免内存泄漏
//    - 注意：ext_mem是初始化所必须的内存，一旦zxdh_get_ext_mem失败，整个rdma初始化应返回失败
//    - 因此不需要回退offset，因为初始化已经失败，后续不会再使用
void zxdh_free_ext_mem(struct zxdh_pci_f *rf, void __iomem *va, u32 size)
{
    struct zxdh_mapped_region *mapped, *tmp;
    
    if (!rf || !va || size == 0)
        return;
    
    // 遍历链表，查找匹配的映射节点
    // 注意：va 应该等于 mapped->va（因为 zxdh_get_ext_mem 返回的就是 ioremap 的原始返回值）
    list_for_each_entry_safe(mapped, tmp, &rf->mem_info.mapped_list, list) {
        // 精确匹配：va 应该等于 mapped->va
        // 也支持范围匹配（以防万一 va 被修改）
        if (va == mapped->va) {
            // 找到匹配的节点，释放ioremap
            iounmap(mapped->va);
            pr_info("[zxdh_rdma] [%s] free ext_mem: va=0x%llx, host_pa=0x%llx, size=0x%x\n",
                    __func__, (u64)mapped->va, mapped->host_pa, mapped->size);
            
            // 从链表中删除节点
            list_del(&mapped->list);
            
            // 释放节点内存
            kfree(mapped);
             
            return;
        }
    }
    
    // 没有找到匹配的节点
    pr_warn("[zxdh_rdma] [%s] failed to find mapped region for va=0x%llx, size=0x%x\n",
            __func__, (u64)va, size);
}

/**
 * zxdh_init_ext_mem_pool - 初始化扩展内存池
 * 
 * 功能：在 ext_mem 的最后 16MB 区域创建内存池管理系统
 * 内存池采用分级管理策略，包含四种不同大小的内存块：
 * - 1MB 块：4 个，用于大内存分配
 * - 512KB 块：8 个，用于中等内存分配
 * - 64KB 块：64 个，用于较小内存分配
 * - 16KB 块：256 个，用于小内存分配
 * 
 * 内存布局：
 * [ext_mem 起始]                                    [ext_mem 结束]
 * |←────────────── 动态分配区域 ──────────────→|←── 16MB 内存池 ──→|
 *                                              ^
 *                                              base_pa = ext_pa + total_size - 16MB
 * 
 * @rf: ZXDH PCI 功能结构指针
 * @return: 成功返回0，失败返回负错误码
 */
int zxdh_init_ext_mem_pool(struct zxdh_pci_f *rf)
{
    struct zxdh_ext_mem_pool *pool;
    u32 i;
    u64 base_pa, base_host_pa;
    void __iomem *base_va;
    int ret = 0;
    
    if (!rf)
        return -EINVAL;
    
    // 检查是否已初始化
    if (rf->ext_mem_pool && rf->ext_mem_pool->initialized) {
        pr_info("[zxdh_rdma] [%s] memory pool already initialized\n", __func__);
        return 0;
    }
    
    // 分配内存池管理结构
    pool = kzalloc(sizeof(*pool), GFP_KERNEL);
    if (!pool) {
        pr_err("[zxdh_rdma] [%s] failed to allocate memory pool structure\n", __func__);
        return -ENOMEM;
    }
    
    // 检查 ext_mem 总大小是否足够（至少 16MB）
    if (rf->mem_info.total_size < ZXDH_MEM_POOL_TOTAL_SIZE) {
        pr_err("[zxdh_rdma] [%s] ext_mem too small: %llu < %u (16MB required)\n",
               __func__, rf->mem_info.total_size, ZXDH_MEM_POOL_TOTAL_SIZE);
        ret = -EINVAL;
        goto err_free_pool;
    }
    
    // 检查物理地址是否有效
    if (rf->config.ext_pa == 0 || rf->mem_info.base_pa == 0) {
        pr_err("[zxdh_rdma] [%s] invalid physical addresses: ext_pa=0x%llx, base_pa=0x%llx\n",
               __func__, rf->config.ext_pa, rf->mem_info.base_pa);
        ret = -EINVAL;
        goto err_free_pool;
    }
    
    // 计算内存池基地址（ext_mem 的最后 16MB）
    base_pa = rf->config.ext_pa + rf->mem_info.total_size - ZXDH_MEM_POOL_TOTAL_SIZE;
    base_host_pa = rf->mem_info.base_pa + rf->mem_info.total_size - ZXDH_MEM_POOL_TOTAL_SIZE;
    
    // 映射整个 16MB 内存池区域
    base_va = ioremap(base_host_pa, ZXDH_MEM_POOL_TOTAL_SIZE);
    if (!base_va) {
        pr_err("[zxdh_rdma] [%s] failed to ioremap memory pool, host_pa=0x%llx, size=0x%x\n",
               __func__, base_host_pa, ZXDH_MEM_POOL_TOTAL_SIZE);
        ret = -ENOMEM;
        goto err_free_pool;
    }
    
    // 初始化内存池结构
    pool->base_pa = base_pa;
    pool->base_host_pa = base_host_pa;
    pool->base_va = base_va;
    pool->total_size = ZXDH_MEM_POOL_TOTAL_SIZE;
    pool->initialized = false;
    
    // 初始化 1MB 内存池
    pool->pool_1mb.block_size = ZXDH_MEM_BLOCK_1MB;
    pool->pool_1mb.block_count = ZXDH_MEM_POOL_1MB_COUNT;
    pool->pool_1mb.free_count = ZXDH_MEM_POOL_1MB_COUNT;
    spin_lock_init(&pool->pool_1mb.lock);
    
    pool->pool_1mb.blocks = kcalloc(pool->pool_1mb.block_count, 
                                     sizeof(struct zxdh_mem_block), GFP_KERNEL);
    if (!pool->pool_1mb.blocks) {
        ret = -ENOMEM;
        goto err_unmap;
    }
    
    // 初始化 1MB 内存块
    for (i = 0; i < pool->pool_1mb.block_count; i++) {
        pool->pool_1mb.blocks[i].pa = base_pa + (i * ZXDH_MEM_BLOCK_1MB);
        pool->pool_1mb.blocks[i].size = ZXDH_MEM_BLOCK_1MB;
        pool->pool_1mb.blocks[i].status = ZXDH_MEM_BLOCK_FREE;
        INIT_LIST_HEAD(&pool->pool_1mb.blocks[i].list);
    }
    
    // 初始化 512KB 内存池
    pool->pool_512kb.block_size = ZXDH_MEM_BLOCK_512KB;
    pool->pool_512kb.block_count = ZXDH_MEM_POOL_512KB_COUNT;
    pool->pool_512kb.free_count = ZXDH_MEM_POOL_512KB_COUNT;
    spin_lock_init(&pool->pool_512kb.lock);
    
    pool->pool_512kb.blocks = kcalloc(pool->pool_512kb.block_count,
                                       sizeof(struct zxdh_mem_block), GFP_KERNEL);
    if (!pool->pool_512kb.blocks) {
        ret = -ENOMEM;
        goto err_free_1mb;
    }
    
    // 初始化 512KB 内存块
    for (i = 0; i < pool->pool_512kb.block_count; i++) {
        pool->pool_512kb.blocks[i].pa = base_pa + ZXDH_MEM_POOL_1MB_COUNT * ZXDH_MEM_BLOCK_1MB +
                                        (i * ZXDH_MEM_BLOCK_512KB);
        pool->pool_512kb.blocks[i].size = ZXDH_MEM_BLOCK_512KB;
        pool->pool_512kb.blocks[i].status = ZXDH_MEM_BLOCK_FREE;
        INIT_LIST_HEAD(&pool->pool_512kb.blocks[i].list);
    }
    
    // 初始化 64KB 内存池
    pool->pool_64kb.block_size = ZXDH_MEM_BLOCK_64KB;
    pool->pool_64kb.block_count = ZXDH_MEM_POOL_64KB_COUNT;
    pool->pool_64kb.free_count = ZXDH_MEM_POOL_64KB_COUNT;
    spin_lock_init(&pool->pool_64kb.lock);
    
    pool->pool_64kb.blocks = kcalloc(pool->pool_64kb.block_count,
                                      sizeof(struct zxdh_mem_block), GFP_KERNEL);
    if (!pool->pool_64kb.blocks) {
        ret = -ENOMEM;
        goto err_free_512kb;
    }
    
    // 初始化 64KB 内存块
    for (i = 0; i < pool->pool_64kb.block_count; i++) {
        pool->pool_64kb.blocks[i].pa = base_pa + ZXDH_MEM_POOL_1MB_COUNT * ZXDH_MEM_BLOCK_1MB +
                                       ZXDH_MEM_POOL_512KB_COUNT * ZXDH_MEM_BLOCK_512KB +
                                       (i * ZXDH_MEM_BLOCK_64KB);
        pool->pool_64kb.blocks[i].size = ZXDH_MEM_BLOCK_64KB;
        pool->pool_64kb.blocks[i].status = ZXDH_MEM_BLOCK_FREE;
        INIT_LIST_HEAD(&pool->pool_64kb.blocks[i].list);
    }
    
    // 初始化 16KB 内存池
    pool->pool_16kb.block_size = ZXDH_MEM_BLOCK_16KB;
    pool->pool_16kb.block_count = ZXDH_MEM_POOL_16KB_COUNT;
    pool->pool_16kb.free_count = ZXDH_MEM_POOL_16KB_COUNT;
    spin_lock_init(&pool->pool_16kb.lock);
    
    pool->pool_16kb.blocks = kcalloc(pool->pool_16kb.block_count,
                                      sizeof(struct zxdh_mem_block), GFP_KERNEL);
    if (!pool->pool_16kb.blocks) {
        ret = -ENOMEM;
        goto err_free_64kb;
    }
    
    // 初始化 16KB 内存块
    for (i = 0; i < pool->pool_16kb.block_count; i++) {
        pool->pool_16kb.blocks[i].pa = base_pa + ZXDH_MEM_POOL_1MB_COUNT * ZXDH_MEM_BLOCK_1MB +
                                       ZXDH_MEM_POOL_512KB_COUNT * ZXDH_MEM_BLOCK_512KB +
                                       ZXDH_MEM_POOL_64KB_COUNT * ZXDH_MEM_BLOCK_64KB +
                                       (i * ZXDH_MEM_BLOCK_16KB);
        pool->pool_16kb.blocks[i].size = ZXDH_MEM_BLOCK_16KB;
        pool->pool_16kb.blocks[i].status = ZXDH_MEM_BLOCK_FREE;
        INIT_LIST_HEAD(&pool->pool_16kb.blocks[i].list);
    }
    
    // 标记为已初始化
    pool->initialized = true;
    rf->ext_mem_pool = pool;
    
    pr_info("[zxdh_rdma] [%s] memory pool initialized: GPU_PA=0x%llx, Host_PA=0x%llx, VA=0x%llx, size=%uMB\n",
            __func__, base_pa, base_host_pa, (u64)base_va, ZXDH_MEM_POOL_TOTAL_SIZE / (1024*1024));
    
    return 0;

err_free_64kb:
    kfree(pool->pool_64kb.blocks);
err_free_512kb:
    kfree(pool->pool_512kb.blocks);
err_free_1mb:
    kfree(pool->pool_1mb.blocks);
err_unmap:
    iounmap(base_va);
err_free_pool:
    kfree(pool);
    return ret;
}

/**
 * zxdh_destroy_ext_mem_pool - 销毁扩展内存池
 * 
 * @rf: ZXDH PCI 功能结构指针
 */
void zxdh_destroy_ext_mem_pool(struct zxdh_pci_f *rf)
{
    struct zxdh_ext_mem_pool *pool;
    
    if (!rf || !rf->ext_mem_pool)
        return;
    
    pool = rf->ext_mem_pool;
    
    // 释放所有内存块数组
    kfree(pool->pool_1mb.blocks);
    kfree(pool->pool_512kb.blocks);
    kfree(pool->pool_64kb.blocks);
    kfree(pool->pool_16kb.blocks);
    
    // 取消映射
    if (pool->base_va) {
        iounmap(pool->base_va);
    }
    
    // 释放内存池结构
    kfree(pool);
    rf->ext_mem_pool = NULL;
    
    pr_info("[zxdh_rdma] [%s] memory pool destroyed\n", __func__);
}

/**
 * zxdh_get_ext_mem_from_pool - 从内存池分配内存
 * 
 * 分配策略：
 * 1. 根据请求大小选择合适的内存池：
 *    - size > 1MB: 直接失败
 *    - size > 512KB: 从 1MB 池分配
 *    - size > 64KB: 从 512KB 池分配
 *    - size > 16KB: 从 64KB 池分配
 *    - size <= 16KB: 从 16KB 池分配
 * 2. 在对应池中查找空闲块
 * 3. 如果找不到空闲块，返回失败
 * 4. 分配的内存会被清零（因为内存块是复用的）
 * 
 * @rf: ZXDH PCI 功能结构指针
 * @size: 请求分配的内存大小（字节）
 * @pa: 输出参数，返回 GPU 端物理地址
 * @return: 成功返回虚拟地址，失败返回NULL
 */
void __iomem *zxdh_get_ext_mem_from_pool(struct zxdh_pci_f *rf, u32 size, dma_addr_t *pa)
{
    struct zxdh_ext_mem_pool *pool;
    struct zxdh_mem_pool *target_pool = NULL;
    struct zxdh_mem_block *block = NULL;
    unsigned long flags;
    u32 i;
    u32 chunk_size;
    u32 current_chunk;
    void __iomem *va;
    static u8 zero_buffer[64 * 1024] __aligned(8) = {0};
    
    if (!rf || !pa || size == 0)
        return NULL;
    
    // 检查是否已初始化
    if (!rf->ext_mem_pool || !rf->ext_mem_pool->initialized) {
        pr_err("[zxdh_rdma] [%s] memory pool not initialized\n", __func__);
        return NULL;
    }
    
    pool = rf->ext_mem_pool;
    
    // 根据大小选择合适的内存池
    if (size > ZXDH_MEM_BLOCK_1MB) {
        pr_info("[zxdh_rdma] [%s] request size %d exceeds max block size 1MB\n",
                __func__, size);
        return NULL;
    } else if (size > ZXDH_MEM_BLOCK_512KB) {
        target_pool = &pool->pool_1mb;
    } else if (size > ZXDH_MEM_BLOCK_64KB) {
        target_pool = &pool->pool_512kb;
    } else if (size > ZXDH_MEM_BLOCK_16KB) {
        target_pool = &pool->pool_64kb;
    } else {
        target_pool = &pool->pool_16kb;
    }
    
    // 加锁保护
    spin_lock_irqsave(&target_pool->lock, flags);
    
    // 查找空闲块
    for (i = 0; i < target_pool->block_count; i++) {
        if (target_pool->blocks[i].status == ZXDH_MEM_BLOCK_FREE) {
            block = &target_pool->blocks[i];
            block->status = ZXDH_MEM_BLOCK_ALLOCATED;
            target_pool->free_count--;
            break;
        }
    }
    
    spin_unlock_irqrestore(&target_pool->lock, flags);
    
    if (!block) {
        pr_info("[zxdh_rdma] [%s] no free block in pool (size=%d, block_size=%d)\n",
                __func__, size, target_pool->block_size);
        return NULL;
    }
    
    // 计算虚拟地址
    va = pool->base_va + (block->pa - pool->base_pa);
    
    // 清零内存：与 zxdh_get_ext_mem 保持一致，使用 memcpy_toio 配合 zero buffer
    // 对于大内存，分块处理以提高性能
    chunk_size = sizeof(zero_buffer);
    for (i = 0; i < block->size; i += chunk_size) {
        current_chunk = (block->size - i < chunk_size) ? (block->size - i) : chunk_size;
        memcpy_toio(va + i, zero_buffer, current_chunk);
    }
    
    // 设置输出参数
    *pa = block->pa;
    
    // pr_info("[zxdh_rdma] [%s] allocated: va=0x%llx, pa=0x%llx, size=%d, from %dKB pool\n",
    //         __func__, (u64)va, block->pa, size, target_pool->block_size / 1024);
    
    return va;
}

/**
 * zxdh_get_ext_mem_host_pa - 根据虚拟地址获取 host 端物理地址
 * 
 * @rf: ZXDH PCI 功能结构指针
 * @va: zxdh_get_ext_mem_from_pool 返回的虚拟地址
 * @return: host 端物理地址，失败返回 0
 */
u64 zxdh_get_ext_mem_host_pa(struct zxdh_pci_f *rf, void __iomem *va)
{
    struct zxdh_ext_mem_pool *pool;
    u64 offset;
    
    if (!rf || !va || !rf->ext_mem_pool || !rf->ext_mem_pool->initialized) {
        pr_err("[zxdh_rdma] [%s] invalid parameters\n", __func__);
        return 0;
    }
    
    pool = rf->ext_mem_pool;
    
    offset = (uintptr_t)va - (uintptr_t)pool->base_va;
    
    // 验证偏移是否在有效范围内
    if (offset >= pool->total_size) {
        pr_err("[zxdh_rdma] [%s] invalid va offset: 0x%llx\n", __func__, (unsigned long long)offset);
        return 0;
    }
    
    // 计算并返回 host 端物理地址
    return pool->base_host_pa + offset;
}

/**
 * zxdh_free_ext_mem_to_pool - 释放内存回内存池
 * 
 * @rf: ZXDH PCI 功能结构指针
 * @va: zxdh_get_ext_mem_from_pool 返回的虚拟地址
 * @size: 分配的大小（用于验证）
 */
void zxdh_free_ext_mem_to_pool(struct zxdh_pci_f *rf, void __iomem *va, u32 size)
{
    struct zxdh_ext_mem_pool *pool;
    struct zxdh_mem_pool *target_pool = NULL;
    struct zxdh_mem_block *block = NULL;
    unsigned long flags;
    u32 i;
    u64 pa;
    
    if (!rf || !va || size == 0)
        return;
    
    if (!rf->ext_mem_pool || !rf->ext_mem_pool->initialized) {
        pr_warn("[zxdh_rdma] [%s] memory pool not initialized\n", __func__);
        return;
    }
    
    pool = rf->ext_mem_pool;
    
    pa = pool->base_pa + ((uintptr_t)va - (uintptr_t)pool->base_va);
    
    // 根据大小确定所属的内存池
    if (size > ZXDH_MEM_BLOCK_512KB) {
        target_pool = &pool->pool_1mb;
    } else if (size > ZXDH_MEM_BLOCK_64KB) {
        target_pool = &pool->pool_512kb;
    } else if (size > ZXDH_MEM_BLOCK_16KB) {
        target_pool = &pool->pool_64kb;
    } else {
        target_pool = &pool->pool_16kb;
    }
    
    // 加锁保护
    spin_lock_irqsave(&target_pool->lock, flags);
    
    // 查找匹配的内存块
    for (i = 0; i < target_pool->block_count; i++) {
        if (target_pool->blocks[i].pa == pa) {
            block = &target_pool->blocks[i];
            break;
        }
    }
    
    if (!block) {
        spin_unlock_irqrestore(&target_pool->lock, flags);
        pr_warn("[zxdh_rdma] [%s] failed to find block for pa=0x%llx\n",
                __func__, pa);
        return;
    }
    
    if (block->status != ZXDH_MEM_BLOCK_ALLOCATED) {
        spin_unlock_irqrestore(&target_pool->lock, flags);
        pr_warn("[zxdh_rdma] [%s] block pa=0x%llx is not allocated\n",
                __func__, pa);
        return;
    }
    
    // 标记为空闲
    block->status = ZXDH_MEM_BLOCK_FREE;
    target_pool->free_count++;
    
    spin_unlock_irqrestore(&target_pool->lock, flags);
    
    // pr_info("[zxdh_rdma] [%s] freed: va=0x%llx, pa=0x%llx, size=%d\n",
    //         __func__, (u64)va, pa, size);
}

void *zxdh_alloc_raw_mem(struct zxdh_device *iwdev, size_t size, dma_addr_t *dma_handle)
{
    struct zxdh_pci_f *rf;
    
    if (!iwdev || !dma_handle) {
        pr_err("[zxdh_rdma] %s: invalid NULL parameter\n", __func__);
        return NULL;
    }
    
    rf = iwdev->rf;
    if (!rf) {
        pr_err("[zxdh_rdma] %s: invalid NULL rf\n", __func__);
        return NULL;
    }
    
    if (rf->use_ext_mem_flag)
        return (void *)zxdh_get_ext_mem_from_pool(rf, size, dma_handle);
    else
        return dma_alloc_coherent(rf->hw.device, size, dma_handle, GFP_KERNEL);
}

void zxdh_free_raw_mem(struct zxdh_device *iwdev, void *vaddr, size_t size, dma_addr_t dma_handle)
{
    struct zxdh_pci_f *rf;
    
    if (!iwdev) {
        pr_err("[zxdh_rdma] %s: invalid NULL iwdev\n", __func__);
        return;
    }
    
    rf = iwdev->rf;
    if (!rf) {
        pr_err("[zxdh_rdma] %s: invalid NULL rf\n", __func__);
        return;
    }
    
    if (!vaddr)
        return;
    
    if (rf->use_ext_mem_flag)
        zxdh_free_ext_mem_to_pool(rf, vaddr, size);
    else
        dma_free_coherent(rf->hw.device, size, vaddr, dma_handle);
}

