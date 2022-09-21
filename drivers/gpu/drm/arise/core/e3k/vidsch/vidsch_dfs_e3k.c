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

#include "vidsch_dfs_e3k.h"
#include "vidsch.h"
#include "vidschi.h"
#include "chip_include_e3k.h"
#include "vidsch_engine_e3k.h"
#include "register_e3k.h"

#define GFX_INDEX_TABLE_SIZE      (10)
#define VIDEO_INDEX_TABLE_SIZE    (15)

#define DEFAULT_GFX_INDEX         (2)
#define DEFAULT_VIDEO_INDEX       (1)

#define DFS_TASK_THREHOLD_GFX     (2)
#define DFS_TASK_THREHOLD_VIDEO   (2)

#define GFX_ENGINE_MASK (1 << RB_INDEX_GFXL | 1 << RB_INDEX_GFXH)

void vidsch_query_engine_task_status_e3k(adapter_t *adapter, unsigned int *busy_engines, unsigned int *hw_pending_tasks,  unsigned int *pool_pending_tasks);

/*****************************************************************
E/VPLL:

1)  Fref = Fin/NR= Fin/(DIV_N)  //DIV_N is input divider, tie to 0, means DIV_N =1

2) Fvco = Fref(A+2 + B/2^20)  //A=DIV_M, feedback; B=DIV_N 

3) Fout = Fvco/NO = Fvco/(2^od) //od is output divider.

*****************************************************************/

//gpu(gfx) dfs table, divided from epll.
static const elite3000_dfs_table dfs_table_gfx[GFX_INDEX_TABLE_SIZE] =
{
                        // _idx,    _freq,    _freq_r,   _bp,  _od,  _bs,  _r,          _f,   _d) 
    ELITE3000_DFS(    0,      300,          297,      0,     2,      0,     0,     0x28,    1),		
    ELITE3000_DFS(    1,      350,          344,      0,     2,      0,     0,     0x2f,    1),
    ELITE3000_DFS(    2,      400,          398,      0,     2,      1,     0,     0x37,    1),
    ELITE3000_DFS(    3,      700,          688,      0,     1,      0,     0,     0x30,    1), 
    ELITE3000_DFS(    4,      750,          742,      0,     1,      1,     0,     0x34,    1),     
    ELITE3000_DFS(    5,      800,          796,      0,     1,      1,     0,     0x38,    1),      
    ELITE3000_DFS(    6,      850,          850,      0,     1,      1,     0,     0x3c,    1),          
    ELITE3000_DFS(    7,      900,          904,      0,     1,      1,     0,     0x40,    1),       
    ELITE3000_DFS(    8,      950,          945,      0,     1,      1,     0,     0x43,    1),       
    ELITE3000_DFS(    9,     1000,          999,      0,     1,      1,     0,     0x47,    1),           
};

/*
 *To siplify control logic, each level of vcp and vpp setting should be same, namely, real freq should be same.
 *Why? Since vcp & vpp both come from VPLL, if config vcp to Freq_A which will affect vpp current Freq_B.
 *A compromise way is both changing vcp and vpp to same freq at the same time.
 */

//gfvd dfs table, divided from vpll
static const elite3000_dfs_table dfs_table_gfvd[VIDEO_INDEX_TABLE_SIZE] =
{
                        // _idx,    _freq,    _freq_r,   _bp,  _od,  _bs,  _r,          _f,   _d) 
    ELITE3000_DFS(    0,      300,          297,      0,     2,      0,     0,     0x2a,    1),        		
    ELITE3000_DFS(    1,      350,          344,      0,     2,      0,     0,     0x31,    1),    
    ELITE3000_DFS(    2,      400,          398,      0,     2,      1,     0,     0x39,    1),    
    ELITE3000_DFS(    3,      700,          688,      0,     1,      0,     0,     0x31,    1),   
    ELITE3000_DFS(    4,      750,          742,      0,     1,      1,     0,     0x35,    1),     
    ELITE3000_DFS(    5,      800,          796,      0,     1,      1,     0,     0x39,    1),      
    ELITE3000_DFS(    6,      850,          850,      0,     1,      1,     0,     0x3d,    1),          
    ELITE3000_DFS(    7,      900,          904,      0,     1,      1,     0,     0x41,    1),          
    ELITE3000_DFS(    8,      950,          945,      0,     1,      1,     0,     0x44,    1),       
    ELITE3000_DFS(    9,     1000,          999,      0,     1,      1,     0,     0x48,    1),           
    ELITE3000_DFS(   10,     1100,         1093,      0,     1,      1,     0,     0x4f,    1),           
    ELITE3000_DFS(   11,     1200,         1188,      0,     1,      1,     0,     0x56,    1),           
    ELITE3000_DFS(   12,     1300,         1296,      0,     0,      1,     0,     0x2e,    1), 
    ELITE3000_DFS(   13,     1400,         1404,      0,     0,      1,     0,     0x32,    1),           
    ELITE3000_DFS(   14,     1500,         1485,      0,     0,      1,     0,     0x35,    1),           
};

//vpp dfs table, divided from vpll
static const elite3000_dfs_table dfs_table_vpp[VIDEO_INDEX_TABLE_SIZE] =
{
                        // _idx,    _freq,    _freq_r,   _bp,  _od,  _bs,  _r,          _f,   _d) 
    ELITE3000_DFS(    0,      300,          297,      0,     2,      0,     0,     0x2a,    2),        	
    ELITE3000_DFS(    1,      350,          344,      0,     2,      0,     0,     0x31,    2),    
    ELITE3000_DFS(    2,      400,          398,      0,     2,      1,     0,     0x39,    2),    
    ELITE3000_DFS(    3,      700,          688,      0,     1,      0,     0,     0x31,    2),    
    ELITE3000_DFS(    4,      750,          742,      0,     1,      1,     0,     0x35,    2),     
    ELITE3000_DFS(    5,      800,          796,      0,     1,      1,     0,     0x39,    2),      
    ELITE3000_DFS(    6,      850,          850,      0,     1,      1,     0,     0x3d,    2),          
    ELITE3000_DFS(    7,      900,          904,      0,     1,      1,     0,     0x41,    2),          
    ELITE3000_DFS(    8,      950,          945,      0,     1,      1,     0,     0x44,    2),       
    ELITE3000_DFS(    9,     1000,          999,      0,     1,      1,     0,     0x48,    2),           
    ELITE3000_DFS(   10,     1100,         1093,      0,     1,      1,     0,     0x4f,    2),           
    ELITE3000_DFS(   11,     1200,         1188,      0,     1,      1,     0,     0x56,    2),           
    ELITE3000_DFS(   12,     1300,         1296,      0,     0,      1,     0,     0x2e,    2), 
    ELITE3000_DFS(   13,     1400,         1404,      0,     0,      1,     0,     0x32,    2),           
    ELITE3000_DFS(   14,     1500,         1485,      0,     0,      1,     0,     0x35,    2),           
};

 /*
  * Note:
  *  PCLK IN IS FIXED, we can ONLY change PCLK divider for each module.
  *  We use DEFAULT pclk value as parking value, or we can program divider to lower parking clk.
  *  BUT DIFF PCIE GEN HAS DIFF DEFAULT PCLK, SO WE NEED SET DIFF DIVIDER ACCORDING TO PCIE GEN.
  *  PMU_SW_'X'_PVAL_ADDR[4]   : 0 switch to working clk src, 1 switch to parking clk src 
  *  PMU_SW_'X'_PVAL_ADDR[3:0] : divider of parking clk .
  *                                                    0000->2/3*PCLK;
  *                                                    0001->PCLK; 
  *                                                    0010->1/2*PCLK; //default
  *                                                    0011->1/3*PCLK;
  *                                                    ....
  *                                                    1111->1/64*PCLK
  *  'X' should be:
  *         C3D: 0x80050
  *         VCP: 0x80054
  *         VPP: 0x80058
  */    
void vidsch_parking_to_PCLK_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr, unsigned int to_parking)
{
    unsigned int engine_idx = sch_mgr->engine_index;
    unsigned char reg_value = 0;
    unsigned char pcie_gen = 0;    
    unsigned int  offset = 0;
    unsigned char onPCLK = 0;
    unsigned char temp = 0;

    switch(engine_idx)
    {
         case RB_INDEX_GFXH:
         case RB_INDEX_GFXL:            
                 offset = 0;
                 break;
         case RB_INDEX_VCP0:
         case RB_INDEX_VCP1:
                 offset = 1;
                 break;
         case RB_INDEX_VPP:           
                 offset = 2;
                 break;
         default:
                return;
    }

    //0 switch to working clk src, 1 switch to parking clk src
    if(to_parking)
    {
        //program divider according diff pcie generation.
        
        pcie_gen = gf_read8(adapter->mmio + 0x8072);
        reg_value = 1 << (pcie_gen -1);
        
        gf_write8(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_C3D_PVAL_ADDR_OFFSET + offset * 4, reg_value);
        gf_udelay(5);

        reg_value |= (1 << 4); //to parking
        gf_write8(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_C3D_PVAL_ADDR_OFFSET + offset * 4, reg_value);        


        temp=gf_read8(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_C3D_VAL_ADDR_OFFSET + offset * 4);

        do{
           gf_udelay(5);
           onPCLK = gf_read8(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_C3D_VAL_ADDR_OFFSET + offset * 4);      
        }while(!(onPCLK & 0x10));//to park clk stable or not.
    }
    else
    {
        reg_value = gf_read8(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_C3D_PVAL_ADDR_OFFSET + offset * 4);
        gf_write8(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_C3D_PVAL_ADDR_OFFSET + offset * 4, reg_value & 0xEF);

        temp=gf_read8(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_C3D_VAL_ADDR_OFFSET + offset * 4);

        do
        {
           gf_udelay(5);
           onPCLK = gf_read8(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_C3D_VAL_ADDR_OFFSET + offset * 4);      
        }while(onPCLK & 0x10);
    }

}

/*
 * PMU_SW_'X'_VAL_ADDR[3:0]:divider of working clk
 *                                      0000: 2/3*PLL
 *                                      0001: PLL
 *                                      0010: 1/2*PLL
 *                                      0011: 1/3*PLL
 *                                      0100: 2/4*PLL 
 *                                      .........
 * 'X' should be C3D/VCP/VPP
 * Note: PLL should be EPLL for C3D, or VPLL for VCP/VPP
 */
 //actually we need read the divider to confirm set success or not.
void vidsch_program_divider_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr, unsigned int index)
{
    unsigned int engine_idx = sch_mgr->engine_index;
    unsigned char reg_value = 0;
    
    switch(engine_idx)
    {
         case RB_INDEX_GFXH:
         case RB_INDEX_GFXL:                        
             reg_value = gf_read8(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_C3D_VAL_ADDR_OFFSET);
             
             reg_value = (reg_value & ~0xF) | dfs_table_gfx[index].d;
             gf_write8(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_C3D_VAL_ADDR_OFFSET, reg_value);
             gf_udelay(10);
             break;
         case RB_INDEX_VCP0:
         case RB_INDEX_VCP1:
             reg_value = gf_read8(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_C3D_VAL_ADDR_OFFSET + 4);
             
             reg_value = (reg_value & ~0xF) | dfs_table_gfvd[index].d;
             gf_write8(adapter->mmio +  MMIO_PMU_START_ADDRESS + PMU_SW_C3D_VAL_ADDR_OFFSET + 4, reg_value);
             gf_udelay(10);
             break;
         case RB_INDEX_VPP:           
             reg_value = gf_read8(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_C3D_VAL_ADDR_OFFSET + 8);
             
             reg_value = (reg_value & ~0xF) | dfs_table_vpp[index].d;
             gf_write8(adapter->mmio +  MMIO_PMU_START_ADDRESS + PMU_SW_C3D_VAL_ADDR_OFFSET + 8, reg_value);
             gf_udelay(10);             
             break;
        default:
             return;
    }

}

//ONLY for EPLL/VPLL program, divider setting for 3D/VCP/VPP is vidsch_program_divider_e3k()
void vidsch_program_PLL_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr, unsigned int index)
{
    unsigned int engine_idx = sch_mgr->engine_index;
    unsigned int locked = 0;
    Reg_Pll_load Reg_Pll_load={0};

    switch(engine_idx)
    {
         //3D -> EPLL
         case RB_INDEX_GFXH:
         case RB_INDEX_GFXL:            
            {  
                write_reg_e3k(adapter->mmio, CR, 0xE2, (1<<2 | 1<<0), ~(1<<2 | 1<<0)); 

                //program reg setting
                write_reg_e3k(adapter->mmio, CR, 0xDD, ((dfs_table_gfx[index].bs) | (dfs_table_gfx[index].od << 1) ), ~0x7);
                write_reg_e3k(adapter->mmio, CR, 0xDE,  ((dfs_table_gfx[index].f >>7)<<4), 0xEF);
                write_reg_e3k(adapter->mmio, CR, 0xDF, dfs_table_gfx[index].f, 0x80);

                write_reg_e3k(adapter->mmio, CR, 0xE2, (1<<1), ~(1<<1)); 
                gf_udelay(25);
                write_reg_e3k(adapter->mmio, CR, 0xE2, 0, ~(3 << 1)); 
                gf_udelay(5);
                
                //check lock stable or not
                while(!(locked &(1<<EPLL_LOCK)))
                {
                     locked = gf_read32(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_PLL_LOCK_ADDR);
                     gf_udelay(5);
                }
                //power_trace("program cr df:%x, read:%x\n", dfs_table_gfx[index].f, read_reg_e3k(adapter->mmio , CR, 0xDF));
            }
            break;
         // VCP/VPP -> VPLL . 
         //Note: The  VPLL setting on each level of  VCP and VPP should be same, since they are on same clk source.
         //just adjust divider outside.
         case RB_INDEX_VPP:           
         case RB_INDEX_VCP0:
         case RB_INDEX_VCP1:
            {
                write_reg_e3k(adapter->mmio, CR, 0xE0, (1<<2 | 1<<0), ~(1<<2 | 1<<0)); 
                
                //program reg setting
                write_reg_e3k(adapter->mmio, CR, 0xDA, ((dfs_table_vpp[index].bs) | (dfs_table_vpp[index].od << 1)), ~0x7);
                write_reg_e3k(adapter->mmio, CR, 0xDB, ((dfs_table_vpp[index].f >>7)<<4), 0xEF);
                write_reg_e3k(adapter->mmio, CR, 0xDC, dfs_table_vpp[index].f , 0x80);
                //triger loading
                write_reg_e3k(adapter->mmio, CR, 0xE0, (1<<1), ~(1<<1)); 
                gf_udelay(25);
                write_reg_e3k(adapter->mmio, CR, 0xE0, 0, ~(3 << 1)); 
                gf_udelay(5);
                
                //check lock stable or not
                while(!(locked &(1<<VPLL_LOCK)))
                {
                     locked = gf_read32(adapter->mmio + MMIO_PMU_START_ADDRESS + PMU_SW_PLL_LOCK_ADDR);
                     gf_udelay(5);
                }       
                //power_trace("program cr dc:%x, read:%x\n", dfs_table_vpp[index].f, read_reg_e3k(adapter->mmio , CR, 0xDC));
            }
            break;            
         default:
            return;
    }

}


/*
 *self check and setting.
 *Since vcp & vpp both come from VPLL, if config vcp to Freq_A which will affect vpp current Freq_B.
 *A compromise way is both changing vcp and vpp to same freq at the same time.
 */
 
 //Seems change one, need parking other two to PCLK.Since parking clk maybe less than 3d.
 //Perhaps we can parking 3d, vcp, vpp to PCLK to solve this issue.
int  vidsch_program_freq(adapter_t *adapter, gf_dvfs_set_t *dfs_set)
{
    vidsch_mgr_t *sch_mgr = adapter->sch_mgr[dfs_set->engine_index];
    vidschedule_t *schedule = adapter->schedule;
    int ret = E_FAIL;

    //forbit pll lock reset.
    gf_write8(adapter->mmio + 0x8af3, 0x04);
    
    if((dfs_set->engine_index == RB_INDEX_GFXL || dfs_set->engine_index == RB_INDEX_GFXH))       
    {
        if(schedule->dvfs_current_index.gfx_index != dfs_set->table_index)
        {
            vidsch_parking_to_PCLK_e3k(adapter, sch_mgr, TRUE);//3d
            vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_VCP0], TRUE);//vcp
            vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_VPP], TRUE);//vpp 

            //EPLL setting
            vidsch_program_PLL_e3k(adapter, sch_mgr, dfs_set->table_index); 
            //vidsch_program_divider_e3k(adapter, sch_mgr, dfs_set->table_index); 

            vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_VCP0], FALSE);//vcp
            vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_VPP], FALSE);//vpp         
            vidsch_parking_to_PCLK_e3k(adapter, sch_mgr, FALSE);//3d       

            schedule->dvfs_current_index.gfx_index = dfs_set->table_index;
            ret = S_OK;

            power_trace("[DFS] set EPLL to %dMhz\n", dfs_table_gfx[dfs_set->table_index].freq_rel);
        }
	else
        {
            ret = S_OK;
        }
    }
    else if((dfs_set->engine_index == RB_INDEX_VCP0 || dfs_set->engine_index == RB_INDEX_VCP1)  
             || (dfs_set->engine_index == RB_INDEX_VPP) )
    {
        if((schedule->dvfs_current_index.gfvd_index != dfs_set->table_index) || (schedule->dvfs_current_index.vpp_index != dfs_set->table_index))
        {
            vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_GFXL], TRUE);//3d
            vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_VCP0], TRUE);//vcp
            vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_VPP], TRUE);//vpp 

            //VPLL setting, both vcp and vpp should be configed.
            vidsch_program_PLL_e3k(adapter, adapter->sch_mgr[RB_INDEX_VCP0], dfs_set->table_index); 
            //vidsch_program_divider_e3k(adapter, adapter->sch_mgr[RB_INDEX_VCP0], dfs_set->table_index); //vcp divider
            //vidsch_program_divider_e3k(adapter, adapter->sch_mgr[RB_INDEX_VPP], dfs_set->table_index); //vpp divider

            vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_VCP0], FALSE);//vcp
            vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_VPP], FALSE);//vpp 
            vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_GFXL], FALSE);//3d

            schedule->dvfs_current_index.gfvd_index = 
            schedule->dvfs_current_index.vpp_index  = dfs_set->table_index;
            ret = S_OK;
        
            power_trace("[DFS] set VPLL to %dMhz\n", dfs_table_gfvd[dfs_set->table_index].freq_rel);
        }
	else
        {
            ret = S_OK;
        }		
    }

    return ret;
}

int vidsch_query_dfs_clamp_e3k(adapter_t *adapter, gf_dvfs_clamp_status_t *dfs_clamp)
{
    vidschedule_t *schedule = adapter->schedule;
    int ret = S_OK;

    dfs_clamp->uint = schedule->dvfs_clamp_value.uint;

    return ret;
}

int vidsch_dfs_set_e3k(adapter_t *adapter, gf_dvfs_set_t *dfs_set)
{
    vidsch_mgr_t *sch_mgr = adapter->sch_mgr[dfs_set->engine_index];
    vidschedule_t *schedule = adapter->schedule;
    int ret = E_FAIL;
  
    unsigned long flags = 0;

    if(!sch_mgr ||dfs_set->table_index < 0) return ret;

    flags = gf_spin_lock_irqsave(schedule->dvfs_status_lock);

    //hw limitation check: video clk should not lower than 3d.
    if(dfs_set->engine_index == RB_INDEX_VCP0 || dfs_set->engine_index == RB_INDEX_VCP1 )
    {
        if(dfs_set->table_index > schedule->dvfs_clamp_value.gfvd_dvfs_index_max ||dfs_set->table_index < schedule->dvfs_clamp_value.gfvd_dvfs_index_min)
        {
            gf_spin_unlock_irqrestore(schedule->dvfs_status_lock, flags);
            return ret;
        }
		
        if( dfs_table_gfx[schedule->dvfs_current_index.gfx_index].freq_rel > dfs_table_gfvd[dfs_set->table_index].freq_rel)
        {
           power_trace("[DFS] try set vcp from %d Mhz to %d Mhz, which is lower than 3d %d Mhz\n", 
               dfs_table_gfvd[schedule->dvfs_current_index.gfvd_index].freq_rel, dfs_table_gfvd[dfs_set->table_index].freq_rel, 
               dfs_table_gfx[schedule->dvfs_current_index.gfx_index].freq_rel );
           gf_spin_unlock_irqrestore(schedule->dvfs_status_lock, flags);
           return ret;
        }
    }
    else if(dfs_set->engine_index == RB_INDEX_VPP)
    {
        if(dfs_set->table_index > schedule->dvfs_clamp_value.vpp_dvfs_index_max ||dfs_set->table_index < schedule->dvfs_clamp_value.vpp_dvfs_index_min)
        {
            gf_spin_unlock_irqrestore(schedule->dvfs_status_lock, flags);
            return ret;
        }
	
        if( dfs_table_gfx[schedule->dvfs_current_index.gfx_index].freq_rel > dfs_table_vpp[dfs_set->table_index].freq_rel)
        {
            power_trace("[DFS] try set vpp from %d Mhz to %d Mhz, which is lower than 3d %d Mhz\n", 
                dfs_table_vpp[schedule->dvfs_current_index.vpp_index].freq_rel, dfs_table_vpp[dfs_set->table_index].freq_rel,
                dfs_table_gfx[schedule->dvfs_current_index.gfx_index].freq_rel );
            gf_spin_unlock_irqrestore(schedule->dvfs_status_lock, flags);
           return ret;
        }
    }
    else if(dfs_set->engine_index == RB_INDEX_GFXH || dfs_set->engine_index == RB_INDEX_GFXL)
    {

        if(dfs_set->table_index > schedule->dvfs_clamp_value.gfx_dvfs_index_max ||dfs_set->table_index < schedule->dvfs_clamp_value.gfx_dvfs_index_min)
        {
            gf_spin_unlock_irqrestore(schedule->dvfs_status_lock, flags);
            return ret;
        }

        if( dfs_table_gfx[dfs_set->table_index].freq_rel > dfs_table_gfvd[schedule->dvfs_current_index.gfvd_index].freq_rel 
            || dfs_table_gfx[dfs_set->table_index].freq_rel > dfs_table_vpp[schedule->dvfs_current_index.vpp_index].freq_rel )
        {
             power_trace("[DFS] try set 3d from %d Mhz to %d Mhz, which is higher than video, vcp %d Mhz, vpp %d Mhz\n", 
                 dfs_table_gfx[schedule->dvfs_current_index.gfx_index].freq_rel,  dfs_table_gfx[dfs_set->table_index].freq_rel,
                 dfs_table_gfvd[schedule->dvfs_current_index.gfvd_index].freq_rel, dfs_table_vpp[schedule->dvfs_current_index.vpp_index].freq_rel);

             gf_spin_unlock_irqrestore(schedule->dvfs_status_lock, flags);                 
             return ret;
        }
    }
    else
    {
        gf_spin_unlock_irqrestore(schedule->dvfs_status_lock, flags);        
        return ret;
    }

    ret = vidsch_program_freq(adapter, dfs_set);

    gf_spin_unlock_irqrestore(schedule->dvfs_status_lock, flags); 

    return ret;
}

void vidsch_dfs_init_e3k(adapter_t *adapter)
{
    vidsch_mgr_t *sch_mgr = NULL;
    vidschedule_t *schedule = adapter->schedule;
    unsigned int programed = 0;
    
    unsigned long flags = 0;

    // TODO:  still need the mmio offset
    //disable the bit of all reg protection. enable cpu R/W of dfs mmio.
    //reg_value = gf_read32(adapter->mmio +   );
    //reg_value &= ~(1 << 8); 
    //gf_write32(adapter->mmio +   , reg_value);

    flags = gf_spin_lock_irqsave(schedule->dvfs_status_lock);

 //   write_reg_e3k(adapter->mmio, CR, 0xE2, (1<<0), ~(1<<0));  //EPLL use SW mode load CLK setting.
//    write_reg_e3k(adapter->mmio, CR, 0xE0, (1<<0), ~(1<<0));  //VPLL use SW mode load CLK setting.
//    gf_udelay(50);

    vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_GFXL], TRUE);//3d
    vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_VCP0], TRUE);//vcp
    vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_VPP], TRUE);//vpp 

    //EPLL 
    vidsch_program_PLL_e3k(adapter, adapter->sch_mgr[RB_INDEX_GFXL], DEFAULT_GFX_INDEX); 
//    vidsch_program_divider_e3k(adapter, adapter->sch_mgr[RB_INDEX_GFXL], DEFAULT_GFX_INDEX); 

    //VPLL
    vidsch_program_PLL_e3k(adapter, adapter->sch_mgr[RB_INDEX_VCP0], DEFAULT_VIDEO_INDEX); 
//    vidsch_program_divider_e3k(adapter, adapter->sch_mgr[RB_INDEX_VCP0], DEFAULT_VIDEO_INDEX); //vcp divider
//    vidsch_program_divider_e3k(adapter, adapter->sch_mgr[RB_INDEX_VPP], DEFAULT_VIDEO_INDEX);//vpp divider 

    vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_VCP0], FALSE);//vcp
    vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_VPP], FALSE);//vpp 
    vidsch_parking_to_PCLK_e3k(adapter, adapter->sch_mgr[RB_INDEX_GFXL], FALSE);//3d

    schedule->dvfs_current_index.gfx_index  = DEFAULT_GFX_INDEX;
    schedule->dvfs_current_index.gfvd_index = DEFAULT_VIDEO_INDEX;
    schedule->dvfs_current_index.vpp_index  = DEFAULT_VIDEO_INDEX;
    
    schedule->dvfs_clamp_value.uint = 0;

    schedule->dvfs_clamp_value.gfx_dvfs_index_min  = 0;
    schedule->dvfs_clamp_value.gfvd_dvfs_index_min = 0;
    schedule->dvfs_clamp_value.vpp_dvfs_index_min  = 0;

   //not all chip can use all freq in table, so limit to 700M.
    schedule->dvfs_clamp_value.gfx_dvfs_index_max  = 3;//GFX_INDEX_TABLE_SIZE-1;
    schedule->dvfs_clamp_value.gfvd_dvfs_index_max = 3;//VIDEO_INDEX_TABLE_SIZE-1;
    schedule->dvfs_clamp_value.vpp_dvfs_index_max  = 3;//VIDEO_INDEX_TABLE_SIZE-1;
    
    gf_spin_unlock_irqrestore(schedule->dvfs_status_lock, flags);

}

void vidsch_dfs_deinit_e3k(adapter_t *adapter)
{
    unsigned int reg_value = 0;

    //enable the bit of all reg protection. disable cpu R/W of dfs mmio.
    //reg_value = gf_read32(adapter->mmio +   );
    //reg_value |= (1 << 8); 
    //gf_write32(adapter->mmio +   , reg_value);
} 

void vidsch_dfs_tuning_e3k(adapter_t *adapter)
{
    unsigned int busy_engines = 0;
    unsigned int hw_pending_task[MAX_ENGINE_COUNT] = {0};
    unsigned int pool_pending_task[MAX_ENGINE_COUNT] = {0};		
    vidschedule_t *schedule = adapter->schedule;
    vidsch_mgr_t  *sch_mgr  = NULL;
    int   engine_index       = 0;
    unsigned int   successed = 0;    
    gf_dvfs_set_t dfs_set  = {0};

    unsigned int gfx_pending = 0;
    unsigned int video_pending = 0;

     int best_3d = 0;
     int best_video = 0;

     if(!schedule->dvfs_init)
     {
         vidsch_dfs_init_e3k(adapter);
         schedule->dvfs_init = TRUE;
         return;
     }

    vidsch_query_engine_task_status_e3k(adapter, &busy_engines, hw_pending_task, pool_pending_task);	 
	
    for (engine_index = adapter->active_engine_count-1; engine_index >= 0; engine_index--)
    {
        sch_mgr = adapter->sch_mgr[engine_index];

        if(sch_mgr == NULL) continue;

         //vcp and vpp should only has one ring buffer in use at once, while gfx may has high/low ring buffer swtich case.
        if(sch_mgr->engine_index == RB_INDEX_GFXH || sch_mgr->engine_index == RB_INDEX_GFXL)
        {
            gfx_pending += pool_pending_task[sch_mgr->engine_index];
        }

        if(sch_mgr->engine_index == RB_INDEX_VCP0|| sch_mgr->engine_index == RB_INDEX_VCP1 ||sch_mgr->engine_index == RB_INDEX_VPP)
        {
            video_pending += pool_pending_task[sch_mgr->engine_index];
        }
    }

    if(gfx_pending > 2*DFS_TASK_THREHOLD_GFX)
    {
        best_3d =  ((schedule->dvfs_current_index.gfx_index + 1) <= schedule->dvfs_clamp_value.gfx_dvfs_index_max)?
                                 (schedule->dvfs_current_index.gfx_index + 1):schedule->dvfs_clamp_value.gfx_dvfs_index_max;        
    }
    else if(gfx_pending < DFS_TASK_THREHOLD_GFX)
    {
        best_3d =  ((schedule->dvfs_current_index.gfx_index - 1) >= schedule->dvfs_clamp_value.gfx_dvfs_index_min)?
                                 (schedule->dvfs_current_index.gfx_index - 1):schedule->dvfs_clamp_value.gfx_dvfs_index_min;
    }
    else
    {
        best_3d = schedule->dvfs_current_index.gfx_index;        
    }

   if( video_pending > 2* DFS_TASK_THREHOLD_VIDEO)
   {
       best_video = ((schedule->dvfs_current_index.gfvd_index +1) <= schedule->dvfs_clamp_value.gfvd_dvfs_index_max)?
                              (schedule->dvfs_current_index.gfvd_index +1):schedule->dvfs_clamp_value.gfvd_dvfs_index_max;
   }
   else if( video_pending < DFS_TASK_THREHOLD_VIDEO)
   {
       best_video = ((schedule->dvfs_current_index.gfvd_index -1)>= schedule->dvfs_clamp_value.gfvd_dvfs_index_min)?
                             (schedule->dvfs_current_index.gfvd_index -1):schedule->dvfs_clamp_value.gfvd_dvfs_index_min;
   }
   else
   {
       best_video = schedule->dvfs_current_index.gfvd_index;
   } 

    //for cg enable check, when video is idle set them to max clk which will enlarge 3d usable clk range.
    if(adapter->pwm_level.EnableClockGating)
    {
        for (engine_index = adapter->active_engine_count-1; engine_index >= 0 ; engine_index--)
        {
            unsigned long flags = 0;
            sch_mgr = adapter->sch_mgr[engine_index];

	    if(sch_mgr == NULL) continue;

            flags = gf_spin_lock_irqsave(sch_mgr->power_status_lock);

	    if(!sch_mgr->engine_dvfs_power_on)
            {
                switch(engine_index)
                {
                     case RB_INDEX_GFXL:	
                     case RB_INDEX_GFXH:						 	
                          best_3d = ((busy_engines & (1 << RB_INDEX_GFXH)) || (busy_engines & (1 << RB_INDEX_GFXL)))? 
						  	best_3d:schedule->dvfs_clamp_value.gfx_dvfs_index_min;
                         break;
                     case RB_INDEX_VCP0:
                     case RB_INDEX_VCP1:
                     case RB_INDEX_VPP: 		  
                         best_video = ((busy_engines & (1 << RB_INDEX_VCP0)) || (busy_engines & (1 << RB_INDEX_VCP1)) ||(busy_engines & (1 << RB_INDEX_VPP)))?
						 	best_video:schedule->dvfs_clamp_value.gfvd_dvfs_index_max;
                         break;
                     default:
                         ;
                }
            }		
            gf_spin_unlock_irqrestore(sch_mgr->power_status_lock, flags);
        }
    }

   {
        int op_3d = best_3d - schedule->dvfs_current_index.gfx_index;
        int op_video = best_video - schedule->dvfs_current_index.gfvd_index;
        int ret = E_FAIL;        

/*    op_3d                 op_video             set video first
        --------------------------------------------------------
        >0                       <0                         0
        >0                       =0                         0/1
        >0                       >0                         1
        =0                       <0                         0/1
        =0                       =0                         0/1
        =0                       >0                         0/1
        <0                       <0                         0
        <0                       =0                         0
        <0                       >0                         0/1
        --------------------------------------------------------
*/
  //      power_trace("*********************START*******************************\n");     
        if(op_3d > 0 && op_video >0)   
        {           
            dfs_set.engine_index = RB_INDEX_VCP0;
            dfs_set.table_index  =  best_video;
            ret = vidsch_dfs_set_e3k(adapter, &dfs_set);

            if(ret == S_OK)
            {
                dfs_set.engine_index = RB_INDEX_GFXL;
                dfs_set.table_index  =  best_3d;
                ret = vidsch_dfs_set_e3k(adapter, &dfs_set);       
            }
        }
        else
        {
            dfs_set.engine_index = RB_INDEX_GFXL;
            dfs_set.table_index  =  best_3d;
            ret = vidsch_dfs_set_e3k(adapter, &dfs_set);       

            if(ret == S_OK)
            {
                dfs_set.engine_index = RB_INDEX_VCP0;
                dfs_set.table_index  =  best_video;
                ret = vidsch_dfs_set_e3k(adapter, &dfs_set);
            }
        }
//        power_trace("*********************END*******************************\n");             
  }    
}

static inline int vidsch_is_power_clock_dependancy_e3k(vidsch_mgr_t *sch_mgr, unsigned int busy_engines)
{
    int status = FALSE;

    switch(sch_mgr->engine_index)
    {
        case RB_INDEX_GFXL:
        case RB_INDEX_GFXH:
            if((busy_engines & (1<< RB_INDEX_GFXL)) || (busy_engines & (1<< RB_INDEX_GFXH)))
            {
                status = TRUE;
            }
            break;           						
        default:
            status = FALSE;
            break;                        
    }

    return status;
}

static unsigned char vidsch_cacl_available_gpc_bit(adapter_t * adapter)
{
    unsigned char bits = 0;

    if(adapter->hw_caps.chip_slice_mask &0xF00) bits |=0x10;
    if(adapter->hw_caps.chip_slice_mask &0x0F0) bits |=0x20;
    if(adapter->hw_caps.chip_slice_mask &0x00F) bits |=0x40;	
	
    return bits;
}

int vidsch_power_clock_on_off_e3k(vidsch_mgr_t *sch_mgr, unsigned int off)
{
    adapter_t * adapter = sch_mgr->adapter;
    vidschedule_t *schedule = adapter->schedule;	
    unsigned char used_gpc_mask_bit = vidsch_cacl_available_gpc_bit(adapter);
    unsigned long flags = 0;

    flags = gf_spin_lock_irqsave(schedule->power_status_lock);	
    //power_trace("[CG] used_gpc_mask_bit:%x\n", used_gpc_mask_bit);
    
    if(off)
    {
        switch(sch_mgr->engine_index)
        {
            case RB_INDEX_GFXL:
            case RB_INDEX_GFXH: 
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 0, 0x8F);
                break;
            case RB_INDEX_VCP0:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 0, 0xF7);                
                break;                
            case RB_INDEX_VCP1:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 0, 0xFB);
                break;                
            case RB_INDEX_VPP:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 0, 0xFD);
                break;
            default:
                  break;        
        }
        
        power_trace("[CG] engine %d do cg power off,  CR01:%x\n", sch_mgr->engine_index, read_reg_e3k(adapter->mmio, CR_C, 0x01));
    }
    else
    {
        switch(sch_mgr->engine_index)
        {    
            case RB_INDEX_GFXL:
            case RB_INDEX_GFXH: 
                write_reg_e3k(adapter->mmio, CR_C, 0x01, (0x70 & used_gpc_mask_bit), 0x8F);
                break;
            case RB_INDEX_VCP0:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 8, 0xF7);                
                break;                
            case RB_INDEX_VCP1:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 4, 0xFB);
                break;                
            case RB_INDEX_VPP:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 2, 0xFD);
                break;
            default:
                  break;    
        }
        power_trace("[CG] engine %d do cg power on,  CR01:%x\n", sch_mgr->engine_index, read_reg_e3k(adapter->mmio, CR_C, 0x01));
    }

    gf_spin_unlock_irqrestore(schedule->power_status_lock, flags);

    return S_OK;
}

void vidsch_query_engine_task_status_e3k(adapter_t *adapter, unsigned int *busy_engines, unsigned int *hw_pending_tasks,  unsigned int *pool_pending_tasks)
{
    int engine_index;
    vidsch_mgr_t  *sch_mgr = NULL;

    *busy_engines = 0;
	
    for (engine_index = adapter->active_engine_count-1; engine_index >= 0; engine_index--)
    {
        sch_mgr = adapter->sch_mgr[engine_index];

        if (sch_mgr == NULL)  continue;

        if (list_empty(&sch_mgr->submitted_task_list) && //exclude display task, wait task ...which no need submit to hw
            vidsch_is_fence_back(sch_mgr->adapter, sch_mgr->engine_index, sch_mgr->last_send_fence_id))
        {
            hw_pending_tasks[engine_index] = 0;
        }
        else
        {
            hw_pending_tasks[engine_index] = sch_mgr->last_send_fence_id - sch_mgr->returned_fence_id;
            *busy_engines |= (1 << sch_mgr->engine_index);						
	}
		
        {
            pool_pending_tasks[engine_index] = sch_mgr->emergency_task_pool.task_num + sch_mgr->normal_task_pool.task_num;
        }
    }
}

void vidsch_power_tuning_e3k(adapter_t *adapter, unsigned int gfx_only)
{
    vidschedule_t *schedule = adapter->schedule;
    vidsch_mgr_t  *sch_mgr = NULL;
    int engine_index;
    unsigned int busy_engines = 0;
    unsigned int hw_pending_task[MAX_ENGINE_COUNT] = {0};
    unsigned int pool_pending_tasks[MAX_ENGINE_COUNT] = {0};	
    gf_dvfs_set_t dfs_set  = {0};	  	
    unsigned int engine_mask = gfx_only ? GFX_ENGINE_MASK : ALL_ENGINE_MASK;

    if (!adapter->pwm_level.EnableClockGating)
    {
        return;
    }

    for (engine_index = adapter->active_engine_count-1; engine_index >= 0; engine_index--)
    {
        int ret = S_OK; 
        unsigned long flags=0;

        if ((engine_mask & (1<<engine_index)) == 0) continue;

        sch_mgr = adapter->sch_mgr[engine_index];

        if (sch_mgr == NULL)  continue;

        vidsch_update_engine_idle_status(adapter, (1 << engine_index));
        if (sch_mgr->completely_idle && (!vidsch_is_power_clock_dependancy_e3k(sch_mgr,schedule->busy_engine_mask)))
        {
            flags = gf_spin_lock_irqsave(sch_mgr->power_status_lock);
            if (sch_mgr->chip_func->power_clock &&
                sch_mgr->engine_dvfs_power_on &&
                vidsch_is_fence_back(sch_mgr->adapter, sch_mgr->engine_index, sch_mgr->last_send_fence_id))
            {
                if(adapter->hw_caps.dfs_enable)
                {
                    /****************************************************************
                    **  for video with max freq.
                    **  this can easy statisfy the hw limitation "3d freq <= video freq" when CLK ON
                    ****************************************************************/

                    dfs_set.engine_index = sch_mgr->engine_index;
                    dfs_set.table_index  = ( sch_mgr->engine_index == RB_INDEX_VCP0 || sch_mgr->engine_index == RB_INDEX_VCP1 ||sch_mgr->engine_index == RB_INDEX_VPP) ?
						schedule->dvfs_clamp_value.gfvd_dvfs_index_max:schedule->dvfs_current_index.gfx_index;

                    ret = vidsch_dfs_set_e3k(adapter, &dfs_set);
                }

                if(ret == S_OK)
                {
                   //gf_info("power off: last_send_fence_id=%d. \n", sch_mgr->last_send_fence_id);
                   sch_mgr->chip_func->power_clock(sch_mgr, TRUE);
                   sch_mgr->engine_dvfs_power_on = FALSE;
                }
            }
            gf_spin_unlock_irqrestore(sch_mgr->power_status_lock, flags);
        }
    }    
}


