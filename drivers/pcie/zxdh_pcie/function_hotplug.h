#ifndef _ZXDH_PF_HOT_PLUG_H_
#define _ZXDH_PF_HOT_PLUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/kernel.h>

#define FUNC_HP_RESULT_SUCC 1
#define FUNC_HP_RESULT_FAIL 2

#define FUNC_HP_SCENE_CODE_START_BIT                        21
#define FUNC_HP_FUNC_TYPE_START_BIT                         20
#define FUNC_HP_EP_ID_START_BIT                             16
#define FUNC_HP_PF_ID_START_BIT                             12
#define FUNC_HP_VF_ID_START_BIT                             0

#define FUNC_HP_SCENE_CODE_MASK                             (0x7 << FUNC_HP_SCENE_CODE_START_BIT)
#define FUNC_HP_FUNC_TYPE_MASK                              (0x1 << FUNC_HP_FUNC_TYPE_START_BIT)
#define FUNC_HP_EP_ID_MASK                                  (0xF << FUNC_HP_EP_ID_START_BIT)
#define FUNC_HP_PF_ID_MASK                                  (0xF << FUNC_HP_PF_ID_START_BIT)
#define FUNC_HP_VF_ID_MASK                                  (0xFFF << FUNC_HP_VF_ID_START_BIT)

#define SCENE_CODE_OF_FUNC_HP_INFO(hotplug_info)            ((hotplug_info & FUNC_HP_SCENE_CODE_MASK) >> FUNC_HP_SCENE_CODE_START_BIT)
#define FUNC_TYPE_OF_FUNC_HP_INFO(hotplug_info)             ((hotplug_info & FUNC_HP_FUNC_TYPE_MASK) >> FUNC_HP_FUNC_TYPE_START_BIT)
#define EP_ID_OF_FUNC_HP_INFO(hotplug_info)                 ((hotplug_info & FUNC_HP_EP_ID_MASK) >> FUNC_HP_EP_ID_START_BIT)
#define PF_ID_OF_FUNC_HP_INFO(hotplug_info)                 ((hotplug_info & FUNC_HP_PF_ID_MASK) >> FUNC_HP_PF_ID_START_BIT)
#define VF_ID_OF_FUNC_HP_INFO(hotplug_info)                 ((hotplug_info & FUNC_HP_VF_ID_MASK) >> FUNC_HP_VF_ID_START_BIT)

#define FUNCTION_HP_TYPE_COUNT                              2
#define MAX_EP_NUMS                                         4
#define MAX_PF_NUMS_OF_EP                                   8
#define MAX_PF_NUMS                                         (MAX_PF_NUMS_OF_EP * MAX_EP_NUMS)

#define MAX_RESCAN_NUMS                                     100
#define DEFLUAT_DDR_VALUE                                   0x5a5a5a5a
#define INVALID_MASK_VALUE                                  0xFFFFFFFF
#define MASK_BIT                                            0x1

#define FUC_HOTPLUG_PF_INIT_FLAG_OFFSET                     0x10
#define FUC_HOTPLUG_PF_STATE_FLAG_OFFSET                    0x20
#define FUC_HOTPLUG_PF_INIT_FLAG                            0x450000

#define SWITCH_VENDOR_ID                                    0x1cf2
#define SWITCH_DEVICE_ID                                    0x8036

enum FUC_HOTPLUG_INFO_ {
    PCIE_HOTPLUG_START = 1,
    PCIE_HOTPLUG_FINISH,
};

enum zte_pcie_func_type {
    PCIE_FUNC_TYPE_PF = 0,
    PCIE_FUNC_TYPE_VF,
    PCIE_FUNC_TYPE_NUM
};

typedef enum {
    FUNCTION_REMOVE = 1,
    FUNCTION_INSERT,
} FUNCTION_HP_TYPE;

struct func_hotplug_info {
    unsigned int reserved: 8;
    unsigned int scene_code: 3;     // 1-热拔；2-热插；
    unsigned int function_type: 1;  // pf-0;vf-1;
    unsigned int ep_id: 4;          // 5~9(对应ep0~4)
    unsigned int pf_id: 4;          // 0~7
    unsigned int vf_id: 12;         // 0~127
};

struct func_hotplug_req {
    /* hotplug_info
        bit[31:24]: reserved:
        bit[23:21]: scene_code 1-热拔、2-热插
        bit[20]:    function_type 0-PF、1-VF
        bit[19:16]: ep_id 范围: 5~9(对应ep0~4)
        bit[15:12]: pf_id 范围: 0~7
        bit[11:0]:  vf_id 范围: 0~127
    */
    u32 hotplug_info;
    u32 domain;
    u32 bdf;
};

struct func_hotplug_result {
    unsigned int cmd;

    /* hotplug_info 
        bit[31:24]: reserved:
        bit[23:21]: scene_code 0-热拔、1-热插
        bit[20]:    function_type 0-PF、1-VF
        bit[19:16]: ep_id 范围: 5~9(对应ep0~4)
        bit[15:12]: pf_id 范围: 0~7
        bit[11:0]:  vf_id 范围: 0~127
    */
    u32 hotplug_info;
    u32 result;
};

struct func_hotplug_state {
    u32 pf_state_mask;
    u32 domain[4];
    u32 bdf[4];
};

struct zte_dpu_id_info {
    char *name;
    u16 device_id;
    u16 vendor_id;
};

struct fuc_hotplug_bar_msg {
    unsigned int  cmd;
    unsigned int  fuc_hotplug_info;
    unsigned int  timeout;
    unsigned int  cpl_chk;
};

int remove_pci_dev(struct func_hotplug_req *hp_info);
int rescan_pci_dev(struct pci_dev *hp_pdev);
int vf_bind_unbind(struct func_hotplug_req *hp_req);
int request_hpf_msix(struct pci_dev *pdev);
int free_hpf_msix(struct pci_dev *pdev);
int enable_vf(void);
int init_bdf(struct pci_dev *pdev);
int update_pf_state(struct pci_dev *hp_pdev);
int init_dpu_bus_info(struct pci_dev *pdev);

#ifdef __cplusplus
}
#endif

#endif
