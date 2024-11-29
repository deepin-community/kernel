/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HYGON Platform Security Processor (PSP) driver interface
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 */

#ifndef __PSP_HYGON_H__
#define __PSP_HYGON_H__

#include <linux/types.h>
#include <linux/fs.h>
#include <linux/kvm_types.h>

/*****************************************************************************/
/***************************** CSV interface *********************************/
/*****************************************************************************/

#define CSV_FW_MAX_SIZE		0x80000	/* 512KB */

#define CSV_EXT_CSV3_MULT_LUP_DATA_BIT	0
#define CSV_EXT_CSV3_MULT_LUP_DATA	(1 << CSV_EXT_CSV3_MULT_LUP_DATA_BIT)
#define CSV_EXT_CSV3_INJ_SECRET_BIT	1
#define CSV_EXT_CSV3_INJ_SECRET		(1 << CSV_EXT_CSV3_INJ_SECRET_BIT)

/**
 * Guest/platform management commands for CSV
 */
enum csv_cmd {
	CSV_CMD_RING_BUFFER		= 0x00F,
	CSV_CMD_HGSC_CERT_IMPORT        = 0x300,
	CSV_CMD_MAX,
};

/**
 * Guest/platform management commands for CSV3
 */
enum csv3_cmd {
	/* Guest launch commands */
	CSV3_CMD_SET_GUEST_PRIVATE_MEMORY	= 0x200,
	CSV3_CMD_LAUNCH_ENCRYPT_DATA		= 0x201,
	CSV3_CMD_LAUNCH_ENCRYPT_VMCB		= 0x202,
	/* Guest NPT(Nested Page Table) management commands */
	CSV3_CMD_UPDATE_NPT			= 0x203,

	/* Guest migration commands */
	CSV3_CMD_SEND_ENCRYPT_DATA		= 0x210,
	CSV3_CMD_SEND_ENCRYPT_CONTEXT		= 0x211,
	CSV3_CMD_RECEIVE_ENCRYPT_DATA		= 0x212,
	CSV3_CMD_RECEIVE_ENCRYPT_CONTEXT	= 0x213,

	/* Guest debug commands */
	CSV3_CMD_DBG_READ_VMSA			= 0x220,
	CSV3_CMD_DBG_READ_MEM			= 0x221,

	/* Platform secure memory management commands */
	CSV3_CMD_SET_SMR			= 0x230,
	CSV3_CMD_SET_SMCR			= 0x231,

	CSV3_CMD_MAX,
};

/**
 * CSV communication state
 */
enum csv_comm_state {
	CSV_COMM_MAILBOX_ON		= 0x0,
	CSV_COMM_RINGBUFFER_ON		= 0x1,

	CSV_COMM_MAX
};

/**
 * Ring Buffer Mode regions:
 *   There are 4 regions and every region is a 4K area that must be 4K aligned.
 *   To accomplish this allocate an amount that is the size of area and the
 *   required alignment.
 *   The aligned address will be calculated from the returned address.
 */
#define CSV_RING_BUFFER_SIZE		(32 * 1024)
#define CSV_RING_BUFFER_ALIGN		(4 * 1024)
#define CSV_RING_BUFFER_LEN		(CSV_RING_BUFFER_SIZE + CSV_RING_BUFFER_ALIGN)
#define CSV_RING_BUFFER_ESIZE		16

/**
 * struct csv_data_hgsc_cert_import - HGSC_CERT_IMPORT command parameters
 *
 * @hgscsk_cert_address: HGSCSK certificate chain
 * @hgscsk_cert_len: len of HGSCSK certificate
 * @hgsc_cert_address: HGSC certificate chain
 * @hgsc_cert_len: len of HGSC certificate
 */
struct csv_data_hgsc_cert_import {
	u64 hgscsk_cert_address;        /* In */
	u32 hgscsk_cert_len;            /* In */
	u32 reserved;                   /* In */
	u64 hgsc_cert_address;          /* In */
	u32 hgsc_cert_len;              /* In */
} __packed;

#define CSV_COMMAND_PRIORITY_HIGH	0
#define CSV_COMMAND_PRIORITY_LOW	1
#define CSV_COMMAND_PRIORITY_NUM	2

struct csv_cmdptr_entry {
	u16 cmd_id;
	u16 cmd_flags;
	u32 sw_data;
	u64 cmd_buf_ptr;
} __packed;

struct csv_statval_entry {
	u16 status;
	u16 reserved0;
	u32 reserved1;
	u64 reserved2;
} __packed;

struct csv_queue {
	u32 head;
	u32 tail;
	u32 mask; /* mask = (size - 1), inicates the elements max count */
	u32 esize; /* size of an element */
	u64 data;
	u64 data_align;
} __packed;

struct csv_ringbuffer_queue {
	struct csv_queue cmd_ptr;
	struct csv_queue stat_val;
} __packed;

/**
 * struct csv_data_ring_buffer - RING_BUFFER command parameters
 *
 * @queue_lo_cmdptr_address: physical address of the region to be used for
 *                           low priority queue's CmdPtr ring buffer
 * @queue_lo_statval_address: physical address of the region to be used for
 *                            low priority queue's StatVal ring buffer
 * @queue_hi_cmdptr_address: physical address of the region to be used for
 *                           high priority queue's CmdPtr ring buffer
 * @queue_hi_statval_address: physical address of the region to be used for
 *                            high priority queue's StatVal ring buffer
 * @queue_lo_size: size of the low priority queue in 4K pages. Must be 1
 * @queue_hi_size: size of the high priority queue in 4K pages. Must be 1
 * @queue_lo_threshold: queue(low) size, below which an interrupt may be generated
 * @queue_hi_threshold: queue(high) size, below which an interrupt may be generated
 * @int_on_empty: unconditionally interrupt when both queues are found empty
 */
struct csv_data_ring_buffer {
	u64 queue_lo_cmdptr_address;	/* In */
	u64 queue_lo_statval_address;	/* In */
	u64 queue_hi_cmdptr_address;	/* In */
	u64 queue_hi_statval_address;	/* In */
	u8 queue_lo_size;		/* In */
	u8 queue_hi_size;		/* In */
	u16 queue_lo_threshold;		/* In */
	u16 queue_hi_threshold;		/* In */
	u16 int_on_empty;		/* In */
} __packed;

/**
 * struct csv3_data_launch_encrypt_data - CSV3_CMD_LAUNCH_ENCRYPT_DATA command
 *
 * @handle: handle of the VM to update
 * @gpa: guest address where data is copied
 * @length: len of memory to be encrypted
 * @data_blocks: memory regions to hold data page address
 */
struct csv3_data_launch_encrypt_data {
	u32 handle;			/* In */
	u32 reserved;			/* In */
	u64 gpa;			/* In */
	u32 length;			/* In */
	u32 reserved1;			/* In */
	u64 data_blocks[8];		/* In */
} __packed;

/**
 * struct csv3_data_launch_encrypt_vmcb - CSV3_CMD_LAUNCH_ENCRYPT_VMCB command
 *
 * @handle: handle of the VM
 * @vcpu_id: id of vcpu per vmsa/vmcb
 * @vmsa_addr: memory address of initial vmsa data
 * @vmsa_len: len of initial vmsa data
 * @shadow_vmcb_addr: memory address of shadow vmcb data
 * @shadow_vmcb_len: len of shadow vmcb data
 * @secure_vmcb_addr: memory address of secure vmcb data
 * @secure_vmcb_len: len of secure vmcb data
 */
struct csv3_data_launch_encrypt_vmcb {
	u32 handle;			/* In */
	u32 reserved;			/* In */
	u32 vcpu_id;			/* In */
	u32 reserved1;			/* In */
	u64 vmsa_addr;			/* In */
	u32 vmsa_len;			/* In */
	u32 reserved2;			/* In */
	u64 shadow_vmcb_addr;		/* In */
	u32 shadow_vmcb_len;		/* In */
	u32 reserved3;			/* In */
	u64 secure_vmcb_addr;		/* Out */
	u32 secure_vmcb_len;		/* Out */
} __packed;

/**
 * struct csv3_data_update_npt - CSV3_CMD_UPDATE_NPT command
 *
 * @handle: handle assigned to the VM
 * @error_code: nested page fault error code
 * @gpa: guest page address where npf happens
 * @spa: physical address which maps to gpa in host page table
 * @level: page level which can be mapped in nested page table
 * @page_attr: page attribute for gpa
 * @page_attr_mask: which page attribute bit should be set
 * @npages: number of pages from gpa is handled.
 */
struct csv3_data_update_npt {
	u32 handle;			/* In */
	u32 reserved;			/* In */
	u32 error_code;			/* In */
	u32 reserved1;			/* In */
	u64 gpa;			/* In */
	u64 spa;			/* In */
	u64 level;			/* In */
	u64 page_attr;			/* In */
	u64 page_attr_mask;		/* In */
	u32 npages;			/* In/Out */
} __packed;

/**
 * struct csv3_data_mem_region - define a memory region
 *
 * @base_address: base address of a memory region
 * @size: size of memory region
 */
struct csv3_data_memory_region {
	u64 base_address;		/* In */
	u64 size;			/* In */
} __packed;

/**
 * struct csv3_data_set_guest_private_memory - CSV3_CMD_SET_GUEST_PRIVATE_MEMORY
 * command parameters
 *
 * @handle: handle assigned to the VM
 * @nregions: number of memory regions
 * @regions_paddr: address of memory containing multiple memory regions
 */
struct csv3_data_set_guest_private_memory {
	u32 handle;			/* In */
	u32 nregions;			/* In */
	u64 regions_paddr;		/* In */
} __packed;

/**
 * struct csv3_data_set_smr - CSV3_CMD_SET_SMR command parameters
 *
 * @smr_entry_size: size of SMR entry
 * @nregions: number of memory regions
 * @regions_paddr: address of memory containing multiple memory regions
 */
struct csv3_data_set_smr {
	u32 smr_entry_size;		/* In */
	u32 nregions;			/* In */
	u64 regions_paddr;		/* In */
} __packed;

/**
 * struct csv3_data_set_smcr - CSV3_CMD_SET_SMCR command parameters
 *
 * @base_address: start address of SMCR memory
 * @size: size of SMCR memory
 */
struct csv3_data_set_smcr {
	u64 base_address;		/* In */
	u64 size;			/* In */
} __packed;

/**
 * struct csv3_data_dbg_read_vmsa - CSV3_CMD_DBG_READ_VMSA command parameters
 *
 * @handle: handle assigned to the VM
 * @spa: system physical address of memory to get vmsa of the specific vcpu
 * @size: size of the host memory
 * @vcpu_id: the specific vcpu
 */
struct csv3_data_dbg_read_vmsa {
	u32 handle;			/* In */
	u32 reserved;			/* In */
	u64 spa;			/* In */
	u32 size;			/* In */
	u32 vcpu_id;			/* In */
} __packed;

/**
 * struct csv3_data_dbg_read_mem - CSV3_CMD_DBG_READ_MEM command parameters
 *
 * @handle: handle assigned to the VM
 * @gpa: guest physical address of the memory to access
 * @spa: system physical address of memory to get data from gpa
 * @size: size of guest memory to access
 */
struct csv3_data_dbg_read_mem {
	u32 handle;			/* In */
	u32 reserved;			/* In */
	u64 gpa;			/* In */
	u64 spa;			/* In */
	u32 size;			/* In */
} __packed;

/**
 * struct csv3_data_attestation_report - ATTESTATION secure call command parameters
 *
 * @handle: handle of the VM to process
 * @resp_gpa: guest physical address to save the generated report
 * @resp_length: length of the generated report
 * @req_gpa: guest physical address of the input for the report
 * @req_length: length of the input for the report
 * @fw_error_code: firmware status code when generating the report
 */
struct csv3_data_attestation_report {
	u32 handle;				/* Out */
	u32 reserved1;
	u64 resp_gpa;				/* In */
	u8 reserved2[16];
	u32 resp_len;				/* In/Out */
	u32 reserved3;
	u64 req_gpa;				/* In */
	u32 req_len;				/* In,Out */
	u32 fw_error_code;			/* Out */
} __packed;

/**
 * struct csv3_data_send_encrypt_data - SEND_ENCRYPT_DATA command parameters
 *
 * @handle: handle of the VM to process
 * @hdr_address: physical address containing packet header
 * @hdr_len: len of packet header
 * @guest_block: physical address containing multiple guest address
 * @guest_len: len of guest block
 * @flag: flag of send encrypt data
 *        0x00000000: migrate pages in guest block
 *        0x00000001: set readonly of pages in guest block
 *            others: invalid
 * @trans_block: physical address of a page containing multiple host memory pages
 * @trans_len: len of host memory region
 */
struct csv3_data_send_encrypt_data {
	u32 handle;			/* In */
	u32 reserved;			/* In */
	u64 hdr_address;		/* In */
	u32 hdr_len;			/* In/Out */
	u32 reserved1;			/* In */
	u64 guest_block;		/* In */
	u32 guest_len;			/* In */
	u32 flag;			/* In */
	u64 trans_block;		/* In */
	u32 trans_len;			/* In/Out */
} __packed;

/**
 * struct csv3_data_send_encrypt_context - SEND_ENCRYPT_CONTEXT command parameters
 *
 * @handle: handle of the VM to process
 * @hdr_address: physical address containing packet header
 * @hdr_len: len of packet header
 * @trans_block: physical address of a page containing multiple host memory pages
 * @trans_len: len of host memory region
 */
struct csv3_data_send_encrypt_context {
	u32 handle;			/* In */
	u32 reserved;			/* In */
	u64 hdr_address;		/* In */
	u32 hdr_len;			/* In/Out */
	u32 reserved1;			/* In */
	u64 trans_block;		/* In */
	u32 trans_len;			/* In/Out */
} __packed;

/**
 * struct csv3_data_receive_encrypt_data - RECEIVE_ENCRYPT_DATA command parameters
 *
 * @handle: handle of the VM to process
 * @hdr_address: physical address containing packet header blob
 * @hdr_len: len of packet header
 * @guest_block: system physical address containing multiple guest address
 * @guest_len: len of guest block memory region
 * @trans_block: physical address of a page containing multiple host memory pages
 * @trans_len: len of host memory region
 */
struct csv3_data_receive_encrypt_data {
	u32 handle;			/* In */
	u32 reserved;			/* In */
	u64 hdr_address;		/* In */
	u32 hdr_len;			/* In */
	u32 reserved1;			/* In */
	u64 guest_block;		/* In */
	u32 guest_len;			/* In */
	u32 reserved2;			/* In */
	u64 trans_block;		/* In */
	u32 trans_len;			/* In */
} __packed;

/**
 * struct csv3_data_receive_encrypt_context - RECEIVE_ENCRYPT_CONTEXT command parameters
 *
 * @handle: handle of the VM to process
 * @hdr_address: physical address containing packet header
 * @hdr_len: len of packet header
 * @trans_block: physical address of a page containing multiple host memory pages
 * @trans_len: len of host memory region
 * @shadow_vmcb_block: physical address of a page containing multiple shadow vmcb address
 * @secure_vmcb_block: physical address of a page containing multiple secure vmcb address
 * @vmcb_block_len: len of shadow/secure vmcb block
 */
struct csv3_data_receive_encrypt_context {
	u32 handle;			/* In */
	u32 reserved;			/* In */
	u64 hdr_address;		/* In */
	u32 hdr_len;			/* In */
	u32 reserved1;			/* In */
	u64 trans_block;		/* In */
	u32 trans_len;			/* In */
	u32 reserved2;			/* In */
	u64 shadow_vmcb_block;		/* In */
	u64 secure_vmcb_block;		/* In */
	u32 vmcb_block_len;		/* In */
} __packed;

/*
 * enum VPSP_CMD_STATUS - virtual psp command status
 *
 * @VPSP_INIT: the initial command from guest
 * @VPSP_RUNNING: the middle command to check and run ringbuffer command
 * @VPSP_FINISH: inform the guest that the command ran successfully
 */
enum VPSP_CMD_STATUS {
	VPSP_INIT = 0,
	VPSP_RUNNING,
	VPSP_FINISH,
	VPSP_MAX
};

/**
 * struct vpsp_cmd - virtual psp command
 *
 * @cmd_id: the command id is used to distinguish different commands
 * @is_high_rb: indicates the ringbuffer level in which the command is placed
 */
struct vpsp_cmd {
	u32 cmd_id	:	31;
	u32 is_high_rb	:	1;
};

/**
 * struct vpsp_ret - virtual psp return result
 *
 * @pret: the return code from device
 * @resv: reserved bits
 * @format: indicates that the error is a unix error code(is 0) or a psp error(is 1)
 * @index: used to distinguish the position of command in the ringbuffer
 * @status: indicates the current status of the related command
 */
struct vpsp_ret {
	u32 pret	:	16;
	u32 resv	:	1;
	u32 format	:	1;
	u32 index	:	12;
	u32 status	:	2;
};
#define VPSP_RET_SYS_FORMAT    1
#define VPSP_RET_PSP_FORMAT    0

struct kvm_vpsp {
	struct kvm *kvm;
	int (*write_guest)(struct kvm *kvm, gpa_t gpa, const void *data, unsigned long len);
	int (*read_guest)(struct kvm *kvm, gpa_t gpa, void *data, unsigned long len);
	kvm_pfn_t (*gfn_to_pfn)(struct kvm *kvm, gfn_t gfn);
	u32 vm_handle;
	u8 is_csv_guest;
};

#define PSP_2MB_MASK		(2*1024*1024 - 1)
#define PSP_HUGEPAGE_2MB	(2*1024*1024)
#define PSP_HUGEPAGE_NUM_MAX	128
#define TKM_CMD_ID_MIN		0x120
#define TKM_CMD_ID_MAX		0x12f
#define TKM_PSP_CMDID		TKM_CMD_ID_MIN
#define TKM_PSP_CMDID_OFFSET	0x128
#define PSP_VID_MASK            0xff
#define PSP_VID_SHIFT           56
#define PUT_PSP_VID(hpa, vid)   ((__u64)(hpa) | ((__u64)(PSP_VID_MASK & vid) << PSP_VID_SHIFT))
#define GET_PSP_VID(hpa)        ((__u16)((__u64)(hpa) >> PSP_VID_SHIFT) & PSP_VID_MASK)
#define CLEAR_PSP_VID(hpa)      ((__u64)(hpa) & ~((__u64)PSP_VID_MASK << PSP_VID_SHIFT))

struct vpsp_context {
	u32 vid;
	pid_t pid;
	u64 gpa_start;
	u64 gpa_end;

	// `vm_is_bound` indicates whether the binding operation has been performed
	u32 vm_is_bound;
	u32 vm_handle;	// only for csv
};

#ifdef CONFIG_CRYPTO_DEV_SP_PSP

int psp_do_cmd(int cmd, void *data, int *psp_ret);

int csv_ring_buffer_queue_init(void);
int csv_ring_buffer_queue_free(void);
int csv_fill_cmd_queue(int prio, int cmd, void *data, uint16_t flags);
int csv_check_stat_queue_status(int *psp_ret);

/**
 * csv_issue_ringbuf_cmds_external_user - issue CSV commands into a ring
 * buffer.
 */
int csv_issue_ringbuf_cmds_external_user(struct file *filep, int *psp_ret);

int vpsp_try_get_result(uint8_t prio, uint32_t index,
			phys_addr_t phy_addr, struct vpsp_ret *psp_ret);

int vpsp_try_do_cmd(int cmd, phys_addr_t phy_addr, struct vpsp_ret *psp_ret);

int vpsp_get_context(struct vpsp_context **ctx, pid_t pid);

int vpsp_get_default_vid_permission(void);

int kvm_pv_psp_copy_forward_op(struct kvm_vpsp *vpsp, int cmd, gpa_t data_gpa, gpa_t psp_ret_gpa);

int kvm_pv_psp_forward_op(struct kvm_vpsp *vpsp, uint32_t cmd,
				gpa_t data_gpa, uint32_t psp_ret);

/**
 * csv_get_extension_info - collect extension set of the firmware
 *
 * @buf: The buffer to save extension set
 * @size: The size of the @buf
 *
 * Returns:
 * 0 if @buf is filled with extension bitflags
 * -%ENODEV if the CSV device is not available
 * -%EINVAL if @buf is NULL or @size is too smaller
 */
int csv_get_extension_info(void *buf, size_t *size);

#else	/* !CONFIG_CRYPTO_DEV_SP_PSP */

static inline int psp_do_cmd(int cmd, void *data, int *psp_ret) { return -ENODEV; }

static inline int csv_ring_buffer_queue_init(void) { return -ENODEV; }
static inline int csv_ring_buffer_queue_free(void) { return -ENODEV; }
static inline
int csv_fill_cmd_queue(int prio, int cmd, void *data, uint16_t flags) { return -ENODEV; }
static inline int csv_check_stat_queue_status(int *psp_ret) { return -ENODEV; }
static inline int
csv_issue_ringbuf_cmds_external_user(struct file *filep, int *psp_ret) { return -ENODEV; }

static inline int
vpsp_try_get_result(uint8_t prio,
		uint32_t index, phys_addr_t phy_addr, struct vpsp_ret *psp_ret) { return -ENODEV; }

static inline int
vpsp_try_do_cmd(int cmd, phys_addr_t phy_addr,
		struct vpsp_ret *psp_ret) { return -ENODEV; }

static inline int
vpsp_get_context(struct vpsp_context **ctx, pid_t pid) { return -ENODEV; }

static inline int
vpsp_get_default_vid_permission(void) { return -ENODEV; }

static inline int
kvm_pv_psp_copy_forward_op(struct kvm_vpsp *vpsp, int cmd, gpa_t data_gpa,
				gpa_t psp_ret_gpa) { return -ENODEV; }

static inline int
kvm_pv_psp_forward_op(struct kvm_vpsp *vpsp, uint32_t cmd,
			gpa_t data_gpa, uint32_t psp_ret) { return -ENODEV; }

static inline int csv_get_extension_info(void *buf, size_t *size) { return -ENODEV; }

#endif	/* CONFIG_CRYPTO_DEV_SP_PSP */

typedef int (*p2c_notifier_t)(uint32_t id, uint64_t data);

#ifdef CONFIG_HYGON_PSP2CPU_CMD

int psp_register_cmd_notifier(uint32_t cmd_id, p2c_notifier_t notifier);
int psp_unregister_cmd_notifier(uint32_t cmd_id, p2c_notifier_t notifier);

#else	/* !CONFIG_HYGON_PSP2CPU_CMD */

int psp_register_cmd_notifier(uint32_t cmd_id, p2c_notifier_t notifier) { return -ENODEV; }
int psp_unregister_cmd_notifier(uint32_t cmd_id, p2c_notifier_t notifier) { return -ENODEV; }

#endif	/* CONFIG_HYGON_PSP2CPU_CMD */

#endif	/* __PSP_HYGON_H__ */
