/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 Huawei Technologies Co., Ltd */

#ifndef HINIC3_MULTI_HOST_MGMT_H
#define HINIC3_MULTI_HOST_MGMT_H

#define HINIC3_VF_IN_VM			0x3

#define HINIC3_MGMT_SHOST_HOST_ID	0
#define HINIC3_MAX_MGMT_FUNCTIONS	1024
#define HINIC3_MAX_MGMT_FUNCTIONS_64	(HINIC3_MAX_MGMT_FUNCTIONS / 64)

struct hinic3_multi_host_mgmt {
	struct hinic3_hwdev *hwdev;

	/* slave host registered */
	bool	shost_registered;
	u8	shost_host_idx;
	u8	shost_ppf_idx;

	u8	mhost_ppf_idx;
	u8	rsvd1;

	/* slave host functios support nic enable */
	DECLARE_BITMAP(func_nic_en, HINIC3_MAX_MGMT_FUNCTIONS);
	DECLARE_BITMAP(func_vroce_en, HINIC3_MAX_MGMT_FUNCTIONS);

	struct hinic3_hw_pf_infos pf_infos;

	u64	rsvd2;
};

struct hinic3_host_fwd_head {
	unsigned short dst_glb_func_idx;
	unsigned char dst_itf_idx;
	unsigned char mod;

	unsigned char cmd;
	unsigned char rsv[3];
};

/* software cmds, vf->pf and multi-host */
enum hinic3_sw_funcs_cmd {
	HINIC3_SW_CMD_SLAVE_HOST_PPF_REGISTER = 0x0,
	HINIC3_SW_CMD_SLAVE_HOST_PPF_UNREGISTER,
	HINIC3_SW_CMD_GET_SLAVE_FUNC_NIC_STATE,
	HINIC3_SW_CMD_SET_SLAVE_FUNC_NIC_STATE,
	HINIC3_SW_CMD_SEND_MSG_TO_VF,
	HINIC3_SW_CMD_MIGRATE_READY,
	HINIC3_SW_CMD_GET_SLAVE_NETDEV_STATE,

	HINIC3_SW_CMD_GET_SLAVE_FUNC_VROCE_STATE,
	HINIC3_SW_CMD_SET_SLAVE_FUNC_VROCE_STATE,
	HINIC3_SW_CMD_GET_SLAVE_VROCE_DEVICE_STATE = 0x9, // 与vroce_cfg_vf_do.h宏一致
};

/* multi host mgmt event sub cmd */
enum hinic3_mhost_even_type {
	HINIC3_MHOST_NIC_STATE_CHANGE	 = 1,
	HINIC3_MHOST_VROCE_STATE_CHANGE	 = 2,
	HINIC3_MHOST_GET_VROCE_STATE	 = 3,
};

struct hinic3_mhost_nic_func_state {
	u8 status;
	u8 enable;
	u16 func_idx;
};

struct hinic3_multi_host_mgmt_event {
	u16 sub_cmd;
	u16 rsvd[3];

	void *data;
};

int hinic3_multi_host_mgmt_init(struct hinic3_hwdev *hwdev);
int hinic3_multi_host_mgmt_free(struct hinic3_hwdev *hwdev);
int hinic3_mbox_to_host_no_ack(struct hinic3_hwdev *hwdev, enum hinic3_mod_type mod, u8 cmd,
			       void *buf_in, u16 in_size, u16 channel);

struct register_slave_host {
	u8 status;
	u8 version;
	u8 rsvd[6];

	u8 host_id;
	u8 ppf_idx;
	u8 get_nic_en;
	u8 rsvd2[5];

	/* 16 * 64 bits for max 1024 functions */
	u64 funcs_nic_en[HINIC3_MAX_MGMT_FUNCTIONS_64];
	/* 16 * 64 bits for max 1024 functions */
	u64 funcs_vroce_en[HINIC3_MAX_MGMT_FUNCTIONS_64];
};

struct hinic3_slave_func_nic_state {
	u8 status;
	u8 version;
	u8 rsvd[6];

	u16 func_idx;
	u8 enable;
	u8 opened;
	u8 vroce_flag;
	u8 rsvd2[7];
};

void hinic3_set_master_host_mbox_enable(struct hinic3_hwdev *hwdev,
					bool enable);

int hinic3_sw_func_pf_mbox_handler(void *pri_handle, u16 vf_id, u16 cmd,
				   void *buf_in,
				   u16 in_size, void *buf_out, u16 *out_size);

int vf_sw_func_handler(void *hwdev, u8 cmd, void *buf_in,
		       u16 in_size, void *buf_out, u16 *out_size);
int hinic3_set_func_probe_in_host(void *hwdev, u16 func_id, bool probe);
bool hinic3_get_func_probe_in_host(void *hwdev, u16 func_id);

void *hinic3_get_ppf_hwdev_by_pdev(struct pci_dev *pdev);

int hinic3_get_func_nic_enable(void *hwdev, u16 glb_func_idx, bool *en);

#endif
