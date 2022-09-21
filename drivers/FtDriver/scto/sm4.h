#ifndef _SM4_H_
#define _SM4_H_

#ifdef __cplusplus
extern "C" {
#endif



#include "smx_common.h"




//some register offset
#define SKE_RESET_OFFSET          (16)
#define SKE_MODE_OFFSET           (9)
#define SKE_CRYPTO_OFFSET         (8)
#define SKE_UPDATE_IV_OFFSET      (18)
#define SKE_ERR_CFG_OFFSET        (8)


//SKE register struct
#if 1
typedef struct {
    uint32_t ctrl;                      /* Offset: 0x000 (R/W) SKE Control Register */
    uint32_t cfg;                       /* Offset: 0x004 (R/W) SKE Config Register */
    uint32_t sr_1;                      /* Offset: 0x008 (R)   SKE Status Register 1 */
    uint32_t sr_2;                      /* Offset: 0x00C (R/W) SKE Status Register 2 */
    uint32_t rev1[24];
    uint32_t iv[4];                     /* Offset: 0x070 (R/W) Initial Vector */
    uint32_t key[4];                    /* Offset: 0x080 (W)   Key */
    uint32_t rev3[44];
    uint32_t ske_version;               /* Offset: 0x140 (R)   SKE version Register */
    uint32_t rev6[47];
    uint32_t m_din[4];                  /* Offset: 0x200 (W)   SKE Input Register */
    uint32_t m_dout[4];                 /* Offset: 0x210 (R)   SKE Output Register */
} ske_reg_t;

#else

#define rSKE_CTRL			0x0//	(*((volatile uint32_t *)(SKE_BASE_ADDR)))
#define rSKE_CFG			0x4
#define rSKE_SR_1			0x8
#define rSKE_SR_2			0x0c
#define rSKE_IV(i)			(0x70+(i<<2))
#define rSKE_KEY(i)			(0x80+(i<<2))
#define rSKE_VERSION			0x140
#define rSKE_IN(i)			(0x200+(i<<2))
#define rSKE_OUT(i)			(0x210+(i<<2))

#endif



#define SM4_BLOCK_BYTE_LEN  (16)
#define SM4_BLOCK_WORD_LEN  (4)
#define SM4_KEY_BYTE_LEN    SM4_BLOCK_BYTE_LEN
#define SM4_KEY_WORD_LEN    SM4_BLOCK_WORD_LEN


//SM4 return code
enum SM4_RET_CODE
{
	SM4_SUCCESS = 0,
	SM4_BUFFER_NULL,
	SM4_CONFIG_INVALID,
	SM4_INPUT_INVALID,
};


//SM4 Operation Mode
typedef enum
{
    SM4_MODE_ECB                  = 0,   // ECB Mode
    SM4_MODE_CBC                     ,   // CBC Mode
    SM4_MODE_CFB                     ,   // CFB Mode
    SM4_MODE_OFB                     ,   // OFB Mode
    SM4_MODE_CTR                         // CTR Mode
} sm4_mode_e;


//SM4 Crypto Action
typedef enum {
    SM4_CRYPTO_ENCRYPT       = 0,   // encrypt
    SM4_CRYPTO_DECRYPT          ,   // decrypt
} sm4_crypto_e;


//API


uint8_t sm4_init(sm4_mode_e mode, sm4_crypto_e crypto, uint8_t *key, uint8_t *iv);

uint8_t sm4_crypto(uint8_t *in, uint8_t *out, uint32_t byteLen);

uint8_t sm4_dma_init(sm4_mode_e mode, sm4_crypto_e crypto, uint8_t *key, uint8_t *iv);

uint8_t sm4_dma_crypto(uint32_t *in, uint32_t *out, uint32_t byteLen);

void init_ske_reg(void);

void uninit_ske_reg(void);

int get_ske_irq_stat(void);

uint8_t ske_irq_clear(void);


#ifdef __cplusplus
}
#endif

#endif
