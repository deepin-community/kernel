/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_init.c
* 文件标识 :
* 内容摘要 : 芯片初始化源文件
* 其它说明 :
* 当前版本 :
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#include "zxic_common.h"
#include "dpp_module.h"
#include "dpp_dev.h"
#include "dpp_ppu.h"
#include "dpp_se.h"
#include "dpp_dtb.h"
#include "dpp_init.h"
#include "dpp_drv_init.h"

/***********************************************************/
/** 芯片上电初始，完整版本
* @param   dev_id    设备号
* @param   p_init_ctrl  系统初始化控制数据结构，由用户完成实例化和成员赋值
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_init(ZXIC_UINT32 dev_id)
{
    DPP_STATUS rt = 0;
    ZXIC_UINT32 dev_id_array[DPP_DEV_CHANNEL_MAX] = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    
    /* 初始化设备管理模块 */
    rt = dpp_dev_init();
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rt, "dpp_dev_init");

    rt = dpp_dev_add(dev_id,
                     DPP_DEV_TYPE_CHIP,
                     DPP_DEV_ACCESS_TYPE_PCIE,
                     0,
                     0,
                     0,
                     0,
                     NULL,
                     NULL,
                     NULL,
                     NULL);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rt, "dpp_dev_add");

    /* 初始化table模块软件部分 */
    dev_id_array[0] = dev_id;
    rt = dpp_sdt_init(1, dev_id_array);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rt, "dpp_sdt_init");

    rt = dpp_ppu_parse_cls_bitmap(dev_id, DPP_PPU_CLS_ALL_START);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rt, "dpp_ppu_parse_cls_bitmap");
    
    dpp_flow_init_status_init();

    return DPP_OK;
}
