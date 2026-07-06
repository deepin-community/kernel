// SPDX-License-Identifier: GPL-2.0+
/*
 * f_usb_tcm.c -- USB usb_tcm function driver
 *
 * Copyright (C) 2024 Phytium Technology Co., Ltd.*
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/idr.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>

#include "u_f.h"
#include "u_usb_tcm.h"
#include "usb_tcm_pk.h"

#define USB_TCM_G_MINORS	4
#define DEFAULT_PK_SIZE		512

static int major, minors;
static struct class *usb_tcm_g_class;
static DEFINE_IDA(usb_tcm_g_ida);
static DEFINE_MUTEX(usb_tcm_g_ida_lock); /* protects access to usb_tcm_g_ida */

/*-------------------------------------------------------------------------*/
/*                            usb_tcm gadget struct                            */
struct f_usb_tcm_g_req_list {
	struct usb_request	*req;
	unsigned int		pos;
	struct list_head	list;
};

struct f_usb_tcm_g {
	/* configuration */
	unsigned char			idle;

	/* recv report */
	spinlock_t			read_spinlock;
	wait_queue_head_t		read_queue;
	/* recv report - interrupt out only (use_out_ep == 1) */
	struct list_head		completed_out_req;
	unsigned int			qlen;

	spinlock_t			write_spinlock;
	bool				write_pending;
	wait_queue_head_t		write_queue;
	struct usb_request		*req;

	int				minor;
	struct cdev			cdev;
	struct usb_function		func;

	struct usb_ep			*in_ep;
	struct usb_ep			*out_ep;
};

static inline struct f_usb_tcm_g *func_to_usb_tcm_g(struct usb_function *f)
{
	return container_of(f, struct f_usb_tcm_g, func);
}

/*-------------------------------------------------------------------------*/
/*                           Static descriptors                            */

static struct usb_interface_descriptor usb_tcm_g_interface_desc = {
	.bLength		= sizeof(usb_tcm_g_interface_desc),
	.bDescriptorType	= USB_DT_INTERFACE,
	/* .bInterfaceNumber	= DYNAMIC */
	.bAlternateSetting	= 0,
	.bNumEndpoints	= 2,
	.bInterfaceClass	= USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass	= 0,
	.bInterfaceProtocol	= 0,
	/* .iInterface		= DYNAMIC */
};


/* Super-Speed Support */

static struct usb_endpoint_descriptor usb_tcm_g_ss_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize	= cpu_to_le16(128),
	.bInterval		= 4, /* FIXME: Add this field in the
				      * usb_tcm gadget configuration?
				      * (struct usb_tcm_g_func_descriptor)
				      */
};

static struct usb_ss_ep_comp_descriptor usb_tcm_g_ss_in_comp_desc = {
	.bLength                = sizeof(usb_tcm_g_ss_in_comp_desc),
	.bDescriptorType        = USB_DT_SS_ENDPOINT_COMP,

	/* .bMaxBurst           = 0, */
	/* .bmAttributes        = 0, */
	/* .wBytesPerInterval   = DYNAMIC */
};

static struct usb_endpoint_descriptor usb_tcm_g_ss_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize	= cpu_to_le16(128),
	.bInterval		= 4, /* FIXME: Add this field in the
				      * usb_tcm gadget configuration?
				      * (struct usb_tcm_g_func_descriptor)
				      */
};

static struct usb_ss_ep_comp_descriptor usb_tcm_g_ss_out_comp_desc = {
	.bLength                = sizeof(usb_tcm_g_ss_out_comp_desc),
	.bDescriptorType        = USB_DT_SS_ENDPOINT_COMP,

	/* .bMaxBurst           = 0, */
	/* .bmAttributes        = 0, */
	/* .wBytesPerInterval   = DYNAMIC */
};

static struct usb_descriptor_header *usb_tcm_g_ss_descriptors_intout[] = {
	(struct usb_descriptor_header *)&usb_tcm_g_interface_desc,
	(struct usb_descriptor_header *)&usb_tcm_g_ss_in_ep_desc,
	(struct usb_descriptor_header *)&usb_tcm_g_ss_in_comp_desc,
	(struct usb_descriptor_header *)&usb_tcm_g_ss_out_ep_desc,
	(struct usb_descriptor_header *)&usb_tcm_g_ss_out_comp_desc,
	NULL,
};


/* High-Speed Support */

static struct usb_endpoint_descriptor usb_tcm_g_hs_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize	= cpu_to_le16(512),
	.bInterval		= 4, /* FIXME: Add this field in the
				      * usb_tcm gadget configuration?
				      * (struct usb_tcm_g_func_descriptor)
				      */
};

static struct usb_endpoint_descriptor usb_tcm_g_hs_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize	= cpu_to_le16(512),
	.bInterval		= 4, /* FIXME: Add this field in the
				      * usb_tcm gadget configuration?
				      * (struct usb_tcm_g_func_descriptor)
				      */
};

static struct usb_descriptor_header *usb_tcm_g_hs_descriptors_intout[] = {
	(struct usb_descriptor_header *)&usb_tcm_g_interface_desc,
	(struct usb_descriptor_header *)&usb_tcm_g_hs_in_ep_desc,
	(struct usb_descriptor_header *)&usb_tcm_g_hs_out_ep_desc,
	NULL,
};

/* Full-Speed Support */

static struct usb_endpoint_descriptor usb_tcm_g_fs_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize	= cpu_to_le16(32),
	.bInterval		= 10, /* FIXME: Add this field in the
				       * usb_tcm gadget configuration?
				       * (struct usb_tcm_g_func_descriptor)
				       */
};

static struct usb_endpoint_descriptor usb_tcm_g_fs_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize	= cpu_to_le16(32),
	.bInterval		= 10, /* FIXME: Add this field in the
				       * usb_tcm gadget configuration?
				       * (struct usb_tcm_g_func_descriptor)
				       */
};

static struct usb_descriptor_header *usb_tcm_g_fs_descriptors_intout[] = {
	(struct usb_descriptor_header *)&usb_tcm_g_interface_desc,
	(struct usb_descriptor_header *)&usb_tcm_g_fs_in_ep_desc,
	(struct usb_descriptor_header *)&usb_tcm_g_fs_out_ep_desc,
	NULL,
};

/*-------------------------------------------------------------------------*/
/*                                 Strings                                 */

#define CT_FUNC_usb_tcm_IDX	0

static struct usb_string ct_func_string_defs[] = {
	[CT_FUNC_usb_tcm_IDX].s	= "usb_tcm Interface",
	{},			/* end of list */
};

static struct usb_gadget_strings ct_func_string_table = {
	.language	= 0x0409,	/* en-US */
	.strings	= ct_func_string_defs,
};

static struct usb_gadget_strings *ct_func_strings[] = {
	&ct_func_string_table,
	NULL,
};

/*-------------------------------------------------------------------------*/
/*                              Char Device                                */

static ssize_t f_usb_tcm_g_read(struct file *file, char __user *buffer,
				  size_t count, loff_t *ptr)
{
	struct f_usb_tcm_g *usb_tcm_g = file->private_data;
	struct f_usb_tcm_g_req_list *list;
	struct usb_request *req;
	unsigned long flags;
	int ret;

	if (!count)
		return 0;

	spin_lock_irqsave(&usb_tcm_g->read_spinlock, flags);

#define READ_COND_INTOUT (!list_empty(&usb_tcm_g->completed_out_req))

	/* wait for at least one buffer to complete */
	while (!READ_COND_INTOUT) {
		spin_unlock_irqrestore(&usb_tcm_g->read_spinlock, flags);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible(usb_tcm_g->read_queue, READ_COND_INTOUT))
			return -ERESTARTSYS;

		spin_lock_irqsave(&usb_tcm_g->read_spinlock, flags);
	}

	/* pick the first one */
	list = list_first_entry(&usb_tcm_g->completed_out_req,
				struct f_usb_tcm_g_req_list, list);

	/*
	 * Remove this from list to protect it from beign free()
	 * while host disables our function
	 */
	list_del(&list->list);

	req = list->req;
	count = min_t(unsigned int, count, req->actual - list->pos);
	spin_unlock_irqrestore(&usb_tcm_g->read_spinlock, flags);

	/* copy to user outside spinlock */
	count -= copy_to_user(buffer, req->buf + list->pos, count);
	list->pos += count;

	/*
	 * if this request is completely handled and transferred to
	 * userspace, remove its entry from the list and requeue it
	 * again. Otherwise, we will revisit it again upon the next
	 * call, taking into account its current read position.
	 */
	if (list->pos == req->actual) {
		kfree(list);

		req->length = DEFAULT_PK_SIZE;
		ret = usb_ep_queue(usb_tcm_g->out_ep, req, GFP_KERNEL);
		if (ret < 0) {
			free_ep_req(usb_tcm_g->out_ep, req);
			return ret;
		}
	} else {
		spin_lock_irqsave(&usb_tcm_g->read_spinlock, flags);
		list_add(&list->list, &usb_tcm_g->completed_out_req);
		spin_unlock_irqrestore(&usb_tcm_g->read_spinlock, flags);

		wake_up(&usb_tcm_g->read_queue);
	}

	return count;
}

static void f_usb_tcm_g_req_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_usb_tcm_g *usb_tcm_g = (struct f_usb_tcm_g *)ep->driver_data;
	unsigned long flags;

	if (req->status != 0) {
		ERROR(usb_tcm_g->func.config->cdev,
			"End Point Request ERROR: %d\n", req->status);
	}

	spin_lock_irqsave(&usb_tcm_g->write_spinlock, flags);
	usb_tcm_g->write_pending = 0;
	spin_unlock_irqrestore(&usb_tcm_g->write_spinlock, flags);
	wake_up(&usb_tcm_g->write_queue);
}

static ssize_t f_usb_tcm_g_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *offp)
{
	struct f_usb_tcm_g *usb_tcm_g  = file->private_data;
	struct usb_request *req;
	unsigned long flags;
	ssize_t status = -ENOMEM;

	spin_lock_irqsave(&usb_tcm_g->write_spinlock, flags);

	if (!usb_tcm_g->req) {
		spin_unlock_irqrestore(&usb_tcm_g->write_spinlock, flags);
		return -ESHUTDOWN;
	}

#define WRITE_COND (!usb_tcm_g->write_pending)
try_again:
	/* write queue */
	while (!WRITE_COND) {
		spin_unlock_irqrestore(&usb_tcm_g->write_spinlock, flags);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible_exclusive(
				usb_tcm_g->write_queue, WRITE_COND))
			return -ERESTARTSYS;

		spin_lock_irqsave(&usb_tcm_g->write_spinlock, flags);
	}

	usb_tcm_g->write_pending = 1;
	req = usb_tcm_g->req;
	count  = min_t(unsigned int, count, DEFAULT_PK_SIZE);

	spin_unlock_irqrestore(&usb_tcm_g->write_spinlock, flags);

	if (!req) {
		ERROR(usb_tcm_g->func.config->cdev, "usb_tcm_g->req is NULL\n");
		status = -ESHUTDOWN;
		goto release_write_pending;
	}

	status = copy_from_user(req->buf, buffer, count);
	if (status != 0) {
		ERROR(usb_tcm_g->func.config->cdev,
			"copy_from_user error\n");
		status = -EINVAL;
		goto release_write_pending;
	}

	spin_lock_irqsave(&usb_tcm_g->write_spinlock, flags);

	/* when our function has been disabled by host */
	if (!usb_tcm_g->req) {
		free_ep_req(usb_tcm_g->in_ep, req);
		/*
		 * TODO
		 * Should we fail with error here?
		 */
		goto try_again;
	}

	req->status   = 0;
	req->zero     = 0;
	req->length   = count;
	req->complete = f_usb_tcm_g_req_complete;
	req->context  = usb_tcm_g;

	spin_unlock_irqrestore(&usb_tcm_g->write_spinlock, flags);

	if (!usb_tcm_g->in_ep->enabled) {
		ERROR(usb_tcm_g->func.config->cdev, "in_ep is disabled\n");
		status = -ESHUTDOWN;
		goto release_write_pending;
	}

	status = usb_ep_queue(usb_tcm_g->in_ep, req, GFP_ATOMIC);
	if (status < 0)
		goto release_write_pending;
	else
		status = count;

	return status;
release_write_pending:
	spin_lock_irqsave(&usb_tcm_g->write_spinlock, flags);
	usb_tcm_g->write_pending = 0;
	spin_unlock_irqrestore(&usb_tcm_g->write_spinlock, flags);

	wake_up(&usb_tcm_g->write_queue);

	return status;
}

static __poll_t f_usb_tcm_g_poll(struct file *file, poll_table *wait)
{
	struct f_usb_tcm_g	*usb_tcm_g  = file->private_data;
	__poll_t	ret = 0;

	poll_wait(file, &usb_tcm_g->read_queue, wait);
	poll_wait(file, &usb_tcm_g->write_queue, wait);

	if (WRITE_COND)
		ret |= EPOLLOUT | EPOLLWRNORM;

	if (READ_COND_INTOUT)
		ret |= EPOLLIN | EPOLLRDNORM;

	return ret;
}

#undef WRITE_COND
#undef READ_COND_INTOUT

static int f_usb_tcm_g_release(struct inode *inode, struct file *fd)
{
	fd->private_data = NULL;
	return 0;
}

static int f_usb_tcm_g_open(struct inode *inode, struct file *fd)
{
	struct f_usb_tcm_g *usb_tcm_g =
		container_of(inode->i_cdev, struct f_usb_tcm_g, cdev);

	fd->private_data = usb_tcm_g;

	return 0;
}

/*-------------------------------------------------------------------------*/
/*                                usb_function                             */
static void usb_tcm_g_intout_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_usb_tcm_g *usb_tcm_g = (struct f_usb_tcm_g *) req->context;
	struct usb_composite_dev *cdev = usb_tcm_g->func.config->cdev;
	struct f_usb_tcm_g_req_list *req_list;
	unsigned long flags;

	switch (req->status) {
	case 0:
		req_list = kzalloc(sizeof(*req_list), GFP_ATOMIC);
		if (!req_list) {
			ERROR(cdev, "Unable to allocate mem for req_list\n");
			goto free_req;
		}

		req_list->req = req;

		spin_lock_irqsave(&usb_tcm_g->read_spinlock, flags);
		list_add_tail(&req_list->list, &usb_tcm_g->completed_out_req);
		spin_unlock_irqrestore(&usb_tcm_g->read_spinlock, flags);
		wake_up(&usb_tcm_g->read_queue);
		break;
	case -ECONNABORTED:		/* hardware forced ep reset */
	case -ECONNRESET:		/* request dequeued */
	case -ESHUTDOWN:		/* disconnect from host */
	default:
		ERROR(cdev, "Set report failed %d\n", req->status);
free_req:
		free_ep_req(ep, req);
		return;
	}
}

static void usb_tcm_g_disable(struct usb_function *f)
{
	struct f_usb_tcm_g *usb_tcm_g = func_to_usb_tcm_g(f);
	struct f_usb_tcm_g_req_list *list, *next;
	unsigned long flags;

	usb_ep_disable(usb_tcm_g->in_ep);

	usb_ep_disable(usb_tcm_g->out_ep);

	spin_lock_irqsave(&usb_tcm_g->read_spinlock, flags);
	list_for_each_entry_safe(list, next, &usb_tcm_g->completed_out_req, list) {
		free_ep_req(usb_tcm_g->out_ep, list->req);
		list_del(&list->list);
		kfree(list);
	}
	spin_unlock_irqrestore(&usb_tcm_g->read_spinlock, flags);

	spin_lock_irqsave(&usb_tcm_g->write_spinlock, flags);
	if (!usb_tcm_g->write_pending) {
		free_ep_req(usb_tcm_g->in_ep, usb_tcm_g->req);
		usb_tcm_g->write_pending = 1;
	}
	usb_tcm_g->req = NULL;
	spin_unlock_irqrestore(&usb_tcm_g->write_spinlock, flags);
}

static int usb_tcm_g_set_alt(struct usb_function *f, unsigned int intf,
		unsigned int alt)
{
	struct usb_composite_dev		*cdev = f->config->cdev;
	struct f_usb_tcm_g				*usb_tcm_g = func_to_usb_tcm_g(f);
	unsigned long				flags;
	int i, status = 0;
	struct usb_request		*req = NULL, *req_in = NULL;

	VDBG(cdev, "%s intf:%d alt:%d\n", __func__, intf, alt);

	if (usb_tcm_g->in_ep != NULL) {
		/* restart endpoint */
		usb_ep_disable(usb_tcm_g->in_ep);

		status = config_ep_by_speed(f->config->cdev->gadget, f,
					    usb_tcm_g->in_ep);
		if (status) {
			ERROR(cdev, "config_ep_by_speed FAILED!\n");
			goto fail;
		}
		status = usb_ep_enable(usb_tcm_g->in_ep);
		if (status < 0) {
			ERROR(cdev, "Enable IN endpoint FAILED!\n");
			goto fail;
		}
		usb_tcm_g->in_ep->driver_data = usb_tcm_g;

		req_in = alloc_ep_req(usb_tcm_g->in_ep, 512);
		if (!req_in) {
			status = -ENOMEM;
			goto disable_ep_in;
		}
	}

	if (usb_tcm_g->out_ep != NULL) {
		/* restart endpoint */
		usb_ep_disable(usb_tcm_g->out_ep);

		status = config_ep_by_speed(f->config->cdev->gadget, f,
					    usb_tcm_g->out_ep);
		if (status) {
			ERROR(cdev, "config_ep_by_speed FAILED!\n");
			goto free_req_in;
		}
		status = usb_ep_enable(usb_tcm_g->out_ep);
		if (status < 0) {
			ERROR(cdev, "Enable OUT endpoint FAILED!\n");
			goto free_req_in;
		}
		usb_tcm_g->out_ep->driver_data = usb_tcm_g;

		/*
		 * allocate a bunch of read buffers and queue them all at once.
		 */
		for (i = 0; i < usb_tcm_g->qlen && status == 0; i++) {
			req = alloc_ep_req(usb_tcm_g->out_ep, DEFAULT_PK_SIZE);
			if (req) {
				req->complete = usb_tcm_g_intout_complete;
				req->context  = usb_tcm_g;
				status = usb_ep_queue(usb_tcm_g->out_ep, req,
								GFP_ATOMIC);
				if (status) {
					ERROR(cdev, "%s queue req --> %d\n",
						usb_tcm_g->out_ep->name, status);
					free_ep_req(usb_tcm_g->out_ep, req);
				}
			} else {
				status = -ENOMEM;
				goto disable_out_ep;
			}
		}
	}

	if (usb_tcm_g->in_ep != NULL) {
		spin_lock_irqsave(&usb_tcm_g->write_spinlock, flags);
		usb_tcm_g->req = req_in;
		usb_tcm_g->write_pending = 0;
		spin_unlock_irqrestore(&usb_tcm_g->write_spinlock, flags);

	}

	return 0;
disable_out_ep:
	usb_ep_disable(usb_tcm_g->out_ep);
free_req_in:
	if (req_in)
		free_ep_req(usb_tcm_g->in_ep, req_in);
disable_ep_in:
	if (usb_tcm_g->in_ep)
		usb_ep_disable(usb_tcm_g->in_ep);
fail:
	return status;
}

static const struct file_operations f_usb_tcm_g_fops = {
	.owner		= THIS_MODULE,
	.open		= f_usb_tcm_g_open,
	.release	= f_usb_tcm_g_release,
	.write		= f_usb_tcm_g_write,
	.read		= f_usb_tcm_g_read,
	.poll		= f_usb_tcm_g_poll,
	.llseek		= noop_llseek,
};

static int usb_tcm_g_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_ep		*ep;
	struct f_usb_tcm_g		*usb_tcm_g = func_to_usb_tcm_g(f);
	struct usb_string	*us;
	struct device		*device;
	int			status;
	dev_t			dev;

	/* maybe allocate device-global string IDs, and patch descriptors */
	us = usb_gstrings_attach(c->cdev, ct_func_strings,
				 ARRAY_SIZE(ct_func_string_defs));
	if (IS_ERR(us))
		return PTR_ERR(us);
	usb_tcm_g_interface_desc.iInterface = us[CT_FUNC_usb_tcm_IDX].id;

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	usb_tcm_g_interface_desc.bInterfaceNumber = status;

	/* allocate instance-specific endpoints */
	status = -ENODEV;
	ep = usb_ep_autoconfig(c->cdev->gadget, &usb_tcm_g_fs_in_ep_desc);
	if (!ep)
		goto fail;
	usb_tcm_g->in_ep = ep;

	ep = usb_ep_autoconfig(c->cdev->gadget, &usb_tcm_g_fs_out_ep_desc);
	if (!ep)
		goto fail;
	usb_tcm_g->out_ep = ep;

	/* set descriptor dynamic values */
	usb_tcm_g->idle = 1;

	usb_tcm_g_hs_in_ep_desc.bEndpointAddress = usb_tcm_g_fs_in_ep_desc.bEndpointAddress;
	usb_tcm_g_hs_out_ep_desc.bEndpointAddress = usb_tcm_g_fs_out_ep_desc.bEndpointAddress;

	usb_tcm_g_ss_in_ep_desc.bEndpointAddress = usb_tcm_g_fs_in_ep_desc.bEndpointAddress;
	usb_tcm_g_ss_out_ep_desc.bEndpointAddress = usb_tcm_g_fs_out_ep_desc.bEndpointAddress;

	status = usb_assign_descriptors(f,
			usb_tcm_g_fs_descriptors_intout,
			usb_tcm_g_hs_descriptors_intout,
			usb_tcm_g_ss_descriptors_intout,
			usb_tcm_g_ss_descriptors_intout);

	if (status)
		goto fail;

	spin_lock_init(&usb_tcm_g->write_spinlock);
	usb_tcm_g->write_pending = 1;
	usb_tcm_g->req = NULL;
	spin_lock_init(&usb_tcm_g->read_spinlock);
	init_waitqueue_head(&usb_tcm_g->write_queue);
	init_waitqueue_head(&usb_tcm_g->read_queue);
	INIT_LIST_HEAD(&usb_tcm_g->completed_out_req);

	/* create char device */
	cdev_init(&usb_tcm_g->cdev, &f_usb_tcm_g_fops);
	dev = MKDEV(major, usb_tcm_g->minor);
	status = cdev_add(&usb_tcm_g->cdev, dev, 1);
	if (status)
		goto fail_free_descs;

	device = device_create(usb_tcm_g_class, NULL, dev, NULL,
			       "%s%d", "usb_tcm_g", usb_tcm_g->minor);

	if (IS_ERR(device)) {
		status = PTR_ERR(device);
		goto del;
	}

	return 0;
del:
	cdev_del(&usb_tcm_g->cdev);
fail_free_descs:
	usb_free_all_descriptors(f);
fail:
	ERROR(f->config->cdev, "%s FAILED\n", __func__);
	if (usb_tcm_g->req != NULL)
		free_ep_req(usb_tcm_g->in_ep, usb_tcm_g->req);

	return status;
}

static inline int usb_tcm_g_get_minor(void)
{
	int ret;

	ret = ida_simple_get(&usb_tcm_g_ida, 0, 0, GFP_KERNEL);
	if (ret >= USB_TCM_G_MINORS) {
		ida_simple_remove(&usb_tcm_g_ida, ret);
		ret = -ENODEV;
	}

	return ret;
}

static inline struct f_usb_tcm_opts *to_f_usb_tcm_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_usb_tcm_opts,
			    func_inst.group);
}

static void usb_tcm_attr_release(struct config_item *item)
{
	struct f_usb_tcm_opts *opts = to_f_usb_tcm_opts(item);

	usb_put_function_instance(&opts->func_inst);
}

static struct configfs_item_operations usb_tcm_g_item_ops = {
	.release	= usb_tcm_attr_release,
};

static ssize_t f_usb_tcm_opts_dev_show(struct config_item *item, char *page)
{
	return sprintf(page, "%d:%s\n", __LINE__, __func__);
}

CONFIGFS_ATTR_RO(f_usb_tcm_opts_, dev);

static struct configfs_attribute *usb_tcm_attrs[] = {
	&f_usb_tcm_opts_attr_dev,
	NULL,
};

static const struct config_item_type usb_tcm_func_type = {
	.ct_item_ops	= &usb_tcm_g_item_ops,
	.ct_attrs	= usb_tcm_attrs,
	.ct_owner	= THIS_MODULE,
};

static inline void usb_tcm_g_put_minor(int minor)
{
	ida_simple_remove(&usb_tcm_g_ida, minor);
}

static void usb_tcm_g_free_inst(struct usb_function_instance *f)
{
	struct f_usb_tcm_opts *opts;

	opts = container_of(f, struct f_usb_tcm_opts, func_inst);

	mutex_lock(&usb_tcm_g_ida_lock);

	usb_tcm_g_put_minor(opts->minor);
	if (ida_is_empty(&usb_tcm_g_ida))
		g_usb_tcm_cleanup();

	mutex_unlock(&usb_tcm_g_ida_lock);

	kfree(opts);
}

static struct usb_function_instance *usb_tcm_g_alloc_inst(void)
{
	struct f_usb_tcm_opts *opts;
	struct usb_function_instance *ret;
	int status = 0;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);
	mutex_init(&opts->lock);
	opts->func_inst.free_func_inst = usb_tcm_g_free_inst;
	ret = &opts->func_inst;

	mutex_lock(&usb_tcm_g_ida_lock);

	if (ida_is_empty(&usb_tcm_g_ida)) {
		status = g_usb_tcm_setup(NULL, USB_TCM_G_MINORS);
		if (status)  {
			ret = ERR_PTR(status);
			kfree(opts);
			goto unlock;
		}
	}

	opts->minor = usb_tcm_g_get_minor();

	if (opts->minor < 0) {
		ret = ERR_PTR(opts->minor);
		kfree(opts);
		if (ida_is_empty(&usb_tcm_g_ida))
			g_usb_tcm_cleanup();
		goto unlock;
	}
	config_group_init_type_name(&opts->func_inst.group, "", &usb_tcm_func_type);

unlock:
	mutex_unlock(&usb_tcm_g_ida_lock);
	return ret;
}

static void usb_tcm_g_free(struct usb_function *f)
{
	struct f_usb_tcm_g *usb_tcm_g;
	struct f_usb_tcm_opts *opts;

	usb_tcm_g = func_to_usb_tcm_g(f);
	opts = container_of(f->fi, struct f_usb_tcm_opts, func_inst);

	kfree(usb_tcm_g);
	mutex_lock(&opts->lock);
	--opts->refcnt;
	mutex_unlock(&opts->lock);
}

static void usb_tcm_g_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_usb_tcm_g *usb_tcm_g = func_to_usb_tcm_g(f);

	device_destroy(usb_tcm_g_class, MKDEV(major, usb_tcm_g->minor));
	cdev_del(&usb_tcm_g->cdev);

	usb_free_all_descriptors(f);
}

static struct usb_function *usb_tcm_g_alloc(struct usb_function_instance *fi)
{
	struct f_usb_tcm_g *usb_tcm_g;
	struct f_usb_tcm_opts *opts;

	/* allocate and initialize one new instance */
	usb_tcm_g = kzalloc(sizeof(*usb_tcm_g), GFP_KERNEL);
	if (!usb_tcm_g)
		return ERR_PTR(-ENOMEM);

	opts = container_of(fi, struct f_usb_tcm_opts, func_inst);

	mutex_lock(&opts->lock);
	++opts->refcnt;

	usb_tcm_g->minor = opts->minor;

	mutex_unlock(&opts->lock);

	usb_tcm_g->func.name    = "usb_tcm";
	usb_tcm_g->func.bind    = usb_tcm_g_bind;
	usb_tcm_g->func.unbind  = usb_tcm_g_unbind;
	usb_tcm_g->func.set_alt = usb_tcm_g_set_alt;
	usb_tcm_g->func.disable = usb_tcm_g_disable;
	usb_tcm_g->func.free_func = usb_tcm_g_free;

	/* this could be made configurable at some point */
	usb_tcm_g->qlen	   = 4;

	return &usb_tcm_g->func;
}

DECLARE_USB_FUNCTION_INIT(usb_tcm, usb_tcm_g_alloc_inst, usb_tcm_g_alloc);

int g_usb_tcm_setup(struct usb_gadget *g, int count)
{
	int status;
	dev_t dev;
	// wangzc usb_tcm_g_class = class_create(THIS_MODULE, "usb_tcm_g");
	usb_tcm_g_class = class_create("usb_tcm_g");
	if (IS_ERR(usb_tcm_g_class)) {
		status = PTR_ERR(usb_tcm_g_class);
		usb_tcm_g_class = NULL;
		return status;
	}


	status = alloc_chrdev_region(&dev, 0, count, "usb_tcm_g");
	if (status) {
		class_destroy(usb_tcm_g_class);
		usb_tcm_g_class = NULL;
		return status;
	}

	major = MAJOR(dev);
	minors = count;

	return 0;
}

void g_usb_tcm_cleanup(void)
{
	if (major) {
		unregister_chrdev_region(MKDEV(major, 0), minors);
		major = minors = 0;
	}

	class_destroy(usb_tcm_g_class);
	usb_tcm_g_class = NULL;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen Zhenhua <chenzhenhua@phytium.com.cn>");
