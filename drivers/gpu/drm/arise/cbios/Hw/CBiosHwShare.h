//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************


/*****************************************************************************
** DESCRIPTION:
** Include hw layer common header files.
**
** NOTE:
** This header file CAN ONLY be included by hw layer those files under Hw folder. 
******************************************************************************/

#ifndef _CBIOS_HW_SHARE_H_
#define _CBIOS_HW_SHARE_H_

//The register header files are included here
#include "Register/BIU_CR_C_BUS_registers.h"
#include "Register/BIU_HDA_registers.h"
#include "Register/BIU_MM_registers.h"
#include "Register/BIU_TSR_registers.h"
#include "Register/BIU_VCP_registers.h"
#include "Register/DIU_CR_registers.h"
#include "Register/DIU_MM_registers.h"
#include "Register/DIU_MM_registers_Arise.h"
#include "Register/DIU_SR_registers.h"
#include "Register/DIU_vga_registers.h"
#include "Register/Monitor/CBiosDPCDRegister.h"

#include "HwUtil/CBiosUtilHw.h"
#include "HwUtil/CBiosI2C.h"
#include "HwCallback/CBiosCallbacksHw.h"

#define   PARALLEL_TABLE_END_INDEX   0xdeadbeaf
#define   PARALLEL_TABLE_MAX_SIZE      100

#define   STREAM_DISABLE_FAKE_ADDR  (0xABBCC << 2)

typedef  enum  _VIP_PARALLEL_REG_INDEX
{
    VIP_REG0    = 0,
    VIP_REG1,
    VIP_REG2,
    VIP_REG3,
    VIP_REG4,
    VIP_REG5,
    VIP_REG6,
    VIP_REG7,
    VIP_REG8,
    VIP_REG9,
    VIP_REG10,
    VIP_REG11,
    VIP_REG12,
}VIP_PARALLEL_REG_INDEX, *PVIP_PARALLEL_REG_INDEX;

typedef  enum  _VIP_IIC_PARALLEL_REG_INDEX
{
    VIP_IIC_RW_DATA_REG   = 0,
    VIP_IIC_CONTROL_REG,
    VIP_IIC_IE,
}VIP_IIC_PARALLEL_REG_INDEX, *PVIP_IIC_PARALLEL_REG_INDEX;


typedef union _CEC_MISC_REG1
{
    struct  
    {
        CBIOS_U32   IniRetryCnt     :3;         // CEC Initiator Retry times. Valid values must be 1 to 5.
        CBIOS_U32   DeviceAddr      :4;         // CEC Initiator/Follower device address
        CBIOS_U32   IniDestAddr     :4;         // CEC Initiator command destination address
        CBIOS_U32   IniCmdLen       :5;         // CEC Initiator command length. The valid value is [0:16]
        CBIOS_U32   IniBroadcast    :1;         // = TRUE: broadcast; = FALSE: direct access  
        CBIOS_U32   RsvdBit17       :1;
        CBIOS_U32   CECEnable       :1;         // = CBIOS_TRUE: enable CEC; = CBIOS_FALSE: Disable CEC
        CBIOS_U32   IniCmdAvailable :1;         // = 0: NOT available; = 1: CEC Initiator commands have been loaded into register. 
        CBIOS_U32   RsvdBit20_31    :12;

    };
    CBIOS_U32   CECMiscReg1Value;
}CEC_MISC_REG1;

typedef union _CEC_MISC_REG2
{
    struct
    {
        CBIOS_U32   IniTransFinish  :1;         // CEC Initiator has finished the transmission.
        CBIOS_U32   IniTransSucceed :1;         // CEC Initiator transmission has succeeded. 
        CBIOS_U32   RsvdBit2        :1;
        CBIOS_U32   FolSrcAddr      :4;         // The source address received by CEC Follower.
        CBIOS_U32   FolBroadcast    :1;         // Transaction type received by CEC Follower.= TRUE: broadcast; = FALSE: direct access  
        CBIOS_U32   FolCmdLen       :5;         // Command length received by CEC Follower. A valid value is between 0 and 16
        CBIOS_U32   FolReceiveReady :1;         // Follower has successfully received a transaction. 
        CBIOS_U32   RsvdBit14_31    :18;
    };
    CBIOS_U32   CECMiscReg2Value;
}CEC_MISC_REG2;

typedef struct _CBIOS_REGISTER_RANGE
{
    CBIOS_REGISTER_BLOCK_TYPE RegType;
    CBIOS_U8 RegLen;
    CBIOS_U16 StartIndex;
    CBIOS_U16 EndIndex;
}CBIOS_REGISTER_RANGE, *PCBIOS_REGISTER_RANGE;

typedef struct _CBIOS_REGISTER_GROUP
{
    CBIOS_UCHAR GroupName[10];
    PCBIOS_REGISTER_RANGE pRegRange;
    CBIOS_U32 RangeNum;
}CBIOS_REGISTER_GROUP, *PCBIOS_REGISTER_GROUP;
#endif /* _CBIOS_HW_SHARE_H_ */

