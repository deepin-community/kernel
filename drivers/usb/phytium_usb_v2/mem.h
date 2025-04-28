/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __LINUX_PHYTIUM_USB_MEM_H__
#define __LINUX_PHYTIUM_USB_MEM_H__

int gadget_mem_init(void *data);
void gadget_mem_cleanup(void *data);
int gadget_setup_addressable_priv_dev(void *data);
void gadget_copy_ep0_dequeue_into_input_ctx(void *data);

#endif
