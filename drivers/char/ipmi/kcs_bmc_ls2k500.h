/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KCS_BMC_LS2K500__
#define __KCS_BMC_LS2K500__ 1
#include <linux/list.h>
#include "btlock.h"
#define IPMI_KCS_OBF_BIT        0
#define IPMI_KCS_IBF_BIT        1
#define IPMI_KCS_SMS_ATN_BIT    2
#define IPMI_KCS_CD_BIT         3

#define IPMI_KCS_OBF_MASK          (1 << IPMI_KCS_OBF_BIT)
#define IPMI_KCS_GET_OBF(d)        (((d) >> IPMI_KCS_OBF_BIT) & 0x1)
#define IPMI_KCS_SET_OBF(d, v)     ((d) = (((d) & ~IPMI_KCS_OBF_MASK) | \
					(((v) & 1) << IPMI_KCS_OBF_BIT)))
#define IPMI_KCS_IBF_MASK          (1 << IPMI_KCS_IBF_BIT)
#define IPMI_KCS_GET_IBF(d)        (((d) >> IPMI_KCS_IBF_BIT) & 0x1)
#define IPMI_KCS_SET_IBF(d, v)     ((d) = (((d) & ~IPMI_KCS_IBF_MASK) | \
					(((v) & 1) << IPMI_KCS_IBF_BIT)))
#define IPMI_KCS_SMS_ATN_MASK      (1 << IPMI_KCS_SMS_ATN_BIT)
#define IPMI_KCS_GET_SMS_ATN(d)    (((d) >> IPMI_KCS_SMS_ATN_BIT) & 0x1)
#define IPMI_KCS_SET_SMS_ATN(d, v) ((d) = (((d) & ~IPMI_KCS_SMS_ATN_MASK) | \
					((v) & 1) << IPMI_KCS_SMS_ATN_BIT))
#define IPMI_KCS_CD_MASK           (1 << IPMI_KCS_CD_BIT)
#define IPMI_KCS_GET_CD(d)         (((d) >> IPMI_KCS_CD_BIT) & 0x1)
#define IPMI_KCS_SET_CD(d, v)      ((d) = (((d) & ~IPMI_KCS_CD_MASK) | \
					(((v) & 1) << IPMI_KCS_CD_BIT)))

#define IPMI_KCS_IDLE_STATE        0
#define IPMI_KCS_READ_STATE        1
#define IPMI_KCS_WRITE_STATE       2
#define IPMI_KCS_ERROR_STATE       3

#define IPMI_KCS_GET_STATE(d)    (((d) >> 6) & 0x3)
#define IPMI_KCS_SET_STATE(d, v) ((d) = ((d) & ~0xc0) | (((v) & 0x3) << 6))

#define IPMI_KCS_ABORT_STATUS_CMD       0x60
#define IPMI_KCS_WRITE_START_CMD        0x61
#define IPMI_KCS_WRITE_END_CMD          0x62
#define IPMI_KCS_READ_CMD               0x68
#define IPMI_KCS_STATUS_NO_ERR          0x00
#define IPMI_KCS_STATUS_ABORTED_ERR     0x01
#define IPMI_KCS_STATUS_BAD_CC_ERR      0x02
#define IPMI_KCS_STATUS_LENGTH_ERR      0x06
#define KCS_STATUS_CMD_DAT      BIT(3)

typedef struct IPMIKCS {
	union btlock lock;
	uint8_t status_reg;
	uint8_t data_out_reg;

	int16_t data_in_reg;
	int16_t cmd_reg;
	int16_t reserved2;

	uint32_t write_req;
	uint32_t write_ack;

	uint32_t reserved3;
	uint32_t reserved4;
} IPMIKCS;

struct loongson_kcs_bmc {
	struct list_head next;
	IPMIKCS *kcs;
	struct kcs_bmc *bmc;
};
#endif
