#include <linux/device.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/dcache.h>
#include <linux/mm.h>
#include <linux/memory.h>
#include <linux/init.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/stat.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>

#include <linux/netdevice.h>
#include <linux/pfkeyv2.h>
#include <net/xfrm.h>
#include "driver.h"
#include "../en_aux.h"
#include "drs_sec_dtb.h"



UINT32 g_udDownloadSaNum = 1;     //sa表的数量
UINT32 gudTunnelID = 0;
UINT32 gudDtbSaNum = 1;
E_INLINE_TYPE e_gInlineType = 0;  //0是inline入境 1是inline出境
UINT64 guddAntiWindow = 2047;   //得配成2047，否则覆盖不到sn为0的情况

UINT64 guddSecTestSaDtbPdVirAddr = 0;

UINT32 gudSecTestSwanSrcIp = 0x0A04B007;
UINT32 gudSecTestSwanDstIp = 0x0AE3656D;

UINT8  gudIpType = 1;
// 出入境分开下表需要将此字段置0，会影响入境下表
UINT16 gusOutSaOffset=0;
UINT32 gudOutSaId=0;


UINT64 HalBttlSecRegBaseGet(struct zxdh_en_device *en_dev)
{
    PUB_CHECK_NULL_PTR_RET_ERR(en_dev);
    return en_dev->ops->get_bar_virt_addr(en_dev->parent, 0) + 0x7000;   //0x7000是目前sec模块寄存器基地址的固定偏移,包括PF/VF
}


#if 1
static int zxdh_ipsec_cipher_id_get(u8 ealgo, char* p_alg_name,char* p_aead_name,E_HAL_SEC_IPSEC_CIPHER_ALG *p_zxdh_ealgo_id)
{
    int i = 0;
    T_ZXDH_EALGO atZxdhEalgo[] =
    {
        {"rfc7539esp(chacha20,poly1305)","",e_HAL_IPSEC_CIPHER_CHACHA},
    };

    if((NULL == p_alg_name)||(NULL == p_aead_name))
    {
        return -1;
    }
    for(i=0;i<sizeof(atZxdhEalgo)/sizeof(T_ZXDH_EALGO);i++)
    {
        if((0 == strcmp(p_alg_name,atZxdhEalgo[i].alg_name))||(0 == strcmp(p_alg_name,atZxdhEalgo[i].compat_name)))
        {
            *p_zxdh_ealgo_id = atZxdhEalgo[i].e_zxdh_ealgo_id;
            return 0;
        }
        if((0 == strcmp(p_aead_name,atZxdhEalgo[i].alg_name))||(0 == strcmp(p_aead_name,atZxdhEalgo[i].compat_name)))
        {
            *p_zxdh_ealgo_id = atZxdhEalgo[i].e_zxdh_ealgo_id;
            return 0;
        }
    }

    switch (ealgo) {
    case SADB_EALG_NULL:
    case SADB_EALG_NONE:
        *p_zxdh_ealgo_id = e_HAL_IPSEC_CIPHER_NULL;
        break;
    case SADB_EALG_DESCBC:
        *p_zxdh_ealgo_id = e_HAL_IPSEC_CIPHER_DES_CBC;
        break;
    case SADB_EALG_3DESCBC:
        *p_zxdh_ealgo_id = e_HAL_IPSEC_CIPHER_3DES_CBC;
        break;
    case SADB_X_EALG_AESCBC:
        *p_zxdh_ealgo_id = e_HAL_IPSEC_CIPHER_AES_CBC;
        break;
    case SADB_X_EALG_AESCTR:
        *p_zxdh_ealgo_id = e_HAL_IPSEC_CIPHER_AES_CTR;
        break;
    case SADB_X_EALG_AES_CCM_ICV8:
        *p_zxdh_ealgo_id = e_HAL_IPSEC_CIPHER_AES_CCM;
        break;
    case SADB_X_EALG_AES_CCM_ICV12:
        *p_zxdh_ealgo_id = e_HAL_IPSEC_CIPHER_AES_CCM;
        break;
    case SADB_X_EALG_AES_CCM_ICV16:
        *p_zxdh_ealgo_id = e_HAL_IPSEC_CIPHER_AES_CCM;
        break;
    case SADB_X_EALG_AES_GCM_ICV8:
        *p_zxdh_ealgo_id = e_HAL_IPSEC_CIPHER_AES_GCM;
	break;
    case SADB_X_EALG_AES_GCM_ICV12:
        *p_zxdh_ealgo_id = e_HAL_IPSEC_CIPHER_AES_GCM;
        break;
    case SADB_X_EALG_AES_GCM_ICV16:
        *p_zxdh_ealgo_id = e_HAL_IPSEC_CIPHER_AES_GCM;
        break;
    case SADB_X_EALG_NULL_AES_GMAC:
        *p_zxdh_ealgo_id = e_HAL_IPSEC_CIPHER_AES_GMAC;
        break;
        #if 0
        //5.13不支持
    case SADB_X_EALG_SM4CBC:
        *p_zxdh_ealgo_id = e_HAL_IPSEC_CIPHER_SM4_CBC;
        break;
        #endif
    default:
        return -1;
    }

    return 0;
}

static int zxdh_ipsec_auth_id_get(u8 aalgo,char* p_alg_name,E_HAL_SEC_IPSEC_AUTH_ALG *p_zxdh_auth_id)
{
    int i = 0;
    T_ZXDH_ALGO atZxdhAlgo[] =
    {
        {"cmac(aes)","",e_HAL_IPSEC_AUTH_AES_CMAC32},
    };

    if(NULL == p_alg_name)
    {
        return -1;
    }
    for(i=0;i<sizeof(atZxdhAlgo)/sizeof(T_ZXDH_ALGO);i++)
    {
        DH_LOG_INFO(MODULE_SEC, "p_alg_name %s\n", p_alg_name);
        DH_LOG_INFO(MODULE_SEC, "atZxdhAlgo[%d].alg_name %s\n", i, atZxdhAlgo[i].alg_name);
        if((0 == strcmp(p_alg_name,atZxdhAlgo[i].alg_name))||(0 == strcmp(p_alg_name,atZxdhAlgo[i].compat_name)))
        {
            *p_zxdh_auth_id = atZxdhAlgo[i].e_zxdh_auth_id;
            return 0;
        }
    }

    switch (aalgo) {
    case SADB_X_AALG_NULL:
    case SADB_AALG_NONE:
        *p_zxdh_auth_id = e_HAL_IPSEC_AUTH_NULL;
        break;
    case SADB_AALG_MD5HMAC:
        *p_zxdh_auth_id = e_HAL_IPSEC_AUTH_AES_MD5;
        break;
    case SADB_AALG_SHA1HMAC:
        *p_zxdh_auth_id = e_HAL_IPSEC_AUTH_AES_SHA1;
        break;
    case SADB_X_AALG_SHA2_256HMAC:
        *p_zxdh_auth_id = e_HAL_IPSEC_AUTH_AES_SHA256;
        break;
    case SADB_X_AALG_SHA2_384HMAC:
        *p_zxdh_auth_id = e_HAL_IPSEC_AUTH_AES_SHA384;
        break;
    case SADB_X_AALG_SHA2_512HMAC:
        *p_zxdh_auth_id = e_HAL_IPSEC_AUTH_AES_SHA512;
        break;
    case SADB_X_AALG_AES_XCBC_MAC:
        *p_zxdh_auth_id = e_HAL_IPSEC_AUTH_AES_XCBCMAC;
        break;
        #if 0
        //5.13不支持
    case SADB_X_AALG_SM3_256HMAC:
        *p_zxdh_auth_id = e_HAL_IPSEC_AUTH_SM3;    /*不清楚对不对*/
        break;
        #endif
    default:
        return -1;
    }

    return 0;
}

UINT32 CmdkBttlSecSaParamConstruct(UINT32 udEntryValid,E_CMDK_SEC_IPSEC_MODE eTunnelMode,UINT32 udSeqCnterOverflow,E_CMDK_LIVETIME_TYPES eLiveTimeType,E_CMDK_SEC_SA_DF_MODE eSaDfMode,E_CMDK_SEC_ENCRYP_MODE eEncryptionMode,UINT32 udIcvLen,UINT16* pusSaParam)
{
    UINT32 udIcvLenNew = udIcvLen / 4;  //以4字节为单位

    BTTL_PUB_ID_CHECK(udEntryValid,BITWIDTH1+1);
    BTTL_PUB_ID_CHECK(eTunnelMode,e_SEC_IPSEC_MODE_LAST);
    BTTL_PUB_ID_CHECK(udSeqCnterOverflow,BITWIDTH1+1);
    BTTL_PUB_ID_CHECK(eLiveTimeType,e_SEC_SA_LIVETIME_TYPE_LAST);
    BTTL_PUB_ID_CHECK(eSaDfMode,e_SEC_SA_DF_MODE_LAST);
    BTTL_PUB_ID_CHECK(eEncryptionMode,e_SEC_ENCRYP_MODE_LAST);
    BTTL_PUB_ID_CHECK(udIcvLenNew,BITWIDTH6+1);

    //BTTL_PUB_NULL_CHECK(pusSaParam);

    *pusSaParam = udIcvLenNew|(eEncryptionMode<<6)|(eSaDfMode<<9)|(eLiveTimeType<<11)|(udSeqCnterOverflow<<13)|(eTunnelMode<<14)|(udEntryValid<<15);

    return 0;
}
static int zxdh_ipsec_dtb_out_sa_get(struct xfrm_state *xs,T_HAL_SA_DTB_HW_OUT* ptDtbOutSa)
{
    int err = -EINVAL;
    u16 usSaParam = 0;
    u32 udIcvLen = 0;
    E_HAL_SEC_IPSEC_AUTH_ALG zxdh_auth_id;
    E_HAL_SEC_IPSEC_CIPHER_ALG zxdh_ealgo_id;
    E_CMDK_SEC_ENCRYP_MODE zxdh_encpy_mode = e_SEC_ENCRYP_MODE_LAST;
    char test_alg_name[] = "zxdh_alg_test";
    char* p_aalg_alg_name = test_alg_name;
    char* p_ealg_alg_name = test_alg_name;
    char* p_aead_alg_name = test_alg_name;

    if(NULL != xs->aalg)
    {
        p_aalg_alg_name = xs->aalg->alg_name;
    }
    if(NULL != xs->ealg)
    {
        p_ealg_alg_name = xs->ealg->alg_name;
    }
    if(NULL != xs->aead)
    {
        p_aead_alg_name = xs->aead->alg_name;
    }

    /*AH应该提前拦截*/
    /*空加密空认证应该提前拦截*/

    //DH_LOG_INFO(MODULE_SEC, "xs:0x%llx\n",xs);
    //DH_LOG_INFO(MODULE_SEC, "ptDtbOutSa:0x%llx\n",ptDtbOutSa);
    /*应该和pcs的思路一样  ,mlx5e_xfrm_validate_state 参数校验里去把sa的赋值做了*/

    err = zxdh_ipsec_auth_id_get(xs->props.aalgo,p_aalg_alg_name,&zxdh_auth_id);
    if (err) {
        DH_LOG_INFO(MODULE_SEC, "%s Cannot offload xfrm state aalgo:%u\n", __func__, xs->props.aalgo);
        return -EINVAL;
    }
    err = zxdh_ipsec_cipher_id_get(xs->props.ealgo,p_ealg_alg_name,p_aead_alg_name,&zxdh_ealgo_id);
    if (err) {
        DH_LOG_INFO(MODULE_SEC, "%s Cannot offload xfrm state ealgo:%u\n", __func__, xs->props.aalgo);
        return -EINVAL;
    }

    //DH_LOG_INFO(MODULE_SEC, "replay_esn 0x%llx\n",xs->replay_esn);
    ptDtbOutSa->ucAuthkeyLen       = 0;    /*默认值*/

    /*处理单认证算法*/
    if(zxdh_auth_id == e_HAL_IPSEC_AUTH_NULL)
    {
        zxdh_encpy_mode = e_SEC_ENCRYP_ESP_ENCRYP_MODE;
    }
    else
    {
        ptDtbOutSa->ucAuthkeyLen       = (xs->aalg->alg_key_len + 7)/8;
        udIcvLen = (xs->aalg->alg_trunc_len + 7)/8;
        memcpy((ptDtbOutSa->aucSaAuthKey),xs->aalg->alg_key,ptDtbOutSa->ucAuthkeyLen);
    }

    if((zxdh_ealgo_id != e_HAL_IPSEC_CIPHER_NULL)&&(zxdh_auth_id != e_HAL_IPSEC_AUTH_NULL))
    {
        zxdh_encpy_mode = e_SEC_ENCRYP_ESP_AUTH_AND_ESP_ENCRYP_MODE;
    }
    /*这里处理组合算法的4字节的salt*/
    if((zxdh_ealgo_id == e_HAL_IPSEC_CIPHER_AES_GCM)||(zxdh_ealgo_id == e_HAL_IPSEC_CIPHER_CHACHA)||(zxdh_ealgo_id == e_HAL_IPSEC_CIPHER_AES_GMAC))
    {
        zxdh_encpy_mode = e_SEC_ENCRYP_ESP_COMBINED_MODE;

        ptDtbOutSa->ucCipherkeyLen     = (xs->aead->alg_key_len + 7)/8 - 4;
        udIcvLen = (xs->aead->alg_icv_len+ 7)/8;
        memcpy(&(ptDtbOutSa->udSalt), xs->aead->alg_key + ptDtbOutSa->ucCipherkeyLen,sizeof(ptDtbOutSa->udSalt));
        memcpy((ptDtbOutSa->aucSaCipherKey),xs->aead->alg_key,ptDtbOutSa->ucCipherkeyLen);
    }
    /*这里处理组合算法CCM,CCM的salt是3B*/
    else if(zxdh_ealgo_id == e_HAL_IPSEC_CIPHER_AES_CCM)
    {
        zxdh_encpy_mode = e_SEC_ENCRYP_ESP_COMBINED_MODE;

        ptDtbOutSa->ucCipherkeyLen     = (xs->aead->alg_key_len + 7)/8 - 3;
        udIcvLen = (xs->aead->alg_icv_len+ 7)/8;
        memcpy(&(ptDtbOutSa->udSalt), xs->aead->alg_key + ptDtbOutSa->ucCipherkeyLen,sizeof(ptDtbOutSa->udSalt));
        memcpy((ptDtbOutSa->aucSaCipherKey),xs->aead->alg_key,ptDtbOutSa->ucCipherkeyLen);
    }
    /*这里处理有salt的单加密算法CTR,salt是4B*/
    else if(zxdh_ealgo_id == e_HAL_IPSEC_CIPHER_AES_CTR)
    {
        ptDtbOutSa->ucCipherkeyLen     = (xs->ealg->alg_key_len + 7)/8 - 4;
        memcpy(&(ptDtbOutSa->udSalt), xs->ealg->alg_key + ptDtbOutSa->ucCipherkeyLen,sizeof(ptDtbOutSa->udSalt));
        memcpy((ptDtbOutSa->aucSaCipherKey),xs->ealg->alg_key,ptDtbOutSa->ucCipherkeyLen);
    }
    /*空加密算法*/
    else if(zxdh_ealgo_id == e_HAL_IPSEC_CIPHER_NULL)
    {
        zxdh_encpy_mode = e_SEC_ENCRYP_ESP_AUTH_MODE;
        ptDtbOutSa->ucCipherkeyLen     = 0;
    }
    /*单加密算法,且没有salt*/
    else
    {
        ptDtbOutSa->ucCipherkeyLen     = (xs->ealg->alg_key_len + 7)/8;
        memcpy((ptDtbOutSa->aucSaCipherKey),xs->ealg->alg_key,ptDtbOutSa->ucCipherkeyLen);
    }


    ptDtbOutSa->udSN                = xs->replay.oseq;
    ptDtbOutSa->uddProcessedByteCnt = xs->curlft.bytes;   //PUB_HTON64(uddProcessedByteCnt);   
    ptDtbOutSa->udSPI          = xs->id.spi;
    ptDtbOutSa->udSaId        = PUB_HTON32(0x80001);                //PUB_HTON32(udSaId);                             /*这个要软件自己管理,需要设计一下*/

    ptDtbOutSa->ucCiperID      = zxdh_ealgo_id;
    ptDtbOutSa->ucAuthID       = zxdh_auth_id;

    //CmdkBttlSecSaParamConstruct(UINT32 udEntryValid,E_CMDK_SEC_IPSEC_MODE eTunnelMode,UINT32 udSeqCnterOverflow,E_CMDK_LIVETIME_TYPES eLiveTimeType,E_CMDK_SEC_SA_DF_MODE eSaDfMode,E_CMDK_SEC_ENCRYP_MODE eEncryptionMode,UINT32 udIcvLen,UINT16* pusSaParam)
    //E_CMDK_SEC_ENCRYP_MODE 这个只能根据算法反推 gcm ccm gmac chacha是combine  gaucSecSwanIpv6Data
    //udIcvLen
    //mode的定义刚好一样E_CMDK_SEC_IPSEC_MODE ,   XFRM_MODE_TRANSPORT
    /*这个地方还要根据算法做个转换  e_SEC_ENCRYP_ESP_COMBINED_MODE  暂时用GCM*/
    CmdkBttlSecSaParamConstruct(1,xs->props.mode,0,e_SEC_SA_LIVETIME_TIME_TYPE,e_SEC_SA_DF_BYPASS_MODE,zxdh_encpy_mode,udIcvLen,&usSaParam);
    ptDtbOutSa->usSaParam      = PUB_HTON16(usSaParam);



    ptDtbOutSa->usFrag_State   = PUB_HTON16(0xd2c8);

    ptDtbOutSa->udLifetimeSecMax  = PUB_HTON32(0xc4454766);
    ptDtbOutSa->uddLifetimByteCntMax      = PUB_HTON64(0xffffffffffffffff);

    ptDtbOutSa->ucProtocol     = xs->id.proto;     //50esp协议 51ah
    ptDtbOutSa->ucTOS          = 0xbb;

    /*esn相关*/
    ptDtbOutSa->ucEsnFlag  = 0;  /* 默认是非ESN模式 */
    if(xs->props.flags & XFRM_STATE_ESN)
    {
        if(NULL == xs->replay_esn)
            return 1;
        ptDtbOutSa->ucEsnFlag    = 0xff; //ucEsnFlag;    //0xff表示开启ESN,否则不开启
        ptDtbOutSa->udSN         = xs->replay_esn->oseq;
        ptDtbOutSa->udESN        = xs->replay_esn->oseq_hi; /*不需要考虑replay_esn为null的情况?*/
    }

    /*ipv4*/
    if(AF_INET == xs->props.family)
    {
        ptDtbOutSa->ucIpType     = 1<<6;   //bit[7:6] 1:ivp4 2:ipv6    /*换成宏*/
        ptDtbOutSa->udSrcAddress0  = xs->props.saddr.a4;
        ptDtbOutSa->udSrcAddress1  = 0x0;
        ptDtbOutSa->udSrcAddress2  = 0x0;
        ptDtbOutSa->udSrcAddress3  = 0x0;

        ptDtbOutSa->udDstAddress0  = xs->id.daddr.a4;
        ptDtbOutSa->udDstAddress1  = 0x0;
        ptDtbOutSa->udDstAddress2  = 0x0;
        ptDtbOutSa->udDstAddress3  = 0x0;
    }
    /*ipv4*/
    else if(AF_INET6 == xs->props.family)
    {
        ptDtbOutSa->ucIpType     = 2<<6;   //bit[7:6] 1:ivp4 2:ipv6    /*换成宏*/
        ptDtbOutSa->udSrcAddress0  = xs->props.saddr.a6[0];
        ptDtbOutSa->udSrcAddress1  = xs->props.saddr.a6[1];
        ptDtbOutSa->udSrcAddress2  = xs->props.saddr.a6[2];
        ptDtbOutSa->udSrcAddress3  = xs->props.saddr.a6[3];

        ptDtbOutSa->udDstAddress0  = xs->id.daddr.a6[0];
        ptDtbOutSa->udDstAddress1  = xs->id.daddr.a6[1];
        ptDtbOutSa->udDstAddress2  = xs->id.daddr.a6[2];
        ptDtbOutSa->udDstAddress3  = xs->id.daddr.a6[3];
    }
    else
    {
        return -EINVAL;  /*不可能走到这里,前面函数已经校验过了*/
    }

    ptDtbOutSa->udRSV0       = 0x0;
    ptDtbOutSa->udRSV1       = 0x0;
    ptDtbOutSa->udRSV2       = 0x0;

    DH_LOG_INFO(MODULE_SEC, "%s ptDtbOutSa->ucAuthkeyLen:0x%x\n", __func__, ptDtbOutSa->ucAuthkeyLen);
    DH_LOG_INFO(MODULE_SEC, "%s ptDtbOutSa->ucCipherkeyLen:0x%x\n", __func__, ptDtbOutSa->ucCipherkeyLen);
    DH_LOG_INFO(MODULE_SEC, "%s zxdh_encpy_mode:0x%x\n", __func__, zxdh_encpy_mode);
    DH_LOG_INFO(MODULE_SEC, "%s ptDtbOutSa->ucCiperID:0x%x\n", __func__, ptDtbOutSa->ucCiperID);
    DH_LOG_INFO(MODULE_SEC, "%s ptDtbOutSa->ucAuthID:0x%x\n", __func__, ptDtbOutSa->ucAuthID);


    return 0;
}

static int zxdh_ipsec_dtb_in_sa_get(struct xfrm_state *xs,T_HAL_SA_DTB_HW_IN* ptDtbInSa)
{
    int err = -EINVAL;
    u16 usSaParam = 0;
    u32 udIcvLen = 0;
    E_HAL_SEC_IPSEC_AUTH_ALG zxdh_auth_id;
    E_HAL_SEC_IPSEC_CIPHER_ALG zxdh_ealgo_id;
    E_CMDK_SEC_ENCRYP_MODE zxdh_encpy_mode = e_SEC_ENCRYP_MODE_LAST;
    char test_alg_name[] = "zxdh_alg_test";
    char* p_aalg_alg_name = test_alg_name;
    char* p_ealg_alg_name = test_alg_name;
    char* p_aead_alg_name = test_alg_name;

/*应该和pcs的思路一样  ,mlx5e_xfrm_validate_state 参数校验里去把sa的赋值做了*/
    if(NULL != xs->aalg)
    {
        p_aalg_alg_name = xs->aalg->alg_name;
    }
    if(NULL != xs->ealg)
    {
        p_ealg_alg_name = xs->ealg->alg_name;
    }
    if(NULL != xs->aead)
    {
        p_aead_alg_name = xs->aead->alg_name;
    }
    err = zxdh_ipsec_auth_id_get(xs->props.aalgo,p_aalg_alg_name,&zxdh_auth_id);
    if (err) {
        DH_LOG_INFO(MODULE_SEC, "%s Cannot offload xfrm state aalgo:%u\n", __func__, xs->props.aalgo);
        return -EINVAL;
    }
    err = zxdh_ipsec_cipher_id_get(xs->props.ealgo,p_ealg_alg_name,p_aead_alg_name,&zxdh_ealgo_id);
    if (err) {
        DH_LOG_INFO(MODULE_SEC, "%s Cannot offload xfrm state ealgo:%u\n", __func__, xs->props.aalgo);
        return -EINVAL;
    }

    ptDtbInSa->ucAuthkeyLen       = 0;    /*默认值*/

    /*处理单认证算法*/
    if(zxdh_auth_id == e_HAL_IPSEC_AUTH_NULL)
    {
        zxdh_encpy_mode = e_SEC_ENCRYP_ESP_ENCRYP_MODE;
    }
    else
    {
        ptDtbInSa->ucAuthkeyLen       = (xs->aalg->alg_key_len + 7)/8;
        udIcvLen = (xs->aalg->alg_trunc_len + 7)/8;
        memcpy((ptDtbInSa->aucSaAuthKey),xs->aalg->alg_key,ptDtbInSa->ucAuthkeyLen);
    }

    if((zxdh_ealgo_id != e_HAL_IPSEC_CIPHER_NULL)&&(zxdh_auth_id != e_HAL_IPSEC_AUTH_NULL))
    {
        zxdh_encpy_mode = e_SEC_ENCRYP_ESP_AUTH_AND_ESP_ENCRYP_MODE;
    }
    /*这里处理组合算法的4字节的salt*/
    if((zxdh_ealgo_id == e_HAL_IPSEC_CIPHER_AES_GCM)||(zxdh_ealgo_id == e_HAL_IPSEC_CIPHER_CHACHA)||(zxdh_ealgo_id == e_HAL_IPSEC_CIPHER_AES_GMAC))
    {
        zxdh_encpy_mode = e_SEC_ENCRYP_ESP_COMBINED_MODE;

        ptDtbInSa->ucCipherkeyLen     = (xs->aead->alg_key_len + 7)/8 - 4;
        udIcvLen = (xs->aead->alg_icv_len+ 7)/8;
        memcpy(&(ptDtbInSa->udSalt), xs->aead->alg_key + ptDtbInSa->ucCipherkeyLen,sizeof(ptDtbInSa->udSalt));
        memcpy((ptDtbInSa->aucSaCipherKey),xs->aead->alg_key,ptDtbInSa->ucCipherkeyLen);
    }
    /*这里处理组合算法CCM,CCM的salt是3B*/
    else if(zxdh_ealgo_id == e_HAL_IPSEC_CIPHER_AES_CCM)
    {
        zxdh_encpy_mode = e_SEC_ENCRYP_ESP_COMBINED_MODE;

        ptDtbInSa->ucCipherkeyLen     = (xs->aead->alg_key_len + 7)/8 - 3;
        udIcvLen = (xs->aead->alg_icv_len+ 7)/8;
        memcpy(&(ptDtbInSa->udSalt), xs->aead->alg_key + ptDtbInSa->ucCipherkeyLen,sizeof(ptDtbInSa->udSalt));
        memcpy((ptDtbInSa->aucSaCipherKey),xs->aead->alg_key,ptDtbInSa->ucCipherkeyLen);
    }
    /*这里处理有salt的单加密算法CTR,salt是4B*/
    else if(zxdh_ealgo_id == e_HAL_IPSEC_CIPHER_AES_CTR)
    {
        ptDtbInSa->ucCipherkeyLen     = (xs->ealg->alg_key_len + 7)/8 - 4;
        memcpy(&(ptDtbInSa->udSalt), xs->ealg->alg_key + ptDtbInSa->ucCipherkeyLen,sizeof(ptDtbInSa->udSalt));
        memcpy((ptDtbInSa->aucSaCipherKey),xs->ealg->alg_key,ptDtbInSa->ucCipherkeyLen);
    }
    /*空加密算法*/
    else if(zxdh_ealgo_id == e_HAL_IPSEC_CIPHER_NULL)
    {
        zxdh_encpy_mode = e_SEC_ENCRYP_ESP_AUTH_MODE;
        ptDtbInSa->ucCipherkeyLen     = 0;
    }
    /*单加密算法,且没有salt*/
    else
    {
        ptDtbInSa->ucCipherkeyLen     = (xs->ealg->alg_key_len + 7)/8;
        memcpy((ptDtbInSa->aucSaCipherKey),xs->ealg->alg_key,ptDtbInSa->ucCipherkeyLen);
    }

    ptDtbInSa->uddProcessedByteCnt = xs->curlft.bytes;   //PUB_HTON64(uddProcessedByteCnt);  
    ptDtbInSa->udSPI          = xs->id.spi;
    ptDtbInSa->udSaId        = PUB_HTON32(0x80000);            //PUB_HTON32(udSaId);                             /*这个要软件自己管理,需要设计一下*/

    ptDtbInSa->ucCiperID      = zxdh_ealgo_id;
    ptDtbInSa->ucAuthID       = zxdh_auth_id;

    //CmdkBttlSecSaParamConstruct(UINT32 udEntryValid,E_CMDK_SEC_IPSEC_MODE eTunnelMode,UINT32 udSeqCnterOverflow,E_CMDK_LIVETIME_TYPES eLiveTimeType,E_CMDK_SEC_SA_DF_MODE eSaDfMode,E_CMDK_SEC_ENCRYP_MODE eEncryptionMode,UINT32 udIcvLen,UINT16* pusSaParam)
    //E_CMDK_SEC_ENCRYP_MODE 这个只能根据算法反推 gcm ccm gmac chacha是combine  gaucSecSwanIpv6Data
    //udIcvLen
    //mode的定义刚好一样E_CMDK_SEC_IPSEC_MODE ,   XFRM_MODE_TRANSPORT
    CmdkBttlSecSaParamConstruct(1,xs->props.mode,0,e_SEC_SA_LIVETIME_TIME_TYPE,e_SEC_SA_DF_BYPASS_MODE,zxdh_encpy_mode,udIcvLen,&usSaParam);
    ptDtbInSa->usSaParam      = PUB_HTON16(usSaParam);



    ptDtbInSa->usFrag_State   = PUB_HTON16(0xd2c8);

    ptDtbInSa->udLifetimeSecMax  = PUB_HTON32(0xc4454766);
    ptDtbInSa->uddLifetimByteCntMax      = PUB_HTON64(0xffffffffffffffff);

    ptDtbInSa->ucProtocol     = xs->id.proto;     //50esp协议 51ah
    ptDtbInSa->ucTOS          = 0xbb;

    /*esn相关*/
    ptDtbInSa->ucEsnFlag  = 0;  /* 默认是非ESN模式 */
    if(xs->props.flags & XFRM_STATE_ESN)
    {
        if(NULL == xs->replay_esn)
            return 1;
        ptDtbInSa->ucEsnFlag    = 0xff; //ucEsnFlag;    //0xff表示开启ESN,否则不开启
        ptDtbInSa->udAntiWindowHigh        = PUB_HTON32(xs->replay_esn->seq_hi); /*ESN*/
        ptDtbInSa->udAntiWindowLow        = PUB_HTON32(xs->replay_esn->replay_window - 1); /*窗口上限sn,这里使用窗口大小-1*/
        memcpy((void*)ptDtbInSa->aucBitmap,(void*)xs->replay_esn->bmp,xs->replay_esn->bmp_len * sizeof(__u32)); /*需要提前判断bmp_len不能太大,避免超过64(拦截窗口大小就行)*/
    }

    /*ipv4*/
    if(AF_INET == xs->props.family)
    {
        ptDtbInSa->ucIpType     = 1<<6;   //bit[7:6] 1:ivp4 2:ipv6    /*换成宏*/
        ptDtbInSa->udSrcAddress0  = xs->props.saddr.a4;
        ptDtbInSa->udSrcAddress1  = 0x0;
        ptDtbInSa->udSrcAddress2  = 0x0;
        ptDtbInSa->udSrcAddress3  = 0x0;

        ptDtbInSa->udDstAddress0  = xs->id.daddr.a4;
        ptDtbInSa->udDstAddress1  = 0x0;
        ptDtbInSa->udDstAddress2  = 0x0;
        ptDtbInSa->udDstAddress3  = 0x0;
    }
    /*ipv4*/
    else if(AF_INET6 == xs->props.family)
    {
        ptDtbInSa->ucIpType     = 2<<6;   //bit[7:6] 1:ivp4 2:ipv6    /*换成宏*/
        ptDtbInSa->udSrcAddress0  = xs->props.saddr.a6[0];
        ptDtbInSa->udSrcAddress1  = xs->props.saddr.a6[1];
        ptDtbInSa->udSrcAddress2  = xs->props.saddr.a6[2];
        ptDtbInSa->udSrcAddress3  = xs->props.saddr.a6[3];

        ptDtbInSa->udDstAddress0  = xs->id.daddr.a6[0];
        ptDtbInSa->udDstAddress1  = xs->id.daddr.a6[1];
        ptDtbInSa->udDstAddress2  = xs->id.daddr.a6[2];
        ptDtbInSa->udDstAddress3  = xs->id.daddr.a6[3];
    }
    else
    {
        return -EINVAL;  /*不可能走到这里,前面函数已经校验过了*/
    }

    ptDtbInSa->udOutSaId    = 0x0;   /*内核不需要出入境sa一起下吧,固定为0*/
    ptDtbInSa->usOutSaOffset    = 0x0;

    ptDtbInSa->udRSV0       = 0x0;
    ptDtbInSa->udRSV1       = 0x0;

    DH_LOG_INFO(MODULE_SEC, "%s ptDtbInSa->ucAuthkeyLen:0x%x\n", __func__, ptDtbInSa->ucAuthkeyLen);
    DH_LOG_INFO(MODULE_SEC, "%s ptDtbInSa->ucCipherkeyLen:0x%x\n", __func__, ptDtbInSa->ucCipherkeyLen);
    DH_LOG_INFO(MODULE_SEC, "%s zxdh_encpy_mode:0x%x\n", __func__, zxdh_encpy_mode);
    DH_LOG_INFO(MODULE_SEC, "%s ptDtbInSa->ucCiperID:0x%x\n", __func__, ptDtbInSa->ucCiperID);
    DH_LOG_INFO(MODULE_SEC, "%s ptDtbInSa->ucAuthID:0x%x\n", __func__, ptDtbInSa->ucAuthID);


    return 0;
}
#endif


VOID RdlSecWrite(UINT64 uddSecBase, UINT32 udRegOff, UINT32 udRegVal)
{
    PUB_WRITE_REG32(uddSecBase + udRegOff, udRegVal);
}

UINT32 HalSecWrite(struct zxdh_en_device *en_dev, UINT32 udSecEngineId, UINT32 udRegOff, UINT32 udRegVal)
{
    UINT64 uddBttlSecBase = 0;
    UINT32 udSecnBaseOff = 0;
    UINT64 uddSecnBase = 0;

    PUB_CHECK_NULL_PTR_RET_ERR(en_dev);

    uddBttlSecBase = HalBttlSecRegBaseGet(en_dev);
    //udSecnBaseOff  = udSecEngineId * REG_SEC_IDX_OFFSET; 
    uddSecnBase    = uddBttlSecBase + udSecnBaseOff;
    //DH_LOG_INFO(MODULE_SEC, "HalBttlSecRegBaseGet regBase vir:0x%llx\n",uddSecnBase);
    //DH_LOG_INFO(MODULE_SEC, "HalBttlSecRegBaseGet regBase pa:0x%llx\n",virt_to_phys((void*)uddSecnBase));
    RdlSecWrite(uddSecnBase, udRegOff, udRegVal);

    return 0;
}

UINT32 RdlSecRead(UINT64 uddSecBase, UINT32 udRegOff)
{
    return PUB_READ_REG32(uddSecBase + udRegOff);
}

UINT32 HalSecRead(struct zxdh_en_device *en_dev, UINT32 udSecEngineId, UINT32 udRegOff)
{
    UINT64 uddBttlSecBase = 0;
    UINT32 udSecnBaseOff = 0;
    UINT64 uddSecnBase = 0;

    PUB_CHECK_NULL_PTR_RET_ERR(en_dev);
    uddBttlSecBase = HalBttlSecRegBaseGet(en_dev);
    udSecnBaseOff  = udSecEngineId * REG_SEC_IDX_OFFSET;
    uddSecnBase    = uddBttlSecBase + udSecnBaseOff;

    return RdlSecRead(uddSecnBase, udRegOff);
}


UINT64 HalBttlVaToVpa(struct zxdh_en_device *en_dev, UINT64 pVaAddr)
{
    PUB_CHECK_NULL_PTR_RET_ERR(en_dev);
    return (UINT64)virt_to_phys((void*)pVaAddr);
}

UINT64 HalBttlVpaToVa(struct zxdh_en_device *en_dev, UINT64 pVpaAddr)
{
    PUB_CHECK_NULL_PTR_RET_ERR(en_dev);
    return (UINT64)phys_to_virt(pVpaAddr);
}

#if 0
VOID PubDumpBuf(UINT8 *pucBuf, UINT32 udLen)
{
    UINT32 i = 0;
    UINT32 j = 0;
    UINT8 *pucPtr =NULL;

    pucPtr = pucBuf;
    for( j=0; j<48; j++ )
    {
        PUB_PRINTF("-");
    }
    PUB_PRINTF("\n");

    for( i=0; i<udLen; i++ )
    {
        PUB_PRINTF("%02X ", pucPtr[i] );
        if ( 15 == i%16 )
        {
            PUB_PRINTF("   *");
            PUB_PRINTF("*\n");
        }
    }

    if ( 0 == udLen%16 )
    {
        PUB_PRINTF("\n");
    }
    else
    {
        for ( i=(udLen%16); i<16; i++ )
        {
            PUB_PRINTF("   ");
        }
        PUB_PRINTF("   *");
        PUB_PRINTF("\n\n");
    }
}
#endif

void BttlPubDump(unsigned char *ucBuf, UINT32 udLen)
{
    int i = 0;
    int j = 0;
    unsigned char *ptr =NULL;

    ptr = ucBuf;

    if (ucBuf == NULL || udLen == 0)
    {
        return;
    }

    for( j=0; j<64; j++ )
    {
        printk("-");
    }
    printk("\n");

    for( i=0; i<udLen; i++ )
    {
        if ( 0 == i%16 )
        {
            printk("0x%08x ", (i/16)*16);
        }
        printk("%02X ", ptr[i] );
        if ( 15 == i%16 )
        {
            printk("   *");
            printk("*\n");
        }
    }

    if ( 0 == udLen%16 )
    {
        printk("\n");
    }
    else
    {
        for ( i=(udLen%16); i<16; i++ )
        {
            printk("   ");
        }
        printk("   *");
        printk("\n\n");
    }

    return;
}

UINT32 CmdkBttlTestSaAckRslGet(UINT64 uddSaVirAddr,E_CMDK_DTB_SA_CMD_TYPE eDtbSaCmdType,UINT32 *pudIsDtbAckFinish,UINT32 *pudDtbAckRsl)
{
    UINT32 udVal = 0;
    UINT32 udDtbAckFinish = 0;

    /*入参检查*/
    //BTTL_PUB_ID_CHECK(eDtbSaCmdType, E_DTB_SA_CMD_LAST);
    //BTTL_PUB_NULL_CHECK(pudIsDtbAckFinish);
    //BTTL_PUB_NULL_CHECK(pudDtbAckRsl);

    *pudIsDtbAckFinish = 0;  /* 默认置为响应未完成 */

    udVal = *((UINT32 *)(uddSaVirAddr ));
    udDtbAckFinish = PUB_BIT_FIELD_RIGHT_JUST_GET64(udVal,0,24);
    *pudDtbAckRsl = PUB_BIT_FIELD_RIGHT_JUST_GET64(udVal,24,8);

    if(E_DTB_SA_CMD_FLOW_DOWN == eDtbSaCmdType)
    {
        if(udDtbAckFinish == 0x5a5a5a)
        {
            *pudIsDtbAckFinish = 1;
        }
    }
    else
    {
        if(udDtbAckFinish == 0x555555)
        {
            *pudIsDtbAckFinish = 1;
        }
    }

    return 0;

}

/*参考NP的函数 dpp_dtb_user_info_set*/
UINT32 CmdkBttlSecSaDownload(struct zxdh_en_device *en_dev, UINT32 udSecEngineId, T_QUEUE_DTB_REG *pt,UINT32 udQueIndex)
{
    UINT32 udRet;
    UINT32 udRegVal;
    //UINT32 udQueIndex = 1;
    UINT32 udEpldVfunNum = 0;
    UINT32 udPcieDbiEn = 1;        /*1为dbi中断，0为match中断,NP这里是全局变量,暂时写死*/
    UINT32 udEpid = 5;
    UINT32 udVfuncNum = 0;
    UINT32 udCfgMsixVector = 2;    /*以前host驱动写的是2,暂时写死*/
    UINT32 udFuncNum = 2;
    UINT32 udVfuncActive = 0;
    UINT16 usVport = 0;

    //BTTL_PUB_ID_CHECK(en_dev, CMDK_BTTL_PUB_CHIP_MAX);
    //BTTL_PUB_ID_CHECK(udSecEngineId, HAL_SEC_MAX_ENGINE);
    //BTTL_PUB_NULL_CHECK(pt);
    PUB_CHECK_NULL_PTR_RET_ERR(en_dev);

    usVport = en_dev->ops->get_vport(en_dev->parent);

    //写sa的队列锁状态寄存器CFG_DTB_QUEUE_LOCK_STATE，共128个队列,理论上应该查询
    //udRet = HalSecWrite(en_dev, udSecEngineId, REG_SEC_DTB_QUEUE_LOCK_STATE_0_3(0), PUB_BIT_SET(udLockMask,udQueIndex));


    //暂时沟通是，只需要将epid配置为0，下表模块就会去riscv侧下表，暂时可以不配
    udEpid = EPID(usVport) + 5;
    udVfuncNum = VFUNC_NUM(usVport);
    udFuncNum = FUNC_NUM(usVport);
    udVfuncActive = VF_ACTIVE(usVport);

    DH_LOG_INFO_DEV(MODULE_SEC, en_dev->parent, "udEpid:0x%x,udVfuncNum:0x%x\n",udEpid,udVfuncNum);
    DH_LOG_INFO_DEV(MODULE_SEC, en_dev->parent, "udFuncNum:0x%x,udVfuncActive:0x%x\n",udFuncNum,udVfuncActive);

    PUB_BIT_FIELD_SET64(udEpldVfunNum,udVfuncActive,0,1);
    PUB_BIT_FIELD_SET64(udEpldVfunNum,udFuncNum,5,3);
    PUB_BIT_FIELD_SET64(udEpldVfunNum,udCfgMsixVector,8,7);
    PUB_BIT_FIELD_SET64(udEpldVfunNum,udVfuncNum,16,8);
    PUB_BIT_FIELD_SET64(udEpldVfunNum,udEpid,24,4);
    PUB_BIT_FIELD_SET64(udEpldVfunNum,udPcieDbiEn,31,1);

    //return 0;
    DH_LOG_INFO_DEV(MODULE_SEC, en_dev->parent, "udEpldVfunNum = 0x%x\n",udEpldVfunNum);
    HalSecWrite(en_dev, udSecEngineId, REG_SEC_CFG_EPID_V_FUNC_NUM_0_127(udQueIndex), udEpldVfunNum);

    //查询所申请队列剩余空间,如果队列剩余空间大于0则可入队；
     udRegVal = HalSecRead(en_dev, udSecEngineId, REG_SEC_INFO_QUEUE_BUF_SPACE_LEFT_0_127(udQueIndex));
     if(udRegVal < 2)
     {
         BTTL_PRINTF_DEV(en_dev->parent, "queue:%u buf empty left:%u\n",udQueIndex,udRegVal);
         return 1;
     }
     if(udRegVal > 0x20)
     {
         BTTL_PRINTF_DEV(en_dev->parent, "queue:%u buf left:%u\n",udQueIndex,udRegVal);
         return 1;
     }

    //先写DTB_ADDR[63:32],接着写DTB_ADDR[31:0],最后写usdtb_len(软件需严格遵守该顺序）
    udRet = HalSecWrite(en_dev, udSecEngineId, REG_SEC_CFG_QUEUE_DTB_ADDR_H_0_127(udQueIndex), pt->DtbAddrH);

    //DH_LOG_INFO(MODULE_SEC, "pt->DtbAddrH = 0x%x\n",pt->DtbAddrH);
    udRet = HalSecWrite(en_dev, udSecEngineId, REG_SEC_CFG_QUEUE_DTB_ADDR_L_0_127(udQueIndex), pt->DtbAddrL);

    //DH_LOG_INFO(MODULE_SEC, "pt->DtbAddrL = 0x%x\n",pt->DtbAddrL);
    //DH_LOG_INFO(MODULE_SEC, "pt->DtbAddrVir = 0x%llx\n",HalBttlVpaToVa(en_dev,(UINT64)((UINT64)pt->DtbAddrH)<<32)+pt->DtbAddrL);
    // CMD寄存器最后配
    udRet = HalSecWrite(en_dev, udSecEngineId, REG_SEC_CFG_QUEUE_DTB_LEN_0_127(udQueIndex), pt->DtbCmd);

    //DH_LOG_INFO(MODULE_SEC, "pt->DtbCmd = 0x%x\n",pt->DtbCmd);
    return 0;
}

/*
    sa下表模块测试
    SA存放地址,第二套L2D uddSaL2DPhyAddr= 0x6201000000;理论上为68位，目前场景为64位
    usdtb_len =30;
*/
//E_SA_TYPE geSaType;

UINT32 gudTestCnt = 0;
UINT32 CmdkBttlTestSecDtbSaAdd(struct zxdh_en_device *en_dev,E_CMDK_DTB_SA_CMD_TYPE eDtbSaCmdType,E_SA_TYPE eSaType,UINT64 uddSaVirAddr,UINT32 udDtbSaIsIntEn,UINT32 udDtbLen,UINT32 udQueIndex)
{
    /* int_en指示是否产生需要中断 第29位,cmd_type=0指示为流表下发命令，cmd_type=1指示为流表dump命令 第30位 一对sa表的大小为480字节，以16字节为单位*/
    T_QUEUE_DTB_REG tDtbReg = {0};
    UINT32 udDtbCmd = 0;
    UINT64 uddSaPhaAddr = 0;
    UINT32 udIsDtbAckFinish = 0;
    UINT32 udDtbAckRsl = 0;
    UINT32 udRet = 0;
    int i = 0;


    /*入参检查*/
    //BTTL_PUB_ID_CHECK(en_dev, CMDK_BTTL_PUB_CHIP_MAX);
    //BTTL_PUB_ID_CHECK(eDtbSaCmdType, E_DTB_SA_CMD_LAST);
    PUB_CHECK_NULL_PTR_RET_ERR(en_dev);

    uddSaPhaAddr = (UINT64)HalBttlVaToVpa(en_dev, uddSaVirAddr);
    //BTTL_PUB_0_CHECK(uddSaPhaAddr);
    DH_LOG_INFO_DEV(MODULE_SEC, en_dev->parent, "uddSaVirAddr:0x%llx,uddSaPhaAddr:0x%llx\n",uddSaVirAddr,uddSaPhaAddr);

    //构造udDtbCmd
    PUB_BIT_FIELD_SET64(udDtbCmd,udDtbLen>>4,0,10);
    PUB_BIT_FIELD_SET64(udDtbCmd,eSaType,27,2);
    PUB_BIT_FIELD_SET64(udDtbCmd,udDtbSaIsIntEn,29,1);
    PUB_BIT_FIELD_SET64(udDtbCmd,eDtbSaCmdType,30,1);

    tDtbReg.DtbAddrH = (UINT32)PUB_BIT_FIELD_RIGHT_JUST_GET64(uddSaPhaAddr,32,32);
    tDtbReg.DtbAddrL = (UINT32)PUB_BIT_FIELD_RIGHT_JUST_GET64(uddSaPhaAddr,0,32);
    tDtbReg.DtbCmd  = udDtbCmd;

    /* 配置下表寄存器 */
    for(i=0;i<gudDtbSaNum;i++)
    {
    //gudTestCnt++;
    //(*(volatile UINT32*)(uddSaVirAddr + 16)) = PUB_NTOH32(gudTestCnt);
        udRet = CmdkBttlSecSaDownload(en_dev,0,&tDtbReg,udQueIndex);
        //PUB_CHECK_RET_VAL_RV(udRet);
    }
    /* 等待 */
    msleep(1000);
    // PubUsDelay(10); 

    udRet = CmdkBttlTestSaAckRslGet(uddSaVirAddr,eDtbSaCmdType,&udIsDtbAckFinish,&udDtbAckRsl);
    PUB_CHECK_RET_VAL_RV(udRet);

    if((1 == udIsDtbAckFinish)&&(0xff == udDtbAckRsl))
    {
        return 0;
    }
    else
    {
        BTTL_PRINTF_DEV(en_dev->parent, "CmdkBttlTestSa Dtb Ack is error!! udIsDtbAckFinish:%u,udDtbAckRsl:%u\n",udIsDtbAckFinish,udDtbAckRsl);
        BttlPubDump((unsigned char *)uddSaVirAddr, 0x60);
    return 1;
    }

    return 0;
}

#if 1
static int zxdh_ipsec_add_sa(struct xfrm_state *xs, __attribute__((unused)) struct netlink_ext_ack *extack)
{
    struct xfrm_dev_offload *xso = &xs->xso;
    struct net_device *netdev = xso->dev;
    struct zxdh_en_priv *en_priv = NULL;
    //struct zxdh_en_device *en_dev = NULL;
    struct zxdh_en_device *en_dev = NULL;
    dma_addr_t dma_handle;
    UINT32 dma_size = 0x1000;  //暂定4K,批量下表情况下需要更多

    UINT64 uddDtbSaVirAddr = 0;
    UINT32 udSaTblLen = 0;
    int ret = 0;

    en_priv = netdev_priv(netdev);
    en_dev = &(en_priv->edev);

    if(unlikely(en_dev->drs_sec_pri.SecVAddr == 0))
    {
        en_dev->drs_sec_pri.SecVAddr = (uint64_t)dma_alloc_coherent(en_dev->dmadev, dma_size,&dma_handle, GFP_KERNEL);
        if(en_dev->drs_sec_pri.SecVAddr == 0)
        {
            DH_LOG_INFO_DEV(MODULE_SEC, en_dev->parent, "zxdh_ipsec_add_sa dma_alloc_coherent fail\n");
            return -1;
        }
        en_dev->drs_sec_pri.SecPAddr = dma_handle;
        en_dev->drs_sec_pri.SecMemSize = dma_size;
    }
    uddDtbSaVirAddr = en_dev->drs_sec_pri.SecVAddr;

    DH_LOG_INFO_DEV(MODULE_SEC, en_dev->parent, "uddDtbSaVirAddr:0x%llx\n",uddDtbSaVirAddr);
    //DH_LOG_INFO(MODULE_SEC, "xs:0x%llx\n",xs);

    memset((void*)uddDtbSaVirAddr,0,1024);

    //else if(1 == xs->xso.dir)
    if(xso->flags & XFRM_OFFLOAD_INBOUND)
    {
        ret = zxdh_ipsec_dtb_in_sa_get(xs,(T_HAL_SA_DTB_HW_IN*)(uddDtbSaVirAddr+16));
        if(ret != 0)
        {
            return 1;
        }
        BttlPubDump((unsigned char *)uddDtbSaVirAddr, 0x210);  //传入时加了16字节的回写空间

        #if 1
        udSaTblLen = 512 - 16;
        CmdkBttlTestSecDtbSaAdd(en_dev,E_DTB_SA_CMD_FLOW_DOWN,E_SATYPE_IN,uddDtbSaVirAddr,0,udSaTblLen,2);

        #endif
    }
    //if(2 == xs->xso.dir)
    else
    {
        ret = zxdh_ipsec_dtb_out_sa_get(xs,(T_HAL_SA_DTB_HW_OUT*)(uddDtbSaVirAddr+16));
        if(ret != 0)
        {
            return 1;
        }
        BttlPubDump((unsigned char *)uddDtbSaVirAddr, 0x110);  //传入时加了16字节的回写空间

        #if 1
        udSaTblLen = 256 - 16;
        CmdkBttlTestSecDtbSaAdd(en_dev,E_DTB_SA_CMD_FLOW_DOWN,E_SATYPE_OUT,uddDtbSaVirAddr,0,udSaTblLen,2);
        #endif
    }

    return 0;
}

void zxdh_ipsec_del_sa(struct xfrm_state *xs)
{
    DH_LOG_INFO(MODULE_SEC, "zxdh_ipsec_del_sa\n");
    return;
}

bool zxdh_ipsec_offload_ok(struct sk_buff *skb, struct xfrm_state *xs)
{
    DH_LOG_INFO(MODULE_SEC, "zxdh_ipsec_offload_ok\n");
    return true;
}

void zxdh_ipsec_state_advance_esn (struct xfrm_state *x)
{
    DH_LOG_INFO(MODULE_SEC, "zxdh_ipsec_state_advance_esn\n");
    return;
}
void zxdh_ipsec_state_update_curlft (struct xfrm_state *x)
{
    DH_LOG_INFO(MODULE_SEC, "zxdh_ipsec_state_update_curlft\n");
    return ;
}
int zxdh_ipsec_policy_add (struct xfrm_policy *x)
{
#if 1
    int32_t ret = 0;
    UINT8 aucSip[4] = {0xc8,0xfe,0x00,0x1};
    UINT8 aucDip[4] = {0xc8,0xfe,0x00,0x2};
    UINT8 aucSipMask[4] = {0xff,0xff,0x00,0x0};
    UINT8 aucDipMask[4] = {0xff,0xff,0x00,0x0};
    /*6.2的内核才有*/
    //struct xfrm_dev_offload *xdo = &x->xdo;
    //struct net_device *netdev = xdo->dev;
    struct net_device *netdev = NULL; //低版本内核仅编译通过
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    DPP_PF_INFO_T pf_info = {0};

    DH_LOG_INFO_DEV(MODULE_SEC, en_dev->parent, "zxdh_ipsec_policy_add\n");

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    /*np下表 inline sec模式 打开*/
    ret = dpp_vport_attr_set(&pf_info,SRIOV_VPORT_INLINE_SEC_OFFLOAD,1);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to set port_attr SRIOV_VPORT_INLINE_SEC_OFFLOAD !\n");
    }

    /*配置np ipset加密表*/
    ret = dpp_ipsec_enc_entry_add(&pf_info,0,aucSip,aucDip,aucSipMask,aucDipMask,1,0x80001);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "xfrm policy dpp_ipsec_enc_entry_add Failed!\n");
    }
#endif

    return 0;
}
void zxdh_ipsec_policy_delete (struct xfrm_policy *x)
{
    DH_LOG_INFO(MODULE_SEC, "zxdh_ipsec_policy_delete\n");
    return;
}
void zxdh_ipsec_policy_free (struct xfrm_policy *x)
{
    DH_LOG_INFO(MODULE_SEC, "zxdh_ipsec_policy_free\n");
    return;
}

const struct xfrmdev_ops zxdh_xfrmdev_ops =
{
    .xdo_dev_state_add = zxdh_ipsec_add_sa,
    .xdo_dev_state_delete = zxdh_ipsec_del_sa,
    .xdo_dev_offload_ok = zxdh_ipsec_offload_ok,
    //.xdo_dev_state_advance_esn = zxdh_ipsec_state_advance_esn,
    //.xdo_dev_state_update_curlft = zxdh_ipsec_state_update_curlft,
    //.xdo_dev_policy_add = zxdh_ipsec_policy_add,
    //.xdo_dev_policy_free = zxdh_ipsec_policy_free,
};
#endif

