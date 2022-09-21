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

#ifndef _MXU_INTERFACES_H
#define _MXU_INTERFACES_H

#ifndef    MXU_INTERFACEBASE_INF
   #define MXU_INTERFACEBASE_INF
   #define INTERFACE_MXU_VERSION 1
   #define INTERFACE_MXU_TIMESTAMP "16:19 2018-09-11"
#endif







typedef enum
{
    MXU_HUB_ENTRY_TYPE_READ                                      = 0   ,
    MXU_HUB_ENTRY_TYPE_GM_FLUSH                                  = 1   ,
} MXU_HUB_ENTRY_TYPE;







typedef enum
{
    MXU_L2_REQ_ENTRY_TYPE_READ                                   = 0   ,
    MXU_L2_REQ_ENTRY_TYPE_COMMAND                                = 1   ,
} MXU_L2_REQ_ENTRY_TYPE;



typedef enum
{
    MXU_BIUM_ENTRY_TYPE_READ                                     = 0   ,
    MXU_BIUM_ENTRY_TYPE_WRITE                                    = 1   ,
} MXU_BIUM_ENTRY_TYPE;



typedef enum
{
    MACB_ENTRY_TYPE_READ                                         = 0   ,
    MACB_ENTRY_TYPE_WRITE                                        = 1   ,
} MACB_ENTRY_TYPE;



typedef enum
{
    MAH0B_ENTRY_TYPE_READ                                        = 0   ,
    MAH0B_ENTRY_TYPE_WRITE                                       = 1   ,
} MAH0B_ENTRY_TYPE;



typedef enum
{
    MAH1B_ENTRY_TYPE_READ                                        = 0   ,
    MAH1B_ENTRY_TYPE_WRITE                                       = 1   ,
} MAH1B_ENTRY_TYPE;



typedef enum
{
    MAH2B_ENTRY_TYPE_READ                                        = 0   ,
    MAH2B_ENTRY_TYPE_WRITE                                       = 1   ,
} MAH2B_ENTRY_TYPE;



typedef enum
{
    MAL0B_ENTRY_TYPE_READ                                        = 0   ,
    MAL0B_ENTRY_TYPE_WRITE                                       = 1   ,
} MAL0B_ENTRY_TYPE;



typedef enum
{
    MAL1B_ENTRY_TYPE_READ                                        = 0   ,
    MAL1B_ENTRY_TYPE_WRITE                                       = 1   ,
} MAL1B_ENTRY_TYPE;



typedef enum
{
    MAL2B_ENTRY_TYPE_READ                                        = 0   ,
    MAL2B_ENTRY_TYPE_WRITE                                       = 1   ,
} MAL2B_ENTRY_TYPE;



typedef enum
{
    MAMB_ENTRY_TYPE_READ                                         = 0   ,
    MAMB_ENTRY_TYPE_WRITE                                        = 1   ,
} MAMB_ENTRY_TYPE;



typedef enum
{
    MAGB_ENTRY_TYPE_READ                                         = 0   ,
    MAGB_ENTRY_TYPE_WRITE                                        = 1   ,
} MAGB_ENTRY_TYPE;



typedef enum
{
    MXU_MIU_ENTRY_TYPE_READ                                      = 0   ,
    MXU_MIU_ENTRY_TYPE_WRITE                                     = 1   ,
} MXU_MIU_ENTRY_TYPE;





typedef enum
{
    MAC0_ENTRY_TYPE_READ                                         = 0   ,
    MAC0_ENTRY_TYPE_WRITE                                        = 1   ,
} MAC0_ENTRY_TYPE;



typedef enum
{
    MAH00_ENTRY_TYPE_READ                                        = 0   ,
    MAH00_ENTRY_TYPE_WRITE                                       = 1   ,
} MAH00_ENTRY_TYPE;



typedef enum
{
    MAH10_ENTRY_TYPE_READ                                        = 0   ,
    MAH10_ENTRY_TYPE_WRITE                                       = 1   ,
} MAH10_ENTRY_TYPE;



typedef enum
{
    MAH20_ENTRY_TYPE_READ                                        = 0   ,
    MAH20_ENTRY_TYPE_WRITE                                       = 1   ,
} MAH20_ENTRY_TYPE;



typedef enum
{
    MAM0_ENTRY_TYPE_READ                                         = 0   ,
    MAM0_ENTRY_TYPE_WRITE                                        = 1   ,
} MAM0_ENTRY_TYPE;



typedef enum
{
    MAL00_ENTRY_TYPE_READ                                        = 0   ,
    MAL00_ENTRY_TYPE_WRITE                                       = 1   ,
} MAL00_ENTRY_TYPE;



typedef enum
{
    MAL10_ENTRY_TYPE_READ                                        = 0   ,
    MAL10_ENTRY_TYPE_WRITE                                       = 1   ,
} MAL10_ENTRY_TYPE;



typedef enum
{
    MAL20_ENTRY_TYPE_READ                                        = 0   ,
    MAL20_ENTRY_TYPE_WRITE                                       = 1   ,
} MAL20_ENTRY_TYPE;



typedef enum
{
    MAG0_ENTRY_TYPE_READ                                         = 0   ,
    MAG0_ENTRY_TYPE_WRITE                                        = 1   ,
} MAG0_ENTRY_TYPE;



typedef enum
{
    MAC1_ENTRY_TYPE_READ                                         = 0   ,
    MAC1_ENTRY_TYPE_WRITE                                        = 1   ,
} MAC1_ENTRY_TYPE;



typedef enum
{
    MAH01_ENTRY_TYPE_READ                                        = 0   ,
    MAH01_ENTRY_TYPE_WRITE                                       = 1   ,
} MAH01_ENTRY_TYPE;



typedef enum
{
    MAH11_ENTRY_TYPE_READ                                        = 0   ,
    MAH11_ENTRY_TYPE_WRITE                                       = 1   ,
} MAH11_ENTRY_TYPE;



typedef enum
{
    MAH21_ENTRY_TYPE_READ                                        = 0   ,
    MAH21_ENTRY_TYPE_WRITE                                       = 1   ,
} MAH21_ENTRY_TYPE;



typedef enum
{
    MAM1_ENTRY_TYPE_READ                                         = 0   ,
    MAM1_ENTRY_TYPE_WRITE                                        = 1   ,
} MAM1_ENTRY_TYPE;



typedef enum
{
    MAL01_ENTRY_TYPE_READ                                        = 0   ,
    MAL01_ENTRY_TYPE_WRITE                                       = 1   ,
} MAL01_ENTRY_TYPE;



typedef enum
{
    MAL11_ENTRY_TYPE_READ                                        = 0   ,
    MAL11_ENTRY_TYPE_WRITE                                       = 1   ,
} MAL11_ENTRY_TYPE;



typedef enum
{
    MAL21_ENTRY_TYPE_READ                                        = 0   ,
    MAL21_ENTRY_TYPE_WRITE                                       = 1   ,
} MAL21_ENTRY_TYPE;



typedef enum
{
    MAG1_ENTRY_TYPE_READ                                         = 0   ,
    MAG1_ENTRY_TYPE_WRITE                                        = 1   ,
} MAG1_ENTRY_TYPE;



typedef enum
{
    MAC2_ENTRY_TYPE_READ                                         = 0   ,
    MAC2_ENTRY_TYPE_WRITE                                        = 1   ,
} MAC2_ENTRY_TYPE;



typedef enum
{
    MAH02_ENTRY_TYPE_READ                                        = 0   ,
    MAH02_ENTRY_TYPE_WRITE                                       = 1   ,
} MAH02_ENTRY_TYPE;



typedef enum
{
    MAH12_ENTRY_TYPE_READ                                        = 0   ,
    MAH12_ENTRY_TYPE_WRITE                                       = 1   ,
} MAH12_ENTRY_TYPE;



typedef enum
{
    MAH22_ENTRY_TYPE_READ                                        = 0   ,
    MAH22_ENTRY_TYPE_WRITE                                       = 1   ,
} MAH22_ENTRY_TYPE;



typedef enum
{
    MAM2_ENTRY_TYPE_READ                                         = 0   ,
    MAM2_ENTRY_TYPE_WRITE                                        = 1   ,
} MAM2_ENTRY_TYPE;



typedef enum
{
    MAL02_ENTRY_TYPE_READ                                        = 0   ,
    MAL02_ENTRY_TYPE_WRITE                                       = 1   ,
} MAL02_ENTRY_TYPE;



typedef enum
{
    MAL12_ENTRY_TYPE_READ                                        = 0   ,
    MAL12_ENTRY_TYPE_WRITE                                       = 1   ,
} MAL12_ENTRY_TYPE;



typedef enum
{
    MAL22_ENTRY_TYPE_READ                                        = 0   ,
    MAL22_ENTRY_TYPE_WRITE                                       = 1   ,
} MAL22_ENTRY_TYPE;



typedef enum
{
    MAG2_ENTRY_TYPE_READ                                         = 0   ,
    MAG2_ENTRY_TYPE_WRITE                                        = 1   ,
} MAG2_ENTRY_TYPE;









typedef enum
{
    MXU_VIDEO_FE_ENGINE_ID_DATA_CACHE_TAG                        = 0   ,
    MXU_VIDEO_FE_ENGINE_ID_INS_CACHE_TAG                         = 1   ,
    MXU_VIDEO_FE_ENGINE_ID_VCP_AES_TAG                           = 2   ,
    MXU_VIDEO_FE_ENGINE_ID_RESERVE0                              = 3   ,
    MXU_VIDEO_FE_ENGINE_ID_VCP_VLD_TAG                           = 4   ,
    MXU_VIDEO_FE_ENGINE_ID_VCP_BUF_TAG                           = 5   ,
    MXU_VIDEO_FE_ENGINE_ID_RESERVE1                              = 6   ,
    MXU_VIDEO_FE_ENGINE_ID_VCP_VP9_PRD_TAG                       = 7   ,
    MXU_VIDEO_FE_ENGINE_ID_RESERVE2                              = 8   ,
    MXU_VIDEO_FE_ENGINE_ID_RESERVE3                              = 9   ,
    MXU_VIDEO_FE_ENGINE_ID_VCP_SIG_TAG                           = 10  ,
    MXU_VIDEO_FE_ENGINE_ID_VCP_VP9_TAG                           = 11  ,
    MXU_VIDEO_FE_ENGINE_ID_RESERVE4                              = 12  ,
    MXU_VIDEO_FE_ENGINE_ID_VCP_DMA_FE_BE_TAG                     = 13  ,
    MXU_VIDEO_FE_ENGINE_ID_RESERVE6                              = 14  ,
    MXU_VIDEO_FE_ENGINE_ID_VCP_COUNT_TAG                         = 15  ,
    MXU_VIDEO_FE_ENGINE_ID_VCP_TOTAL_TAG                         = 16  ,
} MXU_VIDEO_FE_ENGINE_ID;



typedef enum
{
    MXU_VIDEO_BE_ENGINE_ID_DATA_CACHE_TAG                        = 0   ,
    MXU_VIDEO_BE_ENGINE_ID_INS_CACHE_TAG                         = 1   ,
    MXU_VIDEO_BE_ENGINE_ID_BCI_PREFETCH_TAG                      = 2   ,
    MXU_VIDEO_BE_ENGINE_ID_SAO_COEF_TAG                          = 3   ,
    MXU_VIDEO_BE_ENGINE_ID_VCP_VLE_TAG                           = 4   ,
    MXU_VIDEO_BE_ENGINE_ID_VCP_BUF_TAG                           = 5   ,
    MXU_VIDEO_BE_ENGINE_ID_RESERVE0                              = 6   ,
    MXU_VIDEO_BE_ENGINE_ID_RESERVE1                              = 7   ,
    MXU_VIDEO_BE_ENGINE_ID_VCP_PRD_TAG                           = 8   ,
    MXU_VIDEO_BE_ENGINE_ID_VCP_ENC_TAG                           = 9   ,
    MXU_VIDEO_BE_ENGINE_ID_VCP_SIG_TAG                           = 10  ,
    MXU_VIDEO_BE_ENGINE_ID_RESERVE2                              = 11  ,
    MXU_VIDEO_BE_ENGINE_ID_VCP_MSVD_TAG                          = 12  ,
    MXU_VIDEO_BE_ENGINE_ID_VCP_DMA_FE_BE_TAG                     = 13  ,
    MXU_VIDEO_BE_ENGINE_ID_RESERVE3                              = 14  ,
    MXU_VIDEO_BE_ENGINE_ID_VCP_COUNT_TAG                         = 15  ,
    MXU_VIDEO_BE_ENGINE_ID_VCP_TOTAL_TAG                         = 16  ,
} MXU_VIDEO_BE_ENGINE_ID;























typedef enum
{
    VPP_MXU_ENTRY_TYPE_NORMAL                                    = 0   ,
    VPP_MXU_ENTRY_TYPE_SYNC_MMIO_WRITE                           = 1   ,
    VPP_MXU_ENTRY_TYPE_SYNC_CSP_CMD                              = 2   ,
    VPP_MXU_ENTRY_TYPE_SYNC_CSP_REG                              = 3   ,
} VPP_MXU_ENTRY_TYPE;

typedef enum
{
    VPP_MXU_NORMAL_CMD_VPP_MXU_CMD_READ                          = 0   ,
    VPP_MXU_NORMAL_CMD_VPP_MXU_CMD_WRITE                         = 1   ,
    VPP_MXU_NORMAL_CMD_VPP_MXU_CMD_WRITE_BL                      = 2   ,
    VPP_MXU_NORMAL_CMD_VPP_MXU_CMD_CMD                           = 3   ,
} VPP_MXU_NORMAL_CMD;



typedef enum
{
    VIDEO_MXU_ENTRY_TYPE_NORMAL                                  = 0   ,
    VIDEO_MXU_ENTRY_TYPE_SYNC_MMIO_WRITE                         = 1   ,
    VIDEO_MXU_ENTRY_TYPE_SYNC_CSP_CMD                            = 2   ,
    VIDEO_MXU_ENTRY_TYPE_SYNC_CSP_REG                            = 3   ,
} VIDEO_MXU_ENTRY_TYPE;

typedef enum
{
    VIDEO_MXU_NORMAL_CMD_VIDEO_MXU_CMD_READ                      = 0   ,
    VIDEO_MXU_NORMAL_CMD_VIDEO_MXU_CMD_WRITE                     = 1   ,
    VIDEO_MXU_NORMAL_CMD_VIDEO_MXU_CMD_FENCE                     = 2   ,
    VIDEO_MXU_NORMAL_CMD_VIDEO_MXU_BLC                           = 3   ,
    VIDEO_MXU_NORMAL_CMD_VIDEO_MXU_WRITE_BL                      = 4   ,
} VIDEO_MXU_NORMAL_CMD;



typedef enum
{
    ISP_MXU_ENTRY_TYPE_NORMAL                                    = 0   ,
    ISP_MXU_ENTRY_TYPE_SYNC_MMIO_WRITE                           = 1   ,
    ISP_MXU_ENTRY_TYPE_SYNC_CSP_CMD                              = 2   ,
    ISP_MXU_ENTRY_TYPE_SYNC_CSP_REG                              = 3   ,
} ISP_MXU_ENTRY_TYPE;







struct Mxu_Csp_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Tag                                          : 18  ;
    unsigned int    Data[16];
};



struct Mxu_Hub_Read_Tag
{
    unsigned int    Gpc_Id                                       : 4   ;
    unsigned long long int    Addr                               : 34  ;
    unsigned int    Engine_Tag                                   : 23  ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Sector_Idx                                   : 2   ;
    unsigned int    Sector_Mask                                  : 4   ;
    unsigned int    Eid                                          : 3   ;
    unsigned int    Is_Compact                                   : 1   ;
    unsigned int    Msaa_Resolve                                 : 1   ;
    unsigned int    Slice_Id                                     : 2   ;
    unsigned int    Data[16];
};

struct Mxu_Hub_Gm_Flush_Tag
{
    unsigned int    Gpc_Id                                       : 4   ;
    unsigned int    Slice_Id                                     : 3   ;
    unsigned int    Coherent_Mode                                : 1   ;
    unsigned int    Batchgroup_Id                                : 16  ;
    unsigned int    Batchgroup_Seqid                             : 8   ;
    unsigned int    Pe_Id                                        : 1   ;
    unsigned int    Thread_Id                                    : 5   ;
    unsigned int    Shader_Type                                  : 3   ;
    unsigned int    Thread_Mode                                  : 1   ;
    unsigned int    Group_End                                    : 1   ;
};

struct Mxu_Hub_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mxu_Hub_Read_Tag                    Read                       ;
        Mxu_Hub_Gm_Flush_Tag                Gm_Flush                   ;
    };
};



struct Mxu_L2_Ack_Tag
{
    unsigned long long int    Addr                               : 34  ;
    unsigned int    Sub_Block                                    : 1   ;
    unsigned int    Is_Garbage                                   : 1   ;
    unsigned int    Sector_Valid_Mask                            : 4   ;
    unsigned int    Is_Compact                                   : 1   ;
    unsigned int    Cacheline_Id                                 : 10  ;
    unsigned int    Tag                                          : 18  ;
    unsigned int    Is_Sparse                                    : 1   ;
    unsigned int    Data[16];
};



struct Mxu_Mce_Tag
{
    unsigned long long int    Addr                               : 34  ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Msaa_Resolve                                 : 1   ;
    unsigned int    Flag                                         : 1   ;
    unsigned int    Is_Comp                                      : 1   ;
    unsigned int    Sector_Id                                    : 2   ;
    unsigned int    Rtid                                         : 1   ;
    unsigned int    Sector_Mask                                  : 4   ;
    unsigned int    Lane_Idx                                     : 8   ;
    unsigned int    Data[16];
};



struct Mxu_L2_Req_Read_Tag
{
    unsigned int    Proc_Id                                      : 4   ;
    unsigned long long int    Addr                               : 34  ;
    unsigned int    Tag                                          : 18  ;
};

struct Mxu_L2_Req_Command_Tag
{
    unsigned int    Reserved                                     : 23  ;
    unsigned int    End_Flag                                     : 1   ;
    unsigned int    Command                                      : 32  ;
};

struct Mxu_L2_Req_Tag
{
    unsigned int    Entry_Type                                   : 1   ;

    union
    {
        Mxu_L2_Req_Read_Tag                 Read                       ;
        Mxu_L2_Req_Command_Tag              Command                    ;
    };
};



struct Mxu_Bium_Read_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mxu_Bium_Write_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned int    Wmask                                        : 32  ;
    unsigned int    Data[8];
};

struct Mxu_Bium_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mxu_Bium_Read_Tag                   Read                       ;
        Mxu_Bium_Write_Tag                  Write                      ;
    };
};



struct Macb_Read_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Macb_Write_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned int    Wmask                                        : 32  ;
    unsigned int    Data[8];
};

struct Macb_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Macb_Read_Tag                       Read                       ;
        Macb_Write_Tag                      Write                      ;
    };
};



struct Mah0b_Read_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mah0b_Write_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned int    Wmask                                        : 32  ;
    unsigned int    Data[8];
};

struct Mah0b_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mah0b_Read_Tag                      Read                       ;
        Mah0b_Write_Tag                     Write                      ;
    };
};



struct Mah1b_Read_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mah1b_Write_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned int    Wmask                                        : 32  ;
    unsigned int    Data[8];
};

struct Mah1b_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mah1b_Read_Tag                      Read                       ;
        Mah1b_Write_Tag                     Write                      ;
    };
};



struct Mah2b_Read_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mah2b_Write_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned int    Wmask                                        : 32  ;
    unsigned int    Data[8];
};

struct Mah2b_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mah2b_Read_Tag                      Read                       ;
        Mah2b_Write_Tag                     Write                      ;
    };
};



struct Mal0b_Read_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mal0b_Write_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned int    Wmask                                        : 32  ;
    unsigned int    Data[8];
};

struct Mal0b_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mal0b_Read_Tag                      Read                       ;
        Mal0b_Write_Tag                     Write                      ;
    };
};



struct Mal1b_Read_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mal1b_Write_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned int    Wmask                                        : 32  ;
    unsigned int    Data[8];
};

struct Mal1b_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mal1b_Read_Tag                      Read                       ;
        Mal1b_Write_Tag                     Write                      ;
    };
};



struct Mal2b_Read_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mal2b_Write_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned int    Wmask                                        : 32  ;
    unsigned int    Data[8];
};

struct Mal2b_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mal2b_Read_Tag                      Read                       ;
        Mal2b_Write_Tag                     Write                      ;
    };
};



struct Mamb_Read_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mamb_Write_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned int    Wmask                                        : 32  ;
    unsigned int    Data[8];
};

struct Mamb_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mamb_Read_Tag                       Read                       ;
        Mamb_Write_Tag                      Write                      ;
    };
};



struct Magb_Read_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Magb_Write_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned int    Wmask                                        : 32  ;
    unsigned int    Data[8];
};

struct Magb_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Magb_Read_Tag                       Read                       ;
        Magb_Write_Tag                      Write                      ;
    };
};



struct Mxu_Miu_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mxu_Miu_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mxu_Miu_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mxu_Miu_Read_Tag                    Read                       ;
        Mxu_Miu_Write_Tag                   Write                      ;
    };
};



struct Mxu_Csp_Ifence_Tag
{
    unsigned int    Fence_Mode                                   : 1   ;
    unsigned int    Fence_Id                                     : 5   ;
    unsigned int    Fence_Value                                  : 16  ;
};



struct Mac0_Read_Tag
{
    unsigned int    Addr                                         : 30  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mac0_Write_Tag
{
    unsigned int    Addr                                         : 30  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mac0_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mac0_Read_Tag                       Read                       ;
        Mac0_Write_Tag                      Write                      ;
    };
};



struct Mah00_Read_Tag
{
    unsigned int    Addr                                         : 30  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mah00_Write_Tag
{
    unsigned int    Addr                                         : 30  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mah00_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mah00_Read_Tag                      Read                       ;
        Mah00_Write_Tag                     Write                      ;
    };
};



struct Mah10_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mah10_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mah10_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mah10_Read_Tag                      Read                       ;
        Mah10_Write_Tag                     Write                      ;
    };
};



struct Mah20_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mah20_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mah20_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mah20_Read_Tag                      Read                       ;
        Mah20_Write_Tag                     Write                      ;
    };
};



struct Mam0_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mam0_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mam0_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mam0_Read_Tag                       Read                       ;
        Mam0_Write_Tag                      Write                      ;
    };
};



struct Mal00_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mal00_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mal00_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mal00_Read_Tag                      Read                       ;
        Mal00_Write_Tag                     Write                      ;
    };
};



struct Mal10_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mal10_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mal10_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mal10_Read_Tag                      Read                       ;
        Mal10_Write_Tag                     Write                      ;
    };
};



struct Mal20_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mal20_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mal20_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mal20_Read_Tag                      Read                       ;
        Mal20_Write_Tag                     Write                      ;
    };
};



struct Mag0_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mag0_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mag0_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mag0_Read_Tag                       Read                       ;
        Mag0_Write_Tag                      Write                      ;
    };
};



struct Mac1_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mac1_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mac1_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mac1_Read_Tag                       Read                       ;
        Mac1_Write_Tag                      Write                      ;
    };
};



struct Mah01_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mah01_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mah01_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mah01_Read_Tag                      Read                       ;
        Mah01_Write_Tag                     Write                      ;
    };
};



struct Mah11_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mah11_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mah11_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mah11_Read_Tag                      Read                       ;
        Mah11_Write_Tag                     Write                      ;
    };
};



struct Mah21_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mah21_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mah21_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mah21_Read_Tag                      Read                       ;
        Mah21_Write_Tag                     Write                      ;
    };
};



struct Mam1_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mam1_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mam1_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mam1_Read_Tag                       Read                       ;
        Mam1_Write_Tag                      Write                      ;
    };
};



struct Mal01_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mal01_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mal01_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mal01_Read_Tag                      Read                       ;
        Mal01_Write_Tag                     Write                      ;
    };
};



struct Mal11_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mal11_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mal11_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mal11_Read_Tag                      Read                       ;
        Mal11_Write_Tag                     Write                      ;
    };
};



struct Mal21_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mal21_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mal21_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mal21_Read_Tag                      Read                       ;
        Mal21_Write_Tag                     Write                      ;
    };
};



struct Mag1_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mag1_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mag1_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mag1_Read_Tag                       Read                       ;
        Mag1_Write_Tag                      Write                      ;
    };
};



struct Mac2_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mac2_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mac2_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mac2_Read_Tag                       Read                       ;
        Mac2_Write_Tag                      Write                      ;
    };
};



struct Mah02_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mah02_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mah02_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mah02_Read_Tag                      Read                       ;
        Mah02_Write_Tag                     Write                      ;
    };
};



struct Mah12_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mah12_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mah12_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mah12_Read_Tag                      Read                       ;
        Mah12_Write_Tag                     Write                      ;
    };
};



struct Mah22_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mah22_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mah22_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mah22_Read_Tag                      Read                       ;
        Mah22_Write_Tag                     Write                      ;
    };
};



struct Mam2_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mam2_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mam2_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mam2_Read_Tag                       Read                       ;
        Mam2_Write_Tag                      Write                      ;
    };
};



struct Mal02_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mal02_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mal02_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mal02_Read_Tag                      Read                       ;
        Mal02_Write_Tag                     Write                      ;
    };
};



struct Mal12_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mal12_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mal12_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mal12_Read_Tag                      Read                       ;
        Mal12_Write_Tag                     Write                      ;
    };
};



struct Mal22_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mal22_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mal22_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mal22_Read_Tag                      Read                       ;
        Mal22_Write_Tag                     Write                      ;
    };
};



struct Mag2_Read_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 2   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
};

struct Mag2_Write_Tag
{
    unsigned int    Addr                                         : 28  ;
    unsigned int    Bl                                           : 3   ;
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Tag                                          : 14  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Mag2_Tag
{
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Mag2_Read_Tag                       Read                       ;
        Mag2_Write_Tag                      Write                      ;
    };
};



struct Biu_Mxu_Mmio_Tag
{
    unsigned int    Mmio_Mxu_Rw                                  : 1   ;
    unsigned int    Mmio_Mxu_Wdata                               : 32  ;
    unsigned long long int    Mmio_Mxu_Waddr                     : 40  ;
};



struct Mxu_Isp_Tag
{
    unsigned int    Proc_Id                                      : 4   ;
    unsigned long long int    Addr_Debug                         : 34  ;
    unsigned int    Sector_Idx                                   : 2   ;
    unsigned int    Tag                                          : 20  ;
    unsigned int    Data[16];
};



struct Mxu_Video_Tag
{
    unsigned int    Proc_Id                                      : 4   ;
    unsigned long long int    Addr                               : 35  ;
    unsigned long long int    Tag                                : 36  ;
    unsigned int    Data[8];
    unsigned int    Rw_Type                                      : 1   ;
    unsigned int    Sram_Data                                    : 1   ;
    unsigned int    Last_Sector                                  : 1   ;
    unsigned int    Bs_Equal_Zero                                : 1   ;
    unsigned int    Burset_Length                                : 4   ;
    unsigned int    Updata_Tope_Mb                               : 1   ;
    unsigned int    Updata_Cur_Mb                                : 1   ;
    unsigned int    Range_Type                                   : 6   ;
    unsigned int    Burst_Length_Req                             : 4   ;
};



struct Mxu_Video_Fe_Tag
{
    unsigned int    N_Line                                       : 8   ;
    unsigned int    Engine_Id                                    : 4   ;
    unsigned int    B_Qtm                                        : 1   ;
    unsigned int    B_Sram                                       : 1   ;
    unsigned int    B_Bypass                                     : 1   ;
    unsigned int    B_Secure                                     : 1   ;
    unsigned int    N_Draw_Id                                    : 2   ;
    unsigned int    N_Bl_Start                                   : 3   ;
    unsigned int    N_Cur_Bl                                     : 3   ;
    unsigned int    N_Block_Id                                   : 1   ;
    unsigned int    Reserved                                     : 7   ;
    unsigned int    B_Comp_Success                               : 1   ;
    unsigned int    Seq_Id                                       : 3   ;
};



struct Mxu_Video_Be_Tag
{
    unsigned int    N_Line                                       : 8   ;
    unsigned int    Engine_Id                                    : 4   ;
    unsigned int    B_Qtm                                        : 1   ;
    unsigned int    B_Sram                                       : 1   ;
    unsigned int    B_Bypass                                     : 1   ;
    unsigned int    B_Secure                                     : 1   ;
    unsigned int    N_Draw_Id                                    : 2   ;
    unsigned int    N_Bl_Start                                   : 3   ;
    unsigned int    N_Cur_Bl                                     : 3   ;
    unsigned int    N_Block_Id                                   : 1   ;
    unsigned int    Reserved                                     : 7   ;
    unsigned int    B_Comp_Success                               : 1   ;
    unsigned int    Seq_Id                                       : 3   ;
};



struct Mxu_Vpp_Tag
{
    unsigned int    Proc_Id                                      : 4   ;
    unsigned long long int    Addr_Debug                         : 35  ;
    unsigned int    Sector_Idx                                   : 2   ;
    unsigned long long int    Tag                                : 39  ;
    unsigned int    Data[8];
};



struct Biu_Mxu_Tag
{
    unsigned int    Data[8];
};



struct Miu_Mxu_Tag
{
    unsigned int    Data[16];
    unsigned int    Engine_Id                                    : 4   ;
};



struct Miu_Mxu_Pte_Tag
{
    unsigned int    Addr                                         : 30  ;
    unsigned int    Data[16];
    unsigned int    Engine_Id                                    : 4   ;
    unsigned int    Engine_Counter                               : 32  ;
    unsigned int    Max_Counter                                  : 32  ;
    unsigned int    Mdx_Counter                                  : 32  ;
};



struct Comp_Out_Tag
{
    unsigned long long int    Address                            : 35  ;
    unsigned int    Bl                                           : 4   ;
    unsigned int    Sid                                          : 2   ;
    unsigned int    Mask                                         : 32  ;
    unsigned int    Data[8];
    unsigned int    Engine_Id                                    : 4   ;
    unsigned int    Engine_Counter                               : 32  ;
    unsigned int    Comp_In_Counter                              : 32  ;
    unsigned int    Comp_Out_Counter                             : 32  ;
};



struct Dump_Comp_In_Dat_Tag
{
    unsigned long long int    Address                            : 35  ;
    unsigned int    Tag                                          : 6   ;
    unsigned int    Mask                                         : 32  ;
    unsigned int    Data[8];
    unsigned int    Engine_Id                                    : 4   ;
    unsigned int    Engine_Counter                               : 32  ;
    unsigned int    Comp_In_Counter                              : 32  ;
};



struct Dump_Ddec_In_Dat_Tag
{
    unsigned long long int    Tag                                : 53  ;
    unsigned int    Data[8];
    unsigned int    Engine_Id                                    : 4   ;
    unsigned int    Engine_Counter                               : 32  ;
    unsigned int    Ddec_In_Counter                              : 32  ;
};



struct Dump_Ddec_Out_Dat_Tag
{
    unsigned long long int    Tag                                : 43  ;
    unsigned int    Data[8];
    unsigned int    Engine_Id                                    : 4   ;
    unsigned int    Engine_Counter                               : 32  ;
    unsigned int    Ddec_In_Counter                              : 32  ;
    unsigned int    Ddec_Out_Counter                             : 32  ;
};



struct Sec_Access_Tag
{
    unsigned int    Agent                                        : 4   ;
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Rw                                           : 1   ;
    unsigned int    Sec_C3d                                      : 1   ;
    unsigned int    Sec_Mem                                      : 1   ;
    unsigned long long int    Mask                               : 33  ;
    unsigned int    Data[8];
};



struct Mxu_Debugger_Tag
{
    unsigned int    Agent                                        : 4   ;
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Rw                                           : 1   ;
    unsigned long long int    Mask                               : 33  ;
    unsigned int    Data[8];
};



struct Vpp_Mxu_Normal_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Cmd                                          : 2   ;
    unsigned int    Bl                                           : 4   ;
    unsigned long long int    Tag                                : 36  ;
    unsigned int    Range_Type                                   : 4   ;
    unsigned int    Idx_2kb                                      : 23  ;
    unsigned int    Bl_Slot_Index                                : 18  ;
    unsigned int    Wmask                                        : 32  ;
    unsigned int    Data[8];
};

struct Vpp_Mxu_Sync_Mmio_Write_Tag
{
    unsigned long long int    Addr                               : 35  ;
};

struct Vpp_Mxu_Sync_Csp_Cmd_Tag
{
    unsigned long long int    Addr                               : 35  ;
};

struct Vpp_Mxu_Sync_Csp_Reg_Tag
{
    unsigned long long int    Addr                               : 35  ;
};

struct Vpp_Mxu_Tag
{
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Vpp_Mxu_Normal_Tag                  Normal                     ;
        Vpp_Mxu_Sync_Mmio_Write_Tag         Sync_Mmio_Write            ;
        Vpp_Mxu_Sync_Csp_Cmd_Tag            Sync_Csp_Cmd               ;
        Vpp_Mxu_Sync_Csp_Reg_Tag            Sync_Csp_Reg               ;
    };
};



struct Video_Mxu_Normal_Tag
{
    unsigned long long int    Addr                               : 35  ;
    unsigned int    Cmd                                          : 3   ;
    unsigned int    Bl                                           : 4   ;
    unsigned long long int    Tag                                : 36  ;
    unsigned int    Range_Type                                   : 6   ;
    unsigned int    Idx_2kb                                      : 23  ;
    unsigned int    Bl_Slot_Index                                : 18  ;
    unsigned int    Wmask                                        : 32  ;
    unsigned int    Data[8];
};

struct Video_Mxu_Sync_Mmio_Write_Tag
{
    unsigned long long int    Addr                               : 35  ;
};

struct Video_Mxu_Sync_Csp_Cmd_Tag
{
    unsigned long long int    Addr                               : 35  ;
};

struct Video_Mxu_Sync_Csp_Reg_Tag
{
    unsigned long long int    Addr                               : 35  ;
};

struct Video_Mxu_Tag
{
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Video_Mxu_Normal_Tag                Normal                     ;
        Video_Mxu_Sync_Mmio_Write_Tag       Sync_Mmio_Write            ;
        Video_Mxu_Sync_Csp_Cmd_Tag          Sync_Csp_Cmd               ;
        Video_Mxu_Sync_Csp_Reg_Tag          Sync_Csp_Reg               ;
    };
};



struct Isp_Mxu_Normal_Tag
{
    unsigned long long int    Addr                               : 34  ;
    unsigned int    Cmd                                          : 3   ;
    unsigned int    Bl                                           : 4   ;
    unsigned long long int    Tag                                : 36  ;
    unsigned int    Range_Type                                   : 6   ;
    unsigned int    Idx_2kb                                      : 23  ;
    unsigned int    Bl_Slot_Index                                : 18  ;
    unsigned long long int    Wmask                              : 64  ;
    unsigned int    Data[16];
};

struct Isp_Mxu_Sync_Mmio_Write_Tag
{
    unsigned long long int    Addr                               : 34  ;
};

struct Isp_Mxu_Sync_Csp_Cmd_Tag
{
    unsigned long long int    Addr                               : 34  ;
};

struct Isp_Mxu_Sync_Csp_Reg_Tag
{
    unsigned long long int    Addr                               : 34  ;
};

struct Isp_Mxu_Tag
{
    unsigned int    Proc_Id                                      : 4   ;
    unsigned int    Entry_Type                                   : 2   ;

    union
    {
        Isp_Mxu_Normal_Tag                  Normal                     ;
        Isp_Mxu_Sync_Mmio_Write_Tag         Sync_Mmio_Write            ;
        Isp_Mxu_Sync_Csp_Cmd_Tag            Sync_Csp_Cmd               ;
        Isp_Mxu_Sync_Csp_Reg_Tag            Sync_Csp_Reg               ;
    };
};



struct Mxu_Mmio_Data_Tag
{
    unsigned int    Data                                         : 32  ;
};

#endif
