/*
 * Copyright(c) 2013 - 2021 Intel Corporation.
 * Compatibility layer for CGS_V5_693 (3.10.0-693) kernel
 */

#include <linux/pci.h>
#include <linux/msi.h>
#include <linux/slab.h>
#include <linux/printk.h>

#ifdef CGS_V5_693

/* PCI IRQ flags - copied from newer kernels */
#ifndef PCI_IRQ_LEGACY
#define PCI_IRQ_LEGACY		(1 << 0) /* Allow legacy interrupts */
#define PCI_IRQ_MSI		(1 << 1) /* Allow MSI interrupts */
#define PCI_IRQ_MSIX		(1 << 2) /* Allow MSI-X interrupts */
#define PCI_IRQ_AFFINITY	(1 << 3) /* Auto-assign affinity */
#define PCI_IRQ_ALL_TYPES \
	(PCI_IRQ_LEGACY | PCI_IRQ_MSI | PCI_IRQ_MSIX)
#endif

/*
 * Global storage for msix_entries pointers per PCI device.
 * We use a simple array indexed by PCI device BDF (Bus/Device/Function).
 * This avoids conflicts with driver_data which drivers use for their own data.
 *
 * Index formula: (bus << 5) | devfn which gives us bus*256 + device*8 + function
 * This properly distinguishes between different functions of the same device.
 * Max index for bus 255, device 31, function 7: 255*256 + 255 = 65535
 *
 * Using volatile to ensure visibility across CPU cores on SMP systems.
 */
#define MAX_PCI_DEVICES 65536
static volatile struct msix_entry *pci_msix_entries_map[MAX_PCI_DEVICES];

/**
 * pci_get_msix_index - Get index into pci_msix_entries_map
 * @dev: PCI device
 *
 * Returns index based on full BDF (Bus/Device/Function)
 */
static inline int pci_get_msix_index(struct pci_dev *dev)
{
	/* Use bus number, device number, and function number */
	return (dev->bus->number << 8) | (dev->devfn & 0xff);
}

/**
 * pci_irq_vector - Get IRQ number for a given MSI/MSI-X vector
 * @dev: PCI device
 * @nr: vector number
 *
 * In 3.10 kernel, pci_irq_vector is not available. This compatibility
 * implementation gets the IRQ from the msix_entry array allocated by
 * pci_alloc_irq_vectors. We store the array pointer in a global map
 * indexed by PCI BDF to avoid conflicts with driver_data.
 *
 * For non-MSI/MSI-X devices, it returns dev->irq when nr is 0.
 * For MSI/MSI-X enabled devices, it returns the corresponding vector.
 */
int pci_irq_vector(struct pci_dev *dev, unsigned int nr)
{
	volatile struct msix_entry *msix_entries;
	int idx;

	/* Get msix_entries from our global map */
	idx = pci_get_msix_index(dev);
	if (idx < 0 || idx >= MAX_PCI_DEVICES) {
		WARN_ON_ONCE(1);
		return -EINVAL;
	}

	msix_entries = READ_ONCE(pci_msix_entries_map[idx]);
	if (msix_entries) {
		int irq = msix_entries[nr].vector;
		return irq;
	}

	/* Fallback: for non-MSI/MSI-X devices, only nr==0 is valid */
	if (!dev->msix_enabled) {
		if (WARN_ON_ONCE(nr > 0))
			return -EINVAL;
		return dev->irq;
	}

	/* Should not happen if pci_alloc_irq_vectors was called */
	printk(KERN_ERR "pci_irq_vector: msix_entries is NULL! dev=%s, idx=%d, nr=%u, msix_enabled=%d\n",
			pci_name(dev), idx, nr, dev->msix_enabled);
	WARN_ON_ONCE(1);
	return -EINVAL;
}
EXPORT_SYMBOL(pci_irq_vector);

/**
 * pci_alloc_irq_vectors_affinity - allocate multiple IRQ vectors for a PCI device
 * @dev: PCI device
 * @min_vecs: minimum number of vectors to allocate
 * @max_vecs: maximum number of vectors to allocate
 * @flags: IRQ allocation flags (PCI_IRQ_MSIX, PCI_IRQ_MSI, PCI_IRQ_LEGACY)
 * @affd: affinity descriptor (ignored in this implementation)
 *
 * In 3.10 kernel, pci_alloc_irq_vectors is not available. This compatibility
 * implementation uses pci_enable_msix_range for MSI-X allocation.
 * The msix_entries pointer is stored in a global map for later use by
 * pci_irq_vector().
 *
 * Returns: number of vectors allocated (>= min_vecs) on success,
 * negative error code on failure.
 */
int pci_alloc_irq_vectors_affinity(struct pci_dev *dev,
				   unsigned int min_vecs,
				   unsigned int max_vecs,
				   unsigned int flags,
				   void *affd)
{
	volatile struct msix_entry *msix_entries;
	int ret, idx, i;

	/* Only MSI-X is supported in this implementation */
	if (!(flags & PCI_IRQ_MSIX))
		return -ENOSPC;

	/* Allocate msix_entries array */
	msix_entries = kmalloc(max_vecs * sizeof(struct msix_entry), GFP_KERNEL);
	if (!msix_entries)
		return -ENOMEM;

	/* Initialize entry field */
	for (i = 0; i < max_vecs; i++)
		msix_entries[i].entry = i;

	/* Try to enable MSI-X with the requested number of vectors */
	ret = pci_enable_msix_range(dev, (struct msix_entry *)msix_entries, min_vecs, max_vecs);
	if (ret < 0) {
		kfree((void *)msix_entries);
		return ret;
	}

	/*
	 * Store msix_entries pointer in our global map for pci_irq_vector()
	 * We use a global map instead of dev->dev.driver_data because the
	 * driver itself uses driver_data for its private data (e.g., en_priv).
	 * Use WRITE_ONCE to ensure visibility across CPU cores.
	 */
	idx = pci_get_msix_index(dev);
	if (idx >= 0 && idx < MAX_PCI_DEVICES) {
		WRITE_ONCE(pci_msix_entries_map[idx], (volatile struct msix_entry *)msix_entries);
	}

	return ret;
}
EXPORT_SYMBOL(pci_alloc_irq_vectors_affinity);

/**
 * pci_alloc_irq_vectors - allocate multiple IRQ vectors for a PCI device
 * @dev: PCI device
 * @min_vecs: minimum number of vectors to allocate
 * @max_vecs: maximum number of vectors to allocate
 * @flags: IRQ allocation flags
 *
 * Wrapper around pci_alloc_irq_vectors_affinity with NULL affinity.
 */
int pci_alloc_irq_vectors(struct pci_dev *dev,
			  unsigned int min_vecs,
			  unsigned int max_vecs,
			  unsigned int flags)
{
	return pci_alloc_irq_vectors_affinity(dev, min_vecs, max_vecs, flags, NULL);
}
EXPORT_SYMBOL(pci_alloc_irq_vectors);

/**
 * pci_free_irq_vectors - free IRQ vectors allocated by pci_alloc_irq_vectors
 * @dev: PCI device
 */
void pci_free_irq_vectors(struct pci_dev *dev)
{
	volatile struct msix_entry *msix_entries;
	int idx;

	if (dev->msix_enabled) {
		pci_disable_msix(dev);
	}

	/* Free the msix_entries array from our global map */
	idx = pci_get_msix_index(dev);
	if (idx >= 0 && idx < MAX_PCI_DEVICES) {
		msix_entries = READ_ONCE(pci_msix_entries_map[idx]);
		if (msix_entries) {
			kfree((void *)msix_entries);
			WRITE_ONCE(pci_msix_entries_map[idx], NULL);
		}
	}
}
EXPORT_SYMBOL(pci_free_irq_vectors);

/**
 * __kc_dma_set_mask_and_coherent - Set DMA mask and coherent DMA mask
 * @dev: device to set DMA mask for
 * @mask: DMA mask value
 *
 * This is a compatibility implementation for kernels < 3.13.0
 * Returns 0 on success, negative error code on failure.
 */
int __kc_dma_set_mask_and_coherent(struct device *dev, u64 mask)
{
	int err;

	err = dma_set_mask(dev, mask);
	if (err)
		return err;

	dma_set_coherent_mask(dev, mask);

	return 0;
}
EXPORT_SYMBOL(__kc_dma_set_mask_and_coherent);

#endif /* CGS_V5_693 */
