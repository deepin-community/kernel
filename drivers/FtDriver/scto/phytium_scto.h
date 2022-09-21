#ifndef _FT_RAS_H_
#define _FT_RAS_H_
 
#include <linux/types.h>

typedef struct _st_sm2_getZ
{
    uint8_t ID[1024];
    uint32_t byteLenofID;
    uint8_t pubKey[65];
    uint8_t Z[32];
}SM2_GETZ;

typedef struct _st_sm2_getE
{
    uint8_t M[128];
    uint32_t byteLen;
    uint8_t Z[32];
    uint8_t E[32];
}SM2_GETE;

typedef struct _st_sm2_sign
{
    uint8_t E[32];
    uint8_t priKey[32];
    uint8_t signature[64];
}SM2_SIGN;

typedef struct _st_sm2_verify
{
    uint8_t E[32];
    uint8_t pubKey[65];
    uint8_t signature[64];
}SM2_VERIFY;

typedef struct _st_sm2_key
{
    uint8_t priKey[32];
    uint8_t pubKey[65];
}SM2_KEY;

typedef struct _st_sm2_encrypt
{
    uint8_t M[256];
    uint32_t MByteLen;
    uint8_t pubKey[65];
    uint8_t order;
    uint8_t C[512];
    uint32_t CByteLen;
}SM2_ENCRYPT;

typedef struct _st_sm2_decrypt
{
    uint8_t C[512];
    uint32_t CByteLen;
    uint8_t priKey[32];
    uint8_t order;
    uint8_t M[256];
    uint32_t MByteLen;
}SM2_DECRYPT;

typedef struct _st_hash_context
{
    uint8_t first_update_flag;
    uint8_t finish_flag;
    uint8_t hash_buffer[64];
    uint32_t total[2];
}SM3_HASH_CONTEXT;

typedef struct _st_hash_process
{
    SM3_HASH_CONTEXT hash_context;
    uint8_t input[128];
    uint32_t byteLen;
}SM3_HASH_PROCESS;

typedef struct _st_hash_done
{
    SM3_HASH_CONTEXT hash_context;
    uint8_t digest[32];
}SM3_HASH_DONE;

typedef struct _st_hash
{
    uint8_t message[128];
    uint32_t byteLen;
    uint8_t digest[32];
}SM3_HASH;

typedef struct _st_dma_hash_context
{
    uint32_t total[2];
}SM3_DMA_HASH_CONTEXT;

typedef struct _st_dma_hash_process
{
    SM3_DMA_HASH_CONTEXT dma_hash_context;
    uint32_t input[128];
    uint32_t wordLen;
    uint32_t output[8];
}SM3_DMA_HASH_PROCESS;

typedef struct _st_dma_hash_done
{
    SM3_DMA_HASH_CONTEXT dma_hash_context;
    uint32_t input[128];
    uint32_t byteLen;
    uint32_t output[8];
}SM3_DMA_HASH_DONE;

typedef struct _st_dma_hash
{
    uint32_t message[128];
    uint32_t byteLen;
    uint32_t digest[8];
}SM3_DMA_HASH;

typedef struct _st_crypto_init
{
    int mode;
    int crypto;
    uint8_t key[128];
    uint8_t iv[128];
}SM4_INIT;

typedef struct _st_crypto_
{
    uint8_t in[128];
    uint8_t out[128];
    uint32_t byteLen;
}SM4_CRYPTO;

typedef struct _st_dma_crypto_
{
    uint32_t in[128];
    uint32_t out[128];
    uint32_t byteLen;
}SM4_DMA_CRYPTO;

#define SCTO_SM2_KEYGET            _IOR('k', 0, SM2_KEY *)
#define SCTO_SM2_ENCRYPT           _IOR('k', 1, SM2_ENCRYPT *)
#define SCTO_SM2_DECRYPT           _IOR('k', 2, SM2_DECRYPT *)
#define SCTO_SM2_GETZ              _IOR('k', 3, SM2_GETZ *)
#define SCTO_SM2_GETE              _IOR('k', 4, SM2_GETE *)
#define SCTO_SM2_SIGN              _IOR('k', 5, SM2_SIGN *)
#define SCTO_SM2_VERIFY            _IOR('k', 6, SM2_VERIFY *)

#define SCTO_SM3_INIT              _IOR('k', 9,  SM3_HASH_CONTEXT *)
#define SCTO_SM3_PROCESS           _IOR('k', 10, SM3_HASH_PROCESS *)
#define SCTO_SM3_DONE              _IOR('k', 11, SM3_HASH_DONE *)
#define SCTO_SM3_HASH              _IOR('k', 12, SM3_HASH *)
#define SCTO_SM3_DMA_INIT          _IOR('k', 13, SM3_DMA_HASH_CONTEXT *)
#define SCTO_SM3_DMA_PROCESS       _IOR('k', 14, SM3_DMA_HASH_PROCESS *)
#define SCTO_SM3_DMA_DONE          _IOR('k', 15, SM3_DMA_HASH_DONE *)
#define SCTO_SM3_DMA_HASH          _IOR('k', 16, SM3_DMA_HASH *)


#define SCTO_SM4_INIT              _IOR('k', 20,  SM4_INIT *)
#define SCTO_SM4_CRYPTO            _IOR('k', 21,  SM4_CRYPTO *)
#define SCTO_SM4_DMA_INIT          _IOR('k', 22,  SM4_INIT *)
#define SCTO_SM4_DMA_CRYPTO        _IOR('k', 23,  SM4_DMA_CRYPTO *)

#endif
