#ifndef _DPP_DTB_TABLE_H_
#define _DPP_DTB_TABLE_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "dpp_dev.h"
#include "dpp_hash.h"
#include "dpp_etcam.h"
#include "dpp_dtb_table_api.h"

#ifndef DISABLE
#define DISABLE (0)
#endif

#ifndef ENABLE
#define ENABLE (1)
#endif

#define DTB_DOWN_TABLE_CMD (0)
#define DTB_DUMP_TABLE_CMD (1)

#define DTB_QUEUE_MAX (128)
#define DTB_QUEUE_ELEMENT_MAX (32)
#define DTB_DATA_SIZE_BIT (16*1024*8)
#define DPP_DTB_TABLE_DATA_BUFF_SIZE (1024*16)
#define DPP_DTB_TABLE_DUMP_INFO_BUFF_SIZE (1024*4)
#define DTB_TABLE_CMD_SIZE_BIT (128)
#define DTB_TABLE_CMD_SIZE_BYTE (16)
#define DTB_ERAM_DATA_SIZE_1b (128)
#define DTB_ERAM_DATA_SIZE_64b (128)
#define DTB_ERAM_DATA_SIZE_128b (256)
#define DTB_ERAM_ENTRY_CNT_MAX_1b (DTB_DATA_SIZE_BIT / DTB_ERAM_DATA_SIZE_1b)
#define DTB_ERAM_ENTRY_CNT_MAX_64b (DTB_DATA_SIZE_BIT / DTB_ERAM_DATA_SIZE_64b)
#define DTB_ERAM_ENTRY_CNT_MAX_128b (DTB_DATA_SIZE_BIT / DTB_ERAM_DATA_SIZE_128b)
#define DTB_ZCAM_LEN_SIZE (5) /*单位16字节*/
#define DTB_ETCAM_LEN_SIZE (6) /*单位16字节*/
#define DTB_MC_HASH_LEN_SIZE (5) /*单位16字节*/
#define DTB_ZCAM_DATA_SIZE   (ZXIC_UINT32)(64)
#define DTB_DMUP_DATA_MAX    (ZXIC_UINT32)(4*1024*1024)
#define DTB_DUMP_DDR_ITEMS_MAX (0x10000)

#define DTB_SDT_DUMP_SIZE (0x400000) //4MB

#define DTB_TABLE_VALID  (1)
#define DTB_LEN_POS_SETP (16) /*DTB len 单位16字节 */

#define LPM_IPV4 (1)
#define LPM_IPV6 (0)
#define LPM_ENABLE (1)
#define LPM_DISABLE (0)

#define DTB_TABLE_MODE_ERAM (0)
#define DTB_TABLE_MODE_DDR (1)
#define DTB_TABLE_MODE_ZCAM (2)
#define DTB_TABLE_MODE_ETCAM (3)
#define DTB_TABLE_MODE_MC_HASH (4)

#define DTB_DUMP_MODE_ERAM (0)
#define DTB_DUMP_MODE_DDR  (1)
#define DTB_DUMP_MODE_ZCAM (2)
#define DTB_DUMP_MODE_ETCAM (3)

#define DTB_ITEM_ADD_OR_UPDATE (0)
#define DTB_ITEM_DELETE (1)

#define ACL_HANDLE_INVALID (0xFFFFFFFF)

/* DTB 表信息管理 */
/*表顺序与g_dpp_dtb_table_info中顺序一致*/
typedef enum dpp_dtb_table_info_e
{
    DTB_TABLE_DDR  			= 0,
    DTB_TABLE_ERAM_1 		= 1,
	DTB_TABLE_ERAM_64 	    = 2,
	DTB_TABLE_ERAM_128      = 3,
    DTB_TABLE_ZCAM 			= 4,
    DTB_TABLE_ETCAM 		= 5,
    DTB_TABLE_MC_HASH 	    = 6,
	DTB_TABLE_ENUM_MAX
}DPP_DTB_TABLE_INFO_E;

typedef enum dpp_dtb_dump_info_e
{
    DTB_DUMP_ERAM = 0,
    DTB_DUMP_DDR  = 1,
    DTB_DUMP_ZCAM = 2,
    DTB_DUMP_ETCAM = 3,
	DTB_DUMP_ENUM_MAX
}DPP_DTB_DUMP_INFO_E;

typedef enum dpp_dtb_dump_zcam_width_e
{
    DTB_DUMP_ZCAM_128b = 0,
    DTB_DUMP_ZCAM_256b = 1,
    DTB_DUMP_ZCAM_512b = 2,
	DTB_DUMP_ZCAM_RSV  = 3,
}DPP_DTB_DUMP_ZCAM_WIDTH_E;

typedef enum dpp_dtb_dump_etcam_width_e
{
    DTB_DUMP_ETCAM_80b = 0,
    DTB_DUMP_ETCAM_160b = 1,
    DTB_DUMP_ETCAM_320b = 2,
	DTB_DUMP_ETCAM_640b  = 3,
	DTB_DUMP_ETCAM_MAX
}DPP_DTB_DUMP_ETCAM_WIDTH_E;

/* DTB下表格式字段定义 */
typedef struct dpp_dtb_ddr_table_form_t
{
	ZXIC_UINT32 valid;       /* 有效标识 1有效*/
	ZXIC_UINT32 type_mode;   /* DDR：0x1 */
    ZXIC_UINT32 rw_len;      /*数据长度 00:128  01:256  10:384  11:512*/
    ZXIC_UINT32 v46_flag;    /*1：IPV4  0:IPV6*/
    ZXIC_UINT32 lpm_wr_vld;  /*lpm表写有效标识*/
    ZXIC_UINT32 baddr;       /*表基地址*/
    ZXIC_UINT32 ecc_en;      /* ECC 使能*/
    ZXIC_UINT32 rw_addr;     /*以数据宽度为单位的index*/
}DPP_DTB_DDR_TABLE_FORM_T;

typedef struct dpp_dtb_eram_table_form_t
{
	ZXIC_UINT32 valid;        /* 有效标识 1有效*/
	ZXIC_UINT32 type_mode;    /* ERAM：0x0 */
	ZXIC_UINT32 data_mode;    /*数据长度 00:128  01:64  10:1*/
	ZXIC_UINT32 cpu_wr;       /*CPU写使能*/
	ZXIC_UINT32 cpu_rd;       /*CPU读使能*/
	ZXIC_UINT32 cpu_rd_mode;  /*CPU读模式 0：读 1：读清*/
	ZXIC_UINT32 addr;         /*访问eram 1bit为单位*/
	ZXIC_UINT32 data_h;       /*数据高32bit*/
	ZXIC_UINT32 data_l;       /*数据低32bit*/
}DPP_DTB_ERAM_TABLE_FORM_T;

typedef struct dpp_dtb_zcam_table_form_t
{
    ZXIC_UINT32 valid;         /* 有效标识 1有效*/
    ZXIC_UINT32 type_mode;     /* zcam：0x2 */
    ZXIC_UINT32 ram_reg_flag;  /* ram reg 标识 */
    ZXIC_UINT32 zgroup_id;     /* zgroup id */
    ZXIC_UINT32 zblock_id;     /* zblock id */
    ZXIC_UINT32 zcell_id;      /* zcell id */
    ZXIC_UINT32 mask;          /* 掩码 */
    ZXIC_UINT32 sram_addr;     /* ram地址 */

}DPP_DTB_ZCAM_TABLE_FORM_T;

typedef struct dpp_dtb_etcam_table_form_t
{
	ZXIC_UINT32 valid;             /* 有效标识 1有效*/
	ZXIC_UINT32 type_mode;         /* etcam：0x3 */
	ZXIC_UINT32 block_sel;         /*block索引 0 - 7*/
	ZXIC_UINT32 init_en;           /*初始化使能 高有效*/
	ZXIC_UINT32 row_or_col_msk;    /* 1 write row mask reg  0:write col mask reg*/
	ZXIC_UINT32 vben;              /* enable the valid bit addressed by addr*/
	ZXIC_UINT32 reg_tcam_flag;     /* 1:配置内部row_col_mask寄存器 0：读写tcam*/
	ZXIC_UINT32 uload;             /*使能标识删除对应addr的表项条目，（80bit为单位，含义与wr_mode一一对应）*/
	ZXIC_UINT32 rd_wr;             /*读写标志 0写 1读*/
	ZXIC_UINT32 wr_mode;           /*写入掩码，最高8bit，对应bit为1代表对应的80bit的数据*/
	ZXIC_UINT32 data_or_mask;      /*数据或掩码标志 1：写x(data),0:写y(mask)*/
	ZXIC_UINT32 addr;              /*etcam地址（0-511）*/
	ZXIC_UINT32 vbit;              /*valid bit input*/

}DPP_DTB_ETCAM_TABLE_FORM_T;

typedef struct dpp_dtb_mc_hash_table_form_t
{
	ZXIC_UINT32 valid;              /* 有效标识 1有效  */
	ZXIC_UINT32 type_mode;          /* 微码写hash 0x4 */
    ZXIC_UINT32 std_h;              /* sdt信息高32bit */
	ZXIC_UINT32 std_l;              /* sdt信息低32bit */
}DPP_DTB_MC_HASH_TABLE_FORM_T;

/* DTB DUMP表格式 */
typedef struct dpp_dtb_eram_dump_form_t
{
	ZXIC_UINT32 valid;              /* 有效标识 1有效 */
	ZXIC_UINT32 up_type;            /* 00:eram */
	ZXIC_UINT32 base_addr;          /* 128bit为单位 */
	ZXIC_UINT32 tb_depth;           /* 表项深度，每条条目位宽128bit */
	ZXIC_UINT32 tb_dst_addr_h;      /* 数据目的地址高32bit */
	ZXIC_UINT32 tb_dst_addr_l;      /* 数据目的地址低32bit */

}DPP_DTB_ERAM_DUMP_FORM_T;

typedef struct dpp_dtb_ddr_dump_form_t
{
	ZXIC_UINT32 valid;
	ZXIC_UINT32 up_type;            /* 01:ddr */
	ZXIC_UINT32 base_addr;          /* 128bit为单位 */
	ZXIC_UINT32 tb_depth;        /* 表项深度，每条条目位宽512bit */
	ZXIC_UINT32 tb_dst_addr_h;
	ZXIC_UINT32 tb_dst_addr_l;

}DPP_DTB_DDR_DUMP_FORM_T;

typedef struct dpp_dtb_zcam_dump_form_t
{
	ZXIC_UINT32 valid;
	ZXIC_UINT32 up_type;            /* 10:zcam */
	ZXIC_UINT32 zgroup_id;          /*  */
	ZXIC_UINT32 zblock_id;          /*  */
	ZXIC_UINT32 ram_reg_flag;
	ZXIC_UINT32 z_reg_cell_id;
	ZXIC_UINT32 sram_addr;
	ZXIC_UINT32 tb_depth;        /* 表项深度 */
	ZXIC_UINT32 tb_width;        /* 表项宽度 */
	ZXIC_UINT32 tb_dst_addr_h;
	ZXIC_UINT32 tb_dst_addr_l;


}DPP_DTB_ZCAM_DUMP_FORM_T;
typedef struct dpp_dtb_etcam_dump_form_t
{
	ZXIC_UINT32 valid;
	ZXIC_UINT32 up_type;         /* 11:etcam */
	ZXIC_UINT32 block_sel;       /* block num */
	ZXIC_UINT32 addr;            /* 640bit位单位 */
	ZXIC_UINT32 rd_mode;         /* 读模式，共8bit，每bit控制ram中对应位置的80bit数据是否有效*/ 
	ZXIC_UINT32 data_or_mask;    /* data：1 mask：0*/
	ZXIC_UINT32 tb_depth;        /* dump出数据深度，以640bit为单位 */
	ZXIC_UINT32 tb_width;        /* dump出数据宽度 00:80bit 01:160bit 10:320bit 11:640bit */
	ZXIC_UINT32 tb_dst_addr_h;   /* dma地址高32bit */
	ZXIC_UINT32 tb_dst_addr_l;   /* dma地址低32bit */
}DPP_DTB_ETCAM_DUMP_FORM_T;

typedef struct etcam_dump_info_t
{
	ZXIC_UINT32 block_sel;          /* block index 0-7 */
	ZXIC_UINT32 addr;               /* 单个block的RAM地址，范围0~511 640bit为单位 */
	ZXIC_UINT32 rd_mode;            /* 读模式，共8bit，每bit控制ram中对应位置的80bit数据是否有效 */
	ZXIC_UINT32 data_or_mask;       /* data：1 mask：0 参照DPP_ETCAM_DATA_TYPE_E定义*/
	ZXIC_UINT32 tb_depth;           /* dump出表深度，以640bit为单位 */
	ZXIC_UINT32 tb_width;           /* dump出数据宽度 00:80bit 01:160bit 10:320bit 11:640bit */
}ETCAM_DUMP_INFO_T;

/* 表信息结构 */
typedef struct dpp_dtb_field_t
{
    ZXIC_CHAR    *p_name;                         /* 字段名 */
    ZXIC_UINT16  lsb_pos;                         /* 最低比特位置，以寄存器列表为准*/
    ZXIC_UINT16  len;                             /* 字段长度，以比特为单位 */
}DPP_DTB_FIELD_T;

typedef struct dpp_dtb_table_t
{
    ZXIC_CHAR    *table_type;                       /* 表类型名称*/
    ZXIC_UINT32  table_no;                          /* 表编号 */
    ZXIC_UINT32  field_num;                         /* 包含的字段个数 */
    DPP_DTB_FIELD_T *p_fields;                      /* 表格式所有字段 */
}DPP_DTB_TABLE_T;

typedef struct dpp_dtb_entry_t
{
	ZXIC_UINT8 *cmd;		      /* 命令 128bit 即 16B*/
	ZXIC_UINT8 *data;	          /* 数据 */
	ZXIC_UINT32 data_in_cmd_flag; /*eram 1/64 bit模式时使用,1表示data在cmd中*/
	ZXIC_UINT32 data_size;        /* 数据长度，以字节为单位 */
}DPP_DTB_ENTRY_T;

typedef struct dpp_dtb_cmd_t
{
	ZXIC_UINT32 queue_id;          /*队列id*/
    ZXIC_UINT32 dtb_phy_addr_hi32; /*dtb描述符物理地址高32bit*/
    ZXIC_UINT32 dtb_phy_addr_lo32; /*dtb描述符物理地址低32bit*/
	ZXIC_UINT32 cmd_type;          /*0为流表下发命令，1为流表dump命令*/
	ZXIC_UINT32 int_enable;        /* 中断使能 */
	ZXIC_UINT32 dtb_len;           /* 指示配表内容或dump描述符长度，以16字节为单位 */
}DPP_DTB_CMD_T;


typedef struct dpp_dtb_mc_hash_key_t
{
    ZXIC_UINT32 hash_key[16];
}DPP_DTB_MC_HASH_KEY_T;

typedef struct dpp_dtb_mixed_table_t
{
    ZXIC_UINT32 down_cmd_len;        /**down描述符长度，字节为单位*/
    ZXIC_UINT32 dump_cmd_len;        /**dump描述符长度，字节为单位*/
    ZXIC_UINT32 down_buff_offset;    /**down buff 偏移，字节为单位*/
    ZXIC_UINT32 dump_buff_offset;    /**dump buff 偏移，字节为单位*/
    ZXIC_UINT8 * p_down_cmd_buff;    /**指向下表描述符空间，空间为16KB*/
    ZXIC_UINT8 * p_dump_cmd_buff;    /**指向dump描述符空间，空间为4KB*/
}DPP_DTB_MIXED_TABLE_T;

typedef struct dpp_dtb_mc_hash_entry_info_t
{
    ZXIC_UINT32 delete_en; /* delete 浣胯兘 */
    ZXIC_UINT32 dma_en;    /* dma 浣胯兘 */
    ZXIC_UINT32 *p_data;    /*hash 鏉＄洰鎸囬拡 闀垮害512bit*/    
}DPP_DTB_MC_HASH_ENTRY_INFO_T;

/** dtb 中断配置
* @param   int_enable  中断使能
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_interrupt_status_set(ZXIC_UINT32 int_enable);

/** dtb 中断获取
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
ZXIC_UINT32 dpp_dtb_interrupt_status_get(ZXIC_VOID);

/** dtb cmd 大小端设置
* @param   int_enable  中断使能
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_cmd_endian_status_set(ZXIC_UINT32 endian);

/** dtb cmd 大小端获取
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_cmd_endian_status_get(ZXIC_VOID);

/***********************************************************/
/** hash表项查找校验(软件获取数据)
* @param   p_entry         入参：hash表项键值 出参：hash表项结果
* @param   key_by_size     键值大小 
* @param   rst_by_size     返回值大小
* @param   p_item_info     512bit单元数据
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_hash_software_item_check(DPP_HASH_ENTRY *p_entry,
                                        ZXIC_UINT32 key_by_size,
                                        ZXIC_UINT32 rst_by_size,
                                        SE_ITEM_CFG *p_item_info);

/** 计算eram 128bit为单位的index
* @param   dev_id        设备号
* @param   eram_mode     eram 位宽模式
* @param   index         以eram_mode为单位的index
* @param   p_row_index   出参，行
* @param   p_col_index   出参，列
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dtb_eram_index_cal(DPP_DEV_T *dev, 
                              ZXIC_UINT32 eram_mode, 
                              ZXIC_UINT32 index,
                              ZXIC_UINT32 *p_row_index, 
                              ZXIC_UINT32* p_col_index);

/** dtb写smmu0中的数据，数据长度不限
* @param   dev_id          设备号
* @param   queue_id        队列号
* @param   smmu0_base_addr smmu0基地址，以128bit为单位
* @param   smmu0_wr_mode   smmu0写模式，参考DPP_ERAM128_OPR_MODE_E，仅支持128bit、64bit、1bit模式
* @param   entry_num       下发的条目数
* @param   p_entry_arr     待下发表项内容结构体数组指针
* @param   element_id      返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2024/01/04
************************************************************/
DPP_STATUS dpp_dtb_smmu0_data_write(DPP_DEV_T *dev, 
                                    ZXIC_UINT32 queue_id,
                                    ZXIC_UINT32 smmu0_base_addr,
                                    ZXIC_UINT32 smmu0_wr_mode,
                                    ZXIC_UINT32 entry_num,
                                    DPP_DTB_ERAM_ENTRY_INFO_T *p_entry_arr,
                                    ZXIC_UINT32 *element_id);

/** dtb flush smmu0中的数据,大数据量
* @param   dev_id          设备号
* @param   queue_id        队列号
* @param   smmu0_base_addr smmu0基地址，以128bit为单位
* @param   smmu0_wr_mode   smmu0写模式，参考DPP_ERAM128_OPR_MODE_E，仅支持128bit、64bit、1bit模式
* @param   start_index     flush开始的条目
* @param   entry_num       下发的条目数
* @param   element_id      返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2024/01/04
************************************************************/
DPP_STATUS dpp_dtb_smmu0_flush(DPP_DEV_T *dev,
                               ZXIC_UINT32 queue_id,
                               ZXIC_UINT32 smmu0_base_addr,
                               ZXIC_UINT32 smmu0_wr_mode,
                               ZXIC_UINT32 start_index,
                               ZXIC_UINT32 entry_num,
                               ZXIC_UINT32 *element_id);

/** dtb写eRam表
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    eram表sdt表号
* @param   entry_cnt 下发的条目数
* @param   p_entry_arr   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_eram_dma_write(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 sdt_no, 
                                      ZXIC_UINT32 entry_num, 
                                      DPP_DTB_ERAM_ENTRY_INFO_T *p_entry_arr,
                                      ZXIC_UINT32 *element_id);

/** dtb模拟微码插入HASH表项
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    hash表sdt表号
* @param   entry_cnt 下发的条目数
* @param   p_arr_hash_entry   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_hash_simu_mcode_insert(DPP_DEV_T *dev,
                                   ZXIC_UINT32 queue_id,
                                   ZXIC_UINT32 sdt_no,
                                   ZXIC_UINT32 entry_num,
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id);

/** dtb模拟微码删除HASH表项
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    hash表sdt表号
* @param   entry_cnt 下发的条目数
* @param   p_arr_hash_entry   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_hash_simu_mcode_delete(DPP_DEV_T *dev,
                                   ZXIC_UINT32 queue_id,
                                   ZXIC_UINT32 sdt_no,
                                   ZXIC_UINT32 entry_num,
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id);

/** dtb模拟微码插入HASH表项
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    hash表sdt表号
* @param   entry_cnt 下发的条目数
* @param   p_arr_hash_entry   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_hash_simu_mcode_insert(DPP_DEV_T *dev,
                                   ZXIC_UINT32 queue_id,
                                   ZXIC_UINT32 sdt_no,
                                   ZXIC_UINT32 entry_num,
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id);

/** dtb模拟微码删除HASH表项
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    hash表sdt表号
* @param   entry_cnt 下发的条目数
* @param   p_arr_hash_entry   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_hash_simu_mcode_delete(DPP_DEV_T *dev,
                                   ZXIC_UINT32 queue_id,
                                   ZXIC_UINT32 sdt_no,
                                   ZXIC_UINT32 entry_num,
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id);

/** dtb写HASH表,在插入条目时如果冲突，则对冲突条目进行记录
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    hash表sdt表号
* @param   entry_cnt 下发的条目数
* @param   p_entry_arr   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无 是否是有一个写不成功就返回，还是继续进行下一个条目并记录错误的条目
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_hash_dma_insert(DPP_DEV_T *dev, 
                                   ZXIC_UINT32 queue_id, 
                                   ZXIC_UINT32 sdt_no, 
                                   ZXIC_UINT32 entry_num, 
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id);

/** dtb删除HASH表
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    hash表sdt表号
* @param   entry_cnt 删除的条目数
* @param   p_entry_arr   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无 
* @see
* @author        @date  2023/03/14
************************************************************/
DPP_STATUS dpp_dtb_hash_dma_delete(DPP_DEV_T *dev, 
                                   ZXIC_UINT32 queue_id, 
                                   ZXIC_UINT32 sdt_no, 
                                   ZXIC_UINT32 entry_num, 
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id); 

/** dtb删除HASH表
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    hash表sdt表号
* @param   entry_cnt 删除的条目数
* @param   p_entry_arr   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无 
* @see
* @author        @date  2023/03/14
************************************************************/
DPP_STATUS dpp_dtb_hash_dma_delete_cycle(DPP_DEV_T *dev, 
                                   ZXIC_UINT32 queue_id, 
                                   ZXIC_UINT32 sdt_no, 
                                   ZXIC_UINT32 entry_num, 
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id); 
								   
/** dtb写ACL表 （SPECIFY模式，条目中指定handle，支持级联64bit/128bit）
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    ACL表sdt表号
* @param   entry_num 下发的条目数
* @param   p_acl_entry_arr   待下发表项内容结构体数组指针
* @param   element_id    返回下表使用的元素id
* @return
* @remark  无 是否是有一个写不成功就返回，还是继续进行下一个条目并记录错误的条目
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_acl_dma_insert(DPP_DEV_T *dev,  
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 sdt_no, 
                                      ZXIC_UINT32 entry_num, 
                                      DPP_DTB_ACL_ENTRY_INFO_T *p_acl_entry_arr,
                                      ZXIC_UINT32 *element_id
                                      );

/** smmu0 dump 只写一个dump描述符的接口------spin锁
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   base_addr 要dump的内容的基地址，以128bit为单位
* @param   depth     dump的深度以128bit为单位
* @param   p_data    dump出数据缓存(128bit * depth)
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_spin_se_smmu0_dma_dump(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 base_addr, 
                                      ZXIC_UINT32 depth,
                                      ZXIC_UINT32 *p_data,
                                      ZXIC_UINT32 *element_id);

/** smmu0 组装dump描述符函数
* @param   dev_id         设备号
* @param   base_addr      smmu0空间基地址，以128bit为单位
* @param   depth          dump的深度以128bit为单位
* @param   addr_high32    dump缓存地址高32bit
* @param   addr_low32     dump缓存地址低32bit
* @param   p_dump_info    dump描述符指针（已分配好空间128bit）
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_smmu0_dump_info_write(DPP_DEV_T *dev,  
                                      ZXIC_UINT32 base_addr, 
                                      ZXIC_UINT32 depth,
                                      ZXIC_UINT32 addr_high32,
                                      ZXIC_UINT32 addr_low32,
                                      ZXIC_UINT32 *p_dump_info);

/** dtb dump eram直接表表项内容
* @param   dev_id       设备号
* @param   queue_id     队列号
* @param   sdt_no       eram表sdt表号
* @param   p_dump_eram_entry  eram数据结构，数据已分配相应内存
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_eram_data_get(DPP_DEV_T *dev, 
                                 ZXIC_UINT32 queue_id, 
                                 ZXIC_UINT32 sdt_no, 
                                 DPP_DTB_ERAM_ENTRY_INFO_T *p_dump_eram_entry);

/** dtb dump eram直接表表项内容
* @param   dev_id       设备号
* @param   queue_id     队列号
* @param   sdt_no       eram表sdt表号
* @param   p_dump_eram_entry  eram数据结构，数据已分配相应内存
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_eram_stat_data_get(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 base_addr, 
                                      ZXIC_UINT32 rd_mode, 
                                      ZXIC_UINT32 index,
                                      ZXIC_UINT32 *p_data);

/** dtb dump eram直接表表项内容
* @param   dev_id       设备号
* @param   queue_id     队列号
* @param   sdt_no       eram表sdt表号
* @param   p_dump_eram_entry  eram数据结构，数据已分配相应内存
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_spin_eram_stat_data_get(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 base_addr, 
                                      ZXIC_UINT32 rd_mode, 
                                      ZXIC_UINT32 index,
                                      ZXIC_UINT32 *p_data);

/***********************************************************/
/** 配置数据获取模式
* @param   srh_mode   0:软件获取  1:硬件获取
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
ZXIC_VOID dpp_dtb_srh_mode_set(ZXIC_UINT32 srh_mode);

/** 获取查找方式
* @param   
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
ZXIC_UINT32 dpp_dtb_srh_mode_get(ZXIC_VOID);

/** 根据键值查找hash表
* @param   dev_id       设备号，支持多芯片 
* @param   queue_id     队列id 
* @param   sdt_no       0~255 
* @param   p_dtb_hash_entry    出参，返回描述符信息
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/

ZXIC_UINT32 dpp_dtb_hash_data_get(DPP_DEV_T *dev, 
                                ZXIC_UINT32 queue_id,
                                ZXIC_UINT32 sdt_no,
                                DPP_DTB_HASH_ENTRY_INFO_T *p_dtb_hash_entry,
                                ZXIC_UINT32 srh_mode);
                

ZXIC_UINT32 dpp_dtb_hash_zcam_get(DPP_DEV_T *dev, 
                                 ZXIC_UINT32 queue_id,
                                 HASH_ENTRY_CFG  *p_hash_entry_cfg,
                                 DPP_HASH_ENTRY  *p_hash_entry,
                                 ZXIC_UINT32 srh_mode,
                                 ZXIC_UINT8 *p_srh_succ);

DPP_STATUS dpp_dtb_hash_zcam_get_hardware(DPP_DEV_T *dev, 
                                 ZXIC_UINT32 queue_id,
                                 HASH_ENTRY_CFG  *p_hash_entry_cfg,
                                 DPP_HASH_ENTRY  *p_hash_entry,
                                 ZXIC_UINT8 *p_srh_succ);

DPP_STATUS dpp_dtb_hash_get_software(DPP_DEV_T *dev, 
                                 HASH_ENTRY_CFG  *p_hash_entry_cfg,
                                 DPP_HASH_ENTRY  *p_hash_entry,
                                 ZXIC_UINT8 *p_srh_succ);            

/** dtb 通过key和mask获取ACL表级联结果
* @param   dev_id       设备号
* @param   queue_id     队列号
* @param   sdt_no       eram表sdt表号
* @param   p_dump_acl_entry  etcam 数据结构，数据已分配相应内存,需要输入key和mask
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_acl_data_get(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id, 
                                ZXIC_UINT32 sdt_no, 
                                DPP_DTB_ACL_ENTRY_INFO_T *p_dump_acl_entry);

/** dtb etcam 数据get接口，通过handle值获取etcam数据
* @param   dev_id       设备号
* @param   queue_id     队列号
* @param   sdt_no       eram表sdt表号
* @param   p_dump_acl_entry  etcam 数据结构，数据已分配相应内存
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_etcam_data_get(DPP_DEV_T *dev,
                                  ZXIC_UINT32 queue_id, 
                                  ZXIC_UINT32 sdt_no, 
                                  DPP_DTB_ACL_ENTRY_INFO_T *p_dump_acl_entry);


/***********************************************************/
/** flush当前hash引擎占用的ZCAM空间
* @param   p_se_cfg   全局数据结构
* @param   queue_id   队列id
* @param   fun_id     hash引擎0~3
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_zcam_space_clr(DPP_DEV_T *dev,
                                  DPP_SE_CFG *p_se_cfg,
                                  ZXIC_UINT32 queue_id,
                                  ZXIC_UINT32 fun_id);

/***********************************************************/
/** flush指定eram空间
* @param   dev_id     设备id
* @param   queue_id   队列id
* @param   sdt_no     sdt号
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_eram_table_flush(DPP_DEV_T *dev,
                               ZXIC_UINT32 queue_id,
                               ZXIC_UINT32 sdt_no);

/***********************************************************/
/** flush指定hash空间(DDR/ZCAM)
* @param   dev_id     设备id
* @param   queue_id   队列id
* @param   sdt_no     sdt号
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_hash_table_flush(DPP_DEV_T *dev,
                               ZXIC_UINT32 queue_id,
                               ZXIC_UINT32 sdt_no);

/***********************************************************/
/** 清除hash表资源(硬件和软件，硬件通过dtb方式清除)
* @param   dev_id     设备id
* @param   queue_id   队列id
* @param   hash_id    hash引擎 0~3
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/09/26
************************************************************/
DPP_STATUS dpp_dtb_hash_all_entry_delete(DPP_DEV_T *dev, 
                                        ZXIC_UINT32 queue_id, 
                                        ZXIC_UINT32 hash_id);

/***********************************************************/
/** DTB etcam 整个流表清空Flush 
* @param   devId      NP设备号
* @param   queueId    DTB队列编号
* @param   sdtNo      流表std号
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/  
DPP_STATUS dpp_dtb_etcam_table_flush(DPP_DEV_T *dev,
                                     ZXIC_UINT32 queue_id,
                                     ZXIC_UINT32 sdt_no);

/** dtb dump eram直接表表项内容 支持64bit/128bit
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    eram表sdt表号
* @param   start_index 要dump的起始index，单位是sdt_no该表的mode
* @param   p_dump_data_arr 本次dump出的数据，数据格式与下表格式相同
* @param   entry_num       本次dump实际的条目数
* @param   next_start_index  下次dump是开始的index  
* @param   finish_flag       整个表dump完成标志，1表示完成，0表示未完成
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
 DPP_STATUS dpp_dtb_eram_table_dump(DPP_DEV_T *dev,
                               ZXIC_UINT32 queue_id,
                               ZXIC_UINT32 sdt_no,
                               DPP_DTB_DUMP_INDEX_T start_index,
                               DPP_DTB_ERAM_ENTRY_INFO_T* p_dump_data_arr,
                               ZXIC_UINT32 *entry_num,
                               DPP_DTB_DUMP_INDEX_T *next_start_index,
                               ZXIC_UINT32 *finish_flag);

/***********************************************************/
/** dump eram表内容
* @param   dev            设备
* @param   queue_id       队列id 0~127
* @param   sdt_no         sdt号  0~255
* @param   pDumpData      出参，dump数据，内存由用户分配，结构体DPP_HASH_ENTRY
* @param   entryNum       出参，dump出的有效hash条目
* @return
* @remark  无
* @see
* @author  cq      @date  2025/04/03
************************************************************/
DPP_STATUS dpp_dtb_eram_dump(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id,
                                ZXIC_UINT32 sdt_no,
                                ZXIC_UINT8  *pDumpData,
                                ZXIC_UINT32 *p_entry_num);

/***********************************************************/
/** 只dump hash表的zcam内容
* @param   dev_id         设备id   
* @param   queue_id       队列id 0~127
* @param   sdt_no         sdt号  0~255
* @param   pDumpData      出参，dump数据，内存由用户分配，结构体DPP_DTB_HASH_ENTRY_INFO_T
* @param   entryNum       出参，dump出的有效hash条目
* @return
* @remark  无
* @see
* @author  cq      @date  2023/08/22
************************************************************/  
DPP_STATUS dpp_dtb_hash_table_only_zcam_dump(DPP_DEV_T *dev,
                                            ZXIC_UINT32 queue_id,
                                            ZXIC_UINT32 sdt_no,
                                            ZXIC_UINT8* pDumpData,
                                            ZXIC_UINT32 *entryNum);

/** dtb dump etcam直接表表项内容 级联eram支持64bit/128bit
* @param   dev_id             设备号
* @param   queue_id           队列号
* @param   sdt_no             acl表sdt表号
* @param   start_index        要dump的起始index，单位是sdt_no该表的mode
* @param   p_dump_data_arr    本次dump出的数据，数据格式与下表格式相同
* @param   entry_num          本次dump实际的条目数
* @param   next_start_index   下次dump是开始的index  
* @param   finish_flag        整个表dump完成标志，1表示完成，0表示未完成
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
 DPP_STATUS dpp_dtb_acl_table_dump(DPP_DEV_T *dev,
                               ZXIC_UINT32 queue_id,
                               ZXIC_UINT32 sdt_no,
                               DPP_DTB_DUMP_INDEX_T start_index,
                               DPP_DTB_ACL_ENTRY_INFO_T* p_dump_data_arr,
                               ZXIC_UINT32 *entry_num,
                               DPP_DTB_DUMP_INDEX_T *next_start_index,
                               ZXIC_UINT32 *finish_flag);

/***********************************************************/
/** dump eram表内容
* @param   dev            设备
* @param   queue_id       队列id 0~127
* @param   sdt_no         sdt号  0~255
* @param   pDumpData      出参，dump数据，内存由用户分配，结构体DPP_HASH_ENTRY
* @param   entryNum       出参，dump出的有效hash条目
* @return
* @remark  无
* @see
* @author  cq      @date  2025/04/03
************************************************************/
DPP_STATUS dpp_dtb_acl_dump(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id,
                                ZXIC_UINT32 sdt_no,
                                ZXIC_UINT8  *pDumpData,
                                ZXIC_UINT32 *p_entry_num);

ZXIC_VOID dpp_data_buff_print(ZXIC_UINT8 *buff, ZXIC_UINT32 size);   
ZXIC_VOID dpp_acl_data_print(ZXIC_UINT8 *p_data, ZXIC_UINT8 *p_mask, ZXIC_UINT32 etcam_mode);                                   
ZXIC_VOID dpp_dtb_data_print(ZXIC_UINT8 *p_data, ZXIC_UINT32 len);

ZXIC_UINT32 dpp_ddr_index_calc(ZXIC_UINT32 index,
                               ZXIC_UINT32 width_mode,
                               ZXIC_UINT32 key_type,
                               ZXIC_UINT32 byte_offset);

DPP_STATUS dpp_dtb_hash_dma_delete_hardware(DPP_DEV_T *dev, 
                                   ZXIC_UINT32 queue_id, 
                                   ZXIC_UINT32 sdt_no, 
                                   ZXIC_UINT32 entry_num, 
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id);

ZXIC_UINT32 dpp_dtb_hash_zcam_delete_hardware(DPP_DEV_T *dev, 
                                 ZXIC_UINT32 queue_id,
                                 HASH_ENTRY_CFG  *p_hash_entry_cfg,
                                 DPP_HASH_ENTRY  *p_hash_entry,
                                 DPP_DTB_ENTRY_T *p_entry,
                                 ZXIC_UINT8 *p_srh_succ);

DPP_STATUS dpp_dtb_se_zcam_dma_dump(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 addr, 
                                      ZXIC_UINT32 tb_width,
                                      ZXIC_UINT32 depth,
                                      ZXIC_UINT32 *p_data,
                                      ZXIC_UINT32 *element_id);

DPP_STATUS dpp_dtb_hash_dump(DPP_DEV_T *dev, 
                                ZXIC_UINT32 queue_id, 
                                ZXIC_UINT32 sdt_no, 
                                ZXIC_UINT8  *pDumpData, 
                                ZXIC_UINT32 *p_entry_num);

ZXIC_UINT32 dpp_dtb_hash_data_parse(ZXIC_UINT32 item_type, 
                                   ZXIC_UINT32 key_by_size,
                                   DPP_HASH_ENTRY *p_entry,
                                   ZXIC_UINT8  *p_item_data,
                                   ZXIC_UINT8  *p_data_offset);

/***********************************************************/
/** 释放vport下的所有index
* @param   dev             NP设备
* @param   sdt_no          流表sdt号(0~255)
* @param   vport           端口号
* @param   index           需要释放的索引值
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/  
DPP_STATUS dpp_dtb_acl_index_release_by_vport(DPP_DEV_T *dev,
                                     ZXIC_UINT32 sdt_no,
                                     ZXIC_UINT32 vport);

/***********************************************************/
/** 释放网卡上的所有index
* @param   dev_id          NP设备号
* @param   sdt_no          流表sdt号(0~255)
* @return
* @remark  无
* @see
* @author  cq      @date  2026/01/12
************************************************************/  
DPP_STATUS dpp_dtb_acl_index_release_all(DPP_DEV_T *dev,
                                     ZXIC_UINT32 sdt_no);   

/***********************************************************/
/** 获取当前vport下分配的所有index
* @param   dev             设备
* @param   queue_id        队列号
* @param   eram_sdt_no     维护index的eram直接表号
* @param   vport           端口号
* @param   index_num       出参，当前vport分配的index个数
* @param   p_index_array   出参，当前vport分配的index数组
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/18
************************************************************/
DPP_STATUS dpp_dtb_acl_index_parse(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id,
                                ZXIC_UINT32 eram_sdt_no,
                                ZXIC_UINT32 vport, 
                                ZXIC_UINT32 *index_num,
                                ZXIC_UINT32 *p_index_array);

/***********************************************************/
/** 获取芯片已分配的所有index
* @param   dev             设备
* @param   queue_id        队列号
* @param   eram_sdt_no     维护index的eram直接表号
* @param   index_num       出参，当前vport分配的index个数
* @param   p_index_array   出参，当前vport分配的index数组
* @return
* @remark  无
* @see
* @author  cq      @date  2025/12/25
************************************************************/
DPP_STATUS dpp_dtb_acl_index_parse_all(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id,
                                ZXIC_UINT32 eram_sdt_no,
                                ZXIC_UINT32 *index_num,
                                ZXIC_UINT32 *p_index_array);

/***********************************************************/
/** 清除指定index的所有eram表项
* @param   dev             设备
* @param   queue_id        队列号
* @param   sdt_no          维护index的eram直接表号
* @param   index_num       当前vport分配的index个数
* @param   p_index_array   当前vport分配的index数组
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/18
************************************************************/
DPP_STATUS dpp_dtb_eram_data_clear(DPP_DEV_T *dev, 
                                  ZXIC_UINT32 queue_id, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 index_num, 
                                  ZXIC_UINT32 *p_index_array);

/***********************************************************/
/** dtb方式清除指定index的所有统计项
* @param   dev             设备
* @param   queue_id        队列号
* @param   counter_id      统计编号，对应微码中的address
* @param   rd_mode         统计读取方式 0:64bit 1:128bit
* @param   sdt_no          维护index的eram直接表号
* @param   index_num       当前vport分配的index个数
* @param   p_index_array   当前vport分配的index数组
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/18
************************************************************/
DPP_STATUS dpp_dtb_eram_stat_data_clear(DPP_DEV_T *dev, 
                                  ZXIC_UINT32 queue_id, 
                                  ZXIC_UINT32 counter_id,
                                  STAT_CNT_MODE_E rd_mode,
                                  ZXIC_UINT32 index_num, 
                                  ZXIC_UINT32 *p_index_array);

/***********************************************************/
/** 清除指定index的所有acl表项
* @param   dev             设备
* @param   queue_id        队列号
* @param   sdt_no          acl表项的sdt号
* @param   index_num       当前vport分配的index个数
* @param   p_index_array   当前vport分配的index数组
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/18
************************************************************/
DPP_STATUS dpp_dtb_acl_data_clear(DPP_DEV_T *dev, 
                                  ZXIC_UINT32 queue_id, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 index_num, 
                                  ZXIC_UINT32 *p_index_array);

/***********************************************************/
/** 指定vport的统计计数读清
* @param   dev              NP设备
* @param   sdt_no           流表sdt号(0~255)
* @param   vport            端口号
* @param   rd_mode          读取位宽模式，参见STAT_CNT_MODE_E，0-64bit，1-128bit
* @param   start_counter_id 统计起始编号，对应微码中的address
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/  
DPP_STATUS dpp_dtb_acl_stat_cnt_clr(DPP_DEV_T *dev,
                                     ZXIC_UINT32 sdt_no,
                                     ZXIC_UINT32 vport,
                                     STAT_CNT_MODE_E rd_mode,
                                     ZXIC_UINT32 start_counter_id);

#ifdef __cplusplus
}
#endif

#endif
