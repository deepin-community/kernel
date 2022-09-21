#include <linux/string.h>
#include <linux/io.h>
#include <linux/delay.h>
#include "sm4.h"


//#define SKE_BIG_ENDIAN        //little endian is default.

extern int smx_read_data(int addr);
extern void smx_write_data(int addr, int val);
extern int smx_dma_read_data(int addr);
extern void smx_dma_write_data(int addr, int val);

//ske register pointer
ske_reg_t *ske_reg = NULL;

void init_ske_reg(void)
{
    if( !ske_reg )
        ske_reg = (ske_reg_t *)ioremap(SKE_BASE_ADDR, 0x300);
}

void uninit_ske_reg(void)
{
    if( ske_reg)
       iounmap(ske_reg);

    ske_reg = NULL;
}

int get_ske_irq_stat(void)
{
    if( ske_reg )
        return ske_reg->sr_2;

    return 0;
}

uint8_t ske_irq_clear(void)
{
    init_ske_reg();
    ske_reg->ctrl |= (1<<SKE_RESET_OFFSET);

    return 0;
}

static inline void sm4_enable_interrupt(void)
{
    init_ske_reg();
    ske_reg->cfg |= (1 << 24);
}

//1: first time to calculate(key and iv must be set already),
//0: not first time(no need to set key and iv)
static uint8_t first_crypto_flag;

//hold current sm4 operation mode
static sm4_mode_e sm4_mode;

//count of 16 blocks
static uint32_t big_block_count;



/* function: soft reset sm4
 * parameters: none
 * return: none
 * caution: none
 */
static inline void sm4_reset(void)
{
    init_ske_reg();
    ske_reg->ctrl |= (1<<SKE_RESET_OFFSET);
}


/* function: set encryption or decryption
 * parameters:
 *     crypto --------------------- input, SM4_CRYPTO_ENCRYPT or SM4_CRYPTO_DECRYPT
 * return: none
 * caution:
 *     1. please make sure crypto is valid
 */
static inline void sm4_set_crypto(sm4_crypto_e crypto)
{
    init_ske_reg();
    ske_reg->cfg &= ~(1 << SKE_CRYPTO_OFFSET);
    ske_reg->cfg |= (crypto << SKE_CRYPTO_OFFSET);
}


/* function: set sm4 alg operation mode
 * parameters:
 *     mode ----------------------- input, operation mode
 * return: none
 * caution:
 *     1. please make sure mode is valid
 */
static inline void sm4_set_mode(sm4_mode_e mode)
{
    init_ske_reg();
    ske_reg->cfg &= ~(7 << SKE_MODE_OFFSET);      //clear bit 3
    ske_reg->cfg |= (mode << SKE_MODE_OFFSET);    //set mode
}


/* function: check ske config.
 * parameters: none
 * return: 0(correct), other(wrong)
 * caution:
 *     1. must be called after config.
 */
static inline uint8_t ske_check_config(void)
{
    init_ske_reg();
    return ske_reg->sr_1 & (1<<SKE_ERR_CFG_OFFSET);
}


/* function: wait till done
 * parameters: none
 * return: 0(success), other(error)
 * caution:
 */
void sm4_wait_till_done(void)
{

    init_ske_reg();
	while((ske_reg->sr_1 & 1) == 1)
	{udelay(100);}

	return;
}


/* function: set iv cfg before start
 * parameters: none
 * return: none
 * caution:  1. if it is the first time to calc, then use the iv/nonce already set,
 *              otherwise use the one that hardware kept or updated
 */
static inline void sm4_set_iv_cfg(void)
{
    init_ske_reg();
	if(1 != first_crypto_flag)
	{
		ske_reg->cfg &= ~(1<<SKE_UPDATE_IV_OFFSET);
	}
	else
	{
		ske_reg->cfg |= (1<<SKE_UPDATE_IV_OFFSET);
		first_crypto_flag = 0;
	}
}


/* function: start sm4 calc
 * parameters: none
 * return: none
 * caution:  1.
 */
void sm4_start_calc(void)
{
	//set iv cfg
	if(SM4_MODE_ECB != sm4_mode)
	{
		sm4_set_iv_cfg();
	}

    init_ske_reg();
	//start
    ske_reg->ctrl |= 1;
}


/* function: set key register
 * parameters:
 *     key ------------------------ input, key in word buffer
 * return: none
 * caution:  1.
 */
static inline void sm4_set_key(uint32_t key[SM4_KEY_WORD_LEN])
{
    uint8_t i=0;

    init_ske_reg();
    for (; i < SM4_KEY_WORD_LEN; i++)
	{
        ske_reg->key[i] = key[i];
    }
}


/* function: set iv register
 * parameters:
 *     mode ----------------------- input, sm4 operation mode
 *     iv ------------------------- input, iv in word buffer
 * return: none
 * caution:
 *     1. please make sure the two parameters are valid
 */
static inline void sm4_set_iv(uint32_t mode, uint32_t iv[SM4_BLOCK_WORD_LEN])
{
    uint32_t i=0;

    //set iv/nonce if the mode is not ECB
    if (SM4_MODE_ECB == mode)
	{
		return;
    }

    init_ske_reg();
	for (; i < SM4_BLOCK_WORD_LEN; i++)
	{
		ske_reg->iv[i] = iv[i];
	}
}


/* function: input one block
 * parameters:
 *     in ------------------------- input, plaintext or ciphertext in word buffer
 * return: none
 * caution:
 *     1. in is a word buffer of only one block.
 */
static inline void sm4_simple_set_input_block(uint32_t *in)
{
    uint32_t i=0;

    init_ske_reg();
	for(; i < SM4_BLOCK_WORD_LEN; i++)
	{
		ske_reg->m_din[i] = in[i];
	}
}


/* function: input one block
 * parameters:
 *     in ------------------------- input, plaintext or ciphertext in byte buffer
 *     tmp ------------------------ input, one temporary word buffer of block_word_len words.
 * return: none
 * caution:
 */
void sm4_set_input_block(uint8_t *in, uint32_t *tmp)
{
#ifdef SKE_BIG_ENDIAN
	if(((uint32_t)in) & 3)
	{
		memcpy(tmp, in, block_byte_len);
	}
	else
	{
		tmp = (uint32_t *)in;
	}
#else
	smx_reverse_word(in, (uint8_t *)tmp, SM4_BLOCK_BYTE_LEN);
#endif

	sm4_simple_set_input_block(tmp);
}


/* function: output one block
 * parameters:
 *     out ------------------------ output, one block output of sm4 in word buffer
 * return: none
 * caution:
 */
static inline void sm4_simple_get_output_block(uint32_t *out)
{
    uint32_t i=0;

    init_ske_reg();
	for(; i < SM4_BLOCK_WORD_LEN; i++)
	{
		out[i] = ske_reg->m_dout[i];
	}
}


/* function: output one block
 * parameters:
 *     out ------------------------ output, one block output of sm4 in byte buffer
 *     tmp ------------------------ input, one temporary word buffer of block_word_len words.
 * return: none
 * caution:
 */
void sm4_get_output_block(uint8_t *out, uint32_t *tmp)
{
#ifdef SKE_BIG_ENDIAN
	if(0 == (((uint32_t)out) & 3))
	{
		tmp = (uint32_t *)out;
	}

    init_ske_reg();
	for(i=0; i < SM4_BLOCK_WORD_LEN; i++)
	{
		tmp[i] = ske_reg->m_dout;//rSKE_OUT[i];
	}

	if(((uint32_t)out) & 3)
	{
		memcpy(out, tmp, SM4_BLOCK_BYTE_LEN);
	}
#else
	sm4_simple_get_output_block(tmp);
	smx_reverse_word((uint8_t *)tmp, out, SM4_BLOCK_BYTE_LEN);
#endif
}


/* function: sm4 init config
 * parameters:
 *     mode ----------------------- input, ske algorithm operation mode, like ECB,CBC,OFB,etc.
 *     crypto --------------------- input, encrypting or decrypting
 *     key ------------------------ input, key in bytes
 *     iv ------------------------- input, iv in bytes
 * return: 0(success), other(error)
 * caution:
 *     1. if mode is ECB, then there is no iv, so iv could be NULL
 */
uint8_t sm4_init(sm4_mode_e mode, sm4_crypto_e crypto, uint8_t *key, uint8_t *iv)
{
	uint32_t tmp[SM4_BLOCK_WORD_LEN];

	if((mode > SM4_MODE_CTR) || (crypto > SM4_CRYPTO_DECRYPT))
	{
		return SM4_INPUT_INVALID;
	}

	if(NULL == key)
	{
		return SM4_BUFFER_NULL;
	}

	if((NULL == iv) && (SM4_MODE_ECB != mode))
	{
		return SM4_BUFFER_NULL;
	}

	//reset sm4
	sm4_reset();
        sm4_enable_interrupt();
	//init first crypto flag;
	first_crypto_flag = 1;

	//keep mode
	sm4_mode = mode;

	//config and check
	sm4_set_mode(mode);
	sm4_set_crypto(crypto);
	if(ske_check_config())
	{
		return SM4_CONFIG_INVALID;
	}

	//set iv or nonce
	if(SM4_MODE_ECB != mode)
	{
#ifdef SKE_BIG_ENDIAN
		if(((uint32_t)iv) & 3)
		{
			memcpy(tmp, iv, block_byte_len);
			sm4_set_iv(mode, tmp);
		}
		else
		{
			sm4_set_iv(mode, (uint32_t *)iv);
		}
#else
		smx_reverse_word(iv, (uint8_t *)tmp, SM4_BLOCK_BYTE_LEN);
		sm4_set_iv(mode, tmp);
#endif
	}

	//set key
	memcpy(tmp, key, SM4_KEY_BYTE_LEN);
#ifndef SKE_BIG_ENDIAN
	smx_reverse_word((uint8_t *)tmp, (uint8_t *)tmp, SM4_KEY_BYTE_LEN);
#endif
	sm4_set_key(tmp);

	return SM4_SUCCESS;
}


/* function: sm4 encryption or decryption
 * parametrs:
 *     in ---------------- input, plaintext or ciphertext
 *     out --------------- output, ciphertext or plaintext
 *     byteLen ----------- input, byte length of input or output.
 * caution:
 *     1. to save memory, in and out could be the same buffer, in this case, the output will
 *        cover the input.
 *     2. byteLen must be multiple of block byte length.
 */
uint8_t sm4_crypto(uint8_t *in, uint8_t *out, uint32_t byteLen)
{
    uint32_t tmp[4];
    uint32_t i;

	if((NULL == in) || (NULL == out))
	{
		return SM4_BUFFER_NULL;
	}

	if(byteLen & (SM4_BLOCK_BYTE_LEN-1))
	{
		return SM4_INPUT_INVALID;
	}

    //input one block ---> calculating ---> output one block
	for (i = 0; i < byteLen; i += SM4_BLOCK_BYTE_LEN)
	{
		sm4_set_input_block(in, tmp);
		sm4_start_calc();
		sm4_wait_till_done();
		sm4_get_output_block(out, tmp);

		in += SM4_BLOCK_BYTE_LEN;
		out += SM4_BLOCK_BYTE_LEN;
	}

	return SM4_SUCCESS;
}


/* function: sm4 init config(use dma)
 * parameters:
 *     mode ----------------------- input, ske algorithm operation mode, like ECB,CBC,OFB,etc.
 *     crypto --------------------- input, encrypting or decrypting
 *     key ------------------------ input, key in bytes
 *     iv ------------------------- input, iv in bytes
 * return: 0(success), other(error)
 * caution:
 *     1. if mode is ECB, then there is no iv, so iv could be NULL
 */
uint8_t sm4_dma_init(sm4_mode_e mode, sm4_crypto_e crypto, uint8_t *key, uint8_t *iv)
{
	smx_dma_write_data(SMX_DMA_CFG, 8);

	return sm4_init(mode, crypto, key, iv);
}



#define sm4_dma_config_after_starting(in, out, current_len, next_len)    { \
in += current_len;                              \
out += current_len;                             \
smx_dma_write_data(SMX_DMA_SADDR0, (((long)in)/4)&0xFFFFFFFF);     \
smx_dma_write_data(SMX_DMA_SADDR1, ((((long)in)/4)>>32)&0x04FF);   \
smx_dma_write_data(SMX_DMA_DADDR0, (((long)out)/4)&0xFFFFFFFF);    \
smx_dma_write_data(SMX_DMA_DADDR1, ((((long)out)/4)>>32)&0x04FF);  \
smx_dma_write_data(SMX_DMA_LEN, next_len);                         \
while(!(smx_read_data(SMX_SR2) & 1))                           \
{udelay(100);}                                             \
}

/* function: sm4 encryption or decryption
 * parametrs:
 *     in ---------------- input, plaintext or ciphertext
 *     out --------------- output, ciphertext or plaintext
 *     byteLen ----------- input, byte length of input or output.
 * caution:
 *     1. to save memory, in and out could be the same buffer, in this case, the output will
 *        cover the input.
 *     2. byteLen must be multiple of block byte length.
 */
uint8_t sm4_dma_crypto(uint32_t *in, uint32_t *out, uint32_t byteLen)
#if 1
{
	uint32_t first_big_block_word_len;

	if((NULL == in) || (NULL == out))
	{
		return SM4_BUFFER_NULL;
	}

	if(byteLen & (SM4_BLOCK_BYTE_LEN-1))
	{
		return SM4_INPUT_INVALID;
	}

	first_big_block_word_len = byteLen & 0xFF;  //byteLen % 256
	if(0 == first_big_block_word_len)
	{
		first_big_block_word_len = 64;   //16 blocks
	}
	else
	{
		first_big_block_word_len >>= 2;
	}
        init_ske_reg();

	/***************************** first big block processing *******************************/
	//src addr
	smx_dma_write_data(SMX_DMA_SADDR0, (((long)in)/4)&0xFFFFFFFF);
	smx_dma_write_data(SMX_DMA_SADDR1, ((((long)in)/4)>>32)&0x04FF);

	//dst addr
	smx_dma_write_data(SMX_DMA_DADDR0, (((long)out)/4)&0xFFFFFFFF);
	smx_dma_write_data(SMX_DMA_DADDR1, ((((long)out)/4)>>32)&0x04FF);

	//data word length
	smx_dma_write_data(SMX_DMA_LEN, first_big_block_word_len);

	//clear flag
	smx_write_data(SMX_SR2, 1);

	//store cfg
	smx_write_data(SMX_CMD, 1);

	if((SM4_MODE_ECB == sm4_mode) || (0 == first_crypto_flag))
	{
		//start
		smx_write_data(SMX_CR, 1);

		sm4_dma_config_after_starting(in, out, first_big_block_word_len, 64);
	}
	else   //(SM4_MODE_ECB != sm4_mode) && (1 == first_crypto_flag)
	{
		//update iv
		ske_reg->cfg |= (1<<SKE_UPDATE_IV_OFFSET);

		//start
		smx_write_data(SMX_CR, 1);

		sm4_dma_config_after_starting(in, out, first_big_block_word_len, 64);

		//not update iv
		ske_reg->cfg &= ~(1<<SKE_UPDATE_IV_OFFSET);

		first_crypto_flag = 0;
	}
	/***************************** first big block processing end *******************************/

	//the remainder part are 16 multiple of sm4 block length
	byteLen -= first_big_block_word_len*4;
	big_block_count = byteLen>>8; //byteLen/(16*16);
	while(big_block_count--)
	{
		//clear flag
		smx_write_data(SMX_SR2, 1);

		//store cfg
		smx_write_data(SMX_CMD, 1);

		//start
		smx_write_data(SMX_CR, 1);

		sm4_dma_config_after_starting(in, out, 64, 64);
	}

	return SM4_SUCCESS;
}
#else
{
	if((NULL == in) || (NULL == out))
	{
		return SM4_BUFFER_NULL;
	}

	if(byteLen & (SM4_BLOCK_BYTE_LEN-1))
	{
		return SM4_INPUT_INVALID;
	}

	//set iv cfg
	if(SM4_MODE_ECB != sm4_mode)
	{
		sm4_set_iv_cfg();
	}

	//src addr
	smx_dma_write_data(SMX_DMA_SADDR0, (((long)in)/4)&0xFFFFFFFF);
	smx_dma_write_data(SMX_DMA_SADDR1, ((((long)in)/4)>>32)&0x04FF);

	//dst addr
	smx_dma_write_data(SMX_DMA_DADDR0, (((long)out)/4)&0xFFFFFFFF);
	smx_dma_write_data(SMX_DMA_DADDR1, ((((long)out)/4)>>32)&0x04FF);

	//data word length
	smx_write_data(SMX_DMA_LEN,(byteLen>>2));

	//clear flag
	smx_write_data(SMX_SR2, 1);

	//store cfg
	smx_write_data(SMX_CMD, 1);

	//start
	smx_write_data(SMX_CR, 1);

	//wait till done
	while(!(smx_read_data(SMX_SR2) & 1)){
            udelay(100);
        }

	return SM4_SUCCESS;
}
#endif

