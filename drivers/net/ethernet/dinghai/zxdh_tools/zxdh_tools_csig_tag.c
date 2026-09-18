#include <linux/dinghai/dh_cmd.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/log.h>
#include <linux/dinghai/kcompat.h>
#include "../en_aux.h"
#include "../en_aux/en_aux_cmd.h"
#include "../cmd/msg_chan_priv.h"
#include "zxdh_tools_ioctl.h"
#include "dpp_drv_sdt.h"
#include "dpp_tbl_api.h"

typedef enum {
    MSG_CSIG_TAG_OK                 = 0,
    MSG_CSIG_TAG_RECV_NOT_FOUND     = 1,
    MSG_CSIG_TAG_COMMON_ERR         = 2,
    MSG_CSIG_TAG_ERR_MSG_LEN        = 3,
    MSG_CSIG_TAG_ERR_PARAM          = 4,
    MSG_CSIG_TAG_NULL_POINT         = 5,
    MSG_CSIG_TAG_ACL_REQ_FAIL       = 6,
    MSG_CSIG_TAG_ACL_KEY_NOT_EXIST  = 7,
    MSG_CSIG_TAG_ACL_ADD_FAIL       = 8,
    MSG_CSIG_TAG_ACL_REL_FAIL       = 9,
    MSG_CSIG_TAG_ACL_INDEX_REL_FAIL = 10,
    MSG_CSIG_TAG_ACL_SRH_FAIL       = 11,
    MSG_CSIG_TAG_ACL_DUMP_FAIL      = 12,
    MSG_CSIG_TAG_ACL_FLUSH_FAIL     = 13,
    MSG_CSIG_TAG_RET_MAX
}MSG_CSIG_TAG_RET_ENUM;

typedef enum zxdh_tools_csig_tag_msg_opcode_t {
    MSG_CSIG_TAG_FLAG                = 0,       /*预留，填充payload[0]一个字节，表示enable或disable*/
    MSG_CSIG_TAG_ADD_RULE            = 1,       /*填充/回复 ZXDH_TOOLS_ACL_TBL_T*/
    MSG_CSIG_TAG_DEL_RULE            = 2,       /*填充/回复 ZXDH_TOOLS_ACL_TBL_T*/
    MSG_CSIG_TAG_SRH_RULE            = 3,       /*填充/回复 ZXDH_TOOLS_ACL_TBL_T*/
    MSG_CSIG_TAG_DUMP_RULE           = 4,       /*发送消息不需要填充，回复消息填充ZXDH_TOOLS_ACL_TBL_T*/
    MSG_CSIG_TAG_FLUSH_RULE          = 5,       /*payload不需要填充，根据opcode识别*/
    MSG_CSIG_TAG_GET_ACL_INFO        = 6,       /*发送消息不需要填充,回复消息填充ZXDH_TOOLS_ACL_INFO_T*/
    MSG_CSIG_TAG_DEL_RULE_BY_INDEX   = 7,       /*填充/回复 ZXDH_TOOLS_ACL_TBL_T*/
    MSG_CSIG_TAG_SRH_RULE_BY_INDEX   = 8,       /*填充/回复 ZXDH_TOOLS_ACL_TBL_T*/
    MSG_CSIG_TAG_DUMP_RULE_ALL       = 9,       /*发送消息不需要填充，回复消息填充ZXDH_TOOLS_ACL_TBL_T*/
    MSG_CSIG_TAG_FLUSH_RULE_ALL      = 10,      /*payload不需要填充，根据opcode识别*/
    MSG_CSIG_TAG_OPCODE_MAX
} ZXDH_TOOLS_CSIG_TAG_MSG_OPCODE_T;

/* struct zxdh_tools_msg->payload */
typedef struct zxdh_tools_csig_tag_msg_t {
    uint32_t op_code;        /*zxdh_tools_csig_tag_msg_op_code_t*/
    uint32_t payload_len;    
    uint8_t  payload[0];       
} ZXDH_TOOLS_CSIG_TAG_MSG_T;

typedef struct zxdh_tools_acl_item_info_t{
    uint32_t handle;
    uint8_t key_data[80];
    uint8_t key_mask[80];
    uint8_t as_rst[16];
} ZXDH_TOOLS_ACL_ITEM_INFO_T;

typedef struct zxdh_tools_acl_tbl_t {
    uint32_t item_num;
    ZXDH_TOOLS_ACL_ITEM_INFO_T item_info[0];
} ZXDH_TOOLS_ACL_TBL_T;

typedef struct zxdh_tools_acl_info_t {
    uint32_t acl_depth;
    uint32_t acl_used_num;
} ZXDH_TOOLS_ACL_INFO_T;

typedef struct zxdh_csig_tag_opcode_callback_t {
    uint32_t op_code;
    int32_t (*callback)(DPP_PF_INFO_T *pf_info, ZXDH_TOOLS_CSIG_TAG_MSG_T *csig_tag_msg, void *msg_reps);
} ZXDH_CSIG_TAG_MSG_OPCODE_CALLBACK_T;

static int32_t zxdh_csig_tag_flag(DPP_PF_INFO_T *pf_info, ZXDH_TOOLS_CSIG_TAG_MSG_T *csig_tag_msg, void *msg_reps)
{
    return MSG_CSIG_TAG_OK;
}

static void print_csig_tag_msg(uint32_t op_code, ZXDH_TOOLS_ACL_TBL_T *p_csig_tag_rule)
{
    uint32_t i = 0;
    uint32_t j = 0;
    ZXDH_TOOLS_ACL_ITEM_INFO_T *p_item_info = NULL;
    uint8_t *key_data = NULL;
    uint8_t *key_mask = NULL;
    uint8_t *as_rst = NULL;

    if(p_csig_tag_rule==NULL)
        return;

    DHTOOLS_LOG_DEBUG("op_code=%u(1:add 2:del 3:srh 4:dump 5:flush 6:acl info 7:del by index 8:srh by index)\n",op_code);
    if((op_code==MSG_CSIG_TAG_FLUSH_RULE) 
        || (op_code==MSG_CSIG_TAG_FLAG) 
        || (op_code==MSG_CSIG_TAG_GET_ACL_INFO))
    {
        return;
    }
    
    for(i=0; i<p_csig_tag_rule->item_num;i++)
    {
        p_item_info = &p_csig_tag_rule->item_info[i];
        key_data = p_item_info->key_data;
        key_mask = p_item_info->key_mask;
        as_rst = p_item_info->as_rst;

        DHTOOLS_LOG_DEBUG("No[%u] handle=0x%x\n",i,p_item_info->handle);
        if(p_item_info->handle==0xFFFFFFFF)
        {
            continue;
        }
        DHTOOLS_LOG_DEBUG("key_data:\n");
        for(j=0;j<80/16;j++)
        {
            DHTOOLS_LOG_DEBUG("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", 
                     key_data[j*16],key_data[j*16+1],key_data[j*16+2],key_data[j*16+3],
                     key_data[j*16+4],key_data[j*16+5],key_data[j*16+6],key_data[j*16+7],
                     key_data[j*16+8],key_data[j*16+9],key_data[j*16+10],key_data[j*16+11],
                     key_data[j*16+12],key_data[j*16+13],key_data[j*16+14],key_data[j*16+15]);
        }
        DHTOOLS_LOG_DEBUG("key_mask:\n");
        for(j=0;j<80/16;j++)
        {
            DHTOOLS_LOG_DEBUG("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", 
                     key_mask[j*16],key_mask[j*16+1],key_mask[j*16+2],key_mask[j*16+3],
                     key_mask[j*16+4],key_mask[j*16+5],key_mask[j*16+6],key_mask[j*16+7],
                     key_mask[j*16+8],key_mask[j*16+9],key_mask[j*16+10],key_mask[j*16+11],
                     key_mask[j*16+12],key_mask[j*16+13],key_mask[j*16+14],key_mask[j*16+15]);
        }
        DHTOOLS_LOG_DEBUG("as rst:");
        DHTOOLS_LOG_DEBUG("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", 
                as_rst[0],as_rst[1],as_rst[2],as_rst[3],as_rst[4],as_rst[5],as_rst[6],as_rst[7],
                as_rst[8],as_rst[9],as_rst[10],as_rst[11],as_rst[12],as_rst[13],as_rst[14],as_rst[15]);
    }
}

static int32_t zxdh_csig_tag_add_rule(DPP_PF_INFO_T *pf_info, ZXDH_TOOLS_CSIG_TAG_MSG_T *csig_tag_msg, void *msg_reps)
{
    uint32_t ret = 0;
    uint32_t acl_index = 0;
    uint32_t entry_num = 0;
    uint32_t index_num = 0;
    uint32_t max_index_num = 0;
    uint32_t i = 0;
    uint32_t sdt_no = ZXDH_SDT_CSIG_TAG_TABLE;
    uint32_t rsp_len = 0;
    uint32_t *p_acl_index_list = NULL;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;
    ZXDH_TOOLS_ACL_TBL_T *p_csig_tag_rule = NULL;

    if(IS_ERR_OR_NULL(pf_info) || IS_ERR_OR_NULL(csig_tag_msg) 
       || IS_ERR_OR_NULL(msg_reps) || !IS_PF(pf_info->vport))
    {
        DHTOOLS_LOG_ERR("null point or vf!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM; 
    }

    p_csig_tag_rule = (ZXDH_TOOLS_ACL_TBL_T *)csig_tag_msg->payload;
    if(IS_ERR(p_csig_tag_rule))
    {
        DHTOOLS_LOG_ERR("rcv add rule msg failed!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM;
    }

    entry_num = p_csig_tag_rule->item_num;
    if(csig_tag_msg->payload_len != sizeof(ZXDH_TOOLS_ACL_TBL_T)+entry_num*sizeof(ZXDH_TOOLS_ACL_ITEM_INFO_T))
    {
        DHTOOLS_LOG_ERR("csig tag mag payload len(%u) error!!!\n",csig_tag_msg->payload_len);
        return MSG_CSIG_TAG_ERR_PARAM;
    }

    ret = dpp_acl_index_max_num(pf_info,sdt_no,&max_index_num);
    if(ret)
    {
        DHTOOLS_LOG_ERR("get max entry num fail!!!\n");
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    if(entry_num > max_index_num)
    {
        DHTOOLS_LOG_ERR("entry_num(%u) beyond max num(%u)!!!\n", entry_num, max_index_num);
        return MSG_CSIG_TAG_ERR_PARAM ;
    }

    p_acl_index_list = kzalloc(entry_num * sizeof(uint32_t), GFP_KERNEL);
    if (unlikely(NULL == p_acl_index_list))
    {
        DHTOOLS_LOG_ERR("failed to kzalloc\n");
        return MSG_CSIG_TAG_NULL_POINT;
    }
    
    for(i=0; i<entry_num; i++)
    {
        ret = dpp_acl_index_request(pf_info, sdt_no, &acl_index);
        if(ret)
        {
            ret = MSG_CSIG_TAG_ACL_REQ_FAIL;
            DHTOOLS_LOG_ERR("dpp_acl_index_request fail!\n");
            goto err_free_index;
        }
        index_num++;
        p_acl_index_list[i] = acl_index;
        p_csig_tag_rule->item_info[i].handle = acl_index;
    }

    p_acl_entry_info = (DPP_ACL_ENTRY_INFO_T *)kzalloc(sizeof(DPP_ACL_ENTRY_INFO_T) * entry_num, GFP_KERNEL);
    if (unlikely(NULL == p_acl_entry_info))
    {
        ret  = MSG_CSIG_TAG_NULL_POINT;
        DHTOOLS_LOG_ERR("failed to kzalloc acl entry!!\n");
        goto err_free_index;
    }

    for(i=0; i<entry_num; i++)
    {
        p_acl_entry_info[i].handle = p_csig_tag_rule->item_info[i].handle;
        p_acl_entry_info[i].key_data = p_csig_tag_rule->item_info[i].key_data;
        p_acl_entry_info[i].key_mask = p_csig_tag_rule->item_info[i].key_mask;
        p_acl_entry_info[i].p_as_rslt = p_csig_tag_rule->item_info[i].as_rst;
    }

    ret = dpp_acl_entry_add(pf_info,  sdt_no, entry_num, p_acl_entry_info);
    if(ret)
    {
        DHTOOLS_LOG_ERR("dpp_acl_entry_add failed!!!\n");
        ret = MSG_CSIG_TAG_ACL_ADD_FAIL;
        goto err_free_entry;
    }

    for(i=0; i<entry_num; i++)
    {
        p_csig_tag_rule->item_info[i].handle = p_acl_entry_info[i].handle;
    }

    rsp_len = sizeof(ZXDH_TOOLS_ACL_TBL_T) + entry_num*sizeof(ZXDH_TOOLS_ACL_ITEM_INFO_T);
    if (unlikely(copy_to_user((uint8_t __user *)msg_reps, p_csig_tag_rule, rsp_len)))
    {
        DHTOOLS_LOG_ERR("copy_to_user failed!!!\n");
        ret = MSG_CSIG_TAG_COMMON_ERR;
        goto err_free_entry;
    }

    print_csig_tag_msg(csig_tag_msg->op_code, p_csig_tag_rule);

    kfree(p_acl_entry_info);
    kfree(p_acl_index_list);
    return MSG_CSIG_TAG_OK;

err_free_entry:
     kfree(p_acl_entry_info);
err_free_index:
    for(i=0;i<index_num;i++)
    {
        dpp_acl_index_release(pf_info, ZXDH_SDT_CSIG_TAG_TABLE, p_acl_index_list[i]);
    }
    kfree(p_acl_index_list);
    return ret;
}

static int32_t zxdh_csig_tag_del_rule(DPP_PF_INFO_T *pf_info, ZXDH_TOOLS_CSIG_TAG_MSG_T *csig_tag_msg, void *msg_reps)
{
    uint32_t ret = 0;
    uint32_t entry_num = 0;
    uint32_t index = 0;
    uint32_t rsp_len = 0;
    uint32_t max_index_num = 0;
    uint32_t sdt_no = ZXDH_SDT_CSIG_TAG_TABLE;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;
    ZXDH_TOOLS_ACL_TBL_T *p_csig_tag_rule = NULL;

    if(IS_ERR_OR_NULL(pf_info) || IS_ERR_OR_NULL(csig_tag_msg) 
       || IS_ERR_OR_NULL(msg_reps) || !IS_PF(pf_info->vport))
    {
        DHTOOLS_LOG_ERR("null point or vf!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM; 
    }

    p_csig_tag_rule = (ZXDH_TOOLS_ACL_TBL_T *)csig_tag_msg->payload;
    if(IS_ERR_OR_NULL(p_csig_tag_rule))
    {
        DHTOOLS_LOG_ERR("rcv rel rule msg failed!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM;
    }

    entry_num = p_csig_tag_rule->item_num;
    if(csig_tag_msg->payload_len != sizeof(ZXDH_TOOLS_ACL_TBL_T)+entry_num*sizeof(ZXDH_TOOLS_ACL_ITEM_INFO_T))
    {
        DHTOOLS_LOG_ERR("csig tag mag payload len(%u) error!!!\n",csig_tag_msg->payload_len);
        return MSG_CSIG_TAG_ERR_PARAM;
    }

    ret = dpp_acl_index_max_num(pf_info,sdt_no,&max_index_num);
    if(ret)
    {
        DHTOOLS_LOG_ERR("get max entry num fail!!!\n");
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    if(entry_num > max_index_num)
    {
        DHTOOLS_LOG_ERR("entry_num(%u) beyond max num(%u)!!!\n", entry_num, max_index_num);
        return MSG_CSIG_TAG_ERR_PARAM ;
    }

    p_acl_entry_info = (DPP_ACL_ENTRY_INFO_T *)kzalloc(sizeof(DPP_ACL_ENTRY_INFO_T) * entry_num, GFP_KERNEL);
    if (unlikely(NULL == p_acl_entry_info))
    {
        DHTOOLS_LOG_ERR("failed to kzalloc acl entry!!\n");
        return MSG_CSIG_TAG_NULL_POINT;
    }

    for(index=0; index<entry_num; index++)
    {
        p_acl_entry_info[index].handle = p_csig_tag_rule->item_info[index].handle;
        p_acl_entry_info[index].key_data = p_csig_tag_rule->item_info[index].key_data;
        p_acl_entry_info[index].key_mask = p_csig_tag_rule->item_info[index].key_mask;
        p_acl_entry_info[index].p_as_rslt = p_csig_tag_rule->item_info[index].as_rst;
    }

    ret = dpp_acl_entry_del_by_key(pf_info, ZXDH_SDT_CSIG_TAG_TABLE, entry_num, p_acl_entry_info);
    if(ret)
    {
        DHTOOLS_LOG_ERR("failed to del acl entry!!\n");
        kfree(p_acl_entry_info);
        return MSG_CSIG_TAG_ACL_REL_FAIL;
    }

    for(index=0; index<entry_num; index++)
    {
        p_csig_tag_rule->item_info[index].handle = p_acl_entry_info[index].handle;
    }
    
    rsp_len = sizeof(ZXDH_TOOLS_ACL_TBL_T) + entry_num*sizeof(ZXDH_TOOLS_ACL_ITEM_INFO_T);
    if (unlikely(copy_to_user((uint8_t __user *)msg_reps, p_csig_tag_rule, rsp_len)))
    {
        DHTOOLS_LOG_ERR("rsp msg(len=%u) fail!!\n", rsp_len);
        kfree(p_acl_entry_info);
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    print_csig_tag_msg(csig_tag_msg->op_code, p_csig_tag_rule);
    
    kfree(p_acl_entry_info);
    return MSG_CSIG_TAG_OK;
}

static int32_t zxdh_csig_tag_del_rule_by_index(DPP_PF_INFO_T *pf_info, ZXDH_TOOLS_CSIG_TAG_MSG_T *csig_tag_msg, void *msg_reps)
{
    uint32_t ret = 0;
    uint32_t entry_num = 0;
    uint32_t index = 0;
    uint32_t rsp_len = 0;
    uint32_t max_index_num = 0;
    uint32_t sdt_no = ZXDH_SDT_CSIG_TAG_TABLE;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;
    ZXDH_TOOLS_ACL_TBL_T *p_csig_tag_rule = NULL;

    if(IS_ERR_OR_NULL(pf_info) || IS_ERR_OR_NULL(csig_tag_msg) 
       || IS_ERR_OR_NULL(msg_reps) || !IS_PF(pf_info->vport))
    {
        DHTOOLS_LOG_ERR("null point or vf!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM; 
    }

    p_csig_tag_rule = (ZXDH_TOOLS_ACL_TBL_T *)csig_tag_msg->payload;
    if(IS_ERR_OR_NULL(p_csig_tag_rule))
    {
        DHTOOLS_LOG_ERR("rcv rel rule msg failed!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM;
    }

    entry_num = p_csig_tag_rule->item_num;
    if(csig_tag_msg->payload_len != sizeof(ZXDH_TOOLS_ACL_TBL_T)+entry_num*sizeof(ZXDH_TOOLS_ACL_ITEM_INFO_T))
    {
        DHTOOLS_LOG_ERR("csig tag mag payload len(%u) error!!!\n",csig_tag_msg->payload_len);
        return MSG_CSIG_TAG_ERR_PARAM;
    }

    ret = dpp_acl_index_max_num(pf_info,sdt_no,&max_index_num);
    if(ret)
    {
        DHTOOLS_LOG_ERR("get max entry num fail!!!\n");
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    if(entry_num > max_index_num)
    {
        DHTOOLS_LOG_ERR("entry_num(%u) beyond max num(%u)!!!\n", entry_num, max_index_num);
        return MSG_CSIG_TAG_ERR_PARAM ;
    }

    p_acl_entry_info = (DPP_ACL_ENTRY_INFO_T *)kzalloc(sizeof(DPP_ACL_ENTRY_INFO_T) * entry_num, GFP_KERNEL);
    if (unlikely(NULL == p_acl_entry_info))
    {
        DHTOOLS_LOG_ERR("failed to kzalloc acl entry!!\n");
        return MSG_CSIG_TAG_NULL_POINT;
    }

    for(index=0; index<entry_num; index++)
    {
        p_acl_entry_info[index].handle = p_csig_tag_rule->item_info[index].handle;
        p_acl_entry_info[index].key_data = p_csig_tag_rule->item_info[index].key_data;
        p_acl_entry_info[index].key_mask = p_csig_tag_rule->item_info[index].key_mask;
        p_acl_entry_info[index].p_as_rslt = p_csig_tag_rule->item_info[index].as_rst;
    }

    ret = dpp_acl_entry_del_by_index(pf_info, ZXDH_SDT_CSIG_TAG_TABLE, entry_num, p_acl_entry_info);
    if(ret)
    {
        DHTOOLS_LOG_ERR("failed to del acl entry!!\n");
        kfree(p_acl_entry_info);
        return MSG_CSIG_TAG_ACL_REL_FAIL;
    }

    for(index=0; index<entry_num; index++)
    {
        p_csig_tag_rule->item_info[index].handle = p_acl_entry_info[index].handle;
    }
    
    rsp_len = sizeof(ZXDH_TOOLS_ACL_TBL_T) + entry_num*sizeof(ZXDH_TOOLS_ACL_ITEM_INFO_T);
    if (unlikely(copy_to_user((uint8_t __user *)msg_reps, p_csig_tag_rule, rsp_len)))
    {
        DHTOOLS_LOG_ERR("rsp msg(len=%u) fail!!\n", rsp_len);
        kfree(p_acl_entry_info);
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    print_csig_tag_msg(csig_tag_msg->op_code, p_csig_tag_rule);
    
    kfree(p_acl_entry_info);
    return MSG_CSIG_TAG_OK;
}

static int32_t zxdh_csig_tag_srh_rule(DPP_PF_INFO_T *pf_info, ZXDH_TOOLS_CSIG_TAG_MSG_T *csig_tag_msg, void *msg_reps)
{
    uint32_t ret = 0;
    uint32_t entry_num = 0;
    uint32_t index = 0;
    uint32_t rsp_len = 0;
    uint32_t max_index_num = 0;
    uint32_t sdt_no = ZXDH_SDT_CSIG_TAG_TABLE;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;
    ZXDH_TOOLS_ACL_TBL_T *p_csig_tag_rule = NULL;

    if(IS_ERR_OR_NULL(pf_info) || IS_ERR_OR_NULL(csig_tag_msg) 
       || IS_ERR_OR_NULL(msg_reps) || !IS_PF(pf_info->vport))
    {
        DHTOOLS_LOG_ERR("null point or vf!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM; 
    }

    p_csig_tag_rule = (ZXDH_TOOLS_ACL_TBL_T *)csig_tag_msg->payload;
    if(IS_ERR_OR_NULL(p_csig_tag_rule))
    {
        DHTOOLS_LOG_ERR("rcv srh rule msg failed!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM;
    }

    entry_num = p_csig_tag_rule->item_num;
    if(csig_tag_msg->payload_len != sizeof(ZXDH_TOOLS_ACL_TBL_T)+entry_num*sizeof(ZXDH_TOOLS_ACL_ITEM_INFO_T))
    {
        DHTOOLS_LOG_ERR("csig tag mag payload len(%u) error!!!\n",csig_tag_msg->payload_len);
        return MSG_CSIG_TAG_ERR_PARAM;
    }

    ret = dpp_acl_index_max_num(pf_info,sdt_no,&max_index_num);
    if(ret)
    {
        DHTOOLS_LOG_ERR("get max entry num fail!!!\n");
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    if(entry_num > max_index_num)
    {
        DHTOOLS_LOG_ERR("entry_num(%u) beyond max num(%u)!!!\n", entry_num, max_index_num);
        return MSG_CSIG_TAG_ERR_PARAM ;
    }

    p_acl_entry_info = (DPP_ACL_ENTRY_INFO_T *)kzalloc(sizeof(DPP_ACL_ENTRY_INFO_T) * entry_num, GFP_KERNEL);
    if (unlikely(NULL == p_acl_entry_info))
    {
        DHTOOLS_LOG_ERR("failed to kzalloc acl entry!!\n");
        return MSG_CSIG_TAG_NULL_POINT;
    }

    for(index=0; index<entry_num; index++)
    {
        p_acl_entry_info[index].handle = p_csig_tag_rule->item_info[index].handle;
        p_acl_entry_info[index].key_data = p_csig_tag_rule->item_info[index].key_data;
        p_acl_entry_info[index].key_mask = p_csig_tag_rule->item_info[index].key_mask;
        p_acl_entry_info[index].p_as_rslt = p_csig_tag_rule->item_info[index].as_rst;
    }

    ret = dpp_acl_entry_search_by_key(pf_info, sdt_no, entry_num, p_acl_entry_info);
    if(ret)
    {
        DHTOOLS_LOG_ERR("failed to search acl entry!!\n");
        kfree(p_acl_entry_info);
        return MSG_CSIG_TAG_ACL_SRH_FAIL;
    }

    for(index=0; index<entry_num; index++)
    {
        p_csig_tag_rule->item_info[index].handle = p_acl_entry_info[index].handle;
    }

    rsp_len = sizeof(ZXDH_TOOLS_ACL_TBL_T) + entry_num*sizeof(ZXDH_TOOLS_ACL_ITEM_INFO_T);
    if (unlikely(copy_to_user((uint8_t __user *)msg_reps, p_csig_tag_rule, rsp_len)))
    {
        DHTOOLS_LOG_ERR("rsp msg(len=%u) fail!!\n", rsp_len);
        kfree(p_acl_entry_info);
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    print_csig_tag_msg(csig_tag_msg->op_code, p_csig_tag_rule);
    
    kfree(p_acl_entry_info);

    return MSG_CSIG_TAG_OK;
}

static int32_t zxdh_csig_tag_srh_rule_by_index(DPP_PF_INFO_T *pf_info, ZXDH_TOOLS_CSIG_TAG_MSG_T *csig_tag_msg, void *msg_reps)
{
    uint32_t ret = 0;
    uint32_t entry_num = 0;
    uint32_t index = 0;
    uint32_t rsp_len = 0;
    uint32_t max_index_num = 0;
    uint32_t sdt_no = ZXDH_SDT_CSIG_TAG_TABLE;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;
    ZXDH_TOOLS_ACL_TBL_T *p_csig_tag_rule = NULL;

    if(IS_ERR_OR_NULL(pf_info) || IS_ERR_OR_NULL(csig_tag_msg) 
       || IS_ERR_OR_NULL(msg_reps) || !IS_PF(pf_info->vport))
    {
        DHTOOLS_LOG_ERR("null point or vf!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM; 
    }

    p_csig_tag_rule = (ZXDH_TOOLS_ACL_TBL_T *)csig_tag_msg->payload;
    if(IS_ERR_OR_NULL(p_csig_tag_rule))
    {
        DHTOOLS_LOG_ERR("rcv srh rule msg failed!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM;
    }

    entry_num = p_csig_tag_rule->item_num;
    if(csig_tag_msg->payload_len != sizeof(ZXDH_TOOLS_ACL_TBL_T)+entry_num*sizeof(ZXDH_TOOLS_ACL_ITEM_INFO_T))
    {
        DHTOOLS_LOG_ERR("csig tag mag payload len(%u) error!!!\n",csig_tag_msg->payload_len);
        return MSG_CSIG_TAG_ERR_PARAM;
    }

    ret = dpp_acl_index_max_num(pf_info,sdt_no,&max_index_num);
    if(ret)
    {
        DHTOOLS_LOG_ERR("get max entry num fail!!!\n");
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    if(entry_num > max_index_num)
    {
        DHTOOLS_LOG_ERR("entry_num(%u) beyond max num(%u)!!!\n", entry_num, max_index_num);
        return MSG_CSIG_TAG_ERR_PARAM ;
    }

    p_acl_entry_info = (DPP_ACL_ENTRY_INFO_T *)kzalloc(sizeof(DPP_ACL_ENTRY_INFO_T) * entry_num, GFP_KERNEL);
    if (unlikely(NULL == p_acl_entry_info))
    {
        DHTOOLS_LOG_ERR("failed to kzalloc acl entry!!\n");
        return MSG_CSIG_TAG_NULL_POINT;
    }

    for(index=0; index<entry_num; index++)
    {
        p_acl_entry_info[index].handle = p_csig_tag_rule->item_info[index].handle;
        p_acl_entry_info[index].key_data = p_csig_tag_rule->item_info[index].key_data;
        p_acl_entry_info[index].key_mask = p_csig_tag_rule->item_info[index].key_mask;
        p_acl_entry_info[index].p_as_rslt = p_csig_tag_rule->item_info[index].as_rst;
    }

    ret = dpp_acl_entry_search_by_index(pf_info, sdt_no, entry_num, p_acl_entry_info);
    if(ret)
    {
        DHTOOLS_LOG_ERR("failed to search acl entry!!\n");
        kfree(p_acl_entry_info);
        return MSG_CSIG_TAG_ACL_SRH_FAIL;
    }

    for(index=0; index<entry_num; index++)
    {
        p_csig_tag_rule->item_info[index].handle = p_acl_entry_info[index].handle;
    }

    rsp_len = sizeof(ZXDH_TOOLS_ACL_TBL_T) + entry_num*sizeof(ZXDH_TOOLS_ACL_ITEM_INFO_T);
    if (unlikely(copy_to_user((uint8_t __user *)msg_reps, p_csig_tag_rule, rsp_len)))
    {
        DHTOOLS_LOG_ERR("rsp msg(len=%u) fail!!\n", rsp_len);
        kfree(p_acl_entry_info);
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    print_csig_tag_msg(csig_tag_msg->op_code, p_csig_tag_rule);
    
    kfree(p_acl_entry_info);

    return MSG_CSIG_TAG_OK;
}

static int32_t zxdh_csig_tag_dump_rule(DPP_PF_INFO_T *pf_info, ZXDH_TOOLS_CSIG_TAG_MSG_T *csig_tag_msg, void *msg_reps)
{
    uint32_t ret = 0;
    uint32_t max_index_num = 0;
    uint32_t index = 0;
    uint32_t dump_len = 0;
    uint32_t sdt_no = ZXDH_SDT_CSIG_TAG_TABLE;
    uint32_t dump_entry_num = 0;
    ZXDH_TOOLS_ACL_TBL_T *acl_tbl_dump = NULL;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;

    if(IS_ERR_OR_NULL(pf_info) || IS_ERR_OR_NULL(csig_tag_msg) 
       || IS_ERR_OR_NULL(msg_reps) || !IS_PF(pf_info->vport))
    {
        DHTOOLS_LOG_ERR("null point or vf!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM; 
    }

    ret = dpp_acl_index_max_num(pf_info,sdt_no,&max_index_num);
    if(ret)
    {
        DHTOOLS_LOG_ERR("get max entry num fail!!!\n");
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    dump_len = sizeof(ZXDH_TOOLS_ACL_TBL_T) + max_index_num*sizeof(ZXDH_TOOLS_ACL_ITEM_INFO_T);
    acl_tbl_dump = (ZXDH_TOOLS_ACL_TBL_T *)vzalloc(dump_len);
    if (unlikely(NULL == acl_tbl_dump))
    {
        DHTOOLS_LOG_ERR("failed to malloc dump mem!!\n");
        return MSG_CSIG_TAG_NULL_POINT;
    }

    p_acl_entry_info = (DPP_ACL_ENTRY_INFO_T *)kzalloc(sizeof(DPP_ACL_ENTRY_INFO_T) * max_index_num, GFP_KERNEL);
    if (unlikely(NULL == p_acl_entry_info))
    {
        DHTOOLS_LOG_ERR("failed to kzalloc acl entry!!\n");
        vfree(acl_tbl_dump);
        return MSG_CSIG_TAG_NULL_POINT;
    }

    for(index=0;index<max_index_num;index++)
    {
        p_acl_entry_info[index].key_data = acl_tbl_dump->item_info[index].key_data;
        p_acl_entry_info[index].key_mask = acl_tbl_dump->item_info[index].key_mask;
        p_acl_entry_info[index].p_as_rslt = acl_tbl_dump->item_info[index].as_rst;
    }

    ret = dpp_acl_entry_dump_by_vport(pf_info, ZXDH_SDT_CSIG_TAG_TABLE, &dump_entry_num,p_acl_entry_info);
    if(ret)
    {
        DHTOOLS_LOG_ERR("dump entry fail!!!\n");
        vfree(acl_tbl_dump);
        kfree(p_acl_entry_info);
        return MSG_CSIG_TAG_ACL_DUMP_FAIL;
    }

    acl_tbl_dump->item_num = dump_entry_num;
    for(index=0;index<max_index_num;index++)
    {
        acl_tbl_dump->item_info[index].handle = p_acl_entry_info[index].handle;
    }

    if (unlikely(copy_to_user((uint8_t __user *)msg_reps, acl_tbl_dump, dump_len)))
    {
        DHTOOLS_LOG_ERR("rsp msg(len=%u) fail!!\n", dump_len);
        vfree(acl_tbl_dump);
        kfree(p_acl_entry_info);
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    print_csig_tag_msg(csig_tag_msg->op_code, acl_tbl_dump);
    vfree(acl_tbl_dump);
    kfree(p_acl_entry_info);

    return MSG_CSIG_TAG_OK;
}

static int32_t zxdh_csig_tag_dump_rule_all(DPP_PF_INFO_T *pf_info, ZXDH_TOOLS_CSIG_TAG_MSG_T *csig_tag_msg, void *msg_reps)
{
    uint32_t ret = 0;
    uint32_t max_index_num = 0;
    uint32_t index = 0;
    uint32_t dump_len = 0;
    uint32_t sdt_no = ZXDH_SDT_CSIG_TAG_TABLE;
    uint32_t dump_entry_num = 0;
    ZXDH_TOOLS_ACL_TBL_T *acl_tbl_dump = NULL;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;

    if(IS_ERR_OR_NULL(pf_info) || IS_ERR_OR_NULL(csig_tag_msg) 
       || IS_ERR_OR_NULL(msg_reps) || !IS_PF(pf_info->vport))
    {
        DHTOOLS_LOG_ERR("null point or vf!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM; 
    }

    ret = dpp_acl_index_max_num(pf_info,sdt_no,&max_index_num);
    if(ret)
    {
        DHTOOLS_LOG_ERR("get max entry num fail!!!\n");
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    dump_len = sizeof(ZXDH_TOOLS_ACL_TBL_T) + max_index_num*sizeof(ZXDH_TOOLS_ACL_ITEM_INFO_T);
    acl_tbl_dump = (ZXDH_TOOLS_ACL_TBL_T *)vzalloc(dump_len);
    if (unlikely(NULL == acl_tbl_dump))
    {
        DHTOOLS_LOG_ERR("failed to malloc dump mem!!\n");
        return MSG_CSIG_TAG_NULL_POINT;
    }

    p_acl_entry_info = (DPP_ACL_ENTRY_INFO_T *)kzalloc(sizeof(DPP_ACL_ENTRY_INFO_T) * max_index_num, GFP_KERNEL);
    if (unlikely(NULL == p_acl_entry_info))
    {
        DHTOOLS_LOG_ERR("failed to kzalloc acl entry!!\n");
        vfree(acl_tbl_dump);
        return MSG_CSIG_TAG_NULL_POINT;
    }

    for(index=0;index<max_index_num;index++)
    {
        p_acl_entry_info[index].key_data = acl_tbl_dump->item_info[index].key_data;
        p_acl_entry_info[index].key_mask = acl_tbl_dump->item_info[index].key_mask;
        p_acl_entry_info[index].p_as_rslt = acl_tbl_dump->item_info[index].as_rst;
    }

    ret = dpp_acl_entry_dump_all(pf_info, ZXDH_SDT_CSIG_TAG_TABLE, &dump_entry_num,p_acl_entry_info);
    if(ret)
    {
        DHTOOLS_LOG_ERR("dump entry fail!!!\n");
        vfree(acl_tbl_dump);
        kfree(p_acl_entry_info);
        return MSG_CSIG_TAG_ACL_DUMP_FAIL;
    }

    acl_tbl_dump->item_num = dump_entry_num;
    for(index=0;index<max_index_num;index++)
    {
        acl_tbl_dump->item_info[index].handle = p_acl_entry_info[index].handle;
    }

    if (unlikely(copy_to_user((uint8_t __user *)msg_reps, acl_tbl_dump, dump_len)))
    {
        DHTOOLS_LOG_ERR("rsp msg(len=%u) fail!!\n", dump_len);
        vfree(acl_tbl_dump);
        kfree(p_acl_entry_info);
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    print_csig_tag_msg(csig_tag_msg->op_code, acl_tbl_dump);
    vfree(acl_tbl_dump);
    kfree(p_acl_entry_info);

    return MSG_CSIG_TAG_OK;
}

static int32_t zxdh_csig_tag_flush_rule(DPP_PF_INFO_T *pf_info, ZXDH_TOOLS_CSIG_TAG_MSG_T *csig_tag_msg, void *msg_reps)
{
    uint32_t ret = 0;
    uint32_t sdt_no = ZXDH_SDT_CSIG_TAG_TABLE;

    if(IS_ERR_OR_NULL(pf_info) || IS_ERR_OR_NULL(csig_tag_msg) || !IS_PF(pf_info->vport))
    {
        DHTOOLS_LOG_ERR("null point or vf!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM; 
    }

    ret = dpp_acl_entry_flush_by_vport(pf_info, sdt_no);
    if(ret)
    {
        DHTOOLS_LOG_ERR("flush vport(0x%x) acl fail,ret=0x%x!!\n",pf_info->vport,ret);
        return MSG_CSIG_TAG_ACL_FLUSH_FAIL; 
    }

    return MSG_CSIG_TAG_OK;
}

static int32_t zxdh_csig_tag_flush_rule_all(DPP_PF_INFO_T *pf_info, ZXDH_TOOLS_CSIG_TAG_MSG_T *csig_tag_msg, void *msg_reps)
{
    uint32_t ret = 0;
    uint32_t sdt_no = ZXDH_SDT_CSIG_TAG_TABLE;

    if(IS_ERR_OR_NULL(pf_info) || IS_ERR_OR_NULL(csig_tag_msg) || !IS_PF(pf_info->vport))
    {
        DHTOOLS_LOG_ERR("null point or vf!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM; 
    }

    ret = dpp_acl_entry_flush_all(pf_info, sdt_no);
    if(ret)
    {
        DHTOOLS_LOG_ERR("flush all acl fail,ret=0x%x!!\n",ret);
        return MSG_CSIG_TAG_ACL_FLUSH_FAIL; 
    }

    return MSG_CSIG_TAG_OK;
}

static int32_t zxdh_csig_tag_get_acl_info(DPP_PF_INFO_T *pf_info, ZXDH_TOOLS_CSIG_TAG_MSG_T *csig_tag_msg, void *msg_reps)
{
    uint32_t ret = 0;
    uint32_t sdt_entry_num = 0;
    uint32_t free_index_num = 0;
    uint32_t sdt_no = ZXDH_SDT_CSIG_TAG_TABLE;
    ZXDH_TOOLS_ACL_INFO_T acl_info = {0};

    if(IS_ERR_OR_NULL(pf_info) || IS_ERR_OR_NULL(csig_tag_msg) 
       || IS_ERR_OR_NULL(msg_reps) || !IS_PF(pf_info->vport))
    {
        DHTOOLS_LOG_ERR("null point or vf!!!\n");
        return MSG_CSIG_TAG_ERR_PARAM; 
    }

    ret = dpp_acl_index_max_num(pf_info, sdt_no, &sdt_entry_num);
    if(ret)
    {
        DHTOOLS_LOG_ERR("get acl info fail,ret=0x%x!!\n",ret);
        return MSG_CSIG_TAG_COMMON_ERR; 
    }

    ret = dpp_acl_index_unused_num(pf_info, sdt_no, &free_index_num);
    if(ret)
    {
        DHTOOLS_LOG_ERR("get free index fail,ret=0x%x!!\n",ret);
        return MSG_CSIG_TAG_COMMON_ERR; 
    }

    acl_info.acl_depth = sdt_entry_num;
    acl_info.acl_used_num = (sdt_entry_num > free_index_num) ? (sdt_entry_num - free_index_num) : 0;
    if (unlikely(copy_to_user((uint8_t __user *)msg_reps, &acl_info, sizeof(acl_info))))
    {
        DHTOOLS_LOG_ERR("copy rsp msg fail!!\n");
        return MSG_CSIG_TAG_COMMON_ERR;
    }

    return MSG_CSIG_TAG_OK;
}

ZXDH_CSIG_TAG_MSG_OPCODE_CALLBACK_T zxdh_tools_csig_tag_callback[] = {
    {MSG_CSIG_TAG_FLAG,              zxdh_csig_tag_flag},
    {MSG_CSIG_TAG_ADD_RULE,          zxdh_csig_tag_add_rule},
    {MSG_CSIG_TAG_DEL_RULE,          zxdh_csig_tag_del_rule},
    {MSG_CSIG_TAG_SRH_RULE,          zxdh_csig_tag_srh_rule},
    {MSG_CSIG_TAG_DUMP_RULE,         zxdh_csig_tag_dump_rule},
    {MSG_CSIG_TAG_FLUSH_RULE,        zxdh_csig_tag_flush_rule},
    {MSG_CSIG_TAG_GET_ACL_INFO,      zxdh_csig_tag_get_acl_info},
    {MSG_CSIG_TAG_DEL_RULE_BY_INDEX, zxdh_csig_tag_del_rule_by_index},
    {MSG_CSIG_TAG_SRH_RULE_BY_INDEX, zxdh_csig_tag_srh_rule_by_index},
    {MSG_CSIG_TAG_DUMP_RULE_ALL,     zxdh_csig_tag_dump_rule_all},
    {MSG_CSIG_TAG_FLUSH_RULE_ALL,    zxdh_csig_tag_flush_rule_all},
};

uint32_t g_csig_tag_callback_size = ARRAY_SIZE(zxdh_tools_csig_tag_callback);

int32_t zxdh_tools_csig_tag(struct net_device *netdev, struct ifreq *ifr)
{
    uint32_t i = 0;
    int32_t ret = 0;
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    DPP_PF_INFO_T pf_info = {0};
    struct zxdh_tools_msg *tools_msg = NULL;
    struct zxdh_tools_reps tools_reps = {0};
    ZXDH_TOOLS_CSIG_TAG_MSG_T *csig_tag_msg = NULL;

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

    tools_msg = (struct zxdh_tools_msg *)kzalloc(sizeof(struct zxdh_tools_msg), GFP_KERNEL);
    if (tools_msg == NULL)
    {
        DHTOOLS_LOG_ERR("kzalloc tools_msg failed!!!\n");
        return -EFAULT;
    }

    if (unlikely(copy_from_user(tools_msg, ifr->ifr_ifru.ifru_data, sizeof(struct zxdh_tools_msg))))
    {
        DHTOOLS_LOG_ERR("copy_from_user failed!!!\n");
        kfree(tools_msg);
        return -EFAULT;
    }

    if (tools_msg->payload_len == 0 || tools_msg->payload_len > BAR_MSG_PAYLOAD_MAX_LEN)
    {
        DHTOOLS_LOG_ERR("tools_msg payload failed!!!\n");
        kfree(tools_msg);
        return -EINVAL;
    }

    csig_tag_msg = (ZXDH_TOOLS_CSIG_TAG_MSG_T *)kzalloc(tools_msg->payload_len, GFP_KERNEL);
    if (csig_tag_msg == NULL)
    {
        DHTOOLS_LOG_ERR("kzalloc tbl_msg failed!!!\n");
        kfree(tools_msg);
        return -ENOMEM;
    }

    if (unlikely(copy_from_user(csig_tag_msg, (uint8_t *)(ifr->ifr_ifru.ifru_data) + sizeof(struct zxdh_tools_msg), tools_msg->payload_len)))
    {
        DHTOOLS_LOG_ERR("copy_from_user failed!!!\n");
        kfree(csig_tag_msg);
        kfree(tools_msg);
        return -EFAULT;
    }

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;
    for (i = 0; i < g_csig_tag_callback_size; i++)
    {
        if ((zxdh_tools_csig_tag_callback[i].op_code == csig_tag_msg->op_code) && (zxdh_tools_csig_tag_callback[i].callback))
        {
            ret = zxdh_tools_csig_tag_callback[i].callback(&pf_info, csig_tag_msg, tools_msg->msg_reps);
            break;
        }
    }
    tools_reps.status = (i == g_csig_tag_callback_size)? MSG_RECV_NOT_FOUND : (uint32_t)ret;

    if (unlikely(copy_to_user((void __user *)tools_msg->tools_reps, &tools_reps, sizeof(struct zxdh_tools_reps))))
    {
        DHTOOLS_LOG_ERR("copy_to_user failed!!!\n");
        kfree(csig_tag_msg);
        kfree(tools_msg);
        return -EFAULT;
    }

    kfree(csig_tag_msg);
    kfree(tools_msg);

    return 0;
}
