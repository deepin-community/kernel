#ifndef __ZXDH_PF_MSG_FUNC_H__
#define __ZXDH_PF_MSG_FUNC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>
#include "../en_np/flow/api/include/dpp_tbl_fd_cfg.h"
#include "../en_np/driver/include/dpp_drv_sdt.h"
#define RD_CLR_MODE_UNCLR    0 /* 不读清 */
#define RD_CLR_MODE_CLR      1 /* 读清 */

typedef enum
{
    PTP_PORT_VFID_SET,
    PTP_TC_ENABLE_SET,

    MAX_VF_CALL_NP_NUM,
} vf_call_np_num;

int32_t dh_pf_msg_recv_func_register(void);
void dh_pf_msg_recv_func_unregister(void);
void zxdh_vf_item_mac_add(struct zxdh_vf_item *vf_item, uint8_t *mac_addr, uint8_t dhtool_mac_set_flag);
void zxdh_vf_item_mac_del(struct zxdh_vf_item *vf_item, uint8_t *mac_addr);
void zxdh_flow_table_add(struct ethtool_rx_flow_spec *fs, ZXDH_FD_CFG_T *p_fd_cfg, DPP_PF_INFO_T *pf_info);


#ifdef __cplusplus
}
#endif

#endif /* __ZXDH_PF_MSG_FUNC_H__ */
