/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_USER_IOCTL_CMDS_H
#define ZXDH_USER_IOCTL_CMDS_H

#include <linux/types.h>
#include <rdma/ib_user_ioctl_cmds.h>

enum zxdh_ib_dev_get_log_trace_attrs {
	ZXDH_IB_ATTR_DEV_GET_LOG_TARCE_SWITCH = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_dev_set_log_trace_attrs {
	ZXDH_IB_ATTR_DEV_SET_LOG_TARCE_SWITCH = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_dev_cap_start {
	ZXDH_IB_ATTR_DEV_CAP_START = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_DEV_CAP_START_RESP,
};

enum zxdh_ib_dev_cap_stop {
	ZXDH_IB_ATTR_DEV_CAP_STOP = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_dev_cap_free {
	ZXDH_IB_ATTR_DEV_CAP_FREE = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_dev_mp_cap {
	ZXDH_IB_ATTR_DEV_MP_CAP = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_DEV_MP_CAP_RESP,
};

enum zxdh_ib_dev_mp_cap_gqp {
	ZXDH_IB_ATTR_DEV_MP_CAP_GQP = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_DEV_MP_CAP_GQP_RESP,
};

enum zxdh_ib_dev_mp_get_data {
	ZXDH_IB_ATTR_DEV_MP_GET_DATA = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_dev_mp_cap_clear {
	ZXDH_IB_ATTR_DEV_MP_CAP_CLEAR = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_dev_get_act_vhca_gqps {
	ZXDH_IB_ATTR_DEV_GET_ACT_VHCA_GQPS_RESP = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_dev_get_cc_basic_info {
	ZXDH_IB_ATTR_DEV_GET_CC_BASIC_INFO_RESP = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_dev_get_hmc {
	ZXDH_IB_ATTR_DEV_GET_HMC = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_DEV_GET_HMC_RESP,
};

enum zxdh_ib_dev_get_obj_data {
	ZXDH_IB_ATTR_DEV_GET_OBJ_DATA = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_DEV_GET_OBJ_DATA_RESP,
};

enum zxdh_ib_dev_health_check {
	ZXDH_IB_ATTR_DEV_HEALTH_CHECK = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_DEV_HEALTH_CHECK_RESP,
};

enum zxdh_ib_dev_cfg_parameter {
	ZXDH_IB_ATTR_DEV_CFG_PARAMETER = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_dev_show_res_map {
	ZXDH_IB_ATTR_DEV_SHOW_RES_MAP = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_DEV_SHOW_RES_MAP_RESP,
};

enum zxdh_ib_dev_read_ram {
	ZXDH_IB_ATTR_DEV_READ_RAM = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_dev_query_switch {
	ZXDH_IB_ATTR_DEV_GET_SWITCH = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_DEV_GET_SWITCH_RESP,
};

enum zxdh_ib_dev_config_switch {
	ZXDH_IB_ATTR_DEV_CONFIG_SWITCH = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_dev_devx_modify_cq {
	ZXDH_IB_ATTR_DEVX_MODIFY_CQ = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_dev_methods {
	ZXDH_IB_METHOD_DEV_GET_LOG_TRACE = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_METHOD_DEV_SET_LOG_TRACE,
	ZXDH_IB_METHOD_DEV_CAP_START,
	ZXDH_IB_METHOD_DEV_CAP_STOP,
	ZXDH_IB_METHOD_DEV_CAP_FREE,
	ZXDH_IB_METHOD_DEV_MP_CAP,
	ZXDH_IB_METHOD_DEV_MP_GET_DATA,
	ZXDH_IB_METHOD_DEV_MP_CAP_CLEAR,
};

enum zxdh_ib_dev_priv_methods {
	ZXDH_IB_METHOD_GET_PEER_DEV_SBDF = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_METHOD_DEV_MP_CAP_GQP,
};

enum zxdh_ib_device_methods {
	ZXDH_IB_METHOD_DEV_GET_ACT_VHCA_GQPS = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_METHOD_DEV_GET_CC_BASIC_INFO,
	ZXDH_IB_METHOD_DEV_GET_HMC,
	ZXDH_IB_METHOD_DEV_GET_OBJ_DATA,
	ZXDH_IB_METHOD_DEV_HEALTH_CHECK,
	ZXDH_IB_METHOD_DEV_CFG_PARAMETER,
	ZXDH_IB_METHOD_DEV_SHOW_RES_MAP,
	ZXDH_IB_METHOD_DEV_READ_RAM
};

struct zxdh_get_peer_dev_sbdf_resp {
	__u64 sbdf;
};

enum zxdh_ib_get_peer_dev_sbdf {
	ZXDH_IB_ATTR_GET_PEER_DEV_SBDF_RESP = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_device_switch_methods {
	ZXDH_IB_METHOD_DEV_QUERY_RDMA_SWITCH = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_METHOD_DEV_CONFIG_RDMA_SWITCH,
};

enum zxdh_ib_qp_modify_udp_sport_attrs {
	ZXDH_IB_ATTR_QP_UDP_PORT = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_QP_QPN,
};

enum zxdh_ib_qp_query_qpc_attrs {
	ZXDH_IB_ATTR_QP_QUERY_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_QP_QUERY_RESP,
};

enum zxdh_ib_qp_modify_qpc_attrs {
	ZXDH_IB_ATTR_QP_MODIFY_QPC_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_QP_MODIFY_QPC_REQ,
	ZXDH_IB_ATTR_QP_MODIFY_QPC_MASK,
};

enum zxdh_ib_qp_reset_qp_attrs {
	ZXDH_IB_ATTR_QP_RESET_QP_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_QP_RESET_OP_CODE,
};

enum zxdh_ib_qp_credit_flag_attrs {
	ZXDH_IB_ATTR_QP_SET_CREDIT_FLAG_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_QP_CREDIT_FLAG,
};

enum zxdh_ib_qp_methods {
	ZXDH_IB_METHOD_QP_MODIFY_UDP_SPORT = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_METHOD_QP_QUERY_QPC,
	ZXDH_IB_METHOD_QP_MODIFY_QPC,
	ZXDH_IB_METHOD_QP_RESET_QP,
	ZXDH_IB_METHOD_QP_SET_CREDIT_FLAG,
};

enum zxdh_ib_cq_modify_overflow_check_en_attrs {
	ZXDH_IB_ATTR_MODIFY_CQ_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_ATTR_MODIFY_CQ_OVERFLOW_CHECK_EN,
};

enum zxdh_ib_cq_methods {
	ZXDH_IB_METHOD_CQ_MODIFY_OVERFLOW_CHECK_EN = (1U << UVERBS_ID_NS_SHIFT),
};

enum zxdh_ib_objects {
	ZXDH_IB_OBJECT_DEV = (1U << UVERBS_ID_NS_SHIFT),
	ZXDH_IB_OBJECT_QP_OBJ,
	ZXDH_IB_OBJECT_DEVICE_EX,
	ZXDH_IB_OBJECT_CQ_OBJ,
	ZXDH_IB_OBJECT_DEVICE_OBJ,
	ZXDH_IB_OBJECT_DEVICE_PRIV,
};

#endif