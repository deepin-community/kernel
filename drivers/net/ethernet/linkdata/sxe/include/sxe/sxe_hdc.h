
#ifndef __SXE_HDC_H__
#define __SXE_HDC_H__

#ifdef SXE_HOST_DRIVER
#include "sxe_drv_type.h"
#endif

#define HDC_CACHE_TOTAL_LEN     (16 *1024)    
#define ONE_PACKET_LEN_MAX      (1024)        
#define DWORD_NUM               (256)         
#define HDC_TRANS_RETRY_COUNT   (3)           


typedef enum SxeHdcErrnoCode {
    PKG_OK            = 0,     
    PKG_ERR_REQ_LEN,           
    PKG_ERR_RESP_LEN,          
    PKG_ERR_PKG_SKIP,          
    PKG_ERR_NODATA,            
    PKG_ERR_PF_LK,             
    PKG_ERR_OTHER,
} SxeHdcErrnoCode_e;

typedef union HdcHeader {
    struct {
        U8 pid:4;          
        U8 errCode:4;      
        U8 len;            
        U16 startPkg:1;    
        U16 endPkg:1;      
        U16 isRd:1;        
        U16 msi:1;         
        U16 totalLen:12;   
    } head;
    U32 dw0;
} HdcHeader_u;

#endif 

