// SPDX-License-Identifier: GPL-2.0-only
/*
 * Mailbox: Common code for Mailbox controllers and users
 *
 * Copyright (C) 2013-2014 Linaro Ltd.
 * Author: Jassi Brar <jassisinghbrar@gmail.com>
 */

#include <linux/acpi.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <linux/mailbox_client.h>
#include <linux/mailbox_controller.h>
#include <linux/of.h>

#include "../../../mailbox/mailbox.h"
#include "mailbox.h"

static LIST_HEAD(mbox_cons);
static DEFINE_MUTEX(con_mutex);

static void msg_submit(struct mbox_chan *chan)
{
	unsigned int count, idx;
	unsigned long flags;
	void *data;
	int err = -EBUSY;

	spin_lock_irqsave(&chan->lock, flags);

	if (!chan->msg_count || chan->active_req)
		goto exit;

	count = chan->msg_count;
	idx = chan->msg_free;
	if (idx >= count)
		idx -= count;
	else
		idx += MBOX_TX_QUEUE_LEN - count;

	data = chan->msg_data[idx];

	if (chan->cl->tx_prepare)
		chan->cl->tx_prepare(chan->cl, data);
	/* Try to submit a message to the MBOX controller */
	err = chan->mbox->ops->send_data(chan, data);
	if (!err) {
		chan->active_req = data;
		chan->msg_count--;
	}
exit:
	spin_unlock_irqrestore(&chan->lock, flags);

	if (!err && (chan->txdone_method & TXDONE_BY_POLL)) {
		/* kick start the timer immediately to avoid delays */
		spin_lock_irqsave(&chan->mbox->poll_hrt_lock, flags);
		hrtimer_start(&chan->mbox->poll_hrt, 0, HRTIMER_MODE_REL);
		spin_unlock_irqrestore(&chan->mbox->poll_hrt_lock, flags);
	}
}

static void tx_tick(struct mbox_chan *chan, int r)
{
	unsigned long flags;
	void *mssg;

	spin_lock_irqsave(&chan->lock, flags);
	mssg = chan->active_req;
	chan->active_req = NULL;
	spin_unlock_irqrestore(&chan->lock, flags);

	/* Submit next message */
	msg_submit(chan);

	if (!mssg)
		return;

	/* Notify the client */
	if (chan->cl->tx_done)
		chan->cl->tx_done(chan->cl, mssg, r);

	if (r != -ETIME && chan->cl->tx_block)
		complete(&chan->tx_complete);
}

static enum hrtimer_restart txdone_hrtimer(struct hrtimer *hrtimer)
{
	struct mbox_controller *mbox =
		container_of(hrtimer, struct mbox_controller, poll_hrt);
	bool txdone, resched = false;
	int i;
	unsigned long flags;

	for (i = 0; i < mbox->num_chans; i++) {
		struct mbox_chan *chan = &mbox->chans[i];

		if (chan->active_req && chan->cl) {
			txdone = chan->mbox->ops->last_tx_done(chan);
			if (txdone)
				tx_tick(chan, 0);
			else
				resched = true;
		}
	}

	if (resched) {
		spin_lock_irqsave(&mbox->poll_hrt_lock, flags);
		if (!hrtimer_is_queued(hrtimer))
			hrtimer_forward_now(hrtimer, ms_to_ktime(mbox->txpoll_period));
		spin_unlock_irqrestore(&mbox->poll_hrt_lock, flags);

		return HRTIMER_RESTART;
	}
	return HRTIMER_NORESTART;
}

static struct of_phandle_args *to_of_phandle_args(struct fwnode_reference_args *fwnode_args)
{
	struct of_phandle_args of_args;

	of_args.args_count = fwnode_args->nargs;
	of_args.np = to_of_node(fwnode_args->fwnode);
	for (int i = 0; i < fwnode_args->nargs; i++)
		of_args.args[i] = fwnode_args->args[i];

	memcpy(fwnode_args, &of_args, sizeof(struct of_phandle_args));
	return (struct of_phandle_args *)fwnode_args;
}

static struct mbox_chan *mbox_fwnode_parse_chan(struct device *dev, int index)
{
	struct fwnode_reference_args fwnode_args;
	struct mbox_controller *mbox;
	struct mbox_chan *chan;

	if (fwnode_property_get_reference_args(dev->fwnode, "mboxes",
				       "#mbox-cells", 1, index, &fwnode_args)) {
		dev_dbg(dev, "%s: can't parse \"mboxes\" property\n", __func__);
		return ERR_PTR(-ENODEV);
	}

	chan = ERR_PTR(-EPROBE_DEFER);
	list_for_each_entry(mbox, &mbox_cons, node)
		if (mbox->dev->fwnode == fwnode_args.fwnode) {
			if (mbox->dev->of_node && mbox->of_xlate)
				chan = mbox->of_xlate(mbox, to_of_phandle_args(&fwnode_args));
			else
				chan = mbox->fwnode_xlate(mbox, &fwnode_args);
			break;
		}

	fwnode_handle_put(fwnode_args.fwnode);
	return chan;
}

static struct mbox_chan *mbox_parse_chan(struct device *dev, int index)
{
	if (!dev)
		return ERR_PTR(-ENODEV);

	return mbox_fwnode_parse_chan(dev, index);
}

/**
 * cix_mbox_request_channel - Request a mailbox channel.
 * @cl: Identity of the client requesting the channel.
 * @index: Index of mailbox specifier in 'mboxes' property.
 *
 * The Client specifies its requirements and capabilities while asking for
 * a mailbox channel. It can't be called from atomic context.
 * The channel is exclusively allocated and can't be used by another
 * client before the owner calls mbox_free_channel.
 * After assignment, any packet received on this channel will be
 * handed over to the client via the 'rx_callback'.
 * The framework holds reference to the client, so the mbox_client
 * structure shouldn't be modified until the mbox_free_channel returns.
 *
 * Return: Pointer to the channel assigned to the client if successful.
 *		ERR_PTR for request failure.
 */
struct mbox_chan *cix_mbox_request_channel(struct mbox_client *cl, int index)
{
	struct device *dev = cl->dev;
	struct mbox_chan *chan;
	unsigned long flags;
	int ret;

	mutex_lock(&con_mutex);

	chan = mbox_parse_chan(dev, index);

	if (IS_ERR(chan)) {
		mutex_unlock(&con_mutex);
		return chan;
	}

	if (IS_ERR_OR_NULL(chan) || chan->cl ||
	    !try_module_get(chan->mbox->dev->driver->owner)) {
		dev_err(dev, "%s: mailbox not free\n", __func__);
		mutex_unlock(&con_mutex);
		return ERR_PTR(-EBUSY);
	}

	spin_lock_irqsave(&chan->lock, flags);
	chan->msg_free = 0;
	chan->msg_count = 0;
	chan->active_req = NULL;
	chan->cl = cl;
	init_completion(&chan->tx_complete);

	if (chan->txdone_method	== TXDONE_BY_POLL && cl->knows_txdone)
		chan->txdone_method = TXDONE_BY_ACK;

	spin_unlock_irqrestore(&chan->lock, flags);

	if (chan->mbox->ops->startup) {
		ret = chan->mbox->ops->startup(chan);

		if (ret) {
			dev_err(dev, "Unable to startup the chan (%d)\n", ret);
			mbox_free_channel(chan);
			chan = ERR_PTR(ret);
		}
	}

	mutex_unlock(&con_mutex);
	return chan;
}
EXPORT_SYMBOL_GPL(cix_mbox_request_channel);

static struct mbox_chan *
fwnode_mbox_index_xlate(struct mbox_controller *mbox,
		      const struct fwnode_reference_args *fwnode_args)
{
	int ind = fwnode_args->args[0];

	if (ind >= mbox->num_chans)
		return ERR_PTR(-EINVAL);

	return &mbox->chans[ind];
}

/**
 * cix_mbox_controller_register - Register the mailbox controller
 * @mbox:	Pointer to the mailbox controller.
 *
 * The controller driver registers its communication channels
 */
int cix_mbox_controller_register(struct mbox_controller *mbox)
{
	int i, txdone;

	/* Sanity check */
	if (!mbox || !mbox->dev || !mbox->ops || !mbox->num_chans)
		return -EINVAL;

	if (mbox->txdone_irq)
		txdone = TXDONE_BY_IRQ;
	else if (mbox->txdone_poll)
		txdone = TXDONE_BY_POLL;
	else /* It has to be ACK then */
		txdone = TXDONE_BY_ACK;

	if (txdone == TXDONE_BY_POLL) {

		if (!mbox->ops->last_tx_done) {
			dev_err(mbox->dev, "last_tx_done method is absent\n");
			return -EINVAL;
		}

		hrtimer_init(&mbox->poll_hrt, CLOCK_MONOTONIC,
			     HRTIMER_MODE_REL);
		mbox->poll_hrt.function = txdone_hrtimer;
		spin_lock_init(&mbox->poll_hrt_lock);
	}

	for (i = 0; i < mbox->num_chans; i++) {
		struct mbox_chan *chan = &mbox->chans[i];

		chan->cl = NULL;
		chan->mbox = mbox;
		chan->txdone_method = txdone;
		spin_lock_init(&chan->lock);
	}

	if (!mbox->fwnode_xlate)
		mbox->fwnode_xlate = fwnode_mbox_index_xlate;

	mutex_lock(&con_mutex);
	list_add_tail(&mbox->node, &mbox_cons);
	mutex_unlock(&con_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(cix_mbox_controller_register);

static void __devm_mbox_controller_unregister(struct device *dev, void *res)
{
	struct mbox_controller **mbox = res;

	mbox_controller_unregister(*mbox);
}

/**
 * devm_mbox_controller_register() - managed mbox_controller_register()
 * @dev: device owning the mailbox controller being registered
 * @mbox: mailbox controller being registered
 *
 * This function adds a device-managed resource that will make sure that the
 * mailbox controller, which is registered using mbox_controller_register()
 * as part of this function, will be unregistered along with the rest of
 * device-managed resources upon driver probe failure or driver removal.
 *
 * Returns 0 on success or a negative error code on failure.
 */
int cix_devm_mbox_controller_register(struct device *dev,
				  struct mbox_controller *mbox)
{
	struct mbox_controller **ptr;
	int err;

	ptr = devres_alloc(__devm_mbox_controller_unregister, sizeof(*ptr),
			   GFP_KERNEL);
	if (!ptr)
		return -ENOMEM;

	err = cix_mbox_controller_register(mbox);
	if (err < 0) {
		devres_free(ptr);
		return err;
	}

	devres_add(dev, ptr);
	*ptr = mbox;

	return 0;
}
EXPORT_SYMBOL_GPL(cix_devm_mbox_controller_register);

