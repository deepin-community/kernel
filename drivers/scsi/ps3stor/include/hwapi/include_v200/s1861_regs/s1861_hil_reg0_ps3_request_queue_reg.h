#ifndef __S1861_HIL_REG0_PS3_REQUEST_QUEUE_REG_H__ 
#define __S1861_HIL_REG0_PS3_REQUEST_QUEUE_REG_H__ 
#include "s1861_global_baseaddr.h"
#ifndef __S1861_HIL_REG0_PS3_REQUEST_QUEUE_REG_MACRO__
#define HIL_REG0_PS3_REQUEST_QUEUE_PS3_REQUEST_QUEUE_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x0)
#define HIL_REG0_PS3_REQUEST_QUEUE_PS3_REQUEST_QUEUE_RST   (0xFFFFFFFFFFFFFFFF)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOERRCNT_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOERRCNT_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x10)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_RST   (0x0000000C1FFF0000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELCONFIG_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x18)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELCONFIG_RST   (0x0000000300000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFORST_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x20)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFORST_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOIOCNT_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x28)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOIOCNT_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOFLOWCNT_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x30)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOFLOWCNT_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_STATUS_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x38)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_STATUS_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_SET_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x40)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_SET_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_CLR_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x48)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_CLR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_MASK_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x50)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_INT_MASK_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_CNT_CLR_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x58)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_CNT_CLR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOORDERERROR_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x60)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOORDERERROR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFODINSHIFT_ADDR(_n)   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x68 + (_n)*0x8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFODINSHIFT_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFODOUTSHIFT_ADDR(_n)   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x88 + (_n)*0x8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFODOUTSHIFT_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_MAXLEVEL_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xa8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_MAXLEVEL_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOINIT_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xb0)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOINIT_RST   (0x0000000000000002)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOINIT_EN_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xb8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOINIT_EN_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOINIT_MAX_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xc0)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOINIT_MAX_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_ECC_CNT_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xc8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_ECC_CNT_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_ECC_ADDR_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xd0)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOSTATUS_ECC_ADDR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_DECODER_OVERFLOW_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xd8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_DECODER_OVERFLOW_RST   (0x000000000000003F)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_ECC_BAD_PROJECT_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xe0)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFO_ECC_BAD_PROJECT_RST   (0x0000000000000001)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOOVERFLOW_WORD_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xe8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOOVERFLOW_WORD_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORCTL_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xf0)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORCTL_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORCNTCLR_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0xf8)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORCNTCLR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORLOW_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x100)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORLOW_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORMID_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x108)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORMID_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORHIGH_ADDR   (HIL_REG0_PS3_REQUEST_QUEUE_BASEADDR + 0x110)
#define HIL_REG0_PS3_REQUEST_QUEUE_FIFOLEVELMONITORHIGH_RST   (0x0000000000000000)
#endif

#ifndef __S1861_HIL_REG0_PS3_REQUEST_QUEUE_REG_STRUCT__ 
typedef union HilReg0Ps3RequestQueuePs3RequestQueue{

    volatile U64 val;
    struct{

        U64 port                           : 64;  
    }reg;
}HilReg0Ps3RequestQueuePs3RequestQueue_u;

typedef union HilReg0Ps3RequestQueueFifoErrCnt{

    volatile U64 val;
    struct{

        U64 waddrerr                       : 32;  
        U64 reserved1                      : 32;  
    }reg;
}HilReg0Ps3RequestQueueFifoErrCnt_u;

typedef union HilReg0Ps3RequestQueueFifoStatus{

    volatile U64 val;
    struct{

        U64 filled                         : 16;  
        U64 fifoDepth                      : 16;  
        U64 almostfull                     : 1;   
        U64 full                           : 1;   
        U64 almostempty                    : 1;   
        U64 empty                          : 1;   
        U64 reserved6                      : 28;  
    }reg;
}HilReg0Ps3RequestQueueFifoStatus_u;

typedef union HilReg0Ps3RequestQueueFifoLevelConfig{

    volatile U64 val;
    struct{

        U64 cfgAempty                      : 16;  
        U64 cfgAfull                       : 16;  
        U64 emptyProtect                   : 1;   
        U64 fullProtect                    : 1;   
        U64 reserved4                      : 30;  
    }reg;
}HilReg0Ps3RequestQueueFifoLevelConfig_u;

typedef union HilReg0Ps3RequestQueueFifoRst{

    volatile U64 val;
    struct{

        U64 resetPls                       : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RequestQueueFifoRst_u;

typedef union HilReg0Ps3RequestQueueFifoIOCnt{

    volatile U64 val;
    struct{

        U64 wr                             : 32;  
        U64 rd                             : 32;  
    }reg;
}HilReg0Ps3RequestQueueFifoIOCnt_u;

typedef union HilReg0Ps3RequestQueueFifoFlowCnt{

    volatile U64 val;
    struct{

        U64 overflow                       : 32;  
        U64 underflow                      : 32;  
    }reg;
}HilReg0Ps3RequestQueueFifoFlowCnt_u;

typedef union HilReg0Ps3RequestQueueFifoIntStatus{

    volatile U64 val;
    struct{

        U64 overflowStatus                 : 1;   
        U64 underflowStatus                : 1;   
        U64 nemptyStatus                   : 1;   
        U64 eccBadStatus                   : 1;   
        U64 reserved4                      : 60;  
    }reg;
}HilReg0Ps3RequestQueueFifoIntStatus_u;

typedef union HilReg0Ps3RequestQueueFifoIntSet{

    volatile U64 val;
    struct{

        U64 overflowSet                    : 1;   
        U64 underflowSet                   : 1;   
        U64 nemptySet                      : 1;   
        U64 eccBadSet                      : 1;   
        U64 reserved4                      : 60;  
    }reg;
}HilReg0Ps3RequestQueueFifoIntSet_u;

typedef union HilReg0Ps3RequestQueueFifoIntClr{

    volatile U64 val;
    struct{

        U64 overflowClr                    : 1;   
        U64 underflowClr                   : 1;   
        U64 nemptyClr                      : 1;   
        U64 eccBadClr                      : 1;   
        U64 reserved4                      : 60;  
    }reg;
}HilReg0Ps3RequestQueueFifoIntClr_u;

typedef union HilReg0Ps3RequestQueueFifoIntMask{

    volatile U64 val;
    struct{

        U64 overflowMask                   : 1;   
        U64 underflowMask                  : 1;   
        U64 nemptyMask                     : 1;   
        U64 eccBadMask                     : 1;   
        U64 reserved4                      : 60;  
    }reg;
}HilReg0Ps3RequestQueueFifoIntMask_u;

typedef union HilReg0Ps3RequestQueueFifoCntClr{

    volatile U64 val;
    struct{

        U64 fifowrcntClr                   : 1;   
        U64 fifordcntClr                   : 1;   
        U64 fifoerrcntClr                  : 1;   
        U64 fifoordererrwrcntClr           : 1;   
        U64 fifoordererrrdcntClr           : 1;   
        U64 fifobit1errcntClr              : 1;   
        U64 fifobit2errcntClr              : 1;   
        U64 reserved7                      : 57;  
    }reg;
}HilReg0Ps3RequestQueueFifoCntClr_u;

typedef union HilReg0Ps3RequestQueueFifoOrderError{

    volatile U64 val;
    struct{

        U64 wrcnt                          : 32;  
        U64 rdcnt                          : 32;  
    }reg;
}HilReg0Ps3RequestQueueFifoOrderError_u;

typedef union HilReg0Ps3RequestQueueFifoDinShift{

    volatile U64 val;
    struct{

        U64 val                            : 64;  
    }reg;
}HilReg0Ps3RequestQueueFifoDinShift_u;

typedef union HilReg0Ps3RequestQueueFifoDoutShift{

    volatile U64 val;
    struct{

        U64 val                            : 64;  
    }reg;
}HilReg0Ps3RequestQueueFifoDoutShift_u;

typedef union HilReg0Ps3RequestQueueFifostatusMaxlevel{

    volatile U64 val;
    struct{

        U64 val                            : 16;  
        U64 reserved1                      : 48;  
    }reg;
}HilReg0Ps3RequestQueueFifostatusMaxlevel_u;

typedef union HilReg0Ps3RequestQueueFifoInit{

    volatile U64 val;
    struct{

        U64 stat                           : 2;   
        U64 reserved1                      : 62;  
    }reg;
}HilReg0Ps3RequestQueueFifoInit_u;

typedef union HilReg0Ps3RequestQueueFifoinitEn{

    volatile U64 val;
    struct{

        U64 start                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RequestQueueFifoinitEn_u;

typedef union HilReg0Ps3RequestQueueFifoinitMax{

    volatile U64 val;
    struct{

        U64 num                            : 16;  
        U64 reserved1                      : 48;  
    }reg;
}HilReg0Ps3RequestQueueFifoinitMax_u;

typedef union HilReg0Ps3RequestQueueFifostatusEccCnt{

    volatile U64 val;
    struct{

        U64 bit1Err                        : 32;  
        U64 bit2Err                        : 32;  
    }reg;
}HilReg0Ps3RequestQueueFifostatusEccCnt_u;

typedef union HilReg0Ps3RequestQueueFifostatusEccAddr{

    volatile U64 val;
    struct{

        U64 errPoint                       : 64;  
    }reg;
}HilReg0Ps3RequestQueueFifostatusEccAddr_u;

typedef union HilReg0Ps3RequestQueueFifoDecoderOverflow{

    volatile U64 val;
    struct{

        U64 rCmdwordEmpty                  : 1;   
        U64 rPortindexEmpty                : 1;   
        U64 rCmdbackEmpty                  : 1;   
        U64 wCmdwordEmpty                  : 1;   
        U64 wPortindexEmpty                : 1;   
        U64 wCmdbackEmpty                  : 1;   
        U64 rCmdwordFull                   : 1;   
        U64 rPortindexFull                 : 1;   
        U64 rCmdbackFull                   : 1;   
        U64 wCmdwordFull                   : 1;   
        U64 wPortindexFull                 : 1;   
        U64 wCmdbackFull                   : 1;   
        U64 reserved12                     : 52;  
    }reg;
}HilReg0Ps3RequestQueueFifoDecoderOverflow_u;

typedef union HilReg0Ps3RequestQueueFifoEccBadProject{

    volatile U64 val;
    struct{

        U64 en                             : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RequestQueueFifoEccBadProject_u;

typedef union HilReg0Ps3RequestQueueFifooverflowWord{

    volatile U64 val;
    struct{

        U64 record                         : 64;  
    }reg;
}HilReg0Ps3RequestQueueFifooverflowWord_u;

typedef union HilReg0Ps3RequestQueueFifoLevelMonitorCtl{

    volatile U64 val;
    struct{

        U64 low                            : 16;  
        U64 high                           : 16;  
        U64 en                             : 1;   
        U64 reserved3                      : 31;  
    }reg;
}HilReg0Ps3RequestQueueFifoLevelMonitorCtl_u;

typedef union HilReg0Ps3RequestQueueFifoLevelMonitorCntClr{

    volatile U64 val;
    struct{

        U64 en                             : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RequestQueueFifoLevelMonitorCntClr_u;

typedef union HilReg0Ps3RequestQueueFifoLevelMonitorLow{

    volatile U64 val;
    struct{

        U64 cnt                            : 64;  
    }reg;
}HilReg0Ps3RequestQueueFifoLevelMonitorLow_u;

typedef union HilReg0Ps3RequestQueueFifoLevelMonitorMid{

    volatile U64 val;
    struct{

        U64 cnt                            : 64;  
    }reg;
}HilReg0Ps3RequestQueueFifoLevelMonitorMid_u;

typedef union HilReg0Ps3RequestQueueFifoLevelMonitorHigh{

    volatile U64 val;
    struct{

        U64 cnt                            : 64;  
    }reg;
}HilReg0Ps3RequestQueueFifoLevelMonitorHigh_u;

typedef struct HilReg0Ps3RequestQueue{

    HilReg0Ps3RequestQueuePs3RequestQueue_u    ps3RequestQueue;              
    HilReg0Ps3RequestQueueFifoErrCnt_u         fifoErrCnt;                   
    HilReg0Ps3RequestQueueFifoStatus_u         fifoStatus;                   
    HilReg0Ps3RequestQueueFifoLevelConfig_u    fifoLevelConfig;              
    HilReg0Ps3RequestQueueFifoRst_u            fifoRst;                      
    HilReg0Ps3RequestQueueFifoIOCnt_u          fifoIOCnt;                    
    HilReg0Ps3RequestQueueFifoFlowCnt_u        fifoFlowCnt;                  
    HilReg0Ps3RequestQueueFifoIntStatus_u      fifoIntStatus;                
    HilReg0Ps3RequestQueueFifoIntSet_u         fifoIntSet;                   
    HilReg0Ps3RequestQueueFifoIntClr_u         fifoIntClr;                   
    HilReg0Ps3RequestQueueFifoIntMask_u        fifoIntMask;                  
    HilReg0Ps3RequestQueueFifoCntClr_u         fifoCntClr;                   
    HilReg0Ps3RequestQueueFifoOrderError_u     fifoOrderError;               
    HilReg0Ps3RequestQueueFifoDinShift_u       fifoDinShift[4];              
    HilReg0Ps3RequestQueueFifoDoutShift_u      fifoDoutShift[4];             
    HilReg0Ps3RequestQueueFifostatusMaxlevel_u   fifoStatusMaxLevel;           
    HilReg0Ps3RequestQueueFifoInit_u           fifoInit;                     
    HilReg0Ps3RequestQueueFifoinitEn_u         fifoinitEn;                   
    HilReg0Ps3RequestQueueFifoinitMax_u        fifoinitMax;                  
    HilReg0Ps3RequestQueueFifostatusEccCnt_u   fifoStatusEccCnt;             
    HilReg0Ps3RequestQueueFifostatusEccAddr_u   fifoStatusEccAddr;            
    HilReg0Ps3RequestQueueFifoDecoderOverflow_u   fifoDecoderOverflow;          
    HilReg0Ps3RequestQueueFifoEccBadProject_u   fifoEccBadProject;            
    HilReg0Ps3RequestQueueFifooverflowWord_u   fifoOverFlowWord;             
    HilReg0Ps3RequestQueueFifoLevelMonitorCtl_u   fifoLevelMonitorCtl;          
    HilReg0Ps3RequestQueueFifoLevelMonitorCntClr_u   fifoLevelMonitorCntClr;       
    HilReg0Ps3RequestQueueFifoLevelMonitorLow_u   fifoLevelMonitorLow;          
    HilReg0Ps3RequestQueueFifoLevelMonitorMid_u   fifoLevelMonitorMid;          
    HilReg0Ps3RequestQueueFifoLevelMonitorHigh_u   fifoLevelMonitorHigh;         
}HilReg0Ps3RequestQueue_s;
#endif
#endif