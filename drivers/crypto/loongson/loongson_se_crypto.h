/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bitfield.h>
#include <linux/crypto.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/ftrace.h>
#include <linux/iopoll.h>
#include <soc/loongson/se.h>

#include <crypto/engine.h>
#include <crypto/internal/hash.h>
#include <crypto/internal/skcipher.h>
#include <crypto/internal/rng.h>
#include <crypto/sm3.h>
#include <crypto/sm4.h>
#include <crypto/rng.h>

#define SE_ALG_TYPE_RNG		1
#define SE_ALG_TYPE_SM2		2
#define SE_ALG_TYPE_SM3		3
#define SE_ALG_TYPE_SM4		4
#define SE_ALG_TYPE_ZUC		6

/* SM3 definitions */
#define SE_SM3_DATA_SIZE		0x100000
#define SE_SM3_DATA_SIZE_1		0x400000

#define SE_SM3_BUFLEN			0x100

#define SE_HASH_FLAGS_INIT		BIT(0)
#define SE_HASH_FLAGS_FINUP		BIT(1)

#define SE_HASH_OP_UPDATE		1
#define SE_HASH_OP_FINAL		2

/* SM4 definitions */
#define SE_SM4_DATA_SIZE		0x100000
#define SE_SM4_DATA_ALIGN		0x40ULL
#define SE_SM4_DATA_ALIGN_MASK	(SE_SM4_DATA_ALIGN - 1)

/* RNG definitions */
#define SE_RNG_DATA_SIZE		PAGE_SIZE
#define SE_RNG_DATA_ALIGN		0x4ULL

/* ZUC definitions */
#define SE_ZUC_DATA_SIZE		0x100000
#define SE_ZUC_DATA_ALIGN		0x40ULL
#define SE_ZUC_DATA_ALIGN_MASK	(SE_ZUC_DATA_ALIGN - 1)
#define SE_ZUC_KEY_SIZE			16
#define SE_ZUC_IV_SIZE			16
#define SE_ZUC_BLOCK_SIZE		1

#define MODULE_NAME   "Loongson SE"

struct loongson_alg_common {
	u32 type;
	u32 data_size;
	bool need_engine;
	union {
		struct ahash_engine_alg ahash;
		struct skcipher_engine_alg skcipher;
		struct akcipher_engine_alg akcipher;
		struct rng_alg rng;
	} u;
};

struct se_alg_engine {
	struct list_head engine_list;
	struct list_head finish_list;
	u32 type;
	struct crypto_engine *engine;
	struct lsse_ch *se_ch;

	/* TODO There is a probability of problems in multithreaded environments */
	struct list_head wait_list;
	/* Working request */
	void *rctx;
	int cmd_ret;
	u32 cmd;

	/* Buffer information */
	u32 buffer_size;
	u32 buffer_cnt;
	void *in_buffer;
	dma_addr_t in_addr;
	void *out_buffer;
	dma_addr_t out_addr;
	void *key_buffer;
	dma_addr_t key_addr;
	void *info_buffer;
	dma_addr_t info_addr;
};

struct loongson_hash_ctx {
	struct se_alg_engine *sae;
	int keylen;
	u32 key[SM3_BLOCK_SIZE / sizeof(u32)];
};

struct loongson_hash_request_ctx {
	unsigned long op;
	unsigned long flags;
	struct ahash_request *req;

	struct scatterlist *sg;
	int nents;
	u32 rest_bytes;
	u32 copyed_bytes;
};

struct loongson_skcipher_ctx {
	struct se_alg_engine *sae;
	int keylen;
	u32 key[SM4_KEY_SIZE / sizeof(u32)];
};

struct loongson_skcipher_request_ctx {
	struct list_head rctx_list;
	unsigned long op;
	struct skcipher_request *req;

	struct scatterlist *src;
	struct scatterlist *dst;
	int in_nents;
	int out_nents;
	u32 rest_bytes;
	u32 copyed_bytes;

	int ivlen;
	int keylen;
	u32 update_iv[SM4_BLOCK_SIZE / sizeof(u32)];
	u32 iv[SM4_BLOCK_SIZE / sizeof(u32)];
	u32 key[SM4_KEY_SIZE / sizeof(u32)];

	void *aligned_buffer;
	dma_addr_t aligned_addr;
	int aligned_len;
};

struct loongson_rng_ctx {
	struct se_alg_engine *sae;
};

struct lsse_crypto {
	spinlock_t cmd_lock;
	struct list_head alg_engine;
	struct list_head finish_engine;

	/* Synchronous CMD */
	struct completion rng_completion;

	/* Memory copy task */
	struct tasklet_struct task;
};

struct se_alg_req {
	u32 len;
	u32 in_off;
	u32 out_off;
	u32 key_off;
	u32 info_off;
	u32 info[2];
};

struct se_alg_res {
	u32 cmd_ret;
	u32 info[6];
};

struct se_alg_msg {
	u32 cmd;
	union {
		struct se_alg_req req;
		struct se_alg_res res;
	} u;
};

struct se_sg_dma {
	u64 src;
	u64 dst;
	u64 len;
	u64 pad;
};

extern struct device *se_dev;
extern struct loongson_alg_common se_alg_1_1[5];
extern struct loongson_alg_common se_alg_1_2[7];
int loongson_sm3_enqueue(struct ahash_request *req, unsigned int op);
int loongson_sm3_final(struct ahash_request *req);
int loongson_sm3_init(struct ahash_request *req);
//int loongson_sm3_digest(struct ahash_request *req);
int loongson_sm3_setkey(struct crypto_ahash *tfm,
						const u8 *key, unsigned int keylen);
//int loongson_se_ahash_init(struct crypto_tfm *tfm);
void loongson_se_ahash_exit(struct crypto_tfm *tfm);

int loongson_sm4_enqueue(struct skcipher_request *req, unsigned int op);
int loongson_sm4_setkey(struct crypto_skcipher *cipher,
						const u8 *key, unsigned int len);
int loongson_ecb_sm4_encrypt(struct skcipher_request *req);
int loongson_ecb_sm4_decrypt(struct skcipher_request *req);
int loongson_cbc_sm4_encrypt(struct skcipher_request *req);
int loongson_cbc_sm4_decrypt(struct skcipher_request *req);
int loongson_ctr_sm4(struct skcipher_request *req);
//int loongson_se_sm4_init(struct crypto_tfm *tfm);
void loongson_se_sm4_exit(struct crypto_tfm *tfm);
int se_send_sm4_cmd(struct lsse_crypto *se,
		struct se_alg_engine *sae, u32 op, int retry);

int loongson_rng_seed(struct crypto_rng *tfm, const u8 *seed, unsigned int slen);
int loongson_rng_generate(struct crypto_rng *tfm, const u8 *src,
						  unsigned int slen, u8 *dstn, unsigned int dlen);
int loongson_se_rng_init(struct crypto_tfm *tfm);
struct se_alg_engine *se_find_engine(struct lsse_crypto *se, u32 type);
void loongson_se_finish_req(struct lsse_crypto *se);
void loongson_se_task_routine_1_1(unsigned long data);
void loongson_se_task_routine_1_2(unsigned long data);

int loongson_zuc_enqueue(struct skcipher_request *req, unsigned int op);
int loongson_zuc_cipher_once(struct se_alg_engine *sae,
		struct skcipher_request *req, u32 op);
int loongson_zuc_one_request(struct crypto_engine *engine, void *areq);
int loongson_zuc_setkey(struct crypto_skcipher *cipher,
		const u8 *key, unsigned int len);
int loongson_ecb_zuc(struct skcipher_request *req);
int loongson_se_zuc_init(struct crypto_tfm *tfm);
void loongson_se_zuc_exit(struct crypto_tfm *tfm);

int loongson_sm3_hmac_init(struct ahash_request *req);
int loongson_sm3_hmac_final(struct ahash_request *req);
