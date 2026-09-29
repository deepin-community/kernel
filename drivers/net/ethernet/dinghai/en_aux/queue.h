#ifndef __ZXDH_QUEUE_H__
#define __ZXDH_QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/netdevice.h>
#include <linux/filter.h>
#include <linux/average.h>
#ifdef CGS_V5_693
#include <asm/page.h>
#include <linux/mm_types.h>
#else
#include <linux/mm_types_task.h>
#include <linux/bpf.h>
#include <linux/bpf_trace.h>
#include <net/xdp.h>
#endif
#include <linux/printk.h>
#include <linux/dinghai/queue.h>
#include <linux/dinghai/kcompat.h>
#include <linux/dinghai/driver.h>

/*========================================================
 * 是否打开依赖PTP驱动的接口调用代码:
 *     在提交代码时候这里注释掉，在实际调试时需要打开。
 *=========================================================*/
// #define PTP_DRIVER_INTERFACE_EN

/**========================================================
 *  是否打开依赖os时间戳补丁接口的代码:
 *      在提交代码时候这里注释掉，在实际调试时需要打开。
 *=========================================================*/
/* #define CGEL_TSTAMP_2_PATCH_EN TODO 补丁不可用*/

#define ZXDH_CONFIG_SPECIAL_SQ_EN

/* 判断两个值是否相等，相等表示出错，打印信息后返回指定值 */
#define CHECK_EQUAL_ERR(a, b, c, fmt, arg...)   \
do {                                            \
    if (unlikely(a == b))                       \
    {                                           \
        LOG_ERR(fmt, ##arg);                    \
        return c;                               \
    }                                           \
} while(0)

/* 判断两个值是否不等，不等表示出错，打印信息后返回指定值 */
#define CHECK_UNEQUAL_ERR(a, b, c, fmt, arg...) \
do {                                            \
    if (unlikely(a != b))                       \
    {                                           \
        LOG_ERR(fmt, ##arg);                    \
        return c;                               \
    }                                           \
} while(0)

#define DEFAULT_FRAG_LENGTH 2048
#define ZXDH_MQ_PAIRS_NUM 8
#define ZXDH_PQ_PAIRS_NUM 1
#define ZXDH_MAX_PAIRS_NUM 128
#define ZXDH_BOND_ETH_MQ_PAIRS_NUM 1
#define ZXDH_MAX_QUEUES_NUM 4096
#define ZXDH_SEC_QUEUES_NUM(en_dev) (((256 - en_dev->max_queue_pairs * 2) >= 128) ? 128 : (256 - en_dev->max_queue_pairs * 2))
#define ZXDH_PF_MAX_BAR_VAL 0x5
#define ZXDH_PF_BAR0 0
#define ZXDH_PF_MAX_DESC_NUM(en_dev) ((en_dev->board_type == DH_INICD) ? 1024 : (16 * 1024))
#define ZXDH_PF_DEFAULT_DESC_NUM (1 * 1024)
#define ZXDH_PF_MIN_DESC_NUM (64)
#define ZXDH_SEC_MIN_DESC_NUM 1024
#define ZXDH_INDIR_RQT_SIZE 256
#define ZXDH_NET_HASH_KEY_SIZE 40
#define ZXDH_HAS_PI_FLAG 19 //38B, 2B unit, consider the receiving scenario of max 1588 packet
#define ZXDH_TYPE_FLAG_LEN 2
#define ZXDH_DESC_EXTRA_SIZE 512

#define VQM_HOST_BAR_OFFSET 0x0
#define ZXDH_VQ_TLB_OFFSET  0x1bf8
#define PHY_VQ_REG_OFFSET 0x5000
#define LOCK_VQ_REG_OFFSET 0x90
#define ZXDH_PHY_REG_BITS 32
#define ZXDH_PF_LOCK_ENABLE_MASK 0x1
#define ZXDH_PF_RELEASE_LOCK_VAL 0
#define ZXDH_PF_GET_PHY_INDEX_DONE 1
#define ZXDH_PF_GET_PHY_INDEX_BIT 1
#define ZXDH_PF_WAIT_COUNT 6000
#define ZXDH_PF_DELAY_US 100
#define ZXDH_PF_RQ_TYPE 0
#define ZXDH_PF_TQ_TYPE 1
#define ZXDH_PF_POWER_INDEX2 2

#define MSG_PAYLOAD_FIX_FIELD 8
#define MSG_CHAN_PF_MODULE_ID 0
#define MSG_PAYLOAD_TYPE_WRITE 1
#define MSG_PAYLOAD_FIELD_MSG_CHL 2
#define MSG_PAYLOAD_FIELD_DATA_CHL 3
#define MSG_PAYLOAD_MSG_CHL_SLEN 4
#define MSG_RECV_BUF_LEN 6

#define ZXDH_MAC_NUM 6
#define ZXDH_MAX_MTU 13500
#define ZXDH_DEFAULT_MTU 1500
#define DH_SKB_FRAG_PAGE_ORDER  get_order(32768)
#define DH_BUFF_LEN 2048

/* The feature bitmap for zxdh net */
#define ZXDH_NET_F_CSUM                  0    /* Host handles pkts w/ partial csum */
#define ZXDH_NET_F_GUEST_CSUM            1    /* Guest handles pkts w/ partial csum */
#define ZXDH_NET_F_CTRL_GUEST_OFFLOADS   2    /* Dynamic offload configuration. */
#define ZXDH_NET_F_MTU                   3    /* Initial MTU advice */
#define ZXDH_NET_F_MAC                   5    /* Host has given MAC address. */
#define ZXDH_NET_F_GUEST_TSO4            7    /* Guest can handle TSOv4 in. */
#define ZXDH_NET_F_GUEST_TSO6            8    /* Guest can handle TSOv6 in. */
#define ZXDH_NET_F_GUEST_ECN             9    /* Guest can handle TSO[6] w/ ECN in. */
#define ZXDH_NET_F_GUEST_UFO             10   /* Guest can handle UFO in. */
#define ZXDH_NET_F_HOST_TSO4             11   /* Host can handle TSOv4 in. */
#define ZXDH_NET_F_HOST_TSO6             12   /* Host can handle TSOv6 in. */
#define ZXDH_NET_F_HOST_ECN              13   /* Host can handle TSO[6] w/ ECN in. */
#define ZXDH_NET_F_HOST_UFO              14   /* Host can handle UFO in. */
#define ZXDH_NET_F_MRG_RXBUF             15   /* Host can merge receive buffers. */
#define ZXDH_NET_F_STATUS                16   /* net_config.status available */
#define ZXDH_NET_F_CTRL_VQ               17   /* Control channel available */
#define ZXDH_NET_F_MQ                    22   /* Device supports Receive Flow Steering */
#define ZXDH_F_ANY_LAYOUT                27   /* Can the device handle any descriptor layout? */
#define ZXDH_RING_F_INDIRECT_DESC        28   /* We support indirect buffer descriptors */

/* The Guest publishes the used index for which it expects an interrupt
 * at the end of the avail ring. Host should ignore the avail->flags field. */
/* The Host publishes the avail index for which it expects a kick
 * at the end of the used ring. Guest should ignore the used->flags field. */
#define ZXDH_RING_F_EVENT_IDX            29

#define ZXDH_F_VERSION_1                 32   /* v1.0 compliant */

/*
 * If clear - device has the platform DMA (e.g. IOMMU) bypass quirk feature.
 * If set - use platform DMA tools to access the memory.
 *
 * Note the reverse polarity (compared to most other features),
 * this is for compatibility with legacy systems.
 */
#define ZXDH_F_ACCESS_PLATFORM           33

/* This feature indicates support for the packed virtqueue layout. */
#define ZXDH_F_RING_PACKED               34

/*
 * This feature indicates that memory accesses by the driver and the
 * device are ordered in a way described by the platform.
 */
#define ZXDH_F_ORDER_PLATFORM            36

/* This marks a buffer as continuing via the next field. */
#define VRING_DESC_F_NEXT                1
/* This marks a buffer as write-only (otherwise read-only). */
#define VRING_DESC_F_WRITE               2
/* This means the buffer contains a list of buffer descriptors. */
#define VRING_DESC_F_INDIRECT            4

/*
 * Mark a descriptor as available or used in packed ring.
 * Notice: they are defined as shifts instead of shifted values.
 */
#define VRING_PACKED_DESC_F_AVAIL        7
#define VRING_PACKED_DESC_F_USED         15

/* The Host uses this in used->flags to advise the Guest: don't kick me when
 * you add a buffer.  It's unreliable, so it's simply an optimization.  Guest
 * will still kick if it's out of buffers. */
#define VRING_USED_F_NO_NOTIFY           1
/* The Guest uses this in avail->flags to advise the Host: don't interrupt me
 * when you consume a buffer.  It's unreliable, so it's simply an
 * optimization.  */
#define VRING_AVAIL_F_NO_INTERRUPT       1

/* Enable events in packed ring. */
#define VRING_PACKED_EVENT_FLAG_ENABLE   0x0
/* Disable events in packed ring. */
#define VRING_PACKED_EVENT_FLAG_DISABLE  0x1
/*
 * Enable events for a specific descriptor in packed ring.
 * (as specified by Descriptor Ring Change Event Offset/Wrap Counter).
 * Only valid if ZXDH_RING_F_EVENT_IDX has been negotiated.
 */
#define VRING_PACKED_EVENT_FLAG_DESC     0x2

/*
 * Wrap counter bit shift in event suppression structure
 * of packed ring.
 */
#define VRING_PACKED_EVENT_F_WRAP_CTR    15

/* Alignment requirements for vring elements */
#define VRING_AVAIL_ALIGN_SIZE 2
#define VRING_USED_ALIGN_SIZE  4
#define VRING_DESC_ALIGN_SIZE  16

#define MRG_CTX_HEADER_SHIFT 22


/* FIXME: MTU in config. */
#define GOOD_PACKET_LEN (ETH_HLEN + VLAN_HLEN + ETH_DATA_LEN)
#define GOOD_COPY_LEN   128


#define TX_PORT_NP 0x00
#define TX_PORT_DRS 0x01
#define TX_PORT_DTP 0x02
#define HDR_2B_UNIT 2
#define ENABLE_PI_FLAG_32B 0x1
#define DISABLE_PI_FIELD_PARSE 0x80
#define IPV4_TYPE 0x0
#define IPV6_TYPE 0x1
#define NOT_IP_TYPE 0x2
#define PKT_SRC_NP 0x0
#define PKT_SRC_CPU 0x1
#define PCODE_IP 0x1
#define PCODE_TCP 0x2
#define PCODE_UDP 0x3
#define PCODE_NO_IP 0x9
#define PCODE_NO_LRO_TCP 0xc
#define INVALID_ETH_PORT_ID 0xff
#define ETH_MTU_4B_UNIT 4
#define IP_FRG_CSUM_FLAG 0x8000
#define NOT_IP_FRG_CSUM_FLAG 0x6000
#define TCP_FRG_CSUM_FLAG 0x24
#define NOT_TCP_FRG_CSUM_FLAG 0x30
#define HDR_2B_UNIT 2
#define PD_OUTER_TUNNEL_MASK 0x0F00
#define PD_PTYPE_TUNNEL_VXLAN 0x0300
#define PD_PTYPE_TUNNEL_GENEVE 0x0500
#define PD_PTYPE_TUNNEL_GRE 0x0200
#define PD_PTYPE_TUNNEL_IPIP 0x0100
#define PD_INNER_L4_MASK 0xF000
#define PD_PTYPE_L4_TCP 0x1000
#define PD_PTYPE_L4_UDP 0x2000

#define HDR_BUFFER_LEN 100
#define IP_BASE_HLEN 20
#define IPV6_BASE_HLEN 40
#define TCP_BASE_HLEN 20

#define OUTER_IP_CHECKSUM_OFFSET    (12)
#define INNER_IP_CHECKSUM_OFFSET    (15)
#define INNER_L4_CHECKSUM_OFFSET    (2)
//#define PI_HDR_L3_CHKSUM_ERROR_CODE (0xff)
//#define PI_HDR_L4_CHKSUM_ERROR_CODE (0xff)
#define OUTER_IP_CHKSUM_ERROR_CODE  (0x20)
#define OUTER_L4_CHKSUM_ERROR_CODE  (0x800)
#define INNER_L3_CHKSUM_ERROR_CODE  (0x200)
#define DTP_VERIFY_RX_CHECKSUM_BASED_OUTER (0x100)
#define RX_IPV4_UDP_ZERO_CHECKSUM_FLAG (0x800000)
// #define NP_VXLAN_UDP_CHCKSUM_ENABLE (6)
#define NP_UDP_TUNNEL_CHCKSUM_ENABLE (6)

#define NP_IS_VXLAN_FLAG            (5)

#define RX_VLAN_STRIPED_MASK        (1 << 4)
#define RX_QINQ_STRIPED_MASK        (1 << 14)
#define RX_IS_QINQ_PKT_MASK         (1 << 12)
#define RX_TPID_VLAN_ID_MASK        (0xfff)

/* PD header offload flags */
#define PANELID_EN (1 << 15)
#define LB_EN      (1 << 11)

/* PD header sk_prio */
#define ZXDH_DCBNL_SET_SK_PRIO(sk_prio)   ((0x7 & sk_prio) << 8)

#define ZXDH_NET_HDR_F_NEEDS_CSUM 1   /* Use csum_start, csum_offset */
#define ZXDH_NET_HDR_F_DATA_VALID 2   /* Csum is valid */
#define ZXDH_NET_HDR_F_RSC_INFO   4   /* rsc info in csum_ fields */
#define ZXDH_NET_HDR_GSO_NONE 0   /* Not a GSO frame */
#define ZXDH_NET_HDR_GSO_TCPV4    1   /* GSO frame, IPv4 TCP (TSO) */
#define ZXDH_NET_HDR_GSO_UDP      3   /* GSO frame, IPv4 UDP (UFO) */
#define ZXDH_NET_HDR_GSO_TCPV6    4   /* GSO frame, IPv6 TCP */
#define ZXDH_NET_HDR_GSO_ECN      0x80   /* TCP has ECN set */

#define ZXDH_DEV_CAP_MAX_QPAIR    8

/*
 * __vqm{16,32,64} have the following meaning:
 * - __u{16,32,64} for zxdh devices in legacy mode, accessed in native endian
 * - __le{16,32,64} for standard-compliant zxdh devices
 */
typedef __u16 __bitwise __vqm16;
typedef __u32 __bitwise __vqm32;
typedef __u64 __bitwise __vqm64;


/* Constants for MSI-X */
/* Use first vector for configuration changes, second and the rest for
 * virtqueues Thus, we need at least 2 vectors for MSI. */
enum
{
    VP_MSIX_CONFIG_VECTOR = 0,
    VP_MSIX_VQ_VECTOR = 1,
};


struct vring_packed_desc_event
{
    /* Descriptor Ring Change Event Offset/Wrap Counter. */
    __le16 off_wrap;
    /* Descriptor Ring Change Event Flags. */
    __le16 flags;
};

struct vring_packed_desc
{
    /* Buffer Address. */
    __le64 addr;
    /* Buffer Length. */
    __le32 len;
    /* Buffer ID. */
    __le16 id;
    /* The flags depending on descriptor type. */
    __le16 flags;
};

struct vring_desc_state_packed
{
    void *data;         /* Data for callback. */
    struct vring_packed_desc *indir_desc; /* Indirect descriptor, if any. */
    uint16_t num;            /* Descriptor list length. */
    uint16_t last;           /* The last desc state in a list. */
};

struct dh_dma_info
{
    dma_addr_t addr;
    struct page *page;
    uint32_t refcnt_bias;   /* Refcount bias of page */
};

struct vring_desc_extra
{
    dma_addr_t addr;         /* Buffer DMA addr. */
    struct dh_dma_info *di;
    uint32_t len;            /* Buffer length. */
    uint16_t flags;          /* Descriptor flags. */
    uint16_t next;           /* The next desc state in a list. */
    uint16_t last_in_page;
    uint16_t offset;
};

union pkt_type_t
{
    uint8_t pkt_type;
    struct
    {
        uint8_t pkt_code:5;
        uint8_t pkt_src:1;
        uint8_t ip_type:2;
    }type_ctx;
}__attribute__((packed));

struct pi_hdr
{
    uint8_t bttl_pi_len;
    union pkt_type_t pt;
    uint16_t vlan_id;
    uint32_t ipv6_exp_flags;
    uint16_t hdr_l3_offset;
    uint16_t hdr_l4_offset;
    uint8_t eth_port_id;
    uint8_t pkt_action_flag2;
    uint16_t pkt_action_flag1;
    uint8_t sa_index[8];
    uint8_t error_code[2];
    uint8_t rsv[6];
}__attribute__((packed));

struct pd_net_hdr_tx
{
#define TXCAP_STAG_INSERT_EN_BIT      (1 << 14)
#define TXCAP_CTAG_INSERT_EN_BIT      (1 << 13)
#define DELAY_STATISTICS_INSERT_EN_BIT      (1 << 7)
    uint16_t ol_flag;
    uint8_t rsv;
    uint8_t panel_id;
    uint16_t stci;
    uint16_t ctci;
    uint8_t tag_idx;
    uint8_t tag_data;
    uint16_t vfid; /* bit15~11：rsv bit10~0：发送端口vfid */
}__attribute__((packed));


struct pd_net_hdr_rx
{
#define RX_PD_HEAD_VLAN_STRIP_BIT      (1 << 28)
    uint32_t flags;
    uint32_t rss_hash;
    uint32_t fd;
    uint16_t striped_stci;
    uint16_t striped_ctci;
    uint16_t outer_pkt_type;
    uint16_t inner_pkt_type;
    uint16_t pkt_len;
    uint8_t tag_idx;
    uint8_t tag_data;
    uint16_t src_port; /* bit15~11：rsv bit10~0：源端口vfid */
}__attribute__((packed));

/* zxdh net header */
struct pipd_net_hdr_tx
{
    struct pi_hdr pi_hdr; //32B
    struct pd_net_hdr_tx pd_hdr; //12B
}__attribute__((packed));

struct zxdh_net_hdr_tx
{
    uint8_t tx_port; //bit7:2 rsv; bit1:0 00:np, 01:DRS, 10:DTP
    uint8_t pd_len; //bit7 rsv; bit6:0 L2报文前的描述符长度，以2B为单位
    uint8_t num_buffers; //表示接收方向num buffers字段
    uint8_t rsv; //保留

    union
    {
        struct pd_net_hdr_tx pd_hdr; //12B
        struct pipd_net_hdr_tx pipd_hdr; //44B
    };
}__attribute__((packed));

struct zxdh_1588_pd_tx
{
    uint8_t ptp_type[3]; /* 低bit0-16预留，bit17-19 pkt_type, bit23 ptp_udp */
    uint8_t ts_offset;
    uint32_t cpu_tx;
    uint8_t port; /* egress_port/ingress_port, L4报文此字段无用 */
    uint8_t rsv1[4];
    uint8_t sec_1588_key[3];
};

struct zxdh_net_1588_hdr
{
    uint8_t tx_port; //bit7:2 rsv; bit1:0 00:np, 01:DRS, 10:DTP
    uint8_t pd_len; //bit7 rsv; bit6:0 L2报文前的描述符长度，以2B为单位
    uint8_t num_buffers; //表示接收方向num buffers字段
    uint8_t rsv; //保留

    struct pi_hdr pi_hdr;
    struct pd_net_hdr_tx pd_hdr;

    // uint8_t ptp_type[3]; /* 低bit0-16预留，bit17-19 pkt_type, bit23 ptp_udp */
    // uint8_t ts_offset;
    // uint32_t cpu_tx;
    // uint8_t port; /* egress_port/ingress_port, L4报文此字段无用 */
    // uint8_t rsv1[4];
    // uint8_t sec_1588_key[3];
    struct zxdh_1588_pd_tx pd_1588;
}__attribute__((packed));

struct zxdh_net_1588_nopi_hdr
{
    uint8_t tx_port; //bit7:2 rsv; bit1:0 00:np, 01:DRS, 10:DTP
    uint8_t pd_len; //bit7 rsv; bit6:0 L2报文前的描述符长度，以2B为单位
    uint8_t num_buffers; //表示接收方向num buffers字段
    uint8_t rsv; //保留

    struct pd_net_hdr_tx pd_hdr;

    // uint8_t ptp_type[3]; /* 低bit0-16预留，bit17-19 pkt_type, bit23 ptp_udp */
    // uint8_t ts_offset;
    // uint32_t cpu_tx;
    // uint8_t port; /* egress_port/ingress_port, L4报文此字段无用 */
    // uint8_t rsv1[4];
    // uint8_t sec_1588_key[3];
    struct zxdh_1588_pd_tx pd_1588;
}__attribute__((packed));


struct pipd_net_hdr_rx
{
    struct pi_hdr pi_hdr; //32B
    struct pd_net_hdr_rx pd_hdr; //26B
}__attribute__((packed));

struct zxdh_net_hdr_rx
{
    uint8_t tx_port; //bit7:2 rsv; bit1:0 00:np, 01:DRS, 10:DTP
    uint8_t pd_len; //bit7 rsv; bit6:0 L2报文前的描述符长度，以2B为单位
    uint8_t num_buffers; //表示接收方向num buffers字段
    uint8_t rsv; //保留

    union
    {
        struct pd_net_hdr_rx pd_hdr; //26B
        struct pipd_net_hdr_rx pipd_hdr; //58B
    };
}__attribute__((packed));

struct zxdh_1588_pd_rx
{
    uint8_t egress_port;
    uint8_t ptp_type[2]; /* 低bit0-8预留，bit9-11 pkt_type, bit 12-14预留，bit15 ptp_udp */
    uint8_t ts_offset;
    uint32_t rx_ts;
};
struct zxdh_net_1588_hdr_rcv
{
    uint8_t tx_port; //bit7:2 rsv; bit1:0 00:np, 01:DRS, 10:DTP
    uint8_t pd_len; //bit7 rsv; bit6:0 L2报文前的描述符长度，以2B为单位
    uint8_t num_buffers; //表示接收方向num buffers字段
    uint8_t rsv; //保留

    struct pi_hdr pi_hdr;
    struct pd_net_hdr_rx pd_hdr;

    // uint8_t egress_port;
    // uint8_t ptp_type[2]; /* 低bit0-8预留，bit9-11 pkt_type, bit 12-14预留，bit15 ptp_udp */
    // uint8_t ts_offset;
    // uint32_t rx_ts;
    struct zxdh_1588_pd_rx pd_1588;
}__attribute__((packed));

struct zxdh_net_1588_nopi_hdr_rcv
{
    uint8_t tx_port; //bit7:2 rsv; bit1:0 00:np, 01:DRS, 10:DTP
    uint8_t pd_len; //bit7 rsv; bit6:0 L2报文前的描述符长度，以2B为单位
    uint8_t num_buffers; //表示接收方向num buffers字段
    uint8_t rsv; //保留

    struct pd_net_hdr_rx pd_hdr;

    // uint8_t egress_port;
    // uint8_t ptp_type[2]; /* 低bit0-8预留，bit9-11 pkt_type, bit 12-14预留，bit15 ptp_udp */
    // uint8_t ts_offset;
    // uint32_t rx_ts;
    struct zxdh_1588_pd_rx pd_1588;
}__attribute__((packed));

struct zxdh_split_net_hdr
{
    uint8_t flags;
    uint8_t gso_type;
    __vqm16 hdr_len;  /* Ethernet + IP + tcp/udp hdrs */
    __vqm16 gso_size;  /* Bytes to append to hdr_len per frame */
    __vqm16 csum_start;  /* Position to start checksumming from */
    __vqm16 csum_offset;  /* Offset after that to place checksum */
    __vqm16 num_buffers;  /* Number of merged rx buffers */
}__attribute__((packed));

struct zxdh_split_hdr_tx
{
    struct zxdh_split_net_hdr split_hdr; //12B
    struct pd_net_hdr_tx pd_hdr; //12B
}__attribute__((packed));

struct zxdh_split_hdr_rx
{
    struct zxdh_split_net_hdr split_hdr; //12B
    struct pd_net_hdr_rx pd_hdr; //26B
}__attribute__((packed));

#ifdef DEBUG
/* For development, we want to crash whenever the ring is screwed. */
#define BAD_RING(_vq, fmt, args...)                \
    do {                            \
        LOG_ERR("%s:"fmt, (_vq)->vq.name, ##args);    \
        BUG();                        \
    } while (0)
/* Caller is supposed to guarantee no reentry. */
#define START_USE(_vq)                        \
    do {                            \
        if ((_vq)->in_use)                \
            panic("%s:in_use = %i\n",        \
                  (_vq)->vq.name, (_vq)->in_use);    \
        (_vq)->in_use = __LINE__;            \
    } while (0)
#define END_USE(_vq) \
    do { BUG_ON(!(_vq)->in_use); (_vq)->in_use = 0; } while(0)
#define LAST_ADD_TIME_UPDATE(_vq)                \
    do {                            \
        ktime_t now = ktime_get();            \
                                \
        /* No kick or get, with .1 second between?  Warn. */ \
        if ((_vq)->last_add_time_valid)            \
            WARN_ON(ktime_to_ms(ktime_sub(now,    \
                (_vq)->last_add_time)) > 100);    \
        (_vq)->last_add_time = now;            \
        (_vq)->last_add_time_valid = true;        \
    } while (0)
#define LAST_ADD_TIME_CHECK(_vq)                \
    do {                            \
        if ((_vq)->last_add_time_valid) {        \
            WARN_ON(ktime_to_ms(ktime_sub(ktime_get(), \
                      (_vq)->last_add_time)) > 100); \
        }                        \
    } while (0)
#define LAST_ADD_TIME_INVALID(_vq)                \
    ((_vq)->last_add_time_valid = false)
#else
#define BAD_RING(_vq, fmt, args...)                \
    do {                            \
        LOG_ERR("%s:"fmt, (_vq)->vq.name, ##args);    \
        (_vq)->broken = true;                \
    } while (0)
#define START_USE(vq)
#define END_USE(vq)
#define LAST_ADD_TIME_UPDATE(vq)
#define LAST_ADD_TIME_CHECK(vq)
#define LAST_ADD_TIME_INVALID(vq)
#endif


#define vqm_store_mb(weak_barriers, p, v) \
do { \
    if (weak_barriers) { \
        virt_store_mb(*p, v); \
    } else { \
        WRITE_ONCE(*p, v); \
        mb(); \
    } \
} while (0) \


/* This is the PCI capability header: */
struct zxdh_pci_cap
{
    __u8 cap_vndr;        /* Generic PCI field: PCI_CAP_ID_VNDR */
    __u8 cap_next;        /* Generic PCI field: next ptr. */
    __u8 cap_len;         /* Generic PCI field: capability length */
    __u8 cfg_type;        /* Identifies the structure. */
    __u8 bar;             /* Where to find it. */
    __u8 id;              /* Multiple capabilities of the same type */
    __u8 padding[2];      /* Pad to full dword. */
    __le32 offset;        /* Offset within bar. */
    __le32 length;        /* Length of the structure, in bytes. */
};

struct zxdh_pci_notify_cap
{
    struct zxdh_pci_cap cap;
    __le32 notify_off_multiplier; /* Multiplier for queue_notify_off. */
};

struct dh_page_cache_reduce
{
    struct delayed_work reduce_work;
    u32 successive;
    unsigned long next_ts;      /* timer ticks */
    unsigned long graceful_period;
    unsigned long delay;
    struct dh_dma_info *pending;
    u32 npages;
};

struct dh_page_cache
{
    struct dh_dma_info *page_cache;
    int head;
    u32 sz;
    u32 lrs;    /* least recently sampled */
    u16 log_min_sz;
    u16 log_max_sz;
    struct dh_page_cache_reduce reduce;
};

struct virtqueue
{
    struct list_head list;
    void (*callback)(struct virtqueue *vq);
    const char *name;
    struct zxdh_en_device *en_dev;
    uint32_t index;
    uint32_t phy_index;
    uint32_t num_free;
    struct dh_page_cache page_cache;
    void *rq;
    void *priv;
};

/* Alignment requirements for vring elements.
 * When using pre-virtio 1.0 layout, these fall out naturally.
 */
#define VRING_AVAIL_ALIGN_SIZE 2
#define VRING_USED_ALIGN_SIZE 4
#define VRING_DESC_ALIGN_SIZE 16

/* custom queue ring descriptors: 16 bytes. These can chain together via "next". */
struct vring_desc
{
    /* Address (guest-physical). */
    __vqm64 addr;
    /* Length. */
    __vqm32 len;
    /* The flags as indicated above. */
    __vqm16 flags;
    /* We chain unused descriptors via this, too */
    __vqm16 next;
};

struct vring_avail
{
    __vqm16 flags;
    __vqm16 idx;
    __vqm16 ring[];
};

/* u32 is used here for ids for padding reasons. */
struct vring_used_elem
{
    /* Index of start of used descriptor chain. */
    __vqm32 id;
    /* Total length of the descriptor chain which was used (written to) */
    __vqm32 len;
};

typedef struct vring_used_elem __attribute__((aligned(VRING_USED_ALIGN_SIZE)))
    vring_used_elem_t;

struct vring_used
{
    __vqm16 flags;
    __vqm16 idx;
    vring_used_elem_t ring[];
};

typedef struct vring_desc __attribute__((aligned(VRING_DESC_ALIGN_SIZE)))
    vring_desc_t;
typedef struct vring_avail __attribute__((aligned(VRING_AVAIL_ALIGN_SIZE)))
    vring_avail_t;
typedef struct vring_used __attribute__((aligned(VRING_USED_ALIGN_SIZE)))
    vring_used_t;

struct vring
{
    uint32_t num;

    vring_desc_t *desc;

    vring_avail_t *avail;

    vring_used_t *used;
};

struct vring_desc_state_split
{
    void *data; /* Data for callback. */
    struct vring_desc *indir_desc; /* Indirect descriptor, if any. */
};

struct vring_virtqueue
{
    struct virtqueue vq;

    /* Is this a packed ring? */
    bool packed_ring;

    /* Is DMA API used? */
    bool use_dma_api;

    /* Can we use weak barriers? */
    bool weak_barriers;

    /* Other side has made a mess, don't try any more. */
    bool broken;

    /* Host supports indirect buffers */
    bool indirect;

    /* Host publishes avail event idx */
    bool event;

    /* Head of free buffer list. */
    uint32_t free_head;
    /* Number we've added since last sync. */
    uint32_t num_added;

    /* Last used index  we've seen.
     * for split ring, it just contains last used index
     * for packed ring:
     * bits up to VRING_PACKED_EVENT_F_WRAP_CTR include the last used index.
     * bits from VRING_PACKED_EVENT_F_WRAP_CTR include the used wrap counter.
     */
    uint16_t last_used_idx;

    /* Hint for event idx: already triggered no need to disable. */
    bool event_triggered;

    /* Available for packed ring */
    union
    {
        /* Available for split ring */
        struct
        {
            /* Actual memory layout for this queue. */
            struct vring vring;

            /* Last written value to avail->flags */
            uint16_t avail_flags_shadow;

            /*
            * Last written value to avail->idx in
            * guest byte order.
            */
            uint16_t avail_idx_shadow;

            /* Per-descriptor state. */
            struct vring_desc_state_split *desc_state;
            struct vring_desc_extra *desc_extra;

            /* DMA address and size information */
            dma_addr_t queue_dma_addr;
            size_t queue_size_in_bytes;
        } split;

        /* Available for packed ring */
        struct
        {
            /* Actual memory layout for this queue. */
            struct
            {
                uint32_t num;
                struct vring_packed_desc *desc;
                struct vring_packed_desc_event *driver;
                struct vring_packed_desc_event *device;
            } vring;

            /* Driver ring wrap counter. */
            bool avail_wrap_counter;

            /* Avail used flags. */
            uint16_t avail_used_flags;

            /* Index of the next avail descriptor. */
            uint16_t next_avail_idx;

            /*
                * Last written value to driver->flags in
                * guest byte order.
                */
            uint16_t event_flags_shadow;

            /* Per-descriptor state. */
            struct vring_desc_state_packed *desc_state;
            struct vring_desc_extra *desc_extra;

            /* DMA address and size information */
            dma_addr_t ring_dma_addr;
            dma_addr_t driver_event_dma_addr;
            dma_addr_t device_event_dma_addr;
            size_t ring_size_in_bytes;
            size_t event_size_in_bytes;
        } packed;
    };

    /* How to notify other side. FIXME: commonalize hcalls! */
    bool (*notify)(struct virtqueue *vq);

    /* DMA, allocation, and size information */
    bool we_own_ring;

#ifdef DEBUG
    /* They're supposed to lock for us. */
    uint32_t in_use;

    /* Figure out if their kicks are too delayed. */
    bool last_add_time_valid;
    ktime_t last_add_time;
#endif
};

struct zxdh_pci_vq_info
{
    /* the actual virtqueue */
    struct virtqueue *vq;

    /* the list node for the virtqueues list */
    struct list_head node;

    /* channel num map 1-1 to vector*/
    unsigned channel_num;
};

struct virtnet_stat_desc
{
    char desc[ETH_GSTRING_LEN];
    size_t offset;
};

struct virtnet_sq_stats
{
    struct u64_stats_sync syncp;
    uint64_t packets;
    uint64_t bytes;
    uint64_t xdp_tx;
    uint64_t xdp_tx_drops;
    uint64_t kicks;
    uint64_t tx_timeouts;
};

struct virtnet_rq_stats
{
    struct u64_stats_sync syncp;
    uint64_t packets;
    uint64_t bytes;
    uint64_t drops;
    uint64_t xdp_packets;
    uint64_t xdp_tx;
    uint64_t xdp_redirects;
    uint64_t xdp_drops;
    uint64_t kicks;
    uint64_t rx_csum_unnecessary;
    uint64_t rx_removed_vlan_packets;
    uint64_t rx_csum_unnecessary_tunnel;
};
#define VIRTNET_SQ_STAT(m)    offsetof(struct virtnet_sq_stats, m)
#define VIRTNET_RQ_STAT(m)    offsetof(struct virtnet_rq_stats, m)

#ifdef ZXDH_CONFIG_SPECIAL_SQ_EN
struct zxdh_sq_flow_map {
    struct hlist_node hlist;
    uint32_t dst_ip;
    uint16_t dst_port;
    uint16_t queue_index;
};
#endif

static const struct virtnet_stat_desc virtnet_sq_stats_desc[] =
{
    { "packets",        VIRTNET_SQ_STAT(packets) },
    { "bytes",          VIRTNET_SQ_STAT(bytes) },
    { "xdp_tx",         VIRTNET_SQ_STAT(xdp_tx) },
    { "xdp_tx_drops",   VIRTNET_SQ_STAT(xdp_tx_drops) },
    { "kicks",          VIRTNET_SQ_STAT(kicks) },
    { "tx_timeouts",    VIRTNET_SQ_STAT(tx_timeouts) },
};

static const struct virtnet_stat_desc virtnet_rq_stats_desc[] =
{
    { "packets",        VIRTNET_RQ_STAT(packets) },
    { "bytes",          VIRTNET_RQ_STAT(bytes) },
    { "drops",          VIRTNET_RQ_STAT(drops) },
    { "xdp_packets",    VIRTNET_RQ_STAT(xdp_packets) },
    { "xdp_tx",         VIRTNET_RQ_STAT(xdp_tx) },
    { "xdp_redirects",  VIRTNET_RQ_STAT(xdp_redirects) },
    { "xdp_drops",      VIRTNET_RQ_STAT(xdp_drops) },
    { "kicks",          VIRTNET_RQ_STAT(kicks) },
    { "rx_csum_unnecessary", VIRTNET_RQ_STAT(rx_csum_unnecessary) },
    { "rx_removed_vlan_packets", VIRTNET_RQ_STAT(rx_removed_vlan_packets) },
    { "rx_csum_unnecessary_tunnel", VIRTNET_RQ_STAT(rx_csum_unnecessary_tunnel) },
};

#define VIRTNET_SQ_STATS_LEN    ARRAY_SIZE(virtnet_sq_stats_desc)
#define VIRTNET_RQ_STATS_LEN    ARRAY_SIZE(virtnet_rq_stats_desc)

#define vring_used_event(vr) ((vr)->avail->ring[(vr)->num])
#define vring_avail_event(vr) (*(__vqm16 *)&(vr)->used->ring[(vr)->num])

/* RX packet size EWMA. The average packet size is used to determine the packet
 * buffer size when refilling RX rings. As the entire RX ring may be refilled
 * at once, the weight is chosen so that the EWMA will be insensitive to short-
 * term, transient changes in packet size.
 */
#ifdef CGS_V5_693
/* 3.10 内核 DECLARE_EWMA 的 factor 参数必须是 2 的幂次，使用 1 代替 0 */
DECLARE_EWMA(pkt_len, 1, 64)
#else
DECLARE_EWMA(pkt_len, 0, 64)
#endif


/* Internal representation of a send virtqueue */
struct send_queue
{
    /* Virtqueue associated with this send _queue */
    struct virtqueue *vq;

    /* TX: fragments + linear part + custom queue header */
    struct scatterlist sg[MAX_SKB_FRAGS + 2];

    /* Name of the send queue: output.$index */
    char name[40];

    struct virtnet_sq_stats stats;

    struct napi_struct napi;

    uint8_t *hdr_buf;
    uint16_t hdr_idx;

#ifdef ZXDH_CONFIG_SPECIAL_SQ_EN
    struct zxdh_sq_flow_map flow_map;
#endif
};

struct dh_rq_stats
{
    u64 cache_reuse;
    u64 cache_full;
    u64 cache_empty;
    u64 cache_busy;
    u64 cache_extend;
    u64 cache_reduce;
    u64 cache_alloc;
    u64 cache_waive;
};

/* Internal representation of a receive virtqueue */
struct receive_queue
{
    /* Virtqueue associated with this receive_queue */
    struct virtqueue *vq;

    struct napi_struct napi;

    struct bpf_prog __rcu *xdp_prog;

    struct virtnet_rq_stats stats;

    /* Chain pages by the private ptr. */
    struct page *pages;

    /* Average packet length for mergeable receive buffers. */
    struct ewma_pkt_len mrg_avg_pkt_len; //todo

    /* Page frag for packet buffer allocation. */
    struct page_frag alloc_frag;

    /* RX: fragments + linear part + custom queue header */
    struct scatterlist sg[MAX_SKB_FRAGS + 2];

    /* Min single buffer size for mergeable buffers case. */
    uint32_t min_buf_len;

    /* Name of this receive queue: input.$index */
    char name[40];

#ifndef CGS_V5_693
    struct xdp_rxq_info xdp_rxq;
#endif

    struct dh_rq_stats _stats;
    unsigned long state;
};

#define to_vvq(_vq) container_of(_vq, struct vring_virtqueue, vq)

typedef void vq_callback_t(struct virtqueue *);

void zxdh_set_default_xps_cpumasks(struct zxdh_en_device *en_dev);

bool zxdh_has_feature(struct zxdh_en_device *en_dev, uint32_t fbit);

static inline uint16_t __vqm16_to_cpu(bool little_endian, __vqm16 val)
{
	if (little_endian)
		return le16_to_cpu((__force __le16)val);
	else
		return be16_to_cpu((__force __be16)val);
}

static inline __vqm16 __cpu_to_vqm16(bool little_endian, uint16_t val)
{
	if (little_endian)
		return (__force __vqm16)cpu_to_le16(val);
	else
		return (__force __vqm16)cpu_to_be16(val);
}

static inline uint32_t __vqm32_to_cpu(bool little_endian, __vqm32 val)
{
	if (little_endian)
		return le32_to_cpu((__force __le32)val);
	else
		return be32_to_cpu((__force __be32)val);
}

static inline __vqm32 __cpu_to_vqm32(bool little_endian, uint32_t val)
{
	if (little_endian)
		return (__force __vqm32)cpu_to_le32(val);
	else
		return (__force __vqm32)cpu_to_be32(val);
}

static inline uint64_t __vqm64_to_cpu(bool little_endian, __vqm64 val)
{
	if (little_endian)
		return le64_to_cpu((__force __le64)val);
	else
		return be64_to_cpu((__force __be64)val);
}

static inline __vqm64 __cpu_to_vqm64(bool little_endian, uint64_t val)
{
	if (little_endian)
		return (__force __vqm64)cpu_to_le64(val);
	else
		return (__force __vqm64)cpu_to_be64(val);
}

static inline bool zxdh_legacy_is_little_endian(void)
{
#ifdef __LITTLE_ENDIAN
	return true;
#else
	return false;
#endif
}

static inline bool zxdh_is_little_endian(struct zxdh_en_device *en_dev)
{
	return zxdh_has_feature(en_dev, ZXDH_F_VERSION_1) || zxdh_legacy_is_little_endian();
}

/* Memory accessors */
static inline uint16_t vqm16_to_cpu(struct zxdh_en_device *en_dev, __vqm16 val)
{
	return __vqm16_to_cpu(zxdh_is_little_endian(en_dev), val);
}

static inline __vqm16 cpu_to_vqm16(struct zxdh_en_device *en_dev, uint16_t val)
{
	return __cpu_to_vqm16(zxdh_is_little_endian(en_dev), val);
}

static inline uint32_t vqm32_to_cpu(struct zxdh_en_device *en_dev, __vqm32 val)
{
	return __vqm32_to_cpu(zxdh_is_little_endian(en_dev), val);
}

static inline __vqm32 cpu_to_vqm32(struct zxdh_en_device *en_dev, uint32_t val)
{
	return __cpu_to_vqm32(zxdh_is_little_endian(en_dev), val);
}

static inline uint64_t vqm64_to_cpu(struct zxdh_en_device *en_dev, __vqm64 val)
{
	return __vqm64_to_cpu(zxdh_is_little_endian(en_dev), val);
}

static inline __vqm64 cpu_to_vqm64(struct zxdh_en_device *en_dev, uint64_t val)
{
	return __cpu_to_vqm64(zxdh_is_little_endian(en_dev), val);
}

static inline struct zxdh_split_hdr_tx *skb_vnet_hdr_tx(struct sk_buff *skb)
{
	return (struct zxdh_split_hdr_tx *)skb->cb;
}

static inline struct zxdh_split_hdr_rx *skb_vnet_hdr_rx(struct sk_buff *skb)
{
	return (struct zxdh_split_hdr_rx *)skb->cb;
}

void zxdh_print_vring_info(struct virtqueue *vq, uint32_t desc_index, uint32_t desc_num);
void virtnet_napi_enable(struct virtqueue *vq, struct napi_struct *napi);
void virtnet_napi_tx_enable(struct net_device *netdev, struct virtqueue *vq, struct napi_struct *napi);
void virtnet_napi_tx_disable(struct napi_struct *napi);
void refill_work(struct work_struct *work);
int virtnet_poll(struct napi_struct *napi, int budget);
int virtnet_poll_tx(struct napi_struct *napi, int budget);
int32_t txq2vq(int32_t txq);
int32_t rxq2vq(int32_t rxq);
uint8_t vp_get_status(struct net_device *netdev);
void vp_set_status(struct net_device *netdev, uint8_t status);
void vp_set_reset_status(struct net_device *netdev, uint8_t status);
void zxdh_add_status(struct net_device *netdev, uint32_t status);
void zxdh_vp_enable_cbs(struct net_device *netdev);
void zxdh_vp_disable_cbs(struct net_device *netdev);
void zxdh_vp_reset(struct net_device *netdev);
void vring_free_queue(struct zxdh_en_device *en_dev, size_t size, void *queue, dma_addr_t dma_handle);
netdev_tx_t start_xmit(struct sk_buff *skb, struct net_device *netdev);
bool try_fill_recv(struct receive_queue *rq, gfp_t gfp);
int32_t virtqueue_add_outbuf(struct virtqueue *vq, struct scatterlist *sg, uint32_t num, void *data, gfp_t gfp);
void virtqueue_disable_cb(struct virtqueue *_vq);
void free_old_xmit_skbs(struct net_device *netdev, struct send_queue *sq, bool in_napi);
bool virtqueue_enable_cb_delayed(struct virtqueue *_vq);
bool virtqueue_kick_prepare_packed(struct virtqueue *_vq);
bool virtqueue_kick_prepare_split(struct virtqueue *_vq);
bool virtqueue_kick_prepare(struct virtqueue *_vq);
bool virtqueue_notify(struct virtqueue *_vq);
void zxdh_pf_features_init(struct net_device *netdev);
bool zxdh_has_status(struct net_device *netdev, uint32_t sbit);
void zxdh_free_unused_bufs(struct net_device *netdev);
void zxdh_free_receive_bufs(struct net_device *netdev);
void zxdh_free_receive_page_frags(struct net_device *netdev);
void zxdh_virtnet_del_vqs(struct net_device *netdev);
void zxdh_vqs_uninit(struct net_device *netdev);
int32_t zxdh_vqs_init(struct net_device *netdev);
int32_t zxdh_create_vqs(struct zxdh_en_device *en_dev);
void zxdh_destroy_vqs(struct zxdh_en_device *en_dev);
int32_t dh_eq_vqs_vring_int(struct notifier_block *nb, unsigned long action, void *data);
int32_t vq2rxq(struct virtqueue *vq);
void *virtqueue_get_buf_ctx_packed(struct virtqueue *_vq, uint32_t *len, void **ctx, bool is_delay, uint32_t *_id);
void *virtqueue_get_buf_ctx_split(struct virtqueue *_vq, uint32_t *len, void **ctx, bool is_delay);
void *virtqueue_get_buf_ctx(struct virtqueue *_vq, uint32_t *len, void **ctx, bool is_delay, uint32_t *_id);
uint32_t virtqueue_get_vring_size(struct virtqueue *_vq);
void virtqueue_napi_complete(struct napi_struct *napi, struct virtqueue *vq, int32_t processed);
int32_t virtqueue_add_inbuf_ctx(struct virtqueue *vq,
                                struct scatterlist *sg, uint32_t num,
                                void *data,
                                void *ctx,
                                gfp_t gfp);
bool is_used_desc_packed(const struct vring_virtqueue *vq, uint16_t idx, bool used_wrap_counter);
bool dh_skb_page_frag_refill(unsigned int sz, struct page_frag *pfrag, gfp_t gfp);

int32_t zxdh_sec_vqs_init(struct net_device *netdev);
void zxdh_sec_vqs_uninit(struct net_device *netdev, uint8_t qidx);
void zxdh_vvq_reset(struct zxdh_en_device *en_dev);
bool is_flow_stopped(struct zxdh_en_device *en_dev);
#ifndef CGS_V5_693
int zxdh_en_xdp(struct net_device *dev, struct netdev_bpf *xdp);
int zxdh_en_xdp_xmit(struct net_device *dev, int n, struct xdp_frame **frames, uint32_t flags);
#endif

#define DH_PAGE_CACHE_LOG_MAX_RQ_MULT       4
#define DH_PAGE_CACHE_REDUCE_WORK_INTERVAL  200  /* msecs */
#define DH_PAGE_CACHE_REDUCE_GRACE_PERIOD   1000 /* msecs */
#define DH_PAGE_CACHE_REDUCE_SUCCESSIVE_CNT 5

enum {
    DH_RQ_STATE_ENABLED,
    DH_RQ_STATE_CACHE_REDUCE_PENDING
};

#define PAGE_REF_THRSD (PAGE_SIZE / 64)

#ifdef __cplusplus
}
#endif

#endif
