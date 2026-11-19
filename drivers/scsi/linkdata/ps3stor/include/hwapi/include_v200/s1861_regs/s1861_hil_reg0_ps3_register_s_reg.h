/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __S1861_HIL_REG0_PS3_REGISTER_S_REG_H__
#define __S1861_HIL_REG0_PS3_REGISTER_S_REG_H__
#include "s1861_global_baseaddr.h"
#ifndef __S1861_HIL_REG0_PS3_REGISTER_S_REG_MACRO__
#define HIL_REG0_PS3_REGISTER_S_PS3_FUNCTION_LOCK_ADDR                         \
	(HIL_REG0_PS3_REGISTER_S_BASEADDR + 0x0)
#define HIL_REG0_PS3_REGISTER_S_PS3_FUNCTION_LOCK_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_S_PS3_FUNCTION_LOCK_OWNER_ADDR                   \
	(HIL_REG0_PS3_REGISTER_S_BASEADDR + 0x8)
#define HIL_REG0_PS3_REGISTER_S_PS3_FUNCTION_LOCK_OWNER_RST (0x0000000000000003)
#endif

#ifndef __S1861_HIL_REG0_PS3_REGISTER_S_REG_STRUCT__
union HilReg0Ps3RegisterSPs3FucntionLock {
	unsigned long long val;
	struct {

		unsigned long long lock : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterSPs3FunctionLockOwner {
	unsigned long long val;
	struct {

		unsigned long long display : 2;
		unsigned long long reserved1 : 62;
	} reg;
};

struct HilReg0Ps3RegisterS {

	union HilReg0Ps3RegisterSPs3FucntionLock ps3FucntionLock;
	union HilReg0Ps3RegisterSPs3FunctionLockOwner ps3FunctionLockOwner;
};
#endif
#endif
