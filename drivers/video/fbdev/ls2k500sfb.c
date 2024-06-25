// SPDX-License-Identifier: GPL-2.0
/*
 *
 *  linux/drivers/video/ls2k500sfb.c
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>
#include <linux/aperture.h>

#include <linux/uaccess.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/platform_data/simplefb.h>
#include <linux/umh.h>
#include <linux/vt_kern.h>
#include <linux/kbd_kern.h>
#include <linux/console.h>
#include <linux/acpi.h>
#include <linux/gpio.h>
#include <linux/smp.h>
#include <linux/nmi.h>
#include <linux/gpio/machine.h>

static char mode_option[32] = "1280x1024-32@2M";
module_param_string(mode, mode_option, sizeof(mode_option), 0444);
static int useshell;
module_param(useshell, int, 0664);
static int totty = 18;
module_param(totty, int, 0664);
static int resetdelay = 60;
module_param(resetdelay, int, 0664);
static int resetbootwait = 10;
module_param(resetbootwait, int, 0664);
static int GPIO = 14;
module_param(GPIO, int, 0664);
struct ls2k500sfb_struct {
	struct pci_dev *dev;
	struct platform_device *pd;
	struct workqueue_struct *wq;
	struct work_struct work;
	struct delayed_work redraw_work;
	int running;
	unsigned long reset_time;
	char *penv;
	char saved_env[16];
};

static int saved_console;
static unsigned long mscycles;
static atomic_t waiting_for_pciebreak_ipi;

static int switch_console(int console)
{
	struct file	*filp;

	filp = filp_open("/dev/tty1", O_RDWR, 0);
	if (IS_ERR(filp))
		return -ENODEV;

	vfs_ioctl(filp, VT_ACTIVATE, console + 1);
	filp_close(filp, NULL);
	return 0;
}
static void ls2k500sfb_pciebreak_func(void *unused)
{
	atomic_dec(&waiting_for_pciebreak_ipi);

	while (atomic_read(&waiting_for_pciebreak_ipi))
		cpu_relax();
}

static void pciebreak_smp_send_stop(int ms)
{
	/* Wait at most 100 msecond for the other cpus to stop */
	unsigned long max_cycles =  mscycles * ms;
	unsigned long start_time = get_cycles();

	atomic_set(&waiting_for_pciebreak_ipi, num_online_cpus());
	smp_call_function(ls2k500sfb_pciebreak_func, NULL, false);
	while ((atomic_read(&waiting_for_pciebreak_ipi) > 1)
		&& get_cycles() - start_time < max_cycles) {
		cpu_relax();
	}
	if (atomic_read(&waiting_for_pciebreak_ipi) > 1)
		pr_emerg("Non-pciebreaking CPUs did not react to IPI\n");
}
static void ls2k500sfb_redraw_fn(struct work_struct *work)
{
	struct ls2k500sfb_struct *priv =
		container_of(work, struct ls2k500sfb_struct, redraw_work.work);
	/*restore resolution info */
	if (memcmp(priv->penv, priv->saved_env, sizeof(priv->saved_env)))
		memcpy(priv->penv, priv->saved_env, sizeof(priv->saved_env));
	switch_console(saved_console);
}

static unsigned long event_jiffies;
static void ls2k500sfb_events_fn(struct work_struct *work)
{
	struct ls2k500sfb_struct *priv = container_of(work, struct ls2k500sfb_struct, work);
	struct pci_dev *pdev = priv->dev;
	struct pci_dev *ppdev = pdev->bus->self;
	uint32_t i, d, timeout, retry = 0;
	static const uint32_t index[] = {
		0x10, 0x14, 0x18, 0x1c, 0x20, 0x24, 0x30, 0x3c, 0x54, 0x58, 0x78, 0x7c, 0x80, 4
	};

	static uint32_t data[sizeof(index) / 4];
	static const uint32_t cindex[] = { 0x10, 0x3c, 4 };

	static uint32_t cdata[sizeof(cindex) / 4];
	static uint32_t d80c, d71c, ctrl;
	static void *p;

	if (!priv->running) {
		for (i = 0; i < ARRAY_SIZE(index); i++)
			pci_read_config_dword(ppdev, index[i], &data[i]);
		for (i = 0; i < ARRAY_SIZE(cindex); i++)
			pci_read_config_dword(pdev, cindex[i], &cdata[i]);
		if (ppdev->vendor == 0x14) {
			pci_read_config_dword(ppdev, 0x80c, &d80c);
			d80c = (d80c & ~(3 << 17)) | (1 << 17);

			pci_read_config_dword(ppdev, 0x71c, &d71c);
			d71c |= 1 << 26;

			p = pci_iomap(ppdev, 0, 0x100);
		}
		ctrl = readl(p);
		return;
	}
	local_bh_disable();
	pciebreak_smp_send_stop(100);
	wmb(); /* flush all write before we disable pcie window */
	pci_write_config_dword(ppdev, 0x18, 0);
	pci_write_config_dword(ppdev, 0x1c, 0);
	pci_write_config_dword(ppdev, 0x20, 0);
	event_jiffies = jiffies;
	atomic_set(&waiting_for_pciebreak_ipi, 0);
	wmb(); /* flush all write after change pcie window */
	local_bh_enable();
	if (ppdev->vendor == 0x14) {
		timeout = 10000;
		while (timeout) {
			pci_read_config_dword(ppdev, 0x10, &d);
			d &= ~0xf;
			if (!d)
				break;
			mdelay(1);
			timeout--;
		};
		if (!timeout)
			pr_info("bar not clear 0\n");

		pci_read_config_dword(ppdev, 0x0, &d);
		pr_info("pcie port deviceid=0x%x recover begin\n", d);
retrain:
		while (1) {
			pci_write_config_dword(ppdev, index[0], data[0]);
			pci_read_config_dword(ppdev, index[0], &d);
			d &= ~0xf;
			if (d)
				break;
			mdelay(1);
		}

		while (1) {
			for (i = 0; i < ARRAY_SIZE(index); i++) {
				if (index[i] != 0x18 && index[i] != 0x1c && index[i] != 0x20)
					pci_write_config_dword(ppdev, index[i], data[i]);
			}
			pci_write_config_dword(ppdev, 0x80c, d80c);
			pci_write_config_dword(ppdev, 0x71c, d71c);

			pci_read_config_dword(ppdev, 0x10, &d);
			d &= ~0xf;
			if (d)
				break;
			mdelay(1);
		}

		timeout = 10000;

		writel(ctrl | 0x8, p);
		while (1) {
			d = readl(p + 0xc);
			if ((d & 0x11) == 0x11) {
				break;
			} else if (!timeout) {
				pr_info("pcie train failed status=0x%x\n", d);
				goto out;
			}
			mdelay(1);
			timeout--;
		}


		pr_info("pcie recovered done\n");

		if (!retry) {
			/*wait u-boot ddr config */
			set_current_state(TASK_UNINTERRUPTIBLE);
			schedule_timeout(HZ*resetbootwait);
			set_current_state(TASK_RUNNING);
			pci_read_config_dword(ppdev, 0x10, &d);
			d &= ~0xf;
			if (!d) {
				retry = 1;
				goto retrain;
			}
		}
	} else {
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(HZ*resetbootwait);
		set_current_state(TASK_RUNNING);
	}
	local_bh_disable();
	pciebreak_smp_send_stop(10000);
	wmb(); /* flush all write before we update pcie window */
	for (i = 0; i < ARRAY_SIZE(index); i++)
		pci_write_config_dword(ppdev, index[i], data[i]);

	for (i = 0; i < ARRAY_SIZE(cindex); i++)
		pci_write_config_dword(pdev, cindex[i], cdata[i]);
	atomic_set(&waiting_for_pciebreak_ipi, 0);
	wmb(); /* flush all write after we update pcie window */
	local_bh_enable();


	pr_info("redraw console\n");

	saved_console = fg_console;
	switch_console(fg_console > 0?fg_console - 1 : fg_console + 1);
	queue_delayed_work(priv->wq, &priv->redraw_work, HZ);
out:
	priv->running = 0;
}

irqreturn_t ls2k500sfb_interrupt(int irq, void *arg)
{
	struct ls2k500sfb_struct *priv = arg;
	struct pci_dev *pdev = priv->dev;

	if (irq == pdev->irq)
		pr_info("ls2k500sfb pcie interrupt\n");
	else
		pr_info("ls2k500sfb gpio interrupt\n");
	if (system_state != SYSTEM_RUNNING)
		return IRQ_HANDLED;

	if (!priv->running) {
		if (!resetdelay || time_after(jiffies, priv->reset_time + resetdelay * HZ)) {
			priv->running = 1;
			queue_work(priv->wq, &priv->work);
		}
		priv->reset_time = jiffies;
	}
	return IRQ_HANDLED;
}

#ifdef CONFIG_LOONGARCH
#define GPIO_OEN ((void *)IO_BASE+0x1fe00000+0x500)
#define GPIO_FUNCEN ((void *)IO_BASE+0x1fe00000+0x504)
#define GPIO_OUT ((void *)IO_BASE+0x1fe00000+0x508)
#define GPIO_IN ((void *)IO_BASE+0x1fe00000+0x50c)
#define GPIO_INTPOL ((void *)IO_BASE+0x1fe00000+0x510)
#define GPIO_INTEN ((void *)IO_BASE+0x1fe00000+0x514)

static int gpiochip_match_name(struct gpio_chip *chip, void *data)
{
	const char *name = data;

	return !strcmp(chip->label, name);
}
static int get_gpio_irq_from_acpi_table(int gpio)
{
	struct gpio_chip *chip;
	struct gpio_desc *desc;

	chip = gpiochip_find("LOON0007:00", gpiochip_match_name);
	if (!chip)
		return -ENOENT;
	desc = gpiochip_request_own_desc(chip, gpio, "reboot", GPIO_LOOKUP_FLAGS_DEFAULT, GPIOD_IN);
	if (!desc)
		return -ENOENT;
	return gpiod_to_irq(desc);
}

static int get_gpio_irq_from_acpi_gsi(int gpio)
{
	int gsi = 16 + (gpio & 7);

	return  acpi_register_gsi(NULL, gsi, ACPI_EDGE_SENSITIVE, ACPI_ACTIVE_LOW);
}

static int register_gpio_reboot_handler(struct ls2k500sfb_struct *priv)
{
	int irq = get_gpio_irq_from_acpi_table(GPIO);

	if (irq < 0) {
		irq = get_gpio_irq_from_acpi_gsi(GPIO);
		pr_notice("gsi gpio irq %d\n", irq);
	} else
		pr_notice("acpi gpio irq %d\n", irq);
	writel(readl(GPIO_OEN) | (0x1 << GPIO), GPIO_OEN);
	writel(readl(GPIO_FUNCEN) & ~(0x1 << GPIO), GPIO_FUNCEN);
	writel(readl(GPIO_INTPOL) & ~(0x1 << GPIO), GPIO_INTPOL);
	writel(readl(GPIO_INTEN) | (0x1 << GPIO), GPIO_INTEN);
	if (request_irq(irq, ls2k500sfb_interrupt, IRQF_SHARED | IRQF_TRIGGER_FALLING,
				"ls2k500sfb", priv))
		pr_err("request_irq(%d) failed\n", irq);
	return 0;
}
#endif

static const struct fb_fix_screeninfo simplefb_fix = {
	.id		= "simple",
	.type		= FB_TYPE_PACKED_PIXELS,
	.visual		= FB_VISUAL_TRUECOLOR,
	.accel		= FB_ACCEL_NONE,
};

static const struct fb_var_screeninfo simplefb_var = {
	.height		= -1,
	.width		= -1,
	.activate	= FB_ACTIVATE_NOW,
	.vmode		= FB_VMODE_NONINTERLACED,
};

#define PSEUDO_PALETTE_SIZE 16
struct simplefb_par {
	char *penv;
	char *preg;
	u32 palette[PSEUDO_PALETTE_SIZE];
};

static u_long get_line_length(int xres_virtual, int bpp)
{
	u_long length;

	length = xres_virtual * bpp;
	length = (length + 31) & ~31;
	length >>= 3;
	return length;
}

static int simplefb_check_var(struct fb_var_screeninfo *var,
			 struct fb_info *info)
{
	u_long line_length;

	/*
	 *  FB_VMODE_CONUPDATE and FB_VMODE_SMOOTH_XPAN are equal!
	 *  as FB_VMODE_SMOOTH_XPAN is only used internally
	 */

	if (var->vmode & FB_VMODE_CONUPDATE) {
		var->vmode |= FB_VMODE_YWRAP;
		var->xoffset = info->var.xoffset;
		var->yoffset = info->var.yoffset;
	}

	/*
	 *  Some very basic checks
	 */
	if (!var->xres)
		var->xres = 1;
	if (!var->yres)
		var->yres = 1;
	if (var->xres > var->xres_virtual)
		var->xres_virtual = var->xres;
	if (var->yres > var->yres_virtual)
		var->yres_virtual = var->yres;
	if (var->bits_per_pixel <= 16)
		var->bits_per_pixel = 16;
	else if (var->bits_per_pixel <= 32)
		var->bits_per_pixel = 32;
	else
		return -EINVAL;

	if (var->xres_virtual < var->xoffset + var->xres)
		var->xres_virtual = var->xoffset + var->xres;
	if (var->yres_virtual < var->yoffset + var->yres)
		var->yres_virtual = var->yoffset + var->yres;

	/*
	 *  Memory limit
	 */
	line_length =
	    get_line_length(var->xres_virtual, var->bits_per_pixel);
	if (line_length * var->yres_virtual > info->fix.smem_len)
		return -ENOMEM;

	/*
	 * Now that we checked it we alter var. The reason being is that the video
	 * mode passed in might not work but slight changes to it might make it
	 * work. This way we let the user know what is acceptable.
	 */
	switch (var->bits_per_pixel) {
	case 16: /* BGR 565 */
		var->red.offset = 11;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 0;
		var->blue.length = 5;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	case 32:		/* BGRA 8888 */
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 24;
		var->transp.length = 8;
		break;
	}
	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;

	return 0;
}

static int simplefb_set_par(struct fb_info *info)
{
	struct simplefb_par *par = info->par;
	int reg_val;

	info->fix.line_length = get_line_length(info->var.xres_virtual,
						info->var.bits_per_pixel);
	sprintf(par->penv, "video=%dx%d-%d@2M",
			info->var.xres_virtual,
			info->var.yres_virtual,
			info->var.bits_per_pixel);

	reg_val = readl(par->preg);
	writel(reg_val + 1, par->preg);

	return 0;
}

static int simplefb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			      u_int transp, struct fb_info *info)
{
	u32 *pal = info->pseudo_palette;
	u32 cr = red >> (16 - info->var.red.length);
	u32 cg = green >> (16 - info->var.green.length);
	u32 cb = blue >> (16 - info->var.blue.length);
	u32 value;

	if (regno >= PSEUDO_PALETTE_SIZE)
		return -EINVAL;

	value = (cr << info->var.red.offset) |
		(cg << info->var.green.offset) |
		(cb << info->var.blue.offset);
	if (info->var.transp.length > 0) {
		u32 mask = (1 << info->var.transp.length) - 1;

		mask <<= info->var.transp.offset;
		value |= mask;
	}
	pal[regno] = value;

	return 0;
}


static void simplefb_destroy(struct fb_info *info)
{
	if (info->screen_base)
		iounmap(info->screen_base);
}

static const struct fb_ops simplefb_ops = {
	.owner		= THIS_MODULE,
	.fb_destroy	= simplefb_destroy,
	.fb_setcolreg	= simplefb_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_check_var	= simplefb_check_var,
	.fb_set_par	= simplefb_set_par,
};

static struct simplefb_format simplefb_formats[] = SIMPLEFB_FORMATS;

struct simplefb_params {
	u32 width;
	u32 height;
	u32 stride;
	struct simplefb_format *format;
};

static int simplefb_parse_pd(struct platform_device *pdev,
			     struct simplefb_params *params)
{
	struct simplefb_platform_data *pd = dev_get_platdata(&pdev->dev);
	int i;

	params->width = pd->width;
	params->height = pd->height;
	params->stride = pd->stride;

	params->format = NULL;
	for (i = 0; i < ARRAY_SIZE(simplefb_formats); i++) {
		if (strcmp(pd->format, simplefb_formats[i].name))
			continue;

		params->format = &simplefb_formats[i];
		break;
	}

	if (!params->format) {
		dev_err(&pdev->dev, "Invalid format value\n");
		return -EINVAL;
	}

	return 0;
}

static int simplefb_probe(struct platform_device *pdev)
{
	int ret;
	struct simplefb_params params;
	struct fb_info *info;
	struct simplefb_par *par;
	struct resource *mem, *envmem, *regmem;

	ret = simplefb_parse_pd(pdev, &params);

	if (ret)
		return ret;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	envmem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	regmem = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!mem || !envmem || !regmem) {
		dev_err(&pdev->dev, "No memory resource\n");
		return -EINVAL;
	}

	info = framebuffer_alloc(sizeof(struct simplefb_par), &pdev->dev);
	if (!info)
		return -ENOMEM;
	platform_set_drvdata(pdev, info);

	par = info->par;
	par->penv = ioremap(envmem->start, resource_size(envmem));
	par->preg = ioremap(regmem->start, resource_size(regmem));

	info->fix = simplefb_fix;
	info->fix.smem_start = mem->start;
	info->fix.smem_len = resource_size(mem);
	info->fix.line_length = params.stride;

	info->var = simplefb_var;
	info->var.xres = params.width;
	info->var.yres = params.height;
	info->var.xres_virtual = params.width;
	info->var.yres_virtual = params.height;
	info->var.bits_per_pixel = params.format->bits_per_pixel;
	info->var.red = params.format->red;
	info->var.green = params.format->green;
	info->var.blue = params.format->blue;
	info->var.transp = params.format->transp;

	ret = devm_aperture_acquire_for_platform_device(pdev,
							info->fix.smem_start,
							info->fix.smem_len);
	if (ret) {
		dev_info(&pdev->dev, "cannot acquire aperture\n");
		goto error_fb_release;
	}

	info->fbops = &simplefb_ops;
	info->flags = 0;
	info->screen_base = ioremap_wc(info->fix.smem_start,
				       info->fix.smem_len);
	if (!info->screen_base) {
		ret = -ENOMEM;
		goto error_fb_release;
	}
	info->pseudo_palette = par->palette;

	dev_info(&pdev->dev, "framebuffer at 0x%lx, 0x%x bytes, mapped to 0x%p\n",
			     info->fix.smem_start, info->fix.smem_len,
			     info->screen_base);
	dev_info(&pdev->dev, "format=%s, mode=%dx%dx%d, linelength=%d\n",
			     params.format->name,
			     info->var.xres, info->var.yres,
			     info->var.bits_per_pixel, info->fix.line_length);

	ret = register_framebuffer(info);
	if (ret < 0) {
		dev_err(&pdev->dev, "Unable to register simplefb: %d\n", ret);
		goto error_fb_release;
	} else
		dev_info(&pdev->dev, "fb%d: simplefb registered!\n", info->node);

	local_irq_disable();
	mscycles = get_cycles();
	mdelay(1);
	mscycles = get_cycles() - mscycles;
	local_irq_enable();

	return ret;
error_fb_release:
	framebuffer_release(info);
	return ret;
}

static int simplefb_remove(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);

	unregister_framebuffer(info);
	framebuffer_release(info);

	return 0;
}

static struct platform_driver simplefb_driver = {
	.driver = {
		.name = "virt-framebuffer",
	},
	.probe = simplefb_probe,
	.remove = simplefb_remove,
};

static void *kcs_data[2] = {&event_jiffies, &mscycles};
static int ls2k500sfb_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	struct simplefb_platform_data mode;
	struct resource res[3];
	struct platform_device *pd;
	struct ls2k500sfb_struct *priv;
	long phybase, videooffset, videomemorysize;
	char *pmode = mode_option;
	int depth;
	char *penv;
	int ret, i;

	if (!dev->bus->number || pci_enable_device(dev))
		return -ENODEV;
	priv = kzalloc(sizeof(struct ls2k500sfb_struct), GFP_KERNEL);
	priv->dev = dev;

	/* pcimem bar last 16M free, 2MB offset from free for framebuffer */
	phybase = pci_resource_start(dev, 0);
	phybase += pci_resource_len(dev, 0) - 0x1000000;
	penv = ioremap(phybase, 0x100000);
	/*env at last 16M's beginning, first env is video */
	if (!strncmp(penv, "video=", 6))
		pmode = penv + 6;

	priv->penv =  penv + 6;
	memcpy(priv->saved_env, priv->penv, sizeof(priv->saved_env));

	mode.width = simple_strtoul(pmode, &pmode, 0);
	pmode++;
	mode.height = simple_strtoul(pmode, &pmode, 0);
	pmode++;
	depth = simple_strtoul(pmode, &pmode, 0);
	if (pmode && pmode[0]) {
		pmode++;
		videooffset = simple_strtoul(pmode, &pmode, 0);
		if (pmode && pmode[0]) {
			switch (pmode[0]) {
			case 'M':
			case 'm':
				videooffset *= 0x100000;
				break;
			case 'K':
			case 'k':
				videooffset *= 1024;
				break;
			}
		}
	} else
		videooffset = 0x200000;
	mode.stride = mode.width * depth / 8;
	mode.format = depth == 32 ? "a8r8g8b8" : "r5g6b5";

	videomemorysize = 0x400000;

	memset(res, 0, sizeof(res));
	res[0].start = phybase + videooffset;
	res[0].end = phybase + videooffset + videomemorysize - 1;
	res[0].flags = IORESOURCE_MEM;
	res[0].parent = &dev->resource[0];

	res[1].start = phybase;
	res[1].end = phybase + 64 - 1;
	res[1].flags = IORESOURCE_MEM;
	res[1].parent = &dev->resource[0];

	res[2].start = phybase + 0x00f00014;
	res[2].end = phybase + 0x00f0001c - 1;
	res[2].flags = IORESOURCE_MEM;
	res[2].parent = &dev->resource[0];

	priv->pd = pd = platform_device_register_resndata(NULL, "virt-framebuffer", 0,
					res, 3, &mode, sizeof(mode));

	ret = platform_driver_register(&simplefb_driver);
	if (ret)
		return ret;
	priv->wq = create_singlethread_workqueue("ls2k500sfb wq");
	INIT_WORK(&priv->work, ls2k500sfb_events_fn);
	INIT_DELAYED_WORK(&priv->redraw_work, ls2k500sfb_redraw_fn);

	ls2k500sfb_events_fn(&priv->work);
	if (request_irq(dev->irq, ls2k500sfb_interrupt, IRQF_SHARED | IRQF_TRIGGER_RISING,
				"ls2k500sfb", priv))
		pr_err("request_irq(%d) failed\n", dev->irq);
 #ifdef CONFIG_LOONGARCH
	register_gpio_reboot_handler(priv);
 #endif
	pci_set_drvdata(dev, priv);
	for (i = 0; i < 5; i++) {
		res[0].start = phybase + 0x00f00000 + 0x1c*i;
		res[0].end = phybase + 0x00f00000 + 0x1c*(i+1) - 1;
		platform_device_register_resndata(NULL, "ipmi_ls2k500_si", i, res, 1,
						kcs_data, sizeof(kcs_data));
	}

	return PTR_ERR_OR_ZERO(pd);
}

static	void ls2k500sfb_remove(struct pci_dev *dev)
{
	struct ls2k500sfb_struct *priv = pci_get_drvdata(dev);

	platform_device_del(priv->pd);
}

static struct pci_device_id ls2k500sfb_devices[] = {
	{PCI_DEVICE(0x14, 0x1a05)},
	{0, 0, 0, 0, 0, 0, 0}
};
MODULE_DEVICE_TABLE(pci, ls2k500sfb_devices);

static struct pci_driver ls2k500sfb_driver = {
	.name = "ls2k500sfb",
	.id_table = ls2k500sfb_devices,
	.probe = ls2k500sfb_probe,
	.remove = ls2k500sfb_remove,
	.driver = {
		.name = "ls2k500sfb",
	},
};

static int __init ls2k500sfb_init(void)
{
	return pci_register_driver(&ls2k500sfb_driver);
}

module_init(ls2k500sfb_init);

#ifdef MODULE
static void __exit ls2k500sfb_exit(void)
{
	pci_unregister_driver(&ls2k500sfb_driver);
}

module_exit(ls2k500sfb_exit);
#endif

MODULE_LICENSE("GPL");
