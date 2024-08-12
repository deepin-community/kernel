// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include <linux/fs.h>
#include <linux/debugfs.h>

#include "ne6xvf.h"

#ifdef CONFIG_DEBUG_FS
static struct dentry *ne6xvf_dbg_root;

void ne6xvf_showqueue(struct ne6xvf_adapter *pf)
{
	struct ne6x_ring *ring;
	u64 head, tail, oft;
	int i;

	dev_info(&pf->pdev->dev, "--------------------------------------------------------------------------------------------");
	for (i = 0; i < pf->num_active_queues; i++) {
		ring = &pf->rx_rings[i];
		head = rd64(&pf->hw, NE6XVF_REG_ADDR(i, NE6X_RQ_HD_POINTER));
		tail = rd64(&pf->hw, NE6XVF_REG_ADDR(i, NE6X_RQ_TAIL_POINTER));
		oft = rd64(&pf->hw, NE6XVF_REG_ADDR(i, NE6X_RQ_OFST));
		dev_info(&pf->pdev->dev, "----RX: Queue[%d]: H[0x%04llx], T[0x%04llx], RQ[0x%04llx], idle:%04d, alloc:%04d, use:%04d, clean:%04d\n",
			 i,
			 head,
			 tail,
			 oft,
			 NE6X_DESC_UNUSED(ring),
			 ring->next_to_alloc,
			 ring->next_to_use,
			 ring->next_to_clean);
	}

	dev_info(&pf->pdev->dev, "--------------------------------------------------------------------------------------------");
	for (i = 0; i < pf->num_active_queues; i++) {
		ring = &pf->tx_rings[i];
		head = rd64(&pf->hw, NE6XVF_REG_ADDR(i, NE6X_SQ_HD_POINTER));
		tail = rd64(&pf->hw, NE6XVF_REG_ADDR(i, NE6X_SQ_TAIL_POINTER));
		oft = rd64(&pf->hw, NE6XVF_REG_ADDR(i, NE6X_SQ_OFST));
		dev_info(&pf->pdev->dev, "----TX: Queue[%d]: H[0x%04llx], T[0x%04llx], SQ[0x%04llx], idle:%04d, use:%04d, clean:%04d\n",
			 i,
			 head,
			 tail,
			 oft,
			 NE6X_DESC_UNUSED(ring),
			 ring->next_to_use,
			 ring->next_to_clean);
	}

	dev_info(&pf->pdev->dev, "--------------------------------------------------------------------------------------------");
	for (i = 0; i < pf->num_active_queues; i++) {
		ring = &pf->cq_rings[i];
		head = rd64(&pf->hw, NE6XVF_REG_ADDR(i, NE6X_CQ_HD_POINTER));
		tail = rd64(&pf->hw, NE6XVF_REG_ADDR(i, NE6X_CQ_TAIL_POINTER));
		dev_info(&pf->pdev->dev, "----CQ: Queue[%d]: H[0x%04llx], T[0x%04llx], idle:%04d, use:%04d, clean:%04d\n",
			 i,
			 head,
			 tail,
			 NE6X_DESC_UNUSED(ring),
			 ring->next_to_use,
			 ring->next_to_clean);
	}
	dev_info(&pf->pdev->dev, "--------------------------------------------------------------------------------------------");
}

void ne6xvf_showring(struct ne6xvf_adapter *pf)
{
	struct ne6x_tx_desc *tx_desc;
	struct ne6x_cq_desc *cq_desc;
	union ne6x_rx_desc *rx_desc;
	struct ne6x_ring *ring;
	int j, k;

	for (j = 0; j < pf->num_active_queues; j++) {
		ring = &pf->rx_rings[j];

		for (k = 0; k < ring->count; k++) {
			rx_desc = NE6X_RX_DESC(ring, k);
			if (!rx_desc->wb.u.val)
			/* empty descriptor, skip */
				continue;

			dev_info(&pf->pdev->dev, "**** rx_desc[%d], vp[%d], m_len[%d], s_len[%d], s_addr[0x%llx], m_addr[0x%llx], flag[0x%x], vp[%d], pkt_len[%d]\n",
				 k,
				 rx_desc->w.vp,
				 rx_desc->w.mop_mem_len,
				 rx_desc->w.sop_mem_len,
				 rx_desc->w.buffer_sop_addr,
				 rx_desc->w.buffer_mop_addr,
				 rx_desc->wb.u.val,
				 rx_desc->wb.vp,
				 rx_desc->wb.pkt_len);
		}
	}

	for (j = 0; j < pf->num_active_queues; j++) {
		ring = &pf->tx_rings[j];

		for (k = 0; k < ring->count; k++) {
			tx_desc = NE6X_TX_DESC(ring, k);
			if (!tx_desc->buffer_sop_addr)
				/* empty descriptor, skp */
				continue;

			dev_info(&pf->pdev->dev, "**** tx_desc[%d], flag[0x%x], vp[%d], et[%d], ch[%d], tt[%d],sopv[%d],eopv[%d],tso[%d],l3chk[%d],l3oft[%d],l4chk[%d],l4oft[%d],pld[%d],mop[%d],sop[%d],mss[%d],mopa[%lld],sopa[%lld]\n",
				 k,
				 tx_desc->u.val,
				 tx_desc->vp,
				 tx_desc->event_trigger,
				 tx_desc->chain,
				 tx_desc->transmit_type,
				 tx_desc->sop_valid,
				 tx_desc->eop_valid,
				 tx_desc->tso,
				 tx_desc->l3_csum,
				 tx_desc->l3_ofst,
				 tx_desc->l4_csum,
				 tx_desc->l4_ofst,
				 tx_desc->pld_ofst,
				 tx_desc->mop_cnt,
				 tx_desc->sop_cnt,
				 tx_desc->mss,
				 tx_desc->buffer_mop_addr,
				 tx_desc->buffer_sop_addr);
		}
	}

	for (j = 0; j < pf->num_active_queues; j++) {
		ring = &pf->cq_rings[j];

		for (k = 0; k < ring->count; k++) {
			cq_desc = NE6X_CQ_DESC(ring, k);
			if (!cq_desc->num)
			/* empty descriptor, skip */
				continue;

			dev_info(&pf->pdev->dev, "**** cq_desc[%d], vp[%d], ctype[%d], num[%d]\n",
				 k,
				 ring->reg_idx,
				 cq_desc->ctype,
				 cq_desc->num);
		}
	}
}

static const struct ne6xvf_dbg_cmd_wr deg_cmd_wr[] = {
	{"queue", ne6xvf_showqueue},
	{"ring", ne6xvf_showring},
};

/**
 * nce_dbg_command_read - read for command datum
 * @filp: the opened file
 * @buffer: where to write the data for the user to read
 * @count: the size of the user's buffer
 * @ppos: file position offset
 **/
static ssize_t ne6xvf_dbg_command_read(struct file *filp, char __user *buffer, size_t count,
				       loff_t *ppos)
{
	return 0;
}

/**
 * ne6xvf_dbg_command_write - write into command datum
 * @filp: the opened file
 * @buffer: where to find the user's data
 * @count: the length of the user's data
 * @ppos: file position offset
 **/
static ssize_t ne6xvf_dbg_command_write(struct file *filp, const char __user *buffer, size_t count,
					loff_t *ppos)
{
	struct ne6xvf_adapter *pf = filp->private_data;
	char *cmd_buf, *cmd_buf_tmp;
	int bytes_not_copied;
	int i, cnt;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;

	/* don't cross maximal possible value */
	if (count >= NCE_DEBUG_CHAR_LEN)
		return -ENOSPC;

	cmd_buf = kzalloc(count + 1, GFP_KERNEL);
	if (!cmd_buf)
		return count;

	bytes_not_copied = copy_from_user(cmd_buf, buffer, count);
	if (bytes_not_copied) {
		kfree(cmd_buf);
		return -EFAULT;
	}
	cmd_buf[count] = '\0';

	cmd_buf_tmp = strchr(cmd_buf, '\n');
	if (cmd_buf_tmp) {
		*cmd_buf_tmp = '\0';
		count = cmd_buf_tmp - cmd_buf + 1;
	}

	if (strncmp(cmd_buf, "read", 4) == 0) {
		u32 base_addr;
		u32 offset_addr;
		u64 value = 0;

		cnt = sscanf(&cmd_buf[4], "%i %i", &base_addr, &offset_addr);
		if (cnt != 2) {
			dev_warn(&pf->pdev->dev, "read <reg_base> <reg_offset>\n");
			goto command_write_done;
		}
		dev_info(&pf->pdev->dev, "read: 0x%x 0x%x = 0x%llx\n", base_addr, offset_addr,
			 value);
	} else if (strncmp(cmd_buf, "write", 5) == 0) {
		u32 base_addr;
		u32 offset_addr;
		u64 value = 0;

		cnt = sscanf(&cmd_buf[5], "%i %i %lli ", &base_addr, &offset_addr, &value);
		if (cnt != 3) {
			dev_warn(&pf->pdev->dev, "write <reg_base> <reg_offset> <value>\n");
			goto command_write_done;
		}
		dev_info(&pf->pdev->dev, "write: 0x%x 0x%x = 0x%llx\n", base_addr, offset_addr,
			 value);
	} else {
		for (i = 0; i < ARRAY_SIZE(deg_cmd_wr); i++) {
			if (strncmp(cmd_buf, deg_cmd_wr[i].command, count) == 0) {
				deg_cmd_wr[i].command_proc(pf);
				goto command_write_done;
			}
		}

		dev_info(&pf->pdev->dev, "unknown command '%s'\n", cmd_buf);
	}

command_write_done:
	kfree(cmd_buf);
	cmd_buf = NULL;
	return count;
}

static const struct file_operations ne6xvf_dbg_command_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ne6xvf_dbg_command_read,
	.write = ne6xvf_dbg_command_write,
};

/**
 * nce_dbg_pf_init - setup the debugfs directory for the PF
 * @pf: the PF that is starting up
 **/
void ne6xvf_dbg_pf_init(struct ne6xvf_adapter *pf)
{
	const struct device *dev = &pf->pdev->dev;
	const char *name = pci_name(pf->pdev);
	struct dentry *pfile;

	pf->ne6xvf_dbg_pf = debugfs_create_dir(name, ne6xvf_dbg_root);
	if (!pf->ne6xvf_dbg_pf)
		return;

	pfile = debugfs_create_file("command", 0600, pf->ne6xvf_dbg_pf, pf,
				    &ne6xvf_dbg_command_fops);
	if (!pfile)
		goto create_failed;

	return;

create_failed:
	dev_info(dev, "debugfs dir/file for %s failed\n", name);
	debugfs_remove_recursive(pf->ne6xvf_dbg_pf);
}

/**
 * nce_dbg_pf_exit - clear out the PF's debugfs entries
 * @pf: the PF that is stopping
 **/
void ne6xvf_dbg_pf_exit(struct ne6xvf_adapter *pf)
{
	debugfs_remove_recursive(pf->ne6xvf_dbg_pf);
	pf->ne6xvf_dbg_pf = NULL;
}

/**
 * nce_dbg_init - start up debugfs for the driver
 **/
void ne6xvf_dbg_init(void)
{
	ne6xvf_dbg_root = debugfs_create_dir(ne6xvf_driver_name, NULL);
	if (!ne6xvf_dbg_root)
		pr_info("init of debugfs failed\n");
}

/**
 * nce_dbg_exit - clean out the driver's debugfs entries
 **/
void ne6xvf_dbg_exit(void)
{
	debugfs_remove_recursive(ne6xvf_dbg_root);
	ne6xvf_dbg_root = NULL;
}

#endif
