/*

Copyright (c) 2018 Alex Forencich

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#ifndef GRTNIC_CORE_H
#define GRTNIC_CORE_H

#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
//#include <linux/version.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/pci.h>
#include <linux/dca.h>

#define CONFIG_KEYLIN_OS 1

//#ifndef HAVE_SWIOTLB_SKIP_CPU_SYNC
//#define HAVE_SWIOTLB_SKIP_CPU_SYNC //飞腾ARM平台可以强制打开提高性能
//#endif

#include "kcompat.h"
#include "dma_add.h"

#ifdef HAVE_NDO_BUSY_POLL
#include <net/busy_poll.h>
#define BP_EXTENDED_STATS
#endif

//#define CONFIG_DISABLE_PACKET_SPLIT //龙芯平台可以需要打开这个开关
#define GRTNIC_NO_LRO

#define DRIVER_NAME     "grtnic_xgb"
#define DRIVER_VERSION  "1.24.0711"

#define CHANNEL0_PORT_MASK 0x03

#define CHANNEL_NUM_MAX     (16)
#define GRTNIC_PORTS_MAX    (16)

#define GRTNIC_DEFAULT_TXD  (512)
#define GRTNIC_DEFAULT_TX_WORK    (256)
#define GRTNIC_DEFAULT_RXD  (512)

#define GRTNIC_MAX_NUM_DESCRIPTORS 4096
#define GRTNIC_MIN_NUM_DESCRIPTORS 64

/* Number of Transmit and Receive Descriptors must be a multiple of 8 */
#define GRTNIC_REQ_TX_DESCRIPTOR_MULTIPLE  8
#define GRTNIC_REQ_RX_DESCRIPTOR_MULTIPLE  8
#define GRTNIC_REQ_TX_BUFFER_GRANULARITY   1024


#define MAX_Q_VECTORS       10

/* Transmit and receive queues */
#define MAX_RX_QUEUES       8
#define MAX_TX_QUEUES       8

/////////////////////////////////////////////////////////////////////////////
#define XPHY_STATUS       (0x0000)
#define MAC_ADRS_FILTER   (0x0004)

#define DESIGN_STATUS     (0x0010)
#define IPXE_STATUS       (0x0014)
#define TEMP_STATUS       (0x0018)
#define SERIAL_NO         (0x001C)

//#define MAC_ADRS_FILTER   (0x0004) //redefine in macphy.h XXGE_AFC_OFFSET
//#define MAC0_ADRS_LOW     (0x0018) //redefine in macphy.h XXGE_MACADDR_OFFSET

#define MAC_ADRS_ID       (0x0020)
#define MAC_ADRS_LOW     	(0x0024)
#define MAC_ADRS_HIGH    	(0x0028)

#define PHY_TX_DISABLE    (0x0040)
#define MAC_LED_CTL       (0x0044)
#define MAX_LED_PKT_NUM   (0x0048)

#define I2CCTL            (0x0050)
#define ASIC_BOOT         (0x0054)
#define FLASH_CMD         (0x0058)

#define ASIC_RX_FIFO_RST  (0x0064)
#define ASIC_TX_FIFO_RST  (0x0068)

#define FC_WATERMARK      (0x0070)
#define ETH_TX_PAUSE      (0x0074)
#define CSUM_ENABLE       (0x008C)

#define MAC_HASH_TABLE_START  (0x0200)
#define MAC_HASH_TABLE_WR     (0x0204)
#define MAC_RX_OVERFLOW_FRAME (0x0210)


#define RSS_KEY_BEGIN  		(0x0300)
#define RSS_KEY_END     	(0x0324)

#define RSS_RETA_BEGIN  	(0x0330)
#define RSS_RETA_END     	(0x03AC)

#define FIRMWARE_CMD      (0x040C)

#define ETH_HIGH_MARK     (96)
#define ETH_LOW_MARK      (32)

//////////////////////////////////////////////////
#define MAX_JUMBO_FRAME_SIZE		0x3F00
/* The datasheet maximum supported RX size is 9.5KB (9728 bytes) */
#define MAX_RX_JUMBO_FRAME_SIZE		0x2600


/* Supported Rx Buffer Sizes */
#define GRTNIC_RXBUFFER_256       256  /* Used for skb receive header */
#define GRTNIC_RXBUFFER_1536 1536
#define GRTNIC_RXBUFFER_2K 2048
#define GRTNIC_RXBUFFER_3K 3072
#define GRTNIC_RXBUFFER_4K 4096
#ifdef CONFIG_DISABLE_PACKET_SPLIT
#define GRTNIC_RXBUFFER_7K 7168
#define GRTNIC_RXBUFFER_8K 8192
#define GRTNIC_RXBUFFER_15K  15360
#endif /* CONFIG_DISABLE_PACKET_SPLIT */
#define GRTNIC_MAX_RXBUFFER  16384  /* largest size for single descriptor */

/* Attempt to maximize the headroom available for incoming frames.  We
 * use a 2K buffer for receives and need 1536/1534 to store the data for
 * the frame.  This leaves us with 512 bytes of room.  From that we need
 * to deduct the space needed for the shared info and the padding needed
 * to IP align the frame.
 *
 * Note: For cache line sizes 256 or larger this value is going to end
 *   up negative.  In these cases we should fall back to the 3K
 *   buffers.
 */
#if (PAGE_SIZE < 8192)
#define GRTNIC_MAX_2K_FRAME_BUILD_SKB (GRTNIC_RXBUFFER_1536 - NET_IP_ALIGN)
#define GRTNIC_2K_TOO_SMALL_WITH_PADDING \
((NET_SKB_PAD + GRTNIC_RXBUFFER_1536) > SKB_WITH_OVERHEAD(GRTNIC_RXBUFFER_2K))

static inline int grtnic_compute_pad(int rx_buf_len)
{
  int page_size, pad_size;

  page_size = ALIGN(rx_buf_len, PAGE_SIZE / 2);
  pad_size = SKB_WITH_OVERHEAD(page_size) - rx_buf_len;

  return pad_size;
}

static inline int grtnic_skb_pad(void)
{
  int rx_buf_len;

  /* If a 2K buffer cannot handle a standard Ethernet frame then
   * optimize padding for a 3K buffer instead of a 1.5K buffer.
   *
   * For a 3K buffer we need to add enough padding to allow for
   * tailroom due to NET_IP_ALIGN possibly shifting us out of
   * cache-line alignment.
   */
  if (GRTNIC_2K_TOO_SMALL_WITH_PADDING)
    rx_buf_len = GRTNIC_RXBUFFER_3K + SKB_DATA_ALIGN(NET_IP_ALIGN);
  else
    rx_buf_len = GRTNIC_RXBUFFER_1536;

  /* if needed make room for NET_IP_ALIGN */
  rx_buf_len -= NET_IP_ALIGN;

  return grtnic_compute_pad(rx_buf_len);
}

#define GRTNIC_SKB_PAD grtnic_skb_pad()
#else //(PAGE_SIZE < 8192)
#define GRTNIC_SKB_PAD (NET_SKB_PAD + NET_IP_ALIGN)
#endif //!(PAGE_SIZE < 8192) 

/*
 * NOTE: netdev_alloc_skb reserves up to 64 bytes, NET_IP_ALIGN means we
 * reserve 64 more, and skb_shared_info adds an additional 320 bytes more,
 * this adds up to 448 bytes of extra data.
 *
 * Since netdev_alloc_skb now allocates a page fragment we can use a value
 * of 256 and the resultant skb will have a truesize of 960 or less.
 */
#define GRTNIC_RX_HDR_SIZE GRTNIC_RXBUFFER_256

#define GRTNIC_RX_BUFFER_WRITE 16 /* Must be power of 2 */

#ifdef HAVE_STRUCT_DMA_ATTRS
#define GRTNIC_RX_DMA_ATTR NULL
#else
#define GRTNIC_RX_DMA_ATTR \
  (DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)
#endif



#define MAX_EITR			0x00000FF8
#define MIN_EITR			8

/*
 * microsecond values for various ITR rates shifted by 2 to fit itr register
 * with the first 3 bits reserved 0
 */
#define GRTNIC_MIN_RSC_ITR	24
#define GRTNIC_100K_ITR		40
#define GRTNIC_20K_ITR		200
#define GRTNIC_16K_ITR		248
#define GRTNIC_12K_ITR		336

/* this is the size past which hardware will drop packets when setting LPE=0 */
#define MAXIMUM_ETHERNET_VLAN_SIZE 1522


/* Interrupt modes, as used by the IntMode parameter */
#define GRTNIC_INT_MODE_MSIX		0
#define GRTNIC_INT_MODE_MSI			1
#define GRTNIC_INT_MODE_LEGACY	2


/* obtain the 32 most significant (high) bits of a 32-bit or 64-bit address */
#define PCI_DMA_H(addr) ((addr >> 16) >> 16)
/* obtain the 32 least significant (low) bits of a 32-bit or 64-bit address */
#define PCI_DMA_L(addr) (addr & 0xffffffffUL)

#define TX_INT_DELAY 32
#define RX_INT_DELAY 32

#define GRTNIC_TXDCTL_DMA_BURST_ENABLE                          \
   (0x00000000 | /* set descriptor granularity */         \
   (1u << 25) | /* LWTHRESH */                            \
   (8u << 16) | /* wthresh must be +1 more than desired */\
   (1u << 8)  | /* hthresh */                             \
   0x20)        /* pthresh */

#define GRTNIC_RXDCTL_DMA_BURST_ENABLE                          \
   (0x00000000 | /* set descriptor granularity */         \
   (1u << 25) | /* LWTHRESH */                            \
   (8u << 16) | /* set writeback threshold    */          \
   (4u << 8)  | /* set Hrefetch threshold     */          \
   0x20)        /* set Pthresh                */

enum grt_gigeth_boards {
  board_902E_GRT_FF,
  board_902T_GRT_FF,
  board_901ELR_GRT_FF,
	board_1001E_GRT_FF,
	board_1001E_QM_FF,
  board_1002E_GRT_FF,
	board_1005E_GRT_FX
};

struct grt_gigeth_info {
	enum grt_gigeth_boards type;
	int dma_channel_max;
	unsigned char port_type;	//0 for FIBER; 1 for COPPER
  unsigned char port_speed;  //0 for 1G; 1 for 10G
};

extern const struct grt_gigeth_info grt_902eff_info;
extern const struct grt_gigeth_info grt_902tff_info;
extern const struct grt_gigeth_info grt_901elr_info;
extern const struct grt_gigeth_info grt_1001eff_info;
extern const struct grt_gigeth_info qm_1001eff_info;
extern const struct grt_gigeth_info grt_1002eff_info;
extern const struct grt_gigeth_info grt_1005efx_info;

/* Direct Cache Access (DCA) definitions */
#define GRTNIC_DCA_CTRL_DCA_ENABLE	0x00000000 /* DCA Enable */
#define GRTNIC_DCA_CTRL_DCA_DISABLE	0x00000001 /* DCA Disable */

#define GRTNIC_DCA_CTRL_DCA_MODE_CB1	0x00 /* DCA Mode CB1 */
#define GRTNIC_DCA_CTRL_DCA_MODE_CB2	0x02 /* DCA Mode CB2 */

#define GRTNIC_DCA_RXCTRL_CPUID_MASK	0xFF000000 /* Rx CPUID Mask */
#define GRTNIC_DCA_RXCTRL_CPUID_SHIFT	24 /* Rx CPUID Shift */
#define GRTNIC_DCA_RXCTRL_DESC_DCA_EN	(1 << 5) /* Rx Desc enable */
#define GRTNIC_DCA_RXCTRL_HEAD_DCA_EN	(1 << 6) /* Rx Desc header ena */
#define GRTNIC_DCA_RXCTRL_DATA_DCA_EN	(1 << 7) /* Rx Desc payload ena */
#define GRTNIC_DCA_RXCTRL_DESC_RRO_EN	(1 << 9) /* Rx rd Desc Relax Order */
#define GRTNIC_DCA_RXCTRL_DATA_WRO_EN	(1 << 13) /* Rx wr data Relax Order */
#define GRTNIC_DCA_RXCTRL_HEAD_WRO_EN	(1 << 15) /* Rx wr header RO */

#define GRTNIC_DCA_TXCTRL_CPUID_MASK	0xFF000000 /* Tx CPUID Mask */
#define GRTNIC_DCA_TXCTRL_CPUID_SHIFT	24 /* Tx CPUID Shift */
#define GRTNIC_DCA_TXCTRL_DESC_DCA_EN	(1 << 5) /* DCA Tx Desc enable */
#define GRTNIC_DCA_TXCTRL_DESC_RRO_EN	(1 << 9) /* Tx rd Desc Relax Order */
#define GRTNIC_DCA_TXCTRL_DESC_WRO_EN	(1 << 11) /* Tx Desc writeback RO bit */
#define GRTNIC_DCA_TXCTRL_DATA_RRO_EN	(1 << 13) /* Tx rd data Relax Order */

/* iterator for handling rings in ring container */
#define grtnic_for_each_ring(pos, head) \
  for (pos = (head).ring; pos != NULL; pos = pos->next)

#define GRTNIC_TIDV_FPD BIT(31)
#define GRTNIC_RDTR_FPD BIT(31)

#define GRTNIC_GET_DESC(R, i, type)  (&(((union type *)((R).desc))[i]))
#define GRTNIC_TX_DESC(R, i)   GRTNIC_GET_DESC(R, i, grtnic_tx_desc)
#define GRTNIC_RX_DESC(R, i)   GRTNIC_GET_DESC(R, i, grtnic_rx_desc)

#define GRTNIC_MAX_JUMBO_FRAME_SIZE  65536+18

#define GRTNIC_DEAD_READ_RETRIES 10
#define GRTNIC_DEAD_READ_REG 0xdeadbeefU
#define GRTNIC_FAILED_READ_REG 0xffffffffU
#define GRTNIC_FAILED_READ_RETRIES 5


//static inline void write_register(u32 value, void *iomem)
//{
//  iowrite32(value, iomem);
//}
//
//static inline u32 read_register(void *iomem)
//{
//  return ioread32(iomem);
//}

static inline bool grtnic_removed(void __iomem *addr)
{
  return unlikely(!addr);
}
#define GRTNIC_REMOVED(a) grtnic_removed(a)

//////////////////////////////////////////////////////////////////////////////
/* wrapper around a pointer to a socket buffer,
 * so a DMA handle can be stored along with the buffer */
struct grtnic_tx_buffer {
  union grtnic_tx_desc *next_to_watch;
  unsigned long time_stamp;
  struct sk_buff *skb;
  unsigned int bytecount;
  unsigned short gso_segs;
  __be16 protocol;
  DEFINE_DMA_UNMAP_ADDR(dma);
  DEFINE_DMA_UNMAP_LEN(len);
  u32 tx_flags;
};

struct grtnic_rx_buffer {
  struct sk_buff *skb;
  dma_addr_t dma;
  unsigned int in_port;
  u32 length;
#ifndef CONFIG_DISABLE_PACKET_SPLIT
  struct page *page;
#if (BITS_PER_LONG > 32) || (PAGE_SIZE >= 65536)
  __u32 page_offset;
#else
  __u16 page_offset;
#endif
  __u16 pagecnt_bias;
#endif
};

struct grtnic_queue_stats {
  u64 packets;
  u64 bytes;
#ifdef BP_EXTENDED_STATS
  u64 yields;
  u64 misses;
  u64 cleaned;
#endif  /* BP_EXTENDED_STATS */
};

struct grtnic_tx_queue_stats {
  u64 restart_queue;
  u64 tx_busy;
  u64 tx_done_old;
};

struct grtnic_rx_queue_stats {
  u64 rsc_count;
  u64 rsc_flush;
  u64 non_eop_descs;
  u64 alloc_rx_page;
  u64 alloc_rx_page_failed;
  u64 alloc_rx_buff_failed;
  u64 csum_err;
};

/* Statistics counters collected by the MAC */
struct grtnic_hw_stats {
  u64 crcerrs;
  u64 algnerrc;
  u64 scc;
  u64 mcc;
  u64 mpc;
  u64 ecol;
  u64 latecol;
  u64 dc;
  u64 rlec;
  u64 rxpause;
  u64 txpause;
  u64 tx_underrun;
  u64 badopcode;
  u64 prc64;
  u64 prc127;
  u64 prc255;
  u64 prc511;
  u64 prc1023;
  u64 prc1522;
  u64 prcoversize;
  u64 gprc;
  u64 bprc;
  u64 mprc;
  u64 gptc;
  u64 gorc;
  u64 gotc;
  u64 ruc;
  u64 rfc;
  u64 roc;
  u64 ptc64;
  u64 ptc127;
  u64 ptc255;
  u64 ptc511;
  u64 ptc1023;
  u64 ptc1522;
  u64 ptcoversize;
  u64 mptc;
  u64 bptc;
};

enum grtnic_ring_state_t {
#ifndef CONFIG_DISABLE_PACKET_SPLIT
  __GRTNIC_RX_3K_BUFFER,
  __GRTNIC_RX_BUILD_SKB_ENABLED,
#endif
  __GRTNIC_RX_RSC_ENABLED,
  __GRTNIC_RX_CSUM_UDP_ZERO_ERR,
#if IS_ENABLED(CONFIG_FCOE)
  __GRTNIC_RX_FCOE,
#endif
  __GRTNIC_TX_FDIR_INIT_DONE,
  __GRTNIC_TX_XPS_INIT_DONE,
  __GRTNIC_TX_DETECT_HANG,
  __GRTNIC_HANG_CHECK_ARMED,
  __GRTNIC_TX_XDP_RING,
#ifdef HAVE_AF_XDP_ZC_SUPPORT
  __GRTNIC_TX_DISABLED,
#endif
};

#ifndef CONFIG_DISABLE_PACKET_SPLIT

#define ring_uses_build_skb(ring) \
  test_bit(__GRTNIC_RX_BUILD_SKB_ENABLED, &(ring)->state)
#endif

#define check_for_tx_hang(ring) \
  test_bit(__GRTNIC_TX_DETECT_HANG, &(ring)->state)
#define set_check_for_tx_hang(ring) \
  set_bit(__GRTNIC_TX_DETECT_HANG, &(ring)->state)
#define clear_check_for_tx_hang(ring) \
  clear_bit(__GRTNIC_TX_DETECT_HANG, &(ring)->state)
#define ring_is_rsc_enabled(ring) \
  test_bit(__GRTNIC_RX_RSC_ENABLED, &(ring)->state)
#define set_ring_rsc_enabled(ring) \
  set_bit(__GRTNIC_RX_RSC_ENABLED, &(ring)->state)
#define clear_ring_rsc_enabled(ring) \
  clear_bit(__GRTNIC_RX_RSC_ENABLED, &(ring)->state)
#define netdev_ring(ring) (ring->netdev)
#define ring_queue_index(ring) (ring->queue_index)

struct grtnic_ring {
  struct grtnic_ring *next;  /* pointer to next ring in q_vector */
  struct grtnic_q_vector *q_vector;  /* backlink to q_vector */
  struct net_device     *netdev;      /* back pointer to net_device */
  struct device         *dev;             /* device for dma mapping */

  void                  *desc;      /* pointer to ring memory  */
  dma_addr_t            dma;        /* phys address of ring    */

  void                  *desc_wb;   /* pointer to desc writeback memory  */
  dma_addr_t            desc_wb_dma;/* phys address of desc writeback memory */

  struct sk_buff        *skb;
  
  union {
    struct grtnic_tx_buffer *tx_buffer_info;
    struct grtnic_rx_buffer *rx_buffer_info;
  };
  unsigned long         state;
  u8 __iomem *tail;
 
  unsigned int          size;       /* length of ring in bytes */
  u16                   count;      /* number of desc. in ring */

  u8                    queue_index; /* logical index of the ring*/
  u8                    reg_idx;     /* physical index of the ring */

  u16                   next_to_use;
  u16                   next_to_clean;

#ifndef CONFIG_DISABLE_PACKET_SPLIT
  u16                   next_to_alloc;
#endif

//#ifdef CONFIG_DISABLE_PACKET_SPLIT
  u16                   rx_buffer_len;
//#endif

  struct grtnic_queue_stats stats;
#ifdef HAVE_NDO_GET_STATS64
  struct u64_stats_sync syncp;
#endif
  union {
    struct grtnic_tx_queue_stats tx_stats;
    struct grtnic_rx_queue_stats rx_stats;
  };

} ____cacheline_internodealigned_in_smp;


#ifndef CONFIG_DISABLE_PACKET_SPLIT
/*
 * FCoE requires that all Rx buffers be over 2200 bytes in length.  Since
 * this is twice the size of a half page we need to double the page order
 * for FCoE enabled Rx queues.
 */
static inline unsigned int grtnic_rx_bufsz(struct grtnic_ring __maybe_unused *ring)
{
#if MAX_SKB_FRAGS < 8
  return ALIGN(GRTNIC_MAX_RXBUFFER / MAX_SKB_FRAGS, 1024);
#else
  if (test_bit(__GRTNIC_RX_3K_BUFFER, &ring->state))
    return GRTNIC_RXBUFFER_3K;
#if (PAGE_SIZE < 8192)
  if (ring_uses_build_skb(ring))
    return GRTNIC_MAX_2K_FRAME_BUILD_SKB;
#endif
  return GRTNIC_RXBUFFER_2K;
#endif
}

static inline unsigned int grtnic_rx_pg_order(struct grtnic_ring __maybe_unused *ring)
{
#if (PAGE_SIZE < 8192)
  if (test_bit(__GRTNIC_RX_3K_BUFFER, &ring->state))
    return 1;
#endif
  return 0;
}
#define grtnic_rx_pg_size(_ring) (PAGE_SIZE << grtnic_rx_pg_order(_ring))

#endif //CONFIG_DISABLE_PACKET_SPLIT

#define ITR_ADAPTIVE_MIN_INC  2
#define ITR_ADAPTIVE_MIN_USECS  10
#define ITR_ADAPTIVE_MAX_USECS  84
#define ITR_ADAPTIVE_LATENCY  0x80
#define ITR_ADAPTIVE_BULK   0x00
#define ITR_ADAPTIVE_MASK_USECS (ITR_ADAPTIVE_LATENCY - ITR_ADAPTIVE_MIN_INC)

struct grtnic_ring_container {
  struct grtnic_ring *ring;    /* pointer to linked list of rings */
  unsigned long next_update;  /* jiffies value of last update */
  unsigned int total_bytes; /* total bytes processed this int */
  unsigned int total_packets; /* total packets processed this int */
  u16 work_limit;     /* total work allowed per interrupt */
  u8 count;     /* total number of rings in vector */
  u8 itr;       /* current ITR setting for ring */
};


/* MAX_MSIX_Q_VECTORS of these are allocated,
 * but we only use one per queue-specific vector.
 */
struct grtnic_q_vector {
  struct grtnic_adapter *adapter;
  int cpu;  /* CPU for DCA */
  u16 v_idx;  /* index of q_vector within array, also used for
       * finding the bit in EICR and friends that
       * represents the vector for this ring */

  u32 eims_value;     /* EIMS mask value */
  u16 itr;  /* Interrupt throttle rate written to EITR */
  struct grtnic_ring_container rx, tx;

  struct napi_struct napi;
#ifndef HAVE_NETDEV_NAPI_LIST
  struct net_device poll_dev;
#endif
#ifdef HAVE_IRQ_AFFINITY_HINT
  cpumask_t affinity_mask;
#endif
  int node;
  struct rcu_head rcu;  /* to avoid race with update stats on free */
  char name[IFNAMSIZ + 9];
  bool netpoll_rx;

#ifdef HAVE_NDO_BUSY_POLL
  atomic_t state;
#endif  /* HAVE_NDO_BUSY_POLL */

  /* for dynamic allocation of rings associated with this q_vector */
  struct grtnic_ring ring[0] ____cacheline_internodealigned_in_smp;
};


#ifdef HAVE_NDO_BUSY_POLL
enum grtnic_qv_state_t {
  GRTNIC_QV_STATE_IDLE = 0,
  GRTNIC_QV_STATE_NAPI,
  GRTNIC_QV_STATE_POLL,
  GRTNIC_QV_STATE_DISABLE
};

static inline void grtnic_qv_init_lock(struct grtnic_q_vector *q_vector)
{
  /* reset state to idle */
  atomic_set(&q_vector->state, GRTNIC_QV_STATE_IDLE);
}

/* called from the device poll routine to get ownership of a q_vector */
static inline bool grtnic_qv_lock_napi(struct grtnic_q_vector *q_vector)
{
  int rc = atomic_cmpxchg(&q_vector->state, GRTNIC_QV_STATE_IDLE, GRTNIC_QV_STATE_NAPI);
#ifdef BP_EXTENDED_STATS
  if (rc != GRTNIC_QV_STATE_IDLE)
    q_vector->tx.ring->stats.yields++;
#endif

  return rc == GRTNIC_QV_STATE_IDLE;
}

/* returns true is someone tried to get the qv while napi had it */
static inline void grtnic_qv_unlock_napi(struct grtnic_q_vector *q_vector)
{
  WARN_ON(atomic_read(&q_vector->state) != GRTNIC_QV_STATE_NAPI);

  /* flush any outstanding Rx frames */
  if (q_vector->napi.gro_list)
    napi_gro_flush(&q_vector->napi, false);

  /* reset state to idle */
  atomic_set(&q_vector->state, GRTNIC_QV_STATE_IDLE);
}

/* called from ixgbe_low_latency_poll() */
static inline bool grtnic_qv_lock_poll(struct grtnic_q_vector *q_vector)
{
  int rc = atomic_cmpxchg(&q_vector->state, GRTNIC_QV_STATE_IDLE, GRTNIC_QV_STATE_POLL);
#ifdef BP_EXTENDED_STATS
  if (rc != GRTNIC_QV_STATE_IDLE)
    q_vector->rx.ring->stats.yields++;
#endif
  return rc == GRTNIC_QV_STATE_IDLE;
}

/* returns true if someone tried to get the qv while it was locked */
static inline void grtnic_qv_unlock_poll(struct grtnic_q_vector *q_vector)
{
  WARN_ON(atomic_read(&q_vector->state) != GRTNIC_QV_STATE_POLL);

  /* reset state to idle */
  atomic_set(&q_vector->state, GRTNIC_QV_STATE_IDLE);
}

/* true if a socket is polling, even if it did not get the lock */
static inline bool grtnic_qv_busy_polling(struct grtnic_q_vector *q_vector)
{
  return atomic_read(&q_vector->state) == GRTNIC_QV_STATE_POLL;
}

/* false if QV is currently owned */
static inline bool grtnic_qv_disable(struct grtnic_q_vector *q_vector)
{
  int rc = atomic_cmpxchg(&q_vector->state, GRTNIC_QV_STATE_IDLE, GRTNIC_QV_STATE_DISABLE);

  return rc == GRTNIC_QV_STATE_IDLE;
}

#endif /* HAVE_NDO_BUSY_POLL */

enum grtnic_state_t {
  __GRTNIC_TESTING,
  __GRTNIC_RESETTING,
  __GRTNIC_DOWN,
  __GRTNIC_DISABLED,
  __GRTNIC_REMOVING,
  __GRTNIC_SERVICE_SCHED,
  __GRTNIC_SERVICE_INITED,
  __GRTNIC_IN_SFP_INIT,
#ifdef HAVE_PTP_1588_CLOCK
  __GRTNIC_PTP_RUNNING,
  __GRTNIC_PTP_TX_IN_PROGRESS,
#endif
  __GRTNIC_RESET_REQUESTED,
};

struct grtnic_cb {
#ifdef CONFIG_DISABLE_PACKET_SPLIT
  union {       /* Union defining head/tail partner */
    struct sk_buff *head;
    struct sk_buff *tail;
  };
#endif
  dma_addr_t dma;
#ifdef HAVE_VLAN_RX_REGISTER
  u16 vid;      /* VLAN tag */
#endif
  u16 append_cnt;   /* number of skb's appended */
#ifndef CONFIG_DISABLE_PACKET_SPLIT
  bool  page_released;
#endif
};
#define GRTNIC_CB(skb) ((struct grtnic_cb *)(skb)->cb)

enum latency_range {
  lowest_latency = 0,
  low_latency = 1,
  bulk_latency = 2,
  latency_invalid = 255
};

struct grtnic_ps_page {
  struct page *page;
  u64 dma; /* must be u64 - written to hw */
};


union grtnic_tx_desc {
  struct {
    __le64 src_addr; /* Address of descriptor's data buf */
    struct
    {
      u32 len:20;
      u32 desc_num:4;
      u32 chl     :3;
      u32 cmp:1;
      u32 rs :1;
      u32 irq:1;
      u32 eop:1;
      u32 sop:1;
    }len_ctl;
    struct
    {
      u32 csum_info:16;
      u32 reserved:12;
      u32 port:4;
    } tx_info;    /*user data */
  } read;

  struct {
    __le64 rsvd0; /* Reserved */
    struct
    {
      u32 len:20;
      u32 desc_num:4;
      u32 chl     :3;
      u32 cmp:1;
      u32 rs :1;
      u32 irq:1;
      u32 eop:1;
      u32 sop:1;
    }len_ctl;
    __le32 rsvd1;
  } wb;
};


union grtnic_rx_desc {
  struct {
    __le64 src_addr; /* Packet buffer address */
    struct
    {
      u32 len:20;
      u32 desc_num:4;
      u32 chl     :3;
      u32 cmp:1;
      u32 rs :1;
      u32 irq:1;
      u32 eop:1;
      u32 sop:1;
    }len_ctl;
    __le32 rsvd;
  } read;

  struct {
    struct {
      union {
        __le32 data;
        struct {
          __le16 pkt_info; /* RSS, Pkt type */
          __le16 hdr_info; /* Splithdr, hdrlen */
        } hs_rss;
      } lo_dword;
      union {
        __le32 rss; /* RSS Hash */
        struct {
          __le16 ip_id; /* IP id */
          __le16 csum; /* Packet Checksum */
        } csum_ip;
      } hi_dword;
    } lower;
    
    struct
    {
      struct
      {
        u32 len:20;
        u32 desc_num:4;
        u32 chl     :3;
        u32 cmp:1;
        u32 rs :1;
        u32 irq:1;
        u32 eop:1;
        u32 sop:1;
      }len_ctl;

      struct
      {
        u32 csum_ok:1;
        u32 ipcs:1;
        u32 tcpcs:1;
        u32 udpcs:1;
        u32 udp_csum_flag:1;
        u32 reserved:27;
      } rx_info;
    } upper;
  } wb;  /* writeback */
};

/////////////////////////////////////////////////////////////////////////////////////
#define GRTNIC_MAX_TXD_PWR 13
#define GRTNIC_MAX_DATA_PER_TXD  (1 << GRTNIC_MAX_TXD_PWR)

/* Tx Descriptors needed, worst case */
#define TXD_USE_COUNT(S)  DIV_ROUND_UP((S), GRTNIC_MAX_DATA_PER_TXD)
#ifndef MAX_SKB_FRAGS
#define DESC_NEEDED 4
#elif (MAX_SKB_FRAGS < 16)
#define DESC_NEEDED ((MAX_SKB_FRAGS * TXD_USE_COUNT(PAGE_SIZE)) + 4)
#else
#define DESC_NEEDED (MAX_SKB_FRAGS + 4)
#endif


//struct grtnic_buffer {
//  dma_addr_t dma;
//  struct sk_buff *skb;
//  unsigned int in_port;
//  unsigned long time_stamp;
//  u32 length;
//  u16 next_to_watch;
//  unsigned int segs;
//  unsigned int bytecount;
//  u16 mapped_as_page;
//#ifndef CONFIG_DISABLE_PACKET_SPLIT
//  struct page *page;
//#if (BITS_PER_LONG > 32) || (PAGE_SIZE >= 65536)
//  __u32 page_offset;
//#else
//  __u16 page_offset;
//#endif
//  __u16 pagecnt_bias;
//#endif
//};

struct grtnic_desc_wb {
  u32 desc_hw_ptr;
} __packed;

////////////////////////////////////////////////////////////////////////////////////
enum fc_mode {
  fc_none = 0,
  fc_rx_pause,
  fc_tx_pause,
  fc_full,
  fc_default = 0xFF
};

struct fc_info {
  u32 high_water;   /* Flow control high-water mark */
  u32 low_water;    /* Flow control low-water mark */
  u16 pause_time;   /* Flow control pause timer */
  u16 refresh_time; /* Flow control refresh timer */
  bool send_xon;    /* Flow control send XON */
  bool strict_ieee; /* Strict IEEE mode */
  bool fc_autoneg;
  enum fc_mode current_mode;  /* FC mode in effect */
  enum fc_mode requested_mode;  /* FC mode requested by caller */
};

struct grtnic_mac_info {
  u8 addr[ETH_ALEN];
  u8 perm_addr[ETH_ALEN];

  u32 mc_filter_type;
  u16 mta_reg_count;

  struct fc_info fc;

  /* Maximum size of the MTA register table in all supported adapters */
#define MAX_MTA_REG 128
  u32 mta_shadow[MAX_MTA_REG];
  u16 rar_entry_count;
};

struct grtnic_hw {
  // BAR pointers
  void * __iomem dma_bar;
  void * __iomem user_bar;
  resource_size_t dma_bar_len;
  resource_size_t user_bar_len;
  void *back;
  struct grtnic_mac_info  mac;
  bool adapter_stopped;
  u32 phy_addr;
};

/* default to trying for four seconds */
#define GRTNIC_TRY_LINK_TIMEOUT  (4 * HZ)

struct grtnic_adapter {
	struct device *dev;
	struct pci_dev *pdev;
	struct net_device *netdev;

  int func;

	/* Tx fast path data */
	int num_tx_queues;
	u16 tx_itr_setting;
	u16 tx_work_limit;

	/* Rx fast path data */
	int num_rx_queues;
	u16 rx_itr_setting;
	u16 rx_work_limit;

	/* TX */
  struct grtnic_ring *tx_ring[MAX_TX_QUEUES] ____cacheline_aligned_in_smp;

	u64 restart_queue;
	u64 lsc_int;
	u32 tx_timeout_count;
  u64 tx_busy;

	/* RX */
  struct grtnic_ring *rx_ring[MAX_RX_QUEUES];
  u64 hw_csum_rx_error;
  u64 non_eop_descs;
  u32 alloc_rx_page;
  u32 alloc_rx_page_failed;
  u32 alloc_rx_buff_failed;

	const struct grt_gigeth_info *ei;

  int rss_queues;
  int num_q_vectors;

  u8 ivar[MAX_Q_VECTORS];
  struct grtnic_q_vector *q_vector[MAX_Q_VECTORS];
  u32 eims_enable_mask;
  u32 eims_other;


	unsigned int id;

  struct proc_dir_entry *proc_dir; //for test
  u32 tx_count0;
  u32 tx_count1;
  u32 rx_count;

	struct msix_entry *msix_entries;
	int int_mode;

#ifdef ETHTOOL_TEST
  u32 test_icr;
  struct grtnic_ring test_tx_ring;
  struct grtnic_ring test_rx_ring;
#endif

  struct grtnic_hw hw;
  u16 msg_enable;

	unsigned int tx_ring_count;
	unsigned int rx_ring_count;

  u32 link_speed;
  bool link_up;

  unsigned long link_check_timeout;

  struct timer_list service_timer;
  struct work_struct service_task;

  u32 max_frame_size;
  u32 min_frame_size;

#ifndef HAVE_NETDEV_STATS_IN_NETDEV
  struct net_device_stats net_stats;
#endif

  struct grtnic_hw_stats stats;

//#ifdef ETHTOOL_GRXFHINDIR
//  u32 rss_indir_tbl_init;
//  u8 rss_indir_tbl[RETA_SIZE];
//#endif

#define GRTNIC_MAX_RETA_ENTRIES 512
  u8 rss_indir_tbl[GRTNIC_MAX_RETA_ENTRIES];

#define GRTNIC_RSS_KEY_SIZE     40  /* size of RSS Hash Key in bytes */
  u32 *rss_key;

  unsigned long         state;

  /* Some features need tri-state capability,
   * thus the additional *_CAPABLE flags.
   */
  u32 flags;
#define GRTNIC_FLAG_MSI_CAPABLE       (u32)(1 << 0)
#define GRTNIC_FLAG_MSI_ENABLED       (u32)(1 << 1)
#define GRTNIC_FLAG_MSIX_CAPABLE      (u32)(1 << 2)
#define GRTNIC_FLAG_MSIX_ENABLED      (u32)(1 << 3)

#define GRTNIC_FLAG_TXCSUM_CAPABLE    (u32)(1 << 4)
#define GRTNIC_FLAG_RXCSUM_CAPABLE    (u32)(1 << 5)

#if defined(CONFIG_DCA) || defined(CONFIG_DCA_MODULE)
#define GRTNIC_FLAG_DCA_ENABLED       (u32)(1 << 6)
#define GRTNIC_FLAG_DCA_CAPABLE       (u32)(1 << 7)
#define GRTNIC_FLAG_DCA_ENABLED_DATA  (u32)(1 << 8)
#else
#define GRTNIC_FLAG_DCA_ENABLED       (u32)0
#define GRTNIC_FLAG_DCA_CAPABLE       (u32)0
#define GRTNIC_FLAG_DCA_ENABLED_DATA  (u32)0
#endif
#define GRTNIC_FLAG_MQ_CAPABLE        (u32)(1 << 9)
#define GRTNIC_FLAG_DCB_ENABLED       (u32)(1 << 10)
#define GRTNIC_FLAG_VMDQ_ENABLED      (u32)(1 << 11)
#define GRTNIC_FLAG_FAN_FAIL_CAPABLE  (u32)(1 << 12)
#define GRTNIC_FLAG_NEED_LINK_UPDATE  (u32)(1 << 13)
#define GRTNIC_FLAG_NEED_LINK_CONFIG  (u32)(1 << 14)
#define GRTNIC_FLAG_FDIR_HASH_CAPABLE (u32)(1 << 15)
#define GRTNIC_FLAG_FDIR_PERFECT_CAPABLE   (u32)(1 << 16)
#if IS_ENABLED(CONFIG_FCOE)
#define GRTNIC_FLAG_FCOE_CAPABLE      (u32)(1 << 17)
#define GRTNIC_FLAG_FCOE_ENABLED      (u32)(1 << 18)
#endif /* CONFIG_FCOE */
#define GRTNIC_FLAG_SRIOV_CAPABLE     (u32)(1 << 19)
#define GRTNIC_FLAG_SRIOV_ENABLED     (u32)(1 << 20)
#define GRTNIC_FLAG_SRIOV_REPLICATION_ENABLE  (u32)(1 << 21)
#define GRTNIC_FLAG_SRIOV_L2SWITCH_ENABLE     (u32)(1 << 22)
#define GRTNIC_FLAG_SRIOV_VEPA_BRIDGE_MODE    (u32)(1 << 23)
#define GRTNIC_FLAG_RX_HWTSTAMP_ENABLED       (u32)(1 << 24)
#define GRTNIC_FLAG_VXLAN_OFFLOAD_CAPABLE     (u32)(1 << 25)
#define GRTNIC_FLAG_VXLAN_OFFLOAD_ENABLE      (u32)(1 << 26)
#define GRTNIC_FLAG_RX_HWTSTAMP_IN_REGISTER   (u32)(1 << 27)
#define GRTNIC_FLAG_MDD_ENABLED               (u32)(1 << 29)
#define GRTNIC_FLAG_DCB_CAPABLE               (u32)(1 << 30)
#define GRTNIC_FLAG_GENEVE_OFFLOAD_CAPABLE    BIT(31)

//  struct grtnic_mac_info  mac;
  int                     type;
  int                     speed;

  u16 bd_number;
  bool netdev_registered;
};

/* Error Codes */
#define GRTNIC_SUCCESS       0
#define GRTNIC_ERR_OUT_OF_MEM      -34


////////////////////////////////////////////////////////////////
#define DPRINTK(nlevel, klevel, fmt, args...) \
  ((NETIF_MSG_##nlevel & adapter->msg_enable) ? \
  (void)(netdev_printk(KERN_##klevel, adapter->netdev, \
  "%s: " fmt, __func__, ## args)) : NULL)

#define hw_err(hw, format, arg...) \
  netdev_err(ixgbe_hw_to_netdev(hw), format, ## arg)
#define e_dev_info(format, arg...) \
  dev_info(pci_dev_to_dev(adapter->pdev), format, ## arg)
#define e_dev_warn(format, arg...) \
  dev_warn(pci_dev_to_dev(adapter->pdev), format, ## arg)
#define e_dev_err(format, arg...) \
  dev_err(pci_dev_to_dev(adapter->pdev), format, ## arg)
#define e_dev_notice(format, arg...) \
  dev_notice(pci_dev_to_dev(adapter->pdev), format, ## arg)
#define e_dbg(msglvl, format, arg...) \
  netif_dbg(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_info(msglvl, format, arg...) \
  netif_info(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_err(msglvl, format, arg...) \
  netif_err(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_warn(msglvl, format, arg...) \
  netif_warn(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_crit(msglvl, format, arg...) \
  netif_crit(adapter, msglvl, adapter->netdev, format, ## arg)

static inline void GRTNIC_WRITE_REG(struct grtnic_hw *hw, u32 reg, u32 value, u8 bar)
{
  u8 __iomem *reg_addr;

  reg_addr = bar ? hw->dma_bar : hw->user_bar;
  if (GRTNIC_REMOVED(reg_addr))
    return;
  writel(value, reg_addr + reg);
}

#define GRTNIC_READ_REG(h, r, b) grtnic_read_reg(h, r, b) //hw, reg, bar
#define GRTNIC_WRITE_FLUSH(a) GRTNIC_READ_REG(a, XPHY_STATUS, 0)



//#ifdef CONFIG_BQL
static inline struct netdev_queue *txring_txq(const struct grtnic_ring *ring)
{
  return netdev_get_tx_queue(ring->netdev, ring->queue_index);
}
//#endif /* CONFIG_BQL */

u32 grtnic_read_reg(struct grtnic_hw *hw, u32 reg, u8 bar);

#ifdef GRTNIC_PROCFS
void grtnic_procfs_exit(struct grtnic_adapter *adapter);
int grtnic_procfs_init(struct grtnic_adapter *adapter);
int grtnic_procfs_topdir_init(void);
void grtnic_procfs_topdir_exit(void);
#endif /* GRTNIC_PROCFS */

  //main.c
void grtnic_write_itr (struct grtnic_q_vector *q_vector);
void grtnic_update_stats(struct grtnic_adapter *adapter);
void grtnic_down(struct grtnic_adapter *adapter);
void grtnic_assign_netdev_ops(struct net_device *netdev);
irqreturn_t grtnic_msix_other(int __always_unused irq, void *data);
irqreturn_t grtnic_msix_ring(int __always_unused irq, void *data);
irqreturn_t grtnic_isr (int __always_unused irq, void *data);
int grtnic_poll(struct napi_struct *napi, int budget);
void grtnic_close_suspend(struct grtnic_adapter *adapter);

void grtnic_check_options(struct grtnic_adapter *adapter); //in param.c

  //netdev.c
void grtnic_setup_mrqc(struct grtnic_adapter *adapter);
void grtnic_configure_msix(struct grtnic_adapter *adapter);
void grtnic_configure_msi_and_legacy(struct grtnic_adapter *adapter);
int grtnic_request_irq(struct grtnic_adapter *adapter);
void grtnic_irq_enable(struct grtnic_adapter *adapter);
void grtnic_irq_disable(struct grtnic_adapter *adapter);
void grtnic_free_irq(struct grtnic_adapter *adapter);
void grtnic_napi_enable_all(struct grtnic_adapter *adapter);
void grtnic_napi_disable_all(struct grtnic_adapter *adapter);
void grtnic_service_event_schedule(struct grtnic_adapter *adapter);
void grtnic_set_ethtool_ops(struct net_device *netdev);

//#ifdef ETHTOOL_OPS_COMPAT
//  int ethtool_ioctl(struct ifreq *ifr);
//#endif

  //ethtool.c
void grtnic_disable_rx_queue(struct grtnic_adapter *adapter);
void grtnic_disable_tx_queue(struct grtnic_adapter *adapter);
void grtnic_reset(struct grtnic_adapter *adapter);
void grtnic_do_reset(struct net_device *netdev);

void grtnic_configure_tx_ring(struct grtnic_adapter *adapter, struct grtnic_ring *ring);
void grtnic_configure_rx_ring(struct grtnic_adapter *adapter, struct grtnic_ring *ring);
void grtnic_alloc_rx_buffers(struct grtnic_ring *rx_ring, u16 cleaned_count);
netdev_tx_t grtnic_xmit_frame_ring (struct sk_buff *skb, struct grtnic_adapter __maybe_unused *adapter, struct grtnic_ring *tx_ring);

int grtnic_close(struct net_device *netdev);
int grtnic_open(struct net_device *netdev);

int grtnic_setup_tx_resources(struct grtnic_ring *tx_ring);
int grtnic_setup_rx_resources(struct grtnic_ring *rx_ring);
void grtnic_free_tx_resources(struct grtnic_ring *tx_ring);
void grtnic_free_rx_resources(struct grtnic_ring *rx_ring);
void grtnic_up(struct grtnic_adapter *adapter);
void grtnic_store_reta(struct grtnic_adapter *adapter);

u32 grtnic_rss_indir_tbl_entries(struct grtnic_adapter *adapter);
void grtnic_store_key(struct grtnic_adapter *adapter);

#endif /* GRTNIC_CORE_H */
