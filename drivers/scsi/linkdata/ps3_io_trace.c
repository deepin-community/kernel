#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/crc32.h>
#endif
#include "ps3_instance_manager.h"
#include "ps3_scsih_cmd_parse.h"
#include "ps3_io_trace.h"

static inline const char *ps3_io_trace_direct_name(ps3_io_trace_dtype type)
{
	static const char *direct_name[] = {
		"cmd send",
		"cmd recv",
		"unknown"
	};
	if(type >= PS3_IO_TRACE_DIRECT_COUNT) {
		return direct_name[PS3_IO_TRACE_DIRECT_COUNT];
	}
	return direct_name[type];
}
#ifndef _WINDOWS
static inline Bool ps3_is_scsi_read_cmd(struct scsi_cmnd *s_cmnd)
{
	return (s_cmnd != NULL && s_cmnd->sc_data_direction == DMA_FROM_DEVICE &&
		(s_cmnd->cmnd[0] == READ_6 || s_cmnd->cmnd[0] == READ_10 ||
		s_cmnd->cmnd[0] == READ_12 || s_cmnd->cmnd[0] == READ_16));
}

static inline Bool ps3_is_scsi_write_cmd(struct scsi_cmnd *s_cmnd)
{
	return (s_cmnd != NULL && s_cmnd->sc_data_direction == DMA_TO_DEVICE &&
		(s_cmnd->cmnd[0] == WRITE_6 || s_cmnd->cmnd[0] == WRITE_10 ||
		s_cmnd->cmnd[0] == WRITE_12 || s_cmnd->cmnd[0] == WRITE_16));
}

static void ps3_scsi_sgl_crc(const struct ps3_cmd *cmd)
{
	U32 sge_count = 0;
	U32 i = 0;
	U32 crc = 0;
	U32 seed = 0x1234;
	S8 *pdata = NULL;
    U32 buf_len = 0;
	S8  buf[PS3_IO_TRACE_BUF_LEN] = { 0 };
	struct scatterlist *os_sgl = NULL;
	struct ps3_instance *instance = cmd->instance;
	struct scsi_cmnd *s_cmnd = cmd->scmd;

	if (instance->debug_context.io_trace_switch == PS3_FALSE) {
		goto l_out;
	}

	sge_count = cmd->os_sge_map_count;
	scsi_for_each_sg(s_cmnd, os_sgl, sge_count, i) {
		pdata = (S8 *)sg_virt(os_sgl);
		crc = crc32(seed, pdata, sg_dma_len(os_sgl));

		memset(buf, '\0', sizeof(buf));
		buf_len = snprintf(buf, PS3_IO_TRACE_BUF_LEN,
			"hno:%u  channel[%u], target[%u],"
			"trace_id[0x%llx], [%u of %u]sge, sge data addr[0x%llx] length[%u], "
			"D[%llx:%llx] CRC[0x%x] \n",
			PS3_HOST(instance), s_cmnd->device->channel, s_cmnd->device->id,
			cmd->trace_id,
			i + 1, sge_count, sg_dma_address(os_sgl),
			sg_dma_len(os_sgl), *(U64*)pdata, *((U64 *)(pdata + 8)), crc);
		if (buf_len >= PS3_IO_TRACE_BUF_LEN) {
			LOG_ERROR("buf_len > PS3_IO_TRACE_BUF_LEN\n");
			goto l_out;
		}
		DATA_DUMP(NULL, 0, buf);
	}

l_out:
	return;
}
#endif
void ps3_scsih_io_trace(const struct ps3_cmd *cmd, ps3_io_trace_dtype type)
{
	S32 buf_len = 0;
	char buf[PS3_IO_TRACE_BUF_LEN] = { 0 };
	U64 lba = 0;

	if (cmd == NULL || cmd->scmd == NULL) {
		LOG_WARN("cmd or scmd is null\n");
		goto l_out;
	}
	if (scsi_sg_count(cmd->scmd) == 0) {
		goto l_out;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0))
	if (cmd->scmd->cmnd == NULL || cmd->instance->host == NULL) {
		LOG_WARN("cdb null\n");
		goto l_out;
	}
#else
	if (cmd->instance->host == NULL) {
		LOG_WARN("cdb null\n");
		goto l_out;
	}
#endif

	if ((type == PS3_IO_TRACE_DIRECT_SEND && ps3_is_scsi_write_cmd(cmd->scmd)) ||
		(type == PS3_IO_TRACE_DIRECT_RECV && ps3_is_scsi_read_cmd(cmd->scmd))){
		ps3_scsi_sgl_crc(cmd);
	}

	ps3_scsih_lba_parse(cmd->scmd->cmnd, &lba);
	memset(buf, '\0', sizeof(buf));
	buf_len = snprintf(buf, PS3_IO_TRACE_BUF_LEN,
		"%s, trace_id[0x%llx], hno:%u  lba[0x%llx]\n",
		ps3_io_trace_direct_name(type), cmd->trace_id, PS3_HOST(cmd->instance), lba);
	if (buf_len >= PS3_IO_TRACE_BUF_LEN) {
		LOG_ERROR("buf_len > PS3_IO_TRACE_BUF_LEN\n");
		goto l_out;
	}
	DATA_DUMP(sg_virt(cmd->scmd->sdb.table.sgl),
		min_t(U32, cmd->scmd->sdb.table.sgl[0].length,
			PS3_IO_TRACE_PRINT_COUNT), buf);

l_out:
	return;
}

