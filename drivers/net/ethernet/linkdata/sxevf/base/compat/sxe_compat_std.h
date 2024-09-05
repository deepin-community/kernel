#ifndef __SXE_COMPAT_STD_H__
#define __SXE_COMPAT_STD_H__

#ifndef LINUX_VERSION_CODE
#error "LINUX_VERSION_CODE is undefined"
#endif

#ifndef KERNEL_VERSION
#error "KERNEL_VERSION is undefined"
#endif

#ifdef SXE_KERNEL_MATCHED
#error "SXE_KERNEL_MATCHED is defined"
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,15,0))
#define HAVE_ETHTOOL_COALESCE_EXTACK
#else 
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,6)))
#if (RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9,0))
#define HAVE_ETHTOOL_COALESCE_EXTACK
#endif 
#endif 
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9,1)))
#define HAVE_ETHTOOL_COALESCE_EXTACK
#endif 

#if (OPENEULER_VERSION_CODE && (OPENEULER_VERSION_CODE >= OPENEULER_VERSION(2203,1)))
#define HAVE_ETHTOOL_COALESCE_EXTACK
#endif 

#if (SUSE_PRODUCT_CODE && (SUSE_PRODUCT_CODE > SUSE_PRODUCT(1,15,2,0)))
#define HAVE_ETHTOOL_COALESCE_EXTACK
#endif 
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,16,0)) && \
    (LINUX_VERSION_CODE < KERNEL_VERSION(5,11,0))
#define XDP_RXQ_INFO_REQ_API_NEED_3_PARAMS
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,5)))
#undef XDP_RXQ_INFO_REQ_API_NEED_3_PARAMS
#endif 
#else 
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7,6)) && \
    (RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8,5)))
#define XDP_RXQ_INFO_REQ_API_NEED_3_PARAMS
#endif 
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0))
#define HAVE_XDP_BUFF_DATA_META
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,8,0))
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,4)))
#define HAVE_XDP_BUFF_FRAME_SIZE
#endif 
#if (KYLIN_RELEASE_CODE && (KYLIN_RELEASE_CODE >= KYLIN_RELEASE_VERSION(10,3)))
#define HAVE_XDP_BUFF_FRAME_SIZE
#endif 
#else 
#define HAVE_XDP_BUFF_FRAME_SIZE
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,12,0))
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,5)))
#define HAVE_XDP_BUFF_INIT_API
#endif 
#else 
#define HAVE_XDP_BUFF_INIT_API
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,12,0))
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,5)))
#define HAVE_XDP_PREPARE_BUFF_API
#endif 
#else 
#define HAVE_XDP_PREPARE_BUFF_API
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,9,0))
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,5)))
#define HAVE_NETDEV_NESTED_PRIV
#endif 
#else 
#define HAVE_NETDEV_NESTED_PRIV
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0))
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,3)))
#define HAVE_TIMEOUT_TXQUEUE_IDX
#endif 
#else 
#define HAVE_TIMEOUT_TXQUEUE_IDX
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,4)))
#define HAVE_NET_PREFETCH_API
#endif 
#else 
#define HAVE_NET_PREFETCH_API
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,1,0))
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,1)))
#define HAVE_NDO_FDB_ADD_EXTACK
#endif 
#else 
#define HAVE_NDO_FDB_ADD_EXTACK
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0))
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,1)))
#define HAVE_NDO_BRIDGE_SETLINK_EXTACK
#endif 
#else 
#define HAVE_NDO_BRIDGE_SETLINK_EXTACK
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0))
#if (RHEL_RELEASE_CODE && RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(6,6))
#define HAVE_NDO_SET_VF_LINK_STATE
#endif
#else 
#define HAVE_NDO_SET_VF_LINK_STATE
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0))
#define HAVE_MACVLAN_OFFLOAD_SUPPORT
#endif  

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,2,0))
#define ETH_GET_HEADLEN_API_NEED_2_PARAM
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,2)))
#undef ETH_GET_HEADLEN_API_NEED_2_PARAM
#endif 

#if (KYLIN_RELEASE_CODE && (KYLIN_RELEASE_CODE >= KYLIN_RELEASE_VERSION(10,3)))
#undef ETH_GET_HEADLEN_API_NEED_2_PARAM
#endif 
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0))
#define NEED_SKB_FRAG_OFF_ADD_API
#define NEED_SKB_FRAG_OFF_API
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4,14,241) && \
     LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0))
#undef NEED_SKB_FRAG_OFF_API
#endif 
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4,19,200) && \
     LINUX_VERSION_CODE < KERNEL_VERSION(4,20,0))
#undef NEED_SKB_FRAG_OFF_API
#endif 

#if (RHEL_RELEASE_CODE && RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,2))
#undef NEED_SKB_FRAG_OFF_API
#undef NEED_SKB_FRAG_OFF_ADD_API
#endif 

#if (UBUNTU_VERSION_CODE && (UBUNTU_VERSION_CODE >= UBUNTU_VERSION(4,15,0,159) && \
     UBUNTU_VERSION_CODE < UBUNTU_VERSION(4,15,0,999)))
#undef NEED_SKB_FRAG_OFF_API
#endif 

#if (SUSE_PRODUCT_CODE && (SUSE_PRODUCT_CODE >= SUSE_PRODUCT(1,15,2,0)))
#undef NEED_SKB_FRAG_OFF_API
#undef NEED_SKB_FRAG_OFF_ADD_API
#endif 

#if (KYLIN_RELEASE_CODE && (KYLIN_RELEASE_CODE >= KYLIN_RELEASE_VERSION(10,3)))
#undef NEED_SKB_FRAG_OFF_API
#undef NEED_SKB_FRAG_OFF_ADD_API
#endif 
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,3,0))
#ifndef ETH_P_LLDP
#define ETH_P_LLDP	0x88CC
#endif 
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,15,0))
#if (RHEL_RELEASE_CODE && RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9,0))
#define HAVE_NDO_ETH_IOCTL
#endif 
#else 
#define HAVE_NDO_ETH_IOCTL
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0))
#define NETIF_NAPI_ADD_API_NEED_3_PARAMS
#else 
#if (RHEL_RELEASE_CODE && RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9,2))
#define NETIF_NAPI_ADD_API_NEED_3_PARAMS
#endif 
#endif 

#if ( LINUX_VERSION_CODE < KERNEL_VERSION(3,18,0) )
#if (RHEL_RELEASE_CODE && RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7,3))
#define HAVE_SKB_XMIT_MORE
#endif 
#else 
#define HAVE_SKB_XMIT_MORE
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,2,0))
#ifdef HAVE_SKB_XMIT_MORE
#if !(RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,2)))
#define netdev_xmit_more() (skb->xmit_more)
#endif 
#else
#define netdev_xmit_more() (0)
#endif 
#endif 

#ifndef NETIF_F_GSO_IPXIP4
#define NETIF_F_GSO_IPXIP4 0
#endif
#ifndef NETIF_F_GSO_IPXIP6
#define NETIF_F_GSO_IPXIP6 0
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,17,0))
#define DCBNL_OPS_GETAPP_RETURN_U8
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0))
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7,5)))
#define HAVE_DMA_ATTRS_STRUCT
#endif 
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0))
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7,5)))
#define HAVE_NET_DEVICE_EXTENDED
#endif
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0))
#ifndef skb_frag_size
#define NEED_SKB_FRAG_SIZE_API
#endif
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,18,0))
#define NEED_BOOTTIME_SECONDS
#endif 

#if ( LINUX_VERSION_CODE < KERNEL_VERSION(2,4,23) )
#define NOT_INCLUDE_SCTP_H
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0))
#define HAVE_XDP_SUPPORT
#endif 

#ifdef HAVE_XDP_SUPPORT
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,18,0))
#define XDP_XMIT_FRAME_FAILED_NEED_FREE
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,9,0))
#define HAVE_XDP_QUERY_PROG
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,4)))
#undef HAVE_XDP_QUERY_PROG
#endif 
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,8,0))
#if !(RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,4)))
#define xdp_convert_buff_to_frame convert_to_xdp_frame
#endif 
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,17,0))
#define BPF_WARN_INVALID_XDP_ACTION_API_NEED_3_PARAMS
#else 
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,7)))
#if (RHEL_RELEASE_CODE != RHEL_RELEASE_VERSION(9,0))
#define BPF_WARN_INVALID_XDP_ACTION_API_NEED_3_PARAMS
#endif
#endif 
#endif 

#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,17,0))
#define HAVE_ETHTOOL_EXTENDED_RINGPARAMS
#else 
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,7)))
#if RHEL_RELEASE_CODE != RHEL_RELEASE_VERSION(9,0)
#define HAVE_ETHTOOL_EXTENDED_RINGPARAMS
#endif
#endif 

#if (OPENEULER_VERSION_CODE && (OPENEULER_VERSION_CODE >= OPENEULER_VERSION(2203,1)))
#define HAVE_ETHTOOL_EXTENDED_RINGPARAMS
#endif 
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,20,0))
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,1)))
#define HAVE_AF_XDP_ZERO_COPY
#endif 
#else 
#define HAVE_AF_XDP_ZERO_COPY
#endif 

#ifdef HAVE_AF_XDP_ZERO_COPY
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,8,0))
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,4)))
#define HAVE_MEM_TYPE_XSK_BUFF_POOL
#endif 
#else 
#define HAVE_MEM_TYPE_XSK_BUFF_POOL
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,5)))
#define HAVE_NETDEV_BPF_XSK_BUFF_POOL
#endif 
#else 
#define HAVE_NETDEV_BPF_XSK_BUFF_POOL
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0))
#if (SUSE_PRODUCT_CODE && (SUSE_PRODUCT_CODE >= SUSE_PRODUCT(1,15,2,0)))
#define HAVE_NDO_XSK_WAKEUP
#endif 
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,3)))
#define HAVE_NDO_XSK_WAKEUP
#endif 
#else 
#define HAVE_NDO_XSK_WAKEUP
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
#define XSK_BUFF_DMA_SYNC_API_NEED_1_PARAM
#endif 
#if (RHEL_RELEASE_CODE && RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,5))
#undef XSK_BUFF_DMA_SYNC_API_NEED_1_PARAM
#endif 
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
#define NEED_XSK_BUFF_POOL_RENAME
#if (RHEL_RELEASE_CODE && RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,5))
#undef NEED_XSK_BUFF_POOL_RENAME
#endif 
#endif 

#ifdef NEED_XSK_BUFF_POOL_RENAME
#define XDP_SETUP_XSK_POOL XDP_SETUP_XSK_UMEM
#define xsk_tx_release xsk_umem_consume_tx_done
#define xsk_tx_completed xsk_umem_complete_tx
#define xsk_uses_need_wakeup xsk_umem_uses_need_wakeup
#define xsk_get_pool_from_qid xdp_get_umem_from_qid
#define xsk_pool_get_rx_frame_size xsk_umem_get_rx_frame_size
#define xsk_pool_set_rxq_info xsk_buff_set_rxq_info
#define xsk_pool_dma_unmap xsk_buff_dma_unmap
#define xsk_pool_dma_map xsk_buff_dma_map
#define xsk_tx_peek_desc xsk_umem_consume_tx
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,3,0))
#define XSK_UMEM_CONSUME_TX_NEED_3_PARAMS
#if (RHEL_RELEASE_CODE && RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(8,1))
#undef XSK_UMEM_CONSUME_TX_NEED_3_PARAMS
#endif 
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0))
#define xsk_umem_discard_addr xsk_umem_release_addr
#define xsk_umem_discard_addr_rq xsk_umem_release_addr_rq
#else 
#if (RHEL_RELEASE_CODE && RHEL_RELEASE_CODE == RHEL_RELEASE_VERSION(8,3))
#define xsk_umem_discard_addr xsk_umem_release_addr
#define xsk_umem_discard_addr_rq xsk_umem_release_addr_rq
#endif 
#endif 
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,3,0))
#define u64_stats_fetch_begin_irq u64_stats_fetch_begin
#define u64_stats_fetch_retry_irq u64_stats_fetch_retry
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0))
#define CLASS_CREATE_NEED_1_PARAM
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0))
#define DEFINE_SEMAPHORE_NEED_CNT
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0))
#define DELETE_PCIE_ERROR_REPORTING
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,15,0))
#define HAVE_ETH_HW_ADDR_SET_API
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
#define HAVE_PTP_CLOCK_INFO_ADJFINE
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,16,0))
#define HAVE_NO_XDP_BUFF_RXQ
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,18,0))
#define HAVE_NO_OVERFLOW_H
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0))
#if (!(RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7,5))))
#ifndef dma_map_page_attrs
#define dma_map_page_attrs __kc_dma_map_page_attrs
static inline dma_addr_t __kc_dma_map_page_attrs(struct device *dev,
                              struct page *page,
                              size_t offset, size_t size,
                              enum dma_data_direction dir,
                              unsigned long __always_unused attrs)
{
     return dma_map_page(dev, page, offset, size, dir);
}
#endif
#ifndef dma_unmap_page_attrs
#define dma_unmap_page_attrs __kc_dma_unmap_page_attrs
static inline void __kc_dma_unmap_page_attrs(struct device *dev,
                              dma_addr_t addr, size_t size,
                              enum dma_data_direction dir,
                              unsigned long __always_unused attrs)
{
	dma_unmap_page(dev, addr, size, dir);
}
#endif
static inline void __page_frag_cache_drain(struct page *page,
                         unsigned int count)
{
     if (!page_ref_sub_and_test(page, count))
          return;

     init_page_count(page);

     __free_pages(page, compound_order(page));
}
#endif 
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0))
#define HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
#if (RHEL_RELEASE_CODE && RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7,5))
#undef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
#endif
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0))
struct _kc_xdp_buff {
	void *data;
	void *data_end;
	void *data_hard_start;
};
#define xdp_buff _kc_xdp_buff
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0))
#define NO_VOID_NDO_GET_STATS64
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7,5)))
#undef NO_VOID_NDO_GET_STATS64
#endif 
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0))
#define NO_NETDEVICE_MIN_MAX_MTU
#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7,5)))
#undef NO_NETDEVICE_MIN_MAX_MTU
#endif
#ifndef ETH_MIN_MTU
#define ETH_MIN_MTU 68
#endif 
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0))
#if (!RHEL_RELEASE_CODE) || \
    (RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(7,5)))
static inline bool _kc_napi_complete_done(struct napi_struct *napi,
					   int __always_unused work_done)
{
	napi_complete(napi);

	return true;
}

#ifdef napi_complete_done
#undef napi_complete_done
#endif
#define napi_complete_done _kc_napi_complete_done
#endif 
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,12,0))
#define HAVE_NO_PCIE_FLR
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,13,0))
#define HAVE_NO_HWTSTAMP_FILTER_NTP_ALL
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0))
#define TIMER_DATA_TYPE		unsigned long
#define TIMER_FUNC_TYPE		void (*)(TIMER_DATA_TYPE)
#define timer_setup(timer, callback, flags)				\
	__setup_timer((timer), (TIMER_FUNC_TYPE)(callback),		\
		      (TIMER_DATA_TYPE)(timer), (flags))
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0))
#define NO_NEED_SIGNAL_H
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,18,0))
#define HAVE_NO_MACVLAN_DEST_FILTER
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,18,0))
#define HAVE_NO_SB_BIND_CHANNEL
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,18,0))
#define HAVE_NO_MACVLAN_RELEASE
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,18,0))
#define NEED_SET_MACVLAN_MODE
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,18,0))
#define NO_NEED_POOL_DEFRAG
#endif 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0))
#define HAVE_NO_WALK_UPPER_DEV
#endif 

#endif 