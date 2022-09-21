#ifndef _PKE_H_
#define _PKE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <asm/io.h>
#include "smx_common.h"



//PKE register
#define SM2_MEM_STEP     (32)


//PKE offset
#define PKE_INT_ENABLE_OFFSET     (8)
#define PKE_START_CALC            (1)


/********* pke microcode *********/
#define MICROCODE_PDBL         0x04
#define MICROCODE_PADD         0x08
#define MICROCODE_PVER         0x0C
#define MICROCODE_PMUL         0x10
#define MICROCODE_PMULF        0x14
#define MICROCODE_MODMUL       0x1C
#define MICROCODE_MODINV       0x20
#define MICROCODE_MODADD       0x24
#define MICROCODE_MODSUB       0x28
#define MICROCODE_ADD          0x34
#define MICROCODE_SUB          0x38
#define MICROCODE_CAL_PRE_MON  0x3C
#define MICROCODE_MGMR_PRE_N0  0x40


//PKE return code
enum PKE_RET_CODE
{
	PKE_SUCCESS = 0,
	PKE_STOP,
	PKE_NO_MODINV,
	PKE_NOT_ON_CURVE,
	PKE_INVALID_MC,
};


//some sm2 length
#define SM2_BIT_LEN       (256)
#define SM2_BYTE_LEN      (32)
#define SM2_WORD_LEN      (8)

#define SM2_MAX_ID_BYTE_LEN   (1<<13)

#define POINT_NOT_COMPRESSED      (0x04)


//SM2 error code
enum SM2_RET_CODE
{
	SM2_SUCCESS = 0,
	SM2_BUFFER_NULL = 0x50,
	SM2_NOT_ON_CURVE,
	SM2_EXCHANGE_ROLE_INVALID,
	SM2_INPUT_INVALID,
	SM2_ZERO_ALL,
	SM2_INTEGER_TOO_BIG,
	SM2_VERIFY_FAILED,
	SM2_IN_OUT_SAME_BUFFER,
	SM2_DECRY_VERIFY_FAILED
};


//SM2 key exchange role
typedef enum {
	SM2_Role_Sponsor = 0,
	SM2_Role_Responsor
} sm2_exchange_role_e;


// SM2 ciphertext order
typedef enum {
	SM2_C1C3C2   = 0,
    SM2_C1C2C3,
} sm2_cipher_order_e;


//SM2 paras
extern uint32_t const _sm2p256v1_p[8];
extern uint32_t const _sm2p256v1_p_h[8];
extern uint32_t const _sm2p256v1_a[8];
extern uint32_t const _sm2p256v1_b[8];
extern uint32_t const _sm2p256v1_Gx[8];
extern uint32_t const _sm2p256v1_Gy[8];
extern uint32_t const _sm2p256v1_n[8];
extern uint32_t const _sm2p256v1_n_h[8];




//API

uint8_t sm2_pointMul(uint32_t *k, uint32_t *Px, uint32_t *Py, uint32_t *Qx, uint32_t *Qy);

uint8_t sm2_pointAdd(uint32_t *P1x, uint32_t *P1y, uint32_t *P2x, uint32_t *P2y, uint32_t *Qx, uint32_t *Qy);

uint8_t sm2_pointVerify(uint32_t *Px, uint32_t *Py);

uint8_t sm2_getZ(uint8_t *ID, uint32_t byteLenofID, uint8_t pubKey[65], uint8_t Z[32]);

uint8_t sm2_getE(uint8_t *M, uint32_t byteLen, uint8_t Z[32], uint8_t E[32]);

uint8_t sm2_keyget(uint8_t priKey[32], uint8_t pubKey[65]);

uint8_t sm2_sign(uint8_t E[32], uint8_t priKey[32], uint8_t signature[64]);

uint8_t sm2_verify(uint8_t E[32], uint8_t pubKey[65], uint8_t signature[64]);

uint8_t sm2_encrypt(uint8_t *M, uint32_t MByteLen, uint8_t pubKey[65],
					sm2_cipher_order_e order, uint8_t *C, uint32_t *CByteLen);

uint8_t sm2_decrypt(uint8_t *C, uint32_t CByteLen, uint8_t priKey[32],
					sm2_cipher_order_e order, uint8_t *M, uint32_t *MByteLen);

uint8_t sm2_exchangekey(sm2_exchange_role_e role,
						uint8_t *dA, uint8_t *PB,
						uint8_t *rA, uint8_t *RA,
						uint8_t *RB,
						uint8_t *ZA, uint8_t *ZB,
						uint32_t kByteLen,
						uint8_t *KA, uint8_t *S1, uint8_t *SA);


#ifdef __cplusplus
}
#endif

#endif
