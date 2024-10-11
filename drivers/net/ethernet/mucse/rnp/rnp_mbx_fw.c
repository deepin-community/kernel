// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#include <linux/wait.h>
#include <linux/sem.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>

#include "rnp.h"
#include "rnp_mbx.h"
#include "rnp_mbx_fw.h"

#define RNP_FW_MAILBOX_SIZE RNP_VFMAILBOX_SIZE

static bool is_cookie_valid(struct rnp_hw *hw, void *cookie)
{
	unsigned char* begin = (unsigned char*)(&hw->mbx.cookie_pool.cookies[0]);
	unsigned char* end = (unsigned char*)(&hw->mbx.cookie_pool.cookies[MAX_COOKIES_ITEMS]);
	if(((unsigned char*)cookie)>=begin && ((unsigned char*)cookie)< end){
		return true;
	}
	return false;
}

static struct mbx_req_cookie *mbx_cookie_zalloc(struct rnp_hw *hw, int priv_len)
{
	struct mbx_req_cookie *cookie = NULL;
	int loop_cnt = MAX_COOKIES_ITEMS, i;
	bool find = false;

	u64 now_jiffies = get_jiffies_64();

	if (mutex_lock_interruptible(&hw->mbx.lock)) {
		rnp_err("[%s] get mbx lock failed,priv_len:%d\n", __func__, priv_len);
		return NULL;
	}
	i = hw->mbx.cookie_pool.next_idx;
	while(loop_cnt--){
		cookie = &(hw->mbx.cookie_pool.cookies[i]);
		if(cookie->stat == COOKIE_FREE || 
			/* force free cookie if cookie not freed after 120 seconds */
			time_after64(now_jiffies,cookie->alloced_jiffies + (2 * 60) * HZ)){
			find = true;
			cookie->alloced_jiffies = get_jiffies_64();
			cookie->stat = COOKIE_ALLOCED;
			hw->mbx.cookie_pool.next_idx =  (i+1)%MAX_COOKIES_ITEMS;
			break;
		}
		i = (i+1)%MAX_COOKIES_ITEMS;
	}
	mutex_unlock(&hw->mbx.lock);

	if (!find) {
		rnp_err("[%s] no free cookies availble\n", __func__);
		return NULL;
	}

	cookie->timeout_jiffes = 30 * HZ;
	cookie->priv_len = priv_len;

	return cookie;
}

static void mbx_free_cookie(struct mbx_req_cookie *cookie, bool force_free)
{
	if (!cookie)
		return;

	if (force_free) {
		cookie->stat = COOKIE_FREE;
	} else {
		cookie->stat = COOKIE_FREE_WAIT_TIMEOUT;
	}
}

static int rnp_mbx_write_posted_locked(struct rnp_hw *hw, struct mbx_fw_cmd_req *req)
{
	int err = 0;
	int retry = 3;

	if (mutex_lock_interruptible(&hw->mbx.lock)) {
		rnp_err("[%s] get mbx lock failed opcode:0x%x\n", __func__,
			req->opcode);
		return -EAGAIN;
	}

	rnp_logd(LOG_MBX_LOCK, "%s %d lock:%p hw:%p opcode:0x%x\n", __func__,
		 hw->pfvfnum, &hw->mbx.lock, hw, req->opcode);

try_again:
	retry--;
	if (retry < 0) {
		mutex_unlock(&hw->mbx.lock);
		rnp_err("%s: write_posted failed! err:0x%x opcode:0x%x\n",
			__func__, err, req->opcode);
		return -EIO;
	}

	err = hw->mbx.ops.write_posted(
		hw, (u32 *)req, (req->datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);
	if (err) {
		goto try_again;
	}
	mutex_unlock(&hw->mbx.lock);

	return err;
}

static void rnp_link_stat_mark_reset(struct rnp_hw *hw)
{
	wr32(hw, RNP_DMA_DUMY, 0xa5a40000);
}

static void rnp_link_stat_mark_disable(struct rnp_hw *hw)
{
	wr32(hw, RNP_DMA_DUMY, 0);
}

static int rnp_mbx_fw_post_req(struct rnp_hw *hw, struct mbx_fw_cmd_req *req,
			       struct mbx_req_cookie *cookie)
{
	int err = 0;
	struct rnp_adapter *adpt = hw->back;

	cookie->errcode = 0;
	cookie->done = 0;
	init_waitqueue_head(&cookie->wait);

	if (mutex_lock_interruptible(&hw->mbx.lock)) {
		rnp_err("[%s] wait mbx lock timeout pfvf:0x%x opcode:0x%x\n",
			__func__, hw->pfvfnum, req->opcode);
		return -EAGAIN;
	}

	rnp_logd(LOG_MBX_LOCK, "%s %d lock:%p hw:%p opcode:0x%x\n", __func__,
		 hw->pfvfnum, &hw->mbx.lock, hw, req->opcode);

	err = rnp_write_mbx(hw, (u32 *)req,
			    (req->datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);
	if (err) {
		rnp_err("rnp_write_mbx failed! err:%d opcode:0x%x\n", err,
			req->opcode);
		mutex_unlock(&hw->mbx.lock);
		return err;
	}

	if (cookie->timeout_jiffes != 0) {
		int retry_cnt = 4;
retry:
		err = wait_event_interruptible_timeout(cookie->wait,
						       cookie->done == 1,
						       cookie->timeout_jiffes);

		if (err == -ERESTARTSYS && retry_cnt) {
			retry_cnt--;
			goto retry;
		}
		if (err == 0) {
			rnp_err("[%s] %s failed! pfvfnum:0x%x hw:%p timeout err:%d opcode:%x\n",
				adpt->name, __func__, hw->pfvfnum, hw, err,
				req->opcode);
			err = -ETIME;
		} else if (err > 0) {
			err = 0;
		}
	} else {
		wait_event_interruptible(cookie->wait, cookie->done == 1);
	}

	mutex_unlock(&hw->mbx.lock);

	if (cookie->errcode) {
		err = cookie->errcode;
	}

	return err;
}

static int rnp_fw_send_cmd_wait(struct rnp_hw *hw, struct mbx_fw_cmd_req *req,
			 struct mbx_fw_cmd_reply *reply)
{
	int err;
	int retry_cnt = 3;

	if (!hw || !req || !reply || !hw->mbx.ops.read_posted) {
		printk("error: hw:%p req:%p reply:%p\n", hw, req, reply);
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&hw->mbx.lock)) {
		rnp_err("[%s] get mbx lock failed opcode:0x%x\n", __func__,
			req->opcode);
		return -EAGAIN;
	}

	rnp_logd(LOG_MBX_LOCK, "%s %d lock:%p hw:%p opcode:0x%x\n", __func__,
		 hw->pfvfnum, &hw->mbx.lock, hw, req->opcode);
	err = hw->mbx.ops.write_posted(
		hw, (u32 *)req, (req->datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);
	if (err) {
		rnp_err("%s: write_posted failed! err:0x%x opcode:0x%x\n",
			__func__, err, req->opcode);
		mutex_unlock(&hw->mbx.lock);
		return err;
	}

retry:
	retry_cnt--;
	if (retry_cnt < 0) {
		rnp_err("retry timeout opcode:0x%x\n", req->opcode);
		return -EIO;
	}
	err = hw->mbx.ops.read_posted(hw, (u32 *)reply, sizeof(*reply) / 4,
				      MBX_FW);
	if (err) {
		rnp_err("%s: read_posted failed! err:0x%x opcode:0x%x\n",
			__func__, err, req->opcode);
		mutex_unlock(&hw->mbx.lock);
		return err;
	}
	if (reply->opcode != req->opcode)
		goto retry;

	mutex_unlock(&hw->mbx.lock);

	if (reply->error_code) {
		rnp_err("%s: reply err:0x%x req:0x%x\n", __func__,
			reply->error_code, req->opcode);
		return -reply->error_code;
	}
	return 0;
}

int wait_mbx_init_done(struct rnp_hw *hw)
{
	int count = 10000;
	u32 v = rd32(hw, RNP_TOP_NIC_DUMMY);

	while (count) {
		v = rd32(hw, RNP_TOP_NIC_DUMMY);
		if (((v & 0xFF000000) == 0xa5000000) && (v & 0x80))
			break;

		usleep_range(500, 1000);
		printk("waiting fw up\n");
		count--;
	}
	printk("fw init ok %x\n", v);

	return 0;
}

int rnp_mbx_get_lane_stat(struct rnp_hw *hw)
{
	int err = 0;
	struct mbx_fw_cmd_req req;
	struct rnp_adapter *adpt = hw->back;
	struct lane_stat_data *st;
	struct mbx_req_cookie *cookie = NULL;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));

	if (hw->mbx.other_irq_enabled) {
		cookie = mbx_cookie_zalloc(hw,sizeof(struct lane_stat_data));
		if (!cookie) {
			rnp_err("%s: no memory\n", __func__);
			return -ENOMEM;
		}
		st = (struct lane_stat_data *)cookie->priv;

		build_get_lane_status_req(&req, hw->nr_lane, cookie);

		err = rnp_mbx_fw_post_req(hw, &req, cookie);
		if (err) {
			rnp_err("%s: error:%d\n", __func__, err);
			goto quit;
		}
	} else {
		memset(&reply, 0, sizeof(reply));

		build_get_lane_status_req(&req, hw->nr_lane, &req);
		err = rnp_fw_send_cmd_wait(hw, &req, &reply);
		if (err) {
			rnp_err("%s: 1 error:%d\n", __func__, err);
			goto quit;
		}
		st = (struct lane_stat_data *)&(reply.data);
	}

	hw->phy_type = st->phy_type;
	hw->speed = adpt->speed = st->speed;
	if ((st->is_sgmii) || (hw->phy_type == PHY_TYPE_10G_TP)) {
		adpt->phy_addr = st->phy_addr;
	} else {
		adpt->sfp.fault = st->sfp.fault;
		adpt->sfp.los = st->sfp.los;
		adpt->sfp.mod_abs = st->sfp.mod_abs;
		adpt->sfp.tx_dis = st->sfp.tx_dis;
	}
	adpt->si.main = st->si_main;
	adpt->si.pre = st->si_pre;
	adpt->si.post = st->si_post;
	adpt->si.tx_boost = st->si_tx_boost;
	adpt->link_traing = st->link_traing;
	adpt->fec = st->fec;
	hw->is_sgmii = st->is_sgmii;
	hw->pci_gen = st->pci_gen;
	hw->pci_lanes = st->pci_lanes;
	adpt->speed = st->speed;
	adpt->hw.link = st->linkup;
	hw->is_backplane = st->is_backplane;
	hw->supported_link = st->supported_link;
	hw->advertised_link = st->advertised_link;
	hw->tp_mdx = st->tp_mdx;

	if ((hw->hw_type == rnp_hw_n10) || (hw->hw_type == rnp_hw_n400)) {
		if (hw->fw_version >= 0x00050000) {
			hw->sfp_connector = st->sfp_connector;
			hw->duplex = st->duplex;
			adpt->an = st->autoneg;
		} else {
			hw->sfp_connector = 0xff;
			hw->duplex = 1;
			adpt->an = st->an;
		}
		if (hw->fw_version <= 0x00050000) {
			hw->supported_link |= RNP_LINK_SPEED_10GB_FULL |
					      RNP_LINK_SPEED_1GB_FULL;
		}
	}

	rnp_logd(
		LOG_MBX_LINK_STAT,
		"%s:pma_type:0x%x phy_type:0x%x,linkup:%d duplex:%d auton:%d "
		"fec:%d an:%d lt:%d is_sgmii:%d supported_link:0x%x, backplane:%d "
		"speed:%d sfp_connector:0x%x\n",
		adpt->name, st->pma_type, st->phy_type, st->linkup, st->duplex,
		st->autoneg, st->fec, st->an, st->link_traing, st->is_sgmii,
		hw->supported_link, hw->is_backplane, st->speed,
		st->sfp_connector);
quit:
	if (cookie)
		mbx_free_cookie(cookie, err ? false : true);

	return err;
}

int rnp_mbx_get_link_stat(struct rnp_hw *hw)
{
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_get_link_status_req(&req, hw->nr_lane, &req);
	return rnp_fw_send_cmd_wait(hw, &req, &reply);
}

int rnp_mbx_fw_reset_phy(struct rnp_hw *hw)
{
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;
	int ret;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	if (hw->mbx.other_irq_enabled) {
		struct mbx_req_cookie *cookie = mbx_cookie_zalloc(hw,0);

		if (!cookie) {
			return -ENOMEM;
		}

		build_reset_phy_req(&req, cookie);

		ret = rnp_mbx_fw_post_req(hw, &req, cookie);
		mbx_free_cookie(cookie,ret?false:true);
		return ret;
	} else {
		build_reset_phy_req(&req, &req);
		return rnp_fw_send_cmd_wait(hw, &req, &reply);
	}
}

int rnp_maintain_req(struct rnp_hw *hw, int cmd, int arg0, int req_data_bytes,
		     int reply_bytes, dma_addr_t dma_phy_addr)
{
	int err;
	struct mbx_req_cookie *cookie = NULL;
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;
	u64 address = dma_phy_addr;

	cookie = mbx_cookie_zalloc(hw,0);
	if (!cookie) {
		return -ENOMEM;
	}

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));
	cookie->timeout_jiffes = 60 * HZ;

	build_maintain_req(&req, cookie, cmd, arg0, req_data_bytes, reply_bytes,
			   address & 0xffffffff, (address >> 32) & 0xffffffff);

	if (hw->mbx.other_irq_enabled) {
		cookie->timeout_jiffes = 400 * HZ;
		err = rnp_mbx_fw_post_req(hw, &req, cookie);
	} else {
		int old_mbx_timeout = hw->mbx.timeout;
		hw->mbx.timeout =
			(400 * 1000 * 1000) / hw->mbx.usec_delay;
		err = rnp_fw_send_cmd_wait(hw, &req, &reply);
		hw->mbx.timeout = old_mbx_timeout;
	}

	if (cookie)
		mbx_free_cookie(cookie,err?false:true);

	return (err) ? -EIO : 0;
}

int rnp_fw_get_macaddr(struct rnp_hw *hw, int pfvfnum, u8 *mac_addr,
		       int nr_lane)
{
	int err;
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	rnp_dbg("%s: pfvfnum:0x%x nr_lane:%d\n", __func__, pfvfnum, nr_lane);

	if (!mac_addr) {
		rnp_err("%s: mac_addr is null\n", __func__);
		return -EINVAL;
	}

	if (hw->mbx.other_irq_enabled) {
		struct mbx_req_cookie *cookie =
			mbx_cookie_zalloc(hw,sizeof(reply.mac_addr));
		struct mac_addr *mac = (struct mac_addr *)cookie->priv;

		if (!cookie) {
			return -ENOMEM;
		}

		build_get_macaddress_req(&req, 1 << nr_lane, pfvfnum, cookie);

		err = rnp_mbx_fw_post_req(hw, &req, cookie);
		if (err) {
			mbx_free_cookie(cookie,false);
			return err;
		}
		hw->pcode = mac->pcode;

		if ((1 << nr_lane) & mac->lanes) {
			memcpy(mac_addr, mac->addrs[nr_lane].mac, 6);
		}

		mbx_free_cookie(cookie,true);
		return 0;
	} else {
		build_get_macaddress_req(&req, 1 << nr_lane, pfvfnum, &req);
		err = rnp_fw_send_cmd_wait(hw, &req, &reply);
		if (err) {
			rnp_err("%s: failed. err:%d\n", __func__, err);
			return err;
		}

		hw->pcode = reply.mac_addr.pcode;
		if ((1 << nr_lane) & reply.mac_addr.lanes) {
			memcpy(mac_addr, reply.mac_addr.addrs[nr_lane].mac, 6);
			return 0;
		}
	}

	return -ENODATA;
}

static int rnp_mbx_sfp_read(struct rnp_hw *hw, int sfp_i2c_addr, int reg,
			    int cnt, u8 *out_buf)
{
	struct mbx_fw_cmd_req req;
	int err = -EIO;
	int nr_lane = hw->nr_lane;

	if ((cnt > MBX_SFP_READ_MAX_CNT) || !out_buf) {
		rnp_err("%s: cnt:%d should <= %d out_buf:%p\n", __func__, cnt,
			MBX_SFP_READ_MAX_CNT, out_buf);
		return -EINVAL;
	}

	memset(&req, 0, sizeof(req));

	if (hw->mbx.other_irq_enabled) {
		struct mbx_req_cookie *cookie = mbx_cookie_zalloc(hw,cnt);
		if (!cookie) {
			return -ENOMEM;
		}
		build_mbx_sfp_read(&req, nr_lane, sfp_i2c_addr, reg, cnt,
				   cookie);

		err = rnp_mbx_fw_post_req(hw, &req, cookie);
		if (err) {
			mbx_free_cookie(cookie,false);
			return err;
		} else {
			memcpy(out_buf, cookie->priv, cnt);
			err = 0;
			mbx_free_cookie(cookie,true);
		}
	} else {
		struct mbx_fw_cmd_reply reply;

		memset(&reply, 0, sizeof(reply));
		build_mbx_sfp_read(&req, nr_lane, sfp_i2c_addr, reg, cnt,
				   &reply);

		err = rnp_fw_send_cmd_wait(hw, &req, &reply);
		if (err == 0) {
			memcpy(out_buf, reply.sfp_read.value, cnt);
		}
	}

	return err;
}

int rnp_mbx_sfp_module_eeprom_info(struct rnp_hw *hw, int sfp_addr, int reg,
				   int data_len, u8 *buf)
{
	int left = data_len;
	int cnt, err;

	do {
		cnt = (left > MBX_SFP_READ_MAX_CNT) ? MBX_SFP_READ_MAX_CNT :
						      left;
		err = rnp_mbx_sfp_read(hw, sfp_addr, reg, cnt, buf);
		if (err) {
			rnp_err("%s: error:%d\n", __func__, err);
			return err;
		}
		reg += cnt;
		buf += cnt;
		left -= cnt;
	} while (left > 0);

	return 0;
}

int rnp_mbx_sfp_write(struct rnp_hw *hw, int sfp_addr, int reg, short v)
{
	struct mbx_fw_cmd_req req;
	int err;
	int nr_lane = hw->nr_lane;

	memset(&req, 0, sizeof(req));

	build_mbx_sfp_write(&req, nr_lane, sfp_addr, reg, v);
	err = rnp_mbx_write_posted_locked(hw, &req);

	return err;
}

int rnp_mbx_fw_reg_read(struct rnp_hw *hw, int fw_reg)
{
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;
	int err, ret = 0xffffffff;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	if (hw->fw_version < 0x00050200) {
		return -EOPNOTSUPP;
	}

	if (hw->mbx.other_irq_enabled) {
		struct mbx_req_cookie *cookie =
			mbx_cookie_zalloc(hw,sizeof(reply.r_reg));

		build_readreg_req(&req, fw_reg, cookie);
		err = rnp_mbx_fw_post_req(hw, &req, cookie);
		if (err) {
			mbx_free_cookie(cookie,false);
			return ret;
		}
		ret = ((int *)(cookie->priv))[0];
		mbx_free_cookie(cookie,true);
	} else {
		build_readreg_req(&req, fw_reg, &reply);
		err = rnp_fw_send_cmd_wait(hw, &req, &reply);
		if (err) {
			rnp_err("%s: failed. err:%d\n", __func__, err);
			return err;
		} else {
			ret = reply.r_reg.value[0];
		}
	}
	return ret;
}

int rnp_mbx_reg_write(struct rnp_hw *hw, int fw_reg, int value)
{
	struct mbx_fw_cmd_req req;
	int err;
	memset(&req, 0, sizeof(req));

	if (hw->fw_version < 0x00050200) {
		return -EOPNOTSUPP;
	}

	build_writereg_req(&req, NULL, fw_reg, 4, &value);

	err = rnp_mbx_write_posted_locked(hw, &req);
	return err;
}

int rnp_mbx_reg_writev(struct rnp_hw *hw, int fw_reg, int value[4], int bytes)
{
	struct mbx_fw_cmd_req req;
	int err;
	memset(&req, 0, sizeof(req));

	build_writereg_req(&req, NULL, fw_reg, bytes, value);

	err = rnp_mbx_write_posted_locked(hw, &req);
	return err;
}

int rnp_mbx_wol_set(struct rnp_hw *hw, u32 mode)
{
	struct mbx_fw_cmd_req req;
	int err;
	int nr_lane = hw->nr_lane;

	memset(&req, 0, sizeof(req));

	build_mbx_wol_set(&req, nr_lane, mode);

	err = rnp_mbx_write_posted_locked(hw, &req);
	return err;
}

int rnp_mbx_set_dump(struct rnp_hw *hw, int flag)
{
	int err;
	struct mbx_fw_cmd_req req;

	memset(&req, 0, sizeof(req));
	build_set_dump(&req, hw->nr_lane, flag);

	err = rnp_mbx_write_posted_locked(hw, &req);

	return err;
}

int rnp_mbx_force_speed(struct rnp_hw *hw, int speed)
{
	int cmd = 0x01150000;

	if (hw->force_10g_1g_speed_ablity == 0)
		return -EINVAL;

	hw->saved_force_link_speed = speed;
	if (speed == RNP_LINK_SPEED_10GB_FULL) {
		cmd = 0x01150002;
		hw->force_speed_stat = FORCE_SPEED_STAT_10G;
	} else if (speed == RNP_LINK_SPEED_1GB_FULL) {
		cmd = 0x01150001;
		hw->force_speed_stat = FORCE_SPEED_STAT_1G;
	} else {
		cmd = 0x01150000;
		hw->force_speed_stat = FORCE_SPEED_STAT_DISABLED;
	}
	return rnp_mbx_set_dump(hw, cmd);
}

int rnp_mbx_get_dump(struct rnp_hw *hw, int flags, u8 *data_out, int bytes)
{
	int err;
	struct mbx_req_cookie *cookie = NULL;

	struct mbx_fw_cmd_reply reply;
	struct mbx_fw_cmd_req req;
	struct get_dump_reply *get_dump;

	void *dma_buf = NULL;
	dma_addr_t dma_phy = 0;
	u64 address;

	cookie = mbx_cookie_zalloc(hw,sizeof(*get_dump));
	if (!cookie) {
		return -ENOMEM;
	}
	get_dump = (struct get_dump_reply *)cookie->priv;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	if (bytes > sizeof(get_dump->data)) {
		dma_buf = dma_alloc_coherent(&hw->pdev->dev, bytes, &dma_phy,
					     GFP_ATOMIC);
		if (!dma_buf) {
			dev_err(&hw->pdev->dev, "%s: no memory:%d!", __func__,
				bytes);
			err = -ENOMEM;
			goto quit;
		}
	}
	address = dma_phy;
	build_get_dump_req(&req, cookie, hw->nr_lane, address & 0xffffffff,
			   (address >> 32) & 0xffffffff, bytes);

	if (hw->mbx.other_irq_enabled) {
		err = rnp_mbx_fw_post_req(hw, &req, cookie);
	} else {
		err = rnp_fw_send_cmd_wait(hw, &req, &reply);
		get_dump = &reply.get_dump;
	}

quit:
	if (err == 0) {
		hw->dump.version = get_dump->version;
		hw->dump.flag = get_dump->flags;
		hw->dump.len = get_dump->bytes;
	}
	if (err == 0 && data_out) {
		if (dma_buf) {
			memcpy(data_out, dma_buf, bytes);
		} else {
			memcpy(data_out, get_dump->data, bytes);
		}
	}
	if (dma_buf)
		dma_free_coherent(&hw->pdev->dev, bytes, dma_buf, dma_phy);

	if (cookie)
		mbx_free_cookie(cookie, err ? false : true);
	return err ? -err : 0;
}

int rnp_fw_update(struct rnp_hw *hw, int partition, const u8 *fw_bin, int bytes)
{
	int err;
	struct mbx_req_cookie *cookie = NULL;

	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	void *dma_buf = NULL;
	dma_addr_t dma_phy;

	cookie = mbx_cookie_zalloc(hw,0);
	if (!cookie) {
		dev_err(&hw->pdev->dev, "%s: no memory:%d!", __func__, 0);
		return -ENOMEM;
	}

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	dma_buf =
		dma_alloc_coherent(&hw->pdev->dev, bytes, &dma_phy, GFP_ATOMIC);
	if (!dma_buf) {
		dev_err(&hw->pdev->dev, "%s: no memory:%d!", __func__, bytes);
		err = -ENOMEM;
		goto quit;
	}

	memcpy(dma_buf, fw_bin, bytes);

	build_fw_update_req(&req, cookie, partition, dma_phy & 0xffffffff,
			    (dma_phy >> 32) & 0xffffffff, bytes);
	if (hw->mbx.other_irq_enabled) {
		cookie->timeout_jiffes = 400 * HZ;
		err = rnp_mbx_fw_post_req(hw, &req, cookie);
	} else {
		int old_mbx_timeout = hw->mbx.timeout;
		hw->mbx.timeout =
			(400 * 1000 * 1000) / hw->mbx.usec_delay;
		err = rnp_fw_send_cmd_wait(hw, &req, &reply);
		hw->mbx.timeout = old_mbx_timeout;
	}

quit:
	if (dma_buf)
		dma_free_coherent(&hw->pdev->dev, bytes, dma_buf, dma_phy);
	if (cookie)
		mbx_free_cookie(cookie, err ? false : true);
	printk("%s: %s (errcode:%d)\n", __func__, err ? " failed" : " success",
	       err);
	return (err) ? -EIO : 0;
}

int rnp_mbx_link_event_enable(struct rnp_hw *hw, int enable)
{
	struct mbx_fw_cmd_reply reply;
	struct mbx_fw_cmd_req req;
	int err;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	if (enable) {
		int v = rd32(hw, RNP_DMA_DUMY);
		v &= 0x0000ffff;
		v |= 0xa5a40000;
		wr32(hw, RNP_DMA_DUMY, v);
	} else {
		wr32(hw, RNP_DMA_DUMY, 0);
	}

	build_link_set_event_mask(&req, BIT(EVT_LINK_UP),
				  (enable & 1) << EVT_LINK_UP, &req);
	err = rnp_mbx_write_posted_locked(hw, &req);

	return err;
}

int rnp_fw_get_capability(struct rnp_hw *hw, struct phy_abilities *abil)
{
	int err;
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_phy_abalities_req(&req, &req);
	err = rnp_fw_send_cmd_wait(hw, &req, &reply);

	if (err == 0)
		memcpy(abil, &reply.phy_abilities, sizeof(*abil));

	return err;
}

static int to_mac_type(struct phy_abilities *ability)
{
	int lanes = hweight_long(ability->lane_mask);
	if ((ability->phy_type == PHY_TYPE_40G_BASE_KR4) ||
	    (ability->phy_type == PHY_TYPE_40G_BASE_LR4) ||
	    (ability->phy_type == PHY_TYPE_40G_BASE_CR4) ||
	    (ability->phy_type == PHY_TYPE_40G_BASE_SR4)) {
		if (lanes == 1) {
			return rnp_mac_n10g_x8_40G;
		} else {
			return rnp_mac_n10g_x8_10G;
		}
	} else if ((ability->phy_type == PHY_TYPE_10G_BASE_KR) ||
		   (ability->phy_type == PHY_TYPE_10G_BASE_LR) ||
		   (ability->phy_type == PHY_TYPE_10G_BASE_ER) ||
		   (ability->phy_type == PHY_TYPE_10G_BASE_SR)) {
		if (lanes == 1) {
			return rnp_mac_n10g_x2_10G;
		} else if (lanes == 2) {
			return rnp_mac_n10g_x4_10G;
		} else {
			return rnp_mac_n10g_x8_10G;
		}
	} else if (ability->phy_type == PHY_TYPE_1G_BASE_KX) {
		return rnp_mac_n10l_x8_1G;
	} else if (ability->phy_type == PHY_TYPE_SGMII) {
		return rnp_mac_n10l_x8_1G;
	}
	return rnp_mac_unknown;
}

int rnp_set_lane_fun(struct rnp_hw *hw, int fun, int value0, int value1,
		     int value2, int value3)
{
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_set_lane_fun(&req, hw->nr_lane, fun, value0, value1, value2,
			   value3);

	return rnp_mbx_write_posted_locked(hw, &req);
}

int rnp_mbx_ifinsmod(struct rnp_hw *hw, int status)
{
	int err;
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_ifinsmod(&req, hw->nr_lane, status);

	if (mutex_lock_interruptible(&hw->mbx.lock))
		return -EAGAIN;
	err = hw->mbx.ops.write_posted(
		hw, (u32 *)&req, (req.datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);

	mutex_unlock(&hw->mbx.lock);

	rnp_logd(LOG_MBX_IFUP_DOWN, "%s: lane:%d status:%d\n", __func__,
		 hw->nr_lane, status);
	return err;
}

int rnp_mbx_ifsuspuse(struct rnp_hw *hw, int status)
{
	int err;
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_ifsuspuse(&req, hw->nr_lane, status);

	if (mutex_lock_interruptible(&hw->mbx.lock))
		return -EAGAIN;
	err = hw->mbx.ops.write_posted(
		hw, (u32 *)&req, (req.datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);

	mutex_unlock(&hw->mbx.lock);

	rnp_logd(LOG_MBX_IFUP_DOWN, "%s: lane:%d status:%d\n", __func__,
		 hw->nr_lane, status);

	return err;
}

int rnp_mbx_ifforce_control_mac(struct rnp_hw *hw, int status)
{
	int err;
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_ifforce(&req, hw->nr_lane, status);

	if (mutex_lock_interruptible(&hw->mbx.lock))
		return -EAGAIN;

	err = hw->mbx.ops.write_posted(
		hw, (u32 *)&req, (req.datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);

	mutex_unlock(&hw->mbx.lock);

	rnp_logd(LOG_MBX_IFUP_DOWN, "%s: lane:%d status:%d\n", __func__,
		 hw->nr_lane, status);

	return err;
}

int rnp_mbx_ifup_down(struct rnp_hw *hw, int up)
{
	int err;
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_ifup_down(&req, hw->nr_lane, up);

	if (mutex_lock_interruptible(&hw->mbx.lock))
		return -EAGAIN;
	err = hw->mbx.ops.write_posted(
		hw, (u32 *)&req, (req.datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);

	mutex_unlock(&hw->mbx.lock);

	rnp_logd(LOG_MBX_IFUP_DOWN, "%s: lane:%d up:%d\n", __func__,
		 hw->nr_lane, up);

	/* force firmware report link-status */
	if (up)
		rnp_link_stat_mark_reset(hw);

	return err;
}

int rnp_mbx_led_set(struct rnp_hw *hw, int value)
{
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_led_set(&req, hw->nr_lane, value, &reply);

	return rnp_mbx_write_posted_locked(hw, &req);
}

int rnp_mbx_get_capability(struct rnp_hw *hw, struct rnp_info *info)
{
	int err;
	struct phy_abilities ablity;
	int try_cnt = 3;

	memset(&ablity, 0, sizeof(ablity));
	rnp_link_stat_mark_disable(hw);

	while (try_cnt--) {
		err = rnp_fw_get_capability(hw, &ablity);
		if (err == 0 && info) {
			hw->lane_mask = ablity.lane_mask & 0xf;
			info->mac = to_mac_type(&ablity);
			info->adapter_cnt = hweight_long(hw->lane_mask);
			hw->mode = ablity.nic_mode;
			hw->pfvfnum = ablity.pfnum;
			hw->speed = ablity.speed;
			hw->nr_lane = 0; // PF1
			hw->fw_version = ablity.fw_version;
			hw->mac_type = info->mac;
			hw->phy_type = ablity.phy_type;
			hw->axi_mhz = ablity.axi_mhz;
			hw->port_ids = ablity.port_ids;
			hw->bd_uid = ablity.bd_uid;
			hw->phy_id = ablity.phy_id;
			hw->wol = ablity.wol_status;
			hw->eco = ablity.e.v2;
			hw->force_link_supported =
				ablity.e.force_link_supported;

			if (ablity.e.force_link_supported &&
			    (ablity.e.force_down_en & 0x1)) {
				hw->force_status = 1;
			}

			if ((hw->fw_version >= 0x00050201) &&
			    (ablity.speed == SPEED_10000)) {
				hw->force_speed_stat =
					FORCE_SPEED_STAT_DISABLED;
				hw->force_10g_1g_speed_ablity = 1;
			}
			if (ablity.ext_ablity != 0xffffffff && ablity.e.valid) {
				hw->ncsi_en = (ablity.e.ncsi_en == 1);
				hw->ncsi_rar_entries = 1;
				hw->rpu_en = ablity.e.rpu_en;
				if (hw->rpu_en) {
					ablity.e.rpu_availble = 1;
				}
				hw->rpu_availble = ablity.e.rpu_availble;
				hw->fw_lldp_ablity = ablity.e.fw_lldp_ablity;
			} else {
				hw->ncsi_rar_entries = 0;
			}

			if (hw->force_link_supported == 0) {
				hw->force_status = hw->ncsi_en ? 0 : 1;
			}

			pr_info("%s: nic-mode:%d mac:%d adpt_cnt:%d lane_mask:0x%x, phy_type: "
				"0x%x, "
				"pfvfnum:0x%x, fw-version:0x%08x\n, axi:%d Mhz,"
				"port_id:%d bd_uid:0x%08x 0x%x ex-ablity:0x%x fs:%d speed:%d "
				"ncsi_en:%u %d wol=0x%x  rpu:%d-%d v2:%d force-status:%d,%d\n",
				__func__, hw->mode, info->mac,
				info->adapter_cnt, hw->lane_mask, hw->phy_type,
				hw->pfvfnum, ablity.fw_version, ablity.axi_mhz,
				ablity.port_id[0], hw->bd_uid, ablity.phy_id,
				ablity.ext_ablity,
				hw->force_10g_1g_speed_ablity, ablity.speed,
				hw->ncsi_en, hw->ncsi_rar_entries, hw->wol,
				hw->rpu_en, hw->rpu_availble, hw->eco,
				hw->force_status, hw->force_link_supported);
			if (hw->phy_type == PHY_TYPE_10G_TP) {
				hw->supported_link = RNP_LINK_SPEED_10GB_FULL |
						     RNP_LINK_SPEED_1GB_FULL |
						     RNP_LINK_SPEED_1GB_HALF;
				hw->phy.autoneg_advertised = hw->supported_link;
				hw->autoneg = 1;
			}
			if (info->adapter_cnt != 0)
				return 0;
		}
	}

	dev_err(&hw->pdev->dev, "%s: error!\n", __func__);
	return -EIO;
}

int rnp_mbx_get_temp(struct rnp_hw *hw, int *voltage)
{
	int err;
	struct mbx_req_cookie *cookie = NULL;
	struct mbx_fw_cmd_reply reply;
	struct mbx_fw_cmd_req req;
	struct get_temp *temp;
	int temp_v = 0;

	cookie = mbx_cookie_zalloc(hw,sizeof(*temp));
	if (!cookie) {
		return -ENOMEM;
	}
	temp = (struct get_temp *)cookie->priv;

	memset(&req, 0, sizeof(req));

	build_get_temp(&req, cookie);

	if (hw->mbx.other_irq_enabled) {
		err = rnp_mbx_fw_post_req(hw, &req, cookie);
	} else {
		memset(&reply, 0, sizeof(reply));
		err = rnp_fw_send_cmd_wait(hw, &req, &reply);
		temp = &reply.get_temp;
	}

	if (voltage)
		*voltage = temp->volatage;
	temp_v = temp->temp;

	if (cookie) {
		mbx_free_cookie(cookie,err?false:true);
	}
	return temp_v;
}

enum speed_enum {
	speed_10,
	speed_100,
	speed_1000,
	speed_10000,
	speed_25000,
	speed_40000,

};

void rnp_link_stat_mark(struct rnp_hw *hw, int up)
{
	u32 v;

	v = rd32(hw, RNP_DMA_DUMY);
	if ((hw->hw_type == rnp_hw_n10) || (hw->hw_type == rnp_hw_n400)) {
		v &= ~(0xffff0000);
		v |= 0xa5a40000;
		if (up) {
			v |= BIT(0);
		} else {
			v &= ~BIT(0);
		}
	}
	wr32(hw, RNP_DMA_DUMY, v);
}

void rnp_mbx_probe_stat_set(struct rnp_hw *hw, int stat)
{
#define RNP10_DMA_DUMMY_PROBE_STAT_BIT (4)
	u32 v;

	v = rd32(hw, RNP_DMA_DUMY);
	if ((hw->hw_type == rnp_hw_n10) || (hw->hw_type == rnp_hw_n400)) {
		v &= ~(0xffff0000);
		v |= 0xa5a40000;

		if (stat == MBX_PROBE) {
			v |= BIT(RNP10_DMA_DUMMY_PROBE_STAT_BIT);
		} else if (stat == MBX_REMOVE) {
			v = 0xFFA5A6A7;
		} else {
			v &= ~BIT(RNP10_DMA_DUMMY_PROBE_STAT_BIT);
		}
	}
	wr32(hw, RNP_DMA_DUMY, v);
}

static inline int rnp_mbx_fw_req_handler(struct rnp_adapter *adapter,
					 struct mbx_fw_cmd_req *req)
{
	struct rnp_hw *hw = &adapter->hw;

	switch (req->opcode) {
	case LINK_STATUS_EVENT:
		rnp_logd(
			LOG_LINK_EVENT,
			"[LINK_STATUS_EVENT:0x%x] %s:link changed: changed_lane:0x%x, "
			"status:0x%x, speed:%d, duplex:%d\n",
			req->opcode, adapter->name,
			req->link_stat.changed_lanes,
			req->link_stat.lane_status, req->link_stat.st[0].speed,
			req->link_stat.st[0].duplex);

		if (req->link_stat.lane_status) {
			adapter->hw.link = 1;
		} else {
			adapter->hw.link = 0;
		}
		if (req->link_stat.st[0].lldp_status)
			adapter->priv_flags |= RNP_PRIV_FLAG_LLDP_EN_STAT;
		else
			adapter->priv_flags &= (~RNP_PRIV_FLAG_LLDP_EN_STAT);

		if (req->link_stat.port_st_magic == SPEED_VALID_MAGIC) {
			hw->speed = req->link_stat.st[0].speed;
			hw->duplex = req->link_stat.st[0].duplex;

			switch (hw->speed) {
			case 10:
				adapter->speed = RNP_LINK_SPEED_10_FULL;
				break;
			case 100:
				adapter->speed = RNP_LINK_SPEED_100_FULL;
				break;
			case 1000:
				adapter->speed = RNP_LINK_SPEED_1GB_FULL;
				break;
			case 10000:
				adapter->speed = RNP_LINK_SPEED_10GB_FULL;
				break;
			case 25000:
				adapter->speed = RNP_LINK_SPEED_25GB_FULL;
				break;
			case 40000:
				adapter->speed = RNP_LINK_SPEED_40GB_FULL;
				break;
			}
		}
		if (req->link_stat.lane_status) {
			rnp_link_stat_mark(hw, 1);
		} else {
			rnp_link_stat_mark(hw, 0);
		}

		adapter->flags |= RNP_FLAG_NEED_LINK_UPDATE;
		break;
	}
	rnp_service_event_schedule(adapter);

	return 0;
}

static inline int rnp_mbx_fw_reply_handler(struct rnp_adapter *adapter,
					   struct mbx_fw_cmd_reply *reply)
{
	struct mbx_req_cookie *cookie;

	cookie = reply->cookie;
	if (!cookie || is_cookie_valid(&adapter->hw,cookie)== false 
		|| cookie->stat != COOKIE_ALLOCED) {
		return -EIO;
	}

	if (cookie->priv_len > 0) {
		memcpy(cookie->priv, reply->data, cookie->priv_len);
	}

	cookie->done = 1;

	if (reply->flags & FLAGS_ERR) {
		cookie->errcode = reply->error_code;
	} else {
		cookie->errcode = 0;
	}

	if(cookie->stat == COOKIE_ALLOCED){
		wake_up_interruptible(&cookie->wait);
	}
	/* not really free cookie, mark as free-able */
	mbx_free_cookie(cookie, false);

	return 0;
}

static inline int rnp_rcv_msg_from_fw(struct rnp_adapter *adapter)
{
	u32 msgbuf[RNP_FW_MAILBOX_SIZE];
	struct rnp_hw *hw = &adapter->hw;
	s32 retval;

	retval = rnp_read_mbx(hw, msgbuf, RNP_FW_MAILBOX_SIZE, MBX_FW);
	if (retval) {
		printk("Error receiving message from FW:%d\n", retval);
		return retval;
	}

	rnp_logd(LOG_MBX_MSG_IN,
		 "msg from fw: msg[0]=0x%08x_0x%08x_0x%08x_0x%08x\n", msgbuf[0],
		 msgbuf[1], msgbuf[2], msgbuf[3]);

	/* this is a message we already processed, do nothing */
	if (((unsigned short *)msgbuf)[0] & FLAGS_DD) {
		return rnp_mbx_fw_reply_handler(
			adapter, (struct mbx_fw_cmd_reply *)msgbuf);
	} else {
		return rnp_mbx_fw_req_handler(adapter,
					      (struct mbx_fw_cmd_req *)msgbuf);
	}
}

static void rnp_rcv_ack_from_fw(struct rnp_adapter *adapter)
{
	/* do-nothing */
}

int rnp_fw_msg_handler(struct rnp_adapter *adapter)
{
	/* == check fw-req */
	if (!rnp_check_for_msg(&adapter->hw, MBX_FW))
		rnp_rcv_msg_from_fw(adapter);

	/* process any acks */
	if (!rnp_check_for_ack(&adapter->hw, MBX_FW))
		rnp_rcv_ack_from_fw(adapter);

	return 0;
}

int rnp_mbx_phy_write(struct rnp_hw *hw, u32 reg, u32 val)
{
	struct mbx_fw_cmd_req req;
	char nr_lane = hw->nr_lane;
	memset(&req, 0, sizeof(req));

	build_set_phy_reg(&req, NULL, PHY_EXTERNAL_PHY_MDIO, nr_lane, reg, val,
			  0);

	return rnp_mbx_write_posted_locked(hw, &req);
}

int rnp_mbx_phy_read(struct rnp_hw *hw, u32 reg, u32 *val)
{
	struct mbx_fw_cmd_req req;
	int err = -EIO;
	char nr_lane = hw->nr_lane;
	int times = 0;
retry:
	memset(&req, 0, sizeof(req));

	if (hw->mbx.other_irq_enabled) {
		struct mbx_req_cookie *cookie = mbx_cookie_zalloc(hw,4);
		if (!cookie) {
			return -ENOMEM;
		}
		build_get_phy_reg(&req, cookie, PHY_EXTERNAL_PHY_MDIO, nr_lane,
				  reg);

		err = rnp_mbx_fw_post_req(hw, &req, cookie);
		if (err) {
			mbx_free_cookie(cookie,false);
			return err;
		} else {
			memcpy(val, cookie->priv, 4);
			err = 0;
		}
		mbx_free_cookie(cookie,true);
	} else {
		struct mbx_fw_cmd_reply reply;
		memset(&reply, 0, sizeof(reply));
		build_get_phy_reg(&req, &reply, PHY_EXTERNAL_PHY_MDIO, nr_lane,
				  reg);

		err = rnp_fw_send_cmd_wait(hw, &req, &reply);
		if (err == 0) {
			*val = reply.r_reg.value[0];
		}
	}
	if ((*(val) == 0xffff) && (times <= 5)) {
		printk("%x warning mbx_phy_read 0xffff, addr %x\n", times, reg);
		times++;
		goto retry;
	}
	return err;
}

int rnp_mbx_phy_link_set(struct rnp_hw *hw, int adv, int autoneg, int speed,
			 int duplex, int mdix_ctrl)
{
	int err;
	struct mbx_fw_cmd_req req;

	memset(&req, 0, sizeof(req));

	printk("%s:lane:%d adv:0x%x\n", __func__, hw->nr_lane, adv);
	printk("%s:autoneg %x, speed %x, duplex %x\n", __func__, autoneg, speed,
	       duplex);

	build_phy_link_set(&req, adv, hw->nr_lane, autoneg, speed, duplex,
			   mdix_ctrl);

	if (mutex_lock_interruptible(&hw->mbx.lock))
		return -EAGAIN;
	err = hw->mbx.ops.write_posted(
		hw, (u32 *)&req, (req.datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);

	mutex_unlock(&hw->mbx.lock);
	return err;
}

int rnp_mbx_phy_pause_set(struct rnp_hw *hw, int pause_mode)
{
	int err;
	struct mbx_fw_cmd_req req;

	memset(&req, 0, sizeof(req));

	printk("%s:lane:%d pause:0x%x\n", __func__, hw->nr_lane, pause_mode);

	build_phy_pause_set(&req, pause_mode, hw->nr_lane);

	if (mutex_lock_interruptible(&hw->mbx.lock))
		return -EAGAIN;
	err = hw->mbx.ops.write_posted(
		hw, (u32 *)&req, (req.datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);

	mutex_unlock(&hw->mbx.lock);
	return err;
}

int rnp_mbx_lldp_port_enable(struct rnp_hw *hw, bool enable)
{
	struct mbx_fw_cmd_req req;
	int err;
	int nr_lane = hw->nr_lane;

	if (!hw->fw_lldp_ablity) {
		rnp_warn("lldp set not supported\n");
		return -EOPNOTSUPP;
	}

	memset(&req, 0, sizeof(req));

	build_lldp_ctrl_set(&req, nr_lane, enable);

	err = rnp_mbx_write_posted_locked(hw, &req);
	return err;
}

int rnp_mbx_lldp_status_get(struct rnp_hw *hw)
{
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;
	int err, ret = 0;

	if (!hw->fw_lldp_ablity) {
		rnp_warn("fw lldp not supported\n");
		return -EOPNOTSUPP;
	}

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	if (hw->mbx.other_irq_enabled) {
		struct mbx_req_cookie *cookie =
			mbx_cookie_zalloc(hw,sizeof(reply.lldp));

		if (!cookie) {
			return -ENOMEM;
		}
		build_lldp_ctrl_get(&req, hw->nr_lane, cookie);

		err = rnp_mbx_fw_post_req(hw, &req, cookie);
		if (err) {
			mbx_free_cookie(cookie,false);
			return ret;
		}
		ret = ((int *)(cookie->priv))[0];
		mbx_free_cookie(cookie,true);
	} else {
		build_lldp_ctrl_get(&req, hw->nr_lane, &reply);
		err = rnp_fw_send_cmd_wait(hw, &req, &reply);
		if (err) {
			rnp_err("%s: 1 error:%d\n", __func__, err);
			return -EIO;
		}
		ret = reply.lldp.enable_stat;
	}
	return ret;
}

int rnp_mbx_ddr_csl_enable(struct rnp_hw *hw, int enable,
			   dma_addr_t dma_phy,
			   int bytes)
{
	struct mbx_fw_cmd_req req;
	memset(&req, 0, sizeof(req));

	build_ddr_csl(&req, NULL, enable, dma_phy, bytes);

	if (hw->mbx.other_irq_enabled) {
		return rnp_mbx_write_posted_locked(hw, &req);
	} else {
		struct mbx_fw_cmd_reply reply;
		memset(&reply, 0, sizeof(reply));
		return rnp_fw_send_cmd_wait(hw, &req, &reply);
	}
}
