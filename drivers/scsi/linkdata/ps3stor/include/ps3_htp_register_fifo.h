/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __PS3_HTP_REGISTER_FIFO_H__
#define __PS3_HTP_REGISTER_FIFO_H__

#include "hwapi/include_v200/s1861_regs/s1861_global_baseaddr.h"
#include "hwapi/include_v200/s1861_regs/s1861_hil_reg0_ps3_request_queue_reg.h"
#include "hwapi/include_v200/s1861_regs/s1861_hil_reg0_ps3_register_f_reg.h"
#include "hwapi/include_v200/s1861_regs/s1861_hil_reg0_ps3_register_s_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

union ps3RequestFifo {
	unsigned char reserved0[HIL_REG0_PS3_REQUEST_QUEUE_SIZE];
	struct HilReg0Ps3RequestQueue request_fifo;
};

union ps3RegShare {
	unsigned char reserved0[HIL_REG0_PS3_REGISTER_S_SIZE];
	struct HilReg0Ps3RegisterS share_reg;
};

union ps3RegExclusive {
	unsigned char reserved0[HIL_REG0_PS3_REGISTER_F_SIZE];
	struct HilReg0Ps3RegisterF Excl_reg;
};

struct Ps3Fifo {
	union ps3RegExclusive reg_f;
	union ps3RequestFifo cmd_fifo;
	union ps3RegShare reg_s;
};

#ifdef __cplusplus
}
#endif

#endif
