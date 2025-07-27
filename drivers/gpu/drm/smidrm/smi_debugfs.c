// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.

#include <linux/pci.h>
#include <drm/drm_print.h>
#include "smi_drv.h"
#include "ddk768/ddk768_pwm.h"
#include <linux/vmalloc.h>
#include <linux/debugfs.h>
#include <drm/drm_debugfs.h>
#include <linux/uaccess.h>
#include "smi_debugfs.h"

extern int pwm_ctrl;
extern int smi_debug;
extern int force_connect;


static int debugfs_pwm_set(void *data, u64 val)
{
	int ctrl = *(u32*)data;

	if(val == ctrl) {
		pr_info("Nothing to do with %u\n", (int)val);
		return 0;
	} 

	if(ctrl) {
		if((val & 0xf) != (ctrl & 0xf)) {
			ddk768_pwmClose(ctrl & 0xf);
		} 
	
		if((val & ~0xf) != (ctrl & ~0xf)) {
			ddk768_pwmStop(ctrl & 0xf);
		}
	}

	if (val) {
		unsigned long pwm, divider, highCounter, lowCounter;

		pwm = val & 0xf;
		divider = (val & 0xf0) >> 4;
		highCounter = (val & 0xfff00) >> 8;
		lowCounter = (val & 0xfff00000) >> 20;

		if((val & 0xf) != (ctrl & 0xf)) {
			ddk768_pwmOpen(pwm);
		}

		if((val & ~0xf) != (ctrl & ~0xf)) {
			ddk768_pwmStart(pwm, divider, highCounter, lowCounter, 0);
		}
	}

	*(u32 *)data = val;
	pr_info("Setting pwm_ctrl to %u\n", (int)val);

	return 0;
}

static int debugfs_pwm_get(void *data, u64 *val)
{
	*val = *(u32 *)data;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_pwm, debugfs_pwm_get, debugfs_pwm_set, "%llu\n");


static struct smi_regs smiregs[] = {
	{0x60,0x130,"system configuration"},
	{0x80000,0x80028,"DC0 Graphic controller"},
	{0x80040,0x80068,"DC0 video controller"},
	{0x8802c,0x8802c,"LVDS controller"},
	{0x88000,0x88028,"DC1 Display controller"},
	{0x88040,0x88068,"DC1 video controller"},
};

static unsigned int reg_data;

static ssize_t reg_read(struct file *lp, char __user *user_data, size_t cnt, loff_t *lt)
{
	char data[11];
	unsigned long ret;
	if(*lt) return 0;
	snprintf(data, 11, "0x%08x", reg_data);
	ret = copy_to_user (user_data, data, 11);
	*lt += 11;
       	return 11;
}

static ssize_t reg_write(struct file *lp, const char *user_data, size_t cnt, loff_t *lt)
{
	char data[64] = {0};
	char *p;
	unsigned int offset;
	unsigned long ret;
	
	struct drm_device *dev = lp->private_data;
	struct smi_device *sdev  = dev->dev_private;
	void *base = sdev->rmmio, *addr;

	if(cnt > 64) return -1;
	ret = copy_from_user(data, user_data, cnt);
	p = data;
	offset = simple_strtoul(p, &p, 16);
	if( offset > 2*1024*1024 || offset%4) {
		pr_err("invalid address\n");
		return -1;
	}

	addr = base + offset;
	if(*p != '-') {
		reg_data = *(unsigned int *)addr;
	} else {
		p++;
		reg_data = simple_strtoul(p, &p, 16);
		*(unsigned int *)addr = reg_data;
	}

	return cnt;
}

static const struct file_operations reg_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = reg_read,
	.write = reg_write,
};
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
     void smi_debugfs_init(struct drm_minor *minor)
#else
     int smi_debugfs_init(struct drm_minor *minor)
#endif

{
	
	int i,j;
	struct smi_device *sdev;
	struct debugfs_regset32 *regset;
	int regnum = 0;
	struct debugfs_reg32 *regs = NULL;
	

	if (!minor || !minor->debugfs_root) {
		goto DEBUGFS_FAIL;
	}
	
	sdev  = minor->dev->dev_private;

	debugfs_create_u32("smi_debug", S_IRUGO | S_IWUSR, minor->debugfs_root, &smi_debug);

	debugfs_create_u32("nopnp", S_IRUGO | S_IWUSR, minor->debugfs_root, &force_connect);

	if(sdev->specId == SPC_SM768){

			debugfs_create_file_unsafe("pwm_ctl", S_IRUGO | S_IWUSR, minor->debugfs_root, &pwm_ctrl, &fops_pwm);


	regs = vzalloc(REGS_SIZE * sizeof(struct debugfs_reg32));
	if(!regs) {
		goto DEBUGFS_FAIL;
	}


	regset = kzalloc(sizeof(struct debugfs_regset32), GFP_KERNEL);
	if(!regset) {
		goto DEBUGFS_FAIL;
	}
	for(j = 0; j < sizeof(smiregs)/sizeof(struct smi_regs); j ++) {
		for(i = smiregs[j].offset0; i < smiregs[j].offset1 + 4; i = i + 4)
		{
			regs[regnum].name = smiregs[j].regname;
			regs[regnum].offset = i;
			regnum ++;
		}
	}

	regset->regs = regs;
	regset->base = sdev->rmmio;
	regset->nregs = regnum;

	debugfs_create_regset32("regdump", 0644, minor->debugfs_root, regset);

	debugfs_create_file("regrw", 0644, minor->debugfs_root, minor->dev, &reg_fops);
	}


DEBUGFS_FAIL:
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
    return 0;
#else
	return;
#endif

}

