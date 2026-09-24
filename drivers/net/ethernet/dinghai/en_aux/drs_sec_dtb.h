/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : drs_sec.dtb.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 :
* 完成日期 : 2024/01/29
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/
#ifndef DRS_SEC_DTB_H
#define DRS_SEC_DTB_H
/*同步pub宏定义*/
typedef void                VOID;
typedef float               FLOAT;
typedef double              DOUBLE;

typedef signed char         INT8;
typedef unsigned char       UINT8;

typedef char                CHAR;


typedef signed short        INT16;
typedef unsigned short      UINT16;

typedef signed int          INT32;
typedef unsigned int        UINT32;


typedef signed long long    INT64;
typedef unsigned long long  UINT64;

#define BITWIDTH1                     ((UINT32)0x00000001)
#define BITWIDTH2                     ((UINT32)0x00000003)
#define BITWIDTH3                     ((UINT32)0x00000007)
#define BITWIDTH4                     ((UINT32)0x0000000f)
#define BITWIDTH5                     ((UINT32)0x0000001f)
#define BITWIDTH6                     ((UINT32)0x0000003f)
#define BITWIDTH7                     ((UINT32)0x0000007f)
#define BITWIDTH8                     ((UINT32)0x000000ff)
#define BITWIDTH9                     ((UINT32)0x000001ff)
#define BITWIDTH10                    ((UINT32)0x000003ff)
#define BITWIDTH11                    ((UINT32)0x000007ff)
#define BITWIDTH12                    ((UINT32)0x00000fff)
#define BITWIDTH13                    ((UINT32)0x00001fff)
#define BITWIDTH14                    ((UINT32)0x00003fff)
#define BITWIDTH15                    ((UINT32)0x00007fff)
#define BITWIDTH16                    ((UINT32)0x0000ffff)
#define BITWIDTH17                    ((UINT32)0x0001ffff)
#define BITWIDTH18                    ((UINT32)0x0003ffff)
#define BITWIDTH19                    ((UINT32)0x0007ffff)
#define BITWIDTH20                    ((UINT32)0x000fffff)
#define BITWIDTH21                    ((UINT32)0x001fffff)
#define BITWIDTH22                    ((UINT32)0x003fffff)
#define BITWIDTH23                    ((UINT32)0x007fffff)
#define BITWIDTH24                    ((UINT32)0x00ffffff)
#define BITWIDTH25                    ((UINT32)0x01ffffff)
#define BITWIDTH26                    ((UINT32)0x03ffffff)
#define BITWIDTH27                    ((UINT32)0x07ffffff)
#define BITWIDTH28                    ((UINT32)0x0fffffff)
#define BITWIDTH29                    ((UINT32)0x1fffffff)
#define BITWIDTH30                    ((UINT32)0x3fffffff)
#define BITWIDTH31                    ((UINT32)0x7fffffff)
#define BITWIDTH32                    ((UINT32)0xffffffff)



#define PUB_OK                          (0)
#define PUB_ERROR                       (0xffffffff)/*直接定义为0xffffffff*/

#define BTTL_PRINTF(fmt, arg...)               DH_LOG_INFO(MODULE_SEC, fmt, ##arg)
#define BTTL_PUB_PRINT_ERROR(fmt, arg...)      DH_LOG_ERR(MODULE_SEC, fmt, ##arg)

#define BTTL_PRINTF_DEV(dev, fmt, arg...)               DH_LOG_INFO_DEV(MODULE_SEC, dev, fmt, ##arg)
#define BTTL_PUB_PRINT_ERROR_DEV(dev, fmt, arg...)      DH_LOG_ERR_DEV(MODULE_SEC, dev, fmt, ##arg)


/* 寄存器单bit位操作 */

/** 某bit置位，其它bit不变 */
#define PUB_BIT_SET(reg, bit) ((reg) = ((reg) | (1u << (bit))))

/** 某bit清零，其它bit不变 */
#define PUB_BIT_CLEAR(reg, bit) ((reg) = ((reg) & (~(1u << (bit)))))

/** 获取某bit的值 (0/1) */
#define PUB_GET_BIT_VAL(reg, bit) (((reg)>> (bit)) & 1u)

/** 判断某bit的值是否为1 */
#define PUB_IS_BIT_SET(reg, pos) (((reg) & (1u << (pos))) != 0x0u)

/** 判断某bit的值是否为0 */
#define PUB_IS_BIT_CLEAR(reg, pos) (((reg) & (1u << (pos))) == 0x0u)

/** 某bit位填写值val,其他bit不变 */
#define PUB_BIT_INSR(reg, bit, val)                                       \
    ((reg) = (((reg) & (~(1u << (bit)))) | (((val) & 1u) << (bit))))


#define PUB_BIT_FIELD_MASK_GET64(bitoff, bitfieldlen) \
((((UINT64)0x01 << (bitfieldlen)) - 1) << (bitoff))

#define PUB_BIT_FIELD_GET64(val, bitoff, bitfieldlen) \
((val) & PUB_BIT_FIELD_MASK_GET64(bitoff, bitfieldlen))

#define PUB_BIT_FIELD_SET64(var, val, bitoff, bitlen) \
((var) = (((var) & (~ PUB_BIT_FIELD_MASK_GET64(bitoff, bitlen))) | (((UINT64)val) << (bitoff))))

#define PUB_BIT_FIELD_RIGHT_JUST_GET64(val, bitoff, bitfieldlen) \
(((val) >> (bitoff)) & (((UINT64)0x01 << (bitfieldlen)) - 1))

/** 检查空指针，返回错误 */
#define PUB_CHECK_NULL_PTR_RET_ERR(ptr)  \
    do{\
        if(NULL == ptr){\
            DH_LOG_INFO(MODULE_SEC, "Null Ptr Err! Fuc:%s,Line:%d,File:%s\n", __FUNCTION__,__LINE__,__FILE__);\
            return PUB_ERROR;\
        }\
    }while(0)

/** 检查空指针，返回VOID */
#define PUB_CHECK_NULL_PTR_RET_VOID(ptr)  \
    do{\
        if(NULL == ptr){\
            DH_LOG_INFO(MODULE_SEC, "Null Ptr Err! Fuc:%s,Line:%d,File:%s\n", __FUNCTION__,__LINE__,__FILE__);\
            return;\
        }\
    }while(0)

#define PUB_CHECK_RET_VAL_RV(expr) \
    do {    \
        UINT32 _ret = (expr);    \
        if (PUB_OK != _ret) \
        {   \
            DH_LOG_INFO(MODULE_SEC, "%s Error,Line:%d,Ret:0x%x\n", __FUNCTION__,__LINE__,_ret);   \
            return _ret;    \
        }   \
    } while (0)

#define BTTL_PUB_ID_CHECK(id, cmpid)  \
    do{\
        if(cmpid <= (id)){\
            DH_LOG_INFO(MODULE_SEC, " ID %d <= %d check Err! Fuc:%s,Line:%d,File:%s\n", id, cmpid, __FUNCTION__,__LINE__,__FILE__);\
            return 1;\
        }\
    }while(0)

#define BTTL_PUB_0_CHECK(value)   \
    do{\
        if(0 == (value)){\
            DH_LOG_INFO(MODULE_SEC, " value %x 0 check Err! Fuc:%s,Line:%d,File:%s\n", value, __FUNCTION__,__LINE__,__FILE__);\
            return E_INVALID_VALUE;\
        }\
    }while(0)

/* 大小端操作 */
/** 16位数据大小端转换 */
#define PUB_SWAP16(x)  ((UINT16)((((x) >> 8) & 0xffu) | (((x) & 0xffu) << 8)))
/** 32位数据大小端转换 */
#define PUB_SWAP32(x) \
    ((UINT32)( \
        (((UINT32)(x) & (UINT32)0x000000ffUL) << 24) | \
        (((UINT32)(x) & (UINT32)0x0000ff00UL) <<  8) | \
        (((UINT32)(x) & (UINT32)0x00ff0000UL) >>  8) | \
        (((UINT32)(x) & (UINT32)0xff000000UL) >> 24) ))
/** 64位数据大小端转换 */
#define PUB_SWAP64(x) \
    ((UINT64)( \
        (((UINT64)(x) & (UINT64)0x00000000000000ffUL) << 56) | \
        (((UINT64)(x) & (UINT64)0x000000000000ff00UL) << 40) | \
        (((UINT64)(x) & (UINT64)0x0000000000ff0000UL) << 24) | \
        (((UINT64)(x) & (UINT64)0x00000000ff000000UL) << 8 ) | \
        (((UINT64)(x) & (UINT64)0x000000ff00000000UL) >> 8 ) | \
        (((UINT64)(x) & (UINT64)0x0000ff0000000000UL) >> 24) | \
        (((UINT64)(x) & (UINT64)0x00ff000000000000UL) >> 40) | \
        (((UINT64)(x) & (UINT64)0xff00000000000000UL) >> 56) ))


/* 已知数据的大小端，转换为网络序 */
#define PUB_LE_TO_NET16(x)      PUB_SWAP16(x)   /**< 将小端数据转换为网络序 */
#define PUB_LE_TO_NET32(x)      PUB_SWAP32(x)   /**< 将小端数据转换为网络序 */
#define PUB_LE_TO_NET64(x)      PUB_SWAP64(x)   /**< 将小端数据转换为网络序 */
#define PUB_DE_TO_NET16(x)      (x)             /**< 将大端数据转换为网络序 */
#define PUB_DE_TO_NET32(x)      (x)             /**< 将大端数据转换为网络序 */
#define PUB_DE_TO_NET64(x)      (x)             /**< 将大端数据转换为网络序 */


#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

#define PUB_LE_TO_HOST16(x)     PUB_SWAP16(x)   /**< 小端16位数据转换为主机序 */
#define PUB_LE_TO_HOST32(x)     PUB_SWAP32(x)   /**< 小端32位数据转换为主机序 */
#define PUB_LE_TO_HOST64(x)     PUB_SWAP64(x)   /**< 小端64位数据转换为主机序 */
#define PUB_DE_TO_HOST16(x)     (x)             /**< 大端16位数据转换为主机序 */
#define PUB_DE_TO_HOST32(x)     (x)             /**< 大端32位数据转换为主机序 */
#define PUB_DE_TO_HOST64(x)     (x)             /**< 大端64位数据转换为主机序 */
#define PUB_HTON16(x)           (x)             /**< 16位数据主机序转换为网络序 */
#define PUB_HTON32(x)           (x)             /**< 32位数据主机序转换为网络序 */
#define PUB_HTON64(x)           (x)             /**< 64位数据主机序转换为网络序 */
#define PUB_NTOH16(x)           (x)             /**< 16位数据网络序转换为主机序 */
#define PUB_NTOH32(x)           (x)             /**< 32位数据网络序转换为主机序 */
#define PUB_NTOH64(x)           (x)             /**< 64位数据网络序转换为主机序 */


#else
#define PUB_LE_TO_HOST16(x)     (x)
#define PUB_LE_TO_HOST32(x)     (x)
#define PUB_LE_TO_HOST64(x)     (x)
#define PUB_DE_TO_HOST16(x)     PUB_SWAP16(x)
#define PUB_DE_TO_HOST32(x)     PUB_SWAP32(x)
#define PUB_DE_TO_HOST64(x)     PUB_SWAP64(x)
#define PUB_HTON16(x)           PUB_SWAP16(x)
#define PUB_HTON32(x)           PUB_SWAP32(x)
#define PUB_HTON64(x)           PUB_SWAP64(x)
#define PUB_NTOH16(x)           PUB_SWAP16(x)
#define PUB_NTOH32(x)           PUB_SWAP32(x)
#define PUB_NTOH64(x)           PUB_SWAP64(x)


#endif

#if 0
/*因为SEC下表和NP下表硬件基本一样,这里同步NP关于EPID等定义*/
#define VF_ACTIVE(VPORT)            ((VPORT & 0x0800) >> 11)
#define EPID(VPORT)                 ((VPORT & 0x7000) >> 12)
#define FUNC_NUM(VPORT)             ((VPORT & 0x0700) >> 8)
#define VFUNC_NUM(VPORT)            ((VPORT & 0x00FF))

#define PF_VQM_VFID_OFFSET          (1152)
#define IS_PF(VPORT)                (!VF_ACTIVE(VPORT))
#define VQM_VFID(VPORT)             (IS_PF(VPORT) ? \
                                    (PF_VQM_VFID_OFFSET + EPID(VPORT) * 8 + FUNC_NUM(VPORT)) : \
                                    (EPID(VPORT) * 256 + VFUNC_NUM(VPORT)))

#define OWNER_PF_VQM_VFID(VPORT)    (PF_VQM_VFID_OFFSET + EPID(VPORT) * 8 + FUNC_NUM(VPORT))
#define OWNER_PF_VPORT(VPORT)       (((EPID(VPORT)) << 12) | ((FUNC_NUM(VPORT)) << 8))

#define VQM_VFID_MAX_NUM            (2048)
#endif

/*vport格式
15 |14 13 12 |     11    |10  9  8|7 6 5 4 3 2 1 0|
rsv| ep_id   |func_active|func_num|    vfunc_num  |
*/
#define VPORT_EPID_BT_START          (12)  /*EPID起始位*/
#define VPORT_EPID_BT_LEN            (3)  /*EPID长度*/
#define VPORT_FUNC_ACTIVE_BT_START   (11)  /*FUNC_ACTIVE起始位*/
#define VPORT_FUNC_ACTIVE_BT_LEN     (1)  /*FUNC_ACTIVE长度*/
#define VPORT_FUNC_NUM_BT_START      (8)  /*FUNC_NUM起始位*/
#define VPORT_FUNC_NUM_BT_LEN        (3)  /*FUNC_NUM长度*/
#define VPORT_VFUNC_NUM_BT_START     (0)  /*FUNC_NUM起始位*/
#define VPORT_VFUNC_NUM_BT_LEN       (8)  /*FUNC_NUM长度*/

/**
* @name 通用寄存器操作宏
* @brief 读寄存器宏定义
* @{
*/
#define PUB_READ_REG8(addr)                 (*(volatile UINT8 *)(addr))   /**< 读8位寄存器 */
#define PUB_READ_REG16(addr)                (*(volatile UINT16 *)(addr))  /**< 读16位寄存器 */
#define PUB_READ_REG32(addr)                (*(volatile UINT32 *)(addr))  /**< 读32位寄存器 */
/** @} 通用寄存器操作宏 */

/**
* @name 通用寄存器操作宏
* @brief 写寄存器宏定义
* @{
*/
#define PUB_WRITE_REG8(addr, val_8)         (*(volatile UINT8 *)(addr) = val_8)    /**< 写8位寄存器 */
#define PUB_WRITE_REG16(addr, val_16)       (*(volatile UINT16 *)(addr) = val_16)  /**< 写16位寄存器 */
#define PUB_WRITE_REG32(addr, val_32)       (*(volatile UINT32 *)(addr) = val_32)  /**< 写32位寄存器 */
/** @} 通用寄存器操作宏 */


/*寄存器偏移定义*/
#define REG_SEC_IDX_OFFSET             (0x800000)        /* SEC内部基地址偏移 */

#define REG_SEC_TOP_DTB_OFFSET         (0)        /*host驱动 这里为0,因为就是从dtb开始映射的*/
/* CFG_QUEUE_DTB_ADDR_H_0_127 虚机队列入队的高地址寄存器 n=0~127 */
#define REG_SEC_CFG_QUEUE_DTB_ADDR_H_0_127(n)     (REG_SEC_TOP_DTB_OFFSET + 0x0000 + n*32)

/* CFG_QUEUE_DTB_ADDR_L_0_127 虚机队列入队的低地址寄存器 n=0~127*/
#define REG_SEC_CFG_QUEUE_DTB_ADDR_L_0_127(n)     (REG_SEC_TOP_DTB_OFFSET + 0x0004 + n*32)

/* CFG_QUEUE_DTB_LEN_0_127 虚机队列入队的长度寄存器 n=0~127*/
#define REG_SEC_CFG_QUEUE_DTB_LEN_0_127(n)        (REG_SEC_TOP_DTB_OFFSET + 0x0008 + n*32)

/* INFO_QUEUE_BUF_SPACE_LEFT_0_127 靠靠靠靠靠靠?n=0~127*/
#define REG_SEC_INFO_QUEUE_BUF_SPACE_LEFT_0_127(n) (REG_SEC_TOP_DTB_OFFSET + 0x000C + n*32)

/* CFG_EPID_V_FUNC_NUM_0_127 SOC虚机信息配置寄存器 n=0~127*/
#define REG_SEC_CFG_EPID_V_FUNC_NUM_0_127(n)       (REG_SEC_TOP_DTB_OFFSET + 0x0010 + n*32)

/* DTB_QUEUE_LOCK_STATE_0_3 队列锁状态寄存器,4个寄存器共128bit，对应队列0~127 n=0~3 */
#define REG_SEC_DTB_QUEUE_LOCK_STATE_0_3(n)        (REG_SEC_TOP_DTB_OFFSET + 0x4080 + n*4)

typedef enum
{
    e_SEC_IPSEC_TRANSPORT_MODE = 0,  /*传输模式*/
    e_SEC_IPSEC_TUNNEL_MODE,        /*隧道模式*/
    e_SEC_IPSEC_MODE_LAST,
} E_CMDK_SEC_IPSEC_MODE;

typedef enum
{
    e_SEC_SA_DF_BYPASS_MODE = 0,        /*00 bypass DF bit*/
    e_SEC_SA_DF_CLEAR_MODE,             /*01 clear*/
    e_SEC_SA_DF_SET_MODE,               /*10 set*/
    e_SEC_SA_DF_COPY_MODE,              /*11 copy*/
    e_SEC_SA_DF_MODE_LAST,
} E_CMDK_SEC_SA_DF_MODE;

typedef enum
{
    E_DTB_SA_CMD_FLOW_DOWN = 0,
    E_DTB_SA_CMD_DUMP,
    E_DTB_SA_CMD_LAST,
} E_CMDK_DTB_SA_CMD_TYPE;

typedef enum
{
    E_SATYPE_IN  = 1,
    E_SATYPE_OUT ,
    E_SATYPE_IN_AND_OUT = 3,
}E_SA_TYPE;

typedef enum
{
    E_INLINE_IN,
    E_INLINE_OUT,
    E_INLINE_IN_AND_OUT,
}E_INLINE_TYPE;

typedef enum
{
    e_SEC_ENCRYP_AH_MODE = 0,                       /*000 AH认证*/
    e_SEC_ENCRYP_ESP_AUTH_MODE,                     /*001 ESP完整性*/
    e_SEC_ENCRYP_ESP_ENCRYP_MODE,                   /*010 ESP加密*/
    e_SEC_ENCRYP_ESP_AUTH_AND_ESP_ENCRYP_MODE,      /*011 ESP加密+ESP完整*/
    e_SEC_ENCRYP_ESP_COMBINED_MODE,                /*100 ESP组合模式*/
    e_SEC_ENCRYP_MODE_LAST,
} E_CMDK_SEC_ENCRYP_MODE;

typedef enum
{
    e_SEC_SA_LIVETIME_NONE_TYPE = 0,             /*00 none*/
    e_SEC_SA_LIVETIME_TIME_TYPE,                 /*01 生存时间*/
    e_SEC_SA_LIVETIME_BYTE_TYPE,                 /*10 byte数*/
    e_SEC_SA_LIVETIME_PKT_TYPE,                 /*11 pkt数(预留,目前不支持)*/
    e_SEC_SA_LIVETIME_TYPE_LAST,
} E_CMDK_LIVETIME_TYPES;

#pragma pack(1)
typedef struct IPV4_HEAD
{
    UINT8 ip_headlen_version;
    UINT8 ip_tos;
    UINT16 usTotallen;

    UINT16 usIdentify;
    UINT16 ip_fragoff;

    UINT8 uclive_time;
    UINT8 ucProtocal;
    UINT16 usHeadChecksum;

    UINT32  udSrcIpAddr;
    UINT32  udDstIpAddr;
}T_IPV4_HEAD;
#pragma pack()

typedef struct
{
   UINT32  DtbAddrH;  /*地址的高32位*/
   UINT32  DtbAddrL;  /*地址的低32位,两个地址组成64位然后左移4位得到68位的真实地址*/
   UINT32  DtbCmd;    /*研规上的DTB_LEN字段   */
}T_QUEUE_DTB_REG;

//SA下表模块使用的结构体
typedef struct
{
  UINT32   udSPI;
  UINT32   udSaId;
  UINT16   usSaParam;
  UINT8    ucCiperID;
  UINT8    ucAuthID;
  UINT8    ucCipherkeyLen;
  UINT8    ucAuthkeyLen;
  UINT16   usFrag_State;

  UINT32   udESN;
  UINT32   udSN;
  UINT64   uddProcessedByteCnt;

  UINT32   udSalt;
  UINT32   udLifetimeSecMax;
  UINT64   uddLifetimByteCntMax;

  UINT8    ucProtocol;
  UINT8    ucTOS;
  UINT8    ucEsnFlag;
  UINT8    ucIpType;
  UINT32   udRSV0;
  UINT32   udRSV1;
  UINT32   udRSV2;

  UINT32   udSrcAddress0;
  UINT32   udSrcAddress1;
  UINT32   udSrcAddress2;
  UINT32   udSrcAddress3;

  UINT32   udDstAddress0;
  UINT32   udDstAddress1;
  UINT32   udDstAddress2;
  UINT32   udDstAddress3;

  UINT8   aucSaCipherKey[32];
  UINT8   aucSaAuthKey[128];
}__attribute__((packed))T_HAL_SA_DTB_HW_OUT;

 typedef struct
{

  UINT32   udSrcAddress0;
  UINT32   udSrcAddress1;
  UINT32   udSrcAddress2;
  UINT32   udSrcAddress3;

  UINT32   udDstAddress0;
  UINT32   udDstAddress1;
  UINT32   udDstAddress2;
  UINT32   udDstAddress3;

  UINT32   udSPI;
  UINT32   udSaId;
  UINT16   usSaParam;
  UINT8    ucCiperID;
  UINT8    ucAuthID;
  UINT8    ucCipherkeyLen;
  UINT8    ucAuthkeyLen;
  UINT16   usFrag_State;

  UINT32   udSalt;
  UINT32   udLifetimeSecMax;
  UINT64   uddLifetimByteCntMax;

  UINT8    ucProtocol;
  UINT8    ucTOS;
  UINT8    ucEsnFlag;
  UINT8    ucIpType;
  UINT16   usOutSaOffset;
  UINT16   udRSV0;
  UINT32   udOutSaId;
  UINT32   udRSV1;

  UINT8    aucBitmap[256];

  UINT32   udAntiWindowHigh;
  UINT32   udAntiWindowLow;
  UINT64   uddProcessedByteCnt;

  UINT8    aucSaCipherKey[32];
  UINT8    aucSaAuthKey[128];
}__attribute__((packed))T_HAL_SA_DTB_HW_IN;

typedef enum
{
    e_HAL_IPSEC_CIPHER_NULL     = 0x00,
    e_HAL_IPSEC_CIPHER_AES_CTR  = 0x11,
    e_HAL_IPSEC_CIPHER_AES_CBC  = 0x12,
    e_HAL_IPSEC_CIPHER_AES_ECB  = 0x13,
    e_HAL_IPSEC_CIPHER_AES_GCM  = 0x14,
    e_HAL_IPSEC_CIPHER_AES_CCM  = 0x15,
    e_HAL_IPSEC_CIPHER_AES_GMAC = 0x16,
    /* 新增SM4算法 */
    e_HAL_IPSEC_CIPHER_SM4_CTR  = 0x17,
    e_HAL_IPSEC_CIPHER_SM4_CBC  = 0x18,
    e_HAL_IPSEC_CIPHER_SM4_ECB  = 0x19,
    /* 新增XTS算法 */
    e_HAL_IPSEC_CIPHER_AES_XTS  = 0x1a,
    e_HAL_IPSEC_CIPHER_SM4_XTS  = 0x1b,

    e_HAL_IPSEC_CIPHER_DES_CBC  = 0x31,
    e_HAL_IPSEC_CIPHER_3DES_CBC = 0x32,
    e_HAL_IPSEC_CIPHER_CHACHA   = 0x50,
}E_HAL_SEC_IPSEC_CIPHER_ALG;

typedef enum
{
    e_HAL_IPSEC_AUTH_NULL        = 0x00,

    /* 新增 */
    e_HAL_IPSEC_AUTH_AES_GMAC    = 0x16,      /* 1 */
    e_HAL_IPSEC_AUTH_SM4_GMAC    = 0x1e,

    e_HAL_IPSEC_AUTH_AES_CMAC32  = 0x22,      /* 3 */
    e_HAL_IPSEC_AUTH_AES_CMAC96  = 0x23,
    e_HAL_IPSEC_AUTH_AES_XCBCMAC = 0x21,
    e_HAL_IPSEC_AUTH_AES_SHA1    = 0x41,      /* 6 */
    e_HAL_IPSEC_AUTH_AES_SHA224  = 0x42,
    e_HAL_IPSEC_AUTH_AES_SHA256  = 0x44,
    e_HAL_IPSEC_AUTH_AES_SHA384  = 0x45,
    e_HAL_IPSEC_AUTH_AES_SHA512  = 0x46,
    e_HAL_IPSEC_AUTH_AES_MD5     = 0x43,
    e_HAL_IPSEC_AUTH_SM3         = 0x47,
}E_HAL_SEC_IPSEC_AUTH_ALG;

typedef struct
{
    char alg_name[64];
    char compat_name[64];
    E_HAL_SEC_IPSEC_CIPHER_ALG e_zxdh_ealgo_id;
}T_ZXDH_EALGO;

typedef struct
{
    char alg_name[64];
    char compat_name[64];
    E_HAL_SEC_IPSEC_AUTH_ALG e_zxdh_auth_id;
}T_ZXDH_ALGO;


void BttlPubDump(unsigned char *ucBuf, UINT32 udLen);
UINT32 CmdkBttlTestSecDtbSaAdd(struct zxdh_en_device *en_dev,E_CMDK_DTB_SA_CMD_TYPE eDtbSaCmdType,E_SA_TYPE eSaType,UINT64 uddSaVirAddr,UINT32 udDtbSaIsIntEn,UINT32 udDtbLen,UINT32 udQueIndex);
void zxdh_ipsec_del_sa(struct xfrm_state *xs);
bool zxdh_ipsec_offload_ok(struct sk_buff *skb, struct xfrm_state *xs);
void zxdh_ipsec_state_advance_esn (struct xfrm_state *x);
void zxdh_ipsec_state_update_curlft (struct xfrm_state *x);
int  zxdh_ipsec_policy_add (struct xfrm_policy *x);
void zxdh_ipsec_policy_delete (struct xfrm_policy *x);
void zxdh_ipsec_policy_free (struct xfrm_policy *x);

#endif

