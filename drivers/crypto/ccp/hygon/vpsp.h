/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HYGON Secure Processor interface
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Mengbiao Xiong <xiongmengbiao@hygon.cn>
 */

#ifndef __CCP_HYGON_VPSP_H__
#define __CCP_HYGON_VPSP_H__


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
 * @resv2: reserved bits
 * @status: indicates the current status of the related command
 */
struct vpsp_ret {
	u32 pret	:	16;
	u32 resv	:	1;
	u32 format	:	1;
	u32 resv2	:	12;
	u32 status	:	2;
};
#define VPSP_RET_SYS_FORMAT    1
#define VPSP_RET_PSP_FORMAT    0

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

struct vpsp_dev_ctx {
	u32 vid;
	pid_t pid;
	u64 gpa_start;
	u64 gpa_end;

	// `vm_is_bound` indicates whether the binding operation has been performed
	u32 vm_is_bound;
	u32 vm_handle;	// only for csv
};

struct vpsp_cmd_ctx {
	void *data;		// copy forward mode only
	uint32_t data_size;	// copy forward mode only
	uint8_t rb_prio;
	uint32_t rb_index;
	uint32_t statval;
	phys_addr_t psp_cmdbuf_paddr;
	refcount_t ref;

	/**
	 * key1 indicates the GPA
	 * to the data passed by the Guest
	 *
	 * key2 indicates the pid of Qemu Process
	 *
	 * Serves as the key for the vpsp_cmd_ctx_table.
	 */
	gpa_t key1;
	pid_t key2;
	struct hlist_node node;
};

enum VPSP_DEV_CTRL_OPCODE {
	VPSP_OP_VID_ADD,
	VPSP_OP_VID_DEL,
	VPSP_OP_SET_DEFAULT_VID_PERMISSION,
	VPSP_OP_GET_DEFAULT_VID_PERMISSION,
	VPSP_OP_SET_GPA,
};

struct vpsp_dev_ctrl {
	unsigned char op;
	/**
	 * To be compatible with old user mode,
	 * struct vpsp_dev_ctrl must be kept at 132 bytes.
	 */
	unsigned char resv[3];
	union {
		unsigned int vid;
		// Set or check the permissions for the default VID
		unsigned int def_vid_perm;
		struct {
			u64 gpa_start;
			u64 gpa_end;
		} gpa;
		unsigned char reserved[128];
	} __packed data;
};

/* defination of variabled used by virtual psp */
enum VPSP_RB_CHECK_STATUS {
	RB_NOT_CHECK = 0,
	RB_CHECKING,
	RB_CHECKED,
	RB_CHECK_MAX
};
#define VPSP_RB_IS_SUPPORTED(buildid)	(buildid >= 1913)
#define VPSP_CMD_STATUS_RUNNING		0xffff

extern struct csv_ringbuffer_queue vpsp_ring_buffer[CSV_COMMAND_PRIORITY_NUM];
extern struct hygon_psp_hooks_table hygon_psp_hooks;
extern bool vpsp_in_ringbuffer_mode;
extern struct kmem_cache *vpsp_cmd_ctx_slab;

void vpsp_worker_handler(struct work_struct *unused);
int vpsp_try_get_result(struct vpsp_cmd_ctx *cmd_ctx, struct vpsp_ret *psp_ret);
int vpsp_try_do_cmd(int cmd, phys_addr_t phy_addr,
		struct vpsp_cmd_ctx *cmd_ctx, struct vpsp_ret *psp_ret);
void vpsp_cmd_ctx_obj_get(struct vpsp_cmd_ctx *cmd_ctx);

void vpsp_cmd_ctx_obj_put(struct vpsp_cmd_ctx *cmd_ctx, bool force);
int vpsp_get_dev_ctx(struct vpsp_dev_ctx **ctx, pid_t pid);
int vpsp_get_default_vid_permission(void);
int do_vpsp_op_ioctl(struct vpsp_dev_ctrl *ctrl);
int vpsp_rb_check_and_cmd_prio_parse(uint8_t *prio,
		struct vpsp_cmd *vcmd);

#endif	/* __CCP_HYGON_VPSP_H__ */
