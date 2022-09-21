#include <linux/string.h>
#include <asm/memory.h>
#include <asm/io.h>
//#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "sm3.h"

//#define HASH_BIG_ENDIAN        //little endian is default.

//hash register pointer
//volatile static
hash_reg_t *hash_reg = NULL;
static void __iomem *smx_base = NULL;
static void __iomem *smx_dma_base = NULL;
extern u64                      kimage_voffset;

const uint32_t SM3_IV[8]= {0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600, 0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e};

void enable_global_interrupt(void)
{
    if( smx_base )
        writel(0x1, smx_base + SMX_CFG);
}

void uninit_smx_reg(void)
{
    if( smx_base ){
        iounmap(smx_base);
        smx_base = NULL;
    }

    if( hash_reg ){
        iounmap(hash_reg);
        hash_reg = NULL;
    }
}

int smx_read_data(int addr)
{
    if( !smx_base )
        smx_base = ioremap(SMX_BASE_ADDR, 0x30);
    return readl(smx_base +addr);
}
EXPORT_SYMBOL(smx_read_data);

void smx_write_data(int addr, int val)
{
    if( !smx_base )
        smx_base = ioremap(SMX_BASE_ADDR, 0x30);

    writel(val, smx_base +addr);
}
EXPORT_SYMBOL(smx_write_data);

int smx_dma_read_data(int addr)
{
    if( !smx_dma_base )
        smx_dma_base = ioremap(DMA_BASE_ADDR, 0x50);

    return readl(smx_dma_base +addr);
}
EXPORT_SYMBOL(smx_dma_read_data);

void smx_dma_write_data(int addr, long int val)
{
    if( !smx_dma_base )
        smx_dma_base = ioremap(DMA_BASE_ADDR, 0x50);

    writel(val, smx_dma_base +addr);
}
EXPORT_SYMBOL(smx_dma_write_data);

void init_dma_config(void)
{
    if( !smx_dma_base )
        smx_dma_base = ioremap(DMA_BASE_ADDR, 0x50);

    writel(0x01000270,smx_dma_base +0x40);
    writel(0x010003b0,smx_dma_base +0x50);
}

int get_smx_irq_stat(void)
{
    return smx_read_data(SMX_SR2);
}

void smx_irq_clear(void)
{
    smx_write_data(SMX_SR2, 0x7);
}

static inline void init_hash_reg(void)
{
    if( !hash_reg )
        hash_reg = (hash_reg_t *)ioremap(HASH_BASE_ADDR, 0x300);
}

void init_smx_reg(void)
{
    if( !smx_base )
        smx_base = ioremap(SMX_BASE_ADDR, 0x30);

    init_hash_reg();
}

int get_hash_irq_stat(void)
{
    init_hash_reg();
    return hash_reg->HASH_SR_2;
}

void hash_irq_clear(void)
{
    init_hash_reg();
    hash_reg->HASH_SR_2 = 1;
}

/* function: hash total byte length a = a+b
 * parameters:
 *     a -------------------------- input&output, big number a, total byte length of hash message
 *     aWordLen ------------------- input, word length of buffer a
 *     b -------------------------- input, integer to be added to a
 * return: 0(success), other(error, hash total length overflow)
 * caution:  
 */
uint8_t hash_total_len_add_uint32(uint32_t a[], uint32_t aWordLen, uint32_t b)
{
	uint32_t i;
	
        for(i=0; i<aWordLen; i++)
	{
		a[i] += b;
		if(a[i] < b)
		{
			b = 1;
		}
		else
		{
			break;
		}
	}
	
	if(i == aWordLen)
	{
		return 1;
	}
	else if(a[aWordLen-1] & 0xE0000000)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


/* function: soft reset HASH module
 * parameters: none
 * return: none
 * caution: none
 */
void hash_reset(void)
{
    init_hash_reg();
    hash_reg->HASH_CTRL |= (1 << HASH_RESET_OFFSET);
}


/* function: enable hash interruption
 * parameters: none
 * return: none
 * caution: none
 */
void hash_enable_interruption(void)
{
    init_hash_reg();
    hash_reg->HASH_CFG |= (1 << HASH_INTERRUPTION_OFFSET);
}


/* function: disable hash interruption
 * parameters: none
 * return: none
 * caution: none
 */
void hash_disable_interruption(void)
{
    init_hash_reg();
    hash_reg->HASH_CFG &= (~(1 << HASH_INTERRUPTION_OFFSET));
}


/* function: set the tag whether current block is the last message block or not
 * parameters:
 *     tag ------------------------ input, 0(no), other(yes) 
 * return: none
 * caution: 
 *     1. if it is the last block, please config hash_reg->HASH_PCR_LEN, 
 *        then the hardware will do the padding and post-processing.
 */
void hash_set_last_block(uint8_t tag)
{
        init_hash_reg();
	if(tag)
	{
            hash_reg->HASH_CFG |= (1<<HASH_LAST_OFFSET);        //current block is the last one of the message
	}
	else 
	{
            hash_reg->HASH_CFG &= (~(1<<HASH_LAST_OFFSET));	    //current block is not the last one of the message
	}
}


/* function: get current HASH value(iterator value)
 * parameters:
 *     data ----------------------- output, current hash digest iterator value
 * return: none
 * caution:
 *     1. data could be not aligned by word
 */
static inline void sm3_get_data(uint8_t *data)
{
    uint8_t i;
    uint32_t temp;

    init_hash_reg();
    if(((uint32_t)data) & 3) //for the case that data is not aligned by word
	{
        for (i = 0; i < SM3_DIGEST_WORD_LEN; i++)
		{
            temp = hash_reg->HASH_OUT[i];
            memcpy(data+(i<<2), &temp, 4);
        }
	}
	else
	{
		for (i = 0; i < SM3_DIGEST_WORD_LEN; i++) 
		{
			((uint32_t *)data)[i] = hash_reg->HASH_OUT[i];
		}
	}
}


//#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
/* function: input HASH value as current digest iterator value
 * parameters:
 *     data ----------------------- input, hash digest iterator value
 * return: none
 * caution:
 *     1. data must be aligned by word
 */
void sm3_set_data(uint32_t *data)
{
    uint8_t i;    
	
    init_hash_reg();
    for (i = 0; i < SM3_DIGEST_WORD_LEN; i++) 
	{
        hash_reg->HASH_IN[i] = data[i];
    }
}
//#endif


/* function: start HASH iteration calc
 * parameters:
 *     hash_context --------------- input, hash context
 * return: none
 * caution:
 */
static inline void hash_start_calculate(hash_context_t *hash_context)
{
        init_hash_reg();
	if(1 == hash_context->first_update_flag)
	{	
		hash_context->first_update_flag = 0;    //clear the flag
		hash_reg->HASH_CTRL |= 1;               //start to calc using default IV, ie, the initial iterator value
	}
	else 
	{
		hash_reg->HASH_CTRL |= 2;               //start to calc using the last iterator value in hash hardware
	}
}


/* function: wait till done
 * parameters: none
 * return: none
 * caution:
 */
static inline void hash_wait_till_done(void)
{
    init_hash_reg();
    while((hash_reg->HASH_SR_1 & 1))
	{udelay(100);}
}


/* function: init sm3
 * parameters:
 *     hash_context --------------- input, hash context
 * return: 0(success), other(error)
 * caution:
 */
uint8_t sm3_init(hash_context_t *hash_context)
{	
	if(NULL == hash_context)
	{
		return SM3_BUFFER_NULL;
	}

	//reset hash
	hash_reset();

	//disable interrupt
	hash_enable_interruption();
	
	//set not the last block
	hash_set_last_block(0);

	//set context config
	hash_context->first_update_flag = 1;
	hash_context->finish_flag = 0;
	smx_uint32_clear(hash_context->total, 2);

	return SM3_SUCCESS;
}


/* function: sm3 iterate calc with some blocks
 * parameters:
 *     hash_context --------------- input, hash context
 *     input ---------------------- input, message of some blocks
 *     block_count ---------------- input, count of blocks
 * return: none
 * caution: 
 *     1. please make sure the three parameters is valid
 */
static void sm3_block_calc(hash_context_t *hash_context, const uint8_t *input, uint32_t block_count)
{
	uint32_t tmp;
	uint8_t i;
	
        init_hash_reg();
#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    //set the input iterator data 
	if(1 != hash_context->first_update_flag)
	{
        sm3_set_data(hash_context->state);
	}
#endif
	
	while(block_count--)
	{
		//input the block message
#ifdef HASH_BIG_ENDIAN
		if(((uint32_t)input) & 3)
		{
			for(i=0;i<SM3_BLOCK_WORD_LEN;i++)
			{
				memcpy(&tmp, input, 4);
				hash_reg->HASH_M_DIN[i] = tmp;
				input += 4;
			}
		}
		else
		{
			for(i=0;i<SM3_BLOCK_WORD_LEN;i++)
			{
				hash_reg->HASH_M_DIN[i] = ((uint32_t *)input)[i];
			}
			input += SM3_BLOCK_WORD_LEN;
		}
#else	
		for(i=0;i<SM3_BLOCK_WORD_LEN;i++)
		{
			tmp = input[0];
			tmp <<= 8;
			tmp |= input[1];
			tmp <<= 8;
			tmp |= input[2];
			tmp <<= 8;
			tmp |= input[3];
			
			input += 4;
			hash_reg->HASH_M_DIN[i] = tmp;
		}
#endif
		
		//start hardware to calc
		hash_start_calculate(hash_context);
	
		//wait till done
		hash_wait_till_done();
	}

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    //if message update not done, get the new iterator hash value
    if(1 != hash_context->finish_flag)
	{
        sm3_get_data((uint8_t *)(hash_context->state));
	}
#endif
}


/* function: hash update message 
 * parametrs:
 *     hash_context --------------- input, hash context
 *     input ---------------------- input, message
 *     byteLen -------------------- input, byte length of the input message
 * return: 0(success), other(error)
 * caution:  
 *     1. please make sure the three parameters are valid, and hash_context is initialized
 */
uint8_t sm3_process(hash_context_t *hash_context, const uint8_t *input, uint32_t byteLen)
{
	uint8_t left, fill;
	uint32_t count;
	
	if((NULL == hash_context) || (NULL == input))
	{
		return SM3_BUFFER_NULL;
	}
	
	if(0 == byteLen)
	{
		return SM3_SUCCESS;
	}
			
#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD	
	//disable interrupt
	hash_disable_interruption();
	
	//set not the last block
	hash_set_last_block(0);
#endif
	
    left = hash_context->total[0] % SM3_BLOCK_BYTE_LEN;  //byte length of valid message left in block buffer
    fill = SM3_BLOCK_BYTE_LEN - left;                    //byte length that block buffer need to fill a block
	
	//update total byte length 
	if(hash_total_len_add_uint32(hash_context->total, SM3_BLOCK_BYTE_LEN/32, byteLen))
	{
		return SM3_LEN_OVERFLOW;
	}
	
    if(left)
	{
		if(byteLen >= fill)
		{
			memcpy(hash_context->hash_buffer + left, input, fill);
			sm3_block_calc(hash_context, hash_context->hash_buffer, 1);
			byteLen -= fill;
			input += fill;
		}
		else
		{
			memcpy(hash_context->hash_buffer + left, input, byteLen);
			goto end;
		}
	}
	
	//process some blocks
	count = byteLen/SM3_BLOCK_BYTE_LEN;
	if(count)
	{
        sm3_block_calc(hash_context, input, count);
	}
		
	//process the remainder
	input += SM3_BLOCK_BYTE_LEN*count;
	byteLen = byteLen % SM3_BLOCK_BYTE_LEN;
	if(byteLen)
	{
		memcpy(hash_context->hash_buffer, input, byteLen);
	}

end:

    return SM3_SUCCESS;
}


/* function: message update done, get the digest  
 * parametrs:
 *     hash_context --------------- input, hash context
 *     digest --------------------- output, hash digest
 * return: 0(success), other(error)
 * caution:  
 *     1. please make sure the hash_context is valid and initialized
 */
uint8_t sm3_done(hash_context_t *hash_context, uint8_t digest[32])
{
	uint8_t tmp;
	
	if((NULL == hash_context) || (NULL == digest))
	{
		return SM3_BUFFER_NULL;
	}
	
	hash_context->finish_flag = 1;    //the last block calc
	
        init_hash_reg();
	tmp = hash_context->total[0] % SM3_BLOCK_BYTE_LEN;
	if(tmp)
	{
		hash_set_last_block(1);
		
		hash_reg->HASH_PCR_LEN[0] = hash_context->total[0];
		hash_reg->HASH_PCR_LEN[1] = hash_context->total[1];

		memset(hash_context->hash_buffer + tmp, 0, SM3_BLOCK_BYTE_LEN - tmp);
	} 
	else 
	{
		hash_set_last_block(0);
				
		hash_context->hash_buffer[0] = 0x80;
		memset((hash_context->hash_buffer) + 1, 0, SM3_BLOCK_BYTE_LEN - 1 - 8);
		
		hash_context->total[1] = (hash_context->total[1]<<3)|(hash_context->total[0]>>29);
		hash_context->total[0] <<= 3;
		
#ifdef HASH_BIG_ENDIAN
		memcpy(hash_context->hash_buffer + SM3_BLOCK_BYTE_LEN - 8, &(hash_context->total[1]), 4);
		memcpy(hash_context->hash_buffer + SM3_BLOCK_BYTE_LEN - 4, &(hash_context->total[0]), 4);
#else
		smx_reverse_word((uint8_t *)(hash_context->total + 1), hash_context->hash_buffer + SM3_BLOCK_BYTE_LEN - 8, 4);
		smx_reverse_word((uint8_t *)(hash_context->total), hash_context->hash_buffer + SM3_BLOCK_BYTE_LEN - 4, 4);
#endif
	}
	
	//process the last block
	sm3_block_calc(hash_context, hash_context->hash_buffer, 1);
		
    //get the hash result
	sm3_get_data(digest);

#ifndef HASH_BIG_ENDIAN
	smx_reverse_word(digest, digest, SM3_DIGEST_BYTE_LEN);
#endif
	
	//clear the context
	memset(hash_context, 0, sizeof(hash_context_t));
	
	//reset hash to clear internal value
	hash_reset();
	
    return SM3_SUCCESS;
}


/* function: sm3 digest calculate
 * parametrs:
 *     msg ------------------------ input, message
 *     byteLen -------------------- input, byte length of the input message
 *     digest --------------------- output, sm3 digest
 * return: 0(success), other(error)
 * caution:
 */
uint8_t sm3_hash(const uint8_t *msg, uint32_t byteLen, uint8_t digest[32])
{
	hash_context_t hash_context[1];
	uint8_t ret;

	if((NULL == msg) || (NULL == digest))
	{
		return SM3_BUFFER_NULL;
	}

	ret = sm3_init(hash_context);
	if(ret)
	{
		return ret;
	}

	ret = sm3_process(hash_context, msg, byteLen);
	if(ret)
	{
		return ret;
	}

	return sm3_done(hash_context, digest);
}


/* function: init dma sm3
 * parameters:
 *     ctx ------------------------ input, hash dma context
 * return: 0(success), other(error)
 * caution:
 */
uint8_t sm3_dma_init(hash_dma_context_t *ctx)
{
	if(NULL == ctx)
	{
		return SM3_BUFFER_NULL;
	}

	//reset hash
	hash_reset();

	//disable interrupt
	hash_enable_interruption();

	//set not the last block
	hash_set_last_block(0);

	//init context
	smx_uint32_clear(ctx->total, 2);

	//set IV
	sm3_set_data((uint32_t *)SM3_IV);

	return SM3_SUCCESS;
}


/* function: dma sm3 update message
 * parametrs:
 *     ctx ------------------------ input, hash dma context
 *     input ---------------------- input, message
 *     wordLen -------------------- input, word length of the input message, must be multiple of SM3_BLOCK_WORD_LEN
 *     output --------------------- output, hash temporary result
 * return: 0(success), other(error)
 * caution:
 *     1. please make sure the four parameters are valid, and hash_context is initialized
 */
uint8_t sm3_dma_process(hash_dma_context_t *ctx, uint32_t *input, uint32_t wordLen, uint32_t output[8])
{
	if((NULL == ctx) || (NULL == input) || (NULL == output))
	{
		return SM3_BUFFER_NULL;
	}

	if(0 == wordLen)
	{
		return SM3_SUCCESS;
	}
	else if(wordLen % SM3_BLOCK_WORD_LEN)
	{
		return SM3_INPUT_INVALID;
	}

	//update total byte length
	if(hash_total_len_add_uint32(ctx->total, SM3_BLOCK_BYTE_LEN/32, wordLen<<2))
	{
		return SM3_LEN_OVERFLOW;
	}

	//src addr
	smx_dma_write_data(SMX_DMA_SADDR0,(((long)input)/4)&0xFFFFFFFF);
	smx_dma_write_data(SMX_DMA_SADDR1, ((((long)input)/4)>>32)&0x04FF);

	//dst addr
	smx_dma_write_data(SMX_DMA_DADDR0, (((long)output)/4)&0xFFFFFFFF);
	smx_dma_write_data(SMX_DMA_DADDR1,((((long)output)/4)>>32)&0x04FF);

	//data word length
	smx_dma_write_data(SMX_DMA_LEN,wordLen);

	//clear flag
	smx_write_data(SMX_SR2, (smx_read_data(SMX_SR2) | 2));

	//store cfg
	smx_write_data(SMX_CMD, (smx_read_data(SMX_CMD) | 2));

	//start
	smx_write_data(SMX_CR, 1);

	//wait till done
	while(!(smx_read_data(SMX_SR2) & 2))
	{udelay(100);}

	return SM3_SUCCESS;
}


/* function: dma sm3 last update message
 * parametrs:
 *     ctx ------------------------ input, hash dma context
 *     input ---------------------- input, message
 *     byteLen -------------------- input, byte length of the last message, must in [1, SM3_BLOCK_BYTE_LEN]
 *     output --------------------- output, hash digest
 * return: 0(success), other(error)
 * caution:
 *     1. please make sure the four parameters are valid, and hash_context is initialized
 */
uint8_t sm3_dma_last_process(hash_dma_context_t *ctx, uint32_t *input, uint32_t byteLen, uint32_t output[8])
{
        //int i;
	if((NULL == ctx) || (NULL == input) || (NULL == output))
	{
		return SM3_BUFFER_NULL;
	}

	if((0 == byteLen) || (byteLen > SM3_BLOCK_BYTE_LEN))
	{
		return SM3_INPUT_INVALID;
	}

	//update total byte length
	if(hash_total_len_add_uint32(ctx->total, SM3_BLOCK_BYTE_LEN/32, byteLen))
	{
		return SM3_LEN_OVERFLOW;
	}
        
        //printk(KERN_ERR "%s %d input: byteLen:%d\n",__func__,__LINE__,byteLen);
        //for(loop = 0; loop<16; loop++)
        //    printk(KERN_ERR "input data input[%d]:0x%x\n",loop, input[loop]);
        
        init_hash_reg();
	//set not the last block
	hash_set_last_block(1);

	//set total length of message
	hash_reg->HASH_PCR_LEN[0] = ctx->total[0];
	hash_reg->HASH_PCR_LEN[1] = ctx->total[1];

        //printk(KERN_ERR "init_hash_reg 0x%llx\n",init_hash_reg);
        //printk(KERN_ERR "input virt(0x%llx) -> phys(0x%llx)\n", (long long)input, virt_to_phys(input));
        //printk(KERN_ERR "input phys(0x%llx) -> virt(0x%llx)\n", virt_to_phys(input), phys_to_virt(virt_to_phys(input)));
        //printk(KERN_ERR "output virt(0x%llx) -> phys(0x%x)\n", (long long)output, virt_to_phys(output));
	
        //src addr
        smx_dma_write_data(SMX_DMA_SADDR0, ((virt_to_phys(input))&0xFFFFFFFF));
	smx_dma_write_data(SMX_DMA_SADDR1, (((virt_to_phys(input))>>32)&0x04FF));

	//dst addr
        smx_dma_write_data(SMX_DMA_DADDR0, ((virt_to_phys(output))&0xFFFFFFFF));
	smx_dma_write_data(SMX_DMA_DADDR1, (((virt_to_phys(output))>>32)&0x04FF));
	
        //data word length
	smx_dma_write_data(SMX_DMA_LEN, (byteLen+3)/4);

	//clear flag
	smx_write_data(SMX_SR2, (smx_read_data(SMX_SR2) | 2));

	//store cfg
	smx_write_data(SMX_CMD, (smx_read_data(SMX_CMD) | 2));

	//start
	smx_write_data(SMX_CR, 1);

        //wait till done
	while(!(smx_read_data(SMX_SR2) & 2))
	{}

        //for(i = 0; i<8; i++)
        //    printk(KERN_ERR "output[%d]:0x%x\n",i, output[i]);
        
        return SM3_SUCCESS;
}


/* function: dma sm3 digest calculate
 * parametrs:
 *     msg ------------------------ input, message
 *     byteLen -------------------- input, byte length of the last message, must bigger than 1
 *     digest --------------------- output, hash digest
 * return: 0(success), other(error)
 * caution:
 *     1. please make sure the four parameters are valid
 */
uint8_t sm3_dma_hash(const uint32_t *msg, uint32_t byteLen, uint32_t digest[8])
{
	uint32_t wordLen, lastByteLen;
	hash_dma_context_t ctx[1];
	uint8_t ret;

	if((NULL == msg) || (NULL == digest))
	{
		return SM3_BUFFER_NULL;
	}

	if(0 == byteLen)
	{
		return SM3_INPUT_INVALID;
	}

	ret = sm3_dma_init(ctx);
	if(ret)
	{
		return ret;
	}

        lastByteLen = byteLen % SM3_BLOCK_BYTE_LEN;
	wordLen = (byteLen - lastByteLen)/4;
	if(0 == lastByteLen)
	{
		lastByteLen = SM3_BLOCK_BYTE_LEN;
		wordLen -= SM3_BLOCK_WORD_LEN;
	}

	ret = sm3_dma_process(ctx, (uint32_t *)msg, wordLen, digest);
	if(ret)
	{
		return ret;
	}

	return sm3_dma_last_process(ctx, (uint32_t *)(msg+wordLen), lastByteLen, digest);
}

