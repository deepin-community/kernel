// SPDX-License-Identifier: GPL-2.0-only
/*
 *  ahci_zhaoxin_sgpio.c - Driver for Zhaoxin sgpio
 */

#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/nospec.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_cmnd.h>
#include <linux/libata.h>
#include <linux/pci.h>

#include "ahci.h"
#include "libata.h"
#include "ahci_zhaoxin_sgpio.h"

static LIST_HEAD(sgpio_zhaoxin_list);

static unsigned int zhaoxin_em_type __read_mostly = AHCI_EM_MSG_LED_MODE; /*LED protocol*/
module_param(zhaoxin_em_type, int, 0644);
MODULE_PARM_DESC(zhaoxin_em_type,
	"AHCI Enclosure Management Message type control (1 = led on, 2 = sgpio on,3 = sgpio gp on)");

int ahci_wait_em_reset(struct sgpio_zhaoxin *sgpio_zhaoxin, u32 retry)
{
	void __iomem *mmio = sgpio_zhaoxin->mmio;
	u32 em_ctl;

	if (!sgpio_zhaoxin || retry == 0) {
		pr_err("In ahci wait em reset, invalid param\n");
		return -EINVAL;
	}

	while (retry--) { /*EM_CTL needs reset at least 64ms*/
		em_ctl = readl(mmio + HOST_EM_CTL);
		if (em_ctl & EM_CTL_RST)
			msleep(10); /*EM_CTL still in reset, usleep 10ms*/
		else
			break;

		if (!retry)
			pr_err("Wait for EM_CTL reset, time out\n");
	}

	return 0;
}

void ahci_zhaoxin_set_em_sgpio(struct sgpio_zhaoxin *sgpio_zhaoxin)
{
	void __iomem *mmio = sgpio_zhaoxin->mmio;
	void __iomem *em_mmio = mmio + SGPIO_OFFSET;

	volatile u32 read;

	sgpio_zhaoxin->sgpio_reg.cfg_0.enable = 1;

	sgpio_zhaoxin->sgpio_reg.cfg_1.blink_gen_a = 0x7;
	sgpio_zhaoxin->sgpio_reg.cfg_1.blink_gen_b = 0x3;
	sgpio_zhaoxin->sgpio_reg.cfg_1.blink_gen_c = 0x0;
	sgpio_zhaoxin->sgpio_reg.cfg_1.stretch_act_on = 0;
	sgpio_zhaoxin->sgpio_reg.cfg_1.stretch_act_off = 0;
	sgpio_zhaoxin->sgpio_reg.cfg_1.max_act_on = 2;
	sgpio_zhaoxin->sgpio_reg.cfg_1.force_act_off = 1;

	sgpio_zhaoxin->sgpio_reg.gp_transmit_cfg.sload = 0xf;
	sgpio_zhaoxin->sgpio_reg.gp_transmit_cfg.count = 0x0;

	sgpio_zhaoxin->sgpio_reg.transmit_0.sgpio_tx_0 = 0;
	sgpio_zhaoxin->sgpio_reg.transmit_1.sgpio_tx_1 = 0;
	sgpio_zhaoxin->sgpio_reg.gp_transmit_reg.sgpio_tx_gp = 0;

	sgpio_zhaoxin->sgpio_reg.receive_reg.sgpio_rx = 0x07070707;
	sgpio_zhaoxin->sgpio_reg.gp_receive_reg.sgpio_rx_gp = 0;

	/*Setup SGPIO type*/
	read = readl(mmio + sgpio_zhaoxin->em_loc);
	read = read | SGPIO_MESSAGE_HEAD;  /*LED register MSG_HEAD, select SGPIO*/
	writel(read, mmio + sgpio_zhaoxin->em_loc);

	/*Setup gp mode*/
	writel(sgpio_zhaoxin->sgpio_reg.gp_transmit_cfg.sgpio_tx_gp_cfg, em_mmio + 0x38);

	/*Initial SGPIO CFG1*/
	writel(sgpio_zhaoxin->sgpio_reg.cfg_1.sgpio_cfg_1, em_mmio + 0x4);

	/*Initial SGPIO CFG0*/
	read = readl(em_mmio);
	read |= sgpio_zhaoxin->sgpio_reg.cfg_0.sgpio_cfg_0;
	writel(read, em_mmio);
}

void ahci_zhaoxin_set_em_sgpio_gpmode(struct sgpio_zhaoxin *sgpio_zhaoxin)
{
	void __iomem *mmio = sgpio_zhaoxin->mmio;
	void __iomem *em_mmio = mmio + SGPIO_OFFSET;
	u32 read;

	sgpio_zhaoxin->sgpio_reg.cfg_0.enable = 1;

	sgpio_zhaoxin->sgpio_reg.gp_transmit_cfg.sload = 0xf;
	sgpio_zhaoxin->sgpio_reg.gp_transmit_cfg.count = 0xff;

	sgpio_zhaoxin->sgpio_reg.transmit_0.sgpio_tx_0 = 0;
	sgpio_zhaoxin->sgpio_reg.transmit_1.sgpio_tx_1 = 0;
	sgpio_zhaoxin->sgpio_reg.gp_transmit_reg.sgpio_tx_gp = 0;

	sgpio_zhaoxin->sgpio_reg.receive_reg.sgpio_rx = 0;
	sgpio_zhaoxin->sgpio_reg.gp_receive_reg.sgpio_rx_gp = 0xff0f0000;

	/*Setup SGPIO type*/
	read = readl(mmio + sgpio_zhaoxin->em_loc);
	read |= SGPIO_MESSAGE_HEAD;
	writel(read, mmio + sgpio_zhaoxin->em_loc);

	/*Setup gp mode*/
	writel(sgpio_zhaoxin->sgpio_reg.gp_transmit_cfg.sgpio_tx_gp_cfg, em_mmio + 0x38);

	/*Enable SGPIO*/
	writel(sgpio_zhaoxin->sgpio_reg.cfg_0.sgpio_cfg_0, em_mmio);
}

static ssize_t ahci_em_type_sys_show(struct sgpio_zhaoxin *sgpio_zhaoxin, char *buf)
{
	return sprintf(buf, "0x%x\n", zhaoxin_em_type);
}
static ssize_t ahci_em_type_sys_store(struct sgpio_zhaoxin *sgpio_zhaoxin, const char *buf,
				      size_t count)
{
	int code = 0;
	int rc = 0;

	if (kstrtouint(buf, 0, &code))
		return count;

	if (code == AHCI_EM_MSG_LED_MODE) {
		zhaoxin_em_type = code;
	} else if (code == AHCI_EM_MSG_SGPIO_MODE) {
		rc = ahci_wait_em_reset(sgpio_zhaoxin, 7); /*wait at least 64ms*/
		if (rc < 0) {
			pr_err("ahci wait em reset failed!\n");
			return rc;
		}
		zhaoxin_em_type = code;
		ahci_zhaoxin_set_em_sgpio(sgpio_zhaoxin);
	} else if (code == AHCI_EM_MSG_SGPIO_GP_MODE) {
		rc = ahci_wait_em_reset(sgpio_zhaoxin, 7); /*wait at least 64ms*/
		if (rc < 0) {
			pr_err("ahci wait em reset failed!\n");
			return rc;
		}
		zhaoxin_em_type = code;
		ahci_zhaoxin_set_em_sgpio_gpmode(sgpio_zhaoxin);
	} else
		pr_err("Incorrect value:1 = LED on, 2 = SGPIO normal on, 3 = SGPIO GP on)\n");

	return count;
}

static ssize_t ahci_transmit_sgpio_message(unsigned long port_num,
					   struct sgpio_zhaoxin *sgpio_zhaoxin, u16 state,
					   ssize_t size)
{
	void __iomem *mmio = sgpio_zhaoxin->mmio;
	void __iomem *em_mmio = mmio + SGPIO_OFFSET;
	unsigned long flags;

	if (!(sgpio_zhaoxin->em_msg_type & EM_MSG_TYPE_SGPIO))
		return -EINVAL;

	spin_lock_irqsave(&sgpio_zhaoxin->wr_lock, flags);

	switch (port_num) {
	case 0:
		writel(SGPIO_MESSAGE_HEAD, mmio + sgpio_zhaoxin->em_loc);
		writew(state, em_mmio + 0x22);
		sgpio_zhaoxin->sgpio_reg.transmit_0.sgpio_tx_0 &= 0x0000ffff;
		sgpio_zhaoxin->sgpio_reg.transmit_0.drive_0_active = (state & 0x3c0) >> 6;
		sgpio_zhaoxin->sgpio_reg.transmit_0.drive_0_locate = (state & 0x38) >> 3;
		sgpio_zhaoxin->sgpio_reg.transmit_0.drive_0_error = state & 0x7;
		break;
	case 1:
		writel(SGPIO_MESSAGE_HEAD, mmio + sgpio_zhaoxin->em_loc);
		writew(state, em_mmio + 0x20);
		sgpio_zhaoxin->sgpio_reg.transmit_0.sgpio_tx_0 &= 0xffff0000;
		sgpio_zhaoxin->sgpio_reg.transmit_0.drive_1_active = (state & 0x3c0) >> 6;
		sgpio_zhaoxin->sgpio_reg.transmit_0.drive_1_locate = (state & 0x38) >> 3;
		sgpio_zhaoxin->sgpio_reg.transmit_0.drive_1_error = state & 0x7;
		break;
	case 2:
		writel(SGPIO_MESSAGE_HEAD, mmio + sgpio_zhaoxin->em_loc);
		writew(state, em_mmio + 0x26);
		sgpio_zhaoxin->sgpio_reg.transmit_1.sgpio_tx_1 &= 0x0000ffff;
		sgpio_zhaoxin->sgpio_reg.transmit_1.drive_2_active = (state & 0x3c0) >> 6;
		sgpio_zhaoxin->sgpio_reg.transmit_1.drive_2_locate = (state & 0x38) >> 3;
		sgpio_zhaoxin->sgpio_reg.transmit_1.drive_2_error = state & 0x7;
		break;
	case 3:
		writel(SGPIO_MESSAGE_HEAD, mmio + sgpio_zhaoxin->em_loc);
		writew(state, em_mmio + 0x24);
		sgpio_zhaoxin->sgpio_reg.transmit_1.sgpio_tx_1 &= 0xffff0000;
		sgpio_zhaoxin->sgpio_reg.transmit_1.drive_3_active = (state & 0x3c0) >> 6;
		sgpio_zhaoxin->sgpio_reg.transmit_1.drive_3_locate = (state & 0x38) >> 3;
		sgpio_zhaoxin->sgpio_reg.transmit_1.drive_3_error = state & 0x7;
		break;
	default:
		pr_err("Unsupported port number in this controller\n");
		break;
	}

	spin_unlock_irqrestore(&sgpio_zhaoxin->wr_lock, flags);

	return size;
}

static ssize_t ahci_transmit_sgpio_indicator(unsigned long port_num,
					     struct sgpio_zhaoxin *sgpio_zhaoxin,
					     u8 indicator_code, enum SGPIO_INDICATOR type,
					     ssize_t size)
{
	void __iomem *mmio = sgpio_zhaoxin->mmio;
	void __iomem *em_mmio = mmio + SGPIO_OFFSET;
	u16 state;

	if (!(sgpio_zhaoxin->em_msg_type & EM_MSG_TYPE_SGPIO))
		return -EINVAL;

	if (get_ahci_em_messages() && (zhaoxin_em_type != AHCI_EM_MSG_SGPIO_MODE)) {
		pr_err("Current setting not SGPIO normal mode, quit\n");
		return -EINVAL;
	}

	switch (port_num) {
	case 0:
		state = readw(em_mmio + 0x22);
		break;
	case 1:
		state = readw(em_mmio + 0x20);
		break;
	case 2:
		state = readw(em_mmio + 0x26);
		break;
	case 3:
		state = readw(em_mmio + 0x24);
		break;
	default:
		return -EINVAL;
	}

	if (type == SGPIO_ACTIVITY) {
		state &= 0xfc3f;
		state |= (indicator_code&0xf) << 6;
	} else if (type == SGPIO_LOCATE) {
		state &= 0xffc7;
		state |= (indicator_code&0x7) << 3;
	} else if (type == SGPIO_ERROR) {
		state &= 0xfff8;
		state |= indicator_code & 0x7;
	} else {
		return -EINVAL;
	}

	return ahci_transmit_sgpio_message(port_num, sgpio_zhaoxin, state, size);
}

static ssize_t ahci_transmit_sgpio_indicator_gp(unsigned long port_num,
						struct sgpio_zhaoxin *sgpio_zhaoxin,
						u8 indicator_code, enum SGPIO_INDICATOR type,
						ssize_t size)
{
	void __iomem *mmio = sgpio_zhaoxin->mmio;
	void __iomem *em_mmio = mmio + SGPIO_OFFSET;
	union SGPIO_TX_GP state;
	unsigned long flags;

	if (!(sgpio_zhaoxin->em_msg_type & EM_MSG_TYPE_SGPIO))
		return -EINVAL;

	if (get_ahci_em_messages() && (zhaoxin_em_type != AHCI_EM_MSG_SGPIO_GP_MODE)) {
		pr_err("Current setting not SGPIO_GP mode, quit\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&sgpio_zhaoxin->wr_lock, flags);

	state.sgpio_tx_gp = readl(em_mmio + 0x3c);
	switch (port_num) {
	case 0:
		if (type == SGPIO_ACTIVITY)
			state.D00 = indicator_code & 0x1;
		else if (type == SGPIO_LOCATE)
			state.D01 = indicator_code & 0x1;
		else if (type == SGPIO_ERROR)
			state.D02 = indicator_code & 0x1;
		break;
	case 1:
		if (type == SGPIO_ACTIVITY)
			state.D10 = indicator_code & 0x1;
		else if (type == SGPIO_LOCATE)
			state.D11 = indicator_code & 0x1;
		else if (type == SGPIO_ERROR)
			state.D12 = indicator_code & 0x1;
		break;
	case 2:
		if (type == SGPIO_ACTIVITY)
			state.D20 = indicator_code & 0x1;
		else if (type == SGPIO_LOCATE)
			state.D21 = indicator_code & 0x1;
		else if (type == SGPIO_ERROR)
			state.D22 = indicator_code & 0x1;
		break;
	case 3:
		if (type == SGPIO_ACTIVITY)
			state.D30 = indicator_code & 0x1;
		else if (type == SGPIO_LOCATE)
			state.D31 = indicator_code & 0x1;
		else if (type == SGPIO_ERROR)
			state.D32 = indicator_code & 0x1;
		break;
	default:
		return -EINVAL;
	}

	writel(SGPIO_MESSAGE_HEAD, mmio + sgpio_zhaoxin->em_loc);
	writel(state.sgpio_tx_gp, em_mmio + 0x3c);
	sgpio_zhaoxin->sgpio_reg.gp_transmit_reg.sgpio_tx_gp = state.sgpio_tx_gp;

	spin_unlock_irqrestore(&sgpio_zhaoxin->wr_lock, flags);
	return size;
}

static ssize_t sgpio_activity_store(struct sgpio_zhaoxin *sgpio_zhaoxin, const char *buf,
				    size_t count)
{
	unsigned long val = 0;
	unsigned long port_num = 0;
	unsigned long code = 0;

	if (kstrtoul(buf, 0, &val))
		return count;

	port_num = val & 0xf;
	code = val >> 4;

	if (sgpio_zhaoxin->em_msg_type & EM_MSG_TYPE_SGPIO) {
		switch (code) {
		case 0x0:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      ACTIVITY_DISABLE, SGPIO_ACTIVITY, 1);
			break;
		case 0x1:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      ACTIVITY_ENABLE, SGPIO_ACTIVITY, 1);
			break;
		case 0x2:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      ACTIVITY_GA_FON, SGPIO_ACTIVITY, 1);
			break;
		case 0x3:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      ACTIVITY_GA_FOFF, SGPIO_ACTIVITY, 1);
			break;
		case 0x4:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      ACTIVITY_BRIEF_EN_EOF, SGPIO_ACTIVITY, 1);
			break;
		case 0x5:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      ACTIVITY_BRIEF_EN_SOF, SGPIO_ACTIVITY, 1);
			break;
		case 0x6:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      ACTIVITY_GB_FON, SGPIO_ACTIVITY, 1);
			break;
		case 0x7:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      ACTIVITY_GB_FOFF, SGPIO_ACTIVITY, 1);
			break;
		case 0x8:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      ACTIVITY_GC_FON, SGPIO_ACTIVITY, 1);
			break;
		case 0x9:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      ACTIVITY_GC_FOFF, SGPIO_ACTIVITY, 1);
			break;
		case 0x10:
			ahci_transmit_sgpio_indicator_gp(port_num, sgpio_zhaoxin,
							 GP_OFF, SGPIO_ACTIVITY, 1);
			break;
		case 0x11:
			ahci_transmit_sgpio_indicator_gp(port_num, sgpio_zhaoxin,
							 GP_ON, SGPIO_ACTIVITY, 1);
			break;
		default:
			pr_err("Unsupported command for activity indicator, cmd:0x%lx\n", val);
			break;
		}

		return count;
	}

	return -EINVAL;
}

static ssize_t sgpio_locate_store(struct sgpio_zhaoxin *sgpio_zhaoxin, const char *buf,
				  size_t count)
{
	unsigned long val = 0;
	unsigned long port_num = 0;
	unsigned long code = 0;

	if (kstrtoul(buf, 0, &val))
		return count;

	port_num = val & 0xf;
	code = val >> 4;

	if (sgpio_zhaoxin->em_msg_type & EM_MSG_TYPE_SGPIO)  {
		switch (code) {
		case 0x0:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_DISABLE, SGPIO_LOCATE, 1);
			break;
		case 0x1:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_ENABLE, SGPIO_LOCATE, 1);
			break;
		case 0x2:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_GA_FON, SGPIO_LOCATE, 1);
			break;
		case 0x3:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_GA_FOFF, SGPIO_LOCATE, 1);
			break;
		case 0x4:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_GB_FON, SGPIO_LOCATE, 1);
			break;
		case 0x5:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_GB_FOFF, SGPIO_LOCATE, 1);
			break;
		case 0x6:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_GC_FON, SGPIO_LOCATE, 1);
			break;
		case 0x7:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_GC_FOFF, SGPIO_LOCATE, 1);
			break;
		case 0x10:
			ahci_transmit_sgpio_indicator_gp(port_num, sgpio_zhaoxin,
							 GP_OFF, SGPIO_LOCATE, 1);
			break;
		case 0x11:
			ahci_transmit_sgpio_indicator_gp(port_num, sgpio_zhaoxin, GP_ON,
							 SGPIO_LOCATE, 1);
			break;
		default:
			pr_err("Unsupported command for locate indicator, cmd:0x%lx\n", val);
			break;
		}

		return count;
	}
	return -EINVAL;
}

static ssize_t sgpio_error_store(struct sgpio_zhaoxin *sgpio_zhaoxin, const char *buf, size_t count)
{
	unsigned long val = 0;
	unsigned long port_num = 0;
	unsigned long code = 0;

	if (kstrtoul(buf, 0, &val))
		return count;

	port_num = val & 0xf;
	code = val >> 4;

	if (sgpio_zhaoxin->em_msg_type & EM_MSG_TYPE_SGPIO) {
		switch (code) {
		case 0x0:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_DISABLE, SGPIO_ERROR, 1);
			break;
		case 0x1:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_ENABLE, SGPIO_ERROR, 1);
			break;
		case 0x2:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_GA_FON, SGPIO_ERROR, 1);
			break;
		case 0x3:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_GA_FOFF, SGPIO_ERROR, 1);
			break;
		case 0x4:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_GB_FON, SGPIO_ERROR, 1);
			break;
		case 0x5:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_GB_FOFF, SGPIO_ERROR, 1);
			break;
		case 0x6:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_GC_FON, SGPIO_ERROR, 1);
			break;
		case 0x7:
			ahci_transmit_sgpio_indicator(port_num, sgpio_zhaoxin,
						      LOCATE_ERROR_GC_FOFF, SGPIO_ERROR, 1);
			break;
		case 0x10:
			ahci_transmit_sgpio_indicator_gp(port_num, sgpio_zhaoxin,
							 GP_OFF, SGPIO_ERROR, 1);
			break;
		case 0x11:
			ahci_transmit_sgpio_indicator_gp(port_num, sgpio_zhaoxin,
							 GP_ON, SGPIO_ERROR, 1);
			break;
		default:
			pr_err("Unsupport command for error indicator, cmd:0x%lx\n", val);
			break;
		}

		return count;
	}

	return -EINVAL;
}

static struct sgpio_zhaoxin_sysfs_attr dev_attr_ahci_em_type_sys =
		__ATTR(ahci_em_type_sys, 0644, ahci_em_type_sys_show,
		       ahci_em_type_sys_store);
static struct sgpio_zhaoxin_sysfs_attr dev_attr_sgpio_activity =
		__ATTR(sgpio_activity, 0200, NULL, sgpio_activity_store);
static struct sgpio_zhaoxin_sysfs_attr dev_attr_sgpio_locate =
		__ATTR(sgpio_locate, 0200, NULL, sgpio_locate_store);
static struct sgpio_zhaoxin_sysfs_attr dev_attr_sgpio_error =
		__ATTR(sgpio_error, 0200, NULL, sgpio_error_store);

struct attribute *sgpio_attrs[] = {
	&dev_attr_ahci_em_type_sys.attr,
	&dev_attr_sgpio_activity.attr,
	&dev_attr_sgpio_locate.attr,
	&dev_attr_sgpio_error.attr,
	NULL
};

static const struct attribute_group sgpio_attrs_group = {
	.attrs = sgpio_attrs
};
const struct attribute_group *sgpio_groups[] = {
	&sgpio_attrs_group,
	NULL
};

static ssize_t sgpio_zhaoxin_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct sgpio_zhaoxin_sysfs_attr *sgpio_zhaoxin_sysfs_attr = to_sgpio_attr(attr);
	struct sgpio_zhaoxin *sgpio_zhaoxin = to_sgpio_obj(kobj);

	if (!sgpio_zhaoxin_sysfs_attr->show)
		return -EIO;

	return sgpio_zhaoxin_sysfs_attr->show(sgpio_zhaoxin, buf);
}

static ssize_t  sgpio_zhaoxin_attr_store(struct kobject *kobj, struct attribute *attr,
					 const char *buf, size_t len)
{
	struct sgpio_zhaoxin_sysfs_attr *sgpio_zhaoxin_sysfs_attr = to_sgpio_attr(attr);
	struct sgpio_zhaoxin *sgpio_zhaoxin = to_sgpio_obj(kobj);

	if (!sgpio_zhaoxin_sysfs_attr->store)
		return -EIO;

	return sgpio_zhaoxin_sysfs_attr->store(sgpio_zhaoxin, buf, len);
}

const struct sysfs_ops sgpio_zhaoxin_sysfs_ops = {
	.show	= sgpio_zhaoxin_attr_show,
	.store	= sgpio_zhaoxin_attr_store,
};

const struct kobj_type sgpio_zhaoxin_ktype = {
	.sysfs_ops	= &sgpio_zhaoxin_sysfs_ops,
	.default_groups = sgpio_groups,
};

void set_em_messages(struct sgpio_zhaoxin *sgpio_zhaoxin)
{
	void __iomem *mmio = sgpio_zhaoxin->mmio;
	u32 em_loc = readl(mmio + HOST_EM_LOC);
	u32 em_ctl = readl(mmio + HOST_EM_CTL);
	u8 messages;

	if (!get_ahci_em_messages())
		return;

	messages = (em_ctl & EM_CTRL_MSG_TYPE) >> 16;

	if (messages) {
		/* store em_loc */
		sgpio_zhaoxin->em_loc = ((em_loc >> 16) * 4);
		sgpio_zhaoxin->em_buf_sz = ((em_loc & 0xff) * 4);
		sgpio_zhaoxin->em_msg_type = messages;
	}
}

int add_sgpio_zhaoxin(void)
{
	struct pci_dev *pdev_cur = pci_get_device(PCI_VENDOR_ID_ZHAOXIN, 0x9083, NULL);
	struct pci_dev *pdev_next = pdev_cur;
	struct sgpio_zhaoxin *sgpio_zhaoxin;
	int ret = 0;

	if (!get_ahci_em_messages())
		return 0;

	while (pdev_next) {
		pdev_next = pci_get_device(PCI_VENDOR_ID_ZHAOXIN, 0x9083, pdev_cur);

		WARN_ON(MAX_TEST_RESULT_LEN <= 0);

		sgpio_zhaoxin = (struct sgpio_zhaoxin *)get_zeroed_page(GFP_KERNEL);
		if (!sgpio_zhaoxin)
			return -ENOMEM;

		list_add(&sgpio_zhaoxin->list, &sgpio_zhaoxin_list);
		ret = kobject_init_and_add(&sgpio_zhaoxin->kobj, &sgpio_zhaoxin_ktype,
					   &(&pdev_cur->dev)->kobj, "zx_sgpio");
		if (ret) {
			kobject_put(&sgpio_zhaoxin->kobj);
			return -1;
		}

		kobject_uevent(&sgpio_zhaoxin->kobj, KOBJ_ADD);
		spin_lock_init(&sgpio_zhaoxin->wr_lock);
		sgpio_zhaoxin->kobj_valid = 1;
		sgpio_zhaoxin->mmio = pcim_iomap_table(pdev_cur)[5];
		set_em_messages(sgpio_zhaoxin);
		ret = ahci_wait_em_reset(sgpio_zhaoxin, 7); /*wait at least 64ms*/
		if (ret < 0) {
			pr_err("ahci wait em reset failed!\n");
			return ret;
		}

		sgpio_zhaoxin->kobj_valid = 1;

		if (zhaoxin_em_type == AHCI_EM_MSG_SGPIO_GP_MODE)
			ahci_zhaoxin_set_em_sgpio_gpmode(sgpio_zhaoxin);
		else if (zhaoxin_em_type == AHCI_EM_MSG_SGPIO_MODE)
			ahci_zhaoxin_set_em_sgpio(sgpio_zhaoxin);

		pdev_cur = pdev_next;
	}

	return 0;
}


void remove_sgpio_zhaoxin(void)
{
	struct sgpio_zhaoxin *cur = NULL, *next = NULL;

	if (!get_ahci_em_messages())
		return;

	list_for_each_entry_safe(cur, next, &sgpio_zhaoxin_list, list) {
		list_del(&cur->list);
		if (cur->kobj_valid)
			kobject_put(&cur->kobj);

		free_page((unsigned long)cur);
		if (!next)
			break;
	}
}

static int __init zhaoxin_sgpio_init(void)
{
	return add_sgpio_zhaoxin();
}

static void __exit zhaoxin_sgpio_exit(void)
{
	remove_sgpio_zhaoxin();
}

late_initcall(zhaoxin_sgpio_init);
module_exit(zhaoxin_sgpio_exit);

MODULE_DESCRIPTION("Zhaoxin SGPIO driver");
MODULE_AUTHOR("XanderChen");
MODULE_LICENSE("GPL");
