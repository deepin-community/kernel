// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include "ne6xvf.h"
#include "ne6xvf_osdep.h"

int ne6xvf_sdk_send_msg_to_pf(struct ne6xvf_hw *hw, enum virtchnl_ops v_opcode,
			      enum virtchnl_status_code v_retval, u8 *msg, u16 msglen,
			      void *cmd_details)
{
	union u_ne6x_mbx_snap_buffer_data mbx_buffer;

	ne6xvf_acquire_spinlock(&hw->mbx.mbx_spinlock);

	mbx_buffer.snap.data[0] = 0;
	mbx_buffer.snap.data[1] = 0;
	mbx_buffer.snap.data[2] = 0;
	mbx_buffer.snap.data[3] = 0;
	mbx_buffer.snap.data[4] = 0;
	mbx_buffer.snap.data[5] = 0;

	if (msglen) {
		if (msglen > NE6XVF_SDK_LARGE_BUF) {
			ne6xvf_release_spinlock(&hw->mbx.mbx_spinlock);
			return NE6XVF_ERR_INVALID_SIZE;
		}

		memcpy(mbx_buffer.snap.data, msg, msglen);
	}

	mbx_buffer.snap.len = msglen;
	mbx_buffer.snap.type = v_opcode;
	mbx_buffer.snap.state = v_retval;

	NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(0, NE6XVF_MAILBOX_DATA), mbx_buffer.val);
	NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(0, NE6XVF_DB_STATE), 0x2);

	ne6xvf_release_spinlock(&hw->mbx.mbx_spinlock);

	return 0;
}

int ne6xvf_send_pf_msg(struct ne6xvf_adapter *adapter, enum virtchnl_ops op, u8 *msg, u16 len)
{
	struct ne6xvf_hw *hw = &adapter->hw;
	int err;

	if (adapter->flags & NE6XVF_FLAG_PF_COMMS_FAILED)
		return 0; /* nothing to see here, move along */

	err = ne6xvf_sdk_send_msg_to_pf(hw, op, VIRTCHNL_STATUS_SUCCESS, msg, len, NULL);
	if (err)
		dev_dbg(&adapter->pdev->dev, "Unable to send opcode %d to PF, err %d, sdk_err %s\n",
			op, err, hw->err_str);

	return err;
}

/**
 *  ne6xvf_clean_arq_element
 *  @hw: pointer to the hw struct
 *  @e: event info from the receive descriptor, includes any buffers
 *  @pending: number of events that could be left to process
 *
 *  This function cleans one Admin Receive Queue element and returns
 *  the contents through e.  It can also return how many events are
 *  left to process through 'pending'
 **/
enum ne6xvf_status ne6xvf_clean_arq_element(struct ne6xvf_hw *hw, struct ne6xvf_arq_event_info *e,
					    u16 *pending)
{
	union u_ne6x_mbx_snap_buffer_data usnap;
	enum ne6xvf_status ret_code = 0;
	u64 val;
	int i;

	ne6xvf_acquire_spinlock(&hw->mbx.mbx_spinlock);
	val = NE6XVF_READ_REG(hw, NE6XVF_REG_ADDR(0, NE6X_VP_INT));
	if (val & 0x1)
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(0, NE6X_VP_INT), 0x1);

	if (!(val & 0x2)) {
		ne6xvf_release_spinlock(&hw->mbx.mbx_spinlock);
		return NE6XVF_ERR_NOT_READY;
	}

	usnap.val = NE6XVF_READ_REG(hw, NE6XVF_REG_ADDR(0, NE6XVF_PF_MAILBOX_DATA));
	e->msg_len = min_t(u16, (u16)usnap.snap.len, e->buf_len);
	if (e->msg_buf && e->msg_len != 0) {
		for (i = 0; i < e->msg_len && i < NE6XVF_SDK_LARGE_BUF; i++) {
			e->msg_buf[i] = usnap.snap.data[i];
			e->snap.data[i] = usnap.snap.data[i];
		}
	}

	e->snap.type = usnap.snap.type;
	e->snap.state = usnap.snap.state;

	if (pending)
		*pending = 0;

	NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(0, NE6X_VP_INT), 0x2);
	NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(0, NE6XVF_DB_STATE), 0x1);

	ne6xvf_release_spinlock(&hw->mbx.mbx_spinlock);
	return ret_code;
}

/**
 * ne6xvf_poll_virtchnl_msg - poll for virtchnl msg matching the requested_op
 * @adapter: adapter structure
 * @event: event to populate on success
 * @op_to_poll: requested virtchnl op to poll for
 */
int ne6xvf_poll_virtchnl_msg(struct ne6xvf_adapter *adapter, struct ne6xvf_arq_event_info *event,
			     enum virtchnl_ops op_to_poll)
{
	struct ne6xvf_arq_event_info rece_event;
	struct ne6xvf_hw *hw = &adapter->hw;
	enum ne6xvf_status status, v_ret;
	enum virtchnl_ops received_op;
	int timeout = 50000;
	int i;

	rece_event.buf_len = NE6XVF_MAX_AQ_BUF_SIZE;
	rece_event.msg_buf = kzalloc(rece_event.buf_len, GFP_KERNEL);
	if (!rece_event.msg_buf)
		return NE6XVF_ERR_NO_MEMORY;

	while (1) {
		/* When the SDK is empty, ne6xvf_clean_arq_element will return
		 * nonzero and this loop will terminate.
		 */
		status = ne6xvf_clean_arq_element(hw, &rece_event, NULL);
		if (status) {
			if (status == NE6XVF_ERR_NOT_READY && timeout) {
				usleep_range(10, 12);
				timeout--;
				continue;
			}
			kfree(rece_event.msg_buf);
			return status;
		}

		received_op = (enum virtchnl_ops)le32_to_cpu(rece_event.snap.type);
		v_ret = (enum ne6xvf_status)le32_to_cpu(rece_event.snap.state);
		if (op_to_poll == received_op) {
			memcpy(&event->snap, &rece_event.snap,
			       sizeof(struct ne6x_mbx_snap_buffer_data));
			event->msg_len = min(rece_event.msg_len, event->buf_len);
			if (event->msg_buf) {
				for (i = 0; i < event->msg_len && i < NE6XVF_SDK_LARGE_BUF; i++)
					event->msg_buf[i] = rece_event.msg_buf[i];
			}
			break;
		}

		ne6xvf_virtchnl_completion(adapter, received_op, v_ret, rece_event.msg_buf,
					   rece_event.msg_len);
	}

	kfree(rece_event.msg_buf);
	status = (enum ne6xvf_status)le32_to_cpu(event->snap.state);

	return status;
}

int ne6xvf_request_reset(struct ne6xvf_adapter *adapter)
{
	int status;

	if (!adapter->vf_res)
		return 0;
	/* Don't check CURRENT_OP - this is always higher priority */
	status = ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_RESET_VF,
				    &adapter->vf_res->vsi_res[0].default_mac_addr[0], 6);
	adapter->current_op = VIRTCHNL_OP_UNKNOWN;

	return status;
}

int ne6xvf_send_api_ver(struct ne6xvf_adapter *adapter)
{
	struct ne6xvf_arq_event_info event = {.buf_len = 0, .msg_buf = NULL};
	struct ne6xvf_virtchnl_version_info vvi;

	vvi.major = NE6XVF_VIRTCHNL_VERSION_MAJOR;
	vvi.minor = NE6XVF_VIRTCHNL_VERSION_MINOR;

	ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_VERSION, (u8 *)&vvi, sizeof(vvi));
	usleep_range(10, 12);
	return ne6xvf_poll_virtchnl_msg(adapter, &event, VIRTCHNL_OP_VERSION);
}

/**
 * ne6xvf_vf_parse_hw_config
 * @hw: pointer to the hardware structure
 * @msg: pointer to the virtual channel VF resource structure
 *
 * Given a VF resource message from the PF, populate the hw struct
 * with appropriate information.
 **/
void ne6xvf_vf_parse_hw_config(struct ne6xvf_hw *hw, struct virtchnl_vf_resource *msg)
{
	struct virtchnl_vsi_resource *vsi_res;
	int i;

	vsi_res = &msg->vsi_res[0];

	hw->dev_caps.num_vsis = msg->num_vsis;
	hw->dev_caps.num_rx_qp = msg->num_queue_pairs;
	hw->dev_caps.num_tx_qp = msg->num_queue_pairs;
	hw->dev_caps.num_msix_vectors_vf = msg->max_vectors;

	hw->dev_caps.max_mtu = msg->max_mtu;
	for (i = 0; i < msg->num_vsis; i++) {
		if (vsi_res->vsi_type == NE6XVF_VIRTCHNL_VSI_SRIOV) {
			ether_addr_copy(hw->mac.perm_addr, vsi_res->default_mac_addr);
			ether_addr_copy(hw->mac.addr, vsi_res->default_mac_addr);
		}
		vsi_res++;
	}
}

/**
 * ne6xvf_get_vf_config
 * @adapter: private adapter structure
 *
 * Get VF configuration from PF and populate hw structure. Must be called after
 * admin queue is initialized. Busy waits until response is received from PF,
 * with maximum timeout. Response from PF is returned in the buffer for further
 * processing by the caller.
 **/
int ne6xvf_get_vf_config(struct ne6xvf_adapter *adapter)
{
	struct ne6xvf_hw *hw = &adapter->hw;
	struct ne6xvf_arq_event_info event;
	int err;

	event.buf_len = sizeof(struct ne6x_mbx_snap_buffer_data);
	event.msg_buf = kzalloc(event.buf_len, GFP_KERNEL);
	if (!event.msg_buf)
		return -ENOMEM;

	err = ne6xvf_poll_virtchnl_msg(adapter, &event, VIRTCHNL_OP_GET_VF_RESOURCES);

	hw->dev_caps.vf_id = event.msg_buf[0];
	hw->dev_caps.chip_id = 0x0;
	hw->dev_caps.lport = event.msg_buf[1];
	hw->dev_caps.mac_id = event.msg_buf[2];
	hw->dev_caps.base_queue = event.msg_buf[3];
	hw->dev_caps.num_vf_per_pf = event.msg_buf[5];
	adapter->vf_res->num_vsis = 0x1;
	adapter->vf_res->num_queue_pairs = event.msg_buf[4];
	adapter->vf_res->max_vectors = event.msg_buf[4];
	adapter->vf_res->vsi_res[0].vsi_type = NE6XVF_VIRTCHNL_VSI_SRIOV;

	adapter->comm.port_info = hw->dev_caps.lport | (hw->dev_caps.vf_id << 8);

	dev_info(&adapter->pdev->dev, "vf %d Get Resource [ lport: %d, mac_id: %d, base: %d, queue: %d, err = %d]\n",
		 hw->dev_caps.vf_id, hw->dev_caps.lport, hw->dev_caps.mac_id,
		 hw->dev_caps.base_queue, adapter->vf_res->num_queue_pairs, err);

	ne6xvf_vf_parse_hw_config(hw, adapter->vf_res);

	return err;
}

int ne6xvf_config_default_vlan(struct ne6xvf_adapter *adapter)
{
	struct ne6xvf_arq_event_info event;
	struct ne6x_vf_vlan vlan;

	adapter->current_op = VIRTCHNL_OP_ADD_VLAN;

	event.buf_len = 0;
	event.msg_buf = NULL;

	vlan = NE6X_VF_VLAN(0xfff, ETH_P_8021Q);
	ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_ADD_VLAN, (u8 *)&vlan, sizeof(struct ne6x_vf_vlan));
	ne6xvf_poll_virtchnl_msg(adapter, &event, VIRTCHNL_OP_ADD_VLAN);

	return 0;
}

/**
 * ne6xvf_send_vf_config_msg
 * @adapter: adapter structure
 *
 * Send VF configuration request admin queue message to the PF. The reply
 * is not checked in this function. Returns 0 if the message was
 * successfully sent, or one of the NE6XVF_ADMIN_QUEUE_ERROR_ statuses if not.
 **/
int ne6xvf_send_vf_config_msg(struct ne6xvf_adapter *adapter, bool b_init)
{
	u8 mac_addr[ETH_ALEN];

	adapter->current_op = VIRTCHNL_OP_GET_VF_RESOURCES;
	adapter->aq_required &= ~NE6XVF_FLAG_AQ_GET_CONFIG;
	if (b_init) {
		eth_random_addr(mac_addr);
		mac_addr[0] = 0x02;
		mac_addr[1] = 0x31;
		mac_addr[2] = 0x3a;
	} else {
		memcpy(mac_addr, adapter->vf_res->vsi_res[0].default_mac_addr, 6);
	}

	ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_GET_VF_RESOURCES, mac_addr, 6);

	/* mac addr need get for PF */
	adapter->vf_res->vsi_res[0].default_mac_addr[0] = mac_addr[0];
	adapter->vf_res->vsi_res[0].default_mac_addr[1] = mac_addr[1];
	adapter->vf_res->vsi_res[0].default_mac_addr[2] = mac_addr[2];
	adapter->vf_res->vsi_res[0].default_mac_addr[3] = mac_addr[3];
	adapter->vf_res->vsi_res[0].default_mac_addr[4] = mac_addr[4];
	adapter->vf_res->vsi_res[0].default_mac_addr[5] = mac_addr[5];
	adapter->vf_res->vsi_res[0].vsi_type = NE6XVF_VIRTCHNL_VSI_SRIOV;

	return 0;
}

int ne6xvf_send_vf_offload_msg(struct ne6xvf_adapter *adapter)
{
	adapter->current_op = VIRTCHNL_OP_CONFIG_OFFLOAD;
	adapter->aq_required &= ~NE6XVF_FLAG_AQ_CONFIGURE_HW_OFFLOAD;
	dev_info(&adapter->pdev->dev, "adapter->hw_feature = 0x%08X\n", adapter->hw_feature);
	ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_CONFIG_OFFLOAD, (u8 *)&adapter->hw_feature, 4);

	return 0;
}

void ne6xvf_config_rss_info(struct ne6xvf_adapter *adapter)
{
	int count, size = sizeof(struct ne6x_rss_info);
	int index, status;
	u8 *plut_info = (u8 *)&adapter->rss_info;
	struct ne6xvf_arq_event_info event;

	if (adapter->current_op != VIRTCHNL_OP_UNKNOWN) {
		/* bail because we already have a command pending */
		dev_err(&adapter->pdev->dev, "Cannot Configure RSS, command %d pending\n",
			adapter->current_op);
		return;
	}

	adapter->current_op = VIRTCHNL_OP_CONFIG_RSS;

	count = (size + NE6XVF_SDK_LARGE_BUF - 1) / NE6XVF_SDK_LARGE_BUF;

	for (index = 0; index < count; index++) {
		event.buf_len = 0;
		event.msg_buf = NULL;
		status = ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_CONFIG_RSS,
					    &plut_info[index * NE6XVF_SDK_LARGE_BUF],
					    ((size - index * NE6XVF_SDK_LARGE_BUF) >
					     NE6XVF_SDK_LARGE_BUF)
						  ? NE6XVF_SDK_LARGE_BUF
						  : (size - index * NE6XVF_SDK_LARGE_BUF));
		ne6xvf_poll_virtchnl_msg(adapter, &event, VIRTCHNL_OP_CONFIG_RSS);
	}

	adapter->aq_required &= ~NE6XVF_FLAG_AQ_CONFIGURE_RSS;
	adapter->current_op = VIRTCHNL_OP_UNKNOWN;
}

void ne6xvf_changed_rss(struct ne6xvf_adapter *adapter)
{
	struct ne6xvf_arq_event_info event;

	if (adapter->current_op != VIRTCHNL_OP_UNKNOWN) {
		/* bail because we already have a command pending */
		dev_err(&adapter->pdev->dev, "Cannot Configure RSS, command %d pending\n",
			adapter->current_op);
		return;
	}

	event.msg_buf = NULL;
	event.buf_len = 0;

	adapter->current_op = VIRTCHNL_OP_CHANGED_RSS;
	ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_CHANGED_RSS, (u8 *)&adapter->num_active_queues,
			   sizeof(adapter->num_active_queues));
	ne6xvf_poll_virtchnl_msg(adapter, &event, VIRTCHNL_OP_CHANGED_RSS);
	adapter->aq_required &= ~NE6XVF_FLAG_AQ_CHANGED_RSS;
	adapter->current_op = VIRTCHNL_OP_UNKNOWN;
}

int ne6xvf_request_feature(struct ne6xvf_adapter *adapter)
{
	if (adapter->current_op != VIRTCHNL_OP_UNKNOWN) {
		/* bail because we already have a command pending */
		dev_err(&adapter->pdev->dev, "Cannot request feature, command %d pending\n",
			adapter->current_op);
		return -EBUSY;
	}

	adapter->current_op = VIRTCHNL_OP_GET_VF_FEATURE;
	adapter->aq_required &= ~NE6XVF_FLAG_AQ_GET_FEATURE;
	ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_GET_VF_FEATURE, NULL, 0);

	return 0;
}

/**
 * ne6xvf_request_stats
 * @adapter: adapter structure
 *
 * Request VSI statistics from PF.
 **/
void ne6xvf_request_stats(struct ne6xvf_adapter *adapter)
{
	ne6xvf_update_pf_stats(adapter);
}

/**
 * ne6xvf_request_queues
 * @adapter: adapter structure
 * @num: number of requested queues
 *
 * We get a default number of queues from the PF.  This enables us to request a
 * different number.  Returns 0 on success, negative on failure
 **/
int ne6xvf_request_queues(struct ne6xvf_adapter *adapter, int num)
{
	struct ne6xvf_virtchnl_vf_res_request vfres;

	if (adapter->current_op != VIRTCHNL_OP_UNKNOWN) {
		/* bail because we already have a command pending */
		dev_err(&adapter->pdev->dev, "Cannot request queues, command %d pending\n",
			adapter->current_op);
		return -EBUSY;
	}

	vfres.num_queue_pairs = 1;
	vfres.need_reset = 0x0;

	adapter->current_op = VIRTCHNL_OP_REQUEST_QUEUES;
	adapter->flags |= NE6XVF_FLAG_REINIT_ITR_NEEDED;

	return ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_REQUEST_QUEUES, (u8 *)&vfres, sizeof(vfres));
}

/**
 * ne6xvf_enable_queues
 * @adapter: adapter structure
 *
 * We get a default number of queues from the PF.  This enables us to request a
 * different number.  Returns 0 on success, negative on failure
 **/
int ne6xvf_enable_queues(struct ne6xvf_adapter *adapter)
{
	if (adapter->current_op != VIRTCHNL_OP_UNKNOWN) {
		/* bail because we already have a command pending */
		dev_err(&adapter->pdev->dev, "Cannot enable queues, command %d pending\n",
			adapter->current_op);
		return -EBUSY;
	}

	adapter->current_op = VIRTCHNL_OP_ENABLE_QUEUES;
	adapter->aq_required &= ~NE6XVF_FLAG_AQ_ENABLE_QUEUES;

	ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_ENABLE_QUEUES, NULL, 0);
	return 0;
}

int ne6xvf_get_vf_feature(struct ne6xvf_adapter *adapter)
{
	struct ne6xvf_arq_event_info event;
	int status;

	event.buf_len = sizeof(struct ne6x_mbx_snap_buffer_data);
	event.msg_buf = kzalloc(event.buf_len, GFP_KERNEL);
	if (!event.msg_buf)
		return -ENOMEM;

	status = ne6xvf_poll_virtchnl_msg(adapter, &event, VIRTCHNL_OP_GET_VF_FEATURE);
	if (status == 0) {
		adapter->hw_feature = event.snap.data[3];
		adapter->hw_feature = (adapter->hw_feature << 8);
		adapter->hw_feature |= event.snap.data[2];
		adapter->hw_feature = (adapter->hw_feature << 8);
		adapter->hw_feature |= event.snap.data[1];
		adapter->hw_feature = (adapter->hw_feature << 8);
		adapter->hw_feature |= event.snap.data[0];
		dev_info(&adapter->pdev->dev, "vf %d get feature 0x%08X\n",
			 adapter->hw.dev_caps.vf_id, adapter->hw_feature);
	}

	adapter->current_op = VIRTCHNL_OP_UNKNOWN;
	adapter->aq_required &= ~NE6XVF_FLAG_AQ_GET_FEATURE;
	kfree(event.msg_buf);

	return status;
}

/**
 * ne6xvf_add_ether_addrs
 * @adapter: adapter structure
 *
 * Request that the PF add one or more addresses to our filters.
 **/
void ne6xvf_add_ether_addrs(struct ne6xvf_adapter *adapter)
{
	struct ne6xvf_arq_event_info event = {.buf_len = 0, .msg_buf = NULL};
	struct virtchnl_ether_addr_list *veal;
	struct ne6xvf_mac_filter *f;
	int len, i = 0, count = 0;

	if (adapter->current_op != VIRTCHNL_OP_UNKNOWN) {
		/* bail because we already have a command pending */
		dev_err(&adapter->pdev->dev, "Cannot add filters, command %d pending\n",
			adapter->current_op);
		return;
	}

	adapter->aq_required &= ~NE6XVF_FLAG_AQ_ADD_MAC_FILTER;
	adapter->current_op = VIRTCHNL_OP_ADD_ETH_ADDR;
	spin_lock_bh(&adapter->mac_vlan_list_lock);

	list_for_each_entry(f, &adapter->mac_filter_list, list) {
		if (f->add)
			count++;
	}

	if (!count) {
		adapter->aq_required &= ~NE6XVF_FLAG_AQ_ADD_MAC_FILTER;
		spin_unlock_bh(&adapter->mac_vlan_list_lock);
		return;
	}

	len = struct_size(veal, list, count);
	veal = kzalloc(len, GFP_ATOMIC);
	if (!veal) {
		spin_unlock_bh(&adapter->mac_vlan_list_lock);
		return;
	}

	veal->vsi_id = adapter->vsi_res->vsi_id;
	veal->num_elements = count;
	list_for_each_entry(f, &adapter->mac_filter_list, list) {
		if (f->add) {
			ether_addr_copy(veal->list[i].addr, f->macaddr);
			i++;
			f->add = false;
			if (i == count)
				break;
		}
	}
	spin_unlock_bh(&adapter->mac_vlan_list_lock);

	for (i = 0; i < count; i++) {
		event.buf_len = 0;
		event.msg_buf = NULL;
		ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_ADD_ETH_ADDR, (u8 *)veal->list[i].addr, 6);
		ne6xvf_poll_virtchnl_msg(adapter, &event, VIRTCHNL_OP_ADD_ETH_ADDR);
	}

	adapter->current_op = VIRTCHNL_OP_UNKNOWN;

	kfree(veal);
}

void ne6xvf_set_vf_addr(struct ne6xvf_adapter *adapter)
{
	if (adapter->current_op != VIRTCHNL_OP_UNKNOWN) {
		dev_err(&adapter->pdev->dev, "Cannot add filters, command %d pending\n",
			adapter->current_op);
		return;
	}

	adapter->current_op = VIRTCHNL_OP_SET_VF_ADDR;
	ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_SET_VF_ADDR, adapter->hw.mac.addr, 6);
	adapter->aq_required &= ~NE6XVF_FLAG_AQ_SET_VF_MAC;
}

/**
 * ne6xvf_del_ether_addrs
 * @adapter: adapter structure
 *
 * Request that the PF add one or more addresses to our filters.
 **/
void ne6xvf_del_ether_addrs(struct ne6xvf_adapter *adapter)
{
	struct ne6xvf_arq_event_info event = {.buf_len = 0, .msg_buf = NULL};
	struct virtchnl_ether_addr_list *veal;
	struct ne6xvf_mac_filter *f, *temp;
	int len, i = 0, count = 0;

	if (adapter->current_op != VIRTCHNL_OP_UNKNOWN) {
		/* bail because we already have a command pending */
		dev_err(&adapter->pdev->dev, "Cannot add filters, command %d pending\n",
			adapter->current_op);
		return;
	}

	adapter->aq_required &= ~NE6XVF_FLAG_AQ_DEL_MAC_FILTER;
	spin_lock_bh(&adapter->mac_vlan_list_lock);

	list_for_each_entry(f, &adapter->mac_filter_list, list) {
		if (f->remove)
			count++;
	}

	if (!count) {
		adapter->aq_required &= ~NE6XVF_FLAG_AQ_DEL_MAC_FILTER;
		spin_unlock_bh(&adapter->mac_vlan_list_lock);
		return;
	}

	adapter->current_op = VIRTCHNL_OP_DEL_ETH_ADDR;

	len = struct_size(veal, list, count);
	veal = kzalloc(len, GFP_ATOMIC);
	if (!veal) {
		spin_unlock_bh(&adapter->mac_vlan_list_lock);
		return;
	}

	veal->vsi_id = adapter->vsi_res->vsi_id;
	veal->num_elements = count;
	list_for_each_entry_safe(f, temp, &adapter->mac_filter_list, list) {
		if (f->remove) {
			ether_addr_copy(veal->list[i].addr, f->macaddr);
			i++;
			list_del(&f->list);
			kfree(f);
			if (i == count)
				break;
		}
	}
	spin_unlock_bh(&adapter->mac_vlan_list_lock);

	for (i = 0; i < count; i++) {
		event.buf_len = 0;
		event.msg_buf = NULL;
		ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_DEL_ETH_ADDR, (u8 *)veal->list[i].addr, 6);
		ne6xvf_poll_virtchnl_msg(adapter, &event, VIRTCHNL_OP_DEL_ETH_ADDR);
	}

	adapter->current_op = VIRTCHNL_OP_UNKNOWN;

	kfree(veal);
}

#define NE6XVF_MAX_SPEED_STRLEN 13

/**
 * ne6xvf_print_link_message - print link up or down
 * @adapter: adapter structure
 *
 * Log a message telling the world of our wonderous link status
 */
static void ne6xvf_print_link_message(struct ne6xvf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int link_speed_mbps;
	char *speed;

	if (!adapter->link_up) {
		netdev_info(netdev, "NIC Link is Down\n");
		return;
	}

	speed = kcalloc(1, NE6XVF_MAX_SPEED_STRLEN, GFP_KERNEL);
	if (!speed)
		return;

	switch (adapter->link_speed) {
	case NE6X_LINK_SPEED_100GB:
		link_speed_mbps = SPEED_100000;
		break;
	case NE6X_LINK_SPEED_40GB:
		link_speed_mbps = SPEED_40000;
		break;
	case NE6X_LINK_SPEED_25GB:
		link_speed_mbps = SPEED_25000;
		break;
	case NE6X_LINK_SPEED_10GB:
		link_speed_mbps = SPEED_10000;
		break;
	default:
		link_speed_mbps = SPEED_UNKNOWN;
		break;
	}

	snprintf(speed, NE6XVF_MAX_SPEED_STRLEN, "%d %s", link_speed_mbps / 1000, "Gbps");

	netdev_info(netdev, "NIC Link is Up Speed is %s Full Duplex\n", speed);

	kfree(speed);
}

/**
 * ne6xvf_set_promiscuous
 * @adapter: adapter structure
 * @flags: bitmask to control unicast/multicast promiscuous.
 *
 * Request that the PF enable promiscuous mode for our VSI.
 **/
void ne6xvf_set_promiscuous(struct ne6xvf_adapter *adapter)
{
	struct ne6xvf_virtchnl_promisc_info vpi;
	int flags = 0;

	dev_warn(&adapter->pdev->dev, "%s: ....\n", __func__);

	if (adapter->current_op != VIRTCHNL_OP_UNKNOWN) {
		/* bail because we already have a command pending */
		dev_err(&adapter->pdev->dev, "Cannot set promiscuous mode, command %d pending\n",
			adapter->current_op);
		return;
	}

	adapter->current_op = VIRTCHNL_OP_CONFIG_PROMISCUOUS_MODE;

	if (adapter->flags & NE6XVF_FLAG_PROMISC_ON) {
		adapter->hw_feature |= NE6X_F_PROMISC;
		flags |= FLAG_VF_UNICAST_PROMISC;
	} else {
		adapter->hw_feature &= ~NE6X_F_PROMISC;
	}

	if (adapter->flags & NE6XVF_FLAG_ALLMULTI_ON) {
		adapter->hw_feature |= NE6X_F_RX_ALLMULTI;
		flags |= FLAG_VF_MULTICAST_PROMISC;
	} else {
		adapter->hw_feature &= ~NE6X_F_RX_ALLMULTI;
	}

	vpi.vsi_id = adapter->vsi_res->vsi_id;
	vpi.flags = flags;

	ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_CONFIG_PROMISCUOUS_MODE, (u8 *)&vpi, sizeof(vpi));
}

void ne6xvf_vchanel_get_port_link_status(struct ne6xvf_adapter *adapter)
{
	struct ne6xvf_hw *hw = &adapter->hw;
	u8 msg[8] = {0};

	if (adapter->current_op != VIRTCHNL_OP_UNKNOWN) {
		/* bail because we already have a command pending */
		dev_err(&adapter->pdev->dev, "Cannot get_link_status, command %d pending\n",
			adapter->current_op);
		return;
	}

	/* pass queue info to vf */
	msg[0] = hw->dev_caps.base_queue;
	msg[1] = adapter->num_active_queues;

	adapter->current_op = VIRTCHNL_OP_GET_PORT_STATUS;
	ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_GET_PORT_STATUS, msg, 2);
	adapter->aq_required &= ~NE6XVF_FLAG_AQ_GET_PORT_LINK_STATUS;
}

/**
 * ne6xvf_virtchnl_completion
 * @adapter: adapter structure
 * @v_opcode: opcode sent by PF
 * @v_retval: retval sent by PF
 * @msg: message sent by PF
 * @msglen: message length
 *
 * Asynchronous completion function for admin queue messages. Rather than busy
 * wait, we fire off our requests and assume that no errors will be returned.
 * This function handles the reply messages.
 **/
void ne6xvf_virtchnl_completion(struct ne6xvf_adapter *adapter, enum virtchnl_ops v_opcode,
				enum ne6xvf_status v_retval, u8 *msg, u16 msglen)
{
	struct net_device *netdev = adapter->netdev;

	if (v_opcode == VIRTCHNL_OP_EVENT) {
		struct virtchnl_pf_event *vpe = (struct virtchnl_pf_event *)msg;
		bool link_up = vpe->link_status;
		enum ne6x_sdk_link_speed old_link_speed = adapter->link_speed;

		switch (vpe->event) {
		case NE6XVF_VIRTCHNL_EVENT_LINK_CHANGE:
			adapter->link_speed = (vpe->link_speed_0 << 24) |
					      (vpe->link_speed_1 << 16) |
					      (vpe->link_speed_2 << 8) |
					      vpe->link_speed_3;
			if (adapter->current_op == VIRTCHNL_OP_GET_PORT_STATUS)
				adapter->current_op = VIRTCHNL_OP_UNKNOWN;

			/* we've already got the right link status, bail */
			if (adapter->link_up == link_up) {
				if (link_up && old_link_speed != adapter->link_speed)
					ne6xvf_print_link_message(adapter);

				break;
			}

			if (link_up) {
				/* If we get link up message and start queues
				 * before our queues are configured it will
				 * trigger a TX hang. In that case, just ignore
				 * the link status message,we'll get another one
				 * after we enable queues and actually prepared
				 * to send traffic.
				 */
				if (adapter->state != __NE6XVF_RUNNING)
					break;

				/* For ADQ enabled VF, we reconfigure VSIs and
				 * re-allocate queues. Hence wait till all
				 * queues are enabled.
				 */
				if (adapter->flags & NE6XVF_FLAG_QUEUES_DISABLED)
					break;
			}

			adapter->link_up = link_up;
			if (link_up) {
				netif_tx_start_all_queues(netdev);
				netif_carrier_on(netdev);
			} else {
				netif_tx_stop_all_queues(netdev);
				netif_carrier_off(netdev);
			}
			ne6xvf_print_link_message(adapter);
			break;
		case NE6XVF_VIRTCHNL_EVENT_RESET_IMPENDING:
			dev_info(&adapter->pdev->dev, "Reset indication received from the PF\n");
			break;
		default:
			dev_err(&adapter->pdev->dev, "Unknown event %d from PF\n", vpe->event);
			break;
		}
		return;
	}

	if (v_opcode == VIRTCHNL_OP_VF_CONFIG) {
		struct virtchnl_vf_config *vfconfig = (struct virtchnl_vf_config *)msg;

		dev_info(&adapter->pdev->dev, "vf_vonfig_data from the PF,type= %d,value = %d\n",
			 vfconfig->type, vfconfig->data[0]);
		switch (vfconfig->type) {
		case VIRTCHNL_VF_CONFIG_TRUST:
			adapter->trusted = vfconfig->data[0];
			if (!adapter->trusted) {
				adapter->hw_feature &= ~NE6X_F_PROMISC;
				adapter->hw_feature &= ~NE6X_F_RX_ALLMULTI;
				adapter->flags &= ~NE6XVF_FLAG_PROMISC_ON;
				adapter->flags &= ~NE6XVF_FLAG_ALLMULTI_ON;
			}
			break;
		default:
			break;
		}
		return;
	}

	if (v_retval) {
		switch (v_opcode) {
		case VIRTCHNL_OP_SET_VF_ADDR:
			dev_err(&adapter->pdev->dev, "Failed to change MAC address\n");
			ether_addr_copy(adapter->hw.mac.addr, netdev->dev_addr);
			wake_up(&adapter->vc_waitqueue);
			if (adapter->current_op != VIRTCHNL_OP_SET_VF_ADDR)
				return;

			break;
		default:
			dev_err(&adapter->pdev->dev, "PF returned error %d to our request %d\n",
				v_retval, v_opcode);

			/* Assume that the ADQ configuration caused one of the
			 * v_opcodes in this if statement to fail.  Set the
			 * flag so the reset path can return to the pre-ADQ
			 * configuration and traffic can resume
			 */
			if ((v_opcode == VIRTCHNL_OP_ENABLE_QUEUES ||
			     v_opcode == VIRTCHNL_OP_CONFIG_IRQ_MAP ||
			     v_opcode == VIRTCHNL_OP_CONFIG_ADPT_QUEUES)) {
				dev_err(&adapter->pdev->dev,
					"ADQ is enabled and opcode %d failed (%d)\n", v_opcode,
					v_retval);
				netdev_reset_tc(netdev);
				adapter->flags |= NE6XVF_FLAG_REINIT_ITR_NEEDED;
				ne6xvf_schedule_reset(adapter);
				adapter->current_op = VIRTCHNL_OP_UNKNOWN;
				return;
			}
		}
	}

	switch (v_opcode) {
	case VIRTCHNL_OP_SET_VF_ADDR:
		if (!v_retval) {
			if (msglen != 0 && msg) {
				netif_addr_lock_bh(netdev);
				ether_addr_copy(adapter->hw.mac.addr, msg);
				eth_hw_addr_set(netdev, msg);
				netif_addr_unlock_bh(netdev);
			}
		}
		wake_up(&adapter->vc_waitqueue);
		if (adapter->current_op == VIRTCHNL_OP_SET_VF_ADDR)
			adapter->current_op = VIRTCHNL_OP_UNKNOWN;

		break;
	case VIRTCHNL_OP_GET_VF_RESOURCES:
		memcpy(adapter->vf_res, msg, msglen);
		ne6xvf_vf_parse_hw_config(&adapter->hw, adapter->vf_res);
		if (is_zero_ether_addr(adapter->hw.mac.addr)) {
			/* restore current mac address */
			ether_addr_copy(adapter->hw.mac.addr, netdev->dev_addr);
		} else {
			netif_addr_lock_bh(netdev);
			/* refresh current mac address if changed */
			ether_addr_copy(netdev->perm_addr, adapter->hw.mac.addr);
			netif_addr_unlock_bh(netdev);
		}

		ne6xvf_parse_vf_resource_msg(adapter);
		break;
	case VIRTCHNL_OP_GET_VF_FEATURE:
		memcpy(&adapter->hw_feature, msg, 4);
		dev_info(&adapter->pdev->dev, "%s: hw_featrue = 0x%08X\n",
			 ne6xvf_state_str(adapter->state), adapter->hw_feature);
		break;
	case VIRTCHNL_OP_ENABLE_QUEUES:
		/* enable transmits */
		if (adapter->state == __NE6XVF_RUNNING) {
			ne6xvf_irq_enable(adapter, true);
			/* If queues not enabled when handling link event,
			 * then set carrier on now
			 */
			if (adapter->link_up && !netif_carrier_ok(netdev)) {
				netif_tx_start_all_queues(netdev);
				netif_carrier_on(netdev);
			}
		}
		adapter->flags |= NE6XVF_FLAG_QUEUES_ENABLED;
		adapter->flags &= ~NE6XVF_FLAG_QUEUES_DISABLED;
		break;
	case VIRTCHNL_OP_DISABLE_QUEUES:
		ne6xvf_free_all_tg_resources(adapter);
		ne6xvf_free_all_cq_resources(adapter);
		ne6xvf_free_all_tx_resources(adapter);
		ne6xvf_free_all_rx_resources(adapter);
		if (adapter->state == __NE6XVF_DOWN_PENDING)
			ne6xvf_change_state(adapter, __NE6XVF_DOWN);

		adapter->flags &= ~NE6XVF_FLAG_QUEUES_ENABLED;
		break;
	case VIRTCHNL_OP_VERSION:
	case VIRTCHNL_OP_CONFIG_IRQ_MAP:
		/* Don't display an error if we get these out of sequence.
		 * If the firmware needed to get kicked, we'll get these and
		 * it's no problem.
		 */
		if (v_opcode != adapter->current_op)
			return;

		break;
	case VIRTCHNL_OP_REQUEST_QUEUES: {
		struct ne6xvf_virtchnl_vf_res_request *vfres =
			(struct ne6xvf_virtchnl_vf_res_request *)msg;
		if (vfres->num_queue_pairs != adapter->num_req_queues) {
			dev_info(&adapter->pdev->dev, "Requested %d queues, PF can support %d\n",
				 adapter->num_req_queues, vfres->num_queue_pairs);
			adapter->num_req_queues = 0;
			adapter->flags &= ~NE6XVF_FLAG_REINIT_ITR_NEEDED;
		}
	} break;
	default:
		if (adapter->current_op && v_opcode != adapter->current_op)
			dev_dbg(&adapter->pdev->dev, "Expected response %d from PF, received %d\n",
				adapter->current_op, v_opcode);

		break;
	} /* switch v_opcode */

	adapter->current_op = VIRTCHNL_OP_UNKNOWN;
}

/**
 * ne6xvf_add_vlans
 * @adapter: adapter structure
 *
 * Request that the PF add one or more VLAN filters to our VSI.
 **/
void ne6xvf_add_vlans(struct ne6xvf_adapter *adapter)
{
	struct ne6xvf_arq_event_info event = {0};
	struct ne6xvf_vlan_filter *f = NULL;
	struct ne6x_vf_vlan *vlan = NULL;
	int len = 0, i = 0, count = 0;

	dev_info(&adapter->pdev->dev, "%s: adapter->current_op:%d\n", __func__,
		 adapter->current_op);

	if (adapter->current_op != VIRTCHNL_OP_UNKNOWN) {
		/* bail because we already have a command pending */
		dev_err(&adapter->pdev->dev, "Cannot add VLANs, command %d pending\n",
			adapter->current_op);
		return;
	}

	spin_lock_bh(&adapter->mac_vlan_list_lock);

	list_for_each_entry(f, &adapter->vlan_filter_list, list) {
		if (f->add)
			count++;
	}

	if (!count) {
		adapter->aq_required &= ~NE6XVF_FLAG_AQ_ADD_VLAN_FILTER;
		spin_unlock_bh(&adapter->mac_vlan_list_lock);
		return;
	}

	adapter->current_op = VIRTCHNL_OP_ADD_VLAN;

	len = sizeof(struct ne6x_vf_vlan) * count;
	vlan = kzalloc(len, GFP_ATOMIC);
	if (!vlan) {
		spin_unlock_bh(&adapter->mac_vlan_list_lock);
		return;
	}

	list_for_each_entry(f, &adapter->vlan_filter_list, list) {
		if (f->add) {
			vlan[i].tpid = f->vlan.tpid;
			vlan[i].vid = f->vlan.vid;
			i++;
			f->add = false;
			f->is_new_vlan = true;
			if (i == count)
				break;
		}
	}
	adapter->aq_required &= ~NE6XVF_FLAG_AQ_ADD_VLAN_FILTER;

	spin_unlock_bh(&adapter->mac_vlan_list_lock);
	for (i = 0; i < count; i++) {
		event.buf_len = 0;
		event.msg_buf = NULL;
		ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_ADD_VLAN, (u8 *)&vlan[i],
				   sizeof(struct ne6x_vf_vlan));
		ne6xvf_poll_virtchnl_msg(adapter, &event, VIRTCHNL_OP_ADD_VLAN);
	}

	adapter->current_op = VIRTCHNL_OP_UNKNOWN;
	kfree(vlan);
}

/**
 * ne6xvf_del_vlans
 * @adapter: adapter structure
 *
 * Request that the PF remove one or more VLAN filters from our VSI.
 **/
void ne6xvf_del_vlans(struct ne6xvf_adapter *adapter)
{
	struct ne6xvf_arq_event_info event = {0};
	struct ne6xvf_vlan_filter *f, *ftmp;
	struct ne6x_vf_vlan *vlan = NULL;
	int i = 0, count = 0;

	if (adapter->current_op != VIRTCHNL_OP_UNKNOWN) {
		/* bail because we already have a command pending */
		dev_err(&adapter->pdev->dev, "Cannot remove VLANs, command %d pending\n",
			adapter->current_op);
		return;
	}

	spin_lock_bh(&adapter->mac_vlan_list_lock);

	list_for_each_entry_safe(f, ftmp, &adapter->vlan_filter_list, list) {
		/* since VLAN capabilities are not allowed, we dont want to send
		 * a VLAN delete request because it will most likely fail and
		 * create unnecessary errors/noise, so just free the VLAN
		 * filters marked for removal to enable bailing out before
		 * sending a virtchnl message
		 */
		if (f->remove)
			count++;
	}

	if (!count) {
		adapter->aq_required &= ~NE6XVF_FLAG_AQ_DEL_VLAN_FILTER;
		spin_unlock_bh(&adapter->mac_vlan_list_lock);
		return;
	}

	adapter->current_op = VIRTCHNL_OP_DEL_VLAN;
	vlan = kcalloc(count, sizeof(*vlan), GFP_ATOMIC);
	if (!vlan) {
		spin_unlock_bh(&adapter->mac_vlan_list_lock);
		return;
	}

	list_for_each_entry_safe(f, ftmp, &adapter->vlan_filter_list, list) {
		if (f->remove) {
			vlan[i].tpid = f->vlan.tpid;
			vlan[i].vid = f->vlan.vid;
			i++;
			list_del(&f->list);
			kfree(f);
			if (i == count)
				break;
		}
	}

	spin_unlock_bh(&adapter->mac_vlan_list_lock);
	adapter->aq_required &= ~NE6XVF_FLAG_AQ_DEL_VLAN_FILTER;
	for (i = 0; i < count; i++) {
		event.buf_len = 0;
		event.msg_buf = NULL;
		ne6xvf_send_pf_msg(adapter, VIRTCHNL_OP_DEL_VLAN, (u8 *)&vlan[i],
				   sizeof(struct ne6x_vf_vlan));
		ne6xvf_poll_virtchnl_msg(adapter, &event, VIRTCHNL_OP_DEL_VLAN);
	}

	adapter->current_op = VIRTCHNL_OP_UNKNOWN;
	kfree(vlan);
}
