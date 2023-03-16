// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023 Hisilicon Limited.

#include "hclge_main.h"
#include "hnae3.h"
#include "hnae3_ext.h"
#include "hclge_cmd.h"
#include "hclge_ext.h"

static nic_event_fn_t nic_event_call;

/* We use a lock to ensure that the address of the nic_event_call function
 * is valid when it is called. Avoid null pointer exceptions caused by
 * external unregister during invoking.
 */
static DEFINE_MUTEX(hclge_nic_event_lock);

static int hclge_set_pfc_storm_para(struct hclge_dev *hdev, void *data,
				    size_t length)
{
	struct hclge_pfc_storm_para_cmd *para_cmd;
	struct hnae3_pfc_storm_para *para;
	struct hclge_desc desc;
	int ret;

	if (length != sizeof(struct hnae3_pfc_storm_para))
		return -EINVAL;

	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_CFG_PAUSE_STORM_PARA,
				   false);
	para = (struct hnae3_pfc_storm_para *)data;
	para_cmd = (struct hclge_pfc_storm_para_cmd *)desc.data;
	para_cmd->dir = cpu_to_le32(para->dir);
	para_cmd->enable = cpu_to_le32(para->enable);
	para_cmd->period_ms = cpu_to_le32(para->period_ms);
	para_cmd->times = cpu_to_le32(para->times);
	para_cmd->recovery_period_ms = cpu_to_le32(para->recovery_period_ms);

	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret)
		dev_err(&hdev->pdev->dev,
			"failed to set pfc storm para, ret = %d\n", ret);
	return ret;
}

static int hclge_get_pfc_storm_para(struct hclge_dev *hdev, void *data,
				    size_t length)
{
	struct hclge_pfc_storm_para_cmd *para_cmd;
	struct hnae3_pfc_storm_para *para;
	struct hclge_desc desc;
	int ret;

	if (length != sizeof(struct hnae3_pfc_storm_para))
		return -EINVAL;

	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_CFG_PAUSE_STORM_PARA, true);
	para = (struct hnae3_pfc_storm_para *)data;
	para_cmd = (struct hclge_pfc_storm_para_cmd *)desc.data;
	para_cmd->dir = cpu_to_le32(para->dir);
	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get pfc storm para, ret = %d\n", ret);
		return ret;
	}

	para->enable = le32_to_cpu(para_cmd->enable);
	para->period_ms = le32_to_cpu(para_cmd->period_ms);
	para->times = le32_to_cpu(para_cmd->times);
	para->recovery_period_ms = le32_to_cpu(para_cmd->recovery_period_ms);

	return 0;
}

static int hclge_set_reset_task(struct hclge_dev *hdev, void *data,
				size_t length)
{
	u32 *reset_level = (u32 *)data;

	if (length != sizeof(u32))
		return -EINVAL;

	dev_warn(&hdev->pdev->dev, "reset level is %u\n", *reset_level);

	/* request reset & schedule reset task */
	set_bit(*reset_level, &hdev->reset_request);
	hclge_reset_task_schedule(hdev);
	return 0;
}

int hclge_ext_call_event(struct hclge_dev *hdev,
			 enum hnae3_event_type_custom event_t)
{
	if (event_t >= HNAE3_INVALID_EVENT_CUSTOM)
		return -EINVAL;

	mutex_lock(&hclge_nic_event_lock);
	if (!nic_event_call) {
		mutex_unlock(&hclge_nic_event_lock);
		return -EOPNOTSUPP;
	}

	nic_event_call(hdev->vport[0].nic.netdev, event_t);
	mutex_unlock(&hclge_nic_event_lock);
	return 0;
}

int nic_register_event(nic_event_fn_t event_call)
{
	if (!event_call) {
		pr_err("hns3: register event handle is null\n");
		return -EINVAL;
	}

	mutex_lock(&hclge_nic_event_lock);
	if (nic_event_call) {
		mutex_unlock(&hclge_nic_event_lock);
		pr_err("hns3: event already register\n");
		return -EBUSY;
	}

	nic_event_call = event_call;

	mutex_unlock(&hclge_nic_event_lock);
	pr_info("hns3: event register success\n");
	return 0;
}
EXPORT_SYMBOL(nic_register_event);

int nic_unregister_event(void)
{
	mutex_lock(&hclge_nic_event_lock);
	nic_event_call = NULL;

	mutex_unlock(&hclge_nic_event_lock);
	pr_info("hns3: event unregister success\n");
	return 0;
}
EXPORT_SYMBOL(nic_unregister_event);

static int hclge_nic_call_event(struct hclge_dev *hdev, void *data,
				size_t length)
{
#define ERROR_EVENT_TYPE_NUM 4

	u32 event_type[ERROR_EVENT_TYPE_NUM] = {
		HNAE3_PPU_POISON_CUSTOM,
		HNAE3_IMP_RESET_CUSTOM,
		HNAE3_IMP_RD_POISON_CUSTOM,
		HNAE3_ROCEE_AXI_RESP_CUSTOM,
	};
	u32 *index = (u32 *)data;

	if (length != sizeof(u32))
		return -EINVAL;

	if ((*index) >= ERROR_EVENT_TYPE_NUM)
		return 0;

	return hclge_ext_call_event(hdev, event_type[*index]);
}

static enum hnae3_event_type_custom
hclge_get_reset_fail_type(enum hnae3_reset_type reset_type)
{
	const struct hclge_reset_fail_type_map fail_type_map[] = {
		{HNAE3_FUNC_RESET, HNAE3_FUNC_RESET_FAIL_CUSTOM},
		{HNAE3_GLOBAL_RESET, HNAE3_GLOBAL_RESET_FAIL_CUSTOM},
		{HNAE3_IMP_RESET, HNAE3_IMP_RESET_FAIL_CUSTOM},
	};
	u32 i;

	for (i = 0; i < ARRAY_SIZE(fail_type_map); i++)
		if (fail_type_map[i].reset_type == reset_type)
			return fail_type_map[i].custom_type;

	return HNAE3_INVALID_EVENT_CUSTOM;
}

static void hclge_report_reset_fail_custom(struct hclge_dev *hdev)
{
#define HCLGE_RESET_MAX_FAIL_CNT_CUSTOM 1

	u32 max_fail_custom_cnt = HCLGE_RESET_MAX_FAIL_CNT;

	mutex_lock(&hclge_nic_event_lock);
	if (nic_event_call)
		max_fail_custom_cnt = HCLGE_RESET_MAX_FAIL_CNT_CUSTOM;
	mutex_unlock(&hclge_nic_event_lock);

	if (hdev->rst_stats.reset_fail_cnt < max_fail_custom_cnt)
		return;

	dev_err(&hdev->pdev->dev, "failed to report reset!\n");
	hclge_ext_call_event(hdev, hclge_get_reset_fail_type(hdev->reset_type));
}

void hclge_ext_reset_end(struct hclge_dev *hdev, bool done)
{
	if (!done) {
		hclge_report_reset_fail_custom(hdev);
		return;
	}

	hclge_ext_call_event(hdev, HNAE3_RESET_DONE_CUSTOM);
	dev_info(&hdev->pdev->dev, "report reset done!\n");
}

static const hclge_priv_ops_fn hclge_ext_func_arr[] = {
	[HNAE3_EXT_OPC_RESET] = hclge_set_reset_task,
	[HNAE3_EXT_OPC_EVENT_CALLBACK] = hclge_nic_call_event,
	[HNAE3_EXT_OPC_GET_PFC_STORM_PARA] = hclge_get_pfc_storm_para,
	[HNAE3_EXT_OPC_SET_PFC_STORM_PARA] = hclge_set_pfc_storm_para,
};

int hclge_ext_ops_handle(struct hnae3_handle *handle, int opcode,
			 void *data, size_t length)
{
	struct hclge_vport *vport = hclge_get_vport(handle);
	int cmd_num = ARRAY_SIZE(hclge_ext_func_arr);
	struct hclge_dev *hdev = vport->back;
	hclge_priv_ops_fn ext_opcode_func;

	if (opcode >= cmd_num) {
		dev_err(&hdev->pdev->dev, "invalid opcode %d\n", opcode);
		return -EINVAL;
	}

	ext_opcode_func = hclge_ext_func_arr[opcode];
	if (!ext_opcode_func) {
		dev_err(&hdev->pdev->dev, "unsupported opcode %d\n", opcode);
		return -EOPNOTSUPP;
	}

	return ext_opcode_func(hdev, data, length);
}
