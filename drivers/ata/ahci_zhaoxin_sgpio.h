/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ACHI_ZHAOXIN_SGPIO_H
#define _ACHI_ZHAOXIN_SGPIO_H

#define SGPIO_OFFSET 0x580

#define SGPIO_MESSAGE_HEAD 0x3000000

#define ACTIVITY_DISABLE 0x0
#define ACTIVITY_ENABLE 0x1
#define ACTIVITY_GA_FON 0x2
#define ACTIVITY_GA_FOFF 0x3
#define ACTIVITY_BRIEF_EN_EOF 0x4
#define ACTIVITY_BRIEF_EN_SOF 0x5
#define ACTIVITY_GB_FON 0x6
#define ACTIVITY_GB_FOFF 0x7
#define ACTIVITY_GC_FON 0x8
#define ACTIVITY_GC_FOFF 0x9
#define LOCATE_ERROR_DISABLE 0x0
#define LOCATE_ERROR_ENABLE 0x1
#define LOCATE_ERROR_GA_FON 0x2
#define LOCATE_ERROR_GA_FOFF 0x3
#define LOCATE_ERROR_GB_FON 0x4
#define LOCATE_ERROR_GB_FOFF 0x5
#define LOCATE_ERROR_GC_FON 0x6
#define LOCATE_ERROR_GC_FOFF 0x7

#define GP_OFF 0x10
#define GP_ON 0x11

#define to_sgpio_attr(x) container_of(x, struct sgpio_zhaoxin_sysfs_attr, attr)
#define to_sgpio_obj(x) container_of(x, struct sgpio_zhaoxin, kobj)
#define MAX_TEST_RESULT_LEN (PAGE_SIZE - sizeof(struct sgpio_zhaoxin) - 8)

//SGPIO module parameter: 0-off, 1-LED, 2-SGPIO, 3-SGPIO_GP
enum ahci_em_msg_modes {
	AHCI_EM_MSG_OFF = 0,
	AHCI_EM_MSG_LED_MODE,
	AHCI_EM_MSG_SGPIO_MODE,
	AHCI_EM_MSG_SGPIO_GP_MODE,
	AHCI_EM_MSG_NULL,
};

enum SGPIO_INDICATOR {
	SGPIO_ACTIVITY,
	SGPIO_LOCATE,
	SGPIO_ERROR
};

enum SGPIO_CFG1 {
	STRETCH_ACTIVITY_OFF,
	STRETCH_ACTIVITY_ON,
	FORCE_ACTIVITY_OFF,
	MAXIMUM_ACTIVITY_ON,
	BLINK_GENERATIOR_RATE_B,
	BLINK_GENERATIOR_RATE_A,
	BLINK_GENERATIOR_RATE_C
};

union SGPIO_CFG_0 {
	struct {
		u32 reserved0 :8;
		u32 version :4;
		u32 reserved1 :4;
		u32 gp_register_count :4;
		u32 cfg_register_count :3;
		u32 enable :1;
		u32 supported_drive_count :8;
	};
	u32 sgpio_cfg_0;
};

union SGPIO_CFG_1 {
	struct {
		u32 reserved0 :4;
		u32 blink_gen_c :4;
		u32 blink_gen_a :4;
		u32 blink_gen_b :4;
		u32 max_act_on :4;
		u32 force_act_off :4;
		u32 stretch_act_on :4;
		u32 stretch_act_off :4;
	};
	u32 sgpio_cfg_1;
};

union SGPIO_RX {
	struct {
		u32 drive_3_input :3;
		u32 reserved3 :5;
		u32 drive_2_input :3;
		u32 reserved2 :5;
		u32 drive_1_input :3;
		u32 reserved1 :5;
		u32 drive_0_input :3;
		u32 reserved0 :5;
	};
	u32 sgpio_rx;
};

union SGPIO_RX_GP_CFG {
	struct {
		u32 reserved0 :16;
		u32 count :8;
		u32 reserved1 :8;
	};
	u32 sgpio_rx_gp_cfg;
};
union SGPIO_RX_GP {
	struct {
		u32 reserved0 :16;
		u32 D22 :1;
		u32 D30 :1;
		u32 D31 :1;
		u32 D32 :1;
		u32 reserved1:4;
		u32 D00 :1;
		u32 D01 :1;
		u32 D02 :1;
		u32 D10 :1;
		u32 D11 :1;
		u32 D12 :1;
		u32 D20 :1;
		u32 D21 :1;
	};
	u32 sgpio_rx_gp;
};

union SGPIO_TX_0 {
	struct {
		u32 drive_1_error :3;
		u32 drive_1_locate :3;
		u32 drive_1_active :4;
		u32 reserved1 :6;
		u32 drive_0_error :3;
		u32 drive_0_locate :3;
		u32 drive_0_active :4;
		u32 reserved0 :6;
	};
	u32 sgpio_tx_0;
};

union SGPIO_TX_1 {
	struct {
		u32 drive_3_error :3;
		u32 drive_3_locate :3;
		u32 drive_3_active :4;
		u32 reserved3 :6;
		u32 drive_2_error :3;
		u32 drive_2_locate :3;
		u32 drive_2_active :4;
		u32 reserved2 :6;
	};
	u32 sgpio_tx_1;
};

union SGPIO_TX_GP_CFG {
	struct {
		u32 reserved0 :16;
		u32 count :8;
		u32 sload :4;
		u32 reserved1 :4;
	};
	u32 sgpio_tx_gp_cfg;
};

union SGPIO_TX_GP {
	struct {
		u32 reserved0 :16;
		u32 D22 :1;
		u32 D30 :1;
		u32 D31 :1;
		u32 D32 :1;
		u32 reserved1:4;
		u32 D00 :1;
		u32 D01 :1;
		u32 D02 :1;
		u32 D10 :1;
		u32 D11 :1;
		u32 D12 :1;
		u32 D20 :1;
		u32 D21 :1;
	};
	u32 sgpio_tx_gp;
};

struct AHCI_SGPIO_REG {
	union SGPIO_CFG_0 cfg_0;
	union SGPIO_CFG_1 cfg_1;
	union SGPIO_RX receive_reg;
	union SGPIO_RX_GP_CFG gp_receive_cfg;
	union SGPIO_RX_GP gp_receive_reg;
	union SGPIO_TX_0 transmit_0;
	union SGPIO_TX_1 transmit_1;
	union SGPIO_TX_GP_CFG gp_transmit_cfg;
	union SGPIO_TX_GP gp_transmit_reg;
};

struct sgpio_zhaoxin {
	struct kobject kobj;
	struct list_head list;
	unsigned int kobj_valid;
	unsigned int index;
	u32 em_loc; /* enclosure management location */
	u32 em_buf_sz; /* EM buffer size in byte */
	u32 em_msg_type; /* EM message type */
	void __iomem *mmio;
	spinlock_t wr_lock; /* protects sgpio register */
	struct  AHCI_SGPIO_REG sgpio_reg; /* saved sgpio register */
};

struct sgpio_zhaoxin_sysfs_attr {
	struct attribute attr;
	ssize_t (*show)(struct sgpio_zhaoxin *sgpio_zhaoxin, char *buf);
	ssize_t (*store)(struct sgpio_zhaoxin *sgpio_zhaoxin, const char *buf, size_t count);
};

int get_ahci_em_messages(void);

#endif /* _ACHI_ZHAOXIN_SGPIO_H */
