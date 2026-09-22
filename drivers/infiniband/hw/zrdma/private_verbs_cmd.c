// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include <rdma/uverbs_std_types.h>
#define UVERBS_MODULE_NAME zxdh_ib
#include <rdma/uverbs_named_ioctl.h>
#include <linux/io.h>
#include "zxdh_user_ioctl_cmds.h"
#include "private_verbs_cmd.h"
#include "main.h"
#include "zxdh_user_ioctl_verbs.h"
#include "icrdma_hw.h"
#include "slib.h"
#include "zxdh_ext_mem_config.h"

#define DATA_ADDR_BASE 0x620610E0B4u
#define READ_RAM_REG_BASE 0x620610E0B8u
#define MP_DATA_NUM_GEG 0x620610E0BCu
#define BYPASS_REG 0x620740000Cu
#define REPLACE_REG 0x62074000A0u
#define BASE_FOR_LITTLE_GQP 0x6206008000u
#define BASE_FOR_BIG_GQP 0x6206108000u
#define RAM_ADDR 0xA1F40u
#define MP_OFFSET 0x200u
#define REG_BYTE 0x4u
#define CAP_ENABLE_REG_IDX 0x2Du
#define WRITE_RAM_REG_IDX 0x2Eu
#define GQP_MOD 0x14u
#define MP_MOD 0x37u
#define GQP_OFFSET 0x4u
#define GQP_ID_1023 0x3FF
#define GQP_ID_1103 0x44F
#define GQP_ID_2047 0x7FF
#define MP_IDX_INC 0x1u
#define MP_DATA_BYTE 0x40u
#define DDR_MP_DATA_NUM 0x30D3Fu
#define DDR_ADDR_BASE 0x3C0000000u
#define DDR_SIZE 0x3200000u
#define REPLACE_VALUE 0x20000000u
#define FREE_TYPE_MP 1
#define FREE_TYPE_TX 2
#define FREE_TYPE_RX 3
#define FREE_TYPE_IOVA 4
#define FREE_TYPE_HW_OBJ_DATA 5
#define MAX_COPY_SIZE 32
#define MAX_COPY_SIZE_EX 32
#define MAX_READ_REG_SIZE 32
#define MAX_SMMU_READ_REG_SIZE 16
#define BAR_MSG_RETRY_COUNT 3

static DEFINE_MUTEX(cc_basic_info_lock);
static DEFINE_MUTEX(active_gqp_reg_lock);
static DEFINE_MUTEX(cap_tx_reg_lock);
static DEFINE_MUTEX(cap_rx_reg_lock);
static DEFINE_MUTEX(mp_cap_reg_lock);

#define PBLE_QUEUE_CACHE_ID_BASE 0x6206800000
#define AH_CACHE_ID_BASE 0x6206800C08
#define TX_WINDOW_CACHE_ID_BASE 0x620680080C
#define TX_WINDOW_DDR_SIZE_REG 0x62065e0100
#define CQ_DOORBELL_SHADOW_BASE 0x6205800598
#define CQ_INDICATE_ID_BASE 0x6205800594
#define CEQ_INDICATE_ID_BASE 0x6205800680
#define AEQ_INDICATE_ID_BASE 0x620680081c
#define ROUTE_ID_REG_SIZE 0x1000
#define LAST_15_WQE 15


#define VHCA_RC_UD_8K_MAX_CNT 4089   // 1个vhca最大的 8k_idx 数量 = 4088+1
#define PCIE_PF_NUM_MAX 31
#define VHCA_NUM_MAX 257
#define CUSTOM_ERROR_CODE_BASE 200

#define ZXDH_QPN_ERROR (CUSTOM_ERROR_CODE_BASE + 1)
#define ZXDH_QP_NOT_AVALIABLE (CUSTOM_ERROR_CODE_BASE + 2)

#define ZXDH_READ_RAM_MAX_OFFSET 1024

int write_cap_tx_reg_node0(struct zxdh_sc_dev *dev,
			   struct zxdh_cap_cfg *cap_cfg);
int write_cap_tx_reg_node1(struct zxdh_sc_dev *dev,
			   struct zxdh_cap_cfg *cap_cfg);
int write_cap_rx_reg_node0(struct zxdh_sc_dev *dev,
			   struct zxdh_cap_cfg *cap_cfg);
int write_cap_rx_reg_node1(struct zxdh_sc_dev *dev,
			   struct zxdh_cap_cfg *cap_cfg);
static int write_mp_cap_regs(struct zxdh_device *iwdev, bool is_l2d_used,
			     struct zxdh_mp_cap_resp *resp, struct zxdh_ucontext *ucontext);
static int hw_object_query_info(struct zxdh_pci_f *rf, struct hw_object_wqe_context *object_wqe_ctx);
static void free_mmap_addr(struct zxdh_device *iwdev, u32 length, struct zxdh_cap_addr_info* cap_addr_info);
extern u16 get_rc_gqp_from_8k_index(u16 rc_8k_index, u16 rc_gqp_start,
			      u16 rc_gqp_cnt);

#define WRITE_REGISTER_AND_CHECK(rf, reg, value)              \
	do {                                                  \
		int ok = zxdh_rdma_reg_write(rf, reg, value); \
		if (ok != 0)                                  \
			return ok;                            \
	} while (0)

#define READ_REGISTER_AND_CHECK(rf, reg, value)              \
	do {                                                 \
		int ok = zxdh_rdma_reg_read(rf, reg, value); \
		if (ok != 0)                                 \
			return ok;                           \
	} while (0)

#define READ_REGISTERS_AND_CHECK(rf, reg, value, num)              \
	do {                                                 \
		int ok = zxdh_rdma_regs_read(rf, reg, value, num); \
		if (ok != 0)                                 \
			return ok;                           \
	} while (0)

#define REG_OP_AND_CHECK(opfunc, rf, reg, value) \
	do {                                     \
		int ok = opfunc(rf, reg, value); \
		if (ok != 0)                     \
			return ok;               \
	} while (0)

#define REG_WRITE_AND_CHECK_UNLOCK(rf, reg, value) \
	do {                                        \
		ret = zxdh_rdma_reg_write(rf, reg, value); \
		if (ret != 0)                        \
			goto unlock;                \
	} while (0)

#define FUNC_CALL_AND_CHECK_UNLOCK(func) \
	do {                             \
		ret = (func);            \
		if (ret != 0)            \
			goto unlock;    \
	} while (0)

#define CAP_REG_LOCK(cap_pos)                                       \
	do {                                                        \
		if ((cap_pos) == CAP_TX)                            \
			mutex_lock(&cap_tx_reg_lock);               \
		else                                               \
			mutex_lock(&cap_rx_reg_lock);               \
	} while (0)

#define CAP_REG_UNLOCK(cap_pos)                                     \
	do {                                                        \
		if ((cap_pos) == CAP_TX)                            \
			mutex_unlock(&cap_tx_reg_lock);             \
		else                                               \
			mutex_unlock(&cap_rx_reg_lock);             \
	} while (0)

#define GET_REG_AND_WRITE_TO_USE(addr, index) \
	do {									\
		reg_value[i].reg_addr = addr << index;	\
		READ_REGISTER_AND_CHECK(rf, addr, &reg_value[i].value);  \
		(*count) ++;   \
		i ++;	\
	} while (0)

#define COPY_TO_USER_SAFE(dest, src, size) \
    do { \
        if (copy_to_user((void *)(dest), (void *)(src), (size))) \
            return -EFAULT; \
    } while (0)


static int write_and_confirm_regs(struct zxdh_pci_f *rf, uint64_t phy_addr, uint32_t val)
{
	uint32_t read_val;
	int retry;
	int ret = 0;
	for (retry = 0; retry < BAR_MSG_RETRY_COUNT; retry++) {
		ret = zxdh_rdma_reg_write(rf, phy_addr, val);
		if (ret != 0) {
			continue;
		}
		ret = zxdh_rdma_reg_read(rf, phy_addr, &read_val);
		if (ret != 0) {
			continue;
		}
		if (read_val == val) {
			return 0;
		}
	}
	return ret;
}

static inline struct zxdh_rdma_tool_context *zxdh_get_rdma_tool_context(
					struct zxdh_ucontext *ucontext)
{
	if (!ucontext || !ucontext->rdma_tool_context)
		return NULL;

	return (struct zxdh_rdma_tool_context *)ucontext->rdma_tool_context;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_QP_RESET_QP)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_QP_RESET_QP)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{

	struct zxdh_qp *iwqp;
	int ret;
	u64 reset_opcode;
	struct ib_qp *qp =
		uverbs_attr_get_obj(attrs, ZXDH_IB_ATTR_QP_RESET_QP_HANDLE);

	iwqp = to_iwqp(qp);
	ret = uverbs_copy_from(&reset_opcode, attrs,
			       ZXDH_IB_ATTR_QP_RESET_OP_CODE);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	if (reset_opcode <= 0)
		return -EINVAL;

	switch (reset_opcode) {
	case ZXDH_RESET_RETRY_TX_ITEM_FLAG:
		ret = zxdh_reset_retry_tx_item(iwqp);
		break;
	default:
		pr_err("[zxdh_rdma] reset qp unknow opcode:%lld\n", reset_opcode);
		ret = EINVAL;
		break;
	}

	return ret;
}

static void copy_qpc_to_resp(void *va, struct zxdh_query_qpc_resp *resp)
{
	resp->retry_flag = ZXDH_GET_QPC_ITEM(
		u8, va, ZXDH_QPC_RETRY_FALG_BYTE_OFFSET, RDMAQPC_TX_RETRY_FLAG);
	resp->rnr_retry_flag =
		ZXDH_GET_QPC_ITEM(u8, va, ZXDH_QPC_RNR_RETRY_FALG_BYTE_OFFSET,
				  RDMAQPC_TX_RNR_RETRY_FLAG);
	resp->read_retry_flag =
		ZXDH_GET_QPC_ITEM(u8, va, ZXDH_QPC_READ_RETRY_FALG_BYTE_OFFSET,
				  RDMAQPC_TX_READ_RETRY_FLAG);
	resp->cur_retry_count =
		ZXDH_GET_QPC_ITEM(u8, va, ZXDH_QPC_CUR_RETRY_COUNT_BYTE_OFFSET,
				  RDMAQPC_TX_CUR_RETRY_CNT);
	resp->retry_cqe_sq_opcode = ZXDH_GET_QPC_ITEM(
		u8, va, ZXDH_QPC_RETRY_CQE_SQ_OPCODE_BYTE_OFFSET,
		RDMAQPC_TX_RETRY_CQE_SQ_OPCODE_FLAG);
	resp->err_flag = ZXDH_GET_QPC_ITEM(
		u8, va, ZXDH_QPC_ERR_FLAG_BYTE_OFFSET, RDMAQPC_TX_ERR_FLAG);
	resp->ack_err_flag =
		ZXDH_GET_QPC_ITEM(u8, va, ZXDH_QPC_ACK_ERR_FLAG_BYTE_OFFSET,
				  RDMAQPC_TX_ACK_ERR_FLAG);
	resp->package_err_flag =
		ZXDH_GET_QPC_ITEM(u8, va, ZXDH_QPC_PACKAGE_ERR_FLAG_BYTE_OFFSET,
				  RDMAQPC_TX_PACKAGE_ERR_FLAG);
	resp->recv_err_flag =
		ZXDH_GET_QPC_ITEM(u8, va, ZXDH_QPC_RECV_ERR_FLAG_BYTE_OFFSET,
				  RDMAQPC_TX_RECV_ERR_FLAG);
	resp->tx_last_ack_psn =
		ZXDH_GET_QPC_ITEM(u32, va, ZXDH_QPC_TX_LAST_ACK_PSN_BYTE_OFFSET,
				  RDMAQPC_TX_LAST_ACK_PSN);
	resp->retry_count = ZXDH_GET_QPC_ITEM(
		u8, va, ZXDH_QPC_RETY_COUNT_BYTE_OFFSET, RDMAQPC_TX_RETRY_CNT);
}

#ifndef RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
static int zxdh_copy_to_struct_or_zero(const struct uverbs_attr_bundle *bundle,
				       size_t idx, const void *from,
				       size_t size)
{
	const struct uverbs_attr *attr = uverbs_attr_get(bundle, idx);

	if (IS_ERR(attr))
		return PTR_ERR(attr);

	if (size < attr->ptr_attr.len) {
		if (clear_user(u64_to_user_ptr(attr->ptr_attr.data) + size,
			       attr->ptr_attr.len - size))
			return -EFAULT;
	}
	return uverbs_copy_to(bundle, idx, from, size);
}
#endif

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_QP_QUERY_QPC)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_QP_QUERY_QPC)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	struct zxdh_qp *iwqp;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct ib_ucontext *ucontext;
	struct zxdh_dma_mem qpc_buf;
	int err_code = 0;
	struct zxdh_query_qpc_resp resp = { 0 };
	struct ib_qp *qp =
		uverbs_attr_get_obj(attrs, ZXDH_IB_ATTR_QP_QUERY_HANDLE);
#ifdef ZXDH_UAPI_DEF
	ucontext = ib_uverbs_get_ucontext(attrs);
#else
	ucontext = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ucontext))
		return PTR_ERR(ucontext);
	ib_dev = ucontext->device;
	iwdev = to_iwdev(ib_dev);
	iwqp = to_iwqp(qp);

	qpc_buf.va = NULL;
	qpc_buf.size = ALIGN(ZXDH_QP_CTX_SIZE, ZXDH_QPC_ALIGNMENT);
	qpc_buf.va = zxdh_alloc_raw_mem(iwdev, qpc_buf.size,
					&qpc_buf.pa);
	if (!qpc_buf.va) {
		pr_err("[zxdh_rdma] query qpc alloc failed:ENOMEM\n");
		return -ENOMEM;
	}
	err_code = zxdh_fill_qpc(&iwdev->rf->sc_dev, iwqp->sc_qp.qp_ctx_num, &qpc_buf);
	if (err_code) {
		pr_err("[zxdh_rdma] query qpc fill qpc failed:%d\n", err_code);
		zxdh_free_raw_mem(iwdev, qpc_buf.va, qpc_buf.size, qpc_buf.pa);
		return -EFAULT;
	}
	copy_qpc_to_resp(qpc_buf.va, &resp);
	zxdh_free_raw_mem(iwdev, qpc_buf.va, qpc_buf.size, qpc_buf.pa);
#ifdef RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
	return uverbs_copy_to_struct_or_zero(attrs, ZXDH_IB_ATTR_QP_QUERY_RESP,
					     &resp, sizeof(resp));
#else
	return zxdh_copy_to_struct_or_zero(attrs, ZXDH_IB_ATTR_QP_QUERY_RESP,
					   &resp, sizeof(resp));
#endif
}

static void
transfer_modify_qpc_req_to_item(const struct zxdh_modify_qpc_req *req,
				struct zxdh_modify_qpc_item *modify_item)
{
	modify_item->retry_flag = req->retry_flag;
	modify_item->rnr_retry_flag = req->rnr_retry_flag;
	modify_item->read_retry_flag = req->read_retry_flag;
	modify_item->cur_retry_count = req->cur_retry_count;
	modify_item->retry_cqe_sq_opcode = req->retry_cqe_sq_opcode;
	modify_item->err_flag = req->err_flag;
	modify_item->ack_err_flag = req->ack_err_flag;
	modify_item->package_err_flag = req->package_err_flag;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_QP_MODIFY_QPC)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_QP_MODIFY_QPC)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	struct zxdh_qp *iwqp;
	struct ib_ucontext *ucontext;
	struct zxdh_modify_qpc_req req = { 0 };
	int ret;
	struct zxdh_modify_qpc_item modify_item = { 0 };
	u64 modify_mask;
	struct ib_qp *qp =
		uverbs_attr_get_obj(attrs, ZXDH_IB_ATTR_QP_MODIFY_QPC_HANDLE);
#ifdef ZXDH_UAPI_DEF
	ucontext = ib_uverbs_get_ucontext(attrs);
#else
	ucontext = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ucontext))
		return PTR_ERR(ucontext);
	iwqp = to_iwqp(qp);
	ret = uverbs_copy_from(&modify_mask, attrs,
			       ZXDH_IB_ATTR_QP_MODIFY_QPC_MASK);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;
	ret = uverbs_copy_from_or_zero(&req, attrs,
				       ZXDH_IB_ATTR_QP_MODIFY_QPC_REQ);
	if (ret)
		return ret;
	transfer_modify_qpc_req_to_item(&req, &modify_item);
	ret = process_hw_modify_qpc_cmd(iwqp, &modify_item, modify_mask);
	if (ret) {
		pr_err("[zxdh_rdma] modify qpc process modify qpc cmd failed:%d\n", ret);
		return ret;
	}
	return 0;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_QP_MODIFY_UDP_SPORT)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_QP_MODIFY_UDP_SPORT)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	struct zxdh_pd *iwpd;
	struct zxdh_device *iwdev;
	struct ib_ucontext *ucontext;
	struct ib_device *ib_dev;
	struct zxdh_qp *iwqp = NULL;
	struct zxdh_udp_offload_info *udp_info;
	struct zxdh_qp_host_ctx_info *ctx_info;
	struct zxdh_pci_f *rf;
	struct zxdh_sc_dev *dev;
	struct zxdh_modify_qp_info info = {};
	u64 qpc_tx_mask_low = 0;
	u64 qpc_rx_mask_low = 0;
	unsigned long flags;
	u16 udp_sport = 0;
	u32 qpn = 0;
	int ret;

#ifdef ZXDH_UAPI_DEF
	ucontext = ib_uverbs_get_ucontext(attrs);
#else
	ucontext = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ucontext))
		return PTR_ERR(ucontext);
	ib_dev = ucontext->device;
	iwdev = to_iwdev(ib_dev);
	ret = uverbs_copy_from(&udp_sport, attrs, ZXDH_IB_ATTR_QP_UDP_PORT);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;
	ret = uverbs_copy_from(&qpn, attrs, ZXDH_IB_ATTR_QP_QPN);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;
	rf = iwdev->rf;
	dev = &rf->sc_dev;
	if (qpn < (dev->base_qpn + 1) || qpn > (dev->base_qpn + rf->max_qp - 1))
		return -EINVAL;

	iwqp = iwdev->rf->qp_table[qpn - dev->base_qpn];
	if (iwqp == NULL)
		return -EINVAL;

	if (iwqp->ibqp.qp_type != IB_QPT_RC ||
	    !(iwqp->ibqp_state == IB_QPS_RTR || iwqp->ibqp_state == IB_QPS_RTS))
		return -EOPNOTSUPP;

	iwpd = to_iwpd(iwqp->ibqp.pd);
	udp_info = &iwqp->udp_info;
	ctx_info = &iwqp->ctx_info;

	spin_lock_irqsave(&iwqp->lock, flags);
	ctx_info->roce_info->pd_id = iwpd->sc_pd.pd_id;
	udp_info->src_port = udp_sport;
	qpc_tx_mask_low |= RDMAQPC_TX_MASKL_SRC_PORT;
	qpc_rx_mask_low |= RDMAQPC_RX_MASKL_SRC_PORT;
	info.qpc_tx_mask_low = qpc_tx_mask_low;
	info.qpc_rx_mask_low = qpc_rx_mask_low;
	zxdh_sc_qp_modify_ctx_udp_sport(&iwqp->sc_qp, iwqp->host_ctx.va,
					ctx_info);
	spin_unlock_irqrestore(&iwqp->lock, flags);

	if (zxdh_hw_modify_qp(iwdev, iwqp, &info, true))
		return -EINVAL;

	if (!refcount_read(&iwdev->trace_switch.t_switch))
		return 0;

	if (udp_info->ipv4) {
		struct sockaddr_in saddr_in4 = { 0 };
		struct sockaddr_in daddr_in4 = { 0 };

		saddr_in4.sin_addr.s_addr = htonl(udp_info->local_ipaddr[3]);
		daddr_in4.sin_addr.s_addr = htonl(udp_info->dest_ip_addr[3]);

		ibdev_notice(
			&iwdev->ibdev,
			"QP[%u]: modify QP udp sport, type %d, ib qpn 0x%X, state: %s, dest_qpn:%d, src_port:%d, src_ip:%pI4, dest_ip:%pI4\n",
			iwqp->ibqp.qp_num, iwqp->ibqp.qp_type,
			iwqp->ibqp.qp_num,
			zxdh_qp_state_to_string(iwqp->ibqp_state),
			iwqp->roce_info.dest_qp, udp_sport, &saddr_in4.sin_addr,
			&daddr_in4.sin_addr);
	} else {
		struct sockaddr_in6 saddr_in6 = { 0 };
		struct sockaddr_in6 daddr_in6 = { 0 };

		zxdh_copy_ip_htonl(saddr_in6.sin6_addr.in6_u.u6_addr32,
				   udp_info->local_ipaddr);
		zxdh_copy_ip_htonl(daddr_in6.sin6_addr.in6_u.u6_addr32,
				   udp_info->dest_ip_addr);

		ibdev_notice(
			&iwdev->ibdev,
			"QP[%u]: modify QP udp sport, type %d, ib qpn 0x%X, state: %s, dest_qpn:%d, src_port:%d, src_ip:%pI6, dest_ip:%pI6\n",
			iwqp->ibqp.qp_num, iwqp->ibqp.qp_type,
			iwqp->ibqp.qp_num,
			zxdh_qp_state_to_string(iwqp->ibqp_state),
			iwqp->roce_info.dest_qp, udp_sport,
			&saddr_in6.sin6_addr, &daddr_in6.sin6_addr);
	}
	return 0;
}

#if 0
static int zxdh_modify_qp_credit_flag(struct zxdh_qp *iwqp, u64 credit_flag)
{
	__le64 *qp_ctx = iwqp->host_ctx.va;
	struct zxdh_modify_qp_info info = {};
	u64 hdr;
	u64 mask;

	get_64bit_val(qp_ctx, 8, &hdr);
	mask = FIELD_PREP(RDMAQPC_TX_ACKCREDITS, 0x1f);
	hdr &= ~mask;
	/* 0x1e is on, 0x1f is off*/
	hdr |= FIELD_PREP(RDMAQPC_TX_ACKCREDITS, credit_flag ? 0x1e : 0x1f);
	set_64bit_val(qp_ctx, 8, hdr);

	get_64bit_val(qp_ctx, 376, &hdr);
	mask = FIELD_PREP(RDMAQPC_RX_ACK_CREDITS, 0x1);
	hdr &= ~mask;
	hdr |= FIELD_PREP(RDMAQPC_RX_ACK_CREDITS, credit_flag ? 0x0 : 0x1);

	set_64bit_val(qp_ctx, 376, hdr);

	info.qpc_tx_mask_low = (0x1UL << 7);
	info.qpc_tx_mask_high = 0;
	info.qpc_rx_mask_low = (1 << 7);
	info.qpc_rx_mask_high = 0;

	info.qpc_tx_mask_low |= 0x1FFFFFF;
	info.qpc_tx_mask_high |= 0x1UL << 18;
	info.qpc_rx_mask_low |= 0xDA3CE8081E7FFCF0;
	info.qpc_rx_mask_high |= 0x1E9;

	if (zxdh_hw_modify_qp(iwqp->iwdev, iwqp, &info, true))
		return -EINVAL;

	iwqp->sc_qp.is_credit_en = credit_flag ? 1 : 0;
	return 0;
}
#endif

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_QP_SET_CREDIT_FLAG)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_QP_SET_CREDIT_FLAG)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	return 0;
#if 0
	struct zxdh_qp *iwqp;
	struct ib_ucontext *ucontext;
	int ret;
	u64 credity_flag;
	struct ib_qp *qp =
		uverbs_attr_get_obj(attrs, ZXDH_IB_ATTR_QP_SET_CREDIT_FLAG_HANDLE);
#ifdef ZXDH_UAPI_DEF
	ucontext = ib_uverbs_get_ucontext(attrs);
#else
	ucontext = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ucontext))
		return PTR_ERR(ucontext);

	iwqp = to_iwqp(qp);
	ret = uverbs_copy_from(&credity_flag, attrs,
			       ZXDH_IB_ATTR_QP_CREDIT_FLAG);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	ret = zxdh_modify_qp_credit_flag(iwqp, credity_flag);
	return ret;
	#endif
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_GET_LOG_TRACE)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_GET_LOG_TRACE)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	struct zxdh_device *iwdev;
	struct ib_ucontext *ucontext;
	struct ib_device *ib_dev;
	u8 trace_switch;
	int ret;

#ifdef ZXDH_UAPI_DEF
	ucontext = ib_uverbs_get_ucontext(attrs);
#else
	ucontext = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ucontext))
		return PTR_ERR(ucontext);
	ib_dev = ucontext->device;
	iwdev = to_iwdev(ib_dev);
	trace_switch = refcount_read(&iwdev->trace_switch.t_switch);
	ret = uverbs_copy_to(attrs, ZXDH_IB_ATTR_DEV_GET_LOG_TARCE_SWITCH,
			     &trace_switch, sizeof(trace_switch));
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;
	return 0;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_SET_LOG_TRACE)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_SET_LOG_TRACE)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	struct ib_ucontext *ucontext;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	u8 trace_switch;
	int ret;

#ifdef ZXDH_UAPI_DEF
	ucontext = ib_uverbs_get_ucontext(attrs);
#else
	ucontext = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ucontext))
		return PTR_ERR(ucontext);
	ib_dev = ucontext->device;
	iwdev = to_iwdev(ib_dev);
	ret = uverbs_copy_from(&trace_switch, attrs,
			       ZXDH_IB_ATTR_DEV_SET_LOG_TARCE_SWITCH);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	if (trace_switch >= SWITCH_ERROR)
		return -EINVAL;

	spin_lock(&iwdev->trace_switch.lock);
	refcount_set(&iwdev->trace_switch.t_switch, trace_switch);
	spin_unlock(&iwdev->trace_switch.lock);

	return 0;
}

static int check_data_cap_running(struct zxdh_pci_f *rf,
				  struct zxdh_sc_dev *dev,
				  u32 cap_position)
{
	uint32_t reg_val = 0;
	int ret = 0;
	uint64_t reg_addr;

	if (cap_position == CAP_TX)
		reg_addr = RDMATX_DATA_START_CAP;
	else if (cap_position == CAP_RX)
		reg_addr = RDMARX_DATA_START_CAP;
	else {
		pr_err("[zxdh_rdma] check_data_cap_running vhca_id:%u, cap_position:%u error.\n", dev->vhca_id, cap_position);
		return -CAP_CFG_ERROR;
	}

	ret = zxdh_rdma_reg_read(rf, reg_addr, &reg_val);
	if (ret != 0) {
		pr_err("[zxdh_rdma] check_data_cap_running, zxdh_rdma_reg_read ret = %d\n", ret);
		return ret;
	}

	if (reg_val != 0) {
		pr_err("[zxdh_rdma] check_data_cap_running, vhca_id:%u,cap_position:%u already start.\n", dev->vhca_id, cap_position);
		return -EBUSY;
	}

	return 0;
}

int write_cap_tx_reg_node0(struct zxdh_sc_dev *dev,
			   struct zxdh_cap_cfg *cap_cfg)
{
	struct zxdh_pci_f *rf;
	u8 node_select, node_choose, comapre_loop;
	u64 wqe_offset[RDMA_TX_CAP_WQE_MOD_NUM] = {
		RDMATX_CAP_NODE0_WQE_PRE_READ(dev->ep_id), RDMATX_CAP_NODE0_WQE_HANDLE(dev->ep_id),
		RDMATX_CAP_NODE0_PACKAGE(dev->ep_id)
	};
	u64 node0offset[RDMA_TX_SEL_NODE_MODULE_NUM - 1] = {
		RDMATX_CAP_NODE0_ACK, RDMATX_CAP_NODE0_DB, RDMATX_CAP_NODE0_AEQ,
		0, RDMATX_CAP_NODE0_TXWINDOW
	};
	u64 compare_bit_en_offset[EN_32bit_GROUP_NUM] = {
		RDMATX_CAP_COMPARE_BIT_EN0_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN1_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN2_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN3_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN4_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN5_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN6_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN7_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN8_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN9_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN10_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN11_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN12_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN13_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN14_NODE0,
		RDMATX_CAP_COMPARE_BIT_EN15_NODE0
	};
	u64 compare_data_en_offset[EN_32bit_GROUP_NUM] = {
		RDMATX_CAP_COMPARE_DATA0_NODE0,
		RDMATX_CAP_COMPARE_DATA1_NODE0,
		RDMATX_CAP_COMPARE_DATA2_NODE0,
		RDMATX_CAP_COMPARE_DATA3_NODE0,
		RDMATX_CAP_COMPARE_DATA4_NODE0,
		RDMATX_CAP_COMPARE_DATA5_NODE0,
		RDMATX_CAP_COMPARE_DATA6_NODE0,
		RDMATX_CAP_COMPARE_DATA7_NODE0,
		RDMATX_CAP_COMPARE_DATA8_NODE0,
		RDMATX_CAP_COMPARE_DATA9_NODE0,
		RDMATX_CAP_COMPARE_DATA10_NODE0,
		RDMATX_CAP_COMPARE_DATA11_NODE0,
		RDMATX_CAP_COMPARE_DATA12_NODE0,
		RDMATX_CAP_COMPARE_DATA13_NODE0,
		RDMATX_CAP_COMPARE_DATA14_NODE0,
		RDMATX_CAP_COMPARE_DATA15_NODE0
	};

	u32 node0_mask, val, chl_sel_idx;

	chl_sel_idx = cap_cfg->channel_select[NODE0];
	if (chl_sel_idx >= RDMA_TX_SEL_NODE_MODULE_NUM)
		return -EINVAL;

	rf = container_of(dev, struct zxdh_pci_f, sc_dev);
	WRITE_REGISTER_AND_CHECK(rf, RDMATX_CAP_CHL_SEL_NODE0,
				 (cap_cfg->channel_select[NODE0] & 0xF));
	WRITE_REGISTER_AND_CHECK(rf, RDMATX_CAP_CHL_OPEN_NODE0,
				 (cap_cfg->channel_open[NODE0] & 0xF));

	node_choose = cap_cfg->node_choose[NODE0] & 0xFF;
	node0_mask = ~(0xff);
	if (chl_sel_idx == RDMA_TX_SEL_NODE_MODULE_WQE) {
		node_select = (cap_cfg->node_select[NODE0] & 0xFF);
		val = readl((u32 __iomem *)(dev->hw->hw_addr +
					    RDMATX_CAP_NODE0_SEL(dev->ep_id)));
		val = ((val & node0_mask) | node_select);
		writel(val, (u32 __iomem *)(dev->hw->hw_addr +
					    RDMATX_CAP_NODE0_SEL(dev->ep_id)));

		val = readl((u32 __iomem *)(dev->hw->hw_addr +
					    wqe_offset[node_select]));
		val = ((val & node0_mask) | node_choose);
		writel(val, (u32 __iomem *)(dev->hw->hw_addr +
					    wqe_offset[node_select]));
	} else {
		if (chl_sel_idx != RDMA_TX_SEL_NODE_MODULE_NONE) {
			writel(0, (u32 __iomem *)(dev->hw->hw_addr +
						  RDMATX_CAP_NODE0_SEL(dev->ep_id)));

			READ_REGISTER_AND_CHECK(rf, node0offset[chl_sel_idx],
						&val);
			val = ((val & node0_mask) | node_choose);
			WRITE_REGISTER_AND_CHECK(rf, node0offset[chl_sel_idx],
						 val);
		}
	}

	for (comapre_loop = 0; comapre_loop < EN_32bit_GROUP_NUM;
	     comapre_loop++) {
		WRITE_REGISTER_AND_CHECK(
			rf, compare_bit_en_offset[comapre_loop],
			cap_cfg->compare_bit_en[comapre_loop][NODE0]);
		WRITE_REGISTER_AND_CHECK(
			rf, compare_data_en_offset[comapre_loop],
			cap_cfg->compare_data[comapre_loop][NODE0]);
	}

	WRITE_REGISTER_AND_CHECK(rf, RDMATX_CAP_TIME_WRL2D_NODE0,
				 cap_cfg->rdma_time_wrl2d[NODE0]);
	return 0;
}

int write_cap_tx_reg_node1(struct zxdh_sc_dev *dev,
			   struct zxdh_cap_cfg *cap_cfg)
{
	struct zxdh_pci_f *rf;
	u8 node_select, node_choose, comapre_loop;
	u64 wqe_offset[RDMA_TX_CAP_WQE_MOD_NUM] = {
		RDMATX_CAP_NODE1_WQE_PRE_READ(dev->ep_id), RDMATX_CAP_NODE1_WQE_HANDLE(dev->ep_id),
		RDMATX_CAP_NODE1_PACKAGE(dev->ep_id)
	};
	u64 node1offset[RDMA_TX_SEL_NODE_MODULE_NUM - 1] = {
		RDMATX_CAP_NODE1_ACK, RDMATX_CAP_NODE1_DB, RDMATX_CAP_NODE1_AEQ,
		0, RDMATX_CAP_NODE1_TXWINDOW
	};
	u64 compare_bit_en_offset[EN_32bit_GROUP_NUM] = {
		RDMATX_CAP_COMPARE_BIT_EN0_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN1_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN2_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN3_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN4_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN5_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN6_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN7_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN8_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN9_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN10_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN11_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN12_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN13_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN14_NODE1,
		RDMATX_CAP_COMPARE_BIT_EN15_NODE1
	};
	u64 compare_data_en_offset[EN_32bit_GROUP_NUM] = {
		RDMATX_CAP_COMPARE_DATA0_NODE1,
		RDMATX_CAP_COMPARE_DATA1_NODE1,
		RDMATX_CAP_COMPARE_DATA2_NODE1,
		RDMATX_CAP_COMPARE_DATA3_NODE1,
		RDMATX_CAP_COMPARE_DATA4_NODE1,
		RDMATX_CAP_COMPARE_DATA5_NODE1,
		RDMATX_CAP_COMPARE_DATA6_NODE1,
		RDMATX_CAP_COMPARE_DATA7_NODE1,
		RDMATX_CAP_COMPARE_DATA8_NODE1,
		RDMATX_CAP_COMPARE_DATA9_NODE1,
		RDMATX_CAP_COMPARE_DATA10_NODE1,
		RDMATX_CAP_COMPARE_DATA11_NODE1,
		RDMATX_CAP_COMPARE_DATA12_NODE1,
		RDMATX_CAP_COMPARE_DATA13_NODE1,
		RDMATX_CAP_COMPARE_DATA14_NODE1,
		RDMATX_CAP_COMPARE_DATA15_NODE1
	};
	u32 node1_mask, val, chl_sel_idx;

	chl_sel_idx = cap_cfg->channel_select[NODE1];
	if (chl_sel_idx >= RDMA_TX_SEL_NODE_MODULE_NUM)
		return -EINVAL;

	rf = container_of(dev, struct zxdh_pci_f, sc_dev);
	WRITE_REGISTER_AND_CHECK(rf, RDMATX_CAP_CHL_SEL_NODE1,
				 (cap_cfg->channel_select[NODE1] & 0xF));
	WRITE_REGISTER_AND_CHECK(rf, RDMATX_CAP_CHL_OPEN_NODE1,
				 (cap_cfg->channel_open[NODE1] & 0xF));
	node_choose = cap_cfg->node_choose[NODE1] & 0xFF;
	node1_mask = ~(0xff);
	if (chl_sel_idx == RDMA_TX_SEL_NODE_MODULE_WQE) {
		node_select = (cap_cfg->node_select[NODE1] & 0xFF);

		val = readl((u32 __iomem *)(dev->hw->hw_addr +
					    RDMATX_CAP_NODE1_SEL(dev->ep_id)));
		val = ((val & node1_mask) | node_select);
		writel(val, (u32 __iomem *)(dev->hw->hw_addr +
					    RDMATX_CAP_NODE1_SEL(dev->ep_id)));

		val = readl((u32 __iomem *)(dev->hw->hw_addr +
					    wqe_offset[node_select]));
		val = ((val & node1_mask) | node_choose);
		writel(val, (u32 __iomem *)(dev->hw->hw_addr +
					    wqe_offset[node_select]));
	} else {
		if (chl_sel_idx != RDMA_TX_SEL_NODE_MODULE_NONE) {
			writel(0, (u32 __iomem *)(dev->hw->hw_addr +
						  RDMATX_CAP_NODE1_SEL(dev->ep_id)));

			READ_REGISTER_AND_CHECK(rf, node1offset[chl_sel_idx],
						&val);
			val = ((val & node1_mask) | node_choose);
			WRITE_REGISTER_AND_CHECK(rf, node1offset[chl_sel_idx],
						 val);
		}
	}

	for (comapre_loop = 0; comapre_loop < EN_32bit_GROUP_NUM;
	     comapre_loop++) {
		WRITE_REGISTER_AND_CHECK(
			rf, compare_bit_en_offset[comapre_loop],
			cap_cfg->compare_bit_en[comapre_loop][NODE1]);
		WRITE_REGISTER_AND_CHECK(
			rf, compare_data_en_offset[comapre_loop],
			cap_cfg->compare_data[comapre_loop][NODE1]);
	}

	WRITE_REGISTER_AND_CHECK(rf, RDMATX_CAP_TIME_WRL2D_NODE1,
				 cap_cfg->rdma_time_wrl2d[NODE1]);
	return 0;
}

int write_cap_rx_reg_node0(struct zxdh_sc_dev *dev,
			   struct zxdh_cap_cfg *cap_cfg)
{
	struct zxdh_pci_f *rf;
	u8 comapre_loop;
	u64 node0offset[RDMA_RX_SEL_NODE_MODULE_NUM] = {
		RDMARX_CAP_NODE0_SEL_RTT_T4,
		RDMARX_CAP_NODE0_SEL_PKT_PROC,
		RDMARX_CAP_NODE_SEL_HD_CACHE,
		RDMARX_CAP_NODE_SEL_VAPA_DDRWR,
		0,
		RDMARX_CAP_NODE0_SEL_PRIFIELD_CHECK,
		RDMARX_CAP_NODE0_SEL_READ_SRQC,
		RDMARX_CAP_NODE0_SEL_READ_WQE,
		RDMARX_CAP_NODE0_SEL_CNP_GEN,
		RDMARX_CAP_NODE_SEL_ACKNAKFIFO,
		RDMARX_CAP_NODE0_SEL_CQE,
		RDMARX_CAP_NODE0_SEL_COMPLQUEUE,
		RDMARX_CAP_NODE_SEL_NOF,
		RDMARX_CAP_NODE0_SEL_TXSUB
	};

	u64 compare_bit_en_offset[EN_32bit_GROUP_NUM] = {
		RDMARX_CAP_COMPARE_BIT_EN0_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN1_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN2_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN3_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN4_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN5_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN6_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN7_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN8_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN9_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN10_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN11_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN12_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN13_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN14_NODE0,
		RDMARX_CAP_COMPARE_BIT_EN15_NODE0
	};
	u64 compare_data_en_offset[EN_32bit_GROUP_NUM] = {
		RDMARX_CAP_COMPARE_DATA0_NODE0,
		RDMARX_CAP_COMPARE_DATA1_NODE0,
		RDMARX_CAP_COMPARE_DATA2_NODE0,
		RDMARX_CAP_COMPARE_DATA3_NODE0,
		RDMARX_CAP_COMPARE_DATA4_NODE0,
		RDMARX_CAP_COMPARE_DATA5_NODE0,
		RDMARX_CAP_COMPARE_DATA6_NODE0,
		RDMARX_CAP_COMPARE_DATA7_NODE0,
		RDMARX_CAP_COMPARE_DATA8_NODE0,
		RDMARX_CAP_COMPARE_DATA9_NODE0,
		RDMARX_CAP_COMPARE_DATA10_NODE0,
		RDMARX_CAP_COMPARE_DATA11_NODE0,
		RDMARX_CAP_COMPARE_DATA12_NODE0,
		RDMARX_CAP_COMPARE_DATA13_NODE0,
		RDMARX_CAP_COMPARE_DATA14_NODE0,
		RDMARX_CAP_COMPARE_DATA15_NODE0
	};
	u32 node0_mask = 0;
	u32 node0_value = 0;
	u32 val, chl_sel_idx;

	chl_sel_idx = cap_cfg->channel_select[NODE0];
	if (chl_sel_idx >= RDMA_RX_SEL_NODE_MODULE_NUM) {
		return CAP_WRITE_NODE0_REGS_ERROR;
	};

	rf = container_of(dev, struct zxdh_pci_f, sc_dev);
	WRITE_REGISTER_AND_CHECK(rf, RDMARX_CAP_CHL_SEL_NODE0,
				 (cap_cfg->channel_select[NODE0] & 0xF));
	WRITE_REGISTER_AND_CHECK(rf, RDMARX_CAP_CHL_OPEN_NODE0,
				 (cap_cfg->channel_open[NODE0] & 0xF));

	switch (chl_sel_idx) {
	case RDMA_RX_SEL_NODE_MODULE_RTT_T4:
	case RDMA_RX_SEL_NODE_MODULE_PKT_PROC:
	case RDMA_RX_SEL_NODE_MODULE_CEQ:
	case RDMA_RX_SEL_NODE_MODULE_COMPLQUEUE:
	case RDMA_RX_SEL_NODE_MODULE_TX_SUB:
		node0_mask = ~(0xffffffff);
		node0_value = cap_cfg->node_select[NODE0];
		break;
	case RDMA_RX_SEL_NODE_MODULE_HD_CACHE:
	case RDMA_RX_SEL_NODE_MODULE_VAPA_DDRWR:
		node0_mask = ~(0xf);
		node0_value = cap_cfg->node_select[NODE0];
		break;
	case RDMA_RX_SEL_NODE_MODULE_PRIFIELD_CHECK:
	case RDMA_RX_SEL_NODE_MODULE_READ_SRQC:
	case RDMA_RX_SEL_NODE_MODULE_READ_WQE:
		node0_mask = ~(0xff);
		node0_value = cap_cfg->node_select[NODE0];
		break;
	case RDMA_RX_SEL_NODE_MODULE_CNP_GEN:
		node0_mask = ~(0x7);
		node0_value = cap_cfg->node_select[NODE0];
		break;
	case RDMA_RX_SEL_NODE_MODULE_ACKNAKFIFO:
		node0_mask = ~(0xffff);
		node0_value = cap_cfg->node_select[NODE0];
		break;
	case RDMA_RX_SEL_NODE_MODULE_NOF:
		node0_mask = ~(0xffff << 16);
		node0_value = cap_cfg->node_select[NODE0] << 16;
		break;
	default:
		break;
	}
	if (chl_sel_idx != RDMA_RX_SEL_NODE_MODULE_PSN_CHECK) {
		READ_REGISTER_AND_CHECK(rf, node0offset[chl_sel_idx], &val);
		val = ((val & node0_mask) | node0_value);
		WRITE_REGISTER_AND_CHECK(rf, node0offset[chl_sel_idx], val);

		pr_info("[zxdh_rdma] val=%u, node0_value=%u, channel_select=%u, node0_select val= 0x%08llx\n",
			val, node0_value, chl_sel_idx,
			node0offset[chl_sel_idx]);
	}

	for (comapre_loop = 0; comapre_loop < EN_32bit_GROUP_NUM;
	     comapre_loop++) {
		WRITE_REGISTER_AND_CHECK(
			rf, compare_bit_en_offset[comapre_loop],
			cap_cfg->compare_bit_en[comapre_loop][NODE0]);
		WRITE_REGISTER_AND_CHECK(
			rf, compare_data_en_offset[comapre_loop],
			cap_cfg->compare_data[comapre_loop][NODE0]);
	}

	WRITE_REGISTER_AND_CHECK(rf, RDMARX_CAP_TIME_WRL2D_NODE0,
				 cap_cfg->rdma_time_wrl2d[NODE0]);
	return 0;
}

int write_cap_rx_reg_node1(struct zxdh_sc_dev *dev,
			   struct zxdh_cap_cfg *cap_cfg)
{
	struct zxdh_pci_f *rf;
	u8 comapre_loop;
	u64 node1offset[RDMA_RX_SEL_NODE_MODULE_NUM] = {
		RDMARX_CAP_NODE1_SEL_RTT_T4,
		RDMARX_CAP_NODE1_SEL_PKT_PROC,
		RDMARX_CAP_NODE_SEL_HD_CACHE,
		RDMARX_CAP_NODE_SEL_VAPA_DDRWR,
		0,
		RDMARX_CAP_NODE1_SEL_PRIFIELD_CHECK,
		RDMARX_CAP_NODE1_SEL_READ_SRQC,
		RDMARX_CAP_NODE1_SEL_READ_WQE,
		RDMARX_CAP_NODE1_SEL_CNP_GEN,
		RDMARX_CAP_NODE_SEL_ACKNAKFIFO,
		RDMARX_CAP_NODE1_SEL_CQE,
		RDMARX_CAP_NODE1_SEL_COMPLQUEUE,
		RDMARX_CAP_NODE_SEL_NOF,
		RDMARX_CAP_NODE1_SEL_TXSUB
	};

	u64 compare_bit_en_offset[EN_32bit_GROUP_NUM] = {
		RDMARX_CAP_COMPARE_BIT_EN0_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN1_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN2_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN3_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN4_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN5_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN6_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN7_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN8_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN9_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN10_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN11_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN12_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN13_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN14_NODE1,
		RDMARX_CAP_COMPARE_BIT_EN15_NODE1
	};
	u64 compare_data_en_offset[EN_32bit_GROUP_NUM] = {
		RDMARX_CAP_COMPARE_DATA0_NODE1,
		RDMARX_CAP_COMPARE_DATA1_NODE1,
		RDMARX_CAP_COMPARE_DATA2_NODE1,
		RDMARX_CAP_COMPARE_DATA3_NODE1,
		RDMARX_CAP_COMPARE_DATA4_NODE1,
		RDMARX_CAP_COMPARE_DATA5_NODE1,
		RDMARX_CAP_COMPARE_DATA6_NODE1,
		RDMARX_CAP_COMPARE_DATA7_NODE1,
		RDMARX_CAP_COMPARE_DATA8_NODE1,
		RDMARX_CAP_COMPARE_DATA9_NODE1,
		RDMARX_CAP_COMPARE_DATA10_NODE1,
		RDMARX_CAP_COMPARE_DATA11_NODE1,
		RDMARX_CAP_COMPARE_DATA12_NODE1,
		RDMARX_CAP_COMPARE_DATA13_NODE1,
		RDMARX_CAP_COMPARE_DATA14_NODE1,
		RDMARX_CAP_COMPARE_DATA15_NODE1
	};
	u32 node1_mask = 0;
	u32 node1_value = 0;
	u32 val, chl_sel_idx;

	chl_sel_idx = cap_cfg->channel_select[NODE1];
	if (chl_sel_idx >= RDMA_RX_SEL_NODE_MODULE_NUM)
		return CAP_WRITE_NODE1_REGS_ERROR;

	rf = container_of(dev, struct zxdh_pci_f, sc_dev);
	WRITE_REGISTER_AND_CHECK(rf, RDMARX_CAP_CHL_SEL_NODE1,
				 (cap_cfg->channel_select[NODE1] & 0xF));
	WRITE_REGISTER_AND_CHECK(rf, RDMARX_CAP_CHL_OPEN_NODE1,
				 (cap_cfg->channel_open[NODE1] & 0xF));

	switch (chl_sel_idx) {
	case RDMA_RX_SEL_NODE_MODULE_RTT_T4:
	case RDMA_RX_SEL_NODE_MODULE_PKT_PROC:
	case RDMA_RX_SEL_NODE_MODULE_CEQ:
	case RDMA_RX_SEL_NODE_MODULE_COMPLQUEUE:
	case RDMA_RX_SEL_NODE_MODULE_TX_SUB:
		node1_mask = ~(0xffffffff);
		node1_value = cap_cfg->node_select[NODE1];
		break;
	case RDMA_RX_SEL_NODE_MODULE_HD_CACHE:
	case RDMA_RX_SEL_NODE_MODULE_VAPA_DDRWR:
		node1_mask = ~(0xf << 16);
		node1_value = cap_cfg->node_select[NODE1] << 16;
		break;
	case RDMA_RX_SEL_NODE_MODULE_PRIFIELD_CHECK:
	case RDMA_RX_SEL_NODE_MODULE_READ_SRQC:
	case RDMA_RX_SEL_NODE_MODULE_READ_WQE:
	case RDMA_RX_SEL_NODE_MODULE_NOF:
		node1_mask = ~(0xff);
		node1_value = cap_cfg->node_select[NODE1];
		break;
	case RDMA_RX_SEL_NODE_MODULE_CNP_GEN:
		node1_mask = ~(0x7);
		node1_value = cap_cfg->node_select[NODE1];
		break;
	case RDMA_RX_SEL_NODE_MODULE_ACKNAKFIFO:
		node1_mask = ~(0xffff << 16);
		node1_value = cap_cfg->node_select[NODE1] << 16;
		break;
	default:
		break;
	}
	if (chl_sel_idx != RDMA_RX_SEL_NODE_MODULE_PSN_CHECK) {
		READ_REGISTER_AND_CHECK(rf, node1offset[chl_sel_idx], &val);
		val = (val & node1_mask) | node1_value;
		WRITE_REGISTER_AND_CHECK(rf, node1offset[chl_sel_idx], val);

		pr_info("[zxdh_rdma] val=%u, node1_value=%u, channel_select_node1=%u, node1_select val= 0x%08llx\n",
			val, node1_value, chl_sel_idx,
			node1offset[chl_sel_idx]);
	}

	for (comapre_loop = 0; comapre_loop < EN_32bit_GROUP_NUM;
	     comapre_loop++) {
		WRITE_REGISTER_AND_CHECK(
			rf, compare_bit_en_offset[comapre_loop],
			cap_cfg->compare_bit_en[comapre_loop][NODE1]);
		WRITE_REGISTER_AND_CHECK(
			rf, compare_data_en_offset[comapre_loop],
			cap_cfg->compare_data[comapre_loop][NODE1]);
	}

	WRITE_REGISTER_AND_CHECK(rf, RDMARX_CAP_TIME_WRL2D_NODE1,
				 cap_cfg->rdma_time_wrl2d[NODE1]);
	return 0;
}

static int disable_data_cap(struct zxdh_ucontext *ucontext, u32 cap_position)
{
	int ret = 0;
	struct zxdh_device *iwdev;
	struct zxdh_pci_f *rf;
	struct zxdh_sc_dev *dev;
	struct zxdh_rdma_tool_context *rdma_tool_context = NULL;
	

	iwdev = ucontext->iwdev;
	rf = iwdev->rf;
	dev = &rf->sc_dev;

	rdma_tool_context = zxdh_get_rdma_tool_context(ucontext);

	if (cap_position == CAP_TX) {
		dev->tx_stop_on_aeq = 0;
		ret = write_and_confirm_regs(rf, RDMATX_DATA_START_CAP, 0);
		if (ret) {
			pr_err("[zxdh_rdma] data cap cap_position %u disable err!\n", cap_position);
			return ret;
		}
	} else if (cap_position == CAP_RX) {
		dev->rx_stop_on_aeq = 0;
		ret = write_and_confirm_regs(rf, RDMARX_DATA_START_CAP, 0);
		if (ret) {
			pr_err("[zxdh_rdma] data cap cap_position %u disable err!\n", cap_position);
			return ret;
		}
	} else {
		pr_err("[zxdh_rdma] data cap cap_position %u disable err!\n", cap_position);
		return -EINVAL;
	}

	if (rdma_tool_context) {
		rdma_tool_context->data_cap_on = false;
	}

	return 0;
}

static bool check_cap_cfg(struct zxdh_cap_cfg *cap_cfg)
{
	if (cap_cfg->cap_data_start_cap == 0x0) {
		pr_err("[zxdh_rdma] zxdh cap_data_start_cap cfg err!\n");
		return false;
	}

	if (cap_cfg->cap_position != CAP_TX && cap_cfg->cap_position != CAP_RX) {
		pr_err("[zxdh_rdma] zxdh cap_data_start_cap cfg cap_position %d err !\n", cap_cfg->cap_position);
		return false;
	}

	if (cap_cfg->cap_data_start_cap == 0x1) {
		if (cap_cfg->cap_position == CAP_TX &&
		    (cap_cfg->channel_select[NODE0] ==
			     RDMA_TX_SEL_NODE_MODULE_NONE ||
		     cap_cfg->channel_select[NODE0] >
			     RDMA_TX_SEL_NODE_MODULE_WQE)) {
			pr_err("[zxdh_rdma] zxdh cap_data_start_cap cfg tx node0 channel_select:%u err!\n",
			       cap_cfg->channel_select[NODE0]);
			return false;
		}

		if (cap_cfg->cap_position == CAP_RX &&
		    cap_cfg->channel_select[NODE0] >=
			    RDMA_RX_SEL_NODE_MODULE_NUM) {
			pr_err("[zxdh_rdma] zxdh cap_data_start_cap cfg rx node0 channel_select:%u err!\n",
			       cap_cfg->channel_select[NODE0]);
			return false;
		}
	}

	if (cap_cfg->cap_data_start_cap == 0x2) {
		if (cap_cfg->cap_position == CAP_TX &&
		    (cap_cfg->channel_select[NODE1] ==
			     RDMA_TX_SEL_NODE_MODULE_NONE ||
		     cap_cfg->channel_select[NODE1] >
			     RDMA_TX_SEL_NODE_MODULE_WQE)) {
			pr_err("[zxdh_rdma] zxdh cap_data_start_cap cfg tx node1 channel_select:%u err!\n",
			       cap_cfg->channel_select[NODE1]);
			return false;
		}

		if (cap_cfg->cap_position == CAP_RX &&
		    cap_cfg->channel_select[NODE1] >=
			    RDMA_RX_SEL_NODE_MODULE_NUM) {
			pr_err("[zxdh_rdma] zxdh cap_data_start_cap cfg rx node1 channel_select:%u err!\n",
			       cap_cfg->channel_select[NODE1]);
			return false;
		}
	}
	return true;
}

static void clean_data_cap_buff(struct zxdh_sc_dev *dev, u8 cap_position)
{
	u64 half_entries = 0 ;
	u64 start = 0;
   	u64 i;

	half_entries =
		(ZXDH_CAP_DATA_HMC_MEM_SIZE / ZXDH_HMC_DIRECT_BP_SIZE) / 2;

	start = (cap_position == CAP_RX) ? half_entries : 0;

	for (i = 0; i < half_entries; i++) {
		memset(dev->data_cap_sd.entry[start + i].u.bp.addr.va,
		       0, ZXDH_HMC_DIRECT_BP_SIZE);
	}
}

static int allocate_addr_for_data_cap(struct zxdh_device *iwdev, struct zxdh_ucontext *ucontext, struct zxdh_cap_addr_info *cap_addr_info, __u64 *cap_pa){
	cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr = 
		    zxdh_zalloc_mapped(iwdev, &cap_addr_info->addr_info.cap_direct_dma_addr.cap_dma_addr,
							 ZXDH_CAP_DATA_HOST_MEM_SIZE, DMA_BIDIRECTIONAL);
	if (cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr == NULL) {
		pr_err("[zxdh_rdma] allocate_addr_for_data_cap fail!\n");
		return -ENOMEM;
	}

#ifdef RDMA_MMAP_DB_SUPPORT
	cap_addr_info->entry_info.cap_mmap_entry =
		zxdh_cap_mmap_entry_insert(ucontext,
					cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr,
					ZXDH_CAP_DATA_HOST_MEM_SIZE,
					ZXDH_MMAP_PFN,
					cap_pa);
#else
	cap_addr_info->entry_info.cap_mmap_entry =
		zxdh_user_mmap_entry_add_hash(
			ucontext,
			virt_to_phys(cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr),
			ZXDH_MMAP_PFN, cap_pa);

#endif
	if (!cap_addr_info->entry_info.cap_mmap_entry) {
		pr_err("[zxdh_rdma] cap_mmap_entry insert err!\n");
		return -ENOMEM;
	}
	return 0;
}

static int prepare_addr_for_data_cap(struct ib_ucontext *ib_uctx, struct zxdh_device *iwdev, 
						 struct zxdh_cap_cfg *cap_cfg,struct zxdh_cap_start_resp *cap_resp)
{
	struct zxdh_ucontext *ucontext;
	struct zxdh_sc_dev *dev;
	u64 mmap_len;
	int ret = 0;
	int i;
	u64 *cap_iova_addr[CAP_NODE_NUM];
	__u64 cap_pa[CAP_NODE_NUM] = {0};
	u8 cap_position = cap_cfg->cap_position;
	u64 iova_addr_offset = 0;

	ucontext = to_ucontext(ib_uctx);
	dev = &iwdev->rf->sc_dev;
	
	if (cap_cfg->size > ZXDH_CAP_DATA_HOST_MEM_SIZE) {

		if (cap_position == CAP_RX) {
			iova_addr_offset = ZXDH_CAP_DATA_HMC_MEM_SIZE / 2;
		}
		mmap_len = cap_cfg->size;
		cap_iova_addr[NODE0] = &ucontext->hw_data_cap.cap_txrx_use_iova[NODE0].addr_info.cap_iova_addr;
		cap_iova_addr[NODE1] = &ucontext->hw_data_cap.cap_txrx_use_iova[NODE1].addr_info.cap_iova_addr;
		if ((cap_cfg->cap_data_start_cap & 0x3) == 0x3) {
			if (mmap_len > (ZXDH_CAP_DATA_HMC_MEM_SIZE / 4)) {
				pr_err("[zxdh_rdma] prepare_addr_for_data_cap mmap_len:%llu too big!\n", mmap_len);
				return -EINVAL;
			}
			*cap_iova_addr[NODE0] = dev->data_cap_sd.data_cap_base + iova_addr_offset;
			*cap_iova_addr[NODE1] = dev->data_cap_sd.data_cap_base + iova_addr_offset + mmap_len;
			clean_data_cap_buff(dev, cap_position);
		} else if ((cap_cfg->cap_data_start_cap & 0x1) == 0x1) {
			if (mmap_len > ZXDH_CAP_DATA_HMC_MEM_SIZE / 2) {
				pr_err("[zxdh_rdma] prepare_addr_for_data_cap node0 mmap_len:%llu too big!\n", mmap_len);
				return -EINVAL;
			}
			*cap_iova_addr[NODE0] = dev->data_cap_sd.data_cap_base + iova_addr_offset;
			clean_data_cap_buff(dev, cap_position);
		} else if ((cap_cfg->cap_data_start_cap & 0x2) == 0x2) {
			if (mmap_len > ZXDH_CAP_DATA_HMC_MEM_SIZE / 2) {
				pr_err("[zxdh_rdma] prepare_addr_for_data_cap node1 mmap_len:%llu too big!\n", mmap_len);
				return -EINVAL;
			}
			*cap_iova_addr[NODE1] = dev->data_cap_sd.data_cap_base + iova_addr_offset;
			clean_data_cap_buff(dev, cap_position);
		}


		for (i=0; i<CAP_NODE_NUM; i++){
			if (*cap_iova_addr[i] != 0) {
		#ifdef RDMA_MMAP_DB_SUPPORT
				ucontext->hw_data_cap.cap_txrx_use_iova[i].entry_info.cap_mmap_entry =
					zxdh_cap_mmap_entry_insert(
						ucontext, (void *)(*cap_iova_addr[i]),
						mmap_len, ZXDH_MMAP_HMC,
						&cap_pa[i]);
		#else
				ucontext->hw_data_cap.cap_txrx_use_iova[i].entry_info.cap_mmap_entry =
					zxdh_user_mmap_entry_add_hash(ucontext,
									*cap_iova_addr[i],
									ZXDH_MMAP_HMC,
									&cap_pa[i]);

		#endif
				if (i == 0) {
					cap_resp->cap_pa_node0 = cap_pa[NODE0];
				}else {
					cap_resp->cap_pa_node1 = cap_pa[NODE1];
				}
				
				if (!ucontext->hw_data_cap.cap_txrx_use_iova[i].entry_info.cap_mmap_entry) {
					pr_err("[zxdh_rdma] cap_mmap_entry_node0 insert err!\n");
					return -ENOMEM;
				}
			}
		}

	} else {
		if (cap_cfg->size != ZXDH_CAP_DATA_HOST_MEM_SIZE) {
			cap_cfg->size = ZXDH_CAP_DATA_HOST_MEM_SIZE;
		}

		for (i = 0; i < CAP_NODE_NUM; i++) {
			if (cap_cfg->cap_data_start_cap & (1 << i)) {
				if (cap_position == CAP_TX) {
					if (allocate_addr_for_data_cap(iwdev, ucontext, &ucontext->hw_data_cap.cap_tx_use_direct_dma[i], &cap_pa[i]) != 0) {
						pr_err("[zxdh_rdma] zxdh_zalloc_mapped for tx node%u fail!\n", i);
						return -ENOMEM;
					}
				}else if (cap_position == CAP_RX) {
					if (allocate_addr_for_data_cap(iwdev, ucontext, &ucontext->hw_data_cap.cap_rx_use_direct_dma[i], &cap_pa[i]) != 0) {
						pr_err("[zxdh_rdma] zxdh_zalloc_mapped for rx node%u fail!\n", i);
						return -ENOMEM;
					}
				}else{
					pr_err("[zxdh_rdma] zxdh_zalloc_mapped for cap_position:%u err!\n", cap_position);
					return -EINVAL;
				}
			}
			if (i == 0) {
				cap_resp->cap_pa_node0 = cap_pa[NODE0];
			}else {
				cap_resp->cap_pa_node1 = cap_pa[NODE1];
			} 
		}
	}

	return ret;
}

void free_cap_addr(struct zxdh_device *iwdev, struct zxdh_cap_addr_info *cap_addr_info)
{
	if (cap_addr_info->entry_info.cap_mmap_entry != NULL) {
#ifdef RDMA_MMAP_DB_SUPPORT
		rdma_user_mmap_entry_remove(
			cap_addr_info->entry_info.cap_mmap_entry);
#else
		zxdh_user_mmap_entry_del_hash(
			cap_addr_info->entry_info.cap_mmap_entry);
#endif
		cap_addr_info->entry_info.cap_mmap_entry = NULL;
	}

	if (cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr != NULL) {
		zxdh_free_mapped(iwdev, cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr,
					cap_addr_info->addr_info.cap_direct_dma_addr.cap_dma_addr,
					ZXDH_CAP_DATA_HOST_MEM_SIZE,
					DMA_BIDIRECTIONAL);
		cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr = NULL;
	}
}

static void cap_free_for_data_cap(u8 free_type, struct zxdh_device *iwdev, struct zxdh_ucontext *ucontext) 
{
	struct zxdh_cap_addr_info* cap_addr_info;
	int i;
	for (i = 0; i < CAP_NODE_NUM; i++) {
		if (free_type == FREE_TYPE_TX) {
			free_cap_addr(iwdev, &ucontext->hw_data_cap.cap_tx_use_direct_dma[i]);
		}
		
		if (free_type == FREE_TYPE_RX) {
			free_cap_addr(iwdev, &ucontext->hw_data_cap.cap_rx_use_direct_dma[i]);
		}
		
		pr_info("[zxdh_rdma] free_cap_addr for free_type:%d (0:None,1:MP,2:TX,3:RX,4:IOVA) node %d!\n", free_type, i);
		if (free_type == FREE_TYPE_IOVA){
			cap_addr_info = &ucontext->hw_data_cap.cap_txrx_use_iova[i];
			if (cap_addr_info->entry_info.cap_mmap_entry != NULL) {
#ifdef RDMA_MMAP_DB_SUPPORT
				rdma_user_mmap_entry_remove(
					cap_addr_info->entry_info.cap_mmap_entry);
#else
				zxdh_user_mmap_entry_del_hash(
					cap_addr_info->entry_info.cap_mmap_entry);
#endif
				cap_addr_info->entry_info.cap_mmap_entry = NULL;
			}
		}
	}
}

static void free_hw_object_data(struct zxdh_ucontext *ucontext)
{
	struct zxdh_device *iwdev;
	u32 length;
	iwdev = ucontext->iwdev;
	length = ucontext->hw_data_cap.object_buffer_size;
	free_mmap_addr(iwdev, length, &ucontext->hw_data_cap.hw_object_mmap);
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_CAP_START)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_CAP_START)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct zxdh_sc_dev *dev;
	struct ib_device *ib_dev;
	struct zxdh_ucontext *ucontext;
	struct zxdh_cap_cfg cap_cfg = { 0 };
	struct zxdh_cap_start_resp cap_resp = { 0 };
	struct zxdh_rdma_tool_context *rdma_tool_context = NULL;
	int ret = 0;
	u32 dma_addr_low, dma_addr_high, cap_id;
	bool is_host_dyn_mem_used = true;
	dma_addr_t *dma_addr[CAP_NODE_NUM] = {NULL};
	u64 *cap_iova_addr[CAP_NODE_NUM] = {NULL};
	u8 free_type = 0;

	#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);

	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);
	if (iwdev->rf->use_ext_mem_flag) {
		pr_err("[zxdh_rdma] tx_rx_data_cap not supported on current device!\n");
		return -ZXDH_NOT_SUPPORT_CURRENT_DEVICE;
	}
	dev = &iwdev->rf->sc_dev;
	ucontext = to_ucontext(ib_uctx);
	ret = uverbs_copy_from(&cap_cfg, attrs, ZXDH_IB_ATTR_DEV_CAP_START);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	if (!check_cap_cfg(&cap_cfg)) {
		return -EINVAL;
	}

	CAP_REG_LOCK(cap_cfg.cap_position);
	ret = check_data_cap_running(iwdev->rf, dev, cap_cfg.cap_position);
	if (ret != 0)
		goto unlock;
	pr_info("enable_iova_cap %d, cap_cfg.size %llu\n", enable_iova_cap, cap_cfg.size);
	if (!enable_iova_cap) {
		if (cap_cfg.size > ZXDH_CAP_DATA_HOST_MEM_SIZE) {
			pr_err("[zxdh_rdma] CAP size %llu exceeds maximum supported size %d, IOVA capture not supported\n",
				       cap_cfg.size, ZXDH_CAP_DATA_HOST_MEM_SIZE);
			ret = -ZXDH_IOVA_CAP_NOT_SUPPORT;
			goto unlock;
		}
	}
	if (cap_cfg.size > ZXDH_CAP_DATA_HOST_MEM_SIZE) {
		is_host_dyn_mem_used = false;
		free_type = FREE_TYPE_IOVA;
	} else {
		is_host_dyn_mem_used = true;
		if (cap_cfg.cap_position == CAP_TX) {
			free_type = FREE_TYPE_TX;
			dma_addr[NODE0] = &ucontext->hw_data_cap.cap_tx_use_direct_dma[NODE0].addr_info.cap_direct_dma_addr.cap_dma_addr;
			dma_addr[NODE1] = &ucontext->hw_data_cap.cap_tx_use_direct_dma[NODE1].addr_info.cap_direct_dma_addr.cap_dma_addr;
		} else {
			free_type = FREE_TYPE_RX;
			dma_addr[NODE0] = &ucontext->hw_data_cap.cap_rx_use_direct_dma[NODE0].addr_info.cap_direct_dma_addr.cap_dma_addr;
			dma_addr[NODE1] = &ucontext->hw_data_cap.cap_rx_use_direct_dma[NODE1].addr_info.cap_direct_dma_addr.cap_dma_addr;
		}
	}

	cap_iova_addr[NODE0] = &ucontext->hw_data_cap.cap_txrx_use_iova[NODE0].addr_info.cap_iova_addr;
	cap_iova_addr[NODE1] = &ucontext->hw_data_cap.cap_txrx_use_iova[NODE1].addr_info.cap_iova_addr;

	ret = prepare_addr_for_data_cap(ib_uctx, iwdev, &cap_cfg, &cap_resp);
	if (ret != 0) {
		ret = -CAP_ALLOC_ADDR_ERROR;
		goto unlock;
	}
		
	if ((cap_cfg.cap_data_start_cap & 0x1) == 0x1) {
		if (is_host_dyn_mem_used) {
			dma_addr_low = (u32)(*dma_addr[NODE0] & 0xFFFFFFFF);
			dma_addr_high =
				(u32)((*dma_addr[NODE0] >> 32) & 0xFFFFFFFF);
			// access host, smmu not used
			cap_id = (ZXDH_INDICATE_HOST_NOSMMU << 5 |ZXDH_CPU_DDR);
		} else {
			dma_addr_low = (u32)(*cap_iova_addr[NODE0] & 0xFFFFFFFF);
			dma_addr_high =
				(u32)((*cap_iova_addr[NODE0] >> 32) & 0xFFFFFFFF);
			// access host, smmu iova used
			cap_id = (ZXDH_INDICATE_HOST_SMMU << 5 |ZXDH_CPU_DDR);
		}

		pr_info("[zxdh_rdma] is_host_dyn_mem_used:%d,dma_addr_low:0x%X,dma_addr_high:0x%X.\n",
			is_host_dyn_mem_used, dma_addr_low, dma_addr_high);
		pr_info("[zxdh_rdma] vhca_id:%u,cap_id:%X.\n", dev->vhca_id, cap_id);
		
		if (cap_cfg.cap_position == CAP_TX) {
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_AXI_WR_ADDR_LOW_NODE0,
						  dma_addr_low);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_AXI_WR_ADDR_HIGH_NODE0,
						  dma_addr_high);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_AXI_WR_LEN_LOW_NODE0,
						  cap_cfg.size);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_AXI_WR_LEN_HIGH_NODE0, 0);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_VHCA_NUM_NODE0,
						  dev->vhca_id);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_AXI_ID_NODE0, 0);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_CAP_ID_NODE0, cap_id);
			FUNC_CALL_AND_CHECK_UNLOCK(write_cap_tx_reg_node0(dev, &cap_cfg));
		}

		if (cap_cfg.cap_position == CAP_RX) {
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_CAP_AXI_WR_ADDR_LOW_NODE0,
						  dma_addr_low);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_CAP_AXI_WR_ADDR_HIGH_NODE0,
						  dma_addr_high);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_CAP_AXI_WR_LEN_LOW_NODE0,
						  cap_cfg.size);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_CAP_VHCA_NUM_NODE0,
						  dev->vhca_id);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_CAP_AXI_ID_NODE0, 0);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_CAP_CAP_ID_NODE0, cap_id);
			FUNC_CALL_AND_CHECK_UNLOCK(write_cap_rx_reg_node0(dev, &cap_cfg));
		}
	}

	if ((cap_cfg.cap_data_start_cap & 0x2) == 0x2) {		
		if (is_host_dyn_mem_used) {
			dma_addr_low = (u32)(*dma_addr[NODE1] & 0xFFFFFFFF);
			dma_addr_high =
				(u32)((*dma_addr[NODE1] >> 32) & 0xFFFFFFFF);
			// access host, smmu not used
			cap_id = (ZXDH_INDICATE_HOST_NOSMMU << 5 |ZXDH_CPU_DDR);
		} else {
			dma_addr_low = (u32)(*cap_iova_addr[NODE1] & 0xFFFFFFFF);
			dma_addr_high =
				(u32)((*cap_iova_addr[NODE1] >> 32) & 0xFFFFFFFF);
			// access host, smmu iova used
			cap_id = (ZXDH_INDICATE_HOST_SMMU << 5 |ZXDH_CPU_DDR);
		}
		pr_info("[zxdh_rdma] node1 is_host_dyn_mem_used:%d, dma_addr_low:0x%X,dma_addr_high:0x%X.\n",
			     is_host_dyn_mem_used, dma_addr_low, dma_addr_high);
		pr_info("[zxdh_rdma] vhca_id:%u,cap_id:%X.\n", dev->vhca_id, cap_id);
		

		if (cap_cfg.cap_position == CAP_TX) {
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_AXI_WR_ADDR_LOW_NODE1,
						  dma_addr_low);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_AXI_WR_ADDR_HIGH_NODE1,
						  dma_addr_high);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_AXI_WR_LEN_LOW_NODE1,
						  cap_cfg.size);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_AXI_WR_LEN_HIGH_NODE1, 0);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_VHCA_NUM_NODE1,
						  dev->vhca_id);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_AXI_ID_NODE1, 0);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_CAP_CAP_ID_NODE1, cap_id);
			FUNC_CALL_AND_CHECK_UNLOCK(write_cap_tx_reg_node1(dev, &cap_cfg));
		}

		if (cap_cfg.cap_position == CAP_RX) {
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_CAP_AXI_WR_ADDR_LOW_NODE1,
						  dma_addr_low);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_CAP_AXI_WR_ADDR_HIGH_NODE1,
						  dma_addr_high);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_CAP_AXI_WR_LEN_LOW_NODE1,
						  cap_cfg.size);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_CAP_AXI_WR_LEN_HIGH_NODE1, 0);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_CAP_VHCA_NUM_NODE1,
						  dev->vhca_id);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_CAP_AXI_ID_NODE1, 0);
			REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_CAP_CAP_ID_NODE1, cap_id);
			FUNC_CALL_AND_CHECK_UNLOCK(write_cap_rx_reg_node1(dev, &cap_cfg));
		}

	}


	if (cap_cfg.cap_position == CAP_TX)
		REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMATX_DATA_START_CAP,
					  cap_cfg.cap_data_start_cap);
	else
		REG_WRITE_AND_CHECK_UNLOCK(iwdev->rf, RDMARX_DATA_START_CAP,
					  cap_cfg.cap_data_start_cap);

	pr_info("[zxdh_rdma] cap_start msg:cap_position=%u,cap_pa_node0:%llu,cap_pa_node1:%llu\n",
		cap_cfg.cap_position,cap_resp.cap_pa_node0,cap_resp.cap_pa_node1);

	rdma_tool_context = zxdh_get_rdma_tool_context(ucontext);
	if (rdma_tool_context) {
		rdma_tool_context->data_cap_on = true;
		rdma_tool_context->data_cap_position = cap_cfg.cap_position;
		rdma_tool_context->data_cap_mem_type = is_host_dyn_mem_used ?
			(cap_cfg.cap_position == CAP_TX ? FREE_TYPE_TX : FREE_TYPE_RX) : FREE_TYPE_IOVA;
		pr_debug("[zxdh_rdma] zxdh_init_rdma_tool_context: data_cap_on=%d, data_cap_position=%u, data_cap_mem_type=%u\n",
	       rdma_tool_context->data_cap_on,
	       rdma_tool_context->data_cap_position,
	       rdma_tool_context->data_cap_mem_type);
	}

#ifdef RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
	ret = uverbs_copy_to_struct_or_zero(attrs,
					    ZXDH_IB_ATTR_DEV_CAP_START_RESP,
					    &cap_resp, sizeof(cap_resp));
#else
	ret = zxdh_copy_to_struct_or_zero(attrs,
					  ZXDH_IB_ATTR_DEV_CAP_START_RESP,
					  &cap_resp, sizeof(cap_resp));
#endif

unlock:

	if (ret != 0) {
		cap_free_for_data_cap(free_type, iwdev, ucontext);
		if (rdma_tool_context && rdma_tool_context->data_cap_on) {
			if (disable_data_cap(ucontext, cap_cfg.cap_position)) {
				pr_err("[zxdh_rdma] cap_start disable data cap failed!\n");
			}
		}
		pr_err("[zxdh_rdma] data cap_start fail for %d!\n", ret);
	}
	CAP_REG_UNLOCK(cap_cfg.cap_position);
	return ret;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_CAP_STOP)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_CAP_STOP)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	struct ib_ucontext *ib_uctx;
	u8 cap_position;
	int ret;
#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);

	ret = uverbs_copy_from(&cap_position, attrs, ZXDH_IB_ATTR_DEV_CAP_STOP);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	CAP_REG_LOCK(cap_position);
	ret = disable_data_cap(to_ucontext(ib_uctx), cap_position);
	CAP_REG_UNLOCK(cap_position);
	if (ret != 0) {
		pr_err("[zxdh_rdma] disable_data_cap failed, ret=%d\n", ret);
		return ret;
	}

	pr_info("[zxdh_rdma] cap %u stop!1:tx,2:rx\n", cap_position);
	return 0;
}

static void free_mmap_addr(struct zxdh_device *iwdev, u32 length, struct zxdh_cap_addr_info* cap_addr_info)
{
	if (cap_addr_info->entry_info.cap_mmap_entry != NULL) {
		pr_info("[zxdh_rdma] free_mmap_addr rdma_user_mmap_entry_remove!\n");
#ifdef RDMA_MMAP_DB_SUPPORT
		rdma_user_mmap_entry_remove(
			cap_addr_info->entry_info.cap_mmap_entry);
#else
		zxdh_user_mmap_entry_del_hash(
			cap_addr_info->entry_info.cap_mmap_entry);
#endif
        cap_addr_info->entry_info.cap_mmap_entry = NULL;
    }
    /* ext_mem 模式：只释放 GPU 显存，不释放 DMA 地址 */
    if (iwdev->rf->use_ext_mem_flag) {
        if (cap_addr_info->cap_ext_mem_addr.cap_cpu_addr != NULL) {
            void __iomem *ext_mem_va;
            struct zxdh_ext_mem_pool *pool = iwdev->rf->ext_mem_pool;
            u64 host_pa = (u64)cap_addr_info->cap_ext_mem_addr.cap_cpu_addr;
            
            ext_mem_va = pool->base_va + (host_pa - pool->base_host_pa);
            
            zxdh_free_ext_mem_to_pool(iwdev->rf, ext_mem_va, length);
            cap_addr_info->cap_ext_mem_addr.cap_cpu_addr = NULL;
            
            pr_info("[zxdh_rdma] free_mmap_addr ext_mem: host_pa=0x%llx, ext_mem_va=0x%llx, length=%u\n",
                host_pa, (u64)ext_mem_va, length);
        }
    } else {
        /* 标准 DMA 模式：释放 dma coherent 内存 */
        if (cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr != NULL) {
            dma_free_coherent(iwdev->rf->sc_dev.hw->device,
                      length,
                      cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr,
                      cap_addr_info->addr_info.cap_direct_dma_addr.cap_dma_addr);
            cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr = NULL;
			pr_info("[zxdh_rdma] free_mmap_addr dma_free: length=%u\n", length);
        }
    }
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_CAP_FREE)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_CAP_FREE)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct zxdh_ucontext *ucontext;
	u8 free_type;
	int ret = 0;
	u32 length;

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);

	ucontext = to_ucontext(ib_uctx);
	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);
	ret = uverbs_copy_from(&free_type, attrs, ZXDH_IB_ATTR_DEV_CAP_FREE);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	switch (free_type)
	{
	case FREE_TYPE_MP:
		length = ALIGN(ZXDH_L2D_MPCAP_BUFF_SIZE,ZXDH_HW_PAGE_SIZE);
		free_mmap_addr(iwdev, length, &ucontext->hw_data_cap.mp_cap);
		break;
	case FREE_TYPE_HW_OBJ_DATA:
		free_hw_object_data(ucontext);
		break;
	case FREE_TYPE_IOVA:
	case FREE_TYPE_TX:
	case FREE_TYPE_RX:
		cap_free_for_data_cap(free_type, iwdev, ucontext);
		break;
	default:
		pr_err("[zxdh_rdma] ZXDH_IB_METHOD_DEV_CAP_FREE free_type:%u err!\n", free_type);
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int write_mp_cap_regs(struct zxdh_device *iwdev, bool is_l2d_used,
			     struct zxdh_mp_cap_resp *resp, struct zxdh_ucontext *ucontext)
{
	int i;
	uint8_t mp_idx, gqp_idx;
	uint64_t reg_addr, cap_size, addr_val;
	uint32_t reg_val;
	uint32_t read_reg_val = 0;

	if (is_l2d_used) {
		ucontext->hw_data_cap.mp_cap_media_addr_base =
			iwdev->rf->sc_dev.l2d_smmu_addr +
			ZXDH_SMMU_OFFSET + ZXDH_MP_BASERTT_OFFSET +
			ZXDH_SMMU_CMDQ_OFFSET;
		cap_size = ZXDH_L2D_MPCAP_BUFF_SIZE / resp->cap_gqp_num;
	} else {
		ucontext->hw_data_cap.mp_cap_media_addr_base = DDR_ADDR_BASE;
		cap_size = DDR_SIZE;
	}

	for (i = 0; i < resp->cap_gqp_num && i < MAX_CAP_QPS; i++) {
		reg_addr = READ_RAM_REG_BASE + i * MP_OFFSET;
		reg_val = RAM_ADDR + i * MP_OFFSET;
		/* e0b8:read ram address bak*/
		REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, reg_addr, &read_reg_val);
		if (reg_val == read_reg_val) {
			pr_err("[zxdh_rdma] reg_addr:0x%llx, mp cap for cap_gqpid:%u is working!\n", reg_addr, resp->cap_gqpid[i]);
			return -EBUSY;
		}
		mp_idx = 0;
		gqp_idx = 0;
		if (resp->cap_gqpid[i] <= GQP_ID_1023) {
			mp_idx = resp->cap_gqpid[i] / GQP_MOD;
			gqp_idx = resp->cap_gqpid[i] % GQP_MOD;

			reg_addr = (mp_idx * MP_OFFSET) + BASE_FOR_LITTLE_GQP +
				   (REG_BYTE * CAP_ENABLE_REG_IDX);
			reg_val = (uint32_t)(RAM_ADDR + (i * MP_OFFSET));
			/* 80b4:a1f40*/
			REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 reg_addr, reg_val);

			reg_addr = (mp_idx * MP_OFFSET) + BASE_FOR_LITTLE_GQP +
				   (REG_BYTE * WRITE_RAM_REG_IDX);
			reg_val = gqp_idx;
			/* 80b8:gqp_idx*/
			REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 reg_addr, reg_val);
		} else if (resp->cap_gqpid[i] > GQP_ID_1023 &&
			   resp->cap_gqpid[i] <= GQP_ID_1103) {
			mp_idx = ((resp->cap_gqpid[i] - GQP_OFFSET) / GQP_MOD) +
				 MP_IDX_INC;
			gqp_idx = (resp->cap_gqpid[i] - GQP_OFFSET) % GQP_MOD;

			reg_addr = (mp_idx * MP_OFFSET) + BASE_FOR_LITTLE_GQP +
				   (4 * CAP_ENABLE_REG_IDX);
			reg_val = RAM_ADDR + i * MP_OFFSET;
			REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 reg_addr, reg_val);

			reg_addr = (mp_idx * MP_OFFSET) + BASE_FOR_LITTLE_GQP +
				   (REG_BYTE * WRITE_RAM_REG_IDX);
			reg_val = gqp_idx;
			REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 reg_addr, reg_val);

		} else if (resp->cap_gqpid[i] <= GQP_ID_2047) {
			mp_idx = ((resp->cap_gqpid[i] - GQP_OFFSET) / GQP_MOD) -
				 MP_MOD;
			gqp_idx = (resp->cap_gqpid[i] - GQP_OFFSET) % GQP_MOD;

			reg_addr = (mp_idx * MP_OFFSET) + BASE_FOR_BIG_GQP +
				   (REG_BYTE * CAP_ENABLE_REG_IDX);
			reg_val = RAM_ADDR + i * MP_OFFSET;
			REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 reg_addr, reg_val);

			reg_addr = (mp_idx * MP_OFFSET) + BASE_FOR_BIG_GQP +
				   (REG_BYTE * WRITE_RAM_REG_IDX);
			reg_val = gqp_idx;
			REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 reg_addr, reg_val);
		} else {
			pr_err("[zxdh_rdma] gqpid:%u err!\n", resp->cap_gqpid[i]);
			return -EINVAL;
		}

		reg_addr = MP_DATA_NUM_GEG + i * MP_OFFSET;
		if (is_l2d_used) {
			reg_val = ((cap_size / MP_DATA_BYTE) - 1);
		} else {
			reg_val = DDR_MP_DATA_NUM; /* 20w MP data */
		}
		/* data num */
		REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf, reg_addr, reg_val);

		reg_addr = DATA_ADDR_BASE + i * MP_OFFSET;
		addr_val = ucontext->hw_data_cap.mp_cap_media_addr_base + (i * cap_size);
		if ((addr_val >> 32) != 0) {
			reg_val = (REPLACE_VALUE | (addr_val & 0x0FFFFFFF));
		} else {
			reg_val = addr_val;
		}
		/* e0b4:ram data address */
		REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf, reg_addr, reg_val);

		reg_addr = READ_RAM_REG_BASE + i * MP_OFFSET;
		reg_val = RAM_ADDR + i * MP_OFFSET;
		/* e0b8:read ram address bak*/
		REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf, reg_addr, reg_val);
	}

	return 0;
}


static void free_mp_cap_resource(struct zxdh_ucontext *ucontext)
{
	struct zxdh_device *iwdev;
	u32 length;
	iwdev = ucontext->iwdev;
	length = ALIGN(ZXDH_L2D_MPCAP_BUFF_SIZE, ZXDH_HW_PAGE_SIZE);
	free_mmap_addr(iwdev, length, &ucontext->hw_data_cap.mp_cap);
}

static int reset_mp_cap_reg(struct zxdh_mp_cap_gqp *cap_gqp, struct zxdh_ucontext *ucontext)
{
	
	int i;
	uint64_t reg_addr;
	struct zxdh_device *iwdev;
	struct zxdh_rdma_tool_context *rdma_tool_context = NULL;
	int ret = 0;
	
	uint8_t mp_idx = 0;
	uint8_t ram_write_addr_val = 0xff;

	iwdev = ucontext->iwdev;

	for (i = 0; i < cap_gqp->gqp_num; i++) {
		mp_idx = 0;
		if (cap_gqp->gqpid[i] <= GQP_ID_1023) {
			mp_idx = cap_gqp->gqpid[i] / GQP_MOD;
			reg_addr = (mp_idx * MP_OFFSET) + BASE_FOR_LITTLE_GQP +
				   (REG_BYTE * WRITE_RAM_REG_IDX);
			WRITE_REGISTER_AND_CHECK(iwdev->rf, reg_addr, ram_write_addr_val);
		} else if (cap_gqp->gqpid[i] > GQP_ID_1023 &&
			   cap_gqp->gqpid[i] <= GQP_ID_1103) {
			mp_idx = ((cap_gqp->gqpid[i] - GQP_OFFSET) / GQP_MOD) + MP_IDX_INC;
			reg_addr = (mp_idx * MP_OFFSET) + BASE_FOR_LITTLE_GQP +
				   (REG_BYTE * WRITE_RAM_REG_IDX);
			WRITE_REGISTER_AND_CHECK(iwdev->rf, reg_addr, ram_write_addr_val);

		} else if (cap_gqp->gqpid[i] <= GQP_ID_2047) {
			mp_idx = ((cap_gqp->gqpid[i] - GQP_OFFSET) / GQP_MOD) - MP_MOD;
			reg_addr = (mp_idx * MP_OFFSET) + BASE_FOR_BIG_GQP +
				   (REG_BYTE * WRITE_RAM_REG_IDX);
			WRITE_REGISTER_AND_CHECK(iwdev->rf, reg_addr, ram_write_addr_val);
		} else {
			pr_err("[zxdh_rdma] reset_mp_cap_reg gqpid:%u err!\n", cap_gqp->gqpid[i]);
			continue;
		}
		reg_addr = READ_RAM_REG_BASE + i * MP_OFFSET;
		ret = write_and_confirm_regs(iwdev->rf, reg_addr, 0);
		if (ret) {
			pr_err("[zxdh_rdma] reset_mp_cap_reg reg_addr 0x%llx failed\n", reg_addr);
			return ret;
		}
	}

	rdma_tool_context = zxdh_get_rdma_tool_context(ucontext);
	if (rdma_tool_context)
		rdma_tool_context->mp_cap_on = false;

	return ret;
}

static int zxdh_reset_mp_cap_gqp_regs(const struct zxdh_cap_gqp *cap_gqp,
					    struct zxdh_ucontext *ucontext)
{
	int ret = 0;
	struct zxdh_mp_cap_gqp *mp_cap_gqp = NULL;

	if (cap_gqp->gqp_num == 0) {
		return ret;
	}

	if (cap_gqp->gqp_num > MAX_CAP_QPS) {
		return -EINVAL;
	}

	mp_cap_gqp = kzalloc(sizeof(*mp_cap_gqp), GFP_KERNEL);
	if (!mp_cap_gqp) {
		pr_err("[zxdh_rdma] zxdh_reset_mp_cap_gqp_regs: failed to allocate mp_cap_gqp\n");
		return -ENOMEM;
	}

	mp_cap_gqp->gqp_num = cap_gqp->gqp_num;
	memcpy(mp_cap_gqp->gqpid, cap_gqp->gqpid,
	       cap_gqp->gqp_num * sizeof(uint16_t));
	ret = reset_mp_cap_reg(mp_cap_gqp, ucontext);
	if (ret) {
		pr_err("[zxdh_rdma] zxdh_reset_mp_cap_gqp_regs: reset failed, ret=%d\n", ret);
	}

	kfree(mp_cap_gqp);
	return ret;
}

static void zxdh_release_mp_resource(struct zxdh_ucontext *ucontext, struct zxdh_rdma_tool_context *rdma_tool_context)
{
	int ret = 0;
	free_mp_cap_resource(ucontext);
	if (!rdma_tool_context)
		return;
	ret = zxdh_reset_mp_cap_gqp_regs(&rdma_tool_context->mp_cap_gqps, ucontext); 
	if (ret) {
		pr_err("zxdh_reset_mp_cap_gqp_regs failed, ret=%d\n", ret);
	}

}

static void zxdh_release_data_cap_resource(struct zxdh_ucontext *ucontext, struct zxdh_rdma_tool_context *rdma_tool_context)
{
	struct zxdh_device *iwdev;
	int ret = 0;
	u32 cap_position = 0;

	if (!rdma_tool_context)
		return;
	iwdev = ucontext->iwdev;
	cap_free_for_data_cap(rdma_tool_context->data_cap_mem_type, iwdev, ucontext);
	cap_position = rdma_tool_context->data_cap_position;
	if (cap_position != 0) {
		CAP_REG_LOCK(cap_position);
		ret = disable_data_cap(ucontext, cap_position);
		CAP_REG_UNLOCK(cap_position);
		if (ret != 0) {
			pr_err("[zxdh_rdma] zxdh_release_data_cap_resource: disable_data_cap failed, ret=%d\n", ret);
		}
	} else {
		pr_err("[zxdh_rdma] zxdh_release_data_cap_resource failed, please use dhtool reset data cap reg to zeror, tx cap addr: 0x%lx, rx cap addr: 0x%lx to zero!\n",
		       RDMATX_DATA_START_CAP, RDMARX_DATA_START_CAP
		);
	}
}

static int allocate_addr_for_mmap(struct zxdh_device *iwdev, struct zxdh_ucontext *ucontext,
				  uint32_t length, struct zxdh_cap_addr_info *cap_addr_info, __u64 *cap_pa)
{
	if (iwdev->rf->use_ext_mem_flag) {
		/* 分配 ext_mem（GPU 显存） */
		void __iomem *ext_mem_va;
		u64 host_pa;
		
		ext_mem_va = zxdh_get_ext_mem_from_pool(iwdev->rf, length,
					&cap_addr_info->cap_ext_mem_addr.cap_dma_addr);
		if (ext_mem_va == NULL) {
			pr_err("[zxdh_rdma] allocate_addr_for_mmap ext_mem: failed to get ext_mem from pool, length=%u\n",
			       length);
			return -1;
		}
		
		/* 将虚拟地址转换为 host 端物理地址 */
		host_pa = zxdh_get_ext_mem_host_pa(iwdev->rf, ext_mem_va);
		if (!host_pa) {
			pr_err("[zxdh_rdma] allocate_addr_for_mmap ext_mem: failed to get host_pa\n");
			zxdh_free_ext_mem_to_pool(iwdev->rf, ext_mem_va, length);
			return -1;
		}
		
		/* cap_cpu_addr 存储 host 端物理地址 */
		cap_addr_info->cap_ext_mem_addr.cap_cpu_addr = (void *)host_pa;
		
#ifdef RDMA_MMAP_DB_SUPPORT
		cap_addr_info->entry_info.cap_mmap_entry =
			zxdh_mp_mmap_entry_insert(ucontext,
					(u64)cap_addr_info->cap_ext_mem_addr.cap_cpu_addr,
					length,
					ZXDH_MMAP_PFN,
					cap_pa);
#else
		cap_addr_info->entry_info.cap_mmap_entry =
			zxdh_user_mmap_entry_add_hash(
				ucontext,
				(u64)cap_addr_info->cap_ext_mem_addr.cap_cpu_addr,
				ZXDH_MMAP_PFN, cap_pa);
#endif
		if (!cap_addr_info->entry_info.cap_mmap_entry) {
			pr_err("[zxdh_rdma] allocate_addr_for_mmap ext_mem cap_mmap_entry insert err!\n");
			zxdh_free_ext_mem_to_pool(iwdev->rf,
					ext_mem_va, length);
			cap_addr_info->cap_ext_mem_addr.cap_cpu_addr = NULL;
			return -ENOMEM;
		}
	} else {
		/* 标准 DMA 模式：分配 dma coherent 内存，用于 mmap 到用户态 */
		cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr =
			dma_alloc_coherent(iwdev->rf->sc_dev.hw->device, length,
					&cap_addr_info->addr_info.cap_direct_dma_addr.cap_dma_addr,
					GFP_KERNEL);
		if (cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr == NULL) {
			pr_err("[zxdh_rdma] allocate_addr_for_mmap ddr: dma alloc failed\n");
			return -1;
		}
#ifdef RDMA_MMAP_DB_SUPPORT
	cap_addr_info->entry_info.cap_mmap_entry =
		zxdh_cap_mmap_entry_insert(ucontext,
						cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr,
						length,
						ZXDH_MMAP_PFN,
						cap_pa);
#else
	cap_addr_info->entry_info.cap_mmap_entry =
		zxdh_user_mmap_entry_add_hash(
			ucontext, virt_to_phys(cap_addr_info->addr_info.cap_direct_dma_addr.cap_cpu_addr),
			ZXDH_MMAP_PFN, cap_pa);
#endif
	if (!cap_addr_info->entry_info.cap_mmap_entry) {
		pr_err("[zxdh_rdma] allocate_addr_for_mmap cap_mmap_entry insert err!\n");
		return -ENOMEM;
	}
    }
	return 0;
    
}

static void add_unique_gqp_to_resp(struct zxdh_mp_cap_resp *resp, uint16_t gqp_id,
				   const char *log_context)
{
	int j;
	bool same_gqp_exist = false;

	if (resp == NULL) {
		pr_err("[zxdh_rdma] add_unique_gqp_to_resp: resp is NULL, gqp_id:%u %s\n",
		       gqp_id, log_context ? log_context : "");
		return;
	}

	for (j = 0; j < resp->cap_gqp_num; j++) {
		if (resp->cap_gqpid[j] == gqp_id) {
			same_gqp_exist = true;
			pr_info("[zxdh_rdma] same gqp_id:%u %s\n", gqp_id, log_context);
			break;
		}
	}
	if (same_gqp_exist)
		return;

	if (resp->cap_gqp_num >= MAX_CAP_QPS) {
		pr_err("[zxdh_rdma] cap_gqp_num:%u already reached MAX_CAP_QPS:%d, cannot add gqp_id:%u %s\n",
		       resp->cap_gqp_num, MAX_CAP_QPS, gqp_id, log_context);
		return;
	}

	resp->cap_gqpid[resp->cap_gqp_num] = gqp_id;
	resp->cap_gqp_num += 1;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_MP_CAP)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_MP_CAP)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	int ret, i;
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct zxdh_mp_cap_cfg mp_cap_cfg = { 0 };
	struct zxdh_qp *iwqp;
	struct zxdh_ucontext *ucontext;
	uint16_t gqp_id;
	uint64_t mp_reg_addrs[MAX_CAP_QPS];
	struct zxdh_mp_cap_resp mp_cap_resp;
	uint32_t length;
	struct zxdh_rdma_tool_context *rdma_tool_context = NULL;

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);

	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);
	ucontext = to_ucontext(ib_uctx);

	ret = uverbs_copy_from(&mp_cap_cfg, attrs,
			       ZXDH_IB_ATTR_DEV_MP_CAP);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	pr_info("[zxdh_rdma] qpn_num:%u\n", mp_cap_cfg.qpn_num);
	if (mp_cap_cfg.qpn_num == 0 || mp_cap_cfg.qpn_num > MAX_CAP_QPS)
		return -EINVAL;
	mutex_lock(&mp_cap_reg_lock);
	memset(&mp_cap_resp, 0, sizeof(struct zxdh_mp_cap_resp));
	if (mp_cap_cfg.cap_use_l2d) {
		ucontext->hw_data_cap.mp_cap.addr_info.cap_direct_dma_addr.cap_cpu_addr = NULL;
		length = ZXDH_L2D_MPCAP_BUFF_SIZE;
		if (allocate_addr_for_mmap(iwdev, ucontext, length,
					   &ucontext->hw_data_cap.mp_cap,
					   &mp_cap_resp.cap_pa)) {
			pr_err("[zxdh_rdma] allocate_addr_for_mmap err!length:%u.\n",
			       length);
			ret = -ENOMEM;
			goto free;
		}
	}

	memset(mp_reg_addrs, 0, sizeof(mp_reg_addrs));

	for (i = 0; i < mp_cap_cfg.qpn_num; i++) {
		if (mp_cap_cfg.qpn[i] <
		    (iwdev->rf->sc_dev.base_qpn + 1) ||
		    mp_cap_cfg.qpn[i] >
			    (iwdev->rf->sc_dev.base_qpn +
			     iwdev->rf->max_qp - 1)) {
			pr_err("[zxdh_rdma] qpn:%u, base_qpn:%u overload",
			       mp_cap_cfg.qpn[i],
			       iwdev->rf->sc_dev.base_qpn);
			ret = -EINVAL;
			goto free;
		}

		iwqp = NULL;
		iwqp = iwdev->rf->qp_table[mp_cap_cfg.qpn[i] -
					   iwdev->rf->sc_dev.base_qpn];
		if (iwqp == NULL) {
			pr_err("[zxdh_rdma] mp cap invalid qpn:%u, no QP context\n",
			       mp_cap_cfg.qpn[i]);
			ret = -EINVAL;
			goto free;
		}

		if (iwqp->sc_qp.qp_uk.qp_type == ZXDH_QP_TYPE_ROCE_RC) {
			if (iwqp->sc_qp.use_blue_flame_flag) {
				gqp_id = get_rc_gqp_from_8k_index(
					iwqp->sc_qp.qp_uk.qp_8k_index,
					iwqp->sc_qp.ext_dev->vhca_gqp_start,
					iwqp->sc_qp.ext_dev->vhca_gqp_cnt);
			} else {
				gqp_id = get_rc_gqp_from_8k_index(
					iwqp->sc_qp.qp_uk.qp_8k_index,
					iwqp->sc_qp.dev->vhca_gqp_start,
					iwqp->sc_qp.dev->vhca_gqp_cnt);
			}
		} else {
			gqp_id = iwqp->sc_qp.dev->vhca_ud_gqp;
		}

		pr_info("[zxdh_rdma] mp cap qp_type:%u (1:RC,2:UD),qpn:%u,gqp_id:%u,vhcaid:%u!\n",
			iwqp->sc_qp.qp_uk.qp_type, mp_cap_cfg.qpn[i],
			gqp_id, iwqp->sc_qp.dev->vhca_id);

		add_unique_gqp_to_resp(&mp_cap_resp, gqp_id,
				       "for qpn mode");
	}

	ret = write_mp_cap_regs(iwdev, mp_cap_cfg.cap_use_l2d, &mp_cap_resp, ucontext);
	if (ret != 0) {
		pr_err("[zxdh_rdma] write_mp_cap_regs err! gqp_num:%u\n",
		       mp_cap_resp.cap_gqp_num);
		goto free;
	}

	rdma_tool_context = zxdh_get_rdma_tool_context(ucontext);
	if (rdma_tool_context) {
		rdma_tool_context->mp_cap_on = true;
		rdma_tool_context->mp_cap_gqps.gqp_num = mp_cap_resp.cap_gqp_num;
		memcpy(rdma_tool_context->mp_cap_gqps.gqpid, mp_cap_resp.cap_gqpid,
		       sizeof(rdma_tool_context->mp_cap_gqps.gqpid));
	}

	mp_cap_resp.mcode_type = iwdev->rf->mcode_type;
	pr_info("[zxdh_rdma] zxdh_rdma mp cap ib_copy_to_udata gqpid:%u,gqp_num:%u,cap_pa:%llx.mcode_type:%u!\n",
		mp_cap_resp.cap_gqpid[0], mp_cap_resp.cap_gqp_num,
		mp_cap_resp.cap_pa, mp_cap_resp.mcode_type);

#ifdef RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
	ret = uverbs_copy_to_struct_or_zero(attrs, ZXDH_IB_ATTR_DEV_MP_CAP_RESP,
					    &mp_cap_resp, sizeof(mp_cap_resp));
#else
	ret = zxdh_copy_to_struct_or_zero(attrs, ZXDH_IB_ATTR_DEV_MP_CAP_RESP,
					  &mp_cap_resp, sizeof(mp_cap_resp));
#endif

free:
	if (ret) {
		pr_err("[zxdh_rdma] zxdh_rdma mp cap failed %d!\n", ret);
		free_mp_cap_resource(ucontext);
		if (rdma_tool_context && rdma_tool_context->mp_cap_on) {
			ret = zxdh_reset_mp_cap_gqp_regs(&rdma_tool_context->mp_cap_gqps, ucontext); 
			if (ret) {
				pr_err("zxdh_reset_mp_cap_gqp_regs failed, ret=%d\n", ret);
			}
		}
	}
	mutex_unlock(&mp_cap_reg_lock);
	return ret;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_MP_GET_DATA)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_MP_GET_DATA)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	int ret = 0;
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct zxdh_ucontext *ucontext;
	struct zxdh_src_copy_dest src_dest = {};
	int status;
	u8 param;

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);

	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);
	ucontext = to_ucontext(ib_uctx);

	ret = uverbs_copy_from(&param, attrs, ZXDH_IB_ATTR_DEV_MP_GET_DATA);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	src_dest.src = ucontext->hw_data_cap.mp_cap_media_addr_base;

	if (iwdev->rf->use_ext_mem_flag) {
		src_dest.dest = ucontext->hw_data_cap.mp_cap.cap_ext_mem_addr.cap_dma_addr;
	} else {
		src_dest.dest = ucontext->hw_data_cap.mp_cap.addr_info.cap_direct_dma_addr.cap_dma_addr;
	}
	src_dest.len = ZXDH_L2D_MPCAP_BUFF_SIZE;
	status = zxdh_dpuddr_to_host_cmd(&iwdev->rf->sc_dev, &src_dest);
	if (status != 0) {
		pr_info("[zxdh_rdma] status:%d\n", status);
		return -EFAULT;
	}

	// pr_info("[zxdh_rdma] zxdh_dpuddr_to_host_cmd succ!\n");
	return 0;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_MP_CAP_CLEAR)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_MP_CAP_CLEAR)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	int ret = 0;
	struct ib_ucontext *ib_uctx;
	struct zxdh_cap_gqp cap_gqp;
	struct zxdh_ucontext *ucontext;

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);

	ucontext = to_ucontext(ib_uctx);

	ret = uverbs_copy_from(&cap_gqp, attrs, ZXDH_IB_ATTR_DEV_MP_CAP_CLEAR);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	mutex_lock(&mp_cap_reg_lock);
	ret = zxdh_reset_mp_cap_gqp_regs(&cap_gqp, ucontext);
	mutex_unlock(&mp_cap_reg_lock);
	if (ret) {
		pr_err("[zxdh_rdma] ZXDH_IB_METHOD_DEV_MP_CAP_CLEAR: reset failed, ret=%d\n", ret);
	}

	return ret;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_MP_CAP_GQP)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_MP_CAP_GQP)(struct ib_uverbs_file *file,
					      struct uverbs_attr_bundle *attrs)
#endif
{
	int ret, i;
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct zxdh_mp_cap_gqp_cfg gqp_cfg = { 0 };
	struct zxdh_ucontext *ucontext;
	struct zxdh_sc_dev *dev;
	uint16_t gqp_id;
	uint16_t gqp_start, gqp_end, vhca_ud_gqp;
	uint64_t mp_reg_addrs[MAX_CAP_QPS];
	struct zxdh_mp_cap_resp mp_cap_resp;
	uint32_t length;
	struct zxdh_rdma_tool_context *rdma_tool_context = NULL;

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);

	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);
	
	ucontext = to_ucontext(ib_uctx);
	dev = &iwdev->rf->sc_dev;
	vhca_ud_gqp = dev->vhca_ud_gqp;

	ret = uverbs_copy_from(&gqp_cfg, attrs,
			       ZXDH_IB_ATTR_DEV_MP_CAP_GQP);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	if (gqp_cfg.is_vm_capture && iwdev->rf->use_ext_mem_flag) {
		pr_err("[zxdh_rdma] VM capture is not supported on current device!\n");
		return -ZXDH_NOT_SUPPORT_CURRENT_DEVICE;
	}

	if (gqp_cfg.gqp_num == 0 || gqp_cfg.gqp_num > MAX_CAP_QPS)
		return -EINVAL;
	mutex_lock(&mp_cap_reg_lock);
	memset(&mp_cap_resp, 0, sizeof(mp_cap_resp));
	if (gqp_cfg.cap_use_l2d) {
		ucontext->hw_data_cap.mp_cap.addr_info.cap_direct_dma_addr.cap_cpu_addr = NULL;
		length = ZXDH_L2D_MPCAP_BUFF_SIZE;
		if (allocate_addr_for_mmap(iwdev, ucontext, length,
					   &ucontext->hw_data_cap.mp_cap,
					   &mp_cap_resp.cap_pa)) {
			pr_err("[zxdh_rdma] allocate_addr_for_mmap err!length:%u.\n",
			       length);
			ret = -ENOMEM;
			goto free;
		}
	}
	
	memset(mp_reg_addrs, 0, sizeof(mp_reg_addrs));
	gqp_start = dev->vhca_gqp_start;
	gqp_end = gqp_start + dev->vhca_gqp_cnt;
	for (i = 0; i < gqp_cfg.gqp_num; i++) {
		gqp_id = gqp_cfg.gqpid[i];
		if (gqp_cfg.is_vm_capture) {
			if (gqp_id > 2047) {
				pr_err("[zxdh_rdma] VM capture: gqp_id:%u out of range [1-2047]\n",
				       gqp_id);
				ret = -EINVAL;
				goto free;
			}
		} else {
			if ((gqp_id < gqp_start || gqp_id >= gqp_end) &&
			    gqp_id != vhca_ud_gqp) {
				pr_err("[zxdh_rdma] gqp_id:%u out of range [start:%u, end:%u] and not equal to vhca_ud_gqp:%u\n",
				       gqp_id, gqp_start, gqp_end > 1 ? gqp_end - 1 : gqp_end, vhca_ud_gqp);
				ret = -EINVAL;
				goto free;
			}
		}

		add_unique_gqp_to_resp(&mp_cap_resp, gqp_id, "gqp interface");
	}

	ret = write_mp_cap_regs(iwdev, gqp_cfg.cap_use_l2d, &mp_cap_resp, ucontext);
	if (ret != 0) {
		pr_err("[zxdh_rdma] write_mp_cap_regs (gqp) err! gqp_num:%u\n",
		       mp_cap_resp.cap_gqp_num);
		goto free;
	}

	rdma_tool_context = zxdh_get_rdma_tool_context(ucontext);
	if (rdma_tool_context) {
		rdma_tool_context->mp_cap_on = true;
		rdma_tool_context->mp_cap_gqps.gqp_num = mp_cap_resp.cap_gqp_num;
		memcpy(rdma_tool_context->mp_cap_gqps.gqpid, mp_cap_resp.cap_gqpid,
		       sizeof(rdma_tool_context->mp_cap_gqps.gqpid));
	}

	mp_cap_resp.mcode_type = iwdev->rf->mcode_type;
	pr_info("[zxdh_rdma] zxdh_rdma mp cap gqp ib_copy_to_udata gqpid:%u,gqp_num:%u,cap_pa:%llx.mcode_type:%u!\n",
		mp_cap_resp.cap_gqpid[0], mp_cap_resp.cap_gqp_num,
		mp_cap_resp.cap_pa, mp_cap_resp.mcode_type);
#ifdef RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
	ret = uverbs_copy_to_struct_or_zero(attrs, ZXDH_IB_ATTR_DEV_MP_CAP_GQP_RESP,
					    &mp_cap_resp, sizeof(mp_cap_resp));
#else
	ret = zxdh_copy_to_struct_or_zero(attrs, ZXDH_IB_ATTR_DEV_MP_CAP_GQP_RESP,
					  &mp_cap_resp, sizeof(mp_cap_resp));
#endif

free:
	if (ret) {
		pr_err("[zxdh_rdma] zxdh_rdma mp cap failed %d!\n", ret);
		free_mp_cap_resource(ucontext);
		if (rdma_tool_context && rdma_tool_context->mp_cap_on) {
			ret = zxdh_reset_mp_cap_gqp_regs(&rdma_tool_context->mp_cap_gqps, ucontext); 
			if (ret) {
				pr_err("zxdh_reset_mp_cap_gqp_regs failed, ret=%d\n", ret);
			}
		}
	}
	mutex_unlock(&mp_cap_reg_lock);
	return ret;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_GET_ACT_VHCA_GQPS)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_GET_ACT_VHCA_GQPS)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	int ret, gqp_id;
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct zxdh_sc_dev *dev;
	struct ib_device *ib_dev;
	struct zxdh_active_vhca_gqps get_active_vhca_gqps_resp = { 0 };
	u16 vhca_id, gqp_start, gqp_cnt;
	uint32_t read_reg_val = 0;
	uint32_t qps_act_bit = 0x80000000;

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);

	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);
	dev = &iwdev->rf->sc_dev;

	vhca_id = dev->vhca_id;
	if (vhca_id > 257) {
		pr_err("[zxdh_rdma] vhca_id:%u bigger ths 257,err!\n", vhca_id);
		return -EINVAL;
	}
	gqp_start = dev->vhca_gqp_start;
	gqp_cnt = dev->vhca_gqp_cnt;
	pr_info("[zxdh_rdma] zxdh_get_active_vhca_gqps vhca_id:%u, gqp_start:%u, gqp_cnt:%u\n",
		vhca_id, gqp_start, gqp_cnt);
	mutex_lock(&active_gqp_reg_lock);
	for (gqp_id = gqp_start; gqp_id < gqp_start + gqp_cnt; gqp_id++) {
		ret = zxdh_rdma_reg_write(iwdev->rf, C_CHECK_GQP_ACTIVE_WRITE, gqp_id);
		if (ret != 0) {
			mutex_unlock(&active_gqp_reg_lock);
			return ret;
		}
		ret = zxdh_rdma_reg_read(iwdev->rf, C_CHECK_GQP_ACTIVE_READ, &read_reg_val);
		if (ret != 0) {
			mutex_unlock(&active_gqp_reg_lock);
			return ret;
		}
		if ((read_reg_val & qps_act_bit) != 0) {
			if (get_active_vhca_gqps_resp.gqp_num < MAX_ACTIVE_GQP_NUM) {
				get_active_vhca_gqps_resp
					.gqp_id[get_active_vhca_gqps_resp.gqp_num] =
					gqp_id;
				get_active_vhca_gqps_resp.gqp_num++;
			} else {
				pr_warn("[zxdh_rdma] Active GQP count exceeds MAX_ACTIVE_GQP_NUM:%d, gqp_id:%u will be ignored\n",
					MAX_ACTIVE_GQP_NUM, gqp_id);
			}
		}
	}
	mutex_unlock(&active_gqp_reg_lock);
	if (get_active_vhca_gqps_resp.gqp_num > 0)
		get_active_vhca_gqps_resp.vhca_id = vhca_id;
	pr_info("[zxdh_rdma] zxdh_get_active_vhca_gqps vhca_id:%u,gqp_num:%u\n",
		get_active_vhca_gqps_resp.vhca_id,
		get_active_vhca_gqps_resp.gqp_num);
#ifdef RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
	ret = uverbs_copy_to_struct_or_zero(
		attrs, ZXDH_IB_ATTR_DEV_GET_ACT_VHCA_GQPS_RESP,
		&get_active_vhca_gqps_resp, sizeof(get_active_vhca_gqps_resp));
#else
	ret = zxdh_copy_to_struct_or_zero(
		attrs, ZXDH_IB_ATTR_DEV_GET_ACT_VHCA_GQPS_RESP,
		&get_active_vhca_gqps_resp, sizeof(get_active_vhca_gqps_resp));
#endif
	if (ret) {
		pr_err("[zxdh_rdma] zxdh_get_active_vhca_gqps ib_copy_to_udata failed!\n");
		return -EFAULT;
	}
	return 0;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_GET_CC_BASIC_INFO)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_GET_CC_BASIC_INFO)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	int ret;
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	uint8_t i;
	uint32_t read_reg_val = 0;
	uint32_t read_reg_val_ex = 0;
	uint64_t read_value;
	struct zxdh_cc_basic_info resp = { 0 };

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);

	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);

	mutex_lock(&cc_basic_info_lock);
	for (i = 0; i < 8; i++) {
		ret = zxdh_rdma_reg_write(iwdev->rf, C_RDMA_TX_SUB_RW_RSV0, i);
		if (ret != 0) {
			mutex_unlock(&cc_basic_info_lock);
			return ret;
		}
		ret = zxdh_rdma_reg_read(iwdev->rf, C_RDMA_TX_SUB_RO_RSV5, &read_reg_val);
		if (ret != 0) {
			mutex_unlock(&cc_basic_info_lock);
			return ret;
		}
		resp.active_gqp_cnt += read_reg_val;
	}
	mutex_unlock(&cc_basic_info_lock);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 C_SQ_CPU_MAINTAIN_RESERVE1, &read_reg_val);
	resp.active_vhca_sq_cnt = EXTRACT_BITS(read_reg_val, 10, 22);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 C_RQ_CPU_MAINTAIN_RESERVE1, &read_reg_val);
	resp.active_vhca_read_cnt = EXTRACT_BITS(read_reg_val, 10, 22);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 C_ACK_CPU_MAINTAIN_RESERVE1, &read_reg_val);
	resp.active_vhca_ack_cnt = EXTRACT_BITS(read_reg_val, 10, 22);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 C_DB_AXI_INTERFACW_STATE_REG2, &read_reg_val);
	resp.active_qp_sq_cur_cnt = EXTRACT_BITS(read_reg_val, 16, 31);
	resp.active_qp_rq_cur_cnt = EXTRACT_BITS(read_reg_val, 0, 15);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 C_WQE_PREFETCH_TOP_FIFO_WE_RD_CNT0, &read_reg_val);
	resp.task_prefetch_recv_com_cnt = EXTRACT_BITS(read_reg_val, 16, 31);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 C_RDMATX_ARBITRATION_DIN_0, &read_reg_val);
	resp.flight_pkt_cnt = EXTRACT_BITS(read_reg_val, 0, 8);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 HOST3_ERR_INFO_FIFO_OVERFLOW_CNT, &read_reg_val);
	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_PKT_TIME_OUT_CNT,
			 &read_reg_val_ex);
	read_value = EXTRACT_BITS(read_reg_val_ex, 16, 31);
	resp.tx_pkt_cnt = ((read_value << 32) | read_reg_val);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_ICRC_PROC_EOP_CNT_HW,
			 &read_reg_val);
	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_ICRC_PROC_SOP_CNT_HW,
			 &read_reg_val_ex);
	read_value = EXTRACT_BITS(read_reg_val_ex, 0, 15);
	resp.rx_pkt_cnt = ((read_value << 32) | read_reg_val);
	resp.backpres_rx = EXTRACT_BITS(read_reg_val_ex, 23, 26);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 RDMATX_ACK_RSV_RO_REG_0_HW, &read_reg_val);
	resp.retry_timeout_cnt = EXTRACT_BITS(read_reg_val, 0, 15);
	resp.retry_read_cnt = EXTRACT_BITS(read_reg_val, 16, 31);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, RDMATX_ACK_RSV_RO_REG_1,
			 &read_reg_val);
	resp.retry_nak_cnt = EXTRACT_BITS(read_reg_val, 16, 31);
	resp.retry_rnr_cnt = EXTRACT_BITS(read_reg_val, 0, 15);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 RDMATX_ACK_RD_MSG_LOSS_FLAG_CNT, &read_reg_val);
	resp.drop_read_msg_cnt = EXTRACT_BITS(read_reg_val, 0, 15);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_MUL_CACHE_ARBITER_D2B_SOP_CNT,
			 &read_reg_val);
	resp.rx_pkt_ecn_cnt = EXTRACT_BITS(read_reg_val, 16, 31);

	read_reg_val = readl((u32 __iomem *)(iwdev->rf->sc_dev.hw->hw_addr +
					    C_STATE_ERR_CFG(iwdev->rf->sc_dev.ep_id)));
	resp.tx_pkt_cnp_cnt = EXTRACT_BITS(read_reg_val, 0, 15);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 C_NHD_CHECK_ICRC_REMOVAL_EOP_CNT_HW, &read_reg_val);
	resp.rx_pkt_cnp_cnt = read_reg_val;

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, PKT_RTT_T1_GEN_SOP_CNT,
			 &read_reg_val);
	resp.tx_pkt_rtt_t1_cnt = read_reg_val;

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 C_NHD_CHECK_RTT_PROC_SOP_CNT, &read_reg_val);
	resp.rx_pkt_rtt_t2_cnt = read_reg_val;

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_RAM_TEST_RSV_1,
			 &read_reg_val);
	resp.tx_pkt_rtt_t4_cnt = EXTRACT_BITS(read_reg_val, 10, 25);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 C_NHD_CHECK_RTT_PROC_EOP_CNT, &read_reg_val);
	resp.rx_pkt_rtt_t5_cnt = read_reg_val;

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 C_SQ_CPU_FIFO_OVERFLOW_CNT, &read_reg_val);
	resp.limit_tx_sq_cnt = EXTRACT_BITS(read_reg_val, 6, 21);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 C_RQ_CPU_FIFO_OVERFLOW_CNT, &read_reg_val);
	resp.limit_tx_read_ack_cnt = EXTRACT_BITS(read_reg_val, 6, 21);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 C_SQ_CPU_MAINTAIN_RESERVE2, &read_reg_val);
	resp.backpres_tx_pfc_flg_pyh0_3 = read_reg_val;

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf,
			 C_SQ_CPU_MAINTAIN_RESERVE3, &read_reg_val);
	resp.backpres_tx_pfc_flg_pyh4_7 = read_reg_val;

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_NP_RDY_TEST,
			 &read_reg_val);
	resp.backpres_tx_pfc_cnt = EXTRACT_BITS(read_reg_val, 0, 15);

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_ICRC_CHECK_SOP_CNT_HW,
			 &read_reg_val);
	resp.backpres_rx_pfc_cnt = EXTRACT_BITS(read_reg_val, 28, 31);

#ifdef RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
	ret = uverbs_copy_to_struct_or_zero(
		attrs, ZXDH_IB_ATTR_DEV_GET_CC_BASIC_INFO_RESP, &resp,
		sizeof(resp));
#else
	ret = zxdh_copy_to_struct_or_zero(
		attrs, ZXDH_IB_ATTR_DEV_GET_CC_BASIC_INFO_RESP, &resp,
		sizeof(resp));
#endif
	if (ret) {
		pr_err("[zxdh_rdma] zxdh_get_cc_basic_info ib_copy_to_udata failed!\n");
		return -EFAULT;
	}
	return 0;
}

static int
fill_qpc(
	struct zxdh_pci_f *rf, 
	struct zxdh_context_req *context_req,
	struct zxdh_dma_mem *resource_buf) 
{
	struct zxdh_sc_dev *dev;
	int err_code = 0;
	__u32 qpn;

	dev = &rf->sc_dev;
	qpn = context_req->resource_id;
	if (qpn == 1) 
		qpn = dev->base_qpn + 1;
	if (qpn < (dev->base_qpn + 1) || qpn > (dev->base_qpn + rf->max_qp - 1)){
		pr_err("[zxdh_rdma] get qpc err, qpn is out of boundary\n");
		pr_err("[zxdh_rdma] qpn boundary :[%d,%d]\n", dev->base_qpn + 1, dev->base_qpn + rf->max_qp - 1);
		return -EOVERFLOW;
	}
	err_code = zxdh_fill_qpc(dev, qpn, resource_buf);
	return err_code;
}

static int
fill_cqc(
	struct zxdh_pci_f *rf, 
	struct zxdh_context_req *context_req,
	struct zxdh_dma_mem *resource_buf) 
{
	struct zxdh_sc_dev *dev;
	int err_code = 0;
	__u32 cqn;

	dev = &rf->sc_dev;
	cqn = context_req->resource_id;
	if (cqn < (dev->base_cqn + 1) || cqn > (dev->base_cqn + rf->max_cq - 1)) {
		pr_err("[zxdh_rdma] get cqc err, cqn is out of boundary\n");
		pr_err("[zxdh_rdma] cqn boundary :[%d,%d]\n", dev->base_cqn + 1, dev->base_cqn + rf->max_cq - 1);
		return -EOVERFLOW;
	}
	err_code = zxdh_fill_cqc(dev, cqn, resource_buf);
	return err_code;
}

static int
fill_ceqc(
	struct zxdh_pci_f *rf, 
	struct zxdh_context_req *context_req,
	struct zxdh_dma_mem *resource_buf) 
{
	struct zxdh_sc_dev *dev;
	int err_code = 0;
	__u32 ceqn;

	dev = &rf->sc_dev;
	ceqn = context_req->resource_id;
	if (ceqn < dev->base_ceqn || ceqn > (dev->base_ceqn + rf->sc_dev.max_ceqs - 1)) {
		pr_err("[zxdh_rdma] get ceqc err, ceqn is out of boundary\n");
		pr_err("[zxdh_rdma] ceqn boundary :[%d,%d]\n", dev->base_ceqn, dev->base_ceqn + rf->sc_dev.max_ceqs - 1);
		return -EOVERFLOW;
	}
	err_code = zxdh_fill_ceqc(dev, ceqn, resource_buf);
	return err_code;
}

static int
fill_aeqc(
	struct zxdh_pci_f *rf, 
	struct zxdh_context_req *context_req,
	struct zxdh_dma_mem *resource_buf) 
{
	struct zxdh_sc_dev *dev;
	int err_code = 0;

	dev = &rf->sc_dev;
	err_code = zxdh_fill_aeqc(dev, resource_buf);
	return err_code;
}

static int
fill_srqc(
	struct zxdh_pci_f *rf, 
	struct zxdh_context_req *context_req,
	struct zxdh_dma_mem *resource_buf) 
{
	struct zxdh_sc_dev *dev;
	int err_code = 0;
	__u32 srqn;

	dev = &rf->sc_dev;
	srqn = context_req->resource_id;
	if (srqn < dev->base_srqn || srqn > (dev->base_srqn + rf->max_srq - 1)) {
		pr_err("[zxdh_rdma] get srqc err, srqn is out of boundary\n");
		pr_err("[zxdh_rdma] srqn boundary :[%d,%d]\n", dev->base_srqn, dev->base_srqn + rf->max_srq - 1);
		return -EOVERFLOW;
	}
	err_code = zxdh_fill_srqc(dev, srqn, resource_buf);
	return err_code;
}

static int
fill_mrte(
	struct zxdh_pci_f *rf,
	struct zxdh_context_req *context_req,
	struct zxdh_dma_mem *resource_buf)
{
	struct zxdh_src_copy_dest src_dest = { 0 };
	int err_code = 0;
	__u32 stag, stag_index;

	stag = context_req->resource_id;
	stag_index = stag >> ZXDH_CQPSQ_STAG_IDX_S;
	if (stag_index > (rf->max_mr - 1)) {
		pr_err("[zxdh_rdma] get mrte err, stag is out of boundary\n");
		pr_err("[zxdh_rdma] stag_index boundary :[0,%d]\n", rf->max_mr - 1);
		return -EOVERFLOW;
	}
	src_dest.src = 64 * stag_index;
	src_dest.dest = resource_buf->pa;
	src_dest.len = resource_buf->size;
	err_code = zxdh_cqp_rdma_read_mrte_cmd(&rf->sc_dev, &src_dest);
	if (err_code) {
		pr_err("[zxdh_rdma] res mrte entry raw fill qpc failed:%d\n", err_code);
		return err_code;
	}
	return 0;
}

static int
get_buf_info(struct zxdh_context_req *context_req, 
		   int *buf_size, int *data_size, int *buf_alignment,
		   int (**fill_ctx)(struct zxdh_pci_f *, struct zxdh_context_req *, struct zxdh_dma_mem *))
{
	switch (context_req->type)
	{
	case ZXDH_RX_READ_QPC:
		*buf_size = ZXDH_QP_CTX_SIZE;
		*data_size = ZXDH_RX_READ_QPC_SIZE;
		*buf_alignment = ZXDH_QPC_ALIGNMENT;
		*fill_ctx = fill_qpc;
		break;

	case ZXDH_TX_READ_QPC:
		*buf_size = ZXDH_QP_CTX_SIZE;
		*data_size = ZXDH_TX_READ_QPC_SIZE;
		*buf_alignment = ZXDH_QPC_ALIGNMENT;
		*fill_ctx = fill_qpc;
		break;
		
	case ZXDH_READ_CQC:
		*buf_size = ZXDH_CQ_CTX_SIZE;
		*data_size = ZXDH_READ_CQC_SIZE;
		*buf_alignment = ZXDH_CQC_ALIGNMENT;
		*fill_ctx = fill_cqc;
		break;
	
	case ZXDH_READ_CEQC:
		*buf_size = ZXDH_CEQ_CTX_SIZE;
		*data_size = ZXDH_READ_CEQC_SIZE;
		*buf_alignment = ZXDH_CEQC_ALIGNMENT;
		*fill_ctx = fill_ceqc;
		break;
	
	case ZXDH_READ_AEQC:
		*buf_size = ZXDH_AEQ_CTX_SIZE;
		*data_size = ZXDH_READ_AEQC_SIZE;
		*buf_alignment = ZXDH_AEQC_ALIGNMENT;
		*fill_ctx = fill_aeqc;
		break;

	case ZXDH_RX_READ_SRQC:
		*buf_size = ZXDH_SRQ_CTX_SIZE;
		*data_size = ZXDH_RX_READ_SRQC_SIZE;
		*buf_alignment = ZXDH_SRQC_ALIGNMENT;
		*fill_ctx = fill_srqc;
		break;

	case ZXDH_READ_MRTE:
		*buf_size = ZXDH_READ_MRTE_SIZE;
		*data_size = ZXDH_READ_MRTE_SIZE;
		*buf_alignment = ZXDH_QPC_ALIGNMENT;
		*fill_ctx = fill_mrte;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_GET_HMC)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_GET_HMC)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	int ret = 0, buf_size = 0, data_size = 0, buf_alignment = 0, err_code = 0;
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct zxdh_pci_f *rf;
	struct zxdh_context_req context_req = { 0 };
	struct zxdh_dma_mem resource_buf = { 0 };
	struct zxdh_context_resp context_resp;
	int (*fill_ctx)(struct zxdh_pci_f *, struct zxdh_context_req *, struct zxdh_dma_mem *) = NULL;

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);
	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);
	rf = iwdev->rf;
	
	ret = uverbs_copy_from(&context_req, attrs, ZXDH_IB_ATTR_DEV_GET_HMC);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;
	
	memset(&context_resp, 0, sizeof(struct zxdh_context_resp));
	err_code = get_buf_info(&context_req, &buf_size, &data_size, &buf_alignment, &fill_ctx);
	if (err_code) {
		pr_err("[zxdh_rdma] req type not exist: %d\n", err_code);
		return err_code;
	}
	context_resp.context_size = data_size / 8;
	resource_buf.va = NULL;
	resource_buf.size = ALIGN(buf_size, buf_alignment);
	resource_buf.va = zxdh_alloc_raw_mem(iwdev, resource_buf.size, &resource_buf.pa);
	if (!resource_buf.va) {
		pr_err("[zxdh_rdma] get qpc alloc failed:ENOMEM\n");
		return -ENOMEM;
	}

	err_code = fill_ctx(rf, &context_req, &resource_buf);
	if (err_code) {
		pr_err("[zxdh_rdma] get ctx fill buf failed:%d\n", err_code);
		goto free_exit;
	}

	if(context_req.type == ZXDH_RX_READ_QPC) {
		memcpy(context_resp.context_info, (void *)((__u8 *)resource_buf.va + ZXDH_RX_QPC_SHIFT), data_size);
	} else {
		memcpy(context_resp.context_info, resource_buf.va, data_size);
	}
	#ifdef RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
	ret = uverbs_copy_to_struct_or_zero(attrs, ZXDH_IB_ATTR_DEV_GET_HMC_RESP,
					&context_resp, sizeof(context_resp));
	#else
	ret = zxdh_copy_to_struct_or_zero(attrs, ZXDH_IB_ATTR_DEV_GET_HMC_RESP,
					&context_resp, sizeof(context_resp));
	#endif
	if (ret) {
		pr_err("[zxdh_rdma] zxdh_get_ctx ib_copy_to_udata failed!\n");
		return -EFAULT;
	}
free_exit:
	zxdh_free_raw_mem(iwdev, resource_buf.va, resource_buf.size, resource_buf.pa);
	resource_buf.va = NULL;
	return err_code;
}

static int validate_hw_object_id(struct zxdh_get_object_data_req *object_req)
{
	switch (object_req->object_id) {
	case ZXDH_PBLE_MR_OBJ_ID:
	case ZXDH_PBLE_QUEUE_OBJ_ID:
	case ZXDH_AH_OBJ_ID:
	case ZXDH_IRD_OBJ_ID:
	case ZXDH_TX_WINDOW_OBJ_ID:
	case ZXDH_CQ_SHADOW_AREA:
	case ZXDH_CQ:
	case ZXDH_AEQ:
	case ZXDH_RQ:
	case ZXDH_RQ_SHADOW_AREA:
	case ZXDH_SRQP:
	case ZXDH_SRQ:
	case ZXDH_SRQ_SHADOW_AREA:
	case ZXDH_SQ:
	case ZXDH_CEQ:
	case ZXDH_SQC_OBJ_ID:
		return 0;
	default:
		return -ZXDH_NOT_SUPPORT_OBJECT_ID;
	}
}

const int object_interface_type[] = {
	[ZXDH_PBLE_MR_OBJ_ID] = ZXDH_INTERFACE_CACHE,
	[ZXDH_PBLE_QUEUE_OBJ_ID] = ZXDH_INTERFACE_CACHE,
	[ZXDH_AH_OBJ_ID] = ZXDH_INTERFACE_CACHE,
	[ZXDH_IRD_OBJ_ID] = ZXDH_INTERFACE_CACHE,
	[ZXDH_TX_WINDOW_OBJ_ID] = ZXDH_INTERFACE_CACHE,
	[ZXDH_CQ] = ZXDH_INTERFACE_NOTCACHE,
	[ZXDH_CQ_SHADOW_AREA] = ZXDH_INTERFACE_NOTCACHE,
	[ZXDH_AEQ] = ZXDH_INTERFACE_NOTCACHE,
	[ZXDH_SQ] = ZXDH_INTERFACE_NOTCACHE,
	[ZXDH_RQ] = ZXDH_INTERFACE_NOTCACHE,
	[ZXDH_RQ_SHADOW_AREA] = ZXDH_INTERFACE_NOTCACHE,
	[ZXDH_SRQP] = ZXDH_INTERFACE_NOTCACHE,
	[ZXDH_SRQ] = ZXDH_INTERFACE_NOTCACHE,
	[ZXDH_SRQ_SHADOW_AREA] = ZXDH_INTERFACE_NOTCACHE,
	[ZXDH_CEQ] = ZXDH_INTERFACE_NOTCACHE,
	[ZXDH_SQC_OBJ_ID] = ZXDH_INTERFACE_NOTCACHE,
};

static int
pre_validate_hw_object_request(struct zxdh_get_object_data_req *object_req)
{
	int ret = 0;
	ret = validate_hw_object_id(object_req);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, validate request object id err:%d, object_id: %d",
		       ret, object_req->object_id);
		return ret;
	}
	return ret;
}

static void hw_object_wqe_init(struct hw_object_wqe_context *object_wqe_ctx,
			       struct zxdh_sc_dev *dev,
			       struct zxdh_get_object_data_req *object_req)
{
	object_wqe_ctx->op_code = ZXDH_OP_QUERY_HW_OBJECT_INFO;
	object_wqe_ctx->src_vhca_Index = dev->vhca_id;
	object_wqe_ctx->src_object_id = object_req->object_id == ZXDH_SQC_OBJ_ID ? ZXDH_SQ : object_req->object_id;
	object_wqe_ctx->src_waypartition = 0;
	object_wqe_ctx->src_interface_select =
		object_interface_type[object_req->object_id];

	object_wqe_ctx->dest_vhca_index = dev->vhca_id;
	object_wqe_ctx->dest_object_id = ZXDH_DMA_OBJ_ID;
	object_wqe_ctx->dest_waypartition = 0;
	object_wqe_ctx->dest_path_select = ZXDH_INDICATE_HOST_NOSMMU;
	object_wqe_ctx->dest_interface_select = ZXDH_INTERFACE_NOTCACHE;

	object_wqe_ctx->zxdh_hmc_rsrc_type = ZXDH_HMC_IW_MAX;
	object_wqe_ctx->req = object_req;
	object_wqe_ctx->dev = dev;
}

static void
set_cache_object_wqe_ctx(struct hw_object_wqe_context *object_context,
			 struct zxdh_get_object_data_req *object_req,
			 u32 object_size)
{
	object_context->src_address = object_req->entry_idx * object_size;
	object_context->data_length = object_req->object_num * object_size;
	object_context->object_size = object_size;
}
/*
static int query_route_id_from_ram(struct zxdh_sc_dev *dev, u32 ram_num,
				   u8 *route_id)
{
	int ret = 0;
	u32 ram_value = 0;
	ret = zxdh_read_ram_32bit_value(dev, ram_num, ZXDH_RAM_WIDTH_32_BIT,
					ZXDH_RAM_WIDTH_LEN_UNIT_1,
					ZXDH_RAM_32_BIT_IDX_0, &ram_value);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, query object cache id err:%d", ret);
		return ret;
	}
	*route_id = ram_value & 0x3;
	return ret;
}
*/
static int query_route_id_from_reg(struct zxdh_sc_dev *dev, u64 reg_base,
				   u8 bit_low, u8 bit_higth, u8 *route_id)
{
	uint64_t address;
	uint32_t read_reg_val = 0;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	address = reg_base + dev->vhca_id * ROUTE_ID_REG_SIZE;
	REG_OP_AND_CHECK(zxdh_rdma_reg_read, rf, address, &read_reg_val);
	*route_id = EXTRACT_BITS(read_reg_val, bit_low, bit_higth);
	return 0;
}

static int
prepare_query_pble_mr_wqe_context(struct zxdh_sc_dev *dev,
				  struct zxdh_get_object_data_req *object_req,
				  struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	object_context->zxdh_hmc_rsrc_type = ZXDH_HMC_IW_PBLE_MR;
	set_cache_object_wqe_ctx(object_context, object_req,
				 ZXDH_PBLE_MR_QUADRUPLE_SIZE);
	// ret = query_route_id_from_ram(dev, ZXDH_RAM_H35, &cache_id);
	// if (ret) {
	// 	pr_err("[zxdh_rdma] query hw object, query pble mr route id err:%d", ret);
	// 	return ret;
	// }
	// object_context->src_path_select = cache_id;
	object_context->src_path_select = dev->hmc_info->pble_mr_cachid;
	return ret;
}

static int prepare_query_pble_queue_wqe_context(
	struct zxdh_sc_dev *dev, struct zxdh_get_object_data_req *object_req,
	struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	u8 cache_id = 0;
	object_context->zxdh_hmc_rsrc_type = ZXDH_HMC_IW_PBLE;
	set_cache_object_wqe_ctx(object_context, object_req,
				 ZXDH_PBLE_QUEUE_QUADRUPLE_SIZE);
	ret = query_route_id_from_reg(dev, PBLE_QUEUE_CACHE_ID_BASE, 0, 1,
				      &cache_id);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, query pble queue route id err:%d\n",
		       ret);
		return ret;
	}
	object_context->src_path_select = cache_id;
	return ret;
}

static int
prepare_query_ah_wqe_context(struct zxdh_sc_dev *dev,
			     struct zxdh_get_object_data_req *object_req,
			     struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	u8 cache_id = 0;
	object_context->zxdh_hmc_rsrc_type = ZXDH_HMC_IW_AH;
	set_cache_object_wqe_ctx(object_context, object_req, ZXDH_AH_SIZE);
	ret = query_route_id_from_reg(dev, AH_CACHE_ID_BASE, 0, 1, &cache_id);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, query ah route id err:%d\n", ret);
		return ret;
	}
	object_context->src_path_select = cache_id;
	return ret;
}

static int
prepare_query_ird_wqe_context(struct zxdh_sc_dev *dev,
			      struct zxdh_get_object_data_req *object_req,
			      struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	object_context->zxdh_hmc_rsrc_type = ZXDH_HMC_IW_IRD;
	set_cache_object_wqe_ctx(object_context, object_req, ZXDH_IRD_SIZE);
	// ret = query_route_id_from_ram(dev, ZXDH_RAM_H100, &cache_id);
	// if (ret) {
	// 	pr_err("[zxdh_rdma] query hw object, query ird cache id err:%d", ret);
	// 	return ret;
	// }
	// object_context->src_path_select = cache_id;
	object_context->src_path_select = dev->hmc_info->pble_ird_cachid;
	return ret;
}

static bool validate_qpn(struct zxdh_pci_f *rf,
			 struct zxdh_get_object_data_req *object_req)
{
	struct zxdh_sc_dev *dev = NULL;
	dev = &rf->sc_dev;
	if (object_req->queue_id == 1)
		object_req->queue_id = dev->base_qpn + 1;
	if (object_req->queue_id < (dev->base_qpn + 1) ||
	    object_req->queue_id > (dev->base_qpn + rf->max_qp - 1)) {
		pr_err("[zxdh_rdma] qpn is out of boundary\n");
		pr_err("[zxdh_rdma] qpn boundary :[%d,%d]\n", dev->base_qpn + 1,
		       dev->base_qpn + rf->max_qp - 1);
		return false;
	}
	return true;
}

static int
prepare_query_tx_window_context(struct zxdh_sc_dev *dev,
				struct zxdh_get_object_data_req *object_req,
				struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	u8 cache_id = 0;
	uint32_t tx_window_ddr_size = 0;
	struct zxdh_pci_f *rf = dev_to_rf(dev);

	if (!validate_qpn(rf, object_req))
		return -ZXDH_QUEUE_ID_ERROR;

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, rf, TX_WINDOW_DDR_SIZE_REG,
			 &tx_window_ddr_size);
	pr_info("[zxdh_rdma] tx window rdd size 0x%x", tx_window_ddr_size);
	object_req->entry_idx =
		((object_req->queue_id - dev->base_qpn) << tx_window_ddr_size) +
		object_req->entry_idx;
	set_cache_object_wqe_ctx(object_context, object_req,
				 ZXDH_TX_WINDOW_SIZE);
	ret = query_route_id_from_reg(dev, TX_WINDOW_CACHE_ID_BASE, 0, 1,
				      &cache_id);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, query tx window route id err:%d\n",
		       ret);
		return ret;
	}
	object_context->src_path_select = cache_id;
	return ret;
}

static int query_hw_object_cqc(struct zxdh_sc_dev *dev,
			       struct zxdh_get_object_data_req *object_req,
			       struct zxdh_cqc_item *cqc_item)
{
	int ret = 0;
	struct zxdh_context_req context_req = { 0 };
	struct zxdh_dma_mem resource_buf = { 0 };
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	struct zxdh_device *iwdev = rf->iwdev;
	context_req.resource_id = object_req->queue_id;
	resource_buf.va = NULL;
	resource_buf.size = ALIGN(ZXDH_READ_CQC_SIZE, ZXDH_CQC_ALIGNMENT);
	resource_buf.va = zxdh_alloc_raw_mem(iwdev, resource_buf.size,
					     &resource_buf.pa);
	if (!resource_buf.va) {
		pr_err("[zxdh_rdma] query cq shadow area, allocate err: ENOMEM\n");
		return -ENOMEM;
	}
	ret = fill_cqc(rf, &context_req, &resource_buf);
	if (ret) {
		pr_err("[zxdh_rdma] query cq shadow area, query cqc err: %d\n", ret);
		goto free_exit;
	}

	cqc_item->leaf_pbl_size = ZXDH_GET_QPC_ITEM(
		u8, resource_buf.va, ZXDH_CQC_LEAF_PBLE_SIZE_BYTE_OFFSET,
		RDMACPC_LEAF_PBL_SIZE);
	cqc_item->doorbell_shadow_addr = ZXDH_GET_QPC_ITEM(
		u64, resource_buf.va, ZXDH_CQC_DOORBELL_SHADOW_ADDR_BYTE_OFFSET,
		RDMACPC_DOORBELL_SHADOW_ADDR);
	cqc_item->log_cqe_num = ZXDH_GET_QPC_ITEM(
		u8, resource_buf.va, ZXDH_CQC_LOG_CQE_NUM, RDMACPC_LOG_CQE_NUM);
	cqc_item->hw_cq_head = ZXDH_GET_QPC_ITEM(
		u32, resource_buf.va, ZXDH_CQC_HW_CQ_HEAD_BYTE_OFFSET,
		RDMACPC_HW_CQ_HEAD);
	cqc_item->cq_address = ZXDH_GET_QPC_ITEM(
		u64, resource_buf.va, ZXDH_CQC_CQ_ADDRESS_BYTE_OFFSET,
		RDMACPC_CQ_ADDRESS);
	cqc_item->root_pble = ZXDH_GET_QPC_ITEM(
		u64, resource_buf.va, ZXDH_CQC_HW_CQ_ROOT_PBLE_BYTE_OFFSET,
		RDMACPC_ROOT_PBLE);
	pr_info("[zxdh_rdma] query hw object, cqc leaf_pbl_size: 0x%x, doorbell_shadow_addr: 0x%llx, log_cqe_num: 0x%x, hw_cq_head: 0x%x,  cq_address: 0x%llx, root_pble: 0x%llx",
		cqc_item->leaf_pbl_size, cqc_item->doorbell_shadow_addr,
		cqc_item->log_cqe_num, cqc_item->hw_cq_head,
		cqc_item->cq_address, cqc_item->root_pble);

free_exit:
	if (resource_buf.va) {
		zxdh_free_raw_mem(iwdev, resource_buf.va, resource_buf.size,
				  resource_buf.pa);
		resource_buf.va = NULL;
	}
	return ret;
}

static bool validate_cqn(struct hw_object_wqe_context *object_context)
{
	u32 cqn = object_context->req->queue_id;
	u32 base_cqn = object_context->dev->base_cqn;
	u32 max_cqn =
		base_cqn +
		object_context->dev->hmc_info->hmc_obj[ZXDH_HMC_IW_CQ].cnt - 1;
	if (cqn >= base_cqn && cqn <= max_cqn) {
		return true;
	}

	pr_err("[zxdh_rdma] query hw object, validate cqn error, cqn range:[%u,%u], current cqn: %u\n",
	       base_cqn, max_cqn, cqn);
	return false;
}

static int prepare_query_cq_doorbell_wqe_context(
	struct zxdh_sc_dev *dev, struct zxdh_get_object_data_req *object_req,
	struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	struct zxdh_cqc_item cqc_item = { 0 };
	u8 indicate_id = 0;
	if (!validate_cqn(object_context)) {
		pr_err("[zxdh_rdma] query hw object, validate cqp id err:%d", ret);
		return -ZXDH_QUEUE_ID_ERROR;
	}
	ret = query_hw_object_cqc(dev, object_req, &cqc_item);
	if (ret)
		return ret;

	ret = query_route_id_from_reg(dev, CQ_DOORBELL_SHADOW_BASE, 2, 3,
				      &indicate_id);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, query tx window route id err:%d\n",
		       ret);
		return ret;
	}
	object_context->data_length = ZXDH_CQ_SHADOW_AREA_SIZE;
	object_context->src_address = cqc_item.doorbell_shadow_addr << 6;
	object_context->src_path_select = indicate_id;
	return ret;
}

static int
compute_cq_src_address_and_length(struct hw_object_wqe_context *object_context,
				  struct zxdh_cqc_item *cqc_item)
{
	u32 cqe_num = 0;
	int ret = 0;
	u32 mask = 0;
	object_context->object_size = ZXDH_CQ_SIZE;
	switch (cqc_item->leaf_pbl_size) {
	case PBLE_LEVEL_0:
		mask = GENMASK_ULL(cqc_item->log_cqe_num - 1, 0);
		cqe_num = mask & cqc_item->hw_cq_head;
		if (cqe_num < LAST_15_WQE) {
			object_context->data_length =
				(cqe_num + 1) * ZXDH_CQ_SIZE;
			object_context->src_address =
				(cqc_item->cq_address << 8);
		} else {
			object_context->data_length =
				(LAST_15_WQE + 1) * ZXDH_CQ_SIZE;
			object_context->src_address =
				(cqc_item->cq_address << 8) +
				(cqe_num - LAST_15_WQE) * ZXDH_CQ_SIZE;
		}
		break;
	case PBLE_LEVEL_1:
		cqe_num =
			(u32)FIELD_GET(GENMASK_ULL(5, 0), cqc_item->hw_cq_head);
		object_context->data_length = (cqe_num + 1) * ZXDH_CQ_SIZE;
		object_context->src_address = cqc_item->root_pble;
		break;
	default:
		return -ZXDH_NOT_SUPPORT_TWO_LEVEL_PBLE_CODE;
	}
	return ret;
}

static int
prepare_query_cq_wqe_context(struct zxdh_sc_dev *dev,
			     struct zxdh_get_object_data_req *object_req,
			     struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	struct zxdh_cqc_item cqc_item = { 0 };
	u8 indicate_id = 0;
	if (!validate_cqn(object_context)) {
		pr_err("[zxdh_rdma] query hw object, validate cqp id err:%d", ret);
		return -ZXDH_QUEUE_ID_ERROR;
	}
	ret = query_hw_object_cqc(dev, object_req, &cqc_item);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, query cqc err:%d", ret);
		return ret;
	}
	ret = compute_cq_src_address_and_length(object_context, &cqc_item);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, compute cq src address and length err:%d",
		       ret);
		return ret;
	}
	ret = query_route_id_from_reg(dev, CQ_INDICATE_ID_BASE, 2, 3,
				      &indicate_id);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, query indicate id err:%d", ret);
		return ret;
	}
	object_context->src_path_select = indicate_id;
	return ret;
}

static int query_hw_object_ceqc(struct zxdh_sc_dev *dev,
				struct zxdh_get_object_data_req *object_req,
				struct zxdh_ceqc_item *ceqc_item)
{
	int ret = 0;
	struct zxdh_context_req context_req = { 0 };
	struct zxdh_dma_mem resource_buf = { 0 };
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	struct zxdh_device *iwdev = rf->iwdev;
	context_req.resource_id = object_req->queue_id;
	resource_buf.va = NULL;
	resource_buf.size = ALIGN(ZXDH_READ_CEQC_SIZE, ZXDH_CEQC_ALIGNMENT);
	resource_buf.va = zxdh_alloc_raw_mem(iwdev, resource_buf.size,
					     &resource_buf.pa);
	if (!resource_buf.va) {
		pr_err("[zxdh_rdma] query ceqc, allocate err: ENOMEM\n");
		return -ENOMEM;
	}
	ret = fill_ceqc(rf, &context_req, &resource_buf);
	if (ret) {
		pr_err("[zxdh_rdma] query ceqc, query ceqc err: %d\n", ret);
		goto free_exit;
	}

	ceqc_item->leaf_pbl_size = ZXDH_GET_QPC_ITEM(
		u8, resource_buf.va, ZXDH_CEQC_LEAF_PBL_SIZE_OFFSET,
		RDMACEQC_LEAF_PBL_SIZE);
	ceqc_item->ceqe_head = ZXDH_GET_QPC_ITEM(u32, resource_buf.va,
						 ZXDH_CEQC_LEAF_PBL_SIZE_OFFSET,
						 RDMACEQC_CEQE_HEAD);
	ceqc_item->log_ceq_num = ZXDH_GET_QPC_ITEM(
		u64, resource_buf.va, ZXDH_CEQC_LEAF_PBL_SIZE_OFFSET,
		RDMACEQC_LOG_CEQ_NUM);
	ceqc_item->ceq_address = ZXDH_GET_QPC_ITEM(u64, resource_buf.va,
						   ZXDH_CEQC_CEQ_ADDRESS_OFFSET,
						   RDMACEQC_CEQ_ADDRESS);

	pr_info("[zxdh_rdma] query hw object, ceqc leaf_pbl_size: 0x%x, ceqe_head: 0x%x, ceq_address: 0x%llx",
		ceqc_item->leaf_pbl_size, ceqc_item->ceqe_head,
		ceqc_item->ceq_address);

free_exit:
	if (resource_buf.va) {
		zxdh_free_raw_mem(iwdev, resource_buf.va, resource_buf.size,
				  resource_buf.pa);
		resource_buf.va = NULL;
	}
	return ret;
}

static int
compute_ceq_src_address_and_length(struct hw_object_wqe_context *object_context,
				   struct zxdh_ceqc_item *ceqc_item)
{
	u32 ceqe_num = 0;
	int ret = 0;
	u32 mask = 0;
	object_context->object_size = ZXDH_CEQ_SIZE;
	switch (ceqc_item->leaf_pbl_size) {
	case PBLE_LEVEL_0:
		mask = GENMASK_ULL(ceqc_item->log_ceq_num - 1, 0);
		ceqe_num = mask & ceqc_item->ceqe_head;
		if (ceqe_num < LAST_15_WQE) {
			object_context->data_length =
				(LAST_15_WQE + 1) * ZXDH_CEQ_SIZE;
			object_context->src_address =
				(ceqc_item->ceq_address << 7);
		} else {
			if ((ceqe_num - LAST_15_WQE) % 2 == 1) {
				object_context->data_length =
					15 * ZXDH_CEQ_SIZE;
				object_context->src_address =
					(ceqc_item->ceq_address << 7) +
					(ceqe_num - LAST_15_WQE + 1) *
						ZXDH_CEQ_SIZE;
			} else {
				object_context->data_length =
					(LAST_15_WQE + 1) * ZXDH_CEQ_SIZE;
				object_context->src_address =
					(ceqc_item->ceq_address << 7) +
					(ceqe_num - LAST_15_WQE) *
						ZXDH_CEQ_SIZE;
			}
		}
		break;
	default:
		return -ZXDH_NOT_SUPPORT_VIRTUAL_ADDRESS;
	}
	return ret;
}

static int
prepare_query_ceq_wqe_context(struct zxdh_sc_dev *dev,
			      struct zxdh_get_object_data_req *object_req,
			      struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	struct zxdh_ceqc_item ceqc_item = { 0 };
	u8 indicate_id = 0;

	object_context->zxdh_hmc_rsrc_type = ZXDH_HMC_IW_PBLE;

	ret = query_hw_object_ceqc(dev, object_req, &ceqc_item);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, query indicate id err:%d", ret);
		return ret;
	}
	ret = compute_ceq_src_address_and_length(object_context, &ceqc_item);
	if (ret)
		return ret;
	ret = query_route_id_from_reg(dev, CEQ_INDICATE_ID_BASE, 2, 3,
				      &indicate_id);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, query indicate id err:%d", ret);
		return ret;
	}
	object_context->src_path_select = indicate_id;
	return ret;
}

static int query_hw_object_aeqc(struct zxdh_sc_dev *dev,
				struct zxdh_get_object_data_req *object_req,
				struct zxdh_aeqc_item *aeqc_item)
{
	int ret = 0;
	struct zxdh_context_req context_req = { 0 };
	struct zxdh_dma_mem resource_buf = { 0 };
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	struct zxdh_device *iwdev = rf->iwdev;

	context_req.resource_id = object_req->queue_id;
	resource_buf.va = NULL;
	resource_buf.size = ALIGN(ZXDH_READ_AEQC_SIZE, ZXDH_AEQC_ALIGNMENT);
	resource_buf.va = zxdh_alloc_raw_mem(iwdev, resource_buf.size,
					     &resource_buf.pa);
	if (!resource_buf.va) {
		pr_err("[zxdh_rdma] query cq shadow area, allocate err: ENOMEM\n");
		return -ENOMEM;
	}
	ret = fill_aeqc(rf, &context_req, &resource_buf);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object aeq, query  err: %d\n", ret);
		goto free_exit;
	}

	aeqc_item->aeq_head = ZXDH_GET_QPC_ITEM(u32, resource_buf.va,
						ZXDH_AEQC_AEQ_HEAD_OFFSET,
						ZXDH_AEQC_AEQ_HEAD);
	aeqc_item->leaf_pbl_size = ZXDH_GET_QPC_ITEM(u8, resource_buf.va,
						     ZXDH_AEQC_AEQ_HEAD_OFFSET,
						     ZXDH_AEQC_LEAF_PBL_SIZE);
	aeqc_item->virtually_mapped = ZXDH_GET_QPC_ITEM(
		u8, resource_buf.va, ZXDH_AEQC_AEQ_HEAD_OFFSET,
		ZXDH_AEQC_VIRTUALLY_MAPPED);
	aeqc_item->aeq_size = ZXDH_GET_QPC_ITEM(u32, resource_buf.va,
						ZXDH_AEQC_AEQ_HEAD_OFFSET,
						ZXDH_AEQC_AEQ_SIZE);
	aeqc_item->aeq_address = ZXDH_GET_QPC_ITEM(u64, resource_buf.va,
						   ZXDH_AEQC_AEQ_ADDRESS_OFFSET,
						   ZXDH_AEQC_AEQ_ADDRESS);

	pr_info("[zxdh_rdma] query hw object, aeqc aeq_head: 0x%x, leaf_pbl_size: 0x%x, virtually_mapped: 0x%x, aeq_size: 0x%x,  aeq_address: 0x%llx",
		aeqc_item->aeq_head, aeqc_item->leaf_pbl_size,
		aeqc_item->virtually_mapped, aeqc_item->aeq_size,
		aeqc_item->aeq_address);

free_exit:
	if (resource_buf.va) {
		zxdh_free_raw_mem(iwdev, resource_buf.va, resource_buf.size,
				  resource_buf.pa);
		resource_buf.va = NULL;
	}
	return ret;
}

static int
compute_aeq_src_address_and_length(struct hw_object_wqe_context *object_context,
				   struct zxdh_aeqc_item *aeqc_item)
{
	switch (aeqc_item->virtually_mapped) {
	case 0:
		object_context->data_length =
			(aeqc_item->aeq_head + 1) * ZXDH_AEQ_SIZE;
		object_context->src_address = aeqc_item->aeq_address;
		object_context->object_size = ZXDH_AEQ_SIZE;
		return 0;
	default:
		return -ZXDH_NOT_SUPPORT_VIRTUAL_ADDRESS;
	}
}

static int
prepare_query_aeq_wqe_context(struct zxdh_sc_dev *dev,
			      struct zxdh_get_object_data_req *object_req,
			      struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	struct zxdh_aeqc_item aeqc_item = { 0 };
	u8 indicate_id = 0;
	ret = query_hw_object_aeqc(dev, object_req, &aeqc_item);
	if (ret)
		return ret;
	ret = compute_aeq_src_address_and_length(object_context, &aeqc_item);
	if (ret)
		return ret;
	ret = query_route_id_from_reg(dev, AEQ_INDICATE_ID_BASE, 2, 3,
				      &indicate_id);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, query indicate id err:%d", ret);
		return ret;
	}
	object_context->src_path_select = indicate_id;
	return ret;
}

static int query_hw_object_qpc(struct zxdh_sc_dev *dev, struct zxdh_get_object_data_req *object_req, struct zxdh_qpc_item *qpc_item)
{
	int err_code = 0;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	struct zxdh_device *iwdev = rf->iwdev;
	struct zxdh_dma_mem qpc_buf;
	struct zxdh_context_req context_req = { 0 };
	u64 temp;

	context_req.resource_id = object_req->queue_id;
	qpc_buf.va = NULL;
	qpc_buf.size = ALIGN(ZXDH_QP_CTX_SIZE, ZXDH_QPC_ALIGNMENT);
	qpc_buf.va = zxdh_alloc_raw_mem(iwdev, qpc_buf.size,
									&qpc_buf.pa);
	if (!qpc_buf.va)
	{
		pr_err("[zxdh_rdma] query qpc alloc failed:ENOMEM\n");
		return -ENOMEM;
	}
	err_code = fill_qpc(rf, &context_req, &qpc_buf);
	if (err_code)
	{
		pr_err("[zxdh_rdma] query qpc fill qpc failed:%d\n", err_code);
		goto free_exit;
	}

	get_64bit_val(qpc_buf.va, 64, &temp);
	qpc_item->sq_leaf_pbl_size = FIELD_GET(GENMASK_ULL(5, 4), temp);

	get_64bit_val(qpc_buf.va, 72, &temp);
	qpc_item->sq_address = FIELD_GET(GENMASK_ULL(63, 0), temp);

	get_64bit_val(qpc_buf.va, 144, &temp);
	qpc_item->log_sq_size = FIELD_GET(GENMASK_ULL(59, 56), temp);

	get_64bit_val(qpc_buf.va, 352, &temp);
	qpc_item->rq_address = FIELD_GET(GENMASK_ULL(63, 0), temp);

	get_64bit_val(qpc_buf.va, 360, &temp);
	qpc_item->db_address = FIELD_GET(GENMASK_ULL(63, 0), temp);

	get_64bit_val(qpc_buf.va, 376, &temp);
	qpc_item->rq_leaf_pbl_size = FIELD_GET(GENMASK_ULL(52, 51), temp);
	qpc_item->log_rq_wqe_size = FIELD_GET(GENMASK_ULL(16, 14), temp);
	qpc_item->log_rq_size = FIELD_GET(GENMASK_ULL(10, 7), temp);

free_exit:
	if (qpc_buf.va) {
		zxdh_free_raw_mem(iwdev, qpc_buf.va, qpc_buf.size, qpc_buf.pa);
		qpc_buf.va = NULL;
	}
	return err_code;
}

static int get_rq_size(__u8 log_rq_wqe_size, int *log_rq_wqe_real_size)
{
	switch (log_rq_wqe_size) {
	case 0:
		*log_rq_wqe_real_size = ZXDH_WQE_SIZE_16;
		break;
	case 1:
		*log_rq_wqe_real_size = ZXDH_WQE_SIZE_32;
		break;
	case 2:
		*log_rq_wqe_real_size = ZXDH_WQE_SIZE_64;
		break;
	case 3:
		*log_rq_wqe_real_size = ZXDH_WQE_SIZE_128;
		break;
	case 4:
		*log_rq_wqe_real_size = ZXDH_WQE_SIZE_256;
		break;
	case 5:
		*log_rq_wqe_real_size = ZXDH_WQE_SIZE_512;
		break;
	default:
		return 1;
	}
	return 0;
}

static int get_srq_size(__u8 log_srq_stride, int *log_srq_stride_wqe_real_size)
{
	switch (log_srq_stride) {
	case 1:
		*log_srq_stride_wqe_real_size = ZXDH_WQE_SIZE_32;
		break;
	case 2:
		*log_srq_stride_wqe_real_size = ZXDH_WQE_SIZE_64;
		break;
	case 3:
		*log_srq_stride_wqe_real_size = ZXDH_WQE_SIZE_128;
		break;
	case 4:
		*log_srq_stride_wqe_real_size = ZXDH_WQE_SIZE_256;
		break;
	case 5:
		*log_srq_stride_wqe_real_size = ZXDH_WQE_SIZE_512;
		break;
	default:
		return 1;
	}
	return 0;
}

#define MAX_BUFFER_SIZE (2 * 1024 * 1024)
static int check_object_index_range(int min_index, int max_index, int index)
{
	return (index >= min_index && index <= max_index);
}

static int check_object_buffer_size(uint32_t wqe_size, uint32_t wqe_num)
{
	return (wqe_size * wqe_num >= MAX_BUFFER_SIZE);
}

static void query_32_byte_aligned_address(u64 idx, u64 *aligned_address,
					  u64 *aligned_offset)
{
	*aligned_address = idx & ~0x1F;
	*aligned_offset = idx & 0x1F;
}

static void
hw_object_pble_wqe_init(struct hw_object_wqe_context *object_context,
			struct zxdh_sc_dev *dev, u64 entry_idx,
			u64 *aligned_offset)
{
	u64 pble_aligned_address = 0;
	object_context->op_code = ZXDH_OP_QUERY_HW_OBJECT_INFO;
	object_context->src_vhca_Index = dev->vhca_id;
	object_context->src_object_id = ZXDH_PBLE_QUEUE_OBJ_ID;
	object_context->src_waypartition = 0;

	query_32_byte_aligned_address((entry_idx << 3), &pble_aligned_address,
				      aligned_offset);
	object_context->src_address = pble_aligned_address;
	object_context->src_interface_select =
		object_interface_type[ZXDH_PBLE_QUEUE_OBJ_ID];

	object_context->dest_vhca_index = dev->vhca_id;
	object_context->dest_object_id = ZXDH_DMA_OBJ_ID;
	object_context->dest_waypartition = 0;
	object_context->dest_path_select = ZXDH_INDICATE_HOST_NOSMMU;
	object_context->dest_interface_select = ZXDH_INTERFACE_NOTCACHE;

	object_context->zxdh_hmc_rsrc_type = ZXDH_HMC_IW_PBLE;
	object_context->dev = dev;

	object_context->data_length = ZXDH_PBLE_QUEUE_QUADRUPLE_SIZE;
	object_context->object_size = ZXDH_PBLE_QUEUE_QUADRUPLE_SIZE;
}

static int query_hw_object_pble_queue(struct zxdh_sc_dev *dev, u64 entry_idx,
				      u64 *pble_src_addr)
{
	int ret = 0;
	u64 aligned_offset = 0;
	u8 route_id = 0;
	struct hw_object_wqe_context object_context = { 0 };
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	struct zxdh_device *iwdev = rf->iwdev;
	struct zxdh_dma_mem qpc_buf;
	u64 temp;

	hw_object_pble_wqe_init(&object_context, dev, entry_idx,
				&aligned_offset);
	ret = query_route_id_from_reg(dev, PBLE_QUEUE_CACHE_ID_BASE, 0, 1,
				      &route_id);
	if (ret) {
		pr_err("[zxdh_rdma] query route id failed, invalid reg value\n");
		return -EINVAL;
	}
	object_context.src_path_select = route_id;

	qpc_buf.va = NULL;
	qpc_buf.size = object_context.data_length;
	qpc_buf.va = zxdh_alloc_raw_mem(iwdev, qpc_buf.size,
					&qpc_buf.pa);
	if (!qpc_buf.va) {
		pr_err("[zxdh_rdma] query pble queue alloc failed:ENOMEM\n");
		return -ENOMEM;
	}
	object_context.dest_address = qpc_buf.pa;

	ret = hw_object_query_info(rf, &object_context);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, query object info err:%d\n", ret);
		goto free_exit;
	}

	get_64bit_val(qpc_buf.va, aligned_offset, &temp);
	*pble_src_addr = FIELD_GET(GENMASK_ULL(63, 0), temp);

free_exit:
	if (qpc_buf.va) {
		zxdh_free_raw_mem(iwdev, qpc_buf.va, qpc_buf.size, qpc_buf.pa);
		qpc_buf.va = NULL;
	}
	return ret;
}

static __u64 physical_addressing(__u64 qp_address, int qp_wqe_size,
				 int wqe_index)
{
	return (qp_address & ZXDH_MAX_STATS_64) + qp_wqe_size * wqe_index;
}

static __u64 single_level_virtual_addressing(struct zxdh_sc_dev *dev,
					     __u64 qp_address, int qp_wqe_size,
					     int wqe_index)
{
	__u32 pble_index = (qp_address & ZXDH_MAX_STATS_28) +
			   (qp_wqe_size * wqe_index) / 4096;
	__u64 pble_addr;
	int ret = query_hw_object_pble_queue(dev, pble_index, &pble_addr);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, query pble queue address for qp info err:%d\n",
		       ret);
		return -EINVAL;
	}
	return (pble_addr & ZXDH_MAX_STATS_64) +
	       ((qp_wqe_size * wqe_index) & ZXDH_MAX_STATS_12);
}

static int calculate_qp_src_address(struct zxdh_sc_dev *dev,
				    struct zxdh_qp_addr_context qp_addr_ctx,
				    u64 *src_addr)
{
	switch (qp_addr_ctx.addr_mode) {
	case 0:
		*src_addr = physical_addressing(qp_addr_ctx.qp_base_addr,
						qp_addr_ctx.wqe_size,
						qp_addr_ctx.wqe_index);
		break;
	case 1:
		*src_addr = single_level_virtual_addressing(
			dev, qp_addr_ctx.qp_base_addr, qp_addr_ctx.wqe_size,
			qp_addr_ctx.wqe_index);
		break;
	default:
		pr_err("[zxdh_rdma] query address mode is invalid %d\n",
		       qp_addr_ctx.addr_mode);
		return 1;
	}
	return 0;
}

static int
query_hw_object_qpc_for_rq(struct zxdh_sc_dev *dev,
			   struct zxdh_get_object_data_req *object_req,
			   struct zxdh_qp_addr_context *qp_addr_ctx)
{
	int err_code = 0;
	struct zxdh_qpc_item qpc_item = { 0 };
	err_code = query_hw_object_qpc(dev, object_req, &qpc_item);
	if (err_code) {
		pr_err("[zxdh_rdma] query qpc failed:%u\n", err_code);
		return err_code;
	}

	if (get_rq_size(qpc_item.log_rq_wqe_size,
			&qpc_item.log_rq_wqe_real_size)) {
		pr_err("[zxdh_rdma] query qpc, invalid log_rq_wqe_size:%d\n",
		       qpc_item.log_rq_wqe_size);
		return -EINVAL;
	}

	if (!check_object_index_range(0, (1U << qpc_item.log_rq_size) - 1,
				      object_req->entry_idx)) {
		pr_err("[zxdh_rdma] query entry idx is out of index.entry_idx:%u\n",
		       object_req->entry_idx);
		return -ZXDH_ENTRY_IDX_ERROR;
	}
	if (check_object_buffer_size(qpc_item.log_rq_wqe_real_size,
				     object_req->object_num)) {
		pr_err("[zxdh_rdma] query buffer size is more than 2M.\n");
		return -ZXDH_DMA_MEMORY_OVER_2M;
	}

	qp_addr_ctx->wqe_size = qpc_item.log_rq_wqe_real_size;
	qp_addr_ctx->wqe_index = object_req->entry_idx;
	qp_addr_ctx->addr_mode = qpc_item.rq_leaf_pbl_size;
	qp_addr_ctx->qp_base_addr = qpc_item.rq_address;

	return err_code;
}

static int
prepare_query_rq_wqe_context(struct zxdh_sc_dev *dev,
			     struct zxdh_get_object_data_req *object_req,
			     struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	struct zxdh_qp_addr_context qp_addr_ctx = { 0 };
	u64 src_addr = 0;
	u8 route_id = 0;
	ret = query_hw_object_qpc_for_rq(dev, object_req, &qp_addr_ctx);
	if (ret)
		return ret;
	if (qp_addr_ctx.addr_mode != 0 && object_req->object_num != 1) {
		pr_err("[zxdh_rdma] query qpc, When the addressing mode is virtual addressing, object_num can only be 1.object_num:%d\n",
		       object_req->object_num);
		return -ZXDH_PBLE_ADDRESSING_ONLY_SUPPORTS_OBJECT_NUMBER_1;
	}
	if (calculate_qp_src_address(dev, qp_addr_ctx, &src_addr)) {
		pr_err("[zxdh_rdma] query qpc, invalid addressing mode\n");
		return -EINVAL;
	}
	ret = query_route_id_from_reg(dev, C_RQ_INDICATE_ID_REG_CHECK, 2, 3,
				      &route_id);
	if (ret) {
		pr_err("[zxdh_rdma] query route id failed, invalid reg value\n");
		return -EINVAL;
	}
	object_context->src_address = src_addr;
	object_context->src_path_select = route_id;
	object_context->src_interface_select = ZXDH_INTERFACE_NOTCACHE;
	object_context->data_length =
		qp_addr_ctx.wqe_size * object_req->object_num;
	object_context->object_size = qp_addr_ctx.wqe_size;
	object_context->zxdh_hmc_rsrc_type = ZXDH_HMC_IW_PBLE;
	return ret;
}

static int
query_hw_object_qpc_for_rq_db(struct zxdh_sc_dev *dev,
			      struct zxdh_get_object_data_req *object_req,
			      struct hw_object_wqe_context *object_context)
{
	int err_code = 0;
	struct zxdh_qpc_item qpc_item = { 0 };
	err_code = query_hw_object_qpc(dev, object_req, &qpc_item);
	if (err_code) {
		pr_err("[zxdh_rdma] query qpc failed:%u\n", err_code);
		return err_code;
	}
	object_context->src_address = qpc_item.db_address;
	return err_code;
}

static int prepare_query_rq_doorbell_wqe_context(
	struct zxdh_sc_dev *dev, struct zxdh_get_object_data_req *object_req,
	struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	u8 route_id = 0;
	ret = query_hw_object_qpc_for_rq_db(dev, object_req, object_context);
	if (ret)
		return ret;

	ret = query_route_id_from_reg(dev, C_RQDB_INDICATE_ID_REG_CHECK, 2, 3,
				      &route_id);
	if (ret) {
		pr_err("[zxdh_rdma] query route id failed, invalid reg value\n");
		return -EINVAL;
	}
	object_context->data_length = ZXDH_RQ_SHADOW_AREA_SIZE;
	object_context->src_path_select = route_id;
	object_context->src_interface_select = ZXDH_INTERFACE_NOTCACHE;
	return ret;
}

static int query_hw_object_srqc(struct zxdh_sc_dev *dev,
				struct zxdh_get_object_data_req *object_req,
				struct zxdh_srqc_item *srqc_item)
{
	int err_code = 0;
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	struct zxdh_device *iwdev = rf->iwdev;
	struct zxdh_dma_mem srqc_buf;
	struct zxdh_context_req context_req = { 0 };
	u64 temp;

	context_req.resource_id = object_req->queue_id;
	srqc_buf.va = NULL;
	srqc_buf.size = ALIGN(ZXDH_SRQ_CTX_SIZE, ZXDH_SRQC_ALIGNMENT);
	srqc_buf.va = zxdh_alloc_raw_mem(iwdev, srqc_buf.size,
					 &srqc_buf.pa);
	if (!srqc_buf.va) {
		pr_err("[zxdh_rdma] query srqc alloc failed:ENOMEM\n");
		return -ENOMEM;
	}
	err_code = fill_srqc(rf, &context_req, &srqc_buf);
	if (err_code) {
		pr_err("[zxdh_rdma] query srqc fill srqc failed:%d\n", err_code);
		goto free_exit;
	}

	get_64bit_val(srqc_buf.va, 0, &temp);
	srqc_item->leaf_pbl_size = FIELD_GET(GENMASK_ULL(61, 60), temp);
	srqc_item->log_srq_size = FIELD_GET(GENMASK_ULL(59, 56), temp);
	srqc_item->log_srq_stride = FIELD_GET(GENMASK_ULL(26, 24), temp);
	srqc_item->list_leaf_pbl_size = FIELD_GET(GENMASK_ULL(23, 22), temp);

	get_64bit_val(srqc_buf.va, 8, &temp);
	srqc_item->srq_address = FIELD_GET(GENMASK_ULL(63, 0), temp);

	get_64bit_val(srqc_buf.va, 16, &temp);
	srqc_item->srq_list_address = FIELD_GET(GENMASK_ULL(63, 0), temp);

	get_64bit_val(srqc_buf.va, 24, &temp);
	srqc_item->dbr_address = FIELD_GET(GENMASK_ULL(63, 0), temp);

	get_64bit_val(srqc_buf.va, 32, &temp);
	srqc_item->hw_wqe_cnt = FIELD_GET(GENMASK_ULL(15, 0), temp);

free_exit:
	if (srqc_buf.va) {
		zxdh_free_raw_mem(iwdev, srqc_buf.va, srqc_buf.size,
				  srqc_buf.pa);
		srqc_buf.va = NULL;
	}
	return err_code;
}

static int
query_hw_object_srqc_for_srqp(struct zxdh_sc_dev *dev,
			      struct zxdh_get_object_data_req *object_req,
			      struct zxdh_qp_addr_context *qp_addr_ctx)
{
	int err_code = 0;
	struct zxdh_srqc_item srqc_item = { 0 };
	err_code = query_hw_object_srqc(dev, object_req, &srqc_item);
	if (err_code)
		return err_code;
	qp_addr_ctx->addr_mode = srqc_item.list_leaf_pbl_size == 0 ? ZXDH_ADDR_TYPE_ZERO_BASED : ZXDH_ADDR_TYPE_VA_BASED;
	qp_addr_ctx->wqe_size = ZXDH_SRQP_INDEX_SIZE;
	qp_addr_ctx->wqe_index = object_req->entry_idx;
	qp_addr_ctx->qp_base_addr = srqc_item.srq_list_address;

	if (check_object_buffer_size(qp_addr_ctx->wqe_size,
				     object_req->object_num)) {
		pr_err("[zxdh_rdma] query buffer size is more than 2M.\n");
		return -ZXDH_DMA_MEMORY_OVER_2M;
	}

	return err_code;
}

static int
prepare_query_srqp_wqe_context(struct zxdh_sc_dev *dev,
			       struct zxdh_get_object_data_req *object_req,
			       struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	struct zxdh_qp_addr_context qp_addr_ctx = { 0 };
	u64 src_addr = 0, aligned_offset = 0, phy_src_addr = 0;
	u8 route_id = 0;
	ret = query_hw_object_srqc_for_srqp(dev, object_req, &qp_addr_ctx);
	if (ret)
		return ret;

	if (calculate_qp_src_address(dev, qp_addr_ctx, &phy_src_addr)) {
		pr_err("[zxdh_rdma] query qpc, invalid addressing mode\n");
		return -EINVAL;
	}
	query_32_byte_aligned_address(phy_src_addr, &src_addr, &aligned_offset);
	ret = query_route_id_from_reg(dev, C_SRQP_INDICATE_ID_REG_CHECK, 2, 3,
				      &route_id);
	if (ret) {
		pr_err("[zxdh_rdma] query route id failed, invalid reg value\n");
		return -EINVAL;
	}
	object_context->data_length = ZXDH_PBLE_QUEUE_QUADRUPLE_SIZE;
	object_context->src_path_select = route_id;
	object_context->src_interface_select = ZXDH_INTERFACE_NOTCACHE;
	object_context->src_address = src_addr;
	object_context->zxdh_hmc_rsrc_type = ZXDH_HMC_IW_PBLE;
	object_context->srqp_aligned_offset = aligned_offset;
	return ret;
}

static int
query_hw_object_srqc_for_srq(struct zxdh_sc_dev *dev,
			     struct zxdh_get_object_data_req *object_req,
			     struct zxdh_qp_addr_context *qp_addr_ctx)
{
	int err_code = 0;
	struct zxdh_srqc_item srqc_item = { 0 };
	err_code = query_hw_object_srqc(dev, object_req, &srqc_item);
	if (err_code)
		return err_code;
	if (get_srq_size(srqc_item.log_srq_stride,
			 &srqc_item.log_srq_stride_wqe_real_size)) {
		pr_err("[zxdh_rdma] query srqc, invalid log_rq_wqe_size:%d\n",
		       srqc_item.log_srq_stride);
		return -EINVAL;
	}

	if (srqc_item.leaf_pbl_size != 0 && object_req->object_num != 1) {
		pr_err("[zxdh_rdma] query qpc, When the addressing mode is virtual addressing, object_num can only be 1.object_num:%d\n",
		       object_req->object_num);
		return -ZXDH_PBLE_ADDRESSING_ONLY_SUPPORTS_OBJECT_NUMBER_1;
	}

	if (!check_object_index_range(0, (1U << srqc_item.log_srq_size) - 1,
				      object_req->entry_idx)) {
		pr_err("[zxdh_rdma] query entry idx is out of index.entry_idx:%u\n",
		       object_req->entry_idx);
		return -ZXDH_ENTRY_IDX_ERROR;
	}
	if (check_object_buffer_size(srqc_item.log_srq_stride_wqe_real_size,
				     object_req->object_num)) {
		pr_err("[zxdh_rdma] query buffer size is more than 2M.\n");
		return -ZXDH_DMA_MEMORY_OVER_2M;
	}
	qp_addr_ctx->addr_mode = srqc_item.leaf_pbl_size;
	qp_addr_ctx->wqe_size = srqc_item.log_srq_stride_wqe_real_size;
	qp_addr_ctx->wqe_index = object_req->entry_idx;
	qp_addr_ctx->qp_base_addr = srqc_item.srq_address;
	return err_code;
}

static int
prepare_query_srq_wqe_context(struct zxdh_sc_dev *dev,
			      struct zxdh_get_object_data_req *object_req,
			      struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	struct zxdh_qp_addr_context qp_addr_ctx = { 0 };
	u64 src_addr = 0;
	u8 route_id = 0;
	ret = query_hw_object_srqc_for_srq(dev, object_req, &qp_addr_ctx);
	if (ret)
		return ret;
	if (calculate_qp_src_address(dev, qp_addr_ctx, &src_addr)) {
		pr_err("[zxdh_rdma] query srqc, invalid addressing mode\n");
		return -EINVAL;
	}

	ret = query_route_id_from_reg(dev, C_SRQ_INDICATE_ID_REG_CHECK, 2, 3,
				      &route_id);
	if (ret) {
		pr_err("[zxdh_rdma] query route id failed, invalid reg value\n");
		return -EINVAL;
	}
	object_context->src_address = src_addr;
	object_context->src_path_select = route_id;

	object_context->src_interface_select = ZXDH_INTERFACE_NOTCACHE;
	object_context->data_length =
		qp_addr_ctx.wqe_size * object_req->object_num;
	object_context->object_size = qp_addr_ctx.wqe_size;
	object_context->zxdh_hmc_rsrc_type = ZXDH_HMC_IW_PBLE;
	return ret;
}

static int
query_hw_object_srqc_for_srq_db(struct zxdh_sc_dev *dev,
				struct zxdh_get_object_data_req *object_req,
				struct hw_object_wqe_context *object_context)
{
	int err_code = 0;
	struct zxdh_srqc_item srqc_item = { 0 };
	err_code = query_hw_object_srqc(dev, object_req, &srqc_item);
	if (err_code)
		return err_code;
	object_context->src_address = srqc_item.dbr_address;
	return err_code;
}

static int prepare_query_srq_doorbell_wqe_context(
	struct zxdh_sc_dev *dev, struct zxdh_get_object_data_req *object_req,
	struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	u8 route_id = 0;
	ret = query_hw_object_srqc_for_srq_db(dev, object_req, object_context);
	if (ret)
		return ret;

	ret = query_route_id_from_reg(dev, C_SRQDB_INDICATE_ID_REG_CHECK, 2, 3,
				      &route_id);
	if (ret) {
		pr_err("[zxdh_rdma] query route id failed, invalid reg value\n");
		return -EINVAL;
	}
	object_context->data_length = ZXDH_SRQ_SHADOW_AREA_SIZE;
	object_context->src_path_select = route_id;
	object_context->src_interface_select = ZXDH_INTERFACE_NOTCACHE;
	return ret;
}

static int
query_hw_object_qpc_for_sq(struct zxdh_sc_dev *dev,
			   struct zxdh_get_object_data_req *object_req,
			   struct zxdh_qp_addr_context *qp_addr_ctx)
{
	int err_code = 0;
	struct zxdh_qpc_item qpc_item = { 0 };
	err_code = query_hw_object_qpc(dev, object_req, &qpc_item);
	if (err_code) {
		pr_err("[zxdh_rdma] query qpc failed:%u\n", err_code);
		return err_code;
	}

	if (qpc_item.sq_leaf_pbl_size != 0 && object_req->object_num != 1) {
		pr_err("[zxdh_rdma] query qpc, When the addressing mode is virtual addressing, object_num can only be 1.object_num:%d\n",
		       object_req->object_num);
		return -ZXDH_PBLE_ADDRESSING_ONLY_SUPPORTS_OBJECT_NUMBER_1;
	}

	if (!check_object_index_range(0, (1U << qpc_item.log_sq_size) - 1,
				      object_req->entry_idx)) {
		pr_err("[zxdh_rdma] query entry idx is out of index.entry_idx:%u\n",
		       object_req->entry_idx);
		return -ZXDH_ENTRY_IDX_ERROR;
	}
	
	if (check_object_buffer_size(ZXDH_SQ_UNIT_SIZE, object_req->object_num)) {
		pr_err("[zxdh_rdma] query buffer size is more than 2M.\n");
		return -ZXDH_DMA_MEMORY_OVER_2M;
	}
	qp_addr_ctx->wqe_size = ZXDH_SQ_UNIT_SIZE;
	qp_addr_ctx->wqe_index = object_req->entry_idx;
	qp_addr_ctx->addr_mode = qpc_item.sq_leaf_pbl_size;
	qp_addr_ctx->qp_base_addr = qpc_item.sq_address;

	return err_code;
}

static int
prepare_query_sq_wqe_context(struct zxdh_sc_dev *dev,
			     struct zxdh_get_object_data_req *object_req,
			     struct hw_object_wqe_context *object_context)
{
	int ret = 0;
	struct zxdh_qp_addr_context qp_addr_ctx = { 0 };
	u64 src_addr = 0;
	u8 route_id = 0;
	ret = query_hw_object_qpc_for_sq(dev, object_req, &qp_addr_ctx);
	if (ret)
		return ret;
	if (calculate_qp_src_address(dev, qp_addr_ctx, &src_addr)) {
		pr_err("[zxdh_rdma] query qpc, invalid addressing mode\n");
		return -EINVAL;
	}

	ret = query_route_id_from_reg(dev, C_SQ_INDICATE_ID_REG_CHECK, 2, 3,
				      &route_id);
	if (ret) {
		pr_err("[zxdh_rdma] query route id failed, invalid reg value\n");
		return -EINVAL;
	}

	object_context->src_address = src_addr;
	object_context->src_path_select = route_id;
	object_context->src_interface_select = ZXDH_INTERFACE_NOTCACHE;
	object_context->data_length =
		qp_addr_ctx.wqe_size * object_req->object_num;
	object_context->zxdh_hmc_rsrc_type = ZXDH_HMC_IW_PBLE;
	return ret;
}

static int
hw_object_calculate_wqe_context(struct zxdh_sc_dev *dev,
				struct zxdh_get_object_data_req *object_req,
				struct hw_object_wqe_context *object_context)
{
	switch (object_req->object_id) {
	case ZXDH_PBLE_MR_OBJ_ID:
		return prepare_query_pble_mr_wqe_context(dev, object_req,
							 object_context);
	case ZXDH_PBLE_QUEUE_OBJ_ID:
		return prepare_query_pble_queue_wqe_context(dev, object_req,
							    object_context);
	case ZXDH_AH_OBJ_ID:
		return prepare_query_ah_wqe_context(dev, object_req,
						    object_context);
	case ZXDH_IRD_OBJ_ID:
		return prepare_query_ird_wqe_context(dev, object_req,
						     object_context);
	case ZXDH_TX_WINDOW_OBJ_ID:
		return prepare_query_tx_window_context(dev, object_req,
						       object_context);
	case ZXDH_CQ_SHADOW_AREA:
		return prepare_query_cq_doorbell_wqe_context(dev, object_req,
							     object_context);
	case ZXDH_CQ:
		return prepare_query_cq_wqe_context(dev, object_req,
						    object_context);
	case ZXDH_CEQ:
		return prepare_query_ceq_wqe_context(dev, object_req,
						     object_context);
	case ZXDH_AEQ:
		return prepare_query_aeq_wqe_context(dev, object_req,
						     object_context);
    case ZXDH_RQ:
		return prepare_query_rq_wqe_context(dev, object_req, object_context);
	case ZXDH_RQ_SHADOW_AREA:
		return prepare_query_rq_doorbell_wqe_context(dev, object_req, object_context);
	case ZXDH_SRQP:
		return prepare_query_srqp_wqe_context(dev, object_req, object_context);
	case ZXDH_SRQ:
		return prepare_query_srq_wqe_context(dev, object_req, object_context);
	case ZXDH_SRQ_SHADOW_AREA:
		return prepare_query_srq_doorbell_wqe_context(dev, object_req, object_context);
	case ZXDH_SQ:
		return prepare_query_sq_wqe_context(dev, object_req, object_context);
	default:
		return -ZXDH_NOT_SUPPORT_OBJECT_ID;
	}
}

static bool validate_address_32_byte_align(u64 address)
{
	return !(address & 0x1F);
}

static bool
validate_hw_object_cache_id(struct hw_object_wqe_context *hw_object_wqe_ctx)
{
	int use_cache = 0;
	use_cache = !object_interface_type[hw_object_wqe_ctx->src_object_id];
	if (!use_cache) {
		return true;
	}
	switch (hw_object_wqe_ctx->src_object_id) {
	case ZXDH_IRD_OBJ_ID:
		return hw_object_wqe_ctx->src_path_select == 2;
	case ZXDH_TX_WINDOW_OBJ_ID:
		return hw_object_wqe_ctx->src_path_select == 3;
	default:
		return hw_object_wqe_ctx->src_path_select == 1;
	}
}

static bool
validate_allocate_buffer_size(struct hw_object_wqe_context *hw_object_wqe_ctx)
{
	return hw_object_wqe_ctx->data_length <= ZXDH_CAP_DATA_HOST_MEM_SIZE;
}

static bool
validate_require_data_length(struct hw_object_wqe_context *hw_object_wqe_ctx)
{
	u32 max_cnt = 0;
	u64 size = 0;
	u64 relative_address = 0;
	if (hw_object_wqe_ctx->zxdh_hmc_rsrc_type == ZXDH_HMC_IW_MAX) {
		return true;
	}
	max_cnt = hw_object_wqe_ctx->dev->hmc_info
			  ->hmc_obj[hw_object_wqe_ctx->zxdh_hmc_rsrc_type]
			  .max_cnt;
	size = hw_object_wqe_ctx->dev->hmc_info
		       ->hmc_obj[hw_object_wqe_ctx->zxdh_hmc_rsrc_type]
		       .size;
	relative_address = hw_object_wqe_ctx->req->entry_idx *
			   (u64)hw_object_wqe_ctx->object_size;
	return max_cnt * size >=
	       relative_address + hw_object_wqe_ctx->data_length;
}

static int post_validate_hw_object_request(
	struct hw_object_wqe_context *hw_object_wqe_ctx,
	struct zxdh_get_object_data_req *object_data_req)
{
	int ret = 0;
	if (!validate_address_32_byte_align(hw_object_wqe_ctx->src_address)) {
		pr_err("[zxdh_rdma] query hw object, dma read src address not 32 bit aligned, 0x%llx\n",
		       hw_object_wqe_ctx->src_address);
		return -ZXDH_DMA_READ_NOT_32_ALIGN;
	}

	if (!validate_hw_object_cache_id(hw_object_wqe_ctx)) {
		pr_err("[zxdh_rdma] query hw object, validate cache id err: invalid cache id %d \n",
		       hw_object_wqe_ctx->src_path_select);
		return -ZXDH_CACHE_ID_CHECK_ERROR;
	}

	if (!validate_allocate_buffer_size(hw_object_wqe_ctx)) {
		pr_err("[zxdh_rdma] query hw object, validate buffer: buffer size more than 2M 0x%u\n",
		       hw_object_wqe_ctx->data_length);
		return -ZXDH_DMA_MEMORY_OVER_2M;
	}
	if (!validate_require_data_length(hw_object_wqe_ctx)) {
		pr_err("[zxdh_rdma] query hw object, validate data_length: invalid entry_idx %d and object_num %d \n",
		       object_data_req->entry_idx, object_data_req->object_num);
		return -ZXDH_DATA_ENTRY_IDX_OVER_LIMIT;
	}
	return ret;
};

static int hw_object_query_info(struct zxdh_pci_f *rf,
				struct hw_object_wqe_context *object_wqe_ctx)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_sc_dev *dev;
	int ret = 0;
	dev = &rf->sc_dev;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request) {
		pr_err("[zxdh_rdma] query hw object, get cqp resquest err");
		return -ENOMEM;
	}

	cqp_info = &cqp_request->info;
	cqp_info->post_sq = 1;
	cqp_info->cqp_cmd = object_wqe_ctx->op_code;
	cqp_info->in.u.dma_writeread.cqp = dev->cqp;
	pr_info("[zxdh_rdma] dma read hw object, opcode %d:\n", cqp_info->cqp_cmd);

	cqp_info->in.u.dma_writeread.src_dest.src = object_wqe_ctx->src_address;
	cqp_info->in.u.dma_writeread.src_dest.len = object_wqe_ctx->data_length;
	cqp_info->in.u.dma_writeread.src_dest.dest =
		object_wqe_ctx->dest_address;
	pr_info("[zxdh_rdma] dma read hw object, src addr: %llu, dest addr: %llu, len: %u\n",
		cqp_info->in.u.dma_writeread.src_dest.src,
		cqp_info->in.u.dma_writeread.src_dest.dest,
		cqp_info->in.u.dma_writeread.src_dest.len);

	cqp_info->in.u.dma_writeread.src_path_index.vhca_id =
		object_wqe_ctx->src_vhca_Index;
	cqp_info->in.u.dma_writeread.src_path_index.obj_id =
		object_wqe_ctx->src_object_id;
	cqp_info->in.u.dma_writeread.src_path_index.waypartion =
		object_wqe_ctx->src_waypartition;
	cqp_info->in.u.dma_writeread.src_path_index.path_select =
		object_wqe_ctx->src_path_select;
	cqp_info->in.u.dma_writeread.src_path_index.inter_select =
		object_wqe_ctx->src_interface_select;
	pr_info("[zxdh_rdma] dma read hw object, src path index: vhca_id: %u, object id: %d, waypartion %d, path select %d, interface select %d\n",
		cqp_info->in.u.dma_writeread.src_path_index.vhca_id,
		cqp_info->in.u.dma_writeread.src_path_index.obj_id,
		cqp_info->in.u.dma_writeread.src_path_index.waypartion,
		cqp_info->in.u.dma_writeread.src_path_index.path_select,
		cqp_info->in.u.dma_writeread.src_path_index.inter_select);

	cqp_info->in.u.dma_writeread.dest_path_index.vhca_id =
		object_wqe_ctx->dest_vhca_index;
	cqp_info->in.u.dma_writeread.dest_path_index.obj_id =
		object_wqe_ctx->dest_object_id;
	cqp_info->in.u.dma_writeread.dest_path_index.waypartion =
		object_wqe_ctx->dest_waypartition;
	cqp_info->in.u.dma_writeread.dest_path_index.path_select =
		object_wqe_ctx->dest_path_select;
	cqp_info->in.u.dma_writeread.dest_path_index.inter_select =
		object_wqe_ctx->dest_interface_select;
	pr_info("[zxdh_rdma] dma read hw object, dest path index: vhca_id: %u, object id: %d, waypartion %d, path select %d, interface select %d\n",
		cqp_info->in.u.dma_writeread.dest_path_index.vhca_id,
		cqp_info->in.u.dma_writeread.dest_path_index.obj_id,
		cqp_info->in.u.dma_writeread.dest_path_index.waypartion,
		cqp_info->in.u.dma_writeread.dest_path_index.path_select,
		cqp_info->in.u.dma_writeread.dest_path_index.inter_select);

	cqp_info->in.u.dma_writeread.scratch = (uintptr_t)cqp_request;
	ret = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, handle query hw object cpq request err:%d\n",
		       ret);
		return ret;
	}
	return ret;
}

static int hw_object_fill_object_data_resp(
	struct zxdh_get_object_data_resp *object_data_resp,
	struct hw_object_wqe_context *object_wqe_ctx)
{
	object_data_resp->length = object_wqe_ctx->data_length;
	object_data_resp->vhca_id = object_wqe_ctx->src_vhca_Index;
	object_data_resp->route_id = object_wqe_ctx->src_path_select;
	object_data_resp->object_size = object_wqe_ctx->object_size;
	object_data_resp->srqp_aligned_offset = object_wqe_ctx->srqp_aligned_offset;
	object_data_resp->queue_id = object_wqe_ctx->req->queue_id;
	pr_info("[zxdh_rdma] query hw object response, object_mmap_offset: %llu, length %u, vhca_id: %u, route_id: %d, wqe_size: %u, srqp_aligned_offset: %llu\n",
		object_data_resp->object_mmap_offset, object_data_resp->length,
		object_data_resp->vhca_id, object_data_resp->route_id,
		object_data_resp->object_size, object_wqe_ctx->srqp_aligned_offset);
	return 0;
}

#ifdef ZXDH_UAPI_DEF
static int UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_GET_OBJ_DATA)(
	struct uverbs_attr_bundle *attrs)
#else
static int UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_GET_OBJ_DATA)(
	struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	int ret = 0;
	struct ib_ucontext *ib_uctx;
	struct zxdh_ucontext *ucontext;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct zxdh_pci_f *rf;
	struct zxdh_get_object_data_req object_data_req = { 0 };
	struct zxdh_get_object_data_resp object_data_resp;
	struct hw_object_wqe_context object_wqe_ctx = { 0 };
	struct zxdh_sc_dev *dev;
#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif

	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);

	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);
	rf = iwdev->rf;
	dev = &rf->sc_dev;
	ucontext = to_ucontext(ib_uctx);

	if (ucontext->hw_data_cap.hw_object_mmap.addr_info.cap_direct_dma_addr
			    .cap_cpu_addr != NULL ||
	    ucontext->hw_data_cap.hw_object_mmap.entry_info.cap_mmap_entry !=
		    NULL) {
		pr_err("[zxdh_rdma] query hw object, cpu addr or object mmap entry already exist\n");
		return -ENOMEM;
	}

	ret = uverbs_copy_from(&object_data_req, attrs,
			       ZXDH_IB_ATTR_DEV_GET_OBJ_DATA);
	if (IS_UVERBS_COPY_ERR(ret)) {
		pr_err("[zxdh_rdma] query hw object, uverbs copy err: %d\n", ret);
		return ret;
	}

    ret = pre_validate_hw_object_request(&object_data_req);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, pre validate hw object request err: %d\n",
		       ret);
		return ret;
	}

	if (rf->use_ext_mem_flag &&
		(object_data_req.object_id == ZXDH_PBLE_MR_OBJ_ID ||
		 object_data_req.object_id == ZXDH_PBLE_QUEUE_OBJ_ID ||
		 object_data_req.object_id == ZXDH_SRQP ||
		 object_data_req.object_id == ZXDH_SRQ ||
		 object_data_req.object_id == ZXDH_SRQ_SHADOW_AREA)) {
		pr_err("[zxdh_rdma] object id %d not supported on current device!\n", object_data_req.object_id);
		return -ZXDH_NOT_SUPPORT_CURRENT_DEVICE;
    }

    hw_object_wqe_init(&object_wqe_ctx, dev, &object_data_req);

    /* SQC 模式：读取整个SQ（按Log_SQ_Size）。物理寻址一次DMA，PBLE逐WQE DMA。*/
    if (object_data_req.object_id == ZXDH_SQC_OBJ_ID) {
        struct zxdh_qpc_item qpc_item = { 0 };
        u32 total_wqes = 0;
        u8 route_id = 0;
        u64 dest_base = 0;
        u32 total_bytes = 0;
		u32 j, pages, base_index = 0;
        int err = 0;

        err = query_hw_object_qpc(dev, &object_data_req, &qpc_item);
        if (err) {
            pr_err("[zxdh_rdma] SQC: query qpc failed:%d\n", err);
            return err;
        }
        total_wqes = 1U << qpc_item.log_sq_size;
        total_bytes = total_wqes * ZXDH_SQ_UNIT_SIZE;

        if (allocate_addr_for_mmap(iwdev, ucontext, total_bytes,
                       &ucontext->hw_data_cap.hw_object_mmap,
                       &object_data_resp.object_mmap_offset)) {
            pr_err("[zxdh_rdma] SQC: obj_mmap_entry insert err! buf_size:%u\n", total_bytes);
            goto free_exit;
        }
        ucontext->hw_data_cap.object_buffer_size = total_bytes;
        /* 根据 use_ext_mem_flag 选择 DMA 目标地址基址 */
        if (rf->use_ext_mem_flag) {
                dest_base = ucontext->hw_data_cap.hw_object_mmap.cap_ext_mem_addr.cap_dma_addr;
        } else {
                dest_base = ucontext->hw_data_cap.hw_object_mmap.addr_info.cap_direct_dma_addr.cap_dma_addr;
        }

        err = query_route_id_from_reg(dev, C_SQ_INDICATE_ID_REG_CHECK, 2, 3, &route_id);
        if (err) {
            pr_err("[zxdh_rdma] SQC: query indicate id err:%d\n", err);
            goto free_exit;
        }

        if (qpc_item.sq_leaf_pbl_size == 0) {
            /* 物理寻址：一次性搬运整个SQ */
            object_wqe_ctx.src_address = (qpc_item.sq_address & ZXDH_MAX_STATS_64);
            object_wqe_ctx.src_path_select = route_id;
            object_wqe_ctx.src_interface_select = ZXDH_INTERFACE_NOTCACHE;
            object_wqe_ctx.dest_address = dest_base;
            object_wqe_ctx.data_length = total_bytes;
            err = hw_object_query_info(rf, &object_wqe_ctx);
            if (err) {
                pr_err("[zxdh_rdma] SQC: dma read whole SQ failed:%d\n", err);
                goto free_exit;
            }
        } else {
            /* PBLE：按4K页为单位批量DMA，提高效率 */
            const u32 page_size = 4096;
            if (total_bytes % page_size != 0) {
                pr_err("[zxdh_rdma] SQC: total_bytes(0x%x) is not 4K aligned for PBLE paging.\n", total_bytes);
                err = -EINVAL;
                goto free_exit;
            }

            pages = total_bytes / page_size;
            base_index = (u32)(qpc_item.sq_address & ZXDH_MAX_STATS_28);
            for (j = 0; j < pages; j++) {
                u64 leaf_phy_base = 0;
                /* 计算 Leaf PBLE Index 并读取叶页物理基址 */
                u32 leaf_index = base_index + j;
                err = query_hw_object_pble_queue(dev, leaf_index, &leaf_phy_base);
                if (err) {
                    pr_err("[zxdh_rdma] SQC: query leaf PBLE (index=%u) failed:%d\n", leaf_index, err);
                    break;
                }

                object_wqe_ctx.src_address = (leaf_phy_base & ZXDH_MAX_STATS_64);
                object_wqe_ctx.src_path_select = route_id;
                object_wqe_ctx.src_interface_select = ZXDH_INTERFACE_NOTCACHE;
                object_wqe_ctx.dest_address = dest_base + (u64)j * page_size;
                object_wqe_ctx.data_length = page_size;

                err = hw_object_query_info(rf, &object_wqe_ctx);
                if (err) {
                    pr_err("[zxdh_rdma] SQC: dma read page %u failed:%d\n", j, err);
                    break;
                }
            }
            if (err)
                goto free_exit;
        }

        object_wqe_ctx.data_length = total_bytes;
        object_wqe_ctx.object_size = ZXDH_SQ_UNIT_SIZE;
        object_wqe_ctx.src_path_select = route_id;
        object_data_resp.length = total_bytes;
        object_data_resp.vhca_id = object_wqe_ctx.src_vhca_Index;
        object_data_resp.route_id = route_id;
        object_data_resp.object_size = ZXDH_SQ_UNIT_SIZE;
        object_data_resp.srqp_aligned_offset = 0;
		object_data_resp.queue_id = object_data_req.queue_id;

#ifdef RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
        ret = uverbs_copy_to_struct_or_zero(attrs,
                            ZXDH_IB_ATTR_DEV_GET_OBJ_DATA_RESP,
                            &object_data_resp,
                            sizeof(object_data_resp));
#else
        ret = zxdh_copy_to_struct_or_zero(attrs,
                          ZXDH_IB_ATTR_DEV_GET_OBJ_DATA_RESP,
                          &object_data_resp,
                          sizeof(object_data_resp));
#endif
        if (ret) {
            pr_err("[zxdh_rdma] SQC: copy response to user err:%d\n", ret);
            goto free_exit;
        }
        return ret;
    }

	ret = hw_object_calculate_wqe_context(dev, &object_data_req,
					      &object_wqe_ctx);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, caculate wqe context err: %d\n", ret);
		return ret;
	}

	ret = post_validate_hw_object_request(&object_wqe_ctx,
					      &object_data_req);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, pre validate hw object request err: %d\n",
		       ret);
		return ret;
	}

	if (allocate_addr_for_mmap(iwdev, ucontext, object_wqe_ctx.data_length,
				   &ucontext->hw_data_cap.hw_object_mmap,
				   &object_data_resp.object_mmap_offset)) {
		pr_err("[zxdh_rdma] query hw object, obj_mmap_entry insert err!buf_size:%u\n",
		       object_wqe_ctx.data_length);
		goto free_exit;
	}

	ucontext->hw_data_cap.object_buffer_size = object_wqe_ctx.data_length;
	/* 根据 use_ext_mem_flag 选择 DMA 目标地址基址 */
	if (rf->use_ext_mem_flag) {
		object_wqe_ctx.dest_address = ucontext->hw_data_cap.hw_object_mmap.cap_ext_mem_addr.cap_dma_addr;
	} else {
		object_wqe_ctx.dest_address = ucontext->hw_data_cap.hw_object_mmap.addr_info.cap_direct_dma_addr.cap_dma_addr;
	}

	ret = hw_object_query_info(rf, &object_wqe_ctx);
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, query object info err:%d\n", ret);
		goto free_exit;
	}
	hw_object_fill_object_data_resp(&object_data_resp, &object_wqe_ctx);
#ifdef RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
	ret = uverbs_copy_to_struct_or_zero(attrs,
					    ZXDH_IB_ATTR_DEV_GET_OBJ_DATA_RESP,
					    &object_data_resp,
					    sizeof(object_data_resp));
#else
	ret = zxdh_copy_to_struct_or_zero(attrs,
					  ZXDH_IB_ATTR_DEV_GET_OBJ_DATA_RESP,
					  &object_data_resp,
					  sizeof(object_data_resp));
#endif
	if (ret) {
		pr_err("[zxdh_rdma] query hw object, copy response to user err:%d\n", ret);
		goto free_exit;
	}
	return ret;
free_exit:
	pr_err("[zxdh_rdma] query hw object failed, free allocated memory!\n");
	free_mmap_addr(iwdev, ucontext->hw_data_cap.object_buffer_size,
		       &ucontext->hw_data_cap.hw_object_mmap);
	return ret;
}

static u32 get_reg_value_by_addr(struct zxdh_pci_f *rf, 
				u64 reg_va, u16 *count_ret, u32 *reg_list, u64 *base_reg)
{
	u32 value = 0, base_addr = 0;
	long offset = 0;
	switch (reg_va >> 12)
	{
	case 0x6205400:
		base_addr = C_RDMA_RX_PKT_PROC_PAGE(rf->sc_dev.ep_id);
		break;
	case 0x6205420:
		base_addr = C_RDMA_RX_PUBLIC_PAGE1(rf->sc_dev.ep_id);
		break;
	case 0x6205440:
		base_addr = C_RDMA_RX_PUBLIC_PAGE2(rf->sc_dev.ep_id);
		break;
	case 0x6205460:
		base_addr = C_RDMA_RX_CNP_GEN_PAGE(rf->sc_dev.ep_id);
		break;
	case 0x62065f0:
		base_addr = C_RDMA_TX_WQE_PARSE_PAGE(rf->sc_dev.ep_id);
		break;
	case 0x62065f8:
		base_addr = C_RDMA_TX_RTT_PAGE(rf->sc_dev.ep_id);
		break;
	case 0x6330200:
		if (zxdh_rdma_reg_read(rf, reg_va, &value)) {
			pr_err("[zxdh_rdma] [check_health_err] read 0x633 reg value falied, reg_va: %llx\n", reg_va);
			(*count_ret) --;
		}
		break;
	default:
		offset = (reg_va - *base_reg) / 4;
		if (*base_reg == 0 || offset < 0 || offset >= MAX_READ_REG_SIZE) {
			if (zxdh_rdma_regs_read(rf, reg_va, reg_list, MAX_READ_REG_SIZE)) {
				pr_err("[zxdh_rdma] [check_health_err] read reg value falied, reg_va: %llx\n", reg_va);
				(*count_ret) --;
				return 0;
			}
			*base_reg = reg_va;
			value = reg_list[0];
		} else {
			value = reg_list[offset];
		}
		break;
	}
	if (base_addr) {
		value = readl((u32 __iomem *)(rf->sc_dev.hw->hw_addr + base_addr + (reg_va & 0xfff)));
	}
	return value;
}

static int get_reg_value(struct zxdh_pci_f *rf,
				u64 reg_va, u64 value_va, u16 count, u16 *count_ret)
{
	u64 regs[MAX_COPY_SIZE];
	u32 values[MAX_COPY_SIZE];
	u32 reg_list[MAX_READ_REG_SIZE];
	u64 base_reg = 0;
	int i = 0, j = 0, len = 0;

	if (reg_va == 0 || value_va == 0 || 
		count == 0 || count > 512) {
		pr_err("[zxdh_rdma] invalid reg_va or value_va or count!\n");
		return -EINVAL;
	}
	*count_ret = count;
	while (i < count)
	{
		if (count - i >= MAX_COPY_SIZE)
			len = MAX_COPY_SIZE;
		else
			len = count - i;
		i += len;
		if (copy_from_user((void *)regs, (void *)reg_va, sizeof(u64) * len))
			return -EFAULT;

		reg_va += sizeof(u64) * len;
		j = 0;
		while (j < len) {
			values[j] = get_reg_value_by_addr(rf, regs[j], count_ret, reg_list, &base_reg);
			j++;
		}
			
		if (copy_to_user((void *)value_va, (void *)values, sizeof(u32) * len))
			return -EFAULT;

		value_va += sizeof(u32) * len;
	}
	return 0;
}

static int get_reg_value_ex(struct zxdh_pci_f *rf,
				u64 va, u16 *count)
{
	struct zxdh_reg_value reg_value[MAX_COPY_SIZE_EX];
	u32 reg1 ,reg2;
	int i = 0;

	if (va == 0) {
		pr_err("[zxdh_rdma] invalid reg_value_va_ex!\n");
		return -EINVAL;
	}
	WRITE_REGISTER_AND_CHECK(rf, 0x620660b3d0, 0xa0202c0b);
	GET_REG_AND_WRITE_TO_USE(0x620660b240, 0);

	WRITE_REGISTER_AND_CHECK(rf, 0x620660b3d0, 0xa0202c0d);
	GET_REG_AND_WRITE_TO_USE(0x620660b248, 0);

    WRITE_REGISTER_AND_CHECK(rf, 0x620660b3d0, 0xa0202c0f);
	GET_REG_AND_WRITE_TO_USE(0x620660b250, 0);

    WRITE_REGISTER_AND_CHECK(rf, 0x620660b3d0, 0xa0202c11);
	GET_REG_AND_WRITE_TO_USE(0x620660b258, 0);

    WRITE_REGISTER_AND_CHECK(rf, 0x620660b3d0, 0xa0202c13);
	GET_REG_AND_WRITE_TO_USE(0x620660b26c, 0);

	WRITE_REGISTER_AND_CHECK(rf, 0x620660b3d0, 0xa0202c00);
	GET_REG_AND_WRITE_TO_USE(0x620660b26c, 1);

	READ_REGISTER_AND_CHECK(rf, 0x62065F0100, &reg2);
	READ_REGISTER_AND_CHECK(rf, 0x62065F0100, &reg1);
    reg1 |= 1;
    WRITE_REGISTER_AND_CHECK(rf, 0x62065F0100, reg1);
	GET_REG_AND_WRITE_TO_USE(0x62065F0b58, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065F0ae0, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065F07b8, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065F0824, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065F0880, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065F08dc, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065F0938, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065F0994, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065F0a68, 0);
	WRITE_REGISTER_AND_CHECK(rf, 0x62065F0100, reg2);

	READ_REGISTER_AND_CHECK(rf, 0x62065F0100, &reg2);
	READ_REGISTER_AND_CHECK(rf, 0x62065F0100, &reg1);
    reg1 |= (1 << 4);
    WRITE_REGISTER_AND_CHECK(rf, 0x62065F0100, reg1);
	GET_REG_AND_WRITE_TO_USE(0x62065f0f1c, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065f0f24, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065f0f0c, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065f0f14, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065f0f18, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065f0f28, 0);
    WRITE_REGISTER_AND_CHECK(rf, 0x62065F0100, reg2);

	READ_REGISTER_AND_CHECK(rf, 0x62065F0100, &reg2);
	READ_REGISTER_AND_CHECK(rf, 0x62065F0100, &reg1);
    reg1 |= (1 << 5);
    WRITE_REGISTER_AND_CHECK(rf, 0x62065F0100, reg1);
	GET_REG_AND_WRITE_TO_USE(0x62065f10F8, 0);
	GET_REG_AND_WRITE_TO_USE(0x62065f10FC, 0);
    WRITE_REGISTER_AND_CHECK(rf, 0x62065F0100, reg2);

	READ_REGISTER_AND_CHECK(rf, 0x62065F0100, &reg2);
	READ_REGISTER_AND_CHECK(rf, 0x62065F0100, &reg1);
    reg1 &= ~(1 << 4);
	WRITE_REGISTER_AND_CHECK(rf, 0x62065F0100, reg1);
	GET_REG_AND_WRITE_TO_USE(0x62065f0f0c, 1);
	GET_REG_AND_WRITE_TO_USE(0x62065f0f10, 0);
	WRITE_REGISTER_AND_CHECK(rf, 0x62065F0100, reg2);

	if (copy_to_user((void *)va, (void *)reg_value, sizeof(struct zxdh_reg_value) * i))
		return -EFAULT;

	return 0;
}

static int compare_SMMU_reg(struct zxdh_pci_f *rf,
				u64 va, u16 *count)
{
	u32 flag = 0;
	u16 i = 0, j, k, index = 0;
	u64 regs[MAX_SMMU_READ_REG_SIZE];
	while (i < 512 / MAX_SMMU_READ_REG_SIZE) {
		j = 0;
		READ_REGISTERS_AND_CHECK(rf, 0x4ec02000+i*MAX_SMMU_READ_REG_SIZE*0x8, (u32 *)regs, MAX_SMMU_READ_REG_SIZE * 2);
		while (j < MAX_SMMU_READ_REG_SIZE)
		{
			k = 0;
			flag = 0;
			while (k < 4) {
				if (regs[j + k] != 0)
				{
					flag = 1;
					break;
				}
				k++;
			}
			if (flag == 1) {
				for (k = 0; k < 4; k++)
					if (regs[j + k] != 0)
						pr_err("[zxdh_rdma] SMMU reg 0x%x is not zero, reg value is 0x%llx\n", 0x4ec02000+i*MAX_SMMU_READ_REG_SIZE*0x8+(j+k)*0x8, regs[j + k]);
				if (copy_to_user((void *)va, &index, sizeof(index)))
					return -EFAULT;
				va += sizeof(index);
				(*count)++;
			}
			j += 4;
			index++;
		}
		i++;
	}
	return 0;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_HEALTH_CHECK)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_HEALTH_CHECK)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct zxdh_pci_f *rf;
	struct zxdh_health_check_req health_check_req = { 0 };
	struct zxdh_health_check_resp health_check_resp = { 0 };
	int ret = 0;

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif

	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);
	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);
	rf = iwdev->rf;

	ret = uverbs_copy_from(&health_check_req, attrs, ZXDH_IB_ATTR_DEV_HEALTH_CHECK);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	switch (health_check_req.reg_type)
	{
	case ZXDH_WRITE_FIRST_REG:
		ret = get_reg_value_ex(rf, health_check_req.reg_value_va_ex, &health_check_resp.count_ex);
		if (ret) {
			pr_err("[zxdh_rdma] get reg value ex failed!\n");
			return ret;
		}
		fallthrough; 
	case ZXDH_NORMAL_REG:
		ret = get_reg_value(rf, health_check_req.reg_va, health_check_req.value_va, 
					health_check_req.count, &health_check_resp.count);
		break;

	case ZXDH_SMMU_REG:
		ret = compare_SMMU_reg(rf, health_check_req.reg_value_va_ex, &health_check_resp.count_ex);
		break;

	default:
		pr_err("[zxdh_rdma] errro reg_type is %d, not support!\n", health_check_req.reg_type);
		return -EFAULT;
	}
	if (ret) {
		pr_err("[zxdh_rdma] get reg value failed!\n");
		return ret;
	}

	#ifdef RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
	ret = uverbs_copy_to_struct_or_zero(attrs, ZXDH_IB_ATTR_DEV_HEALTH_CHECK_RESP,
					&health_check_resp, sizeof(health_check_resp));
	#else
	ret = zxdh_copy_to_struct_or_zero(attrs, ZXDH_IB_ATTR_DEV_HEALTH_CHECK_RESP,
					&health_check_resp, sizeof(health_check_resp));
	#endif
	if (ret) {
		pr_err("[zxdh_rdma] ib_copy_to_udata failed!\n");
		return -EFAULT;
	}
	return 0;
}

static int clean_cc_basic_cnt_info(struct zxdh_device *iwdev){
	uint32_t read_reg_val = 0;

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_ACTIVE_VHCA_SQ_CNT_CLEAN,
			 &read_reg_val);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_ACTIVE_VHCA_SQ_CNT_CLEAN, read_reg_val |= C_ACTIVE_VHCA_SQ_CNT_CLEAN_MASK);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_ACTIVE_VHCA_SQ_CNT_CLEAN, read_reg_val &= (~C_ACTIVE_VHCA_SQ_CNT_CLEAN_MASK));


	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_ACTIVE_VHCA_READ_CNT_CLEAN,
			 &read_reg_val);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_ACTIVE_VHCA_READ_CNT_CLEAN, read_reg_val |= C_ACTIVE_VHCA_READ_CNT_CLEAN_MASK);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_ACTIVE_VHCA_READ_CNT_CLEAN, read_reg_val &= (~C_ACTIVE_VHCA_READ_CNT_CLEAN_MASK));

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_ACTIVE_VHCA_ACK_CNT_CLEAN,
			 &read_reg_val);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_ACTIVE_VHCA_ACK_CNT_CLEAN, read_reg_val |= C_ACTIVE_VHCA_ACK_CNT_CLEAN_MASK);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_ACTIVE_VHCA_ACK_CNT_CLEAN, read_reg_val &= (~C_ACTIVE_VHCA_ACK_CNT_CLEAN_MASK));

	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_TASK_PREFETCH_RECV_COM_CNT_CLEAN,
			 &read_reg_val);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_TASK_PREFETCH_RECV_COM_CNT_CLEAN, read_reg_val |= C_TASK_PREFETCH_RECV_COM_CNT_CLEAN_MASK);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_TASK_PREFETCH_RECV_COM_CNT_CLEAN, read_reg_val &= (~C_TASK_PREFETCH_RECV_COM_CNT_CLEAN_MASK));


	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_TX_PKT_CNT_CLEAN,
			 &read_reg_val);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_TX_PKT_CNT_CLEAN, read_reg_val |= C_TX_PKT_CNT_CLEAN_MASK);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_TX_PKT_CNT_CLEAN, read_reg_val &= (~C_TX_PKT_CNT_CLEAN_MASK));


	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_RX_PKT_CNT_CLEAN,
			 &read_reg_val);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_RX_PKT_CNT_CLEAN, read_reg_val |= C_RX_PKT_CNT_CLEAN_MASK);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_RX_PKT_CNT_CLEAN, read_reg_val &= (~C_RX_PKT_CNT_CLEAN_MASK));


	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_RETRY_TIMEOUTE_CNT_CLEAN,
			 &read_reg_val);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_RETRY_TIMEOUTE_CNT_CLEAN, read_reg_val |= C_RETRY_TIMEOUTE_CNT_CLEAN_MASK);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_RETRY_TIMEOUTE_CNT_CLEAN, read_reg_val &= (~C_RETRY_TIMEOUTE_CNT_CLEAN_MASK));


	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_TX_PKT_CNP_CNT_CLEAN,
			 &read_reg_val);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_TX_PKT_CNP_CNT_CLEAN, read_reg_val |= C_TX_PKT_CNP_CNT_CLEAN_MASK);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_TX_PKT_CNP_CNT_CLEAN, read_reg_val &= (~C_TX_PKT_CNP_CNT_CLEAN_MASK));


	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_TX_PKT_RTT_T1_CNT_CLEAN,
			 &read_reg_val);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_TX_PKT_RTT_T1_CNT_CLEAN, read_reg_val |= C_TX_PKT_RTT_T1_CNT_CLEAN_MASK);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_TX_PKT_RTT_T1_CNT_CLEAN, read_reg_val &= (~C_TX_PKT_RTT_T1_CNT_CLEAN_MASK));


	REG_OP_AND_CHECK(zxdh_rdma_reg_read, iwdev->rf, C_TX_PKT_RTT_T4_CNT_CLEAN,
			 &read_reg_val);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_TX_PKT_RTT_T4_CNT_CLEAN, read_reg_val |= C_TX_PKT_RTT_T4_CNT_CLEAN_MASK);
	REG_OP_AND_CHECK(zxdh_rdma_reg_write, iwdev->rf,
					 C_TX_PKT_RTT_T4_CNT_CLEAN, read_reg_val &= (~C_TX_PKT_RTT_T4_CNT_CLEAN_MASK));


	return 0;

}

static int clean_all_gqps_mp_cap(struct zxdh_ucontext *ucontext)
{
	struct zxdh_device *iwdev;
	struct zxdh_sc_dev *dev;
	struct zxdh_mp_cap_gqp *cap_gqp;
	u16 gqp_start, gqp_cnt, vhca_ud_gqp;
	int ret = 0;
	int index;

	iwdev = ucontext->iwdev;

	dev = &iwdev->rf->sc_dev;
	gqp_start = dev->vhca_gqp_start;
	gqp_cnt = dev->vhca_gqp_cnt;
	vhca_ud_gqp = dev->vhca_ud_gqp;

	if (gqp_cnt >= VHCA_RC_UD_GQP_MAX_CNT) {
		pr_err("[zxdh_rdma] clean_all_gqps_mp_cap: gqp_cnt:%u bigger than %d\n",
		       gqp_cnt, VHCA_RC_UD_GQP_MAX_CNT);
		return -EINVAL;
	}

	cap_gqp = kzalloc(sizeof(*cap_gqp), GFP_KERNEL);
	if (!cap_gqp) {
		pr_err("[zxdh_rdma] clean_all_gqps_mp_cap: failed to allocate cap_gqp\n");
		return -ENOMEM;
	}

	cap_gqp->gqp_num = gqp_cnt + 1;
	for (index = 0; index < gqp_cnt; index++) {
		cap_gqp->gqpid[index] = gqp_start + index;
	}
	cap_gqp->gqpid[gqp_cnt] = vhca_ud_gqp;

	/* Call reset_mp_cap_reg with zxdh_mp_cap_gqp */
	mutex_lock(&mp_cap_reg_lock);
	ret = reset_mp_cap_reg(cap_gqp, ucontext);
	mutex_unlock(&mp_cap_reg_lock);

	kfree(cap_gqp);
	return ret;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_CFG_PARAMETER)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_CFG_PARAMETER)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct zxdh_cfg_dev_parameter_req req = { 0 };
	struct zxdh_ucontext *ucontext;
	int ret = 0;
	int i;

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif

	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);
	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);
	ucontext = to_ucontext(ib_uctx);
	
	ret = uverbs_copy_from(&req, attrs, ZXDH_IB_ATTR_DEV_CFG_PARAMETER);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;
	switch (req.type)
	{
	case TX_STOP_ON_AEQ:
		iwdev->rf->sc_dev.tx_stop_on_aeq = 1;
		break;
	case RX_STOP_ON_AEQ:
		iwdev->rf->sc_dev.rx_stop_on_aeq = 1;
		break;
	case TXRX_STOP_IOVA_CAP:
		for (i = 0; i < CAP_NODE_NUM; i++) {
			if (ucontext->hw_data_cap.cap_txrx_use_iova[i].addr_info.cap_iova_addr != 0) 
					ucontext->hw_data_cap.cap_txrx_use_iova[i].addr_info.cap_iova_addr  = 0;
		}
		break;
	case CLEAR_ALL_CC_BASIC_CNT:
		mutex_lock(&cc_basic_info_lock);
		ret = clean_cc_basic_cnt_info(iwdev);
		mutex_unlock(&cc_basic_info_lock);
		break;
	case CLEAR_ALL_GQPS_MP_CAP:
		ret = clean_all_gqps_mp_cap(ucontext);
		break;
	default:
		pr_err("[zxdh_rdma] not support type %d!\n", req.type);
		return -EINVAL;
	}
	return ret;
}

static int
get_pf_qpn_reg_value(struct zxdh_pci_f *rf,
		     struct zxdh_db_show_res_map_req *show_res_map_req,
		     struct zxdh_db_show_res_map_resp *show_res_map_resp)
{
	u64 reg[2];
	u32 value[2], idx[2], pf_id_u32;
	u8 pf_id = rf->pf_id;

	if (rf->ftype)
		return 0;
	if (pf_id > PCIE_PF_NUM_MAX) {
		pr_err("[zxdh_rdma] pf_id:%u bigger than 31,err!\n", pf_id);
		return -EINVAL;
	}
	pf_id_u32 = (u32)pf_id;
	idx[0] = pf_id_u32;
	idx[1] = pf_id_u32;
	reg[0] = C_DB_SHOW_PF_START_QPN_MAP + pf_id_u32 * 0x8;
	reg[1] = C_DB_SHOW_PF_END_VHCA_MAP + pf_id_u32 * 0x8;

	COPY_TO_USER_SAFE(show_res_map_req->idx_va, idx, sizeof(u32) * 2);
	COPY_TO_USER_SAFE(show_res_map_req->reg_va, reg, sizeof(u64) * 2);
	READ_REGISTERS_AND_CHECK(rf, reg[0], value, 2);
	COPY_TO_USER_SAFE(show_res_map_req->value_va, value, sizeof(u32) * 2);
	show_res_map_resp->count = 2;
	return 0;
}

static int
get_pf_vhca_reg_value(struct zxdh_pci_f *rf,
		      struct zxdh_db_show_res_map_req *show_res_map_req,
		      struct zxdh_db_show_res_map_resp *show_res_map_resp)
{
	u64 reg;
	u32 value, pf_id_u32;
	u8 pf_id = rf->pf_id;

	if (rf->ftype)
		return 0;
	if (pf_id > PCIE_PF_NUM_MAX) {
		pr_err("[zxdh_rdma] pf_id:%u bigger than 31,err!\n", pf_id);
		return -EINVAL;
	}
	pf_id_u32 = (u32)pf_id;
	reg = C_DB_SHOW_PF_VHCA_MAP + pf_id_u32 * 0x4;

	COPY_TO_USER_SAFE(show_res_map_req->reg_va, &reg, sizeof(u64));
	READ_REGISTER_AND_CHECK(rf, reg, &value);
	COPY_TO_USER_SAFE(show_res_map_req->value_va, &value, sizeof(u32));
	COPY_TO_USER_SAFE(show_res_map_req->idx_va, &pf_id_u32, sizeof(u32));
	show_res_map_resp->count = 1;
	return 0;
}

static int
get_vhca_physical_reg_value(struct zxdh_pci_f *rf,
			    struct zxdh_db_show_res_map_req *show_res_map_req,
			    struct zxdh_db_show_res_map_resp *show_res_map_resp)
{
	u64 reg;
	u32 value, vhca_id_u32;
	struct zxdh_sc_dev *dev;
	u16 vhca_id;

	dev = &rf->sc_dev;
	vhca_id = dev->vhca_id;
	if (vhca_id > VHCA_NUM_MAX) {
		pr_err("[zxdh_rdma] vhca_id:%u bigger than 257,err!\n", vhca_id);
		return -EINVAL;
	}

	vhca_id_u32 = (u32)vhca_id;
	reg = C_DB_SHOW_VHCA_PHYSICAL_MAP + vhca_id_u32 * 0x1000;

	COPY_TO_USER_SAFE(show_res_map_req->reg_va, &reg, sizeof(u64));
	READ_REGISTER_AND_CHECK(rf, reg, &value);
	COPY_TO_USER_SAFE(show_res_map_req->value_va, &value, sizeof(u32));
	COPY_TO_USER_SAFE(show_res_map_req->idx_va, &vhca_id_u32, sizeof(u32));
	show_res_map_resp->count = 1;
	return 0;
}

static int
get_8k_gqp_reg_value(struct zxdh_pci_f *rf,
		     struct zxdh_db_show_res_map_req *show_res_map_req,
		     struct zxdh_db_show_res_map_resp *show_res_map_resp)
{
	u64 *reg_array, reg;
	u32 *idx, *value_array, i, j, chunk_size, ud_value;
	void *base_memory;
	struct zxdh_sc_dev *dev;
	u16 vhca_id, start_8k, cnt_8k, vhca_ud_8k_index;
	int ret;
	size_t total_size = 0;
	uint32_t regs_num;

	dev = &rf->sc_dev;
	vhca_id = dev->vhca_id;
	if (vhca_id > VHCA_NUM_MAX) {
		pr_err("[zxdh_rdma] vhca_id:%u bigger than 257,err!\n", vhca_id);
		return -EINVAL;
	}

	start_8k = dev->vhca_8k_index_start;
	cnt_8k = dev->vhca_8k_index_cnt;
	vhca_ud_8k_index = dev->vhca_ud_8k_index;
	regs_num = cnt_8k + 1;

	if (regs_num > VHCA_RC_UD_8K_MAX_CNT) {
		pr_err("[zxdh_rdma] cnt_8k:%u bigger than %u,err!\n", cnt_8k, VHCA_RC_UD_8K_MAX_CNT);
		return -EINVAL;
	}
	total_size = sizeof(u64) * regs_num + 2 * sizeof(u32) * regs_num;
	base_memory = kmalloc(total_size, GFP_KERNEL);
	if (!base_memory) {
		pr_err("[zxdh_rdma] Memory allocation failed\n");
		return -ENOMEM;
	}
	memset(base_memory, 0, total_size);
	reg_array = (u64 *)base_memory;
	idx = (u32 *)((char *)base_memory + sizeof(u64) * regs_num);
	value_array = (u32 *)((char *)idx + sizeof(u32) * regs_num);

	for (i = start_8k; i < start_8k + cnt_8k; i++) {
		reg = C_DB_SHOW_8K_2K_MAP + i * 0x4;
		reg_array[i - start_8k] = reg;
		idx[i - start_8k] = i;
	}

	for (j = 0; j < cnt_8k; j += MAX_READ_REG_SIZE) {
		chunk_size = (cnt_8k - j) < MAX_READ_REG_SIZE ?
				     (cnt_8k - j) :
				     MAX_READ_REG_SIZE;
		ret = zxdh_rdma_regs_read(rf, reg_array[j], &value_array[j],
					  chunk_size);
		if (ret) {
			kfree(base_memory);
			pr_err("[zxdh_rdma] [get_8k_gqp_reg_value] zxdh_rdma_regs_read failed at chunk %d",
			       j / MAX_READ_REG_SIZE);
			return -EFAULT;
		}
	}

	idx[cnt_8k] = vhca_ud_8k_index;
	reg_array[cnt_8k] = C_DB_SHOW_8K_2K_MAP + vhca_ud_8k_index * 0x4;

	ret = zxdh_rdma_reg_read(rf, reg_array[cnt_8k], &ud_value);
	if (ret) {
		kfree(base_memory);
		pr_err("[zxdh_rdma] [get_8k_gqp_reg_value] zxdh_rdma_reg_read falied");
		return -EFAULT;
	}

	value_array[cnt_8k] = ud_value & 0x7FF;
	if (copy_to_user((void *)show_res_map_req->reg_va, (void *)reg_array,
			 sizeof(u64) * regs_num)) {
		kfree(base_memory);
		return -EFAULT;
	}
	if (copy_to_user((void *)show_res_map_req->value_va,
			 (void *)value_array, sizeof(u32) * regs_num)) {
		kfree(base_memory);
		return -EFAULT;
	}
	if (copy_to_user((void *)show_res_map_req->idx_va, (void *)idx,
			 sizeof(u32) * regs_num)) {
		kfree(base_memory);
		return -EFAULT;
	}
	show_res_map_resp->count = regs_num;

	kfree(base_memory);

	return 0;
}

static int
get_gqp_8k_create_reg_value(struct zxdh_pci_f *rf,
			    struct zxdh_db_show_res_map_req *show_res_map_req,
			    struct zxdh_db_show_res_map_resp *show_res_map_resp)
{
	u64 reg;
	u64 *reg_array = NULL;
	u32 *idx = NULL;
	u32 *value_array = NULL;
	void *base_memory;
	u32 i, j, chunk_size;
	struct zxdh_sc_dev *dev;
	u16 vhca_id, gqp_start, gqp_cnt, vhca_ud_gqp;
	size_t total_size = 0;
	int ret = 0;

	total_size = VHCA_RC_UD_GQP_MAX_CNT * sizeof(u64) + 
				2 * VHCA_RC_UD_GQP_MAX_CNT * sizeof(u32);
	base_memory = kmalloc(total_size, GFP_KERNEL);
	if (!base_memory){
		pr_err("Memory allocation failed\n");
		return -ENOMEM;
	}
	memset(base_memory, 0, total_size);

	reg_array = (u64 *)base_memory;
	idx = (u32 *)((char *)base_memory + sizeof(u64) * VHCA_RC_UD_GQP_MAX_CNT);
	value_array = (u32 *)((char *)idx + sizeof(u32) * VHCA_RC_UD_GQP_MAX_CNT);

	dev = &rf->sc_dev;
	vhca_id = dev->vhca_id;
	if (vhca_id > VHCA_NUM_MAX) {
		pr_err("[zxdh_rdma] vhca_id:%u bigger than 257,err!\n", vhca_id);
		ret = -EINVAL;
		goto out;
	}
	gqp_start = dev->vhca_gqp_start;
	gqp_cnt = dev->vhca_gqp_cnt;
	vhca_ud_gqp = dev->vhca_ud_gqp;
	if (gqp_cnt >= VHCA_RC_UD_GQP_MAX_CNT) {
		pr_err("[zxdh_rdma] gqp_cnt:%u bigger than %d,err!\n", gqp_cnt, VHCA_RC_UD_GQP_MAX_CNT);
		ret = -EINVAL;
		goto out;
	}
	for (i = gqp_start; i < gqp_start + gqp_cnt; i++) {
		reg = C_DB_SHOW_GQP_VHCA_MAP + i * 0x4;
		reg_array[i - gqp_start] = reg;
		idx[i - gqp_start] = i;
	}
	for (j = 0; j < gqp_cnt; j += MAX_READ_REG_SIZE) {
		chunk_size = (gqp_cnt - j) < MAX_READ_REG_SIZE ?
				     (gqp_cnt - j) :
				     MAX_READ_REG_SIZE;
		ret = zxdh_rdma_regs_read(rf, reg_array[j], &value_array[j], chunk_size);
		if (ret) {
			goto out;
		}
	}

	idx[gqp_cnt] = vhca_ud_gqp;
	reg_array[gqp_cnt] = C_DB_SHOW_GQP_VHCA_MAP + vhca_ud_gqp * 0x4;
	ret = zxdh_rdma_reg_read(rf, reg_array[gqp_cnt], &value_array[gqp_cnt]);
	if (ret) {
		goto out;
	}

	if (copy_to_user((void *)show_res_map_req->reg_va,
			 (void *)reg_array, sizeof(u64) * (gqp_cnt + 1))) {
		ret = -EFAULT;
		goto out;
	}

	if (copy_to_user((void *)show_res_map_req->value_va,
			 (void *)value_array, sizeof(u32) * (gqp_cnt + 1))) {
		ret = -EFAULT;
		goto out;
	}

	if (copy_to_user((void *)show_res_map_req->idx_va,
			 (void *)idx, sizeof(u32) * (gqp_cnt + 1))) {
		ret = -EFAULT;
		goto out;
	}

	show_res_map_resp->count = gqp_cnt + 1;

out:
	kfree(base_memory);

	return ret;
}

static int process_vhca_register(u64 *reg_array, u32 *idx, u32 *value_array,
				 u32 *num, u32 index, void *rf)
{
	u32 read_reg_val;
	u64 reg;
	uint32_t qps_act_bit = 0x80000000;
	int ret = 0;

	mutex_lock(&active_gqp_reg_lock);

	ret = zxdh_rdma_reg_write(rf, C_CHECK_GQP_ACTIVE_WRITE, index);
	if (ret != 0)
		goto unlock;

	ret = zxdh_rdma_reg_read(rf, C_CHECK_GQP_ACTIVE_READ, &read_reg_val);
	if (ret != 0)
		goto unlock;

	if ((read_reg_val & qps_act_bit) != 0) {
		reg = C_DB_SHOW_GQP_VHCA_MAP + index * 0x4;
		reg_array[*num] = reg;
		idx[*num] = index;
		ret = zxdh_rdma_reg_read(rf, reg, &value_array[*num]);
		if (ret != 0)
			goto unlock;
		(*num)++;
	}

unlock:
	mutex_unlock(&active_gqp_reg_lock);
	return ret;
}

static int
get_gqp_8k_active_reg_value(struct zxdh_pci_f *rf,
			    struct zxdh_db_show_res_map_req *show_res_map_req,
			    struct zxdh_db_show_res_map_resp *show_res_map_resp)
{
	u32 i;
	u64 *reg_array = NULL;
	u32 *idx = NULL;
	u32 *value_array = NULL;
	struct zxdh_sc_dev *dev;
	u16 vhca_id, gqp_start, gqp_cnt, vhca_ud_gqp;
	u32 num = 0;
	void *base_memory;
	size_t total_size = 0;
	int ret = 0;

	total_size = VHCA_RC_UD_GQP_MAX_CNT * sizeof(u64) + 
				2 * VHCA_RC_UD_GQP_MAX_CNT * sizeof(u32);
	base_memory = kmalloc(total_size, GFP_KERNEL);
	if (!base_memory){
		pr_err("Memory allocation failed\n");
		return -ENOMEM;
	}
	memset(base_memory, 0, total_size);

	reg_array = (u64 *)base_memory;
	idx = (u32 *)((char *)base_memory + sizeof(u64) * VHCA_RC_UD_GQP_MAX_CNT);
	value_array = (u32 *)((char *)idx + sizeof(u32) * VHCA_RC_UD_GQP_MAX_CNT);

	dev = &rf->sc_dev;
	vhca_id = dev->vhca_id;
	if (vhca_id > VHCA_NUM_MAX) {
		pr_err("[zxdh_rdma] vhca_id:%u bigger than 257,err!\n", vhca_id);
		ret = -EINVAL;
		goto out;
	}
	gqp_start = dev->vhca_gqp_start;
	gqp_cnt = dev->vhca_gqp_cnt;
	vhca_ud_gqp = dev->vhca_ud_gqp;
	if (gqp_cnt >= VHCA_RC_UD_GQP_MAX_CNT) {
		pr_err("[zxdh_rdma] gqp_cnt:%u bigger than %d,err!\n", gqp_cnt, VHCA_RC_UD_GQP_MAX_CNT);
		ret = -EINVAL;
		goto out;
	}
	for (i = gqp_start; i < gqp_start + gqp_cnt; i++) {
		process_vhca_register(reg_array, idx, value_array, &num, i, rf);
	}

	process_vhca_register(reg_array, idx, value_array, &num, vhca_ud_gqp, rf);
	if (copy_to_user((void *)show_res_map_req->reg_va,
			 (void *)reg_array, sizeof(u64) * num)) {
		ret = -EFAULT;
		goto out;
	}

	if (copy_to_user((void *)show_res_map_req->value_va,
			 (void *)value_array, sizeof(u32) * num)) {
		ret = -EFAULT;
		goto out;
	}

	if (copy_to_user((void *)show_res_map_req->idx_va,
			 (void *)idx, sizeof(u32) * num)) {
		ret = -EFAULT;
		goto out;
	}

	show_res_map_resp->count = num;

out:
	kfree(base_memory);

	return ret;
}

static int validate_show_map_qpn(struct zxdh_pci_f *rf, u32 *qpn,
				 u32 requested_qpn)
{
	struct zxdh_sc_dev *dev;
	u32 min_qpn, max_qpn;

	if (!rf) {
		pr_err("[zxdh_rdma] Invalid rf pointer\n");
		return -EINVAL;
	}

	dev = &rf->sc_dev;
	min_qpn = dev->base_qpn + 1;
	max_qpn = dev->base_qpn + rf->max_qp - 1;

	if ((requested_qpn != 1) &&
	    (requested_qpn < min_qpn || requested_qpn > max_qpn)) {
		pr_err("[zxdh_rdma] Requested qpn (%u) out of valid range [%u, %u]\n",
		       requested_qpn, min_qpn, max_qpn);
		return -ZXDH_QPN_ERROR;
	}

	if (requested_qpn == 1) {
		*qpn = min_qpn;
	} else {
		*qpn = requested_qpn;
	}

	return 0;
}

static int
get_qp_8k_reg_value(struct zxdh_pci_f *rf, u32 qpn,
		    struct zxdh_db_show_res_map_resp *show_res_map_resp)
{
	struct zxdh_sc_dev *dev;
	struct zxdh_qp *iwqp = NULL;
	dev = &rf->sc_dev;

	iwqp = rf->qp_table[qpn - dev->base_qpn];
	if (!iwqp) {
		pr_err("[zxdh_rdma] get_qp_8k_reg_value: iwqp is null!\n");
		return -ZXDH_QP_NOT_AVALIABLE;
	}
	show_res_map_resp->qp_8k_index = iwqp->sc_qp.qp_uk.qp_8k_index;

	return 0;
}

#ifdef ZXDH_UAPI_DEF
static int UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_SHOW_RES_MAP)(
	struct uverbs_attr_bundle *attrs)
#else
static int UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_SHOW_RES_MAP)(
	struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct zxdh_pci_f *rf;
	struct zxdh_db_show_res_map_req show_res_map_req = { 0 };
	struct zxdh_db_show_res_map_resp show_res_map_resp = { 0 };
	int ret = 0;
	u32 qpn;

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif

	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);
	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);
	rf = iwdev->rf;

	if (iwdev->rf->use_ext_mem_flag)
	{
		pr_info("[zxdh_rdma] show_res_map not supported on current device!\n");
		return -ZXDH_NOT_SUPPORT_CURRENT_DEVICE;
	}

	ret = uverbs_copy_from(&show_res_map_req, attrs,
			       ZXDH_IB_ATTR_DEV_SHOW_RES_MAP);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	switch (show_res_map_req.type) {
	case ZXDH_SHOW_RES_MAP_PF_TO_QPN:
		ret = get_pf_qpn_reg_value(rf, &show_res_map_req,
					   &show_res_map_resp);
		break;
	case ZXDH_SHOW_RES_MAP_PF_TO_VHCA:
		ret = get_pf_vhca_reg_value(rf, &show_res_map_req,
					    &show_res_map_resp);
		break;
	case ZXDH_SHOW_RES_MAP_VHCA_TO_PF:
		ret = get_vhca_physical_reg_value(rf, &show_res_map_req,
						  &show_res_map_resp);
		break;
	case ZXDH_SHOW_RES_MAP_8K_TO_GQP:
		ret = get_8k_gqp_reg_value(rf, &show_res_map_req,
					   &show_res_map_resp);
		break;
	case ZXDH_SHOW_RES_MAP_GQP_TO_VHCA_CREATED:
		ret = get_gqp_8k_create_reg_value(rf, &show_res_map_req,
						  &show_res_map_resp);
		break;
	case ZXDH_SHOW_RES_MAP_GQP_TO_VHCA_ACTIVE:
		ret = get_gqp_8k_active_reg_value(rf, &show_res_map_req,
						  &show_res_map_resp);
		break;
	case ZXDH_SHOW_RES_MAP_QP_TO_8K:
		ret = validate_show_map_qpn(rf, &qpn, show_res_map_req.qp_id);
		if (ret)
			break;
		ret = get_qp_8k_reg_value(rf, qpn, &show_res_map_resp);
		break;
	default:
		pr_err("[zxdh_rdma] errro reg_type is %d, not support!\n",
		       show_res_map_req.type);
		return -EFAULT;
	}
	if (ret) {
		pr_err("[zxdh_rdma] zxdh_db_show_res_map: get reg value failed!\n");
		return ret;
	}

#ifdef RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
	ret = uverbs_copy_to_struct_or_zero(attrs,
					    ZXDH_IB_ATTR_DEV_SHOW_RES_MAP_RESP,
					    &show_res_map_resp,
					    sizeof(show_res_map_resp));
#else
	ret = zxdh_copy_to_struct_or_zero(attrs,
					  ZXDH_IB_ATTR_DEV_SHOW_RES_MAP_RESP,
					  &show_res_map_resp,
					  sizeof(show_res_map_resp));
#endif
	if (ret) {
		pr_err("[zxdh_rdma] ib_copy_to_udata failed!\n");
		return -EFAULT;
	}
	return 0;
}


static int read_ram_data(struct zxdh_device *iwdev,
			 struct zxdh_read_ram_req *req, u32 ram_max_index,
			 u32 *reg_values)
{
	struct read_ram_info ram_info = { 0 };
	ram_info.ram_num = req->ram_id;
	ram_info.ram_width = req->ram_width;
	ram_info.ram_read_cnt = req->read_count;
	ram_info.ram_addr = req->ram_addr;
	ram_info.offset_idx = ram_max_index;

	if (req->hw_module == HW_MODULE_TX) {
		return zxdh_read_ram_tx_values(&iwdev->rf->sc_dev, &ram_info,
					       reg_values);
	}
	if (req->hw_module == HW_MODULE_RX) {
		return zxdh_read_ram_rx_values(&iwdev->rf->sc_dev, &ram_info,
					       reg_values);
	}
	if (req->hw_module == HW_MODULE_CQP) {
		return zxdh_read_ram_cqp_values(&iwdev->rf->sc_dev, &ram_info,
					       reg_values);
	}
	return -1;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_READ_RAM)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_READ_RAM)(struct ib_uverbs_file *file,
					    struct uverbs_attr_bundle *attrs)
#endif
{
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct zxdh_read_ram_req req = { 0 };
	u32 *reg_values = NULL;
	int ret = 0;

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);
	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);
	ret = uverbs_copy_from(&req, attrs, ZXDH_IB_ATTR_DEV_READ_RAM);
	if (IS_UVERBS_COPY_ERR(ret))
		return -ZXDH_COPY_USER_PARAM_ERROR;
	reg_values =
		kmalloc(sizeof(u32) * ZXDH_READ_RAM_MAX_OFFSET, GFP_KERNEL);
	if (!reg_values) {
		pr_err("[zxdh_rdma] zxdh_read_ram error: allocate memory failed!\n");
		return -ENOMEM;
	}
	ret = read_ram_data(iwdev, &req, ZXDH_READ_RAM_MAX_OFFSET, reg_values);
	if (ret) {
		ret = -ZXDH_READ_RAM_ERROR;
		pr_err("[zxdh_rdma] zxdh_read_ram error: read ram reg failed!\n");
		goto cleanup;
	}
	ret = copy_to_user((void *)req.value_va, (void *)reg_values,
			   sizeof(u32) * ZXDH_READ_RAM_MAX_OFFSET);
	if (ret) {
		ret = -ZXDH_COPY_DATA_TO_USER_ERROR;
		pr_err("[zxdh_rdma] zxdh_read_ram error: copy ram data to user failed!\n");
	}
cleanup:
	if (reg_values) {
		kfree(reg_values);
	}
	return ret;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_GET_PEER_DEV_SBDF)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_GET_PEER_DEV_SBDF)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	u64 sbdf = 0;
	int ret = 0;

	u32 domain = 0;
	u32 bus    = 0;
	u32 device = 0; 
	u32 func   = 0;
	char sbdf_str[64];

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);
	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);

	if (!iwdev->rf->use_ext_mem_flag)
	{
		pr_info("[zxdh_rdma] not use ext mem, get peer dev sbdf not supported!\n");
		return -EACCES;
	}

	ret = zxdh_read_ext_mem_info(iwdev->rf,READ_FROM_BAR);  // 优先从BAR读取配置
	if (ret) {
		pr_info("[zxdh_rdma] Failed to read config from BAR: %d\n", ret);
		// bar空间读不到再从文件中获取
		ret = zxdh_read_ext_mem_info(iwdev->rf,READ_FROM_FILE);
		if (ret) {
			pr_info("[zxdh_rdma] No valid config file found, get peer dev sbdf failed\n");
			return -EINVAL;
		}	
	}
	sbdf = iwdev->rf->config.sbdf;
	
	domain = (sbdf >> 32) & 0xFFFFFFFF;
	bus    = (sbdf >> 16) & 0xFFFF;
	device = (sbdf >> 8) & 0xFF;   
	func   =  sbdf & 0x7;   
	zte_snprintf_s(sbdf_str, sizeof(sbdf_str), "%04x%02x%02x%x", domain, bus, device, func);
	zte_sscanf_s(sbdf_str, "%llx", &sbdf);

	pr_info("%s %d sbdf=0x%llx\n",__func__,__LINE__,sbdf);
	ret = uverbs_copy_to(attrs, ZXDH_IB_ATTR_GET_PEER_DEV_SBDF_RESP, &sbdf, sizeof(sbdf));
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	return 0;
}

DECLARE_UVERBS_NAMED_METHOD(ZXDH_IB_METHOD_GET_PEER_DEV_SBDF,
	UVERBS_ATTR_PTR_OUT(ZXDH_IB_ATTR_GET_PEER_DEV_SBDF_RESP,
                       UVERBS_ATTR_TYPE(u64), UA_MANDATORY));

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_QUERY_RDMA_SWITCH)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_QUERY_RDMA_SWITCH)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	int ret = 0;
	int status = 0;
	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct query_rdma_switch_req context_req = { 0 };
	struct query_rdma_switch_resp context_resp = { 0 };

#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	pr_info("[zxdh_rdma] ZXDH_IB_METHOD_DEV_QUERY_RDMA_SWITCH\n");
	if (IS_ERR(ib_uctx)) {
		pr_info("[zxdh_rdma] IS_ERR(ib_uctx)\n");
		return PTR_ERR(ib_uctx);
	}

	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);

	ret = uverbs_copy_from(&context_req, attrs, ZXDH_IB_ATTR_DEV_GET_SWITCH);
	if (IS_UVERBS_COPY_ERR(ret)) {
		pr_info("[zxdh_rdma] IS_UVERBS_COPY_ERR(ret)\n");
		return ret;
	}

	pr_info("[zxdh_rdma] context_req.type %u\n", context_req.type);
	switch (context_req.type) {
	case WQE_RATE_LIMIT:
		context_resp.value = iwdev->wqe_rate_limit;
		if(zxdh_req_query_rdma_switch(iwdev, WQE_RATE_LIMIT, &context_resp.next_reboot_value)) {
			status = -ENXIO;
			pr_err("[zxdh_rdma] get fw wqe_rate_limit failed!\n");
			context_resp.next_reboot_value = 0;
		}
		break;
	case ORD_MAX_SIZE:
		context_resp.value = iwdev->max_hw_ord;
		if(zxdh_req_query_rdma_switch(iwdev, ORD_MAX_SIZE, &context_resp.next_reboot_value)) {
			status = -ENXIO;
			pr_err("[zxdh_rdma] get fw ord_max_size failed!\n");
			context_resp.next_reboot_value = ICRDMA_DEFAULT_ORD_SIZE;
		}
		break;
	default:
		pr_err("[zxdh_rdma] unsupport switch type=%u!\n", context_req.type);
		return -EFAULT;
	}

	#ifdef RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
	ret = uverbs_copy_to_struct_or_zero(attrs, ZXDH_IB_ATTR_DEV_GET_SWITCH_RESP,
					&context_resp, sizeof(context_resp));
	#else
	ret = zxdh_copy_to_struct_or_zero(attrs, ZXDH_IB_ATTR_DEV_GET_SWITCH_RESP,
					&context_resp, sizeof(context_resp));
	#endif
	if (ret) {
		pr_err("[zxdh_rdma] zxdh_get_ctx ib_copy_to_udata failed!\n");
		return -EFAULT;
	}

	return (ret | status);
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_CONFIG_RDMA_SWITCH)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_DEV_CONFIG_RDMA_SWITCH)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	int ret = 0;

	struct ib_ucontext *ib_uctx;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct config_rdma_switch_req context_req = { 0 };

	pr_info("[zxdh_rdma] ZXDH_IB_METHOD_DEV_CONFIG_RDMA_SWITCH\n");
#ifdef ZXDH_UAPI_DEF
	ib_uctx = ib_uverbs_get_ucontext(attrs);
#else
	ib_uctx = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ib_uctx))
		return PTR_ERR(ib_uctx);
	ib_dev = ib_uctx->device;
	iwdev = to_iwdev(ib_dev);

	ret = uverbs_copy_from(&context_req, attrs, ZXDH_IB_ATTR_DEV_GET_SWITCH);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;
	pr_info("[zxdh_rdma] context_req.type=%u, context_req.value=%u\n", context_req.type, context_req.value);
	switch (context_req.type) {
	case WQE_RATE_LIMIT:
		if(zxdh_req_config_rdma_switch(iwdev, WQE_RATE_LIMIT, context_req.value) != STATUS_OK) {
			ret = -ESRCH;
		}
		break;
	case ORD_MAX_SIZE:
		if (!iwdev->fw_ver.patch) {
			pr_err("[zxdh_rdma] fw unsupport ord_max_size!\n");
			return -EINVAL;
		}
		if(zxdh_req_config_rdma_switch(iwdev, ORD_MAX_SIZE, context_req.value) != STATUS_OK) {
			ret = -ESRCH;
		}
		break;
	default:
		pr_err("[zxdh_rdma] unsupport switch type=%u!\n", context_req.type);
		return -EFAULT;
	}

	return ret;
}

#ifdef ZXDH_UAPI_DEF
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_CQ_MODIFY_OVERFLOW_CHECK_EN)(struct uverbs_attr_bundle *attrs)
#else
static int
UVERBS_HANDLER(ZXDH_IB_METHOD_CQ_MODIFY_OVERFLOW_CHECK_EN)(struct ib_uverbs_file *file, struct uverbs_attr_bundle *attrs)
#endif
{
	struct zxdh_cq *iwcq;
	struct zxdh_device *iwdev;
	struct ib_device *ib_dev;
	struct ib_ucontext *ucontext;
	struct zxdh_pci_f *rf;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_cqp_request *cqp_request;
	int ret;
	u32 modify_mask = 0;
	
	struct ib_cq *cq =
		uverbs_attr_get_obj(attrs, ZXDH_IB_ATTR_MODIFY_CQ_HANDLE);
#ifdef ZXDH_UAPI_DEF
	ucontext = ib_uverbs_get_ucontext(attrs);
#else
	ucontext = ib_uverbs_get_ucontext(attrs->ufile);
#endif
	if (IS_ERR(ucontext))
		return PTR_ERR(ucontext);

	ib_dev = ucontext->device;
	iwdev = to_iwdev(ib_dev);
	iwcq = to_iwcq(cq);
	rf = iwdev->rf;

	ret = uverbs_copy_from(&modify_mask, attrs, ZXDH_IB_ATTR_MODIFY_CQ_OVERFLOW_CHECK_EN);
	if (IS_UVERBS_COPY_ERR(ret))
		return ret;

	if (modify_mask != 0 && modify_mask != 1) {
		pr_err("[zxdh_rdma] zxdh_ib_method_cq_modify_overflow_check_en: modify_mask is out of range!\n");
		return -EINVAL;
	}

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request) 
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	cqp_info->cqp_cmd = ZXDH_OP_CQ_MODIFY_OVERFLOW_CHECK_EN;
	cqp_info->in.u.cq_modify.cq = &iwcq->sc_cq;
	cqp_info->in.u.cq_modify.scratch = (uintptr_t)cqp_request;
	cqp_info->post_sq = 1;

	cqp_info->in.u.cq_modify.cq->ceqe_mask = modify_mask;
	ret = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	if (ret)
		return ret;

	return ret;
}

void zxdh_init_rdma_tool_context(struct ib_ucontext *ib_uctx)
{
	struct zxdh_ucontext *ucontext = to_ucontext(ib_uctx);
	struct zxdh_rdma_tool_context *rdma_tool_context = NULL;

	rdma_tool_context = kzalloc(sizeof(*rdma_tool_context), GFP_KERNEL);
	if (!rdma_tool_context) {
		pr_err("[zxdh_rdma] zxdh_init_rdma_tool_context: failed to allocate rdma_tool_context\n");
		return;
	}

	rdma_tool_context->data_cap_on = false;
	rdma_tool_context->data_cap_position = 0;
	rdma_tool_context->data_cap_mem_type = 0;
	ucontext->rdma_tool_context = rdma_tool_context;
}

void zxdh_destroy_rdma_tool_context(struct ib_ucontext *ib_uctx)
{
	struct zxdh_ucontext *ucontext;
	struct zxdh_rdma_tool_context *rdma_tool_context = NULL;

	if (!ib_uctx)
		return;

	ucontext = to_ucontext(ib_uctx);
	rdma_tool_context = zxdh_get_rdma_tool_context(ucontext);

	if (!rdma_tool_context) {
		return;
	}

	if (rdma_tool_context->mp_cap_on) {
		mutex_lock(&mp_cap_reg_lock);
		zxdh_release_mp_resource(ucontext, rdma_tool_context);
		mutex_unlock(&mp_cap_reg_lock);
	}

	if (rdma_tool_context->data_cap_on) {
		zxdh_release_data_cap_resource(ucontext, rdma_tool_context);
	}

	/* Free HW object data if allocated */
	free_hw_object_data(ucontext);

	kfree(rdma_tool_context);
	ucontext->rdma_tool_context = NULL;
}

DECLARE_UVERBS_NAMED_METHOD(ZXDH_IB_METHOD_QP_RESET_QP,
			    UVERBS_ATTR_IDR(ZXDH_IB_ATTR_QP_RESET_QP_HANDLE,
					    UVERBS_OBJECT_QP,
					    UVERBS_ACCESS_READ, UA_MANDATORY),
			    UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_QP_RESET_OP_CODE,
					       UVERBS_ATTR_TYPE(u64),
					       UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
	ZXDH_IB_METHOD_QP_MODIFY_QPC,
	UVERBS_ATTR_IDR(ZXDH_IB_ATTR_QP_MODIFY_QPC_HANDLE, UVERBS_OBJECT_QP,
			UVERBS_ACCESS_READ, UA_MANDATORY),
	UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_QP_MODIFY_QPC_REQ,
			   UVERBS_ATTR_STRUCT(struct zxdh_modify_qpc_req,
					      package_err_flag),
			   UA_MANDATORY),
	UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_QP_MODIFY_QPC_MASK,
			   UVERBS_ATTR_TYPE(u64), UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
	ZXDH_IB_METHOD_QP_QUERY_QPC,
	UVERBS_ATTR_IDR(ZXDH_IB_ATTR_QP_QUERY_HANDLE, UVERBS_OBJECT_QP,
			UVERBS_ACCESS_READ, UA_MANDATORY),
	UVERBS_ATTR_PTR_OUT(ZXDH_IB_ATTR_QP_QUERY_RESP,
			    UVERBS_ATTR_STRUCT(struct zxdh_query_qpc_resp,
					       tx_last_ack_psn),
			    UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(ZXDH_IB_METHOD_QP_MODIFY_UDP_SPORT,
			    UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_QP_UDP_PORT,
					       UVERBS_ATTR_TYPE(u16),
					       UA_MANDATORY),
			    UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_QP_QPN,
					       UVERBS_ATTR_TYPE(u32),
					       UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(ZXDH_IB_METHOD_QP_SET_CREDIT_FLAG,
			    UVERBS_ATTR_IDR(ZXDH_IB_ATTR_QP_SET_CREDIT_FLAG_HANDLE,
					    UVERBS_OBJECT_QP,
					    UVERBS_ACCESS_READ, UA_MANDATORY),
			    UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_QP_CREDIT_FLAG,
					       UVERBS_ATTR_TYPE(u64),
					       UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
	ZXDH_IB_METHOD_DEV_GET_LOG_TRACE,
	UVERBS_ATTR_PTR_OUT(ZXDH_IB_ATTR_DEV_GET_LOG_TARCE_SWITCH,
			    UVERBS_ATTR_TYPE(u8), UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
	ZXDH_IB_METHOD_DEV_SET_LOG_TRACE,
	UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_SET_LOG_TARCE_SWITCH,
			   UVERBS_ATTR_TYPE(u8), UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
	ZXDH_IB_METHOD_DEV_CAP_START,
	UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_CAP_START,
			   UVERBS_ATTR_STRUCT(struct zxdh_cap_cfg,
					      cap_data_start_cap),
			   UA_MANDATORY),
	UVERBS_ATTR_PTR_OUT(ZXDH_IB_ATTR_DEV_CAP_START_RESP,
			    UVERBS_ATTR_STRUCT(struct zxdh_cap_start_resp,
					       cap_pa_node1),
			    UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(ZXDH_IB_METHOD_DEV_CAP_STOP,
			    UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_CAP_STOP,
					       UVERBS_ATTR_TYPE(u8),
					       UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(ZXDH_IB_METHOD_DEV_CAP_FREE,
			    UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_CAP_FREE,
					       UVERBS_ATTR_TYPE(u8),
					       UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
	ZXDH_IB_METHOD_DEV_MP_CAP,
	UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_MP_CAP,
			   UVERBS_ATTR_STRUCT(struct zxdh_mp_cap_cfg, qpn_num),
			   UA_MANDATORY),
	UVERBS_ATTR_PTR_OUT(ZXDH_IB_ATTR_DEV_MP_CAP_RESP,
			    UVERBS_ATTR_STRUCT(struct zxdh_mp_cap_resp, cap_pa),
			    UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
	ZXDH_IB_METHOD_DEV_MP_CAP_GQP,
	UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_MP_CAP_GQP,
			   UVERBS_ATTR_STRUCT(struct zxdh_mp_cap_gqp_cfg, is_vm_capture),
			   UA_MANDATORY),
	UVERBS_ATTR_PTR_OUT(ZXDH_IB_ATTR_DEV_MP_CAP_GQP_RESP,
			    UVERBS_ATTR_STRUCT(struct zxdh_mp_cap_resp, cap_pa),
			    UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(ZXDH_IB_METHOD_DEV_MP_GET_DATA,
			    UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_MP_GET_DATA,
					       UVERBS_ATTR_TYPE(u8),
					       UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
	ZXDH_IB_METHOD_DEV_MP_CAP_CLEAR,
	UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_MP_CAP_CLEAR,
			   UVERBS_ATTR_STRUCT(struct zxdh_cap_gqp, gqp_num),
			   UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
	ZXDH_IB_METHOD_DEV_GET_ACT_VHCA_GQPS,
	UVERBS_ATTR_PTR_OUT(ZXDH_IB_ATTR_DEV_GET_ACT_VHCA_GQPS_RESP,
			    UVERBS_ATTR_STRUCT(struct zxdh_active_vhca_gqps,
					       gqp_num),
			    UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
	ZXDH_IB_METHOD_DEV_GET_CC_BASIC_INFO,
	UVERBS_ATTR_PTR_OUT(ZXDH_IB_ATTR_DEV_GET_CC_BASIC_INFO_RESP,
			    UVERBS_ATTR_STRUCT(struct zxdh_cc_basic_info,
					       backpres_rx),
				UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(					   
	ZXDH_IB_METHOD_DEV_GET_HMC,
	UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_GET_HMC,
			   UVERBS_ATTR_STRUCT(struct zxdh_context_req, resource_id),
			   UA_MANDATORY),
	UVERBS_ATTR_PTR_OUT(ZXDH_IB_ATTR_DEV_GET_HMC_RESP,
			    UVERBS_ATTR_STRUCT(struct zxdh_context_resp, context_size),
			    UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
	ZXDH_IB_METHOD_DEV_GET_OBJ_DATA,
	UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_GET_OBJ_DATA,
			   UVERBS_ATTR_STRUCT(struct zxdh_get_object_data_req,
					      object_num),
			   UA_MANDATORY),
	UVERBS_ATTR_PTR_OUT(ZXDH_IB_ATTR_DEV_GET_OBJ_DATA_RESP,
			    UVERBS_ATTR_STRUCT(struct zxdh_get_object_data_resp,
					       route_id),
			    UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
	ZXDH_IB_METHOD_DEV_HEALTH_CHECK,                                
	UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_HEALTH_CHECK,
						UVERBS_ATTR_STRUCT(struct zxdh_health_check_req, reg_value_va_ex),
						UA_MANDATORY),
	UVERBS_ATTR_PTR_OUT(ZXDH_IB_ATTR_DEV_HEALTH_CHECK_RESP,
						UVERBS_ATTR_STRUCT(struct zxdh_health_check_resp, count_ex),
						UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
	ZXDH_IB_METHOD_DEV_CFG_PARAMETER,                                
	UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_CFG_PARAMETER,
						UVERBS_ATTR_STRUCT(struct zxdh_cfg_dev_parameter_req, reserved2),
						UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
    ZXDH_IB_METHOD_DEV_SHOW_RES_MAP,
    UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_SHOW_RES_MAP,
                       UVERBS_ATTR_STRUCT(struct zxdh_db_show_res_map_req, type),
                       UA_MANDATORY),
    UVERBS_ATTR_PTR_OUT(ZXDH_IB_ATTR_DEV_SHOW_RES_MAP_RESP,
                        UVERBS_ATTR_STRUCT(struct zxdh_db_show_res_map_resp, count),
                        UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(
    ZXDH_IB_METHOD_DEV_READ_RAM,
    UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_READ_RAM,
                       UVERBS_ATTR_STRUCT(struct zxdh_read_ram_req, reserved2),
                       UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(					   
	ZXDH_IB_METHOD_DEV_QUERY_RDMA_SWITCH,
	UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_GET_SWITCH,
			   UVERBS_ATTR_STRUCT(struct query_rdma_switch_req, type),
			   UA_MANDATORY),
	UVERBS_ATTR_PTR_OUT(ZXDH_IB_ATTR_DEV_GET_SWITCH_RESP,
			    UVERBS_ATTR_STRUCT(struct query_rdma_switch_resp, value),
			    UA_MANDATORY));

DECLARE_UVERBS_NAMED_METHOD(					   
	ZXDH_IB_METHOD_DEV_CONFIG_RDMA_SWITCH,
	UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_DEV_CONFIG_SWITCH,
			   UVERBS_ATTR_STRUCT(struct config_rdma_switch_req, type),
			   UA_MANDATORY));			

DECLARE_UVERBS_NAMED_METHOD(ZXDH_IB_METHOD_CQ_MODIFY_OVERFLOW_CHECK_EN,
			    UVERBS_ATTR_IDR(ZXDH_IB_ATTR_MODIFY_CQ_HANDLE,
					    UVERBS_OBJECT_CQ,
					    UVERBS_ACCESS_READ, UA_MANDATORY),
			    UVERBS_ATTR_PTR_IN(ZXDH_IB_ATTR_MODIFY_CQ_OVERFLOW_CHECK_EN,
					       UVERBS_ATTR_TYPE(u32),
					       UA_MANDATORY));

DECLARE_UVERBS_GLOBAL_METHODS(ZXDH_IB_OBJECT_DEV,
			      &UVERBS_METHOD(ZXDH_IB_METHOD_DEV_GET_LOG_TRACE),
			      &UVERBS_METHOD(ZXDH_IB_METHOD_DEV_SET_LOG_TRACE),
			      &UVERBS_METHOD(ZXDH_IB_METHOD_DEV_CAP_START),
			      &UVERBS_METHOD(ZXDH_IB_METHOD_DEV_CAP_STOP),
			      &UVERBS_METHOD(ZXDH_IB_METHOD_DEV_CAP_FREE),
			      &UVERBS_METHOD(ZXDH_IB_METHOD_DEV_MP_CAP),
			      &UVERBS_METHOD(ZXDH_IB_METHOD_DEV_MP_GET_DATA),
			      &UVERBS_METHOD(ZXDH_IB_METHOD_DEV_MP_CAP_CLEAR));

DECLARE_UVERBS_GLOBAL_METHODS(
	ZXDH_IB_OBJECT_DEVICE_EX,
	&UVERBS_METHOD(ZXDH_IB_METHOD_DEV_GET_ACT_VHCA_GQPS),
	&UVERBS_METHOD(ZXDH_IB_METHOD_DEV_GET_CC_BASIC_INFO),
	&UVERBS_METHOD(ZXDH_IB_METHOD_DEV_GET_HMC),
	&UVERBS_METHOD(ZXDH_IB_METHOD_DEV_GET_OBJ_DATA),
	&UVERBS_METHOD(ZXDH_IB_METHOD_DEV_HEALTH_CHECK),
	&UVERBS_METHOD(ZXDH_IB_METHOD_DEV_CFG_PARAMETER),
	&UVERBS_METHOD(ZXDH_IB_METHOD_DEV_SHOW_RES_MAP),
	&UVERBS_METHOD(ZXDH_IB_METHOD_DEV_READ_RAM),
	);

DECLARE_UVERBS_GLOBAL_METHODS(
	ZXDH_IB_OBJECT_DEVICE_OBJ,
	&UVERBS_METHOD(ZXDH_IB_METHOD_DEV_QUERY_RDMA_SWITCH),
	&UVERBS_METHOD(ZXDH_IB_METHOD_DEV_CONFIG_RDMA_SWITCH),
	);

DECLARE_UVERBS_GLOBAL_METHODS(ZXDH_IB_OBJECT_QP_OBJ,
				&UVERBS_METHOD(ZXDH_IB_METHOD_QP_MODIFY_UDP_SPORT),
				&UVERBS_METHOD(ZXDH_IB_METHOD_QP_QUERY_QPC),
				&UVERBS_METHOD(ZXDH_IB_METHOD_QP_MODIFY_QPC),
				&UVERBS_METHOD(ZXDH_IB_METHOD_QP_RESET_QP),
				&UVERBS_METHOD(ZXDH_IB_METHOD_QP_SET_CREDIT_FLAG));

DECLARE_UVERBS_GLOBAL_METHODS(ZXDH_IB_OBJECT_CQ_OBJ,
			      &UVERBS_METHOD(ZXDH_IB_METHOD_CQ_MODIFY_OVERFLOW_CHECK_EN));

DECLARE_UVERBS_GLOBAL_METHODS(ZXDH_IB_OBJECT_DEVICE_PRIV,
			      &UVERBS_METHOD(ZXDH_IB_METHOD_GET_PEER_DEV_SBDF),
			      &UVERBS_METHOD(ZXDH_IB_METHOD_DEV_MP_CAP_GQP));

#ifdef ZXDH_UAPI_DEF
const struct uapi_definition zxdh_ib_dev_defs[] = {
	UAPI_DEF_CHAIN_OBJ_TREE_NAMED(ZXDH_IB_OBJECT_DEV),
	UAPI_DEF_CHAIN_OBJ_TREE_NAMED(ZXDH_IB_OBJECT_QP_OBJ),
	UAPI_DEF_CHAIN_OBJ_TREE_NAMED(ZXDH_IB_OBJECT_DEVICE_EX),
	UAPI_DEF_CHAIN_OBJ_TREE_NAMED(ZXDH_IB_OBJECT_CQ_OBJ),
	UAPI_DEF_CHAIN_OBJ_TREE_NAMED(ZXDH_IB_OBJECT_DEVICE_OBJ),
	UAPI_DEF_CHAIN_OBJ_TREE_NAMED(ZXDH_IB_OBJECT_DEVICE_PRIV),
	{},
};
#else
DECLARE_UVERBS_OBJECT_TREE(devx_objects,
			   &UVERBS_OBJECT(ZXDH_IB_OBJECT_DEV),
			   &UVERBS_OBJECT(ZXDH_IB_OBJECT_QP_OBJ),
			   &UVERBS_OBJECT(ZXDH_IB_OBJECT_DEVICE_EX),
			   &UVERBS_OBJECT(ZXDH_IB_OBJECT_CQ_OBJ),
			   &UVERBS_OBJECT(ZXDH_IB_OBJECT_DEVICE_OBJ),
			   &UVERBS_OBJECT(ZXDH_IB_OBJECT_DEVICE_PRIV));

const struct uverbs_object_tree_def *zxdh_ib_get_devx_tree(void)
{
	return &devx_objects;
}
#endif
