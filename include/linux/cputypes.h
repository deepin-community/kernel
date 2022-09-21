#ifndef _LIB_CPU_TYPE_H
#define _LIB_CPU_TYPE_H

// if arch is arm64 then need to judge cpu type, otherwise return false
#ifdef CONFIG_ARM64
#include <asm/cputype.h>
#define MACHINE_TYPE_FUN_DEFS(cpu)  \
    bool cpu_is_##cpu(void);
#else
#define MACHINE_TYPE_FUN_DEFS(cpu)  \
    static bool cpu_is_##cpu(void) { return false; }
#endif

MACHINE_TYPE_FUN_DEFS(ft_d2000)
MACHINE_TYPE_FUN_DEFS(phytium)
MACHINE_TYPE_FUN_DEFS(ft2004)
MACHINE_TYPE_FUN_DEFS(kunpeng)
MACHINE_TYPE_FUN_DEFS(kunpeng_3211k)
MACHINE_TYPE_FUN_DEFS(hisi)
MACHINE_TYPE_FUN_DEFS(pangu)
MACHINE_TYPE_FUN_DEFS(kunpeng920_series)
MACHINE_TYPE_FUN_DEFS(kunpeng_920l)
MACHINE_TYPE_FUN_DEFS(kunpeng_920s)
#endif
