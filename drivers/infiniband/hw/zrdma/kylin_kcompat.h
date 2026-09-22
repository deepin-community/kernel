#ifndef KYLIN_KCOMPAT_H
#define KYLIN_KCOMPAT_H

#ifdef KYLIN_V10_4
// #define IB_DEV_OPS_FILL_ENTRY
#define CREATE_AH_VER_5
#define ZRDMA_CREATE_SRQ_VER_2
#define ZRDMA_DESTROY_SRQ_VER_3
#define DESTROY_AH_VER_4
#define DESTROY_QP_VER_2
#define COPY_USER_PGADDR_VER_3
#define ALLOC_UCONTEXT_VER_2
#define DEALLOC_UCONTEXT_VER_2
#define ALLOC_PD_VER_3
#define DEALLOC_PD_VER_4

#if (defined(__OFED_24_10__) || defined(__OFED_24_04__))
#define PROCESS_MAD_VER_3
#define CREATE_QP_VER_2

#ifdef __OFED_24_04__
    #define CREATE_CQ_VER_3
#elif defined(__OFED_24_10__)
    #define CREATE_CQ_VER_4
#endif

#define ALLOC_HW_STATS_STRUCT_V2
#define ALLOC_HW_STATS_V3
#define GET_HW_STATS_V2
#define QUERY_PORT_V2
#define MODIFY_PORT_V2
#define REREG_MR_VER_2
#define GET_LINK_LAYER_V2
#define ROCE_PORT_IMMUTABLE_V2
#define QUERY_GID_ROCE_V2
#define QUERY_PKEY_V2
#define GLOBAL_QP_MEM
#else
#define PROCESS_MAD_VER_4
#define CREATE_QP_VER_1
#define CREATE_CQ_VER_3
#define ALLOC_HW_STATS_STRUCT_V1
#define ALLOC_HW_STATS_V1
#define GET_HW_STATS_V1
#define QUERY_PORT_V1
#define MODIFY_PORT_V1
#define REREG_MR_VER_1
#define GET_LINK_LAYER_V1
#define ROCE_PORT_IMMUTABLE_V1
#define QUERY_GID_ROCE_V1
#define QUERY_PKEY_V1
#endif

#define IW_PORT_IMMUTABLE_V1
#define QUERY_GID_V1
#define RDMA_MMAP_DB_SUPPORT
// #define RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
#define ZXDH_ALLOC_MW_VER_2
#define ZXDH_ALLOC_MR_VER_0
#define ZXDH_DESTROY_CQ_VER_4
#define kc_typeq_ib_wr const

#define set_max_sge(props, rf)                                            \
	do {                                                              \
		((props)->max_send_sge =                                  \
			 (rf)->sc_dev.hw_attrs.uk_attrs.max_hw_wq_frags); \
		((props)->max_recv_sge =                                  \
			 (rf)->sc_dev.hw_attrs.uk_attrs.max_hw_wq_frags); \
	} while (0)
#define kc_set_props_ip_gid_caps(props) ((props)->ip_gids = true)
#define kc_rdma_gid_attr_network_type(sgid_attr, gid_type, gid) \
	rdma_gid_attr_network_type(sgid_attr)
#define kc_set_ibdev_add_del_gid(ibdev)
#define kc_deref_sgid_attr(sgid_attr) ((sgid_attr)->ndev)
#define kc_ib_register_device(device, name, dev) ib_register_device(device, name, dev)
#define HAS_IB_SET_DEVICE_OP
#define kc_ib_modify_qp_is_ok(cur_state, next_state, type, mask, ll) \
	ib_modify_qp_is_ok(cur_state, next_state, type, mask)

#define SET_BEST_PAGE_SZ_V2
#define kc_rdma_udata_to_drv_context(ibpd, udata) rdma_udata_to_drv_context(udata, struct zxdh_ucontext, ibucontext)
#if (!defined(__OFED_24_10__) && !defined(__OFED_24_04__))
#define UVERBS_CMD_MASK
#endif
#define NETDEV_TO_IBDEV_SUPPORT
#define IB_DEALLOC_DRIVER_SUPPORT
#define kc_get_ucontext(udata) rdma_udata_to_drv_context(udata, struct zxdh_ucontext, ibucontext)
#define FOR_IFA
#define set_ibdev_dma_device(ibdev, dev)
#define ah_attr_to_dmac(attr) ((attr).roce.dmac)
#define SET_ROCE_CM_INFO_VER_3
#define IB_UMEM_GET_V2
#define DEREG_MR_VER_2

#if (defined(__OFED_24_10__) || defined(__OFED_24_04__))
	#define IB_DEV_CAPS_VER_2
    #define GET_ETH_SPEED_V1
    #define ib_umem_get(device, addr, size, access) \
         ib_umem_get_peer(device, addr, size, access, 1)
#endif
#endif

#ifdef KYLIN_V11_0
#define IB_DEV_CAPS_VER_2
#define IB_UMEM_GET_V3
#define ALLOC_HW_STATS_V3
#define RDMA_MMAP_DB_SUPPORT
#define CREATE_QP_VER_2
#ifdef __OFED_24_10__
    #define CREATE_CQ_VER_4
#else
    #define CREATE_CQ_VER_3
#endif
#define PROCESS_MAD_VER_3

#define CREATE_AH_VER_5
#define ZRDMA_CREATE_SRQ_VER_2
#define ZRDMA_DESTROY_SRQ_VER_3
#define DESTROY_AH_VER_4
#define DESTROY_QP_VER_2
#define COPY_USER_PGADDR_VER_4
#define ALLOC_UCONTEXT_VER_2
#define DEALLOC_UCONTEXT_VER_2
#define ALLOC_PD_VER_3
#define DEALLOC_PD_VER_4

#define ALLOC_HW_STATS_STRUCT_V2
#define ALLOC_HW_STATS_V3
#define GET_HW_STATS_V2
#define QUERY_PORT_V2
#define MODIFY_PORT_V2
#define REREG_MR_VER_2
#define GET_LINK_LAYER_V2
#define ROCE_PORT_IMMUTABLE_V2
#define QUERY_GID_ROCE_V2
#define QUERY_PKEY_V2
#define GLOBAL_QP_MEM

#define QUERY_GID_V1
#define ZXDH_ALLOC_MW_VER_2
#define ZXDH_ALLOC_MR_VER_0
#define ZXDH_DESTROY_CQ_VER_4
#define kc_typeq_ib_wr const

#define set_max_sge(props, rf)                                            \
	do {                                                              \
		((props)->max_send_sge =                                  \
			 (rf)->sc_dev.hw_attrs.uk_attrs.max_hw_wq_frags); \
		((props)->max_recv_sge =                                  \
			 (rf)->sc_dev.hw_attrs.uk_attrs.max_hw_wq_frags); \
	} while (0)
#define kc_set_props_ip_gid_caps(props) ((props)->ip_gids = true)
#define kc_rdma_gid_attr_network_type(sgid_attr, gid_type, gid) \
	rdma_gid_attr_network_type(sgid_attr)
#define kc_set_ibdev_add_del_gid(ibdev)
#define kc_deref_sgid_attr(sgid_attr) ((sgid_attr)->ndev)
#define kc_ib_register_device(device, name, dev) ib_register_device(device, name, dev)
#define HAS_IB_SET_DEVICE_OP
#define kc_ib_modify_qp_is_ok(cur_state, next_state, type, mask, ll) \
	ib_modify_qp_is_ok(cur_state, next_state, type, mask)

#define SET_BEST_PAGE_SZ_V2
#define kc_rdma_udata_to_drv_context(ibpd, udata) rdma_udata_to_drv_context(udata, struct zxdh_ucontext, ibucontext)
#define NETDEV_TO_IBDEV_SUPPORT
#define IB_DEALLOC_DRIVER_SUPPORT
#define kc_get_ucontext(udata) rdma_udata_to_drv_context(udata, struct zxdh_ucontext, ibucontext)
#define FOR_IFA
#define set_ibdev_dma_device(ibdev, dev)
#define ah_attr_to_dmac(attr) ((attr).roce.dmac)
#define SET_ROCE_CM_INFO_VER_3
#define DEREG_MR_VER_2

#if (defined(__OFED_24_10__))
    #define GET_ETH_SPEED_V1
    #define ib_umem_get(device, addr, size, access) \
         ib_umem_get_peer(device, addr, size, access, 1)
#endif

#endif

#endif /* KYLIN_KCOMPAT_H */
