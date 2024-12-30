
#ifndef _PS3_CMD_STATICTIS_H_
#define _PS3_CMD_STATICTIS_H_
#ifndef _WINDOWS
#include <linux/mutex.h>
#include <linux/limits.h>
#include <scsi/scsi_cmnd.h>
#endif

#include "ps3_htp_def.h"
#include "ps3_scsih_cmd_parse.h"
#include "ps3_instance_manager.h"
#include "ps3_cmd_channel.h"
#include "ps3_platform_utils.h"
#include "ps3_scsih.h"

#ifdef __cplusplus
extern "C"{
#endif

#define PS3_CMD_STAT_INIT_VALUE (0)

#define PS3_CMD_WORD_INDEX_SHIFT (0x000000FF)

void ps3_dev_io_statis_init(struct ps3_dev_io_statis *statis);

void ps3_io_statis_inc(struct scsi_device *sdev, enum ps3_dev_io_stat_type type);

void ps3_io_statis_dec(struct scsi_device *sdev,
	enum ps3_dev_io_stat_type type);

void ps3_io_statis_clear(struct scsi_device *sdev);

S32 ps3_cmd_statistics_init(struct ps3_instance *instance);


void ps3_cmd_statistics_exit(struct ps3_instance *instance);

void ps3_dev_io_start_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd);

void ps3_dev_io_start_err_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd);

void ps3_dev_io_start_ok_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd);

void ps3_dev_io_back_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd, U8 status);

static inline void ps3_dev_scsi_err_stat_inc(struct ps3_cmd *cmd)
{
	if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)){
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_R_RECV_ERR);
	} else if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag)){
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_W_RECV_ERR);
	}
}

static inline Bool ps3_is_vd_rw_cmd(const struct scsi_cmnd *s_cmd)
{
	return (PS3_SDEV_PRI_DATA(s_cmd->device)->dev_type == PS3_DEV_TYPE_VD) &&
		ps3_scsih_cdb_is_rw_cmd(s_cmd->cmnd);

}

static inline Bool ps3_is_read_cmd(const struct scsi_cmnd *s_cmd)
{
	return ps3_scsih_cdb_is_read_cmd(s_cmd->cmnd);
}
static inline Bool ps3_is_write_cmd(const struct scsi_cmnd *s_cmd)
{
	return ps3_scsih_cdb_is_write_cmd(s_cmd->cmnd);
}

void ps3_io_outstand_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd);

void ps3_io_outstand_dec(struct ps3_instance *instance,
	const struct scsi_cmnd *s_cmd);

void ps3_vd_outstand_dec(struct ps3_instance *instance, const struct scsi_cmnd *s_cmd);

void ps3_vd_outstand_inc(struct ps3_instance *instance, const struct scsi_cmnd *s_cmd);

void ps3_dev_io_outstand_dec(const struct ps3_cmd *cmd);

void ps3_dev_io_qos_inc(struct ps3_scsi_priv_data* priv_data);

void ps3_dev_io_qos_dec(struct ps3_scsi_priv_data* priv_data);

void ps3_dev_io_outstand_inc(const struct ps3_cmd *cmd);

void ps3_qos_cmd_inc(struct ps3_instance *instance);

void ps3_qos_stat_inc(struct ps3_instance *instance,
	struct ps3_cmd *cmd, U16 stat, U16 type);

static inline U8 ps3_cmd_word_index_get(struct ps3_instance *instance)
{
	U64 cmd_count = (U64)ps3_atomic64_inc_return(&instance->cmd_statistics.cmd_word_send_count);
	return (U8)(cmd_count & PS3_CMD_WORD_INDEX_SHIFT);
}

static inline void ps3_cmd_word_stat_inc(struct ps3_instance *instance,
	struct PS3CmdWord *cmd_word)
{
	(void)cmd_word;
	(void)ps3_cmd_word_index_get(instance);
	return;
}

#ifndef _WINDOWS
void ps3_scmd_inc(struct ps3_instance *instance,
	struct scsi_cmnd *s_cmd, U8 type, U8 status);
#else
void ps3_scmd_inc(struct ps3_instance *instance,
	const struct scsi_cmnd *s_cmd, U32 tag, U8 type, U8 status);
#endif
void ps3_scmd_drv2ioc_start_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd);

void ps3_scmd_drv2ioc_back_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd, U8 status);


static inline U8 ps3_resp_stauts_to_stat_status(U8 reply_flags)
{
	return (reply_flags == PS3_REPLY_WORD_FLAG_SUCCESS) ?
		(U8)PS3_STAT_BACK_OK : (U8)PS3_STAT_BACK_FAIL;
}

void ps3_mgr_start_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd);

void ps3_mgr_back_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd, U8 status);

void ps3_stat_all_clear(struct ps3_instance *instance);

static inline Bool ps3_stat_inc_switch_is_open(const struct ps3_instance *instance)
{
	return (instance->cmd_statistics.cmd_stat_switch & PS3_STAT_INC_SWITCH_OPEN);
}

static inline Bool ps3_stat_log_switch_is_open(const struct ps3_instance *instance)
{
	return (instance->cmd_statistics.cmd_stat_switch & PS3_STAT_LOG_SWITCH_OPEN);
}

static inline Bool ps3_stat_outstand_switch_is_open(const struct ps3_instance *instance)
{
	return (instance->cmd_statistics.cmd_stat_switch & PS3_STAT_OUTSTAND_SWITCH_OPEN);
}

static inline Bool ps3_stat_dev_switch_is_open(const struct ps3_instance *instance)
{
	return (instance->cmd_statistics.cmd_stat_switch & PS3_STAT_DEV_SWITCH_OPEN);
}

#ifndef _WINDOWS
static inline void ps3_sdev_busy_inc(struct scsi_cmnd *scmd)
{
#if defined DRIVER_SUPPORT_PRIV_BUSY
	struct ps3_scsi_priv_data *device_priv_data = (struct ps3_scsi_priv_data*)scmd->device->hostdata;
	if (device_priv_data == NULL) {
		return;
	}

	atomic_inc(&device_priv_data->sdev_priv_busy);
#else
#ifdef PS3_UT
	atomic_inc(&scmd->device->device_busy);
#else
	(void) scmd;
#endif
#endif

	return;
}

static inline void ps3_sdev_busy_dec(struct scsi_cmnd *scmd)
{
#if defined DRIVER_SUPPORT_PRIV_BUSY
	struct ps3_scsi_priv_data *device_priv_data = (struct ps3_scsi_priv_data*)scmd->device->hostdata;
	if (device_priv_data == NULL) {
		return;
	}

	atomic_dec(&device_priv_data->sdev_priv_busy);
#else
#ifdef PS3_UT
	atomic_dec(&scmd->device->device_busy);
#else
	(void) scmd;
#endif
#endif
	return;
}

#define PS3_DEV_BUSY_DEC(cmd) do { \
	ps3_sdev_busy_dec((cmd)); \
} while (0)

#define PS3_DEV_BUSY_INC(cmd) do { \
	ps3_sdev_busy_inc((cmd)); \
} while (0)
#else
#define PS3_DEV_BUSY_DEC(cmd)
#define PS3_DEV_BUSY_INC(cmd)
#endif

static inline Bool ps3_stat_qos_switch_is_open(const struct ps3_instance *instance)
{
	return (instance->cmd_statistics.cmd_stat_switch & PS3_STAT_QOS_SWITCH_OPEN);
}

#define PS3_CMD_WORD_STAT_INC(instance, cmd_word) do { \
	if (ps3_stat_inc_switch_is_open((instance))) { \
		ps3_cmd_word_stat_inc((instance), (cmd_word)); \
	} \
} while (0)

#define PS3_DEV_IO_START_OK_INC(instance, cmd) do { \
	if (ps3_stat_dev_switch_is_open((instance))) { \
		ps3_dev_io_start_ok_inc((instance), (cmd)); \
	} \
} while (0);

#define PS3_DEV_IO_BACK_INC(instance, cmd, status) do { \
	if (ps3_stat_dev_switch_is_open((instance))) { \
		ps3_dev_io_back_inc((instance), (cmd), (status)); \
	} \
} while (0);

#define PS3_DEV_IO_START_ERR_INC(instance, cmd) do { \
	if (ps3_stat_dev_switch_is_open((instance))) { \
		ps3_dev_io_start_err_inc((instance), (cmd)); \
	} \
} while (0);

#define PS3_DEV_IO_START_INC(instance, cmd) do { \
	if (ps3_stat_dev_switch_is_open((instance))) { \
		ps3_dev_io_start_inc((instance), (cmd)); \
	} \
} while (0)
#ifndef _WINDOWS
#define PS3_IO_BACK_INC(instance, s_cmd, status) do { \
	if (ps3_stat_inc_switch_is_open((instance))) { \
		ps3_scmd_inc((instance), (s_cmd), PS3_STAT_BACK, \
			ps3_resp_stauts_to_stat_status((status))); \
	} \
} while (0)
#else
#define PS3_IO_BACK_INC(instance, s_cmd, tag, status) do { \
	if (ps3_stat_inc_switch_is_open((instance))) { \
		ps3_scmd_inc((instance), (s_cmd), (tag), PS3_STAT_BACK, \
			ps3_resp_stauts_to_stat_status((status))); \
	} \
} while (0)
#endif
#define PS3_IO_OUTSTANDING_INC(instance, cmd) do { \
	ps3_io_outstand_inc(instance, cmd); \
} while (0)

#define PS3_DEV_IO_ERR_STAT_INC(instance, cmd) do { \
	if (ps3_stat_dev_switch_is_open((instance))) { \
		ps3_dev_scsi_err_stat_inc((cmd)); \
	} \
} while (0)
#ifndef _WINDOWS
#define PS3_IO_START_INC(instance, s_cmd) do { \
	if (ps3_stat_inc_switch_is_open((instance))) { \
		ps3_scmd_inc((instance), (s_cmd), PS3_STAT_START, PS3_STAT_BACK_OK); \
	} \
} while (0)

#define PS3_IO_BACK_ERR_INC(instance, s_cmd) do { \
	if (ps3_stat_inc_switch_is_open((instance))) { \
		ps3_scmd_inc((instance), (s_cmd), PS3_STAT_BACK, PS3_STAT_BACK_FAIL); \
	} \
} while (0)
#else
#define PS3_IO_START_INC(instance, s_cmd, tag) do { \
	if (ps3_stat_inc_switch_is_open((instance))) { \
		ps3_scmd_inc((instance), (s_cmd), (tag), PS3_STAT_START, PS3_STAT_BACK_OK); \
	} \
} while (0)

#define PS3_IO_BACK_ERR_INC(instance, s_cmd, tag) do { \
	if (ps3_stat_inc_switch_is_open((instance))) { \
		ps3_scmd_inc((instance), (s_cmd), (tag), PS3_STAT_BACK, PS3_STAT_BACK_FAIL); \
	} \
} while (0)
#endif
#define PS3_IO_DRV2IOC_START_INC(instance, cmd) do { \
	if (ps3_stat_inc_switch_is_open((instance))) { \
		ps3_scmd_drv2ioc_start_inc((instance), (cmd)); \
	} \
} while (0)

#define PS3_IOC_DRV2IOC_BACK_INC(instance, cmd, status) { \
	if (ps3_stat_inc_switch_is_open((instance))) { \
		ps3_scmd_drv2ioc_back_inc((instance), (cmd), \
		ps3_resp_stauts_to_stat_status((status))); \
	} \
} while (0)

#define PS3_MGR_CMD_STAT_INC(instance, cmd) do { \
	if (ps3_stat_inc_switch_is_open((instance))) { \
		ps3_mgr_start_inc((instance), (cmd)); \
	} \
} while (0)

#define PS3_MGR_CMD_BACK_INC(instance, cmd, status) do { \
	if (ps3_stat_inc_switch_is_open((instance))) { \
		ps3_mgr_back_inc((instance), (cmd), \
		ps3_resp_stauts_to_stat_status((status))); \
	} \
} while (0)

#define PS3_IO_OUTSTAND_DEC(instance, s_cmd) do { \
	if (ps3_stat_outstand_switch_is_open((instance))) { \
		ps3_io_outstand_dec((instance), (s_cmd)); \
	} \
} while (0)

#define PS3_VD_OUTSTAND_DEC(instance, s_cmd) do { \
			ps3_vd_outstand_dec(instance, s_cmd); \
} while (0)

#define PS3_VD_OUTSTAND_INC(instance, s_cmd) do { \
			ps3_vd_outstand_inc(instance, s_cmd); \
} while (0)

#define PS3_DEV_IO_OUTSTAND_DEC(instance, cmd) do { \
	if (ps3_stat_dev_switch_is_open((instance))) { \
		ps3_dev_io_outstand_dec((cmd)); \
	} \
} while (0)

#define PS3_DEV_IO_OUTSTAND_INC(instance, cmd) do { \
	if (ps3_stat_dev_switch_is_open((instance))) { \
		ps3_dev_io_outstand_inc((cmd)); \
	} \
} while (0)

#define PS3_QOS_CMD_INC(instance) do { \
	if (ps3_stat_qos_switch_is_open((instance))) { \
		ps3_qos_cmd_inc((instance)); \
	} \
} while (0)

#define PS3_QOS_STAT_START(instance, cmd, type) do { \
		if (ps3_stat_inc_switch_is_open((instance))) { \
			ps3_qos_stat_inc((instance), (cmd), (type), PS3_STAT_START); \
		} \
	} while (0)

#define PS3_QOS_STAT_END(instance, cmd, type) do { \
		if (ps3_stat_inc_switch_is_open((instance))) { \
			ps3_qos_stat_inc((instance), (cmd), (type), PS3_STAT_BACK); \
		} \
	} while (0)

S32 ps3_stat_workq_start(struct ps3_instance *instance);

void ps3_stat_workq_stop(struct ps3_instance *instance);

#ifdef __cplusplus
}
#endif

#endif
