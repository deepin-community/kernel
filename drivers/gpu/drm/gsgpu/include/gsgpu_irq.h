#ifndef __GSGPU_IRQ_H__
#define __GSGPU_IRQ_H__

#include <linux/irqdomain.h>
#include "gsgpu_ih.h"
#include "gsgpu_dc_i2c.h"

#define GSGPU_MAX_IRQ_SRC_ID	0x100
#define GSGPU_MAX_IRQ_CLIENT_ID	0x100

/* TODO irq srcid need rewrite*/
#define GSGPU_SRCID_GFX_PAGE_INV_FAULT                 0x00000092  /* 146 */
#define GSGPU_SRCID_GFX_MEM_PROT_FAULT                 0x00000093  /* 147 */
#define GSGPU_SRCID_CP_END_OF_PIPE                     0x000000b5  /* 181 */
#define GSGPU_SRCID_CP_PRIV_REG_FAULT                  0x000000b8  /* 184 */
#define GSGPU_SRCID_CP_PRIV_INSTR_FAULT                0x000000b9  /* 185 */
#define GSGPU_SRCID_XDMA_TRAP          	               0x000000e0  /* 224 */
#define GSGPU_SRCID_XDMA_SRBM_WRITE                    0x000000f7  /* 247 */

struct gsgpu_device;
struct gsgpu_iv_entry;

enum gsgpu_interrupt_state {
	GSGPU_IRQ_STATE_DISABLE,
	GSGPU_IRQ_STATE_ENABLE,
};

struct gsgpu_irq_src {
	unsigned				num_types;
	atomic_t				*enabled_types;
	const struct gsgpu_irq_src_funcs	*funcs;
	void *data;
};

struct gsgpu_irq_client {
	struct gsgpu_irq_src **sources;
};

/* provided by interrupt generating IP blocks */
struct gsgpu_irq_src_funcs {
	int (*set)(struct gsgpu_device *adev, struct gsgpu_irq_src *source,
		   unsigned type, enum gsgpu_interrupt_state state);

	int (*process)(struct gsgpu_device *adev,
		       struct gsgpu_irq_src *source,
		       struct gsgpu_iv_entry *entry);
};

struct gsgpu_irq {
	bool				installed;
	spinlock_t			lock;
	/* interrupt sources */
	struct gsgpu_irq_client	client[GSGPU_IH_CLIENTID_MAX];

	/* status, etc. */
	bool				msi_enabled; /* msi enabled */

	/* interrupt ring */
	struct gsgpu_ih_ring		ih;
	const struct gsgpu_ih_funcs	*ih_funcs;
};

void gsgpu_irq_disable_all(struct gsgpu_device *adev);

int gsgpu_irq_init(struct gsgpu_device *adev);
void gsgpu_irq_fini(struct gsgpu_device *adev);
int gsgpu_irq_add_id(struct gsgpu_device *adev,
		      unsigned client_id, unsigned src_id,
		      struct gsgpu_irq_src *source);
void gsgpu_irq_dispatch(struct gsgpu_device *adev,
			 struct gsgpu_iv_entry *entry);
int gsgpu_irq_update(struct gsgpu_device *adev, struct gsgpu_irq_src *src,
		      unsigned type);
int gsgpu_irq_get(struct gsgpu_device *adev, struct gsgpu_irq_src *src,
		   unsigned type);
int gsgpu_irq_put(struct gsgpu_device *adev, struct gsgpu_irq_src *src,
		   unsigned type);
bool gsgpu_irq_enabled(struct gsgpu_device *adev, struct gsgpu_irq_src *src,
			unsigned type);
void gsgpu_irq_gpu_reset_resume_helper(struct gsgpu_device *adev);

#endif /* __GSGPU_IRQ_H__ */
