
#ifndef __SXE_TRACE_H__
#define __SXE_TRACE_H__

#ifdef SXE_DRIVER_TRACE

#define	SXE_TRACE_NUM_PER_RING	(2048)
#define	SXE_TRACE_PER_RING_MASK	(0x7FF)

#ifndef SXE_TEST
#define SXE_TRACE_DUMP_FILE_NAME ("/var/log/sxe_trace_dump.log")
#else
#define SXE_TRACE_DUMP_FILE_NAME (".sxe_trace_dump.log")
#endif

enum sxe_trace_lab_tx {
	SXE_TRACE_LAB_TX_START = 0,
	SXE_TRACE_LAB_TX_MAY_STOP,
	SXE_TRACE_LAB_TX_VLAN,
	SXE_TRACE_LAB_TX_DCB,
	SXE_TRACE_LAB_TX_IPSEC,
	SXE_TRACE_LAB_TX_TSO,
	SXE_TRACE_LAB_TX_DESC,
	SXE_TRACE_LAB_TX_PPT,
	SXE_TRACE_LAB_TX_FDIR,
	SXE_TRACE_LAB_TX_OL_INFO,
	SXE_TRACE_LAB_TX_MAP,
	SXE_TRACE_LAB_TX_SENT,
	SXE_TRACE_LAB_TX_UPDATE,
	SXE_TRACE_LAB_TX_MAY_STOP_2,
	SXE_TRACE_LAB_TX_WRITE,
	SXE_TRACE_LAB_TX_END,
	SXE_TRACE_LAB_TX_MAX,
};

struct sxe_trace_tx_ring {
	u64 next;		
	u64 timestamp[SXE_TRACE_NUM_PER_RING][SXE_TRACE_LAB_TX_MAX];	
};

enum sxe_trace_lab_rx {
	SXE_TRACE_LAB_RX_START = 0,
	SXE_TRACE_LAB_RX_CLEAN,
	SXE_TRACE_LAB_RX_UNMAP,
	SXE_TRACE_LAB_RX_STATS,
	SXE_TRACE_LAB_RX_HANG,
	SXE_TRACE_LAB_RX_DONE,
	SXE_TRACE_LAB_RX_WAKE,
	SXE_TRACE_LAB_RX_END,
	SXE_TRACE_LAB_RX_MAX,
};

struct sxe_trace_rx_ring {
	u64 next;		
	u64 timestamp[SXE_TRACE_NUM_PER_RING][SXE_TRACE_LAB_RX_MAX];	
};

void sxe_trace_tx_add(u8 ring_idx, enum sxe_trace_lab_tx lab);

void sxe_trace_rx_add(u8 ring_idx, enum sxe_trace_lab_rx lab);

void sxe_trace_dump(void);

#define SXE_TRACE_TX(r_idx, lab)	\
		sxe_trace_tx_add(r_idx, lab)

#define SXE_TRACE_RX(r_idx, lab)	\
		sxe_trace_rx_add(r_idx, lab)

#else
#define SXE_TRACE_TX(r_idx, lab)

#define SXE_TRACE_RX(r_idx, lab)

#endif	
#endif

