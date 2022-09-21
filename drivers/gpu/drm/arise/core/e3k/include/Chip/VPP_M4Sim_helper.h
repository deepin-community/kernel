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

#include "VPP_M4Sim.h"
#include <math.h>

#define RR(x) Vpp_Global_read_reg(pReg, x, 0)
#define FD_SUPPORT_CHANNEL 4


struct FD_detect_info_struct m_FD_detect[FD_SUPPORT_CHANNEL], m_FD_detect_HW[FD_SUPPORT_CHANNEL];


int FD_Judge(struct FD_query_dump_info_struct * s, uint32 index, struct FD_frame_info_struct* m_FD_info,Vpp_Global_regs* pReg, uint32 top_first, uint32 Reset_Fd)
{

    enum FilmCheckResult m_Operation_SW[FD_SUPPORT_CHANNEL] = {enumFC_DeInterlace,enumFC_DeInterlace,enumFC_DeInterlace,enumFC_DeInterlace};

    int mode = enumFC_DeInterlace;
    static int First_useflag = 0;

    int Gb_Fd_Detect_Counter_Init = s->Gb_Fd_Detect_Counter_Init;
    int Gb_Fd_Counter_Init = s->Gb_Fd_Counter_Init;
    int Gb_2v2_Failed_Counter_Th = s->Gb_2v2_Failed_Counter_Th;
    int Gb_2v2_Md_Succeed_counter_Th = s->Gb_2v2_Md_Succeed_counter_Th;
    int Gb_2v2_Sad_Succeed_counter_Th = s->Gb_2v2_Sad_Succeed_counter_Th;


    static int C2v2_MD[FD_SUPPORT_CHANNEL] = {0,0,0,0};
    static int C2v2_counter[FD_SUPPORT_CHANNEL] = {0,0,0,0};
    static int strong_2v2_failed[FD_SUPPORT_CHANNEL] = {0,0,0,0};
    static unsigned int mode_2v2_count[FD_SUPPORT_CHANNEL] = {0,0,0,0};


    if(Reset_Fd)
    {
        memset(&C2v2_MD[index],0x00,sizeof(int));
        memset(&C2v2_counter[index],0x00,sizeof(int));
        memset(&strong_2v2_failed[index],0x00,sizeof(int));
        memset(&mode_2v2_count[index],0x00,sizeof(int));
        First_useflag = 0;
    }

#if 1
    if(m_FD_detect[index].FD_Detect_counter == 0 &&  m_FD_detect[index].FD_Counter == 0)
    {
        m_FD_detect[index].FD_P01 = 0;
        m_FD_detect[index].FD_P10 = 0;
        m_FD_detect[index].FD_P00_Copy = 0;
    }
#endif



    if(m_FD_info->FD_pattern != 0x0  &&  m_FD_info->Sence_change == 0 && m_FD_detect[index].FD_Detect_counter == 0 && m_FD_detect[index].FD_Counter == 0)
    {
        m_FD_detect[index].FD_Detect_counter = Gb_Fd_Detect_Counter_Init;
        m_FD_detect[index].FD_P01 = 0;
        m_FD_detect[index].FD_P10 = 0;
        m_FD_detect[index].FD_P00_Copy = 0;
    }

    if(m_FD_detect[index].FD_Detect_counter || m_FD_detect[index].FD_Counter)
    {
        if(m_FD_info->FD_pattern == 0x2 )
        {
            m_FD_detect[index].FD_P10 ++;
        }
        else if (m_FD_info->FD_pattern == 0x1)
        {
            m_FD_detect[index].FD_P01++;
        }
    }

    if(m_FD_detect[index].FD_Counter )
    {
        if(m_FD_detect[index].FD_P01 > m_FD_detect[index].FD_P10)
        {
            m_FD_detect[index].FD_P01 -= m_FD_detect[index].FD_P10;
            m_FD_detect[index].FD_P10 = 0;

        }
        else
        {
            m_FD_detect[index].FD_P10 -= m_FD_detect[index].FD_P01;
            m_FD_detect[index].FD_P01 = 0;
        }
    }

    if(m_FD_detect[index].FD_Detect_counter || m_FD_detect[index].FD_Counter)
    {
        int c = m_FD_detect[index].FD_P01-m_FD_detect[index].FD_P10;
        if(abs( c) > 1)
        {
            m_FD_detect[index].FD_Counter = 0;
            m_FD_detect[index].FD_Detect_counter = 0;
            m_FD_detect[index].FD_P01 = 0;
            m_FD_detect[index].FD_P10 = 0;
            m_FD_detect[index].FD_P00_Copy = 0;
        }




    }

    if (/*m_FD_detect[index].FD_Detect_counter >= 0 && */m_FD_detect[index].FD_Counter == 0 &&
        m_FD_detect[index].FD_P01 > 0 &&  m_FD_detect[index].FD_P10  >0 && m_FD_detect[index].FD_P00_Copy >0
        && (m_FD_detect[index].FD_P00_Copy > 2 || (m_FD_detect[index].FD_P10 +m_FD_detect[index].FD_P01) >= 3))
    {
        m_FD_detect[index].FD_Counter = Gb_Fd_Counter_Init;
        m_FD_detect[index].FD_Detect_counter = 0;
    }

    if(m_FD_detect[index].FD_Detect_counter || m_FD_detect[index].FD_Counter)
    {
        if(m_FD_info->FD_pattern == 0x2 )
        {
            if(m_FD_detect[index].FD_Counter)
                m_FD_detect[index].FD_Counter = Gb_Fd_Counter_Init;

            mode = enumFC_FrameMerge;
        }
        else if(m_FD_info->FD_pattern == 0x1)
        {
            if(m_FD_detect[index].FD_Counter)
                m_FD_detect[index].FD_Counter = Gb_Fd_Counter_Init;

            mode = enumFC_FrameCopy;
        }
        else
        {
            if(First_useflag)
            {
                mode = enumFC_FrameMerge;
                if(m_FD_detect[index].FD_Counter)
                    m_FD_detect[index].FD_Counter--;
            }
            else
            {
                mode = enumFC_FrameCopy;


                if(!m_FD_info->MD_2v2_small && m_FD_detect[index].FD_Counter)
                {
                    m_FD_detect[index].FD_Counter--;
                }

                if(m_FD_detect[index].FD_Detect_counter > 0)
                {
                    m_FD_detect[index].FD_P00_Copy++;
                    if(m_FD_info->MD_2v2_small  || m_FD_info->Counter_2v2 || m_FD_info->MD_2v2)
                    {

                    }
                    else
                    {
                        m_FD_detect[index].FD_Detect_counter = 0;
                        mode = enumFC_DeInterlace;
                    }
                }
            }
        }
    }


    if(m_FD_detect[index].FD_Detect_counter || m_FD_detect[index].FD_Counter)
    {
        if(mode == enumFC_FrameMerge)
        {
            First_useflag = 1;
        }
        else
        {
            First_useflag = 0;
        }


        if(m_FD_detect[index].FD_Detect_counter > 0)
        {
            mode = enumFC_DeInterlace;
            m_FD_detect[index].FD_Detect_counter--;
        }
    }

    if(m_FD_detect[index].FD_Counter)
    {
        m_Operation_SW[index] = (enum FilmCheckResult)mode;
        return m_Operation_SW[index];
    }


    if(m_FD_info->Counter_Failed_2v2)
    {
        strong_2v2_failed[index]++;
    }
    else
        strong_2v2_failed[index] = 0;


    if(mode_2v2_count[index] > s->Gb_2v2_Frame_Cnt_Th)
    {
        Gb_2v2_Failed_Counter_Th = s->Gb_2v2_Failed_Counter_Th2;
    }

    if(strong_2v2_failed[index] > Gb_2v2_Failed_Counter_Th)
    {
        C2v2_MD[index] = 0;
        C2v2_counter[index] = 0;
    }

    if(m_FD_info->MD_2v2)
    {
        C2v2_MD[index]++;
    }

    if(m_FD_info->Counter_2v2)
        C2v2_counter[index]++;

    if(!m_FD_info->MD_2v2_small)
    {
        if(!m_FD_info->Counter_2v2)
        {
            C2v2_counter[index] = 0;
        }

        if(!m_FD_info->MD_2v2)
        {
            C2v2_MD[index] = 0;
        }
    }



    if(C2v2_MD[index] > Gb_2v2_Md_Succeed_counter_Th || C2v2_counter[index] > Gb_2v2_Sad_Succeed_counter_Th)
    {
        m_FD_detect[index].In_2v2_mode = 1;
        mode_2v2_count[index]++;
    }
    else
    {
        m_FD_detect[index].In_2v2_mode  = 0;
        mode_2v2_count[index] = 0;
    }

    if( m_FD_detect[index].In_2v2_mode )
    {
        m_Operation_SW[index] = enumFC_FrameCopy;
        return m_Operation_SW[index];
    }


    m_Operation_SW[index] = enumFC_DeInterlace;

    return m_Operation_SW[index];
}

int m4_fd_mode(struct FD_query_dump_info_struct * s,Vpp_Global_regs* pReg)
{
    struct FD_frame_info_struct m_FD_info;
    uint32 index;
    unsigned int Gb_2v2_Md_Counter_Th;
    unsigned int Gb_2v2_Failed_Md_Counter_Th ;
    int Gb_2v2_Sad_Counter_Th;

    unsigned int Gb_2v2_Md_Counter_Small_Th ;
    unsigned int Gb_Fd_Counter_Th;
    unsigned int Gb_2v2_Sad_Failed_Range;
    int cur_f[2];

    int delta;

    int bottom_field_result = 0;
    int top_field_result = 0;
    enum FilmCheckResult ret;


    static int m_prev_max[FD_SUPPORT_CHANNEL][2] ={0,0};
    static int m_prev_min[FD_SUPPORT_CHANNEL][2] ={0,0};

    uint64 m_FD_BottomCounter = s->m_FD_BottomCounter;
    uint64 m_FD_TopCounter = s->m_FD_TopCounter;


    uint32 m_2v2_counter[3];  uint32 m_2v2_MD_counter[4];
    uint32 ret_fd = s->ret_fd;
    uint32 top_first = s->top_first;

    memcpy(m_2v2_counter , s->m_2v2_counter, sizeof(s->m_2v2_counter));
    memcpy(m_2v2_MD_counter, s->m_2v2_MD_counter, sizeof(s->m_2v2_MD_counter));
    index = s->index;



    Gb_2v2_Md_Counter_Th = s->Gb_2v2_Md_Counter_Th;
    Gb_2v2_Failed_Md_Counter_Th= s->Gb_2v2_Failed_Md_Counter_Th;
    Gb_2v2_Sad_Counter_Th = s->Gb_2v2_Sad_Counter_Th;
    Gb_2v2_Md_Counter_Small_Th= s->Gb_2v2_Md_Counter_Small_Th;
    Gb_Fd_Counter_Th =s->Gb_Fd_Counter_Th;
    Gb_2v2_Sad_Failed_Range =s->Gb_2v2_Sad_Failed_Range;

    m_FD_info.MD_2v2 = 0;
    m_FD_info.MD_2v2_small = 0;
    m_FD_info.MD_Failed_2v2 = 0;
    m_FD_info.Counter_2v2 = 0;
    m_FD_info.Counter_Failed_2v2 = 0;
    m_FD_info.No_FD_mode = 0;


    if((m_2v2_MD_counter[0]+m_2v2_MD_counter[1]+m_2v2_MD_counter[2]+m_2v2_MD_counter[3]) >  Gb_2v2_Md_Counter_Small_Th)
    {
        if((m_2v2_MD_counter[0]+m_2v2_MD_counter[1])*Gb_2v2_Md_Counter_Th < (m_2v2_MD_counter[2]+m_2v2_MD_counter[3]) )
        {
#if 0
            if( m_pISP->m_CmdVpp.Top_First &&  m_2v2_MD_counter[3]*10 > m_2v2_MD_counter[4] )
            {
                m_FD_info->MD_2v2 = 1;
            }
            else (m_2v2_MD_counter[4]*10 > m_2v2_MD_counter[3])
#endif
                m_FD_info.MD_2v2 = 1;
        }
        m_FD_info.No_FD_mode = 1;
    }
    else
    {
        m_FD_info.MD_2v2_small = 1;
    }

    if(m_2v2_MD_counter[2]> (Gb_2v2_Md_Counter_Small_Th>>1) &&  m_2v2_MD_counter[3]> (Gb_2v2_Md_Counter_Small_Th>>1))
    {
        if(m_2v2_MD_counter[2]*Gb_2v2_Failed_Md_Counter_Th < m_2v2_MD_counter[3] || m_2v2_MD_counter[2] > m_2v2_MD_counter[3]*Gb_2v2_Failed_Md_Counter_Th)
        {
            m_FD_info.MD_Failed_2v2 = 1;
        }
#if 0

        if(m_2v2_MD_counter[2]*Gb_2v2_Failed_Md_Counter_Th > m_2v2_MD_counter[3] && m_2v2_MD_counter[2] > m_2v2_MD_counter[3]*Gb_2v2_Failed_Md_Counter_Th)
        {
            int cc = 0;
        }
#endif

    }




    if(m_2v2_counter[1] == 0)
        cur_f[0] = 127;
    else
        cur_f[0] = m_2v2_counter[2]*64/m_2v2_counter[1];


    if(m_2v2_counter[0] == 0)
        cur_f[1] = 127;
    else
        cur_f[1] = m_2v2_counter[2]*64/m_2v2_counter[0];



    if(cur_f[0] < Gb_2v2_Sad_Counter_Th*2 && cur_f[1] < Gb_2v2_Sad_Counter_Th*2)
    {
        m_FD_info.Counter_2v2 = 1;
    }

    m_FD_info.Counter_Failed_2v2 = 0;


    if(cur_f[0] >Gb_2v2_Sad_Counter_Th*2 || cur_f[1] > Gb_2v2_Sad_Counter_Th*2 )
    {


        if( (cur_f[0]>>1) > (m_prev_min[index][0]>>1) && (cur_f[0]>>1) < (m_prev_max[index][0]>>1) && (cur_f[0]>>1) < (64>>1))
        {
            m_FD_info.Counter_Failed_2v2  = 1;
        }

        if( (cur_f[1]>>1) > (m_prev_min[index][1]>>1) && (cur_f[1]>>1) < (m_prev_max[index][1]>>1) && (cur_f[1]>>1) < (64>>1))
        {
            m_FD_info.Counter_Failed_2v2  = 1;
        }
    }
    delta = (Gb_2v2_Sad_Failed_Range*2);
    m_prev_max[index][0] = cur_f[0]+ delta;
    m_prev_min[index][0] = cur_f[0]- delta;
    m_prev_max[index][1] = cur_f[1]+ delta;
    m_prev_min[index][1] = cur_f[1]- delta;


    if(m_FD_BottomCounter > m_FD_TopCounter *Gb_Fd_Counter_Th  && m_FD_info.No_FD_mode == 0)
    {
        bottom_field_result = 0;
        top_field_result = 1;
    }

    if(m_FD_BottomCounter *Gb_Fd_Counter_Th < m_FD_TopCounter && m_FD_info.No_FD_mode == 0)
    {
        bottom_field_result = 1;
        top_field_result = 0;
    }



    if(top_first)
    {
        m_FD_info.FD_pattern = (top_field_result*2+ bottom_field_result);
    }
    else
    {
        m_FD_info.FD_pattern = (bottom_field_result*2+ top_field_result);
    }

    ret = (enum FilmCheckResult)FD_Judge(s, index,&m_FD_info,pReg,top_first,ret_fd);

    return ret;

}



