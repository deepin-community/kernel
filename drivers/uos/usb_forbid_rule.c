// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2021 Uniontech Ltd, All Rights Reserved.
 * Author: Huangbibo <huangbibo@uniontech.com>
 */

#include <asm/cache.h>
#include <asm/cpu.h>
#include <asm/cpufeature.h>
#include <asm/dmi.h>

#include <linux/bitops.h>
#include <linux/bug.h>
#include <linux/compat.h>
#include <linux/elf.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/personality.h>
#include <linux/preempt.h>
#include <linux/printk.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/usb.h>

#include <linux/genhd.h>
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <linux/cdrom.h>
#include <scsi/scsi_driver.h>
#include <linux/device.h>
#include <scsi/sg.h>
#include <scsi/scsi_proto.h>

#include "../scsi/sd.h"
#include "../scsi/sr.h"
#include "../usb/core/usb.h"
#include "../usb/storage/usb.h"

#define DMI_USB_RULE_TYPE 251
#define DMI_USB_RULE_MAXNUM 0x100
#define DMI_USB_RULE_MAXLEN 0x80
#define USB_RULE_VERSION_V1 0x01

#define BLACK_RULE 0
#define WHITE_RULE 1
#define RULE_MASK 1
#define READ_RIGHT_MASK 0x2
#define WRITE_RIGHT_MASK 0x4
#define EXCE_RIGHT_MASK 0x8
#define UOS_FORBID 0
#define UOS_ALLOW 1

enum uos_usb_base_class_bitmap {
	UOS_USB_CLASS_NONE = 0,  /* 00h	 Device	Use class information in the Interface Descriptors */
	UOS_USB_CLASS_AUDIO = 1, /* 01h	 Interface	Audio */
	UOS_USB_CLASS_COMMUNICATION = 2, /* 02h	Both	Communications&CDC */
	UOS_USB_CLASS_HID = 3, /* 03h	Interface	HID(Human Interface Device) */
	UOS_USB_CLASS_PHY = 5, /* 05h	Interface	Physical */
	UOS_USB_CLASS_IMAGE = 6, /* 06h 	Interface	Image */
	UOS_USB_CLASS_PRINT = 7, /* 07h 	Interface	Printer */
	UOS_USB_CLASS_MASS = 8, /* 08h	Interface	Mass Storage */
	UOS_USB_CLASS_HUB = 9, /* 09h	Device	Hub */
	UOS_USB_CLASS_CDC = 0xA, /* 0Ah	 Interface	CDC-Data */
	UOS_USB_CLASS_SMART = 0xB, /* 0Bh	Interface	Smart Card */
	UOS_USB_CLASS_SECURITY = 0xD, /* 0Dh	Interface	Content Security */
	UOS_USB_CLASS_VIDEO = 0xE, /* 0Eh	Interface	Video */
	UOS_USB_CLASS_HEALTH = 0xF, /* 0Fh	Interface	Personal Healthcare */
	UOS_USB_CLASS_MEDIA = 0x10, /* 10h	Interface	Audio/Video Devices */
	UOS_USB_CLASS_BILL =  0x11, /* 11h	Device	Billboard Device Class */
	UOS_USB_CLASS_TYPEC = 0x12,  /* 12h	Interface	USB Type-C Bridge Class */
	UOS_USB_CLASS_DIAG = 0xDC, /* DCh	Both	Diagnostic Device */
	UOS_USB_CLASS_WIFI = 0xE0, /* E0h	Interface	Wireless Controller */
	UOS_USB_CLASS_MISC = 0xEF, /* EFh	Both	Miscellaneous */
	UOS_USB_CLASS_APP = 0xFE, /* FEh	Interface	Application Specific */
	UOS_USB_CLASS_VENDOR = 0xFF, /* FFh	Both	Vendor Specific */
};


struct dmi_usb_rule_header {
	u8 type;
	u8 length;
	u16 handle;
	u32 signature;
	u16 version;
};

struct dmi_usb_rule_v1 {
	u8 type;
	u8 length;
	u16 handle;
	u32 signature;
	u16 version;
	u16 priority;
	u32 controller_segment;
	u16 controller_bus;
	u8 controller_device;
	u8 controller_function;
	u8 port_path_str[16];
	u16 usb_vendor;
	u16 usb_product;
	u16 device_class;
	u8 device_subclass;
	u8 device_protocol;
	u32 support_property;
};

struct usb_rule_pci_bus_name {
	u32 controller_segment;
	u16 controller_bus;
	u8 controller_device;
	u8 controller_function;
};

struct usb_rule_match_info {
	char *bus_name;
	char *dev_path;
	struct usb_interface_descriptor *desc;
	u16 usb_vendor;
	u8 usb_product;
};

struct mtp_struct {
	u32 container_length;
	u16 container_type;
	u16 operation_code;
	u32 transaction_id;
	void *payload;
};

bool usb_rule_init_flag = false;
EXPORT_SYMBOL(usb_rule_init_flag);
bool usb_rule_is_real = false;
EXPORT_SYMBOL(usb_rule_is_real); /* for do_mount */

static u32 usb_rule_counter; /* usb forbid rules number */
struct dmi_usb_rule_v1 * dmi_usb_rule_table_v1[DMI_USB_RULE_MAXNUM]; /* usb forbid rules number */

static void get_usb_rule_by_dmi(const struct dmi_header *dm, void *data)
{
	struct dmi_usb_rule_header *usb_rule_header;
	void *usb_rule_entry;

	if(!dm || (dm->type != DMI_USB_RULE_TYPE))
		return;

	usb_rule_header = (struct dmi_usb_rule_header *)dm;
	if(usb_rule_header->length > DMI_USB_RULE_MAXLEN)
		return;

	if(usb_rule_header->version == USB_RULE_VERSION_V1) {
		usb_rule_entry = (struct dmi_usb_rule_v1 *) kmalloc (sizeof(struct dmi_usb_rule_v1), GFP_KERNEL);
		if (usb_rule_entry == NULL) {
			printk(KERN_ERR "get_usb_rule_by_dmi alloc memory fail. \n\r");
			return;
		}
		memcpy(usb_rule_entry, dm,sizeof(struct dmi_usb_rule_v1));
		dmi_usb_rule_table_v1[usb_rule_counter] = usb_rule_entry;
		usb_rule_counter++;
	} else {
		return; /* Reserved for other versions */
	}
	return;
}

void usb_rule_data_init(void)
{
	usb_rule_counter = 0;
	memset(&dmi_usb_rule_table_v1[0], 0x0, sizeof(dmi_usb_rule_table_v1));
}

void usb_rule_data_free(void)
{
	int i;

	for (i=0; i < usb_rule_counter; i++) {
		if(!dmi_usb_rule_table_v1[i])
			kfree(dmi_usb_rule_table_v1[i]);
	}
	return;
}

bool bus_name_to_num(const char *bus_name, struct usb_rule_pci_bus_name *pci_bus_name)
{
	char bus_name_tmp[16] = {0};
	unsigned long tmp;

	/* para check */
	if(!bus_name || !pci_bus_name || (strlen(bus_name) >= 16)) {
		return false;
	}

	memcpy(bus_name_tmp, bus_name, strlen(bus_name));

	/* bus name format: xxxx:xx:xx.x */
	/* clear ':' for strings to number */
	bus_name_tmp[4] = 0;
	bus_name_tmp[7] = 0;
	bus_name_tmp[10] = 0;
	bus_name_tmp[12] = 0;

	if(kstrtoul(&bus_name_tmp[0], 16, &tmp) == 0) {
		pci_bus_name->controller_segment = (u32)tmp;
	} else {
		return false;
	}

	if(kstrtoul(&bus_name_tmp[5], 16, &tmp) == 0) {
		pci_bus_name->controller_bus = (u16)tmp;
	} else {
		return false;
	}

	if(kstrtoul(&bus_name_tmp[8], 16, &tmp) == 0) {
		pci_bus_name->controller_device = (u8)tmp;
	} else {
		return false;
	}

	if(kstrtoul(&bus_name_tmp[11], 16, &tmp) == 0) {
		pci_bus_name->controller_function = (u8)tmp;
	} else {
		return false;
	}

	return true;
}

bool uos_usb_rule_bus_name_match(struct dmi_usb_rule_v1 *usb_rule_entry, struct usb_rule_pci_bus_name *pci_bus_name)
{
	if((pci_bus_name->controller_segment != usb_rule_entry->controller_segment) && (usb_rule_entry->controller_segment != 0xffffffff))
		return false;

	if((pci_bus_name->controller_bus != usb_rule_entry->controller_bus) && (usb_rule_entry->controller_bus != 0xffff))
		return false;

	if((pci_bus_name->controller_device != usb_rule_entry->controller_device) && (usb_rule_entry->controller_device != 0xff))
		return false;

	if((pci_bus_name->controller_function != usb_rule_entry->controller_function) && (usb_rule_entry->controller_function != 0xff))
		return false;

	return true;
}

bool uos_usb_rule_interface_class_match(struct dmi_usb_rule_v1	*usb_rule_entry, struct usb_interface_descriptor *desc)
{
	if((desc->bInterfaceClass != usb_rule_entry->device_class) && (usb_rule_entry->device_class != 0xffff))
		return false;

	if((desc->bInterfaceSubClass != usb_rule_entry->device_subclass) && (usb_rule_entry->device_subclass != 0xff))
		return false;

	if((desc->bInterfaceProtocol != usb_rule_entry->device_protocol) && (usb_rule_entry->device_protocol != 0xff))
		return false;

	return true;
}

bool uos_usb_rule_port_path_match(struct dmi_usb_rule_v1 *usb_rule_entry, char *dev_path)
{
	char port_path_str[16]={0};
	char match_path_str[16]={0};
	int i, j, len = 0;

	memcpy(port_path_str, &usb_rule_entry->port_path_str[0], 16);

	for(i=0; i<16; i++) {
		if (port_path_str[i] == 0) {
			len = i;
			break;
		}

		if(port_path_str[i] == 'x' || port_path_str[i] == 'X') {
			len = i-1;
			break;
		}
	}

	if (len <= 0) {
		return true;
	}

	/* aa.bb.cc.dd etc */
	for(i=0,j=0; i<len; i++) {
		if((port_path_str[i] == '0' && i == 0) ||
			(port_path_str[i] == '0' && port_path_str[i-1] == '.'))
			continue;

		match_path_str[j++] = port_path_str[i];
	}

	match_path_str[j] = 0;
	if(strncmp(match_path_str, dev_path, j) == 0) {
//		printk(KERN_INFO "%s: match_path_str=%s, port_path_str=%s\n\r",__func__, match_path_str, port_path_str);
		return true;
	}

	return false;
}

char uos_storage_right(u32 property)
{
	char  rwx = 0;

	if(property & RULE_MASK) { /* BLACK_RULE 0　WHITE_RULE 1 */
		rwx = (u8) ((property & (READ_RIGHT_MASK | WRITE_RIGHT_MASK | EXCE_RIGHT_MASK)) >> 1) & 7;
	} else {
//		rwx = (u8)(~(property & (READ_RIGHT_MASK | WRITE_RIGHT_MASK | EXCE_RIGHT_MASK)) >> 1) & 7;
		rwx = 0;
	}

	return rwx;
}

/* return val: forbid 0 , allow 1 */
bool uos_usb_rule_match(char *bus_name, char *dev_path, struct usb_interface_descriptor *desc)
{
	struct usb_rule_pci_bus_name pci_bus_name;
	bool ret;
	int i, priority = -1;
	int property = -1;
	char bInterfaceClass;
	struct dmi_usb_rule_v1 *usb_rule_entry;


	/* para check */
	if (!bus_name || !dev_path || !usb_rule_counter) {
		return UOS_ALLOW;
	}

	/* usb hub device won't match usb forbid rule */
	if (desc && desc->bInterfaceClass == UOS_USB_CLASS_HUB)
		return UOS_ALLOW;

	if(!bus_name_to_num(bus_name, &pci_bus_name))
		return UOS_ALLOW;

	for(i=0; i < usb_rule_counter; i++) {
		usb_rule_entry = dmi_usb_rule_table_v1[i];
		if(priority >= usb_rule_entry->priority)  /* ignore low priority rule */
			continue;

		ret = uos_usb_rule_bus_name_match(usb_rule_entry, &pci_bus_name);
		if(!ret)
			continue;

		ret = uos_usb_rule_port_path_match(usb_rule_entry, dev_path);
		if(!ret)
			continue;

		ret = uos_usb_rule_interface_class_match(usb_rule_entry, desc);
		if(!ret)
			continue;

		/*  To be expanded
		ret = uos_usb_rule_vid_match(usb_rule_entry, int vid, int pid);
		if(!ret)
			continue;
		*/

		/* match entry */
		priority = usb_rule_entry->priority;
		property = usb_rule_entry->support_property;
		bInterfaceClass = usb_rule_entry->device_class;
		usb_rule_is_real = true;
	}

	if(priority >= 0) { /* matched */
		printk(KERN_INFO "%s:usb rule matched.  property = %d, priority=%d \n\r", __func__, property, priority);
		if(bInterfaceClass == UOS_USB_CLASS_MASS) {	/* storage rule */
			return !!uos_storage_right(property);
		} else {	/* other rule */
			return property & RULE_MASK; /* BLACK_RULE 0　WHITE_RULE 1 */
		}
	}

	/* mismatch return allow */
	return UOS_ALLOW;
}
EXPORT_SYMBOL(uos_usb_rule_match);

/* return val: mismatch rule rwx_full = 7 , others rwx */
unsigned char uos_usb_rule_storage_match(const char *bus_name, char *dev_path)
{
	struct usb_rule_pci_bus_name pci_bus_name;
	bool ret;
	int i, priority = -1;
	int property = -1;
	struct dmi_usb_rule_v1 *usb_rule_entry;
	char rwx_full = 7;

	/* para check */
	if (!bus_name || !dev_path || !usb_rule_counter) {
		return rwx_full;
	}

	if(!bus_name_to_num(bus_name, &pci_bus_name)) {
		return rwx_full;
	}

	for(i=0; i < usb_rule_counter; i++) {
		usb_rule_entry = dmi_usb_rule_table_v1[i];
		if(priority >= usb_rule_entry->priority)  /* ignore low priority rule */
			continue;

		if (usb_rule_entry->device_class != UOS_USB_CLASS_MASS)
			continue;

		ret = uos_usb_rule_bus_name_match(usb_rule_entry, &pci_bus_name);
		if(!ret)
			continue;

		ret = uos_usb_rule_port_path_match(usb_rule_entry, dev_path);
		if(!ret)
			continue;

		/* match entry */
		priority = usb_rule_entry->priority;
		property = usb_rule_entry->support_property;
	}

	if(priority >= 0) {
		return uos_storage_right(property);
	}

	/* mismatch rule */
	return rwx_full;
}
EXPORT_SYMBOL(uos_usb_rule_storage_match);

extern struct device_type usb_if_device_type;

/* set scsi disk read only */
void set_sd_ro(struct scsi_disk *sdkp)
{
	struct us_data *us;
	int rwx_flag;

	if (!strcmp(sdkp->device->host->hostt->name, "usb-storage"))
		us = host_to_us(sdkp->device->host);
	else
		return;
	rwx_flag = uos_usb_rule_storage_match(us->pusb_dev->bus->bus_name, us->pusb_dev->devpath);
	if (!(rwx_flag & 0x2)) {
		sdkp->write_prot = 1;
		sd_printk(KERN_NOTICE, sdkp, "usb rule match,set sd Write Protect");
	}
}
EXPORT_SYMBOL(set_sd_ro);

/* set scsi cdrom read only */
void set_sr_ro(struct scsi_cd *cd)
{
	struct us_data *us;
	int rwx_flag;

	if (!strcmp(cd->device->host->hostt->name, "usb-storage"))
		us = host_to_us(cd->device->host);
	else
		return;
	rwx_flag = uos_usb_rule_storage_match(us->pusb_dev->bus->bus_name, us->pusb_dev->devpath);
	if (!(rwx_flag & 0x2)) {
		cd->writeable = 0;
		set_disk_ro(cd->disk, 1);
		sr_printk(KERN_INFO, cd, "usb rule match,set sr Write Protect");
	}
}
EXPORT_SYMBOL(set_sr_ro);

/* forbid use scsi write command when cd is read only */
void check_sr_read_access(struct scsi_cd *cd, fmode_t *mode, unsigned cmd, void __user *argp)
{
	struct sg_io_hdr hdr;
	u8 opcode;

	if (!cd->writeable && cmd == SG_IO) {
		if (!copy_from_user(&hdr, argp, sizeof(hdr)) && !copy_from_user(&opcode, hdr.cmdp, sizeof(opcode))) {
			if (opcode == WRITE_10) {
				*mode &= ~FMODE_WRITE;
			}
		}
	}
}
EXPORT_SYMBOL(check_sr_read_access);

/* notice usb mtp or ptp device read only*/
bool notice_mtp_ro(struct usb_device *dev, struct usb_host_interface *alt)
{
	char *alt_string;
	bool ret = false;

	alt_string = usb_cache_string(dev, alt->desc.iInterface);
	if (alt_string && (strstr(alt_string, "MTP") || strstr(alt_string, "PTP"))) {
		if (!(uos_usb_rule_storage_match(dev->bus->bus_name, dev->devpath) & 0x2)) {
			dev_info(&dev->dev, "usb rule match,set MTP and PTP interface read-only");
			ret = true;
		}
	}
	return ret;
}
EXPORT_SYMBOL(notice_mtp_ro);

/* forbid mtp and ptp device write command */
bool check_mtp_read_access(struct usb_device *dev, int ifnum, void __user *buffer)
{
	struct usb_interface *intf;
	int rwx_flag;
	bool ret = true;
	struct mtp_struct mtp;

	intf = usb_ifnum_to_if(dev, ifnum);
	if (intf->cur_altsetting->string && (strstr(intf->cur_altsetting->string, "MTP") ||
				strstr(intf->cur_altsetting->string, "PTP"))) {
		rwx_flag = uos_usb_rule_storage_match(dev->bus->bus_name, dev->devpath);
		if (!(rwx_flag & 0x2) && !copy_from_user(&mtp, buffer, sizeof(mtp))) {
			if (mtp.operation_code == 0x100c || mtp.operation_code == 0x100d)
				ret = false;
		}
	}
	return ret;
}
EXPORT_SYMBOL(check_mtp_read_access);

static inline struct scsi_cd *get_scsi_cd(struct gendisk *disk)
{
	return container_of(disk->private_data, struct scsi_cd, driver);
}

/* forbid change disk ro using ioctl */
int get_usb_disk_ro(struct block_device *bdev)
{
	int rwx_flag = 0x7;
	struct scsi_disk *sdkp = NULL;
	struct scsi_cd *cd = NULL;
	struct scsi_device *sdev = NULL;
	struct gendisk *disk;
	struct us_data *us;
	int partno;

	if (!IS_ERR(bdev)){
		partno = (int)bdev->bd_partno;
		disk = get_gendisk(bdev->bd_dev, &partno);
		if (disk) {
			if (strstr(disk->disk_name, "sd")){
				sdkp = scsi_disk(disk);
				sdev = sdkp->device;
			}
			if (strstr(disk->disk_name, "sr")){
				cd = get_scsi_cd(disk);
				sdev = cd->device;
			}
		}
	}
	if (sdev && !strcmp(sdev->host->hostt->name, "usb-storage")) {
		us = host_to_us(sdev->host);
		rwx_flag = uos_usb_rule_storage_match(us->pusb_dev->bus->bus_name, us->pusb_dev->devpath);
	}
	return rwx_flag;
}
EXPORT_SYMBOL(get_usb_disk_ro);



/* only for test */
void uos_create_test_rule(void)
{
	struct dmi_usb_rule_v1 *usb_rule_entry;

	usb_rule_counter = 4;

/* rule 1, disable 0000:02:00.0 usb 01 all functions priority=0 low*/
	usb_rule_entry = (struct dmi_usb_rule_v1 *) kmalloc (sizeof(struct dmi_usb_rule_v1), GFP_KERNEL);
	if (usb_rule_entry == NULL) {
		printk(KERN_ERR "uos_create_rule alloc memory fail. \n\r");
		return;
	}

	usb_rule_entry->type = DMI_USB_RULE_TYPE;
	usb_rule_entry->length =0x30;
	usb_rule_entry->handle =0;
	usb_rule_entry->signature =0x52534455;
	usb_rule_entry->version =1;
	usb_rule_entry->priority =0; /****/
	usb_rule_entry->controller_segment = 0x0000;/****/
	usb_rule_entry->controller_bus=0x02;/****/
	usb_rule_entry->controller_device=0x00;/****/
	usb_rule_entry->controller_function=0x0;/****/
	//usb_rule_entry->port_path_str[16];/****/
	usb_rule_entry->port_path_str[0] = '0';/** left 01**/
	usb_rule_entry->port_path_str[1] = '1';/** left 01**/
	usb_rule_entry->port_path_str[2] = 0; /** left 01**/
	usb_rule_entry->usb_vendor=0xffff;
	usb_rule_entry->usb_product=0xffff;
	usb_rule_entry->device_class=0xffff;
	usb_rule_entry->device_subclass=0xff;
	usb_rule_entry->device_protocol=0xff;
	usb_rule_entry->support_property=0x0;  /* BLACK_RULE 0　WHITE_RULE 1 */
	dmi_usb_rule_table_v1[0] = usb_rule_entry;

/* rule 2, allow 0000:02:00.0 usb 01 hid functions priority=1 high */
	usb_rule_entry = (struct dmi_usb_rule_v1 *) kmalloc (sizeof(struct dmi_usb_rule_v1), GFP_KERNEL);
	if (usb_rule_entry == NULL) {
		printk(KERN_ERR "uos_create_rule alloc memory fail. \n\r");
		return;
	}

	usb_rule_entry->type = DMI_USB_RULE_TYPE;
	usb_rule_entry->length =0x30;
	usb_rule_entry->handle =0;
	usb_rule_entry->signature =0x52534455;
	usb_rule_entry->version =1;
	usb_rule_entry->priority =1; /****/
	usb_rule_entry->controller_segment = 0x0000;/****/
	usb_rule_entry->controller_bus=0x02;/****/
	usb_rule_entry->controller_device=0x00;/****/
	usb_rule_entry->controller_function=0x0;/****/
	//usb_rule_entry->port_path_str[16];/****/
	usb_rule_entry->port_path_str[0] = '0';/** left 01**/
	usb_rule_entry->port_path_str[1] = '1';/** left 01**/
	usb_rule_entry->port_path_str[2] = 0; /** left 01**/
	usb_rule_entry->usb_vendor=0xffff;
	usb_rule_entry->usb_product=0xffff;
	usb_rule_entry->device_class=0x3;
	usb_rule_entry->device_subclass=0xff;
	usb_rule_entry->device_protocol=0xff;
	usb_rule_entry->support_property=0x1;
	dmi_usb_rule_table_v1[1] = usb_rule_entry;


/* rule 3, allow 0000:04:00.0 usb 04 all functions priority=0 low */
	usb_rule_entry = (struct dmi_usb_rule_v1 *) kmalloc (sizeof(struct dmi_usb_rule_v1), GFP_KERNEL);
	if (usb_rule_entry == NULL) {
		printk(KERN_ERR "uos_create_rule alloc memory fail. \n\r");
		return;
	}

	usb_rule_entry->type = DMI_USB_RULE_TYPE;
	usb_rule_entry->length =0x30;
	usb_rule_entry->handle =1;
	usb_rule_entry->signature =0x52534455;
	usb_rule_entry->version =1;
	usb_rule_entry->priority =0; /****/
	usb_rule_entry->controller_segment = 0x0000;/****/
	usb_rule_entry->controller_bus=0x04;/****/
	usb_rule_entry->controller_device=0x0;/****/
	usb_rule_entry->controller_function=0x0;/****/
	usb_rule_entry->port_path_str[0] = '0';/** right 04 **/
	usb_rule_entry->port_path_str[1] = '4';/** right 04 **/
	usb_rule_entry->port_path_str[2] = 0; /** right 04 **/
	usb_rule_entry->usb_vendor=0xffff;
	usb_rule_entry->usb_product=0xffff;
	usb_rule_entry->device_class=0xffff;
	usb_rule_entry->device_subclass=0xff;
	usb_rule_entry->device_protocol=0xff;
	usb_rule_entry->support_property=0x1;
	dmi_usb_rule_table_v1[2] = usb_rule_entry;


/* rule 4, disable 0000:04:00.0 usb 04 storage write&exec priority=1 high */
	usb_rule_entry = (struct dmi_usb_rule_v1 *) kmalloc (sizeof(struct dmi_usb_rule_v1), GFP_KERNEL);
	if (usb_rule_entry == NULL) {
		printk(KERN_ERR "uos_create_rule alloc memory fail. \n\r");
		return;
	}
	usb_rule_entry->type = DMI_USB_RULE_TYPE;
	usb_rule_entry->length =0x30;
	usb_rule_entry->handle =1;
	usb_rule_entry->signature =0x52534455;
	usb_rule_entry->version =1;
	usb_rule_entry->priority =1; /****/
	usb_rule_entry->controller_segment = 0x0000;/****/
	usb_rule_entry->controller_bus=0x04;/****/
	usb_rule_entry->controller_device=0x0;/****/
	usb_rule_entry->controller_function=0x0;/****/
	usb_rule_entry->port_path_str[0] = '0';/** right 04 **/
	usb_rule_entry->port_path_str[1] = '4';/** right 04 **/
	usb_rule_entry->port_path_str[2] = 0; /** right 04 **/
	usb_rule_entry->usb_vendor=0xffff;
	usb_rule_entry->usb_product=0xffff;
	usb_rule_entry->device_class=0x8;
	usb_rule_entry->device_subclass=0xff;
	usb_rule_entry->device_protocol=0xff;
	usb_rule_entry->support_property=0xc;
	dmi_usb_rule_table_v1[3] = usb_rule_entry;

	return;
}


int usb_rule_init(void)
{
	printk(KERN_INFO "%s start ... \r\n", __func__);

	usb_rule_data_init();

	if(!usb_rule_init_flag) {
		dmi_walk(get_usb_rule_by_dmi, NULL);
	}

	if(!usb_rule_counter) {
		printk(KERN_INFO "%s cannot find rules by dmi \r\n", __func__);
	}

#if 0 /* only for test */
	uos_create_test_rule();
#endif

	usb_rule_init_flag = true;
	return 0;
}

device_initcall(usb_rule_init);
