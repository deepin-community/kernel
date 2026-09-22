/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef LINUX_KCOMPAT_H
#define LINUX_KCOMPAT_H

/* IB_IW_PKEY */
#if KERNEL_VERSION(5, 9, 0) > LINUX_VERSION_CODE
#define IB_IW_PKEY
#endif

/* DEV_OPS_FILL_ENTRY */
#if KERNEL_VERSION(5, 9, 0) <= LINUX_VERSION_CODE
#define IB_DEV_OPS_FILL_ENTRY
#endif

/* KMAP_LOCAL_PAGE */
#if KERNEL_VERSION(5, 11, 0) > LINUX_VERSION_CODE
#define USE_KMAP
#endif

#if KERNEL_VERSION(6, 6, 0) <= LINUX_VERSION_CODE
#define IB_DEV_CAPS_VER_2
#endif

/* CREATE_AH */
#if KERNEL_VERSION(5, 8, 0) <= LINUX_VERSION_CODE
#define CREATE_AH_VER_5
#elif KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE
#define CREATE_AH_VER_2
#elif KERNEL_VERSION(5, 0, 0) <= LINUX_VERSION_CODE
#define CREATE_AH_VER_3
#elif KERNEL_VERSION(4, 12, 0) <= LINUX_VERSION_CODE
#define CREATE_AH_VER_1_2
#define ETHER_COPY_VER_2
#else
#define CREATE_AH_VER_1_1
#define ETHER_COPY_VER_1
#endif

/* PROCESS_MAD */
#if KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE
#define PROCESS_MAD_VER_3
#elif KERNEL_VERSION(5, 5, 0) <= LINUX_VERSION_CODE
#define PROCESS_MAD_VER_2
#else
#define PROCESS_MAD_VER_1
#endif

/* ZRDMA_CREATE_SRQ */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
#define ZRDMA_CREATE_SRQ_VER_1
#else
#define ZRDMA_CREATE_SRQ_VER_2
#endif

/* ZRDMA_DESTROY_SRQ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
#define ZRDMA_DESTROY_SRQ_VER_3
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0)
#define ZRDMA_DESTROY_SRQ_VER_2
#else
#define ZRDMA_DESTROY_SRQ_VER_1
#endif /* LINUX_VERSION_CODE */

/* DESTROY_AH */
#if KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE
#define DESTROY_AH_VER_4
#else
#if KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE
#define DESTROY_AH_VER_3
#else
#if KERNEL_VERSION(5, 0, 0) <= LINUX_VERSION_CODE
#define DESTROY_AH_VER_2
#else
#define DESTROY_AH_VER_1
#endif
#endif
#endif

/* CREAT_QP */
#if KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE
#define CREATE_QP_VER_2
#define GLOBAL_QP_MEM
#else
#define CREATE_QP_VER_1
#endif

/* DESTROY_QP */
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
#define DESTROY_QP_VER_1
#define kc_zxdh_destroy_qp(ibqp, udata) zxdh_destroy_qp(ibqp)
#else
#define DESTROY_QP_VER_2
#define kc_zxdh_destroy_qp(ibqp, udata) zxdh_destroy_qp(ibqp, udata)
#endif

/* CREATE_CQ */
#if KERNEL_VERSION(5, 3, 0) <= LINUX_VERSION_CODE
#define CREATE_CQ_VER_3
#elif KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE
#define CREATE_CQ_VER_2
#else
#define CREATE_CQ_VER_1
#endif

/* COPY_USER_PGDADDR */
#if KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE
#define COPY_USER_PGADDR_VER_4
#elif KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE
#define COPY_USER_PGADDR_VER_3
#elif KERNEL_VERSION(5, 1, 0) > LINUX_VERSION_CODE
#define COPY_USER_PGADDR_VER_1
#elif KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
#define COPY_USER_PGADDR_VER_2
#endif

/* ALLOC_UCONTEXT/ DEALLOC_UCONTEXT */
#if KERNEL_VERSION(5, 1, 0) > LINUX_VERSION_CODE
#define ALLOC_UCONTEXT_VER_1
#define DEALLOC_UCONTEXT_VER_1
#else
#define ALLOC_UCONTEXT_VER_2
#define DEALLOC_UCONTEXT_VER_2
#endif

/* ALLOC_PD , DEALLOC_PD */
#if KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE
#define DEALLOC_PD_VER_4
#define ALLOC_PD_VER_3
#else
#if KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE
#define ALLOC_PD_VER_3
#define DEALLOC_PD_VER_3
#else
#if KERNEL_VERSION(5, 1, 0) <= LINUX_VERSION_CODE
#define ALLOC_PD_VER_2
#define DEALLOC_PD_VER_2
#else
#define ALLOC_PD_VER_1
#define DEALLOC_PD_VER_1
#endif
#endif
#endif

#if KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE
#define ALLOC_HW_STATS_STRUCT_V2
#else
#define ALLOC_HW_STATS_STRUCT_V1
#endif

#if KERNEL_VERSION(5, 14, 0) <= LINUX_VERSION_CODE
#define ALLOC_HW_STATS_V3
#elif KERNEL_VERSION(5, 13, 0) <= LINUX_VERSION_CODE
#define ALLOC_HW_STATS_V2
#else
#define ALLOC_HW_STATS_V1
#endif

#if KERNEL_VERSION(5, 13, 0) <= LINUX_VERSION_CODE
#define QUERY_GID_ROCE_V2
#define MODIFY_PORT_V2
#define QUERY_PKEY_V2
#define ROCE_PORT_IMMUTABLE_V2
#define GET_HW_STATS_V2
#define GET_LINK_LAYER_V2
#define IW_PORT_IMMUTABLE_V2
#define QUERY_GID_V2
#define QUERY_PORT_V2
#else
#define QUERY_GID_ROCE_V1
#define MODIFY_PORT_V1
#define QUERY_PKEY_V1
#define ROCE_PORT_IMMUTABLE_V1
#define GET_HW_STATS_V1
#define GET_LINK_LAYER_V1
#define IW_PORT_IMMUTABLE_V1
#define QUERY_GID_V1
#define QUERY_PORT_V1
#endif

#if KERNEL_VERSION(5, 10, 0) > LINUX_VERSION_CODE
#define GET_ETH_SPEED_AND_WIDTH_V1
#else
#define GET_ETH_SPEED_AND_WIDTH_V2
#endif

#if KERNEL_VERSION(4, 20, 0) > LINUX_VERSION_CODE
#define VMA_DATA
#endif

#if KERNEL_VERSION(5, 5, 0) <= LINUX_VERSION_CODE
/* https://lore.kernel.org/linux-rdma/20191217210406.GC17227@ziepe.ca/
 * This series adds mmap DB support and also extends rdma_user_mmap_io API
 * with an extra param
 */
#define RDMA_MMAP_DB_SUPPORT
#endif

#if KERNEL_VERSION(5, 9, 0) <= LINUX_VERSION_CODE
/* https://lore.kernel.org/all/20200630093916.332097-6-leon@kernel.org/
 * EXPORT_SYMBOL(uverbs_copy_to_struct_or_zero);
 */
#define RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
#endif

/* ZXDH_ALLOC_MW */
#if KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE
#define ZXDH_ALLOC_MW_VER_2
#else
#define ZXDH_ALLOC_MW_VER_1
#endif

/* ZXDH_ALLOC_MR */
#if (KERNEL_VERSION(5, 9, 0) > LINUX_VERSION_CODE && \
     KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE)
#define ZXDH_ALLOC_MR_VER_1
#else
#define ZXDH_ALLOC_MR_VER_0
#endif

#if KERNEL_VERSION(4, 16, 0) > LINUX_VERSION_CODE
#define IB_UVERBS_CQ_FLAGS_TIMESTAMP_COMPLETION IB_CQ_FLAGS_TIMESTAMP_COMPLETION
#endif

/* ZXDH_DESTROY_CQ */
#if KERNEL_VERSION(5, 9, 3) <= LINUX_VERSION_CODE
#define ZXDH_DESTROY_CQ_VER_4
#elif KERNEL_VERSION(5, 3, 0) <= LINUX_VERSION_CODE
#define ZXDH_DESTROY_CQ_VER_3
#elif KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE
#define ZXDH_DESTROY_CQ_VER_2
#else
#define ZXDH_DESTROY_CQ_VER_1
#endif /* LINUX_VERSION_CODE */

/* max_sge, ip_gid, gid_attr_network_type, deref_sgid_attr */
#if KERNEL_VERSION(4, 19, 0) > LINUX_VERSION_CODE
#define set_max_sge(props, rf) \
	((props)->max_sge = (rf)->sc_dev.hw_attrs.uk_attrs.max_hw_wq_frags)
#define kc_set_props_ip_gid_caps(props) \
	((props)->port_cap_flags |= IB_PORT_IP_BASED_GIDS)
#define kc_rdma_gid_attr_network_type(sgid_attr, gid_type, gid) \
	ib_gid_to_network_type(gid_type, gid)
#define kc_deref_sgid_attr(sgid_attr) (sgid_attr.ndev)
#define rdma_query_gid(ibdev, port, index, gid) \
	ib_get_cached_gid(ibdev, port, index, gid, NULL)
#define IB_GET_CACHED_GID
#else
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
#define kc_deref_sgid_attr(sgid_attr) ((sgid_attr)->ndev)
#endif

#if KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE
#define kc_typeq_ib_wr const
#else
#define kc_typeq_ib_wr
#endif

/* ib_register_device */
#if KERNEL_VERSION(4, 20, 0) > LINUX_VERSION_CODE
#define kc_ib_register_device(device, name, dev) \
	ib_register_device(device, NULL)
#elif (KERNEL_VERSION(4, 20, 0) <= LINUX_VERSION_CODE) && \
	(KERNEL_VERSION(5, 1, 0) > LINUX_VERSION_CODE)
#define kc_ib_register_device(device, name, dev) \
	ib_register_device(device, name, NULL)
#elif (KERNEL_VERSION(5, 1, 0) <= LINUX_VERSION_CODE) && \
	(KERNEL_VERSION(5, 10, 0) > LINUX_VERSION_CODE)
#define kc_ib_register_device(device, name, dev) \
	ib_register_device(device, name)
#else
#define kc_ib_register_device(device, name, dev) \
	ib_register_device(device, name, dev)
#endif

#if KERNEL_VERSION(5, 0, 0) <= LINUX_VERSION_CODE
#define HAS_IB_SET_DEVICE_OP
#endif /* >= 5.0.0 */

#if KERNEL_VERSION(4, 17, 0) > LINUX_VERSION_CODE
int zxdh_add_gid(struct ib_device *device, u8 port_num, unsigned int index,
		 const union ib_gid *gid, const struct ib_gid_attr *attr,
		 void **context);
int zxdh_del_gid(struct ib_device *device, u8 port_num, unsigned int index,
		 void **context);

#define kc_set_ibdev_add_del_gid(ibdev)        \
	do {                                   \
		ibdev->add_gid = zxdh_add_gid; \
		ibdev->del_gid = zxdh_del_gid; \
	} while (0)
#else
#define kc_set_ibdev_add_del_gid(ibdev)
#endif

#if KERNEL_VERSION(4, 20, 0) > LINUX_VERSION_CODE
#define kc_ib_modify_qp_is_ok(cur_state, next_state, type, mask, ll) \
	ib_modify_qp_is_ok(cur_state, next_state, type, mask, ll)
#else
#define kc_ib_modify_qp_is_ok(cur_state, next_state, type, mask, ll) \
	ib_modify_qp_is_ok(cur_state, next_state, type, mask)
#endif /* < 4.20.0 */

#if (KERNEL_VERSION(4, 17, 0) <= LINUX_VERSION_CODE && \
     KERNEL_VERSION(5, 3, 0) > LINUX_VERSION_CODE)
#define ZXDH_SET_DRIVER_ID
#endif

#if (KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE)
#define kc_rdma_udata_to_drv_context(ibpd, udata) \
	to_ucontext(ibpd->uobject->context)
#define SET_BEST_PAGE_SZ_V1
#else
#define SET_BEST_PAGE_SZ_V2
#define kc_rdma_udata_to_drv_context(ibpd, udata) \
	rdma_udata_to_drv_context(udata, struct zxdh_ucontext, ibucontext)
#endif

#if (KERNEL_VERSION(5, 11, 0) > LINUX_VERSION_CODE)
#define UVERBS_CMD_MASK
#else
#define USE_QP_ATTRS_STANDARD
#endif

#if KERNEL_VERSION(5, 1, 0) > LINUX_VERSION_CODE
#define ib_umem_get(udata, addr, size, access, dmasync) \
	ib_umem_get(pd->uobject->context, addr, size, access, dmasync)
#define ib_device_put(dev)
#define ib_alloc_device(zxdh_device, ibdev) \
	((struct zxdh_device *)ib_alloc_device(sizeof(struct zxdh_device)))
#else
#define NETDEV_TO_IBDEV_SUPPORT
#define IB_DEALLOC_DRIVER_SUPPORT
#endif /* < 5.1.0 */

/******PORT_PHYS_STATE enums***************************************************/
#if (KERNEL_VERSION(5, 4, 0) > LINUX_VERSION_CODE)
enum ib_port_phys_state {
	IB_PORT_PHYS_STATE_SLEEP = 1,
	IB_PORT_PHYS_STATE_POLLING = 2,
	IB_PORT_PHYS_STATE_DISABLED = 3,
	IB_PORT_PHYS_STATE_PORT_CONFIGURATION_TRAINING = 4,
	IB_PORT_PHYS_STATE_LINK_UP = 5,
	IB_PORT_PHYS_STATE_LINK_ERROR_RECOVERY = 6,
	IB_PORT_PHYS_STATE_PHY_TEST = 7,
};
#endif
/*********************************************************/

#if KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE
#define kc_get_ucontext(udata) \
	rdma_udata_to_drv_context(udata, struct zxdh_ucontext, ibucontext)
#else
#define kc_get_ucontext(udata) to_ucontext(context)
#endif

#if KERNEL_VERSION(5, 3, 0) <= LINUX_VERSION_CODE
#define IN_IFADDR
#else
#define FOR_IFA
#endif

#if KERNEL_VERSION(5, 1, 0) > LINUX_VERSION_CODE
struct ib_ucontext *zxdh_alloc_ucontext(struct ib_device *ibdev,
					struct ib_udata *udata);
int zxdh_dealloc_ucontext(struct ib_ucontext *context);
struct ib_pd *zxdh_alloc_pd(struct ib_device *ibdev,
			    struct ib_ucontext *context,
			    struct ib_udata *udata);
int zxdh_dealloc_pd(struct ib_pd *ibpd);
#else
int zxdh_alloc_ucontext(struct ib_ucontext *uctx, struct ib_udata *udata);
void zxdh_dealloc_ucontext(struct ib_ucontext *context);
#if KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE
int zxdh_dealloc_pd(struct ib_pd *ibpd, struct ib_udata *udata);
int zxdh_alloc_pd(struct ib_pd *pd, struct ib_udata *udata);
#else
#if KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE
int zxdh_alloc_pd(struct ib_pd *pd, struct ib_udata *udata);
void zxdh_dealloc_pd(struct ib_pd *ibpd, struct ib_udata *udata);
#else
int zxdh_alloc_pd(struct ib_pd *pd, struct ib_ucontext *context,
		  struct ib_udata *udata);
void zxdh_dealloc_pd(struct ib_pd *ibpd);
#endif
#endif
#endif

/*****SETUP DMA_DEVICE***************************************************/
#if KERNEL_VERSION(4, 11, 0) > LINUX_VERSION_CODE
#define set_ibdev_dma_device(ibdev, dev) ibdev.dma_device = dev
#else
#define set_ibdev_dma_device(ibdev, dev)
#endif /* < 4.11.0 */
/*********************************************************/

#if KERNEL_VERSION(4, 12, 0) > LINUX_VERSION_CODE
#define rdma_ah_attr ib_ah_attr
#define ah_attr_to_dmac(attr) ((attr).dmac)
#else
#define ah_attr_to_dmac(attr) ((attr).roce.dmac)
#endif /* < 4.12.0 */

#if KERNEL_VERSION(4, 13, 0) > LINUX_VERSION_CODE
#define wait_queue_entry __wait_queue
#endif /* < 4.13.0 */

#if KERNEL_VERSION(4, 17, 0) > LINUX_VERSION_CODE
#define ZXDH_ADD_DEL_GID
#endif

#if KERNEL_VERSION(4, 19, 0) > LINUX_VERSION_CODE
#define SET_ROCE_CM_INFO_VER_1
#define IB_IW_MANDATORY_AH_OP
#elif KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
#define SET_ROCE_CM_INFO_VER_2
#else
#define SET_ROCE_CM_INFO_VER_3
#endif

#if KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE
#define IB_UMEM_GET_V3
#elif KERNEL_VERSION(5, 5, 0) <= LINUX_VERSION_CODE
#define IB_UMEM_GET_V2
#else
#define IB_UMEM_GET_V1
#endif

#if KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE
#define DEREG_MR_VER_2
#else
#define DEREG_MR_VER_1
#endif

/* REREG MR  */
#if KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE
#define REREG_MR_VER_2
#else
#define REREG_MR_VER_1
#endif

/* DMABUF */
// #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
// #define SET_DMABUF
// #if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 17, 0)
// #define REG_USER_MR_DMABUF_VER_3
// #elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0)
// #define REG_USER_MR_DMABUF_VER_2
// #else
// #define REG_USER_MR_DMABUF_VER_1
// #endif
// #endif

#endif /* LINUX_KCOMPAT_H */
