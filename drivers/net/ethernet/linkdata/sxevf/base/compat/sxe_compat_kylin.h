#ifndef __SXE_COMPAT_KYLIN_H__
#define __SXE_COMPAT_KYLIN_H__

#if !KYLIN_RELEASE_CODE
#error "KYLIN_RELEASE_CODE is 0 or undefined"
#endif

#if defined KYLIN_RELEASE_CODE && (KYLIN_RELEASE_CODE == KYLIN_RELEASE_VERSION(10,1)) 
#define HAVE_XDP_SUPPORT
#define HAVE_XDP_BUFF_DATA_META
#define XDP_XMIT_FRAME_FAILED_NEED_FREE
#define HAVE_XDP_QUERY_PROG
#define XDP_RXQ_INFO_REQ_API_NEED_3_PARAMS
#define ETH_GET_HEADLEN_API_NEED_2_PARAM
#define NEED_SKB_FRAG_OFF_API
#define NEED_SKB_FRAG_OFF_ADD_API
#define HAVE_MACVLAN_OFFLOAD_SUPPORT
#define xdp_convert_buff_to_frame convert_to_xdp_frame
#define ETH_P_LLDP	0x88CC
#define HAVE_NDO_SET_VF_LINK_STATE
#define SXE_KERNEL_MATCHED
#endif

#if defined KYLIN_RELEASE_CODE && (KYLIN_RELEASE_CODE == KYLIN_RELEASE_VERSION(10,3)) 
#define HAVE_XDP_BUFF_FRAME_SIZE
#define HAVE_XDP_SUPPORT
#define HAVE_XDP_BUFF_DATA_META
#define XDP_XMIT_FRAME_FAILED_NEED_FREE
#define HAVE_XDP_QUERY_PROG
#define XDP_RXQ_INFO_REQ_API_NEED_3_PARAMS
#define HAVE_DEV_PAGE_IS_REUSABLE_API
#define HAVE_MACVLAN_OFFLOAD_SUPPORT
#define HAVE_AF_XDP_ZERO_COPY
#define xdp_convert_buff_to_frame convert_to_xdp_frame
#define ETH_P_LLDP	0x88CC

#define HAVE_NDO_XSK_WAKEUP
#define HAVE_NDO_SET_VF_LINK_STATE
#define XDP_SETUP_XSK_POOL XDP_SETUP_XSK_UMEM
#define xsk_uses_need_wakeup xsk_umem_uses_need_wakeup
#define xsk_tx_peek_desc xsk_umem_consume_tx
#define xsk_tx_release xsk_umem_consume_tx_done
#define xsk_tx_completed xsk_umem_complete_tx
#define SXE_KERNEL_MATCHED
#endif

#if defined KYLIN_RELEASE_CODE && (KYLIN_RELEASE_CODE == KYLIN_RELEASE_VERSION(10,4))  
#define HAVE_XDP_SUPPORT
#define HAVE_XDP_BUFF_DATA_META
#define HAVE_MEM_TYPE_XSK_BUFF_POOL
#define HAVE_XDP_BUFF_FRAME_SIZE
#define HAVE_AF_XDP_ZERO_COPY
#define XDP_RXQ_INFO_REQ_API_NEED_3_PARAMS
#define XSK_BUFF_DMA_SYNC_API_NEED_1_PARAM
#define XDP_XMIT_FRAME_FAILED_NEED_FREE
#define HAVE_XDP_QUERY_PROG
#define HAVE_NETDEV_NESTED_PRIV
#define HAVE_DEV_PAGE_IS_REUSABLE_API
#define HAVE_NDO_XSK_WAKEUP
#define HAVE_NDO_FDB_ADD_EXTACK
#define HAVE_NDO_BRIDGE_SETLINK_EXTACK
#define HAVE_NDO_SET_VF_LINK_STATE
#define HAVE_MACVLAN_OFFLOAD_SUPPORT

#define XDP_SETUP_XSK_POOL XDP_SETUP_XSK_UMEM
#define xsk_pool_get_rx_frame_size xsk_umem_get_rx_frame_size
#define xsk_pool_set_rxq_info xsk_buff_set_rxq_info
#define xsk_pool_dma_map xsk_buff_dma_map
#define xsk_pool_dma_unmap xsk_buff_dma_unmap
#define xsk_uses_need_wakeup xsk_umem_uses_need_wakeup
#define xsk_tx_peek_desc xsk_umem_consume_tx
#define xsk_tx_release xsk_umem_consume_tx_done
#define xsk_tx_completed xsk_umem_complete_tx
#define SXE_KERNEL_MATCHED
#endif

#endif 