#include "dpp_apt_se_api.h"
#include "dpp_se_api.h"
#include "dpp_se_diag.h"
#include "dpp_drv_sdt.h"
#include "dpp_drv_acl.h"

static SE_APT_ACL_CONVERT_T g_se_acl_callback[] =
{ 
    {
        ZXDH_SDT_IPSEC_ENC_TABLE,
        dpp_apt_set_ipsec_enc_data,       /** <@brief 结构体转换为码流 */
        dpp_apt_get_ipsec_enc_data,       /** <@brief 码流转换为结构体 */
    },
    { // res表
        ZXDH_SDT_CAPTURE_PKT_TABLE,
        dpp_apt_set_pkt_cap_data,       /** <@brief 结构体转换为码流 */
        dpp_apt_get_pkt_cap_data,       /** <@brief 码流转换为结构体 */
    },
};

SE_APT_ACL_CONVERT_T *se_acl_callback_get(ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 num = 0;
    num = sizeof(g_se_acl_callback)/sizeof(SE_APT_ACL_CONVERT_T);
    for(index=0;index<num;index++)
    {
        if(g_se_acl_callback[index].sdt_no == sdt_no)
        {
            return &g_se_acl_callback[index];
        }
    }
    return NULL;
}

ZXIC_UINT32 dpp_apt_set_ipsec_enc_data(ZXIC_VOID *pData, DPP_ACL_ENTRY_EX_T *aclEntry)
{
    ZXIC_UINT32 key_data = 0;
    ZXIC_UINT32 key_mask = 0;
    ZXIC_UINT32 rst = 0;

    ZXDH_IPSEC_ENC_T *ipsec_enc_table = pData;

    ZXIC_COMM_CHECK_POINT(aclEntry);
    ZXIC_COMM_CHECK_POINT(ipsec_enc_table);

    aclEntry->pri = ipsec_enc_table->index;

    if(aclEntry->key_data)
    {
        ZXIC_COMM_UINT32_WRITE_BITS(key_data, ipsec_enc_table->key.rsv1, 0, 32);
        zxic_comm_swap((ZXIC_UINT8 *)&key_data, sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_data, &key_data, sizeof(ZXIC_UINT32));

        ZXIC_COMM_UINT32_WRITE_BITS(key_data, ipsec_enc_table->key.rsv2, 0, 32);
        zxic_comm_swap((ZXIC_UINT8 *)&key_data, sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_data + 4, &key_data, sizeof(ZXIC_UINT32));

        ZXIC_COMM_MEMCPY(aclEntry->key_data + 8, ipsec_enc_table->key.sip, 16);
        ZXIC_COMM_MEMCPY(aclEntry->key_data + 24, ipsec_enc_table->key.dip, 16);
    }

    if(aclEntry->key_mask)
    {
        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, ipsec_enc_table->mask.rsv1, 0, 32);
        zxic_comm_swap((ZXIC_UINT8 *)&key_mask, sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_mask, &key_mask, sizeof(ZXIC_UINT32));

        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, ipsec_enc_table->mask.rsv2, 0, 32);
        zxic_comm_swap((ZXIC_UINT8 *)&key_mask, sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_mask + 4, &key_mask, sizeof(ZXIC_UINT32));

        ZXIC_COMM_MEMCPY(aclEntry->key_mask + 8, ipsec_enc_table->mask.sip, 16);
        ZXIC_COMM_MEMCPY(aclEntry->key_mask + 24, ipsec_enc_table->mask.dip, 16);
    }

    if(aclEntry->p_as_rslt)
    {
        ZXIC_COMM_UINT32_WRITE_BITS(rst, ipsec_enc_table->entry.hit_flag, 31, 1);
        ZXIC_COMM_UINT32_WRITE_BITS(rst, ipsec_enc_table->entry.rsv, 0, 31);
        zxic_comm_swap((ZXIC_UINT8 *)&rst, sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->p_as_rslt, &rst, sizeof(ZXIC_UINT32));

        ZXIC_COMM_UINT32_WRITE_BITS(rst, ipsec_enc_table->entry.sa_id, 0, 32);
        zxic_comm_swap((ZXIC_UINT8 *)&rst, sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->p_as_rslt + 4, &rst, sizeof(ZXIC_UINT32));
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_apt_get_ipsec_enc_data(ZXIC_VOID *pData, DPP_ACL_ENTRY_EX_T *aclEntry)
{  
    ZXDH_IPSEC_ENC_T *ipsec_enc_table = pData;

    ZXIC_COMM_CHECK_POINT(aclEntry);
    ZXIC_COMM_CHECK_POINT(ipsec_enc_table);

    ipsec_enc_table->index = aclEntry->pri;

    if(aclEntry->key_data)
    {
        zxic_comm_swap(aclEntry->key_data, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(ipsec_enc_table->key.rsv1, *(ZXIC_UINT32 *)(aclEntry->key_data), 0, 32);

        zxic_comm_swap(aclEntry->key_data + 4, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(ipsec_enc_table->key.rsv2, *(ZXIC_UINT32 *)(aclEntry->key_data + 4), 0, 32);

        ZXIC_COMM_MEMCPY(ipsec_enc_table->key.sip, aclEntry->key_data + 8, 16);
        ZXIC_COMM_MEMCPY(ipsec_enc_table->key.dip, aclEntry->key_data + 24, 16);
    }

    if(aclEntry->key_mask)
    {
        zxic_comm_swap(aclEntry->key_mask, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(ipsec_enc_table->mask.rsv1, *(ZXIC_UINT32 *)(aclEntry->key_mask), 0, 32);

        zxic_comm_swap(aclEntry->key_mask + 4, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(ipsec_enc_table->mask.rsv2, *(ZXIC_UINT32 *)(aclEntry->key_mask + 4), 0, 32);

        ZXIC_COMM_MEMCPY(ipsec_enc_table->mask.sip, aclEntry->key_mask + 8, 16);
        ZXIC_COMM_MEMCPY(ipsec_enc_table->mask.dip, aclEntry->key_mask + 24, 16);
    }

    if(aclEntry->p_as_rslt)
    {
        zxic_comm_swap(aclEntry->p_as_rslt, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(ipsec_enc_table->entry.hit_flag, *(ZXIC_UINT32 *)(aclEntry->p_as_rslt), 31, 1);
        ZXIC_COMM_UINT32_GET_BITS(ipsec_enc_table->entry.rsv, *(ZXIC_UINT32 *)(aclEntry->p_as_rslt), 0, 31);

        zxic_comm_swap(aclEntry->p_as_rslt + 4, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(ipsec_enc_table->entry.sa_id, *(ZXIC_UINT32 *)(aclEntry->p_as_rslt + 4), 0, 32);
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_apt_set_pkt_cap_data(ZXIC_VOID *pData, DPP_ACL_ENTRY_EX_T *aclEntry)
{
    ZXIC_UINT32 key_data = 0;
    ZXIC_UINT32 key_mask = 0;
    ZXIC_UINT32 rst = 0;

    ZXDH_PKT_CAP_T *pkt_cap_table = pData;

    ZXIC_COMM_CHECK_POINT(aclEntry);
    ZXIC_COMM_CHECK_POINT(pkt_cap_table);

    aclEntry->pri = pkt_cap_table->index;

    if(aclEntry->key_data)
    {
        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.rsv, 31, 1);
        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.capture_pkt_flag, 28, 3);
        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.panel_id, 24, 4);
        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.protocol, 16, 8);
        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.vqm_vfid, 0, 16);
        zxic_comm_swap((ZXIC_UINT8 *)&key_data, sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_data, &key_data, sizeof(ZXIC_UINT32));

        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.vhca_id, 16, 16);
        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.ethtype, 0, 16);
        zxic_comm_swap((ZXIC_UINT8 *)&(key_data), sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_data + 4, &key_data, sizeof(ZXIC_UINT32));
        
        ZXIC_COMM_MEMCPY(aclEntry->key_data + 8, pkt_cap_table->key.dmac, 6);
        ZXIC_COMM_MEMCPY(aclEntry->key_data + 14, pkt_cap_table->key.smac, 6);
        ZXIC_COMM_MEMCPY(aclEntry->key_data + 20, pkt_cap_table->key.sip, 16);
        ZXIC_COMM_MEMCPY(aclEntry->key_data + 36, pkt_cap_table->key.dip, 16);

        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.sport, 16, 16);
        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.dport, 0, 16);
        zxic_comm_swap((ZXIC_UINT8 *)&key_data, sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_data + 52, &key_data, sizeof(ZXIC_UINT32));

        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.qp, 8, 24);
        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.rsv, 0, 8);
        zxic_comm_swap((ZXIC_UINT8 *)&key_data, sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_data + 56, &key_data, sizeof(ZXIC_UINT32));

        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.rsv, 20, 12);
        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.key_word_len, 16, 4);
        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.rsv, 13, 3);
        ZXIC_COMM_UINT32_WRITE_BITS(key_data, pkt_cap_table->key.key_word_off, 0, 13);
        zxic_comm_swap((ZXIC_UINT8 *)&key_data, sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_data + 60, &key_data, sizeof(ZXIC_UINT32));

        ZXIC_COMM_MEMCPY(aclEntry->key_data + 64, pkt_cap_table->key.key_word, 15);

        key_data = pkt_cap_table->key.rsv;
        ZXIC_COMM_MEMCPY(aclEntry->key_data + 79, &key_data, 1);
    }

    if(aclEntry->key_mask)
    {
        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.rsv_mask, 31, 1);
        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.capture_pkt_flag_mask, 28, 3);
        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.panel_id_mask, 24, 4);
        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.protocol_mask, 16, 8);
        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.vqm_vfid_mask, 0, 16);
        zxic_comm_swap((ZXIC_UINT8 *)&key_mask, sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_mask, &key_mask, sizeof(ZXIC_UINT32));

        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.vhca_id_mask, 16, 16);
        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.ethtype_mask, 0, 16);
        zxic_comm_swap((ZXIC_UINT8 *)&(key_mask), sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_mask + 4, &key_mask, sizeof(ZXIC_UINT32));
        
        ZXIC_COMM_MEMCPY(aclEntry->key_mask + 8, pkt_cap_table->mask.dmac_mask, 6);
        ZXIC_COMM_MEMCPY(aclEntry->key_mask + 14, pkt_cap_table->mask.smac_mask, 6);
        ZXIC_COMM_MEMCPY(aclEntry->key_mask + 20, pkt_cap_table->mask.sip_mask, 16);
        ZXIC_COMM_MEMCPY(aclEntry->key_mask + 36, pkt_cap_table->mask.dip_mask, 16);

        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.sport_mask, 16, 16);
        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.dport_mask, 0, 16);
        zxic_comm_swap((ZXIC_UINT8 *)&(key_mask), sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_mask + 52, &key_mask, sizeof(ZXIC_UINT32));

        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.qp_mask, 8, 24);
        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.rsv_mask, 0, 8);
        zxic_comm_swap((ZXIC_UINT8 *)&key_mask, sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_mask + 56, &key_mask, sizeof(ZXIC_UINT32));

        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.rsv_mask, 20, 12);
        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.key_word_len_mask, 16, 4);
        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.rsv_mask, 13, 3);
        ZXIC_COMM_UINT32_WRITE_BITS(key_mask, pkt_cap_table->mask.key_word_off_mask, 0, 13);
        zxic_comm_swap((ZXIC_UINT8 *)&key_mask, sizeof(ZXIC_UINT32));
        ZXIC_COMM_MEMCPY(aclEntry->key_mask + 60, &key_mask, sizeof(ZXIC_UINT32));

        ZXIC_COMM_MEMCPY(aclEntry->key_mask + 64, pkt_cap_table->mask.key_word_mask, 15);

        key_mask = pkt_cap_table->mask.rsv_mask;
        ZXIC_COMM_MEMCPY(aclEntry->key_mask + 79, &key_mask, 1);
    }

    if(aclEntry->p_as_rslt)
    {
        ZXIC_COMM_UINT32_WRITE_BITS(rst, pkt_cap_table->entry.hit_flag, 31, 1);
        ZXIC_COMM_UINT32_WRITE_BITS(rst, pkt_cap_table->entry.value_flag, 30, 1);
        ZXIC_COMM_UINT32_WRITE_BITS(rst, 0, 24, 6);
        ZXIC_COMM_UINT32_WRITE_BITS(rst, pkt_cap_table->entry.index, 16, 8);
        ZXIC_COMM_UINT32_WRITE_BITS(rst, pkt_cap_table->entry.vqm_vfid, 0, 16);
        ZXIC_COMM_MEMCPY(aclEntry->p_as_rslt, &rst, sizeof(ZXIC_UINT32));

        rst = 0;
        ZXIC_COMM_MEMCPY(aclEntry->p_as_rslt + 4, &rst, sizeof(ZXIC_UINT32));
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_apt_get_pkt_cap_data(ZXIC_VOID *pData, DPP_ACL_ENTRY_EX_T *aclEntry)
{  
    ZXDH_PKT_CAP_T *pkt_cap_table = pData;

    ZXIC_COMM_CHECK_POINT(aclEntry);
    ZXIC_COMM_CHECK_POINT(pkt_cap_table);

    pkt_cap_table->index = aclEntry->pri;

    if(aclEntry->key_data)
    {
        zxic_comm_swap(aclEntry->key_data, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->key.capture_pkt_flag, *(ZXIC_UINT32 *)(aclEntry->key_data), 28, 3);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->key.panel_id, *(ZXIC_UINT32 *)(aclEntry->key_data), 24, 4);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->key.protocol, *(ZXIC_UINT32 *)(aclEntry->key_data), 16, 8);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->key.vqm_vfid, *(ZXIC_UINT32 *)(aclEntry->key_data), 0, 16);

        zxic_comm_swap(aclEntry->key_data + 4, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->key.vhca_id, *(ZXIC_UINT32 *)(aclEntry->key_data + 4), 16, 16);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->key.ethtype, *(ZXIC_UINT32 *)(aclEntry->key_data + 4), 0, 16);

        ZXIC_COMM_MEMCPY(pkt_cap_table->key.dmac, aclEntry->key_data + 8, 6);
        ZXIC_COMM_MEMCPY(pkt_cap_table->key.smac, aclEntry->key_data + 14, 6);
        ZXIC_COMM_MEMCPY(pkt_cap_table->key.sip, aclEntry->key_data + 20, 16);
        ZXIC_COMM_MEMCPY(pkt_cap_table->key.dip, aclEntry->key_data + 36, 16);

        zxic_comm_swap(aclEntry->key_data + 52, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->key.sport, *(ZXIC_UINT32 *)(aclEntry->key_data + 52), 16, 16);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->key.dport, *(ZXIC_UINT32 *)(aclEntry->key_data + 52), 0, 16);

        zxic_comm_swap(aclEntry->key_data + 56, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->key.qp, *(ZXIC_UINT32 *)(aclEntry->key_data + 56), 8, 24);

        zxic_comm_swap(aclEntry->key_data + 60, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->key.key_word_len, *(ZXIC_UINT32 *)(aclEntry->key_data + 60), 16, 4);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->key.key_word_off, *(ZXIC_UINT32 *)(aclEntry->key_data + 60), 0, 13);

        ZXIC_COMM_MEMCPY(pkt_cap_table->key.key_word, aclEntry->key_data + 64, 15);
    }

    if(aclEntry->key_mask)
    {
        zxic_comm_swap(aclEntry->key_mask, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->mask.capture_pkt_flag_mask, *(ZXIC_UINT32 *)(aclEntry->key_mask), 28, 3);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->mask.panel_id_mask, *(ZXIC_UINT32 *)(aclEntry->key_mask), 24, 4);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->mask.protocol_mask, *(ZXIC_UINT32 *)(aclEntry->key_mask), 16, 8);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->mask.vqm_vfid_mask, *(ZXIC_UINT32 *)(aclEntry->key_mask), 0, 16);

        zxic_comm_swap(aclEntry->key_mask + 4, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->mask.vhca_id_mask, *(ZXIC_UINT32 *)(aclEntry->key_mask + 4), 16, 16);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->mask.ethtype_mask, *(ZXIC_UINT32 *)(aclEntry->key_mask + 4), 0, 16);

        ZXIC_COMM_MEMCPY(pkt_cap_table->mask.dmac_mask, aclEntry->key_mask + 8, 6);
        ZXIC_COMM_MEMCPY(pkt_cap_table->mask.smac_mask, aclEntry->key_mask + 14, 6);
        ZXIC_COMM_MEMCPY(pkt_cap_table->mask.sip_mask, aclEntry->key_mask + 20, 16);
        ZXIC_COMM_MEMCPY(pkt_cap_table->mask.dip_mask, aclEntry->key_mask + 36, 16);

        zxic_comm_swap(aclEntry->key_mask + 52, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->mask.sport_mask, *(ZXIC_UINT32 *)(aclEntry->key_mask + 52), 16, 16);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->mask.dport_mask, *(ZXIC_UINT32 *)(aclEntry->key_mask + 52), 0, 16);

        zxic_comm_swap(aclEntry->key_mask + 56, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->mask.qp_mask, *(ZXIC_UINT32 *)(aclEntry->key_mask + 56), 8, 24);

        zxic_comm_swap(aclEntry->key_mask + 60, sizeof(ZXIC_UINT32));
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->mask.key_word_len_mask, *(ZXIC_UINT32 *)(aclEntry->key_mask + 60), 16, 4);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->mask.key_word_off_mask, *(ZXIC_UINT32 *)(aclEntry->key_mask + 60), 0, 13);

        ZXIC_COMM_MEMCPY(pkt_cap_table->mask.key_word_mask, aclEntry->key_mask + 64, 15);
    }

    if(aclEntry->p_as_rslt)
    {
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->entry.hit_flag, *(ZXIC_UINT32 *)(aclEntry->p_as_rslt), 31, 1);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->entry.value_flag, *(ZXIC_UINT32 *)(aclEntry->p_as_rslt), 30, 1);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->entry.index, *(ZXIC_UINT32 *)(aclEntry->p_as_rslt), 16, 8);
        ZXIC_COMM_UINT32_GET_BITS(pkt_cap_table->entry.vqm_vfid, *(ZXIC_UINT32 *)(aclEntry->p_as_rslt), 0, 16);
    }

    return DPP_OK;
}
