#ifndef _SMX_H_
#define _SMX_H_

#ifdef __cplusplus
extern "C" {
#endif


//#include <stdint.h>    //including int32_t definition
//typedef __int8_t int8_t ;
//typedef __uint8_t uint8_t ;
//typedef __int16_t int16_t ;
//typedef __uint16_t uint16_t ;
//typedef __int32_t int32_t ;
//typedef __uint32_t uint32_t ;


#define SMX_DMA_BASE				(0x29805000UL)   //just for temporary use

#define SMX_BASE_ADDR				(0x28220000UL)                //SMX register base address
#define DMA_BASE_ADDR				(SMX_BASE_ADDR+0x100)         //SMX DMA register base address
#define SKE_BASE_ADDR				(SMX_BASE_ADDR+0x1000)        //SM4 register base address
#define HASH_BASE_ADDR				(SMX_BASE_ADDR+0x2000)        //SM3 register base address
#define TRNG_BASE_ADDR				(SMX_BASE_ADDR+0x3000)        //TRNG register base address
#define PKE_BASE_ADDR				(SMX_BASE_ADDR+0x5000)        //PKE register base address



#define SMX_CR                 0x0//(*(volatile unsigned int *)(SMX_BASE_ADDR))
#define SMX_CMD                0x4//(*(volatile unsigned int *)(SMX_BASE_ADDR+0x04))
#define SMX_CFG                0x8//(*(volatile unsigned int *)(SMX_BASE_ADDR+0x08))
#define SMX_SR1                0x10//(*(volatile unsigned int *)(SMX_BASE_ADDR+0x10))
#define SMX_SR2                0x14//(*(volatile unsigned int *)(SMX_BASE_ADDR+0x14))
#define SMX_CMD_SR             0x20//(*(volatile unsigned int *)(SMX_BASE_ADDR+0x20))
#define SMX_VERSION            0x30//(*(volatile unsigned int *)(SMX_BASE_ADDR+0x30))


#define SMX_DMA_CFG            0x0//(*(volatile unsigned int *)(DMA_BASE_ADDR))
#define SMX_DMA_SR             0x4//(*(volatile unsigned int *)(DMA_BASE_ADDR+0x04))
#define SMX_DMA_TO_THRES       0x8//(*(volatile unsigned int *)(DMA_BASE_ADDR+0x08))
#define SMX_DMA_SADDR0         0x10//(*(volatile unsigned int *)(DMA_BASE_ADDR+0x10))
#define SMX_DMA_SADDR1         0x14//(*(volatile unsigned int *)(DMA_BASE_ADDR+0x14))
#define SMX_DMA_DADDR0         0x20//(*(volatile unsigned int *)(DMA_BASE_ADDR+0x20))
#define SMX_DMA_DADDR1         0x24//(*(volatile unsigned int *)(DMA_BASE_ADDR+0x24))
#define SMX_DMA_LEN            0x30//(*(volatile unsigned int *)(DMA_BASE_ADDR+0x30))


#define SMX_PRINT_BUF
#ifdef SMX_PRINT_BUF
extern void print_buf_U8(uint8_t buf[], uint32_t byteLen, char name[]);
extern void print_buf_U32(uint32_t buf[], uint32_t wordLen, char name[]);
#endif

extern void smx_uint32_copy(uint32_t *dst, uint32_t *src, uint32_t wordLen);

extern void smx_uint32_clear(uint32_t *a, uint32_t wordLen);

extern void smx_reverse_word(uint8_t *in, uint8_t *out, uint32_t bytelen);

extern void smx_dma_reverse_word(uint32_t *in, uint32_t *out, uint32_t wordlen);

extern int32_t uint32_BigNumCmp(uint32_t *a, uint32_t aWordLen, uint32_t *b, uint32_t bWordLen);




#ifdef __cplusplus
}
#endif

#endif
