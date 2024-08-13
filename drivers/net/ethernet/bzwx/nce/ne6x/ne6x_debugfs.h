/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_DEBUGFS_H
#define _NE6X_DEBUGFS_H

struct ne6x_debug_table {
	int table;
	int index;
	int size;
	u32 data[128];
};

#ifdef CONFIG_DEBUG_FS

enum fru_product_part {
	MANUFACTURER_NAME = 0,
	PRODUCT_NAME,
	PRODUCT_PART_NUMBER, /* pn */
	PRODUCT_VERSION,
	PRODUCT_SERIAL_NUMBER, /* sn */
	PRODUCT_ASSET_TAG,
	PRODUCT_FRU_FILE_ID,
};

enum fru_type {
	INTER_USE_AREA = 0,
	CHASSIS_AREA,
	BOARD_AREA,
	PRODUCT_AREA,
	MUILT_AREA,
};

#define NE6X_DEBUG_CHAR_LEN 1024

#define INFO_ROW                    20
#define INFO_COL                    50

extern char ne6x_driver_name[];

struct ne6x_dbg_cmd_wr {
	char command[NE6X_DEBUG_CHAR_LEN];
	void (*command_proc)(struct ne6x_pf *pf, char *cmd_buf, int count);
};

struct ne6x_debug_info {
	u16 system_id;
	char system_name[INFO_COL];
	char system_speed[INFO_COL];
};

void ne6x_dbg_init(void);
void ne6x_dbg_exit(void);

void ne6x_dbg_pf_init(struct ne6x_pf *pf);
void ne6x_dbg_pf_exit(struct ne6x_pf *pf);
#else /* !CONFIG_DEBUG_FS */

static inline void ne6x_dbg_init(void)
{ }
static inline void ne6x_dbg_exit(void)
{ }
static inline void ne6x_dbg_pf_init(struct ne6x_pf *pf)
{ }
static inline void ne6x_dbg_pf_exit(struct ne6x_pf *pf)
{ }
#endif /* end CONFIG_DEBUG_FS */

#endif
