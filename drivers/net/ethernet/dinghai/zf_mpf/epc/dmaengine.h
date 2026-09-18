#ifndef ZXDH_DMAENGINE_H
#define ZXDH_DMAENGINE_H

#include <linux/bug.h>
#include <linux/dmaengine.h>


/**
 * zxdh_dma_cookie_init - initialize the cookies for a DMA channel
 * @chan: dma channel to initialize
 */
static inline void zxdh_dma_cookie_init(struct dma_chan *chan)
{
	chan->cookie = DMA_MIN_COOKIE;
	chan->completed_cookie = DMA_MIN_COOKIE;
}

/**
 * zxdh_dma_cookie_assign - assign a DMA engine cookie to the descriptor
 * @tx: descriptor needing cookie
 *
 * Assign a unique non-zero per-channel cookie to the descriptor.
 * Note: caller is expected to hold a lock to prevent concurrency.
 */
static inline dma_cookie_t zxdh_dma_cookie_assign(struct dma_async_tx_descriptor *tx)
{
	struct dma_chan *chan = tx->chan;
	dma_cookie_t cookie;

	cookie = chan->cookie + 1;
	if (cookie < DMA_MIN_COOKIE)
		cookie = DMA_MIN_COOKIE;
	tx->cookie = chan->cookie = cookie;

	return cookie;
}

/**
 * zxdh_dma_cookie_complete - complete a descriptor
 * @tx: descriptor to complete
 *
 * Mark this descriptor complete by updating the channels completed
 * cookie marker.  Zero the descriptors cookie to prevent accidental
 * repeated completions.
 *
 * Note: caller is expected to hold a lock to prevent concurrency.
 */
static inline void zxdh_dma_cookie_complete(struct dma_async_tx_descriptor *tx)
{
	BUG_ON(tx->cookie < DMA_MIN_COOKIE);
	tx->chan->completed_cookie = tx->cookie;
	tx->cookie = 0;
}

/**
 * zxdh_dma_cookie_status - report cookie status
 * @chan: dma channel
 * @cookie: cookie we are interested in
 * @state: dma_tx_state structure to return last/used cookies
 *
 * Report the status of the cookie, filling in the state structure if
 * non-NULL.  No locking is required.
 */
static inline enum dma_status zxdh_dma_cookie_status(struct dma_chan *chan,
	dma_cookie_t cookie, struct dma_tx_state *state)
{
	dma_cookie_t used, complete;

	used = chan->cookie;
	complete = chan->completed_cookie;
	barrier();
	if (state) {
		state->residue = 0;
		state->in_flight_bytes = 0;
	}

	if (complete <= used) {
		if ((cookie <= complete) || (cookie > used))
			return DMA_COMPLETE;
	} else {
		if ((cookie <= complete) && (cookie > used))
			return DMA_COMPLETE;
	}
	return DMA_IN_PROGRESS;
}

static inline void zxdh_dma_set_residue(struct dma_tx_state *state, u32 residue)
{
	if (state)
		state->residue = residue;
}

static inline void zxdh_dma_set_in_flight_bytes(struct dma_tx_state *state,
					   u32 in_flight_bytes)
{
	if (state)
		state->in_flight_bytes = in_flight_bytes;
}

struct zxdh_dmaengine_desc_callback {
	dma_async_tx_callback callback;
	dma_async_tx_callback_result callback_result;
	void *callback_param;
};

/**
 * zxdh_dmaengine_desc_get_callback - get the passed in callback function
 * @tx: tx descriptor
 * @cb: temp struct to hold the callback info
 *
 * Fill the passed in cb struct with what's available in the passed in
 * tx descriptor struct
 * No locking is required.
 */
static inline void
zxdh_dmaengine_desc_get_callback(struct dma_async_tx_descriptor *tx,
			    struct zxdh_dmaengine_desc_callback *cb)
{
	cb->callback = tx->callback;
	cb->callback_result = tx->callback_result;
	cb->callback_param = tx->callback_param;
}

/**
 * zxdh_dmaengine_desc_callback_invoke - call the callback function in cb struct
 * @cb: temp struct that is holding the callback info
 * @result: transaction result
 *
 * Call the callback function provided in the cb struct with the parameter
 * in the cb struct.
 * Locking is dependent on the driver.
 */
static inline void
zxdh_dmaengine_desc_callback_invoke(struct zxdh_dmaengine_desc_callback *cb,
			       const struct dmaengine_result *result)
{
	struct dmaengine_result dummy_result = {
		.result = DMA_TRANS_NOERROR,
		.residue = 0
	};

	if (cb->callback_result) {
		if (!result)
			result = &dummy_result;
		cb->callback_result(cb->callback_param, result);
	} else if (cb->callback) {
		cb->callback(cb->callback_param);
	}
}

/**
 * zxdh_dmaengine_desc_get_callback_invoke - get the callback in tx descriptor and
 * 					then immediately call the callback.
 * @tx: dma async tx descriptor
 * @result: transaction result
 *
 * Call zxdh_dmaengine_desc_get_callback() and zxdh_dmaengine_desc_callback_invoke()
 * in a single function since no work is necessary in between for the driver.
 * Locking is dependent on the driver.
 */
static inline void
zxdh_dmaengine_desc_get_callback_invoke(struct dma_async_tx_descriptor *tx,
				   const struct dmaengine_result *result)
{
	struct zxdh_dmaengine_desc_callback cb;

	zxdh_dmaengine_desc_get_callback(tx, &cb);
	zxdh_dmaengine_desc_callback_invoke(&cb, result);
}

/**
 * zxdh_dmaengine_desc_callback_valid - verify the callback is valid in cb
 * @cb: callback info struct
 *
 * Return a bool that verifies whether callback in cb is valid or not.
 * No locking is required.
 */
static inline bool
zxdh_dmaengine_desc_callback_valid(struct zxdh_dmaengine_desc_callback *cb)
{
	return cb->callback || cb->callback_result;
}

struct dma_chan *zxdh_dma_get_slave_channel(struct dma_chan *chan);
struct dma_chan *zxdh_dma_get_any_slave_channel(struct dma_device *device);
void zxdh_dma_async_tx_descriptor_init(struct dma_async_tx_descriptor *tx,
	struct dma_chan *chan);

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>

static inline struct dentry *
zxdh_zxdh_dmaengine_get_debugfs_root(struct dma_device *dma_dev) {
	return dma_dev->dbg_dev_root;
}
#else
struct dentry;
static inline struct dentry *
zxdh_zxdh_dmaengine_get_debugfs_root(struct dma_device *dma_dev)
{
	return NULL;
}
#endif /* CONFIG_DEBUG_FS */

#endif
