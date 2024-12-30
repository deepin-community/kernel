
#ifndef __PS3_REGISTER_FIFO_H__
#define __PS3_REGISTER_FIFO_H__

#include "hwapi/include_v200/s1861_regs/s1861_global_baseaddr.h"
#include "hwapi/include_v200/s1861_regs/s1861_hil_reg0_ps3_request_queue_reg.h"
#include "hwapi/include_v200/s1861_regs/s1861_hil_reg0_ps3_register_f_reg.h"
#include "hwapi/include_v200/s1861_regs/s1861_hil_reg0_ps3_register_s_reg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef union ps3RequestFifo {
    U8 reserved0[HIL_REG0_PS3_REQUEST_QUEUE_SIZE];
    HilReg0Ps3RequestQueue_s request_fifo;
} ps3RequestFifo_u;

typedef union ps3RegShare{
    U8 reserved0[HIL_REG0_PS3_REGISTER_S_SIZE];
    HilReg0Ps3RegisterS_s share_reg;
} ps3RegShare_u;

typedef union ps3RegExclusive{
    U8 reserved0[HIL_REG0_PS3_REGISTER_F_SIZE];
    HilReg0Ps3RegisterF_s Excl_reg;
} ps3RegExclusive_u;

typedef struct Ps3Fifo{
    ps3RegExclusive_u reg_f; 
    ps3RequestFifo_u cmd_fifo;
    ps3RegShare_u reg_s;  
} Ps3Fifo_s;

#ifdef __cplusplus
}
#endif

#endif
