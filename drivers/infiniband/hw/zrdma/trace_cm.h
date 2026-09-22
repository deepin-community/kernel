/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#if !defined(__TRACE_CM_H) || defined(TRACE_HEADER_MULTI_READ)
#define __TRACE_CM_H

#include <linux/tracepoint.h>
#include <linux/trace_seq.h>

#include "main.h"

const char *print_ip_addr(struct trace_seq *p, u32 *addr, u16 port, bool ivp4);
const char *parse_iw_event_type(enum iw_cm_event_type iw_type);
const char *parse_cm_event_type(enum zxdh_cm_event_type cm_type);
const char *parse_cm_state(enum zxdh_cm_node_state);
#define __print_ip_addr(addr, port, ipv4) print_ip_addr(p, addr, port, ipv4)

#undef TRACE_SYSTEM
#define TRACE_SYSTEM zxdh_cm

DECLARE_EVENT_CLASS(
	cm_node_ah_template, TP_PROTO(struct zxdh_cm_node *cm_node),
	TP_ARGS(cm_node),
	TP_STRUCT__entry(
		__field(struct zxdh_device *,
			iwdev) __field(struct zxdh_cm_node *,
				       cm_node) __field(struct zxdh_sc_ah *, ah)
			__field(u32, refcount) __field(u16, lport) __field(
				u16, rport) __field(enum zxdh_cm_node_state,
						    state) __field(bool, ipv4)
				__field(u16, vlan_id) __field(int, accel)
					__dynamic_array(u32, laddr, 4)
						__dynamic_array(u32, raddr, 4)),
	TP_fast_assign(__entry->iwdev = cm_node->iwdev;
		       __entry->cm_node = cm_node; __entry->ah = cm_node->ah;
		       __entry->refcount = refcount_read(&cm_node->refcnt);
		       __entry->lport = cm_node->loc_port;
		       __entry->rport = cm_node->rem_port;
		       __entry->state = cm_node->state;
		       __entry->ipv4 = cm_node->ipv4;
		       __entry->vlan_id = cm_node->vlan_id;
		       __entry->accel = cm_node->accelerated;
		       memcpy(__get_dynamic_array(laddr), cm_node->loc_addr, 4);
		       memcpy(__get_dynamic_array(raddr), cm_node->rem_addr,
			      4);),
	TP_printk(
		"iwdev=%p  node=%p  ah=%p  refcnt=%d  vlan_id=%d  accel=%d  state=%s loc: %s  rem: %s",
		__entry->iwdev, __entry->cm_node, __entry->ah,
		__entry->refcount, __entry->vlan_id, __entry->accel,
		parse_cm_state(__entry->state),
		__print_ip_addr(__get_dynamic_array(laddr), __entry->lport,
				__entry->ipv4),
		__print_ip_addr(__get_dynamic_array(raddr), __entry->rport,
				__entry->ipv4)));

DEFINE_EVENT(cm_node_ah_template, zxdh_create_ah,
	     TP_PROTO(struct zxdh_cm_node *cm_node), TP_ARGS(cm_node));

#endif /* __TRACE_CM_H */

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE trace_cm
#include <trace/define_trace.h>
