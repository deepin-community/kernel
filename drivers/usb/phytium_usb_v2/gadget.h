/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __LINUX_PHYTIUM_USB_GADGET_H__
#define __LINUX_PHYTIUM_USB_GADGET_H__

#include <linux/usb/gadget.h>

#define SCT_SEC_TR	0
#define SCT_PRI_TR	1
#define SCT_FOR_TRB(p)		(((p) << 1) & 0x7)
#define SCT_FOR_CTX(p) (((p) << 1) & GENMASK(3, 1))


#define TRB_EVENT_INVALIDATE 8

#define COMP_CODE_MASK		(0xff << 24)
#define GET_COMP_CODE(p)	(((p) & COMP_CODE_MASK) >> 24)
#define COMP_INVALID				0
#define COMP_SUCCESS				1
#define COMP_DATA_BUFFER_ERROR			2
#define COMP_BABBLE_DETECTED_ERROR		3
#define COMP_TRB_ERROR				5
#define COMP_RESOURCE_ERROR			7
#define COMP_NO_SLOTS_AVAILABLE_ERROR		9
#define COMP_INVALID_STREAM_TYPE_ERROR		10
#define COMP_SLOT_NOT_ENABLED_ERROR		11
#define COMP_ENDPOINT_NOT_ENABLED_ERROR		12
#define COMP_SHORT_PACKET			13
#define COMP_RING_UNDERRUN			14
#define COMP_RING_OVERRUN			15
#define COMP_VF_EVENT_RING_FULL_ERROR		16
#define COMP_PARAMETER_ERROR			17
#define COMP_CONTEXT_STATE_ERROR		19
#define COMP_EVENT_RING_FULL_ERROR		21
#define COMP_INCOMPATIBLE_DEVICE_ERROR		22
#define COMP_MISSED_SERVICE_ERROR		23
#define COMP_COMMAND_RING_STOPPED		24
#define COMP_COMMAND_ABORTED			25
#define COMP_STOPPED				26
#define COMP_STOPPED_LENGTH_INVALID		27
#define COMP_STOPPED_SHORT_PACKET		28
#define COMP_MAX_EXIT_LATENCY_TOO_LARGE_ERROR	29
#define COMP_ISOCH_BUFFER_OVERRUN		31
#define COMP_EVENT_LOST_ERROR			32
#define COMP_UNDEFINED_ERROR			33
#define COMP_INVALID_STREAM_ID_ERROR		34


#define GADGET_DEV_MAX_SLOTS 1
#define GADGET_CTX_SIZE 2112

#define GADGET_ENDPOINTS_NUM 31

#define RTSOFF_MASK GENMASK(31, 5)

#define HC_LENGTH(p)	(((p) >> 00) & GENMASK(7, 0))
#define HC_VERSION(p)	(((p) >> 16) & GENMASK(15, 1))

#define HCC_64BIT_ADDR(p)	((p) & BIT(0))
#define HCC_MAX_PSA(p)		((((p) >> 12) & 0xf) + 1)

#define GADGET_STATE_HALTED	BIT(1)
#define GADGET_STATE_DYING	BIT(2)
#define GADGET_STATE_DISCONNECT_PENDING	BIT(3)
#define GADGET_WAKEUP_PENDING	BIT(4)

#define LINK_TOGGLE BIT(1)

#define XDEV_U0		(0x0 << 5)
#define XDEV_U1		(0x1 << 5)
#define XDEV_U2		(0x2 << 5)
#define XDEV_U3		(0x3 << 5)
#define XDEV_DISABLED	(0x4 << 5)
#define XDEV_RXDETECT	(0x5 << 5)
#define XDEV_INACTIVE	(0x6 << 5)
#define XDEV_POLLING	(0x7 << 5)
#define XDEV_RECOVERY	(0x8 << 5)
#define XDEV_HOT_RESET	(0x9 << 5)
#define XDEV_COMP_MODE	(0xa << 5)
#define XDEV_TEST_MODE	(0xb << 5)
#define XDEV_RESUME	(0xf << 5)

#define PORT_CONNECT	BIT(0)
#define	PORT_PED	BIT(1)
#define	PORT_RESET	BIT(4)
#define PORT_LINK_STROBE	BIT(16)
#define PORT_CSC	BIT(17)
#define PORT_WRC	BIT(19)
#define PORT_RC		BIT(21)
#define PORT_PLC	BIT(22)
#define PORT_CEC	BIT(23)
#define PORT_WKCONN_E	BIT(25)
#define PORT_WKDISC_E	BIT(26)
#define PORT_WR		BIT(31)
#define PORT_CHANGE_BITS (PORT_CSC | PORT_WRC | PORT_RC | PORT_PLC | PORT_CEC)
#define PORT_PLS_MASK	GENMASK(8, 5)
#define DEV_SPEED_MASK	GENMASK(13, 10)
#define XDEV_FS		(0x1 << 10)
#define XDEV_HS		(0x3 << 10)
#define XDEV_SS		(0x4 << 10)
#define XDEV_SSP	(0x5 << 10)
#define DEV_UNDEFSPEED(p)	(((p) & DEV_SPEED_MASK) == (0x0 << 10))
#define DEV_FULLSPEED(p)	(((p) & DEV_SPEED_MASK) == XDEV_FS)
#define DEV_HIGHSPEED(p)	(((p) & DEV_SPEED_MASK) == XDEV_HS)
#define DEV_SUPERSPEED(p)	(((p) & DEV_SPEED_MASK) == XDEV_SS)
#define DEV_SUPERSPEEDPLUS(p)	(((p) & DEV_SPEED_MASK) == XDEV_SSP)
#define DEV_SUPERSPEED_ANY(p)	(((p) & DEV_SPEED_MASK) >= XDEV_SS)
#define DEV_PORT_SPEED(p)	(((p) >> 10) & 0xf)
#define DEV_PORT(p)		(((p) & 0xff) << 16)
#define DEV_ADDR_MASK		GENMASK(7, 0)

#define GADGET_PORT_RO (PORT_CONNECT | DEV_SPEED_MASK)
#define GADGET_PORT_RWS (PORT_PLS_MASK | PORT_WKCONN_E | PORT_WKDISC_E)
#define PORT_RWE	BIT(3)
#define PORT_BESL(p)	(((p) << 4) & GENMASK(7, 4))
#define PORT_HLE	BIT(16)
#define RRBESL(p)	(((p) & GENMASK(20, 17)) >> 17)
#define PORT_L1S_MASK	GENMASK(2, 0)
#define PORT_L1S(p)		((p) & PORT_L1S_MASK)
#define PORT_L1S_ACK		PORT_L1S(1)
#define PORT_L1S_NYET		PORT_L1S(2)
#define PORT_L1S_STALL		PORT_L1S(3)
#define PORT_L1S_TIMEOUT	PORT_L1S(4)
#define PORT_U1_TIMEOUT_MASK	GENMASK(7, 0)
#define PORT_U1_TIMEOUT(p)	((p) & PORT_U1_TIMEOUT_MASK)
#define PORT_U2_TIMEOUT_MASK	GENMASK(14, 8)
#define PORT_U2_TIMEOUT(p)	((p) & PORT_U2_TIMEOUT_MASK)

#define PORT_TEST_MODE_MASK	GENMASK(31, 28)
#define PORT_TEST_MODE(p)	(((p) << 28) & PORT_TEST_MODE_MASK)

#define IMAN_IP		BIT(0)
#define IMAN_IE		BIT(1)
#define IMAN_IE_SET(p)		((p) | IMAN_IE)
#define IMAN_IE_CLEAR(p)	((p) & ~IMAN_IE)

#define CMD_R_S		BIT(0)
#define	CMD_RESET	BIT(1)
#define CMD_INTE	BIT(2)
#define CMD_DSEIE	BIT(3)
#define CMD_EWE		BIT(10)
#define CMD_DEVEN	BIT(17)
#define GADGET_IRQS	(CMD_INTE | CMD_DSEIE | CMD_EWE)


#define STS_HALT	BIT(0)
#define STS_FATAL	BIT(2)
#define STS_EINT	BIT(3)
#define STS_PCD		BIT(4)
#define STS_SSS		BIT(8)
#define STS_RSS		BIT(9)
#define STS_SRE		BIT(10)
#define STS_CNR		BIT(11)
#define STS_HCE		BIT(12)

#define CMD_RING_RSVD_BITS GENMASK(5, 0)
#define DBOFF_MASK GENMASK(31, 2)


#define TRB_TO_STREAM_ID(p)	((((p) & GENMASK(31, 16)) >> 16))
#define STREAM_ID_FOR_TRB(p)	((((p)) << 16) & GENMASK(31, 16))
#define TRB_TO_SLOT_ID(p)	(((p) & GENMASK(31, 24)) >> 24)
#define SLOT_ID_FOR_TRB(p)	(((p) << 24) & GENMASK(31, 24))
#define TRB_TO_EP_INDEX(p)	(((p) >> 16) & 0x1f)
#define EP_ID_FOR_TRB(p)	((((p) + 1) << 16) & GENMASK(20, 16))

#define TRB_TYPE_BITMASK GENMASK(15, 10)
#define TRB_TYPE(p) ((p) << 10)
#define TRB_FIELD_TO_TYPE(p)	(((p) & TRB_TYPE_BITMASK) >> 10)

#define TRB_NORMAL		1
#define TRB_SETUP		2
#define TRB_DATA		3
#define TRB_STATUS		4
#define TRB_ISOC		5
#define TRB_LINK		6
#define TRB_EVENT_DATA		7
#define TRB_TR_NOOP		8
#define TRB_ENABLE_SLOT		9
#define TRB_DISABLE_SLOT	10
#define TRB_ADDR_DEV		11
#define TRB_CONFIG_EP		12
#define TRB_EVAL_CONTEXT	13
#define TRB_RESET_EP		14
#define TRB_STOP_RING		15
#define TRB_SET_DEQ		16
#define TRB_RESET_DEV		17
#define TRB_FORCE_EVENT		18
#define TRB_FORCE_HEADER	22
#define TRB_CMD_NOOP		23
#define TRB_TRANSFER		32
#define TRB_COMPLETION		33
#define TRB_PORT_STATUS		34
#define TRB_HC_EVENT		37
#define TRB_MFINDEX_WRAP	39
#define TRB_ENDPOINT_NRDY	48
#define TRB_HALT_ENDPOINT	54
#define TRB_HALT_OVERFLOW	57
#define TRB_FLUSH_ENDPOINT	58

#define TRB_TD_SIZE(p)		(min((p), (u32)31) << 17)
#define GET_TD_SIZE(p)		(((p) & GENMASK(21, 17)) >> 17)
#define TRB_TD_SIZE_TBC(p)	(min((p), (u32)31) << 17)
#define TRB_INTR_TARGET(p)	(((p) << 22) & GENMASK(31, 22))
#define GET_INTR_TARGET(p)	(((p) & GENMASK(31, 22)) >> 22)
#define TRB_TBC(p)		(((p) & 0x3) << 7)
#define TRB_TLBPC(p)		(((p) & 0xf) << 16)

#define TRB_LEN(p)	((p) & GENMASK(16, 0))
#define TRB_CYCLE	BIT(0)
#define TRB_ENT		BIT(1)
#define TRB_ISP		BIT(2)
#define TRB_NO_SNOOP	BIT(3)
#define TRB_CHAIN	BIT(4)
#define TRB_IOC		BIT(5)
#define TRB_IDT		BIT(6)
#define TRB_STAT	BIT(7)
#define TRB_BEI		BIT(9)
#define TRB_DIR_IN	BIT(16)

#define TRB_CONFIG_EP 12

#define TRBS_PER_SEGMENT 256
#define TRBS_PER_EV_DEQ_UPDATE 100
#define TRBS_PER_EVENT_SEGMENT 256
#define TRB_MAX_BUFF_SHIFT	16
#define TRB_MAX_BUFF_SIZE	BIT(TRB_MAX_BUFF_SHIFT)
#define TRB_BUFF_LEN_UP_TO_BOUNDARY(addr)	(TRB_MAX_BUFF_SIZE - \
			((addr) & (TRB_MAX_BUFF_SIZE - 1)))

#define TRB_TYPE_LINK(x)	(((x) & TRB_TYPE_BITMASK) == TRB_TYPE(TRB_LINK))
#define TRB_TYPE_LINK_LE32(x)	(((x) & cpu_to_le32(TRB_TYPE_BITMASK)) == \
					cpu_to_le32(TRB_TYPE(TRB_LINK)))
#define TRB_TYPE_NOOP_LE32(x)	(((x) & cpu_to_le32(TRB_TYPE_BITMASK)) == \
					cpu_to_le32(TRB_TYPE(TRB_TR_NOOP)))

#define TRB_TO_EP_ID(p)	(((p) & GENMASK(20, 16)) >> 16)

#define ERST_EHB BIT(3)
#define ERST_PTR_MASK GENMASK(3, 0)

#define LAST_CTX_MASK	((unsigned int)GENMASK(31, 27))
#define LAST_CTX(p)	((p) << 27)
#define SLOT_FLAG	BIT(0)
#define EP0_FLAG	BIT(1)

#define EP_TYPE(p)	((p) << 3)

#define ISOC_OUT_EP     1
#define BULK_OUT_EP     2
#define INT_OUT_EP      3
#define CTRL_EP         4
#define ISOC_IN_EP      5
#define BULK_IN_EP      6
#define INT_IN_EP	7

#define EP_ENABLED		BIT(0)
#define EP_DIS_IN_PROGRESS	BIT(1)
#define EP_HALTED		BIT(2)
#define EP_STOPPED		BIT(3)
#define EP_WEDGE		BIT(4)
#define EP_HALTED_STATUS	BIT(5)
#define EP_HAS_STREAMS		BIT(6)
#define EP_UNCONFIGURED		BIT(7)

#define EP_STATE_MASK	GENMASK(3, 0)
#define EP_STATE_DISABLED	0
#define EP_STATE_RUNNING	1
#define EP_STATE_HALTED		2
#define EP_STATE_STOPPED	3
#define EP_STATE_ERROR		4
#define GET_EP_CTX_STATE(ctx)	(le32_to_cpu((ctx)->ep_info) & EP_STATE_MASK)


#define GADGET_CMD_TIMEOUT (15 * 1000)

#define CMD_RING_BUSY(p)	((p) & BIT(4))
#define CMD_RING_RUNNING	BIT(3)
#define CMD_RING_RSVD_BITS	GENMASK(5, 0)

#define SLOT_STATE		GENMASK(31, 27)
#define GET_SLOT_STATE(p)	(((p) & SLOT_STATE) >> 27)
#define SLOT_STATE_DISABLED		0
#define SLOT_STATE_ENABLED		0 //czh test
#define SLOT_STATE_DEFAULT		1
#define SLOT_STATE_ADDRESSED	2
#define SLOT_STATE_CONFIGURED	3

#define EP0_HALTED_STATUS	BIT(5)

enum gadget_setup_dev {
	SETUP_CONTEXT_ONLY,
	SETUP_CONTEXT_ADDRESS,
};

enum gadget_ep0_stage {
	GADGET_SETUP_STAGE,
	GADGET_DATA_STAGE,
	GADGET_STATUS_STAGE,
};

struct gadget_slot_ctx {
	__le32 dev_info;
	__le32 dev_port;
	__le32 int_target;
	__le32 dev_state;
	__le32 reserved[4];
};

struct gadget_input_control_ctx {
	__le32 drop_flags;
	__le32 add_flags;
	__le32 rsvd2[6];
};

struct gadget_link_trb {
	__le64 segment_ptr;
	__le32 intr_target;
	__le32 control;
};

struct gadget_event_cmd {
	__le64 cmd_trb;
	__le32 status;
	__le32 flags;
};

struct gadget_transfer_event {
	__le64 buffer;
	__le32 transfer_len;
	__le32 flags;
};

struct gadget_generic_trb {
	__le32 field[4];
};

enum gadget_ring_type {
	TYPE_CTRL = 0,
	TYPE_ISOC,
	TYPE_BULK,
	TYPE_INTR,
	TYPE_STREAM,
	TYPE_COMMAND,
	TYPE_EVENT,
};

struct phytium_device_intr_reg {
	__le32 irq_pending;
	__le32 irq_control;
	__le32 erst_size;
	__le32 rsvd;
	__le64 erst_base;
	__le64 erst_dequeue;
};

struct phytium_device_cap_regs {
	__le32	hc_capbase;
	__le32	hcs_params1;
	__le32	hcs_params2;
	__le32	hcs_params3;
	__le32	hcc_params;
	__le32	db_off;
	__le32	run_regs_off;
	__le32	hcc_params2;
};

struct phytium_device_op_regs {
	__le32 command;
	__le32 status;
	__le32 page_size;
	__le32 reserved1;
	__le32 reserved2;
	__le32 dnctrl;
	__le64 cmd_ring;
	__le32 reserved3[4];
	__le64 dcbaa_ptr;
	__le32 config_reg;
	__le32 reserved4[241];
	__le32 port_reg_base;
};

struct phytium_device_run_regs {
	__le32 microframe_index;
	__le32 rsvd[7];
	struct phytium_device_intr_reg ir_set[128];
};

struct phytium_device_rev_cap {
	__le32 ext_cap;
	__le32 rtl_revision;
	__le32 rx_buff_size;
	__le32 tx_buff_size;
	__le32 ep_supported;
	__le32 ctrl_revision;
};

struct gadget_context_array {
	__le64 dev_context_ptrs[GADGET_DEV_MAX_SLOTS + 1];
	dma_addr_t dma;
};

union gadget_trb {
	struct gadget_link_trb link;
	struct gadget_transfer_event trans_event;
	struct gadget_event_cmd event_cmd;
	struct gadget_generic_trb generic;
};

struct gadget_segment {
	union gadget_trb *trbs;
	struct gadget_segment *next;
	dma_addr_t dma;
	dma_addr_t bounce_dma;
	void *bounce_buf;
	unsigned int bounce_offs;
	unsigned int bounce_len;
};

struct gadget_ring {
	struct gadget_segment *first_seg;
	struct gadget_segment *last_seg;
	union gadget_trb *enqueue;
	struct gadget_segment *enq_seg;
	union gadget_trb *dequeue;
	struct gadget_segment *deq_seg;
	struct list_head td_list;
	u32 cycle_state;
	unsigned int steam_id;
	unsigned int stream_active;
	unsigned int stream_rejected;
	int num_tds;
	unsigned int num_segs;
	unsigned int num_trbs_free;
	unsigned int bounce_buf_len;
	enum gadget_ring_type type;
	bool last_td_was_short;
	struct radix_tree_root *trb_address_map;
};

struct phytium_device_doorbell_array {
	__le32 cmd_db;
	__le32 ep_db;
};

struct gadget_erst_entry {
	__le64 seg_addr;
	__le32 seg_size;
	__le32 rsvd;
};

struct gadget_erst {
	struct gadget_erst_entry *entries;
	unsigned int num_entries;
	dma_addr_t erst_dma_addr;
};

struct phytium_device_20port_cap {
	__le32 ext_cap;
	__le32 port_regs1;
	__le32 port_regs2;
	__le32 port_regs3;
	__le32 port_regs4;
	__le32 port_regs5;
	__le32 port_regs6;
};

struct phytium_device_3xport_cap {
	__le32 ext_cap;
	__le32 mode_addr;
	__le32 reserved[52];
	__le32 mode_2;
};

struct gadget_port_regs {
	__le32 portsc;
	__le32 portpmsc;
	__le32 portli;
	__le32 reserved;
};

struct gadget_port {
	struct gadget_port_regs __iomem *regs;
	u8 port_num;
	u8 exist;
	u8 maj_rev;
	u8 min_rev;
};

struct gadget_container_ctx {
	unsigned int type;
	int size;
	int ctx_size;
	dma_addr_t dma;
	u8 *bytes;
};

struct gadget_ep_ctx {
	__le32 ep_info;
	__le32 ep_info2;
	__le64 deq;
	__le32 tx_info;
	__le32 reserved[3];
};

struct gadget_stream_ctx {
	__le64 stream_ring;
	__le32 reserved[2];
};

struct gadget_stream_info {
	struct gadget_ring **stream_rings;
	unsigned int num_streams;
	struct gadget_stream_ctx *stream_ctx_array;
	unsigned int num_stream_ctxs;
	dma_addr_t ctx_array_dma;
	struct radix_tree_root trb_address_map;
	int td_count;
	u8 first_prime_det;
	u8 drbls_count;
};

struct gadget_ep {
	struct usb_ep endpoint;
	struct list_head pending_list;
	struct phytium_device *pdev;
	u8 number;
	u8 idx;
	u32 interval;
	char name[20];
	u8 direction;
	u8 buffering;
	u8 buffering_period;
	struct gadget_ep_ctx *in_ctx;
	struct gadget_ep_ctx *out_ctx;
	struct gadget_ring *ring;
	struct gadget_stream_info stream_info;
	unsigned int ep_state;
	bool skip;
};

struct gadget_command {
	struct gadget_container_ctx *in_ctx;
	u32 status;
	union gadget_trb *command_trb;
};

struct gadget_td {
	struct list_head td_list;
	struct gadget_request *preq;
	struct gadget_segment *start_seg;
	union gadget_trb *first_trb;
	union gadget_trb *last_trb;
	struct gadget_segment *bounce_seg;
	bool request_length_set;
	bool drbl;
};

struct gadget_dequeue_state {
	struct gadget_segment *new_deq_seg;
	union gadget_trb *new_deq_ptr;
	int new_cycle_state;
	unsigned int stream_id;
};

struct gadget_request {
	struct gadget_td td;
	struct usb_request request;
	struct list_head list;
	struct gadget_ep *pep;
	u8 epnum;
	unsigned direction:1;
};

struct phytium_device {
	struct device *dev;
	struct usb_gadget gadget;
	struct usb_gadget_driver *gadget_driver;
	void __iomem *regs;
	void *setup_buf;
	u8 setup_id;
	u8 setup_speed;
	struct usb_ctrlrequest setup;

	struct phytium_device_cap_regs __iomem *cap_regs;
	struct phytium_device_op_regs __iomem *op_regs;
	struct phytium_device_run_regs __iomem *run_regs;
	struct phytium_device_rev_cap __iomem *rev_cap;
	struct phytium_device_doorbell_array __iomem *dba;
	struct phytium_device_intr_reg __iomem *ir_set;
	struct phytium_device_20port_cap __iomem *port20_regs;
	struct phytium_device_3xport_cap __iomem *port3x_regs;

	__u32 hcs_params1;
	__u32 hcs_params3;
	__u32 hcc_params;

	u8 ep0_expect_in;
	u8 device_address;

	int may_wakeup;
	u16 hci_version;
	unsigned int gadget_state;
	unsigned int link_state;

	spinlock_t lock;

	struct gadget_context_array *dcbaa;
	struct gadget_ring *cmd_ring;
	struct gadget_command cmd;
	struct gadget_ring *event_ring;
	struct gadget_erst erst;
	int slot_id;

	struct gadget_container_ctx out_ctx;
	struct gadget_container_ctx in_ctx;
	struct gadget_ep eps[GADGET_ENDPOINTS_NUM];

	struct dma_pool *device_pool;
	struct dma_pool *segment_pool;

	struct gadget_port usb2_port;
	struct gadget_port usb3_port;
	struct gadget_port *active_port;
	u16 test_mode;

	struct gadget_request ep0_preq;
	enum gadget_ep0_stage ep0_stage;
	u8 three_stage_setup;

	u8 u1_allowed:1;
	u8 u2_allowed:1;
	u8 usb2_hw_lpm_capable:1;
};

static inline struct gadget_request *next_request(struct list_head *list)
{
	return list_first_entry_or_null(list, struct gadget_request, list);
}


int phytium_usb_gadget_init(void *data);

int phytium_gadget_find_next_ext_cap(void __iomem *base, u32 start, int id);

int gadget_reset(struct phytium_device *pdev);

int gadget_endpoint_init(struct phytium_device *pdev,
		struct gadget_ep *pep, gfp_t mem_flags);

int gadget_alloc_streams(struct phytium_device *pdev, struct gadget_ep *pep);

struct gadget_input_control_ctx
	*gadget_get_input_control_ctx(struct gadget_container_ctx *ctx);

void gadget_free_endpoint_rings(struct phytium_device *pdev,
		struct gadget_ep *pep);

struct gadget_slot_ctx *gadget_get_slot_ctx(struct gadget_container_ctx *ctx);

//gadget
u32 gadget_port_state_to_neutral(u32 state);
int gadget_wait_for_cmd_compl(struct phytium_device *pdev);
void disconnect_gadget(struct phytium_device *pdev);
bool gadget_last_trb_on_seg(struct gadget_segment *seg, union gadget_trb *trb);
bool gadget_last_trb_on_ring(struct gadget_ring *ring, struct gadget_segment *seg,
		union gadget_trb *trb);
int gadget_halt(struct phytium_device *pdev);
int gadget_disable_slot(struct phytium_device *pdev);
int gadget_enable_slot(struct phytium_device *pdev);
void gadget_set_usb2_hardware_lpm(struct phytium_device *pdev,
		struct usb_request *req, int enable);
unsigned int gadget_port_speed(unsigned int port_status);
void resume_gadget(struct phytium_device *pdev);
void suspend_gadget(struct phytium_device *pdev);
void gadget_irq_reset(struct phytium_device *pdev);
int gadget_ep_enqueue(struct gadget_ep *pep, struct gadget_request *preq);
void gadget_died(struct phytium_device *pdev);
int gadget_halt_endpoint(struct phytium_device *pdev, struct gadget_ep *pep, int value);
int gadget_reset_device(struct phytium_device *pdev);
int gadget_setup_device(struct phytium_device *pdev, enum gadget_setup_dev setup);
int ep_dequeue(struct gadget_ep *pep, struct gadget_request *preq);
void gadget_update_erst_dequeue(struct phytium_device *pdev, union gadget_trb *event_ring_deq,
		u8 clear_ehb);

//mem
int gadget_ring_expansion(struct phytium_device *pdev, struct gadget_ring *ring,
		unsigned int num_trbs, gfp_t flags);

struct gadget_ep_ctx *gadget_get_ep_ctx(struct gadget_container_ctx *ctx,
		unsigned int ep_index);

void gadget_endpoint_zero(struct phytium_device *pdev, struct gadget_ep *pep);
struct gadget_ring *gadget_dma_to_transfer_ring(struct gadget_ep *pep, u64 address);



//ring
void gadget_ring_cmd_db(struct phytium_device *pdev);
int gadget_cmd_stop_ep(struct phytium_device *pdev, struct gadget_ep *pep);
int gadget_cmd_flush_ep(struct phytium_device *pdev, struct gadget_ep *pep);
int gadget_remove_request(struct phytium_device *pdev, struct gadget_request *preq,
			struct gadget_ep *pep);
void gadget_queue_slot_control(struct phytium_device *pdev, u32 trb_type);
void gadget_inc_deq(struct phytium_device *pdev, struct gadget_ring *ring);
void gadget_queue_reset_ep(struct phytium_device *pdev, unsigned int ep_index);
void gadget_queue_halt_endpoint(struct phytium_device *pdev, unsigned int ep_index);
void gadget_queue_reset_device(struct phytium_device *pdev);
void gadget_set_link_state(struct phytium_device *pdev,
		__le32 __iomem *port_regs, u32 link_state);

#endif
