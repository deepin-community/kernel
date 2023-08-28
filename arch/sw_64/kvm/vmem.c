// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/genalloc.h>
#include <linux/miscdevice.h>
#include <linux/kvm_host.h>
#include <asm/kvm_host.h>

static bool addr_in_pool(struct gen_pool *pool,
		unsigned long start, size_t size)
{
	bool found = false;
	unsigned long end = start + size - 1;
	struct gen_pool_chunk *chunk;

	rcu_read_lock();
	list_for_each_entry_rcu(chunk, &(pool)->chunks, next_chunk) {
		if (start >= chunk->start_addr && start <= chunk->end_addr) {
			if (end <= chunk->end_addr) {
				found = true;
				break;
			}
		}
	}
	rcu_read_unlock();
	return found;
}

static int vmem_vm_insert_page(struct vm_area_struct *vma)
{
	unsigned long addr, uaddr;
	struct page *vmem_page;
	struct vmem_info *info;
	size_t size;
	int ret;

	info = vma->vm_private_data;
	addr = info->start;
	size = info->size;
	uaddr = vma->vm_start;

	vm_flags_init(vma, VM_DONTEXPAND | VM_DONTDUMP | VM_MIXEDMAP);
	vmem_page = pfn_to_page(addr >> PAGE_SHIFT);
	do {
		ret = vm_insert_page(vma, uaddr, vmem_page);
		if (ret < 0) {
			pr_info("vm_insert_page failed: %d\n", ret);
			return ret;
		}
		vmem_page++;
		uaddr += PAGE_SIZE;
		size -= PAGE_SIZE;
	} while (size > 0);

	return 0;
}

static void vmem_vm_open(struct vm_area_struct *vma)
{
	struct vmem_info *info = vma->vm_private_data;

	atomic_inc(&info->refcnt);
}

static void vmem_vm_close(struct vm_area_struct *vma)
{
	unsigned long addr;
	size_t size;
	struct vmem_info *info;

	info = vma->vm_private_data;
	addr = info->start;
	size = round_up(info->size, 8 << 20);

	if (atomic_dec_and_test(&info->refcnt)) {
		if (sw64_kvm_pool && addr_in_pool(sw64_kvm_pool, addr, size)) {
			pr_info("gen pool free addr: %#lx, size: %#lx\n",
					addr, size);
			gen_pool_free(sw64_kvm_pool, addr, size);
		}
		kfree(info);
	}
}

const struct vm_operations_struct vmem_vm_ops = {
	.open = vmem_vm_open,
	.close = vmem_vm_close,
};
EXPORT_SYMBOL_GPL(vmem_vm_ops);

static int vmem_open(struct inode *inode, struct file *flip)
{
	flip->private_data = NULL;
	return 0;
}

static loff_t vmem_llseek(struct file *filp, loff_t offset, int whence)
{
	loff_t newpos = 256UL << 30;
	return newpos;
}

static int vmem_release(struct inode *inode, struct file *flip)
{
	return 0;
}

static int vmem_mmap(struct file *flip, struct vm_area_struct *vma)
{
	unsigned long addr;
	static struct vmem_info *info;
	size_t size = vma->vm_end - vma->vm_start;
	int ret;

	if (!(vma->vm_flags & VM_SHARED)) {
		pr_err("%s: mapping must be shared\n", __func__);
		return -EINVAL;
	}

	if (!sw64_kvm_pool)
		return -ENOMEM;

	if (flip->private_data == NULL) {
		addr = gen_pool_alloc(sw64_kvm_pool, round_up(size, 8 << 20));
		if (!addr)
			return -ENOMEM;

		info = kzalloc(sizeof(struct vmem_info), GFP_KERNEL);
		pr_info("guest phys addr=%#lx, size=%#lx\n", addr, size);
		info->start = addr;
		info->size = size;
		flip->private_data = (void *)info;
	} else {
		info = flip->private_data;
		addr = info->start;
	}

	vma->vm_private_data = (void *)info;
	vma->vm_ops = &vmem_vm_ops;
	vma->vm_ops->open(vma);

	/*to do if size bigger than vm_mem_size*/
	pr_info("sw64_vmem: vm_start=%#lx, size= %#lx\n", vma->vm_start, size);

	ret = vmem_vm_insert_page(vma);
	if (ret < 0)
		return ret;

	return 0;
}

static const struct file_operations vmem_fops = {
	.owner = THIS_MODULE,
	.open = vmem_open,
	.llseek = vmem_llseek,
	.release = vmem_release,
	.mmap = vmem_mmap,
};

static struct miscdevice vmem_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "sw64_vmem",
	.fops  = &vmem_fops,
};

int __init vmem_init(void)
{
	int err;

	err = misc_register(&vmem_dev);
	if (err != 0) {
		pr_err("Could not register sw64_vmem device\n");
		return err;
	}
	return 0;
}

void vmem_exit(void)
{
	misc_deregister(&vmem_dev);
}
