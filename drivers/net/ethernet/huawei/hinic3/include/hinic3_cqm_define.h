/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_CQM_DEFINE_H
#define HINIC3_CQM_DEFINE_H
#if !defined(HIUDK_ULD) && !defined(HIUDK_SDK_ADPT)
#define cqm_init	cqm3_init
#define cqm_uninit	cqm3_uninit
#define cqm_service_register	cqm3_service_register
#define cqm_service_unregister	cqm3_service_unregister
#define cqm_bloomfilter_dec	cqm3_bloomfilter_dec
#define cqm_bloomfilter_inc	cqm3_bloomfilter_inc
#define cqm_cmd_alloc	cqm3_cmd_alloc
#define cqm_cmd_free	cqm3_cmd_free
#define cqm_send_cmd_box	cqm3_send_cmd_box
#define cqm_lb_send_cmd_box	cqm3_lb_send_cmd_box
#define cqm_lb_send_cmd_box_async cqm3_lb_send_cmd_box_async
#define cqm_db_addr_alloc	cqm3_db_addr_alloc
#define cqm_db_addr_free	cqm3_db_addr_free
#define cqm_ring_hardware_db	cqm3_ring_hardware_db
#define cqm_ring_software_db	cqm3_ring_software_db
#define cqm_object_fc_srq_create	cqm3_object_fc_srq_create
#define cqm_object_share_recv_queue_create	cqm3_object_share_recv_queue_create
#define cqm_object_share_recv_queue_add_container	cqm3_object_share_recv_queue_add_container
#define cqm_object_srq_add_container_free	cqm3_object_srq_add_container_free
#define cqm_object_recv_queue_create	cqm3_object_recv_queue_create
#define cqm_object_qpc_mpt_create	cqm3_object_qpc_mpt_create
#define cqm_object_nonrdma_queue_create	cqm3_object_nonrdma_queue_create
#define cqm_object_rdma_queue_create	cqm3_object_rdma_queue_create
#define cqm_object_rdma_table_get	cqm3_object_rdma_table_get
#define cqm_object_delete	cqm3_object_delete
#define cqm_object_offset_addr	cqm3_object_offset_addr
#define cqm_object_get	cqm3_object_get
#define cqm_object_put	cqm3_object_put
#define cqm_object_funcid	cqm3_object_funcid
#define cqm_object_resize_alloc_new	cqm3_object_resize_alloc_new
#define cqm_object_resize_free_new	cqm3_object_resize_free_new
#define cqm_object_resize_free_old	cqm3_object_resize_free_old
#define cqm_function_timer_clear	cqm3_function_timer_clear
#define cqm_function_hash_buf_clear	cqm3_function_hash_buf_clear
#define cqm_srq_used_rq_container_delete	cqm3_srq_used_rq_container_delete
#define cqm_timer_base cqm3_timer_base
#define cqm_get_db_addr                    cqm3_get_db_addr
#define cqm_ring_direct_wqe_db             cqm3_ring_direct_wqe_db
#define cqm_fake_vf_num_set                cqm3_fake_vf_num_set
#define cqm_need_secure_mem                cqm3_need_secure_mem
#endif
#endif
