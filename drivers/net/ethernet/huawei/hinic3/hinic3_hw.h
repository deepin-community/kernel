/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_HW_H
#define HINIC3_HW_H

#include "mpu_inband_cmd.h"
#include "mpu_inband_cmd_defs.h"

#include "hinic3_crm.h"

#ifndef BIG_ENDIAN
#define BIG_ENDIAN	0x4321
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN	0x1234
#endif

#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif
/* X86 */
#define BYTE_ORDER	LITTLE_ENDIAN

/* to use 0-level CLA, page size must be: SQ 16B(wqe) * 64k(max_q_depth) */
#define HINIC3_DEFAULT_WQ_PAGE_SIZE		0x100000
#define HINIC3_HW_WQ_PAGE_SIZE			0x1000
#define HINIC3_MAX_WQ_PAGE_SIZE_ORDER		8
#define SPU_HOST_ID 4

enum hinic3_channel_id {
	HINIC3_CHANNEL_DEFAULT,
	HINIC3_CHANNEL_COMM,
	HINIC3_CHANNEL_NIC,
	HINIC3_CHANNEL_ROCE,
	HINIC3_CHANNEL_TOE,
	HINIC3_CHANNEL_FC,
	HINIC3_CHANNEL_OVS,
	HINIC3_CHANNEL_DSW,
	HINIC3_CHANNEL_MIG,
	HINIC3_CHANNEL_CRYPT,
	HINIC3_CHANNEL_VROCE,

	HINIC3_CHANNEL_MAX = 32,
};

struct hinic3_cmd_buf {
	void		*buf;
	dma_addr_t	dma_addr;
	u16		size;
	/* Usage count, USERS DO NOT USE */
	atomic_t	ref_cnt;
};

enum hinic3_aeq_type {
	HINIC3_HW_INTER_INT = 0,
	HINIC3_MBX_FROM_FUNC = 1,
	HINIC3_MSG_FROM_MGMT_CPU = 2,
	HINIC3_API_RSP = 3,
	HINIC3_API_CHAIN_STS = 4,
	HINIC3_MBX_SEND_RSLT = 5,
	HINIC3_MAX_AEQ_EVENTS
};

enum hinic3_aeq_sw_type {
	HINIC3_STATELESS_EVENT = 0,
	HINIC3_STATEFUL_EVENT = 1,
	HINIC3_MAX_AEQ_SW_EVENTS
};

enum hinic3_hwdev_init_state {
	HINIC3_HWDEV_NONE_INITED = 0,
	HINIC3_HWDEV_MGMT_INITED,
	HINIC3_HWDEV_MBOX_INITED,
	HINIC3_HWDEV_CMDQ_INITED,
};

enum hinic3_ceq_event {
	HINIC3_NON_L2NIC_SCQ,
	HINIC3_NON_L2NIC_ECQ,
	HINIC3_NON_L2NIC_NO_CQ_EQ,
	HINIC3_CMDQ,
	HINIC3_L2NIC_SQ,
	HINIC3_L2NIC_RQ,
	HINIC3_MAX_CEQ_EVENTS,
};

enum hinic3_mbox_seg_errcode {
	MBOX_ERRCODE_NO_ERRORS		= 0,
	/* VF send the mailbox data to the wrong destination functions */
	MBOX_ERRCODE_VF_TO_WRONG_FUNC	= 0x100,
	/* PPF send the mailbox data to the wrong destination functions */
	MBOX_ERRCODE_PPF_TO_WRONG_FUNC	= 0x200,
	/* PF send the mailbox data to the wrong destination functions */
	MBOX_ERRCODE_PF_TO_WRONG_FUNC	= 0x300,
	/* The mailbox data size is set to all zero */
	MBOX_ERRCODE_ZERO_DATA_SIZE	= 0x400,
	/* The sender function attribute has not been learned by hardware */
	MBOX_ERRCODE_UNKNOWN_SRC_FUNC	= 0x500,
	/* The receiver function attr has not been learned by hardware */
	MBOX_ERRCODE_UNKNOWN_DES_FUNC	= 0x600,
};

struct hinic3_ceq_info {
	u32 q_len;
	u32 page_size;
	u16 elem_size;
	u16 num_pages;
	u32 num_elem_in_pg;
};

typedef void (*hinic3_aeq_hwe_cb)(void *pri_handle, u8 *data, u8 size);
typedef u8 (*hinic3_aeq_swe_cb)(void *pri_handle, u8 event, u8 *data);
typedef void (*hinic3_ceq_event_cb)(void *pri_handle, u32 ceqe_data);

typedef int (*hinic3_vf_mbox_cb)(void *pri_handle,
	u16 cmd, void *buf_in, u16 in_size, void *buf_out, u16 *out_size);

typedef int (*hinic3_pf_mbox_cb)(void *pri_handle,
	u16 vf_id, u16 cmd, void *buf_in, u16 in_size, void *buf_out, u16 *out_size);

typedef int (*hinic3_ppf_mbox_cb)(void *pri_handle, u16 pf_idx,
	u16 vf_id, u16 cmd, void *buf_in, u16 in_size, void *buf_out, u16 *out_size);

typedef int (*hinic3_pf_recv_from_ppf_mbox_cb)(void *pri_handle,
	u16 cmd, void *buf_in, u16 in_size, void *buf_out, u16 *out_size);

/**
 * @brief hinic3_aeq_register_hw_cb - register aeq hardware callback
 * @param hwdev: device pointer to hwdev
 * @param event: event type
 * @param hwe_cb: callback function
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_aeq_register_hw_cb(void *hwdev, void *pri_handle,
			      enum hinic3_aeq_type event, hinic3_aeq_hwe_cb hwe_cb);

/**
 * @brief hinic3_aeq_unregister_hw_cb - unregister aeq hardware callback
 * @param hwdev: device pointer to hwdev
 * @param event: event type
 **/
void hinic3_aeq_unregister_hw_cb(void *hwdev, enum hinic3_aeq_type event);

/**
 * @brief hinic3_aeq_register_swe_cb - register aeq soft event callback
 * @param hwdev: device pointer to hwdev
 * @pri_handle: the pointer to private invoker device
 * @param event: event type
 * @param aeq_swe_cb: callback function
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_aeq_register_swe_cb(void *hwdev, void *pri_handle, enum hinic3_aeq_sw_type event,
			       hinic3_aeq_swe_cb aeq_swe_cb);

/**
 * @brief hinic3_aeq_unregister_swe_cb - unregister aeq soft event callback
 * @param hwdev: device pointer to hwdev
 * @param event: event type
 **/
void hinic3_aeq_unregister_swe_cb(void *hwdev, enum hinic3_aeq_sw_type event);

/**
 * @brief hinic3_ceq_register_cb - register ceq callback
 * @param hwdev: device pointer to hwdev
 * @param event: event type
 * @param callback: callback function
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_ceq_register_cb(void *hwdev, void *pri_handle, enum hinic3_ceq_event event,
			   hinic3_ceq_event_cb callback);
/**
 * @brief hinic3_ceq_unregister_cb - unregister ceq callback
 * @param hwdev: device pointer to hwdev
 * @param event: event type
 **/
void hinic3_ceq_unregister_cb(void *hwdev, enum hinic3_ceq_event event);

/**
 * @brief hinic3_register_ppf_mbox_cb - ppf register mbox msg callback
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param pri_handle: private data will be used by the callback
 * @param callback: callback function
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_register_ppf_mbox_cb(void *hwdev, u8 mod, void *pri_handle,
				hinic3_ppf_mbox_cb callback);

/**
 * @brief hinic3_register_pf_mbox_cb - pf register mbox msg callback
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param pri_handle: private data will be used by the callback
 * @param callback: callback function
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_register_pf_mbox_cb(void *hwdev, u8 mod, void *pri_handle,
			       hinic3_pf_mbox_cb callback);
/**
 * @brief hinic3_register_vf_mbox_cb - vf register mbox msg callback
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param pri_handle: private data will be used by the callback
 * @param callback: callback function
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_register_vf_mbox_cb(void *hwdev, u8 mod, void *pri_handle,
			       hinic3_vf_mbox_cb callback);

/**
 * @brief hinic3_unregister_ppf_mbox_cb - ppf unregister mbox msg callback
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 **/
void hinic3_unregister_ppf_mbox_cb(void *hwdev, u8 mod);

/**
 * @brief hinic3_unregister_pf_mbox_cb - pf register mbox msg callback
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 **/
void hinic3_unregister_pf_mbox_cb(void *hwdev, u8 mod);

/**
 * @brief hinic3_unregister_vf_mbox_cb - pf register mbox msg callback
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 **/
void hinic3_unregister_vf_mbox_cb(void *hwdev, u8 mod);

/**
 * @brief hinic3_unregister_ppf_to_pf_mbox_cb - unregister mbox msg callback
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 **/
void hinic3_unregister_ppf_to_pf_mbox_cb(void *hwdev, u8 mod);

typedef void (*hinic3_mgmt_msg_cb)(void *pri_handle,
				   u16 cmd, void *buf_in, u16 in_size,
				   void *buf_out, u16 *out_size);

/**
 * @brief hinic3_register_service_adapter - register mgmt msg callback
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param pri_handle: private data will be used by the callback
 * @param callback: callback function
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_register_mgmt_msg_cb(void *hwdev, u8 mod, void *pri_handle,
				hinic3_mgmt_msg_cb callback);

/**
 * @brief hinic3_unregister_mgmt_msg_cb - unregister mgmt msg callback
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 **/
void hinic3_unregister_mgmt_msg_cb(void *hwdev, u8 mod);

/**
 * @brief hinic3_register_service_adapter - register service adapter
 * @param hwdev: device pointer to hwdev
 * @param service_adapter: service adapter
 * @param type: service type
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_register_service_adapter(void *hwdev, void *service_adapter,
				    enum hinic3_service_type type);

/**
 * @brief hinic3_unregister_service_adapter - unregister service adapter
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 **/
void hinic3_unregister_service_adapter(void *hwdev,
				       enum hinic3_service_type type);

/**
 * @brief hinic3_get_service_adapter - get service adapter
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 * @retval non-zero: success
 * @retval null: failure
 **/
void *hinic3_get_service_adapter(void *hwdev, enum hinic3_service_type type);

/**
 * @brief hinic3_alloc_db_phy_addr - alloc doorbell & direct wqe pyhsical addr
 * @param hwdev: device pointer to hwdev
 * @param db_base: pointer to alloc doorbell base address
 * @param dwqe_base: pointer to alloc direct base address
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_alloc_db_phy_addr(void *hwdev, u64 *db_base, u64 *dwqe_base);

/**
 * @brief hinic3_free_db_phy_addr - free doorbell & direct wqe physical address
 * @param hwdev: device pointer to hwdev
 * @param db_base: pointer to free doorbell base address
 * @param dwqe_base: pointer to free direct base address
 **/
void hinic3_free_db_phy_addr(void *hwdev, u64 db_base, u64 dwqe_base);

/**
 * @brief hinic3_alloc_db_addr - alloc doorbell & direct wqe
 * @param hwdev: device pointer to hwdev
 * @param db_base: pointer to alloc doorbell base address
 * @param dwqe_base: pointer to alloc direct base address
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_alloc_db_addr(void *hwdev, void __iomem **db_base,
			 void __iomem **dwqe_base);

/**
 * @brief hinic3_free_db_addr - free doorbell & direct wqe
 * @param hwdev: device pointer to hwdev
 * @param db_base: pointer to free doorbell base address
 * @param dwqe_base: pointer to free direct base address
 **/
void hinic3_free_db_addr(void *hwdev, const void __iomem *db_base,
			 void __iomem *dwqe_base);

/**
 * @brief hinic3_alloc_db_phy_addr - alloc physical doorbell & direct wqe
 * @param hwdev: device pointer to hwdev
 * @param db_base: pointer to alloc doorbell base address
 * @param dwqe_base: pointer to alloc direct base address
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_alloc_db_phy_addr(void *hwdev, u64 *db_base, u64 *dwqe_base);

/**
 * @brief hinic3_free_db_phy_addr - free physical doorbell & direct wqe
 * @param hwdev: device pointer to hwdev
 * @param db_base: free doorbell base address
 * @param dwqe_base: free direct base address
 **/

void hinic3_free_db_phy_addr(void *hwdev, u64 db_base, u64 dwqe_base);

/**
 * @brief hinic3_set_root_ctxt - set root context
 * @param hwdev: device pointer to hwdev
 * @param rq_depth: rq depth
 * @param sq_depth: sq depth
 * @param rx_buf_sz: rx buffer size
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_set_root_ctxt(void *hwdev, u32 rq_depth, u32 sq_depth,
			 int rx_buf_sz, u16 channel);

/**
 * @brief hinic3_clean_root_ctxt - clean root context
 * @param hwdev: device pointer to hwdev
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_clean_root_ctxt(void *hwdev, u16 channel);

/**
 * @brief hinic3_alloc_cmd_buf - alloc cmd buffer
 * @param hwdev: device pointer to hwdev
 * @retval non-zero: success
 * @retval null: failure
 **/
struct hinic3_cmd_buf *hinic3_alloc_cmd_buf(void *hwdev);

/**
 * @brief hinic3_free_cmd_buf - free cmd buffer
 * @param hwdev: device pointer to hwdev
 * @param cmd_buf: cmd buffer to free
 **/
void hinic3_free_cmd_buf(void *hwdev, struct hinic3_cmd_buf *cmd_buf);

/**
 * hinic3_sm_ctr_rd16 - small single 16 counter read
 * @hwdev: the hardware device
 * @node: the node id
 * @ctr_id: counter id
 * @value: read counter value ptr
 * Return: 0 - success, negative - failure
 **/
int hinic3_sm_ctr_rd16(void *hwdev, u8 node, u8 instance, u32 ctr_id, u16 *value);

/**
 * hinic3_sm_ctr_rd16_clear - small single 16 counter read clear
 * @hwdev: the hardware device
 * @node: the node id
 * @ctr_id: counter id
 * @value: read counter value ptr
 * Return: 0 - success, negative - failure
 **/
int hinic3_sm_ctr_rd16_clear(void *hwdev, u8 node, u8 instance, u32 ctr_id, u16 *value);

/**
 * @brief hinic3_sm_ctr_rd32 - small single 32 counter read
 * @param hwdev: device pointer to hwdev
 * @param node: the node id
 * @param instance: instance id
 * @param ctr_id: counter id
 * @param value: read counter value ptr
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_sm_ctr_rd32(void *hwdev, u8 node, u8 instance, u32 ctr_id,
		       u32 *value);
/**
 * @brief hinic3_sm_ctr_rd32_clear - small single 32 counter read clear
 * @param hwdev: device pointer to hwdev
 * @param node: the node id
 * @param instance: instance id
 * @param ctr_id: counter id
 * @param value: read counter value ptr
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_sm_ctr_rd32_clear(void *hwdev, u8 node, u8 instance,
			     u32 ctr_id, u32 *value);

/**
 * @brief hinic3_sm_ctr_rd64_pair - big pair 128 counter read
 * @param hwdev: device pointer to hwdev
 * @param node: the node id
 * @param instance: instance id
 * @param ctr_id: counter id
 * @param value1: read counter value ptr
 * @param value2: read counter value ptr
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_sm_ctr_rd64_pair(void *hwdev, u8 node, u8 instance,
			    u32 ctr_id, u64 *value1, u64 *value2);

/**
 * hinic3_sm_ctr_rd64_pair_clear - big pair 128 counter read
 * @hwdev: the hardware device
 * @node: the node id
 * @ctr_id: counter id
 * @value1: read counter value ptr
 * @value2: read counter value ptr
 * Return: 0 - success, negative - failure
 **/
int hinic3_sm_ctr_rd64_pair_clear(void *hwdev, u8 node, u8 instance,
				  u32 ctr_id, u64 *value1, u64 *value2);

/**
 * @brief hinic3_sm_ctr_rd64 - big counter 64 read
 * @param hwdev: device pointer to hwdev
 * @param node: the node id
 * @param instance: instance id
 * @param ctr_id: counter id
 * @param value: read counter value ptr
 * @retval zero: success
 * @retval non-zero: failure
 **/
int hinic3_sm_ctr_rd64(void *hwdev, u8 node, u8 instance, u32 ctr_id,
		       u64 *value);

/**
 * hinic3_sm_ctr_rd64_clear - big counter 64 read
 * @hwdev: the hardware device
 * @node: the node id
 * @ctr_id: counter id
 * @value: read counter value ptr
 * Return: 0 - success, negative - failure
 **/
int hinic3_sm_ctr_rd64_clear(void *hwdev, u8 node, u8 instance,
			     u32 ctr_id, u64 *value);

/**
 * @brief hinic3_api_csr_rd32 - read 32 byte csr
 * @param hwdev: device pointer to hwdev
 * @param dest: hardware node id
 * @param addr: reg address
 * @param val: reg value
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_api_csr_rd32(void *hwdev, u8 dest, u32 addr, u32 *val);

/**
 * @brief hinic3_api_csr_wr32 - write 32 byte csr
 * @param hwdev: device pointer to hwdev
 * @param dest: hardware node id
 * @param addr: reg address
 * @param val: reg value
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_api_csr_wr32(void *hwdev, u8 dest, u32 addr, u32 val);

/**
 * @brief hinic3_api_csr_rd64 - read 64 byte csr
 * @param hwdev: device pointer to hwdev
 * @param dest: hardware node id
 * @param addr: reg address
 * @param val: reg value
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_api_csr_rd64(void *hwdev, u8 dest, u32 addr, u64 *val);

/**
 * @brief hinic3_dbg_get_hw_stats - get hardware stats
 * @param hwdev: device pointer to hwdev
 * @param hw_stats: pointer to memory caller to alloc
 * @param out_size: out size
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_dbg_get_hw_stats(const void *hwdev, u8 *hw_stats, const u32 *out_size);

/**
 * @brief hinic3_dbg_clear_hw_stats - clear hardware stats
 * @param hwdev: device pointer to hwdev
 * @retval clear hardware size
 */
u16 hinic3_dbg_clear_hw_stats(void *hwdev);

/**
 * @brief hinic3_get_chip_fault_stats - get chip fault stats
 * @param hwdev: device pointer to hwdev
 * @param chip_fault_stats: pointer to memory caller to alloc
 * @param offset: offset
 */
void hinic3_get_chip_fault_stats(const void *hwdev, u8 *chip_fault_stats,
				 u32 offset);

/**
 * @brief hinic3_msg_to_mgmt_sync - msg to management cpu
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param buf_out: message buffer out
 * @param out_size: out buffer size
 * @param timeout: timeout
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_msg_to_mgmt_sync(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			    u16 in_size, void *buf_out, u16 *out_size,
			    u32 timeout, u16 channel);

/**
 * @brief hinic3_msg_to_mgmt_async - msg to management cpu async
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 *
 * The function does not sleep inside, allowing use in irq context
 */
int hinic3_msg_to_mgmt_async(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			     u16 in_size, u16 channel);

/**
 * @brief hinic3_msg_to_mgmt_no_ack - msg to management cpu don't need no ack
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 *
 * The function will sleep inside, and it is not allowed to be used in
 * interrupt context
 */
int hinic3_msg_to_mgmt_no_ack(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			      u16 in_size, u16 channel);

int hinic3_msg_to_mgmt_api_chain_async(void *hwdev, u8 mod, u16 cmd,
				       const void *buf_in, u16 in_size);

int hinic3_msg_to_mgmt_api_chain_sync(void *hwdev, u8 mod, u16 cmd,
				      void *buf_in, u16 in_size, void *buf_out,
				      u16 *out_size, u32 timeout);

/**
 * @brief hinic3_mbox_to_pf - vf mbox message to pf
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param buf_out: message buffer out
 * @param out_size: out buffer size
 * @param timeout: timeout
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_mbox_to_pf(void *hwdev, u8 mod, u16 cmd, void *buf_in,
		      u16 in_size, void *buf_out, u16 *out_size,
		      u32 timeout, u16 channel);

/**
 * @brief hinic3_mbox_to_vf - mbox message to vf
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf index
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param buf_out: message buffer out
 * @param out_size: out buffer size
 * @param timeout: timeout
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_mbox_to_vf(void *hwdev, u16 vf_id, u8 mod, u16 cmd, void *buf_in,
		      u16 in_size, void *buf_out, u16 *out_size, u32 timeout,
		      u16 channel);

/**
 * @brief hinic3_mbox_to_vf_no_ack - mbox message to vf no ack
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf index
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param buf_out: message buffer out
 * @param out_size: out buffer size
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_mbox_to_vf_no_ack(void *hwdev, u16 vf_id, u8 mod, u16 cmd, void *buf_in,
			     u16 in_size, void *buf_out, u16 *out_size, u16 channel);

int hinic3_clp_to_mgmt(void *hwdev, u8 mod, u16 cmd, const void *buf_in,
		       u16 in_size, void *buf_out, u16 *out_size);
/**
 * @brief hinic3_cmdq_async - cmdq asynchronous message
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_cmdq_async(void *hwdev, u8 mod, u8 cmd, struct hinic3_cmd_buf *buf_in, u16 channel);

/**
 * @brief hinic3_cmdq_async_cos - cmdq asynchronous message by cos
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param cos_id: cos id
 * @param buf_in: message buffer in
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_cmdq_async_cos(void *hwdev, u8 mod, u8 cmd, u8 cos_id,
			  struct hinic3_cmd_buf *buf_in, u16 channel);

/**
 * @brief hinic3_cmdq_detail_resp - cmdq direct message response
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param out_param: message out
 * @param timeout: timeout
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_cmdq_direct_resp(void *hwdev, u8 mod, u8 cmd,
			    struct hinic3_cmd_buf *buf_in,
			    u64 *out_param, u32 timeout, u16 channel);

/**
 * @brief hinic3_cmdq_detail_resp - cmdq detail message response
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param buf_out: message buffer out
 * @param out_param: inline output data
 * @param timeout: timeout
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_cmdq_detail_resp(void *hwdev, u8 mod, u8 cmd,
			    struct hinic3_cmd_buf *buf_in,
			    struct hinic3_cmd_buf *buf_out,
			    u64 *out_param, u32 timeout, u16 channel);

/**
 * @brief hinic3_cmdq_detail_resp - cmdq detail message response
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param cos_id: cos id
 * @param buf_in: message buffer in
 * @param buf_out: message buffer out
 * @param out_param: inline output data
 * @param timeout: timeout
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_cos_id_detail_resp(void *hwdev, u8 mod, u8 cmd, u8 cos_id,
			      struct hinic3_cmd_buf *buf_in,
			      struct hinic3_cmd_buf *buf_out,
			      u64 *out_param, u32 timeout, u16 channel);

/**
 * @brief hinic3_ppf_tmr_start - start ppf timer
 * @param hwdev: device pointer to hwdev
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_ppf_tmr_start(void *hwdev);

/**
 * @brief hinic3_ppf_tmr_stop - stop ppf timer
 * @param hwdev: device pointer to hwdev
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_ppf_tmr_stop(void *hwdev);

/**
 * @brief hinic3_func_tmr_bitmap_set - set timer bitmap status
 * @param hwdev: device pointer to hwdev
 * @param func_id: global function index
 * @param enable: 0-disable, 1-enable
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_func_tmr_bitmap_set(void *hwdev, u16 func_id, bool en);

/**
 * @brief hinic3_get_board_info - get board info
 * @param hwdev: device pointer to hwdev
 * @param info: board info
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_get_board_info(void *hwdev, struct hinic3_board_info *info,
			  u16 channel);

/**
 * @brief hinic3_set_wq_page_size - set work queue page size
 * @param hwdev: device pointer to hwdev
 * @param func_idx: function id
 * @param page_size: page size
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_wq_page_size(void *hwdev, u16 func_idx, u32 page_size,
			    u16 channel);

/**
 * @brief hinic3_event_callback - evnet callback to notify service driver
 * @param hwdev: device pointer to hwdev
 * @param event: event info to service driver
 */
void hinic3_event_callback(void *hwdev, struct hinic3_event_info *event);

/**
 * @brief hinic3_dbg_lt_rd_16byte - liner table read
 * @param hwdev: device pointer to hwdev
 * @param dest: destine id
 * @param instance: instance id
 * @param lt_index: liner table index id
 * @param data: data
 */
int hinic3_dbg_lt_rd_16byte(void *hwdev, u8 dest, u8 instance,
			    u32 lt_index, u8 *data);

/**
 * @brief hinic3_dbg_lt_wr_16byte_mask - liner table write
 * @param hwdev: device pointer to hwdev
 * @param dest: destine id
 * @param instance: instance id
 * @param lt_index: liner table index id
 * @param data: data
 * @param mask: mask
 */
int hinic3_dbg_lt_wr_16byte_mask(void *hwdev, u8 dest, u8 instance,
				 u32 lt_index, u8 *data, u16 mask);

/**
 * @brief hinic3_link_event_stats - link event stats
 * @param hwdev: device pointer to hwdev
 * @param link: link status
 */
void hinic3_link_event_stats(void *dev, u8 link);

/**
 * @brief hinic3_get_link_event_stats - link event stats
 * @param hwdev: device pointer to hwdev
 * @param link: link status
 */
int hinic3_get_link_event_stats(void *dev, int *link_state);

/**
 * @brief hinic3_get_hw_pf_infos - get pf infos
 * @param hwdev: device pointer to hwdev
 * @param infos: pf infos
 * @param channel: channel id
 */
int hinic3_get_hw_pf_infos(void *hwdev, struct hinic3_hw_pf_infos *infos,
			   u16 channel);

/**
 * @brief hinic3_func_reset - reset func
 * @param hwdev: device pointer to hwdev
 * @param func_id: global function index
 * @param reset_flag: reset flag
 * @param channel: channel id
 */
int hinic3_func_reset(void *dev, u16 func_id, u64 reset_flag, u16 channel);

int hinic3_get_ppf_timer_cfg(void *hwdev);

int hinic3_set_bdf_ctxt(void *hwdev, u8 bus, u8 device, u8 function);

int hinic3_init_func_mbox_msg_channel(void *hwdev, u16 num_func);

int hinic3_ppf_ht_gpa_init(void *dev);

void hinic3_ppf_ht_gpa_deinit(void *dev);

int hinic3_get_sml_table_info(void *hwdev, u32 tbl_id, u8 *node_id, u8 *instance_id);

int hinic3_mbox_ppf_to_host(void *hwdev, u8 mod, u16 cmd, u8 host_id,
			    void *buf_in, u16 in_size, void *buf_out,
			    u16 *out_size, u32 timeout, u16 channel);

void hinic3_force_complete_all(void *dev);
int hinic3_get_ceq_page_phy_addr(void *hwdev, u16 q_id,
				 u16 page_idx, u64 *page_phy_addr);
int hinic3_set_ceq_irq_disable(void *hwdev, u16 q_id);
int hinic3_get_ceq_info(void *hwdev, u16 q_id, struct hinic3_ceq_info *ceq_info);

int hinic3_init_single_ceq_status(void *hwdev, u16 q_id);
void hinic3_set_api_stop(void *hwdev);

int hinic3_activate_firmware(void *hwdev, u8 cfg_index);
int hinic3_switch_config(void *hwdev, u8 cfg_index);

bool hinic3_is_optical_module_mode(void *hwdev);
#endif
