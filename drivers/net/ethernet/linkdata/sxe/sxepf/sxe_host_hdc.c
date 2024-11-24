
#include <linux/cpu.h>
#include <linux/percpu-defs.h>
#include <linux/percpu.h>
#include <linux/atomic.h>

#include "sxe.h"
#ifndef NO_NEED_SIGNAL_H
#include <linux/sched/signal.h>
#endif
#include "sxe_host_hdc.h"
#include "sxe_log.h"
#include "sxe_hw.h"
#include "sxe_msg.h"
#include "drv_msg.h"


static atomic_t hdc_available = ATOMIC_INIT(1);

static DEFINE_PER_CPU(union sxe_trace_info, sxe_trace_id);

#define TRACE_ID_CHIP_OUT_COUNT_MASK  0x000FFFFFFFFFFFFFLLU
#define TRACE_ID_CHIP_OUT_CPUID_MASK  0x7FFLLU

#define SXE_HDC_RETRY_CNT   (250)
#define SXE_HDC_RETRY_ITR   (10)

#define NS_TO_MS_UNIT (1000000)

#ifdef DEFINE_SEMAPHORE_NEED_CNT
DEFINE_SEMAPHORE(g_hdc_sema, 1);
#else
DEFINE_SEMAPHORE(g_hdc_sema);
#endif

static void sxe_trace_id_alloc(u64 *trace_id)
{
	union sxe_trace_info *id = NULL;
	u64 trace_id_count = 0;

	preempt_disable();
	id = this_cpu_ptr(&sxe_trace_id);

	trace_id_count = id->sxe_trace_id_param.count;
	++trace_id_count;
	id->sxe_trace_id_param.count = (trace_id_count & TRACE_ID_CHIP_OUT_COUNT_MASK);

	*trace_id = id->trace_id;
	preempt_enable();

	return;
}

static void sxe_trace_id_init(void)
{
	s32 cpu = 0;
	union sxe_trace_info *id = NULL;

	for_each_possible_cpu(cpu) {
		id = &per_cpu(sxe_trace_id, cpu);
		id->sxe_trace_id_param.cpu_id =
					(cpu & TRACE_ID_CHIP_OUT_CPUID_MASK);
		id->sxe_trace_id_param.count = 0;
	}

	return;
}

static s32 sxe_cli_fw_time_sync(struct sxe_adapter *adapter)
{
	s32 ret;
	struct sxe_driver_cmd cmd;
	u64 timestamp = ktime_get_real_ns();
	struct sxe_hw *hw = &adapter->hw;

	timestamp = timestamp / NS_TO_MS_UNIT;
	LOG_INFO_BDF("sync time= %llu ms\n", timestamp);

	cmd.req      = &timestamp;
	cmd.req_len  = sizeof(timestamp);
	cmd.resp     = NULL;
	cmd.resp_len = 0;
	cmd.trace_id = 0;
	cmd.opcode   = SXE_CMD_TINE_SYNC;
	cmd.is_interruptible = true;
	ret = sxe_driver_cmd_trans(hw, &cmd);
	if (ret) {
		LOG_ERROR_BDF("hdc trans failed ret=%d, cmd:time sync,"
			"failed count=%u\n",
			ret, adapter->hdc_ctxt.time_sync_failed);
			adapter->hdc_ctxt.time_sync_failed++;
	}

	return ret;
}

s32 sxe_host_to_fw_time_sync(struct sxe_adapter *adapter)
{
	s32 ret = 0;
	s32 ret_v;
	u32 status;
	struct sxe_hw *hw = &adapter->hw;

	status = hw->hdc.ops->fw_status_get(hw);
	if (status != SXE_FW_START_STATE_FINISHED) {
		LOG_ERROR_BDF("fw[%p] status[0x%x] is not good,"
				"and time_sync_failed=%u\n",
				hw, status, adapter->hdc_ctxt.time_sync_failed);
		adapter->hdc_ctxt.time_sync_failed++;
		ret = -SXE_FW_STATUS_ERR;
		goto l_ret;
	}

	ret_v = sxe_cli_fw_time_sync(adapter);
	if (ret_v) {
		LOG_WARN_BDF("fw time sync failed, ret_v=%d\n",ret_v);
		goto l_ret;
	}

l_ret:
	return ret;
}

void sxe_time_sync_handler(struct work_struct *work)
{
	s32 ret;
	struct sxe_adapter *adapter = container_of(work,
				struct sxe_adapter, hdc_ctxt.time_sync_work);
	ret = sxe_host_to_fw_time_sync(adapter);
	if (ret) {
		LOG_ERROR_BDF("time sync handler err, ret=%d\n", ret);
	}

	return;
}

struct semaphore *sxe_hdc_sema_get(void)
{
	return &g_hdc_sema;
}

void sxe_hdc_available_set(s32 value)
{
	atomic_set(&hdc_available, value);
	return;
}

void sxe_hdc_channel_init(struct sxe_hdc_context *hdc_ctxt)
{
	sxe_trace_id_init();

	init_completion(&hdc_ctxt->sync_done);

	INIT_WORK(&hdc_ctxt->time_sync_work, sxe_time_sync_handler);

	sxe_hdc_available_set(1);
	hdc_ctxt->time_sync_failed = 0;
	return;
}

void sxe_hdc_channel_destroy(struct sxe_hw *hw)
{
	sxe_hdc_available_set(0);
	hw->hdc.ops->resource_clean(hw);
	return;
}

static inline s32 sxe_hdc_lock_get(struct sxe_hw *hw)
{
	s32 ret = SXE_HDC_FALSE;
	struct sxe_adapter *adapter = hw->adapter;

	if (atomic_read(&hdc_available)) {
		ret = hw->hdc.ops->pf_lock_get(hw, SXE_HDC_TRYLOCK_MAX);
	} else {
		LOG_ERROR_BDF("hdc channel not available\n");
	}

	return ret;
}

static inline void sxe_hdc_lock_release(struct sxe_hw *hw)
{
	hw->hdc.ops->pf_lock_release(hw, SXE_HDC_RELEASELOCK_MAX);
	return;
}

static inline s32 sxe_poll_fw_ack(struct sxe_hw *hw, u32 timeout, 
					bool is_interruptible)
{
	s32 ret = 0;
	u32 i;
	bool fw_ov = false;
	struct sxe_adapter *adapter = hw->adapter;

	if (atomic_read(&hdc_available)) {
		for (i = 0; i < timeout; i++) {
			fw_ov = hw->hdc.ops->is_fw_over_set(hw);
			if (fw_ov) {
				break;
			}

			if (is_interruptible) {
				if (msleep_interruptible(SXE_HDC_RETRY_ITR)){
					ret = -EINTR;
					LOG_DEV_INFO("interrupted, exit polling\n");
					goto l_ret;
				}
			} else {
				msleep(SXE_HDC_RETRY_ITR);
			}
		}

		if (i >= timeout) {
			LOG_ERROR_BDF("poll fw_ov timeout...\n");
			ret = -SXE_ERR_HDC_FW_OV_TIMEOUT;
			goto l_ret;
		}

		hw->hdc.ops->fw_ov_clear(hw);
		ret = 0;
	} else {
		ret = SXE_HDC_FALSE;
		LOG_ERROR_BDF("hdc channel not available\n");
	}

l_ret:
	return ret;
}

#ifdef SXE_NEED_PROCESS_CANCEL
static inline bool is_interrupt_signal(struct task_struct *task)
{
	bool is_inter = false;

	if (sigismember(&task->pending.signal, SIGINT) ||
		sigismember(&task->pending.signal, SIGKILL) ||
		sigismember(&task->pending.signal, SIGQUIT)) {
		is_inter = true;
		goto l_ret;
	}

l_ret:
	return is_inter;
}
#endif

void sxe_hdc_irq_handler(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	u32 irq_event = hw->hdc.ops->irq_event_get(hw);

	hw->irq.ops->specific_irq_disable(hw, SXE_EIMC_HDC);

	LOG_DEBUG_BDF("hdc irq interrupt coming\n");
	if (irq_event & MSI_EVT_HDC_FWOV) {
		LOG_DEBUG_BDF("hdc fw over event occur\n");
		hw->hdc.ops->irq_event_clear(hw, MSI_EVT_HDC_FWOV);

		hw->hdc.ops->fw_ov_clear(hw);
		complete(&adapter->hdc_ctxt.sync_done);
	}

	if (irq_event & MSI_EVT_HDC_TIME_SYNC) {
		LOG_DEBUG_BDF("hdc fw sync time event occur\n");

		hw->hdc.ops->irq_event_clear(hw, MSI_EVT_HDC_TIME_SYNC);
		schedule_work(&adapter->hdc_ctxt.time_sync_work);
	}

	return;
}

static s32 sxe_wait_fw_ack(struct sxe_hw *hw, u64 trace_id)
{
	s32 ret;
	struct sxe_adapter *adapter = container_of(hw, struct sxe_adapter, hw);

	while(1) {
		ret = wait_for_completion_interruptible(&adapter->hdc_ctxt.sync_done);
		if (ret == 0) {
			LOG_DEBUG_BDF("cmd trace=0x%llx, "
				"wait_for_completion_interrupted success\n",
				trace_id);
			break;
		}

		ret = signal_pending(current);
		if(!ret) {
			LOG_DEBUG_BDF("cmd trace=0x%llx, no pending signal,"
				"continue wait",trace_id);
			continue;
		} else {
			LOG_DEBUG_BDF("cmd trace=0x%llx got signal, default quit\n",
				trace_id);
			ret = -EINTR;
			break;
		}

#ifdef SXE_NEED_PROCESS_CANCEL
		ret = is_interrupt_signal(current);
		if (ret) {
			LOG_DEBUG_BDF("cmd trace=0x%llx interrupted, need cancel\n",
					trace_id);
			ret = -EINTR;
			break;
		} else {
			LOG_DEBUG_BDF("cmd trace=0x%llx got other signal, ignore\n",
					trace_id);
		}
#endif
	}

	return ret;
}

static s32 hdc_packet_ack_get(struct sxe_hw *hw, u64 trace_id,
				HdcHeader_u *pkt_header, 
				bool use_msi, bool is_interruptible)
{
	s32 ret     = 0;
	u32 timeout = SXE_HDC_WAIT_TIME;
	struct sxe_adapter *adapter = hw->adapter;

	pkt_header->dw0 = 0;
	pkt_header->head.errCode = PKG_ERR_OTHER;

	LOG_INFO_BDF("trace_id=0x%llx hdc cmd ack get start, mode=%s\n",
			 trace_id, use_msi ? "msi inter":"polling");
	if (use_msi) {
		ret = sxe_wait_fw_ack(hw, trace_id);
	}else {
		ret = sxe_poll_fw_ack(hw, timeout, is_interruptible);
	}

	if (ret) {
		LOG_ERROR_BDF("get fw ack failed, mode=%s ret=%d\n",
				use_msi ? "msi inter":"polling", ret);
		goto l_out;
	}

	pkt_header->dw0 = hw->hdc.ops->fw_ack_header_rcv(hw);;
	if (pkt_header->head.errCode == PKG_ERR_PKG_SKIP) {
		ret = -SXE_HDC_PKG_SKIP_ERR;
		goto l_out;
	} else if (pkt_header->head.errCode != PKG_OK) {
		ret = -SXE_HDC_PKG_OTHER_ERR;
		goto l_out;
	}

l_out:
	LOG_INFO_BDF("trace_id=0x%llx hdc cmd ack get end ret=%d\n", trace_id, ret);
	return ret;
}

static void hdc_packet_header_fill(HdcHeader_u *pkt_header,
			u8 pkt_index, u16 total_len,
			u16 pkt_num, u8 is_read, bool use_msi)
{
	U16 pkt_len = 0;

	pkt_header->dw0 = 0;

	pkt_header->head.pid = (is_read == 0) ? pkt_index : (pkt_index - 1);

	pkt_header->head.totalLen = SXE_HDC_LEN_TO_REG(total_len);

	if (pkt_index == 0 && is_read == 0) {
		pkt_header->head.startPkg = SXE_HDC_BIT_1;
	}

	if (pkt_index == (pkt_num - 1)) {
		pkt_header->head.endPkg = SXE_HDC_BIT_1;
		pkt_len = total_len - (DWORD_NUM * (pkt_num - 1));
	} else {
		pkt_len = DWORD_NUM;
	}

	pkt_header->head.len  = SXE_HDC_LEN_TO_REG(pkt_len);
	pkt_header->head.isRd = is_read;
	if (use_msi) {
		pkt_header->head.msi = 1;
	}

	return ;
}

static inline void hdc_channel_clear(struct sxe_hw *hw)
{
	hw->hdc.ops->fw_ov_clear(hw);
}

static inline void hdc_packet_send_done(struct sxe_hw *hw)
{
	hw->hdc.ops->packet_send_done(hw);
	return;
}

static inline void hdc_packet_header_send(struct sxe_hw *hw,
							u32 header)
{
	hw->hdc.ops->packet_header_send(hw, header);
	return;
}

static inline void hdc_packet_data_dword_send(struct sxe_hw *hw,
						u16 dword_index, u32 value)
{
	hw->hdc.ops->packet_data_dword_send(hw, dword_index, value);
	return;
}

static void hdc_packet_send(struct sxe_hw *hw, u64 trace_id,
			HdcHeader_u *pkt_header, u8 *data,
			u16 data_len)
{
	u16          dw_idx   = 0;
	u16          pkt_len       = 0;
	u16          offset        = 0;
	u32          pkg_data      = 0;
	struct sxe_adapter *adapter = hw->adapter;

	LOG_DEBUG_BDF("hw_addr[%p] trace_id=0x%llx send pkt pkg_header[0x%x], "
		"data_addr[%p], data_len[%u]\n",
		hw, trace_id, pkt_header->dw0, data, data_len);

	hdc_packet_header_send(hw, pkt_header->dw0);

	if (data == NULL || data_len == 0) {
		goto l_send_done;
	}

	pkt_len = SXE_HDC_LEN_FROM_REG(pkt_header->head.len);
	for (dw_idx = 0; dw_idx < pkt_len; dw_idx++) {
		pkg_data = 0;

		offset = dw_idx * BYTE_PER_DWORD;

		if ((pkt_header->head.endPkg == SXE_HDC_BIT_1)
			&& (dw_idx == (pkt_len - 1))
			&& (data_len % BYTE_PER_DWORD != 0)) {
			memcpy((u8 *)&pkg_data, data + offset,
					data_len % BYTE_PER_DWORD);
		} else {
			pkg_data = *(u32 *)(data + offset);
		}

		LOG_DEBUG_BDF("trace_id=0x%llx send data to reg[%u] dword[0x%x]\n",
				trace_id, dw_idx, pkg_data);
		hdc_packet_data_dword_send(hw, dw_idx, pkg_data);
	}

l_send_done:
	hdc_channel_clear(hw);

	hdc_packet_send_done(hw);

	return;
}

static inline u32 hdc_packet_data_dword_rcv(struct sxe_hw *hw,
						u16 dword_index)
{
	return hw->hdc.ops->packet_data_dword_rcv(hw, dword_index);
}

static void hdc_resp_data_rcv(struct sxe_hw *hw, u64 trace_id,
				HdcHeader_u *pkt_header, u8 *out_data,
				u16 out_len)
{
	u16          dw_idx      = 0;
	u16          dw_num      = 0;
	u16          offset = 0;
	u32          pkt_data;
	struct sxe_adapter *adapter = hw->adapter;

	dw_num = SXE_HDC_LEN_FROM_REG(pkt_header->head.len);
	for (dw_idx = 0; dw_idx < dw_num; dw_idx++) {
		pkt_data= hdc_packet_data_dword_rcv(hw, dw_idx);
		offset = dw_idx * BYTE_PER_DWORD;
		LOG_DEBUG_BDF("trace_id=0x%llx get data from reg[%u] dword=0x%x\n",
				trace_id, dw_idx, pkt_data);

		if ((pkt_header->head.endPkg == SXE_HDC_BIT_1)
			&& (dw_idx == (dw_num - 1)) && (out_len % BYTE_PER_DWORD != 0)) {
			memcpy(out_data + offset, (u8 *)&pkt_data,
					out_len % BYTE_PER_DWORD);
		} else {
			*(u32 *)(out_data + offset) = pkt_data;
		}
	}

	return;
}

STATIC s32 hdc_req_process(struct sxe_hw *hw, u64 trace_id,
		u8 *in_data, u16 in_len, bool use_msi, bool is_interruptible)
{
	s32 ret 	= 0;
	u32 total_len	= 0;
	u16 pkt_num     = 0;
	u16 index       = 0;
	u16 offset      = 0;
	HdcHeader_u     pkt_header;
	bool is_retry   = false;
	struct sxe_adapter *adapter = hw->adapter;

	total_len  = (in_len + BYTE_PER_DWORD - 1) / BYTE_PER_DWORD;

	pkt_num = (in_len + ONE_PACKET_LEN_MAX - 1) / ONE_PACKET_LEN_MAX;
	LOG_DEBUG_BDF("hw[%p] trace_id=0x%llx req in_data[%p] in_len=%u, "
			"total_len=%uDWORD, pkt_num = %u, mode=%s\n",
			hw, trace_id, in_data, in_len, total_len,
			pkt_num, use_msi ? "msi":"polling");

	for (index = 0; index < pkt_num; index++) {
		LOG_DEBUG_BDF("trace_id=0x%llx fill pkg header[%p], pkg_index[%u], "
			"total_Len[%u], pkg_num[%u], is_read[no]\n",
			trace_id, &pkt_header, index, total_len, pkt_num);
		hdc_packet_header_fill(&pkt_header, index, total_len,
						pkt_num, 0, use_msi);

		offset = index * DWORD_NUM * BYTE_PER_DWORD;
		hdc_packet_send(hw, trace_id, &pkt_header,
				in_data + offset, in_len);

		if (index == pkt_num - 1) {
			break;
		}

		ret = hdc_packet_ack_get(hw, trace_id, &pkt_header, 
					use_msi, is_interruptible);
		if (ret == -EINTR) {
			LOG_ERROR_BDF("hdc cmd trace_id=0x%llx interrupted\n", trace_id);
			goto l_out;
		} else if (ret == -SXE_HDC_PKG_SKIP_ERR) {
			LOG_ERROR_BDF("hdc cmd trace_id=0x%llx req ack"
					"failed, retry\n", trace_id);
			if (is_retry) {
				ret = -SXE_HDC_RETRY_ERR;
				goto l_out;
			}

			index --;
			is_retry = true;
			continue;
		} else if (ret != SXE_HDC_SUCCESS) {
			LOG_ERROR_BDF("hdc cmd trace_id=0x%llx req ack"
					"failed, ret=%d\n", trace_id, ret);
			ret = -SXE_HDC_RETRY_ERR;
			goto l_out;
		}

		LOG_DEBUG_BDF("hdc cmd trace_id=0x%llx get req packet_index[%u]"
			" ack succeed header[0x%x]\n",
			trace_id, index, pkt_header.dw0);
		is_retry = false;
	}

l_out:
	return ret;
}

static s32 hdc_resp_process(struct sxe_hw *hw, u64 trace_id,
		u8 *out_data, u16 out_len, bool use_msi, bool is_interruptible)
{
	s32          ret;
	u32          req_dwords;
	u32          resp_len;
	u32          resp_dwords;
	u16          pkt_num;
	u16          index;
	u16          offset;
	HdcHeader_u  pkt_header;
	bool     retry          = false;
	struct sxe_adapter *adapter = hw->adapter;

	LOG_INFO_BDF("hdc trace_id=0x%llx req's last cmd ack get, mode=%s\n",
		trace_id, use_msi ? "msi" : "polling");
	ret = hdc_packet_ack_get(hw, trace_id, &pkt_header, 
					use_msi, is_interruptible);
	if (ret == -EINTR) {
		LOG_ERROR_BDF("hdc cmd trace_id=0x%llx interrupted\n", trace_id);
		goto l_out;
	} else if(ret) {
		LOG_ERROR_BDF("hdc trace_id=0x%llx ack get failed, ret=%d\n",
				trace_id, ret);
		ret = -SXE_HDC_RETRY_ERR;
		goto l_out;
	}

	LOG_INFO_BDF("hdc trace_id=0x%llx req's last cmd ack get"
		"succeed header[0x%x]\n",trace_id, pkt_header.dw0);

	if (!pkt_header.head.startPkg) {
		ret = -SXE_HDC_RETRY_ERR;
		LOG_ERROR_BDF("trace_id=0x%llx ack header has error: "
				"not set start bit\n",trace_id);
		goto l_out;
	}

	req_dwords = (out_len + BYTE_PER_DWORD - 1) / BYTE_PER_DWORD;
	resp_dwords  = SXE_HDC_LEN_FROM_REG(pkt_header.head.totalLen);
	if (resp_dwords > req_dwords) {
		ret = -SXE_HDC_RETRY_ERR;
		LOG_ERROR_BDF("trace_id=0x%llx rsv len check failed:"
				"resp_dwords=%u, req_dwords=%u\n",trace_id,
				resp_dwords, req_dwords);
		goto l_out;
	}

	resp_len = resp_dwords << DWORD_TO_BYTE_SHIFT;
	LOG_INFO_BDF("outlen = %u bytes, resp_len = %u bytes\n", out_len, resp_len);
	if (resp_len > out_len) {
		resp_len = out_len;
	}

	hdc_resp_data_rcv(hw, trace_id, &pkt_header, out_data, resp_len);

	pkt_num = (resp_len + ONE_PACKET_LEN_MAX - 1) / ONE_PACKET_LEN_MAX;
	for (index = 1; index < pkt_num; index++) {
		LOG_DEBUG_BDF("trace_id=0x%llx fill pkg header[%p], pkg_index[%u], "
			"total_Len[%u], pkg_num[%u], is_read[yes] use_msi=%s\n",
			trace_id, &pkt_header, index, resp_dwords,
			pkt_num, use_msi ? "yes" : "no");
		hdc_packet_header_fill(&pkt_header, index, resp_dwords,
					pkt_num, 1, use_msi);

		hdc_packet_send(hw, trace_id, &pkt_header, NULL, 0);

		ret = hdc_packet_ack_get(hw, trace_id, &pkt_header, 
						use_msi, is_interruptible);
		if (ret == -EINTR) {
			LOG_ERROR_BDF("hdc cmd trace_id=0x%llx interrupted\n", trace_id);
			goto l_out;
		} else if (ret == -SXE_HDC_PKG_SKIP_ERR) {
			LOG_ERROR_BDF("trace_id=0x%llx hdc resp ack polling"
					"failed, ret=%d\n", trace_id, ret);
			if (retry) {
				ret = -SXE_HDC_RETRY_ERR;
				goto l_out;
			}

			index --;
			retry = true;
			continue;
		} else if (ret != SXE_HDC_SUCCESS) {
			LOG_ERROR_BDF("trace_id=0x%llx hdc resp ack polling"
					"failed, ret=%d\n",trace_id, ret);
			ret = -SXE_HDC_RETRY_ERR;
			goto l_out;
		}

		LOG_INFO_BDF("hdc trace_id=0x%llx resp pkt[%u] get "
			"succeed header[0x%x]\n",
			trace_id, index, pkt_header.dw0);

		retry = false;

		offset = index * DWORD_NUM * BYTE_PER_DWORD;
		hdc_resp_data_rcv(hw, trace_id, &pkt_header,
					out_data + offset, resp_len);
	}

l_out:
	return ret;
}

static s32 sxe_hdc_packet_trans(struct sxe_hw *hw, u64 trace_id,
					struct sxe_hdc_trans_info *trans_info,
					bool use_msi, bool is_interruptible)
{
	s32 ret = SXE_SUCCESS;
	u32 status;
	struct sxe_adapter *adapter = hw->adapter;
	u32 channel_state;

	status = hw->hdc.ops->fw_status_get(hw);
	if (status != SXE_FW_START_STATE_FINISHED) {
		LOG_ERROR_BDF("fw[%p] status[0x%x] is not good\n",hw, status);
		ret = -SXE_FW_STATUS_ERR;
		goto l_ret;
	}

	channel_state = hw->hdc.ops->channel_state_get(hw);
	if (channel_state != SXE_FW_HDC_TRANSACTION_IDLE) {
		LOG_ERROR_BDF("hdc channel state is busy\n");
		ret = -SXE_HDC_RETRY_ERR;
		goto l_ret;
	}

	ret = sxe_hdc_lock_get(hw);
	if (ret) {
		LOG_ERROR_BDF("hw[%p] cmd trace_id=0x%llx get hdc lock fail, ret=%d\n",
				hw, trace_id, ret);
		ret = -SXE_HDC_RETRY_ERR;
		goto l_ret;
	}

	ret = hdc_req_process(hw, trace_id, trans_info->in.data,
				trans_info->in.len, use_msi, is_interruptible);
	if (ret) {
		LOG_ERROR_BDF("hdc cmd trace_id=0x%llx req process"
				"failed, ret=%d\n",trace_id, ret);
		goto l_hdc_lock_release;
	}

	ret = hdc_resp_process(hw, trace_id, trans_info->out.data,
				trans_info->out.len, use_msi, is_interruptible);
	if (ret) {
		LOG_ERROR_BDF("hdc cmd trace_id=0x%llx resp process"
				"failed, ret=%d\n",trace_id, ret);
	}

l_hdc_lock_release:
	sxe_hdc_lock_release(hw);
l_ret:
	return ret;
}

STATIC s32 sxe_hdc_cmd_process(struct sxe_hw *hw, u64 trace_id,
				struct sxe_hdc_trans_info *trans_info,
				bool use_msi, bool is_interruptible)
{
	s32 ret;
	u8 retry_idx;
	struct sxe_adapter *adapter = hw->adapter;

	LOG_DEBUG_BDF("hw[%p] %s cmd trace=0x%llx get use sema = %p, count=%u\n",hw,
		use_msi ? "driver" : "user", trace_id,
		sxe_hdc_sema_get(), sxe_hdc_sema_get()->count);
	if (is_interruptible) {
		ret = down_interruptible(sxe_hdc_sema_get());
		if (ret) {
			ret = -EINTR;
			LOG_WARN_BDF("hw[%p] hdc concurrency full\n", hw);
			goto l_ret;
		}
	} else {
		down(sxe_hdc_sema_get());
	}

	for (retry_idx = 0; retry_idx < SXE_HDC_RETRY_CNT; retry_idx++ ) {
		ret = sxe_hdc_packet_trans(hw, trace_id, trans_info, 
						use_msi, is_interruptible);
		if (ret == SXE_SUCCESS) {
			goto l_up;
		} else if (ret == -SXE_HDC_RETRY_ERR) {
			if (is_interruptible) {
				if (msleep_interruptible(SXE_HDC_RETRY_ITR)) {
					ret = -EINTR;
					LOG_ERROR_BDF("interrupted, exit polling\n");
					goto l_up;
				}
			} else {
				msleep(SXE_HDC_RETRY_ITR);
			}

			continue;
	 	} else {
			LOG_ERROR_BDF("sxe hdc packet trace_id=0x%llx"
					"trans error, ret=%d\n", trace_id, ret);
			ret = -EFAULT;
			goto l_up;
		}
	}

l_up:
	LOG_DEBUG_BDF("hw[%p] %s cmd trace=0x%llx up sema = %p, count=%u\n",hw,
			use_msi ? "driver" : "user", trace_id,
			sxe_hdc_sema_get(), sxe_hdc_sema_get()->count);
	up(sxe_hdc_sema_get());
l_ret:
	if (ret == -SXE_HDC_RETRY_ERR) {
		ret = -EFAULT;
	}
	return ret;
}

static void sxe_cmd_hdr_init(struct sxe_hdc_cmd_hdr *cmd_hdr,
					u8 cmd_type)
{
	cmd_hdr->cmd_type = cmd_type;
	cmd_hdr->cmd_sub_type = 0;
	return;
}

static void sxe_driver_cmd_msg_init(struct sxe_hdc_drv_cmd_msg *msg,
						u16 opcode, u64 trace_id,
						void *req_data, u16 req_len)
{
	LOG_DEBUG("cmd[opcode=0x%x], trace=0x%llx, req_data_len=%u start init\n",
			opcode, trace_id, req_len);
	msg->opcode = opcode;
	msg->length.req_len = SXE_HDC_MSG_HDR_SIZE + req_len;
	msg->traceid = trace_id;

	if (req_data && req_len != 0) {
		memcpy(msg->body, (u8 *)req_data, req_len);
	}

	return;
}

static void sxe_hdc_trans_info_init(
					struct sxe_hdc_trans_info *trans_info,
					u8 *in_data_buf, u16 in_len,
					u8 *out_data_buf, u16 out_len)
{
	trans_info->in.data  = in_data_buf;
	trans_info->in.len   = in_len;
	trans_info->out.data = out_data_buf;
	trans_info->out.len  = out_len;
	return;
}

s32 sxe_driver_cmd_trans(struct sxe_hw *hw, struct sxe_driver_cmd *cmd)
{
	s32 ret = SXE_SUCCESS;
	struct sxe_hdc_cmd_hdr *cmd_hdr;
	struct sxe_hdc_drv_cmd_msg *msg;
	struct sxe_hdc_drv_cmd_msg *ack;
	struct sxe_hdc_trans_info trans_info;
	struct sxe_adapter *adapter = hw->adapter;
	void *req_data = cmd->req, *resp_data = cmd->resp;
	u16 opcode = cmd->opcode, req_len = cmd->req_len, resp_len = cmd->resp_len;

	u8 *in_data_buf;
	u8 *out_data_buf;
	u16 in_len;
	u16 out_len;
	u64 trace_id = 0;
	u16 ack_data_len;

	in_len = SXE_HDC_CMD_HDR_SIZE + SXE_HDC_MSG_HDR_SIZE + req_len;
	out_len = SXE_HDC_CMD_HDR_SIZE + SXE_HDC_MSG_HDR_SIZE + resp_len;

	sxe_trace_id_alloc(&trace_id);

	in_data_buf = kzalloc(in_len, GFP_KERNEL);
	if (in_data_buf == NULL) {
		LOG_ERROR_BDF("cmd trace_id=0x%llx kzalloc indata"
				"mem len[%u] failed\n",trace_id, in_len);
		ret = -ENOMEM;
		goto l_ret;
	}

	out_data_buf = kzalloc(out_len, GFP_KERNEL);
	if (out_data_buf == NULL) {
		LOG_ERROR_BDF("cmd trace_id=0x%llx kzalloc out_data"
				"mem len[%u] failed\n",trace_id, out_len);
		ret = -ENOMEM;
		goto l_in_buf_free;
	}

	cmd_hdr = (struct sxe_hdc_cmd_hdr *)in_data_buf;
	sxe_cmd_hdr_init(cmd_hdr, SXE_CMD_TYPE_DRV);

	msg = (struct sxe_hdc_drv_cmd_msg *)((u8 *)in_data_buf + SXE_HDC_CMD_HDR_SIZE);
	sxe_driver_cmd_msg_init(msg, opcode, trace_id, req_data, req_len);

	LOG_DEBUG_BDF("trans drv cmd:trace_id=0x%llx, opcode[0x%x], "
			"inlen=%u, out_len=%u\n",
			trace_id, opcode, in_len, out_len);

	sxe_hdc_trans_info_init(&trans_info,
				in_data_buf, in_len,
				out_data_buf, out_len);

	ret = sxe_hdc_cmd_process(hw, trace_id, &trans_info, false, 
					cmd->is_interruptible);
	if (ret) {
		LOG_DEV_DEBUG("hdc cmd[0x%x] trace_id=0x%llx process"
				"failed, ret=%d\n",opcode, trace_id, ret);
		goto l_out_buf_free;
	}

	ack = (struct sxe_hdc_drv_cmd_msg *)((u8 *)out_data_buf + SXE_HDC_CMD_HDR_SIZE);

	if (ack->errcode) {
		LOG_DEV_DEBUG("driver get hdc ack failed trace_id=0x%llx, err=%d\n",
				trace_id, ack->errcode);
		ret = -SXE_ERR_CLI_FAILED;
		goto l_out_buf_free;
	}

	ack_data_len = ack->length.ack_len - SXE_HDC_MSG_HDR_SIZE;
	if (resp_len != ack_data_len) {
		LOG_DEV_DEBUG("ack trace_id=0x%llx data len[%u]"
			" and resp_len[%u] dont match\n",
			trace_id, ack_data_len, resp_len);
		ret = -SXE_ERR_CLI_FAILED;
		goto l_out_buf_free;
	}

	if (resp_len != 0) {
		memcpy(resp_data, ack->body, resp_len);
	}

	LOG_DEBUG_BDF("driver get hdc ack trace_id=0x%llx,"
			"ack_len=%u, ack_data_len=%u\n",
			trace_id, ack->length.ack_len, ack_data_len);

l_out_buf_free:
	kfree(out_data_buf);
l_in_buf_free:
	kfree(in_data_buf);
l_ret:
	return ret;
}

s32 sxe_cli_cmd_trans(struct sxe_hw *hw, struct sxe_driver_cmd *cmd)
{
	s32 ret = SXE_SUCCESS;
	struct sxe_hdc_cmd_hdr *cmd_hdr;
	struct sxe_hdc_trans_info trans_info;
	struct sxe_adapter *adapter = hw->adapter;
	u64 trace_id = cmd->trace_id;
	u16 in_len = cmd->req_len, out_len = cmd->resp_len;
	u8 *in_data = cmd->req;
	u8 *out_data = cmd->resp;

	u8 *in_data_buf;
	u8 *out_data_buf;
	u16 in_buf_len = in_len + SXE_HDC_CMD_HDR_SIZE;
	u16 out_buf_len = out_len + SXE_HDC_CMD_HDR_SIZE;

	in_data_buf = kzalloc(in_buf_len, GFP_KERNEL);
	if (in_data_buf == NULL) {
		LOG_ERROR_BDF("cmd trace_id=0x%llx kzalloc indata"
				"mem len[%u] failed\n",trace_id, in_buf_len);
		ret = -ENOMEM;
		goto l_ret;
	}

	out_data_buf = kzalloc(out_buf_len, GFP_KERNEL);
	if (out_data_buf == NULL) {
		LOG_ERROR_BDF("cmd trace_id=0x%llx kzalloc out_data"
				"mem len[%u] failed\n",trace_id, out_buf_len);
		ret = -ENOMEM;
		goto l_in_buf_free;
	}

	if (copy_from_user(in_data_buf + SXE_HDC_CMD_HDR_SIZE,
			(void __user*)in_data, in_len)) {
		LOG_ERROR_BDF("hw[%p] cmd trace_id=0x%llx copy from user err\n",
				hw, trace_id);
		ret = -EFAULT;
		goto l_out_buf_free;
	}

	cmd_hdr = (struct sxe_hdc_cmd_hdr *)in_data_buf;
	sxe_cmd_hdr_init(cmd_hdr, SXE_CMD_TYPE_CLI);

	LOG_DEBUG_BDF("trans cli cmd:trace_id=0x%llx,, inlen=%u, out_len=%u\n",
			trace_id, in_len, out_len);
	sxe_hdc_trans_info_init(&trans_info,
				in_data_buf, in_buf_len,
				out_data_buf, out_buf_len);

	ret = sxe_hdc_cmd_process(hw, trace_id, &trans_info, false,
				 cmd->is_interruptible);
	if (ret) {
		LOG_DEV_DEBUG("hdc cmd trace_id=0x%llx hdc packet trans"
				"failed, ret=%d\n",trace_id, ret);
		goto l_out_buf_free;
	}

	if (copy_to_user((void __user*)out_data,
				out_data_buf + SXE_HDC_CMD_HDR_SIZE, out_len)) {
		LOG_ERROR_BDF("hw[%p] cmd trace_id=0x%llx copy to user err\n",
				hw, trace_id);
		ret = -EFAULT;
	}

l_out_buf_free:
	kfree(out_data_buf);
l_in_buf_free:
	kfree(in_data_buf);
l_ret:
	return ret;
}
