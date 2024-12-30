
#ifndef __PS3_TRACE_ID_H__
#define __PS3_TRACE_ID_H__

#define TRACE_ID_CHIP_OUT_COUNT_MASK  0x000FFFFFFFFFFFFFLLU

#define TRACE_ID_CHIP_OUT_CPUID_SHIFT 52
#define TRACE_ID_CHIP_OUT_CPUID_MASK  0x7FFLLU

static inline void traceIdCpuIdSet(unsigned long long *traceId, unsigned short cpuId)
{
    *traceId &= ~(TRACE_ID_CHIP_OUT_CPUID_MASK << TRACE_ID_CHIP_OUT_CPUID_SHIFT);
    *traceId |= ((unsigned long long)cpuId & TRACE_ID_CHIP_OUT_CPUID_MASK) << \
        TRACE_ID_CHIP_OUT_CPUID_SHIFT;
}

#define TRACE_ID_CLI_COUNT_MASK  0x00000000000000FFLLU
#define TRACE_ID_CLI_TID_MASK    0x0000000000FFFF00LLU
#define TRACE_ID_CLI_TIME_MASK   0x00FFFFFFFF000000LLU
#define TRACE_ID_CLI_FLAG        0xFF00000000000000LLU

#define TRACE_ID_CLI_COUNT_SHIFT 0
#define TRACE_ID_CLI_TID_SHIFT   8
#define TRACE_ID_CLI_TIME_SHIFT  24
#define TRACE_ID_CLI_FLAG_SHIFT  56

#define TRACE_ID_CHIP_IN_COUNT_MASK      0x00FFFFFFFFFFFFFFLLU
#define TRACE_ID_CHIP_IN_FLAG            0x8000000000000000LLU

#define TRACE_ID_CHIP_IN_CONTEXTID_SHIFT 56
#define TRACE_ID_CHIP_IN_CONTEXTID_MASK  0x7FLLU

static inline void traceIdContextIdSet(unsigned long long *traceId, unsigned short contextId)
{
    *traceId &= ~(TRACE_ID_CHIP_IN_CONTEXTID_MASK << TRACE_ID_CHIP_IN_CONTEXTID_SHIFT);
    *traceId |= ((unsigned long long)contextId & TRACE_ID_CHIP_IN_CONTEXTID_MASK) << \
        TRACE_ID_CHIP_IN_CONTEXTID_SHIFT;
}
            
#endif
