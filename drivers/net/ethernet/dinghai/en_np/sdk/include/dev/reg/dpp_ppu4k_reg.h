
#ifndef _DPP_PPU4K_REG_H_
#define _DPP_PPU4K_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_ppu4k_cluster_wr_high_data_r_mex_t
{
    ZXIC_UINT32 wr_high_data_r_mex;
}DPP_PPU4K_CLUSTER_WR_HIGH_DATA_R_MEX_T;

typedef struct dpp_ppu4k_cluster_wr_low_data_r_mex_t
{
    ZXIC_UINT32 wr_low_data_r_mex;
}DPP_PPU4K_CLUSTER_WR_LOW_DATA_R_MEX_T;

typedef struct dpp_ppu4k_cluster_addr_r_mex_t
{
    ZXIC_UINT32 operate_type;
    ZXIC_UINT32 addr_r_mex;
}DPP_PPU4K_CLUSTER_ADDR_R_MEX_T;

typedef struct dpp_ppu4k_cluster_sdt_tbl_ind_access_done_t
{
    ZXIC_UINT32 rd_addr_r_mex;
}DPP_PPU4K_CLUSTER_SDT_TBL_IND_ACCESS_DONE_T;

typedef struct dpp_ppu4k_cluster_rd_high_data_r_mex_t
{
    ZXIC_UINT32 rd_high_data_r_mex;
}DPP_PPU4K_CLUSTER_RD_HIGH_DATA_R_MEX_T;

typedef struct dpp_ppu4k_cluster_rd_low_data_r_mex_t
{
    ZXIC_UINT32 rd_low_data_r_mex;
}DPP_PPU4K_CLUSTER_RD_LOW_DATA_R_MEX_T;


#ifdef __cplusplus
}
#endif
#endif

