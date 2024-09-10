// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#include <linux/wait.h>
#include <linux/sem.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>

#include "rnpgbe.h"
#include "rnpgbe_mbx.h"
#include "rnpgbe_mbx_fw.h"

#define RNP_FW_MAILBOX_SIZE RNP_VFMAILBOX_SIZE

static struct mbx_req_cookie *mbx_cookie_zalloc(int priv_len)
{
	struct mbx_req_cookie *cookie =
		kzalloc(sizeof(*cookie) + priv_len, GFP_KERNEL);

	if (cookie) {
		cookie->timeout_jiffes = 30 * HZ;
		cookie->magic = COOKIE_MAGIC;
		cookie->priv_len = priv_len;
	}

	return cookie;
}

static int rnpgbe_mbx_write_posted_locked(struct rnpgbe_hw *hw,
					  struct mbx_fw_cmd_req *req)
{
	int err = 0;
	int retry = 3;

	if (mutex_lock_interruptible(&hw->mbx.lock)) {
		rnpgbe_err("[%s] get mbx lock faild opcode:0x%x\n", __func__,
			   req->opcode);
		return -EAGAIN;
	}

	rnpgbe_logd(LOG_MBX_LOCK, "%s %d lock:%p hw:%p opcode:0x%x\n", __func__,
		    hw->pfvfnum, &hw->mbx.lock, hw, req->opcode);

try_again:
	retry--;
	if (retry < 0) {
		mutex_unlock(&hw->mbx.lock);
		rnpgbe_err("%s: write_posted faild! err:0x%x opcode:0x%x\n",
			   __func__, err, req->opcode);
		return -EIO;
	}

	err = hw->mbx.ops.write_posted(
		hw, (u32 *)req, (req->datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);
	if (err)
		goto try_again;

	mutex_unlock(&hw->mbx.lock);

	return err;
}

/*
 * force firmware report link event to driver
 */
static void rnpgbe_link_stat_mark_reset(struct rnpgbe_hw *hw)
{
	wr32(hw, RNP_DMA_DUMY, 0xa0000000);
}

static void rnpgbe_link_stat_mark_disable(struct rnpgbe_hw *hw)
{
	wr32(hw, RNP_DMA_DUMY, 0);
}

static int rnpgbe_mbx_fw_post_req(struct rnpgbe_hw *hw,
				  struct mbx_fw_cmd_req *req,
				  struct mbx_req_cookie *cookie)
{
	int err = 0;
	struct rnpgbe_adapter *adpt = hw->back;

	cookie->errcode = 0;
	cookie->done = 0;
	init_waitqueue_head(&cookie->wait);

	if (mutex_lock_interruptible(&hw->mbx.lock)) {
		rnpgbe_err("[%s] wait mbx lock timeout opcode:0x%x\n", __func__,
			   req->opcode);
		return -EAGAIN;
	}

	rnpgbe_logd(LOG_MBX_LOCK, "%s %d lock:%p hw:%p opcode:0x%x\n", __func__,
		    hw->pfvfnum, &hw->mbx.lock, hw, req->opcode);

	err = rnpgbe_write_mbx(hw, (u32 *)req,
			       (req->datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);
	if (err) {
		rnpgbe_err("rnpgbe_write_mbx faild! err:%d opcode:0x%x\n", err,
			   req->opcode);
		mutex_unlock(&hw->mbx.lock);
		return err;
	}

	if (cookie->timeout_jiffes != 0) {
	retry:
		err = wait_event_interruptible_timeout(cookie->wait,
						       cookie->done == 1,
						       cookie->timeout_jiffes);
		if (err == -ERESTARTSYS)
			goto retry;
		if (err == 0) {
			rnpgbe_err(
				"[%s] %s faild! pfvfnum:0x%x hw:%p timeout err:%d opcode:%x\n",
				adpt->name, __func__, hw->pfvfnum, hw, err,
				req->opcode);
			err = -ETIME;
		} else {
			err = 0;
		}
	} else {
		wait_event_interruptible(cookie->wait, cookie->done == 1);
	}

	mutex_unlock(&hw->mbx.lock);

	if (cookie->errcode)
		err = cookie->errcode;

	return err;
}

static int rnpgbe_fw_send_cmd_wait(struct rnpgbe_hw *hw,
				   struct mbx_fw_cmd_req *req,
				   struct mbx_fw_cmd_reply *reply)
{
	int err;
	int retry_cnt = 3;

	if (!hw || !req || !reply || !hw->mbx.ops.read_posted) {
		printk(KERN_DEBUG "error: hw:%p req:%p reply:%p\n", hw, req,
		       reply);
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&hw->mbx.lock)) {
		rnpgbe_err("[%s] get mbx lock faild opcode:0x%x\n", __func__,
			   req->opcode);
		return -EAGAIN;
	}

	rnpgbe_logd(LOG_MBX_LOCK, "%s %d lock:%p hw:%p opcode:0x%x\n", __func__,
		    hw->pfvfnum, &hw->mbx.lock, hw, req->opcode);
	err = hw->mbx.ops.write_posted(
		hw, (u32 *)req, (req->datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);
	if (err) {
		rnpgbe_err("%s: write_posted faild! err:0x%x opcode:0x%x\n",
			   __func__, err, req->opcode);
		mutex_unlock(&hw->mbx.lock);
		return err;
	}

retry:
	retry_cnt--;
	if (retry_cnt < 0)
		return -EIO;

	err = hw->mbx.ops.read_posted(hw, (u32 *)reply, sizeof(*reply) / 4,
				      MBX_FW);
	if (err) {
		rnpgbe_err("%s: read_posted faild! err:0x%x opcode:0x%x\n",
			   __func__, err, req->opcode);
		mutex_unlock(&hw->mbx.lock);
		return err;
	}
	if (reply->opcode != req->opcode)
		goto retry;

	mutex_unlock(&hw->mbx.lock);

	if (reply->error_code) {
		rnpgbe_err("%s: reply err:0x%x req:0x%x\n", __func__,
			   reply->error_code, req->opcode);
		return -reply->error_code;
	}
	return 0;
}

int rnpgbe_mbx_get_lane_stat(struct rnpgbe_hw *hw)
{
	int err = 0;
	struct mbx_fw_cmd_req req;
	struct rnpgbe_adapter *adpt = hw->back;
	struct lane_stat_data *st;
	struct mbx_req_cookie *cookie = NULL;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));

	if (hw->mbx.other_irq_enabled) {
		cookie = mbx_cookie_zalloc(sizeof(struct lane_stat_data));

		if (!cookie) {
			rnpgbe_err("%s: no memory\n", __func__);
			return -ENOMEM;
		}

		st = (struct lane_stat_data *)cookie->priv;

		build_get_lane_status_req(&req, hw->nr_lane, cookie);

		err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);

		if (err) {
			rnpgbe_err("%s: error:%d\n", __func__, err);
			goto quit;
		}
	} else {
		memset(&reply, 0, sizeof(reply));

		build_get_lane_status_req(&req, hw->nr_lane, &req);
		err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
		if (err) {
			rnpgbe_err("%s: 1 error:%d\n", __func__, err);
			goto quit;
		}
		st = (struct lane_stat_data *)&(reply.data);
	}

	hw->phy_type = st->phy_type;
	hw->speed = adpt->speed = st->speed;
	if (st->is_sgmii) {
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

	if ((hw->hw_type == rnpgbe_hw_n10) || (hw->hw_type == rnpgbe_hw_n400)) {
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

	rnpgbe_logd(
		LOG_MBX_LINK_STAT,
		"%s:pma_type:0x%x phy_type:0x%x,linkup:%d duplex:%d auton:%d "
		"fec:%d an:%d lt:%d is_sgmii:%d supported_link:0x%x, backplane:%d "
		"speed:%d sfp_connector:0x%x adv %0xx\n",
		adpt->name, st->pma_type, st->phy_type, st->linkup, st->duplex,
		st->autoneg, st->fec, st->an, st->link_traing, st->is_sgmii,
		hw->supported_link, hw->is_backplane, st->speed,
		st->sfp_connector, hw->advertised_link);
quit:
	if (cookie)
		kfree(cookie);
	return err;
}

int rnpgbe_mbx_fw_reset_phy(struct rnpgbe_hw *hw)
{
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;
	int ret;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	if (hw->mbx.other_irq_enabled) {
		struct mbx_req_cookie *cookie = mbx_cookie_zalloc(0);

		if (!cookie)
			return -ENOMEM;

		build_reset_phy_req(&req, cookie);

		ret = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
		kfree(cookie);
		return ret;

	} else {
		build_reset_phy_req(&req, &req);
		return rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
	}
}

int rnpgbe_maintain_req(struct rnpgbe_hw *hw, int cmd, int arg0,
			int req_data_bytes, int reply_bytes,
			dma_addr_t dma_phy_addr)
{
	int err;
	struct mbx_req_cookie *cookie = NULL;

	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	cookie = mbx_cookie_zalloc(0);
	if (!cookie)
		return -ENOMEM;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));
	cookie->timeout_jiffes = 60 * HZ;

	build_maintain_req(&req, cookie, cmd, arg0, req_data_bytes, reply_bytes,
			   dma_phy_addr & 0xffffffff,
			   (dma_phy_addr >> 32) & 0xffffffff);

	if (hw->mbx.other_irq_enabled) {
		err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
	} else {
		int old_mbx_timeout = hw->mbx.timeout;

		hw->mbx.timeout =
			(60 * 1000 * 1000) / hw->mbx.usec_delay; /* wait 60s */
		err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
		hw->mbx.timeout = old_mbx_timeout;
	}

	if (cookie)
		kfree(cookie);

	return (err) ? -EIO : 0;
}

int rnpgbe_fw_get_macaddr(struct rnpgbe_hw *hw, int pfvfnum, u8 *mac_addr,
			  int nr_lane)
{
	int err;
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	rnpgbe_dbg("%s: pfvfnum:0x%x nr_lane:%d\n", __func__, pfvfnum, nr_lane);

	if (!mac_addr) {
		rnpgbe_err("%s: mac_addr is null\n", __func__);
		return -EINVAL;
	}

	if (hw->mbx.other_irq_enabled) {
		struct mbx_req_cookie *cookie =
			mbx_cookie_zalloc(sizeof(reply.mac_addr));
		struct mac_addr *mac = (struct mac_addr *)cookie->priv;

		if (!cookie)
			return -ENOMEM;

		build_get_macaddress_req(&req, 1 << nr_lane, pfvfnum, cookie);

		err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
		if (err) {
			kfree(cookie);
			return err;
		}

		if ((1 << nr_lane) & mac->lanes)
			memcpy(mac_addr, mac->addrs[nr_lane].mac, 6);

		kfree(cookie);
		return 0;

	} else {
		build_get_macaddress_req(&req, 1 << nr_lane, pfvfnum, &req);
		err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
		if (err) {
			rnpgbe_err("%s: faild. err:%d\n", __func__, err);
			return err;
		}

		if ((1 << nr_lane) & reply.mac_addr.lanes) {
			memcpy(mac_addr, reply.mac_addr.addrs[nr_lane].mac, 6);
			return 0;
		}
	}

	return -ENODATA;
}

static int rnpgbe_mbx_sfp_read(struct rnpgbe_hw *hw, int sfp_i2c_addr, int reg,
			       int cnt, u8 *out_buf)
{
	struct mbx_fw_cmd_req req;
	int err = -EIO;
	int nr_lane = hw->nr_lane;

	if ((cnt > MBX_SFP_READ_MAX_CNT) || !out_buf) {
		rnpgbe_err("%s: cnt:%d should <= %d out_buf:%p\n", __func__,
			   cnt, MBX_SFP_READ_MAX_CNT, out_buf);
		return -EINVAL;
	}

	memset(&req, 0, sizeof(req));

	if (hw->mbx.other_irq_enabled) {
		struct mbx_req_cookie *cookie = mbx_cookie_zalloc(cnt);

		if (!cookie)
			return -ENOMEM;

		build_mbx_sfp_read(&req, nr_lane, sfp_i2c_addr, reg, cnt,
				   cookie);

		err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
		if (err) {
			kfree(cookie);
			return err;

		} else {
			memcpy(out_buf, cookie->priv, cnt);
			err = 0;
		}
		kfree(cookie);
	} else {
		struct mbx_fw_cmd_reply reply;

		memset(&reply, 0, sizeof(reply));
		build_mbx_sfp_read(&req, nr_lane, sfp_i2c_addr, reg, cnt,
				   &reply);

		err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
		if (err == 0)
			memcpy(out_buf, reply.sfp_read.value, cnt);
	}

	return err;
}

int rnpgbe_mbx_sfp_module_eeprom_info(struct rnpgbe_hw *hw, int sfp_addr,
				      int reg, int data_len, u8 *buf)
{
	int left = data_len;
	int cnt, err;

	do {
		cnt = (left > MBX_SFP_READ_MAX_CNT) ? MBX_SFP_READ_MAX_CNT :
						      left;
		err = rnpgbe_mbx_sfp_read(hw, sfp_addr, reg, cnt, buf);
		if (err) {
			rnpgbe_err("%s: error:%d\n", __func__, err);
			return err;
		}
		reg += cnt;
		buf += cnt;
		left -= cnt;
	} while (left > 0);

	return 0;
}

int rnpgbe_mbx_sfp_write(struct rnpgbe_hw *hw, int sfp_addr, int reg, short v)
{
	struct mbx_fw_cmd_req req;
	int err;
	int nr_lane = hw->nr_lane;

	memset(&req, 0, sizeof(req));

	build_mbx_sfp_write(&req, nr_lane, sfp_addr, reg, v);

	err = rnpgbe_mbx_write_posted_locked(hw, &req);
	return err;
}

int rnpgbe_mbx_fw_reg_read(struct rnpgbe_hw *hw, int fw_reg)
{
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;
	int err, ret = 0xffffffff;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	if (hw->fw_version < 0x00050200)
		return -EOPNOTSUPP;

	if (hw->mbx.other_irq_enabled) {
		struct mbx_req_cookie *cookie =
			mbx_cookie_zalloc(sizeof(reply.r_reg));

		build_readreg_req(&req, fw_reg, cookie);

		err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
		if (err) {
			kfree(cookie);
			return ret;
		}
		ret = ((int *)(cookie->priv))[0];
	} else {
		build_readreg_req(&req, fw_reg, &reply);
		err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
		if (err) {
			rnpgbe_err("%s: faild. err:%d\n", __func__, err);
			return err;
		}
		ret = reply.r_reg.value[0];
	}
	return ret;
}

int rnpgbe_mbx_reg_write(struct rnpgbe_hw *hw, int fw_reg, int value)
{
	struct mbx_fw_cmd_req req;
	int err;

	memset(&req, 0, sizeof(req));
	if (hw->fw_version < 0x00050200)
		return -EOPNOTSUPP;

	build_writereg_req(&req, NULL, fw_reg, 4, &value);

	err = rnpgbe_mbx_write_posted_locked(hw, &req);
	return err;
}

int rnpgbe_mbx_wol_set(struct rnpgbe_hw *hw, u32 mode)
{
	struct mbx_fw_cmd_req req;
	int err;
	int nr_lane = hw->nr_lane;

	memset(&req, 0, sizeof(req));

	build_mbx_wol_set(&req, nr_lane, mode);

	err = rnpgbe_mbx_write_posted_locked(hw, &req);
	return err;
}

int rnpgbe_mbx_gephy_test_set(struct rnpgbe_hw *hw, u32 mode)
{
	struct mbx_fw_cmd_req req;
	int err;
	int nr_lane = hw->nr_lane;

	memset(&req, 0, sizeof(req));

	build_mbx_gephy_test_set(&req, nr_lane, mode);

	err = rnpgbe_mbx_write_posted_locked(hw, &req);
	return err;
}

int rnpgbe_mbx_lldp_set(struct rnpgbe_hw *hw, u32 enable)
{
	struct mbx_fw_cmd_req req;
	int err;
	int nr_lane = hw->nr_lane;

	memset(&req, 0, sizeof(req));

	build_mbx_lldp_set(&req, nr_lane, enable);

	err = rnpgbe_mbx_write_posted_locked(hw, &req);

	return err;
}

int rnpgbe_mbx_lldp_get(struct rnpgbe_hw *hw)
{
	int err;
	struct mbx_req_cookie *cookie = NULL;
	struct mbx_fw_cmd_reply reply;
	struct mbx_fw_cmd_req req;
	struct get_lldp_reply *get_lldp;

	cookie = mbx_cookie_zalloc(sizeof(*get_lldp));
	if (!cookie)
		return -ENOMEM;
	get_lldp = (struct get_lldp_reply *)cookie->priv;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_get_lldp_req(&req, cookie, hw->nr_lane);

	if (hw->mbx.other_irq_enabled) {
		err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
	} else {
		err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
		get_lldp = &reply.get_lldp;
	}

	if (err == 0) {
		hw->lldp_status.enable = get_lldp->value;
		hw->lldp_status.inteval = get_lldp->inteval;
	}

	if (cookie)
		kfree(cookie);

	return err ? -err : 0;
}

int rnpgbe_mbx_set_dump(struct rnpgbe_hw *hw, int flag)
{
	int err;
	struct mbx_fw_cmd_req req;

	memset(&req, 0, sizeof(req));
	build_set_dump(&req, hw->nr_lane, flag);

	err = rnpgbe_mbx_write_posted_locked(hw, &req);

	return err;
}

/*
 * @speed :
 * 0 : disable force speed
 * 1000 : force 1000Mbps
 * 10000 : force 10000Mbps
 */
int rnpgbe_mbx_force_speed(struct rnpgbe_hw *hw, int speed)
{
	int cmd = 0x01150000;

	if (hw->force_10g_1g_speed_ablity == 0)
		return -EINVAL;

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
	return rnpgbe_mbx_set_dump(hw, cmd);
}

int rnpgbe_mbx_get_dump_flags(struct rnpgbe_hw *hw)
{
	int err;
	struct mbx_req_cookie *cookie = NULL;
	struct mbx_fw_cmd_reply reply;
	struct mbx_fw_cmd_req req;
	struct get_dump_reply *get_dump;

	cookie = mbx_cookie_zalloc(sizeof(*get_dump));
	if (!cookie)
		return -ENOMEM;
	get_dump = (struct get_dump_reply *)cookie->priv;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_get_dump_req(&req, cookie, hw->nr_lane, 0, 0, 0);

	if (hw->mbx.other_irq_enabled) {
		err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
	} else {
		err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
		get_dump = &reply.get_dump;
	}

	if (err == 0) {
		hw->dump.version = get_dump->version;
		hw->dump.flag = get_dump->flags;
		hw->dump.len = get_dump->bytes;
	}
	if (cookie)
		kfree(cookie);

	return err ? -err : 0;
}

int rnpgbe_mbx_get_dump(struct rnpgbe_hw *hw, int flags, u32 *data_out,
			int bytes)
{
	int err;
	struct mbx_req_cookie *cookie = NULL;
	struct rnpgbe_mbx_info *mbx = &hw->mbx;
	struct mbx_fw_cmd_reply reply;
	struct mbx_fw_cmd_req req;
	struct get_dump_reply *get_dump;
	int ram_size = mbx->share_size;
	int i, offset = 0;

	cookie = mbx_cookie_zalloc(sizeof(*get_dump));
	if (!cookie)
		return -ENOMEM;

	get_dump = (struct get_dump_reply *)cookie->priv;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	do {
		build_get_dump_req(&req, cookie, hw->nr_lane, offset, 0,
				   ram_size);

		if (hw->mbx.other_irq_enabled) {
			err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
		} else {
			err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
			get_dump = &reply.get_dump;
		}

		if (err == 0 && data_out) {
			int len = ram_size;

			if ((bytes - offset) < ram_size)
				len = bytes - offset;

			for (i = 0; i < len; i = i + 4)
				*(data_out + offset / 4 + i / 4) =
					rnpgbe_rd_reg(hw->hw_addr +
						      mbx->cpu_vf_share_ram +
						      i);
		}

		offset += ram_size;

	} while (offset < bytes);

	if (cookie)
		kfree(cookie);

	return err ? -err : 0;
}

int rnp500_fw_update(struct rnpgbe_hw *hw, int partition, const u8 *fw_bin,
		     int bytes)
{
	struct rnpgbe_mbx_info *mbx = &hw->mbx;
	int err = 0;
	int offset = 0, ram_size = mbx->share_size;
	struct mbx_req_cookie *cookie = NULL;

	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	int i;
	u32 *msg = (u32 *)fw_bin;

	cookie = mbx_cookie_zalloc(0);
	if (!cookie) {
		dev_err(&hw->pdev->dev, "%s: no memory:%d!", __func__, 0);
		return -ENOMEM;
	}

	/* if bytes more than ram_size, we update header at last */
	if (bytes > ram_size) {
		offset += ram_size;

		/* todo */
		/* n210 should clean header first */
		if (hw->hw_type == rnpgbe_hw_n210) {
			memset(&req, 0, sizeof(req));
			memset(&reply, 0, sizeof(reply));

			for (i = 0; i < ram_size; i = i + 4)
				rnpgbe_wr_reg(hw->hw_addr + mbx->cpu_vf_share_ram + i,
						0xffffffff);

			build_fw_update_n500_req(&req, cookie, partition, 0);
			if (hw->mbx.other_irq_enabled) {
				err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
			} else {
				int old_mbx_timeout = hw->mbx.timeout;

				hw->mbx.timeout = (20 * 1000 * 1000) /
					hw->mbx.usec_delay;
				err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
				hw->mbx.timeout = old_mbx_timeout;
			}
		}
	}

	while (offset < bytes) {
		memset(&req, 0, sizeof(req));
		memset(&reply, 0, sizeof(reply));

		for (i = 0; i < ram_size; i = i + 4)
			rnpgbe_wr_reg(hw->hw_addr + mbx->cpu_vf_share_ram + i,
				      *(msg + offset / 4 + i / 4));

		build_fw_update_n500_req(&req, cookie, partition, offset);
		if (hw->mbx.other_irq_enabled) {
			err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
		} else {
			int old_mbx_timeout = hw->mbx.timeout;

			hw->mbx.timeout = (20 * 1000 * 1000) /
					  hw->mbx.usec_delay;
			err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
			hw->mbx.timeout = old_mbx_timeout;
		}

		if (err)
			goto out;
		offset += ram_size;
	}
	/* we write header at last */
	if (bytes > ram_size) {
		offset = 0;
		memset(&req, 0, sizeof(req));
		memset(&reply, 0, sizeof(reply));

		for (i = 0; i < ram_size; i = i + 4)
			rnpgbe_wr_reg(hw->hw_addr + mbx->cpu_vf_share_ram + i,
				      *(msg + offset / 4 + i / 4));

		build_fw_update_n500_req(&req, cookie, partition, offset);
		if (hw->mbx.other_irq_enabled) {
			err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
		} else {
			int old_mbx_timeout = hw->mbx.timeout;

			hw->mbx.timeout = (20 * 1000 * 1000) /
					  hw->mbx.usec_delay;
			err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
			hw->mbx.timeout = old_mbx_timeout;
		}
	}
out:
	return err ? -err : 0;
}

int rnpgbe_fw_update(struct rnpgbe_hw *hw, int partition, const u8 *fw_bin,
		     int bytes)
{
	int err;
	struct mbx_req_cookie *cookie = NULL;

	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	void *dma_buf = NULL;
	dma_addr_t dma_phy;

	cookie = mbx_cookie_zalloc(0);
	if (!cookie) {
		dev_err(&hw->pdev->dev, "%s: no memory:%d!", __func__, 0);
		return -ENOMEM;
	}

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	dma_buf =
		dma_alloc_coherent(&hw->pdev->dev, bytes, &dma_phy, GFP_ATOMIC);
	if (!dma_buf) {
		err = -ENOMEM;
		goto quit;
	}

	memcpy(dma_buf, fw_bin, bytes);

	build_fw_update_req(&req, cookie, partition, dma_phy & 0xffffffff,
			    (dma_phy >> 32) & 0xffffffff, bytes);
	if (hw->mbx.other_irq_enabled) {
		err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
	} else {
		int old_mbx_timeout = hw->mbx.timeout;

		hw->mbx.timeout = (20 * 1000 * 1000) / hw->mbx.usec_delay;
		err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
		hw->mbx.timeout = old_mbx_timeout;
	}

quit:
	if (dma_buf)
		dma_free_coherent(&hw->pdev->dev, bytes, dma_buf, dma_phy);

	if (cookie)
		kfree(cookie);

	printk(KERN_DEBUG "%s: %s (errcode:%d)\n", __func__,
	       err ? " failed" : " success", err);
	return (err) ? -EIO : 0;
}

int rnpgbe_mbx_link_event_enable(struct rnpgbe_hw *hw, int enable)
{
	struct mbx_fw_cmd_reply reply;
	struct mbx_fw_cmd_req req;
	int err;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	if (enable)
		wr32(hw, RNP_DMA_DUMY, 0xa0000000);

	build_link_set_event_mask(&req, BIT(EVT_LINK_UP),
				  (enable & 1) << EVT_LINK_UP, &req);

	err = rnpgbe_mbx_write_posted_locked(hw, &req);
	if (!enable)
		wr32(hw, RNP_DMA_DUMY, 0);

	return err;
}

int rnpgbe_fw_get_capablity(struct rnpgbe_hw *hw, struct phy_abilities *abil)
{
	int err;
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));
	build_phy_abalities_req(&req, &req);
	err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);

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
		if (lanes == 1)
			return rnpgbe_mac_n10g_x8_40G;
		else
			return rnpgbe_mac_n10g_x8_10G;

	} else if ((ability->phy_type == PHY_TYPE_10G_BASE_KR) ||
		   (ability->phy_type == PHY_TYPE_10G_BASE_LR) ||
		   (ability->phy_type == PHY_TYPE_10G_BASE_ER) ||
		   (ability->phy_type == PHY_TYPE_10G_BASE_SR)) {
		if (lanes == 1)
			return rnpgbe_mac_n10g_x2_10G;
		else if (lanes == 2)
			return rnpgbe_mac_n10g_x4_10G;
		else
			return rnpgbe_mac_n10g_x8_10G;

	} else if (ability->phy_type == PHY_TYPE_1G_BASE_KX) {
		return rnpgbe_mac_n10l_x8_1G;

	} else if (ability->phy_type == PHY_TYPE_SGMII) {
		return rnpgbe_mac_n10l_x8_1G;
	}
	return rnpgbe_mac_unknown;
}

int rnpgbe_set_lane_fun(struct rnpgbe_hw *hw, int fun, int value0, int value1,
			int value2, int value3)
{
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));
	build_set_lane_fun(&req, hw->nr_lane, fun, value0, value1, value2,
			   value3);

	return rnpgbe_mbx_write_posted_locked(hw, &req);
}

int rnpgbe_mbx_ifinsmod(struct rnpgbe_hw *hw, int status)
{
	int err;
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_ifinsmod(&req, hw->driver_version, status);

	if (mutex_lock_interruptible(&hw->mbx.lock))
		return -EAGAIN;

	if (status) {
		err = hw->mbx.ops.write_posted(
			hw, (u32 *)&req, (req.datalen + MBX_REQ_HDR_LEN) / 4,
			MBX_FW);
	} else {
		err = hw->mbx.ops.write(hw, (u32 *)&req,
					(req.datalen + MBX_REQ_HDR_LEN) / 4,
					MBX_FW);
	}

	mutex_unlock(&hw->mbx.lock);

	rnpgbe_logd(LOG_MBX_IFUP_DOWN, "%s: lane:%d status:%d\n", __func__,
		    hw->nr_lane, status);

	return err;
}

int rnpgbe_mbx_ifsuspuse(struct rnpgbe_hw *hw, int status)
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

	rnpgbe_logd(LOG_MBX_IFUP_DOWN, "%s: lane:%d status:%d\n", __func__,
		    hw->nr_lane, status);

	return err;
}

int rnpgbe_mbx_ifforce_control_mac(struct rnpgbe_hw *hw, int status)
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

	rnpgbe_logd(LOG_MBX_IFUP_DOWN, "%s: lane:%d status:%d\n", __func__,
		    hw->nr_lane, status);

	return err;
}

int rnpgbe_mbx_tstamps_show(struct rnpgbe_hw *hw, u32 sec, u32 nanosec)
{
	int err;
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_tstamp_show(&req, sec, nanosec);

	if (mutex_lock_interruptible(&hw->mbx.lock))
		return -EAGAIN;

	err = hw->mbx.ops.write_posted(
		hw, (u32 *)&req, (req.datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);

	mutex_unlock(&hw->mbx.lock);

	return err;
}

int rnpgbe_mbx_ifup_down(struct rnpgbe_hw *hw, int up)
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

	rnpgbe_logd(LOG_MBX_IFUP_DOWN, "%s: lane:%d up:%d\n", __func__,
		    hw->nr_lane, up);

	if (up)
		rnpgbe_link_stat_mark_reset(hw);

	return err;
}

int rnpgbe_mbx_led_set(struct rnpgbe_hw *hw, int value)
{
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_led_set(&req, hw->nr_lane, value, &reply);

	return rnpgbe_mbx_write_posted_locked(hw, &req);
}

int rnpgbe_mbx_get_eee_capability(struct rnpgbe_hw *hw,
				  struct rnpgbe_eee_cap *eee_cap)
{
	int err;
	struct mbx_fw_cmd_req req;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	build_phy_eee_abalities_req(&req, &req);

	err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);

	if (err == 0) {
		memcpy(eee_cap, &reply.phy_eee_abilities, sizeof(*eee_cap));
		return 0;
	}

	return -EIO;
}

int rnpgbe_mbx_phy_eee_set(struct rnpgbe_hw *hw, u32 tx_lpi_timer,
			   u32 local_eee)
{
	int err;
	struct mbx_fw_cmd_req req;

	memset(&req, 0, sizeof(req));

	build_phy_eee_set(&req, local_eee, tx_lpi_timer, hw->nr_lane);

	if (mutex_lock_interruptible(&hw->mbx.lock))
		return -EAGAIN;

	err = hw->mbx.ops.write_posted(
		hw, (u32 *)&req, (req.datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);

	mutex_unlock(&hw->mbx.lock);

	return -EIO;
}

int rnpgbe_mbx_get_capability(struct rnpgbe_hw *hw, struct rnpgbe_info *info)
{
	int err;
	struct phy_abilities ablity;
	int try_cnt = 3;

	memset(&ablity, 0, sizeof(ablity));
	rnpgbe_link_stat_mark_disable(hw);

	while (try_cnt--) {
		err = rnpgbe_fw_get_capablity(hw, &ablity);
		if (err == 0 && info) {
			hw->lane_mask = ablity.lane_mask & 0xf;
			info->mac = to_mac_type(&ablity);
			info->adapter_cnt = hweight_long(hw->lane_mask);
			hw->sfc_boot = (ablity.nic_mode & 0x1) ? 1 : 0;
			hw->pxe_en = (ablity.nic_mode & 0x2) ? 1 : 0;
			hw->ncsi_en = (ablity.nic_mode & 0x4) ? 1 : 0;
			hw->pfvfnum = ablity.pfnum;
			hw->speed = ablity.speed;
			hw->nr_lane = 0;
			hw->fw_version = ablity.fw_version;
			hw->mac_type = info->mac;
			hw->phy_type = ablity.phy_type;
			hw->axi_mhz = ablity.axi_mhz;
			hw->port_ids = ablity.port_ids;
			hw->bd_uid = ablity.bd_uid;
			hw->phy_id = ablity.phy_id;

			if ((hw->fw_version >= 0x00050201) &&
			    (ablity.speed == SPEED_10000)) {
				hw->force_speed_stat =
					FORCE_SPEED_STAT_DISABLED;
				hw->force_10g_1g_speed_ablity = 1;
			}
			if (hw->fw_version >= 0x0001012C) {
				/* this version can get wol_en from hw */
				hw->wol = ablity.wol_status & 0xff;
				hw->wol_en = ablity.wol_status & 0x100;
			} else {
				/* other version only pf0 or ncsi can wol */
				hw->wol = ablity.wol_status & 0xff;
				if ((hw->ncsi_en) || (!ablity.pfnum))
					hw->wol_en = 1;
			}
			/* 0.1.5.0 can get force status from fw */
			if (hw->fw_version >= 0x00010500) {
				hw->force_en = ablity.e.force_down_en;
				hw->force_cap = 1;
			}

			/* 0.1.6.0 can get trim valid from hw */
			if (hw->fw_version >= 0x00010600)
				hw->trim_valid = (ablity.nic_mode & 0x8) ? 1 : 0;

			pr_info("%s: nic-mode:%d mac:%d adpt_cnt:%d lane_mask:0x%x, phy_type: "
				"0x%x, "
				"pfvfnum:0x%x, fw-version:0x%08x\n, axi:%d Mhz,"
				"port_id:%d bd_uid:0x%08x 0x%x ex-ablity:0x%x fs:%d speed:%d\n",
				__func__, hw->mode, info->mac,
				info->adapter_cnt, hw->lane_mask, hw->phy_type,
				hw->pfvfnum, ablity.fw_version, ablity.axi_mhz,
				ablity.port_id[0], hw->bd_uid, ablity.phy_id,
				ablity.ext_ablity,
				hw->force_10g_1g_speed_ablity, ablity.speed);
			if (info->adapter_cnt != 0)
				return 0;
		}
	}

	dev_err(&hw->pdev->dev, "%s: error!\n", __func__);
	return -EIO;
}

int rnpgbe_mbx_get_temp(struct rnpgbe_hw *hw, int *voltage)
{
	int err;
	struct mbx_req_cookie *cookie = NULL;
	struct mbx_fw_cmd_reply reply;
	struct mbx_fw_cmd_req req;
	struct get_temp *temp;
	int temp_v = 0;

	cookie = mbx_cookie_zalloc(sizeof(*temp));
	if (!cookie)
		return -ENOMEM;
	temp = (struct get_temp *)cookie->priv;

	memset(&req, 0, sizeof(req));

	build_get_temp(&req, cookie);

	if (hw->mbx.other_irq_enabled) {
		err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
	} else {
		memset(&reply, 0, sizeof(reply));
		err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
		temp = &reply.get_temp;
	}

	if (voltage)
		*voltage = temp->volatage;
	temp_v = temp->temp;

	if (cookie)
		kfree(cookie);
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

static void rnpgbe_link_stat_mark(struct rnpgbe_hw *hw, int up)
{
	u32 v;
	struct rnpgbe_adapter *adapter = (struct rnpgbe_adapter *)hw->back;

	v = rd32(hw, RNP_DMA_DUMY);
	if ((hw->hw_type == rnpgbe_hw_n10) || (hw->hw_type == rnpgbe_hw_n400)) {
		v &= ~(0xffff0000);
		v |= 0xa5a40000;
		if (up)
			v |= BIT(0);
		else
			v &= ~BIT(0);

	} else if ((hw->hw_type == rnpgbe_hw_n500) ||
		   (hw->hw_type == rnpgbe_hw_n210)) {
		v &= ~(0x0f000f11);
		v |= 0xa0000000;
		if (up) {
			v |= BIT(0);
			switch (hw->speed) {
			case 10:
				v |= (speed_10 << 8);
				break;
			case 100:
				v |= (speed_100 << 8);
				break;
			case 1000:
				v |= (speed_1000 << 8);
				break;
			case 10000:
				v |= (speed_10000 << 8);
				break;
			case 25000:
				v |= (speed_25000 << 8);
				break;
			case 40000:
				v |= (speed_40000 << 8);
				break;
			}
			v |= (hw->duplex << 4);
			v |= (hw->fc.current_mode << 24);
		} else {
			v &= ~BIT(0);
		}
		/* we shoul update lldp_status */
		if (hw->fw_version >= 0x00010500) {
			if (adapter->priv_flags & RNP_PRIV_FLAG_LLDP)
				v |= BIT(6);
			else
				v &= (~BIT(6));
		}
	}
	wr32(hw, RNP_DMA_DUMY, v);
}

static inline int rnpgbe_mbx_fw_req_handler(struct rnpgbe_adapter *adapter,
					    struct mbx_fw_cmd_req *req)
{
	struct rnpgbe_hw *hw = &adapter->hw;

	switch (req->opcode) {
	case LINK_STATUS_EVENT:
		rnpgbe_logd(
			LOG_LINK_EVENT,
			"[LINK_STATUS_EVENT:0x%x] %s:link changed: changed_lane:0x%x, "
			"status:0x%x, speed:%d, duplex:%d\n",
			req->opcode, adapter->name,
			req->link_stat.changed_lanes,
			req->link_stat.lane_status, req->link_stat.st[0].speed,
			req->link_stat.st[0].duplex);

		if (req->link_stat.lane_status)
			adapter->hw.link = 1;
		else
			adapter->hw.link = 0;

		if ((hw->hw_type == rnpgbe_hw_n500) ||
		    (hw->hw_type == rnpgbe_hw_n210)) {
			adapter->local_eee = req->link_stat.st[0].local_eee;
			adapter->partner_eee = req->link_stat.st[0].partner_eee;
			/* fw_version more than 0.1.5.0 can up lldp_status */
			if (hw->fw_version >= 0x00010500) {
				if (req->link_stat.st[0].lldp_status)
					adapter->priv_flags |=  RNP_PRIV_FLAG_LLDP;
				else
					adapter->priv_flags &= (~RNP_PRIV_FLAG_LLDP);
			}
		}

		if (req->link_stat.port_st_magic == SPEED_VALID_MAGIC) {
			hw->speed = req->link_stat.st[0].speed;
			hw->duplex = req->link_stat.st[0].duplex;
			/* n500 can update pause and tp */
			if ((hw->hw_type == rnpgbe_hw_n500) ||
			    (hw->hw_type == rnpgbe_hw_n210)) {
				hw->fc.current_mode =
					req->link_stat.st[0].pause;
				hw->tp_mdx = req->link_stat.st[0].tp_mdx;
			}

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
		if (req->link_stat.lane_status)
			rnpgbe_link_stat_mark(hw, 1);
		else
			rnpgbe_link_stat_mark(hw, 0);

		adapter->flags |= RNP_FLAG_NEED_LINK_UPDATE;
		break;
	}

	return 0;
}

static inline int rnpgbe_mbx_fw_reply_handler(struct rnpgbe_adapter *adapter,
					      struct mbx_fw_cmd_reply *reply)
{
	struct mbx_req_cookie *cookie;

	cookie = reply->cookie;
	if (!cookie || cookie->magic != COOKIE_MAGIC)
		return -EIO;

	if (cookie->priv_len > 0)
		memcpy(cookie->priv, reply->data, cookie->priv_len);

	cookie->done = 1;

	if (reply->flags & FLAGS_ERR)
		cookie->errcode = reply->error_code;
	else
		cookie->errcode = 0;

	wake_up_interruptible(&cookie->wait);

	return 0;
}

static inline int rnpgbe_rcv_msg_from_fw(struct rnpgbe_adapter *adapter)
{
	u32 msgbuf[RNP_FW_MAILBOX_SIZE];
	struct rnpgbe_hw *hw = &adapter->hw;
	s32 retval;

	retval = rnpgbe_read_mbx(hw, msgbuf, RNP_FW_MAILBOX_SIZE, MBX_FW);
	if (retval) {
		printk(KERN_DEBUG "Error receiving message from FW:%d\n",
		       retval);
		return retval;
	}

	rnpgbe_logd(LOG_MBX_MSG_IN,
		    "msg from fw: msg[0]=0x%08x_0x%08x_0x%08x_0x%08x\n",
		    msgbuf[0], msgbuf[1], msgbuf[2], msgbuf[3]);

	/* this is a message we already processed, do nothing */
	if (((unsigned short *)msgbuf)[0] & FLAGS_DD) {
		return rnpgbe_mbx_fw_reply_handler(
			adapter, (struct mbx_fw_cmd_reply *)msgbuf);
	} else {
		return rnpgbe_mbx_fw_req_handler(
			adapter, (struct mbx_fw_cmd_req *)msgbuf);
	}
}

static void rnpgbe_rcv_ack_from_fw(struct rnpgbe_adapter *adapter)
{
	/* do-nothing */
}

int rnpgbe_fw_msg_handler(struct rnpgbe_adapter *adapter)
{
	/* == check fw-req */
	if (!rnpgbe_check_for_msg(&adapter->hw, MBX_FW)) {
		/* printk("check msg from fw\n"); */
		rnpgbe_rcv_msg_from_fw(adapter);
	}

	/* process any acks */
	if (!rnpgbe_check_for_ack(&adapter->hw, MBX_FW))
		rnpgbe_rcv_ack_from_fw(adapter);

	return 0;
}

int rnpgbe_mbx_phy_link_set(struct rnpgbe_hw *hw, int adv, int autoneg,
			    int speed, int duplex, int mdix_ctrl)
{
	int err;
	struct mbx_fw_cmd_req req;

	memset(&req, 0, sizeof(req));

	build_phy_link_set(&req, adv, hw->nr_lane, autoneg, speed, duplex,
			   mdix_ctrl);

	if (mutex_lock_interruptible(&hw->mbx.lock))
		return -EAGAIN;
	err = hw->mbx.ops.write_posted(
		hw, (u32 *)&req, (req.datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);

	mutex_unlock(&hw->mbx.lock);

	return err;
}

int rnpgbe_mbx_phy_pause_set(struct rnpgbe_hw *hw, u32 pause_mode)
{
	int err;
	struct mbx_fw_cmd_req req;

	memset(&req, 0, sizeof(req));

	build_phy_pause_set(&req, pause_mode, hw->nr_lane);

	if (mutex_lock_interruptible(&hw->mbx.lock))
		return -EAGAIN;
	err = hw->mbx.ops.write_posted(
		hw, (u32 *)&req, (req.datalen + MBX_REQ_HDR_LEN) / 4, MBX_FW);

	mutex_unlock(&hw->mbx.lock);

	return err;
}

int rnpgbe_mbx_phy_pause_get(struct rnpgbe_hw *hw, u32 *pause_mode)
{
	struct mbx_fw_cmd_req req;
	int err = -EIO;
	struct mbx_req_cookie *cookie = NULL;
	struct phy_pause_data *st;
	struct mbx_fw_cmd_reply reply;

	memset(&req, 0, sizeof(req));

	if (hw->mbx.other_irq_enabled) {
		cookie = mbx_cookie_zalloc(sizeof(struct lane_stat_data));

		if (!cookie) {
			rnpgbe_err("%s: no memory\n", __func__);
			return -ENOMEM;
		}

		st = (struct phy_pause_data *)cookie->priv;
		build_get_phy_pause_req(&req, hw->nr_lane, cookie);
		err = rnpgbe_mbx_fw_post_req(hw, &req, cookie);
		if (err) {
			rnpgbe_err("%s: error:%d\n", __func__, err);
			goto quit;
		}
	} else {
		memset(&reply, 0, sizeof(reply));

		build_get_phy_pause_req(&req, hw->nr_lane, &req);
		err = rnpgbe_fw_send_cmd_wait(hw, &req, &reply);
		if (err) {
			rnpgbe_err("%s: 1 error:%d\n", __func__, err);
			goto quit;
		}
		st = (struct phy_pause_data *)&(reply.data);
	}

	*pause_mode = st->pause_mode;
quit:
	if (cookie)
		kfree(cookie);
	return err;
}
