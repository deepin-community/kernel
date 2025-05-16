/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef CQM_H
#define CQM_H

#include <linux/completion.h>

#ifndef HIUDK_SDK

#include "hinic3_cqm_define.h"
#include "vram_common.h"

#define CQM_SUCCESS 0
#define CQM_FAIL (-1)
#define CQM_CONTINUE 1

#define CQM_WQE_WF_LINK 1
#define CQM_WQE_WF_NORMAL 0

#define CQM_QUEUE_LINK_MODE 0
#define CQM_QUEUE_RING_MODE 1
#define CQM_QUEUE_TOE_SRQ_LINK_MODE 2
#define CQM_QUEUE_RDMA_QUEUE_MODE 3

struct tag_cqm_linkwqe {
	u32 rsv1 : 14;
	u32 wf : 1;
	u32 rsv2 : 14;
	u32 ctrlsl : 2;
	u32 o : 1;

	u32 rsv3 : 31;
	u32 lp : 1;		/* lp define o-bit is flipping */

	u32 next_page_gpa_h;	/* Record the upper 32 bits of the PADDR of the next page */
	u32 next_page_gpa_l;	/* Record the lower 32 bits of the PADDR of the next page */

	u32 next_buffer_addr_h;	/* Record the upper 32 bits of the VADDR of the next page */
	u32 next_buffer_addr_l;	/* Record the lower 32 bits of the VADDR of the next page */
};

/* The WQE size cannot exceed the common RQE size. */
struct tag_cqm_srq_linkwqe {
	struct tag_cqm_linkwqe linkwqe;
	u32 current_buffer_gpa_h;
	u32 current_buffer_gpa_l;
	u32 current_buffer_addr_h;
	u32 current_buffer_addr_l;

	u32 fast_link_page_addr_h;
	u32 fast_link_page_addr_l;

	u32 fixed_next_buffer_addr_h;
	u32 fixed_next_buffer_addr_l;
};

/* First 64B of standard 128B WQE */
union tag_cqm_linkwqe_first64B {
	struct tag_cqm_linkwqe basic_linkwqe;
	struct tag_cqm_srq_linkwqe toe_srq_linkwqe;
	u32 value[16];
};

/* Last 64 bytes of the standard 128-byte WQE */
struct tag_cqm_linkwqe_second64B {
	u32 rsvd0[4];
	u32 rsvd1[4];
	union {
		struct {
			u32 rsvd0[3];
			u32 rsvd1 : 29;
			u32 toe_o : 1;
			u32 resvd2 : 2;
		} bs;
		u32 value[4];
	} third_16B;

	union {
		struct {
			u32 rsvd0[2];
			u32 rsvd1 : 31;
			u32 ifoe_o : 1;
			u32 rsvd2;
		} bs;
		u32 value[4];
	} forth_16B;
};

/* Standard 128B WQE structure */
struct tag_cqm_linkwqe_128B {
	union tag_cqm_linkwqe_first64B first64B;
	struct tag_cqm_linkwqe_second64B second64B;
};

enum cqm_aeq_event_type {
	CQM_AEQ_BASE_T_NIC = 0,
	CQM_AEQ_BASE_T_ROCE = 16,
	CQM_AEQ_BASE_T_FC = 48,
	CQM_AEQ_BASE_T_IOE = 56,
	CQM_AEQ_BASE_T_TOE = 64,
	CQM_AEQ_BASE_T_VBS = 96,
	CQM_AEQ_BASE_T_IPSEC = 112,
	CQM_AEQ_BASE_T_MAX = 128
};

struct tag_service_register_template {
	u32 service_type;
	u32 srq_ctx_size;
	u32 scq_ctx_size;
	void *service_handle;	/* The ceq/aeq function is called back */
	void (*shared_cq_ceq_callback)(void *service_handle, u32 cqn, void *cq_priv);
	void (*embedded_cq_ceq_callback)(void *service_handle, u32 xid, void *qpc_priv);
	void (*no_cq_ceq_callback)(void *service_handle, u32 xid, u32 qid, void *qpc_priv);
	u8 (*aeq_level_callback)(void *service_handle, u8 event_type, u8 *val);
	void (*aeq_callback)(void *service_handle, u8 event_type, u8 *val);
};

enum cqm_object_type {
	CQM_OBJECT_ROOT_CTX = 0,		///<0:root context
	CQM_OBJECT_SERVICE_CTX,			///<1:QPC
	CQM_OBJECT_MPT,				///<2:RDMA

	CQM_OBJECT_NONRDMA_EMBEDDED_RQ = 10,
	CQM_OBJECT_NONRDMA_EMBEDDED_SQ,
	CQM_OBJECT_NONRDMA_SRQ,
	CQM_OBJECT_NONRDMA_EMBEDDED_CQ,
	CQM_OBJECT_NONRDMA_SCQ,

	CQM_OBJECT_RESV = 20,

	CQM_OBJECT_RDMA_QP = 30,
	CQM_OBJECT_RDMA_SRQ,
	CQM_OBJECT_RDMA_SCQ,

	CQM_OBJECT_MTT = 50,
	CQM_OBJECT_RDMARC,
};

#define CQM_INDEX_INVALID ~(0U)
#define CQM_INDEX_RESERVED (0xfffff)

#define CQM_RDMA_Q_ROOM_1 (1)
#define CQM_RDMA_Q_ROOM_2 (2)

#define CQM_HARDWARE_DOORBELL (1)
#define CQM_SOFTWARE_DOORBELL (2)

struct tag_cqm_buf_list {
	void *va;
	dma_addr_t pa;
	u32 refcount;
};

struct tag_cqm_buf {
	struct tag_cqm_buf_list *buf_list;
	struct tag_cqm_buf_list direct;
	u32 page_number;
	u32 buf_number;
	u32 buf_size;
	struct vram_buf_info buf_info;
	u32 bat_entry_type;
};

struct completion;

struct tag_cqm_object {
	u32 service_type;
	u32 object_type;
	u32 object_size;
	atomic_t refcount;
	struct completion free;
	void *cqm_handle;
};

struct tag_cqm_qpc_mpt {
	struct tag_cqm_object object;
	u32 xid;
	dma_addr_t paddr;
	void *priv;
	u8 *vaddr;
};

struct tag_cqm_queue_header {
	u64 doorbell_record;
	u64 ci_record;
	u64 rsv1;
	u64 rsv2;
};

struct tag_cqm_queue {
	struct tag_cqm_object object;
	u32 index;
	void *priv;
	u32 current_q_doorbell;
	u32 current_q_room;
	struct tag_cqm_buf q_room_buf_1;
	struct tag_cqm_buf q_room_buf_2;
	struct tag_cqm_queue_header *q_header_vaddr;
	dma_addr_t q_header_paddr;
	u8 *q_ctx_vaddr;
	dma_addr_t q_ctx_paddr;
	u32 valid_wqe_num;
	u8 *tail_container;
	u8 *head_container;
	u8 queue_link_mode;
};

struct tag_cqm_mtt_rdmarc {
	struct tag_cqm_object object;
	u32 index_base;
	u32 index_number;
	u8 *vaddr;
};

struct tag_cqm_cmd_buf {
	void *buf;
	dma_addr_t dma;
	u16 size;
};

enum cqm_cmd_ack_type_e {
	CQM_CMD_ACK_TYPE_CMDQ = 0,
	CQM_CMD_ACK_TYPE_SHARE_CQN = 1,
	CQM_CMD_ACK_TYPE_APP_CQN = 2
};

#define CQM_CMD_BUF_LEN 0x800

#endif

#define  hiudk_cqm_object_delete(x, y) cqm_object_delete(y)
#define  hiudk_cqm_object_funcid(x, y) cqm_object_funcid(y)
#define  hiudk_cqm_object_offset_addr(x, y, z, m) cqm_object_offset_addr(y, z, m)
#define  hiudk_cqm_object_put(x, y) cqm_object_put(y)
#define  hiudk_cqm_object_resize_alloc_new(x, y, z) cqm_object_resize_alloc_new(y, z)
#define  hiudk_cqm_object_resize_free_new(x, y) cqm_object_resize_free_new(y)
#define  hiudk_cqm_object_resize_free_old(x, y) cqm_object_resize_free_old(y)
#define  hiudk_cqm_object_share_recv_queue_add_container(x, y) \
	cqm_object_share_recv_queue_add_container(y)
#define  hiudk_cqm_object_srq_add_container_free(x, y, z) cqm_object_srq_add_container_free(y, z)
#define  hiudk_cqm_ring_software_db(x, y, z) cqm_ring_software_db(y, z)
#define  hiudk_cqm_srq_used_rq_container_delete(x, y, z) cqm_srq_used_rq_container_delete(y, z)

s32 cqm3_init(void *ex_handle);
void cqm3_uninit(void *ex_handle);

s32 cqm3_service_register(void *ex_handle,
			  struct tag_service_register_template *service_template);
void cqm3_service_unregister(void *ex_handle, u32 service_type);
s32 cqm3_fake_vf_num_set(void *ex_handle, u16 fake_vf_num_cfg);
bool cqm3_need_secure_mem(void *ex_handle);
struct tag_cqm_queue *cqm3_object_fc_srq_create(void *ex_handle, u32 service_type,
						enum cqm_object_type object_type,
						u32 wqe_number, u32 wqe_size,
						void *object_priv);
struct tag_cqm_queue *cqm3_object_recv_queue_create(void *ex_handle, u32 service_type,
						    enum cqm_object_type object_type,
						    u32 init_rq_num, u32 container_size,
						    u32 wqe_size, void *object_priv);
struct tag_cqm_queue *cqm3_object_share_recv_queue_create(void *ex_handle, u32 service_type,
							  enum cqm_object_type object_type,
							  u32 container_number, u32 container_size,
							  u32 wqe_size);
struct tag_cqm_qpc_mpt *cqm3_object_qpc_mpt_create(void *ex_handle, u32 service_type,
						   enum cqm_object_type object_type,
						   u32 object_size, void *object_priv,
						   u32 index, bool low2bit_align_en);

struct tag_cqm_queue *cqm3_object_nonrdma_queue_create(void *ex_handle, u32 service_type,
						       enum cqm_object_type object_type,
						       u32 wqe_number, u32 wqe_size,
						       void *object_priv);
struct tag_cqm_queue *cqm3_object_rdma_queue_create(void *ex_handle, u32 service_type,
						    enum cqm_object_type object_type,
						    u32 object_size, void *object_priv,
						    bool room_header_alloc, u32 xid);
struct tag_cqm_mtt_rdmarc *cqm3_object_rdma_table_get(void *ex_handle, u32 service_type,
						      enum cqm_object_type object_type,
						      u32 index_base, u32 index_number);
struct tag_cqm_object *cqm3_object_get(void *ex_handle, enum cqm_object_type object_type,
				       u32 index, bool bh);
struct tag_cqm_cmd_buf *cqm3_cmd_alloc(void *ex_handle);
void cqm3_cmd_free(void *ex_handle, struct tag_cqm_cmd_buf *cmd_buf);

s32 cqm3_send_cmd_box(void *ex_handle, u8 mod, u8 cmd,
		      struct tag_cqm_cmd_buf *buf_in, struct tag_cqm_cmd_buf *buf_out,
		      u64 *out_param, u32 timeout, u16 channel);

s32 cqm3_lb_send_cmd_box(void *ex_handle, u8 mod, u8 cmd, u8 cos_id,
			 struct tag_cqm_cmd_buf *buf_in, struct tag_cqm_cmd_buf *buf_out,
			 u64 *out_param, u32 timeout, u16 channel);
s32 cqm3_lb_send_cmd_box_async(void *ex_handle, u8 mod, u8 cmd, u8 cos_id,
			       struct tag_cqm_cmd_buf *buf_in, u16 channel);

s32 cqm3_db_addr_alloc(void *ex_handle, void __iomem **db_addr, void __iomem **dwqe_addr);
void cqm3_db_addr_free(void *ex_handle, const void __iomem *db_addr,
		       void __iomem *dwqe_addr);

void *cqm3_get_db_addr(void *ex_handle, u32 service_type);
s32 cqm3_ring_hardware_db(void *ex_handle, u32 service_type, u8 db_count, u64 db);

s32 cqm_ring_hardware_db_fc(void *ex_handle, u32 service_type, u8 db_count, u8 pagenum, u64 db);
s32 cqm3_ring_hardware_db_update_pri(void *ex_handle, u32 service_type, u8 db_count, u64 db);
s32 cqm3_bloomfilter_inc(void *ex_handle, u16 func_id, u64 id);
s32 cqm3_bloomfilter_dec(void *ex_handle, u16 func_id, u64 id);
void *cqm3_gid_base(void *ex_handle);
void *cqm3_timer_base(void *ex_handle);
void cqm3_function_timer_clear(void *ex_handle, u32 function_id);
void cqm3_function_hash_buf_clear(void *ex_handle, s32 global_funcid);
s32 cqm3_ring_direct_wqe_db(void *ex_handle, u32 service_type, u8 db_count, void *direct_wqe);
s32 cqm_ring_direct_wqe_db_fc(void *ex_handle, u32 service_type, void *direct_wqe);

s32 cqm3_object_share_recv_queue_add_container(struct tag_cqm_queue *common);
s32 cqm3_object_srq_add_container_free(struct tag_cqm_queue *common, u8 **container_addr);

s32 cqm3_ring_software_db(struct tag_cqm_object *object, u64 db_record);
void cqm3_object_put(struct tag_cqm_object *object);

/**
 * @brief Obtains the function ID of an object.
 * @param Object Pointer
 * @retval >=0 function's ID
 * @retval -1 Fails
 */
s32 cqm3_object_funcid(struct tag_cqm_object *object);

s32 cqm3_object_resize_alloc_new(struct tag_cqm_object *object, u32 object_size);
void cqm3_object_resize_free_new(struct tag_cqm_object *object);
void cqm3_object_resize_free_old(struct tag_cqm_object *object);

/**
 * @brief Releasing a container
 * @param Object Pointer
 * @param container Pointer to the container to be released
 * @retval void
 */
void cqm3_srq_used_rq_container_delete(struct tag_cqm_object *object, u8 *container);

void cqm3_object_delete(struct tag_cqm_object *object);

/**
 * @brief Obtains the PADDR and VADDR of the specified offset in the object buffer.
 * @details Only rdma table lookup is supported
 * @param Object Pointer
 * @param offset For an RDMA table, the offset is the absolute index number.
 * @param paddr The physical address is returned only for the RDMA table.
 * @retval u8 *buffer Virtual address at specified offset
 */
u8 *cqm3_object_offset_addr(struct tag_cqm_object *object, u32 offset, dma_addr_t *paddr);

#endif /* CQM_H */

