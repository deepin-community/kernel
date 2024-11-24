#include <linux/etherdevice.h>
#include <net/xfrm.h>

#include "sxevf_hw.h"
#include "sxevf_msg.h"
#include "sxevf.h"
#include "sxevf_regs.h"
#include "sxe_log.h"

#define SXEVF_PFMSG_MASK    0xFF00

#define SXEVF_RESET_DELAY_TIME              10
#define SXEVF_SYNC_UCADDR_DELAY_TIME        200

#define SXEVF_REDIR_TBL_ENTRY_NUM_PER_UINT  16
#define SXEVF_REDIR_TBL_ENTRY_BITS          2

void sxevf_mbx_init(struct sxevf_hw *hw)
{
	hw->mbx.msg_len = SXEVF_MBX_MSG_NUM;

	hw->mbx.stats.rcv_msgs = 0;
	hw->mbx.stats.send_msgs = 0;
	hw->mbx.stats.acks = 0;
	hw->mbx.stats.reqs = 0;
	hw->mbx.stats.rsts = 0;

	hw->mbx.retry = 0;
	hw->mbx.interval = SXEVF_MBX_RETRY_INTERVAL;

	return;
}

static u32 sxevf_mbx_reg_read(struct sxevf_hw *hw)
{
	u32 value = hw->mbx.ops->mailbox_read(hw);

	value |= hw->mbx.reg_value;

	hw->mbx.reg_value |= value & SXE_VFMAILBOX_RC_BIT;

	return value;
}

static bool sxevf_mbx_bit_check(struct sxevf_hw *hw, u32 mask)
{
	bool ret = false;
	u32 value = sxevf_mbx_reg_read(hw);

	if (value & mask) {
		ret = true;
	}

	hw->mbx.reg_value &= ~mask;

	return ret;
}

STATIC bool sxevf_pf_msg_check(struct sxevf_hw *hw)
{
	bool ret = false;

	if (sxevf_mbx_bit_check(hw, SXE_VFMAILBOX_PFSTS)) {
		hw->mbx.stats.reqs++;
		ret = true;
	}

	return ret;
}

STATIC bool sxevf_pf_ack_check(struct sxevf_hw *hw)
{
	bool ret = false;

	if (sxevf_mbx_bit_check(hw, SXE_VFMAILBOX_PFACK)) {
		hw->mbx.stats.acks++;
		ret = true;
	}

	return ret;
}

bool sxevf_pf_rst_check(struct sxevf_hw *hw)
{
	bool ret = false;

	if (!sxevf_mbx_bit_check(hw, (SXE_VFMAILBOX_RSTI |
				      SXE_VFMAILBOX_RSTD))) {
		hw->mbx.stats.rsts++;
		ret = true;
	}

	return ret;
}

STATIC s32 sxevf_mailbox_lock(struct sxevf_hw *hw)
{
	u32 mailbox;
	u32 retry = SXEVF_MBX_RETRY_COUNT;
	s32 ret = -SXEVF_ERR_MBX_LOCK_FAIL;

	while (retry--) {
		mailbox = sxevf_mbx_reg_read(hw);
		mailbox |= SXE_VFMAILBOX_VFU;
		hw->mbx.ops->mailbox_write(hw, mailbox);

		if (sxevf_mbx_reg_read(hw) & SXE_VFMAILBOX_VFU) {
			ret = 0;
			break;
		}

		udelay(hw->mbx.interval);
	}

	return ret;
}

static void sxevf_mailbox_unlock(struct sxevf_hw *hw)
{
	u32 mailbox;

	mailbox = sxevf_mbx_reg_read(hw);
	mailbox &= ~SXE_VFMAILBOX_VFU;
	hw->mbx.ops->mailbox_write(hw, mailbox);

	return;
}

STATIC bool sxevf_msg_poll(struct sxevf_hw *hw)
{
	struct sxevf_mbx_info *mbx = &hw->mbx;
	u32 retry = mbx->retry;
	bool ret = true;
	struct sxevf_adapter *adapter = hw->adapter;

	while (!sxevf_pf_msg_check(hw) && retry) {
		retry--;
		udelay(mbx->interval);
	}

	if (!retry) {
		LOG_ERROR_BDF("retry:%d use up, but don't check pf reply,"
			  " clear retry to 0\n",
			  mbx->retry);
		mbx->retry = 0;
		ret = false;
	}

	return ret;
}

STATIC bool sxevf_ack_poll(struct sxevf_hw *hw)
{
	struct sxevf_mbx_info *mbx = &hw->mbx;
	u32 retry = mbx->retry;
	bool ret = true;
	struct sxevf_adapter *adapter = hw->adapter;

	while (!sxevf_pf_ack_check(hw) && retry) {
		retry--;
		udelay(mbx->interval);
	}

	if (!retry) {
		LOG_ERROR_BDF("send msg to pf, retry:%d but don't check pf ack, "
			  "init mbx retry to 0.\n",
			  mbx->retry);
		mbx->retry = 0;
		ret = false;
	}

	return ret;
}

static void sxevf_pf_msg_and_ack_clear(struct sxevf_hw *hw)
{
	struct sxevf_adapter *adapter = hw->adapter;

	LOG_INFO_BDF("clear pending pf msg and ack.\n");

	sxevf_pf_msg_check(hw);
	sxevf_pf_ack_check(hw);

	return;
}

static s32 sxevf_send_msg_to_pf(struct sxevf_hw *hw, u32 *msg, u16 msg_len)
{
	struct sxevf_mbx_info *mbx = &hw->mbx;
	s32 ret = 0;
	u16 i;
	u32 old;
	struct sxevf_adapter *adapter = hw->adapter;

	if (!mbx->retry) {
		ret = -SXEVF_ERR_NOT_READY;
		LOG_ERROR_BDF("msg:0x%x len:%d send fail due to retry:0.(err:%d)\n",
			  msg[0], msg_len, ret);
		goto l_out;
	}

	if (msg_len > mbx->msg_len) {
		ret = -EINVAL;
		LOG_ERROR_BDF("vf msg:0x%x len:%d exceed limit:%d "
			  "send fail.(err:%d)\n",
			  msg[0], msg_len, mbx->msg_len, ret);
		goto l_out;
	}

	ret = sxevf_mailbox_lock(hw);
	if (ret) {
		LOG_ERROR_BDF("msg:0x%x len:%d send lock mailbox fail.(err:%d)\n",
			   msg[0], msg_len, ret);
		goto l_out;
	}

	sxevf_pf_msg_and_ack_clear(hw);

	old = hw->mbx.ops->msg_read(hw, 0);
	msg[0] |= (old & SXEVF_PFMSG_MASK);

	for (i = 0; i < msg_len; i++) {
		hw->mbx.ops->msg_write(hw, i, msg[i]);
	}

	hw->mbx.ops->pf_req_irq_trigger(hw);

	hw->mbx.stats.send_msgs++;

	if (!sxevf_ack_poll(hw)) {
		ret = -SXEVF_ERR_POLL_ACK_FAIL;
		LOG_ERROR_BDF("msg:0x%x len:%d send done, but don't poll ack.\n",
			   msg[0], msg_len);
		goto l_out;
	}

	LOG_INFO_BDF("vf send msg:0x%x len:%d to pf and polled pf ack done."
		 "stats send_msg:%d ack:%d.\n",
		 msg[0], msg_len,
		 mbx->stats.send_msgs, mbx->stats.acks);

l_out:
	return ret;
}

s32 sxevf_mbx_msg_rcv(struct sxevf_hw *hw, u32 *msg, u16 msg_len)
{
	u32 i;
	u16 msg_entry;
	s32 ret = 0;
	struct sxevf_mbx_info *mbx = &hw->mbx;
	struct sxevf_adapter *adapter = hw->adapter;

	msg_entry = (msg_len > mbx->msg_len) ? mbx->msg_len : msg_len;

	ret = sxevf_mailbox_lock(hw);
	if (ret) {
		LOG_ERROR_BDF("size:%d rcv lock mailbox fail.(err:%d)\n",
			   msg_entry, ret);
		goto l_end;
	}

	for (i = 0; i < msg_entry; i++) {
		msg[i] = hw->mbx.ops->msg_read(hw, i);
	}

	msg[0] &= ~SXEVF_PFMSG_MASK;

	hw->mbx.ops->pf_ack_irq_trigger(hw);

	mbx->stats.rcv_msgs++;
l_end:
	return ret;

}

s32 sxevf_ctrl_msg_rcv(struct sxevf_hw *hw, u32 *msg, u16 msg_len)
{
	u16 i;
	u16 msg_entry;
	s32 ret = 0;
	struct sxevf_mbx_info *mbx = &hw->mbx;
	struct sxevf_adapter *adapter = hw->adapter;

	msg_entry = (msg_len > mbx->msg_len) ? mbx->msg_len : msg_len;

	ret = sxevf_mailbox_lock(hw);
	if (ret) {
		LOG_ERROR_BDF("size:%d rcv lock mailbox fail.(err:%d)\n",
			   msg_entry, ret);
		goto l_end;
	}

	for (i = 0; i < msg_entry; i++) {
		msg[i] = hw->mbx.ops->msg_read(hw, i);
	}

	sxevf_mailbox_unlock(hw);

	LOG_INFO_BDF("rcv pf mailbox msg:0x%x.\n", *msg);

	mbx->stats.rcv_msgs++;
l_end:
	return ret;
}

s32 sxevf_ctrl_msg_rcv_and_clear(struct sxevf_hw *hw, u32 *msg, u16 msg_len)
{
	u16 i;
	u16 msg_entry;
	s32 ret = 0;
	u32 clear;
	struct sxevf_mbx_info *mbx = &hw->mbx;
	struct sxevf_adapter *adapter = hw->adapter;

	msg_entry = (msg_len > mbx->msg_len) ? mbx->msg_len : msg_len;

	ret = sxevf_mailbox_lock(hw);
	if (ret) {
		LOG_ERROR_BDF("size:%d rcv lock mailbox fail.(err:%d)\n",
			   msg_entry, ret);
		goto l_end;
	}

	for (i = 0; i < msg_entry; i++) {
		msg[i] = hw->mbx.ops->msg_read(hw, i);
	}

	clear = msg[0] & (~SXEVF_PFMSG_MASK);
	hw->mbx.ops->msg_write(hw, 0, clear);

	sxevf_mailbox_unlock(hw);

	LOG_INFO_BDF("rcv pf mailbox msg:0x%x.\n", *msg);

	mbx->stats.rcv_msgs++;
l_end:
	return ret;
}

static s32 sxevf_rcv_msg_from_pf(struct sxevf_hw *hw, u32 *msg, u16 msg_len)
{
	s32 ret;
	struct sxevf_adapter *adapter = hw->adapter;

	if (!sxevf_msg_poll(hw)) {
		ret = -SXEVF_ERR_POLL_MSG_FAIL;
		LOG_ERROR_BDF("retry:%d poll pf msg fail.\n", hw->mbx.retry);
		goto l_out;
	}

	ret = sxevf_mbx_msg_rcv(hw, msg, msg_len);
	if (ret < 0) {
		LOG_ERROR_BDF("retry:%d read msg fail.\n", hw->mbx.retry);
		goto l_out;
	}

	LOG_INFO_BDF("vf polled msg:0x%x from pf and rcv pf msg done. "
		"stats req:%d rcv_msg:%d\n",
		 msg[0], hw->mbx.stats.reqs, hw->mbx.stats.rcv_msgs);

l_out:
	return ret;
}

s32 sxevf_send_and_rcv_msg(struct sxevf_hw *hw, u32 *msg, u8 msg_len)
{
	s32 ret;
	u16 msg_type = msg[0] & 0xFF;
	struct sxevf_adapter *adapter = hw->adapter;

	ret = sxevf_send_msg_to_pf(hw, msg, msg_len);
	if (ret) {
		LOG_ERROR_BDF("msg:0x%x len:%u msg send fail.(err:%d).\n",
			   msg[0], msg_len, ret);
		goto l_out;
	}

	if (msg_type == SXEVF_RESET) {
		mdelay(SXEVF_RESET_DELAY_TIME);
	}

	ret = sxevf_rcv_msg_from_pf(hw, msg, msg_len);
	if (ret) {
		LOG_ERROR_BDF("msg:0x%x len:%u rcv fail.(err:%d).\n",
			   msg[0], msg_len, ret);
		goto l_out;
	}

l_out:
	return ret;
}

STATIC s32 __sxevf_uc_addr_set(struct sxevf_hw *hw, u8 *uc_addr)
{
	s32 ret;
	struct sxevf_uc_addr_msg msg = {};
	struct sxevf_adapter *adapter = hw->adapter;

	msg.msg_type = SXEVF_DEV_MAC_ADDR_SET;
	ether_addr_copy(msg.uc_addr, uc_addr);

	ret = sxevf_send_and_rcv_msg(hw, (u32 *)&msg, SXEVF_MSG_NUM(sizeof(msg)));
	if (!ret && (msg.msg_type ==
		   (SXEVF_DEV_MAC_ADDR_SET | SXEVF_MSGTYPE_NACK))) {
		ether_addr_copy(adapter->mac_filter_ctxt.cur_uc_addr, adapter->mac_filter_ctxt.def_uc_addr);
		ret = -EPERM;
		LOG_ERROR_BDF("msg:0x%x uc addr:%pM replyed nack.\n",
			   msg.msg_type, uc_addr);
		goto l_out;
	}

	if (ret) {
		LOG_ERROR_BDF("msg:0x%x uc addr:%pM set fail.(err:%d)\n",
			   msg.msg_type, uc_addr, ret);
		ret = -EPERM;
		goto l_out;
	}

	LOG_INFO_BDF("msg:0x%x uc addr:%pM set success.\n", msg.msg_type, uc_addr);

l_out:
	return ret;
}

STATIC s32 __sxevf_hv_uc_addr_set(struct sxevf_hw *hw, u8 *uc_addr)
{
	struct sxevf_adapter *adapter = hw->adapter;
	s32 ret = -EOPNOTSUPP;

	if (ether_addr_equal(uc_addr, adapter->mac_filter_ctxt.def_uc_addr)) {
		ret = 0;
	}

	return ret;
}

s32 sxevf_uc_addr_set(struct sxevf_hw *hw, u8 *uc_addr)
{
	s32 ret;

	if (hw->board_type == SXE_BOARD_VF_HV) {
		ret = __sxevf_hv_uc_addr_set(hw, uc_addr);
	} else {
		ret = __sxevf_uc_addr_set(hw, uc_addr);
	}

	return ret;
}

STATIC s32 __sxevf_cast_mode_set(struct sxevf_hw *hw, enum sxevf_cast_mode mode)
{
	struct sxevf_cast_mode_msg msg = {};
	s32 ret;
	struct sxevf_adapter *adapter = hw->adapter;

	msg.msg_type = SXEVF_CAST_MODE_SET;
	msg.cast_mode = mode;

	ret = sxevf_send_and_rcv_msg(hw, (u32 *)&msg, SXEVF_MSG_NUM(sizeof(msg)));
	if (ret || (msg.msg_type != (SXEVF_CAST_MODE_SET | SXEVF_MSGTYPE_ACK))) {
		ret = ret ? ret : -SXEVF_ERR_MSG_HANDLE_ERR;
	}

	LOG_INFO_BDF("msg_type:0x%x mode:0x%x msg result:0x%x.(ret:%d)\n",
		   msg.msg_type, mode, msg.msg_type, ret);

	return ret;
}

STATIC s32 __sxevf_hv_cast_mode_set(struct sxevf_hw *hw, enum sxevf_cast_mode mode)
{
	return -EOPNOTSUPP;
}

s32 sxevf_cast_mode_set(struct sxevf_hw *hw, enum sxevf_cast_mode mode)
{
	s32 ret;

	if (hw->board_type == SXE_BOARD_VF_HV) {
		ret = __sxevf_hv_cast_mode_set(hw, mode);
	} else {
		ret = __sxevf_cast_mode_set(hw, mode);
	}

	return ret;

}

static u16 sxevf_mc_addr_extract(u8 *mc_addr)
{
	u16 result = ((mc_addr[4] >> 4) | (((u16)mc_addr[5]) << 4));

        result &= 0xFFF;

	LOG_INFO("extract result:0x%03x mc_addr:%pM\n", result, mc_addr);

        return result;
}

STATIC s32 __sxevf_mc_addr_sync(struct sxevf_hw *hw, struct net_device *netdev)
{
	struct netdev_hw_addr *hw_addr;
	struct sxevf_mc_sync_msg msg = {};
	u16 mc_cnt = min_t(u16, netdev_mc_count(netdev), SXEVF_MC_ENTRY_NUM_MAX);
	s32 ret;
	u8 i = 0; 
	u32 result;
	struct sxevf_adapter *adapter = hw->adapter;

	msg.msg_type = SXEVF_MC_ADDR_SYNC;
	msg.mc_cnt = mc_cnt;

	netdev_for_each_mc_addr(hw_addr, netdev) {
		if ((i < mc_cnt) &&
			!is_link_local_ether_addr(hw_addr->addr)) {
			msg.mc_addr_extract[i++] = sxevf_mc_addr_extract(hw_addr->addr);
		}
	}

	ret = sxevf_send_and_rcv_msg(hw, (u32 *)&msg, SXEVF_MSG_NUM(sizeof(msg)));
	result = *(u32 *)&msg;

	if (ret || ((result & SXEVF_MC_ADDR_SYNC) &&
		    (result & SXEVF_MSGTYPE_NACK))) {
		ret = ret ? ret : -SXEVF_ERR_MSG_HANDLE_ERR;
		goto l_out;
	}

	LOG_INFO_BDF("msg_type:0x%x len:%zu mc_cnt:%d msg "
		  "result:0x%x.(ret:%d)\n",
		  msg.msg_type, SXEVF_MSG_NUM(sizeof(msg)),
		  mc_cnt, result, ret);

l_out:
	return ret;
}

STATIC s32 __sxevf_hv_mc_addr_sync(struct sxevf_hw *hw, struct net_device *netdev)
{
	return -EOPNOTSUPP;
}

s32 sxevf_mc_addr_sync(struct sxevf_hw *hw, struct net_device *netdev)
{
	s32 ret;

	if (hw->board_type == SXE_BOARD_VF_HV) {
		ret = __sxevf_hv_mc_addr_sync(hw, netdev);
	} else {
		ret = __sxevf_mc_addr_sync(hw, netdev);
	}

	return ret;
}

STATIC s32 __sxevf_per_uc_addr_sync(struct sxevf_hw *hw, u16 index, u8 *addr)
{
	struct sxevf_uc_sync_msg msg = {};
	s32 ret;
	u32 result;
	u32 check;
	struct sxevf_adapter *adapter = hw->adapter;

	msg.msg_type = SXEVF_UC_ADDR_SYNC;
	msg.index = index;
	check = *(u32 *)&msg;

	if (addr) {
		ether_addr_copy((u8 *)&msg.addr, addr);
	}

	ret = sxevf_send_and_rcv_msg(hw, (u32 *)&msg, SXEVF_MSG_NUM(sizeof(msg)));
	result = *(u32 *)&msg;

	if (ret || (result != (check | SXEVF_MSGTYPE_ACK))) {
		ret = ret ? ret : -SXEVF_ERR_MSG_HANDLE_ERR;
	}

	LOG_INFO_BDF("msg_type:0x%x index:%d addr:%pM sync done "
		 " result:0x%x msg.(ret:%d)\n",
		 msg.msg_type, index, addr, result, ret);

	return ret;
}

STATIC s32 __sxevf_hv_per_uc_addr_sync(struct sxevf_hw *hw, u16 index, u8 *addr)
{
	return -EOPNOTSUPP;
}

static s32 sxevf_per_uc_addr_sync(struct sxevf_hw *hw, u16 index, u8 *addr)
{
	s32 ret;

	if (hw->board_type == SXE_BOARD_VF_HV) {
		ret = __sxevf_hv_per_uc_addr_sync(hw, index, addr);
	} else {
		ret = __sxevf_per_uc_addr_sync(hw, index, addr);
	}

	return ret;
}

s32 sxevf_uc_addr_sync(struct sxevf_hw *hw, struct net_device *netdev)
{
	u32 index = 0;
	u32 uc_cnt = netdev_uc_count(netdev);
	s32 ret = 0;
	struct sxevf_adapter *adapter = hw->adapter;

	if (uc_cnt > SXEVF_UC_ENTRY_NUM_MAX) {
		ret = -SXEVF_ERR_ARGUMENT_INVALID;
		LOG_DEV_ERR("dev uc list cnt:%d exceed limit:10.(err:%d)\n",
			  uc_cnt, ret);
		goto l_out;
	}

	if (uc_cnt) {
		struct netdev_hw_addr *hw_addr;
		LOG_INFO_BDF("uc_cnt:%u.\n", uc_cnt);
		netdev_for_each_uc_addr(hw_addr, netdev) {
			sxevf_per_uc_addr_sync(hw, ++index, hw_addr->addr);
			udelay(SXEVF_SYNC_UCADDR_DELAY_TIME);
		}
	} else {
		LOG_INFO_BDF("dev uc list null, send msg to pf to clear vf mac list.\n");
		sxevf_per_uc_addr_sync(hw, 0, NULL);
	}

l_out:
	return ret;
}

STATIC s32 __sxevf_redir_tbl_get(struct sxevf_hw *hw, int rx_ring_num, u32 *redir_tbl)
{
	s32 ret;
	u32 i, j;
	u32 mask = 0;
	struct sxevf_redir_tbl_msg msg = {};
	struct sxevf_adapter *adapter = hw->adapter;

	msg.type = SXEVF_REDIR_TBL_GET;

	ret = sxevf_send_and_rcv_msg(hw, (u32 *)&msg, SXEVF_MSG_NUM(sizeof(msg)));
	if (ret) {
		LOG_ERROR_BDF("send and rece err, ret=%d\n", ret);
		goto l_end;
	}

	if ((msg.type & SXEVF_REDIR_TBL_GET) && (msg.type & SXEVF_MSGTYPE_NACK)) {
		LOG_ERROR_BDF("req has been refused, msg type=%x\n", msg.type);
		ret = -EPERM;
		goto l_end;
	}

	if (rx_ring_num > 1) {
		mask = 0x1;
	}

	for (i = 0; i < SXEVF_RETA_ENTRIES_DWORDS; i++) {
		for (j = 0; j < SXEVF_REDIR_TBL_ENTRY_NUM_PER_UINT; j++) {
			redir_tbl[i * SXEVF_REDIR_TBL_ENTRY_NUM_PER_UINT + j] =
				(msg.entries[i] >> (SXEVF_REDIR_TBL_ENTRY_BITS * j)) & mask;
		}
	}

l_end:
	return ret;
}

STATIC s32 __sxevf_hv_redir_tbl_get(struct sxevf_hw *hw, int rx_ring_num, u32 *redir_tbl)
{
	return -EOPNOTSUPP;
}

s32 sxevf_redir_tbl_get(struct sxevf_hw *hw, int rx_ring_num, u32 *redir_tbl)
{
	s32 ret;

	if (hw->board_type == SXE_BOARD_VF_HV) {
		ret = __sxevf_hv_redir_tbl_get(hw, rx_ring_num, redir_tbl);
	} else {
		ret = __sxevf_redir_tbl_get(hw, rx_ring_num, redir_tbl);
	}

	return ret;
}

STATIC s32 __sxevf_rss_hash_key_get(struct sxevf_hw *hw, u8 *rss_key)
{
	s32 ret;
	struct sxevf_adapter *adapter = hw->adapter;

	struct sxevf_rss_hsah_key_msg msg;
	msg.type = SXEVF_RSS_KEY_GET;

	ret = sxevf_send_and_rcv_msg(hw, (u32 *)&msg, SXEVF_MSG_NUM(sizeof(msg)));
	if (ret) {
		LOG_ERROR_BDF("send and rece err, ret=%d\n", ret);
		goto l_end;
	}

	if ((msg.type & SXEVF_RSS_KEY_GET) && (msg.type & SXEVF_MSGTYPE_NACK)) {
		LOG_ERROR_BDF("req has been refused, msg type=%x\n", msg.type);
		ret = -EPERM;
		goto l_end;
	}

	memcpy(rss_key, msg.hash_key, SXEVF_RSS_HASH_KEY_SIZE);

l_end:
	return ret;
}

STATIC s32 __sxevf_hv_rss_hash_key_get(struct sxevf_hw *hw, u8 *rss_key)
{
	return -EOPNOTSUPP;
}

s32 sxevf_rss_hash_key_get(struct sxevf_hw *hw, u8 *rss_key)
{
	s32 ret;

	if (hw->board_type == SXE_BOARD_VF_HV) {
		ret = __sxevf_hv_rss_hash_key_get(hw, rss_key);
	} else {
		ret = __sxevf_rss_hash_key_get(hw, rss_key);
	}

	return ret;
}

STATIC s32 __sxevf_rx_max_frame_set(struct sxevf_hw *hw, u32 max_size)
{
	struct sxevf_max_frame_msg msg = {};
	s32 ret;
	struct sxevf_adapter *adapter = hw->adapter;

	msg.msg_type = SXEVF_LPE_SET;
	msg.max_frame = max_size;

	ret = sxevf_send_and_rcv_msg(hw, (u32 *)&msg,
					     SXEVF_MSG_NUM(sizeof(msg)));
	if (ret || ((msg.msg_type & SXEVF_LPE_SET) &&
		    (msg.msg_type & SXEVF_MSGTYPE_NACK))) {
		ret = ret ? ret : -SXEVF_ERR_MSG_HANDLE_ERR;
	}

	LOG_INFO_BDF("msg_type:0x%x max_frame:0x%x (ret:%d)\n",
		   msg.msg_type, msg.max_frame, ret);

	return ret;
}

STATIC s32 __sxevf_hv_rx_max_frame_set(struct sxevf_hw *hw, u32 max_size)
{
	return 0;
}

s32 sxevf_rx_max_frame_set(struct sxevf_hw *hw, u32 max_size)
{
	s32 ret;

	if (hw->board_type == SXE_BOARD_VF_HV) {
		ret = __sxevf_hv_rx_max_frame_set(hw, max_size);
	} else {
		ret = __sxevf_rx_max_frame_set(hw, max_size);
	}

	return ret;
}

STATIC s32 __sxe_vf_filter_array_vid_update(struct sxevf_hw *hw, u32 vlan,
						 bool vlan_on)
{
	struct sxevf_vlan_filter_msg msg = {};
	s32 ret;
	struct sxevf_adapter *adapter = hw->adapter;

	msg.msg_type = SXEVF_VLAN_SET;
	msg.vlan_id = vlan;
	msg.msg_type |= vlan_on << SXEVF_MSGINFO_SHIFT;

	LOG_INFO_BDF("update vlan[%u], vlan on = %s\n", vlan, vlan_on ? "yes" : "no");
	ret = sxevf_send_and_rcv_msg(hw, (u32 *)&msg,
					     SXEVF_MSG_NUM(sizeof(msg)));
	LOG_INFO_BDF("update vlan[%u] ret = %d\n",vlan, ret);

	msg.msg_type &= ~(0xFF << SXEVF_MSGINFO_SHIFT);

	if (ret || (msg.msg_type != (SXEVF_VLAN_SET | SXEVF_MSGTYPE_ACK))) {
		ret = ret ? ret : -SXEVF_ERR_MSG_HANDLE_ERR;
	}

	return ret;
}

STATIC s32 __sxevf_hv_filter_array_vid_update(struct sxevf_hw *hw, u32 vlan, bool vlan_on)
{
	return -EOPNOTSUPP;
}

s32 sxe_vf_filter_array_vid_update(struct sxevf_hw *hw, u32 vlan,
						 bool vlan_on)
{
	s32 ret;

	if (hw->board_type == SXE_BOARD_VF_HV) {
		ret = __sxevf_hv_filter_array_vid_update(hw, vlan, vlan_on);
	} else {
		ret = __sxe_vf_filter_array_vid_update(hw, vlan, vlan_on);
	}

	return ret;
}

