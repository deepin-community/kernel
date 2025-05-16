/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef CQM_OBJECT_H
#define CQM_OBJECT_H

#include "cqm_define.h"
#include "vram_common.h"

#define CQM_LINKWQE_128B 128
#define CQM_MOD_TOE	HINIC3_MOD_TOE
#define CQM_MOD_CQM	HINIC3_MOD_CQM

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#ifndef HIUDK_SDK

#define CQM_SUCCESS                 0
#define CQM_FAIL                    (-1)
/* Ignore the return value and continue */
#define CQM_CONTINUE                1

/* type of WQE is LINK WQE */
#define CQM_WQE_WF_LINK             1
/* type of WQE is common WQE */
#define CQM_WQE_WF_NORMAL           0

/* chain queue mode */
#define CQM_QUEUE_LINK_MODE         0
/* RING queue mode */
#define CQM_QUEUE_RING_MODE         1
/* SRQ queue mode */
#define CQM_QUEUE_TOE_SRQ_LINK_MODE 2
/* RDMA queue mode */
#define CQM_QUEUE_RDMA_QUEUE_MODE   3

/* generic linkwqe structure */
struct tag_cqm_linkwqe {
	u32 rsv1   : 14;        /* <reserved field */
	u32 wf     : 1;         /* <wf */
	u32 rsv2   : 14;        /* <reserved field */
	u32 ctrlsl : 2;         /* <ctrlsl */
	u32 o      : 1;         /* <o bit */

	u32 rsv3   : 31;        /* <reserved field */
	u32 lp     : 1;         /* The lp field determines whether the o-bit
				 * meaning is reversed.
				 */

	u32 next_page_gpa_h;    /* <record the upper 32b physical address of the
				 * next page for the chip
				 */
	u32 next_page_gpa_l;    /* <record the lower 32b physical address of the
				 * next page for the chip
				 */

	u32 next_buffer_addr_h; /* <record the upper 32b virtual address of the
				 * next page for the driver
				 */
	u32 next_buffer_addr_l; /* <record the lower 32b virtual address of the
				 * next page for the driver
				 */
};

/* SRQ linkwqe structure. The wqe size must not exceed the common RQE size. */
struct tag_cqm_srq_linkwqe {
	struct tag_cqm_linkwqe linkwqe;     /* <generic linkwqe structure */
	u32 current_buffer_gpa_h;  /* <Record the upper 32b physical address of
				    * the current page, which is used when the
				    * driver releases the container and cancels
				    * the mapping.
				    */
	u32 current_buffer_gpa_l;  /* <Record the lower 32b physical address of
				    * the current page, which is used when the
				    * driver releases the container and cancels
				    * the mapping.
				    */
	u32 current_buffer_addr_h; /* <Record the upper 32b of the virtual
				    * address of the current page, which is used
				    * when the driver releases the container.
				    */
	u32 current_buffer_addr_l; /* <Record the lower 32b of the virtual
				    * address of the current page, which is used
				    * when the driver releases the container.
				    */

	u32 fast_link_page_addr_h; /* <Record the upper 32b of the virtual
				    * address of the fastlink page where the
				    * container address is recorded. It is used
				    * when the driver releases the fastlink.
				    */
	u32 fast_link_page_addr_l; /* <Record the lower 32b virtual address of
				    * the fastlink page where the container
				    * address is recorded. It is used when the
				    * driver releases the fastlink.
				    */

	u32 fixed_next_buffer_addr_h; /* <Record the upper 32b virtual address
				       * of the next contianer, which is used to
				       * release driver resources. The driver
				       * cannot be modified.
				       */
	u32 fixed_next_buffer_addr_l; /* <Record the lower 32b virtual address
				       * of the next contianer, which is used to
				       * release driver resources. The driver
				       * cannot be modified.
				       */
};

/* first 64B of standard 128B WQE */
union tag_cqm_linkwqe_first64B {
	struct tag_cqm_linkwqe basic_linkwqe;       /* <generic linkwqe structure */
	struct tag_cqm_srq_linkwqe toe_srq_linkwqe; /* <SRQ linkwqe structure */
	u32 value[16];			            /* <reserved field */
};

/* second 64B of standard 128B WQE */
struct tag_cqm_linkwqe_second64B {
	u32 rsvd0[4];                      /* <first 16B reserved field */
	u32 rsvd1[4];                      /* <second 16B reserved field */
	union {
		struct {
			u32 rsvd0[3];
			u32 rsvd1     : 29;
			u32 toe_o     : 1; /* <o bit of toe */
			u32 resvd2    : 2;
		} bs;
		u32 value[4];
	} third_16B; /* <third 16B */

	union {
		struct {
			u32 rsvd0[2];
			u32 rsvd1     : 31;
			u32 ifoe_o    : 1; /* <o bit of ifoe */
			u32 rsvd2;
		} bs;
		u32 value[4];
	} forth_16B;                       /* <fourth 16B */
};

/* standard 128B WQE structure */
struct tag_cqm_linkwqe_128B {
	union tag_cqm_linkwqe_first64B first64B;    /* <first 64B of standard 128B WQE */
	struct tag_cqm_linkwqe_second64B second64B; /* <back 64B of standard 128B WQE */
};

/* AEQ type definition */
enum cqm_aeq_event_type {
	CQM_AEQ_BASE_T_NIC = 0,     /* <NIC consists of 16 events:0~15 */
	CQM_AEQ_BASE_T_ROCE = 16,   /* <ROCE consists of 32 events:16~47 */
	CQM_AEQ_BASE_T_FC = 48,     /* <FC consists of 8 events:48~55 */
	CQM_AEQ_BASE_T_IOE = 56,    /* <IOE consists of 8 events:56~63 */
	CQM_AEQ_BASE_T_TOE = 64,    /* <TOE consists of 16 events:64~95 */
	CQM_AEQ_BASE_T_VBS = 96,    /* <VBS consists of 16 events:96~111 */
	CQM_AEQ_BASE_T_IPSEC = 112, /* <VBS consists of 16 events:112~127 */
	CQM_AEQ_BASE_T_MAX = 128    /* <maximum of 128 events can be defined */
};

/* service registration template */
struct tag_service_register_template {
	u32 service_type;           /* <service type */
	u32 srq_ctx_size;           /* <SRQ context size */
	u32 scq_ctx_size;           /* <SCQ context size */
	void *service_handle;       /* <pointer to the service driver when the
				     * ceq/aeq function is called back
				     */
	/* <ceq callback:shared cq */
	void (*shared_cq_ceq_callback)(void *service_handle, u32 cqn,
				       void *cq_priv);
	/* <ceq callback:embedded cq */
	void (*embedded_cq_ceq_callback)(void *service_handle, u32 xid,
					 void *qpc_priv);
	/* <ceq callback:no cq */
	void (*no_cq_ceq_callback)(void *service_handle, u32 xid, u32 qid,
				   void *qpc_priv);
	/* <aeq level callback */
	u8 (*aeq_level_callback)(void *service_handle, u8 event_type, u8 *val);
	/* <aeq callback */
	void (*aeq_callback)(void *service_handle, u8 event_type, u8 *val);
};

/* object operation type definition */
enum cqm_object_type {
	CQM_OBJECT_ROOT_CTX = 0, /* <0:root context, which is compatible with
				  * root CTX management
				  */
	CQM_OBJECT_SERVICE_CTX,  /* <1:QPC, connection management object */
	CQM_OBJECT_MPT,		 /* <2:RDMA service usage */

	CQM_OBJECT_NONRDMA_EMBEDDED_RQ = 10, /* <10:RQ of non-RDMA services,
					      * managed by LINKWQE
					      */
	CQM_OBJECT_NONRDMA_EMBEDDED_SQ,      /* <11:SQ of non-RDMA services,
					      * managed by LINKWQE
					      */
	CQM_OBJECT_NONRDMA_SRQ,              /* <12:SRQ of non-RDMA services,
					      * managed by MTT, but the CQM
					      * needs to apply for MTT.
					      */
	CQM_OBJECT_NONRDMA_EMBEDDED_CQ,      /* <13:Embedded CQ for non-RDMA
					      * services, managed by LINKWQE
					      */
	CQM_OBJECT_NONRDMA_SCQ,              /* <14:SCQ of non-RDMA services,
					      * managed by LINKWQE
					      */

	CQM_OBJECT_RESV = 20,

	CQM_OBJECT_RDMA_QP = 30, /* <30:QP of RDMA services, managed by MTT */
	CQM_OBJECT_RDMA_SRQ,     /* <31:SRQ of RDMA services, managed by MTT */
	CQM_OBJECT_RDMA_SCQ,     /* <32:SCQ of RDMA services, managed by MTT */

	CQM_OBJECT_MTT = 50,     /* <50:MTT table of the RDMA service */
	CQM_OBJECT_RDMARC,       /* <51:RC of the RDMA service */
};

/* return value of the failure to apply for the BITMAP table */
#define CQM_INDEX_INVALID     (~(0U))
/* Return value of the reserved bit applied for in the BITMAP table,
 * indicating that the index is allocated by the CQM and
 * cannot be specified by the driver.
 */
#define CQM_INDEX_RESERVED    0xfffff

/* to support ROCE Q buffer resize, the first Q buffer space */
#define CQM_RDMA_Q_ROOM_1     1
/* to support the Q buffer resize of ROCE, the second Q buffer space */
#define CQM_RDMA_Q_ROOM_2     2

/* doorbell mode selected by the current Q, hardware doorbell */
#define CQM_HARDWARE_DOORBELL 1
/* doorbell mode selected by the current Q, software doorbell */
#define CQM_SOFTWARE_DOORBELL 2

/* single-node structure of the CQM buffer */
struct tag_cqm_buf_list {
	void *va;                 /* <virtual address */
	dma_addr_t pa;            /* <physical address */
	u32 refcount;             /* <reference counting of the buf,
				   * which is used for internal buf management.
				   */
};

/* common management structure of the CQM buffer */
struct tag_cqm_buf {
	struct tag_cqm_buf_list *buf_list;  /* <buffer list */
	struct tag_cqm_buf_list direct;     /* <map the discrete buffer list to a group
					     * of consecutive addresses
					     */
	u32 page_number;        /* <buf_number in quantity of page_number=2^n */
	u32 buf_number;         /* <number of buf_list nodes */
	u32 buf_size;           /* <PAGE_SIZE in quantity of buf_size=2^n */
	struct vram_buf_info buf_info;
	u32 bat_entry_type;
};

/* CQM object structure, which can be considered
 * as the base class abstracted from all queues/CTX.
 */
struct tag_cqm_object {
	u32 service_type;       /* <service type */
	u32 object_type;        /* <object type, such as context, queue, mpt,
				 * and mtt, etc
				 */
	u32 object_size;        /* <object Size, for queue/CTX/MPT,
				 * the unit is Byte, for MTT/RDMARC,
				 * the unit is the number of entries,
				 * for containers, the unit is the number of
				 * containers.
				 */
	atomic_t refcount;      /* <reference counting */
	struct completion free; /* <release completed quantity */
	void *cqm_handle;       /* <cqm_handle */
};

/* structure of the QPC and MPT objects of the CQM */
struct tag_cqm_qpc_mpt {
	struct tag_cqm_object object;    /* <object base class */
	u32 xid;	                 /* <xid */
	dma_addr_t paddr;                /* <physical address of the QPC/MTT memory */
	void *priv;                      /* <private information about the object of
					  * the service driver.
					  */
	u8 *vaddr;                       /* <virtual address of the QPC/MTT memory */
};

/* queue header structure */
struct tag_cqm_queue_header {
	u64 doorbell_record;    /* <SQ/RQ DB content */
	u64 ci_record;          /* <CQ DB content */
	u64 rsv1;               /* <This area is a user-defined area for driver
				 * and microcode information transfer.
				 */
	u64 rsv2;               /* <This area is a user-defined area for driver
				 * and microcode information transfer.
				 */
};

/* queue management structure: for queues of non-RDMA services, embedded queues
 * are managed by LinkWQE, SRQ and SCQ are managed by MTT, but MTT needs to be
 * applied by CQM; the queue of the RDMA service is managed by the MTT.
 */
struct tag_cqm_queue {
	struct tag_cqm_object object;       /* <object base class */
	u32 index;                          /* <The embedded queue and QP do not have
					     * indexes, but the SRQ and SCQ do.
					     */
	void *priv;                         /* <private information about the object of
					     * the service driver
					     */
	u32 current_q_doorbell;             /* <doorbell type selected by the current
					     * queue. HW/SW are used for the roce QP.
					     */
	u32 current_q_room;                 /* <roce:current valid room buf */
	struct tag_cqm_buf q_room_buf_1;    /* <nonrdma:only q_room_buf_1 can be set to
					     * q_room_buf
					     */
	struct tag_cqm_buf q_room_buf_2;    /* <The CQ of RDMA reallocates the size of
					     * the queue room.
					     */
	struct tag_cqm_queue_header *q_header_vaddr; /* <queue header virtual address */
	dma_addr_t q_header_paddr;                   /* <physical address of the queue header */
	u8 *q_ctx_vaddr;	                     /* <CTX virtual addresses of SRQ and SCQ */
	dma_addr_t q_ctx_paddr;                      /* <CTX physical addresses of SRQ and SCQ */
	u32 valid_wqe_num;                           /* <number of valid WQEs that are
						      * successfully created
						      */
	u8 *tail_container;                          /* <tail pointer of the SRQ container */
	u8 *head_container;                          /* <head pointer of SRQ container */
	u8 queue_link_mode;                          /* <Determine the connection mode during
						      * queue creation, such as link and ring.
						      */
};

/* MTT/RDMARC management structure */
struct tag_cqm_mtt_rdmarc {
	struct tag_cqm_object object;       /* <object base class */
	u32 index_base;                     /* <index_base */
	u32 index_number;                   /* <index_number */
	u8 *vaddr;	                    /* <buffer virtual address */
};

/* sending command structure */
struct tag_cqm_cmd_buf {
	void *buf;            /* <command buffer virtual address */
	dma_addr_t dma;       /* <physical address of the command buffer */
	u16 size;             /* <command buffer size */
};

/* definition of sending ACK mode */
enum cqm_cmd_ack_type {
	CQM_CMD_ACK_TYPE_CMDQ = 0,      /* <ack is written back to cmdq */
	CQM_CMD_ACK_TYPE_SHARE_CQN = 1, /* <ack is reported through the SCQ of
					 * the root CTX.
					 */
	CQM_CMD_ACK_TYPE_APP_CQN = 2    /* <ack is reported through the SCQ of
					 * service
					 */
};

#endif
/**
 * @brief: create FC SRQ.
 * @details: The number of valid WQEs in the queue must meet the number of
 *	     transferred WQEs. Linkwqe can only be filled at the end of the
 *	     page. The actual number of valid links exceeds the requirement.
 *	     The service needs to be informed of the number of extra links to
 *	     be created.
 * @param ex_handle: device pointer that represents the PF
 * @param service_type: service type
 * @param object_type: object type
 * @param wqe_number: number of WQEs
 * @param wqe_size: wqe size
 * @param object_priv: pointer to object private information
 * @retval struct tag_cqm_queue*: queue structure pointer
 * @date: 2019-5-4
 */
struct tag_cqm_queue *cqm_object_fc_srq_create(void *ex_handle, u32 service_type,
					       enum cqm_object_type object_type,
					       u32 wqe_number, u32 wqe_size,
					       void *object_priv);

/**
 * @brief: create RQ.
 * @details: When SRQ is used, the RQ queue is created.
 * @param ex_handle: device pointer that represents the PF
 * @param service_type: service type
 * @param object_type: object type
 * @param init_rq_num: number of containers
 * @param container_size: container size
 * @param wqe_size: wqe size
 * @param object_priv: pointer to object private information
 * @retval struct tag_cqm_queue*: queue structure pointer
 * @date: 2019-5-4
 */
struct tag_cqm_queue *cqm_object_recv_queue_create(void *ex_handle, u32 service_type,
						   enum cqm_object_type object_type,
						   u32 init_rq_num, u32 container_size,
						   u32 wqe_size, void *object_priv);

/**
 * @brief: SRQ applies for a new container and is linked after the container
 *	   is created.
 * @details: SRQ applies for a new container and is linked after the container
 *	     is created.
 * @param common: queue structure pointer
 * @retval 0: success
 * @retval -1: fail
 * @date: 2019-5-4
 */
s32 cqm_object_share_recv_queue_add_container(struct tag_cqm_queue *common);

/**
 * @brief: SRQ applies for a new container. After the container is created,
 *	   no link is attached to the container. The service is attached to
 *	   the container.
 * @details: SRQ applies for a new container. After the container is created,
 *	     no link is attached to the container. The service is attached to
 *	     the container.
 * @param common: queue structure pointer
 * @param container_addr: returned container address
 * @retval 0: success
 * @retval -1: fail
 * @date: 2019-5-4
 */
s32 cqm_object_srq_add_container_free(struct tag_cqm_queue *common, u8 **container_addr);

/**
 * @brief: create SRQ for TOE services.
 * @details: create SRQ for TOE services.
 * @param ex_handle: device pointer that represents the PF
 * @param service_type: service type
 * @param object_type: object type
 * @param container_number: number of containers
 * @param container_size: container size
 * @param wqe_size: wqe size
 * @retval struct tag_cqm_queue*: queue structure pointer
 * @date: 2019-5-4
 */
struct tag_cqm_queue *cqm_object_share_recv_queue_create(void *ex_handle,
							 u32 service_type,
							 enum cqm_object_type object_type,
							 u32 container_number,
							 u32 container_size,
							 u32 wqe_size);

/**
 * @brief: create QPC and MPT.
 * @details: When QPC and MPT are created, the interface sleeps.
 * @param ex_handle: device pointer that represents the PF
 * @param service_type: service type
 * @param object_type: object type
 * @param object_size: object size, in bytes.
 * @param object_priv: private structure of the service layer.
 *		       The value can be NULL.
 * @param index: apply for reserved qpn based on the value. If automatic
 *		 allocation is required, fill CQM_INDEX_INVALID.
 * @retval struct tag_cqm_qpc_mpt *: pointer to the QPC/MPT structure
 * @date: 2019-5-4
 */
struct tag_cqm_qpc_mpt *cqm_object_qpc_mpt_create(void *ex_handle, u32 service_type,
						  enum cqm_object_type object_type,
						  u32 object_size, void *object_priv,
						  u32 index, bool low2bit_align_en);

/**
 * @brief: create a queue for non-RDMA services.
 * @details: create a queue for non-RDMA services. The interface sleeps.
 * @param ex_handle: device pointer that represents the PF
 * @param service_type: service type
 * @param object_type: object type
 * @param wqe_number: number of Link WQEs
 * @param wqe_size: fixed length, size 2^n
 * @param object_priv: private structure of the service layer.
 *		       The value can be NULL.
 * @retval struct tag_cqm_queue *: queue structure pointer
 * @date: 2019-5-4
 */
struct tag_cqm_queue *cqm_object_nonrdma_queue_create(void *ex_handle, u32 service_type,
						      enum cqm_object_type object_type,
						      u32 wqe_number, u32 wqe_size,
						      void *object_priv);

/**
 * @brief: create a RDMA service queue.
 * @details: create a queue for the RDMA service. The interface sleeps.
 * @param ex_handle: device pointer that represents the PF
 * @param service_type: service type
 * @param object_type: object type
 * @param object_size: object size
 * @param object_priv: private structure of the service layer.
 *		       The value can be NULL.
 * @param room_header_alloc: whether to apply for the queue room and header
 *			     space
 * @retval struct tag_cqm_queue *: queue structure pointer
 * @date: 2019-5-4
 */
struct tag_cqm_queue *cqm_object_rdma_queue_create(void *ex_handle, u32 service_type,
						   enum cqm_object_type object_type,
						   u32 object_size, void *object_priv,
						   bool room_header_alloc, u32 xid);

/**
 * @brief: create the MTT and RDMARC of the RDMA service.
 * @details: create the MTT and RDMARC of the RDMA service.
 * @param ex_handle: device pointer that represents the PF
 * @param service_type: service type
 * @param object_type: object type
 * @param index_base: start index number
 * @param index_number: index number
 * @retval struct tag_cqm_mtt_rdmarc *: pointer to the MTT/RDMARC structure
 * @date: 2019-5-4
 */
struct tag_cqm_mtt_rdmarc *cqm_object_rdma_table_get(void *ex_handle, u32 service_type,
						     enum cqm_object_type object_type,
						     u32 index_base, u32 index_number);

/**
 * @brief: delete created objects.
 * @details: delete the created object. This function does not return until all
 *	     operations on the object are complete.
 * @param object: object pointer
 * @retval: void
 * @date: 2019-5-4
 */
void cqm_object_delete(struct tag_cqm_object *object);

/**
 * @brief: obtains the physical address and virtual address at the specified
 *	   offset of the object buffer.
 * @details: Only RDMA table query is supported to obtain the physical address
 *	     and virtual address at the specified offset of the object buffer.
 * @param object: object pointer
 * @param offset: for a rdma table, offset is the absolute index number.
 * @param paddr: The physical address is returned only for the rdma table.
 * @retval u8 *: buffer specify the virtual address at the offset
 * @date: 2019-5-4
 */
u8 *cqm_object_offset_addr(struct tag_cqm_object *object, u32 offset, dma_addr_t *paddr);

/**
 * @brief: obtain object according index.
 * @details: obtain object according index.
 * @param ex_handle: device pointer that represents the PF
 * @param object_type: object type
 * @param index: support qpn,mptn,scqn,srqn
 * @param bh: whether to disable the bottom half of the interrupt
 * @retval struct tag_cqm_object *: object pointer
 * @date: 2019-5-4
 */
struct tag_cqm_object *cqm_object_get(void *ex_handle, enum cqm_object_type object_type,
				      u32 index, bool bh);

/**
 * @brief: object reference counting release
 * @details: After the function cqm_object_get is invoked, this API must be put.
 *	     Otherwise, the object cannot be released.
 * @param object: object pointer
 * @retval: void
 * @date: 2019-5-4
 */
void cqm_object_put(struct tag_cqm_object *object);

/**
 * @brief: obtain the ID of the function where the object resides.
 * @details: obtain the ID of the function where the object resides.
 * @param object: object pointer
 * @retval >=0: ID of function
 * @retval -1: fail
 * @date: 2020-4-15
 */
s32 cqm_object_funcid(struct tag_cqm_object *object);

/**
 * @brief: apply for a new space for an object.
 * @details: Currently, this parameter is valid only for the ROCE service.
 *	     The CQ buffer size is adjusted, but the CQN and CQC remain
 *	     unchanged. New buffer space is applied for, and the old buffer
 *	     space is not released. The current valid buffer is still the old
 *	     buffer.
 * @param object: object pointer
 * @param object_size: new buffer size
 * @retval 0: success
 * @retval -1: fail
 * @date: 2019-5-4
 */
s32 cqm_object_resize_alloc_new(struct tag_cqm_object *object, u32 object_size);

/**
 * @brief: release the newly applied buffer space for the object.
 * @details: This function is used to release the newly applied buffer space for
 *	     service exception handling.
 * @param object: object pointer
 * @retval: void
 * @date: 2019-5-4
 */
void cqm_object_resize_free_new(struct tag_cqm_object *object);

/**
 * @brief: release old buffer space for objects.
 * @details: This function releases the old buffer and sets the current valid
 *	     buffer to the new buffer.
 * @param object: object pointer
 * @retval: void
 * @date: 2019-5-4
 */
void cqm_object_resize_free_old(struct tag_cqm_object *object);

/**
 * @brief: release container.
 * @details: release container.
 * @param object: object pointer
 * @param container: container pointer to be released
 * @retval: void
 * @date: 2019-5-4
 */
void cqm_srq_used_rq_container_delete(struct tag_cqm_object *object, u8 *container);

void *cqm_get_db_addr(void *ex_handle, u32 service_type);

s32 cqm_ring_hardware_db_fc(void *ex_handle, u32 service_type, u8 db_count,
			    u8 pagenum, u64 db);

/**
 * @brief: provide the interface of knocking on doorbell.
 *	   The CQM converts the pri to cos.
 * @details: provide interface of knocking on doorbell for the CQM to convert
 *	     the pri to cos. The doorbell transferred by the service must be the
 *	     host sequence. This interface converts the network sequence.
 * @param ex_handle: device pointer that represents the PF
 * @param service_type: Each kernel-mode service is allocated a hardware
 *			doorbell page.
 * @param db_count: PI[7:0] beyond 64b in the doorbell
 * @param db: The doorbell content is organized by the service. If there is
 *	      endian conversion, the service needs to complete the conversion.
 * @retval 0: success
 * @retval -1: fail
 * @date: 2019-5-4
 */
s32 cqm_ring_hardware_db_update_pri(void *ex_handle, u32 service_type,
				    u8 db_count, u64 db);

/**
 * @brief: knock on software doorbell.
 * @details: knock on software doorbell.
 * @param object: object pointer
 * @param db_record: software doorbell content. If there is big-endian
 *		     conversion, the service needs to complete the conversion.
 * @retval 0: success
 * @retval -1: fail
 * @date: 2019-5-4
 */
s32 cqm_ring_software_db(struct tag_cqm_object *object, u64 db_record);

/**
 * @brief: reference counting is added to the bloom filter ID.
 * @details: reference counting is added to the bloom filter ID. When the ID
 *	     changes from 0 to 1, the sending API is set to 1.
 *	     This interface sleeps.
 * @param ex_handle: device pointer that represents the PF
 * @param id: id
 * @retval 0: success
 * @retval -1: fail
 * @date: 2019-5-4
 */
void *cqm_gid_base(void *ex_handle);

/**
 * @brief: obtain the base virtual address of the timer.
 * @details: obtain the base virtual address of the timer.
 * @param ex_handle: device pointer that represents the PF
 * @retval void *: base virtual address of the timer
 * @date: 2020-5-21
 */
void *cqm_timer_base(void *ex_handle);

/**
 * @brief: clear timer buffer.
 * @details: clear the timer buffer based on the function ID. Function IDs start
 *	     from 0, and timer buffers are arranged by function ID.
 * @param ex_handle: device pointer that represents the PF
 * @param function_id: function id
 * @retval: void
 * @date: 2019-5-4
 */
void cqm_function_timer_clear(void *ex_handle, u32 function_id);

/**
 * @brief: clear hash buffer.
 * @details: clear the hash buffer based on the function ID.
 * @param ex_handle: device pointer that represents the PF
 * @param global_funcid
 * @retval: void
 * @date: 2019-5-4
 */
void cqm_function_hash_buf_clear(void *ex_handle, s32 global_funcid);

s32 cqm_ring_direct_wqe_db(void *ex_handle, u32 service_type, u8 db_count,
			   void *direct_wqe);
s32 cqm_ring_direct_wqe_db_fc(void *ex_handle, u32 service_type,
			      void *direct_wqe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* CQM_OBJECT_H */
