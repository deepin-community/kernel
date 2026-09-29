#ifndef _DPP_NP_INIT_H_
#define _DPP_NP_INIT_H_

#include <linux/pci.h>
#include "zxic_common.h"
#include "dpp_dev.h"

ZXIC_UINT32 dpp_vport_register(DPP_PF_INFO_T* pf_info, struct pci_dev *p_dev);
ZXIC_UINT32 dpp_vport_unregister(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_vport_reset(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_dev_status_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 dev_status);

#endif
