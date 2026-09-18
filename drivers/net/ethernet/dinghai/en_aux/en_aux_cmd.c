#include <linux/dinghai/kcompat.h>
#include <linux/dinghai/driver.h>
#include <linux/netdevice.h>
#include <linux/scatterlist.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/random.h>
#include <linux/jiffies.h>
#include <xen/xen.h>
#include "../slib.h"
#include <linux/dinghai/dh_cmd.h>
#include "../en_aux.h"
#include "../en_np/table/include/dpp_tbl_api.h"
#include "../msg_common.h"
#include "en_aux_cmd.h"
#include "../en_tc/en_tc.h"
#include "../en_np/driver/include/dpp_drv_sdt.h"

#define GET_EEPROM_INTERVAL_BEFORE_INIT_COMPLETE    1
#define GET_EEPROM_INTERVAL                         10

static int32_t write_queue_index_to_message(struct zxdh_en_device *en_dev, uint32_t queue_nums,
                                            uint32_t field, uint16_t *bytes, uint16_t *data, union zxdh_msg *old_msg)
{
    uint32_t ix = 0;
    uint16_t old_queue_nums = 0;


    if (OP_CODE_DATA_CHAN == field)
    {
        *bytes = (uint16_t)((queue_nums + 1) * ZXDH_QS_PAIRS);
        data[0] = (uint16_t)queue_nums;

        for (ix = 0; ix < queue_nums; ix = ix + ZXDH_QS_PAIRS)
        {
            data[ix + 1] = en_dev->phy_index[ix];     //en_dev->rq[ix / ZXDH_QS_PAIRS].vq->phy_index;
            data[ix + 2] = en_dev->phy_index[ix + 1]; //en_dev->sq[ix / ZXDH_QS_PAIRS].vq->phy_index;
        }

        if (old_msg != NULL)
        {
            LOG_DEBUG_DEV(en_dev->parent, "old_msg->reps.cmn_vq_msg.queue_nums: %u; queue_nums: %u",
                        old_msg->reps.cmn_vq_msg.queue_nums, queue_nums);
            if (old_msg->reps.cmn_vq_msg.queue_nums > 0)
            {
                old_queue_nums = old_msg->reps.cmn_vq_msg.queue_nums;
                if ((old_queue_nums + queue_nums) > 256)
                {
                    LOG_ERR_DEV(en_dev->parent, "Exceeded the maximum number of queues, old_queue_nums(%d)+queue_nums(%d)\n", old_queue_nums, queue_nums);
                    return -1;
                }

                *bytes = (uint16_t)((queue_nums + old_queue_nums + 1) * ZXDH_QS_PAIRS);
                data[0] = (uint16_t)(queue_nums + old_queue_nums);
                memcpy(data + queue_nums + 1, old_msg->reps.cmn_vq_msg.phy_qidx, old_queue_nums * ZXDH_QS_PAIRS);

                for (ix = 1; ix <= (queue_nums + old_queue_nums); ix++)
                {
                    LOG_DEBUG_DEV(en_dev->parent, "vq phy_qid: %d ", data[ix]);
                }
            }
        }
    }

    return 0;
}

static int32_t cmd_tbl_messgae_to_riscv_send(struct zxdh_en_device *en_dev, void *payload, uint32_t pld_len)
{
    int32_t ret = 0;
    struct cmd_hdr_recv *hdr_recv;
    struct cmd_tbl_ack cmd_tbl_ack = {0};
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_TBL, payload, &cmd_tbl_ack, &para);
    if (0 != ret)
    {
        LOG_ERR_DEV(en_dev->parent, "en_dev->ops->msg_send_cmd failed\n");
        goto out;
    }

    hdr_recv =(struct cmd_hdr_recv*)&cmd_tbl_ack;
    if (hdr_recv->check != OP_CODE_TBL_STAT)
    {
        LOG_ERR_DEV(en_dev->parent, "tbl init message recv check failed\n");
        ret = -1;
    }
out:
    return ret;
}

static int32_t cmd_common_tbl_init(struct zxdh_en_device *en_dev, uint32_t queue_nums, uint32_t field, union zxdh_msg *old_msg)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -1;
    }

    if ((2 * ZXDH_MAX_PAIRS_NUM) < queue_nums)
    {
        LOG_ERR_DEV(en_dev->parent, "queue pairs %u out of range\n", queue_nums);
        kfree(msg);
        return -ENOMEM;
    }

    msg->payload.hdr_to_cmn.field = field;
    msg->payload.hdr_to_cmn.type = OP_CODE_WRITE;
    msg->payload.hdr_to_cmn.pcie_id = en_dev->pcie_id;
    ret = write_queue_index_to_message(en_dev, queue_nums, field, \
                    &msg->payload.hdr_to_cmn.write_bytes, msg->payload.cmn_tbl_msg, old_msg);
    if (0 != ret)
    {
        LOG_ERR_DEV(en_dev->parent, "write_queue_index_to_message failed, ret: %d\n", ret);
        kfree(msg);
        return ret;
    }

    ret = cmd_tbl_messgae_to_riscv_send(en_dev, msg, MSG_STRUCT_HD_LEN + msg->payload.hdr_to_cmn.write_bytes);
    if (0 != ret)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_bar_chan_sync_msg_send failed, ret: %d\n", ret);
    }

    kfree(msg);

    return ret;
}

int32_t zxdh_common_tbl_init(struct net_device *netdev, union zxdh_msg *old_msg)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t ret = 0;

    ret = cmd_common_tbl_init(en_dev, en_dev->curr_queue_pairs * ZXDH_QS_PAIRS, OP_CODE_DATA_CHAN, old_msg);
    if (0 != ret)
    {
        LOG_ERR_DEV(en_dev->parent, "field data message failed\n");
        return -1;
    }

    return 0;
}

int32_t get_common_table_msg(struct zxdh_en_device *en_dev, uint16_t pcie_id, \
                                    uint8_t field, void *ack)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -1;
    }

    msg->payload.hdr_to_cmn.type = RISC_TYPE_READ;
    msg->payload.hdr_to_cmn.field = field;
    msg->payload.hdr_to_cmn.pcie_id = pcie_id;
    msg->payload.hdr_to_cmn.write_bytes = 0;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_TBL, msg, ack, &para);

    kfree(msg);

    return ret;
}

int32_t zxdh_hash_id_get(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -1;
    }

    ret = get_common_table_msg(en_dev, en_dev->pcie_id, RISC_FIELD_HASHID_CHANNEL, msg);
    if(ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "get own hash_id failed: %d\n", ret);
        kfree(msg);
        return ret;
    }

    en_dev->hash_search_idx = msg->reps.cmn_recv_msg.value;
    LOG_DEBUG_DEV(en_dev->parent, "hash_id: %u\n", en_dev->hash_search_idx);
    if (en_dev->hash_search_idx > ZXDH_MAX_HASH_INDEX)
    {
        LOG_ERR_DEV(en_dev->parent, "hash_id is invalid value: %u\n", en_dev->hash_search_idx);
        kfree(msg);
        return -EINVAL;
    }
    if (en_dev->hash_search_idx == ZXDH_MAX_HASH_INDEX) //TODO:if should be delete
    {
        en_dev->hash_search_idx = 1;
    }

    kfree(msg);

    return ret;
}

int32_t zxdh_phyport_get(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -1;
    }

    ret = get_common_table_msg(en_dev, en_dev->pcie_id, RISC_FIELD_PHYPORT_CHANNEL, msg);
    if(ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "get own phyport failed: %d\n", ret);
        kfree(msg);
        return ret;
    }

    en_dev->phy_port = msg->reps.cmn_recv_msg.value;
    if (en_dev->phy_port == INVALID_PHY_PORT)
    {
        LOG_ERR_DEV(en_dev->parent, "get phy_port failed\n");
        kfree(msg);
        return -EINVAL;
    }
    en_dev->ops->set_pf_phy_port(en_dev->parent, en_dev->phy_port);
    LOG_DEBUG_DEV(en_dev->parent, "0x%x phy_port: %u\n", en_dev->ep_bdf, en_dev->phy_port);

    kfree(msg);

    return ret;
}

int32_t zxdh_pf_macpcs_num_get(struct zxdh_en_device *en_dev)
{
    int32_t phy_port = 0;
    int32_t mac_num = 0; //0-2

    phy_port = en_dev->phy_port;

    if (phy_port < 4)
    {
        mac_num = 0;
    }
    else if (phy_port < 8)
    {
        mac_num = 1;
    }
    else if (phy_port < 10)
    {
        mac_num = 2;
    }
    else
    {
        LOG_ERR_DEV(en_dev->parent, "phy_port(%d) err, not in 0-9!!\n", phy_port);
        mac_num = -1;
        return mac_num;
    }

    LOG_DEBUG_DEV(en_dev->parent, "mac_num: %d\n", mac_num);
    return mac_num;
}

int32_t zxdh_lldp_enable_set(struct zxdh_en_device *en_dev,bool lldp_enable)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    msg->payload.hdr_to_agt.op_code = AGENT_DEBUG_LLDP_ENABLE_SET;
    msg->payload.hdr_to_agt.port_id = en_dev->panel_id;

    if (en_dev->ops->is_bond(en_dev->parent))
    {
        msg->payload.hdr_to_agt.port_id = en_dev->pannel_id;
    }
    msg->payload.lldp_msg.lldp_enable = lldp_enable;

    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_DEBUG, msg, msg, &para);
    kfree(msg);
    return err;
}

static int32_t zxdh_vf_dualtor_label_get(struct zxdh_en_device *en_dev, uint32_t *dual_tor)
{
    uint64_t dula_label_addr = 0;

    if (!en_dev)
    {
        LOG_ERR("en_dev is null.\n");
        return -1;
    }

    dula_label_addr = en_dev->ops->get_bar_virt_addr(en_dev->parent, 0) + ZXDH_DUALTOR_LABEL_OFFSET;
    *dual_tor = !!((ZXDH_BAR_DUALTOR_LABEL_ON == *(uint32_t*)dula_label_addr));
    return 0;
}

int32_t zxdh_prio_stat_switch(struct zxdh_en_device *en_dev, bool state)
{
    int ret = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    ret = dpp_pktrx_mcode_glb_cfg_write(&pf_info, ZXDH_NP_PRIO_STAT_ENABLE_BIT, ZXDH_NP_PRIO_STAT_ENABLE_BIT, state);
    if (ret != 0)
    {
        LOG_ERR("switch prio stat state: %u failed.\n", state);
        return -1;
    }

    return ret;
}

int32_t zxdh_dual_tor_switch(struct zxdh_en_device *en_dev, bool state)
{
    int ret = 0;
    DPP_PF_INFO_T pf_info = {0};
    uint64_t dula_label_addr = 0;

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) /* VF */
    {
        LOG_ERR_DEV(en_dev->parent, "vfs do not support dual switch.\n");
        return 0;
    }

    ret = dpp_pktrx_mcode_glb_cfg_write(&pf_info, ZXDH_NP_GLOBAL_PSN_ENABLE_BIT, ZXDH_NP_GLOBAL_PSN_ENABLE_BIT, state);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "switch dual tor to state: %u failed.\n", state);
        return -1;
    }

    dula_label_addr = en_dev->ops->get_bar_virt_addr(en_dev->parent, 0) + ZXDH_DUALTOR_LABEL_OFFSET;
    *(uint32_t *)dula_label_addr = state? ZXDH_BAR_DUALTOR_LABEL_ON : 0;

    ret = dpp_l2d_psn_cfg_set(&pf_info, state);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_l2d_psn_cfg_set failed.\n");
        return -1;
    }

    LOG_INFO_DEV(en_dev->parent, "switch dual tor to state: %u success.\n", state);
    return 0;
}

int32_t zxdh_dual_tor_label_get(struct zxdh_en_device *en_dev)
{
    int ret = 0;
    DPP_PF_INFO_T pf_info = {0};
    uint32_t global_value = 0;
    uint32_t psn_cfg = 0;
    uint32_t dula_tor = 0;

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) /* VF */
    {
        ret =  zxdh_vf_dualtor_label_get(en_dev, &dula_tor);
        if (ret != 0)
        {
            return -1;
        }
        goto succ;
    }

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    ret = dpp_l2d_psn_cfg_get(&pf_info, &psn_cfg);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_l2d_psn_cfg_get failed.\n");
        return -1;
    }

    if(psn_cfg != 0)
    {
        return psn_cfg;
    }

    ret = dpp_glb_cfg_get_1(&pf_info, &global_value);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_glb_cfg_get_1 failed.\n");
        return -1;
    }
    dula_tor = !!(global_value & ((uint32_t)1 << ZXDH_NP_GLOBAL_PSN_ENABLE_BIT));
succ:
    /* 开启为1，关闭为0, 异常为-1*/
    return dula_tor;
}

int32_t zxdh_sshd_enable_set(struct zxdh_en_device *en_dev, bool sshd_enable)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    if (sshd_enable)
    {
        msg->payload.hdr_to_agt.op_code = AGENT_SSHD_START;
    }
    else
    {
        msg->payload.hdr_to_agt.op_code = AGENT_SSHD_STOP;
    }

    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_LOGIN_CTRL, msg, msg, &para);
    kfree(msg);
    return err;
}

int32_t zxdh_lldp_enable_get(struct zxdh_en_device *en_dev, uint32_t *lldp_enable)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    msg->payload.hdr_to_agt.op_code = AGENT_DEBUG_LLDP_ENABLE_GET;
    msg->payload.hdr_to_agt.port_id = en_dev->panel_id;

    if (en_dev->ops->is_bond(en_dev->parent))
    {
        msg->payload.hdr_to_agt.port_id = en_dev->pannel_id;
    }

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_DEBUG, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_lldp_enable_get failed: %d\n", ret);
        kfree(msg);
        return ret;
    }

    *lldp_enable = (uint32_t)(msg->reps.debug_lldp_msg.lldp_status);
    kfree(msg);
    return ret;
}

int32_t zxdh_spm_port_enable_cfg(struct zxdh_en_device *en_dev, uint32_t enable)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    if (!zxdh_en_is_panel_port(en_dev))
        return ret;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    msg->payload.hdr_to_agt.op_code = AGENT_SPM_PORT_ENABLE_SET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;
    msg->payload.spm_port_enable_set.enable = enable;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "set spm port enable failed: %d\n", ret);
    }

    kfree(msg);
    return ret;
}

int32_t zxdh_en_firmware_version_get(struct zxdh_en_device *en_dev, uint8_t *fw_version)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    msg->payload.hdr_to_agt.op_code = AGENT_FLASH_FIR_VERSION_GET;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_FLASH, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "en_dev->ops->msg_send_cmd failed: %d\n", ret);
        goto free_msg;
    }

    memcpy(fw_version, msg->reps.flash_msg.firmware_version, FW_VERSION_LEN);
    fw_version[FW_VERSION_LEN - 1] = '\0';
free_msg:
    kfree(msg);
    return ret;
}

int32_t zxdh_get_np_fd_stats(DPP_PF_INFO_T *pf_info, uint8_t np_mode, bool fd_enable, uint64_t *hit_bytes_cnt, uint64_t *hit_packets_cnt, uint64_t *drop_bytes_cnt, uint64_t *drop_packets_cnt)
{
#define FD_ACL_INDEX_MAX   (2048)
    uint32_t err = 0;
    uint32_t index_num = 0;
    uint32_t i = 0;
    uint32_t *p_index_array = NULL;
    uint32_t index = 0;
    uint64_t bytes = 0;
    uint64_t packets = 0;
    uint64_t fdir_hits_bytes = 0;
    uint64_t fdir_hits_packets = 0;
    uint64_t fdir_drop_bytes = 0;
    uint64_t fdir_drop_packets = 0;
    ZXDH_FD_CFG_T p_fd_cfg = {0};

    /* 如果未开启fd功能, 统计值显示均为0 */
    if (!fd_enable)
    {
        *hit_bytes_cnt = 0;
        *hit_packets_cnt = 0;
        *drop_bytes_cnt = 0;
        *drop_packets_cnt = 0;
        return err;
    }

    /* 开启则获取统计值 */
    p_index_array = kzalloc(sizeof(ZXIC_UINT32) * FD_ACL_INDEX_MAX, GFP_KERNEL);
    if (p_index_array == NULL)
    {
        err = 1;
        LOG_ERR("kzalloc(%lu, GFP_KERNEL) failed !", sizeof(ZXIC_UINT32) * FD_ACL_INDEX_MAX);
        return err;
    }

    err = dpp_fd_acl_index_dump(pf_info, p_index_array, &index_num);
    if (err != 0)
    {
        LOG_ERR("dpp_fd_acl_index_dump failed!\n");
        goto free_index_array;
    }

    if (index_num > FD_ACL_INDEX_MAX)
    {
        err = 1;
        LOG_ERR("dpp_fd_acl_index_dump index_num is %u, exceed %u!\n", index_num, FD_ACL_INDEX_MAX);
        goto free_index_array;
    }

    for (i = 0; i < index_num; i++)
    {
        index = p_index_array[i];

        /* 普通规则和drop规则统计值累加 */
        err = dpp_tbl_fd_cfg_get(pf_info, ZXDH_SDT_FD_CFG_TABLE, index, &p_fd_cfg);
        if (err != 0) {
            LOG_ERR("dpp_tbl_fd_cfg_get failed!\n");
            goto free_index_array;
        }

        err = dpp_stat_fd_stat_cnt_get(pf_info, index, np_mode, &bytes, &packets);
        if (err != 0) {
            LOG_ERR("dpp_stat_fd_stat_cnt_get failed!\n");
            goto free_index_array;
        }

        fdir_hits_bytes += bytes;
        fdir_hits_packets += packets;

        if (p_fd_cfg.as_rlt.action_index & ACTION_TYPE_DROP)
        {
            fdir_drop_bytes += bytes;
            fdir_drop_packets += packets;
        }
    }

    *hit_bytes_cnt = fdir_hits_bytes;
    *hit_packets_cnt= fdir_hits_packets;
    *drop_bytes_cnt = fdir_drop_bytes;
    *drop_packets_cnt= fdir_drop_packets;

free_index_array:
    kfree(p_index_array);
    return err;
}

void do_get_np_ext_stats(struct zxdh_en_device *en_dev, struct zxdh_en_vport_stats *vport_stats)
{
    struct zxdh_np_ext_stats *ext_stats = NULL;

    if (!en_dev->ops->if_suport_np_ext_stats(en_dev->parent))
    {
        return;
    }

    ext_stats = en_dev->ops->get_np_ext_stats(en_dev->parent, en_dev->phy_port);

    /* vport_stats->np_stats.rx_vport_idma_drop_packets = ext_stats->rx_vport2np_packets; */
}

int32_t do_get_vport_stats(struct zxdh_en_device *en_dev, uint8_t np_mode,
                           struct zxdh_en_vport_stats *vport_stats, bool is_init_get)
{
    union zxdh_msg *msg = NULL;
    uint32_t vf_id = GET_VFID(en_dev->vport);
    uint32_t pf_id_offst = 0;
    int32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};
    struct zxdh_bar_extra_para para = {0};
    bool fd_enable = false;

    para.is_sync = true;
    para.retrycnt = 0;

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    fd_enable = en_dev->netdev->features & NETIF_F_NTUPLE;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    //DTP统计
    msg->payload.hdr_to_agt.op_code = AGENT_DTP_STATS_GET;
    msg->payload.hdr_to_agt.vf_id = vf_id;
    msg->payload.hdr_to_agt.pcie_id = en_dev->pcie_id;
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_DTP, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dtp_stats_get failed, err: %d\n", err);
        goto free_msg;
    }
    vport_stats->dtp_stats.rx_lro_packets = msg->reps.stats_msg.rx_total;
    vport_stats->dtp_stats.rx_udp_csum_fail_packets = msg->reps.stats_msg.tx_total;
    vport_stats->dtp_stats.tx_udp_csum_fail_packets = msg->reps.stats_msg.rx_total_bytes;
    vport_stats->dtp_stats.rx_tcp_csum_fail_packets = msg->reps.stats_msg.tx_total_bytes;
    vport_stats->dtp_stats.tx_tcp_csum_fail_packets = msg->reps.stats_msg.rx_good_bytes;
    vport_stats->dtp_stats.rx_ipv4_csum_fail_packets = msg->reps.stats_msg.tx_good_bytes;
    vport_stats->dtp_stats.tx_ipv4_csum_fail_packets = msg->reps.stats_msg.rx_error;

    //NP & RDMA统计
    memset(msg, 0, sizeof(union zxdh_msg));
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
    {
        msg->payload.hdr.op_code = ZXDH_GET_NP_STATS;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.vf_id = vf_id;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        msg->payload.np_stats_get_msg.clear_mode = np_mode;
        msg->payload.np_stats_get_msg.is_init_get = is_init_get;
        msg->payload.np_stats_get_msg.fd_enable = fd_enable;
        err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        if(err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_pf failed: %d\n", err);
            goto free_msg;
        }
        memcpy(&(vport_stats->np_stats), &(msg->reps.np_stats_msg), sizeof(vport_stats->np_stats));
    }
    else
    {
        dpp_stat_port_uc_packet_rx_cnt_get(&pf_info, vf_id, np_mode, &(vport_stats->np_stats.rx_vport_unicast_bytes), &(vport_stats->np_stats.rx_vport_unicast_packets));
        dpp_stat_port_uc_packet_tx_cnt_get(&pf_info, vf_id, np_mode, &(vport_stats->np_stats.tx_vport_unicast_bytes), &(vport_stats->np_stats.tx_vport_unicast_packets));
        dpp_stat_port_mc_packet_rx_cnt_get(&pf_info, vf_id, np_mode, &(vport_stats->np_stats.rx_vport_multicast_bytes), &(vport_stats->np_stats.rx_vport_multicast_packets));
        dpp_stat_port_mc_packet_tx_cnt_get(&pf_info, vf_id, np_mode, &(vport_stats->np_stats.tx_vport_multicast_bytes), &(vport_stats->np_stats.tx_vport_multicast_packets));
        dpp_stat_port_bc_packet_rx_cnt_get(&pf_info, vf_id, np_mode, &(vport_stats->np_stats.rx_vport_broadcast_bytes), &(vport_stats->np_stats.rx_vport_broadcast_packets));
        dpp_stat_port_bc_packet_tx_cnt_get(&pf_info, vf_id, np_mode, &(vport_stats->np_stats.tx_vport_broadcast_bytes), &(vport_stats->np_stats.tx_vport_broadcast_packets));
        dpp_stat_MTU_packet_msg_rx_cnt_get(&pf_info, vf_id, np_mode, &(vport_stats->np_stats.rx_vport_mtu_drop_bytes), &(vport_stats->np_stats.rx_vport_mtu_drop_packets));
        dpp_stat_MTU_packet_msg_tx_cnt_get(&pf_info, vf_id, np_mode, &(vport_stats->np_stats.tx_vport_mtu_drop_bytes), &(vport_stats->np_stats.tx_vport_mtu_drop_packets));
        dpp_stat_plcr_packet_drop_rx_cnt_get(&pf_info, vf_id, np_mode, &(vport_stats->np_stats.rx_vport_plcr_drop_bytes), &(vport_stats->np_stats.rx_vport_plcr_drop_packets));
        dpp_stat_plcr_packet_drop_tx_cnt_get(&pf_info, vf_id, np_mode, &(vport_stats->np_stats.tx_vport_plcr_drop_bytes), &(vport_stats->np_stats.tx_vport_plcr_drop_packets));
        pf_id_offst = DH_AUX_PF_ID_OFFSET(en_dev->vport);
        dpp_stat_spoof_packet_drop_cnt_get(&pf_info, pf_id_offst, np_mode, &(vport_stats->np_stats.tx_vport_ssvpc_packets));
        do_get_np_ext_stats(en_dev, vport_stats);
        err = zxdh_get_np_fd_stats(&pf_info, np_mode, fd_enable, &(vport_stats->np_stats.rx_vport_fdir_hits_bytes), &(vport_stats->np_stats.rx_vport_fdir_hits_packets), \
                            &(vport_stats->np_stats.rx_vport_fdir_drop_bytes), &(vport_stats->np_stats.rx_vport_fdir_drop_packets));
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_get_np_fd_stats failed, err: %d\n", err);
            goto free_msg;
        }
    }

    //VQM统计
    vport_stats->vqm_stats.rx_vport_packets = (vport_stats->np_stats.rx_vport_unicast_packets + vport_stats->np_stats.rx_vport_multicast_packets +
                                               vport_stats->np_stats.rx_vport_broadcast_packets);
    vport_stats->vqm_stats.tx_vport_packets = (vport_stats->np_stats.tx_vport_unicast_packets + vport_stats->np_stats.tx_vport_multicast_packets +
                                               vport_stats->np_stats.tx_vport_broadcast_packets);
    vport_stats->vqm_stats.rx_vport_bytes = (vport_stats->np_stats.rx_vport_unicast_bytes + vport_stats->np_stats.rx_vport_multicast_bytes +
                                             vport_stats->np_stats.rx_vport_broadcast_bytes);
    vport_stats->vqm_stats.tx_vport_bytes = (vport_stats->np_stats.tx_vport_unicast_bytes + vport_stats->np_stats.tx_vport_multicast_bytes +
                                             vport_stats->np_stats.tx_vport_broadcast_bytes);

free_msg:
    kfree(msg);
    return err;
}

int32_t zxdh_en_vport_pre_stats_get(struct zxdh_en_device *en_dev)
{
    int32_t err = 0;
    struct zxdh_en_vport_stats *vport_stats = &en_dev->pre_stats;

    err = do_get_vport_stats(en_dev, NP_GET_PKT_CNT, vport_stats, TRUE);
    if(err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_vport_pre_stat_get failed\n");
    }
    en_dev->last_stats = en_dev->pre_stats;
    en_dev->last_tx_vport_ssvpc_packets = en_dev->pre_stats.np_stats.tx_vport_ssvpc_packets;
    return err;
}

int32_t zxdh_en_udp_pkt_stats_get(struct zxdh_en_device *en_dev)
{
    union zxdh_msg *msg = NULL;
    uint32_t vf_id = GET_VFID(en_dev->vport);
    DPP_PF_INFO_T pf_info = {0};
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (!zxdh_en_is_panel_port(en_dev))
        return 0;

    if(en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
        if (msg == NULL)
        {
            LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
            return -ENOMEM;
        }

        msg->payload.hdr.op_code = ZXDH_VF_GET_UDP_STATS;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.vf_id = vf_id;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        if(err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_pf failed: %d\n", err);
            kfree(msg);
            return err;
        }
        zte_memcpy_s(&en_dev->hw_stats.udp_stats, &msg->reps.udp_phy_stats_msg, sizeof(udp_phy_stats));
        kfree(msg);
        return 0;
    }

    err = dpp_stat_asn_phyport_rx_pkt_cnt_get(&pf_info, en_dev->phy_port, STAT_RD_CLR_MODE_UNCLR, \
                                                 &en_dev->hw_stats.udp_stats.rx_arn_phy);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_stat_asn_phyport_rx_pkt_cnt_get failed: %d\n", err);
        return -1;
    }

    err = dpp_stat_psn_phyport_tx_pkt_cnt_get(&pf_info, en_dev->phy_port, STAT_RD_CLR_MODE_UNCLR, \
                                                &en_dev->hw_stats.udp_stats.tx_psn_phy);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_stat_psn_phyport_tx_pkt_cnt_get failed: %d\n", err);
        return -1;
    }

    err = dpp_stat_psn_phyport_rx_pkt_cnt_get(&pf_info, en_dev->phy_port, STAT_RD_CLR_MODE_UNCLR, \
                                                &en_dev->hw_stats.udp_stats.rx_psn_phy);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_stat_psn_phyport_rx_pkt_cnt_get failed: %d\n", err);
        return -1;
    }

    err = dpp_stat_psn_ack_phyport_tx_pkt_cnt_get(&pf_info, en_dev->phy_port, STAT_RD_CLR_MODE_UNCLR, \
                                                &en_dev->hw_stats.udp_stats.tx_psn_ack_phy);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_stat_psn_ack_phyport_tx_pkt_cnt_get failed: %d\n", err);
        return -1;
    }

    err = dpp_stat_psn_ack_phyport_rx_pkt_cnt_get(&pf_info, en_dev->phy_port, STAT_RD_CLR_MODE_UNCLR, \
                                                    &en_dev->hw_stats.udp_stats.rx_psn_ack_phy);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_stat_psn_ack_phyport_rx_pkt_cnt_get failed: %d\n", err);
        return -1;
    }

    return err;
}

int32_t zxdh_en_np_stats_get(struct zxdh_en_device *en_dev)
{
    int32_t err = 0;
    u_int32_t i = 0;

    DPP_PF_INFO_T pf_info = {0};
    DPP_AGENT_CHANNEL_STAT_INFO_T stats_info = {0};
    DPP_PRIO_STAT_DATA_T stat_data = {0};
    DPP_STAT_VALUE_U stat_value = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (!en_dev->dcb_para.ets_node_list_head[ZXDH_DCBNL_ETS_TREE_FLOW_LEVEL].flow_next) {
        LOG_DEBUG("NULL pointer detected in stats collection\n");
        return -EINVAL;
    }
    // TC[0]->sp7
    stats_info.flow_id = en_dev->dcb_para.ets_node_list_head[ZXDH_DCBNL_ETS_TREE_FLOW_LEVEL].flow_next->flow_id - 7;
    stats_info.phy_port = en_dev->phy_port;

    // LOG_INFO("slot: %u, vport: 0x%x, phy_port: %u, flow_id: %u\n", pf_info.slot, pf_info.vport, stats_info.phy_port, stats_info.flow_id);

    err = dpp_channel_stat_info_get(&pf_info, &stats_info, &stat_data);
    if (err != 0) {
        LOG_DEBUG("dpp_channel_stat_info_get failed: 0x%x\n", err);
        return err;
    }

    if(en_dev->ets_info.cur_ets)
    {
        for (i = 0; i < ZXDH_DCBNL_MAX_PRIORITY; i++) {
            en_dev->hw_stats.tm_odma_stats[i].tx_pkts = 
                stat_data.tm_ets_stat_data[i].deque_pkt_cnt;
            en_dev->hw_stats.tm_odma_stats[i].tx_bytes = 
                stat_data.tm_ets_stat_data[i].deque_pktB_cnt;
        }
    }
    else
    {
        // LOG_INFO("slot: %u, vport: 0x%x, phy_port: %u\n", pf_info.slot, pf_info.vport, stats_info.phy_port);
        for (i = 0; i < DPP_PRIO_COS_NUM; i++) {
            err = dpp_stat_item_cnt_get(&pf_info, DPP_STAT_ITEM_ODMA_PHYPORT_PRIO_STAT, stats_info.phy_port * 8 + i, 0, &stat_value);
            if (err != 0 && err != DPP_RC_TABLE_STAT_ITEM_INVALID) {
                LOG_DEBUG("dpp_channel_stat_info_get failed: 0x%x\n", err);
                return err;
            }
            en_dev->hw_stats.tm_odma_stats[i].tx_pkts = (err == 0) ? stat_value.stat_cnt_128.pkts : (uint64_t)stat_data.odma_stat_data[i].queue_tx_cnt;
            en_dev->hw_stats.tm_odma_stats[i].tx_bytes = (err == 0) ? stat_value.stat_cnt_128.bytes : 0;
        }
    }

    for (i = 0; i < DPP_PRIO_COS_NUM; i++) {
        err = dpp_stat_item_cnt_get(&pf_info, DPP_STAT_ITEM_IDMA_PHYPORT_PRIO_STAT, stats_info.phy_port * 8 + i, 0, &stat_value);
        if (err != 0) {
            LOG_DEBUG("dpp_channel_stat_info_get failed: 0x%x\n", err);
            return err;
        }
        en_dev->hw_stats.idma_stats[i].rx_pkts = stat_value.stat_cnt_128.pkts;
        en_dev->hw_stats.idma_stats[i].rx_bytes = stat_value.stat_cnt_128.bytes;
    }

    return err;
}

static void en_stats_work_handler(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct zxdh_en_device *en_dev = container_of(dwork, struct zxdh_en_device, stats_work);
    int32_t rc;

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        if (en_dev->ops->is_fw_feature_support(en_dev->parent, FW_FEATURE_PRIO_STATS) &&
            zxdh_en_is_panel_port(en_dev) && en_dev->device_state != ZXDH_DEVICE_STATE_INTERNAL_ERROR)
        {
            rc = zxdh_en_np_stats_get(en_dev);
            if (rc != 0 && rc != DPP_RC_DTB_STAT_QUEUE_NOT_ENABLE && rc != DPP_RC_DTB_STAT_OVER_TIME &&
                rc != ZXIC_PAR_CHK_BAR_ABNORMAL && rc != DPP_RC_DTB_STAT_QUEUE_ITEM_SW_EMPTY)
            {
                LOG_DEBUG("zxdh_en_np_stats_get failed: 0x%x\n", rc);
            }
        }
    }

    rc = zxdh_vport_stats_get(en_dev, TRUE);
    if (rc != 0)
    {
        LOG_DEBUG("zxdh_vport_stats_get failed: 0x%x\n", rc);
    }
}

#define EN_STATS_SERVICE_TIMER_PF_BASE_MS 1000UL
#define EN_STATS_SERVICE_TIMER_VF_BASE_MS 5000UL
#define RANDOM_RANGE_0_TO_2000 2001
static void en_stats_service_timer(struct timer_list *t)
{
    unsigned long next_stat_offset = 0;
    unsigned long period_ms = 0;
    struct zxdh_en_device *en_dev = from_timer(en_dev, t, service_stat_timer);

    /* PF: 固定 1 秒 1 次(1000ms);
     * 非 PF(VF 等): 采集周期 5~7 秒, 精确到 ms 级, 步进 1ms。
     * 注: 最终精度受限于 jiffies (1/HZ 秒), HZ=1000 时即 1ms。
     */
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF) {
        period_ms = EN_STATS_SERVICE_TIMER_PF_BASE_MS;
    } else {
        /* 0~2000 的随机毫秒偏移, 加上基准 5000ms 后落在 [5000, 7000]ms */
        uint32_t rand_ms = (uint32_t)(get_random_u32() % RANDOM_RANGE_0_TO_2000);
        period_ms = EN_STATS_SERVICE_TIMER_VF_BASE_MS + (unsigned long)rand_ms;
    }

    next_stat_offset = msecs_to_jiffies(period_ms);

    if (en_dev->stats_wq) {

        queue_delayed_work(en_dev->stats_wq, &en_dev->stats_work, 0);
    }

    mod_timer(&en_dev->service_stat_timer, jiffies + next_stat_offset);
}

void zxdh_en_stats_init(struct zxdh_en_device *en_dev)
{
    // 创建单线程工作队列
    en_dev->stats_wq = create_singlethread_workqueue("stats_wq");
    // 初始化延迟工作项
    INIT_DELAYED_WORK(&en_dev->stats_work, en_stats_work_handler);

    timer_setup(&en_dev->service_stat_timer, en_stats_service_timer, 0);
    mod_timer(&en_dev->service_stat_timer, jiffies);
}

void zxdh_en_stats_uninit(struct zxdh_en_device *en_dev)
{
    del_timer_sync(&en_dev->service_stat_timer);
    if (en_dev->stats_wq) {
        cancel_delayed_work_sync(&en_dev->stats_work);
        flush_workqueue(en_dev->stats_wq);
        destroy_workqueue(en_dev->stats_wq);
        en_dev->stats_wq = NULL;
    }
}

static struct zxdh_en_module_eeprom_param g_module_regions[] = {
    [ZXDH_MOD_LOW_OFF0_LEN1]    = { .i2c_addr = SFF_I2C_ADDRESS_LOW,  .bank = 0, .page = 0,  .offset = 0,   .length = 1   },
    [ZXDH_MOD_LOW_OFF0_LEN128]  = { .i2c_addr = SFF_I2C_ADDRESS_LOW,  .bank = 0, .page = 0,  .offset = 0,   .length = 128 },
    [ZXDH_MOD_LOW_OFF128]       = { .i2c_addr = SFF_I2C_ADDRESS_LOW,  .bank = 0, .page = 0,  .offset = 128, .length = 128 },
    [ZXDH_MOD_LOW_PG1]          = { .i2c_addr = SFF_I2C_ADDRESS_LOW,  .bank = 0, .page = 1,  .offset = 128, .length = 128 },
    [ZXDH_MOD_LOW_PG2]          = { .i2c_addr = SFF_I2C_ADDRESS_LOW,  .bank = 0, .page = 2,  .offset = 128, .length = 128 },
    [ZXDH_MOD_LOW_PG3]          = { .i2c_addr = SFF_I2C_ADDRESS_LOW,  .bank = 0, .page = 3,  .offset = 128, .length = 128 },
    [ZXDH_MOD_LOW_PG17_BANK0]   = { .i2c_addr = SFF_I2C_ADDRESS_LOW,  .bank = 0, .page = 17, .offset = 128, .length = 128 },
    [ZXDH_MOD_HIGH_OFF0_LEN128] = { .i2c_addr = SFF_I2C_ADDRESS_HIGH, .bank = 0, .page = 0,  .offset = 0,   .length = 128 },
};

static void zxdh_netlink_module_info_free(struct zxdh_netlink_module_info *mi)
{
    int i;

    if (!mi)
        return;

    for (i = 0; i < ARRAY_SIZE(g_module_regions); i++) {
        if (mi->regions[i].data) {
            kfree(mi->regions[i].data);
            mi->regions[i].data = NULL;
        }
        mi->regions[i].len  = 0;
        mi->regions[i].read_bytes  = 0;
    }
    mutex_destroy(&mi->lock);
}

static int32_t zxdh_netlink_module_info_init(struct zxdh_netlink_module_info *mi)
{
    int i;

    if (!mi)
        return -EINVAL;

    mutex_init(&mi->lock);
    for (i = 0; i < ARRAY_SIZE(g_module_regions); i++) {
        uint8_t len = g_module_regions[i].length;
        mi->regions[i].data = kzalloc(len, GFP_KERNEL);
        if (!mi->regions[i].data) {
            LOG_ERR("kzalloc module region[%d] (len=%u) failed\n", i, len);
            zxdh_netlink_module_info_free(mi);
            return -ENOMEM;
        }
        mi->regions[i].len = len;
        mi->regions[i].read_bytes  = 0;
    }
    return 0;
}

static void zxdh_module_region_refresh(struct zxdh_en_device *en_dev,
                                       enum zxdh_module_region_id id)
{
    struct zxdh_netlink_module_info   *mi = &en_dev->netlink_module_info;
    struct zxdh_module_region *r;
    struct zxdh_en_module_eeprom_param *q;
    uint8_t *new_data = NULL;
    uint32_t n;

    if ((unsigned int)id >= ARRAY_SIZE(g_module_regions))
        return;

    r = &mi->regions[id];
    q = &g_module_regions[id];
    if (!r->data || r->len == 0)
        return;

    /* 在锁外准备新 buffer，慢路径读取期间读侧可继续用旧缓存 */
    new_data = kzalloc(r->len, GFP_KERNEL);
    if (!new_data) {
        LOG_DEBUG_DEV(en_dev->parent, "kzalloc buffer (len=%u) failed\n", r->len);
        return;
    }

    /* 慢路径 eeprom_read 在锁外执行，读侧不会被 bar 通道往返阻塞 */
    n = zxdh_en_module_eeprom_read(en_dev, q, new_data);
    if (n != q->length) {
        /* 读取失败：旧 r->data 不变，直接丢弃新 buffer */
        kfree(new_data);
        return;
    }

    /* 仅交换瞬间持锁：拷贝新数据 + 发布 read_bytes（临界区约微秒级） */
    mutex_lock(&mi->lock);
    zte_memcpy_s(r->data, new_data, r->len);
    r->read_bytes = (int32_t)n;
    mutex_unlock(&mi->lock);

    kfree(new_data);
}

static void zxdh_module_regions_refresh(struct zxdh_en_device *en_dev,
                                           const enum zxdh_module_region_id *ids,
                                           uint32_t nr_ids)
{
    uint32_t i;

    if (!ids || nr_ids == 0)
        return;
    if (nr_ids > ARRAY_SIZE(g_module_regions))
        return;
    for (i = 0; i < nr_ids; i++) {
        zxdh_module_region_refresh(en_dev, ids[i]);
    }
    return;
}

int32_t zxdh_module_region_copy_from_cache(struct zxdh_en_device *en_dev,
                                           struct zxdh_en_module_eeprom_param *query,
                                           uint8_t *dst)
{
    struct zxdh_netlink_module_info *mi = &en_dev->netlink_module_info;
    int i;
    int32_t ret = 0;

    if (!dst || !query)
        return ret;

    for (i = 0; i < ARRAY_SIZE(g_module_regions); i++) {
        struct zxdh_en_module_eeprom_param *d = &g_module_regions[i];
        struct zxdh_module_region                *r = &mi->regions[i];

        if (d->i2c_addr != query->i2c_addr || d->bank != query->bank || \
            d->page != query->page || d->offset != query->offset || d->length != query->length)
            continue;
        if (!r->data)
            continue;

        mutex_lock(&mi->lock);
        zte_memcpy_s(dst, r->data, r->read_bytes);
        ret = r->read_bytes;
        mutex_unlock(&mi->lock);
        break;
    }
    return ret;
}

static int32_t zxdh_get_module_identify(struct zxdh_en_device *en_dev, uint8_t *identify)
{
    uint32_t read_bytes;
    struct zxdh_en_module_eeprom_param query = {0};

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    query.i2c_addr = SFF_I2C_ADDRESS_LOW;
    query.page = 0;
    query.offset = 0;
    query.length = 1;
    read_bytes = zxdh_en_module_eeprom_read(en_dev, &query, identify);
    if(read_bytes != query.length)
    {
        return -EIO;
    }
    return 0;
}

static const enum zxdh_module_region_id sff8079_regions[] = {
    ZXDH_MOD_LOW_OFF0_LEN1,
    ZXDH_MOD_LOW_OFF0_LEN128,    /* A0h @ off 0, len 128 */
    ZXDH_MOD_HIGH_OFF0_LEN128,   /* A2h @ off 0, len 128 */
};

static void zxdh_sff8079_get_module_data(struct zxdh_en_device *en_dev)
{
    /* 由 task 已做 device_state / panel_port / init 检查，这里直接刷缓存 */
    zxdh_module_regions_refresh(en_dev, sff8079_regions,
                                ARRAY_SIZE(sff8079_regions));
}

static const enum zxdh_module_region_id sff8636_regions[] = {
    ZXDH_MOD_LOW_OFF0_LEN1,
    ZXDH_MOD_LOW_OFF0_LEN128,    /* A0h page 0 @ off 0,   len 128 — Serial ID */
    ZXDH_MOD_LOW_OFF128,         /* A0h page 0 @ off 128, len 128 — Status/Threshold */
    ZXDH_MOD_LOW_PG3,            /* A0h page 3 @ off 128, len 128 — Page 3 扩展 */
};

static void zxdh_sff8636_get_module_data(struct zxdh_en_device *en_dev)
{
    zxdh_module_regions_refresh(en_dev, sff8636_regions,
                                ARRAY_SIZE(sff8636_regions));
}

static const enum zxdh_module_region_id cmis_regions[] = {
    ZXDH_MOD_LOW_OFF0_LEN1,
    ZXDH_MOD_LOW_OFF0_LEN128,
    ZXDH_MOD_LOW_OFF128,
    ZXDH_MOD_LOW_PG1,
    ZXDH_MOD_LOW_PG2,
    ZXDH_MOD_LOW_PG17_BANK0,
};

static void zxdh_cmis_get_module_data(struct zxdh_en_device *en_dev)
{
    zxdh_module_regions_refresh(en_dev, cmis_regions,
                                ARRAY_SIZE(cmis_regions));
}

static const enum zxdh_module_region_id default_regions[] = {
    ZXDH_MOD_LOW_OFF0_LEN1,
    ZXDH_MOD_LOW_OFF0_LEN128,
};

static void zxdh_get_default_module_data(struct zxdh_en_device *en_dev)
{
    zxdh_module_regions_refresh(en_dev, default_regions,
                                ARRAY_SIZE(default_regions));
}

static void en_eeprom_work_handler(struct work_struct *_work)
{
    struct zxdh_en_device *en_dev = container_of(_work, struct zxdh_en_device,
                                                 eeprom_work);
    uint8_t identifier;
    int32_t ret;

    ZXDH_AUX_INIT_COMP_CHECK(en_dev);

    if ((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) ||
        (!zxdh_en_is_panel_port(en_dev))) {
        return;
    }

    ret = zxdh_get_module_identify(en_dev, &identifier);
    if (ret != 0) {
        return;
    }

    switch (identifier) {
    case ZXDH_MODULE_ID_SFP:
        zxdh_sff8079_get_module_data(en_dev);
        break;
    case ZXDH_MODULE_ID_QSFP:
    case ZXDH_MODULE_ID_QSFP_PLUS:
    case ZXDH_MODULE_ID_QSFP28:
        zxdh_sff8636_get_module_data(en_dev);
        break;
    case ZXDH_MODULE_ID_QSFP_DD:
    case ZXDH_MODULE_ID_OSFP:
    case ZXDH_MODULE_ID_DSFP:
    case ZXDH_MODULE_ID_QSFP_PLUS_WITH_CMIS:
    case ZXDH_MODULE_ID_SFP_DD_WITH_CMIS:
    case ZXDH_MODULE_ID_SFP_PLUS_WITH_CMIS:
        zxdh_cmis_get_module_data(en_dev);
        break;
    default:
        zxdh_get_default_module_data(en_dev);
        break;
    }
    return;
}

static void en_eeprom_service_timer(struct timer_list *t)
{
    unsigned long next_event_offset;
    struct zxdh_en_device *en_dev = from_timer(en_dev, t, service_eeprom_timer);

    if (en_dev->init_comp_flag != AUX_INIT_COMPLETED)
    {
        next_event_offset = HZ * GET_EEPROM_INTERVAL_BEFORE_INIT_COMPLETE;
    }
    else
    {
        next_event_offset = HZ * GET_EEPROM_INTERVAL;
    }

    if (en_dev->eeprom_wq) {
        queue_work(en_dev->eeprom_wq, &en_dev->eeprom_work);
    }
    mod_timer(&en_dev->service_eeprom_timer, next_event_offset + jiffies);
}

void zxdh_en_eeprom_init(struct zxdh_en_device *en_dev)
{
    int32_t ret;

    if((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) ||
       (!zxdh_en_is_panel_port(en_dev)))
    {
        return;
    }

    ret = zxdh_netlink_module_info_init(&en_dev->netlink_module_info);
    if (ret != 0)
    {
        LOG_ERR("zxdh_netlink_module_info_init failed: %d\n", ret);
        return;
    }

    en_dev->eeprom_wq = create_singlethread_workqueue("dh_eeprom_get");
    INIT_WORK(&en_dev->eeprom_work, en_eeprom_work_handler);

    timer_setup(&en_dev->service_eeprom_timer, en_eeprom_service_timer, 0);
    mod_timer(&en_dev->service_eeprom_timer, jiffies);
}

void zxdh_en_eeprom_uninit(struct zxdh_en_device *en_dev)
{
    if((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) ||
       (!zxdh_en_is_panel_port(en_dev)))
    {
        return;
    }

    del_timer_sync(&en_dev->service_eeprom_timer);
    if (en_dev->eeprom_wq) {
        destroy_workqueue(en_dev->eeprom_wq);
        en_dev->eeprom_wq = NULL;
    }

    zxdh_netlink_module_info_free(&en_dev->netlink_module_info);
}

#define UPDATE_VPORT_U32_EXTENDED_STATS(stats, member) \
    do { \
        if (cur_stats->stats.member < last_stats->stats.member) { \
            high_count->stats.member++; \
        } \
        extended_stats->stats.member = \
            ((uint64_t)high_count->stats.member << 32) | \
            (uint64_t)cur_stats->stats.member; \
    } while(0)

int32_t zxdh_vport_stats_get(struct zxdh_en_device *en_dev, bool is_time_get)
{
    int32_t err = 0;
    struct zxdh_en_vport_stats *last_stats;
    struct zxdh_en_vport_stats *cur_stats;
    struct zxdh_en_vport_stats *high_count;
    struct zxdh_en_vport_stats *extended_stats;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    cur_stats = kzalloc(sizeof(struct zxdh_en_vport_stats), GFP_KERNEL);
    if (cur_stats == NULL)
    {
        LOG_DEBUG("kzalloc(zxdh_en_vport_stats %lu, GFP_KERNEL) failed !", sizeof(struct zxdh_en_vport_stats));
        return -ENOMEM;
    }

    err = do_get_vport_stats(en_dev, NP_GET_PKT_CNT, cur_stats, FALSE);
    if(err != 0)
    {
        kfree(cur_stats);
        LOG_ERR_DEV(en_dev->parent, "zxdh_vport_stats_get failed\n");
        return err;
    }

    spin_lock_bh(&en_dev->vport_stats_lock);

    high_count = &en_dev->high_count;
    extended_stats = &en_dev->extended_stats;
    last_stats = &en_dev->last_stats;
    UPDATE_VPORT_U32_EXTENDED_STATS(dtp_stats, rx_lro_packets);
    UPDATE_VPORT_U32_EXTENDED_STATS(dtp_stats, rx_udp_csum_fail_packets);
    UPDATE_VPORT_U32_EXTENDED_STATS(dtp_stats, tx_udp_csum_fail_packets);
    UPDATE_VPORT_U32_EXTENDED_STATS(dtp_stats, rx_tcp_csum_fail_packets);
    UPDATE_VPORT_U32_EXTENDED_STATS(dtp_stats, tx_tcp_csum_fail_packets);
    UPDATE_VPORT_U32_EXTENDED_STATS(dtp_stats, rx_ipv4_csum_fail_packets);
    UPDATE_VPORT_U32_EXTENDED_STATS(dtp_stats, tx_ipv4_csum_fail_packets);
    *last_stats = *cur_stats;

    if(!is_time_get)
    {
        struct zxdh_en_vport_stats *vport_stats = &en_dev->hw_stats.vport_stats;
        struct zxdh_en_vport_stats *pre_stats = &en_dev->pre_stats;

        vport_stats->vqm_stats.rx_vport_packets = CALC_DELTA_64(last_stats->vqm_stats.rx_vport_packets,
            pre_stats->vqm_stats.rx_vport_packets);
        vport_stats->vqm_stats.tx_vport_packets = CALC_DELTA_64(last_stats->vqm_stats.tx_vport_packets,
            pre_stats->vqm_stats.tx_vport_packets);
        vport_stats->vqm_stats.rx_vport_bytes = CALC_DELTA_64(last_stats->vqm_stats.rx_vport_bytes,
            pre_stats->vqm_stats.rx_vport_bytes);
        vport_stats->vqm_stats.tx_vport_bytes = CALC_DELTA_64(last_stats->vqm_stats.tx_vport_bytes,
            pre_stats->vqm_stats.tx_vport_bytes);

        vport_stats->dtp_stats.rx_lro_packets = CALC_DELTA_64(extended_stats->dtp_stats.rx_lro_packets,
            pre_stats->dtp_stats.rx_lro_packets);
        vport_stats->dtp_stats.rx_udp_csum_fail_packets = CALC_DELTA_64(extended_stats->dtp_stats.rx_udp_csum_fail_packets,
            pre_stats->dtp_stats.rx_udp_csum_fail_packets);
        vport_stats->dtp_stats.tx_udp_csum_fail_packets = CALC_DELTA_64(extended_stats->dtp_stats.tx_udp_csum_fail_packets,
            pre_stats->dtp_stats.tx_udp_csum_fail_packets);
        vport_stats->dtp_stats.rx_tcp_csum_fail_packets = CALC_DELTA_64(extended_stats->dtp_stats.rx_tcp_csum_fail_packets,
            pre_stats->dtp_stats.rx_tcp_csum_fail_packets);
        vport_stats->dtp_stats.tx_tcp_csum_fail_packets = CALC_DELTA_64(extended_stats->dtp_stats.tx_tcp_csum_fail_packets,
            pre_stats->dtp_stats.tx_tcp_csum_fail_packets);
        vport_stats->dtp_stats.rx_ipv4_csum_fail_packets = CALC_DELTA_64(extended_stats->dtp_stats.rx_ipv4_csum_fail_packets,
            pre_stats->dtp_stats.rx_ipv4_csum_fail_packets);
        vport_stats->dtp_stats.tx_ipv4_csum_fail_packets = CALC_DELTA_64(extended_stats->dtp_stats.tx_ipv4_csum_fail_packets,
            pre_stats->dtp_stats.tx_ipv4_csum_fail_packets);

        vport_stats->np_stats.rx_vport_unicast_packets = CALC_DELTA_64(last_stats->np_stats.rx_vport_unicast_packets,
            pre_stats->np_stats.rx_vport_unicast_packets);
        vport_stats->np_stats.tx_vport_unicast_packets = CALC_DELTA_64(last_stats->np_stats.tx_vport_unicast_packets,
            pre_stats->np_stats.tx_vport_unicast_packets);
        vport_stats->np_stats.rx_vport_unicast_bytes = CALC_DELTA_64(last_stats->np_stats.rx_vport_unicast_bytes,
            pre_stats->np_stats.rx_vport_unicast_bytes);
        vport_stats->np_stats.tx_vport_unicast_bytes = CALC_DELTA_64(last_stats->np_stats.tx_vport_unicast_bytes,
            pre_stats->np_stats.tx_vport_unicast_bytes);
        vport_stats->np_stats.rx_vport_multicast_packets = CALC_DELTA_64(last_stats->np_stats.rx_vport_multicast_packets,
            pre_stats->np_stats.rx_vport_multicast_packets);
        vport_stats->np_stats.tx_vport_multicast_packets = CALC_DELTA_64(last_stats->np_stats.tx_vport_multicast_packets,
            pre_stats->np_stats.tx_vport_multicast_packets);
        vport_stats->np_stats.rx_vport_multicast_bytes = CALC_DELTA_64(last_stats->np_stats.rx_vport_multicast_bytes,
            pre_stats->np_stats.rx_vport_multicast_bytes);
        vport_stats->np_stats.tx_vport_multicast_bytes = CALC_DELTA_64(last_stats->np_stats.tx_vport_multicast_bytes,
            pre_stats->np_stats.tx_vport_multicast_bytes);
        vport_stats->np_stats.rx_vport_broadcast_packets = CALC_DELTA_64(last_stats->np_stats.rx_vport_broadcast_packets,
            pre_stats->np_stats.rx_vport_broadcast_packets);
        vport_stats->np_stats.tx_vport_broadcast_packets = CALC_DELTA_64(last_stats->np_stats.tx_vport_broadcast_packets,
            pre_stats->np_stats.tx_vport_broadcast_packets);
        vport_stats->np_stats.rx_vport_broadcast_bytes = CALC_DELTA_64(last_stats->np_stats.rx_vport_broadcast_bytes,
            pre_stats->np_stats.rx_vport_broadcast_bytes);
        vport_stats->np_stats.tx_vport_broadcast_bytes = CALC_DELTA_64(last_stats->np_stats.tx_vport_broadcast_bytes,
            pre_stats->np_stats.tx_vport_broadcast_bytes);
        vport_stats->np_stats.rx_vport_mtu_drop_packets = CALC_DELTA_64(last_stats->np_stats.rx_vport_mtu_drop_packets,
            pre_stats->np_stats.rx_vport_mtu_drop_packets);
        vport_stats->np_stats.tx_vport_mtu_drop_packets = CALC_DELTA_64(last_stats->np_stats.tx_vport_mtu_drop_packets,
            pre_stats->np_stats.tx_vport_mtu_drop_packets);
        vport_stats->np_stats.rx_vport_mtu_drop_bytes = CALC_DELTA_64(last_stats->np_stats.rx_vport_mtu_drop_bytes,
            pre_stats->np_stats.rx_vport_mtu_drop_bytes);
        vport_stats->np_stats.tx_vport_mtu_drop_bytes = CALC_DELTA_64(last_stats->np_stats.tx_vport_mtu_drop_bytes,
            pre_stats->np_stats.tx_vport_mtu_drop_bytes);
        vport_stats->np_stats.rx_vport_plcr_drop_packets = CALC_DELTA_64(last_stats->np_stats.rx_vport_plcr_drop_packets,
            pre_stats->np_stats.rx_vport_plcr_drop_packets);
        vport_stats->np_stats.tx_vport_plcr_drop_packets = CALC_DELTA_64(last_stats->np_stats.tx_vport_plcr_drop_packets,
            pre_stats->np_stats.tx_vport_plcr_drop_packets);
        vport_stats->np_stats.rx_vport_plcr_drop_bytes = CALC_DELTA_64(last_stats->np_stats.rx_vport_plcr_drop_bytes,
            pre_stats->np_stats.rx_vport_plcr_drop_bytes);
        vport_stats->np_stats.tx_vport_plcr_drop_bytes = CALC_DELTA_64(last_stats->np_stats.tx_vport_plcr_drop_bytes,
            pre_stats->np_stats.tx_vport_plcr_drop_bytes);
        vport_stats->np_stats.tx_vport_ssvpc_packets = CALC_DELTA_64(last_stats->np_stats.tx_vport_ssvpc_packets,
            pre_stats->np_stats.tx_vport_ssvpc_packets);
        vport_stats->np_stats.rx_vport_fdir_hits_packets = CALC_DELTA_64(last_stats->np_stats.rx_vport_fdir_hits_packets,
            pre_stats->np_stats.rx_vport_fdir_hits_packets);
        vport_stats->np_stats.rx_vport_fdir_hits_bytes = CALC_DELTA_64(last_stats->np_stats.rx_vport_fdir_hits_bytes,
            pre_stats->np_stats.rx_vport_fdir_hits_bytes);
        vport_stats->np_stats.rx_vport_fdir_drop_packets = CALC_DELTA_64(last_stats->np_stats.rx_vport_fdir_drop_packets,
            pre_stats->np_stats.rx_vport_fdir_drop_packets);
        vport_stats->np_stats.rx_vport_fdir_drop_bytes = CALC_DELTA_64(last_stats->np_stats.rx_vport_fdir_drop_bytes,
            pre_stats->np_stats.rx_vport_fdir_drop_bytes);
    }

    kfree(cur_stats);
    spin_unlock_bh(&en_dev->vport_stats_lock);
    return err;
}

static inline bool is_zf_dev(struct zxdh_en_device *en_dev)
{
    /* bit[12:14]-ep_id(0~4) */
    if ((en_dev->pcie_id & BIT(14)) != 0)
        return true;
    else
        return false;
}

bool zxdh_en_is_panel_port(struct zxdh_en_device *en_dev)
{
    return en_dev->ops->is_panel_port(en_dev->parent);
}

int32_t get_mac_stats_from_msg(struct zxdh_en_device *en_dev)
{
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};
    int32_t ret = 0;

    if (en_dev == NULL)
        return -1;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;
    msg->payload.hdr_to_agt.op_code = AGENT_MAC_STATS_GET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;
    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "get_mac_stats_from_msg failed, ret:%d\n", ret);
        kfree(msg);
        return ret;
    }

    if (msg->reps.stats_msg.rx_total != UINT64_MAX)
        zte_memcpy_s(&(en_dev->hw_stats.phy_stats), &msg->reps.stats_msg, sizeof(en_dev->hw_stats.phy_stats));

    kfree(msg);

    return ret;
}

int32_t get_400g_mac_stats_from_reg(struct zxdh_en_device *en_dev)
{
    uint64_t virt_addr = 0;
    T_SPM_MAC_STATS spm_stats = {0};
    struct zxdh_en_phy_stats *phy_stats = &en_dev->hw_stats.phy_stats;

    if (en_dev == NULL)
        return -1;

    virt_addr = en_dev->ops->get_bar_virt_addr(en_dev->parent, 0);
    zte_memcpy_s(&spm_stats, (void __iomem*)(virt_addr + ZXDH_DEV_400G_MAC_STATS), sizeof(spm_stats));

    /* 规避自愈过程中bar全F导致错包统计异常的问题 */
    if ((spm_stats.rx_error == UINT64_MAX) && (spm_stats.tx_error == UINT64_MAX))
        return 0;

    phy_stats->rx_packets_phy = spm_stats.rx_total;
    phy_stats->rx_pause_phy = spm_stats.rx_pause;
    phy_stats->rx_unicast_phy = spm_stats.rx_unicast;
    phy_stats->rx_multicast_phy = spm_stats.rx_multicast;
    phy_stats->rx_broadcast_phy = spm_stats.rx_broadcast;
    phy_stats->rx_vlan_phy = spm_stats.rx_vlan;
    phy_stats->rx_size_64_phy = spm_stats.rx_size_64;
    phy_stats->rx_size_65_127 = spm_stats.rx_size_65_127;
    phy_stats->rx_size_128_255 = spm_stats.rx_size_128_255;
    phy_stats->rx_size_256_511 = spm_stats.rx_size_256_511;
    phy_stats->rx_size_512_1023 = spm_stats.rx_size_512_1023;
    phy_stats->rx_size_1024_1518 = spm_stats.rx_size_1024_1518;
    phy_stats->rx_size_1519_mru = spm_stats.rx_size_1519_mru;
    phy_stats->rx_under64_drop = spm_stats.rx_undersize;
    phy_stats->rx_undersize_phy = spm_stats.rx_undersize;
    phy_stats->rx_oversize_phy = spm_stats.rx_oversize;
    phy_stats->rx_fragment_phy = spm_stats.rx_fragment;
    phy_stats->rx_jabber_phy = spm_stats.rx_jabber;
    phy_stats->rx_mac_control_phy = spm_stats.rx_control;
    phy_stats->rx_eee_phy = spm_stats.rx_eee;
    phy_stats->rx_error_phy = spm_stats.rx_error;
    phy_stats->rx_crc_errors = spm_stats.rx_fcs_error;
    phy_stats->rx_drop_phy = spm_stats.rx_drop;
    phy_stats->rx_bytes_phy = spm_stats.rx_total_bytes;
    phy_stats->rx_good_bytes_phy = spm_stats.rx_good_bytes;

    phy_stats->tx_packets_phy = spm_stats.tx_total;
    phy_stats->tx_pause_phy = spm_stats.tx_pause;
    phy_stats->tx_unicast_phy = spm_stats.tx_unicast;
    phy_stats->tx_multicast_phy = spm_stats.tx_multicast;
    phy_stats->tx_broadcast_phy = spm_stats.tx_broadcast;
    phy_stats->tx_vlan_phy = spm_stats.tx_vlan;
    phy_stats->tx_size_64_phy = spm_stats.tx_size_64;
    phy_stats->tx_size_65_127 = spm_stats.tx_size_65_127;
    phy_stats->tx_size_128_255 = spm_stats.tx_size_128_255;
    phy_stats->tx_size_256_511 = spm_stats.tx_size_256_511;
    phy_stats->tx_size_512_1023 = spm_stats.tx_size_512_1023;
    phy_stats->tx_size_1024_1518 = spm_stats.tx_size_1024_1518;
    phy_stats->tx_size_1519_mtu = spm_stats.tx_size_1519_mtu;
    phy_stats->tx_undersize_phy = spm_stats.tx_undersize;
    phy_stats->tx_oversize_phy = spm_stats.tx_oversize;
    phy_stats->tx_fragment_phy = spm_stats.tx_fragment;
    phy_stats->tx_jabber_phy = spm_stats.tx_jabber;
    phy_stats->tx_mac_control_phy = spm_stats.tx_control;
    phy_stats->tx_eee_phy = spm_stats.tx_eee;
    phy_stats->tx_error_phy = spm_stats.tx_error;
    phy_stats->tx_crc_errors = spm_stats.tx_fcs_error;
    phy_stats->tx_drop_phy = spm_stats.tx_drop;
    phy_stats->tx_bytes_phy = spm_stats.tx_total_bytes;
    phy_stats->tx_good_bytes_phy = spm_stats.tx_good_bytes;

    return 0;
}

int32_t get_mac_stats_from_reg(struct zxdh_en_device *en_dev)
{
    uint64_t virt_addr = 0;
    uint64_t stats_addr = 0;
    uint64_t bytes_addr = 0;
    uint32_t cntp_offset = 0;
    uint64_t bar_size = 0;
    struct zxdh_en_spm_stats spm_stats;
    struct zxdh_en_spm_bytes spm_bytes;
    struct zxdh_en_phy_stats *phy_stats = &en_dev->hw_stats.phy_stats;
    uint16_t i = 0;
    uint16_t phyport_info;

    if (en_dev == NULL)
        return -1;

    virt_addr = en_dev->ops->get_bar_virt_addr(en_dev->parent, 0);
    bar_size = en_dev->ops->get_bar_size(en_dev->parent, 0);
    phyport_info = *(uint16_t *)(virt_addr + ZXDH_SPM_EXTRA_OFFSET);
    if ((phyport_info & (1 << en_dev->phy_port)) != 0) {
        cntp_offset = *(uint32_t *)(virt_addr + ZXDH_SPM_EXTRA_CNTP_OFFSET + (en_dev->phy_port / 4) * 4);
    } else {
        cntp_offset = ZXDH_SPM_NORMAL_CNTP_OFFSET;
    }

    if ((bar_size <= ZXDH_SPM_CNTP_SIZE) || (cntp_offset > (bar_size - ZXDH_SPM_CNTP_SIZE))) {
        LOG_DEBUG_DEV(en_dev->parent, "bar_size:0x%llx cntp_offset:0x%x is invalid\n", bar_size, cntp_offset);
        return 0;
    }

    stats_addr = cntp_offset + ZXDH_SPM_STATS_OFFSET;
    bytes_addr = cntp_offset + ZXDH_SPM_BYTES_OFFSET;

    switch (en_dev->curr_speed_modes)
    {
        case BIT(SPM_SPEED_1X_1G):
        case BIT(SPM_SPEED_1X_10G):
        case BIT(SPM_SPEED_1X_25G):
        case BIT(SPM_SPEED_1X_50G):
        {
            stats_addr += (en_dev->phy_port % 4) * sizeof(struct zxdh_en_spm_stats);
            bytes_addr += (en_dev->phy_port % 4) * sizeof(struct zxdh_en_spm_bytes);
            break;
        }
        case BIT(SPM_SPEED_2X_100G):
        case BIT(SPM_SPEED_4X_40G):
        case BIT(SPM_SPEED_4X_100G):
        case BIT(SPM_SPEED_4X_200G):
        {
            stats_addr += (4 + (en_dev->phy_port % 4) / 2) * sizeof(struct zxdh_en_spm_stats);
            bytes_addr += (4 + (en_dev->phy_port % 4) / 2) * sizeof(struct zxdh_en_spm_bytes);
            break;
        }
        default:
        {
            return 0;
        }
    }

    if (is_zf_dev(en_dev))
    {
        for (i = 0; i < sizeof(struct zxdh_en_spm_stats); i++)
        {
            *((uint8_t *)&spm_stats + i) = *(uint8_t *)(virt_addr + TO_ZF_ADDR(stats_addr + i));
        }
        for (i = 0; i < sizeof(struct zxdh_en_spm_bytes); i++)
        {
            *((uint8_t *)&spm_bytes + i) = *(uint8_t *)(virt_addr + TO_ZF_ADDR(bytes_addr + i));
        }
    }
    else
    {
        zte_memcpy_s(&spm_stats, (void *)(virt_addr + stats_addr), sizeof(struct zxdh_en_spm_stats));
        zte_memcpy_s(&spm_bytes, (void *)(virt_addr + bytes_addr), sizeof(struct zxdh_en_spm_bytes));
    }

    /* 规避自愈过程中bar全F导致错包统计异常的问题 */
    if ((spm_stats.rx_error == UINT64_MAX) || (spm_stats.tx_error == UINT64_MAX))
        return 0;

    phy_stats->rx_packets_phy = spm_stats.rx_total;
    phy_stats->tx_packets_phy = spm_stats.tx_total;
    phy_stats->rx_bytes_phy = spm_bytes.rx_total_bytes;
    phy_stats->tx_bytes_phy = spm_bytes.tx_total_bytes;
    phy_stats->rx_error_phy = spm_stats.rx_error;
    phy_stats->tx_error_phy = spm_stats.tx_error;
    phy_stats->rx_drop_phy = spm_stats.rx_drop;
    phy_stats->tx_drop_phy = spm_stats.tx_drop;
    phy_stats->rx_good_bytes_phy = spm_bytes.rx_good_bytes;
    phy_stats->tx_good_bytes_phy = spm_bytes.tx_good_bytes;
    phy_stats->rx_unicast_phy = spm_stats.rx_unicast;
    phy_stats->tx_unicast_phy = spm_stats.tx_unicast;
    phy_stats->rx_multicast_phy = spm_stats.rx_multicast;
    phy_stats->tx_multicast_phy = spm_stats.tx_multicast;
    phy_stats->rx_broadcast_phy = spm_stats.rx_broadcast;
    phy_stats->tx_broadcast_phy = spm_stats.tx_broadcast;
    phy_stats->rx_under64_drop = spm_stats.rx_undersize;
    phy_stats->rx_undersize_phy = spm_stats.rx_undersize;
    phy_stats->rx_size_64_phy = spm_stats.rx_size_64;
    phy_stats->rx_size_65_127 = spm_stats.rx_size_65_127;
    phy_stats->rx_size_128_255 = spm_stats.rx_size_128_255;
    phy_stats->rx_size_256_511 = spm_stats.rx_size_256_511;
    phy_stats->rx_size_512_1023 = spm_stats.rx_size_512_1023;
    phy_stats->rx_size_1024_1518 = spm_stats.rx_size_1024_1518;
    phy_stats->rx_size_1519_mru = spm_stats.rx_size_1519_mru;
    phy_stats->rx_oversize_phy = spm_stats.rx_oversize;
    phy_stats->tx_undersize_phy = spm_stats.tx_undersize;
    phy_stats->tx_size_64_phy = spm_stats.tx_size_64;
    phy_stats->tx_size_65_127 = spm_stats.tx_size_65_127;
    phy_stats->tx_size_128_255 = spm_stats.tx_size_128_255;
    phy_stats->tx_size_256_511 = spm_stats.tx_size_256_511;
    phy_stats->tx_size_512_1023 = spm_stats.tx_size_512_1023;
    phy_stats->tx_size_1024_1518 = spm_stats.tx_size_1024_1518;
    phy_stats->tx_size_1519_mtu = spm_stats.tx_size_1519_mtu;
    phy_stats->tx_oversize_phy = spm_stats.tx_oversize;
    phy_stats->rx_pause_phy = spm_stats.rx_pause;
    phy_stats->tx_pause_phy = spm_stats.tx_pause;
    phy_stats->rx_crc_errors = spm_stats.rx_fcs_error;
    phy_stats->tx_crc_errors = spm_stats.tx_fcs_error;
    phy_stats->rx_mac_control_phy = spm_stats.rx_control;
    phy_stats->tx_mac_control_phy = spm_stats.tx_control;
    phy_stats->rx_fragment_phy = spm_stats.rx_fragment;
    phy_stats->tx_fragment_phy = spm_stats.tx_fragment;
    phy_stats->rx_jabber_phy = spm_stats.rx_jabber;
    phy_stats->tx_jabber_phy = spm_stats.tx_jabber;
    phy_stats->rx_vlan_phy = spm_stats.rx_vlan;
    phy_stats->tx_vlan_phy = spm_stats.tx_vlan;
    phy_stats->rx_eee_phy = spm_stats.rx_eee;
    phy_stats->tx_eee_phy = spm_stats.tx_eee;

    return 0;
}

int32_t get_400g_mac_stats(struct zxdh_en_device *en_dev)
{
    if (!en_dev->ops->is_fw_feature_support(en_dev->parent, FW_FEATURE_400G_MAC_STATS)) {
        return get_mac_stats_from_msg(en_dev);
    }

    return get_400g_mac_stats_from_reg(en_dev);
}

int32_t zxdh_mac_stats_get(struct zxdh_en_device *en_dev)
{
    if ((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) ||
        (en_dev->phy_port > ZXDH_PHY_PORT_MAX))
        return 0;

    if (en_dev->board_type == DH_STD_E318)
        return get_400g_mac_stats(en_dev);
    else
        return get_mac_stats_from_reg(en_dev);

}

int32_t zxdh_mac_stats_clear(struct zxdh_en_device *en_dev)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    if (!zxdh_en_is_panel_port(en_dev))
        return err;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_MAC_STATS_CLEAR;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_mac_stats_clear failed, err: %d\n", err);
        kfree(msg);
        return err;
    }
    kfree(msg);
    return err;
}

int32_t zxdh_en_phyport_init(struct zxdh_en_device *en_dev)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct link_info_struct link_info_val = {0};
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_MAC_PHYPORT_INIT;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;
    if (en_dev->ops->is_upf(en_dev->parent))
    {
        msg->payload.hdr_to_agt.phyport = 0;
        msg->payload.hdr_to_agt.is_upf = 1;
        en_dev->link_up = FALSE;
        en_dev->speed = SPEED_UNKNOWN;
        en_dev->ops->set_pf_link_up(en_dev->parent, en_dev->link_up);
        netif_carrier_off(en_dev->netdev);
        LOG_INFO_DEV(en_dev->parent, "upf link down init\n");
        kfree(msg);
        return err;
    }
    else if ((en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_NE0) ||
             (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_NE1) ||
             (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_SRIOV))
    {
        msg->payload.hdr_to_agt.phyport = 0;
        msg->payload.hdr_to_agt.is_upf = 1;
        //TODO 确定VGCF设备link状态获取方案
        en_dev->link_up = true;
        en_dev->speed = SPEED_100000;
        if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_SRIOV)
        {
            en_dev->speed = SPEED_200000;
        }
        en_dev->ops->set_pf_link_up(en_dev->parent, en_dev->link_up);
        netif_carrier_on(en_dev->netdev);
    }

    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA)
    {
        msg->payload.hdr_to_agt.phyport = 0;
    }

    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_riscv_mac failed, err: %d\n", err);
        kfree(msg);
        return err;
    }

    en_dev->supported_speed_modes = msg->reps.mac_set_msg.speed_modes;
    en_dev->advertising_speed_modes = msg->reps.mac_set_msg.speed_modes;

    link_info_val.speed = en_dev->speed;
    link_info_val.autoneg_enable = en_dev->autoneg_enable;
    link_info_val.supported_speed_modes = en_dev->supported_speed_modes;
    link_info_val.advertising_speed_modes = en_dev->advertising_speed_modes;
    link_info_val.duplex = en_dev->duplex;
    en_dev->ops->update_pf_link_info(en_dev->parent, &link_info_val);

    kfree(msg);
    return err;
}

int32_t zxdh_en_autoneg_set(struct zxdh_en_device *en_dev, uint8_t enable, uint32_t speed_modes)
{
    int32_t err = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt  = BAR_MSG_RETRY_CNT_MAX;

    if (!zxdh_en_is_panel_port(en_dev))
        return err;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_MAC_AUTONEG_SET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;
    msg->payload.mac_set_msg.autoneg = enable;
    msg->payload.mac_set_msg.speed_modes = speed_modes;

    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_riscv_mac failed, err: %d\n", err);
    }
    kfree(msg);
    return err;
}

int32_t zxdh_en_fec_mode_set(struct zxdh_en_device *en_dev, uint32_t fec_cfg)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    if (!zxdh_en_is_panel_port(en_dev))
        return ret;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_MAC_FEC_MODE_SET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;
    msg->payload.mac_fec_mode_msg.fec_cfg = fec_cfg;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    kfree(msg);
    return ret;
}

int32_t zxdh_en_fec_mode_get(struct zxdh_en_device *en_dev, uint32_t *fec_cap, uint32_t *fec_cfg, uint32_t *fec_active)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = 0;

    if (!zxdh_en_is_panel_port(en_dev))
        return err;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_MAC_FEC_MODE_GET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;

    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_riscv_mac failed, err: %d\n", err);
        kfree(msg);
        return err;
    }

    if(fec_cap)
        *fec_cap = msg->reps.mac_fec_mode_msg.fec_cap;
    if(fec_cfg)
        *fec_cfg = msg->reps.mac_fec_mode_msg.fec_cfg;
    if(fec_active)
        *fec_active = msg->reps.mac_fec_mode_msg.fec_link;

    kfree(msg);
    return err;
}

int32_t zxdh_en_fc_mode_set(struct zxdh_en_device *en_dev, uint32_t fc_mode)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    if (!zxdh_en_is_panel_port(en_dev))
        return ret;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_MAC_FC_MODE_SET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;
    msg->payload.mac_fc_mode_msg.fc_mode = fc_mode;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    kfree(msg);
    return ret;
}

int32_t zxdh_en_fc_mode_get(struct zxdh_en_device *en_dev, uint32_t *fc_mode)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = 0;

    if (!zxdh_en_is_panel_port(en_dev))
        return err;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_MAC_FC_MODE_GET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;

    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_riscv_mac failed, err: %d\n", err);
        kfree(msg);
        return err;
    }

    if(fc_mode)
        *fc_mode = msg->reps.mac_fc_mode_msg.fc_mode;

    kfree(msg);
    return err;
}

uint32_t zxdh_en_module_eeprom_read(struct zxdh_en_device *en_dev, struct zxdh_en_module_eeprom_param *query, uint8_t *data)
{
    union zxdh_msg *msg = NULL;
    uint8_t length = 0;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    if (!zxdh_en_is_panel_port(en_dev))
        return err;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_MAC_MODULE_EEPROM_READ;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;
    msg->payload.module_eeprom_msg.i2c_addr = query->i2c_addr;
    msg->payload.module_eeprom_msg.bank = query->bank;
    msg->payload.module_eeprom_msg.page = query->page;
    msg->payload.module_eeprom_msg.offset = query->offset;
    msg->payload.module_eeprom_msg.length = query->length;

    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_riscv_mac failed, err: %d\n", err);
        kfree(msg);
        return 0;
    }

    if(data)
        memcpy(data, msg->reps.module_eeprom_msg.data, msg->reps.module_eeprom_msg.length);

    length = msg->reps.module_eeprom_msg.length;
    kfree(msg);
    return length;
}

int32_t zxdh_vf_1588_call_np_interface(struct zxdh_en_device *en_dev)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_VF_1588_CALL_NP;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.vf_1588_call_np.vfid = VQM_VFID(msg->payload.hdr.vport);
    msg->payload.vf_1588_call_np.call_np_interface_num = en_dev->vf_1588_call_np_num;
    msg->payload.vf_1588_call_np.ptp_tc_enable_opt = en_dev->ptp_tc_enable_opt;
    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if(ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_pf failed: %d\n", ret);
        kfree(msg);
        return ret;
    }

    kfree(msg);
    return ret;
}

int32_t zxdh_vf_port_create(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    uint8_t link_up = 0;
    bool is_upf = false;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    if (!zxdh_en_is_panel_port(en_dev))
    {
        is_upf = true;
    }

    msg->payload.hdr.op_code = ZXDH_VF_PORT_INIT;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.vf_init_msg.base_qid = en_dev->phy_index[0];
    msg->payload.vf_init_msg.hash_search_idx = en_dev->hash_search_idx;
    msg->payload.vf_init_msg.rss_enable = 1;
    msg->payload.vf_init_msg.is_upf = is_upf;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if(ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_pf failed: %d\n", ret);
        kfree(msg);
        return ret;
    }

    if (!is_upf ||
        en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA)
    {
        en_dev->ops->get_link_info_from_vqm(en_dev->parent, &link_up);
        en_dev->link_up = link_up;
        LOG_DEBUG_DEV(en_dev->parent, "vf read link_up: %d\n", link_up);
    }
    else
    {
        en_dev->link_up = msg->reps.vf_init_msg.link_up;
    }

    zxdh_netdev_addr_set(en_dev->netdev, msg->reps.vf_init_msg.mac_addr);
    ether_addr_copy(en_dev->last_np_mac_addr.sa_data, en_dev->netdev->dev_addr);
    en_dev->netdev->addr_assign_type =msg->reps.vf_init_msg.addr_assign_type;
    en_dev->autoneg_enable = msg->reps.vf_init_msg.autoneg_enable;
    en_dev->supported_speed_modes = msg->reps.vf_init_msg.sup_link_modes;
    en_dev->advertising_speed_modes = msg->reps.vf_init_msg.adv_link_modes;
    en_dev->vlan_dev.vlan_id = msg->reps.vf_init_msg.vlan_id;
    en_dev->vlan_dev.qos = msg->reps.vf_init_msg.vlan_qos;
    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA) {
        en_dev->duplex = DUPLEX_FULL;
        en_dev->speed = SPEED_200000;
    } else {
        en_dev->duplex = msg->reps.vf_init_msg.duplex;
        en_dev->speed = msg->reps.vf_init_msg.speed;
    }

    if (!is_upf)
    {
        en_dev->phy_port = msg->reps.vf_init_msg.phy_port;
        en_dev->ops->set_pf_phy_port(en_dev->parent, en_dev->phy_port);
    }

    if (en_dev->link_up)
    {
        en_dev->ops->set_pf_link_up(en_dev->parent, TRUE);
        netif_carrier_on(en_dev->netdev);
    }
    else
    {
        en_dev->ops->set_pf_link_up(en_dev->parent, FALSE);
        netif_carrier_off(en_dev->netdev);
    }

    en_dev->ops->set_rdma_speed(en_dev->parent, en_dev->speed);

    kfree(msg);
    return ret;
}

int32_t zxdh_vf_port_delete(struct zxdh_en_device *en_dev)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    //dpp_np_uninit
    msg->payload.hdr.op_code = ZXDH_VF_PORT_UNINIT;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    kfree(msg);
    return ret;
}

#ifdef VF_STATS_UPDATE
int32_t zxdh_vf_item_init_stats_update(struct zxdh_en_device *en_dev)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    uint32_t vf_id = GET_VFID(en_dev->vport);
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = 0;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_GET_NP_STATS;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.vf_id = vf_id;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.np_stats_get_msg.clear_mode = NP_GET_PKT_CNT;
    msg->payload.np_stats_get_msg.is_init_get = true;
    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if(ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_pf failed: %d\n", ret);
    }

    kfree(msg);
    return ret;
}
#endif

int32_t zxdh_vf_dpp_add_mac(struct zxdh_en_device *en_dev, const uint8_t *dev_addr, uint8_t filter_flag)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_MAC_ADD;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.mac_addr_set_msg.filter_flag = filter_flag;
    memcpy(msg->payload.mac_addr_set_msg.mac_addr, dev_addr, en_dev->netdev->addr_len);

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0)
    {
        if (msg->reps.vf_mac_set_msg.mac_err_flag == ZXDH_REPS_BEYOND_MAC)
        {
            kfree(msg);
            return ZXDH_REPS_BEYOND_MAC;
        }
        else if (msg->reps.vf_mac_set_msg.mac_err_flag == ZXDH_REPS_EXIST_MAC)
        {
            kfree(msg);
            return ZXDH_REPS_EXIST_MAC;
        }
    }
    kfree(msg);
    return ret;
}

int32_t zxdh_vf_dpp_dump_mac(struct zxdh_en_device *en_dev, const uint8_t *dev_addr)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_MAC_DUMP;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    memcpy(msg->payload.mac_addr_set_msg.mac_addr, dev_addr, en_dev->netdev->addr_len);

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "en_dev->ops->msg_send_cmd failed, ret = %d\n", ret);
        kfree(msg);
        return ZXDH_REPS_EXIST_MAC;
    }

    kfree(msg);
    return ret;
}

int32_t zxdh_vf_dpp_del_mac(struct zxdh_en_device *en_dev, const uint8_t *dev_addr, uint8_t filter_flag, bool mac_flag)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_MAC_DEL;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.mac_addr_set_msg.filter_flag = filter_flag;
    msg->payload.mac_addr_set_msg.mac_flag = mac_flag;
    memcpy(msg->payload.mac_addr_set_msg.mac_addr, dev_addr, en_dev->netdev->addr_len);

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "en_dev->ops->msg_send_cmd failed, ret = %d\n", ret);
    }
    kfree(msg);

    return ret;
}

int32_t zxdh_vf_rss_en_set(struct zxdh_en_device *en_dev, uint32_t enable)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_RSS_EN_SET;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.rss_enable_msg.rss_enable = enable;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "en_dev->ops->msg_send_cmd failed, ret = %d\n", ret);
    }
    kfree(msg);

    return ret;
}

int32_t zxdh_vf_dpp_add_ipv6_mac(struct zxdh_en_device *en_dev, const uint8_t *mac_addr)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_IPV6_MAC_ADD;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    memcpy(msg->payload.mac_addr_set_msg.mac_addr, mac_addr, en_dev->netdev->addr_len);
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if ((err != 0) && (msg->reps.vf_mac_set_msg.mac_err_flag == ZXDH_REPS_BEYOND_MAC))
    {
        LOG_ERR_DEV(en_dev->parent, "Add Multicast MAC Address(%pM) Failed, Beyond Max MAC Num in the whole transfer area\n", mac_addr);
    }
    kfree(msg);

    return err;
}

int32_t zxdh_vf_dpp_del_ipv6_mac(struct zxdh_en_device *en_dev, const uint8_t *mac_addr)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_IPV6_MAC_DEL;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    memcpy(msg->payload.mac_addr_set_msg.mac_addr, mac_addr, en_dev->netdev->addr_len);

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "en_dev->ops->msg_send_cmd failed, ret = %d\n", ret);
    }
    kfree(msg);

    return ret;
}

int32_t zxdh_vf_dpp_add_lacp_mac(struct zxdh_en_device *en_dev, const uint8_t *mac_addr)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_LACP_MAC_ADD;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    memcpy(msg->payload.mac_addr_set_msg.mac_addr, mac_addr, en_dev->netdev->addr_len);
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "Add LACP Multicast MAC Address(%pM) Failed\n", mac_addr);
    }
    kfree(msg);

    return err;
}

int32_t zxdh_vf_dpp_del_lacp_mac(struct zxdh_en_device *en_dev, const uint8_t *mac_addr)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_LACP_MAC_DEL;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    memcpy(msg->payload.mac_addr_set_msg.mac_addr, mac_addr, en_dev->netdev->addr_len);

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "DEL LACP Multicast MAC Address(%pM) Failed\n", mac_addr);
    }
    kfree(msg);

    return ret;
}

int32_t zxdh_vf_egr_port_attr_set(struct zxdh_en_device *en_dev, uint32_t mode, uint32_t value, uint8_t fow)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_PORT_ATTRS_SET;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.port_attr_set_msg.mode = mode;
    msg->payload.port_attr_set_msg.value = value;
    msg->payload.port_attr_set_msg.allmulti_follow = fow;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "en_dev->ops->msg_send_cmd failed, ret = %d\n", ret);
    }
    kfree(msg);

    return ret;
}

int32_t zxdh_vf_egr_port_attr_get(struct zxdh_en_device *en_dev, ZXDH_SRIOV_VPORT_T *port_attr_entry)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_PORT_ATTRS_GET;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "en_dev->ops->msg_send_cmd failed, ret = %d\n", ret);
        kfree(msg);
        return ret;
    }

    memcpy(port_attr_entry, &msg->reps.port_attr_get_msg.port_attr_entry, sizeof(ZXDH_SRIOV_VPORT_T));
    kfree(msg);

    return ret;
}

int32_t zxdh_vf_port_promisc_set(struct zxdh_en_device *en_dev, uint8_t mode, uint8_t value, uint8_t fow)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt =  BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_PROMISC_SET;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.promisc_set_msg.mode = mode;
    msg->payload.promisc_set_msg.value = value;
    msg->payload.promisc_set_msg.mc_follow = fow;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "en_dev->ops->msg_send_cmd failed, ret = %d\n", ret);
    }

    kfree(msg);

    return ret;
}

int32_t zxdh_get_vf_err_stats(struct zxdh_en_device *en_dev, zxdh_get_sw_stats *payload, zxdh_sw_stats_reply *reply)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = 0;

    /* 判断vf是否probe */
    if (!en_dev->ops->get_vf_is_probe(en_dev->parent, payload->vf_idx))
    {
        LOG_ERR_DEV(en_dev->parent, "vf(%u) is not probed\n",  payload->vf_idx);
        return VF_ERR;
    }

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return GET_STAT_FAILED;
    }

    /* 发送消息填写 */
    msg->payload.hdr_vf.op_code = ZXDH_GET_SW_STATS;
    msg->payload.hdr_vf.dst_pcie_id = FIND_VF_PCIE_ID(en_dev->pcie_id, payload->vf_idx);
    memcpy(&msg->payload.vf_sw_stats, payload, sizeof(zxdh_get_sw_stats));

    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_PF_BAR_MSG_TO_VF, msg, msg, &para);
    if (err != 0)
    {
        if (err == ZXDH_INVALID_OP_CODE)
        {
            LOG_ERR_DEV(en_dev->parent, "vf is used by kernel driver, action is not supported!!!\n");
            kfree(msg);
            return ACTION_IS_NOT_SUPPORTED;
        }
        LOG_ERR_DEV(en_dev->parent, "failed to get VF[%d] err stats:%d\n", payload->vf_idx, err);
        kfree(msg);
        return GET_STAT_FAILED;
    }
    memcpy(reply, &msg->reps.vf_sw_stats_rsp, sizeof(zxdh_sw_stats_reply));
    kfree(msg);
    return GET_STAT_SUCCESS;
}
EXPORT_SYMBOL(zxdh_get_vf_err_stats);

int32_t zxdh_cfg_misx_mode(struct zxdh_en_device *en_dev, uint32_t intr_adaptataion_flag)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->vqm_msg.opcode = OPCODE_SET;
    msg->vqm_msg.cmd = VQM_MSIX_ADAPTIVE_CFG_CMD;
    msg->vqm_msg.msix_mode_sel.intr_adaptataion_flag = intr_adaptataion_flag;
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_CFG_VQM, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "send cfg msix mode msg to riscv failed\n");
    }
    kfree(msg);
    return err;
}

int32_t zxdh_get_misx_mode(struct zxdh_en_device *en_dev, uint32_t *intr_adaptataion_flag)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->vqm_msg.opcode = OPCODE_GET;
    msg->vqm_msg.cmd =  VQM_MSIX_ADAPTIVE_CFG_CMD;
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_CFG_VQM, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "send cfg msix mode msg to riscv failed\n");
        kfree(msg);
        return 1;
    }

    *intr_adaptataion_flag = msg->vqm_reps.msix_mode_sel.intr_adaptataion_flag;

    kfree(msg);
    return 0;
}

int32_t zxdh_cfg_coalesce_usecs(struct zxdh_en_device *en_dev, uint32_t rx_coalesce_usecs, uint32_t tx_coalesce_usecs)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->vqm_msg.opcode = OPCODE_SET;
    msg->vqm_msg.cmd = COALESCE_USECS_CMD;
    msg->vqm_msg.wr_used_t.rx_used_ring_t = rx_coalesce_usecs;
    msg->vqm_msg.wr_used_t.tx_used_ring_t = tx_coalesce_usecs;
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_CFG_VQM, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "send cfg msix mode msg to riscv failed\n");
    }
    kfree(msg);
    return err;
}

int32_t zxdh_get_coalesce_usecs(struct zxdh_en_device *en_dev, uint32_t *rx_coalesce_usecs, uint32_t *tx_coalesce_usecs)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->vqm_msg.opcode = OPCODE_GET;
    msg->vqm_msg.cmd = COALESCE_USECS_CMD;
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_CFG_VQM, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "send get_coalesce_usecs msg to riscv failed\n");
        kfree(msg);
        return 1;
    }

    *rx_coalesce_usecs = msg->vqm_reps.wr_used_t.rx_used_ring_t;
    *tx_coalesce_usecs = msg->vqm_reps.wr_used_t.tx_used_ring_t;
    kfree(msg);
    return 0;
}

int32_t zxdh_get_coalesce_params(struct zxdh_en_device *en_dev, uint32_t *rx_coalesce_usecs,
                                uint32_t *tx_coalesce_usecs, uint16_t *rx_coalesced_frames,
                                uint16_t *tx_coalesced_frames)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR("kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->vqm_msg.opcode = OPCODE_GET;
    msg->vqm_msg.cmd = COALESCE_PARAMS_CMD;
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_CFG_VQM, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR("send get_coalesce_params msg to riscv failed\n");
        kfree(msg);
        return 1;
    }

    *rx_coalesce_usecs = msg->vqm_reps.coalesce_data.rx_coalesce_usecs;
    *tx_coalesce_usecs = msg->vqm_reps.coalesce_data.tx_coalesce_usecs;
    *rx_coalesced_frames = msg->vqm_reps.coalesce_data.rx_coalesced_frames;
    *tx_coalesced_frames = msg->vqm_reps.coalesce_data.tx_coalesced_frames;

    LOG_DEBUG("get coalesce params from new interface\n");
    kfree(msg);
    return 0;
}

int32_t zxdh_en_vport_create(struct zxdh_en_device *en_dev)
{
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;
    if (!en_dev->ops->if_init(en_dev->parent))
    {
        return 0;
    }

    return dpp_vport_create(&pf_info);
}

int32_t zxdh_en_vport_delete(struct zxdh_en_device *en_dev)
{
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;
    if (!en_dev->ops->if_init(en_dev->parent))
    {
        return 0;
    }

    return dpp_vport_delete(&pf_info);
}

int32_t zxdh_pf_vport_create(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};
    uint32_t lag_id = 0; /* 默认为0 */

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    ret = zxdh_en_vport_create(en_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_vport_create failed: %d\n", ret);
        return ret;
    }

    ret = dpp_vport_bond_pf(&pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vport_bond_pf failed: %d\n", ret);
        goto err_vport;
    }

    if (!zxdh_en_is_panel_port(en_dev))
    {
        if ((en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_NE1))
            lag_id = 1;

        ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_LAG_ID, lag_id);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_set lag_id 0 failed: %d\n", ret);
            goto err_vport;
        }

        ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_LAG_EN_OFF, 1);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_set bond_en 1 failed: %d\n", ret);
            goto err_vport;
        }
    }
    else
    {
        ret = dpp_uplink_phy_bond_vport(&pf_info, en_dev->phy_port);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_uplink_phy_bond_vport failed: %d\n", ret);
            goto err_vport;
        }
    }

    return ret;

err_vport:
    zxdh_en_vport_delete(en_dev);
    return ret;
}

int32_t zxdh_rxfh_set(struct zxdh_en_device *en_dev, uint32_t *queue_map)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -1;
    }


    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (queue_map == NULL)
    {
        kfree(msg);
        return -1;
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        err = dpp_rxfh_set(&pf_info, queue_map, ZXDH_INDIR_RQT_SIZE);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_rxfh_set failed: %d\n", err);
        }
    }
    else
    {
        msg->payload.hdr.op_code = ZXDH_RXFH_SET;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        memcpy(msg->payload.rxfh_set_msg.queue_map, queue_map, ZXDH_INDIR_RQT_SIZE * sizeof(uint32_t));
        err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        if(err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_pf_np failed: %d\n", err);
        }
    }

    kfree(msg);
    return err;
}

void zxdh_rxfh_del(struct zxdh_en_device *en_dev)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};
    struct zxdh_bar_extra_para para = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;
    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    if (en_dev->quick_remove)
        return;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return;
    }

    if (en_dev->ops->is_bond(en_dev->parent))
    {
        kfree(msg);
        return;
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        dpp_rxfh_del(&pf_info);
    }
    else
    {
        msg->payload.hdr.op_code = ZXDH_RXFH_DEL;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        if(err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_pf_np failed: %d\n", err);
        }
    }
    kfree(msg);
}

int32_t zxdh_ethtool_init(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    ret = dpp_vport_hash_funcs_set(&pf_info, en_dev->eth_config.hash_func);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vport_hash_funcs_set failed: %d\n", ret);
        return ret;
    }

    ret = dpp_vport_rx_flow_hash_set(&pf_info, en_dev->eth_config.hash_mode);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_rx_flow_hash_set failed: %d\n", ret);
        return ret;
    }

    ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_PORT_BASE_QID, (uint16_t)en_dev->phy_index[0]);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_set %d failed: %d\n", en_dev->phy_index[0], ret);
        return ret;
    }

    ret = dpp_vqm_vfid_vlan_init(&pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vqm_vfid_vlan_init failed: %d\n", ret);
        return ret;
    }

    ret = dpp_vlan_filter_init(&pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vlan_filter_init failed: %d\n", ret);
        return ret;
    }

    ret = dpp_add_vlan_filter(&pf_info, 0);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_add_vlan_filter 0 failed: %d\n", ret);
       return ret;
    }

    return ret;
}

int32_t zxdh_pf_flush_mac(struct zxdh_en_device *en_dev)
{
    int32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    /* 删除此转发域所有单播mac地址 */
    err = dpp_unicast_all_mac_delete(&pf_info);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_unicast_all_mac_delete failed\n");
        return err;
}
    LOG_DEBUG_DEV(en_dev->parent, "dpp_unicast_all_mac_delete succeed\n");

    /* 删除此转发域中所有组播mac地址 */
    err = dpp_multicast_all_mac_delete(&pf_info);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_multicast_all_mac_delete failed\n");
        return err;
    }
    LOG_DEBUG_DEV(en_dev->parent, "dpp_multicast_all_mac_delete succeed\n");

    return err;
}

int32_t zxdh_pf_flush_mac_online(struct zxdh_en_device *en_dev)
{
    int32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    /* 删除此转发域所有单播mac地址 */
    err = dpp_unicast_all_mac_online_delete(&pf_info);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_unicast_all_mac_online_delete failed:%d\n", err);
        return err;
    }

    /* 删除此转发域中所有组播mac地址 */
    err = dpp_multicast_all_mac_online_delete(&pf_info);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_multicast_all_mac_online_delete failed:%d\n", err);
        return err;
    }

    return err;
}

int32_t zxdh_pf_port_delete(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};

    if (en_dev == NULL)
    {
        return -1;
    }

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;
    dpp_vport_uc_promisc_set(&pf_info, 0);
    dpp_vport_mc_promisc_set(&pf_info, 0);
    zxdh_dual_tor_switch(en_dev, 0);

    /* pf删除所有配置到np的mac地址 */
    if (!en_dev->ops->is_bond(en_dev->parent))
    {
        ret = zxdh_pf_flush_mac_online(en_dev);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_pf_flush_mac_online failed: %d\n", ret);
            return ret;
        }
    }
    ret = dpp_fd_acl_all_delete(&pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_fd_acl_all_delete failed: %d\n", ret);
        return ret;
    }

    ret = dpp_acl_entry_flush_by_vport(&pf_info, ZXDH_SDT_CSIG_TAG_TABLE);
    if (ret != 0)
    {
        LOG_ERR("dpp_acl_entry_flush_by_vport failed: %d\n", ret);
        return -1;
    }

    ret = zxdh_en_vport_delete(en_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vport_delete failed: %d\n", ret);
        return ret;
    }

    return ret;
}

int32_t zxdh_aux_alloc_pannel(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    struct zxdh_pannle_port port;

    ret = en_dev->ops->request_port(en_dev->parent, &port);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_aux_alloc_pannel failed \n");
        goto out;
    }

    en_dev->phy_port = port.phyport;
    en_dev->pannel_id = port.pannel_id;
    en_dev->link_check_bit = port.link_check_bit;

    LOG_DEBUG_DEV(en_dev->parent, "bond pf: pannel %u, phyport %u check bit %u \n",
        en_dev->pannel_id, en_dev->phy_port, en_dev->link_check_bit);

out:
    return ret;
}

int32_t zxdh_vf_set_rx_num(struct zxdh_en_device *en_dev, uint16_t rx_num)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};
    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;
    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    msg->payload.hdr.op_code = ZXDH_VF_RX_NUM_SET;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.vf_rx_num_msg.rx_num = rx_num;
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "Rx_num_set:zxdh_send_command_to_pf failed: %d\n", err);
    }
    kfree(msg);
    return err;
}

int32_t zxdh_vf_fd_en_set(struct zxdh_en_device *en_dev, uint32_t enable)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_FD_EN_SET;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.vf_fd_enable_msg.fd_enable = enable;

    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "Fd_set:zxdh_send_command_to_pf_np to set fd failed: %d\n", err);
    }

    kfree(msg);
    return err;
}

int32_t zxdh_vf_add_fd(struct zxdh_en_device *en_dev, struct ethtool_rx_flow_spec *fs,
                            uint32_t *index)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -1;
    }
    msg->payload.hdr.op_code = ZXDH_FD_ADD;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    zte_memcpy_s(&msg->payload.vf_fd_cfg_msg.fs, fs, sizeof(*fs));

    msg->payload.vf_fd_cfg_msg.index = DEFAULT_ADD_INDEX;
    if (en_dev->fs.ethtool_fs[fs->location].is_used)
        msg->payload.vf_fd_cfg_msg.index = en_dev->fs.ethtool_fs[fs->location].index;

    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (err != 0) {
        LOG_ERR_DEV(en_dev->parent, "Add_fd:zxdh_send_command_to_pf_np failed: %d\n", err);
        kfree(msg);
        return err;
    }
    /* 非替换操作，更新index */
    *index = msg->reps.fd_cfg_resp.index;
    kfree(msg);
    return err;
}

int32_t zxdh_vf_get_fd(struct zxdh_en_device *en_dev, uint32_t index)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -1;
    }
    msg->payload.hdr.op_code = ZXDH_FD_GET;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;

    msg->payload.vf_fd_cfg_msg.index = index;

    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (err != 0) {
        LOG_ERR_DEV(en_dev->parent, "Get_fd:zxdh_send_command_to_pf_np failed: %d\n", err);
    }
    kfree(msg);
    return err;
}

int32_t zxdh_vf_del_fd(struct zxdh_en_device *en_dev, uint32_t index)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -1;
    }

    msg->payload.hdr.op_code = ZXDH_FD_DEL;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.vf_fd_cfg_msg.index = index;
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "Del_fd:zxdh_send_command_to_pf_np failed: %d\n", err);
    }

    kfree(msg);
    return err;
}
#if 0
int32_t zxdh_aux_query_phyport(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    struct aux_phyport_message recv = {0};
    struct aux_phyport_message *recv_data = &recv;
    zxdh_aux_phyport_msg msg = {0};
    zxdh_aux_phyport_msg *payload = &msg;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;
    payload->pcie_id = en_dev->pcie_id;
    payload->pannel_id = en_dev->pannel_id;
    payload->rsv = en_dev->pannel_id;


    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_PHYPORT_QUERY, payload, recv_data, &para);
    if (ret != 0)
    {
        LOG_ERR("zxdh_aux_query_phyport send message failed \n");
        goto out;
    }

    en_dev->phy_port = recv_data->phyport;

out:
    return ret;
}
#endif

static void pf_recover_mac_get(struct zxdh_en_device *en_dev)
{
    struct netdev_hw_addr *ha = NULL;
    uint32_t i = 0;
    uint32_t j = 0;

    /* 给net_device结构体上锁 */
    netif_addr_lock_bh(en_dev->netdev);

    /* 获取单播链表mac */
    list_for_each_entry(ha, &en_dev->netdev->uc.list, list)
    {
        if (i >= VF_MAX_UNICAST_MAC)
        {
            LOG_ERR_DEV(en_dev->parent, "umac_num: %d exceed the max num: %d\n", i, VF_MAX_UNICAST_MAC);
            break;
        }
        zte_memcpy_s(en_dev->eth_config.pf_recover_mac.umac[i].mac_addr, ha->addr, ETH_ALEN);
        i++;
    }

    /* 获取组播链表mac */
    list_for_each_entry(ha, &en_dev->netdev->mc.list, list)
    {
        if (j >= VF_MAX_MULTICAST_MAC)
        {
            LOG_ERR_DEV(en_dev->parent, "mmac_num: %d exceed the max num: %d", j, VF_MAX_MULTICAST_MAC);
            break;
        }
        zte_memcpy_s(en_dev->eth_config.pf_recover_mac.mmac[j].mac_addr, ha->addr, ETH_ALEN);
        j++;
    }

    /* 给net_device结构体释放锁 */
    netif_addr_unlock_bh(en_dev->netdev);
    en_dev->eth_config.pf_recover_mac.umac_num = i;
    en_dev->eth_config.pf_recover_mac.mmac_num = j;
    return;
}

static int32_t eth_pf_mac_addr_recover(struct zxdh_en_device *en_dev)
{
    DPP_PF_INFO_T pf_info = {0};
    uint32_t i = 0;
    int32_t ret = 0;

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    /* 本地mac配置 */
    ret = dpp_add_mac(&pf_info, en_dev->netdev->dev_addr, 0, 0);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "pf add mac failed in recover local mac: %d\n", ret);
        return ret;
    }

    /* 遍历链表，获取当前待回复mac */
    pf_recover_mac_get(en_dev);

    /* 单播链表mac配置 */
    for (i = 0; i < en_dev->eth_config.pf_recover_mac.umac_num; i++)
    {
        ret = dpp_add_mac(&pf_info, en_dev->eth_config.pf_recover_mac.umac[i].mac_addr, 0, 0);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "pf add mac failed in recover uc list: %d\n", ret);
            return ret;
        }
    }

    /* 组播链表mac配置 */
    for (i = 0; i < en_dev->eth_config.pf_recover_mac.mmac_num; i++)
    {
        ret = dpp_multi_mac_add_member(&pf_info, en_dev->eth_config.pf_recover_mac.mmac[i].mac_addr);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "pf add mac failed in recover mc list: %d\n", ret);
            return ret;
        }
    }
    return 0;
}

static int32_t zxdh_recover_fd_cfg(struct zxdh_en_device *en_dev)
{
    uint32_t orig_index = 0;
    uint32_t new_index = 0;
    uint32_t flow_num = 0;
    uint32_t location = 0;
    int32_t err = 0;
    ZXDH_FD_CFG_T p_fd_cfg = {0};
    DPP_PF_INFO_T pf_info = {0};
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;
    LOG_INFO_DEV(en_dev->parent, "recover_flow_table: total flow_num is %d", en_dev->fs.tot_num_rules);
    while (flow_num < en_dev->fs.tot_num_rules && location < ETHTOOL_FD_MAX_NUM) {
        if (en_dev->fs.ethtool_fs[location].is_used) {
            orig_index = en_dev->fs.ethtool_fs[location].index;
            if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) {
                en_dev->fs.ethtool_fs[location].is_used = false;
                err = zxdh_vf_add_fd(en_dev, &en_dev->fs.ethtool_fs[location].rfs, &new_index);
                if (err) {
                    LOG_ERR_DEV(en_dev->parent, "zxdh_vf_recover_fd failed, location %d\n", location);
                    return -1;
                }
                en_dev->fs.ethtool_fs[location].is_used = true;
            } else {
                zxdh_flow_table_add(&en_dev->fs.ethtool_fs[location].rfs, &p_fd_cfg, &pf_info);
                err = zxdh_flow_table_pf_action_add(en_dev, &en_dev->fs.ethtool_fs[location].rfs, &p_fd_cfg);
                if (err) {
                    LOG_ERR_DEV(en_dev->parent, "zxdh_cfg_fd_add_action failed, location %d", location);
                    return -EINVAL;
                }
                err = dpp_fd_acl_index_request(&pf_info, &new_index);
                if (err != 0) {
                    LOG_ERR_DEV(en_dev->parent, "zxdh_cfg_np_fd_acl_request failed, location %d\n", location);
                    return -1;
                }
                err = dpp_tbl_fd_cfg_add(&pf_info, ZXDH_SDT_FD_CFG_TABLE, new_index,
                    &p_fd_cfg);
                if (err != 0) {
                    LOG_ERR_DEV(en_dev->parent, "zxdh_cfg_np_fd_recover failed, location %d\n", location);
                    return -1;
                }
            }
            en_dev->fs.ethtool_fs[location].index = new_index;
            flow_num++;
            LOG_INFO_DEV(en_dev->parent, "recover_flow_table: location is %u, orig_index is %u, new_index is %u",
                    location, orig_index, new_index);
        }
        location++;
    }
    return 0;
}

int32_t zxdh_pf_roce_overlay_init(struct zxdh_en_device *en_dev, DPP_PF_INFO_T *pf_info)
{
    int32_t ret = 0;

    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_SPECIAL_BOND)
    {
        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_ROCE_OVERLAY_EN, 1);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_vport_attr_set roce_overlay_enable failed: %d\n", ret);
        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_MULTI_HOST_EN, 1);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_vport_attr_set multi_host_enable failed: %d\n", ret);

        ret = dpp_uplink_phy_attr_set(pf_info, en_dev->phy_port, UPLINK_PHY_PORT_ROCE_OVERLAY_EN, 1);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_uplink_phy_attr_set roce_overlay_enable failed: %d\n", ret);
        ret = dpp_uplink_phy_attr_set(pf_info, en_dev->phy_port, UPLINK_PHY_PORT_MULTI_HOST_EN, 1);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_uplink_phy_attr_set multi_host_enable failed: %d\n", ret);

        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_HW_BOND_EN_OFF, 0);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_vport_attr_set hw_bond_enable failed: %d\n", ret);

        if (en_dev->hash_search_idx == 0)
        {
            ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_MULTI_HOST_GROUP_ID, 0);
            ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_vport_attr_set multi_host_group_id 0 failed: %d\n", ret);

            ret = dpp_uplink_phy_attr_set(pf_info, en_dev->phy_port, UPLINK_PHY_PORT_MULTI_HOST_GROUP_ID, 0);
            ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_uplink_phy_attr_set multi_host_group_id 0 failed: %d\n", ret);
        }
        else if (en_dev->hash_search_idx == 1)
        {
            ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_MULTI_HOST_GROUP_ID, 1);
            ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_vport_attr_set multi_host_group_id 1 failed: %d\n", ret);

            ret = dpp_uplink_phy_attr_set(pf_info, en_dev->phy_port, UPLINK_PHY_PORT_MULTI_HOST_GROUP_ID, 1);
            ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_uplink_phy_attr_set multi_host_group_id 1 failed: %d\n", ret);
        }
    }

    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_SRIOV)
    {
        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_K8S_CNI_FLAG, 1);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_vport_attr_set k8s_cni_flag failed: %d\n", ret);
    }

    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA)
    {
        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_BUSINESS_EN_OFF, 1);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_vport_attr_set business_enable failed: %d\n", ret);
        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_ROCE_OVERLAY_EN, 1);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_vport_attr_set roce_overlay_enable failed: %d\n", ret);
        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_LAG_EN_OFF, 1);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_vport_attr_set lag_enable failed: %d\n", ret);
    }

    return 0;

err_vport:
    return ret;
}

int32_t zxdh_pf_port_init(struct zxdh_en_device *en_dev, bool boot)
{
    bool vepa = false;
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};

    if (en_dev == NULL)
    {
        return -1;
    }

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

#if 0
    en_dev->ops->dpp_np_init(en_dev->parent, en_dev->vport);
#endif

    ret = zxdh_pf_vport_create(en_dev);
    ZXDH_CHECK_RET_RETURN(ret, "zxdh_pf_vport_create failed: %d\n", ret);

    if (zxdh_en_is_panel_port(en_dev))
        dpp_uplink_phy_attr_set(&pf_info, en_dev->phy_port, UPLINK_PHY_PORT_MAGIC_PACKET_ENABLE,
                                    (en_dev->wolopts == WAKE_MAGIC));

    zxdh_mac_stats_clear(en_dev);

    if (en_dev->ops->is_bond(en_dev->parent))
    {
        if (!en_dev->ops->if_init(en_dev->parent))
        {
            LOG_INFO_DEV(en_dev->parent, "First net-device is init\n");
            return 0;
        }

        /* 只将第一个网络设备的队列配置到vport属性表中 */
        ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_PORT_BASE_QID, (uint16_t)en_dev->phy_index[0]);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "set qid: %d failed: %d\n", (uint16_t)en_dev->phy_index[0], ret);
        return 0;
    }

    vepa = en_dev->ops->get_vepa(en_dev->parent);
    ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_VEPA_EN_OFF, (uint32_t)vepa);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_vport_attr_set SRIOV_VPORT_VEPA_EN_OFF failed: %d\n", ret);
    LOG_DEBUG_DEV(en_dev->parent, "init vport(0x%x) to %s mode\n", en_dev->vport, vepa? "vepa" : "veb");

    ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_HASH_SEARCH_INDEX, en_dev->hash_search_idx);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "set hash_search_index %u failed: %d\n", en_dev->hash_search_idx, ret);

    if (en_dev->ops->is_lowlatency(en_dev->parent))
    {
        ret = dpp_pktrx_udf_icmp_item_set(&pf_info, en_dev->phy_port);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_pktrx_udf_icmp_item_set failed: %d\n", ret);

        ret = dpp_pktrx_tcam_icmp_item_set(&pf_info, (unsigned char *)en_dev->netdev->dev_addr, en_dev->phy_port);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_pktrx_tcam_icmp_item_set failed: %d\n", ret);
    }

    ret = zxdh_pf_roce_overlay_init(en_dev, &pf_info);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "zxdh_pf_roce_overlay_init failed: %d\n", ret);

    ret = zxdh_ethtool_init(en_dev);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "zxdh_ethtool_init failed: %d\n", ret);

    if (boot) {
        /* PF删除复位前配置到np的mac */
        ret = zxdh_pf_flush_mac(en_dev);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "zxdh_pf_flush_mac failed: %d\n", ret);

        ret = dpp_add_mac(&pf_info, en_dev->netdev->dev_addr, 0, 0);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_add_mac failed: %d\n", ret);

        ret = dpp_acl_entry_flush_by_vport(&pf_info, ZXDH_SDT_CSIG_TAG_TABLE);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_acl_entry_flush_by_vport failed: %d\n", ret);
        /* 删除roce_cycle功能的残留表项配置 */
        ret = dpp_hash_entry_flush(&pf_info, ZXDH_SDT_ROCE_MATCH_TABLE, HASH_FLUSH_OFFLINE_MODE);
        if (ret != DPP_OK) {
            LOG_WARN_DEV(en_dev->parent, "dpp_hash_entry_flush failed: %d, maybe fw version do not support roce_cycle_stat.\n", ret);
        }
        en_dev->roce_cycle_speed = ZXDH_ROCE_CYCLE_INIT_SPEED;
        ret = dpp_pktrx_mcode_glb_cfg_wr(&pf_info, DPP_GLB_CFG_REG_INDEX_1, ZXDH_NP_ROCE_CYCLE_STAT_ENABLE_BIT, ZXDH_NP_ROCE_CYCLE_STAT_ENABLE_BIT, 0);
        if (ret != DPP_OK) {
            LOG_WARN_DEV(en_dev->parent, "dpp_pktrx_mcode_glb_cfg_set failed: %d, maybe fw version do not support roce_cycle_stat.\n", ret);
        }
    } else {
        ret = eth_pf_mac_addr_recover(en_dev);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "eth_pf_mac_addr_recover failed: %d\n", ret);
    }

    if (!boot) {
        ret = zxdh_vlan_trunk_recover(&pf_info, en_dev->eth_config.vlan_trunk_bitmap);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "zxdh_vlan_trunk_recover failed: %d\n", ret);
    }

    ether_addr_copy(en_dev->last_np_mac_addr.sa_data, en_dev->netdev->dev_addr);
    if (en_dev->promisc_enabled) {
        dpp_vport_uc_promisc_set(&pf_info, 1);
        dpp_vport_mc_promisc_set(&pf_info, 1);
        dpp_vport_promisc_en_set(&pf_info, 1);
    } else if (en_dev->allmulti_enabled) {
        dpp_vport_mc_promisc_set(&pf_info, 1);
    } else {
        dpp_vport_uc_promisc_set(&pf_info, 0);
        dpp_vport_mc_promisc_set(&pf_info, 0);
    }

    if (!boot) {
        /* 删除复位前配置的fd表 */
        ret = dpp_fd_acl_all_delete(&pf_info);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_fd_acl_all_delete failed: %d\n", ret);
        ret = zxdh_recover_fd_cfg(en_dev);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_fd_acl_all_recover failed: %d\n", ret);
    }

    if ((!zxdh_en_is_panel_port(en_dev)) || (!boot))
        return 0;

    /* pf先清除arn/psn类型的udp报文统计 */
    ret = dpp_stat_asn_phyport_rx_pkt_cnt_get(&pf_info, en_dev->phy_port, STAT_RD_CLR_MODE_CLR, \
                                                 &en_dev->hw_stats.udp_stats.rx_arn_phy);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_stat_asn_phyport_rx_pkt_cnt_get failed: %d\n", ret);

    ret = dpp_stat_psn_phyport_tx_pkt_cnt_get(&pf_info, en_dev->phy_port, STAT_RD_CLR_MODE_CLR, \
                                                &en_dev->hw_stats.udp_stats.tx_psn_phy);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_stat_psn_phyport_tx_pkt_cnt_get failed: %d\n", ret);

    ret = dpp_stat_psn_phyport_rx_pkt_cnt_get(&pf_info, en_dev->phy_port, STAT_RD_CLR_MODE_CLR, \
                                                &en_dev->hw_stats.udp_stats.rx_psn_phy);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_stat_psn_phyport_rx_pkt_cnt_get failed: %d\n", ret);

    ret = dpp_stat_psn_ack_phyport_tx_pkt_cnt_get(&pf_info, en_dev->phy_port, STAT_RD_CLR_MODE_CLR, \
                                                &en_dev->hw_stats.udp_stats.tx_psn_ack_phy);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_stat_psn_ack_phyport_tx_pkt_cnt_get failed: %d\n", ret);

    ret = dpp_stat_psn_ack_phyport_rx_pkt_cnt_get(&pf_info, en_dev->phy_port, STAT_RD_CLR_MODE_CLR, \
                                                    &en_dev->hw_stats.udp_stats.rx_psn_ack_phy);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_stat_psn_ack_phyport_rx_pkt_cnt_get failed: %d\n", ret);

    zxdh_dual_tor_switch(en_dev, 0);

    return 0;

err_vport:
    zxdh_en_vport_delete(en_dev);
    return ret;
}

int32_t zxdh_vf_dpp_port_init(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;

    ret = zxdh_vf_port_create(en_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_vf_port_create failed: %d\n", ret);
    }

    return ret;
}

int32_t zxdh_port_reload(struct zxdh_en_device *en_dev)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    bool is_upf = false;
    uint8_t link_up = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL) {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_VF_PORT_RELOAD;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;

    msg->payload.vf_reload_msg.base_qid = en_dev->phy_index[0];
    is_upf = !(zxdh_en_is_panel_port(en_dev));
    msg->payload.vf_reload_msg.is_upf = is_upf;
    msg->payload.vf_reload_msg.hash_search_idx = en_dev->hash_search_idx;
    zte_memcpy_s(msg->payload.vf_reload_msg.queue_map, en_dev->eth_config.queue_map, ZXDH_INDIR_RQT_SIZE * sizeof(uint32_t));

    msg->payload.vf_reload_msg.hash_mode = en_dev->eth_config.hash_mode;
    msg->payload.vf_reload_msg.hash_func = en_dev->eth_config.hash_func;

    /* 将vlan trunk表copy到消息中*/
    zte_memcpy_s(msg->payload.vf_reload_msg.vlan_trunk_bitmap, en_dev->eth_config.vlan_trunk_bitmap, sizeof(en_dev->eth_config.vlan_trunk_bitmap));

    if (en_dev->promisc_enabled) {
        msg->payload.vf_reload_msg.uc_promisc = true;
        msg->payload.vf_reload_msg.mc_promisc = true;
    } else if (en_dev->allmulti_enabled) {
        msg->payload.vf_reload_msg.mc_promisc = true;
    }

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if(ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_pf failed: %d\n", ret);
        kfree(msg);
        return ret;
    }

    if (!is_upf ||
        en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA)
    {
        en_dev->ops->get_link_info_from_vqm(en_dev->parent, &link_up);
        en_dev->link_up = link_up;
        LOG_DEBUG_DEV(en_dev->parent, "vf read link_up: %d\n", link_up);
    }
    else
    {
        en_dev->link_up = msg->reps.vf_reload_msg.link_up;
    }

    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA) {
        en_dev->speed = SPEED_200000;
        en_dev->duplex = DUPLEX_FULL;
    } else {
        en_dev->speed = msg->reps.vf_reload_msg.speed;
        en_dev->duplex = msg->reps.vf_reload_msg.duplex;
    }

    netif_tx_wake_all_queues(en_dev->netdev);
    if (en_dev->link_up) {
        en_dev->ops->set_pf_link_up(en_dev->parent, TRUE);
        netif_carrier_on(en_dev->netdev);
    } else {
        en_dev->ops->set_pf_link_up(en_dev->parent, FALSE);
        netif_carrier_off(en_dev->netdev);
    }

    kfree(msg);
    return ret;
}

int32_t zxdh_port_init(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t err = 0;

    err = zxdh_indir_to_queue_map(en_dev, en_dev->indir_rqt);
    ZXDH_CHECK_RET_RETURN(err, "zxdh_indir_to_queue_map failed: %d\n", err);

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF) {
        err = zxdh_pf_port_init(en_dev, false);
        ZXDH_CHECK_RET_RETURN(err, "zxdh_port_init failed: %d\n", err);

        err = zxdh_en_hash_key_recover(en_dev);
        ZXDH_CHECK_RET_GOTO_ERR(err, port_uninit, "zxdh_en_hash_key_recover failed: %d\n", err);

        if (!en_dev->ops->is_bond(en_dev->parent)) {
            err = zxdh_rxfh_set(en_dev, en_dev->eth_config.queue_map);
            ZXDH_CHECK_RET_GOTO_ERR(err, port_uninit, "zxdh_rxfh_set failed: %d\n", err);
        }
    } else {
        err = zxdh_port_reload(en_dev);
        ZXDH_CHECK_RET_RETURN(err, "zxdh_port_reload failed: %d\n", err);
        err = zxdh_recover_fd_cfg(en_dev);
        ZXDH_CHECK_RET_GOTO_ERR(err, port_uninit, "zxdh_port_recover_fd failed: %d\n", err);
    }

    err = zxdh_tc_flow_recover(en_priv);
    ZXDH_CHECK_RET_GOTO_ERR(err, port_uninit, "zxdh_tc_flow_recover failed: %d\n", err);

    err = zxdh_en_config_mtu_to_np(netdev, netdev->mtu);
    ZXDH_CHECK_RET_GOTO_ERR(err, port_uninit, "zxdh_en_config_mtu_to_np failed: %d\n", err);

    if (!en_dev->ops->is_bond(en_dev->parent)) {
        err = zxdh_en_sync_features(en_dev, en_dev->features);
        ZXDH_CHECK_RET_GOTO_ERR(err, port_uninit, "zxdh_en_sync_features failed: %d\n", err);
    }

    return 0;
port_uninit:
    zxdh_vport_uninit(netdev, false);
    return err;
}

void zxdh_vport_uninit(struct net_device *netdev, bool is_remove)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t ret = 0;

    if (!is_remove && (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF))
    {
        netif_carrier_off(netdev);
        netif_tx_stop_all_queues(netdev);
    }

    if (en_dev->quick_remove)
        return;

    if (en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
        return;

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        ret = zxdh_pf_port_delete(netdev);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_pf_port_delete failed: %d\n", ret);
        }
    }
    else
    {
#ifdef VF_STATS_UPDATE
        ret = zxdh_vf_item_init_stats_update(en_dev);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_item_init_stats_update failed: %d\n", ret);
        }
#endif
        ret = zxdh_vf_port_delete(en_dev);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_port_delete failed: %d\n", ret);
        }
    }
}

uint32_t zxdh_uplink_phy_attr_set(DPP_PF_INFO_T* pf_info, uint8_t phy_port, uint32_t attr, uint32_t value)
{
    if (phy_port == INVALID_PHY_PORT)
        return 0;

    return dpp_uplink_phy_attr_set(pf_info, phy_port, attr, value);
}



