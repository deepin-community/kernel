#ifndef __GSGPU_XDMA_H__
#define __GSGPU_XDMA_H__

#define GSGPU_XDMA_FLAG_UMAP 0x20000

extern const struct gsgpu_ip_block_version xdma_ip_block;

void xdma_ring_test_xdma_loop(struct gsgpu_ring *ring, long timeout);

#endif /*__GSGPU_XDMA_H__*/
