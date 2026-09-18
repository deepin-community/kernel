#ifndef DINGHAI_IFC_H
#define DINGHAI_IFC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>

struct dh_ifc_cmd_hca_cap_bits {

};
union dh_ifc_hca_cap_union_bits{

};

struct dh_ifc_mbox_in_bits {
    uint8_t         opcode[0x10];
    uint8_t         uid[0x10];

    uint8_t         reserved_at_20[0x10];
    uint8_t         op_mod[0x10];

    uint8_t         reserved_at_40[0x40];
};

struct dh_ifc_mbox_out_bits {
    uint8_t         status[0x8];
    uint8_t         reserved_at_8[0x18];

    uint8_t         syndrome[0x20];

    uint8_t         reserved_at_40[0x40];
};

struct dh_ifc_query_hca_cap_out_bits {
    uint8_t         status[0x8];
    uint8_t         reserved_at_8[0x18];

    uint8_t         syndrome[0x20];

    uint8_t         reserved_at_40[0x40];

    union dh_ifc_hca_cap_union_bits capability;
};

#ifdef __cplusplus
}
#endif

#endif