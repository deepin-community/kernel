/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_ppu_api.h
* 文件标识 : 
* 内容摘要 : PPU模块对外数据结构和函数声明
* 其它说明 : 
* 当前版本 : 
* 作    者 : wcl
* 完成日期 : 2015/02/13
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/

#ifndef _DPP_PPU_API_H_
#define _DPP_PPU_API_H_


#if ZXIC_REAL("header file")
#include "dpp_module.h"

#endif

#if ZXIC_REAL("data struct define")

/** mcode version time */
typedef struct dpp_version_t
{
    /*对应微码伪指令 version = expr_a.expr_b.expr_c*/
    ZXIC_UINT32 version_a; /**<  @brief expr_a*/
    ZXIC_UINT32 version_b; /**<  @brief expr_b*/
    ZXIC_UINT32 version_c; /**<  @brief xpr_c*/

    /*对应微码编译版本的时间 年/月/日/小时*/
    ZXIC_UINT32 year;
    ZXIC_UINT32 month;
    ZXIC_UINT32 day;
    ZXIC_UINT32 hour;
}DPP_VERSION_T;

/**  SDT表数据*/
typedef struct dpp_sdt_tbl_data_t
{
    ZXIC_UINT32 data_high32; /**<  @brief SDT表数据高32bit*/
    ZXIC_UINT32 data_low32;  /**<  @brief SDT表数据低32bit*/
}DPP_SDT_TBL_DATA_T;

#endif

#if ZXIC_REAL("interrupt data struct define")
/**  PPU 指令RAM ECC错误中断原因*/
typedef struct dpp_ppu_ram_parity_err_t
{
    ZXIC_UINT32 instrmem2_bank3_parity_err; /**<  @brief 指令RAM instrmem2 BANK3 parity 错误*/
    ZXIC_UINT32 instrmem2_bank2_parity_err; /**<  @brief 指令RAM instrmem2 BANK2 parity 错误*/
    ZXIC_UINT32 instrmem2_bank1_parity_err; /**<  @brief 指令RAM instrmem2 BANK1 parity 错误*/
    ZXIC_UINT32 instrmem2_bank0_parity_err; /**<  @brief 指令RAM instrmem2 BANK0 parity 错误*/
    ZXIC_UINT32 instrmem1_bank3_parity_err; /**<  @brief 指令RAM instrmem1 BANK3 parity 错误*/
    ZXIC_UINT32 instrmem1_bank2_parity_err; /**<  @brief 指令RAM instrmem1 BANK2 parity 错误*/
    ZXIC_UINT32 instrmem1_bank1_parity_err; /**<  @brief 指令RAM instrmem1 BANK1 parity 错误*/
    ZXIC_UINT32 instrmem1_bank0_parity_err; /**<  @brief 指令RAM instrmem1 BANK0 parity 错误*/
    ZXIC_UINT32 instrmem0_bank3_parity_err; /**<  @brief 指令RAM instrmem0 BANK3 parity 错误*/
    ZXIC_UINT32 instrmem0_bank2_parity_err; /**<  @brief 指令RAM instrmem0 BANK2 parity 错误*/
    ZXIC_UINT32 instrmem0_bank1_parity_err; /**<  @brief 指令RAM instrmem0 BANK1 parity 错误*/
    ZXIC_UINT32 instrmem0_bank0_parity_err; /**<  @brief 指令RAM instrmem0 BANK0 parity 错误*/
}DPP_PPU_RAM_PARITY_ERR_T;

/**  PPU ME指令RAM ECC错误中断原因*/
typedef struct dpp_ppu_me_interrupt_t
{
    ZXIC_UINT32 me7_interrupt_mask ;     /**<  @brief me7core的中断掩码*/
    ZXIC_UINT32 me6_interrupt_mask ;     /**<  @brief me6core的中断掩码*/
    ZXIC_UINT32 me5_interrupt_mask ;     /**<  @brief me5core的中断掩码*/
    ZXIC_UINT32 me4_interrupt_mask ;     /**<  @brief me4core的中断掩码*/
    ZXIC_UINT32 me3_interrupt_mask ;     /**<  @brief me3core的中断掩码*/
    ZXIC_UINT32 me2_interrupt_mask ;     /**<  @brief me2core的中断掩码*/
    ZXIC_UINT32 me1_interrupt_mask ;     /**<  @brief me1core的中断掩码*/
    ZXIC_UINT32 me0_interrupt_mask ;     /**<  @brief me0core的中断掩码*/

}DPP_PPU_ME_INTERRUPT_T;

/**  cluster mex的错误中断原因*/
typedef struct dpp_ppu_cluster_600m_mex_fifo_int_t
{
    ZXIC_UINT32 ppu_se_ikey_afifo_underflow;       /**<  @brief se片内表key fifo读空中断*/
    ZXIC_UINT32 ppu_se_ekey_afifo_underflow;       /**<  @brief se片外表key fifo读空中断*/
    ZXIC_UINT32 ppu_sta_key_afifo_underflow;       /**<  @brief 统计计数key fifo读空中断*/
    ZXIC_UINT32 ppu_cluster_mf_in_overflow;        /**<  @brief metafram接收 fifo满写中断*/
    ZXIC_UINT32 ppu_ese_rsp_afifo_overflow;        /**<  @brief se片外表 rsp fifo满写中断*/
    ZXIC_UINT32 ppu_ise_rsp_afifo_overflow;        /**<  @brief se片内表 rsp fifo满写中断*/
    ZXIC_UINT32 ppu_sta_rsp_afifo_overflow;        /**<  @brief 统计计数 rsp fifo满写中断*/
}DPP_PPU_CLUSTER_600M_MEX_FIFO_INT_T;

/**  cluster mex的错误中断原因*/
typedef struct dpp_ppu_cluster_1200m_mex_fifo_int_t
{
    ZXIC_UINT32 ppu_se_key_afifo_32x54_wrapper_overflow_flag;              /**<  @brief ppu_se_key_afifo_32x54_wrapper满写中断掩码*/
    ZXIC_UINT32 ppu_se_key_afifo_32x665_wrapper_overflow_flag;             /**<  @brief ppu_se_key_afifo_32x665_wrapper满写中断掩码*/
    ZXIC_UINT32 ppu_sta_key_afifo_32x110_wrapper_overflow_flag;            /**<  @brief ppu_sta_key_afifo_32x110_wrapper满写中断掩码*/
    ZXIC_UINT32 ppu_cluster_mf_in_afifo_16x2048_wrapper_underflow_flag;    /**<  @brief ppu_cluster_mf_in_afifo_32x2048_wrapper读空中断掩码*/
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_fifo_32x13_wrapper_overflow_flag;     /**<  @brief ppu_pbu_mcode_pf_rsp_fifo满写中断掩码*/
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_fifo_32x13_wrapper_underflow_flag;    /**<  @brief ppu_pbu_mcode_pf_rsp_fifo_32x13_wrapper读空中断掩码*/
    ZXIC_UINT32 ppu_coprocess_rsp_fifo_32x77_wrapper_overflow_flag;        /**<  @brief ppu_coprocess_rsp_fifo_32x77_wrapper满写中断掩码*/
    ZXIC_UINT32 ppu_coprocess_rsp_fifo_32x77_wrapper_underflow_flag;       /**<  @brief ppu_coprocess_rsp_fifo_32x77_wrapper读空中断掩码*/
    ZXIC_UINT32 ppu_coprocess_rsp_fwft_fifo_128x78_wrapper_overflow_flag;  /**<  @brief ppu_coprocess_rsp_fwft_fifo_128x78_wrapper满写中断掩码*/
    ZXIC_UINT32 ppu_coprocess_rsp_fwft_fifo_128x78_wrapper_underflow_flag; /**<  @brief ppu_coprocess_rsp_fwft_fifo_128x78_wrapper读空中断掩码*/
    ZXIC_UINT32 ppu_ese_rsp_afifo_64x271_wrapper_u0_underflow_flag;        /**<  @brief ppu_ese_rsp_afifo_64x271_wrapper_u0满写中断掩码*/
    
    ZXIC_UINT32 ese_rsp_ram_free_ptr_u0_overflow_flag;                     /**<  @brief ese_rsp_ram_free_ptr_u0满写中断掩码*/
    ZXIC_UINT32 ese_rsp_ram_free_ptr_u0_underflow_flag;                    /**<  @brief ese_rsp_ram_free_ptr_u0读空中断掩码*/
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u0_overflow_flag;  /**<  @brief ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u0满写中断掩码*/
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u0_underflow_flag; /**<  @brief ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u0读空中断掩码*/
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u1_overflow_flag;  /**<  @brief ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u1满写中断掩码*/
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u1_underflow_flag; /**<  @brief ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u1读空中断掩码*/
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u2_overflow_flag;  /**<  @brief ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u2满写中断掩码*/
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u2_underflow_flag; /**<  @brief ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u2读空中断掩码*/
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u3_overflow_flag;  /**<  @brief ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u3满写中断标记*/
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u3_underflow_flag; /**<  @brief ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u3读空中断标记*/
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u4_overflow_flag;  /**<  @brief ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u4满写中断标记*/
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u4_underflow_flag; /**<  @brief ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u4读空中断标记*/
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u5_overflow_flag;  /**<  @brief ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u5满写中断标记*/
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u5_underflow_flag; /**<  @brief ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u5读空中断标记*/
    ZXIC_UINT32 ppu_ise_rsp_afifo_64x143_wrapper_u0_underflow_flag;        /**<  @brief ppu_ise_rsp_afifo_64x143_wrapper_u0读空中断标记*/
    ZXIC_UINT32 ise_rsp_ram_free_ptr_u0_overflow_flag;                     /**<  @brief ise_rsp_ram_free_ptr_u0满写中断标记  */
    ZXIC_UINT32 ise_rsp_ram_free_ptr_u0_underflow_flag;                    /**<  @brief ise_rsp_ram_free_ptr_u0读空中断标记*/
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u0_overflow_flag;  /**<  @brief ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u0满写中断标记*/
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u0_underflow_flag; /**<  @brief ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u0读空中断标记*/
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u1_overflow_flag;  /**<  @brief ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u1满写中断标记*/
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u1_underflow_flag; /**<  @brief ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u1读空中断标记*/
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u2_overflow_flag;  /**<  @brief ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u2满写中断标记*/
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u2_underflow_flag; /**<  @brief ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u2读空中断标记*/
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u3_overflow_flag;  /**<  @brief ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u3满写中断标记*/
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u3_underflow_flag; /**<  @brief ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u3读空中断标记*/
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u4_overflow_flag;  /**<  @brief ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u4满写中断标记*/
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u4_underflow_flag; /**<  @brief ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u4读空中断标记*/
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u5_overflow_flag;  /**<  @brief ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u5满写中断标记*/
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u5_underflow_flag; /**<  @brief ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u5读空中断标记*/
    ZXIC_UINT32 ppu_sta_rsp_afifo_64x79_wrapper_underflow_flag;            /**<  @brief ppu_sta_rsp_afifo_64x79_wrapper读空中断标记*/
    ZXIC_UINT32 ppu_sta_rsp_fwft_fifo_128x79_wrapper_overflow_flag;        /**<  @brief ppu_sta_rsp_fwft_fifo_128x79_wrapper满写中断标记*/
    ZXIC_UINT32 ppu_sta_rsp_fwft_fifo_128x79_wrapper_underflow_flag;       /**<  @brief ppu_sta_rsp_fwft_fifo_128x79_wrapper读空中断标记*/
}DPP_PPU_CLUSTER_1200M_MEX_FIFO_INT_T;

typedef struct dpp_ppu_ppu_ram_err_t
{
    ZXIC_UINT32 dup_freeptr_fwft_fifo_128x7_wrapper_u0_parity_err_flag;             /**<  @brief dup_freeptr_fwft_fifo_128x7_wrapper_u0_parity_err中断标记*/
    ZXIC_UINT32 free_global_num_fwft_fifo_8192x13_wrapper_u0_parity_err_flag;       /**<  @brief free_global_num_fwft_fifo_8192x13_wrapper_u0_parity_err中断标记*/
    ZXIC_UINT32 ppu_mccnt_fifo_32x15_wrapper_u0_parity_err_flag;                    /**<  @brief ppu_mccnt_fifo_32x15_wrapper_u0_parity_err中断标记*/
    ZXIC_UINT32 ppu_reorder_link_table_ram_1r1w_8192x13_wrapper_u1_parity_err_flag; /**<  @brief ppu_reorder_link_table_ram_1r1w_8192x13_wrapper_u1_parity_err中断标记*/
    ZXIC_UINT32 ppu_reorder_link_table_ram_1r1w_8192x13_wrapper_u0_parity_err_flag; /**<  @brief ppu_reorder_link_table_ram_1r1w_8192x13_wrapper_u0_parity_err中断标记*/
}DPP_PPU_PPU_RAM_ERR_T;

typedef struct dpp_ppu_cls_ram_err_600m_t
{
    ZXIC_UINT32 ppu_sdt_table_ram_2rw_256x64_wrapper_parity_errb_flag; /**<  @brief ppu_sdt_table_ram_2rw_256x64_wrapper_parity_errb中断标记*/
    ZXIC_UINT32 ppu_sdt_table_ram_2rw_256x64_wrapper_parity_erra_flag; /**<  @brief ppu_sdt_table_ram_2rw_256x64_wrapper_parity_errba中断标记*/
}DPP_PPU_CLS_RAM_ERR_600M_T;

/**  cluster mex的错误中断原因*/
typedef struct dpp_ppu_cluster_me_fifo_int_t
{
    ZXIC_UINT32 me_except_refetch_pc_overflow_flag;  /**<  @brief me_except_refetch_pc满写中断标记*/
    ZXIC_UINT32 me_except_refetch_pc_underflow_flag; /**<  @brief me_except_refetch_pc读空中断标记*/
    
    ZXIC_UINT32 me_free_pkt_q_overflow_flag;         /**<  @brief me_free_pkt_q满写中断状态*/
    ZXIC_UINT32 me_free_pkt_q_underflow_flag;        /**<  @brief me_free_pkt_q读空中断状态*/
    ZXIC_UINT32 me_free_thread_q_overflow_flag;      /**<  @brief me_free_thread_q满写中断状态*/
    ZXIC_UINT32 me_free_thread_q_underflow_flag;     /**<  @brief me_free_thread_q读空中断状态*/
    ZXIC_UINT32 me_pkt_in_overflow_flag;             /**<  @brief me_pkt_in_满写中断状态*/  
    ZXIC_UINT32 me_pkt_in_underflow_flag;            /**<  @brief me_pkt_in_读空中断状态*/ 
    ZXIC_UINT32 me_rdy_q_overflow_flag;              /**<  @brief me_rdy_q满写中断状态*/ 
    ZXIC_UINT32 me_rdy_q_underflow_flag;             /**<  @brief me_rdy_q读空中断状态*/
    ZXIC_UINT32 me_pkt_out_q_overflow_flag;          /**<  @brief me_pkt_out_q满写中断状态*/
    ZXIC_UINT32 me_pkt_out_q_underflow_flag;         /**<  @brief me_pkt_out_q读空中断状态*/
    ZXIC_UINT32 me_continue_q_overflow_flag;         /**<  @brief me_continue_q满写中断状态*/
    ZXIC_UINT32 me_continue_q_underflow_flag;        /**<  @brief me_continue_q读空中断状态*/
    ZXIC_UINT32 me_esrh_q_overflow_flag;             /**<  @brief me_esrh_q满写中断状态*/ 
    ZXIC_UINT32 me_esrh_q_underflow_flag;            /**<  @brief me_esrh_q读空中断状态*/
    ZXIC_UINT32 me_isrh_q_overflow_flag;             /**<  @brief me_isrh_q满写中断状态*/
    ZXIC_UINT32 me_isrh_q_underflow_flag;            /**<  @brief me_isrh_q读空中断状态*/  
    ZXIC_UINT32 me_cache_miss_q_overflow_flag;       /**<  @brief me_cache_miss_q满写中断状态*/
    ZXIC_UINT32 me_cache_miss_q_underflow_flag;      /**<  @brief me_cache_miss_q读空中断状态*/ 
    ZXIC_UINT32 me_base_q_u0_overflow_flag;          /**<  @brief me_base_q_u0满写中断状态*/
    ZXIC_UINT32 me_base_q_u0_underflow_flag;         /**<  @brief me_base_q_u0读空中断状态*/
    ZXIC_UINT32 me_base_q_u1_overflow_flag;          /**<  @brief me_base_q_u1满写中断状态*/ 
    ZXIC_UINT32 me_base_q_u1_underflow_flag;         /**<  @brief me_base_q_u1读空中断状态*/
    ZXIC_UINT32 me_base_q_u2_overflow_flag;          /**<  @brief me_base_q_u2满写中断状态*/ 
    ZXIC_UINT32 me_base_q_u2_underflow_flag;         /**<  @brief me_base_q_u2读空中断状态*/
    ZXIC_UINT32 me_base_q_u3_overflow_flag;          /**<  @brief me_base_q_u3满写中断状态*/ 
    ZXIC_UINT32 me_base_q_u3_underflow_flag;         /**<  @brief me_base_q_u3读空中断状态*/
    ZXIC_UINT32 me_reg_pc_q_overflow_flag;           /**<  @brief me_reg_pc_q满写中断状态*/
    ZXIC_UINT32 me_reg_pc_q_underflow_flag;          /**<  @brief me_reg_pc_q读空中断状态*/
    ZXIC_UINT32 me_branch_q_overflow_flag;           /**<  @brief me_branch_q满写中断状态*/
    ZXIC_UINT32 me_branch_q_underflow_flag;          /**<  @brief me_branch_q读空中断状态*/
    ZXIC_UINT32 me_pkt_base_q_overflow_flag;         /**<  @brief me_pkt_base_q满写中断状态*/
    ZXIC_UINT32 me_pkt_base_q_underflow_flag;        /**<  @brief me_pkt_base_q读空中断状态*/
}DPP_PPU_CLUSTER_ME_FIFO_INT_T;


typedef struct dpp_ppu_ppu_isu_ppu_demux_fifo_int_t
{
    ZXIC_UINT32 isu_in_para_fwft_fifo_32x81_wrapper_u0_overflow_flag;  /**<  @brief isu_in_para_fwft_fifo_32x81_wrapper_u0_overflow中断标记*/
    ZXIC_UINT32 isu_in_para_fwft_fifo_32x81_wrapper_u0_underflow_flag; /**<  @brief isu_in_para_fwft_fifo_32x81_wrapper_u0_underflow中断标记*/
    ZXIC_UINT32 isu_in_fifo_64x81_wrapper_u0_overflow_flag;            /**<  @brief isu_in_fifo_64x81_wrapper_u0_overflow中断标记*/
    ZXIC_UINT32 isu_in_fifo_64x81_wrapper_u0_underflow_flag;           /**<  @brief isu_in_fifo_64x81_wrapper_u0_underflow中断标记*/
}DPP_PPU_PPU_ISU_PPU_DEMUX_FIFO_INT_T;

typedef struct dpp_ppu_ppu_ppu_multicast_fifo_int_t
{
    ZXIC_UINT32 pf_req_fwft_fifo_16x36_wrapper_u0_overflow_flag;        /**<  @brief pf_req_fwft_fifo_16x36_wrapper_u0_overflow中断标记*/
    ZXIC_UINT32 pf_req_fwft_fifo_16x36_wrapper_u0_underflow_flag;       /**<  @brief pf_req_fwft_fifo_16x36_wrapper_u0_underflow中断标记*/
    ZXIC_UINT32 pf_rsp_fwft_fifo_32x34_wrapper_u0_overflow_flag;        /**<  @brief pf_rsp_fwft_fifo_32x34_wrapper_u0_overflow中断标记*/
    ZXIC_UINT32 pf_rsp_fwft_fifo_32x34_wrapper_u0_underflow_flag;       /**<  @brief pf_rsp_fwft_fifo_32x34_wrapper_u0_underflow中断标记*/
    ZXIC_UINT32 dup_para_fwft_fifo_16x35_wrapper_u0_overflow_flag;      /**<  @brief dup_para_fwft_fifo_16x35_wrapper_u0_overflow中断标记*/
    ZXIC_UINT32 dup_para_fwft_fifo_16x35_wrapper_u0_underflow_flag;     /**<  @brief dup_para_fwft_fifo_16x35_wrapper_u0_underflow中断标记*/
    ZXIC_UINT32 se_mc_rsp_fwft_fifo_32x17_wrapper_u0_overflow_flag;     /**<  @brief se_mc_rsp_fwft_fifo_32x17_wrapper_u0_overflow中断标记*/
    ZXIC_UINT32 se_mc_rsp_fwft_fifo_32x17_wrapper_u0_underflow_flag;    /**<  @brief se_mc_rsp_fwft_fifo_32x17_wrapper_u0_underflow中断标记*/
    ZXIC_UINT32 sa_para_fwft_fifo_64x17_wrapper_u0_overflow_flag;       /**<  @brief sa_para_fwft_fifo_64x17_wrapper_u0_overflow中断标记*/
    ZXIC_UINT32 sa_para_fwft_fifo_64x17_wrapper_u0_underflow_flag;      /**<  @brief sa_para_fwft_fifo_64x17_wrapper_u0_underflow中断标记*/
    ZXIC_UINT32 group_id_fifo_64x16_wrapper_u0_overflow_flag;           /**<  @brief group_id_fifo_64x16_wrapper_u0_overflow中断标记*/
    ZXIC_UINT32 group_id_fifo_64x16_wrapper_u0_underflow_flag;          /**<  @brief group_id_fifo_64x16_wrapper_u0_underflow中断标记*/
    ZXIC_UINT32 isu_mc_para_fwft_fifo_128x34_wrapper_u0_overflow_flag;  /**<  @brief isu_mc_para_fwft_fifo_128x34_wrapper_u0_overflow中断标记*/
    ZXIC_UINT32 isu_mc_para_fwft_fifo_128x34_wrapper_u0_underflow_flag; /**<  @brief isu_mc_para_fwft_fifo_128x34_wrapper_u0_underflow中断标记*/
    ZXIC_UINT32 dup_freeptr_fwft_fifo_128x7_wrapper_u0_overflow_flag;   /**<  @brief dup_freeptr_fwft_fifo_128x7_wrapper_u0_overflow中断标记*/
    ZXIC_UINT32 dup_freeptr_fwft_fifo_128x7_wrapper_u0_underflow_flag;  /**<  @brief dup_freeptr_fwft_fifo_128x7_wrapper_u0_underflow中断标记*/
    ZXIC_UINT32 car_flag_fifo_32x1_wrapper_overflow_flag;               /**<  @brief car_flag_fifo_32x1_wrapper_overflow中断标记*/
    ZXIC_UINT32 car_flag_fifo_32x1_wrapper_underflow_flag;              /**<  @brief car_flag_fifo_32x1_wrapper_underflow中断标记*/
}DPP_PPU_PPU_PPU_MULTICAST_FIFO_INT_T;

typedef struct dpp_ppu_ppu_ppu_in_schedule_fifo_int_t
{
    ZXIC_UINT32 free_global_num_fwft_fifo_8192x13_wrapper_u0_overflow_flag;  /**<  @brief free_global_num_fwft_fifo_8192x13_满写中断标记*/
    ZXIC_UINT32 free_global_num_fwft_fifo_8192x13_wrapper_u0_underflow_flag; /**<  @brief free_global_num_fwft_fifo_8192x13_空读中断标记*/
    ZXIC_UINT32 mc_mf_fifo_16x2048_wrapper_u0_overflow_flag;                 /**<  @brief mc_mf_fifo_16x2048_满写中断标记*/
    ZXIC_UINT32 mc_mf_fifo_16x2048_wrapper_u0_underflow_flag;                /**<  @brief mc_mf_fifo_16x2048_空读中断标记*/
    ZXIC_UINT32 uc_mf_fifo_96x2048_wrapper_u0_overflow_flag;                 /**<  @brief uc_mf_fifo_96x2048_满写中断标记*/
    ZXIC_UINT32 uc_mf_fifo_96x2048_wrapper_u0_underflow_flag;                /**<  @brief uc_mf_fifo_96x2048_空读中断标记*/
}DPP_PPU_PPU_PPU_IN_SCHEDULE_FIFO_INT_T;

typedef struct dpp_ppu_ppu_ppu_mf_out_fifo_int_t
{
    ZXIC_UINT32 ppu_cluster5_mf_out_afifo_16x2048_wrapper_overflow_flag;  /**<  @brief cluster5发送描述符fifo满写中断标记*/
    ZXIC_UINT32 ppu_cluster5_mf_out_afifo_16x2048_wrapper_underflow_flag; /**<  @brief cluster5发送描述符fifo空读中断标记*/
    ZXIC_UINT32 ppu_cluster4_mf_out_afifo_16x2048_wrapper_overflow_flag;  /**<  @brief cluster4发送描述符fifo满写中断标记*/
    ZXIC_UINT32 ppu_cluster4_mf_out_afifo_16x2048_wrapper_underflow_flag; /**<  @brief cluster4发送描述符fifo空读中断标记*/
    ZXIC_UINT32 ppu_cluster3_mf_out_afifo_16x2048_wrapper_overflow_flag;  /**<  @brief cluster3发送描述符fifo满写中断标记*/
    ZXIC_UINT32 ppu_cluster3_mf_out_afifo_16x2048_wrapper_underflow_flag; /**<  @brief cluster3发送描述符fifo空读中断标记*/
    ZXIC_UINT32 ppu_cluster2_mf_out_afifo_16x2048_wrapper_overflow_flag;  /**<  @brief cluster2发送描述符fifo满写中断标记*/
    ZXIC_UINT32 ppu_cluster2_mf_out_afifo_16x2048_wrapper_underflow_flag; /**<  @brief cluster2发送描述符fifo空读中断标记*/
    ZXIC_UINT32 ppu_cluster1_mf_out_afifo_16x2048_wrapper_overflow_flag;  /**<  @brief cluster1发送描述符fifo满写中断标记*/
    ZXIC_UINT32 ppu_cluster1_mf_out_afifo_16x2048_wrapper_underflow_flag; /**<  @brief cluster1发送描述符fifo空读中断标记*/
    ZXIC_UINT32 ppu_cluster0_mf_out_afifo_16x2048_wrapper_overflow_flag;  /**<  @brief cluster0发送描述符fifo满写中断标记*/
    ZXIC_UINT32 ppu_cluster0_mf_out_afifo_16x2048_wrapper_underflow_flag; /**<  @brief cluster0发送描述符fifo空读中断标记*/
}DPP_PPU_PPU_PPU_MF_OUT_FIFO_INT_T;

typedef struct dpp_ppu_ppu_pbu_mcode_pf_req_schedule_fifo_int_t
{
    ZXIC_UINT32 ppu_cluster5_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_flag;  /**<  @brief cluster5指针预取fifo满写中断标记*/
    ZXIC_UINT32 ppu_cluster4_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_flag;  /**<  @brief cluster4指针预取fifo满写中断标记*/
    ZXIC_UINT32 ppu_cluster3_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_flag;  /**<  @brief cluster3指针预取fifo满写中断标记*/
    ZXIC_UINT32 ppu_cluster2_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_flag;  /**<  @brief cluster2指针预取fifo满写中断标记*/
    ZXIC_UINT32 ppu_cluster1_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_flag;  /**<  @brief cluster1指针预取fifo满写中断标记*/
    ZXIC_UINT32 ppu_cluster0_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_flag;  /**<  @brief cluster0指针预取fifo满写中断标记*/
    ZXIC_UINT32 ppu_cluster5_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_flag; /**<  @brief cluster5指针预取fifo空读中断标记*/
    ZXIC_UINT32 ppu_cluster4_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_flag; /**<  @brief cluster4指针预取fifo空读中断标记*/
    ZXIC_UINT32 ppu_cluster3_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_flag; /**<  @brief cluster3指针预取fifo空读中断标记*/
    ZXIC_UINT32 ppu_cluster2_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_flag; /**<  @brief cluster2指针预取fifo空读中断标记*/
    ZXIC_UINT32 ppu_cluster1_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_flag; /**<  @brief cluster1指针预取fifo空读中断标记*/
    ZXIC_UINT32 ppu_cluster0_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_flag; /**<  @brief cluster0指针预取fifo空读中断标记*/
}DPP_PPU_PPU_PBU_MCODE_PF_REQ_SCHEDULE_FIFO_INT_T;

typedef struct dpp_ppu_ppu_pbu_mcode_pf_rsp_schedule_fifo_int_t
{
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_afifo_64x16_wrapper_u0r_underflow_flag; /**<  @brief ppu微码申请指针返回fifo空读中断标记*/
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_afifo_64x16_wrapper_u0_overflow_flag;   /**<  @brief ppu微码申请指针返回fifo满写中断标记*/
}DPP_PPU_PPU_PBU_MCODE_PF_RSP_SCHEDULE_FIFO_INT_T;

typedef struct dpp_ppu_ppu_ppu_mccnt_fifo_int_t
{
    ZXIC_UINT32 ppu_mccnt_fifo_32x15_wrapper_u0_overflow_flag;       /**<  @brief ppu_mccnt_fifo_32x15_wrapper_u0_overflow中断标记*/
    ZXIC_UINT32 ppu_mccnt_fifo_32x15_wrapper_u0_underflow_flag;      /**<  @brief ppu_mccnt_fifo_32x15_wrapper_u0_underflow中断标记*/
    ZXIC_UINT32 ppu_wb_data_fifo_32x2048_wrapper_u0_overflow_flag;   /**<  @brief ppu_wb_data_fifo_32x2048_wrapper_u0_overflow中断标记*/
    ZXIC_UINT32 ppu_wb_data_fifo_32x2048_wrapper_u0_underflow_flag;  /**<  @brief ppu_wb_data_fifo_32x2048_wrapper_u0_underflow中断标记*/
    ZXIC_UINT32 mccnt_rsp_fifo_32x1_wrapper_u0_overflow_flag;        /**<  @brief mccnt_rsp_fifo_32x1_wrapper_u0_overflow中断标记*/
    ZXIC_UINT32 mccnt_rsp_fifo_32x1_wrapper_u0_underflow_flag;       /**<  @brief mccnt_rsp_fifo_32x1_wrapper_u0_underflow中断标记*/
}DPP_PPU_PPU_PPU_MCCNT_FIFO_INT_T;

typedef struct dpp_ppu_ppu_coprocessor_fifo_int_t
{
    ZXIC_UINT32 ppu_cop_random_mod_para_delay_fifo_48x16_wrapper_overflow_flag;   /**<  @brief 协处理random_mod参数fifo错误满写中断标记*/
    ZXIC_UINT32 ppu_cop_random_mod_para_delay_fifo_48x16_wrapper_underflow_flag;  /**<  @brief 协处理random_mod参数fifo错误空读中断标记*/

    ZXIC_UINT32 ppu_cop_result_fwft_fifo_80x80_wrapper_overflow_flag;    /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_80x80_wrapper_underflow_flag;   /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_delay_fifo_48x16_wrapper_overflow_flag;          /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_delay_fifo_48x16_wrapper_underflow_flag;         /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_delay_fifo_16x48_wrapper_overflow_flag;          /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_delay_fifo_16x48_wrapper_underflow_flag;         /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_delay_fifo_16x32_wrapper_overflow_flag;          /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_delay_fifo_16x32_wrapper_underflow_flag;         /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_96x80_wrapper_overflow_flag;    /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_96x80_wrapper_underflow_flag;   /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_delay_fifo_16x16_wrapper_overflow_flag;          /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_delay_fifo_16x16_wrapper_underflow_flag;         /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_32x80_wrapper_overflow_flag;    /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_32x80_wrapper_underflow_flag;   /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_16x80_wrapper_overflow_flag;    /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_16x80_wrapper_underflow_flag;   /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec5_cop_key_crc_fifo_32x625_wrapper_overflow_flag;      /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec5_cop_key_crc_fifo_32x625_wrapper_underflow_flag;     /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec5_cop_key_checksum_fifo_32x180_wrapper_overflow_flag;   /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec5_cop_key_checksum_fifo_32x180_wrapper_underflow_flag;  /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec5_cop_key_mul_fifo_32x52_wrapper_overflow_flag;         /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec5_cop_key_mul_fifo_32x52_wrapper_underflow_flag;        /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec5_cop_key_random_mod_fifo_32x44_wrapper_overflow_flag;  /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec5_cop_key_random_mod_fifo_32x44_wrapper_underflow_flag; /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec4_cop_key_crc_fifo_32x625_wrapper_overflow_flag;        /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec4_cop_key_crc_fifo_32x625_wrapper_underflow_flag;       /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec4_cop_key_checksum_fifo_32x180_wrapper_overflow_flag;   /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec4_cop_key_checksum_fifo_32x180_wrapper_underflow_flag;  /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec4_cop_key_mul_fifo_32x52_wrapper_overflow_flag;         /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec4_cop_key_mul_fifo_32x52_wrapper_underflow_flag;        /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec4_cop_key_random_mod_fifo_32x44_wrapper_overflow_flag;  /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec4_cop_key_random_mod_fifo_32x44_wrapper_underflow_flag; /**<  @brief fifo错误中断标记*/

    ZXIC_UINT32 mec3_cop_key_crc_fifo_32x625_wrapper_overflow_flag;        /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec3_cop_key_crc_fifo_32x625_wrapper_underflow_flag;       /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec3_cop_key_checksum_fifo_32x180_wrapper_overflow_flag;   /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec3_cop_key_checksum_fifo_32x180_wrapper_underflow_flag;  /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec3_cop_key_mul_fifo_32x52_wrapper_overflow_flag;         /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec3_cop_key_mul_fifo_32x52_wrapper_underflow_flag;        /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec3_cop_key_random_mod_fifo_32x44_wrapper_overflow_flag;  /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec3_cop_key_random_mod_fifo_32x44_wrapper_underflow_flag; /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec2_cop_key_crc_fifo_32x625_wrapper_overflow_flag;        /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec2_cop_key_crc_fifo_32x625_wrapper_underflow_flag;       /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec2_cop_key_checksum_fifo_32x180_wrapper_overflow_flag;   /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec2_cop_key_checksum_fifo_32x180_wrapper_underflow_flag;  /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec2_cop_key_mul_fifo_32x52_wrapper_overflow_flag;         /**<  @brief fifo错误中断标记*/  
    ZXIC_UINT32 mec2_cop_key_mul_fifo_32x52_wrapper_underflow_flag;        /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec2_cop_key_random_mod_fifo_32x44_wrapper_overflow_flag;  /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec2_cop_key_random_mod_fifo_32x44_wrapper_underflow_flag; /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec1_cop_key_crc_fifo_32x625_wrapper_overflow_flag;        /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec1_cop_key_crc_fifo_32x625_wrapper_underflow_flag;       /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec1_cop_key_checksum_fifo_32x180_wrapper_overflow_flag;   /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec1_cop_key_checksum_fifo_32x180_wrapper_underflow_flag;  /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec1_cop_key_mul_fifo_32x52_wrapper_overflow_flag;         /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec1_cop_key_mul_fifo_32x52_wrapper_underflow_flag;        /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec1_cop_key_random_mod_fifo_32x44_wrapper_overflow_flag;  /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec1_cop_key_random_mod_fifo_32x44_wrapper_underflow_flag; /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec0_cop_key_crc_fifo_32x625_wrapper_overflow_flag;        /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec0_cop_key_crc_fifo_32x625_wrapper_underflow_flag;       /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec0_cop_key_checksum_fifo_32x180_wrapper_overflow_flag;   /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec0_cop_key_checksum_fifo_32x180_wrapper_underflow_flag;  /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec0_cop_key_mul_fifo_32x52_wrapper_overflow_flag;         /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec0_cop_key_mul_fifo_32x52_wrapper_underflow_flag;        /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec0_cop_key_random_mod_fifo_32x44_wrapper_overflow_flag;  /**<  @brief fifo错误中断标记*/
    ZXIC_UINT32 mec0_cop_key_random_mod_fifo_32x44_wrapper_underflow_flag; /**<  @brief fifo错误中断标记*/
}DPP_PPU_PPU_COPROCESSOR_FIFO_INT_T;


typedef struct dpp_ppu_ppu_cos_meter_cfg_t
{
    ZXIC_UINT32 cbs;           /**<  @brief 0~7队列CBS配置*/
    ZXIC_UINT32 pbs;           /**<  @brief 0~7队列PBS配置*/
    ZXIC_UINT32 green_action;  /**<  @brief 0~7队列绿色报文动作配置，关闭CAR使能的时候，需要同时将该动作置为1*/
    ZXIC_UINT32 yellow_action; /**<  @brief 0~7队列黄色报文动作配置*/
    ZXIC_UINT32 red_action;    /**<  @brief 0~7队列红色报文动作配置*/
    ZXIC_UINT32 cir;           /**<  @brief 0~7队列CIR配置，单位Mpps，0x24a对应600Mpps，按照线性变化*/  
    ZXIC_UINT32 pir;           /**<  @brief 0~7队列PIR配置，单位Mpps，0x24a对应600Mpps，按照线性变化*/
    ZXIC_UINT32 car_en;        /**<  @brief 0~7队列CAR使能配置*/
}DPP_PPU_PPU_COS_METER_CFG_T;

#endif

/***********************************************************/
/** 配置SDT表
* @param   dev_id   设备号，范围0~3
* @param   cluster_id  me cluster编号，范围0~7
* @param   index       地址，即sdt表号，范围0~255
* @param   p_sdt_data  sdt表数据
*
* @return
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_ppu_sdt_tbl_write(DPP_DEV_T *dev, ZXIC_UINT32 cluster_id, ZXIC_UINT32 index, DPP_SDT_TBL_DATA_T *p_sdt_data);

DPP_STATUS dpp_ppu_set_debug_mode(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 *dbg_status);
DPP_STATUS dpp_ppu_close_debug_mode(DPP_PF_INFO_T* pf_info);
#endif /* dpp_ppu_api.h */
