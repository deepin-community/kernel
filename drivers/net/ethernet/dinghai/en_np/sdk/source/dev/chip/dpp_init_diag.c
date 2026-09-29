#include "zxic_common.h"
#include "dpp_init_diag.h"
#include "dpp_np_init.h"
#include "dpp_dtb.h"

extern DPP_DEV_MGR_T *dpp_dev_mgr_get(ZXIC_VOID);

ZXIC_UINT32 diag_dpp_device_info_prt(ZXIC_VOID)
{
    ZXIC_UINT32 dev_id             = 0;
    ZXIC_UINT32 device_index       = 0;
    ZXIC_UINT16 slot               = 0;
    ZXIC_UINT16 channel_id         = 0;
    ZXIC_UINT32 queue_id           = 0;
    ZXIC_CONST ZXIC_CHAR *type_str = ZXIC_NULL;
    DPP_DEV_CFG_T *p_dev_info      = ZXIC_NULL;
    DPP_DEV_MGR_T *p_dev_mgr       = ZXIC_NULL;
    DPP_DTB_MGR_T *p_dtb_mgr       = ZXIC_NULL;

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);

    if (!p_dev_mgr->is_init)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: Device Manager is not init!!!\n", DPP_RC_DEV_MGR_NOT_INIT);
        return DPP_RC_DEV_MGR_NOT_INIT;
    }

    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info);

    /* 打印标题分隔线 */
    ZXIC_COMM_PRINT("===================================================\n");
    ZXIC_COMM_PRINT("DPP DEVICE INFORMATION DIAGNOSTIC REPORT\n");
    ZXIC_COMM_PRINT("===================================================\n");

    for (slot = 0; slot < DPP_PCIE_SLOT_MAX; slot++)
    {
        p_dtb_mgr = dpp_dtb_mgr_get(slot, dev_id);

        for (channel_id = 0; channel_id < DPP_PCIE_CHANNEL_MAX; channel_id++)
        {
            if (p_dev_info->pcie_channel[slot][channel_id].is_used == 1)
            {
                /* 设备基本信息标题 */
                ZXIC_COMM_PRINT("┌─────────────────────────────────────────────┐\n");
                ZXIC_COMM_PRINT("│ DEVICE [%04u] - SLOT %02u, CHANNEL %04u       │\n", device_index, slot, channel_id);
                ZXIC_COMM_PRINT("└─────────────────────────────────────────────┘\n");

                /* PCIe通道信息 - 使用表格格式 */
                ZXIC_COMM_PRINT("  PCIe Channel Configuration:\n");
                ZXIC_COMM_PRINT("  ┌─────────────────────────────────────────────────────────────┐\n");
                ZXIC_COMM_PRINT("  │ %-15s: 0x%08x  │ %-15s: 0x%08x  │\n", "VPort",
                                p_dev_info->pcie_channel[slot][channel_id].vport, "PCIe ID",
                                p_dev_info->pcie_channel[slot][channel_id].pcie_id);
                ZXIC_COMM_PRINT("  │ %-15s: %10u  │ %-15s: %10u  │\n", "Dev Status",
                                p_dev_info->pcie_channel[slot][channel_id].dev_status, "BAR Msg Num",
                                p_dev_info->bar_msg_num[slot]);
                ZXIC_COMM_PRINT("  └─────────────────────────────────────────────────────────────┘\n");

                /* 地址信息 - 分组显示 */
                ZXIC_COMM_PRINT("  Address Information:\n");
                ZXIC_COMM_PRINT("    Offset Addr : 0x%016llx\n",
                                p_dev_info->pcie_channel[slot][channel_id].offset_addr);
                ZXIC_COMM_PRINT("    Base Addr   : 0x%016llx\n", p_dev_info->pcie_channel[slot][channel_id].base_addr);
                ZXIC_COMM_PRINT("    DMA Phy Addr: 0x%016llx\n",
                                p_dev_info->pcie_channel[slot][channel_id].dump_dma_phy_addr);
                ZXIC_COMM_PRINT("    DMA Vir Addr: 0x%016llx\n",
                                p_dev_info->pcie_channel[slot][channel_id].dump_dma_vir_addr);
                ZXIC_COMM_PRINT("    DMA Size    : 0x%08x\n", p_dev_info->pcie_channel[slot][channel_id].dump_dma_size);

                /* DTB队列信息 */
                if (p_dtb_mgr != ZXIC_NULL)
                {
                    ZXIC_COMM_PRINT("  DTB Queues (VPort 0x%04x):\n", p_dev_info->pcie_channel[slot][channel_id].vport);
                    ZXIC_COMM_PRINT("  ┌──────┬────────────┬────────────────────┬────────────────────┐\n");
                    ZXIC_COMM_PRINT("  │ ID   │ Type       │ UP Address         │ DOWN Address       │\n");
                    ZXIC_COMM_PRINT("  ├──────┼────────────┼────────────────────┼────────────────────┤\n");

                    for (queue_id = 0; queue_id < DPP_DTB_QUEUE_NUM_MAX; queue_id++)
                    {
                        if (p_dtb_mgr->queue_info[queue_id].init_flag != 0)
                        {
                            if (p_dev_info->pcie_channel[slot][channel_id].vport
                                == p_dtb_mgr->queue_info[queue_id].vport)
                            {
                                type_str = (p_dtb_mgr->queue_info[queue_id].func_type == DPP_DTB_QUEUE_TYPE_TABLE)
                                               ? "Table"
                                               : "Statistic";

                                ZXIC_COMM_PRINT("  │ %-4u │ %-10s │ 0x%016llx │ 0x%016llx │\n", queue_id, type_str,
                                                p_dtb_mgr->queue_info[queue_id].tab_up.start_phy_addr,
                                                p_dtb_mgr->queue_info[queue_id].tab_down.start_phy_addr);
                            }
                        }
                    }
                    ZXIC_COMM_PRINT("  └──────┴────────────┴────────────────────┴────────────────────┘\n");
                }
                else
                {
                    ZXIC_COMM_PRINT("  DTB Queue Information: Not Found\n");
                }

                /* 统计记录状态 */
                ZXIC_COMM_PRINT("  Statistics Record: %s\n",
                                (p_dev_info->p_std_nic_res[slot] == ZXIC_NULL) ? "Not Initialized" : "Initialized");

                /* 设备分隔线 */
                ZXIC_COMM_PRINT("\n");
                ZXIC_COMM_PRINT("─────────────────────────────────────────────────────────────\n");
                ZXIC_COMM_PRINT("\n");

                device_index++;
            }
        }
    }

    /* 打印总结信息 */
    if (device_index == 0)
    {
        ZXIC_COMM_PRINT("No active devices found.\n");
    }
    else
    {
        ZXIC_COMM_PRINT("Total Devices Found: %u\n", device_index);
    }

    ZXIC_COMM_PRINT("===================================================\n");
    ZXIC_COMM_PRINT("END OF REPORT\n");
    ZXIC_COMM_PRINT("===================================================\n");

    return DPP_OK;
}
