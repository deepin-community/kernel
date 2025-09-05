/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/mailbox_controller.h>
struct mbox_chan *cix_mbox_request_channel(struct mbox_client *cl, int index);
int cix_mbox_controller_register(struct mbox_controller *mbox);
int cix_devm_mbox_controller_register(struct device *dev,
				  struct mbox_controller *mbox);
