
#ifndef _PS3_IOC_MANAGER_H_
#define _PS3_IOC_MANAGER_H_

#include "ps3_instance_manager.h"
#include "ps3_inner_data.h"
#include "ps3_module_para.h"
#include "ps3_ioc_state.h"
#ifndef _WINDOWS
#define PS3_REG_SESSION_ADDR_WRITE(instance, val, reg)                     \
	do {                                                                   \
		ps3_reg_dump(instance, reg, val, PS3_FALSE);                       \
		writeq(val, reg);                                                  \
	} while(0)
#else
#define PS3_REG_SESSION_ADDR_WRITE(instance, val, reg)                     \
	do {                                                                   \
		ps3_reg_dump(instance, reg, val, PS3_FALSE);                       \
		ps3_ioc_reg_write(instance, val, reg);                             \
	} while(0)
#endif

#define PS3_MAX_CMD_COUNT_DURING_RESET_DEVICES	(100)
#define PS3_REG_READ_INTERVAL_MS	(10)
#if (defined PS3_HARDWARE_FPGA && defined PS3_MODEL_V200)
#define PS3_REG_WRITE_RETRY_NUM		(60)
#else
#define PS3_REG_WRITE_RETRY_NUM		(10)
#endif
#define PS3_REG_WRITE_INTERVAL_MS	(10)
#define PS3_REG_READ_RETRY_NUM		(5000)
#define PS3_ATU_SUPPORT_READ_RETRY_NUM (3)
#define PS3_REG_READ_SAFE_RETRY_NUM (3)
#define PS3_HOST_MEM_INFO_NUM (1)
#define PS3_IOC_STATE_HALT_SUPPORT(ins) \
	(ins->is_ioc_halt_support)

#define PS3_IOC_SHALLOW_SOFT_RECOVERY_SUPPORT(ins) \
	(ins->is_shallow_soft_recovery_support)

#define PS3_IOC_DEEP_SOFT_RECOVERY_SUPPORT(ins) \
	((ins->is_deep_soft_recovery_support) && ps3_deep_soft_reset_enable_query())

#define PS3_IOC_HARD_RECOVERY_SUPPORT(ins) \
	(PS3_INSTANCE_ABNORMAL_FORCE_HARD_RECOVERY(ins) || \
	ins->is_hard_recovery_support)

#define PS3_HALT_CLI_SUPPORT(ins) \
	(ins->is_halt_support_cli)

#define PS3_IOC_REG_READ_WITH_CHECK(instance, reg_type, reg_name, read_value) do {\
	ps3_atomic_inc(&(instance)->reg_op_count); \
	mb(); \
	if((instance)->is_hard_reset == PS3_FALSE && (instance)->is_pci_reset == PS3_FALSE) { \
		read_value = ps3_ioc_reg_read_with_check(instance, &(instance)->reg_set->reg_type.reg_name); \
	} else { \
		LOG_FILE_ERROR("hno:%u   register %p,read blocked by hardreset(%d) or pci err(%d)\n", \
			PS3_HOST(instance), &(instance)->reg_set->reg_type.reg_name, \
			(instance)->is_hard_reset, (instance)->is_pci_reset); \
		read_value = U64_MAX; \
	} \
	ps3_atomic_dec(&(instance)->reg_op_count); \
} while(0);

#define PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_type, reg_name, read_value) do {\
	ps3_atomic_inc(&(instance)->reg_op_count); \
	mb(); \
	if((instance)->is_hard_reset == PS3_FALSE && (instance)->is_pci_reset == PS3_FALSE) { \
		read_value = ps3_ioc_reg_retry_safe_read(instance, &(instance)->reg_set->reg_type.reg_name); \
	} else { \
		LOG_FILE_ERROR("hno:%u register %p,read blocked by hardreset(%d) or pci err(%d)\n", \
			PS3_HOST(instance), &(instance)->reg_set->reg_type.reg_name, \
			(instance)->is_hard_reset, (instance)->is_pci_reset); \
		read_value = U64_MAX; \
	} \
	ps3_atomic_dec(&(instance)->reg_op_count); \
} while(0);

#define PS3_IOC_REG_READ_OFFSET(instance, offset, read_value) do {\
		ps3_atomic_inc(&(instance)->reg_op_count); \
		mb(); \
		if((instance)->is_hard_reset == PS3_FALSE && (instance)->is_pci_reset == PS3_FALSE) { \
			read_value = ps3_ioc_reg_read(instance, (U8 *)(instance)->reg_set + (offset)); \
		} else { \
			LOG_FILE_ERROR("hno:%u   register %p, read blocked by hardreset(%d) or pci err(%d)\n", \
				PS3_HOST(instance), (U8 *)(instance)->reg_set + (offset), \
				(instance)->is_hard_reset, instance->is_pci_reset); \
			read_value = U64_MAX; \
		} \
		ps3_atomic_dec(&(instance)->reg_op_count); \
} while(0);

#define PS3_IOC_REG_READ_OFFSET_WITCH_CHECK(instance, offset, read_value) do {\
		ps3_atomic_inc(&(instance)->reg_op_count); \
		mb(); \
		if((instance)->is_hard_reset == PS3_FALSE && (instance)->is_pci_reset == PS3_FALSE) { \
			read_value = ps3_ioc_reg_read_with_check(instance, (U8 *)(instance)->reg_set + (offset)); \
		} else { \
			LOG_WARN("hno:%u   register %p, read blocked by hardreset(%d) or pci recovery(%d)\n", \
				PS3_HOST(instance), (U8 *)(instance)->reg_set + (offset), \
				(instance)->is_hard_reset, instance->is_pci_reset); \
			read_value = U64_MAX; \
		} \
		ps3_atomic_dec(&(instance)->reg_op_count); \
} while(0);

#define PS3_IOC_REG_WRITE(instance, reg_type, reg_name, value) do {\
	ps3_atomic_inc(&(instance)->reg_op_count); \
	mb(); \
	if((instance)->is_hard_reset == PS3_FALSE && (instance)->is_pci_reset == PS3_FALSE) { \
		ps3_ioc_reg_write(instance, value, &(instance)->reg_set->reg_type.reg_name); \
	} else { \
		LOG_FILE_ERROR("hno:%u   register %p,write blocked by hardreset(%d) or pci err(%d)\n", \
			PS3_HOST(instance), &(instance)->reg_set->reg_type.reg_name, \
			(instance)->is_hard_reset, instance->is_pci_reset); \
	} \
	ps3_atomic_dec(&(instance)->reg_op_count); \
} while(0)

#define PS3_IOC_REG_WRITE_OFFSET(instance, offset, value) do {\
	ps3_atomic_inc(&(instance)->reg_op_count); \
	mb(); \
	if((instance)->is_hard_reset == PS3_FALSE && (instance)->is_pci_reset == PS3_FALSE) { \
		ps3_ioc_reg_write(instance, value, (U8 *)(instance)->reg_set + (offset)); \
	} else { \
		LOG_FILE_ERROR("hno:%u   register %p,write blocked by hardreset(%d) or pci err(%d)\n", \
			PS3_HOST(instance), (U8 *)(instance)->reg_set + (offset), \
			(instance)->is_hard_reset, instance->is_pci_reset); \
	} \
	ps3_atomic_dec(&(instance)->reg_op_count); \
} while(0)

#define PS3_IOC_REG_WRITE_WITH_CHECK(instance, reg_type, reg_name, value) do {\
	ps3_atomic_inc(&(instance)->reg_op_count); \
	mb(); \
	if((instance)->is_hard_reset == PS3_FALSE && (instance)->is_pci_reset == PS3_FALSE) { \
		ps3_ioc_reg_write_with_check(instance, value, &(instance)->reg_set->reg_type.reg_name); \
	} else { \
		LOG_WARN("hno:%u   register %p,write blocked by hardreset(%d) or pci recovery(%d)\n", \
			PS3_HOST(instance), &(instance)->reg_set->reg_type.reg_name, \
			(instance)->is_hard_reset, instance->is_pci_reset); \
	} \
	ps3_atomic_dec(&(instance)->reg_op_count); \
} while(0)

#define PS3_IOC_REG_WRITE_OFFSET_WITH_CHECK(instance, offset, value) do {\
	ps3_atomic_inc(&(instance)->reg_op_count); \
	mb(); \
	if((instance)->is_hard_reset == PS3_FALSE && (instance)->is_pci_reset == PS3_FALSE) { \
		ps3_ioc_reg_write_with_check(instance, value, (U8 *)(instance)->reg_set + (offset)); \
	} else { \
		LOG_WARN("hno:%u   register %p,write blocked by hardreset(%d) or pci recovery(%d)\n", \
			PS3_HOST(instance), (U8 *)(instance)->reg_set + (offset), \
			(instance)->is_hard_reset, instance->is_pci_reset); \
	} \
	ps3_atomic_dec(&(instance)->reg_op_count); \
} while(0)

S32 ps3_ioc_init_cmd_context_init(struct ps3_instance *instance);
void ps3_ioc_init_cmd_context_exit(struct ps3_instance *instance);
void ps3_ioc_cmd_send(struct ps3_instance *instance,
                      struct PS3CmdWord *cmd_word);
void ps3_switch_init_cmd_send(struct ps3_instance *instance,
                              struct PS3CmdWord *cmd_word);
void ps3_switch_normal_cmd_send(struct ps3_instance *instance,
                                struct PS3CmdWord *cmd_word);
void ps3_ioc_legacy_irqs_enable(struct ps3_instance *instance);
void ps3_ioc_legacy_irqs_disable(struct ps3_instance *instance);
void ps3_ioc_msi_enable(struct ps3_instance *instance);
void ps3_ioc_msi_disable(struct ps3_instance *instance);
void ps3_ioc_msix_enable(struct ps3_instance *instance);
void ps3_ioc_msix_disable(struct ps3_instance *instance);
Bool ps3_ioc_is_legacy_irq_existed(struct ps3_instance *instance);
S32 ps3_drv_info_buf_alloc(struct ps3_instance *instance);

void ps3_drv_info_buf_free(struct ps3_instance *instance);

S32 ps3_host_mem_info_buf_alloc(struct ps3_instance *instance);

void ps3_host_mem_info_buf_free(struct ps3_instance *instance);

S32 ps3_ioc_init_to_ready(struct ps3_instance *instance);

S32 ps3_ioc_hard_reset_to_ready(struct ps3_instance *instance);

S32 ps3_ioc_init_proc(struct ps3_instance *instance);

void ps3_ioc_reg_write(struct ps3_instance *instance, U64 val,
                       void __iomem *reg);
void ps3_ioc_hardreset_reg_write(struct ps3_instance *instance,
	U64 val, void __iomem *reg, Bool is_warn_prk);
U64 ps3_ioc_hardreset_reg_read(struct ps3_instance *instance,
 void __iomem *reg);
U64 ps3_ioc_reg_read(struct ps3_instance *instance, void __iomem *reg);

U64 ps3_ioc_reg_read_with_check(struct ps3_instance *instance,
	void __iomem *reg);

static inline Bool ps3_ioc_state_valid_check(U32 fw_cur_state) {
	return ((fw_cur_state > PS3_FW_STATE_START) && (fw_cur_state != PS3_FW_STATE_MASK));
}

U64 ps3_ioc_reg_retry_safe_read(struct ps3_instance *instance, void __iomem *reg);

static inline void ps3_ioc_reg_write_with_check(struct ps3_instance *instance,
	U64 val, void __iomem *reg)
{
	U8 try_count = 0;

	ps3_reg_dump(instance, reg, val, PS3_FALSE);
	if (instance->ioc_adpter->reg_write) {
		while (try_count != PS3_REG_WRITE_RETRY_NUM) {
			instance->ioc_adpter->reg_write(instance, val, reg);
			if (instance->ioc_adpter->reg_read(instance,  reg) == val) {
				break;
			}
			try_count++;
			ps3_msleep(PS3_REG_WRITE_INTERVAL_MS);
		}
	} else {
		LOG_ERROR("hno:%u  no register write\n", PS3_HOST(instance));
	}

	if (try_count == PS3_REG_WRITE_RETRY_NUM) {
		ps3_instance_state_transfer_to_dead(instance);
	}
}

U64 ps3_switch_ioc_reg_read(struct ps3_instance *instance, void __iomem *reg);

static inline void
ps3_ioc_mgr_req_queue_lock_init(struct ps3_instance *instance)
{
    ps3_spin_lock_init(&instance->req_queue_lock);
}

static inline Bool ps3_ioc_mgr_max_fw_cmd_get(struct ps3_instance *instance,
	U32 *max_cmd_count)
{
	Bool ret = PS3_TRUE;
	HilReg0Ps3RegisterFPs3MaxFwCmd_u *reg_max_fw_cmd = NULL;
	U64 value = 0;

	PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3MaxFwCmd, value);
	INJECT_START(PS3_ERR_IJ_GET_REPLYQ_COUNT_INVALID, &value)
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3MaxFwCmd NOK!\n",
			PS3_HOST(instance));
		*max_cmd_count = 0;
		ret = PS3_FALSE;
		goto l_out;
	}

	reg_max_fw_cmd = (HilReg0Ps3RegisterFPs3MaxFwCmd_u *)&value;
	*max_cmd_count = (U32)reg_max_fw_cmd->reg.ps3MaxFwCmd;

	if(instance->is_adjust_register_count) {
		*max_cmd_count += 1;
	}
	if (reset_devices && *max_cmd_count > PS3_MAX_CMD_COUNT_DURING_RESET_DEVICES) {
		*max_cmd_count = PS3_MAX_CMD_COUNT_DURING_RESET_DEVICES;
	}
l_out:
	return ret;

}

static inline U32
ps3_ioc_mgr_max_msix_vectors_get(struct ps3_instance *instance)
{
	U64 reg_max_msix_vectors = PS3_MAX_REPLY_QUE_COUNT;
	(void)instance;
	return (U32)(reg_max_msix_vectors & PS3_FW_MAX_MSIX_VECTORS_MASK);
}

static inline Bool ps3_ioc_mgr_max_chain_size_get(
	struct ps3_instance *instance, U32 *max_chain_size)
{
	Bool ret = PS3_TRUE;
	HilReg0Ps3RegisterFPs3MaxChainSize_u *max_chain_size_u = NULL;
	U64 value = 0;

	PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3MaxChainSize, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3MaxChainSize NOK!\n",
			PS3_HOST(instance));
		*max_chain_size = 0;
		ret = PS3_FALSE;
		goto l_out;
	}

	max_chain_size_u = (HilReg0Ps3RegisterFPs3MaxChainSize_u *)&value;
	*max_chain_size = (U32)max_chain_size_u->reg.ps3MaxChainSize;
l_out:
	return ret;
}

static inline Bool ps3_ioc_mgr_max_vd_info_size_get(
	struct ps3_instance *instance, U32 *vd_info_size)
{
	Bool ret = PS3_TRUE;
	HilReg0Ps3RegisterFPs3MaxVdInfoSize_u *vd_info_size_u = NULL;
	U64 value = 0;

	PS3_IOC_REG_READ_WITH_CHECK(instance, reg_f.Excl_reg, ps3MaxVdInfoSize, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3MaxVdInfoSize NOK!\n",
			PS3_HOST(instance));
		*vd_info_size = 0;
		ret = PS3_FALSE;
		goto l_out;
	}
	vd_info_size_u = (HilReg0Ps3RegisterFPs3MaxVdInfoSize_u *)&value;

	*vd_info_size = (U32)vd_info_size_u->reg.ps3MaxVdInfoSize;
l_out:
	return ret;
}

static inline Bool ps3_ioc_mgr_max_nvme_page_size_get(
	struct ps3_instance *instance, U32 *max_nvme_page_size)
{
	Bool ret = PS3_TRUE;
	HilReg0Ps3RegisterFPs3MaxNvmePageSize_u *max_nvme_page_size_u = NULL;
	U64 value = 0;

	PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3MaxNvmePageSize, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3MaxNvmePageSize NOK!\n",
			PS3_HOST(instance));
		*max_nvme_page_size = 0;
		ret = PS3_FALSE;
		goto l_out;
	}

	max_nvme_page_size_u = (HilReg0Ps3RegisterFPs3MaxNvmePageSize_u *)&value;
	*max_nvme_page_size = (U32)max_nvme_page_size_u->reg.ps3MaxNvmePageSize;
l_out:
	return ret;
}

static inline Bool ps3_ioc_mgr_is_dma64_support(struct ps3_instance *instance,
	Bool *is_dma64_support)
{
	Bool ret = PS3_TRUE;
	HilReg0Ps3RegisterFPs3FeatureSupport_u *ps3_feature_support = NULL;
	U64 value = 0;

	PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3FeatureSupport, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3FeatureSupport NOK!\n",
			PS3_HOST(instance));
		*is_dma64_support = PS3_FALSE;
		ret = PS3_FALSE;
		goto l_out;
	}

	ps3_feature_support = (HilReg0Ps3RegisterFPs3FeatureSupport_u *)&value;
	*is_dma64_support = (ps3_feature_support->reg.dmaBit64Support != 0);
l_out:
	return ret;
}

static inline Bool ps3_max_replyq_count_get(struct ps3_instance *instance,
	U32 *max_replyq_count)
{
	Bool ret = PS3_TRUE;
	U64 value = 0;

	PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3MaxReplyque, value);
	INJECT_START(PS3_ERR_IJ_GET_REPLYQ_COUNT_INVALID, &value)
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3MaxReplyque NOK!\n",
			PS3_HOST(instance));
		*max_replyq_count = 0;
		ret = PS3_FALSE;
		goto l_out;
	}

	*max_replyq_count = value & 0xffff;
l_out:
	return ret;
}

static inline Bool ps3_ioc_fw_version_get(struct ps3_instance *instance)
{
	Bool ret = PS3_TRUE;
	HilReg0Ps3RegisterFPs3FirmwareVersion_u *pver = NULL;
	U64 fw_version_last = instance->ioc_fw_version;
	U64 ver = 0;

	PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3FirmwareVersion, ver);
	if (ver == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3FirmwareVersion NOK!\n",
			PS3_HOST(instance));
		instance->ioc_fw_version = fw_version_last;
		ret = PS3_FALSE;
		goto l_out;
	}

	pver = (HilReg0Ps3RegisterFPs3FirmwareVersion_u *)&ver;
	instance->ioc_fw_version = (U64)pver->reg.ps3FmVer;
l_out:
	return ret;
}

static inline Bool ps3_ioc_sgl_mode_support(struct ps3_instance *instance,
	Bool *is_sgl_mode_support)
{
	Bool ret = PS3_TRUE;
	HilReg0Ps3RegisterFPs3FeatureSupport_u *ps3_feature_support = NULL;
	U64 value = 0;

	PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3FeatureSupport, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3FeatureSupport NOK!\n",
			PS3_HOST(instance));
		*is_sgl_mode_support = PS3_FALSE;
		ret = PS3_FALSE;
		goto l_out;
	}

	ps3_feature_support = (HilReg0Ps3RegisterFPs3FeatureSupport_u *)&value;
	*is_sgl_mode_support = (ps3_feature_support->reg.sglModeSupport == 1);
l_out:
	return ret;
}

static inline Bool ps3_ioc_dump_support_get(struct ps3_instance *instance)
{
	Bool ret = PS3_TRUE;
	Bool last_dump_support = instance->dump_context.is_dump_support;
	HilReg0Ps3RegisterFPs3FeatureSupport_u *ps3_feature_support = NULL;
	U64 value = 0;


	PS3_IOC_REG_READ_WITH_CHECK(instance, reg_f.Excl_reg, ps3FeatureSupport, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3FeatureSupport NOK!\n",
			PS3_HOST(instance));
		instance->dump_context.is_dump_support = last_dump_support;
		ret = PS3_FALSE;
		goto l_out;
	}

	ps3_feature_support = (HilReg0Ps3RegisterFPs3FeatureSupport_u *)&value;
	instance->dump_context.is_dump_support =
		(ps3_feature_support->reg.dumpCrashSupport == 1);
l_out:
	return ret;
}

static inline Bool ps3_ioc_state_halt_support_get(struct ps3_instance *instance)
{
	Bool ret = PS3_TRUE;
	Bool last_halt_support = instance->is_ioc_halt_support;
	HilReg0Ps3RegisterFPs3FeatureSupport_u *ps3_feature_support = NULL;
	U64 value = 0;


	PS3_IOC_REG_READ_WITH_CHECK(instance, reg_f.Excl_reg, ps3FeatureSupport, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3FeatureSupport NOK!\n",
			PS3_HOST(instance));
		instance->is_ioc_halt_support = last_halt_support;
		ret = PS3_FALSE;
		goto l_out;
	}

	ps3_feature_support = (HilReg0Ps3RegisterFPs3FeatureSupport_u *)&value;
	instance->is_ioc_halt_support = (ps3_feature_support->reg.fwHaltSupport == 1);
l_out:
	return ret;
}

static inline Bool ps3_ioc_recovery_count_get(struct ps3_instance *instance,
	U32 *recovery_count)
{
#if 0
	Bool ret = PS3_TRUE;
	U64 value = 0;

	PS3_IOC_REG_READ_WITH_CHECK(instance, reg_f.Excl_reg, ps3SoftresetCounter, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3Debug4 failed!\n",
			PS3_HOST(instance));
		*recovery_count = 0;
		ret = PS3_FALSE;
		goto l_out;
	}

	*recovery_count = (U32)(value & PS3_IOC_RECOVERY_COUNT_MASK);
l_out:
	return ret;
#else
	(void) instance;
	*recovery_count = 0;
	return PS3_TRUE;
#endif
}

static inline U32 ps3_ioc_state_get(struct ps3_instance *instance)
{
	U64 ioc_state = 0;

	PS3_IOC_REG_READ_WITH_CHECK(instance, reg_f.Excl_reg, ps3SocFwState, ioc_state);
	return (U32)(ioc_state & PS3_FW_STATE_MASK);
}

static inline Bool ps3_ioc_state_get_with_check(struct ps3_instance *instance,
	U32 *ioc_state)
{
	Bool ret = PS3_TRUE;
	U64 value = 0;

	PS3_IOC_REG_READ_WITH_CHECK(instance, reg_f.Excl_reg, ps3SocFwState, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3SocFwState NOK!\n",
			PS3_HOST(instance));
		*ioc_state = 0;
		ret = PS3_FALSE;
		goto l_out;
	}

	*ioc_state = (U32)value & PS3_FW_STATE_MASK;
l_out:
	return ret;
}

static inline Bool ps3_get_doorbell_done_with_check(struct ps3_instance *instance,
	Bool *is_doorbell_done)
{
	Bool ret = PS3_TRUE;
	U64 value = 0;

	PS3_IOC_REG_READ_WITH_CHECK(instance, reg_f.Excl_reg, ps3RegCmdState, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3RegCmdState NOK!\n",
			PS3_HOST(instance));
		*is_doorbell_done = PS3_FALSE;
		ret = PS3_FALSE;
		goto l_out;
	}

	*is_doorbell_done = value & PS3_DOORBELL_DONE;

	if (*is_doorbell_done) {
		value &= ~PS3_DOORBELL_DONE;
		if (instance->peer_instance != NULL &&
			instance->peer_instance->reg_set != NULL) {
			PS3_IOC_REG_WRITE_WITH_CHECK(instance, reg_f.Excl_reg, ps3RegCmdState, value);
			PS3_IOC_REG_WRITE_WITH_CHECK(instance->peer_instance,
				reg_f.Excl_reg, ps3RegCmdState, value);
		} else {
			PS3_IOC_REG_WRITE_WITH_CHECK(instance, reg_f.Excl_reg, ps3RegCmdState, value);
		}

	}
l_out:
	return ret;
}

static inline Bool ps3_get_max_r1x_cmds_with_check(struct ps3_instance *instance,
	U16 *max_r1x_cmds)
{
	Bool ret = PS3_TRUE;
	HilReg0Ps3RegisterFPs3MaxSecR1xCmds_u *ps3_max_sec_r1x_cmds = NULL;
	U64 value = 0;

	PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3MaxSecR1xCmds, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3Debug6 NOK!\n",
			PS3_HOST(instance));
		*max_r1x_cmds = 0;
		ret = PS3_FALSE;
		goto l_out;
	}

	ps3_max_sec_r1x_cmds = (HilReg0Ps3RegisterFPs3MaxSecR1xCmds_u *)&value;
	*max_r1x_cmds = ps3_max_sec_r1x_cmds->reg.ps3MaxSecR1xCmds;
l_out:
	return ret;
}

static inline Bool ps3_check_debug0_valid_with_check(
	struct ps3_instance *instance, Bool *is_doorbell_valid, U32 check_mask)
{
	Bool ret = PS3_TRUE;
	U64 value = 0;

	PS3_IOC_REG_READ_WITH_CHECK(instance, reg_f.Excl_reg, ps3CmdTrigger, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3CmdTrigger NOK!\n",
			PS3_HOST(instance));
		*is_doorbell_valid = PS3_FALSE;
		ret = PS3_FALSE;
		goto l_out;
	}

	*is_doorbell_valid = (value & check_mask);
l_out:
	return ret;
}

static inline Bool ps3_ioc_heartbeat_get(struct ps3_instance *instance,
	U64 *heartbeat_value)
{
	Bool ret = PS3_TRUE;
	static Bool last_fault_state = PS3_TRUE;

	*heartbeat_value = ps3_ioc_reg_read_with_check(instance,
		&instance->reg_set->reg_f.Excl_reg.ps3SocFwState);
	INJECT_START(PS3_ERR_IJ_READ_FW_STATE_REG_FF, heartbeat_value)
	if (*heartbeat_value == U64_MAX) {
		if (last_fault_state) {
			LOG_ERROR("hno:%u  read reg ps3SocFwState NOK!\n",
				PS3_HOST(instance));
			last_fault_state = PS3_FALSE;
		}
		*heartbeat_value = 0;
		ret = PS3_FALSE;
		goto l_out;
	} else {
		last_fault_state = PS3_TRUE;
		LOG_DEBUG("hno:%u  read reg ps3SocFwState success value[%llu]!\n",
			PS3_HOST(instance), *heartbeat_value);
	}

l_out:
	return ret;
}

static inline Bool ps3_ioc_recovery_support_get(struct ps3_instance *instance)
{
	Bool ret = PS3_TRUE;
	Bool last_shallow_soft_recovery_support =
		instance->is_shallow_soft_recovery_support;
	Bool last_deep_soft_recovery_support =
		instance->is_deep_soft_recovery_support;
	Bool last_hard_recovery_support =
		instance->is_hard_recovery_support;
	HilReg0Ps3RegisterFPs3FeatureSupport_u *ps3_feature_support = NULL;
	U64 value = 0;


	PS3_IOC_REG_READ_WITH_CHECK(instance, reg_f.Excl_reg, ps3FeatureSupport, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3FeatureSupport NOK!\n",
			PS3_HOST(instance));
		instance->is_shallow_soft_recovery_support =
			last_shallow_soft_recovery_support;
		instance->is_deep_soft_recovery_support =
			last_deep_soft_recovery_support;
		instance->is_hard_recovery_support =
			last_hard_recovery_support;
		ret = PS3_FALSE;
		goto l_out;
	}

	ps3_feature_support = (HilReg0Ps3RegisterFPs3FeatureSupport_u *)&value;
	instance->is_shallow_soft_recovery_support =
		(ps3_feature_support->reg.shallowSoftRecoverySupport == 1);
	instance->is_deep_soft_recovery_support =
		(ps3_feature_support->reg.deepSoftRecoverySupport == 1);
	instance->is_hard_recovery_support =
		(ps3_feature_support->reg.hardRecoverySupport == 1);
l_out:
	return ret;
}

static inline Bool ps3_ioc_multi_func_support(struct ps3_instance *instance)
{
	Bool ret = PS3_TRUE;
	HilReg0Ps3RegisterFPs3FeatureSupport_u *ps3_feature_support = NULL;
	U64 value = 0;

	PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3FeatureSupport, value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u  read reg ps3FeatureSupport failed!\n",
			PS3_HOST(instance));
		ret = PS3_FALSE;
		goto l_out;
	}

	ps3_feature_support = (HilReg0Ps3RegisterFPs3FeatureSupport_u *)&value;
	ret = (ps3_feature_support->reg.multiDevfnSupport == 1);
l_out:
	return ret;
}

static inline Bool ps3_ioc_security_state_check(struct ps3_instance *instance)
{
    U64 value = 0;
    Bool ret = PS3_FALSE;

    PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3Debug7, value);
    if (value == U64_MAX || (value & 0x1)) {
        ret = PS3_TRUE;
        LOG_ERROR("hno:%u read register NOK or ioc is security [%llu]\n",
            PS3_HOST(instance), value);
    }

#ifdef PS3_HARDWARE_SIM
    ret = PS3_FALSE;
#endif

    return ret;
}

static inline Bool ps3_ioc_atu_support_get(struct ps3_instance *instance, U8 *bit_pos)
{
	Bool ret = PS3_TRUE;
	HilRegPs3RegisterFPs3AtuSupport_u *ps3_atu_support = NULL;
	U64 value = PS3_BIT_POS_DEFAULT;

	PS3_IOC_REG_READ_WITH_CHECK(instance, reg_f.Excl_reg, ps3Debug8, value);
	INJECT_START(PS3_ERR_IJ_READ_ATU_REG_FAILED, &value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u read reg ps3AtuSupport NOK!\n",
			PS3_HOST(instance));
		*bit_pos = PS3_BIT_POS_DEFAULT;
		ret = PS3_FALSE;
		goto l_out;
	}
	ps3_atu_support = (HilRegPs3RegisterFPs3AtuSupport_u *)&value;
	*bit_pos = ps3_atu_support->reg.bitPos;
l_out:
	return ret;
}

static inline Bool ps3_ioc_atu_support_safe_get(struct ps3_instance *instance, U8 *bit_pos)
{
	Bool ret = PS3_FALSE;
	U32 fw_cur_state = PS3_FW_STATE_UNDEFINED;
	U32 count = 0;
	U32 retry_cnt = 0;
	U8 tmp_bit_pos = 0;
	Bool is_first = PS3_TRUE;
	for (; retry_cnt < PS3_ATU_SUPPORT_READ_RETRY_NUM; retry_cnt++) {
		if (!ps3_ioc_atu_support_get(instance, &tmp_bit_pos)) {
			goto l_out;
		}
		fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
		INJECT_START(PS3_ERR_IJ_IOC_TO_READY_FAILED, &fw_cur_state);
		if (!ps3_ioc_state_valid_check(fw_cur_state)) {
			for (; count < PS3_REG_READ_RETRY_NUM; count++) {
				fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
				INJECT_START(PS3_ERR_IJ_IOC_TO_READY_FAILED, &fw_cur_state);
				if (ps3_ioc_state_valid_check(fw_cur_state)) {
					if (!ps3_ioc_atu_support_get(instance, &tmp_bit_pos)) {
						goto l_out;
					}
					*bit_pos = tmp_bit_pos;
					ret = PS3_TRUE;
					goto l_out;
				}
				ps3_msleep(PS3_LOOP_TIME_INTERVAL_20MS);
			}
			goto l_out;
		}
		if (is_first) {
			*bit_pos = tmp_bit_pos;
			is_first = PS3_FALSE;
			continue;
		}
		INJECT_START(PS3_ERR_IJ_BIT_POS_ERR, &tmp_bit_pos);
		if (*bit_pos != tmp_bit_pos) {
			goto l_out;
		}
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_50MS);
	}
	ret = PS3_TRUE;
l_out:
	return ret;
}

static inline Bool ps3_ioc_atu_support_retry_read(struct ps3_instance *instance, U8 *bit_pos)
{
	U32 retry_cnt = PS3_ATU_SUPPORT_READ_RETRY_NUM;
	Bool ret = PS3_FALSE;

	while (retry_cnt--) {
		if (ps3_ioc_atu_support_safe_get(instance, bit_pos)) {
			ret = PS3_TRUE;
			break;
		}
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_20MS);
	}
	return ret;
}

static inline void ps3_ioc_can_hardreset_set(struct ps3_instance *instance, U8 enable)
{
	U64 can_hardreset = 0;
	HilRegPs3RegisterFPs3CanHardReset_u *ps3_can_hardreset = NULL;

	ps3_can_hardreset = (HilRegPs3RegisterFPs3CanHardReset_u*)&can_hardreset;
	ps3_can_hardreset->reg.canHardReset = enable;
	PS3_IOC_REG_WRITE(instance, reg_f.Excl_reg, ps3Debug9, ps3_can_hardreset->val);
}

Bool ps3_feature_support_reg_get(struct ps3_instance *instance);

void ps3_ioc_scsi_cmd_send(struct ps3_instance *instance,
	struct PS3CmdWord *cmd_word);

struct pglist_data *first_online_pgdat(void);

struct pglist_data *next_online_pgdat(struct pglist_data *pgdat);

#endif
