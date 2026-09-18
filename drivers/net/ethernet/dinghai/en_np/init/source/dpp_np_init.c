#include <linux/pci.h>
#include "zxic_common.h"
#include "dpp_np_init.h"
#include "dpp_dev.h"
#include "dpp_dtb.h"
#include "dpp_init.h"
#include "dpp_kernel_init.h"
#include "dpp_netlink.h"
#include "dpp_dtb_table_api.h"
#include "dpp_drv_init.h"
#include "dpp_tbl_comm.h"
#include "dpp_apt_se.h"
#include "dpp_cmd_init.h"
#include "dpp_agent_channel.h"
#include "dpp_hash.h"
#include "dpp_sdt_mgr.h"
#include "dpp_tbl_pkt_cap.h"
#include "dpp_drv_sdt.h"
#include "dpp_tbl_api.h"
extern DPP_DEV_MGR_T *dpp_dev_mgr_get(ZXIC_VOID);

ZXIC_UINT32 dpp_vport_register(DPP_PF_INFO_T* pf_info, struct pci_dev *p_dev)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_dev);
    ZXIC_COMM_CHECK_INDEX_EQUAL_RETURN_OK(IS_PF(pf_info->vport), 0);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x register start.\n", pf_info->slot, pf_info->vport);

    rc = dpp_dev_pcie_channel_add(pf_info, p_dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_pcie_channel_add");

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_bar_msg_num_init(&dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_bar_msg_num_init");

    rc = dpp_dtb_init(&dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_init");

    rc = dpp_apt_dtb_res_init(&dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_dtb_res_init");

    rc = dpp_dev_status_update(&dev, 1);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_status_update");

    rc = dpp_vport_mgr_init(pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_mgr_init");

    rc = dpp_flow_init(&dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_flow_init");

    rc = dpp_std_nic_stat_recode_init(pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_std_nic_stat_recode_init");

    ZXIC_COMM_PRINT("slot: %u vport: 0x%04x register success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_register);

ZXIC_UINT32 dpp_vport_unregister(DPP_PF_INFO_T* pf_info)
{
    DPP_DEV_T dev = {0};
    ZXIC_UINT32 rc     = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_INDEX_EQUAL_RETURN_OK(IS_PF(pf_info->vport), 0);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x unregister start.\n", pf_info->slot, pf_info->vport);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_pkt_capture_uninit(pf_info);
    ZXIC_COMM_CHECK_RC_NONE(rc, "dpp_pkt_capture_uninit");

    rc = dpp_dev_status_update(&dev, 0);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_status_update");

    rc = dpp_dtb_queue_release_ex(&dev);
    if(rc != DPP_OK)
    {
        rc = dpp_dtb_queue_release_soft(&dev);
        ZXIC_COMM_CHECK_RC_NONE(rc, "dpp_dtb_queue_release_soft");
    }

    rc = dpp_std_nic_stat_recode_uninit(pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_std_nic_stat_recode_uninit");

    rc = dpp_flow_uninit(&dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_flow_uninit");

    rc = dpp_dev_pcie_channel_del(pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_pcie_channel_del");

    rc = dpp_vport_mgr_release(pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_mgr_release");

    ZXIC_COMM_PRINT("slot: %u vport: 0x%04x unregister success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_unregister);

ZXIC_UINT32 dpp_vport_reset(DPP_PF_INFO_T* pf_info)
{
    ZXIC_UINT32 rc     = DPP_OK;
    DPP_DEV_T dev = {0};
    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("start.\n");

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dev_status_update(&dev, 0);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_status_update");

    rc = dpp_dtb_queue_release_soft(&dev);
    ZXIC_COMM_CHECK_RC_NONE(rc, "dpp_dtb_queue_release_soft");

    rc = dpp_rdma_trans_item_soft_delete(pf_info);
    ZXIC_COMM_CHECK_RC_NONE(rc, "dpp_rdma_trans_item_soft_delete");
    
    rc = dpp_unicast_all_mac_soft_delete(pf_info);
    ZXIC_COMM_CHECK_RC_NONE(rc, "dpp_unicast_all_mac_soft_delete");

    rc = dpp_multicast_all_mac_soft_delete(pf_info);
    ZXIC_COMM_CHECK_RC_NONE(rc, "dpp_multicast_all_mac_soft_delete");

    rc = dpp_dev_pcie_channel_del(pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_pcie_channel_del");

    rc = dpp_vport_mgr_release(pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_mgr_release");

    ZXIC_COMM_PRINT("slot: %u vport: 0x%04x reset success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_reset);

ZXIC_UINT32 dpp_dev_status_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 dev_status)
{
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 channel_id = 0;
    ZXIC_UINT32 dev_id = 0;

    DPP_DEV_CFG_T *p_dev_info = NULL;
    DPP_DEV_MGR_T *p_dev_mgr  = NULL;

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pf_info);

    slot = pf_info->slot;
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot, 0, DPP_PCIE_SLOT_MAX - 1);

    channel_id = DPP_PCIE_CHANNEL_ID(pf_info->vport);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    if (!p_dev_mgr->is_init)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: Device Manager is not init!!!\n",
                                                                     DPP_RC_DEV_MGR_NOT_INIT);
        return DPP_RC_DEV_MGR_NOT_INIT;
    }
    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info);

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info->pcie_channel[slot][channel_id].device);

    p_dev_info->pcie_channel[slot][channel_id].dev_status = dev_status;
    
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_dev_status_set);

static int __init dpp_np_init(void)
{
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_TRACE_NOTICE("start.\n");

    rc = dpp_init(0);
    ZXIC_COMM_CHECK_RC(rc, "dpp_init");

    rc = dpp_agent_channel_init();
    ZXIC_COMM_CHECK_RC(rc, "dpp_agent_channel_init");

    rc = dpp_netlink_init();
    ZXIC_COMM_CHECK_RC(rc, "dpp_netlink_init");

    rc = dpp_cmd_init();
    ZXIC_COMM_CHECK_RC(rc, "dpp_cmd_init");

    ZXIC_COMM_TRACE_NOTICE("success.\n");

    return DPP_OK;
}

ZXIC_UINT32 dpp_np_online_uninstall(void)
{
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_hash_tbl_clr(0);
    ZXIC_COMM_CHECK_RC_NONE(rc, "dpp_hash_tbl_clr");

    //rc = dpp_acl_res_destroy(0);
    //ZXIC_COMM_CHECK_RC_NONE(rc, "dpp_acl_res_destroy");

    rc = dpp_dtb_mgr_destory_all();
    ZXIC_COMM_CHECK_RC_NONE(rc, "dpp_dtb_mgr_destory_all");

    rc = dpp_sdt_mgr_destroy(0);
    ZXIC_COMM_CHECK_RC_NONE(rc, "dpp_sdt_mgr_destroy");

    rc = dpp_dev_del(0);
    ZXIC_COMM_CHECK_RC_NONE(rc, "dpp_dev_del");

    return DPP_OK;
}

static void __exit dpp_np_exit(void)
{
    ZXIC_COMM_TRACE_NOTICE("start.\n");

    dpp_netlink_exit();
    dpp_agent_channel_exit();
    dpp_np_online_uninstall();

    ZXIC_COMM_TRACE_NOTICE("success.\n");
}

module_init(dpp_np_init);
module_exit(dpp_np_exit);

MODULE_LICENSE("GPL");
