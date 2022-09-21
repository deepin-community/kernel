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

#ifndef _VPP_MMIO_REGISTERSHELPER_H
#define _VPP_MMIO_REGISTERSHELPER_H
#include <assert.h>
#include <string.h>
#include "VPP_MMIO_registers.h"


enum{
   GB_FD_MODE,
   GB_BLS_GAIN,
   RESERVED_M4,
   VPP_MMIO_REG_ENUM_END
};
static inline int Vpp_Mmio_write_reg(Vpp_Mmio_regs * G_regs, int reg, int index , int value)
{
   switch(reg)
   {
       case GB_FD_MODE:
           assert(index < 4);
           G_regs->reg_M4_Reg[index].reg.Gb_Fd_Mode= value;
           return 1;
       case GB_BLS_GAIN:
           assert(index < 4);
           G_regs->reg_M4_Reg[index].reg.Gb_Bls_Gain= value;
           return 1;
       case RESERVED_M4:
           assert(index < 4);
           G_regs->reg_M4_Reg[index].reg.Reserved_M4= value;
           return 1;
   }
   return 0;
}


static inline unsigned int Vpp_Mmio_read_reg(Vpp_Mmio_regs * G_regs, int reg, int index)
{
   switch(reg)
   {
       case GB_FD_MODE:
           assert(index < 4);
           return G_regs->reg_M4_Reg[index].reg.Gb_Fd_Mode;
       case GB_BLS_GAIN:
           assert(index < 4);
           return G_regs->reg_M4_Reg[index].reg.Gb_Bls_Gain;
       case RESERVED_M4:
           assert(index < 4);
           return G_regs->reg_M4_Reg[index].reg.Reserved_M4;
   }
   assert(0);
   return 0;
}


static inline int Vpp_Mmio_get_reg_index(char * name)
{


   {
       if(!strcmp( "gb_fd_mode", name))
           return GB_FD_MODE;
       if(!strcmp( "gb_bls_gain", name))
           return GB_BLS_GAIN;
       if(!strcmp( "reserved_m4", name))
           return RESERVED_M4;
   }
   assert(0);
   return 0;
}


static inline unsigned int Vpp_Mmio_get_reg_name(int reg, char * name)
{

   switch(reg)
   {
       case GB_FD_MODE:
           strcpy(name,"gb_fd_mode");
           return 1;
       case GB_BLS_GAIN:
           strcpy(name,"gb_bls_gain");
           return 1;
       case RESERVED_M4:
           strcpy(name,"reserved_m4");
           return 1;
   }
   assert(0);
   return 0;
}


#endif
