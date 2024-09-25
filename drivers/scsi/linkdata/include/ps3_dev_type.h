
#ifndef __PS3_DEV_TYPE_H__
#define __PS3_DEV_TYPE_H__

typedef enum DriverType {
    DRIVER_TYPE_UNKNOWN = 0,
    DRIVER_TYPE_SAS,                
    DRIVER_TYPE_SATA,               
    DRIVER_TYPE_SES,                
    DRIVER_TYPE_NVME,               
    DRIVER_TYPE_EXP_SAS,            
    DRIVER_TYPE_VEP,                
    DRIVER_TYPE_CHASSIS,            
    DRIVER_TYPE_BACKPLANE,          
    DRIVER_TYPE_MAX,
} DriverType_e;

static inline const char *getDriverTypeName(DriverType_e driverType)
{
    static const char *driverTypeName[] = {
        [DRIVER_TYPE_UNKNOWN] = "DRIVER_TYPE_UNKNOWN",
        [DRIVER_TYPE_SAS]     = "DRIVER_TYPE_SAS",
        [DRIVER_TYPE_SATA]    = "DRIVER_TYPE_SATA",
        [DRIVER_TYPE_SES]     = "DRIVER_TYPE_SES",
        [DRIVER_TYPE_NVME]    = "DRIVER_TYPE_NVME",
        [DRIVER_TYPE_EXP_SAS] = "DRIVER_TYPE_EXP_SAS",
        [DRIVER_TYPE_VEP]     = "DRIVER_TYPE_VEP",
        [DRIVER_TYPE_CHASSIS] = "DRIVER_TYPE_CHASSIS",
        [DRIVER_TYPE_BACKPLANE] = "DRIVER_TYPE_BACKPLANE",
        [DRIVER_TYPE_MAX]     = "DRIVER_TYPE_MAX",
    };

    return driverTypeName[driverType];
}


typedef enum MediumType {
    DEVICE_TYPE_UNKNOWN = 0,
    DEVICE_TYPE_HDD,               
    DEVICE_TYPE_SSD,               
    DEVICE_TYPE_ENCLOSURE,         
    DEVICE_TYPE_MAX,
} MediumType_e;

static inline const char *getMediumTypeName(MediumType_e mediumType)
{
    static const char *mediumTypeName[] = {
        [DEVICE_TYPE_UNKNOWN]   = "DEVICE_TYPE_UNKNOWN",
        [DEVICE_TYPE_HDD]       = "DEVICE_TYPE_HDD",
        [DEVICE_TYPE_SSD]       = "DEVICE_TYPE_SSD",
        [DEVICE_TYPE_ENCLOSURE] = "DEVICE_TYPE_ENCLOSURE",
        [DEVICE_TYPE_MAX]       = "DEVICE_TYPE_MAX",
    };

    return mediumTypeName[mediumType];
}


typedef enum DiskSpinUpState {
    DISK_SPIN_INVALID = 0,
    DISK_SPIN_UP      = 1,
    DISK_SPIN_TRANS   = 2,
    DISK_SPIN_DOWN    = 3,
} DiskSpinUpState_e;


typedef enum DeviceState {
    DEVICE_STATE_FREE = 0x0,        
    DEVICE_STATE_INSERTING,         
    DEVICE_STATE_ONLINE,            
    DEVICE_STATE_WAIT,              
    DEVICE_STATE_RECOVER,           
    DEVICE_STATE_PREONLINE,         
    DEVICE_STATE_OUTING,            
    DEVICE_STATE_MAX,
} DeviceState_e;

static inline const char *getDeviceStateName(DeviceState_e pdState)
{
    static const char *pdStateName[] = {
        [DEVICE_STATE_FREE]        = "DEVICE_STATE_FREE",
        [DEVICE_STATE_INSERTING]   = "DEVICE_STATE_INSERTING",
        [DEVICE_STATE_ONLINE]      = "DEVICE_STATE_ONLINE",
        [DEVICE_STATE_WAIT]        = "DEVICE_STATE_WAIT",
        [DEVICE_STATE_RECOVER]     = "DEVICE_STATE_RECOVER",
        [DEVICE_STATE_PREONLINE]   = "DEVICE_STATE_PREONLINE",
        [DEVICE_STATE_OUTING]      = "DEVICE_STATE_OUTING",
        [DEVICE_STATE_MAX]         = "DEVICE_STATE_MAX",
    };

    return pdStateName[pdState];
}

#define DEVICE_NOT_RUNNING(state) \
    ((state != DEVICE_STATE_ONLINE)  && (state != DEVICE_STATE_WAIT)   && \
     (state != DEVICE_STATE_RECOVER) && (state != DEVICE_STATE_PREONLINE))

#endif
