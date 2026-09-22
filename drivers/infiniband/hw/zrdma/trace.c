// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */
#define CREATE_TRACE_POINTS
#include "trace.h"

const char *print_ip_addr(struct trace_seq *p, u32 *addr, u16 port, bool ipv4)
{
	const char *ret = trace_seq_buffer_ptr(p);

	if (ipv4) {
		__be32 myaddr = htonl(*addr);

		trace_seq_printf(p, "%pI4:%d", &myaddr, htons(port));
	} else {
		trace_seq_printf(p, "%pI6:%d", addr, htons(port));
	}
	trace_seq_putc(p, 0);

	return ret;
}

const char *parse_iw_event_type(enum iw_cm_event_type iw_type)
{
	switch (iw_type) {
	case IW_CM_EVENT_CONNECT_REQUEST:
		return "IwRequest";
	case IW_CM_EVENT_CONNECT_REPLY:
		return "IwReply";
	case IW_CM_EVENT_ESTABLISHED:
		return "IwEstablished";
	case IW_CM_EVENT_DISCONNECT:
		return "IwDisconnect";
	case IW_CM_EVENT_CLOSE:
		return "IwClose";
	}

	return "Unknown";
}

const char *parse_cm_event_type(enum zxdh_cm_event_type cm_type)
{
	switch (cm_type) {
	case ZXDH_CM_EVENT_ESTABLISHED:
		return "CmEstablished";
	case ZXDH_CM_EVENT_MPA_REQ:
		return "CmMPA_REQ";
	case ZXDH_CM_EVENT_MPA_CONNECT:
		return "CmMPA_CONNECT";
	case ZXDH_CM_EVENT_MPA_ACCEPT:
		return "CmMPA_ACCEPT";
	case ZXDH_CM_EVENT_MPA_REJECT:
		return "CmMPA_REJECT";
	case ZXDH_CM_EVENT_MPA_ESTABLISHED:
		return "CmMPA_ESTABLISHED";
	case ZXDH_CM_EVENT_CONNECTED:
		return "CmConnected";
	case ZXDH_CM_EVENT_RESET:
		return "CmReset";
	case ZXDH_CM_EVENT_ABORTED:
		return "CmAborted";
	case ZXDH_CM_EVENT_UNKNOWN:
		return "none";
	}
	return "Unknown";
}

const char *parse_cm_state(enum zxdh_cm_node_state state)
{
	switch (state) {
	case ZXDH_CM_STATE_UNKNOWN:
		return "UNKNOWN";
	case ZXDH_CM_STATE_INITED:
		return "INITED";
	case ZXDH_CM_STATE_LISTENING:
		return "LISTENING";
	case ZXDH_CM_STATE_SYN_RCVD:
		return "SYN_RCVD";
	case ZXDH_CM_STATE_SYN_SENT:
		return "SYN_SENT";
	case ZXDH_CM_STATE_ONE_SIDE_ESTABLISHED:
		return "ONE_SIDE_ESTABLISHED";
	case ZXDH_CM_STATE_ESTABLISHED:
		return "ESTABLISHED";
	case ZXDH_CM_STATE_ACCEPTING:
		return "ACCEPTING";
	case ZXDH_CM_STATE_MPAREQ_SENT:
		return "MPAREQ_SENT";
	case ZXDH_CM_STATE_MPAREQ_RCVD:
		return "MPAREQ_RCVD";
	case ZXDH_CM_STATE_MPAREJ_RCVD:
		return "MPAREJ_RECVD";
	case ZXDH_CM_STATE_OFFLOADED:
		return "OFFLOADED";
	case ZXDH_CM_STATE_FIN_WAIT1:
		return "FIN_WAIT1";
	case ZXDH_CM_STATE_FIN_WAIT2:
		return "FIN_WAIT2";
	case ZXDH_CM_STATE_CLOSE_WAIT:
		return "CLOSE_WAIT";
	case ZXDH_CM_STATE_TIME_WAIT:
		return "TIME_WAIT";
	case ZXDH_CM_STATE_LAST_ACK:
		return "LAST_ACK";
	case ZXDH_CM_STATE_CLOSING:
		return "CLOSING";
	case ZXDH_CM_STATE_LISTENER_DESTROYED:
		return "LISTENER_DESTROYED";
	case ZXDH_CM_STATE_CLOSED:
		return "CLOSED";
	}
	return ("Bad state");
}
