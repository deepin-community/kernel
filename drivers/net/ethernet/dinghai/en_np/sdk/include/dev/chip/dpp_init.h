/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_init.h
* 文件标识 :
* 内容摘要 : 芯片初始化头文件
* 其它说明 :
* 当前版本 :
* 作    者 : wcl
* 完成日期 : 2015/03/17
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/
#ifndef _DPP_INIT_H_
#define _DPP_INIT_H_

/**  系统初始化标志*/
#define DPP_INIT_FLAG_ACCESS_TYPE      (1<<0)  /**<  @brief 访问模式: 0-PCRISCV1-MDIO*/
#define DPP_INIT_FLAG_SERDES_DOWN_TP   (1<<1)  /**<  @brief serdes加载方式:  0-组播，1-单播*/
#define DPP_INIT_FLAG_DDR_BACKDOOR     (1<<2)  /**<  @brief DDR3校准:  0-校准，1-不校准*/
#define DPP_INIT_FLAG_SA_MODE          (1<<3)  /**<  @brief SA工作模式: 0-内置sa，1-非SA模式*/
#define DPP_INIT_FLAG_SA_MESH          (1<<4)  /**<  @brief SA mesh模式: 0-非mesh，1-mesh*/
#define DPP_INIT_FLAG_SA_SERDES_MODE   (1<<5)  /**<  @brief SA serdes编码方式: 0-64B/66B; 1-8B/10B */
#define DPP_INIT_FLAG_INT_DEST_MODE    (1<<6)  /**<  @brief 中断上报方式: 0-PCIe; 1-LocalBus(管脚)*/
#define DPP_INIT_FLAG_LIF0_MODE        (1<<7)  /**<  @brief 0: PON模式 1: QSGMII模式*/
#define DPP_INIT_FLAG_DMA_ENABLE       (1<<8)  /**<  @brief DMA使能:  0-使能，1-不使能*/
#define DPP_INIT_FLAG_TM_IMEM_FLAG     (1<<9)  /**<  @brief TM片内外模式:  0-片外，1-片内*/

/**  dpp sdk系统初始化控制结构*/
typedef struct dpp_sys_init_ctrl_t
{
    DPP_DEV_TYPE_E device_type;         /**<  @brief 设备类型，取值参考DPP_DEV_TYPE_E的定义*/
    ZXIC_UINT32 flags;                       /**<  @brief 初始化标志，按bitmap使用，*/
    ZXIC_UINT32 sa_id;                       /**<  @brief 内置sa模式下，sa的ID，范围0~127*/
    ZXIC_UINT32 case_num;                    /**<  @brief TM四组QMU初始化场景编号为1-8;ddr*bank:1:4x2;2:4x4;3:8x2;4:4x8.*/
    ZXIC_UINT32 lif0_port_type;              /**<  @brief LIF0是PON口模式下，lif0的PON端口类型，详见DPP_LIF0_PON_TYPE_E */
    ZXIC_UINT32 lif1_port_type;              /**<  @brief 非内置SA模式下，lif1的端口类型，详见DPP_LIF1_PORT_MODE_E */
    ZXIC_ADDR_T pcie_vir_baddr;              /**<  @brief PCIe映射虚拟基地址*/
    ZXIC_ADDR_T riscv_vir_baddr;              /**<  @brief RISCV映射虚拟基地址 */
    ZXIC_ADDR_T dma_vir_baddr;               /**<  @brief DMA映射虚拟地址*/
    ZXIC_ADDR_T dma_phy_baddr;               /**<  @brief DMA内存物理地址*/
    DPP_DEV_WRITE_FUNC pcie_write_fun;  /**<  @brief PCIe硬件写回调函数 */
    DPP_DEV_READ_FUNC  pcie_read_fun;   /**<  @brief PCIe硬件读回调函数*/
    DPP_DEV_WRITE_FUNC riscv_write_fun;  /**<  @brief RISCV硬件写回调函数 */
    DPP_DEV_READ_FUNC  riscv_read_fun;   /**<  @brief RISCV硬件读回调函数*/
    DPP_ACCESS_SWITCH_FUNC access_switch_fun; /**<  @brief PCIe/RISCV切换回调函数*/
} DPP_SYS_INIT_CTRL_T;

/***********************************************************/
/** 芯片上电初始，完整版本
* @param   dev_id    设备号
* @param   p_init_ctrl  系统初始化控制数据结构，由用户完成实例化和成员赋值
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/03/26
************************************************************/
DPP_STATUS dpp_init(ZXIC_UINT32 dev_id);

#endif /* dpp_init.h */
