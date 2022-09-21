#ifndef _SM3_H_
#define _SM3_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "smx_common.h"


//#define CONFIG_HASH_SUPPORT_MUL_THREAD



//SM3 digest and block length
#define SM3_DIGEST_WORD_LEN   (8)
#define SM3_DIGEST_BYTE_LEN   (SM3_DIGEST_WORD_LEN<<2)
#define SM3_BLOCK_WORD_LEN    (16)
#define SM3_BLOCK_BYTE_LEN    (SM3_BLOCK_WORD_LEN<<2)


//some register offset
#define HASH_RESET_OFFSET          (16)
#define HASH_ERR_CFG_OFFSET        (16)
#define HASH_INTERRUPTION_OFFSET   (16)
#define HASH_LAST_OFFSET           (24)



//SM3 return code
enum SM3_RET_CODE
{
	SM3_SUCCESS = 0,
	SM3_BUFFER_NULL,
	SM3_INPUT_INVALID,
	SM3_LEN_OVERFLOW,
};

//HASH register struct
typedef struct {
    uint32_t HASH_CTRL;                   /* Offset: 0x000 (R/W)  Control register */
    uint32_t HASH_CFG;                    /* Offset: 0x004 (R/W)  Config register */
    uint32_t HASH_SR_1;                   /* Offset: 0x008 (R)    Status register 1 */
    uint32_t HASH_SR_2;                   /* Offset: 0x00c (R/W)  Status register 2 */
    uint32_t REV_1[4];
    uint32_t HASH_PCR_LEN[2];             /* Offset: 0x020 (R/W)  Processing control register */
    uint32_t REV_2[2];
    uint32_t HASH_OUT[8];                 /* Offset: 0x030 (R)    Output register */
    uint32_t REV_3[8];
    uint32_t HASH_IN[8];                  /* Offset: 0x070 (W)    Hash iterator Input register */
    uint32_t REV_4[8];
    uint32_t HASH_VERSION;                /* Offset: 0x0B0 (R)    Version register */
    uint32_t REV_5[19];
    uint32_t HASH_M_DIN[16];              /* Offset: 0x100 (W)    Hash message Input register */
} hash_reg_t;


//HASH context
typedef struct
{
	uint8_t first_update_flag;                      //whether first time to update message(1:yes, 0:no)
	uint8_t finish_flag;                            //whether the whole message has been inputted(1:yes, 0:no)
	uint8_t hash_buffer[SM3_BLOCK_BYTE_LEN];        //block buffer
	uint32_t total[2];                              //total byte length of the whole message

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    uint32_t state[SM3_DIGEST_WORD_LEN];            //keep current hash iterator value for multiple thread
#endif
} hash_context_t;


//HASH dma context
typedef struct
{
    uint32_t total[2];                              //total byte length of the whole message
} hash_dma_context_t;



//API

uint8_t sm3_init(hash_context_t *hash_context);

uint8_t sm3_process(hash_context_t *hash_context, const uint8_t *input, uint32_t byteLen);

uint8_t sm3_done(hash_context_t *hash_context, uint8_t digest[32]);

uint8_t sm3_hash(const uint8_t *msg, uint32_t byteLen, uint8_t digest[32]);


uint8_t sm3_dma_init(hash_dma_context_t *ctx);

uint8_t sm3_dma_process(hash_dma_context_t *ctx, uint32_t *input, uint32_t wordLen, uint32_t output[8]);

uint8_t sm3_dma_last_process(hash_dma_context_t *ctx, uint32_t *input, uint32_t byteLen, uint32_t output[8]);

uint8_t sm3_dma_hash(const uint32_t *msg, uint32_t byteLen, uint32_t digest[8]);

void enable_global_interrupt(void);

void init_smx_reg(void);

void uninit_smx_reg(void);

int get_smx_irq_stat(void);

void smx_irq_clear(void);

int get_hash_irq_stat(void);

void hash_irq_clear(void);

void init_dma_config(void);

#ifdef __cplusplus
}
#endif

#endif
