/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * The Hygon TDM CPU-to-PSP communication driver
 *
 * Copyright (C) 2022 Hygon Info Technologies Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* Change log:
 * Version: 0.7 (fw version 1.4)
 *	1.Adjust the TDM driver to accommodate multiple versions of the kernel.
 * Version: 0.6 (fw version 1.4)
 *	1.remove psp_get_fw_info from hygon_tdm_init, add tdm show device support to ioctl for hag.
 * Version: 0.5 (fw version 1.4)
 *	1.add support for hanging machine when task exception with special attribute.
 * Version: 0.4 (fw version 1.3)
 *	1.add vpcr support.
 *	2.add task create by vaddr.
 * Version: 0.3 (fw version 1.2)
 *	1.add remote authentication support.
 */
#ifndef __TDM_DEV_H__
#define __TDM_DEV_H__

#include <linux/sched.h>
#include <linux/version.h>

#define MIN_VPCR				10
#define MAX_VPCR				16

/*Macro definition for measurement*/
#define TDM_MAX_TASK_BITMAP			16
#define TDM_MAX_NONCE_SIZE			32

#define RANGE_CNT_MAX				0x80
#define MEASURE_TASK_MAX			100
#define AUTHCODE_MAX				16
#define AUTH_TRY_DELAY				1

#define HASH_ALGO_SM3				0
#define HASH_ALGO_SHA1				1
#define HASH_ALGO_SHA256			2
#define HASH_ALGO_SHA384			3
#define HASH_ALGO_SHA512			4

#define SM3_256_DIGEST_SIZE			32
#define SHA1_DIGEST_SIZE			20
#define SHA256_DIGEST_SIZE			32
#define SHA384_DIGEST_SIZE			48
#define SHA512_DIGEST_SIZE			64

#define CONTEXT_CHECK_PID			0x1
#define CONTEXT_CHECK_COMM			0x2
#define CONTEXT_CHECK_MODNAME			0x4
#define TASK_ATTR_NO_UPDATE			0x10000
#define TASK_SUPPORT_VPCR			0x20000
#define TASK_CREATE_VADDR			0x40000
#define TASK_EXCEPTION_CRASH			0x80000

#define MEASURE_UPDATE_ALGO			0x1
#define MEASURE_UPDATE_EXPECTED_MEASUREMENT	0x2

/*Macro definition for tdm certificate*/
#define TDM_MAX_CHIP_ID_LEN			40
#define TDM_CURVE_SM2_ID			0x3
#define TDM_PUBKEY_LEN				32
#define TDM_MAX_USER_ID_LEN			126
#define TDM_SIG_LEN				32
#define TDM_HEADER_AND_PUBKEY_LEN		284

/*Macro definition for tdm report*/
#define TDM_TASK_ALL				0xffffffff
#define TDM_REPORT_SUMMARY			0
#define TDM_REPORT_DETAIL			1

/* CPU to psp command declaration */
enum C2P_CMD_TYPE {
	TDM_TASK_CREATE = 0x0,
	TDM_TASK_VERIFY_AUTH,
	TDM_TASK_QUERY,
	TDM_TASK_DESTROY,
	TDM_TASK_UPDATE,
	TDM_TASK_STOP,
	TDM_TASK_START,
	TDM_FW_VERSION,
	TDM_EXPORT_CERT,
	TDM_GET_REPORT,
	TDM_VPCR_AUDIT,
	TDM_MAX_CMD
};

/* User interaction command declaration */
enum USER_CMD_TYPE {
	USER_EXPORT_CERT = 0x80,
	USER_GET_REPORT,
	USER_VPCR_AUDIT,
	USER_SHOW_DEVICE,
	USER_MAX_CMD
};

/*Public usage id definition for tdm certificate*/
enum _tdm_key_usage_id {
	TDM_INVALID_USAGE_ID =			0x1000,
	TDM_CEK_USAGE_ID =			0x1004,
	TDM_AK_USAGE_ID =			0x2001,
	TDM_MAX_USAGE_ID
};

/*Public status ans type declaration*/
enum TDM_TASK_STATUS {
	DYN_INIT = 0x0,
	DYN_TO_RUN,
	DYN_RUN,
	DYN_TO_STOP,
	DYN_STOP
};

enum TDM_MEASURE_STATUS {
	MER_NORMAL = 0x0,
	MER_ERR
};

enum DYN_ERROR_TYPE {
	DYN_NORMAL = 0x0,
	DYN_NOT_EXIST,
	DYN_AUTH_FAIL,
	DYN_STATUS_NOT_SUIT,
	DYN_BEYOND_MAX,
	DYN_DA_PERIOD,
	DYN_NULL_POINTER,
	DYN_ERR_API,
	DYN_EEXIST,
	DYN_ERR_MEM,
	DYN_ERR_AUTH_LEN,
	DYN_ERR_KEY_ID,
	DYN_NO_ALLOW_UPDATE,
	DYN_ERR_HASH_ALGO,
	DYN_ERR_REPORT_TYPE,
	DYN_ERR_SIZE_SMALL,
	DYN_ERR_ADDR_MAPPING,
	DYN_ERR_PCR_NUM,
	DYN_ERR_ORIG_TPM_PCR,
	DYN_MAX_ERR_TYPE
};

/*Data structure declaration for measurement*/
struct addr_info {
	uint64_t addr_start;
	uint64_t length;
} __packed;

struct addr_range_info {
	uint32_t count;
	struct addr_info addr[];
} __packed;

struct measure_data {
	uint32_t hash_algo;
	uint8_t expected_measurement[32];
	uint32_t period_ms;
	uint32_t pcr;
} __packed;

struct authcode_2b {
	uint16_t len;
	uint8_t val[];
} __packed;

struct measure_status {
	uint8_t status;
	uint8_t error;
	uint64_t count;
} __packed;

struct measure_update_data {
	uint32_t update_flag;
	uint32_t algo;
	uint8_t expected_measurement[32];
} __packed;

struct da_status {
	uint64_t err_time;
	uint16_t interval_time;
	uint16_t err_cnt;
} __packed;

struct tdm_version {
	uint8_t api_major;
	uint8_t api_minor;
	uint32_t buildId;
	uint32_t task_max;
	uint32_t range_max_per_task;
} __packed;

struct task_selection_2b {
	uint16_t len;
	uint8_t bitmap[];
};

struct data_2b {
	uint16_t len;
	uint8_t val[];
};

/*Data structure declaration for vpcr*/
struct pcr_select {
	uint16_t hash;
	uint32_t pcr;
} __packed;

union tpmu_ha {
	uint8_t sha1[SHA1_DIGEST_SIZE];
	uint8_t sha256[SHA256_DIGEST_SIZE];
	uint8_t sha384[SHA384_DIGEST_SIZE];
	uint8_t sha512[SHA512_DIGEST_SIZE];
	uint8_t sm3_256[SM3_256_DIGEST_SIZE];
};

struct tpm2b_digest {
	uint16_t size;
	uint8_t buffer[sizeof(union tpmu_ha)];
} __packed;

struct tdm_task_data {
	uint32_t task_id;
	uint8_t hash[32];
} __packed;

struct tdm_pcr_value_2b {
	uint32_t task_nums;
	struct tdm_task_data task_data[];
} __packed;

/*Data structure declaration for tdm certificate*/
struct _tdm_ecc_pubkey {
	uint32_t curve_id;
	uint8_t pubkey_qx[TDM_PUBKEY_LEN];
	uint8_t pubkey_qy[TDM_PUBKEY_LEN];
	uint16_t user_id_len;
	uint8_t user_id[TDM_MAX_USER_ID_LEN];
} __packed;

struct _tdm_ecc_signature {
	uint8_t sig_r[TDM_SIG_LEN];
	uint8_t sig_s[TDM_SIG_LEN];
} __packed;

/*
 ************************ Hygon TDM Certificate - ECC256***************************
 *|00h |31:0     |VERSION          |Certificate version. 0.<major>.<minor>.<fix>     |
 *|04h |7:0      |-                |Reserved. Set to zero                            |
 *|06h |7:0      |CHIP_ID_LEN      |                                                 |
 *|08h |319:0    |CHIP_ID          |Unique ID of every chip.                         |
 *|30h |31:0     |KEY_USAGE_ID     |Usage id of the key.                             |
 *|34h |63:0     |-                |Reserved. Set to zero.                           |
 *|3Ch |31:0     |CURVE_ID         |ECC curve id                                     |
 *|40h |255:0    |Qx               |Public key Qx                                    |
 *|60h |255:0    |Qy               |Public key Qy                                    |
 *|80h |7:0      |USER_ID_LEN      |GM user id len                                   |
 *|82h |1007:0   |USER_ID          |GM user id                                       |
 *|100h|223:0    |-                |Reserved. Set to zero.                           |
 *|11Ch|31:0     |SIG1_KEY_USAGE_ID|Key type for sig1.                               |
 *|120h|255:0    |SIG1_R           |Signature R of key1.                             |
 *|140h|255:0    |SIG1_S           |Signature S of key1.                             |
 *|160h|223:0    |-                |Reserved. Set to zero                            |
 *|17Ch|31:0     |SIG2_KEY_USAGE_ID|Key type for sig2.                               |
 *|180h|255:0    |SIG2_R           |Signature R of key2.                             |
 *|1A0h|255:0    |SIG2_S           |Signature S of key2.                             |
 *************************************************************************************
 */
struct tdm_cert {
	uint32_t version;
	uint8_t reserved_0[2];
	uint16_t chip_id_len;
	uint8_t chip_id[TDM_MAX_CHIP_ID_LEN];
	uint32_t key_usage_id;
	uint8_t reserved_1[8];
	struct _tdm_ecc_pubkey ecc_pubkey;
	uint8_t reserved_2[28];
	uint32_t sig1_key_usage_id;
	struct _tdm_ecc_signature ecc_sig1;
	uint8_t reserved_3[28];
	uint32_t sig2_key_usage_id;
	struct _tdm_ecc_signature ecc_sig2;
} __packed;

/*Data structure declaration for tdm measurement report*/
/*
 ******************** Hygon TDM Report for Single Task - ECC256***********************
 *|+(00h) |31:0     |TASK_ID                |Measured task ID                          |
 *|+(04h) |31:0     |PERIOD_MS              |Meaured period time for the related task  |
 *|+(08h) |63:0     |MEAURED_COUNT          |Meaured count for the related task        |
 *|+(10h) |31:0     |LAST_MEASURE_ELAPSED_MS|Meaured time for last mesurement.         |
 *|+(14h) |95:0     |-                      |Reserved. Set to zero                     |
 *|+(20h) |255:0    |MEASURED_HASH          |Mesured hash for the related task.        |
 *************************************************************************************
 */
struct tdm_detail_task_status {
	uint32_t task_id;
	uint32_t period_ms;
	uint64_t measured_count;
	uint32_t last_measure_elapsed_ms;
	uint8_t reserved[12];
	uint8_t measured_hash[32];
} __packed;

/*
 ************************ Hygon TDM Report - ECC256***************************
 *|00h |31:0     |VERSION            |Certificate version. 0.<major>.<minor>.<fix>     |
 *|04h |31:0     |FW_VERSION         |Firmware verfion,BUILD_ID                        |
 *|08h |7:0      |REPORT_TYPE        |Summary report:0, Detailed report:1              |
 *|09h |39:0     |-                  |Reserved. Set to zero.                           |
 *|0Eh |15:0     |TASK_NUMS          |ALL task numbers.                                |
 *|10h |127:0    |TASK_BITMAP        |ALL task bitmap.                                 |
 *|20h |127:0    |TASK_ERROR_BITMAP  |Bitmap for error tasks                           |
 *|30h |127:0    |TASK_RUNNING_BITMAP|Bitmap for runnint tasks                         |
 *|40h |239:0    |-                  |Reserved. Set to zero.                           |
 *|5Eh |15:0     |USER_DATA_LEN      |User supplied data length.                       |
 *|60h |255:0    |USER_DATA          |User supplied data.                              |
 *|80h |255:0    |AGGREGATE_HASH     |Aggregate hash for tasks                         |
 *************************************************************************************
 */
struct tdm_report {
	uint32_t version;
	uint32_t fw_version;
	uint8_t report_type;
	uint8_t reserved_0[5];
	uint16_t task_nums;
	uint8_t task_bitmap[TDM_MAX_TASK_BITMAP];
	uint8_t task_error_bitmap[TDM_MAX_TASK_BITMAP];
	uint8_t task_running_bitmap[TDM_MAX_TASK_BITMAP];
	uint8_t reserved_1[30];
	uint16_t user_supplied_data_len;
	uint8_t user_supplied_data[TDM_MAX_NONCE_SIZE];
	uint8_t aggregate_hash[32];
	struct tdm_detail_task_status detailed_task_status[];
} __packed;

/*
 ************************ Hygon TDM Report Signature - ECC256*************************
 *|A0h |223:0     |-                |Reserved. Set to zero                            |
 *|BCh |31:0      |SIG_KEY_USAGE_ID |Key type for sig.                                |
 *|C0h |255:0     |SIG_R            |Signature R of key.                              |
 *|E0h |255:0     |SIG_S            |Signature S of key.                              |
 *************************************************************************************
 */
struct tdm_report_sig {
	uint8_t reserved[28];
	uint32_t sig_key_usage_id;
	uint8_t sig_r[TDM_SIG_LEN];
	uint8_t sig_s[TDM_SIG_LEN];
} __packed;

/*Data structure declaration for tdm command/response interface*/
/*
 * The following commands use this structure:
 * psp_register_measure_exception_handler
 * psp_destroy_measure_task
 * psp_update_measure_task
 * psp_startstop_measure_task
 */
struct tdm_common_cmd {
	uint32_t cmd_type;
	uint32_t task_id;
	uint16_t code_len;
	uint8_t code_val[AUTHCODE_MAX];
	uint8_t context_hash[32];
} __packed;

/*TASK_CREATE*/
struct tdm_create_cmd {
	uint32_t cmd_type;
	uint32_t cmd_ctx_flag;
	struct measure_data m_data;
	uint16_t authcode_len;
	uint8_t context_hash[32];
	struct addr_range_info range_info;
} __packed;

struct tdm_create_resp {
	uint32_t task_id;
	uint16_t authcode_len;
	uint8_t authcode_val[AUTHCODE_MAX];
} __packed;

/*TASK_VERIFY_AUTH*/
struct tdm_register_cmd {
	struct tdm_common_cmd cmd;
} __packed;

/*TASK_QUERY*/
struct tdm_query_cmd {
	uint32_t cmd_type;
	uint32_t task_id;
} __packed;

struct tdm_query_resp {
	struct measure_status m_status;
} __packed;

/*TASK_DESTROY*/
struct tdm_destroy_cmd {
	struct tdm_common_cmd cmd;
} __packed;

/*TASK_UPDATE*/
struct tdm_update_cmd {
	struct tdm_common_cmd cmd;
	struct measure_update_data update_data;
} __packed;

/*TASK_STOP,TASK_START*/
struct tdm_startstop_cmd {
	struct tdm_common_cmd cmd;
} __packed;

struct tdm_startstop_resp {
	struct measure_status m_status;
} __packed;

/*TDM_VERSION*/
struct tdm_fw_cmd {
	uint32_t cmd_type;
} __packed;

struct tdm_fw_resp {
	struct tdm_version version;
} __packed;

/*TDM_EXPORT_CERT*/
struct tdm_export_cert_cmd {
	uint32_t cmd_type;
	uint32_t key_usage_id;
} __packed;

struct tdm_export_cert_resp {
	struct tdm_cert cert;
} __packed;

/*TDM_GET_REPORT*/
struct tdm_get_report_cmd {
	uint32_t cmd_type;
	uint32_t task_id;
	uint16_t selection_len;
	uint8_t selection_bitmap[TDM_MAX_TASK_BITMAP];
	uint16_t user_data_len;
	uint8_t user_data_val[TDM_MAX_NONCE_SIZE];
	uint8_t report_type;
	uint32_t key_usage_id;
} __packed;

/* Resopnse:
 * struct tdm_report measure_report;
 * struct tdm_report_sig measure_report_sig;
 */

struct tdm_user_report_cmd {
	struct tdm_get_report_cmd report_cmd;
	uint32_t needed_length;
} __packed;

/*TDM_VPCR_AUDIT*/
struct tdm_get_vpcr_cmd {
	uint32_t cmd_type;
	struct pcr_select pcr;
} __packed;

struct tdm_get_vpcr_resp {
	uint32_t pcr;
	struct tpm2b_digest digest;
	struct tdm_pcr_value_2b pcr_values;
} __packed;

struct tdm_show_device {
	struct tdm_version version;
} __packed;

/*Public api definition for tdm*/
typedef int (*measure_exception_handler_t)(uint32_t task_id);

int psp_check_tdm_support(void);
int psp_get_fw_info(struct tdm_version *version);
int psp_create_measure_task(struct addr_range_info *range, struct measure_data *data,
		uint32_t flag, struct authcode_2b *code);
int psp_query_measure_status(uint32_t task_id, struct measure_status *status);
int psp_register_measure_exception_handler(uint32_t task_id, struct authcode_2b *code,
		measure_exception_handler_t handler);
int psp_destroy_measure_task(uint32_t task_id, struct authcode_2b *code);
int psp_update_measure_task(uint32_t task_id, struct authcode_2b *code,
		struct measure_update_data *data);
int psp_startstop_measure_task(uint32_t task_id, struct authcode_2b *code, bool start);
int tdm_export_cert(uint32_t key_usage_id, struct tdm_cert *cert);
int tdm_get_report(uint32_t task_id, struct task_selection_2b *selection,
		struct data_2b *user_supplied_data, uint8_t report_type, uint32_t key_usage_id,
		uint8_t *report_buffer, uint32_t *length);
int tdm_get_vpcr_audit(struct pcr_select pcr, struct tpm2b_digest *digest,
		struct tdm_pcr_value_2b *pcr_values);

int tdm_dev_init(void);
int tdm_dev_destroy(void);
#endif /* __TDM_DEV_H__*/
