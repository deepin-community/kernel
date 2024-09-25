
#ifndef __PS3_META_H__
#define __PS3_META_H__


typedef enum MicPdState {
    MIC_PD_STATE_UNKNOWN = 0,
    MIC_PD_STATE_READY   = 1,       
    MIC_PD_STATE_UBAD    = 2,       
    MIC_PD_STATE_DSPARE  = 3,       
    MIC_PD_STATE_GSPARE  = 4,       
    MIC_PD_STATE_OFFLINE = 5,       
    MIC_PD_STATE_ONLINE  = 6,       
    MIC_PD_STATE_MISSING = 7,       
    MIC_PD_STATE_FAILED  = 8,       
    MIC_PD_STATE_REBUILD = 9,       
    MIC_PD_STATE_REPLACE = 10,      
    MIC_PD_STATE_FOREIGN = 11,      
    MIC_PD_STATE_JBOD    = 12,      
    MIC_PD_STATE_UNSUPPORT = 13,    
    MIC_PD_STATE_PDM       = 14,    
    MIC_PD_STATE_CFSHLD    = 15,    
    MIC_PD_STATE_HSPSHLD   = 16,    
    MIC_PD_STATE_RUNSP     = 17,    
    MIC_PD_STATE_UBUNSP    = 18,    
    MIC_PD_STATE_MAX
}MicPdState_e;


static inline const char *getPdStateName(MicPdState_e pdSate, unsigned char isRaid)
{
    static const char *raidPdName[] = {
        [MIC_PD_STATE_UNKNOWN] = "UNKNOWN",
        [MIC_PD_STATE_READY]   = "UGOOD",
        [MIC_PD_STATE_UBAD]    = "UBAD",
        [MIC_PD_STATE_DSPARE]  = "DSPARE",
        [MIC_PD_STATE_GSPARE]  = "GSPARE",
        [MIC_PD_STATE_OFFLINE] = "OFFLINE",
        [MIC_PD_STATE_ONLINE]  = "ONLINE",
        [MIC_PD_STATE_MISSING] = "MISSING",
        [MIC_PD_STATE_FAILED]  = "FAILED",
        [MIC_PD_STATE_REBUILD] = "REBUILD",
        [MIC_PD_STATE_REPLACE] = "REPLACE",
        [MIC_PD_STATE_FOREIGN] = "FOREIGN",
        [MIC_PD_STATE_JBOD]    = "JBOD",
        [MIC_PD_STATE_UNSUPPORT] = "UNSUPPORT",
        [MIC_PD_STATE_PDM]       = "PDM",
        [MIC_PD_STATE_CFSHLD]    = "CFSHLD",
        [MIC_PD_STATE_HSPSHLD]   = "HSPSHLD",
        [MIC_PD_STATE_RUNSP]     = "UGUNSP",
        [MIC_PD_STATE_UBUNSP]    = "UBUNSP",
    };

    static const char *hbaPdName[] = {
        [MIC_PD_STATE_UNKNOWN] = "UNKNOWN",
        [MIC_PD_STATE_READY]   = "READY",
        [MIC_PD_STATE_UBAD]    = "UBAD",
        [MIC_PD_STATE_DSPARE]  = "DSPARE",
        [MIC_PD_STATE_GSPARE]  = "GSPARE",
        [MIC_PD_STATE_OFFLINE] = "OFFLINE",
        [MIC_PD_STATE_ONLINE]  = "ONLINE",
        [MIC_PD_STATE_MISSING] = "MISSING",
        [MIC_PD_STATE_FAILED]  = "FAILED",
        [MIC_PD_STATE_REBUILD] = "REBUILD",
        [MIC_PD_STATE_REPLACE] = "REPLACE",
        [MIC_PD_STATE_FOREIGN] = "FOREIGN",
        [MIC_PD_STATE_JBOD]    = "JBOD",
        [MIC_PD_STATE_UNSUPPORT] = "UNSUPPORT",
        [MIC_PD_STATE_PDM]       = "PDM",
        [MIC_PD_STATE_CFSHLD]    = "CFSHLD",
        [MIC_PD_STATE_HSPSHLD]   = "HSPSHLD",
        [MIC_PD_STATE_RUNSP]     = "RUNSP",
        [MIC_PD_STATE_UBUNSP]    = "UBUNSP",
    };

    if (isRaid) {
        return raidPdName[pdSate];
    } else {
        return hbaPdName[pdSate];
    }
}


typedef enum MicVdState {
    MIC_VD_STATE_UNKNOWN = 0,
    MIC_VD_STATE_OFFLINE,         
    MIC_VD_STATE_OPTIMAL,         
    MIC_VD_STATE_PARTIAL_DEGRADE, 
    MIC_VD_STATE_DEGRADE          
}MicVdState_e;

static inline const char *getVdStateName(MicVdState_e vdSate)
{
    static const char *vdStateName[] = {
        [MIC_VD_STATE_UNKNOWN]         = "UNKNOWN",
        [MIC_VD_STATE_OFFLINE]         = "OFFLINE",
        [MIC_VD_STATE_OPTIMAL]         = "OPTIMAL",
        [MIC_VD_STATE_PARTIAL_DEGRADE] = "PARTIALLY DEGRADED",
        [MIC_VD_STATE_DEGRADE]         = "DEGRADED"
    };

    return vdStateName[vdSate];
}


typedef enum RaidLevel {
    RAID0 = 0x00,    
    RAID1 = 0x01,    
    RAID5 = 0x05,    
    RAID6 = 0x06,    
    JBOD = 0x0A,     
    RAID10 = 0x10,   
    RAID1E = 0x11,   
    RAID00 = 0x20,   
    RAID50 = 0x50,   
    RAID60 = 0x60,   
    RAID_UNKNOWN = 0xFF
}RaidLevel_e;

typedef enum VDAccessPolicy {
    VD_ACCESS_POLICY_READ_WRITE = 0,
    VD_ACCESS_POLICY_READ_ONLY,     
    VD_ACCESS_POLICY_BLOCK,         
    VD_ACCESS_POLICY_REMOVE_ACCESS  
} VDAccessPolicy_e;


typedef enum VDBgt {
    VD_BGT_IDLE_READY = 0,     
    VD_BGT_FULL_INIT  = 1,     
} VDBgt_e;

#endif
