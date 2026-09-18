
#ifndef _DPP_DTB4K_REG_H_
#define _DPP_DTB4K_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_dtb4k_dtb_enq_cfg_queue_dtb_addr_h_0_127_t
{
    ZXIC_UINT32 cfg_queue_dtb_addr_h;
}DPP_DTB4K_DTB_ENQ_CFG_QUEUE_DTB_ADDR_H_0_127_T;

typedef struct dpp_dtb4k_dtb_enq_cfg_queue_dtb_addr_l_0_127_t
{
    ZXIC_UINT32 cfg_queue_dtb_addr_l;
}DPP_DTB4K_DTB_ENQ_CFG_QUEUE_DTB_ADDR_L_0_127_T;

typedef struct dpp_dtb4k_dtb_enq_cfg_queue_dtb_len_0_127_t
{
    ZXIC_UINT32 cfg_dtb_cmd_type;
    ZXIC_UINT32 cfg_dtb_cmd_int_en;
    ZXIC_UINT32 cfg_queue_dtb_len;
}DPP_DTB4K_DTB_ENQ_CFG_QUEUE_DTB_LEN_0_127_T;

typedef struct dpp_dtb4k_dtb_enq_info_queue_buf_space_left_0_127_t
{
    ZXIC_UINT32 info_queue_buf_space_left;
}DPP_DTB4K_DTB_ENQ_INFO_QUEUE_BUF_SPACE_LEFT_0_127_T;

typedef struct dpp_dtb4k_dtb_enq_cfg_epid_v_func_num_0_127_t
{
    ZXIC_UINT32 dbi_en;
    ZXIC_UINT32 queue_en;
    ZXIC_UINT32 cfg_epid;
    ZXIC_UINT32 cfg_vfunc_num;
    ZXIC_UINT32 cfg_vector;
    ZXIC_UINT32 cfg_func_num;
    ZXIC_UINT32 cfg_vfunc_active;
}DPP_DTB4K_DTB_ENQ_CFG_EPID_V_FUNC_NUM_0_127_T;


#ifdef __cplusplus
}
#endif
#endif

