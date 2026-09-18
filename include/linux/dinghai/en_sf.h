#ifndef __ZXDH_EN_SF_H__
#define __ZXDH_EN_SF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/zxdh_auxiliary_bus.h>
#include <linux/dinghai/dh_cmd.h>

struct zxdh_en_sf_if {
    int (*en_sf_vq_irqs_request)(struct dh_core_dev *dh_dev, struct dh_irq **vq_irqs, int vq_channels, void *data);
    void (*en_sf_affinity_irqs_release)(struct dh_core_dev *dh_dev, struct dh_irq **irqs, int32_t num_irqs);
    void __iomem * (*en_sf_map_vq_notify)(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, resource_size_t *pa);
    void (*en_sf_unmap_vq_notify)(struct dh_core_dev *dh_dev, void *priv);
    void (*en_sf_set_status)(struct dh_core_dev *dh_dev, uint8_t status);
    uint8_t (*en_sf_get_status)(struct dh_core_dev *dh_dev);
    uint8_t (*en_sf_get_cfg_gen)(struct dh_core_dev *dh_dev);
    bool (*en_sf_get_rp_link_status)(struct dh_core_dev *dh_dev);
    void (*en_sf_set_vf_mac)(struct dh_core_dev *dh_dev, uint8_t *mac, int32_t vf_id);
    void (*en_sf_get_vf_mac)(struct dh_core_dev *dh_dev, uint8_t *mac, int32_t vf_id);
    void (*en_sf_set_mac)(struct dh_core_dev *dh_dev, uint8_t *mac);
    void (*en_sf_get_mac)(struct dh_core_dev *dh_dev, uint8_t *mac);
    uint64_t (*en_sf_get_features)(struct dh_core_dev *dh_dev);
    void (*en_sf_set_features)(struct dh_core_dev *dh_dev, uint64_t features);
    void (*en_sf_set_queue_enable)(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, bool enable);
    uint16_t (*en_sf_get_channels_num)(struct dh_core_dev *dh_dev);
    uint16_t (*en_sf_get_queue_num)(struct dh_core_dev *dh_dev);
    uint16_t (*en_sf_get_queue_size)(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index);
    uint16_t (*en_sf_get_queue_vector)(struct dh_core_dev *dh_dev, uint16_t channel, struct list_head *eqs_list, uint16_t phy_index, uint16_t index, uint16_t vq_idx);
    void (*en_sf_release_queue_vector)(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index);
    void (*en_sf_set_queue_size)(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, uint16_t size);
    void (*en_sf_set_queue_address)(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, uint64_t desc_addr, uint64_t driver_addr, uint64_t device_addr);
    void (*en_sf_switch_irq)(struct dh_core_dev *dh_dev, int32_t i, int32_t op);
    int32_t (*en_sf_get_vq_lock)(struct dh_core_dev *dh_dev);
    int32_t (*en_sf_release_vq_lock)(struct dh_core_dev *dh_dev);
    int32_t (*en_sf_find_valid_vqs)(struct dh_core_dev *dh_dev, uint16_t vq_cnt, int32_t *phy_index);
    int32_t (*en_sf_write_vqs_bit)(struct dh_core_dev *dh_dev, uint16_t vq_cnt, uint32_t *phy_index);
    int32_t (*en_sf_write_queue_tlb)(struct dh_core_dev *dh_dev, uint16_t vq_cnt, uint32_t *phy_index, bool need_msgq);
    uint16_t (*en_sf_get_fw_patch)(struct dh_core_dev *dh_dev);
    uint16_t (*en_sf_get_epbdf)(struct dh_core_dev *dh_dev);
    uint64_t (*en_sf_get_spec_sbdf)(struct dh_core_dev *dh_dev);
    bool (*en_sf_is_multi_ep)(struct dh_core_dev *dh_dev);
    uint16_t (*en_sf_get_vport)(struct dh_core_dev *dh_dev);
    enum dh_coredev_type (*en_sf_get_coredev_type)(struct dh_core_dev *dh_dev);
    uint16_t (*en_sf_get_pcie_id)(struct dh_core_dev *dh_dev);
    uint16_t (*en_sf_get_slot_id)(struct dh_core_dev *dh_dev);
    bool (*en_sf_is_bond)(struct dh_core_dev *dh_dev);
    bool (*en_sf_is_upf)(struct dh_core_dev *dh_dev);
    struct pci_dev * (*en_sf_get_pdev)(struct dh_core_dev *dh_dev);
    uint64_t (*en_sf_get_bar_virt_addr)(struct dh_core_dev *dh_dev, uint8_t bar_num);
    uint64_t (*en_sf_get_bar_phy_addr)(struct dh_core_dev *dh_dev, uint8_t bar_num);
    uint64_t (*en_sf_get_bar_size)(struct dh_core_dev *dh_dev, uint8_t bar_num);
    int32_t (*en_sf_msg_send_cmd)(struct dh_core_dev *dh_dev, uint16_t module_id, void *msg, \
                                    void *ack, struct zxdh_bar_extra_para *para);
    int32_t (*en_sf_async_eq_enable)(struct dh_core_dev *dh_dev, struct dh_eq_async *eq, const char *name, bool attach);
    void (*en_sf_nh_attach)(struct dh_core_dev *dev, struct dh_nb *nb, bool attach);
    struct zxdh_vf_item *(*en_sf_get_vf_item)(struct dh_core_dev *dh_dev, uint16_t vf_idx);
    void (*en_sf_set_pf_link_up)(struct dh_core_dev *dh_dev, bool link_up);
    bool (*en_sf_get_pf_link_up)(struct dh_core_dev *dh_dev);
    void (*en_sf_update_pf_link_info)(struct dh_core_dev *dh_dev, struct link_info_struct *link_info_val);
    int32_t (*en_sf_get_drv_msg)(struct dh_core_dev *dh_dev, uint8_t *drv_version, uint8_t *drv_version_len);
    void (*en_sf_set_vepa)(struct dh_core_dev *dh_dev, bool vepa);
    bool (*en_sf_get_vepa)(struct dh_core_dev *dh_dev);
    void (*en_sf_set_bond_num)(struct dh_core_dev *dh_dev, bool add);
    bool (*en_sf_if_init)(struct dh_core_dev *dh_dev);
    int32_t (*en_sf_request_port_info)(struct dh_core_dev *dh_dev, void *data);
    int32_t (*en_sf_release_port_info)(struct dh_core_dev *dh_dev, uint32_t port_id);
    void (*en_sf_get_link_info_from_vqm)(struct dh_core_dev *dh_dev, uint8_t *link_up);
    bool (*en_sf_set_vf_link_info)(struct dh_core_dev *dh_dev, uint16_t vf_idx, uint8_t link_up);
    uint8_t (*en_sf_get_vf_link_info)(struct dh_core_dev *dh_dev, uint16_t vf_idx);
    bool (*en_sf_get_vf_is_probe)(struct dh_core_dev *dh_dev, uint16_t vf_idx);
    void (*en_sf_set_pf_phy_port)(struct dh_core_dev *dh_dev, uint8_t phy_port);
    uint8_t (*en_sf_get_pf_phy_port)(struct dh_core_dev *dh_dev);
    void (*en_sf_set_init_comp_flag)(struct dh_core_dev *dh_dev, uint8_t flag);
    int32_t (*en_sf_events_call_chain)(struct dh_core_dev *dh_dev, unsigned long type, void *data);
    struct zxdh_ipv6_mac_tbl * (*en_sf_get_ip6mac_tbl)(struct dh_core_dev *dh_dev);
    bool (*en_sf_is_nic)(struct dh_core_dev *dh_dev);
    bool (*en_sf_is_special_bond)(struct dh_core_dev *dh_dev);
    bool (*en_sf_is_support_bond_config)(struct dh_core_dev *dh_dev);
    uint8_t (*en_sf_get_queue_pairs)(struct dh_core_dev *dh_dev);
    struct zxdh_core_health * (*en_sf_get_core_health)(struct dh_core_dev *dh_dev);
    int32_t (*en_sf_get_cpl_timeout_if_mask)(struct dh_core_dev *dh_dev);
    int32_t (*en_sf_set_cpl_timeout_mask)(struct dh_core_dev *dh_dev, uint32_t mask);
    int32_t (*en_sf_get_hp_irq_ctrl_status)(struct dh_core_dev *dh_dev);
    int32_t (*en_sf_set_hp_irq_ctrl_status)(struct dh_core_dev *dh_dev, uint32_t status);
    bool (*en_sf_is_rdma_enable)(struct dh_core_dev *dh_dev);
    uint32_t (*en_sf_get_dev_type)(struct dh_core_dev *dh_dev);
    bool (*en_sf_is_panel_port)(struct dh_core_dev *dh_dev);
    uint8_t (*en_sf_get_panel_id)(struct dh_core_dev *dh_dev);
    bool (*en_sf_pf_suport_np_ext_stats)(struct dh_core_dev *dh_dev);
    struct zxdh_np_ext_stats * (*en_sf_get_np_ext_stats)(struct dh_core_dev *dh_dev, uint8_t panel_id);
    bool (*en_sf_is_drs_sec_enable)(struct dh_core_dev *dh_dev);
    bool (*en_sf_is_pf_rate_enable)(struct dh_core_dev *dh_dev);
    bool (*en_sf_is_fw_feature_support)(struct dh_core_dev *dh_dev, uint32_t feature);
    uint16_t (*en_sf_get_ovs_pf_vfid)(struct dh_core_dev *dh_dev);
    uint8_t (*en_sf_get_board_type)(struct dh_core_dev *dh_dev);
    bool (*en_sf_is_hwbond)(struct dh_core_dev *dh_dev, bool is_hwbond, bool update_pf);
    bool (*en_sf_is_rdma_aux_plug)(struct dh_core_dev *dh_dev, bool is_rdma_aux_plug, bool update_pf);
    bool (*en_sf_is_primary_port)(struct dh_core_dev *dh_dev, bool is_primary_port, bool update_pf);
    void (*en_sf_optim_hardware_bond_time)(struct dh_core_dev *dh_dev, bool enable);
    void (*en_sf_update_mp_enable)(struct dh_core_dev *dh_dev, bool enable);
    void (*en_sf_get_psn_feature_info)(struct dh_core_dev *dh_dev, uint8_t *psn_version, bool *dual_tor, bool *quad_tor);
    int32_t (*en_sf_update_hb_file_val)(struct dh_core_dev *dh_dev, uint64_t spec_sbdf, const char *file_name, bool flag);
    bool (*en_sf_get_ro_info_from_fwshrd)(struct dh_core_dev *dh_dev);
    uint32_t (*en_sf_get_bond_config_from_fwshrd)(struct dh_core_dev *dh_dev);
    bool  (*en_sf_update_active_bond_config_to_fwshrd)(struct dh_core_dev *dh_dev, uint32_t active_bond_info);
    bool (*en_sf_is_rdma_no_panel)(struct dh_core_dev *dh_dev);
    bool (*en_sf_is_lowlatency)(struct dh_core_dev *dh_dev);
    void (*en_sf_set_bond_link_info)(struct dh_core_dev *dh_dev, uint16_t bond_link_info);
    void (*en_sf_set_bond_slave_flag)(struct dh_core_dev *dh_dev, bool is_bond_slave);
    uint64_t (*en_sf_get_mcode_feature)(struct dh_core_dev *dh_dev);
    uint8_t (*en_sf_get_bond_port_type)(struct dh_core_dev *dh_dev);
    uint8_t (*en_sf_get_no_bondpf_panel_num)(struct dh_core_dev *dh_dev);
    void *(*en_sf_get_dev_cap)(struct dh_core_dev *dh_dev);
    uint16_t (*en_sf_get_pre_bond_qidx)(struct dh_core_dev *dh_dev);
    void (*en_sf_set_pre_bond_qidx)(struct dh_core_dev *dh_dev, uint16_t pre_bond_qidx);
    void (*en_sf_get_fw_version)(struct dh_core_dev *dh_dev, uint8_t *fw_version);
    void (*en_sf_get_vendor)(struct dh_core_dev *dh_dev, uint8_t *vendor);
    bool (*en_sf_get_split_packed)(struct dh_core_dev *dh_dev);
};

struct zxdh_en_sf_container {
    struct zxdh_auxiliary_device adev;
    struct dh_core_dev *dh_dev;
    struct dh_core_dev *cdev;
    struct zxdh_en_sf_if *ops;
    int    max_channels;
};

int32_t zxdh_en_sf_driver_register(void);
void zxdh_en_sf_driver_unregister(void);


#ifdef __cplusplus
}
#endif

#endif
