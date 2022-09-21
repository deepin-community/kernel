#include <linux/types.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/rcupdate.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/acpi.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <asm/io.h>
#include <linux/cputypes.h>

MODULE_AUTHOR("huangbibo");
MODULE_DESCRIPTION("phytium lpc driver");
MODULE_LICENSE("GPL");

#define EC_EVENT_BIT        (1 << 11)
#define I8042_KEY_BIT       (1 << 1)
#define I8042_TOUCH_BIT     (1 << 12)


#define LPC_REG_BASE        0x7FFFF00
#define LPC_REG_SIZE        0x100

#define LPC_STATUS_REG      0xF4
#define LPC_INTERRUPT_REG   0xF0
#define LPC_INT_EN_REG      0xD8

static int ec_init_flag = 0;
static int kbd_init_flag = 0;
static void __iomem *lpc_iobase;
static void __iomem *lpc_base_address;
static int lpc_irq = -1;

void get_lpc_iobase(void __iomem **iobase)
{
    *iobase = lpc_iobase;
}
EXPORT_SYMBOL(get_lpc_iobase);

void get_lpc_reg_base(void __iomem **iobase)
{
    *iobase = lpc_base_address;
}
EXPORT_SYMBOL(get_lpc_reg_base);

int get_lpc_status_reg(void)
{
	return readl(lpc_base_address + LPC_STATUS_REG);
}
EXPORT_SYMBOL(get_lpc_status_reg);

int get_lpc_int_reg(void)
{
    return readl(lpc_base_address + LPC_STATUS_REG);
}
EXPORT_SYMBOL(get_lpc_int_reg);

void set_lpc_int_reg(int data)
{
	writel(data, lpc_base_address + LPC_INTERRUPT_REG);
}
EXPORT_SYMBOL(set_lpc_int_reg);

int get_lpc_irq(void)
{
    return lpc_irq;
}
EXPORT_SYMBOL(get_lpc_irq);

void ec_int_init_set(void)
{
    ec_init_flag = 1;
}
EXPORT_SYMBOL(ec_int_init_set);

void kbd_int_init_set(void)
{
    kbd_init_flag = 1;
}
EXPORT_SYMBOL(kbd_int_init_set);

int is_lpc_int_init_done(void)
{
    return (ec_init_flag && kbd_init_flag);
}
EXPORT_SYMBOL(is_lpc_int_init_done);

void lpc_int_disable(void)
{
	if (lpc_base_address == NULL) {
		return;
	}

    /* clear all interrupt status */
    writel(0, lpc_base_address + LPC_INTERRUPT_REG);

    /* disable lpc interrupt */
    writel(0xffffffff, lpc_base_address + LPC_INT_EN_REG);

    printk(KERN_INFO "lpc_int_disable... \r\n");
    return;
}
EXPORT_SYMBOL(lpc_int_disable);

void lpc_int_enable(void)
{

	if (lpc_base_address == NULL) {
		return;
	}

    /* clear all interrupt status */
    writel(0, lpc_base_address + LPC_INTERRUPT_REG);

    /* enable lpc interrupt */
    writel(0x0, lpc_base_address + LPC_INT_EN_REG);

    printk(KERN_INFO "lpc_int_enable ... \r\n");
}
EXPORT_SYMBOL(lpc_int_enable);


void lpc_reg_show(void)
{
    int reg_data = 0;
    int reg_addr = 0;

    for(reg_addr = LPC_INT_EN_REG+4; reg_addr <= LPC_STATUS_REG; reg_addr+=4) {
        reg_data =readl(lpc_base_address + reg_addr);
        printk(KERN_INFO "debug --> lpc_reg_show reg_addr = 0x%x, reg_val = 0x%x \r\n", reg_addr, reg_data);
    }
}

static int lpc_probe(struct platform_device *dev)
{
	struct resource *res;

	res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if(NULL == res){
		printk(KERN_ERR "get resource failed\n");
		return EFAULT;
	}

	lpc_iobase = devm_ioremap_resource(&dev->dev, res);

	if( IS_ERR(lpc_iobase) ){
		lpc_iobase = NULL;
		printk(KERN_ERR "ioremap failed\n");
		return EFAULT;
	}

	lpc_irq = platform_get_irq(dev, 0);
	if( lpc_irq < 0 ){
		printk(KERN_ERR "get lpc irq failed\n");
		return EFAULT;
	}

	if (res->end < (res->start + LPC_REG_BASE)) {
		lpc_base_address = ioremap((unsigned long)res->start + LPC_REG_BASE, LPC_REG_SIZE);
		if(IS_ERR(lpc_base_address)){
			lpc_base_address = NULL;
			printk(KERN_ERR "ioremap lpc register failed\n");
			return EFAULT;


		}
	} else {
		lpc_base_address = lpc_iobase + LPC_REG_BASE;
	}

	lpc_int_disable();

	return 0;
}

static const struct acpi_device_id lpc_acpi_match[] = {
        { "LPC0001", 0 },
        { "PHYT0007", 0 },
        {}
};
MODULE_DEVICE_TABLE(acpi, lpc_acpi_match);

static struct platform_driver lpc_driver = {
	.driver		= {
		.name	= "phytium_lpc",
                .acpi_match_table = ACPI_PTR(lpc_acpi_match),
	},
	.probe		= lpc_probe,
};

static int __init lpc_init(void)
{
        int ret;
        lpc_iobase = NULL;

		if (!cpu_is_phytium())
		{
			return 0;
		}

        ret = platform_driver_register(&lpc_driver);
        if (ret)
        {
            pr_info("platform_driver_register fail\n");
            return 0;
        }

	return 0;

}

static void __exit lpc_exit(void)
{
	if (!cpu_is_phytium())
	{
		return 0;
	}

    platform_driver_unregister(&lpc_driver);
}

module_init(lpc_init);
module_exit(lpc_exit);
