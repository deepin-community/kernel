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

#ifndef _REG_BUSYSTATUS_ELT_H_
#define _REG_BUSYSTATUS_ELT_H_
typedef union
{
    struct
    {
        unsigned int BCI_busy_0         :1;
        unsigned int EUVS_busy_1        :1;
        unsigned int EU0PS_busy_2       :1;
        unsigned int TAS_busy_3         :1;
        unsigned int TGZ0_busy_4        :1;
        unsigned int SGVS_busy_5        :1;
        unsigned int FFC0_busy_6        :1;
        unsigned int L2_busy_7          :1;
        unsigned int IU0_busy_8         :1;
        unsigned int TU0_busy_9         :1;
        unsigned int WBU0_busy_10       :1;
        unsigned int MXU_busy_11        :1;
        unsigned int ISP_busy_12        :1;
        unsigned int VCP_busy_13        :1;
        unsigned int QTM_busy_14        :1;
        unsigned int CMG_busy_15        :1;
        unsigned int EU1PS_busy_16      :1;
        unsigned int FFC1_busy_17       :1;
        unsigned int IU1_busy_18        :1;
        unsigned int TGZ1_busy_19       :1;
        unsigned int TU1_busy_20        :1;
        unsigned int WBU1_busy_21       :1;
        unsigned int Reserved          : 10;
    };
    unsigned int uint;
} Reg_BusyStatus_elt;

#endif
