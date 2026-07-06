// SPDX-License-Identifier: GPL-2.0+
/*
 * usb_tcm_phytium.c -- USB usb_tcm device driver
 *
 * Copyright (C) 2024 Phytium Technology Co., Ltd.*
 */

#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/device.h>

#define DEFAULT_PK_SIZE	512

struct phytium_usb_tcm {
	char name[128];
	struct usb_device *usbdev;
	struct urb *read_urb;
	struct urb *write_urb;

	struct usb_host_endpoint *bulk_out_ep;
	struct usb_host_endpoint *bulk_in_ep;

	signed char *read_data_buf;
	dma_addr_t read_data_dma;

	signed char *write_data_buf;
	dma_addr_t write_data_dma;

	wait_queue_head_t read_wq;
	int read_condition;

	wait_queue_head_t write_wq;
	int write_condition;

	struct device *dev;
};

static struct phytium_usb_tcm *ptcm;

void usb_complete_in(struct urb *urb)
{
	ptcm->read_condition = 1;
	wake_up_interruptible(&ptcm->read_wq);
}

void usb_complete_out(struct urb *urb)
{
	ptcm->write_condition = 1;
	wake_up_interruptible(&ptcm->write_wq);
}

int usb_tcm_write(const char *buffer, int count)
{
	int ret, len, pipe;

	len = count > DEFAULT_PK_SIZE ? DEFAULT_PK_SIZE : count;

	memcpy(ptcm->write_data_buf, buffer, count);

	pipe = usb_sndbulkpipe(ptcm->usbdev,
			ptcm->bulk_out_ep->desc.bEndpointAddress);

	usb_fill_bulk_urb(ptcm->write_urb, ptcm->usbdev, pipe,
			ptcm->write_data_buf, len, usb_complete_out,
			(void *)ptcm);

	ret = usb_submit_urb(ptcm->write_urb, GFP_ATOMIC);
	if (ret) {
		pr_err("%s error:%d\n", __func__, ret);
		return ret;
	}

	ret = wait_event_interruptible(ptcm->write_wq, ptcm->write_condition);
	if (ret) {
		pr_err("receive signal ....\n");
		return ret;
	}

	return len;
}


int usb_tcm_read(char *buffer, int count)
{
	int ret, pipe, len;
	char buf[30];

	memset(buf, 0, sizeof(buf));

	len = count > DEFAULT_PK_SIZE ? DEFAULT_PK_SIZE : count;

	pipe = usb_rcvbulkpipe(ptcm->usbdev,
			ptcm->bulk_in_ep->desc.bEndpointAddress);
	usb_fill_bulk_urb(ptcm->read_urb, ptcm->usbdev, pipe,
			ptcm->read_data_buf, len, usb_complete_in,
			(void *)ptcm);

	ret = usb_submit_urb(ptcm->read_urb, GFP_ATOMIC);
	if (ret) {
		pr_err("%s error:%d\n", __func__, ret);
		return ret;
	}

	ret = wait_event_interruptible(ptcm->read_wq, ptcm->read_condition);
	if (ret) {
		pr_err("%s receive signal ....\n", __func__);
		return ret;
	}

	ptcm->read_condition = 0;

	memcpy(buf, ptcm->read_data_buf, ptcm->read_urb->actual_length);

	return ptcm->read_urb->actual_length;
}

int usb_tcm_status(void)
{
	return 0;
}

static int phytium_usb_tcm_probe(struct usb_interface *intf,
		const struct usb_device_id *id)
{
	struct phytium_usb_tcm *tcm;
	struct usb_device *dev = interface_to_usbdev(intf);
	int ret = -ENOMEM;

	if (intf->cur_altsetting->desc.bNumEndpoints != 2)
		return -ENODEV;

	tcm = kzalloc(sizeof(struct phytium_usb_tcm), GFP_KERNEL);
	if (!tcm)
		return ret;

	//usb_authorize_device(dev);

	tcm->bulk_in_ep = &intf->cur_altsetting->endpoint[0];
	tcm->bulk_out_ep = &intf->cur_altsetting->endpoint[1];

	tcm->read_data_buf = usb_alloc_coherent(dev, DEFAULT_PK_SIZE,
			GFP_KERNEL, &tcm->read_data_dma);
	if (!tcm->read_data_buf)
		goto fail1;

	tcm->write_data_buf = usb_alloc_coherent(dev, DEFAULT_PK_SIZE,
			GFP_KERNEL, &tcm->write_data_dma);
	if (!tcm->write_data_buf)
		goto fail2;

	tcm->read_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!tcm->read_urb)
		goto fail3;

	tcm->write_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!tcm->write_urb)
		goto fail4;

	tcm->usbdev = dev;

	if (dev->manufacturer)
		strscpy(tcm->name, dev->manufacturer, sizeof(tcm->name));

	if (dev->product) {
		if (dev->manufacturer)
			strlcat(tcm->name, " ", sizeof(tcm->name));
		strlcat(tcm->name, dev->product, sizeof(tcm->name));
	}

	if (!strlen(tcm->name))
		snprintf(tcm->name, sizeof(tcm->name),
			"USB tcm %04x:%04x",
			le16_to_cpu(dev->descriptor.idVendor),
			le16_to_cpu(dev->descriptor.idProduct));

	init_waitqueue_head(&tcm->read_wq);
	init_waitqueue_head(&tcm->write_wq);

	usb_set_intfdata(intf, tcm);
	ptcm = tcm;

	return 0;
fail4:
	usb_free_urb(tcm->read_urb);
fail3:
	usb_free_coherent(dev, DEFAULT_PK_SIZE, tcm->write_data_buf,
			tcm->write_data_dma);

fail2:
	usb_free_coherent(dev, DEFAULT_PK_SIZE, tcm->read_data_buf,
			tcm->read_data_dma);
fail1:
	kfree(tcm);

	return ret;
}

static void phytium_usb_tcm_disconnect(struct usb_interface *intf)
{
	struct phytium_usb_tcm *tcm = usb_get_intfdata(intf);

	if (tcm) {
		usb_kill_urb(tcm->read_urb);
		usb_kill_urb(tcm->write_urb);
		usb_free_urb(tcm->read_urb);
		usb_free_urb(tcm->write_urb);
		usb_free_coherent(interface_to_usbdev(intf), DEFAULT_PK_SIZE,
				tcm->read_data_buf, tcm->read_data_dma);
		usb_free_coherent(interface_to_usbdev(intf), DEFAULT_PK_SIZE,
				tcm->write_data_buf, tcm->write_data_dma);
		kfree(tcm);
	}
}

static const struct usb_device_id phytium_usb_tcm_id_table[] = {
	{ USB_DEVICE(0x4875, 0x0100) },
	{ },
};
MODULE_DEVICE_TABLE(usb, phytium_usb_tcm_id_table);

static struct usb_driver phytium_usb_tcm_driver = {
	.name		= "phytium usb tcm",
	.probe		= phytium_usb_tcm_probe,
	.disconnect	= phytium_usb_tcm_disconnect,
	.id_table	= phytium_usb_tcm_id_table,
};

module_usb_driver(phytium_usb_tcm_driver);

MODULE_DESCRIPTION("Phytium usb tcm");
MODULE_AUTHOR("Chen Zhenhua <chenzhenhua@phytium.com.cn>");
MODULE_LICENSE("GPL");
