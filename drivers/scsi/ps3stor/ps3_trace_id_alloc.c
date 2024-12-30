#ifndef _WINDOWS

#include <linux/cpu.h>
#include <linux/percpu-defs.h>
#include <linux/percpu.h>

#include "ps3_trace_id.h"
#include "ps3_err_def.h"
#include "ps3_trace_id_alloc.h"

static DEFINE_PER_CPU(ps3_trace_id_u, ps3_trace_id);

static U8 g_ps3_trace_id_swith = ps3_trace_id_switch_open;

void ps3_trace_id_alloc(U64 *trace_id)
{
	ps3_trace_id_u *id = NULL;
	U64 trace_id_count = 0;

	if (g_ps3_trace_id_swith != ps3_trace_id_switch_open) {
		*trace_id = 0;
		goto l_out;
	}

	preempt_disable();
	id = this_cpu_ptr(&ps3_trace_id);

	trace_id_count = id->ps3_trace_id.count;
	++trace_id_count;
	id->ps3_trace_id.count = (trace_id_count & TRACE_ID_CHIP_OUT_COUNT_MASK);

	*trace_id = id->trace_id;
	preempt_enable();

l_out:
	return ;
}

void ps3_trace_id_init(void)
{
	S32 cpu = 0;
	ps3_trace_id_u *id = NULL;

	for_each_possible_cpu(cpu) {
		id = &per_cpu(ps3_trace_id, cpu);
		id->ps3_trace_id.flag = ps3_trace_id_flag_host_driver;
		id->ps3_trace_id.cpu_id = (cpu & TRACE_ID_CHIP_OUT_CPUID_MASK);
		id->ps3_trace_id.count = 0;
	}

	return;
}

S32 ps3_trace_id_switch_store(U8 value)
{
	S32 ret = PS3_SUCCESS;

	if (value != ps3_trace_id_switch_close &&
		value != ps3_trace_id_switch_open) {
		ret = PS3_FAILED;
		goto l_out;
	}

	g_ps3_trace_id_swith = value;

l_out:
	return ret;
}

U8 ps3_trace_id_switch_show(void)
{
	return g_ps3_trace_id_swith;
}
#endif
