// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include <uapi/rdma/rdma_netlink.h>
#ifndef RDMA_MMAP_DB_SUPPORT
#include <linux/hashtable.h>
#endif
#include <linux/skbuff.h>
#include <rdma/restrack.h>
#include "restrack.h"
#include "main.h"

#ifdef IB_DEV_OPS_FILL_ENTRY

static int fill_res_mr_entry(struct sk_buff *msg, struct ib_mr *ibmr)
{
	struct zxdh_mr *mr = to_iwmr(ibmr);
	struct nlattr *table_attr;

	table_attr = nla_nest_start(msg, RDMA_NLDEV_ATTR_DRIVER);
	if (!table_attr)
		goto err;
	switch (mr->type) {
	case ZXDH_MEMREG_TYPE_MEM:
		if (rdma_nl_put_driver_string(msg, "type", "mem"))
			goto err;
		break;
	case ZXDH_MEMREG_TYPE_QP:
		if (rdma_nl_put_driver_string(msg, "type", "qp"))
			goto err;
		break;
	case ZXDH_MEMREG_TYPE_CQ:
		if (rdma_nl_put_driver_string(msg, "type", "cq"))
			goto err;
		break;
	case ZXDH_MEMREG_TYPE_SRQ:
		if (rdma_nl_put_driver_string(msg, "type", "srq"))
			goto err;
		break;
	default:
		goto err;
	}
	nla_nest_end(msg, table_attr);
	return 0;

err:
	pr_err("[zxdh_rdma] res mr entry failed\n");
	nla_nest_cancel(msg, table_attr);
	return -EMSGSIZE;
}

static int fill_res_mr_entry_raw(struct sk_buff *msg, struct ib_mr *ibmr)
{
	struct zxdh_mr *iwmr = to_iwmr(ibmr);
	struct zxdh_device *iwdev = to_iwdev(ibmr->device);
	struct zxdh_src_copy_dest src_dest = { 0 };
	struct zxdh_dma_mem qpc_buf = { 0 };
	int err_code = 0;

	qpc_buf.size = 64;
	qpc_buf.va = zxdh_alloc_raw_mem(iwdev, qpc_buf.size, &qpc_buf.pa);
	if (!qpc_buf.va) {
		pr_err("[zxdh_rdma] res mr entry raw failed:ENOMEM\n");
		return -ENOMEM;
	}
	if (iwmr->type != ZXDH_MEMREG_TYPE_MEM) {
		err_code = nla_put(msg, RDMA_NLDEV_ATTR_RES_RAW, 0, qpc_buf.va);
		goto free_buff;
	}
	src_dest.src = 64 * (iwmr->stag >> ZXDH_CQPSQ_STAG_IDX_S);
	src_dest.dest = qpc_buf.pa;
	src_dest.len = qpc_buf.size;
	err_code = zxdh_cqp_rdma_read_mrte_cmd(&iwdev->rf->sc_dev, &src_dest);
	if (err_code) {
		pr_err("[zxdh_rdma] res qp entry raw fill qpc failed:%d\n", err_code);
		goto free_buff;
	}
	err_code =
		nla_put(msg, RDMA_NLDEV_ATTR_RES_RAW, qpc_buf.size, qpc_buf.va);
free_buff:
	zxdh_free_raw_mem(iwdev, qpc_buf.va, qpc_buf.size, qpc_buf.pa);
	qpc_buf.va = NULL;
	return err_code;
}

static int fill_res_qp_entry_raw(struct sk_buff *msg, struct ib_qp *ibqp)
{
	struct zxdh_qp *iwqp = to_iwqp(ibqp);
	struct zxdh_device *iwdev = iwqp->iwdev;
	struct zxdh_dma_mem qpc_buf;
	int err_code = 0;

	qpc_buf.va = NULL;
	qpc_buf.size = ALIGN(ZXDH_QP_CTX_SIZE, ZXDH_QPC_ALIGNMENT);
	qpc_buf.va = zxdh_alloc_raw_mem(iwdev, qpc_buf.size, &qpc_buf.pa);
	if (!qpc_buf.va) {
		pr_err("[zxdh_rdma] res qp entry raw failed:ENOMEM\n");
		return -ENOMEM;
	}
	err_code = zxdh_fill_qpc(&iwdev->rf->sc_dev, iwqp->sc_qp.qp_ctx_num, &qpc_buf);
	if (err_code) {
		pr_err("[zxdh_rdma] res qp entry raw fill qpc failed:%d\n", err_code);
		goto free_buff;
	}
	err_code =
		nla_put(msg, RDMA_NLDEV_ATTR_RES_RAW, qpc_buf.size, qpc_buf.va);
free_buff:
	zxdh_free_raw_mem(iwdev, qpc_buf.va, qpc_buf.size, qpc_buf.pa);
	qpc_buf.va = NULL;
	return err_code;
}

static int fill_res_cq_entry_raw(struct sk_buff *msg, struct ib_cq *ibcq)
{
	struct zxdh_cq *iwcq = to_iwcq(ibcq);
	struct zxdh_device *iwdev = to_iwdev(ibcq->device);
	struct zxdh_dma_mem cqc_buf;
	int err_code = 0;

	cqc_buf.va = NULL;
	cqc_buf.size = ALIGN(ZXDH_CQ_CTX_SIZE, ZXDH_CQC_ALIGNMENT);
	cqc_buf.va = zxdh_alloc_raw_mem(iwdev, cqc_buf.size, &cqc_buf.pa);
	if (!cqc_buf.va) {
		pr_err("[zxdh_rdma] res cq entry raw failed:ENOMEM\n");
		return -ENOMEM;
	}
	err_code = zxdh_fill_cqc(&iwdev->rf->sc_dev, iwcq->sc_cq.cq_uk.cq_id, &cqc_buf);
	if (err_code) {
		pr_err("[zxdh_rdma] res cq entry raw fill cqc failed:%d\n", err_code);
		goto free_buff;
	}
	err_code =
		nla_put(msg, RDMA_NLDEV_ATTR_RES_RAW, cqc_buf.size, cqc_buf.va);
free_buff:
	zxdh_free_raw_mem(iwdev, cqc_buf.va, cqc_buf.size, cqc_buf.pa);
	cqc_buf.va = NULL;
	return err_code;
}

static const struct ib_device_ops restrack_ops = {
	.fill_res_cq_entry_raw = fill_res_cq_entry_raw,
	.fill_res_mr_entry = fill_res_mr_entry,
	.fill_res_qp_entry_raw = fill_res_qp_entry_raw,
	.fill_res_mr_entry_raw = fill_res_mr_entry_raw,

};

int zxdh_set_restrack_ops(struct ib_device *ibdev)
{
	ib_set_device_ops(ibdev, &restrack_ops);
	return 0;
}
#endif
