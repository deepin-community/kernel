// SPDX-License-Identifier: GPL-2.0

/* Generic Event Device for ACPI. */

#include <linux/acpi.h>
#include <linux/cpu.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>

/* offset should be the same as QEMU */
#define OFFSET_START_ADDR	0
#define OFFSET_LENGTH		8
#define OFFSET_STATUS		16
#define OFFSET_SLOT		24
#define OFFSET_CPUID		32
#define OFFSET_NODE		40

/* Memory hotplug event */
#define SUNWAY_MEMHOTPLUG_ADD		0x1
#define SUNWAY_MEMHOTPLUG_REMOVE	0x2
/* Cpu hotplug event */
#define SUNWAY_CPUHOTPLUG_ADD		0x4
#define SUNWAY_CPUHOTPLUG_REMOVE	0x8


struct sunway_memory_device {
	struct sunway_ged_device *device;
	unsigned int state;     /* State of the memory device */
	struct list_head list;

	u64 start_addr;         /* Memory Range start physical addr */
	u64 length;             /* Memory Range length */
	u64 slot;		/* Memory Range slot */
	u64 node;               /* Memory Range node */
	unsigned int enabled:1;
};

struct sunway_ged_device {
	struct device *dev;
	void __iomem *membase;
	void *driver_data;
	spinlock_t lock;
	struct list_head dev_list;
};

#ifdef CONFIG_MEMORY_HOTPLUG
static int sunway_memory_enable_device(struct sunway_memory_device *mem_device)
{
	int num_enabled = 0;
	int result = 0;

	if (mem_device->enabled) {	/* just sanity check...*/
		num_enabled++;
		goto out;
	}

	/*
	 * If the memory block size is zero, please ignore it.
	 * Don't try to do the following memory hotplug flowchart.
	 */
	if (!mem_device->length)
		goto out;

	result = add_memory(mem_device->node, mem_device->start_addr, mem_device->length, MHP_NONE);
	/*
	 * If the memory block has been used by the kernel, add_memory()
	 * returns -EEXIST. If add_memory() returns the other error, it
	 * means that this memory block is not used by the kernel.
	 */
	if (result && result != -EEXIST)
		goto out;

	mem_device->enabled = 1;

	/*
	 * Add num_enable even if add_memory() returns -EEXIST, so the
	 * device is bound to this driver.
	 */
	num_enabled++;
out:
	if (!num_enabled) {
		dev_err(mem_device->device->dev, "add_memory failed\n");
		return -EINVAL;
	}

	return 0;
}

static void sunway_memory_device_remove(struct sunway_ged_device *device)
{
	struct sunway_memory_device *mem_dev, *n;
	unsigned long start_addr, length, slot;

	if (!device)
		return;

	start_addr = readq(device->membase + OFFSET_START_ADDR);
	length = readq(device->membase + OFFSET_LENGTH);
	slot = readq(device->membase + OFFSET_SLOT);

	list_for_each_entry_safe(mem_dev, n, &device->dev_list, list) {
		if (!mem_dev->enabled)
			continue;

		if ((start_addr == mem_dev->start_addr) &&
				(length == mem_dev->length)) {
			remove_memory(start_addr, length);
			list_del(&mem_dev->list);
			kfree(mem_dev);
		}
	}

	writeq(slot, device->membase + OFFSET_SLOT);
}

static int sunway_memory_device_add(struct sunway_ged_device *device)
{
	struct sunway_memory_device *mem_device;
	int result;

	if (!device)
		return -EINVAL;

	mem_device = kzalloc(sizeof(struct sunway_memory_device), GFP_KERNEL);
	if (!mem_device)
		return -ENOMEM;

	INIT_LIST_HEAD(&mem_device->list);
	mem_device->device = device;

	/* Get the range from the IO */
	mem_device->start_addr = readq(device->membase + OFFSET_START_ADDR);
	mem_device->length = readq(device->membase + OFFSET_LENGTH);
	mem_device->slot = readq(device->membase + OFFSET_SLOT);
	mem_device->node = readq(device->membase + OFFSET_NODE);

	result = sunway_memory_enable_device(mem_device);
	if (result) {
		dev_err(device->dev, "sunway_memory_enable_device() error\n");
		sunway_memory_device_remove(device);

		return result;
	}

	list_add_tail(&mem_device->list, &device->dev_list);
	dev_dbg(device->dev, "Memory device configured\n");

	return 1;
}
#endif

#ifdef CONFIG_HOTPLUG_CPU
static int sunway_cpu_device_add(struct sunway_ged_device *device)
{
	struct device *dev;
	int cpuid;

	cpuid = readq(device->membase + OFFSET_CPUID);

	if (!device)
		return -EINVAL;
	set_cpu_present(cpuid, true);
	dev = get_cpu_device(cpuid);
	if (device_attach(dev) >= 0)
		return 1;
	dev_err(dev, "Processor driver could not be attached\n");
	set_cpu_present(cpuid, false);
	return 0;
}

static void sunway_cpu_device_del(struct sunway_ged_device *device)
{
	struct device *dev;
	int cpuid;

	cpuid = readq(device->membase + OFFSET_CPUID);

	if (!device)
		return;
	set_cpu_present(cpuid, false);
	dev = get_cpu_device(cpuid);
	device_release_driver(dev);

	writeq(cpuid, device->membase + OFFSET_CPUID);
}
#endif

static irqreturn_t sunwayged_ist(int irq, void *data)
{
	struct sunway_ged_device *sunwayged_dev = data;
	unsigned int status;

	status = readq(sunwayged_dev->membase + OFFSET_STATUS);
#ifdef CONFIG_HOTPLUG_CPU
	/* through IO status to add or remove cpu  */
	if (status & SUNWAY_CPUHOTPLUG_ADD)
		sunway_cpu_device_add(sunwayged_dev);

	if (status & SUNWAY_CPUHOTPLUG_REMOVE)
		sunway_cpu_device_del(sunwayged_dev);
#endif

#ifdef CONFIG_MEMORY_HOTPLUG
	/* through IO status to add or remove memory device  */
	if (status & SUNWAY_MEMHOTPLUG_ADD)
		sunway_memory_device_add(sunwayged_dev);

	if (status & SUNWAY_MEMHOTPLUG_REMOVE)
		sunway_memory_device_remove(sunwayged_dev);
#endif

	return IRQ_HANDLED;
}

static irqreturn_t sunwayged_irq_handler(int irq, void *data)
{
	return IRQ_WAKE_THREAD;
}

static int sunwayged_probe(struct platform_device *pdev)
{
	struct resource *regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	int irq = platform_get_irq(pdev, 0);
	struct sunway_ged_device *geddev;
	struct device *dev;
	int irqflags;

	if (!regs) {
		dev_err(dev, "no registers defined\n");
		return -EINVAL;
	}

	geddev = devm_kzalloc(&pdev->dev, sizeof(*geddev), GFP_KERNEL);
	if (!geddev)
		return -ENOMEM;

	spin_lock_init(&geddev->lock);
	geddev->membase = devm_ioremap(&pdev->dev,
					regs->start, resource_size(regs));
	if (!geddev->membase)
		return -ENOMEM;

	INIT_LIST_HEAD(&geddev->dev_list);
	geddev->dev = &pdev->dev;
	irqflags = IRQF_SHARED;

	if (request_threaded_irq(irq, sunwayged_irq_handler, sunwayged_ist,
				irqflags, "SUNWAY:Ged", geddev)) {
		dev_err(dev, "failed to setup event handler for irq %u\n", irq);

		return -EINVAL;
	}

	platform_set_drvdata(pdev, geddev);

	return 0;
}

static int sunwayged_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id sunwayged_of_match[] = {
	{.compatible = "sw6,sunway-ged", },
	{ }
};
MODULE_DEVICE_TABLE(of, sunwayged_of_match);

#ifdef CONFIG_ACPI
static const struct acpi_device_id sunwayged_acpi_match[] = {
	{ "SUNW1000", 0 },
	{ }
};
MODULE_DEVICE_TABLE(acpi, sunwayged_acpi_match);
#endif

static struct platform_driver sunwayged_platform_driver = {
	.driver = {
		.name		= "sunway-ged",
		.of_match_table	= sunwayged_of_match,
		.acpi_match_table = ACPI_PTR(sunwayged_acpi_match),
	},
	.probe			= sunwayged_probe,
	.remove			= sunwayged_remove,
};
module_platform_driver(sunwayged_platform_driver);

MODULE_AUTHOR("Lu Feifei");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Sunway ged driver");
