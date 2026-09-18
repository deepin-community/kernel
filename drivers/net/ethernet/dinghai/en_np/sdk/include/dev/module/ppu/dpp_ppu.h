/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_ppu.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 完成日期 : 2014/03/18
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#ifndef _DPP_PPU_H_
#define _DPP_PPU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dpp_module.h"
#include "dpp_ppu_api.h"
#include "dpp_ppu_reg.h"
#include "zxic_comm_thread.h"

#define PPU_CLS_ME_NUM         (8)
#define PPU_INSTR_MEM_NUM      (3)    /*ppu中的微码指令空间数量 2个cluster 共用一个指令空间*/
#define PPU_INSTR_REG_NUM      (4)    /*ppu中指令空间中的数据寄存器数目*/
#define PPU_INSTR_NUM_MAX      (32*1024)    /* xjw mod at 18.6.2 from 16k to 32k */
#define PPU_SDT_IDX_MIN        (0)
#define PPU_SDT_IDX_MAX        (255)
#define PPU_DUP_IDX_MIN        (0)
#define PPU_DUP_IDX_MAX        (63)
#define PPU_INSTR_COL_MAX      (4)

/**  ME指令调试中断*/
#define PPU_ME0_INT_BT_START   (0)
#define PPU_ME0_INT_BT_LEN     (1)
#define PPU_ME1_INT_BT_START   (1)
#define PPU_ME1_INT_BT_LEN     (1)
#define PPU_ME2_INT_BT_START   (2)
#define PPU_ME2_INT_BT_LEN     (1)
#define PPU_ME3_INT_BT_START   (3)
#define PPU_ME3_INT_BT_LEN     (1)
#define PPU_ME4_INT_BT_START   (4)
#define PPU_ME4_INT_BT_LEN     (1)
#define PPU_ME5_INT_BT_START   (5)
#define PPU_ME5_INT_BT_LEN     (1)
#define PPU_ME6_INT_BT_START   (6)
#define PPU_ME6_INT_BT_LEN     (1)
#define PPU_ME7_INT_BT_START   (7)
#define PPU_ME7_INT_BT_LEN     (1)

#define DPP_FPGA_MAX_FLOWTCAM_NUM (32)
#define DPP_PPU_CLS_0_BIT_MAP  (1<<0) /*bit0 = 1 代表cluster0 启动*/
#define DPP_PPU_CLS_1_BIT_MAP  (1<<1) /*bit1 = 1 代表cluster1 启动*/
#define DPP_PPU_CLS_2_BIT_MAP  (1<<2) /*bit2 = 1 代表cluster2 启动*/
#define DPP_PPU_CLS_3_BIT_MAP  (1<<3) /*bit3 = 1 代表cluster3 启动*/
#define DPP_PPU_CLS_4_BIT_MAP  (1<<4) /*bit4 = 1 代表cluster4 启动*/
#define DPP_PPU_CLS_5_BIT_MAP  (1<<5) /*bit5 = 1 代表cluster5 启动*/

#define DPP_PPU_CLS_ALL_START  (0x3F) /*打开所有cluster*/

/*该结构在ppu初始化的时候生成 全局不可修改*/
typedef struct dpp_ppu_cls_bitmap_t
{
    ZXIC_UINT32 cls_use[DPP_PPU_CLUSTER_NUM]; /*记录配置生效的 cluster 由bitmap解析获得*/
    ZXIC_UINT32 instr_mem[PPU_INSTR_MEM_NUM]; /*记录配置生效的 指令空间索引号, 每两个cluster 共享一个指令空间*/
} DPP_PPU_CLS_BITMAP_T;

typedef struct dpp_ppu_ppu_cop_thash_rsk_t
{
    ZXIC_UINT32 rsk_319_288;
    ZXIC_UINT32 rsk_287_256;
    ZXIC_UINT32 rsk_255_224;
    ZXIC_UINT32 rsk_223_192;
    ZXIC_UINT32 rsk_191_160;
    ZXIC_UINT32 rsk_159_128;
    ZXIC_UINT32 rsk_127_096;
    ZXIC_UINT32 rsk_095_064;
    ZXIC_UINT32 rsk_063_032;
    ZXIC_UINT32 rsk_031_000;

} DPP_PPU_PPU_COP_THASH_RSK_T;

ZXIC_UINT32 dpp_ppu_cls_use_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 cluster_id, ZXIC_UINT32 flag);
ZXIC_UINT32 dpp_ppu_cls_use_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 cluster_id);
ZXIC_UINT32 dpp_ppu_instr_mem_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 mem_id, ZXIC_UINT32 flag);
ZXIC_UINT32 dpp_ppu_parse_cls_bitmap(ZXIC_UINT32 dev_id, ZXIC_UINT32 bitmap);

DPP_STATUS dpp_ppu_ppu_cop_thash_rsk_set(DPP_DEV_T *dev, DPP_PPU_PPU_COP_THASH_RSK_T *p_para);
DPP_STATUS dpp_ppu_ppu_cop_thash_rsk_get(DPP_DEV_T *dev, DPP_PPU_PPU_COP_THASH_RSK_T *p_ppu_cop_thash_rsk);
#ifdef __cplusplus
}
#endif

#endif
