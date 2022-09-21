#ifndef _TRNG_H_
#define _TRNG_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <asm/io.h>
#include "smx_common.h"





#define DEFAULT_FIFO_DEPTH  (0x7F)


#define TRNG_RO_FREQ_4      (0)
#define TRNG_RO_FREQ_8      (1)
#define TRNG_RO_FREQ_16     (2)
#define TRNG_RO_FREQ_32     (3)     //default



//TRNG return code
enum TRNG_RET_CODE
{
	TRNG_SUCCESS = 0,
	TRNG_BUFFER_NULL,
	TRNG_HT_FAILURE,
	TRNG_READ_DATA_NULL,
};




//API

uint8_t get_seed(uint8_t *a, uint32_t byteLen);

uint8_t get_seed_with_post_processing(uint8_t *a, uint32_t byteLen);

uint8_t get_rand(uint8_t *a, uint32_t byteLen);

void trng_interrupt_init(void);

#ifdef __cplusplus
}
#endif

#endif
