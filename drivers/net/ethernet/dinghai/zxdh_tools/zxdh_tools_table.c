#include <linux/dinghai/dh_cmd.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/log.h>
#include <linux/dinghai/kcompat.h>
#include "../en_aux.h"
#include "../en_aux/en_aux_cmd.h"
#include "../cmd/msg_chan_priv.h"
#include "zxdh_tools_ioctl.h"
#include "dpp_drv_sdt.h"

typedef enum zxdh_sdt_tbl_msg_op_code_t {
    MSG_SDT_TABLE_NONE = 0,
    MSG_SDT_TABLE_DUMP_INFO,
    MSG_SDT_TABLE_DUMP_ITEM,
    MSG_SDT_TABLE_DUMP_ITEM_BY_INDEX,
} ZXDH_SDT_TBL_MSG_OP_CODE_T;

typedef struct zxdh_sdt_tbl_msg_t {
    uint32_t op_code;
    uint32_t payload_len;
    uint8_t  payload[0];
} ZXDH_SDT_TBL_MSG_T;

typedef struct zxdh_sdt_tbl_dump_item_t {
    uint32_t sdtno;
    uint32_t index;
} ZXDH_SDT_TBL_DUMP_ITEM_T;

typedef struct zxdh_sdt_tbl_op_code_callback_t {
    uint32_t op_code;
    int32_t (*callback)(DPP_PF_INFO_T *pf_info, ZXDH_SDT_TBL_MSG_T *tbl_msg, void *msg_reps);
} ZXDH_SDT_TBL_MSG_OP_CODE_CALLBACK_T;

#define ZXDH_SDT_TABLE_MSG_MAX_LEN  (2048)

static int32_t zxdh_sdt_table_dump_info(DPP_PF_INFO_T *pf_info, ZXDH_SDT_TBL_MSG_T *tbl_msg, void *msg_reps)
{
    uint32_t i = 0;
    ZXIC_UINT32 tbl_type = 0;
    ZXDH_SDT_TBL_INFO_T sdt_tbl_info = {0};

    if (pf_info == NULL || tbl_msg == NULL || msg_reps == NULL)
    {
        return MSG_RECV_FAILED;
    }

    for (i = 0; i < DPP_DEV_SDT_ID_MAX; i++)
    {
        if (dpp_sdt_tbl_type_get(pf_info, i, &tbl_type) != DPP_OK)
        {
            return MSG_RECV_FAILED;
        }

        if (tbl_type == DPP_SDT_TBLT_INVALID || tbl_type >= DPP_SDT_TBLT_MAX)
        {
            continue;
        }

        if (dpp_sdt_tbl_info_dump(pf_info, i, &sdt_tbl_info) != DPP_OK)
        {
            return MSG_RECV_FAILED;
        }

        if (unlikely(copy_to_user((uint8_t __user *)msg_reps + i * sizeof(ZXDH_SDT_TBL_INFO_T), &sdt_tbl_info, sizeof(ZXDH_SDT_TBL_INFO_T))))
        {
            DHTOOLS_LOG_ERR("copy_to_user failed!!!\n");
            return MSG_RECV_FAILED;
        }
    }

    return MSG_RECV_OK;
}

static int32_t zxdh_sdt_table_dump_item(DPP_PF_INFO_T *pf_info, ZXDH_SDT_TBL_MSG_T *tbl_msg, void *msg_reps)
{
    uint32_t tbl_item_len = 0;
    ZXDH_SDT_TBL_ITEM_T *tbl_item = NULL;
    ZXDH_SDT_TBL_DUMP_ITEM_T *tbl_dump_req = NULL;
    ZXDH_SDT_TBL_INFO_T sdt_tbl_info = {0};

    if (pf_info == NULL || tbl_msg == NULL || msg_reps == NULL)
    {
        return MSG_RECV_FAILED;
    }

    tbl_dump_req = (ZXDH_SDT_TBL_DUMP_ITEM_T *)tbl_msg->payload;
    if (tbl_dump_req == NULL)
    {
        return MSG_RECV_FAILED;
    }

    if (dpp_sdt_tbl_info_dump(pf_info, tbl_dump_req->sdtno, &sdt_tbl_info) != DPP_OK)
    {
        return MSG_RECV_FAILED;
    }

    tbl_item_len = sizeof(ZXDH_SDT_TBL_ITEM_T) + sizeof(ZXDH_SDT_TBL_ITEM_DATA_T) * (sdt_tbl_info.tbl_depth);
    tbl_item = (ZXDH_SDT_TBL_ITEM_T *)vmalloc(tbl_item_len);
    if (tbl_item == NULL)
    {
        DHTOOLS_LOG_ERR("vmalloc tbl_item failed!!!\n");
        return MSG_RECV_FAILED;
    }

    if (dpp_sdt_tbl_item_dump(pf_info, tbl_dump_req->sdtno, tbl_item->item_data, &(tbl_item->item_num)) != DPP_OK)
    {
        vfree(tbl_item);
        return MSG_RECV_FAILED;
    }

    if (unlikely(copy_to_user((uint8_t __user *)msg_reps, tbl_item, tbl_item_len)))
    {
        DHTOOLS_LOG_ERR("copy_to_user failed!!!\n");
        vfree(tbl_item);
        return MSG_RECV_FAILED;
    }

    vfree(tbl_item);
    return MSG_RECV_OK;
}

static int32_t zxdh_sdt_table_dump_item_by_index(DPP_PF_INFO_T *pf_info, ZXDH_SDT_TBL_MSG_T *tbl_msg, void *msg_reps)
{
    uint32_t tbl_item_len = 0;
    ZXDH_SDT_TBL_ITEM_T *tbl_item = NULL;
    ZXDH_SDT_TBL_DUMP_ITEM_T *tbl_dump_req = NULL;

    if (pf_info == NULL || tbl_msg == NULL || msg_reps == NULL)
    {
        return MSG_RECV_FAILED;
    }

    tbl_dump_req = (ZXDH_SDT_TBL_DUMP_ITEM_T *)tbl_msg->payload;
    if (tbl_dump_req == NULL)
    {
        return MSG_RECV_FAILED;
    }

    tbl_item_len = sizeof(ZXDH_SDT_TBL_ITEM_T) + sizeof(ZXDH_SDT_TBL_ITEM_DATA_T);
    tbl_item = (ZXDH_SDT_TBL_ITEM_T *)vmalloc(tbl_item_len);
    if (tbl_item == NULL)
    {
        DHTOOLS_LOG_ERR("vmalloc tbl_item failed!!!\n");
        return MSG_RECV_FAILED;
    }

    if (dpp_sdt_tbl_item_get(pf_info, tbl_dump_req->sdtno, tbl_dump_req->index, tbl_item->item_data) != DPP_OK)
    {
        vfree(tbl_item);
        return MSG_RECV_FAILED;
    }
    tbl_item->item_num = 1;

    if (unlikely(copy_to_user((uint8_t __user *)msg_reps, tbl_item, tbl_item_len)))
    {
        DHTOOLS_LOG_ERR("copy_to_user failed!!!\n");
        vfree(tbl_item);
        return MSG_RECV_FAILED;
    }

    vfree(tbl_item);
    return MSG_RECV_OK;
}

ZXDH_SDT_TBL_MSG_OP_CODE_CALLBACK_T zxdh_tools_sdt_table_callback[] = {
    {MSG_SDT_TABLE_DUMP_INFO,           zxdh_sdt_table_dump_info},
    {MSG_SDT_TABLE_DUMP_ITEM,           zxdh_sdt_table_dump_item},
    {MSG_SDT_TABLE_DUMP_ITEM_BY_INDEX,  zxdh_sdt_table_dump_item_by_index},
};

int32_t zxdh_tools_sdt_table(struct net_device *netdev, struct ifreq *ifr)
{
    uint32_t i = 0;
    int32_t ret = 0;
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    DPP_PF_INFO_T pf_info = {0};
    struct zxdh_tools_msg *tools_msg = NULL;
    struct zxdh_tools_reps tools_reps = {0};
    ZXDH_SDT_TBL_MSG_T *tbl_msg = NULL;

    en_priv = netdev_priv(netdev);
    if (en_priv == NULL)
    {
        DHTOOLS_LOG_ERR("en_priv is NULL!!!\n");
        return -EFAULT;
    }

    en_dev = &en_priv->edev;
    if (en_dev == NULL)
    {
        DHTOOLS_LOG_ERR("en_dev is NULL!!!\n");
        return -EFAULT;
    }
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    tools_msg = (struct zxdh_tools_msg *)kzalloc(sizeof(struct zxdh_tools_msg), GFP_KERNEL);
    if (tools_msg == NULL)
    {
        DHTOOLS_LOG_ERR_DEV(en_dev->parent, "kzalloc tools_msg failed!!!\n");
        return -EFAULT;
    }

    if (unlikely(copy_from_user(tools_msg, ifr->ifr_ifru.ifru_data, sizeof(struct zxdh_tools_msg))))
    {
        DHTOOLS_LOG_ERR_DEV(en_dev->parent, "copy_from_user failed!!!\n");
        kfree(tools_msg);
        return -EFAULT;
    }

    if (tools_msg->payload_len == 0 || tools_msg->payload_len > ZXDH_SDT_TABLE_MSG_MAX_LEN)
    {
        DHTOOLS_LOG_ERR_DEV(en_dev->parent, "tools_msg payload failed!!!\n");
        kfree(tools_msg);
        return -EINVAL;
    }

    tbl_msg = (ZXDH_SDT_TBL_MSG_T *)kzalloc(tools_msg->payload_len, GFP_KERNEL);
    if (tbl_msg == NULL)
    {
        DHTOOLS_LOG_ERR_DEV(en_dev->parent, "kzalloc tbl_msg failed!!!\n");
        kfree(tools_msg);
        return -ENOMEM;
    }

    if (unlikely(copy_from_user(tbl_msg, (uint8_t *)(ifr->ifr_ifru.ifru_data) + sizeof(struct zxdh_tools_msg), tools_msg->payload_len)))
    {
        DHTOOLS_LOG_ERR_DEV(en_dev->parent, "copy_from_user failed!!!\n");
        kfree(tbl_msg);
        kfree(tools_msg);
        return -EFAULT;
    }

    for (i = 0; i < ARRAY_SIZE(zxdh_tools_sdt_table_callback); i++)
    {
        if ((zxdh_tools_sdt_table_callback[i].op_code == tbl_msg->op_code) && (zxdh_tools_sdt_table_callback[i].callback))
        {
            ret = zxdh_tools_sdt_table_callback[i].callback(&pf_info, tbl_msg, tools_msg->msg_reps);
            break;
        }
    }
    tools_reps.status = (i == ARRAY_SIZE(zxdh_tools_sdt_table_callback))? MSG_RECV_NOT_FOUND : (uint32_t)ret;

    if (unlikely(copy_to_user((void __user *)tools_msg->tools_reps, &tools_reps, sizeof(struct zxdh_tools_reps))))
    {
        DHTOOLS_LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!!!\n");
        kfree(tbl_msg);
        kfree(tools_msg);
        return -EFAULT;
    }

    kfree(tbl_msg);
    kfree(tools_msg);

    return 0;
}
