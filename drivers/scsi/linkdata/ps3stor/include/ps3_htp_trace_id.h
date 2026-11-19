/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __PS3_HTP_TRACE_ID_H__
#define __PS3_HTP_TRACE_ID_H__

#define TRACE_ID_CHIP_OUT_COUNT_MASK 0x000FFFFFFFFFFFFFLLU

#define TRACE_ID_CHIP_OUT_CPUID_SHIFT 52
#define TRACE_ID_CHIP_OUT_CPUID_MASK 0x7FFLLU

static inline void traceIdCpuIdSet(unsigned long long *traceId,
			unsigned short cpuId)
{
	*traceId &= ~(TRACE_ID_CHIP_OUT_CPUID_MASK
		<< TRACE_ID_CHIP_OUT_CPUID_SHIFT);
	*traceId |= ((unsigned long long)cpuId & TRACE_ID_CHIP_OUT_CPUID_MASK)
		<< TRACE_ID_CHIP_OUT_CPUID_SHIFT;
}

#endif
