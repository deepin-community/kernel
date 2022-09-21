#include <linux/types.h>
#include "smx_common.h"

/* function: set uint32 buffer
 * parameters:
 *     a -------------------------- output, output word buffer
 *     value ---------------------- input, input word value
 *     wordLen -------------------- input, word length of buffer a
 * return: none
 * caution:
 */
void pke_uint32_set(uint32_t *a, uint32_t value, uint32_t wordLen)
{
	while(wordLen)
	{
		a[--wordLen] = value;
	}
}

/* function: copy uint32 buffer
 * parameters:
 *     dst ------------------------ output, output word buffer
 *     src ------------------------ input, input word buffer
 *     wordLen -------------------- input, word length of buffer dst or src
 * return: none
 * caution:
 */
void smx_uint32_copy(uint32_t *dst, uint32_t *src, uint32_t wordLen)
{
	uint32_t i;

	if(dst != src)
	{
		for(i=0; i<wordLen; i++)
		{
			dst[i] = src[i];
		}
	}
}


/* function: clear uint32 buffer
 * parameters:
 *     a -------------------------- input&output, word buffer a
 *     aWordLen ------------------- input, word length of buffer a
 * return: none
 * caution:
 */
void smx_uint32_clear(uint32_t *a, uint32_t wordLen)
{
	while(wordLen)
	{
		a[--wordLen] = 0;
	}
}


/* function: reverse byte order in every uint32_t word
 * parameters:
 *     in ------------------------- input, input byte buffer
 *     out ------------------------ output, output word buffer
 *     bytelen -------------------- input, byte length of buffer in or out
 * return: none
 * caution:  1. byteLen must be multiple of 4
 */
void smx_reverse_word(uint8_t *in, uint8_t *out, uint32_t bytelen)
{
	uint32_t i, len;
	uint8_t tmp;
	uint8_t *p = in;

	if(in == out)
	{
		while(bytelen>0)
		{
			tmp=*p;
			*p=*(p+3);
			*(p+3)=tmp;
			p+=1;
			tmp=*p;
			*p=*(p+1);
			*(p+1)=tmp;
			bytelen-=4;
			p+=3;
		}
	}
	else
	{
	    for (i = 0; i < bytelen; i++)
		{
			len = i >> 2;
			len = len << 3;
			out[i] = p[len + 3 - i];
		}
    }
}


/* function: reverse byte order in every uint32_t word
 * parameters:
 *     in ------------------------- input, input word buffer
 *     out ------------------------ output, output word buffer
 *     wordlen -------------------- input, word length of buffer in or out
 * return: none
 * caution:  1.
 */
void smx_dma_reverse_word(uint32_t *in, uint32_t *out, uint32_t wordlen)
{
	uint32_t i;
	uint32_t tmp;
	uint32_t *p=out;

	for (i = 0; i < wordlen; i++)
	{
		tmp = *in;
		*out = tmp&0xFF;
		*out <<= 8;
		*out |= (tmp>>8)&0xFF;
		*out <<= 8;
		*out |= (tmp>>16)&0xFF;
		*out <<= 8;
		*out |= (tmp>>24)&0xFF;

		in++;
		out++;
	}

	while(wordlen>=4)
	{
		for (i = 0; i < 2; i++)
		{
			tmp = p[i];
			p[i] = p[4 - 1 - i];
			p[4 - 1 - i] = tmp;
		}//print_buf_U32(p, 4, "");
		p+=4;
		wordlen-=4;
	}
}

/* function: get real word lenth of big number a of max_words words
 * parameters:
 *     a -------------------------- input, big integer a
 *     max_words ------------------ input, max word length of a
 * return: real word lenth of big number a
 * caution:
 */
uint32_t get_valid_words(uint32_t *a, uint32_t max_words)//------------------------------
{
    uint32_t i;

    for (i = max_words; i > 0; i--)
    {
        if (a[i - 1])
        {
            return i;
        }
    }

    return 0;
}


/* function: compare big integer a and b
 * parameters:
 *     a -------------------------- input, big integer a
 *     aWordLen ------------------- input, word length of a
 *     b -------------------------- input, big integer b
 *     bWordLen ------------------- input, word length of b
 * return:
 *     0:a=b,   1:a>b,   -1: a<b
 * caution:
 */
int32_t uint32_BigNumCmp(uint32_t *a, uint32_t aWordLen, uint32_t *b, uint32_t bWordLen)
{
	int32_t i;

	aWordLen = get_valid_words(a, aWordLen);
	bWordLen = get_valid_words(b, bWordLen);

	if(aWordLen > bWordLen)
	{
		return 1;
	}

	if(aWordLen < bWordLen)
	{
		return -1;
	}

	for(i=(aWordLen-1);i>=0;i--)
	{
		if(a[i] > b[i])
		{
			return 1;
		}

		if(a[i] < b[i])
		{
			return -1;
		}
	}

	return 0;
}

