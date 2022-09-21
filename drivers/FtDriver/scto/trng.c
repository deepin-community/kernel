#include <linux/string.h>
#include <linux/printk.h>
#include "trng.h"



#define POST_PROCESSING_MODE       (1)
#define NO_POST_PROCESSING_MODE    (0)
static void __iomem *smx_base=NULL;



//TRNG register
#define TRNG_CR				0
#define TRNG_RTCR			0x04
#define TRNG_SR				0x08
#define TRNG_DR				0x0C
#define TRNG_FIFO_CR		        0x20
#define TRNG_FIFO_SR		        0x24
#define TRNG_HT_SR			0x70
#define TRNG_RO_CR			0x80
#define TRNG_RO_CR2			0x84
#define TRNG_RO_CR3			0x88

static inline int smx_read_data(int addr)
{
    if( !smx_base )
        smx_base = ioremap(TRNG_BASE_ADDR, 0x88);

    return readl(smx_base+addr);
}

static inline void smx_write_data(int addr, int val)
{
    if( !smx_base )
        smx_base = ioremap(TRNG_BASE_ADDR, 0x88);

    writel(val, smx_base + addr);
}

void trng_with_post_processing(void)
{
        smx_write_data(TRNG_RTCR, (smx_read_data(TRNG_RTCR) | 1));
	//TRNG_RTCR |= 1;
}


void trng_without_post_processing(void)
{
        smx_write_data(TRNG_RTCR, (smx_read_data(TRNG_RTCR) & (-1)));
	//TRNG_RTCR &= (~1);
}


void trng_enable(void)
{
        smx_write_data(TRNG_CR, (smx_read_data(TRNG_CR) | 1));
	//TRNG_CR |= 1;
}


void trng_disable(void)
{
        smx_write_data(TRNG_CR, (smx_read_data(TRNG_CR) & (-1)));
	//TRNG_CR &= (~1);
}


void trng_interrupt_init(void)
{
    int count;
    smx_write_data(TRNG_CR, 0x0);
    smx_write_data(TRNG_SR, 7);
    smx_write_data(TRNG_CR, 0x1F);
    //wait for a while
    count = 0x3F;
    while(count)
    {
        count--;
    }
}

// make sure enable_config < 16
void ro_enable(uint8_t enable_config)
{

        smx_write_data(TRNG_CR, ((smx_read_data(TRNG_CR) & (~(0x0F<<1))) | ((((uint32_t)enable_config) & 0x0F)<<1)));
	//TRNG_CR &= ~(0x0F<<1);
	//TRNG_CR |= ((((uint32_t)enable_config) & 0x0F)<<1);
}

// make sure ro_series_number < 4
void ro_internal_enable(uint8_t ro_series_number, uint16_t value)
{
	uint32_t cfg = value;

	switch(ro_series_number)
	{
	case 0:
                smx_write_data(TRNG_RO_CR, ((smx_read_data(TRNG_RO_CR) & 0x0000FFFF) | (cfg<<16)));
		//TRNG_RO_CR &= 0x0000FFFF;
		//TRNG_RO_CR |= (cfg<<16);
		break;

	case 1:
                smx_write_data(TRNG_RO_CR, ((smx_read_data(TRNG_RO_CR) & 0xFFFF0000) | cfg));
		//TRNG_RO_CR &= 0xFFFF0000;
		//TRNG_RO_CR |= cfg;
		break;

	case 2:
                smx_write_data(TRNG_RO_CR2, ((smx_read_data(TRNG_RO_CR2) & 0x0000FFFF) | (cfg<<16)));
		//TRNG_RO_CR2 &= 0x0000FFFF;
		//TRNG_RO_CR2 |= (cfg<<16);
		break;

	case 3:
                smx_write_data(TRNG_RO_CR2, ((smx_read_data(TRNG_RO_CR2) & 0xFFFF0000) | cfg));
		//TRNG_RO_CR2 &= 0xFFFF0000;
		//TRNG_RO_CR2 |= cfg;
		break;

	default:
		break;
	}
}


void trng_set_freq(uint8_t freq)
{
        smx_write_data(TRNG_RO_CR3, freq);
	//TRNG_RO_CR3 = freq;
}


void trng_set_fifo_depth(uint8_t FIFO_depth, uint8_t with_post_processing)
{
	if(with_post_processing)
	{
                smx_write_data(TRNG_FIFO_CR, (smx_read_data(TRNG_FIFO_CR) & (~(0x7F))) | FIFO_depth);
		//TRNG_FIFO_CR &= (~(0x7F));
		//TRNG_FIFO_CR |= FIFO_depth;
	}
	else
	{
                smx_write_data(TRNG_FIFO_CR, (smx_read_data(TRNG_FIFO_CR) & (~((0x7F)<<16))) | (((uint32_t)FIFO_depth)<<16));
		//TRNG_FIFO_CR &= (~((0x7F)<<16));
		//TRNG_FIFO_CR |= (((uint32_t)FIFO_depth)<<16);
	}
}


/* function: wait till rand FIFO ready
 * parameters:
 *    with_post_processing -------- 1:with, 0:without
 * return: count of words in FIFO
 * caution:
 */
uint8_t trng_wait_till_ready(uint32_t with_post_processing)
{
	uint32_t empty_flag;
        uint32_t data;
	volatile uint8_t count;

	if(with_post_processing)
	{
		empty_flag = (1<<8);
	}
	else
	{
		empty_flag = (1<<24);
	}

WAIT_TILL_READY:

	//wait till done
	while(smx_read_data(TRNG_FIFO_SR) & empty_flag)
	{
                data = smx_read_data(TRNG_FIFO_SR);

		//HT test fail
		if(smx_read_data(TRNG_SR) & 1)
		{
			trng_disable();
                        smx_write_data(TRNG_SR, 7);
			//wait for a while
			count = 0x3F;
			while(count)
			{
				count--;
			}

			trng_enable();

			//wait for a while
			count = 0x3F;
			while(count)
			{
				count--;
			}

			goto WAIT_TILL_READY;
		}
	}

	//return counts of random in FIFO
	if(with_post_processing)
	{
		return (smx_read_data(TRNG_FIFO_SR) & 0xFF);
	}
	else
	{
		return ((smx_read_data(TRNG_FIFO_SR)>>16) & 0xFF);
	}
}


/* function: get rand
 * parameters:
 *     a -------------------------- input, byte buffer a
 *     byteLen -------------------- input, byte length of rand
 *     with_post_processing ------- input, 1:with, 0:without
 * return: TRNG_SUCCESS(success), other(error)
 * caution:
 */
uint8_t get_rand_internal(uint8_t *a, uint32_t byteLen, uint8_t with_post_processing)
{
	uint32_t i;
	uint32_t tmp, rng_data;
	uint8_t count;

	if(NULL == a)
	{
		return TRNG_BUFFER_NULL;
	}

	if(0 == byteLen)
	{
		return TRNG_SUCCESS;
	}

	//make sure trng is enable and post processing config are both correct
	if((0 == (smx_read_data(TRNG_CR) & 1)) || (with_post_processing != (smx_read_data(TRNG_RTCR) & 1)))
	{
		trng_disable();

		smx_write_data(TRNG_SR,7);   //clear flag

		if(with_post_processing)
		{
			trng_with_post_processing();
		}
		else
		{
			trng_without_post_processing();
		}

		//default config
#if 0
		ro_enable(0x0F);
		ro_internal_enable(0, 0xFFFF);
		ro_internal_enable(1, 0xFFFF);
		ro_internal_enable(2, 0xFFFF);
		ro_internal_enable(3, 0xFFFF);

		trng_set_freq(TRNG_RO_FREQ_32);
		trng_set_fifo_depth(DEFAULT_FIFO_DEPTH, with_post_processing);
#endif

		trng_enable();
	}

	tmp = ((uint64_t)a) & 3;
	if(tmp)
	{
		i = 4-tmp;

		trng_wait_till_ready(with_post_processing);
		rng_data = smx_read_data(TRNG_DR);
		if(byteLen > i)
		{
			memcpy(a, &rng_data, i);
			a += i;
			byteLen -= i;
		}
		else
		{
			memcpy(a, &rng_data, byteLen);
			return TRNG_SUCCESS;
		}
	}

	tmp = byteLen/4;
	while(tmp)
	{
		count = trng_wait_till_ready(with_post_processing);
		if(count > tmp)
		{
			count = tmp;
		}

		for(i=0; i<count; i++)
		{
			*((uint32_t *)a) = smx_read_data(TRNG_DR);
			a += 4;
		}

		tmp -= count;
	}

	byteLen = byteLen & 3;
	if(byteLen)
	{
		trng_wait_till_ready(with_post_processing);
		rng_data = smx_read_data(TRNG_DR);
		memcpy(a, &rng_data, byteLen);
	}

	return TRNG_SUCCESS;
}


/* function: get output of entropy source directly
 * parameters:
 *     a -------------------------- input, byte buffer a
 *     byteLen -------------------- input, byte length of rand
 * return: TRNG_SUCCESS(success), other(error)
 * caution:
 */
uint8_t get_seed(uint8_t *a, uint32_t byteLen)
{
	return get_rand_internal(a, byteLen, NO_POST_PROCESSING_MODE);
}


/* function: get output of entropy source with post-processing
 * parameters:
 *     a -------------------------- input, byte buffer a
 *     byteLen -------------------- input, byte length of rand
 * return: TRNG_SUCCESS(success), other(error)
 * caution:
 */
uint8_t get_seed_with_post_processing(uint8_t *a, uint32_t byteLen)
{
	return get_rand_internal(a, byteLen, POST_PROCESSING_MODE);
}




uint32_t trng_get_a_word_with_post_processing(void)
{
    uint32_t i, total, count, result = 0;

    total = 1000;
    while(total)
    {
    	count = trng_wait_till_ready(POST_PROCESSING_MODE);

        if(count > total)
        {
        	count = total;
        }

        for(i=0; i<count; i++)
        {
        	result ^= smx_read_data(TRNG_DR);
        }

        total -= count;
    }

    return result;
}


/* function: get rand(entropy not reduced)
 * parameters:
 *     a -------------------------- input, byte buffer a
 *     byteLen -------------------- input, byte length of rand
 * return: TRNG_SUCCESS(success), other(error)
 * caution:
 */
uint8_t get_rand(uint8_t *a, uint32_t byteLen)
{
	uint32_t i;
	uint32_t tmp, rng_data;

	if(NULL == a)
	{
		return TRNG_BUFFER_NULL;
	}

	if(0 == byteLen)
	{
		return TRNG_SUCCESS;
	}

	//make sure trng is enable and with post processing
	if((0 == (smx_read_data(TRNG_CR) & 1))  || (0 == (smx_read_data(TRNG_RTCR) & 1)))
	{
		trng_disable();

		smx_write_data(TRNG_SR, 7);   //clear flag

		trng_with_post_processing();

		//default config
#if 0
		ro_enable(0x0F);
		ro_internal_enable(0, 0xFFFF);
		ro_internal_enable(1, 0xFFFF);
		ro_internal_enable(2, 0xFFFF);
		ro_internal_enable(3, 0xFFFF);

		trng_set_freq(TRNG_RO_FREQ_32);
		trng_set_fifo_depth(DEFAULT_FIFO_DEPTH, POST_PROCESSING_MODE);
#endif

		trng_enable();
	}

	tmp = ((uint64_t)a) & 3;
	if(tmp)
	{
		i = 4-tmp;
		rng_data = trng_get_a_word_with_post_processing();

		if(byteLen > i)
		{
			memcpy(a, &rng_data, i);
			a += i;
			byteLen -= i;
		}
		else
		{
			memcpy(a, &rng_data, byteLen);
			return TRNG_SUCCESS;
		}
	}

	tmp = byteLen/4;
	while(tmp)
	{
		*((uint32_t *)a) = trng_get_a_word_with_post_processing();
		a += 4;
		tmp--;
	}

	byteLen = byteLen & 3;
	if(byteLen)
	{
		rng_data = trng_get_a_word_with_post_processing();
		memcpy(a, &rng_data, byteLen);
	}

	return TRNG_SUCCESS;
}

