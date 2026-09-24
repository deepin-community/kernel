#include <linux/dinghai/kcompat.h>
#include <linux/netdevice.h>
#include <linux/scatterlist.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/page_ref.h>
#include <linux/pci.h>
#ifndef CGS_V5_693
#include <net/xdp.h>
#include <linux/bpf.h>
#endif
#include <xen/xen.h>
#include "../en_aux.h"
#include "queue.h"
#include "en_1588_pkt_proc.h"
#include "../en_ethtool/ethtool.h"
#include "zxdh_tools/zxdh_tools_ioctl.h"
#include "../slib.h"

#define RCV_1588_MSG_BIT 27

/* Amount of XDP headroom to prepend to packets for use by xdp_adjust_head */
#define ZXDH_XDP_HEADROOM 256

/* Separating two types of XDP xmit */
#define ZXDH_XDP_TX     BIT(0)
#define ZXDH_XDP_REDIR  BIT(1)
#define ZXDH_XDP_FLAG   BIT(0)

#define PAGE_REF_ELEV  (U16_MAX)
#define NUM_FREE_MASK  0xfffffff0

#define ZXDH_MAX_FRAG_NUM 64
struct zxdh_frag
{
    uint32_t frag_num;
    uint32_t frag_id[ZXDH_MAX_FRAG_NUM];
};

static void dh_rx_cache_reduce_clean_pending(struct receive_queue *rq);
static void dh_rx_cache_reduce_work(struct work_struct *work);
static void dh_rx_cache_reduce_reset_watch(struct dh_page_cache *cache);

void dh_page_release_dynamic(struct receive_queue *rq,
                             struct dh_dma_info *dma_info,
                             bool recycle);

static void dh_put_page(struct dh_dma_info *dma_info)
{
    page_ref_sub(dma_info->page, dma_info->refcnt_bias);
    put_page(dma_info->page);
}

static inline void page_ref_elev(struct dh_dma_info *dma_info)
{
    page_ref_add(dma_info->page, PAGE_REF_ELEV);
    dma_info->refcnt_bias += PAGE_REF_ELEV;
}

/*
* The DMA ops on various arches are rather gnarly right now, and
* making all of the arch DMA ops work on the vring device itself
* is a mess.  For now, we use the parent device for DMA ops.
*/
static inline struct device *vring_dma_dev(const struct vring_virtqueue *vq)
{
    return vq->vq.en_dev->dmadev;
}

static inline bool dh_rx_cache_is_empty(struct dh_page_cache *cache)
{
    return cache->head < 0;
}

void dh_rx_free_page_cache(struct receive_queue *rq)
{
    struct dh_page_cache *cache = &rq->vq->page_cache;
    struct dh_page_cache_reduce *reduce = &cache->reduce;
    int i;

    if (unlikely(NULL == cache->page_cache))
        return;

    cancel_delayed_work_sync(&reduce->reduce_work);
    dh_rx_cache_reduce_clean_pending(rq);
    kvfree(reduce->pending);

    for (i = 0; i <= cache->head; i++)
    {
        struct dh_dma_info *dma_info = &cache->page_cache[i];

        dh_page_release_dynamic(rq, dma_info, false);
        dma_info->page = NULL;
    }
    kvfree(cache->page_cache);
    zte_memset_s(cache, 0, sizeof(struct dh_page_cache));
    cache->head = -1;
}

static inline bool dh_rx_cache_page_busy(struct dh_page_cache *cache,
                    uint32_t i)
{
    struct dh_dma_info *di = &cache->page_cache[i];
    return (page_ref_count(di->page) - di->refcnt_bias) != 1;
}

static inline void dh_rx_cache_page_swap(struct dh_page_cache *cache,
                    uint32_t a, uint32_t b)
{
    struct dh_dma_info tmp;

    tmp = cache->page_cache[a];
    cache->page_cache[a] = cache->page_cache[b];
    cache->page_cache[b] = tmp;
}


static inline bool dh_rx_cache_get(struct receive_queue *rq,
                    struct dh_dma_info *dma_info)
{
    struct dh_page_cache *cache = &rq->vq->page_cache;
    struct dh_rq_stats *stats = &rq->_stats;

    struct virtqueue *_vq = rq->vq;
    struct vring_virtqueue *vq = to_vvq(_vq);

    if (unlikely(NULL == cache->page_cache))
        return false;

    if (unlikely(dh_rx_cache_is_empty(cache)))
    {
        stats->cache_empty++;
        goto err_no_page;
    }

    dh_rx_cache_page_swap(cache, cache->head, cache->lrs);
    cache->lrs++;
    if (cache->lrs >= cache->head)
        cache->lrs = 0;
    if (dh_rx_cache_page_busy(cache, cache->head))
    {
        stats->cache_busy++;
        goto err_no_page;
    }

    stats->cache_reuse++;
    *dma_info = cache->page_cache[cache->head--];

    dma_sync_single_for_device(vring_dma_dev(vq), dma_info->addr,
                                PAGE_SIZE,
                                DMA_FROM_DEVICE);
    if (unlikely(page_ref_count(dma_info->page) <= PAGE_REF_THRSD))
        page_ref_elev(dma_info);

    return true;

err_no_page:
    stats->cache_alloc++;
    cache->reduce.successive = 0;

    return false;
}

static inline int dh_page_alloc_pool(struct receive_queue *rq,
                    struct dh_dma_info *dma_info)
{
    struct virtqueue *_vq = rq->vq;
    struct vring_virtqueue *vq = to_vvq(_vq);

    if (dh_rx_cache_get(rq, dma_info))
        return 0;

    dma_info->page = dev_alloc_page();
    if (unlikely(!dma_info->page))
        return -ENOMEM;

    dma_info->refcnt_bias = 0;
    page_ref_elev(dma_info);

    if (!vq->use_dma_api)
    {
        dma_info->addr = (dma_addr_t)page_to_phys(dma_info->page);
        return 0;
    }

    dma_info->addr = dma_map_page(vring_dma_dev(vq), dma_info->page, 0,
                                    PAGE_SIZE, DMA_FROM_DEVICE);
    if (unlikely(dma_mapping_error(vring_dma_dev(vq), dma_info->addr)))
    {
        dh_put_page(dma_info);
        dma_info->page = NULL;
        return -ENOMEM;
    }
    return 0;
}

int dh_rx_alloc_page_cache(struct virtqueue *vq, uint8_t log_init_sz)
{
    struct dh_page_cache *cache = &vq->page_cache;
    struct dh_page_cache_reduce *reduce = &cache->reduce;
    uint32_t max_sz;

    cache->log_max_sz = log_init_sz + DH_PAGE_CACHE_LOG_MAX_RQ_MULT;
    cache->log_min_sz = log_init_sz;
    max_sz = 1 << cache->log_max_sz;

    cache->page_cache = kvzalloc(max_sz * sizeof(*cache->page_cache),
                    GFP_KERNEL);
    if (!cache->page_cache)
        return -ENOMEM;

    reduce->pending = kvzalloc(max_sz * sizeof(*reduce->pending),
                    GFP_KERNEL);
    if (!reduce->pending)
        goto err_free_cache;

    cache->sz = 1 << cache->log_min_sz;
    cache->head = -1;
    cache->lrs = 0;
    INIT_DELAYED_WORK(&reduce->reduce_work, dh_rx_cache_reduce_work);
    reduce->successive = 0;
    reduce->npages = 0;
    reduce->delay = msecs_to_jiffies(DH_PAGE_CACHE_REDUCE_WORK_INTERVAL);
    reduce->graceful_period = msecs_to_jiffies(DH_PAGE_CACHE_REDUCE_GRACE_PERIOD);
    reduce->next_ts = MAX_JIFFY_OFFSET; /* in init, no reduce is needed */

    return 0;

err_free_cache:
    kvfree(cache->page_cache);
    cache->page_cache = NULL;

    return -ENOMEM;
}

static inline bool dh_rx_cache_check_reduce(struct receive_queue *rq)
{
    struct dh_page_cache *cache = &rq->vq->page_cache;

    if (!cache->page_cache)
        return false;

    if (unlikely(test_bit(DH_RQ_STATE_CACHE_REDUCE_PENDING, &rq->state)))
        return false;

    if (time_before(jiffies, cache->reduce.next_ts))
        return false;

    if (likely(!dh_rx_cache_is_empty(cache)) &&
        dh_rx_cache_page_busy(cache, cache->head))
        goto reset_watch;

    if (ilog2(cache->sz) == cache->log_min_sz)
        goto reset_watch;

    /* would like to reduce */
    if (cache->reduce.successive < DH_PAGE_CACHE_REDUCE_SUCCESSIVE_CNT) {
        cache->reduce.successive++;
        return false;
    }

    return true;

reset_watch:
    dh_rx_cache_reduce_reset_watch(cache);
    return false;
}

static inline void dh_rx_cache_may_reduce(struct receive_queue *rq)
{
    struct dh_page_cache *cache = &rq->vq->page_cache;
    struct dh_page_cache_reduce *reduce = &cache->reduce;
    int max_new_head;

    if (!dh_rx_cache_check_reduce(rq))
        return;

    /* do reduce */
    rq->_stats.cache_reduce++;
    cache->sz >>= 1;
    max_new_head = (cache->sz >> 1) - 1;
    if (cache->head > max_new_head)
    {
        uint32_t npages = cache->head - max_new_head;

        cache->head = max_new_head;
        if (cache->lrs >= cache->head)
            cache->lrs = 0;

        zte_memcpy_s(reduce->pending, &cache->page_cache[cache->head + 1],
            npages * sizeof(*reduce->pending));
        reduce->npages = npages;
        set_bit(DH_RQ_STATE_CACHE_REDUCE_PENDING, &rq->state);
    }

    dh_rx_cache_reduce_reset_watch(cache);
}


static void dh_rx_cache_reduce_clean_pending(struct receive_queue *rq)
{
    struct dh_page_cache_reduce *reduce = &rq->vq->page_cache.reduce;
    int i;

    if (!test_bit(DH_RQ_STATE_CACHE_REDUCE_PENDING, &rq->state))
        return;

    for (i = 0; i < reduce->npages; i++)
    {
        dh_page_release_dynamic(rq, &reduce->pending[i], false);
    }
    reduce->npages = 0;

    clear_bit(DH_RQ_STATE_CACHE_REDUCE_PENDING, &rq->state);
}

static void dh_rx_cache_reduce_work(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct dh_page_cache_reduce *reduce =
        container_of(dwork, struct dh_page_cache_reduce, reduce_work);
    struct dh_page_cache *cache =
        container_of(reduce, struct dh_page_cache, reduce);
    struct virtqueue *vq = container_of(cache, struct virtqueue, page_cache);
    struct receive_queue *rq = vq->rq;

    local_bh_disable();
    napi_schedule(&rq->napi);
    local_bh_enable();
    dh_rx_cache_reduce_clean_pending(rq);

    if (ilog2(cache->sz) > cache->log_min_sz)
        schedule_delayed_work_on(smp_processor_id(), dwork, reduce->delay);
}

static inline void
dh_rx_cache_reduce_reset_watch(struct dh_page_cache *cache)
{
    struct dh_page_cache_reduce *reduce = &cache->reduce;

    reduce->next_ts = ilog2(cache->sz) == cache->log_min_sz ?
                        MAX_JIFFY_OFFSET :
                        jiffies + reduce->graceful_period;
    reduce->successive = 0;
}

static inline bool dh_rx_cache_extend(struct receive_queue *rq)
{
    struct dh_page_cache *cache = &rq->vq->page_cache;
    struct dh_page_cache_reduce *reduce = &cache->reduce;

    if (ilog2(cache->sz) >= cache->log_max_sz)
        return false;

    rq->_stats.cache_extend++;
    cache->sz <<= 1;

    dh_rx_cache_reduce_reset_watch(cache);
    schedule_delayed_work_on(smp_processor_id(), &reduce->reduce_work,
                reduce->delay);
    return true;
}

static inline bool dh_page_is_reserved(struct page *page)
{
    return page_is_pfmemalloc(page) || page_to_nid(page) != numa_mem_id();
}

static inline bool dh_rx_cache_put(struct receive_queue *rq,
                    struct dh_dma_info *dma_info)
{
    struct dh_page_cache *cache = &rq->vq->page_cache;
    struct dh_rq_stats *stats = &rq->_stats;

    if (unlikely(NULL == cache->page_cache))
        return false;

    if (unlikely(cache->head == cache->sz - 1))
    {
        if (!dh_rx_cache_extend(rq))
        {
            rq->_stats.cache_full++;
            return false;
        }
    }

    if (unlikely(dh_page_is_reserved(dma_info->page)))
    {
        stats->cache_waive++;
        return false;
    }
    cache->page_cache[++cache->head] = *dma_info;

    return true;
}

void dh_page_dma_unmap(struct receive_queue *rq,
                struct dh_dma_info *dma_info)
{
    struct virtqueue *_vq = rq->vq;
    struct vring_virtqueue *vq = to_vvq(_vq);
    if (vq->use_dma_api)
    {
        dma_unmap_page_attrs(vring_dma_dev(vq), dma_info->addr,
                PAGE_SIZE, DMA_FROM_DEVICE, DMA_ATTR_SKIP_CPU_SYNC);
        // dma_unmap_page(vring_dma_dev(vq), dma_info->addr, PAGE_SIZE, DMA_FROM_DEVICE);
    }
}

void dh_page_release_dynamic(struct receive_queue *rq,
                struct dh_dma_info *dma_info,
                bool recycle)
{
    if (likely(recycle) && dh_rx_cache_put(rq, dma_info))
        return;

    dh_page_dma_unmap(rq, dma_info);
    dh_put_page(dma_info);
}

static uint32_t features_table[] =
{
    ZXDH_NET_F_MRG_RXBUF, ZXDH_NET_F_STATUS, ZXDH_NET_F_CTRL_VQ, ZXDH_NET_F_MQ, \
    ZXDH_RING_F_INDIRECT_DESC, ZXDH_RING_F_EVENT_IDX, ZXDH_F_VERSION_1, ZXDH_F_RING_PACKED
};

#ifndef CGS_V5_693
static bool is_xdp_frame(void *ptr)
{
    return (unsigned long)ptr & ZXDH_XDP_FLAG;
}

static void *xdp_to_ptr(struct xdp_frame *ptr)
{
    return (void *)((unsigned long)ptr | ZXDH_XDP_FLAG);
}

static struct xdp_frame *ptr_to_xdp(void *ptr)
{
    return (struct xdp_frame *)((unsigned long)ptr & ~ZXDH_XDP_FLAG);
}

static unsigned int zxdh_en_get_headroom(struct zxdh_en_device *en_dev)
{
    return en_dev->xdp_enabled ? ZXDH_XDP_HEADROOM : 0;
}

/* We copy the packet for XDP in the following cases:
 *
 * 1) Packet is scattered across multiple rx buffers.
 * 2) Headroom space is insufficient.
 *
 * This is inefficient but it's a temporary condition that
 * we hit right after XDP is enabled and until queue is refilled
 * with large buffers with sufficient headroom - so it should affect
 * at most queue size packets.
 * Afterwards, the conditions to enable
 * XDP should preclude the underlying device from sending packets
 * across multiple buffers (num_buf > 1), and we make sure buffers
 * have enough headroom.
 */
static struct page *xdp_linearize_page(struct receive_queue *rq,
                       u16 *num_buf,
                       struct page *p,
                       int offset,
                       int page_off,
                       unsigned int *len)
{
    struct page *page = alloc_page(GFP_ATOMIC);
    struct vring_virtqueue *vq = to_vvq(rq->vq);

    if (!page)
        return NULL;

    memcpy(page_address(page) + page_off, page_address(p) + offset, *len);
    page_off += *len;

    while (--*num_buf) {
        int tailroom = SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
        unsigned int buflen;
        void *buf;
        int off;
        uint32_t _id;

        buf = virtqueue_get_buf_ctx(rq->vq, &buflen, NULL, FALSE, &_id);
        if (unlikely(!buf))
            goto err_buf;

        p = virt_to_head_page(buf);
        off = buf - page_address(p);

        /* guard against a misconfigured or uncooperative backend that
         * is sending packet larger than the MTU.
         */
        if ((page_off + buflen + tailroom) > PAGE_SIZE) {
            if (vq->packed.desc_extra[_id].last_in_page)
            {
                dh_page_release_dynamic(rq, vq->packed.desc_extra[_id].di, true);
                vq->packed.desc_extra[_id].di->page = NULL;
            }
            goto err_buf;
        }

        memcpy(page_address(page) + page_off,
               page_address(p) + off, buflen);
        page_off += buflen;
        if (vq->packed.desc_extra[_id].last_in_page)
        {
            dh_page_release_dynamic(rq, vq->packed.desc_extra[_id].di, true);
            vq->packed.desc_extra[_id].di->page = NULL;
        }
    }

    /* Headroom does not contribute to packet length */
    *len = page_off - ZXDH_XDP_HEADROOM;
    return page;
err_buf:
    __free_pages(page, 0);
    return NULL;
}
#endif

/* Started by AICoder, pid:190d6pb680vb44f144e40bc550817b92a7110b94 */
void zxdh_print_vring_info(struct virtqueue *vq, uint32_t desc_index, uint32_t desc_num)
{
    struct vring_virtqueue *vvq = to_vvq(vq);
    struct vring_packed_desc *packed_desc = NULL;
    struct vring_desc *split_desc = NULL;
    struct vring_avail *split_avail = NULL;
    struct vring_used *split_used = NULL;
    uint32_t i = 0;
    uint32_t j = 0;

    if (vvq->packed_ring == true)
    {
        LOG_INFO("queue_index       : %d\n", vq->index);
        LOG_INFO("phy_index         : %d\n", vq->phy_index);
        LOG_INFO("desc_addr         : 0x%llx\n", (uint64_t)vvq->packed.vring.desc);
        LOG_INFO("desc_pa           : 0x%llx\n", (uint64_t)virt_to_phys(vvq->packed.vring.desc));
        LOG_INFO("ring_dma_addr     : 0x%llx\n", (uint64_t)vvq->packed.ring_dma_addr);
        LOG_INFO("driver_dma_addr   : 0x%llx\n", (uint64_t)vvq->packed.driver_event_dma_addr);
        LOG_INFO("device_dma_addr   : 0x%llx\n", (uint64_t)vvq->packed.device_event_dma_addr);
        LOG_INFO("vring size        : %d\n", vvq->packed.vring.num);
        LOG_INFO("num_free          : %d\n", vq->num_free);
        LOG_INFO("free_head         : %d\n", vvq->free_head);
        LOG_INFO("last_used_idx     : %d\n", vvq->last_used_idx);
        LOG_INFO("avail_wrap_counter: %d\n", vvq->packed.avail_wrap_counter);
        LOG_INFO("avail_used_flags  : %d\n", vvq->packed.avail_used_flags);
        LOG_INFO("next_avail_idx    : %d\n", vvq->packed.next_avail_idx);
        LOG_INFO("driver->flags     : 0x%x\n", vvq->packed.vring.driver->flags);
        LOG_INFO("driver->off_wrap  : %d\n", vvq->packed.vring.driver->off_wrap);
        LOG_INFO("device->flags     : 0x%x\n", vvq->packed.vring.device->flags);
        LOG_INFO("device->off_wrap  : %d\n", vvq->packed.vring.device->off_wrap);
        LOG_INFO("DESC[x]:\tDESC_ADDR\t[BUFFER_ADDR]\t\t[LEN]\t\t[ID]\t[FLAG]\n");

        for (i = 0; i < desc_num; i++)
        {
            j = (desc_index + i) % vvq->packed.vring.num;
            packed_desc = &vvq->packed.vring.desc[j];
            LOG_INFO("DESC[%d] 0x%llx:\t0x%016llx\t0x%08x\t%8d\t0x%x\n", \
                    j, (uint64_t)packed_desc, packed_desc->addr, packed_desc->len, packed_desc->id, packed_desc->flags);
        }
    }
    else
    {
        struct zxdh_en_device *en_dev = vq->en_dev;
        LOG_INFO("queue_index       : %d\n", en_dev->logic_index_split[vq->index]);
        LOG_INFO("phy_index         : %d\n", vq->phy_index);
        LOG_INFO("desc_addr         : 0x%llx\n", (uint64_t)vvq->split.vring.desc);
        LOG_INFO("desc_pa           : 0x%llx\n", (uint64_t)virt_to_phys(vvq->split.vring.desc));
        LOG_INFO("queue_dma_addr    : 0x%llx\n", (uint64_t)vvq->split.queue_dma_addr);
        LOG_INFO("avail_off         : 0x%llx\n", (uint64_t)vvq->split.vring.avail - (uint64_t)vvq->split.vring.desc);
        LOG_INFO("used_off          : 0x%llx\n", (uint64_t)vvq->split.vring.used - (uint64_t)vvq->split.vring.desc);
        LOG_INFO("vring size        : %d\n", vvq->split.vring.num);
        LOG_INFO("num_free          : %d\n", vq->num_free);
        LOG_INFO("free head         : %d\n", vvq->free_head);
        LOG_INFO("last_used_idx     : %d\n", vvq->last_used_idx);
        LOG_INFO("avail_idx_shadow  : %d\n", vvq->split.avail_idx_shadow);
        LOG_INFO("avail->flags      : 0x%04x\n", (uint16_t)vvq->split.vring.avail->flags);
        LOG_INFO("avail->idx        : 0x%04x\n", (uint16_t)vvq->split.vring.avail->idx);
        LOG_INFO("used->flags       : 0x%04x\n", (uint16_t)vvq->split.vring.used->flags);
        LOG_INFO("used->idx         : 0x%04x\n", (uint16_t)vvq->split.vring.used->idx);
        LOG_INFO("avail_event_flag  : 0x%04x\n", (uint16_t)(vring_avail_event(&vvq->split.vring)));
        LOG_INFO("used_event_flag   : 0x%04x\n", (uint16_t)(vring_used_event(&vvq->split.vring)));
        LOG_INFO("DESC[x]:\tDESC_ADDR\t[BUFFER_ADDR]\t\t[LEN]\t\t[ID]\t[FLAG]\n");

        for (i = 0; i < desc_num; i++)
        {
            j = (desc_index + i) % vvq->split.vring.num;
            split_desc = &vvq->split.vring.desc[j];
            LOG_INFO("DESC[%d] 0x%llx:\t0x%016llx\t0x%08x\t%8d\t0x%x\n", \
                    j, (uint64_t)split_desc, split_desc->addr, split_desc->len, split_desc->next, split_desc->flags);
        }

        LOG_INFO("\n");
        for (i = 0; i < desc_num; i++)
        {
            j = (desc_index + i) % vvq->split.vring.num;
            split_avail = &vvq->split.vring.avail[j];
            LOG_INFO("AVAIL[%d] id:%d\n", j, split_avail->ring[j]);
        }
        LOG_INFO("\n");

        for (i = 0; i < desc_num; i++)
        {
            j = (desc_index + i) % vvq->split.vring.num;
            split_used = &vvq->split.vring.used[j];
            LOG_INFO("USED[%d] id:%d, len:0x%x\n", j, split_used->ring[j].id, split_used->ring[j].len);
        }
    }

    return;
}
/* Ended by AICoder, pid:190d6pb680vb44f144e40bc550817b92a7110b94 */

/* enable irq handlers */
void zxdh_vp_enable_cbs(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t i = 0;

    for (i = 0; i< en_dev->channels_num; i++)
    {
        en_dev->ops->switch_vqs_channel(en_dev->parent, i, 1);
    }
}

/* disable irq handlers */
void zxdh_vp_disable_cbs(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t i = 0;

    for (i = 0; i< en_dev->channels_num; i++)
    {
        en_dev->ops->switch_vqs_channel(en_dev->parent, i, 0);
    }
}

#define  VP_RESET_MS_TIMEOUT_CNT        (10000)
void zxdh_vp_reset(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t timeout_cnt = 0;

    /* 0 status means a reset. */
    en_dev->ops->set_status(en_dev->parent, 0);

    /* After writing 0 to device_status, the driver MUST wait for a read of
     * device_status to return 0 before reinitializing the device.
     * This will flush out the status write, and flush in device writes,
     * including MSI-X interrupts, if any.
     */
    while (en_dev->ops->get_status(en_dev->parent) != 0)
    {
        msleep(1);
        timeout_cnt++;
        if (timeout_cnt >= VP_RESET_MS_TIMEOUT_CNT)
        {
            LOG_ERR_DEV(en_dev->parent, "vp reset time out!\n");
            break;
        }
    }
    LOG_INFO_DEV(en_dev->parent, "vp reset take %u ms\n", timeout_cnt);

    return;
}

void zxdh_add_status(struct net_device *netdev, uint32_t status)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint8_t dev_status = 0;

    might_sleep();

    dev_status = en_dev->ops->get_status(en_dev->parent);

    en_dev->ops->set_status(en_dev->parent, (dev_status | status));

    return;
}

bool zxdh_has_status(struct net_device *netdev, uint32_t sbit)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint8_t dev_status = 0;

    dev_status = en_dev->ops->get_status(en_dev->parent);

    return (dev_status & sbit);
}

bool zxdh_has_feature(struct zxdh_en_device *en_dev, uint32_t fbit)
{
    return en_dev->guest_feature & BIT_ULL(fbit);
}

void zxdh_pf_features_init(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t i = 0;
    uint64_t features = 0;

    en_dev->device_feature = en_dev->ops->get_features(en_dev->parent);
    en_dev->driver_feature = 0;

    for (i = 0; i < ARRAY_SIZE(features_table); i++)
    {
        features = features_table[i];
        en_dev->driver_feature |= (1ULL << features);
    }

    en_dev->guest_feature = en_dev->device_feature & 0xfffffff7dfffffff;
    en_dev->packed_status = en_dev->ops->get_split_packed(en_dev->parent);
    if (!en_dev->packed_status)
    {
        en_dev->guest_feature |= BIT(29);
        en_dev->guest_feature &= 0xfffffffbffffffff;
        netdev->features &= ~(NETIF_F_RXCSUM | NETIF_F_HW_CSUM | NETIF_F_TSO | NETIF_F_LRO | NETIF_F_TSO6 | NETIF_F_GSO_UDP_TUNNEL_CSUM | NETIF_F_GSO_GRE | NETIF_F_GSO_GRE_CSUM);
        netdev->hw_features &= ~(NETIF_F_RXCSUM | NETIF_F_HW_CSUM | NETIF_F_TSO | NETIF_F_LRO | NETIF_F_TSO6 | NETIF_F_GSO_UDP_TUNNEL_CSUM | NETIF_F_GSO_GRE | NETIF_F_GSO_GRE_CSUM);
    }
    else
    {
        en_dev->guest_feature |= (1ULL << 34);
    }
    LOG_DEBUG_DEV(en_dev->parent, "device_feature: 0x%llx, guest_feature: 0x%llx, packed_status: %d\n", en_dev->device_feature, en_dev->guest_feature, en_dev->packed_status);
    en_dev->ops->set_features(en_dev->parent, en_dev->guest_feature);

    return;
}

int32_t vq2txq(struct virtqueue *vq)
{
    return (vq->index - 1) / 2;
}

int32_t txq2vq(int32_t txq)
{
    return txq * 2 + 1;
}
int32_t vq2rxq(struct virtqueue *vq)
{
    return vq->index / 2;
}

int32_t rxq2vq(int32_t rxq)
{
    return rxq * 2;
}

inline void vqm_mb(bool weak_barriers)
{
    if (weak_barriers)
    {
        virt_mb();
    }
    else
    {
        mb();
    }
}

inline void vqm_rmb(bool weak_barriers)
{
    if (weak_barriers)
    {
        virt_rmb();
    }
    else
    {
        dma_rmb();
    }
}

inline void vqm_wmb(bool weak_barriers)
{
    if (weak_barriers)
    {
        virt_wmb();
    }
    else
    {
        dma_wmb();
    }
}

void vring_del_virtqueue(struct virtqueue *_vq)
{
    struct zxdh_en_device *en_dev = _vq->en_dev;
    struct vring_virtqueue *vq = to_vvq(_vq);

    spin_lock(&en_dev->vqs_list_lock);
    list_del(&_vq->list);
    spin_unlock(&en_dev->vqs_list_lock);

    if (vq->we_own_ring)
    {
        if (vq->packed_ring)
        {
            vring_free_queue(vq->vq.en_dev,
                    vq->packed.ring_size_in_bytes,
                    vq->packed.vring.desc,
                    vq->packed.ring_dma_addr);

            vring_free_queue(vq->vq.en_dev,
                    vq->packed.event_size_in_bytes,
                    vq->packed.vring.driver,
                    vq->packed.driver_event_dma_addr);

            vring_free_queue(vq->vq.en_dev,
                    vq->packed.event_size_in_bytes,
                    vq->packed.vring.device,
                    vq->packed.device_event_dma_addr);

            kfree(vq->packed.desc_state);
            vq->packed.desc_state = NULL;
            if (vq->packed.desc_extra[0].di)
                kfree(vq->packed.desc_extra[0].di);
            kfree(vq->packed.desc_extra);
            vq->packed.desc_extra = NULL;
        }
        else
        {
            vring_free_queue(vq->vq.en_dev,
                    vq->split.queue_size_in_bytes,
                    vq->split.vring.desc,
                    vq->split.queue_dma_addr);
        }
    }
    if (!vq->packed_ring)
    {
        kfree(vq->split.desc_state);
        vq->split.desc_state = NULL;
        kfree(vq->split.desc_extra);
        vq->split.desc_extra = NULL;
    }

    kfree(vq);
    vq = NULL;
}

void del_vq(struct zxdh_pci_vq_info *info)
{
    struct virtqueue *vq = info->vq;
    struct zxdh_en_device *en_dev = vq->en_dev;

    en_dev->ops->vp_modern_unmap_vq_notify(en_dev->parent, vq->priv);

    vring_del_virtqueue(vq);
}

void vp_del_vq(struct virtqueue *vq)
{
    struct zxdh_en_device *en_dev = vq->en_dev;
    struct zxdh_pci_vq_info *info = en_dev->vqs[vq->index];
    unsigned long flags;

    spin_lock_irqsave(&en_dev->lock, flags);
    list_del(&info->node);
    spin_unlock_irqrestore(&en_dev->lock, flags);

    en_dev->vqs[vq->index] = NULL;
    del_vq(info);
    kfree(info);
}

void vp_detach_vqs(void *para)
{
    struct net_device *netdev = para;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct virtqueue *vq;
    struct virtqueue *n;

    list_for_each_entry_safe(vq, n, &en_dev->vqs_list, list)
    {
        vp_del_vq(vq);
    }
}

void vp_del_vqs(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    vp_detach_vqs(netdev);

    kfree(en_dev->vqs);
    en_dev->vqs = NULL;
}

/**
 * virtqueue_get_vring_size - return the size of the virtqueue's vring
 * @_vq: the struct virtqueue containing the vring of interest.
 *
 * Returns the size of the vring.  This is mainly used for boasting to
 * userspace.  Unlike other operations, this need not be serialized.
 */
uint32_t virtqueue_get_vring_size(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    return vq->packed_ring ? vq->packed.vring.num : vq->split.vring.num;
}

dma_addr_t virtqueue_get_desc_addr(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    BUG_ON(!vq->we_own_ring);

    if (vq->packed_ring)
        return vq->packed.ring_dma_addr;

    return vq->split.queue_dma_addr;
}

dma_addr_t virtqueue_get_avail_addr(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    BUG_ON(!vq->we_own_ring);

    if (vq->packed_ring)
        return vq->packed.driver_event_dma_addr;

    return vq->split.queue_dma_addr + ((char *)vq->split.vring.avail - (char *)vq->split.vring.desc);
}

dma_addr_t virtqueue_get_used_addr(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    BUG_ON(!vq->we_own_ring);

    if (vq->packed_ring)
        return vq->packed.device_event_dma_addr;

    return vq->split.queue_dma_addr + ((char *)vq->split.vring.used - (char *)vq->split.vring.desc);
}

bool vqm_has_dma_quirk(struct zxdh_en_device *en_dev)
{
    /*
     * Note the reverse polarity of the quirk feature (compared to most
     * other features), this is for compatibility with legacy systems.
     */
    return !zxdh_has_feature(en_dev, ZXDH_F_ACCESS_PLATFORM);
}

bool vring_use_dma_api(struct zxdh_en_device *en_dev)
{
    if (!vqm_has_dma_quirk(en_dev))
    {
        return true;
    }

    /* Otherwise, we are left to guess. */
    /*
     * In theory, it's possible to have a buggy QEMU-supposed
     * emulated Q35 IOMMU and Xen enabled at the same time.  On
     * such a configuration, zxdh has never worked and will
     * not work without an even larger kludge.  Instead, enable
     * the DMA API if we're a Xen guest, which at least allows
     * all of the sensible Xen configurations to work correctly.
     */
    if (xen_domain())
    {
        return true;
    }

    return false;
}

void vring_free_queue(struct zxdh_en_device *en_dev, size_t size, void *queue, dma_addr_t dma_handle)
{
    if (vring_use_dma_api(en_dev))
    {
        dma_free_coherent(en_dev->dmadev, size, queue, dma_handle);
    }
    else
    {
        free_pages_exact(queue, PAGE_ALIGN(size));
    }
}

void *vring_alloc_queue(struct zxdh_en_device *en_dev, size_t size, dma_addr_t *dma_handle, gfp_t flag)
{
    if (vring_use_dma_api(en_dev))
    {
        return dma_alloc_coherent(en_dev->dmadev, size, dma_handle, flag);
    }
    else
    {
        void *queue = alloc_pages_exact(PAGE_ALIGN(size), flag);

        if (queue)
        {
            phys_addr_t phys_addr = virt_to_phys(queue);
            *dma_handle = (dma_addr_t)phys_addr;

            /*
             * Sanity check: make sure we dind't truncate
             * the address.  The only arches I can find that
             * have 64-bit phys_addr_t but 32-bit dma_addr_t
             * are certain non-highmem MIPS and x86
             * configurations, but these configurations
             * should never allocate physical pages above 32
             * bits, so this is fine.  Just in case, throw a
             * warning and abort if we end up with an
             * unrepresentable address.
             */
            if (WARN_ON_ONCE(*dma_handle != phys_addr))
            {
                free_pages_exact(queue, PAGE_ALIGN(size));
                return NULL;
            }
        }
        return queue;
    }
}

void vring_set_desc_extra_info(struct vring_desc_extra *desc_extra, uint32_t num)
{
    uint32_t i = 0;

    for (i = 0; i < num; i++)
    {
        desc_extra[i].next = i + 1;
        if (i == (num - 1))
            desc_extra[i].next = 0;
        else
            desc_extra[i].next = i + 1;
        desc_extra[i].offset += (i % (PAGE_SIZE  / DEFAULT_FRAG_LENGTH)) * DEFAULT_FRAG_LENGTH;
        desc_extra[i].last_in_page = ((i + 1) % (PAGE_SIZE / DEFAULT_FRAG_LENGTH)) ? 0 : 1;
        if (i > 0) {
           desc_extra[i].di = ((i % (PAGE_SIZE / DEFAULT_FRAG_LENGTH)) == 0) ?
                            (desc_extra[i-1].di + 1) : desc_extra[i-1].di;
        }
    }
}

struct vring_desc_extra *vring_alloc_desc_extra_packed(uint32_t num, uint32_t actual_num)
{
    struct vring_desc_extra *desc_extra = NULL;
    struct dh_dma_info *dh_dma = NULL;

    dh_dma = kmalloc_array(num, sizeof(struct dh_dma_info), GFP_KERNEL);
    if (unlikely(dh_dma == NULL))
    {
        LOG_ERR("dh_dma array failed\n");
        return NULL;
    }
    zte_memset_s(dh_dma, 0, num * sizeof(struct dh_dma_info));

    desc_extra = kmalloc_array(num, sizeof(struct vring_desc_extra), GFP_KERNEL);
    if (unlikely(desc_extra == NULL))
    {
        LOG_ERR("desc_extra kmalloc_array failed\n");
        kfree(dh_dma);
        return NULL;
    }

    zte_memset_s(desc_extra, 0, num * sizeof(struct vring_desc_extra));

    desc_extra[0].di = &dh_dma[0];
    vring_set_desc_extra_info(desc_extra, actual_num);

    return desc_extra;
}

struct vring_desc_extra *vring_alloc_desc_extra_split(uint32_t num)
{
    struct vring_desc_extra *desc_extra = NULL;
    uint32_t i = 0;

    desc_extra = kmalloc_array(num, sizeof(struct vring_desc_extra), GFP_KERNEL);
    if (unlikely(desc_extra == NULL))
    {
        LOG_ERR("desc_extra kmalloc_array failed\n");
        return NULL;
    }

    zte_memset_s(desc_extra, 0, num * sizeof(struct vring_desc_extra));

    for (i = 0; i < num - 1; i++)
    {
        desc_extra[i].next = i + 1;
    }

    return desc_extra;
}

struct vring_desc_extra *vring_alloc_desc_extra(struct zxdh_en_device *en_dev, uint32_t num, uint32_t actual_num)
{
    if (en_dev->packed_status)
    {
        return vring_alloc_desc_extra_packed(num, actual_num);
    }

    return vring_alloc_desc_extra_split(num);
}

unsigned vring_size(unsigned int num, unsigned long align)
{
	return ((sizeof(struct vring_desc) * num + sizeof(__vqm16) * (3 + num)
		 + align - 1) & ~(align - 1))
		+ sizeof(__vqm16) * 3 + sizeof(struct vring_used_elem) * num;
}

void vring_init(struct vring *vr, struct zxdh_en_device *en_dev, unsigned int num, void *p, unsigned long align)
{
	vr->num = num;
	vr->desc = p;
	vr->avail = (struct vring_avail *)((char *)p + ZXDH_PF_MAX_DESC_NUM(en_dev) * sizeof(struct vring_desc));
	vr->used = (void *)(((uintptr_t)&vr->avail->ring[ZXDH_PF_MAX_DESC_NUM(en_dev)] + sizeof(__vqm16)
		+ align-1) & ~(align - 1));
}

/* Only available for split ring */
struct virtqueue *__vring_new_virtqueue(unsigned int index,
					struct vring vring,
					struct zxdh_en_device *en_dev,
					bool context,
					bool (*notify)(struct virtqueue *),
					void (*callback)(struct virtqueue *),
					const char *name)
{
	struct vring_virtqueue *vq;

	if (en_dev->packed_status)
		return NULL;

	vq = kmalloc(sizeof(*vq), GFP_KERNEL);
	if (!vq)
		return NULL;

	vq->packed_ring = false;
	vq->vq.callback = callback;
	vq->vq.en_dev = en_dev;
	vq->vq.name = name;
	vq->vq.num_free = vring.num;
	vq->vq.index = index;
	vq->we_own_ring = false;
	vq->notify = notify;
	vq->weak_barriers = true;
	vq->broken = false;
	vq->last_used_idx = 0;
	vq->event_triggered = false;
	vq->num_added = 0;
	vq->use_dma_api = vring_use_dma_api(en_dev);
#ifdef DEBUG
	vq->in_use = false;
	vq->last_add_time_valid = false;
#endif

	vq->indirect = zxdh_has_feature(en_dev, ZXDH_RING_F_INDIRECT_DESC) &&
		!context;
	vq->event = 0;//zxdh_has_feature(en_dev, ZXDH_RING_F_EVENT_IDX);

	if (zxdh_has_feature(en_dev, ZXDH_F_ORDER_PLATFORM))
		vq->weak_barriers = false;

	vq->split.queue_dma_addr = 0;
	vq->split.queue_size_in_bytes = 0;

	vq->split.vring = vring;
	vq->split.avail_flags_shadow = 0;
	vq->split.avail_idx_shadow = 0;

	/* No callback?  Tell other side not to bother us. */
	if (!callback) {
		vq->split.avail_flags_shadow |= VRING_AVAIL_F_NO_INTERRUPT;
		if (!vq->event)
			vq->split.vring.avail->flags = cpu_to_vqm16(en_dev,
					vq->split.avail_flags_shadow);
	}

	vq->split.desc_state = kmalloc_array(ZXDH_PF_MAX_DESC_NUM(en_dev),
			sizeof(struct vring_desc_state_split), GFP_KERNEL);
	if (!vq->split.desc_state)
		goto err_state;

	vq->split.desc_extra = vring_alloc_desc_extra(en_dev, ZXDH_PF_MAX_DESC_NUM(en_dev), vring.num);
	if (!vq->split.desc_extra)
		goto err_extra;

	/* Put everything in free lists. */
	vq->free_head = 0;
	zte_memset_s(vq->split.desc_state, 0, ZXDH_PF_MAX_DESC_NUM(en_dev) * sizeof(struct vring_desc_state_split));

	spin_lock(&en_dev->vqs_list_lock);
	list_add_tail(&vq->vq.list, &en_dev->vqs_list);
	spin_unlock(&en_dev->vqs_list_lock);
	return &vq->vq;

err_extra:
	kfree(vq->split.desc_state);
	vq->split.desc_state = NULL;
err_state:
	kfree(vq);
	vq = NULL;
	return NULL;
}

struct virtqueue *vring_create_virtqueue_split(
	unsigned int index,
	unsigned int num,
	struct zxdh_en_device *en_dev,
	bool context,
	bool (*notify)(struct virtqueue *),
	void (*callback)(struct virtqueue *),
	const char *name)
{
	struct virtqueue *vq;
	void *queue = NULL;
	dma_addr_t dma_addr;
	size_t queue_size_in_bytes;
	struct vring vring;
	unsigned int max_desc_num = ZXDH_PF_MAX_DESC_NUM(en_dev);

	/* We assume num is a power of 2. */
	if (num & (num - 1)) {
		LOG_ERR("Bad virtqueue length %u\n", num);
		return NULL;
	}

	/* TODO: allocate each queue chunk individually */
	for (; num && vring_size(max_desc_num, SMP_CACHE_BYTES) > PAGE_SIZE; max_desc_num /= 2) {
		queue = vring_alloc_queue(en_dev, vring_size(max_desc_num, SMP_CACHE_BYTES),
					  &dma_addr,
					  GFP_KERNEL|__GFP_NOWARN|__GFP_ZERO);
		if (queue)
			break;
	}

	if (!num)
		return NULL;

	if (!queue) {
		/* Try to get a single page. You are my only hope! */
		queue = vring_alloc_queue(en_dev, vring_size(max_desc_num, SMP_CACHE_BYTES),
					  &dma_addr, GFP_KERNEL|__GFP_ZERO);
	}
	if (!queue)
		return NULL;

	queue_size_in_bytes = vring_size(max_desc_num, SMP_CACHE_BYTES);
	vring_init(&vring, en_dev, num, queue, SMP_CACHE_BYTES);

	vq = __vring_new_virtqueue(index, vring, en_dev, context,
				   notify, callback, name);
	if (!vq) {
		vring_free_queue(en_dev, queue_size_in_bytes, queue,
				 dma_addr);
		return NULL;
	}

	to_vvq(vq)->split.queue_dma_addr = dma_addr;
	to_vvq(vq)->split.queue_size_in_bytes = queue_size_in_bytes;
	to_vvq(vq)->we_own_ring = true;

	return vq;
}

struct virtqueue *vring_create_virtqueue_packed(uint32_t index,
                                                uint32_t num,
                                                struct zxdh_en_device *en_dev,
                                                bool context,
                                                bool (*notify)(struct virtqueue *),
                                                void (*callback)(struct virtqueue *),
                                                const char *name)
{
    struct vring_virtqueue *vq = NULL;
    struct vring_packed_desc *ring = NULL;
    struct vring_packed_desc_event *driver = NULL;
    struct vring_packed_desc_event *device = NULL;
    dma_addr_t ring_dma_addr;
    dma_addr_t driver_event_dma_addr;
    dma_addr_t device_event_dma_addr;
    size_t ring_size_in_bytes;
    size_t event_size_in_bytes;

    ring_size_in_bytes = ZXDH_PF_MAX_DESC_NUM(en_dev) * sizeof(struct vring_packed_desc) + ZXDH_DESC_EXTRA_SIZE;
    ring = vring_alloc_queue(en_dev, ring_size_in_bytes, &ring_dma_addr, GFP_KERNEL|__GFP_NOWARN|__GFP_ZERO);
    if (unlikely(ring == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "ring vring_alloc_queue failed\n");
        goto err_ring;
    }

    event_size_in_bytes = sizeof(struct vring_packed_desc_event);

    driver = vring_alloc_queue(en_dev, event_size_in_bytes, &driver_event_dma_addr, GFP_KERNEL|__GFP_NOWARN|__GFP_ZERO);
    if (unlikely(driver == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "driver vring_alloc_queue failed\n");
        goto err_driver;
    }

    device = vring_alloc_queue(en_dev, event_size_in_bytes, &device_event_dma_addr, GFP_KERNEL|__GFP_NOWARN|__GFP_ZERO);
    if (unlikely(device == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "device vring_alloc_queue failed\n");
        goto err_device;
    }

    vq = kmalloc(sizeof(*vq), GFP_KERNEL);
    if (unlikely(vq == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "vq kmalloc failed\n");
        goto err_vq;
    }
    memset(vq, 0, sizeof(struct vring_virtqueue));

    vq->vq.callback = callback;
    vq->vq.en_dev = en_dev;
    vq->vq.name = name;
    vq->vq.num_free = num;
    vq->vq.index = index;
    vq->we_own_ring = true;
    vq->notify = notify;
    vq->weak_barriers = true;
    vq->broken = false;
    vq->last_used_idx = 0 | (1 << VRING_PACKED_EVENT_F_WRAP_CTR);
    vq->event_triggered = false;
    vq->num_added = 0;
    vq->packed_ring = true;
    vq->use_dma_api = vring_use_dma_api(en_dev);
#ifdef DEBUG
    vq->in_use = false;
    vq->last_add_time_valid = false;
#endif

    vq->indirect = zxdh_has_feature(en_dev, ZXDH_RING_F_INDIRECT_DESC) && !context;
    vq->event = zxdh_has_feature(en_dev, ZXDH_RING_F_EVENT_IDX);

    if (zxdh_has_feature(en_dev, ZXDH_F_ORDER_PLATFORM))
    {
        vq->weak_barriers = false;
    }

    vq->packed.ring_dma_addr = ring_dma_addr;
    vq->packed.driver_event_dma_addr = driver_event_dma_addr;
    vq->packed.device_event_dma_addr = device_event_dma_addr;

    vq->packed.ring_size_in_bytes = ring_size_in_bytes;
    vq->packed.event_size_in_bytes = event_size_in_bytes;

    vq->packed.vring.num = num;
    vq->packed.vring.desc = ring;
    vq->packed.vring.driver = driver;
    vq->packed.vring.device = device;

    vq->packed.next_avail_idx = 0;
    vq->packed.avail_wrap_counter = 1;
    vq->packed.event_flags_shadow = 0;
    vq->packed.avail_used_flags = 1 << VRING_PACKED_DESC_F_AVAIL;

    vq->packed.desc_state = kmalloc_array(ZXDH_PF_MAX_DESC_NUM(en_dev), sizeof(struct vring_desc_state_packed), GFP_KERNEL);
    if (unlikely(vq->packed.desc_state == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "vq->packed.desc_state kmalloc_array failed\n");
        goto err_desc_state;
    }

    memset(vq->packed.desc_state, 0, ZXDH_PF_MAX_DESC_NUM(en_dev) * sizeof(struct vring_desc_state_packed));

    /* Put everything in free lists. */
    vq->free_head = 0;

    vq->packed.desc_extra = vring_alloc_desc_extra(en_dev, ZXDH_PF_MAX_DESC_NUM(en_dev), num);
    if (unlikely(vq->packed.desc_extra == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "vq->packed.desc_extra vring_alloc_desc_extra failed\n");
        goto err_desc_extra;
    }
    if (index % 2 == 0) {
        if (dh_rx_alloc_page_cache(&vq->vq, ilog2(num)) < 0)
        {
            LOG_ERR_DEV(en_dev->parent, "alloc rxq id %u page cache failed\n", index / 2);
            goto err_page_cache;
        }
    }

    /* No callback? Tell other side not to bother us. */
    if (!callback)
    {
        vq->packed.event_flags_shadow = VRING_PACKED_EVENT_FLAG_DISABLE;
        vq->packed.vring.driver->flags = cpu_to_le16(vq->packed.event_flags_shadow);
    }

    spin_lock(&en_dev->vqs_list_lock);
    list_add_tail(&vq->vq.list, &en_dev->vqs_list);
    spin_unlock(&en_dev->vqs_list_lock);

    return &vq->vq;

err_page_cache:
    if (vq->packed.desc_extra[0].di)
        kfree(vq->packed.desc_extra[0].di);
    kfree(vq->packed.desc_extra);
    vq->packed.desc_extra = NULL;
err_desc_extra:
    kfree(vq->packed.desc_state);
    vq->packed.desc_state = NULL;
err_desc_state:
    kfree(vq);
    vq = NULL;
err_vq:
    vring_free_queue(en_dev, event_size_in_bytes, device, device_event_dma_addr);
err_device:
    vring_free_queue(en_dev, event_size_in_bytes, driver, driver_event_dma_addr);
err_driver:
    vring_free_queue(en_dev, ring_size_in_bytes, ring, ring_dma_addr);
err_ring:
    return NULL;
}

void zxdh_en_xmit_pkts(struct virtqueue *tvq);
void zxdh_vvq_reset(struct zxdh_en_device *en_dev)
{
    struct virtqueue *vq = NULL;
    struct vring_virtqueue *vvq = NULL;
    struct dh_dma_info *dh_dma = NULL;
    uint16_t num;
    int32_t i;
    int32_t j;

    for (i = 0; i < 2 * en_dev->max_queue_pairs; ++i) {
        vq = en_dev->vqs[i]->vq;
        vvq = to_vvq(vq);

        if (i % 2 == 0)
            num = en_dev->eth_config.rx_queue_size;
        else {
            num = en_dev->eth_config.tx_queue_size;
            vq->callback = zxdh_en_xmit_pkts;
        }

        if (en_dev->packed_status) {
            vq->num_free = num;
            vvq->last_used_idx = 0 | (1 << VRING_PACKED_EVENT_F_WRAP_CTR);
            vvq->num_added = 0;
            vvq->packed.vring.num = num;
            vvq->packed.next_avail_idx = 0;
            vvq->packed.avail_wrap_counter = 1;
            vvq->packed.avail_used_flags = 1 << VRING_PACKED_DESC_F_AVAIL;
            vvq->free_head = 0;
            dh_dma = vvq->packed.desc_extra[0].di;

            memset(vvq->packed.desc_state, 0, ZXDH_PF_MAX_DESC_NUM(en_dev) * sizeof(struct vring_desc_state_packed));
            memset(vvq->packed.vring.desc, 0, ZXDH_PF_MAX_DESC_NUM(en_dev) * sizeof(struct vring_packed_desc) + ZXDH_DESC_EXTRA_SIZE);
            memset(dh_dma, 0, ZXDH_PF_MAX_DESC_NUM(en_dev) * sizeof(struct dh_dma_info));
            memset(vvq->packed.desc_extra, 0, ZXDH_PF_MAX_DESC_NUM(en_dev) * sizeof(struct vring_desc_extra));

            vvq->packed.desc_extra[0].di = &dh_dma[0];
            vring_set_desc_extra_info(vvq->packed.desc_extra, num);

            if (i % 2 == 0) {
                if (dh_rx_alloc_page_cache(&vvq->vq, ilog2(num)) < 0)
                    LOG_INFO_DEV(en_dev->parent, "alloc page cache failed, rx qid %d not use page pool", i / 2);
            }
        }
        else {
            vq->num_free = num;
            vvq->last_used_idx = 0;
            vvq->num_added = 0;
            vvq->split.vring.num = num;
            vvq->split.avail_flags_shadow = 0;
            vvq->split.avail_idx_shadow = 0;
            vvq->free_head = 0;

            zte_memset_s(vvq->split.desc_state, 0, ZXDH_PF_MAX_DESC_NUM(en_dev) * sizeof(struct vring_desc_state_split));
            zte_memset_s(vvq->split.vring.desc, 0, vring_size(ZXDH_PF_MAX_DESC_NUM(en_dev), SMP_CACHE_BYTES));
            zte_memset_s(vvq->split.desc_extra, 0, ZXDH_PF_MAX_DESC_NUM(en_dev) * sizeof(struct vring_desc_extra));

            for (j = 0; j < ZXDH_PF_MAX_DESC_NUM(en_dev) - 1; ++j)
            {
                vvq->split.desc_extra[j].next = j + 1;
            }
        }

        en_dev->ops->set_queue_size(en_dev->parent, en_dev->phy_index[i], en_dev->logic_index_split[i], num);
        en_dev->ops->set_queue_enable(en_dev->parent, en_dev->phy_index[i], en_dev->logic_index_split[i], true);
    }
}

struct virtqueue *vring_create_virtqueue(uint32_t index,
                                         uint32_t num,
                                         struct zxdh_en_device *en_dev,
                                         bool context,
                                         bool (*notify)(struct virtqueue *),
                                         void (*callback)(struct virtqueue *),
                                         const char *name)
{
    if (en_dev->packed_status)
    {
        return vring_create_virtqueue_packed(index, num, en_dev, context, notify, callback, name);
    }

    return vring_create_virtqueue_split(index, num, en_dev, context, notify, callback, name);
}

/* the notify function used when creating a virt queue */
bool vp_notify(struct virtqueue *vq)
{
    struct zxdh_en_device *en_dev = vq->en_dev;
    /* we write the queue's selector into the notification register to
     * signal the other end */
    if (en_dev->packed_status)
    {
        iowrite16(vq->phy_index, (void __iomem *)vq->priv);
    }
    else
    {
        iowrite16(en_dev->logic_index_split[vq->index], (void __iomem *)vq->priv);
    }

    return true;
}

struct virtqueue *vp_setup_vq(struct net_device *netdev, unsigned index,
                              void (*callback)(struct virtqueue *vq),
                              const char *name, bool ctx, uint16_t channel_num)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_pci_vq_info *info = kmalloc(sizeof *info, GFP_KERNEL);
    struct virtqueue *vq = NULL;
    struct virtqueue *n = NULL;
    unsigned long flags;
    uint16_t num = 0;
    uint16_t alloc_channel_num = 0;
    int32_t err = 0;
    struct dh_vq_handler vq_handler;

    /* fill out our structure that represents an active queue */
    if (unlikely(info == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "info kmalloc failed\n");
        return ERR_PTR(-ENOMEM);
    }

    if (index % 2 == 0)
        num = en_dev->eth_config.rx_queue_size;
    else
        num = en_dev->eth_config.tx_queue_size;

    /* create the vring */
    vq = vring_create_virtqueue(index, num, en_dev, ctx, vp_notify, callback, name);
    if (vq == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "create the vring failed\n");
        err = -ENOMEM;
        goto out_info;
    }

    /* activate the queue */
    en_dev->ops->activate_phy_vq(en_dev->parent, en_dev->phy_index[index], en_dev->logic_index_split[index], virtqueue_get_vring_size(vq),
                                 virtqueue_get_desc_addr(vq), virtqueue_get_avail_addr(vq), virtqueue_get_used_addr(vq));

    vq->priv = (void __force *)en_dev->ops->vp_modern_map_vq_notify(en_dev->parent, en_dev->phy_index[index], en_dev->logic_index_split[index], NULL);
    if (!vq->priv)
    {
        LOG_ERR_DEV(en_dev->parent, "vp_modern_map_vq_notify failed\n");
        err = -ENOMEM;
        goto err_map_notify;
    }

    vq->phy_index = en_dev->phy_index[index];
    vq->index = index;
    info->channel_num = channel_num;

    memset(&vq_handler, 0, sizeof(struct dh_vq_handler));
    vq_handler.callback = dh_eq_vqs_vring_int;
    alloc_channel_num = en_dev->ops->get_channels_num(en_dev->parent);
    if (channel_num < alloc_channel_num && index % 2 == 0)
    {
        err = en_dev->ops->vqs_channel_bind_handler(en_dev->parent, channel_num, &vq_handler);
        if (err < 0)
        {
            LOG_ERR_DEV(en_dev->parent, "vqs_channel_bind_handler failed: %d\n", err);
            goto err_vqs_channel_bind_handler;
        }
    }

    if (channel_num >= alloc_channel_num)
    {
        if (alloc_channel_num == 0)
        {
            channel_num = 0;
        }
        else
        {
            channel_num = (channel_num - alloc_channel_num) % alloc_channel_num;
        }
    }
    err = en_dev->ops->vq_bind_channel(en_dev->parent, channel_num, en_dev->phy_index[index], en_dev->logic_index_split[index], index);
    if (err < 0)
    {
        LOG_ERR_DEV(en_dev->parent, "vq_bind_channel failed: %d\n", err);
        goto err_vq_bind_channel;
    }

    if (callback)
    {
        spin_lock_irqsave(&en_dev->lock, flags);
        err = en_dev->ops->vqs_bind_eqs(en_dev->parent, channel_num, &info->node);
        spin_unlock_irqrestore(&en_dev->lock, flags);
        if (err < 0)
        {
            LOG_ERR_DEV(en_dev->parent, "vqs_bind_eqs failed: %d\n", err);
            goto err_vqs_bind_eqs;
        }
    }
    else
    {
        INIT_LIST_HEAD(&info->node);
    }

    info->vq = vq;
    en_dev->vqs[index] = info;
    return vq;

err_vqs_bind_eqs:
    list_for_each_entry_safe(vq, n, &en_dev->vqs_list, list)
    {
        en_dev->ops->vq_unbind_channel(en_dev->parent, vq->phy_index, en_dev->logic_index_split[vq->index]);
    }
err_vq_bind_channel:
    if (channel_num < alloc_channel_num)
    {
        en_dev->ops->vqs_channel_unbind_handler(en_dev->parent, channel_num);
    }
err_vqs_channel_bind_handler:
    en_dev->ops->vp_modern_unmap_vq_notify(en_dev->parent, (void __iomem __force *)vq->priv);
err_map_notify:
    vring_del_virtqueue(vq);
out_info:
    kfree(info);
    en_dev->vqs[index] = NULL;
    return ERR_PTR(err);
}

uint32_t get_mergeable_buf_len(struct receive_queue *rq, struct ewma_pkt_len *avg_pkt_len, uint32_t room)
{
    struct zxdh_en_device *en_dev = rq->vq->en_dev;
    size_t hdr_len = 0;
    uint32_t len = 0;
    bool packed_status = en_dev->packed_status;

    if (packed_status)
    {
        hdr_len = sizeof(struct zxdh_net_hdr_rx);
        if (en_dev->dtp_drs_offload == false)
        {
            hdr_len = sizeof(struct zxdh_net_hdr_rx) - sizeof(struct pi_hdr);
        }
    }
    else
    {
        hdr_len = sizeof(struct zxdh_split_hdr_rx);
    }

    if (room)
    {
        return PAGE_SIZE - room;
    }

    len = hdr_len + DH_BUFF_LEN;

    return ALIGN(len, L1_CACHE_BYTES);
}

/* Map one sg entry. */
dma_addr_t vring_map_one_sg(const struct vring_virtqueue *vq, struct scatterlist *sg, enum dma_data_direction direction)
{
    if (!vq->use_dma_api)
    {
        return (dma_addr_t)sg_phys(sg);
    }

    /*
     * We can't use dma_map_sg, because we don't use scatterlists in
     * the way it expects (we don't guarantee that the scatterlist
     * will exist for the lifetime of the mapping).
     */
    return dma_map_page(vring_dma_dev(vq), sg_page(sg), sg->offset, sg->length, direction);
}

dma_addr_t vring_map_single(const struct vring_virtqueue *vq,
                            void *cpu_addr, size_t size,
                            enum dma_data_direction direction)
{
    if (!vq->use_dma_api)
    {
        return (dma_addr_t)virt_to_phys(cpu_addr);
    }

    return dma_map_single(vring_dma_dev(vq), cpu_addr, size, direction);
}

int32_t vring_mapping_error(const struct vring_virtqueue *vq, dma_addr_t addr)
{
    if (!vq->use_dma_api)
    {
        return 0;
    }

    return dma_mapping_error(vring_dma_dev(vq), addr);
}

/*
 * Packed ring specific functions - *_packed().
 */
void vring_unmap_state_packed(const struct vring_virtqueue *vq, struct vring_desc_extra *state)
{
    uint16_t flags = 0;

    if (!vq->use_dma_api)
    {
        return;
    }

    flags = state->flags;
    if (flags & VRING_DESC_F_INDIRECT)
    {
        dma_unmap_single(vring_dma_dev(vq),
                         state->addr, state->len,
                         (flags & VRING_DESC_F_WRITE) ?
                         DMA_FROM_DEVICE : DMA_TO_DEVICE);
    }
    else
    {
        dma_unmap_page(vring_dma_dev(vq),
                       state->addr, state->len,
                       (flags & VRING_DESC_F_WRITE) ?
                       DMA_FROM_DEVICE : DMA_TO_DEVICE);
    }
}

void vring_unmap_desc_packed(const struct vring_virtqueue *vq, struct vring_packed_desc *desc)
{
    uint16_t flags = 0;

    if (!vq->use_dma_api)
    {
        return;
    }

    flags = le16_to_cpu(desc->flags);

    if (flags & VRING_DESC_F_INDIRECT)
    {
        dma_unmap_single(vring_dma_dev(vq),
                         le64_to_cpu(desc->addr),
                         le32_to_cpu(desc->len),
                         (flags & VRING_DESC_F_WRITE) ?
                         DMA_FROM_DEVICE : DMA_TO_DEVICE);
    }
    else
    {
        dma_unmap_page(vring_dma_dev(vq),
                       le64_to_cpu(desc->addr),
                       le32_to_cpu(desc->len),
                       (flags & VRING_DESC_F_WRITE) ?
                       DMA_FROM_DEVICE : DMA_TO_DEVICE);
    }
}

void *mergeable_len_to_ctx(uint32_t truesize, uint32_t headroom)
{
    return (void *)(unsigned long)((headroom << MRG_CTX_HEADER_SHIFT) | truesize);
}

inline bool virtqueue_use_indirect(struct virtqueue *_vq, unsigned int total_sg)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    /*
     * If the host supports indirect descriptor tables, and we have multiple
     * buffers, then go indirect. FIXME: tune this threshold
     */
    return (vq->indirect && total_sg > 1 && vq->vq.num_free);
}

struct vring_packed_desc *alloc_indirect_packed(unsigned int total_sg, gfp_t gfp)
{
    struct vring_packed_desc *desc = NULL;

    /*
     * We require lowmem mappings for the descriptors because
     * otherwise virt_to_phys will give us bogus addresses in the
     * virtqueue.
     */
    gfp &= ~__GFP_HIGHMEM;

    desc = kmalloc_array(total_sg, sizeof(struct vring_packed_desc), gfp);

    return desc;
}

int virtqueue_add_indirect_packed(struct vring_virtqueue *vq,
                                  struct scatterlist *sgs[],
                                  unsigned int total_sg,
                                  unsigned int out_sgs,
                                  unsigned int in_sgs,
                                  void *data,
                                  gfp_t gfp)
{
    struct vring_packed_desc *desc = NULL;
    struct scatterlist *sg = NULL;
    uint32_t i = 0;
    uint32_t n = 0;
    uint32_t err_idx = 0;
    uint16_t head = 0;
    uint16_t id = 0;
    dma_addr_t addr;

    head = vq->packed.next_avail_idx;
    desc = alloc_indirect_packed(total_sg, gfp);
    if (desc == NULL)
    {
        LOG_ERR("desc alloc_indirect_packed failed\n");
        return -ENOMEM;
    }

    if (unlikely(vq->vq.num_free < 1))
    {
        kfree(desc);
        END_USE(vq);
        return -ENOSPC;
    }

    i = 0;
    id = vq->free_head;
    BUG_ON(id == vq->packed.vring.num);

    for (n = 0; n < out_sgs + in_sgs; n++)
    {
        for (sg = sgs[n]; sg; sg = sg_next(sg))
        {
            addr = vring_map_one_sg(vq, sg, n < out_sgs ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
            if (vring_mapping_error(vq, addr))
            {
                LOG_ERR("vring_map_one_sg error\n");
                goto unmap_release;
            }

            desc[i].flags = cpu_to_le16(n < out_sgs ? 0 : VRING_DESC_F_WRITE);
            desc[i].addr = cpu_to_le64(addr);
            desc[i].len = cpu_to_le32(sg->length);
            i++;
        }
    }

    /* Now that the indirect table is filled in, map it. */
    addr = vring_map_single(vq, desc, total_sg * sizeof(struct vring_packed_desc), DMA_TO_DEVICE);
    if (vring_mapping_error(vq, addr))
    {
        LOG_ERR("vring_map_single error\n");
        goto unmap_release;
    }

    vq->packed.vring.desc[head].addr = cpu_to_le64(addr);
    vq->packed.vring.desc[head].len = cpu_to_le32(total_sg * sizeof(struct vring_packed_desc));
    vq->packed.vring.desc[head].id = cpu_to_le16(id);

    if (vq->use_dma_api)
    {
        vq->packed.desc_extra[id].addr = addr;
        vq->packed.desc_extra[id].len = total_sg * sizeof(struct vring_packed_desc);
        vq->packed.desc_extra[id].flags = VRING_DESC_F_INDIRECT | vq->packed.avail_used_flags;
    }

    /*
     * A driver MUST NOT make the first descriptor in the list
     * available before all subsequent descriptors comprising
     * the list are made available.
     */
    vqm_wmb(vq->weak_barriers);
    vq->packed.vring.desc[head].flags = cpu_to_le16(VRING_DESC_F_INDIRECT | vq->packed.avail_used_flags);

    /* We're using some buffers from the free list. */
    vq->vq.num_free -= 1;

    /* Update free pointer */
    n = head + 1;
    if (n >= vq->packed.vring.num)
    {
        n = 0;
        vq->packed.avail_wrap_counter ^= 1;
        vq->packed.avail_used_flags ^=
                1 << VRING_PACKED_DESC_F_AVAIL |
                1 << VRING_PACKED_DESC_F_USED;
    }
    vq->packed.next_avail_idx = n;
    vq->free_head = vq->packed.desc_extra[id].next;

    /* Store token and indirect buffer state. */
    vq->packed.desc_state[id].num = 1;
    vq->packed.desc_state[id].data = data;
    vq->packed.desc_state[id].indir_desc = desc;
    vq->packed.desc_state[id].last = id;

    vq->num_added += 1;

    //LOG_DEBUG("added buffer head %i to %p\n", head, vq);
    END_USE(vq);

    return 0;

unmap_release:
    err_idx = i;

    for (i = 0; i < err_idx; i++)
    {
        vring_unmap_desc_packed(vq, &desc[i]);
    }

    kfree(desc);

    END_USE(vq);
    return -ENOMEM;
}

int32_t virtqueue_add_packed_in(struct receive_queue *rq,
                                uint32_t total_sg,
                                uint32_t out_sgs,
                                uint32_t in_sgs,
                                void *ctx,
                                gfp_t gfp)
{
    struct virtqueue *_vq = rq->vq;
    struct vring_virtqueue *vq = to_vvq(_vq);
    struct vring_packed_desc *desc = NULL;
    uint32_t i = 0;
    uint32_t n = 0;
    uint32_t c = 0;
    uint32_t descs_used = 0;
    __le16 head_flags = 0;
    __le16 flags = 0;
    uint16_t head = 0;
    uint16_t id = 0;
    uint16_t prev = 0;
    uint16_t curr = 0;
    uint16_t avail_used_flags = 0;
    int32_t err = 0;

    START_USE(vq);

    BUG_ON(ctx && vq->indirect);

    if (unlikely(vq->broken))
    {
        LOG_ERR("vq->broken\n");
        END_USE(vq);
        return -EIO;
    }

    LAST_ADD_TIME_UPDATE(vq);

    BUG_ON(total_sg == 0);

    if (virtqueue_use_indirect(_vq, total_sg))
    {
        END_USE(vq);
        return -1;
    }

    head = vq->packed.next_avail_idx;
    avail_used_flags = vq->packed.avail_used_flags;

    WARN_ON_ONCE(total_sg > vq->packed.vring.num && !vq->indirect);

    desc = vq->packed.vring.desc;
    i = head;
    descs_used = total_sg;

    if (unlikely(vq->vq.num_free < descs_used))
    {
        END_USE(vq);
        return -ENOSPC;
    }

    id = vq->free_head;
    BUG_ON(id == vq->packed.vring.num);

    curr = id;

    if (0 == vq->packed.desc_extra[curr].offset)
    {
        if (NULL == vq->packed.desc_extra[curr].di->page)
        {
            err = dh_page_alloc_pool(rq, vq->packed.desc_extra[curr].di);
            if (err)
            {
                END_USE(vq);
                return err;
            }
        }
        else
        {
            END_USE(vq);
            return -EPERM;
        }
    }

    for (n = 0; n < out_sgs + in_sgs; n++)
    {
        dma_addr_t addr = vq->packed.desc_extra[curr].di->addr + vq->packed.desc_extra[curr].offset;

        flags = cpu_to_le16(vq->packed.avail_used_flags |
                (++c == total_sg ? 0 : VRING_DESC_F_NEXT) |
                (n < out_sgs ? 0 : VRING_DESC_F_WRITE));

        desc[i].addr = cpu_to_le64(addr);
        desc[i].len = cpu_to_le32(DEFAULT_FRAG_LENGTH);
        desc[i].id = cpu_to_le16(id);

        if (i == head)
        {
            head_flags = flags;
        }
        else
        {
            desc[i].flags = flags;
        }

        if (unlikely(vq->use_dma_api))
        {
            vq->packed.desc_extra[curr].addr = addr;
            vq->packed.desc_extra[curr].len = DEFAULT_FRAG_LENGTH;
            vq->packed.desc_extra[curr].flags = le16_to_cpu(flags);
        }
        prev = curr;
        curr = vq->packed.desc_extra[curr].next;

        if (unlikely(++i >= vq->packed.vring.num))
        {
            i = 0;
            vq->packed.avail_used_flags ^=
                1 << VRING_PACKED_DESC_F_AVAIL |
                1 << VRING_PACKED_DESC_F_USED;
        }
    }

    if (i < head)
    {
        vq->packed.avail_wrap_counter ^= 1;
    }

    /* We're using some buffers from the free list. */
    vq->vq.num_free -= descs_used;

    /* Update free pointer */
    vq->packed.next_avail_idx = i;
    vq->free_head = curr;

    /* Store token. */
    vq->packed.desc_state[id].num = descs_used;
    vq->packed.desc_state[id].data = page_to_virt(vq->packed.desc_extra[id].di->page)
                                   + vq->packed.desc_extra[id].offset;
    vq->packed.desc_state[id].indir_desc = ctx;
    vq->packed.desc_state[id].last = prev;

    /*
     * A driver MUST NOT make the first descriptor in the list
     * available before all subsequent descriptors comprising
     * the list are made available.
     */
    vqm_wmb(vq->weak_barriers);
    vq->packed.vring.desc[head].flags = head_flags;
    vq->num_added += descs_used;

    END_USE(vq);

    return 0;
}

int32_t virtqueue_add_packed(struct virtqueue *_vq,
                             struct scatterlist *sgs[],
                             uint32_t total_sg,
                             uint32_t out_sgs,
                             uint32_t in_sgs,
                             void *data,
                             void *ctx,
                             gfp_t gfp)
{
    struct vring_virtqueue *vq = to_vvq(_vq);
    struct vring_packed_desc *desc = NULL;
    struct scatterlist *sg = NULL;
    uint32_t i = 0;
    uint32_t n = 0;
    uint32_t c = 0;
    uint32_t descs_used = 0;
    uint32_t err_idx = 0;
    __le16 head_flags = 0;
    __le16 flags = 0;
    uint16_t head = 0;
    uint16_t id = 0;
    uint16_t prev = 0;
    uint16_t curr = 0;
    uint16_t avail_used_flags = 0;
    int32_t err = 0;

    START_USE(vq);

    BUG_ON(data == NULL);
    BUG_ON(ctx && vq->indirect);

    if (unlikely(vq->broken))
    {
        LOG_ERR("vq->broken\n");
        END_USE(vq);
        return -EIO;
    }

    LAST_ADD_TIME_UPDATE(vq);

    BUG_ON(total_sg == 0);

    if (virtqueue_use_indirect(_vq, total_sg))
    {
        err = virtqueue_add_indirect_packed(vq, sgs, total_sg, out_sgs, in_sgs, data, gfp);
        if (err != -ENOMEM)
        {
            END_USE(vq);
            return err;
        }
        /* fall back on direct */
    }

    head = vq->packed.next_avail_idx;
    avail_used_flags = vq->packed.avail_used_flags;

    WARN_ON_ONCE(total_sg > vq->packed.vring.num && !vq->indirect);

    desc = vq->packed.vring.desc;
    i = head;
    descs_used = total_sg;

    if (unlikely(vq->vq.num_free < descs_used))
    {
        END_USE(vq);
        return -ENOSPC;
    }

    id = vq->free_head;
    BUG_ON(id == vq->packed.vring.num);

    curr = id;
    c = 0;
    for (n = 0; n < out_sgs + in_sgs; n++)
    {
        for (sg = sgs[n]; sg; sg = sg_next(sg))
        {
            dma_addr_t addr = vring_map_one_sg(vq, sg, n < out_sgs ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
            if (vring_mapping_error(vq, addr))
            {
                LOG_ERR("vring_map_one_sg error\n");
                goto unmap_release;
            }

            flags = cpu_to_le16(vq->packed.avail_used_flags |
                    (++c == total_sg ? 0 : VRING_DESC_F_NEXT) |
                    (n < out_sgs ? 0 : VRING_DESC_F_WRITE));

            desc[i].addr = cpu_to_le64(addr);
            desc[i].len = cpu_to_le32(sg->length);
            desc[i].id = cpu_to_le16(id);

            if (i == head)
            {
                head_flags = flags;
            }
            else
            {
                desc[i].flags = flags;
            }

            if (unlikely(vq->use_dma_api))
            {
                vq->packed.desc_extra[curr].addr = addr;
                vq->packed.desc_extra[curr].len = sg->length;
                vq->packed.desc_extra[curr].flags = le16_to_cpu(flags);
            }
            prev = curr;
            curr = vq->packed.desc_extra[curr].next;

            if ((unlikely(++i >= vq->packed.vring.num)))
            {
                i = 0;
                vq->packed.avail_used_flags ^=
                    1 << VRING_PACKED_DESC_F_AVAIL |
                    1 << VRING_PACKED_DESC_F_USED;
            }
        }
    }

    if (i < head)
    {
        vq->packed.avail_wrap_counter ^= 1;
    }

    /* We're using some buffers from the free list. */
    vq->vq.num_free -= descs_used;

    /* Update free pointer */
    vq->packed.next_avail_idx = i;
    vq->free_head = curr;

    /* Store token. */
    vq->packed.desc_state[id].num = descs_used;
    vq->packed.desc_state[id].data = data;
    vq->packed.desc_state[id].indir_desc = ctx;
    vq->packed.desc_state[id].last = prev;

    /*
     * A driver MUST NOT make the first descriptor in the list
     * available before all subsequent descriptors comprising
     * the list are made available.
     */
    vqm_wmb(vq->weak_barriers);
    vq->packed.vring.desc[head].flags = head_flags;
    vq->num_added += descs_used;

    //LOG_INFO("added buffer head %i to %p\n", head, vq);
    END_USE(vq);

    return 0;

unmap_release:
    err_idx = i;
    i = head;
    curr = vq->free_head;

    vq->packed.avail_used_flags = avail_used_flags;

    for (n = 0; n < total_sg; n++)
    {
        if (i == err_idx)
        {
            break;
        }

        vring_unmap_state_packed(vq, &vq->packed.desc_extra[curr]);
        curr = vq->packed.desc_extra[curr].next;
        i++;
        if (i >= vq->packed.vring.num)
        {
            i = 0;
        }
    }

    END_USE(vq);
    return -EIO;
}

struct vring_desc *alloc_indirect_split(struct virtqueue *_vq, unsigned int total_sg, gfp_t gfp)
{
	struct vring_desc *desc;
	unsigned int i;

	/*
	 * We require lowmem mappings for the descriptors because
	 * otherwise virt_to_phys will give us bogus addresses in the
	 * virtqueue.
	 */
	gfp &= ~__GFP_HIGHMEM;

	desc = kmalloc_array(total_sg, sizeof(struct vring_desc), gfp);
	if (!desc)
		return NULL;

	for (i = 0; i < total_sg; i++)
		desc[i].next = cpu_to_vqm16(_vq->en_dev, i + 1);
	return desc;
}

unsigned int virtqueue_add_desc_split(struct virtqueue *vq,
						    struct vring_desc *desc,
						    unsigned int i,
						    dma_addr_t addr,
						    unsigned int len,
						    u16 flags,
						    bool indirect)
{
	struct vring_virtqueue *vring = to_vvq(vq);
	struct vring_desc_extra *extra = vring->split.desc_extra;
	u16 next;

	desc[i].flags = cpu_to_vqm16(vq->en_dev, flags);
	desc[i].addr = cpu_to_vqm64(vq->en_dev, addr);
	desc[i].len = cpu_to_vqm32(vq->en_dev, len);

	if (!indirect) {
		next = extra[i].next;
		desc[i].next = cpu_to_vqm16(vq->en_dev, next);

		extra[i].addr = addr;
		extra[i].len = len;
		extra[i].flags = flags;
	} else
		next = vqm16_to_cpu(vq->en_dev, desc[i].next);

	return next;
}

bool virtqueue_kick(struct virtqueue *vq)
{
	if (virtqueue_kick_prepare(vq))
		return virtqueue_notify(vq);
	return true;
}

void vring_unmap_one_split_indirect(const struct vring_virtqueue *vq, struct vring_desc *desc)
{
	u16 flags;

	if (!vq->use_dma_api)
		return;

	flags = vqm16_to_cpu(vq->vq.en_dev, desc->flags);

	dma_unmap_page(vring_dma_dev(vq),
		       vqm64_to_cpu(vq->vq.en_dev, desc->addr),
		       vqm32_to_cpu(vq->vq.en_dev, desc->len),
		       (flags & VRING_DESC_F_WRITE) ?
		       DMA_FROM_DEVICE : DMA_TO_DEVICE);
}

unsigned int vring_unmap_one_split(const struct vring_virtqueue *vq, unsigned int i)
{
	struct vring_desc_extra *extra = vq->split.desc_extra;
	u16 flags;

	if (!vq->use_dma_api)
		goto out;

	flags = extra[i].flags;

	if (flags & VRING_DESC_F_INDIRECT) {
		dma_unmap_single(vring_dma_dev(vq),
				 extra[i].addr,
				 extra[i].len,
				 (flags & VRING_DESC_F_WRITE) ?
				 DMA_FROM_DEVICE : DMA_TO_DEVICE);
	} else {
		dma_unmap_page(vring_dma_dev(vq),
			       extra[i].addr,
			       extra[i].len,
			       (flags & VRING_DESC_F_WRITE) ?
			       DMA_FROM_DEVICE : DMA_TO_DEVICE);
	}

out:
	return extra[i].next;
}

int virtqueue_add_split(struct virtqueue *_vq,
				      struct scatterlist *sgs[],
				      unsigned int total_sg,
				      unsigned int out_sgs,
				      unsigned int in_sgs,
				      void *data,
				      void *ctx,
				      gfp_t gfp)
{
	struct vring_virtqueue *vq = to_vvq(_vq);
	struct scatterlist *sg = NULL;
	struct vring_desc *desc = NULL;
	unsigned int i = 0;
	unsigned int n = 0;
	unsigned int avail = 0;
	unsigned int descs_used = 0;
	unsigned int prev = 0;
	unsigned int err_idx = 0;
	int head = 0;
	bool indirect = false;

	START_USE(vq);

	BUG_ON(data == NULL);
	BUG_ON(ctx && vq->indirect);

	if (unlikely(vq->broken)) {
		END_USE(vq);
		return -EIO;
	}

	LAST_ADD_TIME_UPDATE(vq);

	BUG_ON(total_sg == 0);

	head = vq->free_head;

	if (virtqueue_use_indirect(_vq, total_sg))
		desc = alloc_indirect_split(_vq, total_sg, gfp);
	else {
		desc = NULL;
		WARN_ON_ONCE(total_sg > vq->split.vring.num && !vq->indirect);
	}

	if (desc) {
		/* Use a single buffer which doesn't continue */
		indirect = true;
		/* Set up rest to use this indirect table. */
		i = 0;
		descs_used = 1;
	} else {
		indirect = false;
		desc = vq->split.vring.desc;
		i = head;
		descs_used = total_sg;
	}

	if (unlikely(vq->vq.num_free < descs_used)) {
		/* FIXME: for historical reasons, we force a notify here if
		 * there are outgoing parts to the buffer.  Presumably the
		 * host should service the ring ASAP. */
		if (out_sgs)
			vq->notify(&vq->vq);
		if (indirect)
			kfree(desc);
		END_USE(vq);
		return -ENOSPC;
	}

	for (n = 0; n < out_sgs; n++) {
		for (sg = sgs[n]; sg; sg = sg_next(sg)) {
			dma_addr_t addr = vring_map_one_sg(vq, sg, DMA_TO_DEVICE);
			if (vring_mapping_error(vq, addr))
				goto unmap_release;

			prev = i;
			/* Note that we trust indirect descriptor
			 * table since it use stream DMA mapping.
			 */
			i = virtqueue_add_desc_split(_vq, desc, i, addr, sg->length,
						     VRING_DESC_F_NEXT,
						     indirect);
		}
	}
	for (; n < (out_sgs + in_sgs); n++) {
		for (sg = sgs[n]; sg; sg = sg_next(sg)) {
			dma_addr_t addr = vring_map_one_sg(vq, sg, DMA_FROM_DEVICE);
			if (vring_mapping_error(vq, addr))
				goto unmap_release;

			prev = i;
			/* Note that we trust indirect descriptor
			 * table since it use stream DMA mapping.
			 */
			i = virtqueue_add_desc_split(_vq, desc, i, addr,
						     sg->length,
						     VRING_DESC_F_NEXT |
						     VRING_DESC_F_WRITE,
						     indirect);
		}
	}
	/* Last one doesn't continue. */
	desc[prev].flags &= cpu_to_vqm16(_vq->en_dev, ~VRING_DESC_F_NEXT);
	if (!indirect && vq->use_dma_api)
		vq->split.desc_extra[prev & (vq->split.vring.num - 1)].flags &=
			~VRING_DESC_F_NEXT;

	if (indirect) {
		/* Now that the indirect table is filled in, map it. */
		dma_addr_t addr = vring_map_single(
			vq, desc, total_sg * sizeof(struct vring_desc),
			DMA_TO_DEVICE);
		if (vring_mapping_error(vq, addr))
			goto unmap_release;

		virtqueue_add_desc_split(_vq, vq->split.vring.desc,
					 head, addr,
					 total_sg * sizeof(struct vring_desc),
					 VRING_DESC_F_INDIRECT,
					 false);
	}

	/* We're using some buffers from the free list. */
	vq->vq.num_free -= descs_used;

	/* Update free pointer */
	if (indirect)
		vq->free_head = vq->split.desc_extra[head].next;
	else
		vq->free_head = i;

	/* Store token and indirect buffer state. */
	vq->split.desc_state[head].data = data;
	if (indirect)
		vq->split.desc_state[head].indir_desc = desc;
	else
		vq->split.desc_state[head].indir_desc = ctx;

	/* Put entry in available array (but don't update avail->idx until they
	 * do sync). */
	avail = vq->split.avail_idx_shadow & (vq->split.vring.num - 1);
	vq->split.vring.avail->ring[avail] = cpu_to_vqm16(_vq->en_dev, head);

	/* Descriptors and available array need to be set before we expose the
	 * new available array entries. */
	vqm_wmb(vq->weak_barriers);
	vq->split.avail_idx_shadow++;
	vq->split.vring.avail->idx = cpu_to_vqm16(_vq->en_dev,
						vq->split.avail_idx_shadow);
	vq->num_added++;

	END_USE(vq);

	/* This is very unlikely, but theoretically possible.  Kick
	 * just in case. */
	if (unlikely(vq->num_added == (1 << 16) - 1))
		virtqueue_kick(_vq);

	return 0;

unmap_release:
	err_idx = i;

	if (indirect)
		i = 0;
	else
		i = head;

	for (n = 0; n < total_sg; n++) {
		if (i == err_idx)
			break;
		if (indirect) {
			vring_unmap_one_split_indirect(vq, &desc[i]);
			i = vqm16_to_cpu(_vq->en_dev, desc[i].next);
		} else
			i = vring_unmap_one_split(vq, i);
	}

	if (indirect)
		kfree(desc);

	END_USE(vq);
	return -ENOMEM;
}

int32_t virtqueue_add(struct virtqueue *_vq,
                      struct scatterlist *sgs[],
                      unsigned int total_sg,
                      unsigned int out_sgs,
                      unsigned int in_sgs,
                      void *data,
                      void *ctx,
                      gfp_t gfp)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    return vq->packed_ring ? virtqueue_add_packed(_vq, sgs, total_sg, out_sgs, in_sgs, data, ctx, gfp) :
                             virtqueue_add_split(_vq, sgs, total_sg, out_sgs, in_sgs, data, ctx, gfp);
}

/**
 * virtqueue_add_inbuf_ctx - expose input buffers to other end
 * @vq: the struct virtqueue we're talking about.
 * @sg: scatterlist (must be well-formed and terminated!)
 * @num: the number of entries in @sg writable by other side
 * @data: the token identifying the buffer.
 * @ctx: extra context for the token
 * @gfp: how to do memory allocations (if necessary).
 *
 * Caller must ensure we don't call this with other virtqueue operations
 * at the same time (except where noted).
 *
 * Returns zero or a negative error (ie. ENOSPC, ENOMEM, EIO).
 */
int32_t virtqueue_add_inbuf_ctx(struct virtqueue *vq,
                                struct scatterlist *sg, uint32_t num,
                                void *data,
                                void *ctx,
                                gfp_t gfp)
{
    return virtqueue_add(vq, &sg, num, 0, 1, data, ctx, gfp);
}

int32_t virtqueue_add_inbuf_ctx_in(struct receive_queue *rq,
                                   void *ctx,
                                   gfp_t gfp)
{
    return virtqueue_add_packed_in(rq, 1, 0, 1, ctx, gfp);
}

bool is_used_desc_packed(const struct vring_virtqueue *vq, uint16_t idx, bool used_wrap_counter)
{
    bool avail = false;
    bool used = false;
    uint16_t flags = 0;

    flags = le16_to_cpu(vq->packed.vring.desc[idx].flags);
    avail = !!(flags & (1 << VRING_PACKED_DESC_F_AVAIL));
    used = !!(flags & (1 << VRING_PACKED_DESC_F_USED));

    return avail == used && used == used_wrap_counter;
}

bool virtqueue_poll_packed(struct virtqueue *_vq, uint16_t off_wrap)
{
    struct vring_virtqueue *vq = to_vvq(_vq);
    bool wrap_counter = false;
    uint16_t used_idx = 0;

    wrap_counter = off_wrap >> VRING_PACKED_EVENT_F_WRAP_CTR;
    used_idx = off_wrap & ~(1 << VRING_PACKED_EVENT_F_WRAP_CTR);

    return is_used_desc_packed(vq, used_idx, wrap_counter);
}

bool virtqueue_poll_split(struct virtqueue *_vq, unsigned last_used_idx)
{
	struct vring_virtqueue *vq = to_vvq(_vq);

	return (u16)last_used_idx != vqm16_to_cpu(_vq->en_dev,
			vq->split.vring.used->idx);
}

/**
 * virtqueue_poll - query pending used buffers
 * @_vq: the struct virtqueue we're talking about.
 * @last_used_idx: virtqueue state (from call to virtqueue_enable_cb_prepare).
 *
 * Returns "true" if there are pending used buffers in the queue.
 *
 * This does not need to be serialized.
 */
bool virtqueue_poll(struct virtqueue *_vq, unsigned last_used_idx)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    if (unlikely(vq->broken))
    {
        LOG_ERR("vq->broken\n");
        return false;
    }

    vqm_mb(vq->weak_barriers);
    return vq->packed_ring ? virtqueue_poll_packed(_vq, last_used_idx) : virtqueue_poll_split(_vq, last_used_idx);
}

unsigned virtqueue_enable_cb_prepare_packed(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    START_USE(vq);

    /*
     * We optimistically turn back on interrupts, then check if there was
     * more to do.
     */
    if (vq->event)
    {
        vq->packed.vring.driver->off_wrap = cpu_to_le16(vq->last_used_idx);
        /*
         * We need to update event offset and event wrap
         * counter first before updating event flags.
         */
        vqm_wmb(vq->weak_barriers);
    }

    if (vq->packed.event_flags_shadow == VRING_PACKED_EVENT_FLAG_DISABLE)
    {
        vq->packed.event_flags_shadow = vq->event ?
                VRING_PACKED_EVENT_FLAG_DESC :
                VRING_PACKED_EVENT_FLAG_ENABLE;
        vq->packed.vring.driver->flags = cpu_to_le16(vq->packed.event_flags_shadow);
    }

    END_USE(vq);
    return vq->last_used_idx;
}

unsigned virtqueue_enable_cb_prepare_split(struct virtqueue *_vq)
{
	struct vring_virtqueue *vq = to_vvq(_vq);
	u16 last_used_idx;

	START_USE(vq);

	/* We optimistically turn back on interrupts, then check if there was
	 * more to do. */
	/* Depending on the ZXDH_RING_F_EVENT_IDX feature, we need to
	 * either clear the flags bit or point the event index at the next
	 * entry. Always do both to keep code simple. */
	if (vq->split.avail_flags_shadow & VRING_AVAIL_F_NO_INTERRUPT) {
		vq->split.avail_flags_shadow &= ~VRING_AVAIL_F_NO_INTERRUPT;
		if (!vq->event)
			vq->split.vring.avail->flags =
				cpu_to_vqm16(_vq->en_dev,
						vq->split.avail_flags_shadow);
	}
	vring_used_event(&vq->split.vring) = cpu_to_vqm16(_vq->en_dev,
			last_used_idx = vq->last_used_idx);
	END_USE(vq);
	return last_used_idx;
}

int32_t virtqueue_enable_cb_prepare(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    if (vq->event_triggered)
    {
        vq->event_triggered = false;
    }

    return vq->packed_ring ? virtqueue_enable_cb_prepare_packed(_vq) : virtqueue_enable_cb_prepare_split(_vq);
}

bool packed_used_wrap_counter(uint16_t last_used_idx)
{
    return !!(last_used_idx & (1 << VRING_PACKED_EVENT_F_WRAP_CTR));
}

uint16_t packed_last_used(uint16_t last_used_idx)
{
    return last_used_idx & ~(-(1 << VRING_PACKED_EVENT_F_WRAP_CTR));
}

bool more_used_packed(const struct vring_virtqueue *vq)
{
    uint16_t last_used = 0;
    uint16_t last_used_idx = 0;
    bool used_wrap_counter = false;

    last_used_idx = READ_ONCE(vq->last_used_idx);
    last_used = packed_last_used(last_used_idx);
    used_wrap_counter = packed_used_wrap_counter(last_used_idx);

    return is_used_desc_packed(vq, last_used, used_wrap_counter);
}

bool more_used_split(const struct vring_virtqueue *vq)
{
	return vq->last_used_idx != vqm16_to_cpu(vq->vq.en_dev,
			vq->split.vring.used->idx);
}

bool more_used(const struct vring_virtqueue *vq)
{
	return vq->packed_ring ? more_used_packed(vq) : more_used_split(vq);
}

#define MIN_WAIT_COUNT 10
bool is_flow_stopped(struct zxdh_en_device *en_dev)
{
    struct virtqueue *vq = NULL;
    struct vring_virtqueue *vvq = NULL;
    int32_t consecutive_false_count = 0;
    uint16_t last_used = 0;
    uint16_t last_used_idx = 0;
    int32_t i = 0;
    int32_t j = 0;

    for (i = 0; i < 2 * en_dev->max_queue_pairs; ++i) {
        vq = en_dev->vqs[i]->vq;
        vvq = to_vvq(vq);
        j = 0;
        consecutive_false_count = 0;

        for (j = 0; j < 2000; ++j) {
            if (i % 2 == 0) {
                if (more_used(vvq)) {
                    synchronize_net();
                    consecutive_false_count = 0;
                } else {
                    if (++consecutive_false_count >= MIN_WAIT_COUNT) {
                        break;
                    }
                }
            } else {
                vq->callback = NULL;
                return true;
            }
            usleep_range(5, 10);
        }

        if (consecutive_false_count < MIN_WAIT_COUNT) {
            if (en_dev->packed_status) {
                last_used_idx = READ_ONCE(vvq->last_used_idx);
                last_used = packed_last_used(last_used_idx);
            }
            else {
                last_used = (vvq->last_used_idx & (vvq->split.vring.num - 1));
            }
            zxdh_print_vring_info(vq, last_used - 10, 10);
            zxdh_print_vring_info(vq, last_used, 30);
            return false;
        }
    }

    return true;
}

void detach_buf_packed(struct vring_virtqueue *vq, uint32_t id, void **ctx, uint32_t *_id)
{
    struct vring_desc_state_packed *state = NULL;
    struct vring_packed_desc *desc = NULL;
    uint32_t i = 0;
    uint32_t curr = 0;

    state = &vq->packed.desc_state[id];

    /* Clear data ptr. */
    state->data = NULL;

    vq->vq.num_free += state->num;

    if (_id != NULL)
    {
        *_id = id;
    }
    else
    {
        vq->packed.desc_extra[state->last].next = vq->free_head;
        vq->free_head = id;
        if (unlikely(vq->use_dma_api))
        {
            curr = id;
            for (i = 0; i < state->num; i++)
            {
                vring_unmap_state_packed(vq, &vq->packed.desc_extra[curr]);
                curr = vq->packed.desc_extra[curr].next;
            }
        }
    }

    if (vq->indirect)
    {
        uint32_t len;

        /* Free the indirect table, if any, now that it's unmapped. */
        desc = state->indir_desc;
        if (!desc)
        {
            return;
        }

        if (vq->use_dma_api)
        {
            len = vq->packed.desc_extra[id].len;
            for (i = 0; i < len / sizeof(struct vring_packed_desc); i++)
            {
                vring_unmap_desc_packed(vq, &desc[i]);
            }
        }
        kfree(desc);
        state->indir_desc = NULL;
    }
    else if (ctx)
    {
        *ctx = state->indir_desc;
    }
}

void detach_buf_split(struct vring_virtqueue *vq, unsigned int head, void **ctx)
{
	unsigned int i, j;
	__vqm16 nextflag = cpu_to_vqm16(vq->vq.en_dev, VRING_DESC_F_NEXT);

	/* Clear data ptr. */
	vq->split.desc_state[head].data = NULL;

	/* Put back on free list: unmap first-level descriptors and find end */
	i = head;

	while (vq->split.vring.desc[i].flags & nextflag) {
		vring_unmap_one_split(vq, i);
		i = vq->split.desc_extra[i].next;
		vq->vq.num_free++;
	}

	vring_unmap_one_split(vq, i);
	vq->split.desc_extra[i].next = vq->free_head;
	vq->free_head = head;

	/* Plus final descriptor */
	vq->vq.num_free++;

	if (vq->indirect) {
		struct vring_desc *indir_desc =
				vq->split.desc_state[head].indir_desc;
		u32 len;

		/* Free the indirect table, if any, now that it's unmapped. */
		if (!indir_desc)
			return;

		len = vq->split.desc_extra[head].len;

		BUG_ON(!(vq->split.desc_extra[head].flags &
				VRING_DESC_F_INDIRECT));
		BUG_ON(len == 0 || len % sizeof(struct vring_desc));

		for (j = 0; j < len / sizeof(struct vring_desc); j++)
			vring_unmap_one_split_indirect(vq, &indir_desc[j]);

		kfree(indir_desc);
		vq->split.desc_state[head].indir_desc = NULL;
	} else if (ctx) {
		*ctx = vq->split.desc_state[head].indir_desc;
	}
}

void *virtqueue_get_buf_ctx_packed(struct virtqueue *_vq, uint32_t *len, void **ctx, bool is_delay, uint32_t *_id)
{
    struct vring_virtqueue *vq = to_vvq(_vq);
    uint16_t last_used = 0;
    uint16_t id = 0;
    uint16_t last_used_idx = 0;
    bool used_wrap_counter = false;
    void *ret = NULL;

    START_USE(vq);

    if (unlikely(vq->broken))
    {
        END_USE(vq);
        return NULL;
    }

    if (!more_used_packed(vq))
    {
        //LOG_ERR("no more buffers in queue\n");
        END_USE(vq);
        return NULL;
    }

    if (is_delay)
    {
        udelay(1);
    }

    /* Only get used elements after they have been exposed by host. */
    vqm_rmb(vq->weak_barriers);

    last_used_idx = READ_ONCE(vq->last_used_idx);
    used_wrap_counter = packed_used_wrap_counter(last_used_idx);
    last_used = packed_last_used(last_used_idx);
    id = le16_to_cpu(vq->packed.vring.desc[last_used].id);
    *len = le32_to_cpu(vq->packed.vring.desc[last_used].len);

    if (unlikely(id >= vq->packed.vring.num))
    {
        zxdh_print_vring_info(_vq, 0, vq->packed.vring.num);
        BAD_RING(vq, "id %u out of range\n", id);
        return NULL;
    }
    if (unlikely(!vq->packed.desc_state[id].data))
    {
        zxdh_print_vring_info(_vq, last_used - 10, 10);
        zxdh_print_vring_info(_vq, last_used, 30);
        BAD_RING(vq, "id %u is not a head!\n", id);
        return NULL;
    }

    /* detach_buf_packed clears data, so grab it now. */
    ret = vq->packed.desc_state[id].data;
    detach_buf_packed(vq, id, ctx, _id);

    last_used += vq->packed.desc_state[id].num;
    if (unlikely(last_used >= vq->packed.vring.num))
    {
        last_used -= vq->packed.vring.num;
        used_wrap_counter ^= 1;
    }

    last_used = (last_used | (used_wrap_counter << VRING_PACKED_EVENT_F_WRAP_CTR));
    WRITE_ONCE(vq->last_used_idx, last_used);

    /*
     * If we expect an interrupt for the next entry, tell host
     * by writing event index and flush out the write before
     * the read in the next get_buf call.
     */
    if (vq->packed.event_flags_shadow == VRING_PACKED_EVENT_FLAG_DESC)
        vqm_store_mb(vq->weak_barriers,
                &vq->packed.vring.driver->off_wrap,
                cpu_to_le16(vq->last_used_idx));

    LAST_ADD_TIME_INVALID(vq);

    END_USE(vq);
    return ret;
}

void *virtqueue_get_buf_ctx_split(struct virtqueue *_vq, uint32_t *len, void **ctx, bool is_delay)
{
	struct vring_virtqueue *vq = to_vvq(_vq);
	void *ret;
	unsigned int i;
	u16 last_used;

	START_USE(vq);

	if (unlikely(vq->broken)) {
		END_USE(vq);
		return NULL;
	}

	if (!more_used_split(vq)) {
		END_USE(vq);
		return NULL;
	}

	if (is_delay)
	{
		udelay(1);
	}

	/* Only get used array entries after they have been exposed by host. */
	vqm_rmb(vq->weak_barriers);

	last_used = (vq->last_used_idx & (vq->split.vring.num - 1));
	i = vqm32_to_cpu(_vq->en_dev, vq->split.vring.used->ring[last_used].id);
	*len = vqm32_to_cpu(_vq->en_dev, vq->split.vring.used->ring[last_used].len);

	if (unlikely(i >= vq->split.vring.num)) {
		BAD_RING(vq, "id %u out of range\n", i);
		return NULL;
	}
	if (unlikely(!vq->split.desc_state[i].data)) {
		BAD_RING(vq, "id %u is not a head!\n", i);
		return NULL;
	}

	/* detach_buf_split clears data, so grab it now. */
	ret = vq->split.desc_state[i].data;
	detach_buf_split(vq, i, ctx);
	vq->last_used_idx++;
	/* If we expect an interrupt for the next entry, tell host
	 * by writing event index and flush out the write before
	 * the read in the next get_buf call. */
	if (!(vq->split.avail_flags_shadow & VRING_AVAIL_F_NO_INTERRUPT))
		vqm_store_mb(vq->weak_barriers,
				&vring_used_event(&vq->split.vring),
				cpu_to_vqm16(_vq->en_dev, vq->last_used_idx));

	LAST_ADD_TIME_INVALID(vq);

	END_USE(vq);
	return ret;
}

void *virtqueue_get_buf_ctx(struct virtqueue *_vq, uint32_t *len, void **ctx, bool is_delay, uint32_t *_id)
{
	struct vring_virtqueue *vq = to_vvq(_vq);

	return vq->packed_ring ? virtqueue_get_buf_ctx_packed(_vq, len, ctx, is_delay, _id) :
				 virtqueue_get_buf_ctx_split(_vq, len, ctx, is_delay);
}

/*
 * private is used to chain pages for big packets, put the whole
 * most recent used list in the beginning for reuse
 */
void give_pages(struct receive_queue *rq, struct page *page)
{
    struct page *end = NULL;

    /* Find end of list, sew whole thing into vi->rq.pages. */
    for (end = page; end->private; end = (struct page *)end->private);
    end->private = (unsigned long)rq->pages;
    rq->pages = page;
}

void free_old_xmit_skbs(struct net_device *netdev, struct send_queue *sq, bool in_napi)
{
    uint32_t len = 0;
    uint32_t packets = 0;
    uint32_t bytes = 0;
    void *ptr = NULL;

    while ((ptr = virtqueue_get_buf_ctx(sq->vq, &len, NULL, FALSE, NULL)) != NULL)
    {
#ifdef CGS_V5_693
        struct sk_buff *skb = ptr;

        //LOG_DEBUG("sent skb %p\n", skb);

        bytes += skb->len;
        napi_consume_skb(skb, in_napi);
        packets++;
#else
        if (likely(!is_xdp_frame(ptr))) {
            struct sk_buff *skb = ptr;

            bytes += skb->len;
            napi_consume_skb(skb, in_napi);
        } else {
            struct xdp_frame *frame = ptr_to_xdp(ptr);

            bytes += frame->len;
            xdp_return_frame(frame);
        }
        packets++;
#endif
    }

    /* Avoid overhead when no packets have been processed
     * happens when called speculatively from start_xmit.
     */
    if (!packets)
    {
        return;
    }

    u64_stats_update_begin(&sq->stats.syncp);
    sq->stats.bytes += bytes;
    sq->stats.packets += packets;
    u64_stats_update_end(&sq->stats.syncp);
}

void virtqueue_disable_cb_packed(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    if (vq->packed.event_flags_shadow != VRING_PACKED_EVENT_FLAG_DISABLE)
    {
        vq->packed.event_flags_shadow = VRING_PACKED_EVENT_FLAG_DISABLE;
        vq->packed.vring.driver->flags = cpu_to_le16(vq->packed.event_flags_shadow);
    }
}

void virtqueue_disable_cb_split(struct virtqueue *_vq)
{
	struct vring_virtqueue *vq = to_vvq(_vq);

	if (!(vq->split.avail_flags_shadow & VRING_AVAIL_F_NO_INTERRUPT)) {
		vq->split.avail_flags_shadow |= VRING_AVAIL_F_NO_INTERRUPT;
		if (vq->event)
			/* TODO: this is a hack. Figure out a cleaner value to write. */
			vring_used_event(&vq->split.vring) = 0x0;
		else
			vq->split.vring.avail->flags = cpu_to_vqm16(_vq->en_dev, vq->split.avail_flags_shadow);
	}
}

/**
 * virtqueue_disable_cb - disable callbacks
 * @_vq: the struct virtqueue we're talking about.
 *
 * Note that this is not necessarily synchronous, hence unreliable and only
 * useful as an optimization.
 *
 * Unlike other operations, this need not be serialized.
 */
void virtqueue_disable_cb(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    /* If device triggered an event already it won't trigger one again:
     * no need to disable.
     */
    if (vq->event_triggered)
    {
        return;
    }

    if (vq->packed_ring)
    {
        virtqueue_disable_cb_packed(_vq);
    }
    else
    {
        virtqueue_disable_cb_split(_vq);
    }
}

void virtqueue_napi_schedule(struct napi_struct *napi, struct virtqueue *vq)
{
    if (napi_schedule_prep(napi))
    {
        virtqueue_disable_cb(vq);
        __napi_schedule(napi);
    }
}

void virtnet_napi_enable(struct virtqueue *vq, struct napi_struct *napi)
{
    napi_enable(napi);

    /* If all buffers were filled by other side before we napi_enabled, we
     * won't get another interrupt, so process any outstanding packets now.
     * Call local_bh_enable after to trigger softIRQ processing.
     */
    local_bh_disable();
    virtqueue_napi_schedule(napi, vq);
    local_bh_enable();
}

void virtnet_napi_tx_enable(struct net_device *netdev, struct virtqueue *vq, struct napi_struct *napi)
{
    if (!napi->weight)
    {
        return;
    }

    virtnet_napi_enable(vq, napi);

    return;
}

void virtnet_napi_tx_disable(struct napi_struct *napi)
{
    if (napi->weight)
    {
        napi_disable(napi);
    }
}

#ifndef CGS_V5_693
static bool is_xdp_raw_buffer_queue(struct zxdh_en_device *en_dev, int q)
{
    if (q < (en_dev->curr_queue_pairs - en_dev->xdp_queue_pairs))
        return false;
    else if (q < en_dev->eth_config.num_txq)
        return true;
    else
        return false;
}
#endif

int virtnet_poll_tx(struct napi_struct *napi, int budget)
{
    struct send_queue *sq = container_of(napi, struct send_queue, napi);
    struct zxdh_en_device *en_dev = sq->vq->en_dev;
    uint32_t index = vq2txq(sq->vq);
    struct netdev_queue *txq = NULL;
    int32_t opaque = 0;
    bool done = false;

#ifndef CGS_V5_693
    if (unlikely(is_xdp_raw_buffer_queue(en_dev, index))) {
        /* We don't need to enable cb for XDP */
        napi_complete_done(napi, 0);
        return 0;
    }
#endif

    txq = netdev_get_tx_queue(en_dev->netdev, index);
    __netif_tx_lock(txq, raw_smp_processor_id());
    virtqueue_disable_cb(sq->vq);
    free_old_xmit_skbs(en_dev->netdev, sq, true);

    if (sq->vq->num_free >= 2 + MAX_SKB_FRAGS)
    {
        netif_tx_wake_queue(txq);
    }

    opaque = virtqueue_enable_cb_prepare(sq->vq);

    done = napi_complete_done(napi, 0);

    if (!done)
    {
        virtqueue_disable_cb(sq->vq);
    }

    __netif_tx_unlock(txq);

    if (done)
    {
        if (unlikely(virtqueue_poll(sq->vq, opaque)))
        {
            if (napi_schedule_prep(napi))
            {
                __netif_tx_lock(txq, raw_smp_processor_id());
                virtqueue_disable_cb(sq->vq);
                __netif_tx_unlock(txq);
                __napi_schedule(napi);
            }
        }
    }

    return 0;
}

bool virtqueue_enable_cb_delayed_packed(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);
    uint16_t used_idx = 0;
    uint16_t wrap_counter = 0;
    uint16_t last_used_idx = 0;
    uint16_t bufs = 0;

    START_USE(vq);

    /*
     * We optimistically turn back on interrupts, then check if there was
     * more to do.
     */

    if (vq->event)
    {
        /* TODO: tune this threshold */
        bufs = (vq->packed.vring.num - vq->vq.num_free) * 3 / 4;
        last_used_idx = READ_ONCE(vq->last_used_idx);
        wrap_counter = packed_used_wrap_counter(last_used_idx);

        used_idx = packed_last_used(last_used_idx) + bufs;
        if (used_idx >= vq->packed.vring.num)
        {
            used_idx -= vq->packed.vring.num;
            wrap_counter ^= 1;
        }

        vq->packed.vring.driver->off_wrap = cpu_to_le16(used_idx |
            (wrap_counter << VRING_PACKED_EVENT_F_WRAP_CTR));

        /*
         * We need to update event offset and event wrap
         * counter first before updating event flags.
         */
        vqm_wmb(vq->weak_barriers);
    }

    if (vq->packed.event_flags_shadow == VRING_PACKED_EVENT_FLAG_DISABLE)
    {
        vq->packed.event_flags_shadow = vq->event ?
                VRING_PACKED_EVENT_FLAG_DESC :
                VRING_PACKED_EVENT_FLAG_ENABLE;
        vq->packed.vring.driver->flags = cpu_to_le16(vq->packed.event_flags_shadow);
    }

    /*
     * We need to update event suppression structure first
     * before re-checking for more used buffers.
     */
    vqm_mb(vq->weak_barriers);

    last_used_idx = READ_ONCE(vq->last_used_idx);
    wrap_counter = packed_used_wrap_counter(last_used_idx);
    used_idx = packed_last_used(last_used_idx);
    if (is_used_desc_packed(vq, used_idx, wrap_counter))
    {
        END_USE(vq);
        return false;
    }

    END_USE(vq);
    return true;
}

bool virtqueue_enable_cb_delayed_split(struct virtqueue *_vq)
{
	struct vring_virtqueue *vq = to_vvq(_vq);
	u16 bufs;

	START_USE(vq);

	/* We optimistically turn back on interrupts, then check if there was
	 * more to do. */
	/* Depending on the ZXDH_RING_F_USED_EVENT_IDX feature, we need to
	 * either clear the flags bit or point the event index at the next
	 * entry. Always update the event index to keep code simple. */
	if (vq->split.avail_flags_shadow & VRING_AVAIL_F_NO_INTERRUPT) {
		vq->split.avail_flags_shadow &= ~VRING_AVAIL_F_NO_INTERRUPT;
		if (!vq->event)
			vq->split.vring.avail->flags =
				cpu_to_vqm16(_vq->en_dev,
						vq->split.avail_flags_shadow);
	}
	/* TODO: tune this threshold */
	bufs = (u16)(vq->split.avail_idx_shadow - vq->last_used_idx) * 3 / 4;

	vqm_store_mb(vq->weak_barriers,
			&vring_used_event(&vq->split.vring),
			cpu_to_vqm16(_vq->en_dev, vq->last_used_idx + bufs));

	if (unlikely((u16)(vqm16_to_cpu(_vq->en_dev, vq->split.vring.used->idx)
					- vq->last_used_idx) > bufs)) {
		END_USE(vq);
		return false;
	}

	END_USE(vq);
	return true;
}

uint32_t mergeable_ctx_to_headroom(void *mrg_ctx)
{
    return (unsigned long)mrg_ctx >> MRG_CTX_HEADER_SHIFT;
}

uint32_t mergeable_ctx_to_truesize(void *mrg_ctx)
{
    return (unsigned long)mrg_ctx & ((1 << MRG_CTX_HEADER_SHIFT) - 1);
}

/**
 * virtqueue_enable_cb_delayed - restart callbacks after disable_cb.
 * @_vq: the struct virtqueue we're talking about.
 *
 * This re-enables callbacks but hints to the other side to delay
 * interrupts until most of the available buffers have been processed;
 * it returns "false" if there are many pending buffers in the queue,
 * to detect a possible race between the driver checking for more work,
 * and enabling callbacks.
 *
 * Caller must ensure we don't call this with other virtqueue
 * operations at the same time (except where noted).
 */
bool virtqueue_enable_cb_delayed(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    if (vq->event_triggered)
    {
        vq->event_triggered = false;
    }

    return vq->packed_ring ? virtqueue_enable_cb_delayed_packed(_vq) : virtqueue_enable_cb_delayed_split(_vq);
}

void virtnet_poll_cleantx(struct receive_queue *rq)
{
    struct zxdh_en_device *en_dev = rq->vq->en_dev;
    uint32_t index = vq2rxq(rq->vq);
    struct send_queue *sq = &en_dev->sq[index];
    struct netdev_queue *txq = netdev_get_tx_queue(en_dev->netdev, index);

#ifndef CGS_V5_693
    if (!sq->napi.weight || is_xdp_raw_buffer_queue(en_dev, index))
    {
        return;
    }
#endif

    if (__netif_tx_trylock(txq))
    {
        do
        {
            virtqueue_disable_cb(sq->vq);
            free_old_xmit_skbs(en_dev->netdev, sq, true);
        } while (unlikely(!virtqueue_enable_cb_delayed(sq->vq)));

        if (sq->vq->num_free >= 2 + MAX_SKB_FRAGS)
        {
            netif_tx_wake_queue(txq);
        }

        __netif_tx_unlock(txq);
    }
}

/* Called from bottom half context */
struct sk_buff *page_to_skb(struct zxdh_en_device *en_dev,
                            struct receive_queue *rq,
                            struct page *page, uint32_t offset,
                            uint32_t len, uint32_t truesize,
                            uint32_t metasize, uint32_t headroom,
                            uint32_t *_id, uint32_t split_hdr_len)
{
    struct sk_buff *skb = NULL;
    uint32_t copy = 0;
    uint32_t hdr_len = 0;
    struct page *page_to_free = NULL;
    int32_t tailroom = 0;
    int32_t shinfo_size = 0;
    bool packed_status = en_dev->packed_status;
    char *p = NULL;
    char *hdr_p = NULL;
    char *buf = NULL;
    struct vring_virtqueue *vq = to_vvq(rq->vq);

    p = page_address(page) + offset;
    hdr_p = p;

    if (packed_status)
    {
        hdr_len = (((struct zxdh_net_hdr_rx *)hdr_p)->pd_len) * HDR_2B_UNIT;
    }
    else
    {
        hdr_len = split_hdr_len;
    }

    /* If headroom is not 0, there is an offset between the beginning of the
     * data and the allocated space, otherwise the data and the allocated
     * space are aligned.
     *
     * Buffers with headroom use PAGE_SIZE as alloc size, see
     * add_recvbuf_mergeable() + get_mergeable_buf_len()
     */
    truesize = headroom ? PAGE_SIZE : truesize;
    tailroom = truesize - headroom;
    buf = p - headroom;

    len -= hdr_len;
    offset += hdr_len;
    p += hdr_len;
    tailroom -= hdr_len + len;

    shinfo_size = SKB_DATA_ALIGN(sizeof(struct skb_shared_info));

    /* copy small packet so we can reuse these pages */
    if (!NET_IP_ALIGN && len > GOOD_COPY_LEN && tailroom >= shinfo_size)
    {
        skb = build_skb(buf, truesize);
        if (unlikely(!skb))
        {
            LOG_ERR_DEV(en_dev->parent, "build_skb is null\n");
            return NULL;
        }

        skb_reserve(skb, p - buf);
        skb_put(skb, len);

        if (_id)
            vq->packed.desc_extra[*_id].di->refcnt_bias--;
        else
        {
            page = (struct page *)page->private;
            if (page)
            {
                give_pages(rq, page);
            }
        }

        goto ok;
    }

    /* copy small packet so we can reuse these pages for small data */
    skb = napi_alloc_skb(&rq->napi, GOOD_COPY_LEN);
    if (unlikely(!skb))
    {
        LOG_ERR_DEV(en_dev->parent, "napi_alloc_skb is null\n");
        return NULL;
    }

    /* Copy all frame if it fits skb->head */
    if (len <= skb_tailroom(skb))
    {
        copy = len;
    }
    else
    {
        copy = ETH_HLEN + metasize;
    }
    skb_put_data(skb, p, copy);

    len -= copy;
    offset += copy;

    if (len)
    {
        if (_id)
            vq->packed.desc_extra[*_id].di->refcnt_bias--;
        skb_add_rx_frag(skb, 0, page, offset, len, truesize);
    }
    else
    {
        page_to_free = page;
    }

ok:
    if (!packed_status)
    {
        struct zxdh_split_hdr_rx *hdr = NULL;
        hdr = skb_vnet_hdr_rx(skb);
        memcpy(hdr, hdr_p, hdr_len);
    }

    if (page_to_free && _id == NULL)
    {
        put_page(page_to_free);
    }

    if (metasize)
    {
        __skb_pull(skb, metasize);
        skb_metadata_set(skb, metasize);
    }

    return skb;
}

/**
 * virtqueue_add_outbuf - expose output buffers to other end
 * @vq: the struct virtqueue we're talking about.
 * @sg: scatterlist (must be well-formed and terminated!)
 * @num: the number of entries in @sg readable by other side
 * @data: the token identifying the buffer.
 * @gfp: how to do memory allocations (if necessary).
 *
 * Caller must ensure we don't call this with other virtqueue operations
 * at the same time (except where noted).
 *
 * Returns zero or a negative error (ie. ENOSPC, ENOMEM, EIO).
 */
int32_t virtqueue_add_outbuf(struct virtqueue *vq, struct scatterlist *sg, uint32_t num, void *data, gfp_t gfp)
{
    return virtqueue_add(vq, &sg, num, 1, 0, data, NULL, gfp);
}

#ifndef CGS_V5_693
static int __zxdh_en_xdp_xmit_one(struct zxdh_en_device *en_dev,
                                    struct send_queue *sq,
                                    struct xdp_frame *xdpf)
{
    struct zxdh_net_hdr_tx *hdr;
    int err;

    if (unlikely(xdpf->headroom < en_dev->hdr_len))
        return -EOVERFLOW;

    /* Make room for virtqueue hdr (also change xdpf->headroom?) */
    xdpf->data -= en_dev->hdr_len;
    /* Zero header and leave csum up to XDP layers */
    hdr = xdpf->data;
    memset(hdr, 0, en_dev->hdr_len);
    xdpf->len   += en_dev->hdr_len;

    hdr->pd_len = en_dev->hdr_len / HDR_2B_UNIT;

    sg_init_one(sq->sg, xdpf->data, xdpf->len);

    err = virtqueue_add_outbuf(sq->vq, sq->sg, 1, xdp_to_ptr(xdpf), GFP_ATOMIC);
    if (unlikely(err))
        return -ENOSPC; /* Caller handle free/refcnt */

    return 0;
}

/* when vi->curr_queue_pairs > nr_cpu_ids, the txq/sq is only used for xdp tx on
 * the current cpu, so it does not need to be locked.
 *
 * Here we use marco instead of inline functions because we have to deal with
 * three issues at the same time: 1. the choice of sq. 2. judge and execute the
 * lock/unlock of txq 3. make sparse happy. It is difficult for two inline
 * functions to perfectly solve these three problems at the same time.
 */
#define zxdh_en_xdp_get_sq(en_dev) ({                                       \
    int cpu = smp_processor_id();                                   \
    struct netdev_queue *txq;                                       \
    typeof(en_dev) v = (en_dev);                                            \
    unsigned int qp;                                                \
                                    \
    if (v->curr_queue_pairs > nr_cpu_ids) {                         \
        qp = v->curr_queue_pairs - v->xdp_queue_pairs;          \
        qp += cpu;                                              \
        txq = netdev_get_tx_queue(v->netdev, qp);                  \
        __netif_tx_acquire(txq);                                \
    } else {                                                        \
        qp = cpu % v->curr_queue_pairs;                         \
        txq = netdev_get_tx_queue(v->netdev, qp);                  \
        __netif_tx_lock(txq, cpu);                              \
    }                                                               \
    v->sq + qp;                                                     \
})

#define zxdh_en_xdp_put_sq(en_dev, q) {                                     \
    struct netdev_queue *txq;                                       \
    typeof(en_dev) v = (en_dev);                                            \
                                    \
    txq = netdev_get_tx_queue(v->netdev, (q) - v->sq);                 \
    if (v->curr_queue_pairs > nr_cpu_ids)                           \
        __netif_tx_release(txq);                                \
    else                                                            \
        __netif_tx_unlock(txq);                                 \
}

int zxdh_en_xdp_xmit(struct net_device *dev,
                int n, struct xdp_frame **frames, u32 flags)
{
    struct zxdh_en_priv *en_priv = netdev_priv(dev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct receive_queue *rq = en_dev->rq;
    struct bpf_prog *xdp_prog;
    struct send_queue *sq;
    unsigned int len;
    int packets = 0;
    int bytes = 0;
    int nxmit = 0;
    int kicks = 0;
    void *ptr;
    int ret;
    int i;

    /* Only allow ndo_xdp_xmit if XDP is loaded on dev, as this
     * indicate XDP resources have been successfully allocated.
     */
    xdp_prog = rcu_access_pointer(rq->xdp_prog);
    if (!xdp_prog)
        return -ENXIO;

    sq = zxdh_en_xdp_get_sq(en_dev);

    if (unlikely(flags & ~XDP_XMIT_FLAGS_MASK)) {
        ret = -EINVAL;
        goto out;
    }

    /* Free up any pending old buffers before queueing new ones. */
    while ((ptr = virtqueue_get_buf_ctx(sq->vq, &len, NULL, FALSE, NULL)) != NULL) {
        if (likely(is_xdp_frame(ptr))) {
            struct xdp_frame *frame = ptr_to_xdp(ptr);

            bytes += frame->len;
            xdp_return_frame(frame);
        } else {
            struct sk_buff *skb = ptr;

            bytes += skb->len;
            napi_consume_skb(skb, false);
        }
        packets++;
    }

    for (i = 0; i < n; i++) {
        struct xdp_frame *xdpf = frames[i];

        if (__zxdh_en_xdp_xmit_one(en_dev, sq, xdpf))
            break;
        nxmit++;
    }
    ret = nxmit;

    if (flags & XDP_XMIT_FLUSH) {
        if (virtqueue_kick_prepare(sq->vq) && virtqueue_notify(sq->vq))
            kicks = 1;
    }
out:
    u64_stats_update_begin(&sq->stats.syncp);
    sq->stats.bytes += bytes;
    sq->stats.packets += packets;
    sq->stats.xdp_tx += n;
    sq->stats.xdp_tx_drops += n - nxmit;
    sq->stats.kicks += kicks;
    u64_stats_update_end(&sq->stats.syncp);

    zxdh_en_xdp_put_sq(en_dev, sq);
    return ret;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(5,7,0))
static __always_inline void
zxdh_xdp_init_buff(struct xdp_buff *xdp, u32 frame_sz, struct xdp_rxq_info *rxq)
{
    xdp->frame_sz = frame_sz;
    xdp->rxq = rxq;
}

static __always_inline void
zxdh_xdp_prepare_buff(struct xdp_buff *xdp, unsigned char *hard_start,
         int headroom, int data_len, const bool meta_valid)
{
    unsigned char *data = hard_start + headroom;

    xdp->data_hard_start = hard_start;
    xdp->data = data;
    xdp->data_end = data + data_len;
    xdp->data_meta = meta_valid ? data : data + 1;
}
#endif
#endif /* CGS_V5_693 */

struct sk_buff *receive_mergeable_split(struct net_device *netdev,
                                        struct zxdh_en_device *en_dev,
                                        struct receive_queue *rq,
                                        void *buf,
                                        void *ctx,
                                        uint32_t len,
                                        uint32_t split_hdr_len,
                                        struct virtnet_rq_stats *stats)
{
    struct zxdh_split_hdr_rx *hdr = buf;
    uint16_t num_buf = vqm16_to_cpu(en_dev, hdr->split_hdr.num_buffers);
    struct page *page = virt_to_head_page(buf);
    int32_t offset = buf - page_address(page);
    struct sk_buff *head_skb = NULL;
    struct sk_buff *curr_skb = NULL;
    uint32_t truesize = mergeable_ctx_to_truesize(ctx);
    uint32_t headroom = mergeable_ctx_to_headroom(ctx);
    uint32_t metasize = 0;

    stats->bytes += (len - split_hdr_len);

    if (unlikely(len > truesize))
    {
        LOG_ERR_DEV(en_dev->parent, "%s: rx error: len %u exceeds truesize %lu\n", netdev->name, len, (unsigned long)ctx);
        netdev->stats.rx_length_errors++;
        netdev->stats.rx_errors++;
        goto err_skb;
    }

    head_skb = page_to_skb(en_dev, rq, page, offset, len, truesize, metasize, headroom, NULL, split_hdr_len);
    curr_skb = head_skb;

    if (unlikely(!curr_skb))
    {
        LOG_ERR_DEV(en_dev->parent, "page_to_skb is null\n");
        goto err_skb;
    }
    while (--num_buf)
    {
        int32_t num_skb_frags;

        buf = virtqueue_get_buf_ctx(rq->vq, &len, &ctx, FALSE, NULL);
        if (unlikely(!buf))
        {
            LOG_ERR_DEV(en_dev->parent, "%s: rx error: %d buffers out of %d missing\n",
                        netdev->name, num_buf, vqm16_to_cpu(en_dev, hdr->split_hdr.num_buffers));
            netdev->stats.rx_length_errors++;
            netdev->stats.rx_errors++;
            goto err_buf;
        }

        stats->bytes += len;
        page = virt_to_head_page(buf);

        truesize = mergeable_ctx_to_truesize(ctx);
        if (unlikely(len > truesize))
        {
            LOG_ERR_DEV(en_dev->parent, "%s: rx error: len %u exceeds truesize %lu\n", netdev->name, len, (unsigned long)ctx);
            netdev->stats.rx_length_errors++;
            netdev->stats.rx_errors++;
            goto err_skb;
        }

        num_skb_frags = skb_shinfo(curr_skb)->nr_frags;
        if (unlikely(num_skb_frags == MAX_SKB_FRAGS))
        {
            struct sk_buff *nskb = alloc_skb(0, GFP_ATOMIC);

            if (unlikely(!nskb))
            {
                LOG_ERR_DEV(en_dev->parent, "alloc_skb is null\n");
                goto err_skb;
            }
            if (curr_skb == head_skb)
            {
                skb_shinfo(curr_skb)->frag_list = nskb;
            }
            else
            {
                curr_skb->next = nskb;
            }
            curr_skb = nskb;
            head_skb->truesize += nskb->truesize;
            num_skb_frags = 0;
        }

        if (curr_skb != head_skb)
        {
            head_skb->data_len += len;
            head_skb->len += len;
            head_skb->truesize += truesize;
        }
        offset = buf - page_address(page);

        if (skb_can_coalesce(curr_skb, num_skb_frags, page, offset))
        {
            put_page(page);
        #ifndef CGS_V5_693
            skb_coalesce_rx_frag(curr_skb, num_skb_frags - 1, len, truesize);
        #endif
        }
        else
        {
            skb_add_rx_frag(curr_skb, num_skb_frags, page, offset, len, truesize);
        }
    }

    ewma_pkt_len_add(&rq->mrg_avg_pkt_len, head_skb->len);
    return head_skb;

err_skb:
    put_page(page);
    while (num_buf-- > 1)
    {
        buf = virtqueue_get_buf_ctx(rq->vq, &len, NULL, FALSE, NULL);
        if (unlikely(!buf))
        {
            LOG_ERR_DEV(en_dev->parent, "%s: rx error: %d buffers missing\n", netdev->name, num_buf);
            netdev->stats.rx_length_errors++;
            netdev->stats.rx_errors++;
            break;
        }
        stats->bytes += len;
        page = virt_to_head_page(buf);
        put_page(page);
    }
err_buf:
    stats->drops++;
    dev_kfree_skb(head_skb);
    return NULL;
}

struct sk_buff *receive_mergeable(struct net_device *netdev,
                                  struct zxdh_en_device *en_dev,
                                  struct receive_queue *rq,
                                  void *buf,
                                  void *ctx,
                                  uint32_t len,
                                  uint32_t *xdp_xmit,
                                  struct virtnet_rq_stats *stats,
                                  struct zxdh_frag *frag)
{
    struct zxdh_net_hdr_rx *hdr = buf;
    uint16_t num_buf = vqm16_to_cpu(en_dev, hdr->num_buffers);
    uint16_t hdr_len = hdr->pd_len * HDR_2B_UNIT;
    struct page *page = virt_to_head_page(buf);
    int32_t offset = buf - page_address(page);
    struct sk_buff *head_skb = NULL;
    struct sk_buff *curr_skb = NULL;
#ifndef CGS_V5_693
    struct bpf_prog *xdp_prog;
#endif
    uint32_t truesize = mergeable_ctx_to_truesize(ctx);
    uint32_t headroom = mergeable_ctx_to_headroom(ctx);
    uint32_t metasize = 0;
    struct vring_virtqueue *vq = to_vvq(rq->vq);
    uint32_t _id = frag->frag_id[0];
#ifndef CGS_V5_693
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5,7,0))
    uint32_t frame_sz = 0;
#endif
    int32_t err;
#endif

    stats->bytes += (len - hdr_len);

    if (unlikely(len > truesize))
    {
        LOG_ERR_DEV(en_dev->parent, "%s: rx error: len %u exceeds truesize %lu\n", netdev->name, len, (unsigned long)ctx);
        netdev->stats.rx_length_errors++;
        netdev->stats.rx_errors++;
        goto err_skb;
    }

#ifndef CGS_V5_693
    if (likely(!en_dev->xdp_enabled)) {
        xdp_prog = NULL;
        goto skip_xdp;
    }

    rcu_read_lock();
    xdp_prog = rcu_dereference(rq->xdp_prog);
    if (xdp_prog) {
        struct xdp_frame *xdpf;
        struct page *xdp_page;
        struct xdp_buff xdp;
        void *data;
        uint32_t act;

        /* Transient failure which in theory could occur if
         * in-flight packets from before XDP was enabled reach
         * the receive path after XDP is loaded.
         */
//        if (unlikely(hdr->hdr.gso_type)) //ZY TODO
//            goto err_xdp;

        /* Buffers with headroom use PAGE_SIZE as alloc size,
         * see add_recvbuf_mergeable() + get_mergeable_buf_len()
         */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5,7,0))
        frame_sz = headroom ? PAGE_SIZE : truesize;
#endif

        /* This happens when rx buffer size is underestimated
         * or headroom is not enough because of the buffer
         * was refilled before XDP is set. This should only
         * happen for the first several packets, so we don't
         * care much about its performance.
         */
        if (unlikely(num_buf > 1 || headroom < zxdh_en_get_headroom(en_dev))) {
            /* linearize data for XDP */
            xdp_page = xdp_linearize_page(rq, &num_buf,
                              page, offset,
                              ZXDH_XDP_HEADROOM,
                              &len);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5,7,0))
            frame_sz = PAGE_SIZE;
#endif

            if (!xdp_page)
                goto err_xdp;
            offset = ZXDH_XDP_HEADROOM;
        } else {
            xdp_page = page;
        }

        /* Allow consuming headroom but reserve enough space to push
         * the descriptor on if we get an XDP_TX return code.
         */
        data = page_address(xdp_page) + offset;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(5,7,0))
        zxdh_xdp_init_buff(&xdp, frame_sz - hdr_len, &rq->xdp_rxq);
        zxdh_xdp_prepare_buff(&xdp, data - ZXDH_XDP_HEADROOM + hdr_len,
                 ZXDH_XDP_HEADROOM, len - hdr_len, true);
#else
        xdp.data_hard_start = data - ZXDH_XDP_HEADROOM + hdr_len;
        xdp.data = data + hdr_len;
        xdp.data_end = xdp.data + (len - hdr_len);
        xdp.data_meta = xdp.data;
        xdp.rxq = &rq->xdp_rxq;
#endif

        act = bpf_prog_run_xdp(xdp_prog, &xdp);
        stats->xdp_packets++;

        switch (act) {
        case XDP_PASS:
            metasize = xdp.data - xdp.data_meta;

            /* recalculate offset to account for any header
             * adjustments and minus the metasize to copy the
             * metadata in page_to_skb(). Note other cases do not
             * build an skb and avoid using offset
             */
            offset = xdp.data - page_address(xdp_page) - hdr_len - metasize;

            /* recalculate len if xdp.data, xdp.data_end or
             * xdp.data_meta were adjusted
             */
            len = xdp.data_end - xdp.data + hdr_len + metasize;
            /* We can only create skb based on xdp_page. */
            if (unlikely(xdp_page != page)) {
                rcu_read_unlock();
                head_skb = page_to_skb(en_dev, rq, xdp_page, offset,
                               len, PAGE_SIZE, metasize,
                               ZXDH_XDP_HEADROOM, NULL, 0);
                return head_skb;
            }
            break;
        case XDP_TX:
            stats->xdp_tx++;
            xdpf = xdp_convert_buff_to_frame(&xdp);
            if (unlikely(!xdpf))
                goto err_xdp;
            err = zxdh_en_xdp_xmit(netdev, 1, &xdpf, 0);
            if (unlikely(!err)) {
                xdp_return_frame_rx_napi(xdpf);
            } else if (unlikely(err < 0)) {
                trace_xdp_exception(en_dev->netdev, xdp_prog, act);
                if (unlikely(xdp_page != page))
                    put_page(xdp_page);
                goto err_xdp;
            }
            *xdp_xmit |= ZXDH_XDP_TX;
            rcu_read_unlock();
            goto xdp_xmit;
        case XDP_REDIRECT:
            stats->xdp_redirects++;
            err = xdp_do_redirect(netdev, &xdp, xdp_prog);
            if (err) {
                if (unlikely(xdp_page != page))
                    put_page(xdp_page);
                goto err_xdp;
            }
            *xdp_xmit |= ZXDH_XDP_REDIR;
            rcu_read_unlock();
            goto xdp_xmit;
        default:
#if ((RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8, 7)) || \
     (!RHEL_RELEASE_CODE && (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0))))
            bpf_warn_invalid_xdp_action(en_dev->netdev, xdp_prog, act);
#else
            bpf_warn_invalid_xdp_action(act);
#endif
            if (unlikely(xdp_page != page))
                __free_pages(xdp_page, 0);
            goto err_xdp;
        case XDP_ABORTED:
        case XDP_DROP:
            if (unlikely(xdp_page != page))
                __free_pages(xdp_page, 0);
            goto err_xdp;
        }
    }
    rcu_read_unlock();

skip_xdp:
#endif
    head_skb = page_to_skb(en_dev, rq, page, offset, len, truesize, metasize, headroom, &_id, 0);
    curr_skb = head_skb;

    if (unlikely(!curr_skb))
    {
        LOG_ERR_DEV(en_dev->parent, "page_to_skb is null\n");
        goto err_skb;
    }
    while (--num_buf)
    {
        int32_t num_skb_frags;

        buf = virtqueue_get_buf_ctx(rq->vq, &len, &ctx, FALSE, &_id);
        if (unlikely(!buf))
        {
            LOG_ERR_DEV(en_dev->parent, "%s: rx error: %d buffers out of %d missing\n",
                        netdev->name, num_buf, vqm16_to_cpu(en_dev, hdr->num_buffers));
            netdev->stats.rx_length_errors++;
            netdev->stats.rx_errors++;
            goto err_buf;
        }

        if (frag->frag_num < ZXDH_MAX_FRAG_NUM)
        {
            frag->frag_id[frag->frag_num] = _id;
            frag->frag_num++;
        }
        else
        {
            if (vq->packed.desc_extra[_id].last_in_page)
            {
                dh_page_release_dynamic(rq, vq->packed.desc_extra[_id].di, true);
                vq->packed.desc_extra[_id].di->page = NULL;
            }
        }

        stats->bytes += len;
        page = virt_to_head_page(buf);

        truesize = mergeable_ctx_to_truesize(ctx);
        if (unlikely(len > truesize))
        {
            LOG_ERR_DEV(en_dev->parent, "%s: rx error: len %u exceeds truesize %lu\n", netdev->name, len, (unsigned long)ctx);
            netdev->stats.rx_length_errors++;
            netdev->stats.rx_errors++;
            goto err_skb;
        }

        num_skb_frags = skb_shinfo(curr_skb)->nr_frags;
        if (unlikely(num_skb_frags == MAX_SKB_FRAGS))
        {
            struct sk_buff *nskb = alloc_skb(0, GFP_ATOMIC);

            if (unlikely(!nskb))
            {
                LOG_ERR_DEV(en_dev->parent, "alloc_skb is null\n");
                goto err_skb;
            }
            if (curr_skb == head_skb)
            {
                skb_shinfo(curr_skb)->frag_list = nskb;
            }
            else
            {
                curr_skb->next = nskb;
            }
            curr_skb = nskb;
            head_skb->truesize += nskb->truesize;
            num_skb_frags = 0;
        }

        if (curr_skb != head_skb)
        {
            head_skb->data_len += len;
            head_skb->len += len;
            head_skb->truesize += truesize;
        }
        offset = buf - page_address(page);

        vq->packed.desc_extra[_id].di->refcnt_bias--;
        skb_add_rx_frag(curr_skb, num_skb_frags, page, offset, len, truesize);
    }

    return head_skb;

#ifndef CGS_V5_693
err_xdp:
    rcu_read_unlock();
    stats->xdp_drops++;
#endif
err_skb:
    while (num_buf-- > 1)
    {
        buf = virtqueue_get_buf_ctx(rq->vq, &len, NULL, FALSE, &_id);
        if (unlikely(!buf))
        {
            LOG_ERR_DEV(en_dev->parent, "%s: rx error: %d buffers missing\n", netdev->name, num_buf);
            netdev->stats.rx_length_errors++;
            netdev->stats.rx_errors++;
            break;
        }
        stats->bytes += len;
        if (vq->packed.desc_extra[_id].last_in_page)
        {
            dh_page_release_dynamic(rq, vq->packed.desc_extra[_id].di, true);
            vq->packed.desc_extra[_id].di->page = NULL;
        }
    }
err_buf:
    stats->drops++;
    dev_kfree_skb(head_skb);
#ifndef CGS_V5_693
xdp_xmit:
#endif
    return NULL;
}

/* Started by AICoder, pid:n0cb0l30fcn91ca1492c0abc703edf55b7e2b083 */
void zxdh_rx_csum_offload(struct net_device *netdev, struct sk_buff *skb, struct zxdh_net_hdr_rx *hdr_rcv, struct virtnet_rq_stats *stats)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint16_t pkt_tunnel_type = (hdr_rcv->pipd_hdr.pd_hdr.outer_pkt_type & PD_OUTER_TUNNEL_MASK);
    bool is_valid_tunnel_type = (pkt_tunnel_type == PD_PTYPE_TUNNEL_VXLAN \
                              || pkt_tunnel_type == PD_PTYPE_TUNNEL_GENEVE \
                              || pkt_tunnel_type == PD_PTYPE_TUNNEL_GRE \
                              || pkt_tunnel_type == PD_PTYPE_TUNNEL_IPIP);

    skb->ip_summed = CHECKSUM_NONE;

    if (!(netdev->features & NETIF_F_RXCSUM))
        goto csum_none;

    if (pkt_tunnel_type && (!is_valid_tunnel_type || !en_dev->is_outer_l4_rxcsum_offload))
        goto csum_none;

    if ((hdr_rcv->pipd_hdr.pi_hdr.error_code[1]) \
     || (hdr_rcv->pipd_hdr.pi_hdr.error_code[0]) \
     || (hdr_rcv->pipd_hdr.pd_hdr.flags & OUTER_IP_CHKSUM_ERROR_CODE) \
     || (hdr_rcv->pipd_hdr.pd_hdr.flags & OUTER_L4_CHKSUM_ERROR_CODE) \
     || (hdr_rcv->pipd_hdr.pd_hdr.flags & INNER_L3_CHKSUM_ERROR_CODE))
        goto csum_none;

    if (is_valid_tunnel_type)
    {
        skb->ip_summed = CHECKSUM_UNNECESSARY;
        if (!(hdr_rcv->pipd_hdr.pd_hdr.flags & DTP_VERIFY_RX_CHECKSUM_BASED_OUTER) ||
            (hdr_rcv->pipd_hdr.pd_hdr.flags & RX_IPV4_UDP_ZERO_CHECKSUM_FLAG)) {
            skb->csum_level = 1;
            stats->rx_csum_unnecessary_tunnel++;
        } else {
            stats->rx_csum_unnecessary++;
        }

    } else {
        switch (hdr_rcv->pipd_hdr.pi_hdr.pt.type_ctx.pkt_code)
        {
            case PCODE_TCP:
            case PCODE_UDP:
            case PCODE_NO_LRO_TCP:
                skb->ip_summed = CHECKSUM_UNNECESSARY;
                stats->rx_csum_unnecessary++;
                break;
            default:
                en_dev->hw_stats.netdev_stats.rx_csum_none++;
                break;
        }
    }
    return;

csum_none:
    en_dev->hw_stats.netdev_stats.rx_csum_none++;
}
/* Ended by AICoder, pid:n0cb0l30fcn91ca1492c0abc703edf55b7e2b083 */

void pipd_receive_handle(struct net_device *netdev, struct sk_buff *skb, struct zxdh_net_hdr_rx *hdr_rcv,  struct virtnet_rq_stats *stats)
{
    uint16_t cvid = 0;
    uint16_t svid = 0;
    uint16_t vid = 0;
    bool vlan_striped = false;

    /* rx packet contain the strip label & open rxvlan offloading*/
    vlan_striped =  hdr_rcv->pipd_hdr.pd_hdr.flags & RX_VLAN_STRIPED_MASK;
    if ((netdev->features & NETIF_F_HW_VLAN_CTAG_RX) && vlan_striped)
    {
        cvid = htons(hdr_rcv->pipd_hdr.pd_hdr.striped_ctci) & RX_TPID_VLAN_ID_MASK;
        svid = htons(hdr_rcv->pipd_hdr.pd_hdr.striped_stci) & RX_TPID_VLAN_ID_MASK;
        vid = (hdr_rcv->pipd_hdr.pd_hdr.flags & RX_IS_QINQ_PKT_MASK)? svid : cvid;
        __vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), vid);
        stats->rx_removed_vlan_packets++;
    }

    zxdh_rx_csum_offload(netdev, skb, hdr_rcv, stats);
}

void pd_receive_handle(struct net_device *netdev, struct sk_buff *skb, struct pd_net_hdr_rx *pd_hdr,  struct virtnet_rq_stats *stats)
{
    uint16_t cvid = 0;
    uint16_t svid = 0;
    uint16_t vid = 0;
    bool vlan_striped = false;

    /* rx packet contain the strip label & open rxvlan offloading*/
    vlan_striped = pd_hdr->flags & RX_VLAN_STRIPED_MASK;
    if ((netdev->features & NETIF_F_HW_VLAN_CTAG_RX) && vlan_striped)
    {
        cvid = htons(pd_hdr->striped_ctci) & RX_TPID_VLAN_ID_MASK;
        svid = htons(pd_hdr->striped_stci) & RX_TPID_VLAN_ID_MASK;
        vid = (pd_hdr->flags & RX_IS_QINQ_PKT_MASK)? svid : cvid;
        __vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), vid);
        stats->rx_removed_vlan_packets++;
    }
}

bool net_hdr_match_proto_split(__be16 protocol, __u8 gso_type)
{
	switch (gso_type & ~ZXDH_NET_HDR_GSO_ECN) {
	case ZXDH_NET_HDR_GSO_TCPV4:
		return protocol == cpu_to_be16(ETH_P_IP);
	case ZXDH_NET_HDR_GSO_TCPV6:
		return protocol == cpu_to_be16(ETH_P_IPV6);
	case ZXDH_NET_HDR_GSO_UDP:
		return protocol == cpu_to_be16(ETH_P_IP) ||
		       protocol == cpu_to_be16(ETH_P_IPV6);
	default:
		return false;
	}
}

int net_hdr_set_proto_split(struct sk_buff *skb, const struct zxdh_split_net_hdr *hdr)
{
	if (skb->protocol)
		return 0;

	switch (hdr->gso_type & ~ZXDH_NET_HDR_GSO_ECN) {
	case ZXDH_NET_HDR_GSO_TCPV4:
	case ZXDH_NET_HDR_GSO_UDP:
		skb->protocol = cpu_to_be16(ETH_P_IP);
		break;
	case ZXDH_NET_HDR_GSO_TCPV6:
		skb->protocol = cpu_to_be16(ETH_P_IPV6);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#ifdef ZXDH_SKB_FLOW_KEYS_BASIC
int net_hdr_to_skb_split(struct sk_buff *skb, const struct zxdh_split_net_hdr *hdr, bool little_endian)
{
	unsigned int gso_type = 0;
	unsigned int thlen = 0;
	unsigned int p_off = 0;
	unsigned int ip_proto = 0;
	uint16_t start = 0;
	uint16_t off = 0;
	uint32_t needed = 0;
	__be16 protocol = 0;
	uint16_t gso_size = 0;
	unsigned int nh_off = 0;
	struct flow_keys_basic keys;
	struct skb_shared_info *shinfo = NULL;


	if (hdr->gso_type != ZXDH_NET_HDR_GSO_NONE) {
		switch (hdr->gso_type & ~ZXDH_NET_HDR_GSO_ECN) {
		case ZXDH_NET_HDR_GSO_TCPV4:
			gso_type = SKB_GSO_TCPV4;
			ip_proto = IPPROTO_TCP;
			thlen = sizeof(struct tcphdr);
			break;
		case ZXDH_NET_HDR_GSO_TCPV6:
			gso_type = SKB_GSO_TCPV6;
			ip_proto = IPPROTO_TCP;
			thlen = sizeof(struct tcphdr);
			break;
		case ZXDH_NET_HDR_GSO_UDP:
			gso_type = SKB_GSO_UDP;
			ip_proto = IPPROTO_UDP;
			thlen = sizeof(struct udphdr);
			break;
		default:
			return -EINVAL;
		}

		if (hdr->gso_type & ZXDH_NET_HDR_GSO_ECN)
			gso_type |= SKB_GSO_TCP_ECN;

		if (hdr->gso_size == 0)
			return -EINVAL;
	}

	skb_reset_mac_header(skb);

	if (hdr->flags & ZXDH_NET_HDR_F_NEEDS_CSUM) {
		start = __vqm16_to_cpu(little_endian, hdr->csum_start);
		off = __vqm16_to_cpu(little_endian, hdr->csum_offset);
		needed = start + max_t(u32, thlen, off + sizeof(__sum16));

		if (!pskb_may_pull(skb, needed))
			return -EINVAL;

		if (!skb_partial_csum_set(skb, start, off))
			return -EINVAL;

		p_off = skb_transport_offset(skb) + thlen;
		if (!pskb_may_pull(skb, p_off))
			return -EINVAL;
	} else {
		/* gso packets without NEEDS_CSUM do not set transport_offset.
		 * probe and drop if does not match one of the above types.
		 */
		if (gso_type && skb->network_header) {
			if (!skb->protocol) {
				protocol = dev_parse_header_protocol(skb);

				if (!protocol)
					net_hdr_set_proto_split(skb, hdr);
				else if (!net_hdr_match_proto_split(protocol, hdr->gso_type))
					return -EINVAL;
				else
					skb->protocol = protocol;
			}
retry:
			if (!skb_flow_dissect_flow_keys_basic(NULL, skb, &keys, NULL, 0, 0, 0, 0))
			{
				/* UFO does not specify ipv4 or 6: try both */
				if (gso_type & SKB_GSO_UDP &&
				    skb->protocol == htons(ETH_P_IP)) {
					skb->protocol = htons(ETH_P_IPV6);
					goto retry;
				}
				return -EINVAL;
			}

			p_off = keys.control.thoff + thlen;
			if (!pskb_may_pull(skb, p_off) ||
			    keys.basic.ip_proto != ip_proto)
				return -EINVAL;

			skb_set_transport_header(skb, keys.control.thoff);
		} else if (gso_type) {
			p_off = thlen;
			if (!pskb_may_pull(skb, p_off))
				return -EINVAL;
		}
	}

	if (hdr->gso_type != ZXDH_NET_HDR_GSO_NONE) {
		gso_size = __vqm16_to_cpu(little_endian, hdr->gso_size);
		nh_off = p_off;
		shinfo = skb_shinfo(skb);

		/* UFO may not include transport header in gso_size. */
		if (gso_type & SKB_GSO_UDP)
			nh_off -= thlen;

		/* Too small packets are not really GSO ones. */
		if (skb->len - nh_off > gso_size) {
			shinfo->gso_size = gso_size;
			shinfo->gso_type = gso_type;

			/* Header must be checked, and gso_segs computed. */
			shinfo->gso_type |= SKB_GSO_DODGY;
			shinfo->gso_segs = 0;
		}
	}

	return 0;
}
#else
int net_hdr_to_skb_split(struct sk_buff *skb, const struct zxdh_split_net_hdr *hdr, bool little_endian)
{
	unsigned int gso_type = 0;
	uint16_t start = 0;
	uint16_t off = 0;
	uint16_t gso_size = 0;

	if (hdr->gso_type != ZXDH_NET_HDR_GSO_NONE) {
		switch (hdr->gso_type & ~ZXDH_NET_HDR_GSO_ECN) {
		case ZXDH_NET_HDR_GSO_TCPV4:
			gso_type = SKB_GSO_TCPV4;
			break;
		case ZXDH_NET_HDR_GSO_TCPV6:
			gso_type = SKB_GSO_TCPV6;
			break;
		case ZXDH_NET_HDR_GSO_UDP:
			gso_type = SKB_GSO_UDP;
			break;
		default:
			return -EINVAL;
		}

		if (hdr->gso_type & ZXDH_NET_HDR_GSO_ECN)
			gso_type |= SKB_GSO_TCP_ECN;

		if (hdr->gso_size == 0)
			return -EINVAL;
	}

	if (hdr->flags & ZXDH_NET_HDR_F_NEEDS_CSUM) {
		start = __vqm16_to_cpu(little_endian, hdr->csum_start);
		off = __vqm16_to_cpu(little_endian, hdr->csum_offset);

		if (!skb_partial_csum_set(skb, start, off))
			return -EINVAL;
	}

	if (hdr->gso_type != ZXDH_NET_HDR_GSO_NONE) {
		gso_size = __vqm16_to_cpu(little_endian, hdr->gso_size);

		skb_shinfo(skb)->gso_size = gso_size;
		skb_shinfo(skb)->gso_type = gso_type;

		/* Header must be checked, and gso_segs computed. */
		skb_shinfo(skb)->gso_type |= SKB_GSO_DODGY;
		skb_shinfo(skb)->gso_segs = 0;
	}

	return 0;
}
#endif

void receive_buf_split(struct zxdh_en_device *en_dev, struct receive_queue *rq,
                       void *buf, uint32_t len, void **ctx, struct virtnet_rq_stats *stats)
{
    struct net_device *netdev = en_dev->netdev;
    struct sk_buff *skb = NULL;
    struct zxdh_split_hdr_rx *hdr = NULL;
    uint8_t pkt_flag = 0;
    uint8_t packet_to_file = 0;
    uint32_t split_hdr_len = en_dev->hdr_len_rx_split;
    struct zxdh_split_hdr_rx *hdr_rcv = (struct zxdh_split_hdr_rx *)buf;

    if ((hdr_rcv->pd_hdr.flags & 0xff) == ZXDH_PKT_FLAG)
    {
        pkt_flag = 1;
        split_hdr_len = sizeof(struct zxdh_split_net_hdr) + ZXDH_PKT_PD_HDR_LEN;   /* 28 */
    }

    if (unlikely(len < split_hdr_len + ETH_HLEN))
    {
        LOG_ERR_DEV(en_dev->parent, "%s: short packet %i\n", netdev->name, len);
        netdev->stats.rx_length_errors++;
        netdev->stats.rx_errors++;

        pkt_packet_process(en_dev, buf, len, pkt_flag);
        goto ret_out;
    }

    /* 因为后面还要用buf。先get一下将引用计数加一，防止buf所在page被释放。 */
    get_page(virt_to_head_page(buf));
    skb = receive_mergeable_split(netdev, en_dev, rq, buf, ctx, len, split_hdr_len, stats);

    if (unlikely(!skb))
    {
        LOG_ERR_DEV(en_dev->parent, "skb receive_mergeable null\n");
        pkt_packet_process(en_dev, buf, len, pkt_flag);
        goto ret_out;
    }

    hdr = skb_vnet_hdr_rx(skb);

    if (hdr->split_hdr.flags & ZXDH_NET_HDR_F_DATA_VALID)
        skb->ip_summed = CHECKSUM_UNNECESSARY;

    if (net_hdr_to_skb_split(skb, &hdr->split_hdr, zxdh_is_little_endian(en_dev)))
    {
        LOG_ERR_DEV(en_dev->parent, "%s: bad gso: type: %u, size: %u\n",
                    netdev->name, hdr->split_hdr.gso_type, hdr->split_hdr.gso_size);
        netdev->stats.rx_frame_errors++;
        pkt_packet_process(en_dev, buf, len, pkt_flag);
        dev_kfree_skb(skb);
        goto ret_out;
    }

    pd_receive_handle(netdev, skb, &hdr->pd_hdr, stats);

    skb_record_rx_queue(skb, vq2rxq(rq->vq));
    packet_to_file = pkt_skb_packet_process(en_dev, skb, pkt_flag);
    skb->protocol = eth_type_trans(skb, netdev);

    if (packet_to_file == 0)
    {
        napi_gro_receive(&rq->napi, skb);
    }

ret_out:
    /* buf使用完，引用计数减1。 */
    put_page(virt_to_head_page(buf));
    return;

}

void receive_buf(struct zxdh_en_device *en_dev, struct receive_queue *rq,
                 void *buf, uint32_t len, void **ctx,
                 uint32_t *xdp_xmit,
                 struct virtnet_rq_stats *stats,
                 uint32_t _id)
{
    struct net_device *netdev = en_dev->netdev;
    struct sk_buff *skb = NULL;
    struct zxdh_net_hdr_rx *hdr_rcv = (struct zxdh_net_hdr_rx *)buf;
    int32_t ret = 0;
    struct zxdh_net_1588_hdr_rcv *hdr_rcv_1588 = NULL;
    struct zxdh_net_1588_nopi_hdr_rcv *hdr_rcv_nopi_1588 = NULL;
    uint8_t pd_len = 0;
    uint8_t pkt_flag = 0;
    uint8_t packet_to_file = 0;
    struct zxdh_frag frag = {};
    struct vring_virtqueue *vq= to_vvq(rq->vq);
    // struct iphdr *iph = NULL;
    // struct udphdr *udph = NULL;
    // __sum16 skb_sum = 0;
    DEBUG_1588_DEV(en_dev->parent, "receive buf:");
    DEBUG_1588_DATA((uint8_t *)buf, len);

    pd_len = hdr_rcv->pd_len * HDR_2B_UNIT;
    DEBUG_1588_DEV(en_dev->parent, "pd_len:%hhu", pd_len);

    if ((hdr_rcv->pd_hdr.flags & 0xff) == ZXDH_PKT_FLAG)
    {
        pkt_flag = 1;
    }

    frag.frag_id[0] = _id;
    frag.frag_num = 1;

    if (unlikely(len < (hdr_rcv->pd_len * HDR_2B_UNIT) + ETH_HLEN))
    {
        LOG_ERR_DEV(en_dev->parent, "%s: short packet %i\n", netdev->name, len);
        netdev->stats.rx_length_errors++;
        netdev->stats.rx_errors++;
        pkt_packet_process(en_dev, buf, len, pkt_flag);
        goto ret_out;
    }

    skb = receive_mergeable(netdev, en_dev, rq, buf, ctx, len, xdp_xmit, stats, &frag);

    if (unlikely(!skb))
    {
        LOG_ERR_DEV(en_dev->parent, "skb receive_mergeable null\n");
        pkt_packet_process(en_dev, buf, len, pkt_flag);
        goto ret_out;
    }

    if (hdr_rcv->pd_len > ZXDH_HAS_PI_FLAG)
    {
        pipd_receive_handle(netdev, skb, hdr_rcv, stats);
    }
    else if (hdr_rcv->pd_len != ZXDH_TYPE_FLAG_LEN)/* True:非LACP报文， False:LACP报文*/
    {
        pd_receive_handle(netdev, skb, &hdr_rcv->pd_hdr, stats);
    }

    if (en_dev->enable_1588 == true)
    {
        if (pd_len == sizeof(struct zxdh_net_1588_hdr_rcv) || pd_len == sizeof(struct zxdh_net_1588_nopi_hdr_rcv))
        {
            if(en_dev->dtp_drs_offload == true)
            {
                hdr_rcv_1588 = (struct zxdh_net_1588_hdr_rcv *)hdr_rcv;
                ret = pkt_1588_proc_rcv(skb, &(hdr_rcv_1588->pd_1588), en_dev->clock_no, en_dev);

                DEBUG_1588_DEV(en_dev->parent, "vport 0x%x rx 1588 hdr :",en_dev->vport);
                DEBUG_1588_DATA((uint8_t *)hdr_rcv_1588, sizeof(struct zxdh_net_1588_hdr_rcv));
            }
            else
            {
                hdr_rcv_nopi_1588 = (struct zxdh_net_1588_nopi_hdr_rcv *)hdr_rcv;
                ret = pkt_1588_proc_rcv(skb, &(hdr_rcv_nopi_1588->pd_1588), en_dev->clock_no, en_dev);

                DEBUG_1588_DEV(en_dev->parent, "vport 0x%x rx 1588 hdr nopi :",en_dev->vport);
                DEBUG_1588_DATA((uint8_t *)hdr_rcv_nopi_1588, sizeof(struct zxdh_net_1588_nopi_hdr_rcv));
            }

            if ((ret != PTP_SUCCESS) && (ret != IS_NOT_PTP_MSG))
            {
                DEBUG_1588_DEV(en_dev->parent, "dev %s vport 0x%x pkt_1588_proc_rcv !!!\n",en_dev->netdev->name, en_dev->vport);
            }

            if (skb->ip_summed == CHECKSUM_NONE)
            {
                skb->ip_summed = CHECKSUM_UNNECESSARY;
                // // 计算 IP 校验和
                // iph = ip_hdr(skb);
                // iph->check = 0;
                // iph->check = ip_fast_csum((unsigned char *)iph, iph->ihl);

                // // 计算 UDP 伪首部校验和
                // udph = udp_hdr(skb);
                // udph->check = 0;
                // udph->check = csum_tcpudp_magic(iph->saddr, iph->daddr, skb->len - ip_hdrlen(skb), IPPROTO_UDP, 0);

                // //计算skb校验和
                // skb_sum = skb_checksum(skb, 0, skb->len, 0);
                // skb->csum = skb_sum;
            }

            DEBUG_1588_DEV(en_dev->parent, "rx skb->data:");
            DEBUG_1588_DATA((uint8_t *)skb->data, skb->len);
        }
    }

    skb_record_rx_queue(skb, vq2rxq(rq->vq));
    packet_to_file = pkt_skb_packet_process(en_dev, skb, pkt_flag);
    skb->protocol = eth_type_trans(skb, netdev);

    //LOG_INFO("receiving skb proto 0x%04x len %i type %i\n", ntohs(skb->protocol), skb->len, skb->pkt_type);
    if (packet_to_file == 0)
    {
        napi_gro_receive(&rq->napi, skb);
    }
    else
    {
        dev_kfree_skb(skb);
    }

ret_out:
    while(frag.frag_num > 0)
    {
        uint32_t id = frag.frag_id[--frag.frag_num];
        if (vq->packed.desc_extra[id].last_in_page)
        {
            dh_page_release_dynamic(rq, vq->packed.desc_extra[id].di, true);
            vq->packed.desc_extra[id].di->page = NULL;
        }
    }

    return;
}

/**
 * virtqueue_notify - second half of split virtqueue_kick call.
 * @_vq: the struct virtqueue
 *
 * This does not need to be serialized.
 *
 * Returns false if host notify failed or queue is broken, otherwise true.
 */
bool virtqueue_notify(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    if (unlikely(vq->broken))
    {
        LOG_ERR("vq->broken\n");
        return false;
    }

    /* Prod other side to tell it about changes. */
    if (!vq->notify(_vq))
    {
        LOG_ERR("vq->notify(_vq) failed\n");
        vq->broken = true;
        return false;
    }

    return true;
}

bool dh_skb_page_frag_refill(unsigned int sz, struct page_frag *pfrag, gfp_t gfp)
{
    if (pfrag->page) {
        if (page_ref_count(pfrag->page) == 1) {
            pfrag->offset = 0;
            return true;
        }
        if (pfrag->offset + sz <= pfrag->size)
            return true;
        put_page(pfrag->page);
    }
    pfrag->offset = 0;
    if (1) {
        /* Avoid direct reclaim but allow kswapd to wake */
    #ifndef CGS_V5_693
        pfrag->page = alloc_pages((gfp & ~__GFP_DIRECT_RECLAIM) |
                      __GFP_COMP | __GFP_NOWARN |
                      __GFP_NORETRY,
                      DH_SKB_FRAG_PAGE_ORDER);
    #else
        pfrag->page = alloc_pages((gfp & ~___GFP_RECLAIMABLE) |
            __GFP_COMP | __GFP_NOWARN |
            __GFP_NORETRY,
            DH_SKB_FRAG_PAGE_ORDER);
    #endif
        if (likely(pfrag->page)) {
            pfrag->size = PAGE_SIZE << DH_SKB_FRAG_PAGE_ORDER;
            return true;
        }
    }
    pfrag->page = alloc_page(gfp);
    if (likely(pfrag->page)) {
        pfrag->size = PAGE_SIZE;
        return true;
    }
    return false;
}

int32_t add_recvbuf_mergeable_split(struct receive_queue *rq, gfp_t gfp)
{
    struct page_frag *alloc_frag = &rq->alloc_frag;
    uint32_t headroom = 0;
    uint32_t tailroom = 0;
    uint32_t room = SKB_DATA_ALIGN(headroom + tailroom);
    char *buf = NULL;
    void *ctx = NULL;
    int32_t err = 0;
    uint32_t len = 0;
    uint32_t hole = 0;

    /* Extra tailroom is needed to satisfy XDP's assumption. This
     * means rx frags coalescing won't work, but consider we've
     * disabled GSO for XDP, it won't be a big issue.
     */
    len = get_mergeable_buf_len(rq, &rq->mrg_avg_pkt_len, room);
    if (unlikely(!dh_skb_page_frag_refill(len + room, alloc_frag, gfp)))
    {
        LOG_ERR("dh_skb_page_frag_refill failed\n");
        return -ENOMEM;
    }

    buf = (char *)page_address(alloc_frag->page) + alloc_frag->offset;
    buf += headroom; /* advance address leaving hole at front of pkt */
    get_page(alloc_frag->page);
    alloc_frag->offset += len + room;
    hole = alloc_frag->size - alloc_frag->offset;
    if (hole < len + room)
    {
        /* To avoid internal fragmentation, if there is very likely not
         * enough space for another buffer, add the remaining space to
         * the current buffer.
         */
        len += hole;
        alloc_frag->offset += hole;
    }

    sg_init_one(rq->sg, buf, len);
    ctx = mergeable_len_to_ctx(len, headroom);
    err = virtqueue_add_inbuf_ctx(rq->vq, rq->sg, 1, buf, ctx, gfp);
    if (err < 0)
    {
        put_page(virt_to_head_page(buf));
    }

    return err;
}

int32_t add_recvbuf_mergeable(struct receive_queue *rq, gfp_t gfp)
{
    struct zxdh_en_device *en_dev = rq->vq->en_dev;
    bool packed_status = en_dev->packed_status;
    void *ctx = mergeable_len_to_ctx(DEFAULT_FRAG_LENGTH, 0);

    if (packed_status)
    {
        return virtqueue_add_inbuf_ctx_in(rq, ctx, gfp);
    }
    return add_recvbuf_mergeable_split(rq, gfp);
}

/* Assuming a given event_idx value from the other side, if
 * we have just incremented index from old to new_idx,
 * should we trigger an event? */
int32_t vring_need_event(__u16 event_idx, __u16 new_idx, __u16 old)
{
    /* Note: Xen has similar logic for notification hold-off
     * in include/xen/interface/io/ring.h with req_event and req_prod
     * corresponding to event_idx + 1 and new_idx respectively.
     * Note also that req_event and req_prod in Xen start at 1,
     * event indexes in custom queue start at 0. */
    return (__u16)(new_idx - event_idx - 1) < (__u16)(new_idx - old);
}

bool virtqueue_kick_prepare_packed(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);
    uint16_t new = 0;
    uint16_t old = 0;
    uint16_t off_wrap = 0;
    uint16_t flags = 0;
    uint16_t wrap_counter = 0;
    uint16_t event_idx = 0;
    bool needs_kick = false;
    union
    {
        struct
        {
            __le16 off_wrap;
            __le16 flags;
        };
        uint32_t u32;
    } snapshot;

    START_USE(vq);

    /*
     * We need to expose the new flags value before checking notification
     * suppressions.
     */
    vqm_mb(vq->weak_barriers);

    old = vq->packed.next_avail_idx - vq->num_added;
    new = vq->packed.next_avail_idx;
    vq->num_added = 0;

    snapshot.u32 = *(uint32_t *)vq->packed.vring.device;
    flags = le16_to_cpu(snapshot.flags);

    LAST_ADD_TIME_CHECK(vq);
    LAST_ADD_TIME_INVALID(vq);

    if (flags != VRING_PACKED_EVENT_FLAG_DESC)
    {
        needs_kick = (flags != VRING_PACKED_EVENT_FLAG_DISABLE);
        goto out;
    }

    off_wrap = le16_to_cpu(snapshot.off_wrap);

    wrap_counter = off_wrap >> VRING_PACKED_EVENT_F_WRAP_CTR;
    event_idx = off_wrap & ~(1 << VRING_PACKED_EVENT_F_WRAP_CTR);
    if (wrap_counter != vq->packed.avail_wrap_counter)
    {
        event_idx -= vq->packed.vring.num;
    }

    needs_kick = vring_need_event(event_idx, new, old);
out:
    END_USE(vq);
    return needs_kick;
}

bool virtqueue_kick_prepare_split(struct virtqueue *_vq)
{
	struct vring_virtqueue *vq = to_vvq(_vq);
	u16 new, old;
	bool needs_kick;

	START_USE(vq);
	/* We need to expose available array entries before checking avail
	 * event. */
	vqm_mb(vq->weak_barriers);

	old = vq->split.avail_idx_shadow - vq->num_added;
	new = vq->split.avail_idx_shadow;
	vq->num_added = 0;

	LAST_ADD_TIME_CHECK(vq);
	LAST_ADD_TIME_INVALID(vq);

	if (vq->event) {
		needs_kick = vring_need_event(vqm16_to_cpu(_vq->en_dev,
					vring_avail_event(&vq->split.vring)),
					      new, old);
	} else {
		needs_kick = !(vq->split.vring.used->flags &
					cpu_to_vqm16(_vq->en_dev,
						VRING_USED_F_NO_NOTIFY));
	}
	END_USE(vq);
	return needs_kick;
}

bool virtqueue_kick_prepare(struct virtqueue *_vq)
{
	struct vring_virtqueue *vq = to_vvq(_vq);

	return vq->packed_ring ? virtqueue_kick_prepare_packed(_vq) :
				 virtqueue_kick_prepare_split(_vq);
}

/*
 * Returns false if we couldn't fill entirely (OOM).
 *
 * Normally run in the receive path, but can also be run from ndo_open
 * before we're receiving packets, or from refill_work which is
 * careful to disable receiving (using napi_disable).
 */
bool try_fill_recv(struct receive_queue *rq, gfp_t gfp)
{
    int32_t err = 0;
    bool oom = 0;
    unsigned long flags = 0;
    struct zxdh_en_device *en_dev = rq->vq->en_dev;
    bool packed_status = en_dev->packed_status;

    if (packed_status)
    {
        do
        {
            err = add_recvbuf_mergeable(rq, gfp);
            oom = err == -ENOMEM;
            if (err)
            {
                break;
            }
        } while (rq->vq->num_free & NUM_FREE_MASK);

        if (virtqueue_kick_prepare(rq->vq) && virtqueue_notify(rq->vq))
        {
        #ifdef CGS_V5_693
            /* 3.10 内核使用 u64_stats_update_begin，手动保存 flags */
            flags = 0;
            u64_stats_update_begin(&rq->stats.syncp);
            rq->stats.kicks++;
            u64_stats_update_end(&rq->stats.syncp);
        #else
            flags = u64_stats_update_begin_irqsave(&rq->stats.syncp);
            rq->stats.kicks++;
            u64_stats_update_end_irqrestore(&rq->stats.syncp, flags);
        #endif
        }

        dh_rx_cache_may_reduce(rq);
    }
    else
    {
        do
        {
            err = add_recvbuf_mergeable(rq, gfp);
            oom = err == -ENOMEM;
            if (err)
            {
                break;
            }
        } while (rq->vq->num_free);

        if (virtqueue_kick_prepare(rq->vq) && virtqueue_notify(rq->vq))
        {
        #ifdef CGS_V5_693
            /* 3.10 内核使用 u64_stats_update_begin，手动保存 flags */
            flags = 0;
            u64_stats_update_begin(&rq->stats.syncp);
            rq->stats.kicks++;
            u64_stats_update_end(&rq->stats.syncp);
        #else
            flags = u64_stats_update_begin_irqsave(&rq->stats.syncp);
            rq->stats.kicks++;
            u64_stats_update_end_irqrestore(&rq->stats.syncp, flags);
        #endif
        }
    }

    return !oom;
}

int32_t virtnet_receive(struct receive_queue *rq, int32_t budget, uint32_t *xdp_xmit)
{
    struct zxdh_en_device *en_dev = rq->vq->en_dev;
    struct virtnet_rq_stats stats = {};
    uint32_t len = 0;
    void *buf = NULL;
    int32_t i = 0;
    void *ctx = NULL;
    uint32_t _id;
    bool packed_status = en_dev->packed_status;

    while (stats.packets < budget && (buf = virtqueue_get_buf_ctx(rq->vq, &len, &ctx, en_dev->ro_flag, &_id)))
    {
        if (packed_status)
        {
            receive_buf(en_dev, rq, buf, len, ctx, xdp_xmit, &stats, _id);
        }
        else
        {
            receive_buf_split(en_dev, rq, buf, len, ctx, &stats);
        }
        stats.packets++;
    }

    if (rq->vq->num_free > min((uint32_t)budget, virtqueue_get_vring_size(rq->vq)) / 2)
    {
        if (!try_fill_recv(rq, GFP_ATOMIC))
        {
            schedule_delayed_work(&en_dev->refill, 0);
        }
    }

    u64_stats_update_begin(&rq->stats.syncp);
    for (i = 0; i < VIRTNET_RQ_STATS_LEN; i++)
    {
        size_t offset = virtnet_rq_stats_desc[i].offset;
        uint64_t *item;

        item = (uint64_t *)((uint8_t *)&rq->stats + offset);
        *item += *(uint64_t *)((uint8_t *)&stats + offset);
    }
    u64_stats_update_end(&rq->stats.syncp);

    return stats.packets;
}

void virtqueue_napi_complete(struct napi_struct *napi, struct virtqueue *vq, int32_t processed)
{
    int32_t opaque = 0;

    opaque = virtqueue_enable_cb_prepare(vq);
    if (napi_complete_done(napi, processed))
    {
        if (unlikely(virtqueue_poll(vq, opaque)))
        {
            virtqueue_napi_schedule(napi, vq);
        }
    }
    else
    {
        virtqueue_disable_cb(vq);
    }
}

int virtnet_poll(struct napi_struct *napi, int budget)
{
    struct receive_queue *rq = container_of(napi, struct receive_queue, napi);
#ifndef CGS_V5_693
    struct zxdh_en_device *en_dev = rq->vq->en_dev;
    struct send_queue *sq;
#endif
    uint32_t received = 0;
    uint32_t xdp_xmit = 0;

    virtnet_poll_cleantx(rq);

    received = virtnet_receive(rq, budget, &xdp_xmit);

    /* Out of packets? */
    if (received < budget)
    {
        virtqueue_napi_complete(napi, rq->vq, received);
    }

#ifndef CGS_V5_693
    if (xdp_xmit & ZXDH_XDP_REDIR)
    {
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5,5,0))
        xdp_do_flush();
#else
        xdp_do_flush_map();
#endif
    }


    if (xdp_xmit & ZXDH_XDP_TX) {
        sq = zxdh_en_xdp_get_sq(en_dev);
        if (virtqueue_kick_prepare(sq->vq) && virtqueue_notify(sq->vq)) {
            u64_stats_update_begin(&sq->stats.syncp);
            sq->stats.kicks++;
            u64_stats_update_end(&sq->stats.syncp);
        }
        zxdh_en_xdp_put_sq(en_dev, sq);
    }
#endif
    return received;
}

int32_t virtnet_alloc_queues(struct zxdh_en_device *en_dev)
{
    int32_t i = 0;

    zte_memset_s(en_dev->sq, 0, sizeof(*en_dev->sq) * en_dev->max_queue_pairs);
    zte_memset_s(en_dev->rq, 0, sizeof(*en_dev->rq) * en_dev->max_queue_pairs);

    INIT_DELAYED_WORK(&en_dev->refill, refill_work);

    for (i = 0; i < en_dev->curr_queue_pairs; i++)
    {
        en_dev->rq[i].pages = NULL;
#if defined(ZXDH_ADAPT_REDHAT_9_2) || defined(NETIF_NAPI_ADD_NO_WEIGHT) || defined(Rocky_9_3) || defined(ZXDH_ADAPT_REDHAT_9_6) || defined(KYLIN_V11_6_6)
        netif_napi_add(en_dev->netdev, &en_dev->rq[i].napi, virtnet_poll);
        netif_napi_add_tx_weight(en_dev->netdev, &en_dev->sq[i].napi, virtnet_poll_tx, NAPI_POLL_WEIGHT);
#else
        netif_napi_add(en_dev->netdev, &en_dev->rq[i].napi, virtnet_poll, NAPI_POLL_WEIGHT);
        netif_tx_napi_add(en_dev->netdev, &en_dev->sq[i].napi, virtnet_poll_tx, NAPI_POLL_WEIGHT);
#endif

        sg_init_table(en_dev->rq[i].sg, ARRAY_SIZE(en_dev->rq[i].sg));
        ewma_pkt_len_init(&en_dev->rq[i].mrg_avg_pkt_len);
        sg_init_table(en_dev->sq[i].sg, ARRAY_SIZE(en_dev->sq[i].sg));

        u64_stats_init(&en_dev->rq[i].stats.syncp);
        u64_stats_init(&en_dev->sq[i].stats.syncp);
    }

    return 0;
}

/**
 * virtqueue_set_affinity - setting affinity for a virtqueue
 * @vq: the virtqueue
 * @cpu_mask: the cpu no.
 *
 * Pay attention the function are best-effort: the affinity hint may not be set
 * due to config support, irq type and sharing.
 *
 */
int32_t virtqueue_set_affinity(struct virtqueue *vq, const struct cpumask *cpu_mask)
{
    if (!vq->callback)
    {
        LOG_ERR("vq->callback is null\n");
        return -EINVAL;
    }

    return 0;
}

void refill_work(struct work_struct *work)
{
    int32_t i = 0;
    bool still_empty = false;
    struct zxdh_en_device *en_dev = container_of(work, struct zxdh_en_device, refill.work);

    for (i = 0; i < en_dev->eth_config.num_rxq; i++)
    {
        struct receive_queue *rq = &en_dev->rq[i];

        napi_disable(&rq->napi);
        still_empty = !try_fill_recv(rq, GFP_KERNEL);
        virtnet_napi_enable(rq->vq, &rq->napi);

        /* In theory, this can happen: if we don't get any buffers in
         * we will *never* try to fill again.
         */
        if (still_empty)
        {
            schedule_delayed_work(&en_dev->refill, HZ/2);
        }
    }
}

int32_t dh_eq_vqs_vring_int(struct notifier_block *nb, unsigned long action, void *data)
{
    struct dh_eq_vq *eq_vq = container_of(nb, struct dh_eq_vq, irq_nb);
    struct dh_eq_vqs *eq_vqs = container_of(eq_vq, struct dh_eq_vqs, vq_s);
    struct list_head *item = NULL;
    struct zxdh_pci_vq_info *info = NULL;
    struct vring_virtqueue *vq = NULL;
    struct zxdh_en_device *en_dev = NULL;
    unsigned long flags;

    en_dev = (struct zxdh_en_device *)data;
    spin_lock_irqsave(&en_dev->lock, flags);

    list_for_each(item, &eq_vqs->vqs)
    {
        info = list_entry(item, struct zxdh_pci_vq_info, node);

        vq = to_vvq(info->vq);
        if (!more_used(vq))
        {
            continue;
        }

        if (unlikely(vq->broken))
        {
            LOG_ERR_DEV(en_dev->parent, "vq:%d is broken\n", info->vq->phy_index);
            continue;
        }

        /* Just a hint for performance: so it's ok that this can be racy! */
        if (vq->event)
        {
            vq->event_triggered = true;
        }

        if (vq->vq.callback)
        {
            vq->vq.callback(&vq->vq);
        }
    }

    spin_unlock_irqrestore(&en_dev->lock, flags);

    return 0;
}

int32_t zxdh_vqm_get_start_qid(struct zxdh_en_device *en_dev, uint16_t *start_qid)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed!\n", sizeof(union zxdh_msg));
        return -1;
    }

    msg->vqm_msg.vqm_vfid = VQM_VFID(en_dev->vport);
    msg->vqm_msg.opcode = OPCODE_GET;
    msg->vqm_msg.cmd = START_QID_QPAIR;
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_CFG_VQM, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "send get start qid msg to riscv failed!\n");
    }
    else
    {
        if (msg->vqm_reps.check_result != ZXDH_REPS_SUCC)
        {
            LOG_ERR_DEV(en_dev->parent, "get vfid(%u) start_qid %u failed!\n", VQM_VFID(en_dev->vport), msg->vqm_reps.queue.start_qid)
            err = -1;
        }
        else
        {
            *start_qid = msg->vqm_reps.queue.start_qid;
            LOG_INFO_DEV(en_dev->parent, "get vfid(%u) start_qid %u success.\n", VQM_VFID(en_dev->vport), msg->vqm_reps.queue.start_qid);
        }
    }
    kfree(msg);
    return err;
}

int32_t zxdh_get_split_phy_vqs(struct zxdh_en_device *en_dev, uint16_t vq_cnt)
{
    uint16_t start_qid = 0;
    int32_t err = 0;
    uint16_t max_qpair = 0;
    uint16_t pre_bond_qidx = 0;
    int32_t i = 0;

    err = zxdh_vqm_get_start_qid(en_dev, &start_qid);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_vqm_get_start_qid failed, err:%d\n", err);
        return -1;
    }

    max_qpair = ioread16(en_dev->ops->get_dev_cap(en_dev->parent) + ZXDH_DEV_CAP_MAX_QPAIR);
    if (max_qpair > ZXDH_MAX_QUEUES_NUM)
    {
        LOG_ERR_DEV(en_dev->parent, "max_qpair:%u out of rang:%d\n", max_qpair, ZXDH_MAX_QUEUES_NUM);
        return -1;
    }

    if (en_dev->ops->is_bond(en_dev->parent))
    {
        pre_bond_qidx = en_dev->ops->get_pre_bond_qidx(en_dev->parent);
        for (i = 0; i < vq_cnt; i++)
        {
            en_dev->logic_index_split[i] = pre_bond_qidx + i;
            en_dev->phy_index[i] = pre_bond_qidx + start_qid + i;
            LOG_DEBUG_DEV(en_dev->parent, "bond logic_index_split[%u]:%u, phy_index[%u]:%u\n",
                        i, en_dev->logic_index_split[i], i, en_dev->phy_index[i]);
        }
        en_dev->ops->set_pre_bond_qidx(en_dev->parent, pre_bond_qidx + vq_cnt);
    }
    else
    {
        for (i = 0; i < vq_cnt; i++)
        {
            en_dev->logic_index_split[i] = i;
            en_dev->phy_index[i] = start_qid + i;
            LOG_DEBUG_DEV(en_dev->parent, "logic_index_split[%u]:%u, phy_index[%u]:%u\n",
                        i, en_dev->logic_index_split[i], i, en_dev->phy_index[i]);
        }
    }

    if (i > (max_qpair * 2))
    {
        LOG_ERR_DEV(en_dev->parent, "current device queue count:%u out of FW allocate max queue count:%u\n", i, (max_qpair * 2));
        return -1;
    }

    return 0;
}

int32_t vp_get_phy_vqs(struct net_device *netdev, uint16_t vq_cnt, uint32_t *phy_index, const char *type)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint16_t fw_patch = en_dev->ops->get_fw_patch(en_dev->parent);
    int32_t err = 0;
    union zxdh_msg *old_msg = NULL;
    uint32_t i = 0;
    unsigned long flags = 0;

    if (!en_dev->packed_status)
    {
        err = zxdh_get_split_phy_vqs(en_dev, vq_cnt);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_get_split_phy_vqs failed, err:%d\n", err);
        }
        return err;
    }

    if (fw_patch < DH_NEW_QUEEU_ALLOC_PATCH)
    {
        old_msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
        if (old_msg == NULL)
        {
            LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
            return -1;
        }

        /* if bond device, read queue already used */
        if (en_dev->ops->is_bond(en_dev->parent))
        {
            LOG_DEBUG_DEV(en_dev->parent, "Start get_common_table_msg!!!");
            err = get_common_table_msg(en_dev, en_dev->pcie_id, OP_CODE_DATA_CHAN, old_msg);
            if (err != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "Failed to get bond device queue information: %d\n", err);
                kfree(old_msg);
                return -1;
            }

            LOG_DEBUG_DEV(en_dev->parent, "old_msg->reps.cmn_vq_msg.queue_nums=%u", old_msg->reps.cmn_vq_msg.queue_nums);
            for (i=0; i<old_msg->reps.cmn_vq_msg.queue_nums; i++)
            {
                LOG_DEBUG_DEV(en_dev->parent, "old_msg->reps.cmn_vq_msg.phy_qidx[%u]: %u", i, old_msg->reps.cmn_vq_msg.phy_qidx[i]);
            }
        }
    }

    if (fw_patch >= DH_NEW_QUEEU_ALLOC_PATCH)
    {
        local_irq_save(flags);       // 关闭中断
        preempt_disable();           // 关闭抢占
    }

    /* get phy vq lock */
    err = en_dev->ops->get_vq_lock(en_dev->parent);
    if (err < 0)
    {
        goto err_get_lock;
    }

    /* find valid vqs */
    err = en_dev->ops->find_valid_vqs(en_dev->parent, vq_cnt, phy_index);
    if (err < 0)
    {
        goto err_find_valid_vqs;
    }

    /* write common list */
    if (fw_patch < DH_NEW_QUEEU_ALLOC_PATCH)
    {
        err = zxdh_common_tbl_init(netdev, old_msg);
    }
    else
    {
        err = en_dev->ops->write_queue_tlb(en_dev->parent, vq_cnt, phy_index, false);
    }
    if (err != 0)
    {
        goto err_find_valid_vqs;
    }

    /* write vq list */
    en_dev->ops->write_vqs_bit(en_dev->parent, vq_cnt, phy_index);

err_find_valid_vqs:
    /* release phy vq lock */
    en_dev->ops->release_vq_lock(en_dev->parent);
err_get_lock:
    if (fw_patch < DH_NEW_QUEEU_ALLOC_PATCH)
    {
        kfree(old_msg);
    }

    if (fw_patch >= DH_NEW_QUEEU_ALLOC_PATCH)
    {
        preempt_enable();
        local_irq_restore(flags);
    }

    return err;
}

int32_t vp_find_vqs_msix(struct net_device *netdev, unsigned nvqs,
                         struct virtqueue *vqs[], vq_callback_t *callbacks[],
                         const char * const names[], const bool *ctx)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t err = 0;
    uint16_t qidx = 0;
    struct dh_core_dev *dh_dev = en_dev->parent;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev->parent);
    struct pci_dev *pdev = to_pci_dev(netdev->dev.parent);
    uint8_t fw_version[FW_VERSION_LEN] = {0};
    bool skip_queue_enable = false;

    if (nvqs > ZXDH_MAX_QUEUES_NUM)
    {
        LOG_ERR_DEV(en_dev->parent, "Too many vqs: vq_cnt=%d out of rang:%d", nvqs, ZXDH_MAX_QUEUES_NUM);
        return -1;
    }

    en_dev->vqs = kcalloc(nvqs, sizeof(*en_dev->vqs), GFP_KERNEL);
    if (unlikely(en_dev->vqs == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "en_dev->vqs kcalloc failed\n");
        return -ENOMEM;
    }

    en_dev->ops->get_fw_version(en_dev->parent, fw_version);
    if ((strstr(fw_version, "2.24.40.01") != NULL) && \
        (!((pdev->device == 0x8040) && (pdev->vendor == 0x16c3))) && \
        (pf_dev->board_type == DH_INICB))
    {
        skip_queue_enable = true;
    }

    if (!skip_queue_enable) {
        err = vp_get_phy_vqs(netdev, nvqs, en_dev->phy_index, "std");
        if (err < 0)
        {
            LOG_ERR_DEV(en_dev->parent, "get vq phy lock failed!");
            goto err_find_vq;
        }
    } else {
        LOG_INFO_DEV(en_dev->parent, "+++Enter none-queue version!+++");
    }

    for (qidx = 0; qidx < nvqs; ++qidx)
    {
        vqs[qidx] = vp_setup_vq(netdev, qidx, callbacks[qidx], names[qidx], ctx ? ctx[qidx] : false, (qidx / 2));
        if (IS_ERR_OR_NULL(vqs[qidx]))
        {
            err = PTR_ERR(vqs[qidx]);
            LOG_ERR_DEV(en_dev->parent, "vp_setup_vq failed: %d\n", err);
            goto err_setup_vq;
        }

        if (skip_queue_enable)
        {
            continue;
        }

        en_dev->ops->set_queue_enable(en_dev->parent, en_dev->phy_index[qidx], en_dev->logic_index_split[qidx], true);
    }
    return 0;

err_setup_vq:
    zxdh_vp_reset(netdev);
err_find_vq:
    vp_del_vqs(netdev);
    return err;
}

void zxdh_en_recv_pkts(struct virtqueue *rvq)
{
    struct zxdh_en_device *en_dev = rvq->en_dev;
    struct receive_queue *rq = &en_dev->rq[vq2rxq(rvq)];

    virtqueue_napi_schedule(&rq->napi, rvq);
}

void zxdh_en_xmit_pkts(struct virtqueue *tvq)
{
    struct zxdh_en_device *en_dev = tvq->en_dev;
    struct napi_struct *napi = &en_dev->sq[vq2txq(tvq)].napi;

    /* Suppress further interrupts. */
    virtqueue_disable_cb(tvq);

    if (napi->weight)
    {
        virtqueue_napi_schedule(napi, tvq);
    }
    else
    {
        /* We were probably waiting for more output buffers. */
        netif_wake_subqueue(en_dev->netdev, vq2txq(tvq));
        en_dev->hw_stats.q_stats[vq2txq(tvq)].q_tx_wake++;
    }
}

void zxdh_free_hdr_buf(struct zxdh_en_device *en_dev)
{
    int32_t i = 0;

    for (i = 0; i < en_dev->max_queue_pairs; i++)
    {
        if (en_dev->sq[i].hdr_buf != NULL)
        {
            kfree(en_dev->sq[i].hdr_buf);
            en_dev->sq[i].hdr_buf = NULL;
        }
    }
}

int32_t zxdh_alloc_hdr_buf(struct zxdh_en_device *en_dev)
{
    int32_t i = 0;

    for (i = 0; i < en_dev->max_queue_pairs; i++)
    {
        en_dev->sq[i].hdr_idx = 0;
        en_dev->sq[i].hdr_buf = kzalloc(ZXDH_PF_MAX_DESC_NUM(en_dev) * HDR_BUFFER_LEN, GFP_KERNEL);
        if (en_dev->sq[i].hdr_buf == NULL)
        {
            LOG_ERR_DEV(en_dev->parent, "en_dev->sq[%d].hdr_buf kzalloc failed\n", i);
            zxdh_free_hdr_buf(en_dev);
            return -1;
        }
    }

    return 0;
}

int32_t virtnet_find_vqs(struct zxdh_en_device *en_dev)
{
    vq_callback_t **callbacks = NULL;
    struct virtqueue **vqs = NULL;
    int32_t ret = -ENOMEM;
    int32_t i = 0;
    int32_t total_vqs = 0;
    const char **names = NULL;
    bool *ctx = NULL;

    total_vqs = en_dev->max_queue_pairs * 2;

    /* Allocate space for find_vqs parameters */
    vqs = kcalloc(total_vqs, sizeof(*vqs), GFP_KERNEL);
    if (unlikely(vqs == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "vqs kcalloc failed\n");
        goto err_vq;
    }

    callbacks = kmalloc_array(total_vqs, sizeof(*callbacks), GFP_KERNEL);
    if (unlikely(callbacks == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "callbacks kmalloc_array failed\n");
        goto err_callback;
    }

    names = kmalloc_array(total_vqs, sizeof(*names), GFP_KERNEL);
    if (unlikely(names == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "names kmalloc_array failed\n");
        goto err_names;
    }

    ctx = kcalloc(total_vqs, sizeof(*ctx), GFP_KERNEL);
    if (unlikely(ctx == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "ctx kmalloc failed\n");
        goto err_ctx;
    }

    /* Allocate/initialize parameters for services send/receive virtqueues */
    for (i = 0; i < en_dev->max_queue_pairs; i++)
    {
        callbacks[rxq2vq(i)] = zxdh_en_recv_pkts;
        callbacks[txq2vq(i)] = zxdh_en_xmit_pkts;
        sprintf(en_dev->rq[i].name, "input.%d", i);
        sprintf(en_dev->sq[i].name, "output.%d", i);
        names[rxq2vq(i)] = en_dev->rq[i].name;
        names[txq2vq(i)] = en_dev->sq[i].name;
        if (ctx)
        {
            ctx[rxq2vq(i)] = true;
        }
    }

    ret = vp_find_vqs_msix(en_dev->netdev, total_vqs, vqs, callbacks, names, ctx);
    if (ret)
    {
        LOG_ERR_DEV(en_dev->parent, "vp_find_vqs_msix failed: %d\n", ret);
        goto err_find;
    }

    for (i = 0; i < en_dev->max_queue_pairs; i++)
    {
        en_dev->rq[i].vq = vqs[rxq2vq(i)];
        en_dev->rq[i].vq->rq = &en_dev->rq[i];
        en_dev->sq[i].vq = vqs[txq2vq(i)];
    }

err_find:
    kfree(ctx);
    ctx = NULL;
err_ctx:
    kfree(names);
    names = NULL;
err_names:
    kfree(callbacks);
    callbacks = NULL;
err_callback:
    kfree(vqs);
    vqs = NULL;
err_vq:
    return ret;
}

void virtnet_free_queues(struct zxdh_en_device *en_dev)
{
    int32_t i = 0;
    uint16_t qpairs = 0;

    qpairs = en_dev->max_queue_pairs;
    for (i = 0; i < qpairs; i++)
    {
        netif_napi_del(&en_dev->rq[i].napi);
        netif_napi_del(&en_dev->sq[i].napi);
    }

    /* We called __netif_napi_del(),
     * we need to respect an RCU grace period before freeing zxdev->rq
     */
    synchronize_net();
}

void *virtqueue_detach_unused_buf_packed(struct virtqueue *_vq, bool get_id)
{
    struct vring_virtqueue *vq = to_vvq(_vq);
    uint32_t i = 0;
    void *buf = NULL;
    uint32_t _id = 0;

    START_USE(vq);

    for (i = 0; i < vq->packed.vring.num; i++)
    {
        if (!vq->packed.desc_state[i].data)
        {
            continue;
        }

        /* detach_buf clears data, so grab it now. */
        buf = vq->packed.desc_state[i].data;
        detach_buf_packed(vq, i, NULL, get_id ? &_id : NULL);
        if (get_id)
        {
            if(vq->packed.desc_extra[_id].last_in_page && vq->packed.desc_extra[_id].di->page)
            {
                dh_page_release_dynamic(_vq->rq, vq->packed.desc_extra[_id].di, false);
                vq->packed.desc_extra[_id].di->page = NULL;
            }
        }
        END_USE(vq);
        return buf;
    }

    /* That should have freed everything. */
    BUG_ON(vq->vq.num_free != vq->packed.vring.num);

    END_USE(vq);
    return NULL;
}

void *virtqueue_detach_unused_buf_split(struct virtqueue *_vq)
{
	struct vring_virtqueue *vq = to_vvq(_vq);
	unsigned int i;
	void *buf;

	START_USE(vq);

	for (i = 0; i < vq->split.vring.num; i++) {
		if (!vq->split.desc_state[i].data)
			continue;
		/* detach_buf_split clears data, so grab it now. */
		buf = vq->split.desc_state[i].data;
		detach_buf_split(vq, i, NULL);
		vq->split.avail_idx_shadow--;
		vq->split.vring.avail->idx = cpu_to_vqm16(_vq->en_dev,
				vq->split.avail_idx_shadow);
		END_USE(vq);
		return buf;
	}
	/* That should have freed everything. */
	BUG_ON(vq->vq.num_free != vq->split.vring.num);

	END_USE(vq);
	return NULL;
}

void *virtqueue_detach_unused_buf(struct virtqueue *_vq, bool get_id)
{
	struct vring_virtqueue *vq = to_vvq(_vq);

	return vq->packed_ring ? virtqueue_detach_unused_buf_packed(_vq, get_id) :
				 virtqueue_detach_unused_buf_split(_vq);
}

void zxdh_free_unused_bufs(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct virtqueue *vq = NULL;
    void *buf = NULL;
    int32_t i = 0;

    for (i = 0; i < en_dev->max_queue_pairs; i++)
    {
        vq = en_dev->sq[i].vq;
        while ((buf = virtqueue_detach_unused_buf(vq, false)) != NULL)
        {
        #ifdef CGS_V5_693
            dev_kfree_skb(buf);
        #else
            if (!is_xdp_frame(buf))
                dev_kfree_skb(buf);
            else
                xdp_return_frame(ptr_to_xdp(buf));
        #endif
        }
    }

    for (i = 0; i < en_dev->max_queue_pairs; i++)
    {
        vq = en_dev->rq[i].vq;
        if (en_dev->packed_status)
        {
            while ((buf = virtqueue_detach_unused_buf(vq, true)) != NULL)
            {
                /* nothing to do */
            }//每8k睡眠一次
            dh_rx_free_page_cache(&en_dev->rq[i]);
        }
        else
        {
            vq = en_dev->rq[i].vq;
            while ((buf = virtqueue_detach_unused_buf(vq, true)) != NULL)
            {
                put_page(virt_to_head_page(buf));
            }
        }
    }
}

struct page *get_a_page(struct receive_queue *rq, gfp_t gfp_mask)
{
    struct page *p = rq->pages;

    if (p)
    {
        rq->pages = (struct page *)p->private;
        /* clear private here, it is used to chain pages */
        p->private = 0;
    }
    else
    {
        p = alloc_page(gfp_mask);
    }
    return p;
}

void _free_receive_bufs(struct zxdh_en_device *en_dev)
{
#ifndef CGS_V5_693
    struct bpf_prog *old_prog = NULL;
#endif
    int32_t i = 0;

    for (i = 0; i < en_dev->max_queue_pairs; i++)
    {
        while (en_dev->rq[i].pages)
        {
            __free_pages(get_a_page(&en_dev->rq[i], GFP_KERNEL), 0);
#ifndef CGS_V5_693
            old_prog = rtnl_dereference(en_dev->rq[i].xdp_prog);
            RCU_INIT_POINTER(en_dev->rq[i].xdp_prog, NULL);
            if (old_prog)
                bpf_prog_put(old_prog);
#endif
        }
    }
}

void zxdh_free_receive_bufs(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    rtnl_lock();
    _free_receive_bufs(en_dev);
    rtnl_unlock();
}

void zxdh_free_receive_page_frags(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t i = 0;

    for (i = 0; i < en_dev->max_queue_pairs; i++)
    {
        if (en_dev->rq[i].alloc_frag.page)
        {
            put_page(en_dev->rq[i].alloc_frag.page);
        }
    }
}

void zxdh_virtnet_del_vqs(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    vp_del_vqs(netdev);
    en_dev->ops->vqs_unbind_eqs(en_dev->parent, (en_dev->channels_num - 1));
    en_dev->ops->vqs_channel_unbind_handler(en_dev->parent, (en_dev->channels_num - 1));
    virtnet_free_queues(en_dev);
}

void zxdh_sec_release_vqs(struct net_device *netdev, struct zxdh_sec_info *sec_info, uint8_t qidx)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    en_dev->ops->vp_modern_unmap_vq_notify(en_dev->parent, (void __iomem __force *)sec_info[qidx].notify_phy_addr);
    dma_free_coherent(en_dev->dmadev, sec_info[qidx].event_size_in_bytes, sec_info[qidx].device, sec_info[qidx].device_event_dma_addr);
    dma_free_coherent(en_dev->dmadev, sec_info[qidx].event_size_in_bytes, sec_info[qidx].driver, sec_info[qidx].driver_event_dma_addr);
    dma_free_coherent(en_dev->dmadev, sec_info[qidx].ring_size_in_bytes, sec_info[qidx].desc, sec_info[qidx].ring_dma_addr);

    return;
}

int8_t zxdh_sec_create_vqs(struct net_device *netdev, struct zxdh_sec_info *sec_info, uint8_t qidx)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct vring_packed_desc *ring = NULL;
    struct vring_packed_desc_event *driver = NULL;
    struct vring_packed_desc_event *device = NULL;
    dma_addr_t ring_dma_addr;
    dma_addr_t driver_event_dma_addr;
    dma_addr_t device_event_dma_addr;
    size_t ring_size_in_bytes;
    size_t event_size_in_bytes;
    void *notify_addr = NULL;

    ring_size_in_bytes = ZXDH_SEC_MIN_DESC_NUM * sizeof(struct vring_packed_desc);
    ring = dma_alloc_coherent(en_dev->dmadev, ring_size_in_bytes, &ring_dma_addr, GFP_KERNEL|__GFP_NOWARN|__GFP_ZERO);
    if (unlikely(ring == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "ring dma_alloc_coherent failed\n");
        goto err_ring;
    }

    event_size_in_bytes = sizeof(struct vring_packed_desc_event);
    driver = dma_alloc_coherent(en_dev->dmadev, event_size_in_bytes, &driver_event_dma_addr, GFP_KERNEL|__GFP_NOWARN|__GFP_ZERO);
    if (unlikely(driver == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "driver dma_alloc_coherent failed\n");
        goto err_driver;
    }
    device = dma_alloc_coherent(en_dev->dmadev, event_size_in_bytes, &device_event_dma_addr, GFP_KERNEL|__GFP_NOWARN|__GFP_ZERO);
    if (unlikely(device == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "device dma_alloc_coherent failed\n");
        goto err_device;
    }

    en_dev->ops->activate_phy_vq(en_dev->parent, en_dev->sec_phy_index[qidx], en_dev->logic_index_split[qidx], ZXDH_SEC_MIN_DESC_NUM,
                                 ring_dma_addr, driver_event_dma_addr, device_event_dma_addr);
    notify_addr = (void __force *)en_dev->ops->vp_modern_map_vq_notify(en_dev->parent, en_dev->sec_phy_index[qidx], en_dev->logic_index_split[qidx], &en_dev->notify_phy_addr);
    if (unlikely(notify_addr == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "vp_modern_map_vq_notify failed\n");
        goto err_map_notify;
    }
    en_dev->ops->vq_unbind_channel(en_dev->parent, en_dev->sec_phy_index[qidx], en_dev->logic_index_split[qidx]);
    en_dev->ops->set_queue_enable(en_dev->parent, en_dev->sec_phy_index[qidx], en_dev->logic_index_split[qidx], true);

    sec_info[qidx].ring_dma_addr = ring_dma_addr;
    sec_info[qidx].driver_event_dma_addr = driver_event_dma_addr;
    sec_info[qidx].device_event_dma_addr = device_event_dma_addr;

    sec_info[qidx].desc = ring;
    sec_info[qidx].driver = driver;
    sec_info[qidx].device = device;

    sec_info[qidx].ring_size_in_bytes = ring_size_in_bytes;
    sec_info[qidx].event_size_in_bytes = event_size_in_bytes;

    sec_info[qidx].desc_num = ZXDH_SEC_MIN_DESC_NUM;
    sec_info[qidx].queue_pairs = ZXDH_SEC_QUEUES_NUM(en_dev) / 2;
    sec_info[qidx].phy_index = en_dev->sec_phy_index[qidx];
    sec_info[qidx].notify_phy_addr = en_dev->notify_phy_addr;

    sec_info[qidx].bar0_phy_addr = en_dev->ops->get_bar_phy_addr(en_dev->parent, 0);
    sec_info[qidx].bar0_vir_addr = en_dev->ops->get_bar_virt_addr(en_dev->parent, 0);
    sec_info[qidx].bar0_size = en_dev->ops->get_bar_size(en_dev->parent, 0);
    sec_info[qidx].pcie_id = en_dev->ops->get_pcie_id(en_dev->parent);
    sec_info[qidx].pdev = en_dev->ops->get_pdev(en_dev->parent);

    return 0;

err_map_notify:
    dma_free_coherent(en_dev->dmadev, event_size_in_bytes, device, device_event_dma_addr);
err_device:
    dma_free_coherent(en_dev->dmadev, event_size_in_bytes, driver, driver_event_dma_addr);
err_driver:
    dma_free_coherent(en_dev->dmadev, ring_size_in_bytes, ring, ring_dma_addr);
err_ring:
    return -1;
}

void zxdh_sec_vqs_uninit(struct net_device *netdev, uint8_t qidx)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint8_t i = 0;

    for (i = 0; i < qidx; i++)
    {
        zxdh_sec_release_vqs(netdev, en_dev->sec_info, i);
    }

    return;
}

int32_t zxdh_sec_vqs_init(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint8_t qidx = 0;
    int32_t err = 0;

    en_dev->sec_info = kzalloc(sizeof(struct zxdh_sec_info) * ZXDH_SEC_QUEUES_NUM(en_dev), GFP_KERNEL);
    if (unlikely(en_dev->sec_info == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "sec_info kzalloc failed\n");
        return -1;
    }

    err = vp_get_phy_vqs(netdev, ZXDH_SEC_QUEUES_NUM(en_dev), en_dev->sec_phy_index, "sec");
    if (err < 0)
    {
        LOG_ERR_DEV(en_dev->parent, "get vq phy lock failed!");
        goto err_find_vq;
    }

    for (qidx = 0; qidx < ZXDH_SEC_QUEUES_NUM(en_dev); qidx++)
    {
        err = zxdh_sec_create_vqs(netdev, en_dev->sec_info, qidx);
        if(err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_sec_create_vqs failed: %d\n", err);
            goto err_create_vqs;
        }
    }

    en_dev->ops->set_sec_info(en_dev->parent, en_dev->sec_info);

    return 0;

err_create_vqs:
    zxdh_sec_vqs_uninit(netdev, qidx);
err_find_vq:
    kfree(en_dev->sec_info);
    en_dev->sec_info = NULL;
    return -1;
}

void zxdh_vqs_uninit(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if (en_dev->device_state != ZXDH_DEVICE_STATE_INTERNAL_ERROR &&
        !en_dev->quick_remove) {
        zxdh_vp_reset(netdev);
    }

    cancel_delayed_work_sync(&en_dev->refill);
    zxdh_free_unused_bufs(netdev);
    zxdh_free_receive_bufs(netdev);
    zxdh_free_receive_page_frags(netdev);
    zxdh_free_hdr_buf(en_dev);
    zxdh_virtnet_del_vqs(netdev);
}

void zxdh_set_default_xps_cpumasks(struct zxdh_en_device *en_dev)
{
    uint16_t queue_pairs = en_dev->eth_config.num_txq;
    cpumask_var_t xps_mask;
    int i;
    int numa = dev_to_node(en_dev->dmadev);

    if (queue_pairs == 0)
    {
        LOG_INFO_DEV(en_dev->parent, "en_dev->eth_config.num_txq is 0\n");
        return;
    }

    if (!zalloc_cpumask_var(&xps_mask, GFP_KERNEL))
    {
        LOG_ERR_DEV(en_dev->parent, "zalloc_cpumask_var failed for xps_mask\n");
        return;
    }

    for (i = 0; i < queue_pairs; i++) {

        cpumask_set_cpu(cpumask_local_spread(i, numa), xps_mask);

        netif_set_xps_queue(en_dev->netdev, xps_mask, i);
        cpumask_clear(xps_mask);
    }

    free_cpumask_var(xps_mask);
}

int32_t zxdh_create_vqs(struct zxdh_en_device *en_dev)
{
    en_dev->max_queue_pairs = en_dev->max_vq_pairs;
    en_dev->curr_queue_pairs = en_dev->max_queue_pairs;

    en_dev->sq = kcalloc(en_dev->max_queue_pairs, sizeof(*en_dev->sq), GFP_KERNEL);
    if (unlikely(en_dev->sq == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "en_dev->sq kcalloc failed\n");
        return -ENOMEM;
    }

    en_dev->rq = kcalloc(en_dev->max_queue_pairs, sizeof(*en_dev->rq), GFP_KERNEL);
    if (unlikely(en_dev->rq == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "en_dev->rq kcalloc failed\n");
        kfree(en_dev->sq);
        en_dev->sq = NULL;
        return -ENOMEM;
    }

    return 0;
}

void zxdh_destroy_vqs(struct zxdh_en_device *en_dev)
{
    kfree(en_dev->sq);
    en_dev->sq = NULL;

    kfree(en_dev->rq);
    en_dev->rq = NULL;
}

int32_t zxdh_vqs_init(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t ret = 0;

    zxdh_netdev_features_over_dtp(netdev);

    en_dev->any_header_sg = zxdh_has_feature(en_dev, ZXDH_F_ANY_LAYOUT);
#ifndef CGS_V5_693
    en_dev->mergeable_rx_bufs = zxdh_has_feature(en_dev, ZXDH_NET_F_MRG_RXBUF);
#endif
    en_dev->netdev->needed_headroom = sizeof(struct zxdh_net_hdr_rx);
    if (!en_dev->packed_status)
    {
        en_dev->netdev->needed_headroom = sizeof(struct zxdh_split_hdr_rx);
    }
    en_dev->max_queue_pairs = en_dev->max_vq_pairs;
    en_dev->curr_queue_pairs = en_dev->max_queue_pairs;
    memset(en_dev->phy_index, 0xFF, sizeof(en_dev->phy_index));

    INIT_LIST_HEAD(&en_dev->vqs_list);
    spin_lock_init(&en_dev->vqs_list_lock);

    INIT_LIST_HEAD(&en_dev->virtqueues);
    spin_lock_init(&en_dev->lock);

    /* Allocate services send & receive queues */
    ret = virtnet_alloc_queues(en_dev);
    if (ret)
    {
        LOG_ERR_DEV(en_dev->parent, "virtnet_alloc_queues failed: %d\n", ret);
        return ret;
    }

    ret = zxdh_alloc_hdr_buf(en_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_alloc_hdr_buf failed\n");
        goto err_alloc_hdr_buf;
    }

    ret = virtnet_find_vqs(en_dev);
    if (ret)
    {
        LOG_ERR_DEV(en_dev->parent, "virtnet_find_vqs failed: %d\n", ret);
        goto err_find_vqs;
    }

    zxdh_set_default_xps_cpumasks(en_dev);

    rtnl_lock();
    netif_set_real_num_tx_queues(en_dev->netdev, en_dev->eth_config.num_txq);
    rtnl_unlock();
    rtnl_lock();
    netif_set_real_num_rx_queues(en_dev->netdev, en_dev->eth_config.num_rxq);
    rtnl_unlock();

    return 0;

err_find_vqs:
    zxdh_free_hdr_buf(en_dev);
err_alloc_hdr_buf:
    virtnet_free_queues(en_dev);
    return ret;
}
