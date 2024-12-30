#ifndef __S1861_HIL_REG0_PS3_REGISTER_S_REG_H__ 
#define __S1861_HIL_REG0_PS3_REGISTER_S_REG_H__ 
#include "s1861_global_baseaddr.h"
#ifndef __S1861_HIL_REG0_PS3_REGISTER_S_REG_MACRO__
#define HIL_REG0_PS3_REGISTER_S_PS3_FUCNTION_LOCK_ADDR   (HIL_REG0_PS3_REGISTER_S_BASEADDR + 0x0)
#define HIL_REG0_PS3_REGISTER_S_PS3_FUCNTION_LOCK_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_S_PS3_FUNCTION_LOCK_OWNER_ADDR   (HIL_REG0_PS3_REGISTER_S_BASEADDR + 0x8)
#define HIL_REG0_PS3_REGISTER_S_PS3_FUNCTION_LOCK_OWNER_RST   (0x0000000000000003)
#endif

#ifndef __S1861_HIL_REG0_PS3_REGISTER_S_REG_STRUCT__ 
typedef union HilReg0Ps3RegisterSPs3FucntionLock{

    volatile U64 val;
    struct{

        U64 lock                           : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterSPs3FucntionLock_u;

typedef union HilReg0Ps3RegisterSPs3FunctionLockOwner{

    volatile U64 val;
    struct{

        U64 display                        : 2;   
        U64 reserved1                      : 62;  
    }reg;
}HilReg0Ps3RegisterSPs3FunctionLockOwner_u;

typedef struct HilReg0Ps3RegisterS{

    HilReg0Ps3RegisterSPs3FucntionLock_u       ps3FucntionLock;              
    HilReg0Ps3RegisterSPs3FunctionLockOwner_u   ps3FunctionLockOwner;         
}HilReg0Ps3RegisterS_s;
#endif
#endif