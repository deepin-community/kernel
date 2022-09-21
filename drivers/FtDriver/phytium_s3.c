/*
 *  Phytium S3 device only for S3 resume CPU register
 */

#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/cputypes.h>

#ifdef CONFIG_PM
extern int phytium_uart_suspend(void);
extern int phytium_uart_resume(void);
extern int phytium_uart_port_init(void);


/*--------- pinctrl resume  start-----------*/
#define PHYTIUM_PINCTRL_BASE 0x28180000
#define PHYTIUM_PINCTRL_SIZE 0x500
#define PHYTIUM_PINCTRL_FUNC_SEL_OFFSE 0x200
#define PHYTIUM_PINCTRL_FUNC_SIZE 12
#define PHYTIUM_PINCTRL_IO_DELAY_OFFSE 0x300
#define PHYTIUM_PINCTRL_IO_DELAY_SIZE ((0x398 - 0x300)/4 +1)
#define PHYTIUM_PINCTRL_DRIVING_OFFSE 0x488


void __iomem *pinctrl_ioaddr = NULL;
unsigned int phytium_pinctrl_func_val[PHYTIUM_PINCTRL_FUNC_SIZE] = {0};
unsigned int phytium_pinctrl_delay_val[PHYTIUM_PINCTRL_IO_DELAY_SIZE] = {0};
unsigned int phytium_pinctrl_driving_val = 0;

int phytium_pinctrl_early_init(void)
{
	if (!cpu_is_phytium()) {
		return 0;
	}

	pinctrl_ioaddr = ioremap(PHYTIUM_PINCTRL_BASE, PHYTIUM_PINCTRL_SIZE);
	if(pinctrl_ioaddr == NULL) {
		printk(KERN_ERR "phytium_pinctrl_early_init ioremap error \n\r");
		return -1;
	}

	return 0;
}

int phytium_pinctrl_suspend(void)
{
	int i;
	void __iomem * pinctrl_func_reg = pinctrl_ioaddr + PHYTIUM_PINCTRL_FUNC_SEL_OFFSE;

	if (pinctrl_ioaddr == NULL) {
		printk(KERN_ERR "%s pinctrl_ioaddr is NULL \n\r", __func__);
		return -1;
	}

	for(i = 0; i < PHYTIUM_PINCTRL_FUNC_SIZE; i++) {
		phytium_pinctrl_func_val[i] = readl(pinctrl_func_reg + i * 4);
	}

	void __iomem * pinctrl_delay_reg = pinctrl_ioaddr + PHYTIUM_PINCTRL_IO_DELAY_OFFSE;
	for(i = 0; i < PHYTIUM_PINCTRL_IO_DELAY_SIZE; i++) {
		phytium_pinctrl_delay_val[i] = readl(pinctrl_delay_reg + i * 4);
	}

	phytium_pinctrl_driving_val = readl(pinctrl_ioaddr + PHYTIUM_PINCTRL_DRIVING_OFFSE);

	return 0;
}

int phytium_pinctrl_resume(void)
{
	int i = 0;
	unsigned int temp =   0;
	void __iomem * pinctrl_func_reg = pinctrl_ioaddr + PHYTIUM_PINCTRL_FUNC_SEL_OFFSE;

	if (pinctrl_ioaddr == NULL) {
		printk(KERN_ERR "%s pinctrl_ioaddr is NULL \n\r", __func__);
		return -1;
	}

	for(i = 0; i < PHYTIUM_PINCTRL_FUNC_SIZE; i++) {
		temp = readl(pinctrl_func_reg + i * 4);
		if (temp != phytium_pinctrl_func_val[i]) {
			writel (phytium_pinctrl_func_val[i], pinctrl_func_reg + i * 4);
		}
	}

	void __iomem * pinctrl_delay_reg = pinctrl_ioaddr + PHYTIUM_PINCTRL_IO_DELAY_OFFSE;

	for(i = 0; i < PHYTIUM_PINCTRL_IO_DELAY_SIZE; i++) {
		temp = readl(pinctrl_delay_reg + i * 4);
		if (temp != phytium_pinctrl_delay_val[i]) {
			writel (phytium_pinctrl_delay_val[i], pinctrl_delay_reg + i * 4);
		}
	}

	temp = readl(pinctrl_ioaddr + PHYTIUM_PINCTRL_DRIVING_OFFSE);
	if (temp != phytium_pinctrl_driving_val) {
		writel (phytium_pinctrl_driving_val, pinctrl_ioaddr + PHYTIUM_PINCTRL_DRIVING_OFFSE);
	}

	return 0;
}
/*--------- pinctrl resume  end-----------*/

/* phytium s3 ops */
struct  phytium_s3_ops {
    char * name;
    int (*init)(void);
    int (*suspend)(void);
    int (*resume)(void);
};

/* resgiter s3 ops */
struct  phytium_s3_ops phytium_s3_ops_list[] = {
    { "uart", phytium_uart_port_init, phytium_uart_suspend, phytium_uart_resume},
    { "pinctrl", phytium_pinctrl_early_init, phytium_pinctrl_suspend, phytium_pinctrl_resume},
    {NULL, NULL, NULL, NULL},
};

/************* define platform_device struct *******************/
static struct platform_device phytium_s3_device = {
    .name   = "phytium-s3",
    .id     = -1,
};


static int phytium_s3_probe(struct platform_device *pdev)
{
    return 0;
}

static int phytium_s3_init_early(void)
{
    int id = 0,ret = 0;

    while(phytium_s3_ops_list[id].name) {
        if(phytium_s3_ops_list[id].init)
            ret |= (phytium_s3_ops_list[id].init)();

        id++;
    }

    return ret;
}

static int phytium_s3_suspend(struct platform_device * pdev, pm_message_t state)
{
    int id = 0,ret = 0;

    while(phytium_s3_ops_list[id].name) {
        if(phytium_s3_ops_list[id].suspend)
            ret |= (phytium_s3_ops_list[id].suspend)();

        id++;
    }

    return ret;
}

static int phytium_s3_resume(struct platform_device * pdev)
{
    int id = 0,ret = 0;

    while(phytium_s3_ops_list[id].name) {
        if(phytium_s3_ops_list[id].resume)
            ret |= (phytium_s3_ops_list[id].resume)();

        id++;
    }

    return ret;
}
#endif

static struct platform_driver phytium_s3_driver = {
    .probe      = phytium_s3_probe,
#ifdef CONFIG_PM
    .suspend    = phytium_s3_suspend,
    .resume     = phytium_s3_resume,
#endif
    .driver     = {
        .name   = "phytium-s3",
        .owner  = THIS_MODULE,
    }
};

static int __init phytium_s3_init(void)
{
    int ret;

    if (!cpu_is_phytium()) {
        return 0;
    }

    ret = phytium_s3_init_early();
    if(ret) {
        printk(KERN_ERR "phytium_s3_port_init return error \n\r");
        return ret;
    }

    ret = platform_device_register(&phytium_s3_device);
    if(ret) {
        printk(KERN_ERR " phytium_s3_device register fail \n");
        return ret;
    }

    ret = platform_driver_register(&phytium_s3_driver);
    if(ret) {
        printk(KERN_ERR " phytium_s3_driver register fail \n");
        platform_device_unregister(&phytium_s3_device);
        return ret;
    }

	return 0;
}


static void __exit phytium_s3_exit(void)
{
    if (!cpu_is_phytium()) {
        return;
    }

    platform_driver_unregister(&phytium_s3_driver);
    platform_device_unregister(&phytium_s3_device);
}

module_init(phytium_s3_init);
module_exit(phytium_s3_exit);


