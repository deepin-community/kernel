#include <linux/types.h>
#include <linux/string.h>
#include <linux/printk.h>

#include "sm2.h"
#include "sm3.h"
#include "trng.h"


//#define PKE_BIG_ENDIAN        //little endian is default.


#define SM2_HIGH_SPEED

static void __iomem *pke_base=NULL;

#define PKE_CTRL         0
#define PKE_CFG          0x04
#define PKE_MC_PTR       0x10
#define PKE_STAT         0x20
#define PKE_RT_CODE      0x24
#define PKE_VERSION      0x80
#define PKE_A(a, step)   (0x0400+(a)*(step))
#define PKE_B(a, step)   (0x1000+(a)*(step))

#define sm2_default_id_byte_len    (16)
static const char sm2_default_id[] = "1234567812345678";


//SM2 algorithm parameters
const uint32_t sm2p256v1_p[8]    = {0xFFFFFFFF,0xFFFFFFFF,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFE};
const uint32_t sm2p256v1_p_h[8]  = {0x00000003,0x00000002,0xFFFFFFFF,0x00000002,0x00000001,0x00000001,0x00000002,0x00000004};
const uint32_t sm2p256v1_a[8]    = {0xFFFFFFFC,0xFFFFFFFF,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFE};
const uint32_t sm2p256v1_b[8]    = {0x4D940E93,0xDDBCBD41,0x15AB8F92,0xF39789F5,0xCF6509A7,0x4D5A9E4B,0x9D9F5E34,0x28E9FA9E};
const uint32_t sm2p256v1_Gx[8]   = {0x334C74C7,0x715A4589,0xF2660BE1,0x8FE30BBF,0x6A39C994,0x5F990446,0x1F198119,0x32C4AE2C};
const uint32_t sm2p256v1_Gy[8]   = {0x2139F0A0,0x02DF32E5,0xC62A4740,0xD0A9877C,0x6B692153,0x59BDCEE3,0xF4F6779C,0xBC3736A2};
const uint32_t sm2p256v1_n[8]    = {0x39D54123,0x53BBF409,0x21C6052B,0x7203DF6B,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFE};
const uint32_t sm2p256v1_n_h[8]  = {0x7C114F20,0x901192AF,0xDE6FA2FA,0x3464504A,0x3AFFE0D4,0x620FC84C,0xA22B3D3B,0x1EB5E412};

//SM2 para (n-1), for private key checking
uint32_t const sm2p256v1_n_1[8]  = {0x39D54122,0x53BBF409,0x21C6052B,0x7203DF6B,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFE};

//[2^128]G, for [k]G of high speed
uint32_t const sm2p256v1_2_128_G_x[8] = {0xD13A42ED,0xEAE3D9A9,0x484E1B38,0x2B2308F6,0x88C21F3A,0x3DB7B248,0x74D55DA9,0xB692E5B5};
uint32_t const sm2p256v1_2_128_G_y[8] = {0xE295E5AB,0xD186469D,0x73438E6D,0xDB61AC17,0x544926F9,0x5A924F85,0x0F3FB613,0xA175051B};

static inline void init_pke_base(void)
{
    if( !pke_base )
        pke_base = ioremap(PKE_BASE_ADDR, 0x1080);
}

static inline int pke_read_data(int addr)
{
    if( !pke_base )
        pke_base = ioremap(PKE_BASE_ADDR, 0x1080);

    return readb(pke_base + addr);
}

static inline uint32_t pke_readl_data(int addr)
{
    if( !pke_base )
        pke_base = ioremap(PKE_BASE_ADDR, 0x1080);

    return readl(pke_base + addr);
}

static inline void pke_write_data(int addr, int val)
{
    if( !pke_base )
        pke_base = ioremap(PKE_BASE_ADDR, 0x1080);

    writeb(val, pke_base +addr);
}

/* function: C = A XOR B
 * parameters:
 *     A -------------------------- input, byte buffer a
 *     B -------------------------- input, byte buffer b
 *     C -------------------------- output, C = A XOR B
 *     byteLen -------------------- input, byte length of A,B,C
 * return: none
 * caution:
 */
void uint8_XOR(uint8_t *A, uint8_t *B, uint8_t *C, uint32_t byteLen)
{
	uint32_t i;

	for(i=0; i<byteLen; i++)
	{
		C[i] = A[i] ^ B[i];
	}
}


#ifdef PKE_BIG_ENDIAN
/* function: convert word array
 * parameters:
 *     in ------------------------- input, input buffer
 *     out ------------------------ output, output buffer
 *     wordLen -------------------- input, word length of in or out
 * return: none
 * caution:
 *    1. in and out could point the same buffer
 */
void convert_word_array(uint8_t *in, uint32_t *out, uint32_t wordLen)
{
    uint32_t idx, round = wordLen >> 1;
	uint32_t tmp;
	uint32_t *p_in;

	if(((uint32_t)(in))&3)
	{
		memcpy(out, in, wordLen<<2);
		p_in = out;
	}
	else
	{
		p_in = (uint32_t *)in;
	}

    for (idx = 0; idx < round; idx++)
	{
        tmp = p_in[idx];
        out[idx] = p_in[wordLen - 1 - idx];
        out[wordLen - 1 - idx] = tmp;
    }

    if ((wordLen & 0x1) && (p_in != out))
	{
        out[round] = p_in[round];
    }
}
#else
/* function: convert byte array
 * parameters:
 *     in ------------------------- input, input buffer
 *     out ------------------------ output, output buffer
 *     byteLen -------------------- input, byte length of in or out
 * return: none
 * caution:
 *    1. in and out could point the same buffer
 */
void convert_byte_array(uint8_t *in, uint8_t *out, uint32_t byteLen)
{
    uint32_t idx, round = byteLen >> 1;
	uint8_t tmp;

    for (idx = 0; idx < round; idx++)
	{
        tmp = in[idx];
        out[idx] = in[byteLen - 1 - idx];
        out[byteLen - 1 - idx] = tmp;
    }

    if ((byteLen & 0x1) && (in != out))
	{
        out[round] = in[round];
    }
}
#endif


/* function: check whether big number or uint8_t buffer a is all zero or not
 * parameters:
 *     a -------------------------- input, byte buffer a
 *     aByteLen ------------------- input, byte length of a
 * return: 0(a is not zero),1(a is all zero)
 * caution:
 */
uint8_t uint8_BigNum_Check_Zero(uint8_t a[], uint32_t aByteLen)
{
	uint32_t i;

	for(i=0; i<aByteLen; i++)
	{
		if(a[i])
		{
			return 0;
		}
	}

	return 1;
}


/* function: check whether big number or uint32_t buffer a is all zero or not
 * parameters:
 *     a -------------------------- input, big integer or word buffer a
 *     aWordLen ------------------- input, word length of a
 * return: 0(a is not zero), 1(a is all zero)
 * caution:
 */
uint8_t uint32_BigNum_Check_Zero(uint32_t a[], uint32_t aWordLen)
{
	uint32_t i;

	for(i=0; i<aWordLen; i++)
	{
		if(a[i])
		{
			return 0;
		}
	}

	return 1;
}


/* function: a=a+1 (for 1+dA in SM2 signing)
 * parameters:
 *     a -------------------------- input, destination data
 *     wordLen -------------------- input, word length of data
 * return: none
 * caution:
 *     1. if a of wordLen words can not hold the carry, then the carry will be discarded
 *        actually this is used in sm2 signing(1+dA)
 */
void uint32_BigNum_Add_One(uint32_t *a, uint32_t wordLen)
{
	uint32_t i, carry;

	carry = 1;
	for(i=0; i<wordLen; i++)
	{
		a[i] += carry;
		if(a[i] < carry)
		{
			carry = 1;
		}
		else
		{
			break;
		}
	}
}


/* function: load input operand to baseaddr
 * parameters:
 *     baseaddr ------------------- input, destination data
 *     data ----------------------- output, source data
 *     wordLen -------------------- input, word length of data
 * return: none
 * caution:
 */
void pke_load_operand(uint32_t *baseaddr, uint32_t *data, uint32_t wordLen)
{
    if(baseaddr != data)
	{
		smx_uint32_copy(baseaddr, data, wordLen);
	}
}


/* function: get result
 * parameters:
 *     baseaddr ------------------- input, source data
 *     data ----------------------- output, destination data
 *     wordLen -------------------- input, word length of data
 * return: none
 * caution:
 */
static void pke_read_operand(uint32_t *baseaddr, uint32_t *data, uint32_t wordLen)
{
    uint32_t i;

    if(baseaddr != data)
	{
		for (i = 0; i < wordLen; i++)
		{
			data[i] = *baseaddr;//pke_readl_data(baseaddr);
			baseaddr++;
		}
	}
}


/* function: clear and disable interrupt tag
 * parameters: none
 * return: none
 * caution:
 */
void pke_clear_interrupt(void)
{
	if(pke_read_data(PKE_STAT) & 1)
	{
                pke_write_data(PKE_STAT, pke_read_data(PKE_STAT) | 0x01);
		//PKE_STAT |= 0x01;
	}
}


/* function: enable pke interrupt
 * parameters: none
 * return: none
 * caution:
 */
static void pke_enable_interrupt(void)
{
    pke_write_data(PKE_CFG, pke_read_data(PKE_CFG) | (1 << PKE_INT_ENABLE_OFFSET));
    //PKE_CFG |= (1 << PKE_INT_ENABLE_OFFSET);
}


/* function: disable pke interrupt
 * parameters: none
 * return: none
 * caution:
 */
static void pke_disable_interrupt(void)
{
    pke_write_data(PKE_CFG, pke_read_data(PKE_CFG) & (~(1 << PKE_INT_ENABLE_OFFSET)));
    //PKE_CFG &= ~(1 << PKE_INT_ENABLE_OFFSET);
}


/* function: set operation micro code
 * parameters:
 *     addr ----------------------- input, specific micro code
 * return: none
 * caution:
 */
void pke_set_microcode(uint32_t addr)
{
        pke_write_data(PKE_MC_PTR, addr);
	//PKE_MC_PTR = addr;
}


/* function: start pke calc
 * parameters: none
 * return: none
 * caution:
 */
void pke_start(void)
{
    pke_write_data(PKE_CTRL, pke_read_data(PKE_CTRL) | PKE_START_CALC);
    //PKE_CTRL |= PKE_START_CALC;
}


/* function: return calc return code
 * parameters: none
 * return 0(success), other(error)
 * caution:
 */
uint8_t pke_check_rt_code(void)
{
    return pke_read_data(PKE_RT_CODE) & 0x07;
}

/* function: wait till done
 * parameters: none
 * return: none
 * caution:
 */
void pke_wait_till_done(void)
{
    while(!(pke_read_data(PKE_STAT) & 1))
	{;}
}


/* function: ainv = a^(-1) mod modulus
 * parameters:
 *     modulus -------------------- input, modulus
 *     a -------------------------- input, integer a
 *     ainv ----------------------- output, ainv = a^(-1) mod modulus
 *     modWordLen ----------------- input, word length of modulus
 *     aWordLen ------------------- input, word length of a
 * return: 0(success), other(inverse not exists or error)
 * caution:
 */
uint8_t sm2_modinv(const uint32_t *modulus, const uint32_t *a, uint32_t *ainv, uint32_t modWordLen,
				   uint32_t aWordLen)
{
	uint8_t ret;
        init_pke_base();
	pke_load_operand((uint32_t *)(pke_base + PKE_A(0,SM2_MEM_STEP)), (uint32_t *)modulus, modWordLen); //A0 modulus
	pke_load_operand((uint32_t *)(pke_base + PKE_B(1,SM2_MEM_STEP)), (uint32_t *)a, aWordLen);         //B1 a
	if(modWordLen > aWordLen)
	{
		smx_uint32_clear((uint32_t *)(PKE_B(1,SM2_MEM_STEP))+aWordLen, modWordLen-aWordLen);
	}

	pke_set_microcode(MICROCODE_MODINV);

	pke_clear_interrupt();

	pke_start();

	pke_wait_till_done();

	ret = pke_check_rt_code();
	if(ret)
	{
		return ret;
	}

    pke_read_operand((uint32_t *)(pke_base + PKE_A(1, SM2_MEM_STEP)), ainv, modWordLen);               //A1 ainv

	return PKE_SUCCESS;
}


/* function: out = (a+b) mod modulus
 * parameters:
 *     modulus -------------------- input, modulus
 *     a -------------------------- input, integer a
 *     b -------------------------- input, integer b
 *     out ------------------------ output, out = a+b mod modulus
 *     wordLen -------------------- input, word length of modulus, a, b
 * return: 0(success), other(error)
 * caution:
 *     1. a,b must less than modulus
 */
uint8_t sm2_modadd(const uint32_t *modulus, const uint32_t *a, const uint32_t *b,
				   uint32_t *out, uint32_t wordLen)
{
	uint8_t ret;
        init_pke_base();
	pke_load_operand((uint32_t *)(pke_base + PKE_A(0,SM2_MEM_STEP)), (uint32_t *)modulus, wordLen); //A0 modulus
	pke_load_operand((uint32_t *)(pke_base + PKE_A(1,SM2_MEM_STEP)), (uint32_t *)a, wordLen);       //A1 a
	pke_load_operand((uint32_t *)(pke_base + PKE_B(1,SM2_MEM_STEP)), (uint32_t *)b, wordLen);       //B1 b

	pke_set_microcode(MICROCODE_MODADD);

    pke_clear_interrupt();

	pke_start();

	pke_wait_till_done();

	ret = pke_check_rt_code();
	if(ret)
	{
		return ret;
	}

    pke_read_operand((uint32_t *)(pke_base + PKE_A(1, SM2_MEM_STEP)), out, wordLen);                //A1 result

	return PKE_SUCCESS;
}


/* function: out = (a-b) mod modulus
 * parameters:
 *     modulus -------------------- input, modulus
 *     a -------------------------- input, integer a
 *     b -------------------------- input, integer b
 *     out ------------------------ output, out = a-b mod modulus
 *     wordLen -------------------- input, word length of modulus, a, b
 * return: 0(success), other(error)
 * caution:
 *     1. a,b must less than modulus
 */
uint8_t sm2_modsub(const uint32_t *modulus, const uint32_t *a, const uint32_t *b,
				   uint32_t *out, uint32_t wordLen)
{
	uint8_t ret;

	init_pke_base();
	pke_load_operand((uint32_t *)(pke_base + PKE_A(0,SM2_MEM_STEP)), (uint32_t *)modulus, wordLen); //A0 modulus
	pke_load_operand((uint32_t *)(pke_base + PKE_A(1,SM2_MEM_STEP)), (uint32_t *)a, wordLen);       //A1 a
	pke_load_operand((uint32_t *)(pke_base + PKE_B(1,SM2_MEM_STEP)), (uint32_t *)b, wordLen);       //B1 b

	pke_set_microcode(MICROCODE_MODSUB);

    pke_clear_interrupt();

	pke_start();

	pke_wait_till_done();

	ret = pke_check_rt_code();
	if(ret)
	{
		return ret;
	}

    pke_read_operand((uint32_t *)(pke_base + PKE_A(1,SM2_MEM_STEP)), out, wordLen);                 //A1 result

	return PKE_SUCCESS;
}


/* function: out = a-b
 * parameters:
 *     a -------------------------- input, integer a
 *     b -------------------------- input, integer b
 *     out ------------------------ output, out = a-b
 *     wordLen -------------------- input, word length of a, b
 * return: 0(success), other(error)
 * caution:
 *     1. a,b must less than modulus
 */
uint8_t sm2_sub(const uint32_t *a, const uint32_t *b, uint32_t *out, uint32_t wordLen)
{
	uint8_t ret;

        init_pke_base();
	pke_load_operand((uint32_t *)(pke_base + PKE_A(1,SM2_MEM_STEP)), (uint32_t *)a, wordLen);       //A1 a
	pke_load_operand((uint32_t *)(pke_base + PKE_B(1,SM2_MEM_STEP)), (uint32_t *)b, wordLen);       //B1 b

	pke_set_microcode(MICROCODE_SUB);

    pke_clear_interrupt();

	pke_start();

	pke_wait_till_done();

	ret = pke_check_rt_code();
	if(ret)
	{
		return ret;
	}

    pke_read_operand((uint32_t *)(pke_base + PKE_A(1,SM2_MEM_STEP)), out, wordLen);                 //A1 result

	return PKE_SUCCESS;
}


/* function: calc n0(- modulus ^(-1) mod 2^w) for modMul, and pointMul. etc.
 * parameters: none
 * return: 0(success), other(error)
 * caution:
 *     1. before calling, please make sure the modulos is set in PKE_MEM_A
 *     2. please make sure the modulos is odd, and word length of the modulos
 *        is not bigger than 8
 *     3. the result is set in the internal register, no need to output.
 */
uint8_t pke_pre_calc_mont_N0(void)
{
    pke_set_microcode(MICROCODE_MGMR_PRE_N0);

    pke_clear_interrupt();

	pke_start();

	pke_wait_till_done();

	return pke_check_rt_code();
}


/* function: calc H(R^2 mod modulus) for modMul, and pointMul. etc.
 * parameters:
 *     modulus -------------------- input, modulus
 *     wordLen -------------------- input, word length of modulus or H
 *     H -------------------------- output, R^2 mod modulus
 * return: 0(success), other(error)
 * caution:
 *     1. modulus must be odd
 *     2. please make sure word length of buffer H is equal to wordLen(word length of modulus)
 */
static uint8_t pke_pre_calc_mont(const uint32_t *modulus, uint32_t wordLen, uint32_t *H)
{
	uint8_t ret;

        init_pke_base();
    pke_load_operand((uint32_t *)(pke_base + PKE_A(0,SM2_MEM_STEP)), (uint32_t *)modulus, wordLen);

    pke_set_microcode(MICROCODE_CAL_PRE_MON);

    pke_clear_interrupt();

	pke_start();

	pke_wait_till_done();

	ret = pke_check_rt_code();
	if(ret)
	{
		return ret;
	}

    pke_read_operand((uint32_t *)(pke_base + PKE_B(0,SM2_MEM_STEP)), H, wordLen);

    return PKE_SUCCESS;
}


/* function: like function pke_pre_calc_mont(), but this one is without output here
 * parameters:
 *     modulus -------------------- input, modulus
 *     wordLen -------------------- input, word length of modulus
 * return: 0(success), other(error)
 * caution:
 *     1. modulus must be odd
 */
uint8_t pke_pre_calc_mont_no_output(const uint32_t *modulus, uint32_t wordLen)
{
	return pke_pre_calc_mont(modulus, wordLen, (uint32_t *)(PKE_B(0,SM2_MEM_STEP)));
}


/* function: load the pre-calculated mont parameters H(R^2 mod modulus)
 * parameters:
 *     H -------------------------- input, R^2 mod modulus
 *     wordLen -------------------- input, word length of modulus or H
 * return: none
 * caution:
 *     1. please make sure the 3 input parameters are all valid
 */
void pke_load_pre_calc_mont_H(uint32_t *H,  uint32_t wordLen)
{
        init_pke_base();
	pke_load_operand((uint32_t *)(pke_base + PKE_B(0,SM2_MEM_STEP)), H, wordLen);
}


/* function: out = a*b mod modulus
 * parameters:
 *     modulus -------------------- input, modulus
 *     a -------------------------- input, integer a
 *     b -------------------------- input, integer b
 *     out ------------------------ output, out = a*b mod modulus
 *     wordLen -------------------- input, word length of modulus, a, b
 * return: 0(success), other(error)
 * caution:
 *     1. modulus must be odd
 *     2. a, b must less than modulus
 *     3. before calling this function, please make sure the pre-calculated mont arguments
 *        of modulos is located in the right address.
 */
uint8_t pke_modmul_internal(const uint32_t *modulus, const uint32_t *a, const uint32_t *b, uint32_t *out, uint32_t wordLen)
{
	uint8_t ret;

        init_pke_base();
	pke_load_operand((uint32_t *)(pke_base + PKE_A(0,SM2_MEM_STEP)), (uint32_t *)modulus, wordLen); //A0 modulus
	pke_pre_calc_mont_N0();

	pke_load_operand((uint32_t *)(pke_base + PKE_A(1,SM2_MEM_STEP)), (uint32_t *)a, wordLen);       //A1 a
	pke_load_operand((uint32_t *)(pke_base + PKE_B(1,SM2_MEM_STEP)), (uint32_t *)b, wordLen);       //B1 b

	pke_set_microcode(MICROCODE_MODMUL);

    pke_clear_interrupt();

	pke_start();

	pke_wait_till_done();

	ret = pke_check_rt_code();
	if(ret)
	{
		return ret;
	}

    pke_read_operand((uint32_t *)(pke_base + PKE_A(1,SM2_MEM_STEP)), out, wordLen);                 //A0 out

	return PKE_SUCCESS;
}


/* function: out = a*b mod modulus
 * parameters:
 *     modulus -------------------- input, modulus
 *     a -------------------------- input, integer a
 *     b -------------------------- input, integer b
 *     out ------------------------ output, out = a*b mod modulus
 *     wordLen -------------------- input, word length of modulus, a, b
 * return: 0(success), other(error)
 * caution:
 *     1. modulus must be odd
 *     2. a, b must less than modulus
 */
uint8_t pke_modmul(const uint32_t *modulus, const uint32_t *a, const uint32_t *b, uint32_t *out, uint32_t wordLen)
{
	uint8_t ret;

	ret = pke_pre_calc_mont_no_output(modulus, wordLen);
	if(ret)
	{
		return ret;
	}

	return pke_modmul_internal(modulus, a, b, out, wordLen);
}


/* function: ECCP SM2 shamir point mul(Q = [k1]P1 + [k2]P2)
 * parameters:
 *     k1 ------------------------- input, scalar k1
 *     P1x ------------------------ input, x coordinate of point P1
 *     P1y ------------------------ input, y coordinate of point P1
 *     k2 ------------------------- input, scalar k2
 *     P2x ------------------------ input, x coordinate of point P2
 *     P2y ------------------------ input, y coordinate of point P2
 *     Qx ------------------------- output, x coordinate of point Q
 *     Qy ------------------------- output, y coordinate of point Q
 * return: 0(success), other(error)
 * caution:
 *     1. please make sure k1,k2 in [1,n-1], n is order of SM2 curve
 *     2. please make sure input point P1,P2 is on the curve
 */
uint8_t sm2_pointMul_Shamir(uint32_t *k1, uint32_t *P1x, uint32_t *P1y,
							uint32_t *k2, uint32_t *P2x, uint32_t *P2y,
							uint32_t *Qx, uint32_t *Qy)
{
	uint8_t ret;

        init_pke_base();
	pke_load_operand((uint32_t *)(pke_base + PKE_A(0,SM2_MEM_STEP)), (uint32_t *)sm2p256v1_p, SM2_WORD_LEN);    //p
	pke_pre_calc_mont_N0();

	pke_load_operand((uint32_t *)(pke_base + PKE_B(0,SM2_MEM_STEP)), (uint32_t *)sm2p256v1_p_h, SM2_WORD_LEN);  //p_h

	pke_load_operand((uint32_t *)(pke_base + PKE_B(1,SM2_MEM_STEP)), P1x, SM2_WORD_LEN);                        //P1x
	pke_load_operand((uint32_t *)(pke_base + PKE_B(2,SM2_MEM_STEP)), P1y, SM2_WORD_LEN);                        //P1y
	smx_uint32_clear((uint32_t *)(pke_base + PKE_A(3,SM2_MEM_STEP)), SM2_WORD_LEN);                             //P1z=1
	pke_write_data(PKE_A(3,SM2_MEM_STEP), 1);

	pke_load_operand((uint32_t *)(pke_base + PKE_B(5,SM2_MEM_STEP)), P2x, SM2_WORD_LEN);                        //P2x
	pke_load_operand((uint32_t *)(pke_base + PKE_B(6,SM2_MEM_STEP)), P2y, SM2_WORD_LEN);                        //P2y

	pke_load_operand((uint32_t *)(pke_base + PKE_B(4,SM2_MEM_STEP)), (uint32_t *)sm2p256v1_a, SM2_WORD_LEN);    //a
	pke_load_operand((uint32_t *)(pke_base + PKE_A(4,SM2_MEM_STEP)), k1, SM2_WORD_LEN);                         //k1
	pke_load_operand((uint32_t *)(pke_base + PKE_A(5,SM2_MEM_STEP)), k2, SM2_WORD_LEN);                         //k2

	pke_set_microcode(MICROCODE_PMULF);

	pke_clear_interrupt();

	pke_start();

	pke_wait_till_done();

	ret = pke_check_rt_code();
	if(ret)
	{
		return ret;
	}

	pke_read_operand((uint32_t *)(pke_base + PKE_A(1,SM2_MEM_STEP)), Qx, SM2_WORD_LEN);
	if(Qy != NULL)
	{
		pke_read_operand((uint32_t *)(pke_base + PKE_A(2,SM2_MEM_STEP)), Qy, SM2_WORD_LEN);
	}

	return ret;
}


/* function: [k]G for SM2 curve
 * parameters:
 *     k -------------------------- input, scalar k
 *     Qx ------------------------- output, x coordinate of point Q
 *     Qy ------------------------- output, y coordinate of point Q
 * return: 0(success), other(error)
 * caution: just for the case that G is the SM2 curve base point
 */
uint8_t sm2_pointMul_base(uint32_t *k, uint32_t *x, uint32_t *y)
{
	uint32_t k1[SM2_WORD_LEN], k2[SM2_WORD_LEN];

	//k1: high half part
	smx_uint32_copy(k1, k+SM2_WORD_LEN/2, SM2_WORD_LEN/2);
	smx_uint32_clear(k1+SM2_WORD_LEN/2, SM2_WORD_LEN/2);

	//k2: low half part
	smx_uint32_copy(k2, k, SM2_WORD_LEN/2);
	smx_uint32_clear(k2+SM2_WORD_LEN/2, SM2_WORD_LEN/2);

	return sm2_pointMul_Shamir(k1, (uint32_t *)sm2p256v1_2_128_G_x, (uint32_t *)sm2p256v1_2_128_G_y,
							   k2, (uint32_t *)sm2p256v1_Gx, (uint32_t *)sm2p256v1_Gy,
							   x, y);
}


/* function: ECCP SM2 curve point mul(random point), Q=[k]P
 * parameters:
 *     k -------------------------- input, scalar
 *     Px ------------------------- input, x coordinate of point P
 *     Py ------------------------- input, y coordinate of point P
 *     Qx ------------------------- output, x coordinate of point Q
 *     Qy ------------------------- output, y coordinate of point Q
 * return: 0(success), other(error)
 * caution:
 *     1. please make sure k in [1,n-1], n is order of SM2 curve
 *     2. please make sure input point P is on the curve
 */
uint8_t sm2_pointMul(uint32_t *k, uint32_t *Px, uint32_t *Py, uint32_t *Qx, uint32_t *Qy)
{
	uint8_t ret;

        init_pke_base();
	pke_load_operand((uint32_t *)(pke_base + PKE_A(0,SM2_MEM_STEP)), (uint32_t *)sm2p256v1_p, SM2_WORD_LEN);    //p
	pke_pre_calc_mont_N0();

	pke_load_operand((uint32_t *)(pke_base + PKE_B(0,SM2_MEM_STEP)), (uint32_t *)sm2p256v1_p_h, SM2_WORD_LEN);  //p_h

	pke_load_operand((uint32_t *)(pke_base + PKE_B(1,SM2_MEM_STEP)), Px, SM2_WORD_LEN);                         //Px
	pke_load_operand((uint32_t *)(pke_base + PKE_B(2,SM2_MEM_STEP)), Py, SM2_WORD_LEN);                         //Py
	smx_uint32_clear((uint32_t *)(pke_base + PKE_A(3,SM2_MEM_STEP)), SM2_WORD_LEN);                             //Pz=1
	pke_write_data(PKE_A(3,SM2_MEM_STEP), 1);

	pke_load_operand((uint32_t *)(pke_base + PKE_B(4,SM2_MEM_STEP)), (uint32_t *)sm2p256v1_a, SM2_WORD_LEN);    //a
	pke_load_operand((uint32_t *)(pke_base + PKE_A(4,SM2_MEM_STEP)), k, SM2_WORD_LEN);                          //k

	pke_set_microcode(MICROCODE_PMUL);

	pke_clear_interrupt();

	pke_start();
	pke_wait_till_done();
	ret = pke_check_rt_code();
	if(ret)
	{
		return ret;
	}

	pke_read_operand((uint32_t *)(pke_base + PKE_A(1,SM2_MEM_STEP)), Qx, SM2_WORD_LEN);
	if(Qy != NULL)
	{
		pke_read_operand((uint32_t *)(pke_base + PKE_A(2,SM2_MEM_STEP)), Qy, SM2_WORD_LEN);
	}

	return PKE_SUCCESS;
}


/* function: ECCP SM2 curve point add, Q=P1+P2
 * parameters:
 *     P1x ------------------------ input, x coordinate of point P1
 *     P1y ------------------------ input, y coordinate of point P1
 *     P2x ------------------------ input, x coordinate of point P2
 *     P2y ------------------------ input, y coordinate of point P2
 *     Qx ------------------------- output, x coordinate of point Q=P1+P2
 *     Qy ------------------------- output, y coordinate of point Q=P1+P2
 * return: 0(success), other(error)
 * caution:
 *     1. please make sure input point P1 and P2 are both on the curve
 */
uint8_t sm2_pointAdd(uint32_t *P1x, uint32_t *P1y, uint32_t *P2x, uint32_t *P2y,
					 uint32_t *Qx, uint32_t *Qy)
{
	uint8_t ret;

        init_pke_base();
	pke_load_operand((uint32_t *)(pke_base + PKE_A(0,SM2_MEM_STEP)), (uint32_t *)sm2p256v1_p, SM2_WORD_LEN);    //p
	pke_pre_calc_mont_N0();

	pke_load_operand((uint32_t *)(pke_base + PKE_B(0,SM2_MEM_STEP)), (uint32_t *)sm2p256v1_p_h, SM2_WORD_LEN);  //p_h

	pke_load_operand((uint32_t *)(pke_base + PKE_A(1,SM2_MEM_STEP)), P1x, SM2_WORD_LEN);                       //P1x
	pke_load_operand((uint32_t *)(pke_base + PKE_A(2,SM2_MEM_STEP)), P1y, SM2_WORD_LEN);                       //P1y
	smx_uint32_clear((uint32_t *)(pke_base + PKE_B(3,SM2_MEM_STEP)), SM2_WORD_LEN);                            //P1z=1
	pke_write_data(PKE_B(3,SM2_MEM_STEP), 1);

	pke_load_operand((uint32_t *)(pke_base + PKE_B(1,SM2_MEM_STEP)), P2x, SM2_WORD_LEN);                       //P2x
	pke_load_operand((uint32_t *)(pke_base + PKE_B(2,SM2_MEM_STEP)), P2y, SM2_WORD_LEN);                       //P2y
	smx_uint32_clear((uint32_t *)(pke_base + PKE_A(3,SM2_MEM_STEP)), SM2_WORD_LEN);                            //P2z=1
	pke_write_data(PKE_A(3,SM2_MEM_STEP), 1);

	pke_set_microcode(MICROCODE_PADD);

	pke_clear_interrupt();

	pke_start();

	pke_wait_till_done();

	ret = pke_check_rt_code();
	if(ret)
	{
		return ret;
	}

	pke_read_operand((uint32_t *)(pke_base + PKE_A(1,SM2_MEM_STEP)), Qx, SM2_WORD_LEN);
	pke_read_operand((uint32_t *)(pke_base + PKE_A(2,SM2_MEM_STEP)), Qy, SM2_WORD_LEN);

	return PKE_SUCCESS;
}


/* function: check whether the input point P is on ECCP SM2 curve or not
 * parameters:
 *     Px ------------------------- input, x coordinate of point P
 *     Py ------------------------- input, y coordinate of point P
 * return: 0(success, on the curve), other(error or not on the curve)
 * caution:
 */
uint8_t sm2_pointVerify(uint32_t *Px, uint32_t *Py)
{
        init_pke_base();
	pke_load_operand((uint32_t *)(pke_base + PKE_A(0,SM2_MEM_STEP)), (uint32_t *)sm2p256v1_p, SM2_WORD_LEN);   //p
	pke_pre_calc_mont_N0();

	pke_load_operand((uint32_t *)(pke_base + PKE_B(0,SM2_MEM_STEP)), (uint32_t *)sm2p256v1_p_h, SM2_WORD_LEN); //p_h
	pke_load_operand((uint32_t *)(pke_base + PKE_A(1,SM2_MEM_STEP)), (uint32_t *)Px, SM2_WORD_LEN);            //Px
	pke_load_operand((uint32_t *)(pke_base + PKE_A(2,SM2_MEM_STEP)), (uint32_t *)Py, SM2_WORD_LEN);            //Py
	pke_load_operand((uint32_t *)(pke_base + PKE_B(4,SM2_MEM_STEP)), (uint32_t *)sm2p256v1_a, SM2_WORD_LEN);   //a
	pke_load_operand((uint32_t *)(pke_base + PKE_A(4,SM2_MEM_STEP)), (uint32_t *)sm2p256v1_b, SM2_WORD_LEN);   //b

	pke_set_microcode(MICROCODE_PVER);

	pke_clear_interrupt();

	pke_start();

	pke_wait_till_done();

	return pke_check_rt_code();
}


/* function: a=a+1 (for SM2 KDF counter addition)
 * parameters:
 *     a[4] ----------------------- input, count of 4 bytes, big-endian
 * return: none
 * caution: if a of 4 bytes can not hold the carry, then the carry will be discarded
 */
void sm2_kdf_counter_add_one(uint8_t a[4])
{
	int32_t i;
	uint8_t carry;

	carry = 1;
	for(i=3; i>=0; i--)
	{
		a[i] += carry;
		if(a[i] < carry)
		{
			carry = 1;
		}
		else
		{
			break;
		}
	}
}


/* function: SM2 kdf (for SM2 encrypting, decrypting and key exchange)
 * parameters:
 *     in ------------------------- input, sm2 kdf input
 *     inByteLen ------------------ input, byte length of in
 *     k -------------------------- output, output key
 *     kByteLen ------------------- input, byte length of output key
 * return:
 *     0(success); other(error)
 * caution:
 *     1.
 */
uint8_t sm2_kdf(uint8_t *in , uint32_t inByteLen, uint8_t *k, uint32_t kByteLen)
{
	uint8_t digest[SM3_DIGEST_BYTE_LEN];
	uint32_t i, t;
	uint8_t counter[4] = {0x00,0x00,0x00,0x01};      // count = 1;
	hash_context_t ctx[1];
	uint8_t ret;

	t = kByteLen>>5;                                 // t = kByteLen/32;
	for(i=0; i<t; i++)
	{
            ret = sm3_init(ctx);
            if(ret)
            {
                return ret;
            }

            ret = sm3_process(ctx, in, inByteLen);
            if(ret)
            {
                return ret;
            }

            ret = sm3_process(ctx, counter, 4);
            if(ret)
            {
                return ret;
            }

            ret = sm3_done(ctx, k+(i<<5));
            if(ret)
            {
                return ret;
            }

            sm2_kdf_counter_add_one(counter);
        }

        kByteLen = kByteLen & 0x1F;
        if(kByteLen)
        {
            ret = sm3_init(ctx);
            if(ret)
            {
                return ret;
            }

            ret = sm3_process(ctx, in, inByteLen);
            if(ret)
            {
                return ret;
            }

            ret = sm3_process(ctx, counter, 4);
            if(ret)
            {
                return ret;
            }

            ret = sm3_done(ctx, digest);
            if(ret)
            {
                return ret;
            }

            memcpy(k + (t<<5), digest, kByteLen);
	}

	return SM2_SUCCESS;
}


/* function: get SM2 Z value = SM3(bitLenofID||ID||a||b||Gx||Gy||Px||Py)
 * parameters:
 *     ID ------------------------- input, User ID
 *     byteLenofID ---------------- input, byte length of ID, must be less than 2^13
 *     pubKey --------------------- input, public key(0x04 + x + y), 65 bytes, big-endian
 *     Z -------------------------- output, Z value, SM3 digest, 32 bytes
 * return:
 *     0(success); other(error)
 * caution:
 *     1. bit length of ID must be less than 2^16, thus byte length must be less than 2^13
 *     2. if ID is NULL, then replace it with sm2 default ID
 *     3. please make sure the pubKey is valid
 */
uint8_t sm2_getZ(uint8_t *ID, uint32_t byteLenofID, uint8_t pubKey[65], uint8_t Z[32])
{
	uint32_t tmp[SM2_WORD_LEN];
	hash_context_t ctx[1];
	uint8_t ret;

	if(NULL == pubKey || NULL == Z)
	{
		return SM2_BUFFER_NULL;
	}

	if(NULL == ID)
	{
		ID = (uint8_t *)sm2_default_id;
		byteLenofID = sm2_default_id_byte_len;
	}
	else if((0 == byteLenofID) || (byteLenofID >= SM2_MAX_ID_BYTE_LEN))
	{
		return SM2_INPUT_INVALID;
	}

	if(POINT_NOT_COMPRESSED != pubKey[0])
	{
		return SM2_INPUT_INVALID;
	}

	ret = sm3_init(ctx);
	if(ret)
	{
		goto end;
	}

	byteLenofID <<= 3;
	ret = (byteLenofID>>8) & 0xFF;
	ret = sm3_process(ctx, (uint8_t *)&ret, 1);
	if(ret)
	{
		goto end;
	}

	ret = byteLenofID & 0xFF;
	ret = sm3_process(ctx, (uint8_t *)&ret, 1);
	if(ret)
	{
		goto end;
	}

	byteLenofID >>= 3;
	ret = sm3_process(ctx, ID, byteLenofID);
	if(ret)
	{
		goto end;
	}

#ifdef PKE_BIG_ENDIAN
	convert_word_array((uint8_t *)sm2p256v1_a, tmp, SM2_WORD_LEN);
#else
	convert_byte_array((uint8_t *)sm2p256v1_a, (uint8_t *)tmp, SM2_BYTE_LEN);
#endif
	ret = sm3_process(ctx, (uint8_t *)tmp, SM2_BYTE_LEN);
	if(ret)
	{
		goto end;
	}

#ifdef PKE_BIG_ENDIAN
	convert_word_array((uint8_t *)sm2p256v1_b, tmp, SM2_WORD_LEN);
#else
	convert_byte_array((uint8_t *)sm2p256v1_b, (uint8_t *)tmp, SM2_BYTE_LEN);
#endif
	ret = sm3_process(ctx, (uint8_t *)tmp, SM2_BYTE_LEN);
	if(ret)
	{
		goto end;
	}

#ifdef PKE_BIG_ENDIAN
	convert_word_array((uint8_t *)sm2p256v1_Gx, tmp, SM2_WORD_LEN);
#else
	convert_byte_array((uint8_t *)sm2p256v1_Gx, (uint8_t *)tmp, SM2_BYTE_LEN);
#endif
	ret = sm3_process(ctx, (uint8_t *)tmp, SM2_BYTE_LEN);
	if(ret)
	{
		goto end;
	}

#ifdef PKE_BIG_ENDIAN
	convert_word_array((uint8_t *)sm2p256v1_Gy, tmp, SM2_WORD_LEN);
#else
	convert_byte_array((uint8_t *)sm2p256v1_Gy, (uint8_t *)tmp, SM2_BYTE_LEN);
#endif
	ret = sm3_process(ctx, (uint8_t *)tmp, SM2_BYTE_LEN);
	if(ret)
	{
		goto end;
	}

	ret = sm3_process(ctx, pubKey+1, SM2_BYTE_LEN<<1);
	if(ret)
	{
		goto end;
	}

	ret = sm3_done(ctx, Z);
	if(ret)
	{
		goto end;
	}

	ret = SM2_SUCCESS;

end:

	return ret;
}


/* function: get SM2 E value = SM3(Z||M)
 * parameters:
 *     M      --------------------- input, Message
 *     byteLen -------------------- input, byte length of M
 *     Z      --------------------- input, Z value, 32 bytes
 *     E      --------------------- output, E value, 32 bytes
 * return:
 *     0(success); other(error)
 * caution:
 */
uint8_t sm2_getE(uint8_t *M, uint32_t byteLen, uint8_t Z[32], uint8_t E[32])
{
	hash_context_t ctx[1];
	uint8_t ret;

	if(NULL == M || NULL == Z || NULL == E)
	{
		return SM2_BUFFER_NULL;
	}

	if(0 == byteLen)
	{
		return SM2_INPUT_INVALID;
	}

	ret = sm3_init(ctx);
	if(ret)
	{
		goto end;
	}

	ret = sm3_process(ctx, Z, 32);
	if(ret)
	{
		goto end;
	}

	ret = sm3_process(ctx, M, byteLen);
	if(ret)
	{
		goto end;
	}

	ret = sm3_done(ctx, E);
	if(ret)
	{
		goto end;
	}

	ret = SM2_SUCCESS;

end:

	return ret;
}


/* function: Generate SM2 Key pair
 * parameters:
 *     priKey --------------------- output, private key, 32 bytes, big-endian
 *     pubKey --------------------- output, public key(0x04 + x + y), 65 bytes, big-endian
 * return:
 *     0(success); other(error)
 * caution:
 */
uint8_t sm2_keyget(uint8_t priKey[32], uint8_t pubKey[65])
{
	uint32_t k[SM2_WORD_LEN], tmp[SM2_WORD_LEN<<1];
	uint8_t ret;

	if(NULL == priKey || NULL == pubKey)
	{
		return SM2_BUFFER_NULL;
	}

SM2_GETKEY_LOOP:
	ret = get_rand((uint8_t *)k, SM2_BYTE_LEN);
	if(ret)
	{
		return ret;
	}
	//make sure priKey in [1, n-2]
	if(uint32_BigNum_Check_Zero(k, SM2_WORD_LEN))
	{
		goto SM2_GETKEY_LOOP;
	}
	if(uint32_BigNumCmp(k, SM2_WORD_LEN, (uint32_t *)sm2p256v1_n_1, SM2_WORD_LEN) >= 0)
	{
		goto SM2_GETKEY_LOOP;
	}

#ifdef SM2_HIGH_SPEED
	ret = sm2_pointMul_base(k, tmp, tmp+SM2_WORD_LEN);
#else
	ret = sm2_pointMul(k, (uint32_t *)sm2p256v1_Gx, (uint32_t *)sm2p256v1_Gy, tmp, tmp+SM2_WORD_LEN);
#endif
	if(ret)
	{
		return ret;
	}

	pubKey[0] = POINT_NOT_COMPRESSED;
#ifdef PKE_BIG_ENDIAN
	convert_word_array((uint8_t *)k, k, SM2_WORD_LEN);
	memcpy(priKey, k, SM2_BYTE_LEN);
	convert_word_array((uint8_t *)tmp, k, SM2_WORD_LEN);
	memcpy(pubKey+1, k, SM2_BYTE_LEN);
	convert_word_array((uint8_t *)(tmp+SM2_WORD_LEN), k, SM2_WORD_LEN);
	memcpy(pubKey+1+SM2_BYTE_LEN, k, SM2_BYTE_LEN);
#else
	convert_byte_array((uint8_t *)k, priKey, SM2_BYTE_LEN);
	convert_byte_array((uint8_t *)tmp, pubKey+1, SM2_BYTE_LEN);
	convert_byte_array((uint8_t *)(tmp+SM2_WORD_LEN), pubKey+1+SM2_BYTE_LEN, SM2_BYTE_LEN);
#endif

	return SM2_SUCCESS;
}


/* function: Generate SM2 Signature r and s with rand k
 * parameters:
 *     e[8]   --------------------- input, e value, 8 words, little-endian
 *     k[8]   --------------------- input, random number k, 8 words, little-endian
 *     dA[8]  --------------------- input, private key, 8 words, little-endian
 *     r[8]   --------------------- output, Signature r, 8 words, little-endian
 *     s[8]   --------------------- output, Signature s, 8 words, little-endian
 * return:
 *     0(success); other(error)
 * caution:
 *     1. e and dA can not be modified
 *     2. e must be less than n(order of the SM2 curve)
 *     3. dA must be in [1, n-2]
 */
uint8_t sm2_sign_with_k(uint32_t e[8], uint32_t k[8], uint32_t dA[8], uint32_t r[8], uint32_t s[8])
{
	uint32_t tmp1[SM2_WORD_LEN], tmp2[SM2_WORD_LEN];
	uint8_t ret;

	if(NULL == e || NULL == k || NULL == dA || NULL == r || NULL == s)
	{
		return SM2_BUFFER_NULL;
	}

	//make sure k in [1, n-1]
	if(uint32_BigNum_Check_Zero(k, SM2_WORD_LEN))
	{
		return SM2_ZERO_ALL;
	}
	if(uint32_BigNumCmp(k, SM2_WORD_LEN, (uint32_t *)sm2p256v1_n, SM2_WORD_LEN) >= 0)
	{
		return SM2_INTEGER_TOO_BIG;
	}

#ifdef SM2_HIGH_SPEED
	ret = sm2_pointMul_base(k, tmp1, NULL);
#else
	ret = sm2_pointMul(k, (uint32_t *)sm2p256v1_Gx, (uint32_t *)sm2p256v1_Gy, tmp1, NULL);
#endif
	if(ret)
	{
		return ret;
	}

	//tmp1 = x1 mod n
	if(uint32_BigNumCmp(tmp1, SM2_WORD_LEN, (uint32_t *)sm2p256v1_n, SM2_WORD_LEN) >= 0)
	{
		ret = sm2_sub(tmp1, (uint32_t *)sm2p256v1_n, tmp1, SM2_WORD_LEN);
		if(ret)
		{
			return ret;
		}
	}

	//r = e + x1 mod n
	ret = sm2_modadd((uint32_t *)sm2p256v1_n, e, tmp1, r, SM2_WORD_LEN);
	if(ret)
	{
		return ret;
	}

	//make sure r is not zero
	if(uint32_BigNum_Check_Zero(r, SM2_WORD_LEN))
	{
		return SM2_ZERO_ALL;
	}

	//tmp1 = r + k mod n
	ret = sm2_modadd((uint32_t *)sm2p256v1_n, r, k, tmp1, SM2_WORD_LEN);
	if(ret)
	{
		return ret;
	}
	//make sure r+k is not n
	if(uint32_BigNum_Check_Zero(tmp1, SM2_WORD_LEN))
	{
		return SM2_ZERO_ALL;
	}

	//tmp1 =  r*dA mod n
	pke_load_pre_calc_mont_H((uint32_t *)sm2p256v1_n_h, SM2_WORD_LEN);
	ret = pke_modmul_internal((uint32_t *)sm2p256v1_n, r, dA, tmp1, SM2_WORD_LEN);
	if(ret)
	{
		return ret;
	}

	//tmp1 =  (k - r*dA) mod n
	ret = sm2_modsub((uint32_t *)sm2p256v1_n, k, tmp1, tmp1, SM2_WORD_LEN);
	if(ret)
	{
		return ret;
	}

	//tmp2 = (1+dA)^(-1) mod n
	smx_uint32_copy(tmp2, dA, SM2_WORD_LEN);
	uint32_BigNum_Add_One(tmp2, SM2_WORD_LEN);
	ret = sm2_modinv((uint32_t *)sm2p256v1_n, tmp2, tmp2, SM2_WORD_LEN, SM2_WORD_LEN);
	if(ret)
	{
		return ret;
	}

	//s = ((1+dA)^(-1))*(k - r*dA) mod n
	//pke_load_pre_calc_mont_H((uint32_t *)sm2p256v1_n_h, SM2_WORD_LEN);
	ret = pke_modmul_internal((uint32_t *)sm2p256v1_n, tmp1, tmp2, s, SM2_WORD_LEN);
	if(ret)
	{
		return ret;
	}

	//make sure s is not zero
	if(uint32_BigNum_Check_Zero(s, SM2_WORD_LEN))
	{
		return SM2_ZERO_ALL;
	}


	return SM2_SUCCESS;
}


/* function: Generate SM2 Signature
 * parameters:
 *     E[32]       ------ input, E value, 32 bytes, big-endian
 *     priKey[32]  ------ input, private key, 32 bytes, big-endian
 *     signature[64] ---- output, Signature r and s, 64 bytes, big-endian
 * return:
 *     0(success); other(error)
 * caution:
 */
uint8_t sm2_sign(uint8_t E[32], uint8_t priKey[32], uint8_t signature[64])
{
	uint32_t e[SM2_WORD_LEN], k[SM2_WORD_LEN], dA[SM2_WORD_LEN], r[SM2_WORD_LEN], s[SM2_WORD_LEN];
	uint8_t ret;

	if(NULL == E || NULL == priKey || NULL == signature)
	{
		return SM2_BUFFER_NULL;
	}

    //e = e mod n
#ifdef PKE_BIG_ENDIAN
	convert_word_array(E, e, SM2_WORD_LEN);
#else
	convert_byte_array(E, (uint8_t *)e, SM2_BYTE_LEN);
#endif
	if(uint32_BigNumCmp(e, SM2_WORD_LEN, (uint32_t *)sm2p256v1_n, SM2_WORD_LEN) >= 0)
	{
		ret = sm2_sub(e, (uint32_t *)sm2p256v1_n, e, SM2_WORD_LEN);
		if(ret)
		{
			return ret;
		}
	}

	//make sure priKey in [1, n-2]
#ifdef PKE_BIG_ENDIAN
	convert_word_array(priKey, dA, SM2_WORD_LEN);
#else
	convert_byte_array(priKey, (uint8_t *)dA, SM2_BYTE_LEN);
#endif
	if(uint32_BigNum_Check_Zero(dA, SM2_WORD_LEN))
	{
		return SM2_ZERO_ALL;
	}
	if(uint32_BigNumCmp(dA, SM2_WORD_LEN, (uint32_t *)sm2p256v1_n_1, SM2_WORD_LEN) >= 0)
	{
		return SM2_INTEGER_TOO_BIG;
	}

SM2_SIGN_LOOP:
	ret = get_rand((uint8_t *)k, SM2_BYTE_LEN);
	if(ret)
	{
		return ret;
	}
	ret = sm2_sign_with_k(e, k, dA, r, s);
	if(SM2_ZERO_ALL == ret || SM2_INTEGER_TOO_BIG == ret)
	{
		goto SM2_SIGN_LOOP;
	}
	else if(ret)
	{
		return ret;
	}

#ifdef PKE_BIG_ENDIAN
	if(((uint32_t)(signature)) & 3)
	{
		convert_word_array((uint8_t *)r, r, SM2_WORD_LEN);
		convert_word_array((uint8_t *)s, s, SM2_WORD_LEN);
		memcpy(signature, r, SM2_BYTE_LEN);
		memcpy(signature+SM2_BYTE_LEN, s, SM2_BYTE_LEN);
	}
	else
	{
		convert_word_array((uint8_t *)r, (uint32_t *)signature, SM2_WORD_LEN);
		convert_word_array((uint8_t *)s, (uint32_t *)(signature+SM2_BYTE_LEN), SM2_WORD_LEN);
	}
#else
	convert_byte_array((uint8_t *)r, signature, SM2_BYTE_LEN);
	convert_byte_array((uint8_t *)s, signature+SM2_BYTE_LEN, SM2_BYTE_LEN);
#endif

	return SM2_SUCCESS;
}


/* function: Verify SM2 Signature
 * parameters:
 *     E[32]       ---------------- input, E value, 32 bytes, big-endian
 *     pubKey[65]  ---------------- input, public key(0x04 + x + y), 65 bytes, big-endian
 *     signature[64] -------------- input, Signature r and s, 64 bytes, big-endian
 * return:
 *     0(success, the signature is valid); other(error or the signature is invalid)
 * caution:
 */
uint8_t sm2_verify(uint8_t E[32], uint8_t pubKey[65], uint8_t signature[64])
{
	uint32_t e[SM2_WORD_LEN], r[SM2_WORD_LEN], s[SM2_WORD_LEN], tmp[SM2_WORD_LEN*4];
	uint32_t *t = e;
	uint8_t ret;

	if(NULL == E || NULL == pubKey || NULL == signature)
	{
		return SM2_BUFFER_NULL;
	}

	//make sure pubKey[0] is POINT_NOT_COMPRESSED
	if(POINT_NOT_COMPRESSED != pubKey[0])
	{
		return SM2_INPUT_INVALID;
	}

	//make sure r in [1, n-1]
#ifdef PKE_BIG_ENDIAN
	convert_word_array(signature, r, SM2_WORD_LEN);
#else
	convert_byte_array(signature, (uint8_t *)r, SM2_BYTE_LEN);
#endif
	if(uint32_BigNum_Check_Zero(r, SM2_WORD_LEN))
	{
		ret = SM2_ZERO_ALL;
		goto end;
	}
	if(uint32_BigNumCmp(r, SM2_WORD_LEN, (uint32_t *)sm2p256v1_n, SM2_WORD_LEN) >= 0)
	{
		ret = SM2_INTEGER_TOO_BIG;
		goto end;
	}

	//make sure s in [1, n-1]
#ifdef PKE_BIG_ENDIAN
	convert_word_array(signature+SM2_BYTE_LEN, s, SM2_WORD_LEN);
#else
	convert_byte_array(signature+SM2_BYTE_LEN, (uint8_t *)s, SM2_BYTE_LEN);
#endif
	if(uint32_BigNum_Check_Zero(s, SM2_WORD_LEN))
	{
		ret = SM2_ZERO_ALL;
		goto end;
	}
	if(uint32_BigNumCmp(s, SM2_WORD_LEN, (uint32_t *)sm2p256v1_n, SM2_WORD_LEN) >= 0)
	{
		ret = SM2_INTEGER_TOO_BIG;
		goto end;
	}

	//t = (r+s) mod n
	ret = sm2_modadd((uint32_t *)sm2p256v1_n, r, s, t, SM2_WORD_LEN);
	if(ret)
	{
		goto end;
	}

	//if t is 0, refuse the signature
	if(uint32_BigNum_Check_Zero(t, SM2_WORD_LEN))
	{
		ret = SM2_ZERO_ALL;
		goto end;
	}

	//get PA
#ifdef PKE_BIG_ENDIAN
	convert_word_array(pubKey+1, tmp+2*SM2_WORD_LEN, SM2_WORD_LEN);
	convert_word_array(pubKey+1+SM2_BYTE_LEN, tmp+3*SM2_WORD_LEN, SM2_WORD_LEN);
#else
	convert_byte_array(pubKey+1, (uint8_t *)(tmp+2*SM2_WORD_LEN), SM2_BYTE_LEN);
	convert_byte_array(pubKey+1+SM2_BYTE_LEN, (uint8_t *)(tmp+3*SM2_WORD_LEN), SM2_BYTE_LEN);
#endif

#ifdef SM2_HIGH_SPEED
	ret = sm2_pointMul_Shamir(s, (uint32_t *)sm2p256v1_Gx, (uint32_t *)sm2p256v1_Gy,
							  t, tmp+2*SM2_WORD_LEN, tmp+3*SM2_WORD_LEN,
							  tmp, NULL);
#else
	//[s]G
	ret = sm2_pointMul(s, (uint32_t *)sm2p256v1_Gx, (uint32_t *)sm2p256v1_Gy, tmp, tmp+SM2_WORD_LEN);
	if(ret)
	{
		goto end;
	}

	//[t]PA
	ret = sm2_pointMul(t, tmp+2*SM2_WORD_LEN, tmp+3*SM2_WORD_LEN, tmp+2*SM2_WORD_LEN,
						tmp+3*SM2_WORD_LEN);
	if(ret)
	{
		goto end;
	}

	//[s]G + [t]PA
	ret = sm2_pointAdd(tmp, tmp+SM2_WORD_LEN, tmp+2*SM2_WORD_LEN, tmp+3*SM2_WORD_LEN,
						tmp, NULL);
#endif
	if(ret)
	{
		goto end;
	}

	//e = e mod n
#ifdef PKE_BIG_ENDIAN
	convert_word_array(E, e, SM2_WORD_LEN);
#else
	convert_byte_array(E, (uint8_t *)e, SM2_BYTE_LEN);
#endif
	if(uint32_BigNumCmp(e, SM2_WORD_LEN, (uint32_t *)sm2p256v1_n, SM2_WORD_LEN) >= 0)
	{
		ret = sm2_sub(e, (uint32_t *)sm2p256v1_n, e, SM2_WORD_LEN);
		if(ret)
		{
			goto end;
		}
	}

	//tmp = x1 mod n
	if(uint32_BigNumCmp(tmp, SM2_WORD_LEN, (uint32_t *)sm2p256v1_n, SM2_WORD_LEN) >= 0)
	{
		ret = sm2_sub(tmp, (uint32_t *)sm2p256v1_n, tmp, SM2_WORD_LEN);
		if(ret)
		{
			goto end;
		}
	}

	//tmp = e + x1 mod n
	ret = sm2_modadd((uint32_t *)sm2p256v1_n, e, tmp, tmp, SM2_WORD_LEN);
	if(ret)
	{
		goto end;
	}

	//cmp
	if(uint32_BigNumCmp(tmp, SM2_WORD_LEN, r, SM2_WORD_LEN))
	{
		ret = SM2_VERIFY_FAILED;
		goto end;
	}

	//success
	ret = SM2_SUCCESS;

end:

	return ret;
}


/* function: SM2 Encryption with rand k
 * parameters:
 *     M -------------------------- input, plaintext, MByteLen bytes, big-endian
 *     MByteLen ------------------- input, byte length of M
 *     k[8] ----------------------- input, random number k, 8 words, little-endian
 *     pubkey_x ------------------- input, x coordinate of public key point, 8 words, little-endian
 *     pubkey_y ------------------- input, y coordinate of public key point, 8 words, little-endian
 *     order ---------------------- input, 0(the output C will be C1||C3||C2); 1(the output C will be C1||C2||C3)
 *     C -------------------------- output, ciphertext, CByteLen bytes, big-endian
 *     CByteLen ------------------- output, byte length of C, should be MByteLen+97 if success
 * return:
 *     0(success); other(error)
 * caution:
 *     1. M and C can not be the same buffer
 *     2. please make sure pubkey_x and pubkey_y are valid
 */
uint8_t sm2_encrypt_with_k(uint8_t *M, uint32_t MByteLen, uint32_t *k,
						   uint32_t *pubkey_x, uint32_t *pubkey_y,
						   sm2_cipher_order_e order,
						   uint8_t *C, uint32_t *CByteLen)
{
	uint32_t xy[SM2_WORD_LEN<<1];
	uint8_t *C2, *C3;
	hash_context_t ctx[1];
	uint8_t ret;

	if(NULL == M || NULL == k || NULL == pubkey_x || NULL == pubkey_y || NULL == C || NULL == CByteLen)
	{
		return SM2_BUFFER_NULL;
	}

	if(MByteLen == 0)
	{
		return SM2_INPUT_INVALID;
	}

	if(order > SM2_C1C2C3)
	{
		return SM2_INPUT_INVALID;
	}

	if(M == C)
	{
		return SM2_IN_OUT_SAME_BUFFER;
	}

	//make sure k in [1, n-1]
	if(uint32_BigNum_Check_Zero(k, SM2_WORD_LEN))
	{
		return SM2_ZERO_ALL;
	}
	if(uint32_BigNumCmp(k, SM2_WORD_LEN, (uint32_t *)sm2p256v1_n, SM2_WORD_LEN) >= 0)
	{
		return SM2_INTEGER_TOO_BIG;
	}

	//get [k]G
#ifdef SM2_HIGH_SPEED
	ret = sm2_pointMul_base(k, xy, xy+SM2_WORD_LEN);
#else
	ret = sm2_pointMul(k, (uint32_t *)sm2p256v1_Gx, (uint32_t *)sm2p256v1_Gy, xy, xy+SM2_WORD_LEN);
#endif
	if(ret)
	{
		return ret;
	}

	//output C1
	C[0] = POINT_NOT_COMPRESSED;
#ifdef PKE_BIG_ENDIAN
	convert_word_array((uint8_t *)xy, xy, SM2_WORD_LEN);
	convert_word_array((uint8_t *)(xy+SM2_WORD_LEN), xy+SM2_WORD_LEN, SM2_WORD_LEN);
	memcpy(C+1, xy, SM2_BYTE_LEN);
	memcpy(C+1+SM2_BYTE_LEN, xy+SM2_WORD_LEN, SM2_BYTE_LEN);
#else
	convert_byte_array((uint8_t *)xy, C+1, SM2_BYTE_LEN);
	convert_byte_array((uint8_t *)(xy+SM2_WORD_LEN), C+1+SM2_BYTE_LEN, SM2_BYTE_LEN);
#endif

	//get [k]PB
	ret = sm2_pointMul(k, pubkey_x, pubkey_y, xy, xy+SM2_WORD_LEN);
	if(ret)
	{
		return ret;
	}

	//get C2
#ifdef PKE_BIG_ENDIAN
	convert_word_array((uint8_t *)xy, xy, SM2_WORD_LEN);
	convert_word_array((uint8_t *)(xy+SM2_WORD_LEN), xy+SM2_WORD_LEN, SM2_WORD_LEN);
#else
	convert_byte_array((uint8_t *)xy, (uint8_t *)xy, SM2_BYTE_LEN);
	convert_byte_array((uint8_t *)(xy+SM2_WORD_LEN), (uint8_t *)(xy+SM2_WORD_LEN), SM2_BYTE_LEN);
#endif

	C2 = C+1+2*SM2_BYTE_LEN + ((SM2_C1C2C3 == order)?0:SM2_BYTE_LEN);

	ret = sm2_kdf((uint8_t *)xy, SM2_BYTE_LEN<<1, C2, MByteLen);
	if(ret)
	{
		return ret;
	}

	if(uint8_BigNum_Check_Zero(C2, MByteLen))
	{
		return SM2_ZERO_ALL;
	}

	uint8_XOR(C2, M, C2, MByteLen);

	//get C3
	C3 = C+1+2*SM2_BYTE_LEN +((SM2_C1C2C3 == order)?MByteLen:0);

	ret = sm3_init(ctx);
	if(ret)
	{
		return ret;
	}

	ret = sm3_process(ctx, (uint8_t *)xy, SM2_BYTE_LEN);
	if(ret)
	{
		return ret;
	}

	ret = sm3_process(ctx, M, MByteLen);
	if(ret)
	{
		return ret;
	}

	ret = sm3_process(ctx, (uint8_t *)(xy+SM2_WORD_LEN), SM2_BYTE_LEN);
	if(ret)
	{
		return ret;
	}

	ret = sm3_done(ctx, C3);
	if(ret)
	{
		return ret;
	}

	*CByteLen = MByteLen+1+3*SM2_BYTE_LEN;

	return SM2_SUCCESS;
}


/* function: SM2 Encryption
 * parameters:
 *     M -------------------------- input, plaintext, MByteLen bytes, big-endian
 *     MByteLen ------------------- input, byte length of M
 *     pubKey[65] ----------------- input, public key, 65 bytes, big-endian
 *     order ---------------------- input, 0(the output C will be C1||C3||C2); 1(the output C will be C1||C2||C3)
 *     C -------------------------- output, ciphertext, CByteLen bytes, big-endian
 *     CByteLen ------------------- output, byte length of C, should be MByteLen+97 if success
 * return:
 *     0(success); other(error)
 * caution:
 *     1. M and C can not be the same buffer
 *     2. please make sure pubKey is valid
 */
uint8_t sm2_encrypt(uint8_t *M, uint32_t MByteLen, uint8_t pubKey[65],
					sm2_cipher_order_e order, uint8_t *C, uint32_t *CByteLen)
{
	uint32_t k[SM2_WORD_LEN];
	uint32_t pubkey_x[SM2_WORD_LEN],pubkey_y[SM2_WORD_LEN];
	uint8_t ret;
	if(NULL == pubKey)
	{
		return SM2_BUFFER_NULL;
	}

	if(POINT_NOT_COMPRESSED != pubKey[0])
	{
		return SM2_INPUT_INVALID;
	}

#ifdef PKE_BIG_ENDIAN
	convert_word_array(pubKey+1, pubkey_x, SM2_WORD_LEN);
	convert_word_array(pubKey+1+SM2_BYTE_LEN, pubkey_y, SM2_WORD_LEN);
#else
	convert_byte_array(pubKey+1, (uint8_t *)pubkey_x, SM2_BYTE_LEN);
	convert_byte_array(pubKey+1+SM2_BYTE_LEN, (uint8_t *)pubkey_y, SM2_BYTE_LEN);
#endif

SM2_ENCRYPT_LOOP:

        ret = get_rand((uint8_t *)k, SM2_BYTE_LEN);
	if(ret)
	{
		return ret;
	}
	ret = sm2_encrypt_with_k(M, MByteLen, k, pubkey_x, pubkey_y, order, C, CByteLen);
	if(SM2_ZERO_ALL == ret || SM2_INTEGER_TOO_BIG == ret)
	{
		goto SM2_ENCRYPT_LOOP;
	}
	else if(ret)
	{
		return ret;
	}

	return SM2_SUCCESS;
}


/* function: SM2 Decryption
 * parameters:
 *     C -------------------------- input, ciphertext, CByteLen bytes, big-endian
 *     CByteLen ------------------- input, byte length of C, make sure MByteLen>97
 *     priKey[32] ----------------- input, private key, 32 bytes, big-endian
 *     M -------------------------- output, plaintext, MByteLen bytes, big-endian
 *     MByteLen ------------------- output, byte length of M, should be CByteLen-97 if success
 * return:
 *     0(success); other(error)
 * caution:
 *     1. M and C can not be the same buffer
 */
uint8_t sm2_decrypt(uint8_t *C, uint32_t CByteLen, uint8_t priKey[32],
					sm2_cipher_order_e order, uint8_t *M, uint32_t *MByteLen)
{
	uint32_t temLen;
	uint32_t dA[SM2_WORD_LEN], xy[SM2_WORD_LEN<<1];
	uint8_t digest[SM2_BYTE_LEN];
	uint8_t *C2, *C3;
	hash_context_t ctx[1];
	uint8_t ret;

	if(NULL == C || NULL == priKey || NULL == M || NULL == MByteLen)
	{
		return SM2_BUFFER_NULL;
	}

	if(M == C)
	{
		return SM2_IN_OUT_SAME_BUFFER;
	}

	if(CByteLen <= 1+3*SM2_BYTE_LEN)                                        //97 = 1+3*ECCP_BYTELEN
	{
		return SM2_INPUT_INVALID;
	}

	if(order > SM2_C1C2C3)
	{
		return SM2_INPUT_INVALID;
	}

	//make sure C1 on the SM2 curve
#ifdef PKE_BIG_ENDIAN
	convert_word_array(C+1, xy, SM2_WORD_LEN);
	convert_word_array(C+1+SM2_BYTE_LEN, xy+SM2_WORD_LEN, SM2_WORD_LEN);
#else
	convert_byte_array(C+1, (uint8_t *)xy, SM2_BYTE_LEN);
	convert_byte_array(C+1+SM2_BYTE_LEN, (uint8_t *)(xy+SM2_WORD_LEN), SM2_BYTE_LEN);
#endif
	ret = sm2_pointVerify(xy, xy+SM2_WORD_LEN);
	if(ret)
	{
		return SM2_NOT_ON_CURVE;
	}

	//make sure priKey in [1, n-2]
#ifdef PKE_BIG_ENDIAN
	convert_word_array(priKey, dA, SM2_WORD_LEN);
#else
	convert_byte_array(priKey, (uint8_t *)dA, SM2_BYTE_LEN);
#endif
	if(uint32_BigNum_Check_Zero(dA, SM2_WORD_LEN))
	{
		return SM2_ZERO_ALL;
	}
	if(uint32_BigNumCmp(dA, SM2_WORD_LEN, (uint32_t *)sm2p256v1_n_1, SM2_WORD_LEN) >= 0)
	{
		return SM2_INTEGER_TOO_BIG;
	}

	//[dA]C1
	ret = sm2_pointMul(dA, xy, xy+SM2_WORD_LEN, xy, xy+SM2_WORD_LEN);
	if(ret)
	{
		return ret;
	}

#ifdef PKE_BIG_ENDIAN
	convert_word_array((uint8_t *)xy, xy, SM2_WORD_LEN);
	convert_word_array((uint8_t *)(xy+SM2_WORD_LEN), xy+SM2_WORD_LEN, SM2_WORD_LEN);
#else
	convert_byte_array((uint8_t *)xy, (uint8_t *)xy, SM2_BYTE_LEN);
	convert_byte_array((uint8_t *)(xy+SM2_WORD_LEN), (uint8_t *)(xy+SM2_WORD_LEN), SM2_BYTE_LEN);
#endif

	C2 = C+1+2*SM2_BYTE_LEN +((SM2_C1C2C3 == order)?0:SM2_BYTE_LEN);

	temLen = CByteLen-1-(3*SM2_BYTE_LEN);
	ret = sm2_kdf((uint8_t *)xy, SM2_BYTE_LEN<<1, M, temLen);
	if(ret)
	{
		return ret;
	}

	if(uint8_BigNum_Check_Zero(M, temLen))
	{
		return SM2_ZERO_ALL;
	}

	uint8_XOR(M, C2, M, temLen);

	C3 = C+1+2*SM2_BYTE_LEN +((SM2_C1C2C3 == order)?temLen:0);

	ret = sm3_init(ctx);
	if(ret)
	{
		return ret;
	}

	ret = sm3_process(ctx, (uint8_t *)xy, SM2_BYTE_LEN);
	if(ret)
	{
		return ret;
	}

	ret = sm3_process(ctx, M, temLen);
	if(ret)
	{
		return ret;
	}

	ret = sm3_process(ctx, (uint8_t *)(xy+SM2_WORD_LEN), SM2_BYTE_LEN);
	if(ret)
	{
		return ret;
	}

	ret = sm3_done(ctx, digest);
	if(ret)
	{
		return ret;
	}

	if(memcmp(C3, digest, SM2_BYTE_LEN))
	{
		return SM2_DECRY_VERIFY_FAILED;
	}

	*MByteLen = temLen;

	return SM2_SUCCESS;
}



/* function: SM2 Key Exchange
 * parameters:
 *     role        ------ input, 0 - sponsor, 1 - responsor
 *     dA[32]      ------ input, sponsor's permanent private key
 *     PB[65]      ------ input, responsor's permanent public key
 *     rA[32]      ------ input, sponsor's temporary private key
 *     RA[65]      ------ input, sponsor's temporary public key
 *     RB[65]      ------ input, responsor's temporary public key
 *     ZA[32]      ------ input, sponsor's Z value
 *     ZB[32]      ------ input, responsor's Z value
 *     kByteLen    ------ input, byte length of output key, should be less than (2^32 - 1)bit
 *     KA[kByteLen]------ output, output key
 *     S1[32]      ------ output, sponsor's S1, or responsor's S2
 *     SA[32]      ------ output, sponsor's SA, or responsor's SB
 * return:
 *     0(success); other(error)
 * caution: *
 *     1. please make sure the inputs are valid
 *     2. if S1=SB,S2=SA, then success.
 */
uint8_t sm2_exchangekey(sm2_exchange_role_e role,
						uint8_t *dA, uint8_t *PB,
						uint8_t *rA, uint8_t *RA,
						uint8_t *RB,
						uint8_t *ZA, uint8_t *ZB,
						uint32_t kByteLen,
						uint8_t *KA, uint8_t *S1, uint8_t *SA)
{
	uint32_t x1[SM2_WORD_LEN], t1[SM2_WORD_LEN], tmp[SM2_WORD_LEN<<2];
	hash_context_t ctx[1];
	uint8_t ret;

	if(NULL == dA || NULL == PB || NULL == rA || NULL == RA || NULL == RB)
	{
		return SM2_BUFFER_NULL;
	}

	if(NULL == ZA || NULL == ZB || NULL == KA || NULL == S1 || NULL == SA)
	{
		return SM2_BUFFER_NULL;
	}

	if(role > SM2_Role_Responsor)
	{
		return SM2_EXCHANGE_ROLE_INVALID;
	}

	if(0 == kByteLen)
	{
		return SM2_INPUT_INVALID;
	}

	if((POINT_NOT_COMPRESSED != PB[0]) || (POINT_NOT_COMPRESSED != RA[0]) || (POINT_NOT_COMPRESSED != RB[0]))
	{
		return SM2_INPUT_INVALID;
	}

	smx_uint32_clear(x1+SM2_WORD_LEN/2, SM2_WORD_LEN/2);
#ifdef PKE_BIG_ENDIAN
	convert_word_array(RA+1+(SM2_BYTE_LEN/2), x1, SM2_WORD_LEN/2);
#else
	convert_byte_array(RA+1+(SM2_BYTE_LEN/2), (uint8_t *)x1, SM2_BYTE_LEN/2);
#endif
	x1[(SM2_WORD_LEN/2)-1] |= 0x80000000;

	//make sure rA in [1, n-2]
#ifdef PKE_BIG_ENDIAN
	convert_word_array(rA, t1, SM2_WORD_LEN);
#else
	convert_byte_array(rA, (uint8_t *)t1, SM2_BYTE_LEN);
#endif
	if(uint32_BigNum_Check_Zero(t1, SM2_WORD_LEN))
	{
		return SM2_ZERO_ALL;
	}
	if(uint32_BigNumCmp(t1, SM2_WORD_LEN, (uint32_t *)sm2p256v1_n_1, SM2_WORD_LEN) >= 0)
	{
		return SM2_INTEGER_TOO_BIG;
	}

	//t1 = x1*rA mod n
	pke_load_pre_calc_mont_H((uint32_t *)sm2p256v1_n_h, SM2_WORD_LEN);
	ret = pke_modmul_internal((uint32_t *)sm2p256v1_n, x1, t1, t1, SM2_WORD_LEN);
	if(ret)
	{
		goto end;
	}

	//make sure dA in [1, n-2]
#ifdef PKE_BIG_ENDIAN
	convert_word_array(dA, x1, SM2_WORD_LEN);
#else
	convert_byte_array(dA, (uint8_t *)x1, SM2_BYTE_LEN);
#endif
	if(uint32_BigNum_Check_Zero(x1, SM2_WORD_LEN))
	{
		return SM2_ZERO_ALL;
	}
	if(uint32_BigNumCmp(x1, SM2_WORD_LEN, (uint32_t *)sm2p256v1_n_1, SM2_WORD_LEN) >= 0)
	{
		return SM2_INTEGER_TOO_BIG;
	}

    //t1 = (dA + x1*rA) mod n
	ret = sm2_modadd((uint32_t *)sm2p256v1_n, t1, x1, t1, SM2_WORD_LEN);
	if(ret)
	{
		goto end;
	}

	//make sure RB on the SM2 curve
#ifdef PKE_BIG_ENDIAN
	convert_word_array(RB+1, tmp, SM2_WORD_LEN);
	convert_word_array(RB+1+SM2_BYTE_LEN, tmp+SM2_WORD_LEN, SM2_WORD_LEN);
#else
	convert_byte_array(RB+1, (uint8_t *)tmp, SM2_BYTE_LEN);
	convert_byte_array(RB+1+SM2_BYTE_LEN, (uint8_t *)(tmp+SM2_WORD_LEN), SM2_BYTE_LEN);
#endif
	ret = sm2_pointVerify(tmp, tmp+SM2_WORD_LEN);
	if(ret)
	{
		return SM2_NOT_ON_CURVE;
	}

	smx_uint32_clear(x1+SM2_WORD_LEN/2, SM2_WORD_LEN/2);
#ifdef PKE_BIG_ENDIAN
	convert_word_array(RB+1+(SM2_BYTE_LEN/2), x1, SM2_WORD_LEN/2);
#else
	convert_byte_array(RB+1+(SM2_BYTE_LEN/2), (uint8_t *)x1, SM2_BYTE_LEN/2);
#endif
	x1[(SM2_WORD_LEN/2)-1] |= 0x80000000;

#ifdef SM2_HIGH_SPEED
	//x1 = tA*x2 mod n
	pke_load_pre_calc_mont_H((uint32_t *)sm2p256v1_n_h, SM2_WORD_LEN);
	ret = pke_modmul_internal((uint32_t *)sm2p256v1_n, t1, x1, x1, SM2_WORD_LEN);
	if(ret)
	{
		goto end;
	}

#ifdef PKE_BIG_ENDIAN
	convert_word_array(PB+1, tmp+2*SM2_WORD_LEN, SM2_WORD_LEN);
	convert_word_array(PB+1+SM2_BYTE_LEN, tmp+3*SM2_WORD_LEN, SM2_WORD_LEN);
#else
	convert_byte_array(PB+1, (uint8_t *)(tmp+2*SM2_WORD_LEN), SM2_BYTE_LEN);
	convert_byte_array(PB+1+SM2_BYTE_LEN, (uint8_t *)(tmp+3*SM2_WORD_LEN), SM2_BYTE_LEN);
#endif

	//[tA]PB +[tA*x2 mod n]RB
	ret = sm2_pointMul_Shamir(t1, tmp+2*SM2_WORD_LEN, tmp+3*SM2_WORD_LEN,
							  x1, tmp, tmp+SM2_WORD_LEN,
							  tmp, tmp+SM2_WORD_LEN);
#else
	ret = sm2_pointMul(x1, tmp, tmp+SM2_WORD_LEN, tmp, tmp+SM2_WORD_LEN);
	if(ret)
	{
		goto end;
	}

#ifdef PKE_BIG_ENDIAN
	convert_word_array(PB+1, tmp+2*SM2_WORD_LEN, SM2_WORD_LEN);
	convert_word_array(PB+1+SM2_BYTE_LEN, tmp+3*SM2_WORD_LEN, SM2_WORD_LEN);
#else
	convert_byte_array(PB+1, (uint8_t *)(tmp+2*SM2_WORD_LEN), SM2_BYTE_LEN);
	convert_byte_array(PB+1+SM2_BYTE_LEN, (uint8_t *)(tmp+3*SM2_WORD_LEN), SM2_BYTE_LEN);
#endif
	ret = sm2_pointAdd(tmp, tmp+SM2_WORD_LEN, tmp+2*SM2_WORD_LEN, tmp+3*SM2_WORD_LEN,
					   tmp, tmp+SM2_WORD_LEN);
	if(ret)
	{
		goto end;
	}

	ret = sm2_pointMul(t1, tmp, tmp+SM2_WORD_LEN, tmp, tmp+SM2_WORD_LEN);
#endif
	if(ret)
	{
		goto end;
	}

	//xU||yU
#ifdef PKE_BIG_ENDIAN
	convert_word_array((uint8_t *)tmp, tmp, SM2_WORD_LEN);
	convert_word_array((uint8_t *)(tmp+SM2_WORD_LEN), tmp+SM2_WORD_LEN, SM2_WORD_LEN);
#else
	convert_byte_array((uint8_t *)tmp, (uint8_t *)tmp, SM2_BYTE_LEN);
	convert_byte_array((uint8_t *)(tmp+SM2_WORD_LEN), (uint8_t *)(tmp+SM2_WORD_LEN), SM2_BYTE_LEN);
#endif

	if(SM2_Role_Sponsor == role)
	{
		memcpy(tmp+2*SM2_WORD_LEN, ZA, SM2_BYTE_LEN);
		memcpy(tmp+3*SM2_WORD_LEN, ZB, SM2_BYTE_LEN);
	}
	else
	{
		memcpy(tmp+2*SM2_WORD_LEN, ZB, SM2_BYTE_LEN);
		memcpy(tmp+3*SM2_WORD_LEN, ZA, SM2_BYTE_LEN);
	}

	//KA
	ret = sm2_kdf((uint8_t *)tmp, SM2_BYTE_LEN<<2, KA, kByteLen);
	if(ret)
	{
		goto end;
	}

	ret = sm3_init(ctx);
	if(ret)
	{
		goto end;
	}

	ret = sm3_process(ctx, (uint8_t *)tmp, SM2_BYTE_LEN);
	if(ret)
	{
		goto end;
	}

	if(SM2_Role_Sponsor == role)
	{
		ret = sm3_process(ctx, ZA, SM2_BYTE_LEN);
		if(ret)
		{
			goto end;
		}

		ret = sm3_process(ctx, ZB, SM2_BYTE_LEN);
		if(ret)
		{
			goto end;
		}

		ret = sm3_process(ctx, RA+1, SM2_BYTE_LEN<<1);
		if(ret)
		{
			goto end;
		}

		ret = sm3_process(ctx, RB+1, SM2_BYTE_LEN<<1);
		if(ret)
		{
			goto end;
		}
	}
	else
	{
		ret = sm3_process(ctx, ZB, SM2_BYTE_LEN);
		if(ret)
		{
			goto end;
		}

		ret = sm3_process(ctx, ZA, SM2_BYTE_LEN);
		if(ret)
		{
			goto end;
		}

		ret = sm3_process(ctx, RB+1, SM2_BYTE_LEN<<1);
		if(ret)
		{
			goto end;
		}

		ret = sm3_process(ctx, RA+1, SM2_BYTE_LEN<<1);
		if(ret)
		{
			goto end;
		}
	}
	ret = sm3_done(ctx, (uint8_t *)t1);
	if(ret)
	{
		goto end;
	}

	ret = sm3_init(ctx);
	if(ret)
	{
		goto end;
	}

	*(((uint8_t *)(tmp+SM2_WORD_LEN))-1) = 0x03;
	ret = sm3_process(ctx, ((uint8_t *)(tmp+SM2_WORD_LEN))-1, SM2_BYTE_LEN+1);
	if(ret)
	{
		goto end;
	}

	ret = sm3_process(ctx, (uint8_t *)t1, SM2_BYTE_LEN);
	if(ret)
	{
		goto end;
	}

	if(SM2_Role_Sponsor == role)
	{
		ret = sm3_done(ctx, SA);
		if(ret)
		{
			goto end;
		}

	}
	else
	{
		ret = sm3_done(ctx, S1);
		if(ret)
		{
			goto end;
		}
	}

	ret = sm3_init(ctx);
	if(ret)
	{
		goto end;
	}

	*(((uint8_t *)(tmp+SM2_WORD_LEN))-1) = 0x02;
	ret = sm3_process(ctx, ((uint8_t *)(tmp+SM2_WORD_LEN))-1, SM2_BYTE_LEN+1);
	if(ret)
	{
		goto end;
	}

	ret = sm3_process(ctx, (uint8_t *)t1, SM2_BYTE_LEN);
	if(ret)
	{
		goto end;
	}

	if(SM2_Role_Sponsor == role)
	{
		ret = sm3_done(ctx, S1);
		if(ret)
		{
			goto end;
		}
	}
	else
	{
		ret = sm3_done(ctx, SA);
		if(ret)
		{
			goto end;
		}
	}

	ret = SM2_SUCCESS;

end:

	return ret;
}
